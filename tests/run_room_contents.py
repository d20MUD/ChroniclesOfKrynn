#!/usr/bin/env python3
"""Exercise production room reports and exact, visibility-checked target lookup."""
from pathlib import Path
import subprocess
import tempfile
root=Path(__file__).resolve().parents[1]
def function(file,signature):
    s=(root/file).read_text();i=s.index(signature);return s[i:s.index('\n}',i)+2]
periodic=(root/'src/comm.c').read_text().split('static void msdp_update(void)\n{',1)[1]
assert 'update_msdp_room_contents(ch);' in periodic
code=r'''
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <json-c/json.h>
#define FALSE false
#define NOWHERE -1
#define eMSDP_ROOM_CONTENTS 0
#define MAX_VARIABLE_LENGTH 524288
#define FIND_CHAR_ROOM 1
#define ITEM_WEAR_TAKE 1
#define PRF_HOLYLIGHT 1
struct variable { bool bReport; };
struct protocol { bool bMSDP;struct variable **pVariables; };
struct descriptor_data { struct protocol *pProtocol; };
struct char_data { long id;int room;bool visible,npc,dead,extracting;const char *name;struct char_data *next_in_room;struct descriptor_data *desc;struct obj_data *carrying; };
struct obj_data { long id;bool visible,takeable;const char *name,*description;struct obj_data *next_content; };
static struct { int vnum;struct char_data *people;struct obj_data *contents; } world[2];
#define GET_ID(x) ((x)->id)
#define IN_ROOM(ch) ((ch)->room)
#define VALID_ROOM_RNUM(r) ((r)>=0 && (r)<2)
#define GET_ROOM_VNUM(r) (world[r].vnum)
#define CAN_SEE(ch,p) ((p)->visible)
#define CAN_INFRA(ch,p) false
#define CAN_SEE_OBJ(ch,o) ((o)->visible)
#define CAN_WEAR(o,f) ((o)->takeable)
#define PRF_FLAGGED(ch,f) false
#define IS_NPC(ch) ((ch)->npc)
#define POS_DEAD 0
#define GET_POS(ch) ((ch)->dead ? 0 : 1)
#define MOB_NOTDEADYET 1
#define PLR_NOTDEADYET 1
#define MOB_FLAGGED(ch,f) ((ch)->npc && (ch)->extracting)
#define PLR_FLAGGED(ch,f) (!(ch)->npc && (ch)->extracting)
#define PERS(p,ch) ((p)->name)
#define GET_OBJ_SHORT(o) ((o)->name)
static char *response;
static int writes;
static void MSDPSetString(struct descriptor_data *d,int variable,const char *text) { writes++;free(response);response=strdup(text); }
static int get_number(char **name) { char *dot=strchr(*name,'.');if(!dot)return 1;int n=atoi(*name);*name=dot+1;return n; }
static int str_cmp(const char *a,const char *b) {return strcmp(a,b);}
static bool isname(const char *name,const char *keywords) {return !strcmp(name,keywords);}
static bool identity_matches(struct char_data *ch,struct char_data *p,const char *name) {return isname(name,p->name);}
static bool can_blindsense_creature(struct char_data *ch,struct char_data *p) {return false;}
static struct char_data *get_player_vis(struct char_data *ch,char *name,int *number,int where) {return NULL;}
'''
code+=function('src/handler.c','static bool room_entity_matches(')+'\n'
code+=function('src/handler.c','struct char_data *get_char_room_vis(')+'\n'
code+=function('src/handler.c','struct obj_data *get_obj_in_list_vis(')+'\n'
code+=function('src/comm.c','static void update_msdp_room_contents(')+'\n'
code+=r'''
static struct json_object *field(struct json_object *o,const char *key) {struct json_object *v;assert(json_object_object_get_ex(o,key,&v));return v;}
int main(void) {
  struct variable variable={.bReport=true};struct variable *variables[]={&variable};
  struct protocol protocol={.bMSDP=true,.pVariables=variables};struct descriptor_data descriptor={.pProtocol=&protocol};
  struct char_data player={.id=1,.visible=true,.room=0,.name="self",.desc=&descriptor};
  struct char_data mob1={.id=100,.visible=true,.npc=true,.name="guard"},mob2={.id=101,.visible=true,.npc=true,.name="guard"};
  struct char_data hidden={.id=102,.visible=false,.npc=true,.name="secret"},other={.id=103,.visible=true,.name="a stranger"};
  struct obj_data sword={.id=200,.visible=true,.takeable=true,.name="sword",.description="A sword."};
  struct obj_data sign={.id=201,.visible=true,.name="sign",.description="A sign."};
  struct obj_data invisible={.id=202,.visible=false,.name="hidden",.description="Hidden."};
  struct obj_data omitted={.id=203,.visible=true,.name="trigger",.description=".hidden trigger"};
  world[0].vnum=1005;world[0].people=&player;player.next_in_room=&mob1;mob1.next_in_room=&mob2;mob2.next_in_room=&hidden;hidden.next_in_room=&other;
  world[0].contents=&sword;sword.next_content=&sign;sign.next_content=&invisible;invisible.next_content=&omitted;
  update_msdp_room_contents(&player);assert(writes==1);
  struct json_object *report=json_tokener_parse(response);assert(report);
  assert(json_object_get_int(field(report,"room"))==1005);
  struct json_object *entries=field(report,"entries");assert(json_object_array_length(entries)==5);
  assert(!strcmp(json_object_get_string(field(json_object_array_get_idx(entries,0),"target")),"}c100"));
  assert(!strcmp(json_object_get_string(field(json_object_array_get_idx(entries,2),"name")),"a stranger"));
  assert(json_object_get_boolean(field(json_object_array_get_idx(entries,3),"takeable")));
  assert(!json_object_get_boolean(field(json_object_array_get_idx(entries,4),"takeable")));
  json_object_put(report);
  char target1[]="}c101",target2[]="}c102",stale[]="}c999",normal[]="2.guard",wrongkind[]="}o101";
  assert(get_char_room_vis(&player,target1,NULL)==&mob2);
  assert(!get_char_room_vis(&player,target2,NULL));assert(!get_char_room_vis(&player,stale,NULL));
  assert(!get_char_room_vis(&player,wrongkind,NULL));assert(get_char_room_vis(&player,normal,NULL)==&mob2);
  mob1.next_in_room=&hidden;assert(!get_char_room_vis(&player,target1,NULL));
  char objtarget[]="}o201",objhidden[]="}o202",objstale[]="}o999";
  assert(get_obj_in_list_vis(&player,objtarget,NULL,world[0].contents)==&sign);
  assert(!get_obj_in_list_vis(&player,objhidden,NULL,world[0].contents));
  assert(!get_obj_in_list_vis(&player,objstale,NULL,world[0].contents));
  assert(!room_entity_matches("}c101\nkill",'c',101));
  assert(!room_entity_matches("}c999999999999999999999999999999999",'c',101));
  /* Regular refreshes remove departed, dead and pending-extraction entities. */
  update_msdp_room_contents(&player);
  report=json_tokener_parse(response);entries=field(report,"entries");
  assert(json_object_array_length(entries)==4);assert(!strstr(response,"}c101"));json_object_put(report);
  mob1.dead=true;other.extracting=true;
  update_msdp_room_contents(&player);
  report=json_tokener_parse(response);entries=field(report,"entries");
  assert(json_object_array_length(entries)==2 && !strstr(response,"}c100") && !strstr(response,"}c103"));json_object_put(report);
  mob1.dead=false;mob1.extracting=true;other.extracting=false;
  update_msdp_room_contents(&player);assert(!strstr(response,"}c100") && strstr(response,"}c103"));
  mob1.extracting=false;
  update_msdp_room_contents(&player);assert(strstr(response,"}c100"));
  variable.bReport=false;update_msdp_room_contents(&player);assert(writes==5);
  variable.bReport=true;player.room=NOWHERE;update_msdp_room_contents(&player);
  report=json_tokener_parse(response);assert(json_object_array_length(field(report,"entries"))==0);json_object_put(report);
  free(response);puts("Room contents: visibility, short descriptions, takeable flags, duplicates, stale IDs and opt-in passed");
}
'''
with tempfile.TemporaryDirectory(prefix='room-contents-') as temp:
    c=Path(temp)/'room.c';c.write_text(code);exe=Path(temp)/'room'
    subprocess.run(['cc','-std=gnu11','-Wall','-Wextra','-Wno-unused-parameter','-fsanitize=address,undefined',str(c),'-ljson-c','-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
