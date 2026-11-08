#!/usr/bin/env python3
"""Exercise production quest acceptance/completion payloads, opt-in and flushing."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'src/quest.c').read_text()

def function(signature):
    start = source.index(signature)
    return source[start:source.index('\n}', start) + 2]

code = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <json-c/json.h>
typedef int qst_rnum;
#define eMSDP_QUEST_ACCEPTED 0
#define eMSDP_QUEST_COMPLETE 1
#define RACE_VAMPIRE 10
#define RACE_LICH 11
#define AQ_DIALOGUE 24
#define QST_NUM(r) 70001
#define QST_NAME(r) "A quest <script> & \"quotes\""
#define QST_INFO(r) "Instructions\n\tRRed\tn"
#define QST_DONE(r) "Completion prose\n\nYou helped!"
#define QST_TIME(r) 20
#define QST_TYPE(r) AQ_DIALOGUE
#define QST_GOLD(r) 100
#define QST_EXP(r) 1000
#define QST_RACE(r) 0
static struct { int diplomacy_dc, intimidate_dc, bluff_dc; } aquest_table[1]={{10,0,12}};
struct variable { bool bReport,bDirty; };
struct protocol { bool bMSDP; struct variable **pVariables; };
struct descriptor_data { struct protocol *pProtocol; };
struct char_data { struct descriptor_data *desc; };
static int writes,flushes,last_var;
static char *response;
static void MSDPSetString(struct descriptor_data *d,int var,const char *text) {
  writes++;last_var=var;free(response);response=strdup(text);
}
static void MSDPFlush(struct descriptor_data *d,int var) {
  assert(var==last_var && d->pProtocol->pVariables[var]->bDirty);
  d->pProtocol->pVariables[var]->bDirty=false;flushes++;
}
'''
code += function('void send_quest_acceptance(') + '\n'
code += function('static void send_quest_completion(') + '\n'
code += r'''
static struct json_object *field(struct json_object *o,const char *name) {
  struct json_object *v;assert(json_object_object_get_ex(o,name,&v));return v;
}
int main(void) {
  struct variable accepted={0},completed={0};
  struct variable *vars[]={&accepted,&completed};
  struct protocol protocol={.bMSDP=true,.pVariables=vars};
  struct descriptor_data descriptor={.pProtocol=&protocol};
  struct char_data player={.desc=&descriptor};
  send_quest_acceptance(&player,0);
  send_quest_completion(&player,0,6,150,1250,NULL,NULL);
  assert(!writes && !flushes);
  accepted.bReport=completed.bReport=true;
  send_quest_acceptance(&player,0);
  assert(writes==1 && flushes==1 && last_var==eMSDP_QUEST_ACCEPTED);
  struct json_object *event=json_tokener_parse(response);assert(event);
  assert(!strcmp(json_object_get_string(field(event,"name")),QST_NAME(0)));
  assert(!strcmp(json_object_get_string(field(event,"body")),QST_INFO(0)));
  assert(json_object_get_int(field(event,"time"))==20);
  assert(json_object_array_length(field(event,"commands"))==2);
  char *id=strdup(json_object_get_string(field(event,"id")));json_object_put(event);
  send_quest_acceptance(&player,0);
  event=json_tokener_parse(response);
  assert(strcmp(id,json_object_get_string(field(event,"id"))));free(id);json_object_put(event);
  send_quest_completion(&player,0,6,150,1250,"a sword","a follower");
  assert(writes==3 && flushes==3 && last_var==eMSDP_QUEST_COMPLETE);
  event=json_tokener_parse(response);assert(event);
  assert(!strcmp(json_object_get_string(field(event,"body")),QST_DONE(0)));
  struct json_object *rewards=field(event,"rewards");
  assert(json_object_get_int(field(rewards,"Quest points"))==6);
  assert(json_object_get_int(field(rewards,"Gold coins"))==150);
  assert(json_object_get_int(field(rewards,"Experience"))==1250);
  assert(!strcmp(json_object_get_string(field(rewards,"Item")),"a sword"));
  assert(!strcmp(json_object_get_string(field(rewards,"Follower")),"a follower"));
  id=strdup(json_object_get_string(field(event,"id")));json_object_put(event);
  send_quest_completion(&player,0,6,150,1250,NULL,NULL);
  event=json_tokener_parse(response);
  assert(strcmp(id,json_object_get_string(field(event,"id"))));free(id);json_object_put(event);
  protocol.bMSDP=false;
  send_quest_acceptance(&player,0);send_quest_completion(&player,0,0,0,0,NULL,NULL);
  assert(writes==4 && flushes==4);
  free(response);
  puts("Quest acceptance/completion: opt-in, real rewards, JSON, unique IDs and immediate flushing passed");
}
'''
with tempfile.TemporaryDirectory(prefix='quest-events-') as temp:
    c = Path(temp) / 'events.c'
    exe = Path(temp) / 'events'
    c.write_text(code)
    subprocess.run(['cc', '-std=gnu11', '-Wall', '-Wextra', '-Wno-unused-parameter',
                    '-fsanitize=address,undefined', str(c), '-ljson-c', '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
