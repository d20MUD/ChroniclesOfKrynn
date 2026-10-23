#!/usr/bin/env python3
"""Run production give parsing and character lookup against two same-keyword NPCs."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
def function(file, signature):
    source = (root / 'src' / file).read_text()
    start = source.index(signature + '\n{')
    return source[start:source.index('\n}', start) + 2] + '\n'

code = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#define MAX_INPUT_LENGTH 256
#define MAX_STRING_LENGTH 1024
#define FIND_CHAR_ROOM 0
#define FIND_CHAR_WORLD 1
#define FIND_INDIV 0
#define FIND_ALL 1
#define FIND_ALLDOT 2
#define CONFIG_NOPERSON "No such person."
#define AN(arg) "a"
#define IN_ROOM(ch) 0
#define CAN_SEE(ch,i) 1
#define CAN_INFRA(ch,i) 0
#define CAN_SEE_OBJ(ch,obj) 1
#define ACMDU(name) void name(struct char_data *ch, char *argument, int cmd, int subcmd)
#define str_cmp strcasecmp
#define LOWER(c) tolower((unsigned char)(c))
struct obj_data { char *name; struct obj_data *next_content; };
struct char_data { char *name; int given, gold; struct obj_data *carrying; struct char_data *next_in_room; };
struct { struct char_data *people; } world[1];
static int errors;
void skip_spaces(char **arg) { while (**arg && isspace((unsigned char)**arg)) ++*arg; }
void skip_spaces_c(const char **arg) { while (**arg && isspace((unsigned char)**arg)) ++*arg; }
int fill_word(char *arg) { return !strcmp(arg,"to"); }
size_t strlcpy(char *d, const char *s, size_t n) { size_t len=strlen(s); if(n) snprintf(d,n,"%s",s); return len; }
bool can_blindsense_creature(struct char_data *ch, struct char_data *i) { return false; }
bool identity_matches(struct char_data *ch, struct char_data *i, char *name) { return !strcmp(i->name,name); }
struct char_data *get_player_vis(struct char_data *ch, char *name, int *num, int where) { return NULL; }
struct char_data *get_char_world_vis(struct char_data *ch, char *name, int *num) { abort(); }
void send_to_char(struct char_data *ch, const char *fmt, ...) { errors++; }
int is_number(char *arg) { if (!*arg) return 0; for(;*arg;arg++) if(!isdigit((unsigned char)*arg)) return 0; return 1; }
int find_all_dots(char *arg) { if(!strcmp(arg,"all")) return FIND_ALL; if(!strncmp(arg,"all.",4)) { memmove(arg,arg+4,strlen(arg+4)+1); return FIND_ALLDOT; } return FIND_INDIV; }
int isname(char *name,char *keywords) { return !strcmp(name,keywords); }
struct obj_data *get_obj_in_list_vis(struct char_data *ch,char *name,int *num,struct obj_data *obj) {
  for(;obj;obj=obj->next_content) if(!strcmp(obj->name,name)) return obj; return NULL;
}
bool perform_give(struct char_data *ch,struct char_data *vict,struct obj_data *obj) {
  struct obj_data **link=&ch->carrying;
  while(*link!=obj) link=&(*link)->next_content;
  *link=obj->next_content; obj->next_content=NULL; vict->given++; return true;
}
void perform_give_gold(struct char_data *ch,struct char_data *vict,int amount) { vict->gold+=amount; }
void quest_give(struct char_data *ch,struct char_data *vict) {}
void save_char(struct char_data *ch,int unused) {}
'''
for file, signature in [
    ('helpers.c','char *one_argument_u(char *argument, char *first_arg)'),
    ('helpers.c','const char *one_argument(const char *argument, char *first_arg, size_t n)'),
    ('handler.c','int get_number(char **name)'),
    ('handler.c','struct char_data *get_char_room_vis(struct char_data *ch, char *name, int *number)'),
    ('handler.c','struct char_data *get_char_vis(struct char_data *ch, char *name, int *number, int where)'),
    ('act.item.c','static struct char_data *give_find_vict(struct char_data *ch, char *arg)'),
    ('act.item.c','ACMDU(do_give)')]:
    code += function(file,signature)
code += r'''
static void give(struct char_data *ch, const char *text) {
  char argument[256]; snprintf(argument,sizeof(argument),"%s",text); do_give(ch,argument,0,0);
}
int main(void) {
  struct char_data player={0}, first={.name="healer"}, second={.name="healer"};
  struct obj_data mushroom={.name="mushroom"}, extra={.name="mushroom"};
  world[0].people=&first; first.next_in_room=&second;
  player.carrying=&mushroom;
  give(&player,"mushroom to 2.healer");
  assert(second.given==1 && first.given==0 && !player.carrying);
  for(int i=3;i<=5;i++) {
    char arg[80]; snprintf(arg,sizeof(arg),"mushroom to %d.healer",i);
    player.carrying=&mushroom; int before=errors; give(&player,arg);
    assert(errors==before+1 && player.carrying==&mushroom);
    assert(second.given==1 && first.given==0);
  }
  give(&player,"mushroom healer"); assert(first.given==1);
  player.carrying=&mushroom; give(&player,"mushroom 2.healer"); assert(second.given==2);
  player.carrying=&mushroom; mushroom.next_content=&extra;
  give(&player,"2 mushroom to 2.healer"); assert(second.given==4 && !player.carrying);
  player.carrying=&mushroom; mushroom.next_content=&extra;
  give(&player,"all.mushroom to 2.healer"); assert(second.given==6 && !player.carrying);
  give(&player,"10 gold to 2.healer"); assert(second.gold==10 && first.gold==0);
  give(&player,"10 gold to 3.healer"); assert(second.gold==10 && first.gold==0);
  { char arg[]="2.healer"; assert(get_char_room_vis(&player,arg,NULL)==&second); }
  { char arg[]="3.healer"; assert(!get_char_room_vis(&player,arg,NULL)); }
  puts("Numbered give: single, multiple, all, gold, and out-of-range recipients passed.");
}
'''
with tempfile.TemporaryDirectory() as tmp:
    c = Path(tmp) / 'give.c'
    c.write_text(code)
    exe = Path(tmp) / 'give'
    subprocess.run(['cc','-std=gnu99','-fsanitize=address,undefined',str(c),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
