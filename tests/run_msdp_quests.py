#!/usr/bin/env python3
"""Exercise production quest serialization and lossless protocol queueing."""
from pathlib import Path
import os
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'src/comm.c').read_text()
def function(name):
    start = source.index(name)
    return source[start:source.index('\n}', start) + 2]
code = r'''
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "quest.h"
#include "protocol.h"
#include <json-c/json.h>
#include <assert.h>
#undef log
#define log(...) ((void)0)
#undef CREATE
#define CREATE(p,t,n) do { (p)=calloc((n),sizeof(t)); assert(p); } while(0)
static int slots[3], counters[3], times[3];
#undef GET_QUEST
#undef GET_QUEST_COUNTER
#undef GET_QUEST_TIME
#undef GET_NAME
#define GET_QUEST(ch,i) slots[i]
#define GET_QUEST_COUNTER(ch,i) counters[i]
#define GET_QUEST_TIME(ch,i) times[i]
#define GET_NAME(ch) "Test Archer"
struct aq_data quests[3];
struct aq_data *aquest_table = quests;
struct room_data rooms[1], *world = rooms;
struct obj_data objects[1], *obj_proto = objects;
struct char_data mobs[1], *mob_proto = mobs;
const char *quest_types[NUM_AQ_TYPES+1];
static char *reported;
static struct txt_block *bufpool;
static int buf_largecount;
qst_rnum real_quest(qst_vnum v) { return v >= 0 && v < 3 ? v : NOTHING; }
obj_rnum real_object(obj_vnum v) { return v == 1 ? 0 : NOTHING; }
room_rnum real_room(room_vnum v) { return v == 1 ? 0 : NOWHERE; }
mob_rnum real_mobile(mob_vnum v) { return v == 1 ? 0 : NOBODY; }
void MSDPSetString(struct descriptor_data *d, variable_t var, const char *value) {
  assert(var==eMSDP_QUEST_INFO); free(reported); reported=strdup(value);
}
'''
code += '\n' + function('static void append_msdp_quest_target(')
code += '\n' + function('static void update_msdp_quest_info(struct descriptor_data *d, struct char_data *ch)\n{')
code += '\n' + function('bool write_protocol_packet(')
protocol_source = (root / 'src/protocol.c').read_text()
start = protocol_source.index('void MSDPUpdate(')
code += r'''
static bool queued;
bool_t MSDPSend(struct descriptor_data *d, variable_t var) { return queued; }
'''
code += protocol_source[start:protocol_source.index('\n}', start)+2]

code += r'''
static struct json_object *field(struct json_object *o,const char *key) {
  struct json_object *v=NULL; assert(json_object_object_get_ex(o,key,&v)); return v;
}
int main(void) {
  struct descriptor_data d={0}; struct char_data ch={0};
  char *long_text=malloc(49152); memset(long_text,'x',49151);long_text[49151]=0;
  const char *prefix="\"\\\n\t<script>caf\303\251";memcpy(long_text,prefix,strlen(prefix));
  for(int i=0;i<3;i++) {
    slots[i]=i;counters[i]=3;times[i]=7;
    quests[i].vnum=i;quests[i].name="A \"quest\"";quests[i].desc=long_text;
    quests[i].info=long_text;quests[i].type=AQ_DIALOGUE;quests[i].target=1;
    quests[i].value[6]=5;quests[i].bluff_dc=10;
  }
  mobs[0].player.short_descr="a target";quest_types[AQ_DIALOGUE]="Dialogue";
  update_msdp_quest_info(&d,&ch);
  assert(strlen(reported)>16384 && strlen(reported)<MAX_VARIABLE_LENGTH);
  struct json_object *decoded=json_tokener_parse(reported);assert(decoded);
  assert(json_object_array_length(decoded)==3);
  struct json_object *q=json_object_array_get_idx(decoded,2);
  assert(!strcmp(json_object_get_string(field(q,"info")),long_text));
  assert(json_object_get_int(field(field(q,"progress"),"completed"))==2);
  assert(!strcmp(json_object_get_string(json_object_array_get_idx(field(q,"dialogue_commands"),0)),"beguile"));
  json_object_put(decoded);
  // A full-size packet must survive queue promotion, percent signs, and colors.
  d.output=d.small_outbuf;d.bufspace=SMALL_BUFSIZE-1;
  size_t length=strlen(reported)+32;char *packet=malloc(length);
  snprintf(packet,length,"\377\372\105\001QUEST_INFO\002%s\377\360",reported);
  assert(write_protocol_packet(&d,packet));
  assert(!strcmp(d.output,packet));assert(d.bufptr==strlen(packet));
  assert(d.large_outbuf && buf_largecount==1);
  int before=d.bufptr;size_t left=d.bufspace;
  char *too_large=malloc(LARGE_BUFSIZE+1);memset(too_large,'z',LARGE_BUFSIZE);too_large[LARGE_BUFSIZE]=0;
  assert(!write_protocol_packet(&d,too_large));assert(d.bufptr==before && d.bufspace==left);
  // Removing all active quests clears the client rather than retaining old data.
  for(int i=0;i<3;i++)slots[i]=NOTHING;
  update_msdp_quest_info(&d,&ch);assert(!strcmp(reported,"[]"));
  protocol_t protocol={0}; MSDP_t values[eMSDP_MAX]={0}; MSDP_t *pointers[eMSDP_MAX];
  for(int i=0;i<eMSDP_MAX;i++)pointers[i]=&values[i];
  protocol.pVariables=pointers;d.pProtocol=&protocol;
  values[eMSDP_QUEST_INFO].bReport=true;values[eMSDP_QUEST_INFO].bDirty=true;
  queued=false;MSDPUpdate(&d);assert(values[eMSDP_QUEST_INFO].bDirty);
  queued=true;MSDPUpdate(&d);assert(!values[eMSDP_QUEST_INFO].bDirty);
  free(too_large);free(packet);free(d.large_outbuf->text);free(d.large_outbuf);
  free(long_text);free(reported);puts("MSDP quest and protocol buffer tests passed");
}
'''
with tempfile.TemporaryDirectory() as temp:
    c = Path(temp) / 'test.c'
    exe = Path(temp) / 'test'
    c.write_text(code)
    subprocess.run([os.environ.get('CC', 'cc'), '-std=gnu11', '-I'+str(root/'src'), str(c), '-ljson-c', '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
