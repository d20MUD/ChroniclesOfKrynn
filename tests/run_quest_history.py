#!/usr/bin/env python3
"""Exercise native request-driven MSDP quest history and ownership checks."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1]
s=(root/'src/quest.c').read_text();a=s.index('#define WEB_QUEST_HISTORY_PAGE_SIZE');b=s.index('ACMD(do_quest)',a)
code=r'''
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <ctype.h>
#include <sys/time.h>
#include <json-c/json.h>
#define TRUE true
#define FALSE false
#define NOTHING -1
#define MAX_COMPLETED_QUESTS 1024
#define MAX_VARIABLE_LENGTH 16384
#define eMSDP_QUEST_HISTORY 0
#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))
typedef int qst_rnum;
struct variable{bool bReport,bDirty;};
struct protocol{bool bMSDP;struct variable **pVariables;};
struct descriptor_data{struct protocol *pProtocol;};
struct special{struct{int *completed_quests;int num_completed_quests;}saved;};
struct char_data{struct descriptor_data *desc;struct special *player_specials;};
#define GET_NUM_QUESTS(ch) ((ch)->player_specials->saved.num_completed_quests)
#define QST_NAME(r) quest_name
static const char *quest_name="\tCCompleted Quest\tn";
#define QST_DESC(r) "A past adventure."
#define QST_INFO(r) info
#define QST_DONE(r) "Completion\n\n\tRWell done!\tn"
static const char *info="Quest information.";
static char *response;
static int writes;
int real_quest(int id){return id==999?NOTHING:0;}
void MSDPSetString(struct descriptor_data *d,int var,const char *text){assert(var==0);free(response);response=strdup(text);writes++;}
void MSDPFlush(struct descriptor_data *d,int var){assert(d->pProtocol->pVariables[var]->bDirty);}
'''+s[a:b]+r'''
static struct json_object *field(struct json_object *o,const char *key){struct json_object *v;assert(json_object_object_get_ex(o,key,&v));return v;}
int main(void){
 struct variable var={0};struct variable *vars[]={&var};struct protocol protocol={true,vars};struct descriptor_data d={&protocol};struct special ps={0};struct char_data ch={&d,&ps};
 quest_web_history(&ch,"",false);assert(!writes);var.bReport=true;
 quest_web_history(&ch,"",false);assert(writes==1);struct json_object *data=json_tokener_parse(response);
 assert(!json_object_get_int(field(data,"total")) && !json_object_array_length(field(data,"entries")));json_object_put(data);
 int completed[1024];for(int i=0;i<1024;i++)completed[i]=100+i;ps.saved.completed_quests=completed;ps.saved.num_completed_quests=51;
 quest_web_history(&ch,"0",false);data=json_tokener_parse(response);assert(json_object_get_int(field(data,"total"))==51 && json_object_get_int(field(data,"pages"))==2);assert(json_object_array_length(field(data,"entries"))==50);json_object_put(data);
 quest_web_history(&ch,"1",false);data=json_tokener_parse(response);assert(json_object_array_length(field(data,"entries"))==1);json_object_put(data);
 quest_web_history(&ch,"99999",false);data=json_tokener_parse(response);assert(json_object_get_int(field(data,"page"))==1);json_object_put(data);
 quest_web_history(&ch,"100",true);data=json_tokener_parse(response);assert(!strcmp(json_object_get_string(field(data,"name")),QST_NAME(0)));assert(!strcmp(json_object_get_string(field(data,"completion")),QST_DONE(0)));json_object_put(data);
 const char *bad[]={"999", "99", "", "-1", "100 trailing", "99999999999999999999999999"};
 for(unsigned i=0;i<sizeof(bad)/sizeof(*bad);i++){quest_web_history(&ch,bad[i],true);data=json_tokener_parse(response);field(data,"error");struct json_object *v;assert(!json_object_object_get_ex(data,"info",&v));json_object_put(data);}
 completed[0]=999;quest_web_history(&ch,"0",false);data=json_tokener_parse(response);assert(!json_object_get_boolean(field(json_object_array_get_idx(field(data,"entries"),0),"available")));json_object_put(data);
 // Max-sized histories stay bounded and page safely.
 ps.saved.num_completed_quests=MAX_COMPLETED_QUESTS;quest_web_history(&ch,"20",false);data=json_tokener_parse(response);assert(json_object_array_length(field(data,"entries"))==24);json_object_put(data);
 char *huge=malloc(MAX_VARIABLE_LENGTH+100);memset(huge,'x',MAX_VARIABLE_LENGTH+99);huge[MAX_VARIABLE_LENGTH+99]=0;info=huge;
 quest_web_history(&ch,"101",true);data=json_tokener_parse(response);field(data,"error");assert(strlen(response)<MAX_VARIABLE_LENGTH);json_object_put(data);
 info="Info";quest_name=huge;quest_web_history(&ch,"0",false);data=json_tokener_parse(response);field(data,"error");assert(strlen(response)<MAX_VARIABLE_LENGTH);json_object_put(data);free(huge);
 protocol.bMSDP=false;int before=writes;quest_web_history(&ch,"",false);assert(writes==before);
 free(response);return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='quest-history-') as tmp:
 p=Path(tmp)/'test.c';binary=Path(tmp)/'test';p.write_text(code)
 subprocess.run(['cc','-fsanitize=address,undefined','-g','-std=gnu99',str(p),'-ljson-c','-o',str(binary)],check=True)
 subprocess.run([str(binary)],check=True)
print('Quest history: MSDP opt-in, empty/completed lists, pagination, ownership, deleted quests, colors, malformed requests, maximum history and payload bounds passed.')
