#!/usr/bin/env python3
"""Exercise production dialogue outcomes and alternative-quest modal dispatch."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root/'src/act.comm.c').read_text()
start = source.index('ACMD(do_dialogue_quest)\n{')
handler = source[start:source.index('\n}', start)+2]
code = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#define NOTHING -1
#define MAX_CURRENT_QUESTS 4
#define AQ_DIALOGUE 24
#define LONG_STRING 4096
#define TO_CHAR 0
#define SCMD_DIALOGUE_DIPLOMACY 1
#define SCMD_DIALOGUE_INTIMIDATE 2
#define SCMD_DIALOGUE_BLUFF 3
#define ABILITY_DIPLOMACY 1
#define ABILITY_INTIMIDATE 2
#define ABILITY_BLUFF 3
#define FALSE false
#define ACMD(name) void name(struct char_data *ch,const char *argument,int cmd,int subcmd)
typedef int qst_rnum;
struct char_data { int quests[4],npc,vnum; struct char_data *next_in_room; };
struct { int vnum,diplomacy_dc,intimidate_dc,bluff_dc,dialogue_alternative_quest; const char *info; } aquest_table[2];
static struct { struct char_data *people; } world[1];
#define GET_QUEST(ch,i) ((ch)->quests[i])
#define GET_QUEST_TYPE(ch,i) AQ_DIALOGUE
#define IN_ROOM(ch) 0
#define IS_NPC(ch) ((ch)->npc)
#define QST_TARGET(q) 123
#define GET_MOB_VNUM(ch) ((ch)->vnum)
#define CAN_SEE(ch,i) true
#define QST_INFO(q) (aquest_table[q].info)
static int roll=6,skill=-1,modals,saves,failed,completed,successes,generic_completed,assigned;
static int notified_quest=-1;
static char output[16384];
static int real_quest(int vnum) { for(int i=0;i<2;i++) if(aquest_table[i].vnum==vnum)return i; return NOTHING; }
static int d20(struct char_data *ch) { return roll; }
static int compute_ability(struct char_data *ch,int ability) { return skill; }
static void send_to_char(struct char_data *ch,const char *fmt,...) {
  va_list args;va_start(args,fmt);vsnprintf(output+strlen(output),sizeof(output)-strlen(output),fmt,args);va_end(args);
}
static void act(const char *text,int hide,struct char_data *ch,void *obj,struct char_data *vict,int where) { send_to_char(ch,"%s\n",text); }
static void set_dialogue_quest_succeeded(struct char_data *ch,int quest) { successes++; }
static void generic_complete_quest(struct char_data *ch,int slot) { generic_completed++; }
static void set_dialogue_quest_failed(struct char_data *ch,int quest) { failed++; }
static void add_completed_quest(struct char_data *ch,int quest) { completed++; }
static void clear_quest(struct char_data *ch,int slot) { ch->quests[slot]=NOTHING; }
static void set_quest(struct char_data *ch,int quest,int slot) { ch->quests[slot]=aquest_table[quest].vnum;assigned++; }
static void send_quest_acceptance(struct char_data *ch,int quest) { modals++;notified_quest=quest;assert(ch->quests[0]==aquest_table[quest].vnum); }
static void save_char(struct char_data *ch,int unused) { saves++; }
'''
code += handler + r'''
int main(void) {
  struct char_data player={0},mob={.npc=1,.vnum=123};world[0].people=&mob;
  aquest_table[0].vnum=100;aquest_table[0].diplomacy_dc=aquest_table[0].intimidate_dc=aquest_table[0].bluff_dc=15;
  aquest_table[0].dialogue_alternative_quest=101;aquest_table[1].vnum=101;
  aquest_table[1].info="\tCThe dockmaster rolls his eyes.\n\nGo kill 10 wharf rats and return to the dockmaster.\tn";
  for(int method=1;method<=3;method++) {
    for(int i=0;i<4;i++)player.quests[i]=NOTHING;
    player.quests[0]=100;output[0]=0;
    do_dialogue_quest(&player,"",0,method);
    assert(modals==method && notified_quest==1 && assigned==method && saves==method);
    assert(failed==method && completed==method && !successes && !generic_completed);
    assert(strstr(output,"Go kill 10 wharf rats"));
  }
  player.quests[0]=100;aquest_table[0].dialogue_alternative_quest=NOTHING;
  do_dialogue_quest(&player,"",0,SCMD_DIALOGUE_DIPLOMACY);
  assert(modals==3 && assigned==3 && player.quests[0]==100);
  roll=20;skill=5;
  do_dialogue_quest(&player,"",0,SCMD_DIALOGUE_DIPLOMACY);
  assert(successes==1 && generic_completed==1 && modals==3);
  puts("Dialogue quest failure: diplomacy, intimidate, bluff, fallback assignment and modal event passed");
}
'''
with tempfile.TemporaryDirectory(prefix='dialogue-modal-') as tmp:
    c=Path(tmp)/'dialogue.c';c.write_text(code);exe=Path(tmp)/'dialogue'
    subprocess.run(['cc','-std=gnu11','-Wall','-Wextra','-Wno-unused-parameter','-fsanitize=address,undefined',str(c),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
