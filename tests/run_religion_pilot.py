#!/usr/bin/env python3
"""Validate pilot topology and native quest parsing/teleport restrictions offline."""
from pathlib import Path
import os
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
phase3 = os.environ.get('RELIGION_PILOT_PHASE') == '3'
pilot = root / ('examples/religion_phase3' if phase3 else 'examples/religion_phase2')
quest_count = 12 if phase3 else 8
rooms = {}
for entry in re.split(r'(?m)^#', (pilot/'699.wld').read_text())[1:]:
    vnum, body = entry.split('\n', 1)
    name, description, rest = body.split('~', 2)
    lines = rest.strip().splitlines()
    flags = int(lines[0].split()[1])
    exits = {int(direction): int(destination) for direction, destination in
             re.findall(r'(?m)^D(\d+)\n[^~]*~\n[^~]*~\n0 -1 (\d+)', rest)}
    rooms[int(vnum)] = (name, flags, exits)
assert len(rooms) == (20 if phase3 else 12)
for entrance, sanctuary in [(69900,69909),(69910,69919)]:
    flags = rooms[sanctuary][1]
    assert all(flags & (1 << bit) for bit in (19,21,24))
    assert rooms[sanctuary][2] == ({3: entrance, 0: sanctuary-4} if phase3 else {3: entrance})
    assert rooms[entrance][2][1] == sanctuary
    inbound = [(v, direction) for v, (_, _, exits) in rooms.items()
               for direction, dest in exits.items() if dest == sanctuary]
    assert sorted(inbound) == sorted([(entrance,1)] + ([(sanctuary-4,2)] if phase3 else []))
    assert not rooms[entrance][1] & flags  # public entrance is not trapped
for _, _, exits in rooms.values():
    assert all(destination in rooms for destination in exits.values())
mobs = (pilot/'699.mob').read_text()
for vnum, deity in [(69902,2),(69903,8)]:
    block = mobs.split(f'#{vnum}\n')[1].split('\nE\n')[0]
    assert '10 16384 0 65536 0 0 0 0 0 E' in block  # east + temple rank
    assert f'MobDeity: {deity}' in block and 'TempleRank: 2' in block
zone = (pilot/'699.zon').read_text()
assert 'M 0 69902 1 69900 100' in zone and 'M 0 69903 1 69910 100' in zone


def function(source, signature):
    start = source.index(signature)
    return source[start:source.index('\n}', start)+2]+'\n'

code = r'''
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "quest.h"
#include "mud_event.h"
#include "pfdefaults.h"
#include <assert.h>
struct player_special_data dummy_mob;
struct aq_data *aquest_table;
qst_rnum total_quests;
struct room_data *world;
room_rnum top_of_world=2;
struct zone_data *zone_table;
void basic_mud_log(const char *fmt,...) {(void)fmt;}
int get_line(FILE *f,char *out) {
  while(fgets(out,1024,f)) {out[strcspn(out,"\r\n")]=0;if(*out && *out!='*')return 1;}
  return 0;
}
char *fread_string(FILE *f,const char *error) {
  char text[8192];int c;size_t n=0;(void)error;
  while((c=fgetc(f))!=EOF && c!='~'){assert(n<sizeof(text)-1);text[n++]=c;}
  assert(c=='~');text[n]=0;while((c=fgetc(f))!='\n' && c!=EOF){}return strdup(text);
}
bitvector_t asciiflag_conv(const char *text) {return (bitvector_t)strtoul(text,NULL,10);}
mob_rnum real_mobile(mob_vnum v) {return v==69900 ? 0 : v==69901 ? 1 : NOBODY;}
int House_can_enter(struct char_data *ch,room_vnum v) {(void)ch;(void)v;return 1;}
'''
code += function((root/'src/quest.c').read_text(), 'void parse_quest(')
code += function((root/'src/spells.c').read_text(), 'int valid_mortal_tele_dest(')
code += r'''
static int objective_proofs, quest_events;
qst_rnum real_quest(qst_vnum v) {return v>=69900 && v<69908 ? v-69900 : NOTHING;}
bool has_duplicate_quest(struct char_data *ch) {(void)ch;return false;}
void remove_duplicate_quests(struct char_data *ch) {(void)ch;}
void religion_task_objective_complete(struct char_data *ch) {(void)ch;objective_proofs++;}
bool save_char_checked(struct char_data *ch,int mode) {(void)ch;(void)mode;return true;}
void save_char(struct char_data *ch,int mode) {(void)ch;(void)mode;}
size_t send_to_char(struct char_data *ch,const char *fmt,...) {(void)ch;(void)fmt;return 0;}
struct mud_event_data *char_has_mud_event(struct char_data *ch,event_id id) {(void)ch;(void)id;return NULL;}
struct mud_event_data *new_mud_event(event_id id,void *data,const char *text) {(void)id;(void)data;(void)text;return NULL;}
void attach_mud_event(struct mud_event_data *event,long delay) {(void)event;(void)delay;quest_events++;}
static void write_slot(FILE *fl,struct char_data *ch) {
#define BUFFER_WRITE(...) fprintf(fl,__VA_ARGS__)
'''
players = (root/'src/players.c').read_text()
a = players.index('  if (GET_QUEST(ch, RELIGION_QUEST_SLOT) != PFDEF_CURRQUEST)')
b = players.index('  if (GET_QUEST(ch, 0) != PFDEF_CURRQUEST)',a)
code += players[a:b] + '\n#undef BUFFER_WRITE\n}\n'
code += 'static void read_slot(struct char_data *ch,const char *tag,const char *line) {\n'
for tag in ('Qcu3','Qcn3','Qti3'):
    a = players.index('        else if (!strcmp(tag, "'+tag+'"))')
    fragment = players[a:players.index('\n',players.index('\n',a)+1)]
    code += fragment.replace('else if','if',1)+'\n'
