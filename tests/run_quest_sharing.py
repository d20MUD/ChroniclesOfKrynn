#!/usr/bin/env python3
"""Exercise production quest sharing, eligibility, confirmation, and QEDIT flags."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
quest = (root / 'src/quest.c').read_text()
header = (root / 'src/quest.h').read_text()

def function(source, signature):
    a = source.index(signature)
    return source[a:source.index('\n}', a) + 2] + '\n'

# Keep production quest constants and field accessors in the isolated harness.
constants = '\n'.join(line for line in header.splitlines()
                      if line.startswith(('#define AQ_REPEATABLE ', '#define AQ_NO_SHARE ',
                                          '#define NUM_AQ_FLAGS ', '#define QST_',
                                          '#define SCMD_QUEST_')))
code = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <ctype.h>
#define TRUE true
#define FALSE false
#define NOTHING -1
#define NOWHERE -1
#define NUM_AQ_TYPES 25
#define AQ_DIALOGUE 24
#define MAX_INPUT_LENGTH 512
#define MAX_STRING_LENGTH 49152
#define MAX_CURRENT_QUESTS 4
#define NUM_WEARS 2
#define LVL_IMMORT 50
#define TO_CHAR 0
#define TO_ROOM 1
#define FIND_CHAR_ROOM 0
#define ACMD(name) void name(struct char_data *ch, const char *argument, int cmd, int subcmd)
#define IS_SET(v,b) ((v)&(b))
#define TOGGLE_BIT(v,b) ((v)^=(b))
struct obj_data { int rnum; char *short_description; struct obj_data *next; };
struct aq_data {
  int vnum,type,prereq,prev_quest,dialogue_alternative_quest;
  long flags;
  int value[7], diplomacy_dc,intimidate_dc,bluff_dc;
  char *name,*desc,*info;
};
struct descriptor_data { bool playing; struct aq_data *quest; };
struct char_data {
  long id; char *name; bool npc; int level,room; void *group;
  int quests[4],counter[4],timer[4]; bool completed[4],failed[4];
  struct obj_data *carrying,*eq[2]; struct descriptor_data *desc;
  struct char_data *next;
  long quest_share_from_id; int quest_share_vnum; time_t quest_share_expires;
};
typedef int qst_vnum;
typedef int qst_rnum;
typedef int obj_rnum;
struct aq_data aquest_table[4];
struct obj_data obj_proto[2]={{.rnum=0,.short_description="required token"}};
struct char_data *character_list;
#define IS_NPC(ch) ((ch)->npc)
#define GET_LEVEL(ch) ((ch)->level)
#define GET_NAME(ch) ((ch)->name)
#define GET_IDNUM(ch) ((ch)->id)
#define GET_MOB_VNUM(ch) 123
#define GET_QUEST(ch,i) ((ch)->quests[i])
#define GET_QUEST_COUNTER(ch,i) ((ch)->counter[i])
#define GET_QUEST_TIME(ch,i) ((ch)->timer[i])
#define GET_EQ(ch,i) ((ch)->eq[i])
#define GET_OBJ_RNUM(obj) ((obj)->rnum)
#define GROUP(ch) ((ch)->group)
#define IN_ROOM(ch) ((ch)->room)
#define IS_PLAYING(d) ((d)->playing)
#define PERS(ch,viewer) ((ch)->name)
#define QUEST_SHARE_TIMEOUT 120
''' + constants + r'''
static char output[65536];
static int saves;
void send_to_char(struct char_data *ch,const char *fmt,...) {
  va_list ap; va_start(ap,fmt); vsnprintf(output+strlen(output),sizeof(output)-strlen(output),fmt,ap); va_end(ap);
}
void save_char(struct char_data *ch,int unused) { saves++; }
void act(const char *text,int hide,struct char_data *ch,void *obj,void *vict,int type) {}
void draw_line(struct char_data *ch,int width,char a,char b) {}
int real_quest(int vnum) { for(int i=0;i<4;i++) if(aquest_table[i].vnum==vnum) return i; return NOTHING; }
int real_object(int vnum) { return vnum==999 ? 0 : NOTHING; }
bool is_complete(struct char_data *ch,int vnum) { int i=real_quest(vnum); return i!=NOTHING && ch->completed[i]; }
bool is_dialogue_quest_failed(struct char_data *ch,int vnum) { int i=real_quest(vnum); return i!=NOTHING && ch->failed[i]; }
bool is_dialogue_alternative_quest(int vnum) { for(int i=0;i<4;i++) if(aquest_table[i].dialogue_alternative_quest==vnum) return true; return false; }
int get_dialogue_alternative_quest_rnum(int vnum) { return real_quest(vnum); }
struct obj_data *get_obj_in_list_num(int rnum,struct obj_data *obj) { for(;obj;obj=obj->next) if(obj->rnum==rnum) return obj; return NULL; }
void skip_spaces_c(const char **s) { while(**s && isspace((unsigned char)**s)) ++*s; }
const char *one_argument(const char *s,char *out,size_t n) {
  size_t len=0; skip_spaces_c(&s); while(*s && !isspace((unsigned char)*s)) { if(len+1<n) out[len++]=*s; s++; } out[len]=0; return s;
}
const char *two_arguments(const char *s,char *a,size_t na,char *b,size_t nb) { return one_argument(one_argument(s,a,na),b,nb); }
size_t strlcpy(char *d,const char *s,size_t n) { snprintf(d,n,"%s",s); return strlen(s); }
int search_block(const char *arg,const char **list,bool exact) { for(int i=0;strcmp(list[i],"\n");i++) if(!strncmp(arg,list[i],strlen(arg))) return i; return -1; }
struct char_data *get_char_vis(struct char_data *ch,char *name,int *num,int where) {
  for(struct char_data *p=character_list;p;p=p->next) {
    if(p->room==ch->room && !strcmp(name,p->name)) return p;
  }
  return NULL;
}
int find_quest_by_qmnum(struct char_data *ch,int qm,int number) { return number>=1 && number<=4 ? aquest_table[number-1].vnum : NOTHING; }
void quest_hist(struct char_data *ch,char *arg) {}
void quest_quit(struct char_data *ch,char *arg) {}
void quest_progress(struct char_data *ch,char *arg) {}
void quest_stat(struct char_data *ch,char *arg) {}
void quest_assign(struct char_data *ch,char *arg) {}
'''
a = quest.index('static const char *quest_cmd[]')
b = quest.index('/*--------------------------------------------------------------------------*/', a)
code += quest[a:b]
for signature in [
    'void set_quest(', 'static bool quest_join_eligible(', 'static void quest_begin(',
    'void quest_join(', 'static void quest_share_clear(', 'static bool quest_share_party(',
    'static bool quest_share_active(', 'static void quest_share_offer(',
    'static void quest_share_respond(', 'ACMD(do_quest)']:
    code += function(quest, signature)
qedit = (root/'src/qedit.c').read_text()
a = qedit.index('  case QEDIT_FLAGS:')
b = qedit.index('  case QEDIT_QUANTITY:', a)
code += r'''
#define QEDIT_FLAGS 1
#define OLC_QUEST(d) ((d)->quest)
static void qedit_disp_flag_menu(struct descriptor_data *d) {}
static void write_to_output(struct descriptor_data *d,const char *fmt,...) {}
static void toggle_flag(struct descriptor_data *d,int number) {
  switch(QEDIT_FLAGS) {
''' + qedit[a:b] + r'''
  }
}
static struct char_data sender,recipient,other;
static struct descriptor_data ds={.playing=true},dr={.playing=true},dt={.playing=true};
static int group_a,group_b;
static void setup(void) {
  memset(&sender,0,sizeof(sender)); memset(&recipient,0,sizeof(recipient)); memset(&other,0,sizeof(other));
  sender=(struct char_data){.id=1,.name="sender",.level=10,.room=5,.group=&group_a,.desc=&ds,.next=&recipient};
  recipient=(struct char_data){.id=2,.name="recipient",.level=10,.room=5,.group=&group_a,.desc=&dr,.next=&other};
  other=(struct char_data){.id=3,.name="other",.level=10,.room=5,.group=&group_a,.desc=&dt};
  for(int i=0;i<4;i++) {
    sender.quests[i]=recipient.quests[i]=other.quests[i]=NOTHING;
    aquest_table[i]=(struct aq_data){.vnum=100+i,.type=3,.prereq=NOTHING,.prev_quest=NOTHING,.dialogue_alternative_quest=NOTHING,
      .name="Shared quest",.desc="Quest description",.info="Quest instructions"};
    aquest_table[i].value[2]=5; aquest_table[i].value[3]=20;
    aquest_table[i].value[4]=30; aquest_table[i].value[6]=5;
  }
  character_list=&sender; sender.quests[0]=100; sender.counter[0]=2; sender.timer[0]=7;
  other.quests[0]=101; other.counter[0]=1;
  saves=0; output[0]=0;
}
static void offer(void) { do_quest(&sender,"share 0 recipient",0,0); }
static void accept(void) { do_quest(&recipient,"accept",0,0); }
static void expect_no_offer(void) { assert(!recipient.quest_share_expires && recipient.quests[0]==NOTHING && saves==0); }
int main(void) {
  int slot;
  setup(); offer();
  assert(recipient.quest_share_expires && recipient.quests[0]==NOTHING && saves==0);
  assert(strstr(output,"quest accept") && strstr(output,"Quest description"));
  accept(); assert(recipient.quests[0]==100 && recipient.counter[0]==5 && recipient.timer[0]==30 && saves==1);
  assert(sender.counter[0]==2 && sender.timer[0]==7 && !recipient.quest_share_expires);
  accept(); assert(saves==1);
  setup(); offer(); do_quest(&recipient,"decline",0,0); expect_no_offer();
  setup(); offer(); recipient.quest_share_expires=time(NULL)-1; accept(); expect_no_offer();
  setup(); recipient.npc=true; offer(); expect_no_offer();
  setup(); sender.npc=true; offer(); expect_no_offer();
  setup(); recipient.group=&group_b; offer(); expect_no_offer();
  setup(); recipient.group=NULL; offer(); expect_no_offer();
  setup(); recipient.room=6; offer(); expect_no_offer();
  setup(); recipient.desc=NULL; offer(); expect_no_offer();
  setup(); sender.counter[0]=0; offer(); expect_no_offer();
  setup(); sender.quests[0]=NOTHING; offer(); expect_no_offer();
  setup(); aquest_table[0].flags=AQ_NO_SHARE; offer(); expect_no_offer();
  setup(); recipient.level=4; offer(); expect_no_offer();
  setup(); recipient.level=21; offer(); expect_no_offer();
  setup(); recipient.level=LVL_IMMORT; offer(); expect_no_offer(); /* no staff overrides for sharing */
  setup(); recipient.completed[0]=true; offer(); expect_no_offer();
  aquest_table[0].flags=AQ_REPEATABLE; offer(); accept(); assert(recipient.quests[0]==100);
  setup(); for(int i=0;i<4;i++) recipient.quests[i]=101; offer(); assert(!recipient.quest_share_expires && saves==0);
  setup(); recipient.quests[0]=100; offer(); assert(!recipient.quest_share_expires && saves==0);
  setup(); aquest_table[0].prev_quest=101; offer(); expect_no_offer();
  recipient.completed[1]=true; offer(); accept(); assert(recipient.quests[0]==100);
  setup(); aquest_table[0].prereq=999; offer(); expect_no_offer();
  recipient.carrying=&obj_proto[0]; offer(); accept(); assert(recipient.quests[0]==100);
  setup(); aquest_table[0].prereq=999; recipient.eq[0]=&obj_proto[0]; offer(); accept(); assert(recipient.quests[0]==100);
  setup(); aquest_table[1].type=AQ_DIALOGUE; aquest_table[1].dialogue_alternative_quest=100; aquest_table[0].prev_quest=101;
  recipient.completed[1]=true; offer(); expect_no_offer();
  recipient.failed[1]=true; offer(); accept(); assert(recipient.quests[0]==100);
  setup(); aquest_table[1].type=AQ_DIALOGUE; aquest_table[1].dialogue_alternative_quest=102; aquest_table[0].prev_quest=101;
  recipient.completed[1]=recipient.failed[1]=true; offer(); expect_no_offer();
  recipient.completed[2]=true; offer(); accept(); assert(recipient.quests[0]==100);
  /* Revalidate every changing condition after the invitation. */
  setup(); offer(); recipient.group=&group_b; accept(); expect_no_offer();
  setup(); offer(); recipient.room=6; accept(); expect_no_offer();
  setup(); offer(); sender.desc=NULL; accept(); expect_no_offer();
  setup(); offer(); character_list=&recipient; accept(); expect_no_offer();
  setup(); offer(); aquest_table[0].flags=AQ_NO_SHARE; accept(); expect_no_offer();
  setup(); offer(); sender.quests[0]=NOTHING; accept(); expect_no_offer();
  setup(); offer(); aquest_table[0].vnum=999; accept(); expect_no_offer();
  setup(); offer(); sender.counter[0]=0; accept(); expect_no_offer();
  setup(); aquest_table[0].prev_quest=101; recipient.completed[1]=true; offer(); recipient.completed[1]=false; accept(); expect_no_offer();
  setup(); offer(); recipient.level=1; accept(); expect_no_offer();
  setup(); aquest_table[0].prereq=999; recipient.carrying=&obj_proto[0]; offer(); recipient.carrying=NULL; accept(); expect_no_offer();
  setup(); offer(); recipient.completed[0]=true; accept(); expect_no_offer();
  setup(); offer(); for(int i=0;i<4;i++) recipient.quests[i]=101; accept(); assert(saves==0 && !recipient.quest_share_expires);
  setup(); offer(); do_quest(&other,"share 0 recipient",0,0); assert(recipient.quest_share_from_id==sender.id); accept(); assert(recipient.quests[0]==100);
  setup(); do_quest(&sender,"share 0 sender",0,0); expect_no_offer();
  setup(); do_quest(&sender,"share 0 missing",0,0); expect_no_offer();
  setup(); do_quest(&sender,"share",0,0); expect_no_offer();
  setup(); do_quest(&sender,"share nonsense recipient",0,0); expect_no_offer();
  do_quest(&sender,"share -1 recipient",0,0); expect_no_offer();
  do_quest(&sender,"share 9999999999999999999999999999 recipient",0,0); expect_no_offer();
  /* Normal questmaster joining still uses the same eligibility and setup. */
  setup(); { char join_arg[MAX_INPUT_LENGTH]="1"; quest_join(&recipient,&other,join_arg); } assert(recipient.quests[0]==100 && saves==1);
  setup(); aquest_table[0].flags=AQ_NO_SHARE;
  { char join_arg[MAX_INPUT_LENGTH]="1"; quest_join(&recipient,&other,join_arg); }
  assert(recipient.quests[0]==100 && saves==1); /* flag only prevents sharing */
  setup(); recipient.level=LVL_IMMORT; assert(quest_join_eligible(&recipient,0,&slot,true));
  /* QEDIT uses independent bit masks for menu options, including NO-SHARE. */
  setup(); struct descriptor_data editor={.quest=&aquest_table[0]};
  toggle_flag(&editor,1); assert(aquest_table[0].flags==AQ_REPEATABLE);
  toggle_flag(&editor,3); assert(aquest_table[0].flags==(AQ_REPEATABLE|AQ_NO_SHARE));
  toggle_flag(&editor,2); assert(aquest_table[0].flags==(AQ_REPEATABLE|AQ_NO_SHARE|2));
  toggle_flag(&editor,3); assert(aquest_table[0].flags==(AQ_REPEATABLE|2));
  toggle_flag(&editor,4); assert(aquest_table[0].flags==(AQ_REPEATABLE|2));
  puts("Quest sharing: consent, prerequisites, party restrictions, revalidation, fresh progress, normal joining, and QEDIT flags passed.");
}
'''
with tempfile.TemporaryDirectory(prefix='quest-sharing-') as tmp:
    c=Path(tmp)/'sharing.c'; c.write_text(code); exe=Path(tmp)/'sharing'
    subprocess.run(['cc','-std=gnu99','-Wall','-Wextra','-Wno-unused-parameter','-fsanitize=address,undefined',str(c),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
