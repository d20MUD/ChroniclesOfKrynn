#!/usr/bin/env python3
"""Audit the native reforge command against the complete object instance."""
from pathlib import Path
import re, subprocess, tempfile
root=Path(__file__).resolve().parents[1]
def function(file,signature):
    text=(root/'src'/file).read_text()
    start=text.index(signature+'\n{')
    return text[start:text.index('\n}',start)+2]
headers=''.join(f'#include "{h}"\n' for h in ['conf.h','sysdep.h','structs.h','utils.h','comm.h','db.h','interpreter.h','handler.h','craft.h','crafting_new.h','assign_wpn_armor.h','treasure.h','legendary_equipment.h','spells.h'])
code=r'''
#include <assert.h>
struct config_data config_info;
struct player_special_data dummy_mob;
struct weapon_table weapon_list[NUM_WEAPON_TYPES];
struct armor_table armor_list[NUM_SPEC_ARMOR_TYPES];
static int saves, events;
static struct obj_data *same_object;
void basic_mud_log(const char *fmt, ...) {assert(0);}
bool legendary_tree_present(const struct obj_data *obj) {return false;}
bool legendary_tree_check(struct char_data *ch,struct obj_data *obj,enum legendary_route route) {return true;}
size_t send_to_char(struct char_data *ch,const char *fmt,...) {return 0;}
const char *act(const char *s,int hide,struct char_data *ch,struct obj_data *obj,void *vict,int type) {return s;}
void save_char(struct char_data *ch,int mode) {assert(ch->carrying==same_object);saves++;}
void Crash_crashsave(struct char_data *ch) {assert(ch->carrying==same_object);saves++;}
char *object_short_description(const struct obj_data *obj) {return obj->short_description;}
int material_type_to_crafting_skill(int material) {return 1;}
bool has_crafting_station_in_room(struct char_data *ch,int skill) {return true;}
const char *get_crafting_station_name(int skill) {return "station";}
int isname(const char *name,const char *list) {return !strcmp(name,list);}
int is_abbrev(const char *a,const char *b) {return !strcasecmp(a,b);}
void half_chop(char *input,char *first,char *rest) {
  char *space=strchr(input,' ');assert(space);memcpy(first,input,space-input);first[space-input]=0;strcpy(rest,space+1);
}
#define NEW_EVENT(a,b,c,d) (++events)
'''
for file,signature in [('assign_wpn_armor.c','bool is_legacy_composite_bow(int weapon_type)'),('assign_wpn_armor.c','int current_weapon_type(int weapon_type)')]:
    # These stable type helpers may use a different parameter name; find declaration.
    text=(root/'src'/file).read_text()
    name=signature.split('(')[0].split()[-1]
    match=re.search(r'^(?:bool|int) '+name+r'\([^\n]*\)',text,re.M)
    assert match,name
    code+=function(file,match.group())+'\n'
for signature in ['void set_weapon_object_for_restore(struct obj_data *obj, int type)','void set_weapon_object(struct obj_data *obj, int type)','void set_armor_object(struct obj_data *obj, int type)']:
    code+=function('treasure.c',signature)+'\n'
