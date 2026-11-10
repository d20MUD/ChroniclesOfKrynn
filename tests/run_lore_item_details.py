#!/usr/bin/env python3
"""Exercise canonical item lore sections, universal spells, and identify procs."""
from pathlib import Path
import subprocess
import tempfile
root=Path(__file__).resolve().parents[1]
items=(root/'src/act.item.c').read_text()
def function(file,signature):
    text=(root/file).read_text();start=text.index(signature)
    return text[start:text.index('\n}',start)+2]+'\n'
values=function('src/act.item.c','void display_item_object_values(')
# Every item-type branch and the universal spell/proc tail must be included,
# not a second subset of item data with a growing list of omissions.
assert values.index('web_info_output_mark(ch)') < values.index('switch (GET_OBJ_TYPE(item))')
assert values.rindex('web_info_output_section(ch, "Item details"') > values.index('(name)(ch, item, 0, "identify")')
stat=function('src/act.item.c','void do_stat_object(')
assert stat.index('"Properties", lore_section_start') > stat.index('Special Timer %d')
assert stat.index('"Effects and bonuses", lore_section_start') > stat.index('feat_specific_name(')
assert stat.index('"Effects and bonuses", lore_section_start') > stat.index('APPLY_SPELL_POTENCY')

headers=['conf.h','sysdep.h','structs.h','utils.h','comm.h','db.h','spells.h','act.h','item.h','protocol.h']
code='\n'.join('#include "'+header+'"' for header in headers)+r'''
#include <json-c/json.h>
#include <assert.h>
#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))
struct spell_info_type spell_info[NUM_SPELLS+1];
struct index_data *obj_index;
static int terminal_writes;
'''
for signature in ['static void capture_output_printf(', 'void web_info_field(', 'size_t web_info_output_mark(', 'void web_info_output_section(']:
    code+=function('src/comm.c',signature)
code+=r'''
size_t send_to_char(struct char_data *ch,const char *format,...) {
 va_list args;va_start(args,format);
 if(ch->desc && ch->desc->output_capture)capture_output_printf(ch->desc->output_capture,format,args);
 else terminal_writes++;
 va_end(args);return 1;
}
void send_to_group(struct char_data *ch,struct group_data *group,const char *format,...) {assert(0);}
static int identify_proc(struct char_data *ch,void *obj,int cmd,const char *argument) {
 assert(!strcmp(argument,"identify"));
 send_to_char(ch,"Unique identify information: 100%% magical.\r\n");return 1;
}
static void universal_details(struct char_data *ch,struct obj_data *item,int mode) {
 int i;int (*name)(struct char_data *,void *,int,const char *);
 size_t lore_section_start=web_info_output_mark(ch);
'''
a=values.index('  /* gear spells */')
b=values.rindex('  if (mode == ITEM_STAT_MODE_LORE_SKILL)')
code+=values[a:b]+'''  if(mode==ITEM_STAT_MODE_LORE_SKILL)web_info_output_section(ch,"Item details",lore_section_start);\n}\n'''
code+=r'''
static struct json_object *last_value(struct output_capture *capture) {
 struct json_object *fields,*value;
 assert(json_object_object_get_ex(capture->web_data,"fields",&fields));
 assert(json_object_object_get_ex(json_object_array_get_idx(fields,json_object_array_length(fields)-1),"value",&value));return value;
}
int main(void) {
 struct char_data ch={0};struct descriptor_data descriptor={0};struct output_capture capture={0};
 struct obj_data item={0};struct index_data index[1]={{0}};obj_index=index;index[0].func=identify_proc;
 ch.desc=&descriptor;descriptor.output_capture=&capture;
 capture.web_data=json_object_new_object();json_object_object_add(capture.web_data,"fields",json_object_new_array());
 spell_info[SPELL_MAGIC_MISSILE].name="magic missile";spell_info[SPELL_HEAL].name="heal";
 GET_OBJ_TYPE(&item)=ITEM_WORN;GET_OBJ_RNUM(&item)=0;
 item.has_spells=true;GET_WEAPON_SPELL(&item,0)=SPELL_MAGIC_MISSILE;
 item.wpn_spells[0].level=12;GET_WEAPON_SPELL_PCT(&item,0)=25;GET_WEAPON_SPELL_AGG(&item,0)=1;
 item.activate_spell[ACT_SPELL_SPELLNUM]=SPELL_HEAL;item.activate_spell[ACT_SPELL_LEVEL]=15;
 item.activate_spell[ACT_SPELL_MAX_USES]=3;item.activate_spell[ACT_SPELL_CURRENT_USES]=2;item.activate_spell[ACT_SPELL_COOLDOWN]=7;
 send_to_char(&ch,"Unrelated prefix that must not be copied.\r\n");
 universal_details(&ch,&item,ITEM_STAT_MODE_LORE_SKILL);
 const char *value=json_object_get_string(last_value(&capture));
 assert(strstr(value,"Gear Spells:") && strstr(value,"magic missile, Level: 12, Percent: 25, Procs in combat?: Yes"));
 assert(strstr(value,"'heal' spell at level 15 with 2/3 uses remaining"));
 assert(strstr(value,"less than 7 minutes") && strstr(value,"activate 'heal' (target)"));
 assert(strstr(value,"Unique identify information: 100% magical.") && !strstr(value,"Unrelated prefix"));
 assert(strstr(capture.text,"Unrelated prefix") && strstr(capture.text,value));
 item.activate_spell[ACT_SPELL_CURRENT_USES]=item.activate_spell[ACT_SPELL_COOLDOWN]=0;
 universal_details(&ch,&item,ITEM_STAT_MODE_LORE_SKILL);
 value=json_object_get_string(last_value(&capture));assert(strstr(value,"3/3 uses remaining"));
 assert(!strstr(value,"less than"));
 /* Spellbook/proc sections can be much larger than a normal printf field. */
 size_t start=web_info_output_mark(&ch);char long_text[12001];memset(long_text,'x',12000);long_text[12000]=0;
 send_to_char(&ch,"%s",long_text);web_info_output_section(&ch,"Item details",start);
 assert(json_object_get_string_len(last_value(&capture))==12000);
 json_object_put(capture.web_data);free(capture.text);descriptor.output_capture=NULL;
 web_info_output_section(&ch,"Item details",0);assert(terminal_writes==0);
 puts("Lore item details: complete switch coverage, gear spells, activated spell uses/cooldown/command, identify procs, long sections and unchanged full output passed.");
}
'''
with tempfile.TemporaryDirectory(prefix='lore-item-details-') as directory:
    path=Path(directory)/'check.c';binary=Path(directory)/'check';path.write_text(code)
    subprocess.run(['cc','-std=gnu99','-I'+str(root/'src'),'-fsanitize=address,undefined',str(path),'-ljson-c','-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
