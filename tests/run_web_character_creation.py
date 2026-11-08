#!/usr/bin/env python3
"""Exercise production account-scoped creation choices and Previous navigation."""
from pathlib import Path
import subprocess
import tempfile
root=Path(__file__).resolve().parents[1]
source=(root/'src/interpreter.c').read_text()
def function(signature):
    i=source.index(signature);return source[i:source.index('\n}',i)+2]
defines='\n'.join(line for line in (root/'src/structs.h').read_text().splitlines() if line.startswith('#define CON_'))
code=r'''
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <json-c/json.h>
#define CAMPAIGN_DL
#define FALSE false
#define TRUE true
#define eMSDP_CHARACTER_CREATION 0
#define NUM_EXTENDED_RACES 3
#define NUM_CLASSES 5
#define NUM_ALIGNMENTS 3
#define STATE(d) ((d)->state)
#define GET_PREMADE_BUILD_CLASS(ch) ((ch)->premade)
#define GET_REAL_RACE(ch) ((ch)->race)
#define GET_CLASS(ch) ((ch)->classnum)
#define CLSLIST_LOCK(i) (class_list[i].locked)
#define CLSLIST_NAME(i) (class_list[i].name)
#define CONFIG_MENU "1) Enter the game\n2) Character details\n"
struct output_capture { char *text; bool truncated; };
struct variable { bool bReport,bDirty; };
struct protocol { bool bMSDP;struct variable **pVariables; };
struct char_data { struct { char *name; } player;int premade,race,classnum; };
struct descriptor_data { int state;void *account,*str;struct protocol *pProtocol;struct output_capture *output_capture;bool web_character_creation;struct char_data *character; };
static struct { bool is_pc,locked,unlocked;const char *name,*type,*descrip; } race_list[3]={
 {true,false,false,"human","Human","Versatile"},{true,true,false,"irda","Irda","Ancient magic"},{false,false,false,"dragon","Dragon","Not playable"}};
static int race_sort_info[]={0,1,2};
static struct { bool in_game,prestige_class,locked,unlocked,valid;const char *name; } class_list[5]={
 {true,false,false,false,true,"Warrior"},{true,true,false,false,true,"Assassin"},{true,false,true,false,true,"Wizard"},{false,false,false,false,true,"Unavailable"},{true,false,false,false,false,"Restricted"}};
static const char *class_short_descriptions[]={"Martial training","Prestige","Magic","Hidden","Restricted"};
static const char *alignment_names[]={"Good","Neutral","Evil"};
static bool is_locked_race(int i){return race_list[i].locked;}
static bool has_unlocked_race(struct char_data *ch,int i){return race_list[i].unlocked;}
static bool has_unlocked_class(struct char_data *ch,int i){return class_list[i].unlocked;}
static bool valid_class_race_alignment(int i,int race){return class_list[i].valid;}
static bool valid_align_by_class(int i,int classnum){return i!=2;}
static bool valid_align_by_race(int i,int race){return i!=0;}
static int str_cmp(const char *a,const char *b){return strcasecmp(a,b);}
static void write_to_output(struct descriptor_data *d,const char *format,...){
  if(!d->output_capture)return;
  char text[4096];va_list args;va_start(args,format);vsnprintf(text,sizeof(text),format,args);va_end(args);
  free(d->output_capture->text);d->output_capture->text=strdup(text);
}
static void nanny(struct descriptor_data *d,char *input);
static void show_character_rp_menu(struct descriptor_data *d){write_to_output(d,"0) Appearance\n1) Description\n");}
static char *response;static int writes,flushes;
static void MSDPSetString(struct descriptor_data *d,int var,const char *s){writes++;free(response);response=strdup(s);}
static void MSDPFlush(struct descriptor_data *d,int var){assert(d->pProtocol->pVariables[var]->bDirty);flushes++;}
'''+defines+'\n'+r'''
static void nanny(struct descriptor_data *d,char *input){assert(!strcmp(input,"n"));d->state=CON_CHARACTER_DEITY_SELECT;}
'''
for signature in ['bool web_creation_begin(', 'static bool web_creation_can_back(', 'bool web_creation_back(', 'static void creation_choice(', 'void web_creation_finish(']:code+=function(signature)+'\n'
code+=r'''
static struct json_object *field(struct json_object *o,const char *name){struct json_object *v;assert(json_object_object_get_ex(o,name,&v));return v;}
static struct json_object *snapshot(struct descriptor_data *d,int state){
 d->state=state;struct output_capture capture={.text=strdup("Prompt <script> & color \tCquoted\tn" )};d->output_capture=&capture;
 web_creation_finish(d,&capture);assert(!d->output_capture);struct json_object *event=json_tokener_parse(response);assert(event);return event;
}
int main(void){
 struct variable var={.bReport=true};struct variable *variables[]={&var};struct protocol protocol={.bMSDP=true,.pVariables=variables};
 struct char_data ch={.player={.name=strdup("Newname")},.premade=-1};
 struct descriptor_data d={.state=CON_ACCOUNT_MENU,.account=&var,.pProtocol=&protocol,.character=&ch};
 struct output_capture capture={0};
 var.bReport=false;assert(!web_creation_begin(&d,"c",&capture));var.bReport=true;
 assert(!web_creation_begin(&d,"1",&capture));assert(!d.web_character_creation);
 assert(web_creation_begin(&d,"c",&capture));assert(d.web_character_creation && d.output_capture==&capture);d.output_capture=NULL;
 struct json_object *event=snapshot(&d,CON_QRACE),*choices=field(event,"choices");
 assert(json_object_array_length(choices)==1);assert(!strcmp(json_object_get_string(field(json_object_array_get_idx(choices,0),"value")),"human"));json_object_put(event);
 race_list[1].unlocked=true;event=snapshot(&d,CON_QRACE);assert(json_object_array_length(field(event,"choices"))==2);json_object_put(event);
 event=snapshot(&d,CON_QCLASS);assert(json_object_array_length(field(event,"choices"))==1);json_object_put(event);
 class_list[2].unlocked=true;event=snapshot(&d,CON_QCLASS);assert(json_object_array_length(field(event,"choices"))==2);json_object_put(event);
 event=snapshot(&d,CON_CHAR_RP_DECIDE);choices=field(event,"choices");
 assert(!strcmp(json_object_get_string(field(event,"step")),"roleplay"));
 assert(json_object_array_length(choices)==3);
 for(int i=0;i<3;i++){char token[2]={(char)('1'+i),0};assert(!strcmp(json_object_get_string(field(json_object_array_get_idx(choices,i),"value")),token));}
 json_object_put(event);
 event=snapshot(&d,CON_QALIGN);assert(json_object_array_length(field(event,"choices"))==1);json_object_put(event);
 d.state=CON_QALIGN;ch.premade=0;assert(web_creation_back(&d,"@creation-back"));assert(d.state==CON_CONFIRM_PREMADE && ch.premade==-1);
 assert(web_creation_back(&d,"@creation-back"));assert(d.state==CON_QCLASS);
 assert(web_creation_back(&d,"@creation-back"));assert(d.state==CON_QRACE);
 assert(web_creation_back(&d,"@creation-back"));assert(d.state==CON_QSEX);
 assert(web_creation_back(&d,"@creation-back"));assert(d.state==CON_GET_NAME && !ch.player.name);
 d.str=&var;d.state=CON_GEN_DESCS_MENU;assert(!web_creation_can_back(&d));d.str=NULL;
 d.state=CON_CHARACTER_DEITY_SELECT;assert(web_creation_back(&d,"@creation-back"));assert(d.state==CON_CHAR_RP_MENU);
 assert(web_creation_back(&d,"@creation-back"));assert(d.state==CON_MENU);
 d.state=CON_SETPREFS;assert(!web_creation_can_back(&d));
 event=snapshot(&d,CON_PLAYING);assert(!json_object_get_boolean(field(event,"active")) && !d.web_character_creation);json_object_put(event);
 assert(writes==flushes && writes>=6);free(response);
 puts("Web creation: account opt-in, unlocked races/classes, alignment rules, Previous navigation, editor safety and completion passed");
}
'''
with tempfile.TemporaryDirectory(prefix='web-creation-') as temp:
 c=Path(temp)/'creation.c';c.write_text(code);exe=Path(temp)/'creation'
 subprocess.run(['cc','-std=gnu11','-Wall','-Wextra','-Wno-unused-parameter','-fsanitize=address,undefined',str(c),'-ljson-c','-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