code+=function('crafting_new.c','static char *replace_substring_ci(const char *src, const char *find, const char *repl)')+'\n'
code+=function('crafting_new.c','static void impl_do_reforge_new_(struct char_data *ch, char *argument,\n                                 int cmd __attribute__((unused)),\n                                 int subcmd __attribute__((unused)))')+'\n'
code+=r'''
int main(void) {
  struct char_data ch={0};struct player_special_data ps={0};
  struct obj_data obj={0},before,expected;
  int i,kind;
  ch.player_specials=&ps;CONFIG_CRAFTING_SYSTEM=CRAFTING_SYSTEM_MOTES;
  for(i=0;i<NUM_WEAPON_TYPES;i++)weapon_list[i].name="unused";
  weapon_list[WEAPON_TYPE_LONG_BOW]=(struct weapon_table){.name="long bow",.numDice=2,.diceSize=6,.cost=50,.weight=12,.size=2,.material=MATERIAL_IRON};
  for(i=0;i<NUM_SPEC_ARMOR_TYPES;i++)armor_list[i].name="unused";
  armor_list[1]=(struct armor_table){.name="armor",.armorBonus=7,.cost=70,.weight=18,.material=MATERIAL_IRON,.wear=ITEM_WEAR_BODY};
  for(kind=0;kind<2;kind++) {
    memset(&obj,0,sizeof(obj));
    obj.name=strdup("item");obj.short_description=strdup("a blade of shining yellow light");obj.description=strdup("A blade of shining yellow light lies here.");
    obj.restring_identifier=strdup("blade");
    obj.action_description="Original action description";obj.arcane_mark="the smith";
    obj.item_number=4214;obj.id=12345;obj.estate_delivery_id=987;
    obj.has_spells=true;obj.tinker_bonus=6;
    /* Object 4214's K record: level-10 daylight, two maximum uses. */
    obj.activate_spell[ACT_SPELL_LEVEL]=10;
    obj.activate_spell[ACT_SPELL_SPELLNUM]=SPELL_DAYLIGHT;
    obj.activate_spell[ACT_SPELL_MAX_USES]=2;
    obj.activate_spell[ACT_SPELL_CURRENT_USES]=1;
    obj.activate_spell[ACT_SPELL_COOLDOWN]=7;
    for(i=0;i<MAX_WEAPON_SPELLS;i++)obj.wpn_spells[i]=(struct weapon_spells){i+1,17,25,1,7};
    for(i=0;i<MAX_WEAPON_CHANNEL_SPELLS;i++)obj.channel_spells[i]=(struct weapon_spells){i+3,12,50,0,8};
    obj.weapon_poison=(struct obj_weapon_poison){3,7,12};
    for(i=0;i<MAX_OBJ_AFFECT;i++)obj.affected[i]=(struct obj_affected_type){1,i+2,3,4};
    for(i=0;i<NUM_OBJ_VAL_POSITIONS;i++)GET_OBJ_VAL((&obj),i)=i+8;
    for(i=0;i<SPEC_TIMER_MAX;i++)obj.obj_flags.spec_timer[i]=i+9;
    obj.special_abilities=(void *)&obj;obj.script=(void *)&ch;obj.events=(void *)&ps;
    GET_OBJ_TYPE((&obj))=kind?ITEM_ARMOR:ITEM_WEAPON;
    GET_OBJ_VAL((&obj),0)=kind?0:WEAPON_TYPE_LONG_SWORD;GET_OBJ_VAL((&obj),1)=0;
    GET_OBJ_COST((&obj))=1000;GET_OBJ_WEIGHT((&obj))=5;GET_OBJ_MATERIAL((&obj))=MATERIAL_BRONZE;
    GET_OBJ_BOUND_ID((&obj))=77;GET_OBJ_LEVEL((&obj))=19;
    SET_BIT_AR(GET_OBJ_EXTRA((&obj)),ITEM_REFORGEABLE);
    SET_BIT_AR(GET_OBJ_WEAR((&obj)),kind?ITEM_WEAR_BODY:ITEM_WEAR_WIELD);
    ch.carrying=&obj;obj.carried_by=&ch;same_object=&obj;
    GET_GOLD((&ch))=10000;IS_CARRYING_W((&ch))=105;
    before=obj;
    char argument[64];strcpy(argument,kind?"item armor":"item long bow");
    impl_do_reforge_new_(&ch,argument,0,0);
    assert(GET_CRAFTING_OBJ((&ch))==&obj && GET_CRAFTING_TYPE((&ch))==SCMD_REFORGE);
    assert(GET_GOLD((&ch))==9500);
    assert(IS_CARRYING_W((&ch))==100+GET_OBJ_WEIGHT((&obj)));
    assert(!strcmp(obj.restring_identifier,kind?"armor":"long bow"));
    assert(strstr(obj.short_description,kind?"armor of shining yellow light":"long bow of shining yellow light"));
    expected=before;
    expected.name=obj.name;expected.short_description=obj.short_description;
    expected.description=obj.description;expected.restring_identifier=obj.restring_identifier;
    /* Only base geometry, wear placement and compatible material may change. */
    expected.obj_flags.weight=obj.obj_flags.weight;
    expected.obj_flags.size=obj.obj_flags.size;
    memcpy(expected.obj_flags.wear_flags,obj.obj_flags.wear_flags,sizeof(obj.obj_flags.wear_flags));
    expected.obj_flags.value[0]=obj.obj_flags.value[0];
    expected.obj_flags.value[1]=obj.obj_flags.value[1];
    if(!kind)expected.obj_flags.value[2]=obj.obj_flags.value[2];
    assert(!memcmp(&obj,&expected,sizeof(obj)));
    assert(obj.activate_spell[ACT_SPELL_SPELLNUM]==SPELL_DAYLIGHT && obj.activate_spell[ACT_SPELL_CURRENT_USES]==1 && obj.activate_spell[ACT_SPELL_COOLDOWN]==7);
    free(obj.name);free(obj.short_description);free(obj.description);free(obj.restring_identifier);
  }
  assert(saves==4 && events==2);
  puts("Reforge: complete instance preserved for weapon/armor, including activate charges/cooldown, proc/channel spells, effects, identity and pointers; carrier weight updated.");
}
'''
with tempfile.TemporaryDirectory(prefix='krynn-reforge-') as directory:
    path=Path(directory);(path/'test.c').write_text(headers+code)
    subprocess.run(['cc','-std=gnu99','-Wall','-Wextra','-Werror','-Wno-unused-parameter','-fsanitize=address,undefined','-fno-pie','-no-pie','-I'+str(root/'src'),str(path/'test.c'),'-o',str(path/'test')],check=True)
    subprocess.run([str(path/'test')],check=True)
