#!/usr/bin/env python3
"""Exercise production ammo HUD serialization, selection and total quantities."""
from pathlib import Path
import subprocess
import tempfile
root = Path(__file__).resolve().parents[1]
def function(file, signature):
    s = (root/file).read_text(); start = s.index(signature)
    return s[start:s.index('\n}', start)+2] + '\n'
# Both refresh paths must publish the same status. Web subscriptions can be
# established after login, so a descriptor-only update silently misses them.
comm = (root/'src/comm.c').read_text()
for signature in ['static void update_msdp_descriptor(struct descriptor_data *d, struct char_data *ch)\n{', 'static void msdp_update(void)\n{']:
    start = comm.index(signature)
    body = comm[start:comm.index('\n}', start) + 2]
    assert 'update_msdp_ammo_status(ch);' in body, signature + ' must refresh ammo status'

code = r'''
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "protocol.h"
#include "assign_wpn_armor.h"
#include "spec_abilities.h"
#include <json-c/json.h>
#include <assert.h>
#define MAX(a,b) ((a)>(b)?(a):(b))
#define act(...) ((void)0)
#define send_to_char(...) ((void)0)
struct weapon_table weapon_list[NUM_WEAPON_TYPES];
struct index_data *obj_index;
obj_rnum top_of_objt=2;
struct obj_data *obj_proto;
static char *response;
static int reports;
bool obj_has_special_ability(struct obj_data *obj,int ability) {return false;}
obj_rnum real_object(obj_vnum vnum) {return vnum>=100 && vnum<=102 ? vnum-100 : NOTHING;}
void MSDPSetString(struct descriptor_data *d,variable_t var,const char *text) {
 assert(var==eMSDP_AMMO_STATUS);free(response);response=strdup(text);reports++;
}
'''
for sig in ['bool is_throwing_weapon(', 'bool ammunition_is_unlimited(', 'int ammunition_remaining(', 'bool usable_ammunition(']:
    code += function('src/assign_wpn_armor.c', sig)
code += function('src/comm.c', 'static void update_msdp_ammo_status(')
code += r'''
static struct json_object *field(struct json_object *o,const char *key) {
 struct json_object *v;assert(json_object_object_get_ex(o,key,&v));return v;
}
static void check(struct char_data *ch,int quantity,const char *name,bool preferred) {
 update_msdp_ammo_status(ch);struct json_object *o=json_tokener_parse(response);assert(o);
 assert(json_object_get_boolean(field(o,"ranged")));
 assert(json_object_get_int(field(o,"quantity"))==quantity);
 assert(!strcmp(json_object_get_string(field(o,"name")),name));
 assert(json_object_get_boolean(field(o,"preferred"))==preferred);json_object_put(o);
}
int main(void) {
 struct char_data player={0};struct player_special_data special={0};struct descriptor_data d={0};
 protocol_t protocol={0};MSDP_t variable={0};MSDP_t *variables[eMSDP_MAX]={0};
 struct obj_data bow={0},pouch={0},a={0},b={0},c={0},prototypes[3]={{0}};
 struct index_data indices[3]={{0}};obj_index=indices;obj_proto=prototypes;
 for(int i=0;i<3;i++)indices[i].vnum=100+i;
 prototypes[0].short_description="arrows";prototypes[1].short_description="bolts";prototypes[2].short_description="steel arrows";
 player.player_specials=&special;player.desc=&d;d.pProtocol=&protocol;protocol.pVariables=variables;
 variables[eMSDP_AMMO_STATUS]=&variable;
 update_msdp_ammo_status(&player);assert(reports==0);
 variable.bReport=true;
 weapon_list[WEAPON_TYPE_SHORT_BOW].weaponFlags=WEAPON_FLAG_RANGED;
 GET_OBJ_TYPE(&bow)=ITEM_WEAPON;GET_OBJ_VAL(&bow,0)=WEAPON_TYPE_SHORT_BOW;
 GET_EQ(&player,WEAR_WIELD_2H)=&bow;GET_EQ(&player,WEAR_AMMO_POUCH)=&pouch;
 GET_OBJ_TYPE(&a)=GET_OBJ_TYPE(&b)=GET_OBJ_TYPE(&c)=ITEM_MISSILE;
 GET_OBJ_RNUM(&a)=GET_OBJ_RNUM(&b)=0;GET_OBJ_RNUM(&c)=1;
 a.short_description=b.short_description="arrows";c.short_description="bolts";
 GET_OBJ_VAL(&a,0)=GET_OBJ_VAL(&b,0)=AMMO_TYPE_ARROW;GET_OBJ_VAL(&c,0)=AMMO_TYPE_BOLT;
 GET_OBJ_VAL(&a,AMMO_VAL_MAX)=GET_OBJ_VAL(&b,AMMO_VAL_MAX)=GET_OBJ_VAL(&c,AMMO_VAL_MAX)=50;
 GET_OBJ_VAL(&a,AMMO_VAL_QUANTITY)=10;GET_OBJ_VAL(&b,AMMO_VAL_QUANTITY)=20;GET_OBJ_VAL(&c,AMMO_VAL_QUANTITY)=40;
 pouch.contains=&c;c.next_content=&a;a.next_content=&b;
 check(&player,30,"arrows",false);
 assert(pouch.contains==&c && c.next_content==&a && a.next_content==&b && !FIRING(&player));
 GET_PREFERRED_AMMO((&player))=103;check(&player,0,"steel arrows",true);
 GET_PREFERRED_AMMO((&player))=101;check(&player,30,"arrows",true);
 GET_OBJ_VAL(&a,AMMO_VAL_QUANTITY)=0;check(&player,20,"arrows",true);
 GET_OBJ_VAL(&b,AMMO_VAL_UNLIMITED)=1;update_msdp_ammo_status(&player);
 struct json_object *o=json_tokener_parse(response);assert(json_object_get_boolean(field(o,"unlimited")));json_object_put(o);
 GET_EQ(&player,WEAR_AMMO_POUCH)=NULL;update_msdp_ammo_status(&player);
 o=json_tokener_parse(response);assert(!json_object_get_boolean(field(o,"pouch")));json_object_put(o);
 GET_EQ(&player,WEAR_WIELD_2H)=NULL;update_msdp_ammo_status(&player);assert(!strcmp(response,"{\"ranged\":false}"));
 GET_EQ(&player,WEAR_WIELD_2H)=&bow;GET_OBJ_VAL(&bow,0)=WEAPON_TYPE_HOOPAK;update_msdp_ammo_status(&player);
 o=json_tokener_parse(response);assert(json_object_get_boolean(field(o,"ranged")));json_object_put(o);
 GET_OBJ_VAL(&bow,0)=NUM_WEAPON_TYPES;update_msdp_ammo_status(&player);assert(!strcmp(response,"{\"ranged\":false}"));
 int before=reports;variable.bReport=false;update_msdp_ammo_status(&player);assert(reports==before);
 free(response);puts("Ammo HUD: matching stack totals, preferred exhaustion, unlimited ammo, missing pouch, weapon removal, hoopak and opt-in passed.");
}
'''
with tempfile.TemporaryDirectory(prefix='ammo-hud-') as directory:
    path=Path(directory)/'test.c'; binary=Path(directory)/'test';path.write_text(code)
    subprocess.run(['cc','-std=gnu99','-I'+str(root/'src'),'-fsanitize=address,undefined',str(path),'-ljson-c','-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
