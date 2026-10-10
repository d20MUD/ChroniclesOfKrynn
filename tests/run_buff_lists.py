#!/usr/bin/env python3
"""Exercise production buff commands, persistence and pulse logic with isolated game stubs."""
from pathlib import Path
import os
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
src = root / "src"
other = (src / "act.other.c").read_text()
players = (src / "players.c").read_text()
limits = (src / "limits.c").read_text()
utils = (src / "utils.c").read_text()
macros = "\n".join(line for line in (src / "utils.h").read_text().splitlines()
                   if line.startswith(("#define GET_BUFF", "#define GET_CURRENT_BUFF_SLOT", "#define IS_BUFFING")))
constants = "\n".join(line for line in (src / "structs.h").read_text().splitlines()
                      if line.startswith(("#define MAX_BUFFS ", "#define MAX_BUFF_LISTS ")))
commands = other[other.index("#define NOBUFF_MSG"):other.index("ACMDU(do_devote)")]
loaders = players[players.index("static void load_buffs(FILE *fl, struct char_data *ch)\n{"):
                  players.index("static void load_scrolls(FILE *fl, struct char_data *ch)\n{")]
save = players[players.index("  // Save Buffs"):players.index("  // Save Bags")]
a = limits.index("static bool buff_target_argument(")
b = limits.index("\n}", limits.index("void self_buffing(void)", a)) + 2
pulse = limits[a:b]
a = utils.index("void char_from_buff_targets(")
cleanup = utils[a:utils.index("\n}", a) + 2]