code += '}\n'
code += function((root/'src/quest.c').read_text(), 'void generic_complete_quest(')

code += r'''
int main(int argc,char **argv) {
  char line[1024];FILE *f;int n=0;struct char_data ch={0};
  assert(argc==2);aquest_table=calloc(8,sizeof(*aquest_table));f=fopen(argv[1],"r");assert(f);
  while(get_line(f,line) && *line!='$') {assert(*line=='#');parse_quest(f,atoi(line+1));n++;}
  fclose(f);assert(n==8 && total_quests==8);
  for(int i=0;i<8;i++) {
    assert(QST_NUM(i)==69900+i && QST_TYPE(i)==AQ_ROOM_FIND && IS_SET(QST_FLAGS(i),AQ_REPEATABLE));
    assert(QST_TARGET(i)==(obj_vnum)((i<4 ? 69900:69910)+i%4+1));
    assert(QST_MASTER(i)==(mob_vnum)(i<4 ? 69900:69901));
    assert(!QST_POINTS(i) && !QST_GOLD(i) && !QST_EXP(i) && QST_OBJ(i)==NOTHING);
    assert(QST_TIME(i)==-1 && QST_QUANTITY(i)==1);
    assert(QST_PREV(i)==NOTHING && QST_NEXT(i)==NOTHING && QST_DIAGN(i)==NOTHING);
  }
  ch.player_specials=calloc(1,sizeof(*ch.player_specials));
  GET_QUEST(&ch,RELIGION_QUEST_SLOT)=69900;GET_QUEST_COUNTER(&ch,RELIGION_QUEST_SLOT)=3;GET_QUEST_TIME(&ch,RELIGION_QUEST_SLOT)=-1;
  f=tmpfile();assert(f);write_slot(f,&ch);rewind(f);GET_QUEST(&ch,RELIGION_QUEST_SLOT)=NOTHING;
  while(fgets(line,sizeof(line),f)) {char tag[5],value[100];assert(sscanf(line,"%4[^:]: %99[^\\n]",tag,value)==2);read_slot(&ch,tag,value);}
  fclose(f);assert(GET_QUEST(&ch,RELIGION_QUEST_SLOT)==69900 && GET_QUEST_COUNTER(&ch,RELIGION_QUEST_SLOT)==3 && GET_QUEST_TIME(&ch,RELIGION_QUEST_SLOT)==-1);
  GET_QUEST_COUNTER(&ch,RELIGION_QUEST_SLOT)=1;generic_complete_quest(&ch,RELIGION_QUEST_SLOT);
  assert(objective_proofs==1 && !quest_events);generic_complete_quest(&ch,RELIGION_QUEST_SLOT);assert(objective_proofs==1);
  GET_QUEST(&ch,0)=69900;GET_QUEST_COUNTER(&ch,0)=1;generic_complete_quest(&ch,0);assert(quest_events==1 && objective_proofs==1);
  world=calloc(3,sizeof(*world));zone_table=calloc(1,sizeof(*zone_table));
  for(int i=0;i<3;i++){world[i].zone=0;world[i].number=i;}
  for(int i=1;i<3;i++) {
    SET_BIT_AR(ROOM_FLAGS(i),ROOM_NOTELEPORT);SET_BIT_AR(ROOM_FLAGS(i),ROOM_NOSUMMON);SET_BIT_AR(ROOM_FLAGS(i),ROOM_NORECALL);
    assert(!valid_mortal_tele_dest(&ch,i,false));
  }
  assert(valid_mortal_tele_dest(&ch,0,false));
  puts("Religion pilots: native quest files, zero economic rewards, direction guards, public objectives, outward routes and native teleport denial passed");
}
'''
if phase3:
    code = code.replace('aquest_table=calloc(8,', 'aquest_table=calloc(12,').replace('n==8 && total_quests==8', 'n==12 && total_quests==12').replace('i<8;i++', 'i<12;i++')
    code = code.replace('assert(QST_TARGET(i)==(obj_vnum)((i<4 ? 69900:69910)+i%4+1));', 'assert(QST_TARGET(i)==(obj_vnum)(i<8 ? ((i<4?69900:69910)+i%4+1) : i==8?69908 : i==9?69904 : i==10?69918 : 69914));')
    code = code.replace('(i<4 ? 69900:69901)', '(i<4 || i==8 || i==9 ? 69900:69901)')
with tempfile.TemporaryDirectory(prefix='religion-pilot-') as directory:
    source = Path(directory)/'pilot.c'
    binary = Path(directory)/'pilot'
    source.write_text(code)
    subprocess.run(['cc','-std=gnu99','-I'+str(root/'src'),str(source),'-o',str(binary)],check=True)
    subprocess.run([str(binary),str(pilot/'699.qst')],check=True)
