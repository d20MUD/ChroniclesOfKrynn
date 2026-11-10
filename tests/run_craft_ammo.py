#!/usr/bin/env python3
"""Exercise production ammunition recipes and finite object construction."""
from pathlib import Path
import os, re, subprocess, tempfile
root = Path(__file__).resolve().parents[1]
s = (root/'src/crafting_new.c').read_text()
def function(signature):
    start=s.index(signature+'\n{')
    return s[start:s.index('\n}',start)+2]
headers='\n'.join('#include "'+h+'"' for h in ['conf.h','sysdep.h','structs.h','utils.h','comm.h','db.h','constants.h','crafting_new.h','assign_wpn_armor.h','spells.h','interpreter.h','vnums.h'])
defines='\n'.join(re.findall(r'^#define CRAFT_MOTES_REQ_.*$',s,re.M))+'\n#define CREATE_BASE_DC 10\n#define CREATE_BASE_EXP 50\n'
players=(root/'src/players.c').read_text();a=players.index('static void load_ammo_craft_settings(struct char_data *ch, const char *line)\n{');loader=players[a:players.index('\n}',a)+2]
code=s[s.index('#define CRAFT_AMMO_BATCH_SIZE'):s.index('static bool obj_has_catalyst_target(',s.index('#define CRAFT_AMMO_BATCH_SIZE'))]
stubs=r'''
#include <assert.h>
char *CAP(char *s) {if(*s) *s=toupper((unsigned char)*s);return s;}
#undef HAS_FEAT
#define HAS_FEAT(ch,f) 0
#undef CAN_CARRY_N
#define CAN_CARRY_N(ch) 100
#undef CAN_CARRY_W
#define CAN_CARRY_W(ch) 1000
#undef WAIT_STATE
#define WAIT_STATE(ch,p) ((void)0)

const char *crafting_motes[NUM_CRAFT_MOTES];
const char *ability_names[NUM_ABILITIES];
#undef GET_OBJ_VNUM
#define GET_OBJ_VNUM(o) ((o)->item_number)
static bool tool=true,station=true,roll=true,missing=false;
static int saves,crashes,removed;
static struct obj_data *delivered;
size_t send_to_char(struct char_data*c,const char*f,...) {return 0;}
bool player_has_supply_order(struct char_data*c) {return false;}
static bool craft_has_pending_noncreate_project(struct char_data*c) {return false;}
bool is_wearing_tool_for_crafting_ability(struct char_data*c,int a) {return tool;}
bool has_crafting_station_in_room(struct char_data*c,int a) {return station;}
const char *get_crafting_station_name(int a) {return "station";}
int get_craft_skill_value(struct char_data*c,int a) {return 100;}
int get_proficient_talent_bonus(struct char_data*c,int a) {return 0;}
int get_elbow_grease_bonus(struct char_data*c) {return 0;}
int get_jack_of_all_trades_bonus(struct char_data*c) {return 0;}
static bool artificer_efficient_crafter_applies(struct char_data*c) {return false;}
bool create_craft_skill_check(struct char_data*c,struct obj_data*o,int sk,char*m,int xp,int dc) {return roll;}
void gain_craft_exp(struct char_data*c,int x,int sk,bool happy) {}
void obj_to_char(struct obj_data*o,struct char_data*c) {delivered=o;}
char *object_short_description(const struct obj_data*o) {return o->short_description;}
void save_char(struct char_data*c,int r) {saves++;}
void Crash_crashsave(struct char_data*c) {crashes++;}
void extract_obj(struct obj_data*o) {removed++;free(o->special_abilities);free(o->name);free(o->short_description);free(o->description);free(o);}
int MAX(int a,int b) {return a>b?a:b;}
int is_abbrev(const char*a,const char*b) {return *a && !strncasecmp(a,b,strlen(a));}
int get_level_adjustment_by_apply_and_modifier(int a,int b,int c) {return 0;}
void free_obj_special_abilities(struct obj_special_ability*p) {free(p);}
struct obj_data *read_object(obj_vnum n,int t) {
 if(missing) return NULL; assert(n==AMMO_PROTO || n==40448); struct obj_data *o=calloc(1,sizeof(*o));
 memset(&o->obj_flags,255,sizeof(o->obj_flags));
 memset(o->affected,255,sizeof(o->affected));
 memset(&o->weapon_poison,255,sizeof(o->weapon_poison));
 o->special_abilities=calloc(1,sizeof(*o->special_abilities)); if(n==40448) {memset(&o->obj_flags,0,sizeof(o->obj_flags)); GET_OBJ_TYPE(o)=ITEM_CRAFTING_TOOL; o->item_number=n; o->short_description=strdup("a portable fletching kit");} return o;
}
'''
checks=r'''
int main(void) {
 struct ammo_craft_plan p;
 struct char_data ch={0}; struct player_special_data ps={0};ch.player_specials=&ps;
 GET_POS(&ch)=POS_STANDING;
 load_ammo_craft_settings(&ch,"2 1 2 3 4 1 6");assert(AMMO_SETTINGS(&ch)[0]==2 && AMMO_SETTINGS(&ch)[6]==6);
 const char *bad_saved[]={"", "3 1 2 3 4 1 6", "0 99999999999999999999999 2 3 4 1 6", "0 1 2 3 9 1 6", "0 1 2 3 4 6 6", "0 1 2 3 4 1 7", "0 1 2 3 4 1 6 extra", "0 -1 2 3 4 1 6"};
 for(unsigned i=0;i<sizeof(bad_saved)/sizeof(*bad_saved);i++) {load_ammo_craft_settings(&ch,bad_saved[i]);assert(AMMO_SETTINGS(&ch)[0]==2 && AMMO_SETTINGS(&ch)[6]==6);}
 memset(AMMO_SETTINGS(&ch),0,sizeof(AMMO_SETTINGS(&ch)));
 for(int i=0;i<NUM_CRAFT_MATS;i++) GET_CRAFT_MAT((&ch),i)=1000;
 for(int i=0;i<NUM_CRAFT_MOTES;i++) {crafting_motes[i]="mote";GET_CRAFT_MOTES((&ch),i)=1000;}
 const char *bad[]={"", "arrows -1", "arrows 9", "arrows 2 returning", "arrows 0 unlimited", "arrows 2 fire 7", "arrows 8 fire 4", "arrows 2 fire 6 extra", "arrows 0 none 6", "arrows 99999999999999999999999", "b", "arrows 2x"};
 for(unsigned i=0;i<sizeof(bad)/sizeof(*bad);i++) assert(!plan_ammo_craft(bad[i],&p));
 assert(ammo_material_by_name("ash wood")==CRAFT_MAT_ASH_WOOD);
 assert(ammo_material_by_name("cold iron")==CRAFT_MAT_COLD_IRON);
 const char *types[]={"arrows","bolts","bullets"};
 const char *elements[]={"none","fire","cold","acid","electricity","sonic"};
 for(int t=0;t<3;t++) for(int e=0;e<6;e++) for(int b=0;b<=8;b++) {
   char arg[100]; snprintf(arg,sizeof(arg),"%s %d %s",types[t],b,elements[e]);
   bool valid=plan_ammo_craft(arg,&p);
   assert(valid==(get_level_adjustment_by_enhancement_bonus(b)+(e?6:0)<=30));
   if(!valid) continue;
   assert(p.dc==9+get_level_adjustment_by_enhancement_bonus(b)+(e?6:0));
   int expected[NUM_CRAFT_MOTES]={0};
   if(b) expected[CRAFTING_MOTE_FIRE]+=craft_motes_required(0,0,0,b);
   if(e) expected[p.element->mote]+=craft_motes_required(0,0,0,3);
   assert(!memcmp(expected,p.motes,sizeof(expected)));
   struct obj_data *o=create_crafted_ammo(&p);
   assert(GET_OBJ_TYPE(o)==ITEM_MISSILE && GET_OBJ_VAL(o,0)==p.recipe->type);
   assert(GET_OBJ_VAL(o,4)==b && GET_OBJ_VAL(o,AMMO_VAL_DAMAGE_DIE)==(e?6:0));
   assert(GET_OBJ_VAL(o,AMMO_VAL_MIN)==50 && GET_OBJ_VAL(o,AMMO_VAL_MAX)==50 && GET_OBJ_VAL(o,AMMO_VAL_QUANTITY)==50);
   assert(!GET_OBJ_VAL(o,AMMO_VAL_UNLIMITED) && !GET_OBJ_VAL(o,AMMO_VAL_RETURNING));
   assert(!o->special_abilities && !o->affected[0].modifier);
   extract_obj(o);
 }
 // Each slot improves quality and lowers DC; enchantment and damage increase DC.
 int low[]={CRAFT_MAT_ASH_WOOD,CRAFT_MAT_TIN,CRAFT_MAT_HEMP};
 int high[]={CRAFT_MAT_IRONWOOD,CRAFT_MAT_DRAGONMETAL,CRAFT_MAT_SATIN};
 for(int slot=0;slot<3;slot++) {
   assert(plan_ammo_craft("arrows 4 fire 6",&p));
   memcpy(p.slots,low,sizeof(low));assert(finish_ammo_plan(&p));int base=p.dc;
   p.slots[slot]=high[slot];assert(finish_ammo_plan(&p));assert(p.dc<base);
 }
 assert(plan_ammo_craft("arrows",&p));memcpy(p.slots,high,sizeof(high));assert(finish_ammo_plan(&p));
 assert(p.quality_bonus==4 && p.enhancement==4 && p.dc==5 && !p.motes[CRAFTING_MOTE_FIRE]);
 assert(plan_ammo_craft("arrows",&p)); int base=p.dc;
 assert(plan_ammo_craft("arrows 2",&p) && p.dc>base);base=p.dc;
 assert(plan_ammo_craft("arrows 2 fire 6",&p) && p.dc>base);
 assert(!plan_ammo_craft("arrows material steel",&p));
 assert(plan_ammo_craft("bullets material iron",&p) && p.materials[0][1]==4 && p.materials[1][0]==CRAFT_MAT_HEMP);
 for(int guard=0;guard<5;guard++) {
   tool=guard!=0;station=guard!=1;missing=guard==2;
   if(guard==3) GET_CRAFT_MAT((&ch),CRAFT_MAT_ASH_WOOD)=0;
   if(guard==4) GET_CRAFT_MOTES((&ch),CRAFTING_MOTE_FIRE)=0;
   craft_ammunition(&ch,"arrows 2 fire 6");assert(!saves && !delivered);
   assert(GET_CRAFT_MAT((&ch),CRAFT_MAT_IRON)==1000);
   GET_CRAFT_MAT((&ch),CRAFT_MAT_ASH_WOOD)=1000;GET_CRAFT_MOTES((&ch),CRAFTING_MOTE_FIRE)=1000;
 }
 tool=station=true;missing=false;
 GET_CRAFT_MAT((&ch),CRAFT_MAT_IRON)=3;craft_ammunition(&ch,"bullets material iron");
 assert(!saves && !delivered && GET_CRAFT_MAT((&ch),CRAFT_MAT_IRON)==3);GET_CRAFT_MAT((&ch),CRAFT_MAT_IRON)=1000;
 GET_CRAFT((&ch)).crafting_item_type=CRAFT_TYPE_WEAPON;craft_ammunition(&ch,"arrows");assert(!delivered);
 GET_CRAFT((&ch)).crafting_item_type=CRAFT_TYPE_NONE;
 craft_ammunition(&ch,"type bolts");craft_ammunition(&ch,"material 1 maple wood");craft_ammunition(&ch,"material 2 cold iron");craft_ammunition(&ch,"material 3 wool");
 assert(AMMO_SETTINGS(&ch)[1]==CRAFT_MAT_MAPLE_WOOD && AMMO_SETTINGS(&ch)[2]==CRAFT_MAT_COLD_IRON && AMMO_SETTINGS(&ch)[3]==CRAFT_MAT_WOOL);
 craft_ammunition(&ch,"material 1 iron");assert(AMMO_SETTINGS(&ch)[1]==CRAFT_MAT_MAPLE_WOOD);
 craft_ammunition(&ch,"enhancement 3");craft_ammunition(&ch,"damage fire 6");assert(saved_ammo_plan(&ch,&p));
 int before=GET_CRAFT_MAT((&ch),CRAFT_MAT_WOOL);craft_ammunition(&ch,"begin");
 assert(delivered && GET_CRAFT_MAT((&ch),CRAFT_MAT_WOOL)==before-1);extract_obj(delivered);delivered=NULL;
 roll=false;before=GET_CRAFT_MAT((&ch),CRAFT_MAT_WOOL);craft_ammunition(&ch,"begin");
 assert(!delivered && GET_CRAFT_MAT((&ch),CRAFT_MAT_WOOL)==before-1);roll=true;
 // Portable kit bypasses stations and tools for ammo only; fake names/types cannot.
 struct obj_data kit={0};kit.item_number=40448;GET_OBJ_TYPE(&kit)=ITEM_CRAFTING_TOOL;ch.carrying=&kit;
 tool=station=false;craft_ammunition(&ch,"begin");assert(delivered);extract_obj(delivered);delivered=NULL;
 before=GET_CRAFT_MAT((&ch),CRAFT_MAT_ASH_WOOD);craft_ammunition(&ch,"kit");assert(!delivered && GET_CRAFT_MAT((&ch),CRAFT_MAT_ASH_WOOD)==before);
 kit.item_number=1;craft_ammunition(&ch,"begin");assert(!delivered);ch.carrying=NULL;
 tool=station=true;
 int bronze_before=GET_CRAFT_MAT((&ch),CRAFT_MAT_BRONZE),iron_before=GET_CRAFT_MAT((&ch),CRAFT_MAT_IRON);
 GET_CRAFT_MAT((&ch),CRAFT_MAT_BRONZE)=1;craft_ammunition(&ch,"kit");
 assert(!delivered && GET_CRAFT_MAT((&ch),CRAFT_MAT_ASH_WOOD)==before && GET_CRAFT_MAT((&ch),CRAFT_MAT_IRON)==iron_before);
 GET_CRAFT_MAT((&ch),CRAFT_MAT_BRONZE)=bronze_before;
 craft_ammunition(&ch,"kit");assert(delivered && GET_OBJ_TYPE(delivered)==ITEM_CRAFTING_TOOL);
 assert(GET_CRAFT_MAT((&ch),CRAFT_MAT_BRONZE)==bronze_before-2);
 assert(GET_CRAFT_MAT((&ch),CRAFT_MAT_IRON)==iron_before);
 assert(GET_CRAFT_MAT((&ch),CRAFT_MAT_ASH_WOOD)==before-2);extract_obj(delivered);delivered=NULL;
 craft_ammunition(&ch,"reset");assert(!AMMO_SETTINGS(&ch)[0] && !AMMO_SETTINGS(&ch)[3] && !AMMO_SETTINGS(&ch)[5]);
 return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='craft-ammo-') as tmp:
    p=Path(tmp)/'test.c'; binary=Path(tmp)/'test'
    native='\n'.join(function(sig) for sig in ['int canonical_craft_material(int material)', 'int material_grade(int material)', 'int craft_group_by_material(int material)', 'int craft_material_to_obj_material(int craftmat)'])
    constants=(root/'src/constants.c').read_text(); a=constants.index('const char *crafting_materials[]'); names=constants[a:constants.index(';',a)+1]
    helpers=(root/'src/helpers.c').read_text();a=helpers.index('void half_chop_c('); chop=helpers[a:helpers.index('\n}',a)+2]
    for sig in ['const char *any_one_arg_c(', 'void skip_spaces_c(']:
        source=helpers if sig in helpers else (root/'src/utils.c').read_text();a=source.index(sig);chop=source[a:source.index('\n}',a)+2]+chop
    p.write_text(headers+'\n'+defines+stubs+names+chop+native+function('int get_level_adjustment_by_enhancement_bonus(int bonus_amt)')+function('int craft_motes_required(int location, int modifier, int bonus_type, int enhancement)')+'\n'+code+loader+checks)
    subprocess.run([os.environ.get('CC','cc'),'-fsanitize=address','-g','-std=gnu99','-I'+str(root/'src'),str(p),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
print('Ammo crafting: recipes, bounds, elemental costs, enhancement, and material quality and finite template sanitization, guards, and success/failure resource consumption passed.')
