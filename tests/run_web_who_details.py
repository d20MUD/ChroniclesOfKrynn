#!/usr/bin/env python3
"""Test production structured Who staff titles and happy-hour fields."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1]
s=(root/'src/act.informative.c').read_text()
a=s.index('static void web_who_staff_title(');b=s.index('ACMD(do_who_text)',a)
code=r'''
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include <json-c/json.h>
#include <assert.h>
struct happyhour happy_data;
int next_tick=30;
int MAX(int a,int b){return a>b?a:b;}
static struct json_object *fields;
void web_info_field(struct char_data *ch,const char *section,const char *label,const char *fmt,...){
 char value[128];va_list args;va_start(args,fmt);vsnprintf(value,sizeof(value),fmt,args);va_end(args);
 assert(!strcmp(section,"Happy hour"));json_object_object_add(fields,label,json_object_new_string(value));
}
'''+s[a:b]+r'''
int main(void){
 struct char_data ch={0};struct json_object *player=json_object_new_object(),*value;
 GET_LEVEL(&ch)=LVL_IMMORT;GET_IMM_TITLE(&ch)="\tCCaretaker\tn";
 web_who_staff_title(player,&ch,false);assert(json_object_object_get_ex(player,"staffTitle",&value));assert(!strcmp(json_object_get_string(value),GET_IMM_TITLE(&ch)));json_object_put(player);
 player=json_object_new_object();web_who_staff_title(player,&ch,true);assert(!json_object_object_get_ex(player,"staffTitle",&value));
 GET_LEVEL(&ch)=1;web_who_staff_title(player,&ch,false);assert(!json_object_object_get_ex(player,"staffTitle",&value));json_object_put(player);
 fields=json_object_new_object();web_who_happyhour(&ch);assert(!json_object_object_length(fields));
 HAPPY_EXP=50;HAPPY_TIME=2;web_who_happyhour(&ch);assert(json_object_object_length(fields)==2);
 assert(json_object_object_get_ex(fields,"Experience",&value) && !strcmp(json_object_get_string(value),"+50%"));
 assert(json_object_object_get_ex(fields,"Time remaining",&value) && !strcmp(json_object_get_string(value),"0h 1m 45s"));json_object_put(fields);
 fields=json_object_new_object();HAPPY_TIME=0;web_who_happyhour(&ch);assert(!json_object_object_length(fields));
 HAPPY_TIME=1;HAPPY_EXP=0;HAPPY_CRAFTING_EXP=25;web_who_happyhour(&ch);assert(json_object_object_length(fields)==2);
 assert(json_object_object_get_ex(fields,"Crafting experience",&value));json_object_put(fields);
 fields=json_object_new_object();HAPPY_EXP=HAPPY_GOLD=HAPPY_QP=HAPPY_TREASURE=HAPPY_CRAFTING_EXP=HAPPY_HARVESTING_EXP=HAPPY_HARVESTING_MATERIALS=HAPPY_HARVESTING_MOTES_CHANCE=HAPPY_HARVESTING_MOTES_OBTAINED=10;
 web_who_happyhour(&ch);assert(json_object_object_length(fields)==10);json_object_put(fields);
 return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='web-who-') as tmp:
 source=Path(tmp)/'test.c';binary=Path(tmp)/'test';source.write_text(code)
 subprocess.run(['cc','-fsanitize=address','-g','-std=gnu99','-I'+str(root/'src'),str(source),'-ljson-c','-o',str(binary)],check=True)
 subprocess.run([str(binary)],check=True)
print('Who details: colored staff titles, anonymity, mortals, inactive/expired happy hour, crafting-only bonuses, all nine bonuses and remaining time passed.')
