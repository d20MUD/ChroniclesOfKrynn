#!/usr/bin/env python3
"""Exercise actual corpse event construction and shared MSDP delivery."""
from pathlib import Path
import subprocess, tempfile
root=Path(__file__).resolve().parents[1]
def function(file, signature):
    s=(root/file).read_text();a=s.index(signature+'\n{');return s[a:s.index('\n}',a)+2]
headers='\n'.join('#include "'+h+'"' for h in ['conf.h','sysdep.h','structs.h','utils.h','comm.h','protocol.h','treasure.h'])
stubs=r'''
#include <assert.h>
#include <json-c/json.h>
#undef CAN_SEE_OBJ
#define CAN_SEE_OBJ(ch,obj) (!GET_OBJ_VAL(obj,15))
static char *response;
static int sent;
char *show_pers(struct char_data*c,struct char_data*v) {return "a goblin";}
char *object_short_description(const struct obj_data*o) {return o->short_description;}
void MSDPSetString(struct descriptor_data*d,variable_t var,const char*s) {
 assert(!d->output_capture && var==eMSDP_LOOT_INFO);free(response);response=strdup(s);
}
void MSDPFlush(struct descriptor_data*d,variable_t var) {assert(var==eMSDP_LOOT_INFO);sent++;}
'''
code='\n'.join(function('src/comm.c',sig) for sig in ['bool web_info_begin(struct char_data *ch, int variable, struct output_capture *capture)','void web_info_field(struct char_data *ch, const char *section, const char *label, const char *format, ...)','void web_info_item(struct char_data *ch, const char *section, struct obj_data *obj)','void web_info_finish(struct char_data *ch, int variable, struct output_capture *capture, const char *topic)'])
code+='\n'+function('src/treasure.c','void web_notify_corpse_loot(struct char_data *recipient, struct char_data *mob, struct obj_data *corpse)')
checks=r'''
int main(void) {
 struct char_data ch={0},mob={0};struct descriptor_data d={0};protocol_t protocol={0};
 MSDP_t variable={0};MSDP_t *variables[eMSDP_MAX]={0};
 struct obj_data corpse={0},visible={0},hidden={0},money={0};
 ch.desc=&d;d.pProtocol=&protocol;protocol.pVariables=variables;
 variables[eMSDP_LOOT_INFO]=&variable;protocol.bMSDP=true;variable.bReport=true;
 SET_BIT_AR(MOB_FLAGS(&mob),MOB_ISNPC);
 GET_ID(&visible)=12345;visible.short_description="\tcA fine blade\tn";GET_OBJ_TYPE(&visible)=ITEM_WEAPON;
 hidden.short_description="hidden";GET_OBJ_VAL(&hidden,15)=1;
 GET_OBJ_TYPE(&money)=ITEM_MONEY;
 corpse.contains=&visible;visible.next_content=&hidden;hidden.next_content=&money;
 web_notify_corpse_loot(&ch,&mob,&corpse);assert(sent==1 && !d.output_capture);
 struct json_object *event=json_tokener_parse(response),*fields,*topic,*id;
 assert(json_object_object_get_ex(event,"fields",&fields) && json_object_array_length(fields)==2);
 assert(strstr(response,"A fine blade") && !strstr(response,"hidden"));
 assert(strstr(response,"collect to receive") && strstr(response,"a goblin"));
 assert(strstr(response,"\"loreTarget\":\"}o12345\""));
 assert(corpse.contains==&visible && visible.next_content==&hidden);
 assert(json_object_object_get_ex(event,"topic",&topic) && !strcmp(json_object_get_string(topic),"Corpse loot"));
 assert(json_object_object_get_ex(event,"id",&id));char *first=strdup(json_object_get_string(id));json_object_put(event);
 web_notify_corpse_loot(&ch,&mob,&corpse);assert(sent==2);
 event=json_tokener_parse(response);assert(json_object_object_get_ex(event,"id",&id));assert(strcmp(first,json_object_get_string(id)));free(first);json_object_put(event);
 variable.bReport=false;web_notify_corpse_loot(&ch,&mob,&corpse);assert(sent==2);
 variable.bReport=true;corpse.contains=&money;web_notify_corpse_loot(&ch,&mob,&corpse);assert(sent==2 && !d.output_capture);
 corpse.contains=NULL;web_notify_corpse_loot(&ch,&mob,&corpse);assert(sent==2);
 REMOVE_BIT_AR(MOB_FLAGS(&mob),MOB_ISNPC);corpse.contains=&visible;
 web_notify_corpse_loot(&ch,&mob,&corpse);assert(sent==2);
 web_notify_corpse_loot(NULL,&mob,&corpse);assert(sent==2);
 free(response);return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='loot-notifications-') as tmp:
    p=Path(tmp)/'check.c';b=Path(tmp)/'check';p.write_text(headers+'\n'+stubs+code+checks)
    subprocess.run(['cc','-std=gnu99','-I'+str(root/'src'),str(p),'-ljson-c','-o',str(b)],check=True)
    subprocess.run([str(b)],check=True)
print('Loot notifications: visible corpse items, native colors, source, unique events, opt-in, empty corpses and player exclusion passed.')