stubs = r'''
#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdarg.h>
#define MAX_INPUT_LENGTH 1024
#define MAX_STRING_LENGTH 8192
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define TRUE true
#define ACMD(name) void name(struct char_data *ch, const char *argument, int cmd, int subcmd)
#define IS_NPC(ch) ((ch)->npc)
#define GET_POS(ch) ((ch)->pos)
#define IN_ROOM(ch) ((ch)->room)
#define GET_NAME(ch) ((ch)->name)
#define POS_FIGHTING 7
#define PRF_AUGMENT_BUFFS 1
#define PRF_FLAGGED(ch,f) ((ch)->augment)
#define IS_SET(a,b) ((a)&(b))
#define TAR_NOT_SELF 1
#define GET_FIGHT_TO_THE_DEATH_COOLDOWN(ch) ((ch)->cooldown)
#define IS_AFFECTED(ch,f) 0
#define AFF_TIME_STOPPED 1
#define AFF_RAPID_BUFF 2
#define PERK_CLERIC_BATTLE_BLESSING 1
#define SPELL_MINOR_RAPID_BUFF 100
#define SPELL_RAPID_BUFF 101
#define SPELL_GREATER_RAPID_BUFF 102
#define SCMD_CAST_SPELL 1
#define SCMD_CAST_PSIONIC 2
#define TO_CHAR 1
#define TO_VICT 2
struct char_data;
struct saved { int buff_abilities[MAX_BUFF_LISTS][MAX_BUFFS][2]; };
struct specials {
  struct saved saved;
  int buff_list, buff_slot, buff_timer;
  bool is_buffing;
  struct char_data *buff_target;
};
struct char_data {
  struct specials *player_specials;
  int room, pos, cooldown;
  bool npc, augment;
  const char *name;
  struct char_data *next;
};
struct descriptor_data { struct char_data *character; struct descriptor_data *next; };
static struct descriptor_data *descriptor_list;
static struct char_data *character_list, *buddy, *other_buddy;
static struct { const char *name; int violent, targets, time, ritual_spell; } spell_info[4] = {
  {"reserved", 0, 0, 0, 0}, {"armor", 0, 0, 1, 0},
  {"bless", 0, 0, 1, 0}, {"mind shield", 0, 0, 1, 0}
};
static int saves, casts, manifests;
static char output[8192], cast_argument[8192];
static size_t strlcpy(char *dest, const char *s, size_t n) {
  size_t len = strlen(s);
  if (n) { size_t copy = len < n-1 ? len : n-1; memcpy(dest,s,copy); dest[copy]=0; }
  return len;
}
static void skip_spaces_c(const char **p) { while (isspace((unsigned char)**p)) ++*p; }
static const char *one_argument(const char *p, char *out, size_t n) {
  skip_spaces_c(&p); size_t i=0;
  while (*p && !isspace((unsigned char)*p)) { if (i+1<n) out[i++]=*p; p++; }
  out[i]=0; skip_spaces_c(&p); return p;
}
static bool is_abbrev(const char *s, const char *word) { return *s && !strncasecmp(s,word,strlen(s)); }
static int str_cmp(const char *s, const char *t) { return strcasecmp(s,t); }
static int find_skill_num(const char *s) {
  for (int i=1;i<=3;i++) if (!strcasecmp(s,spell_info[i].name)) return i;
  return -1;
}
static int is_spell_or_power(int n) { return n>=1 && n<=3 ? (n==3?1:2) : 0; }
static void send_to_char(struct char_data *ch, const char *fmt, ...) {
  (void)ch; va_list ap; va_start(ap,fmt); vsnprintf(output,sizeof(output),fmt,ap); va_end(ap);
}
static void act(const char *s, bool b, struct char_data *ch, int obj, struct char_data *t, int mode) {
  (void)b;(void)ch;(void)obj;(void)t;(void)mode; strlcpy(output,s,sizeof(output));
}
static struct char_data *get_char_room_vis(struct char_data *ch, char *name, int *number) {
  if (!strcasecmp(name,"self") || !strcasecmp(name,"me")) return ch;
  int ordinal = number ? *number : 1;
  if (!number && isdigit((unsigned char)*name)) {
    ordinal = atoi(name); name = strchr(name,'.'); if (!name) return NULL; name++;
  }
  if (strcasecmp(name,"buddy")) return NULL;
  if (ordinal == 1 && buddy && buddy->room == ch->room) return buddy;
  if (ordinal == 2 && other_buddy && other_buddy->room == ch->room) return other_buddy;
  return NULL;
}
static void save_char(struct char_data *ch, int n) { (void)ch;(void)n;saves++; }
static void affect_from_char(struct char_data *ch, int n) { (void)ch;(void)n; }
static int get_line(FILE *fl, char *s) { return fgets(s,MAX_INPUT_LENGTH+1,fl)!=NULL; }
static int has_perk(struct char_data *ch, int n) { (void)ch;(void)n;return 0; }
static int max_augment_psp_allowed(struct char_data *ch, int n) { (void)ch;(void)n;return 7; }
static void do_gen_cast(struct char_data *ch, const char *s, int cmd, int subcmd) {
  (void)ch;(void)cmd;(void)subcmd;casts++;strlcpy(cast_argument,s,sizeof(cast_argument));
}
static void do_manifest(struct char_data *ch, const char *s, int cmd, int subcmd) {
  (void)ch;(void)cmd;(void)subcmd;manifests++;strlcpy(cast_argument,s,sizeof(cast_argument));
}
'''
checks = r'''
#define BUFFER_WRITE(...) fprintf(fl, __VA_ARGS__)
static void save_buffs(FILE *fl, struct char_data *ch) {
  int i,j;
SAVE_BLOCK
}
static FILE *records(const char *s) {
  FILE *fl=tmpfile(); assert(fl); fputs(s,fl); rewind(fl); return fl;
}
static void command(struct char_data *ch, const char *s) { do_buff(ch,s,0,0); }
int main(void) {
  struct specials data={0}, loaded={0};
  struct char_data ch={.player_specials=&data,.room=1,.pos=7,.name="caster"};
  struct char_data target={.room=1,.name="buddy"}, target2={.room=1,.name="buddy"};
  struct char_data restored={.player_specials=&loaded};
  struct descriptor_data desc={&ch,NULL}; descriptor_list=&desc;
  buddy=&target; other_buddy=&target2;
  command(&ch,"add armor"); assert(GET_BUFF(&ch,0,0)==1);
  command(&ch,"add 2 bless"); assert(GET_BUFF_IN_LIST(&ch,1,0,0)==2);
  command(&ch,"add 10 mind shield"); assert(GET_BUFF_IN_LIST(&ch,9,0,0)==3);
  int saved=saves;
  command(&ch,"add 0 armor"); command(&ch,"add 11 armor");
  command(&ch,"add -1 armor"); command(&ch,"add 2oops armor");
  assert(saves==saved);
  command(&ch,"add bless"); command(&ch,"remove armor"); command(&ch,"add bless");
  assert(GET_BUFF(&ch,0,0)==0 && GET_BUFF(&ch,1,0)==2); /* Duplicate after a hole. */
  command(&ch,"list 10"); assert(strstr(output,"mind shield"));
  FILE *fl=tmpfile(); assert(fl); save_buffs(fl,&ch); rewind(fl);
  char line[MAX_INPUT_LENGTH+1]; assert(get_line(fl,line) && !strcmp(line,"Buff:\n"));
  load_buffs(fl,&restored);
  assert(get_line(fl,line) && !strcmp(line,"BfLs:\n")); load_buff_lists(fl,&restored); fclose(fl);
  assert(!memcmp(&data.saved,&loaded.saved,sizeof(data.saved)));
  fl=records("0 1 4\n40 2 0\n-2 2 0\n1 -7 0\n-1 -1 -1\nnext\n");
  load_buffs(fl,&restored); assert(GET_BUFF(&restored,0,0)==1 && GET_BUFF(&restored,0,1)==4);
  assert(get_line(fl,line) && !strcmp(line,"next\n")); fclose(fl);
  fl=records("10 0 1 0\n9 40 1 0\n9 -1 1 0\n0 0 2 0\n9 39 2 -5\n-1 -1 -1 -1\n");
  load_buff_lists(fl,&restored); fclose(fl);
  assert(GET_BUFF_IN_LIST(&restored,9,39,0)==2 && GET_BUFF_IN_LIST(&restored,9,39,1)==0);
  for (int i=0;i<MAX_BUFFS;i++) GET_BUFF_IN_LIST(&ch,2,i,0)=1;
  command(&ch,"add 3 bless"); assert(strstr(output,"full"));
  command(&ch,"perform 2 missing"); assert(!IS_BUFFING(&ch));
  command(&ch,"perform 2 buddy"); assert(IS_BUFFING(&ch) && GET_BUFF_LIST(&ch)==1);
  command(&ch,"remove 2 bless"); assert(GET_BUFF_IN_LIST(&ch,1,0,0)==2);
  command(&ch,"perform 10"); assert(GET_BUFF_LIST(&ch)==1);
  self_buffing(); assert(casts==1 && strstr(cast_argument,"'bless' 1.buddy"));
  self_buffing(); assert(!IS_BUFFING(&ch)); /* Exhausted list cannot read past the array. */
  command(&ch,"perform 10 2.buddy"); ch.augment=true;
  self_buffing(); assert(manifests==1 && strstr(cast_argument,"7 'mind shield' 2.buddy"));
  command(&ch,"cancel"); command(&ch,"perform 2.buddy");
  assert(GET_BUFF_LIST(&ch)==0 && GET_BUFF_TARGET(&ch)==&target2);
  target2.room=2; self_buffing(); assert(!IS_BUFFING(&ch) && casts==1);
  command(&ch,"target"); assert(!GET_BUFF_TARGET(&ch));
  command(&ch,"perform 1 self"); self_buffing(); assert(strstr(cast_argument,"'bless' self"));
  command(&ch,"cancel");
  command(&ch,"perform 2 buddy"); character_list=&ch;
  char_from_buff_targets(&target); assert(!IS_BUFFING(&ch) && !GET_BUFF_TARGET(&ch));
  command(&ch,"perform 3 self"); GET_CURRENT_BUFF_SLOT(&ch)=MAX_BUFFS-1;
  GET_BUFF_TIMER(&ch)=1; self_buffing(); self_buffing(); assert(!IS_BUFFING(&ch));
  puts("PASS: ten lists, commands, legacy/new persistence, bounds, spells/powers, exact targets and target loss.");
}
'''.replace("SAVE_BLOCK", save)
with tempfile.TemporaryDirectory(prefix="buff-lists-") as directory:
    source = Path(directory) / "buff_lists.c"
    source.write_text(constants + "\n" + stubs + "\n" + macros + "\n" + commands + loaders + pulse + cleanup + checks)
    binary = Path(directory) / "buff_lists"
    subprocess.run([os.environ.get("CC", "cc"), "-std=c11", "-g", "-Wall", "-Wextra",
                    "-Wno-unused-parameter", "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
                    str(source), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
