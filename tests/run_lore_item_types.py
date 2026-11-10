#!/usr/bin/env python3
"""Exercise the lore modal's exact weapon, armor piece and shield types."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1]
s=(root/'src/act.item.c').read_text()
def function(signature):
 a=s.index(signature);return s[a:s.index('\n}',a)+2]
headers='\n'.join('#include "'+h+'"' for h in ['conf.h','sysdep.h','structs.h','utils.h','assign_wpn_armor.h'])
code=headers+r'''
#include <assert.h>
#include <stdarg.h>
struct weapon_table weapon_list[NUM_WEAPON_TYPES];
struct armor_table armor_list[NUM_SPEC_ARMOR_TYPES];
const char *item_types[NUM_ITEM_TYPES]={[ITEM_WEAPON]="Weapon",[ITEM_ARMOR]="Armor",[ITEM_CONTAINER]="Container"};
static char output[256];
void web_info_field(struct char_data *ch,const char *section,const char *label,const char *format,...) {
 assert(!strcmp(section,"Item") && !strcmp(label,"Type"));
 va_list args;va_start(args,format);vsnprintf(output,sizeof(output),format,args);va_end(args);
}
'''+function('static const char *lore_item_type(')
a=s.index('    web_info_field(ch, "Item", "Type",');b=s.index('\n',a)
code+='\nstatic void item_type_field(struct obj_data *j) {struct char_data *ch=NULL;\n'+s[a:b]+'\n}\n'
code+=r'''
int main(void) {
 struct obj_data obj={0};
 weapon_list[WEAPON_TYPE_SHORT_BOW].name="short bow";
 weapon_list[WEAPON_TYPE_LONG_SWORD].name="long sword";
 armor_list[SPEC_ARMOR_TYPE_LIGHT_CHAIN_ARMS].name="light chainmail sleeves";
 armor_list[SPEC_ARMOR_TYPE_BUCKLER].name="buckler shield";
 GET_OBJ_TYPE(&obj)=ITEM_WEAPON;GET_OBJ_VAL(&obj,0)=WEAPON_TYPE_SHORT_BOW;
 item_type_field(&obj);assert(!strcmp(output,"short bow"));
 GET_OBJ_VAL(&obj,0)=WEAPON_TYPE_LONG_SWORD;item_type_field(&obj);assert(!strcmp(output,"long sword"));
 GET_OBJ_TYPE(&obj)=ITEM_ARMOR;GET_OBJ_VAL(&obj,1)=SPEC_ARMOR_TYPE_LIGHT_CHAIN_ARMS;
 item_type_field(&obj);assert(!strcmp(output,"light chainmail sleeves"));
 GET_OBJ_VAL(&obj,1)=SPEC_ARMOR_TYPE_BUCKLER;item_type_field(&obj);assert(!strcmp(output,"buckler shield"));
 GET_OBJ_VAL(&obj,1)=-1;item_type_field(&obj);assert(!strcmp(output,"Armor"));
 GET_OBJ_VAL(&obj,1)=NUM_SPEC_ARMOR_TYPES;item_type_field(&obj);assert(!strcmp(output,"Armor"));
 GET_OBJ_TYPE(&obj)=ITEM_WEAPON;GET_OBJ_VAL(&obj,0)=NUM_WEAPON_TYPES;
 item_type_field(&obj);assert(!strcmp(output,"Weapon"));
 GET_OBJ_TYPE(&obj)=ITEM_CONTAINER;item_type_field(&obj);assert(!strcmp(output,"Container"));
 puts("PASS: lore modal exact weapon, armor piece and shield types, generic item fallback and invalid subtype bounds.");
}
'''
with tempfile.TemporaryDirectory(prefix='lore-item-types-') as directory:
 p=Path(directory);(p/'test.c').write_text(code)
 subprocess.run(['cc','-std=gnu99','-g','-fsanitize=address,undefined','-fno-pie','-no-pie','-I'+str(root/'src'),str(p/'test.c'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
