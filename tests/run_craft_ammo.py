#!/usr/bin/env python3
"""Exercise production ammunition recipes and finite object construction."""
from pathlib import Path
import os, re, subprocess, tempfile
root = Path(__file__).resolve().parents[1]
s = (root/'src/crafting_new.c').read_text()
def function(signature):
    start=s.index(signature+'\n{')
    return s[start:s.index('\n}',start)+2]
headers='\n'.join('#include "'+h+'"' for h in ['conf.h','sysdep.h','structs.h','utils.h','comm.h','db.h','constants.h','crafting_new.h','assign_wpn_armor.h','spells.h','interpreter.h','vnums.h','talents.h'])
defines='\n'.join(re.findall(r'^#define CRAFT_MOTES_REQ_.*$',s,re.M))+'\n#define CREATE_BASE_DC 10\n#define CREATE_BASE_EXP 50\n#define CREATE_BASE_TIME 60\n'
players=(root/'src/players.c').read_text();a=players.index('static void load_ammo_craft_settings(struct char_data *ch, const char *line)\n{');loader=players[a:players.index('\n}',a)+2]
code=s[s.index('#define CRAFT_AMMO_BATCH_SIZE'):s.index('static bool obj_has_catalyst_target(',s.index('#define CRAFT_AMMO_BATCH_SIZE'))]
stubs=r'''
#include <assert.h>
struct player_special_data dummy_mob;
void basic_mud_log(const char *f,...) {assert(0);}
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
struct weapon_table weapon_list[NUM_WEAPON_TYPES];
#undef GET_OBJ_VNUM
#define GET_OBJ_VNUM(o) ((o)->item_number)
static bool tool=true,station=true,roll=true,missing=false;
static int saves,crashes,removed,rolls,rapid,natural_roll=10,check_skill=100;
static int efficient,expertise,proficient,percent_roll=100,last_xp_skill;
int get_efficient_talent_bonus(struct char_data *ch,int skill) {return efficient;}
int get_talent_rank(struct char_data *ch,int talent) {
 return talent==TALENT_WOODWORKING_EXPERTISE||talent==TALENT_WEAPONSMITHING_EXPERTISE?expertise:0;
}
int rand_number(int low,int high){assert(low==1&&high==100);return percent_roll;}
int get_rapid_talent_bonus(struct char_data*c,int sk) {return rapid;}
const char *act(const char*s,int h,struct char_data*c,struct obj_data*o,void*v,int t) {return s;}
static struct obj_data *delivered;
size_t send_to_char(struct char_data*c,const char*f,...) {return 0;}
bool player_has_supply_order(struct char_data*c) {return false;}
static bool craft_has_pending_noncreate_project(struct char_data*c) {return false;}
bool is_wearing_tool_for_crafting_ability(struct char_data*c,int a) {return tool;}
bool has_crafting_station_in_room(struct char_data*c,int a) {return station;}
const char *get_crafting_station_name(int a) {return "station";}
int get_craft_production_bonus(struct char_data *ch,int skill) {return 0;}
int get_craft_skill_value(struct char_data*c,int a) {return check_skill;}
int get_proficient_talent_bonus(struct char_data*c,int a) {return proficient;}
int get_elbow_grease_bonus(struct char_data*c) {return 0;}
int get_jack_of_all_trades_bonus(struct char_data*c) {return 0;}
static bool artificer_efficient_crafter_applies(struct char_data*c) {return false;}
int d20(struct char_data*c) {rolls++;return natural_roll;}
void reset_current_craft(struct char_data*c,char*a,bool v,bool r) {assert(!r);GET_CRAFT(c).crafting_method=0;GET_CRAFT(c).craft_duration=0;}
void gain_craft_exp(struct char_data*c,int x,int sk,bool happy) {last_xp_skill=sk;}
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
 if(missing) return NULL; assert(n==AMMO_PROTO || n==FLETCHING_KIT_VNUM); struct obj_data *o=calloc(1,sizeof(*o));
 memset(&o->obj_flags,255,sizeof(o->obj_flags));
 memset(o->affected,255,sizeof(o->affected));
 memset(&o->weapon_poison,255,sizeof(o->weapon_poison));
 o->special_abilities=calloc(1,sizeof(*o->special_abilities)); if(n==FLETCHING_KIT_VNUM) {memset(&o->obj_flags,0,sizeof(o->obj_flags)); GET_OBJ_TYPE(o)=ITEM_OTHER; o->item_number=n; o->short_description=strdup("a portable fletching kit");} return o;
}
'''
checks=r'''
static void craft_and_wait(struct char_data *ch, const char *argument) {
 int attempts=rolls;
 craft_ammunition(ch,argument);
 if(GET_CRAFT(ch).crafting_method==SCMD_NEWCRAFT_AMMO) {
   assert(GET_CRAFT(ch).craft_duration==60);
   assert(!delivered && rolls==attempts);
   int settings[7];memcpy(settings,AMMO_SETTINGS(ch),sizeof(settings));
   memset(AMMO_SETTINGS(ch),0,sizeof(settings)); /* settings changes cannot alter pending job */
   GET_CRAFT(ch).craft_duration=0;
   craft_ammunition_complete(ch);
   assert(GET_CRAFT(ch).crafting_method==0 && rolls==attempts+1);
   int completed_rolls=rolls;craft_ammunition_complete(ch);assert(rolls==completed_rolls);
   memcpy(AMMO_SETTINGS(ch),settings,sizeof(settings));
 }
}

int main(void) {
 struct ammo_craft_plan p;
 assert(NUM_AMMO_CRAFT_RECIPES==9);
 struct char_data ch={0}; struct player_special_data ps={0};ch.player_specials=&ps;
 GET_POS(&ch)=POS_STANDING;
 load_ammo_craft_settings(&ch,"8 4 4 3 2 1 6");assert(AMMO_SETTINGS(&ch)[0]==8);
 load_ammo_craft_settings(&ch,"2 1 2 3 4 1 6");assert(AMMO_SETTINGS(&ch)[0]==2 && AMMO_SETTINGS(&ch)[6]==6);
 const char *bad_saved[]={"", "9 1 2 3 4 1 6", "0 99999999999999999999999 2 3 4 1 6", "0 1 2 3 9 1 6", "0 1 2 3 4 6 6", "0 1 2 3 4 1 7", "0 1 2 3 4 1 6 extra", "0 -1 2 3 4 1 6"};
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
 // Crafted throwing bundles use real native weapon profiles and finite ammo.
 const char *thrown_names[]={"knives","axes","hammers","javelins","darts","shuriken"};
 const int thrown_types[]={WEAPON_TYPE_THROWING_KNIFE,WEAPON_TYPE_THROWING_AXE,WEAPON_TYPE_THROWING_HAMMER,WEAPON_TYPE_JAVELIN,WEAPON_TYPE_DART,WEAPON_TYPE_SHURIKEN};
 for(int t=0;t<6;t++) {
   char arg[100];snprintf(arg,sizeof(arg),"%s 2 fire 6",thrown_names[t]);assert(plan_ammo_craft(arg,&p));
   assert(p.recipe->weapon_type==thrown_types[t]);
   weapon_list[thrown_types[t]].weaponFlags=WEAPON_FLAG_THROWN;
   weapon_list[thrown_types[t]].numDice=1;weapon_list[thrown_types[t]].diceSize=6;
   struct obj_data *o=create_crafted_ammo(&p),profile={0};
   assert(o && GET_OBJ_TYPE(o)==ITEM_MISSILE && GET_OBJ_VAL(o,AMMO_VAL_WEAPON_TYPE)==thrown_types[t]);
   assert(thrown_weapon_type(o)==thrown_types[t] && make_thrown_weapon_profile(o,&profile));
   assert(GET_OBJ_TYPE(&profile)==ITEM_WEAPON && GET_OBJ_VAL(&profile,0)==thrown_types[t]);
   assert(GET_OBJ_VAL(o,AMMO_VAL_QUANTITY)==50 && !GET_OBJ_VAL(o,AMMO_VAL_UNLIMITED) && !GET_OBJ_VAL(o,AMMO_VAL_RETURNING));
   assert(GET_OBJ_VAL(o,4)==p.enhancement && GET_OBJ_VAL(o,AMMO_VAL_DAMAGE_DIE)==6);
   extract_obj(o);
 }
 assert(plan_ammo_craft("shuriken",&p));assert(p.materials[0][0]==CRAFT_MAT_IRON && p.materials[0][1]==4);
 assert(!ammo_material_valid(p.recipe,0,CRAFT_MAT_ASH_WOOD));
 assert(!ammo_material_valid(p.recipe,1,CRAFT_MAT_GOLD));
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
 // Every graded wood, hard metal and cloth remains selectable in its slot.
 for(int material=1;material<NUM_CRAFT_MATS;material++) {
   if(material_grade(material)<1) continue;
   int group=craft_group_by_material(material),slot=-1;
   if(group==CRAFT_GROUP_WOOD)slot=0;
   if(group==CRAFT_GROUP_HARD_METALS)slot=1;
   if(group==CRAFT_GROUP_CLOTH)slot=2;
   if(slot<0)continue;
   assert(plan_ammo_craft("arrows",&p));p.slots[slot]=material;
   assert(finish_ammo_plan(&p) && p.slots[slot]==material);
 }
 assert(plan_ammo_craft("arrows",&p)); int base=p.dc;
 assert(plan_ammo_craft("arrows 2",&p) && p.dc>base);base=p.dc;
 assert(plan_ammo_craft("arrows 2 fire 6",&p) && p.dc>base);
 assert(!plan_ammo_craft("arrows material steel",&p));
 assert(plan_ammo_craft("bullets material iron",&p) && p.materials[0][1]==4 && p.materials[1][0]==CRAFT_MAT_HEMP);
 for(int guard=0;guard<5;guard++) {
   tool=guard!=0;station=guard!=1;missing=guard==2;
   if(guard==3) GET_CRAFT_MAT((&ch),CRAFT_MAT_ASH_WOOD)=0;
   if(guard==4) GET_CRAFT_MOTES((&ch),CRAFTING_MOTE_FIRE)=0;
   craft_and_wait(&ch,"arrows 2 fire 6");assert(!saves && !delivered);
   assert(GET_CRAFT_MAT((&ch),CRAFT_MAT_IRON)==1000);
   GET_CRAFT_MAT((&ch),CRAFT_MAT_ASH_WOOD)=1000;GET_CRAFT_MOTES((&ch),CRAFTING_MOTE_FIRE)=1000;
 }
 tool=station=true;missing=false;
 GET_CRAFT_MAT((&ch),CRAFT_MAT_IRON)=3;craft_and_wait(&ch,"bullets material iron");
 assert(!saves && !delivered && GET_CRAFT_MAT((&ch),CRAFT_MAT_IRON)==3);GET_CRAFT_MAT((&ch),CRAFT_MAT_IRON)=1000;
 GET_CRAFT((&ch)).crafting_item_type=CRAFT_TYPE_WEAPON;craft_and_wait(&ch,"arrows");assert(!delivered);
 GET_CRAFT((&ch)).crafting_item_type=CRAFT_TYPE_NONE;
 craft_and_wait(&ch,"type bolts");craft_and_wait(&ch,"material 1 maple wood");craft_and_wait(&ch,"material 2 cold iron");craft_and_wait(&ch,"material 3 wool");
 assert(AMMO_SETTINGS(&ch)[1]==CRAFT_MAT_MAPLE_WOOD && AMMO_SETTINGS(&ch)[2]==CRAFT_MAT_COLD_IRON && AMMO_SETTINGS(&ch)[3]==CRAFT_MAT_WOOL);
 craft_and_wait(&ch,"material 1 iron");assert(AMMO_SETTINGS(&ch)[1]==CRAFT_MAT_MAPLE_WOOD);
 craft_and_wait(&ch,"enhancement 3");craft_and_wait(&ch,"damage fire 6");assert(saved_ammo_plan(&ch,&p));
 // Bare ammo type commands must use saved materials/magic, not demand iron.
 int recipe_settings[7];memcpy(recipe_settings,AMMO_SETTINGS(&ch),sizeof(recipe_settings));
 int iron_stock=GET_CRAFT_MAT((&ch),CRAFT_MAT_IRON),ash_stock=GET_CRAFT_MAT((&ch),CRAFT_MAT_ASH_WOOD);
 GET_CRAFT_MAT((&ch),CRAFT_MAT_IRON)=0;GET_CRAFT_MAT((&ch),CRAFT_MAT_ASH_WOOD)=0;
 int maple_stock=GET_CRAFT_MAT((&ch),CRAFT_MAT_MAPLE_WOOD),cold_stock=GET_CRAFT_MAT((&ch),CRAFT_MAT_COLD_IRON),wool_stock=GET_CRAFT_MAT((&ch),CRAFT_MAT_WOOL);
 assert(saved_ammo_plan_for_recipe(&ch,0,&p) && p.slots[0]==CRAFT_MAT_MAPLE_WOOD && p.slots[1]==CRAFT_MAT_COLD_IRON && p.slots[2]==CRAFT_MAT_WOOL);
 craft_and_wait(&ch,"arrows");assert(delivered && GET_OBJ_VAL(delivered,0)==AMMO_TYPE_ARROW && GET_OBJ_VAL(delivered,4)==p.enhancement && GET_OBJ_VAL(delivered,AMMO_VAL_DAMAGE_DIE)==6);
 assert(GET_CRAFT_MAT((&ch),CRAFT_MAT_MAPLE_WOOD)==maple_stock-2 && GET_CRAFT_MAT((&ch),CRAFT_MAT_COLD_IRON)==cold_stock-1 && GET_CRAFT_MAT((&ch),CRAFT_MAT_WOOL)==wool_stock-1);
 assert(GET_CRAFT_MAT((&ch),CRAFT_MAT_IRON)==0 && GET_CRAFT_MAT((&ch),CRAFT_MAT_ASH_WOOD)==0);
 extract_obj(delivered);delivered=NULL;
 // Bolts and bullets use exactly the same saved-recipe path as arrows.
 for(int recipe=1;recipe<=2;recipe++) {
   assert(saved_ammo_plan_for_recipe(&ch,recipe,&p));int stock[3];
   for(int m=0;m<3;m++)stock[m]=GET_CRAFT_MAT((&ch),p.materials[m][0]);
   craft_and_wait(&ch,recipe==1?"bolts":"bullets");assert(delivered && GET_OBJ_VAL(delivered,0)==p.recipe->type);
   for(int m=0;m<3;m++)assert(GET_CRAFT_MAT((&ch),p.materials[m][0])==stock[m]-p.materials[m][1]);
   assert(GET_OBJ_VAL(delivered,4)==p.enhancement && GET_OBJ_VAL(delivered,AMMO_VAL_DAMAGE_DIE)==6);
   assert(GET_CRAFT_MAT((&ch),CRAFT_MAT_IRON)==0);extract_obj(delivered);delivered=NULL;
 }
 craft_and_wait(&ch,"cost arrows");assert(!delivered && !memcmp(recipe_settings,AMMO_SETTINGS(&ch),sizeof(recipe_settings)));
 assert(saved_ammo_plan_for_recipe(&ch,2,&p) && p.slots[0]==CRAFT_MAT_STONE && p.slots[1]==CRAFT_MAT_COLD_IRON && p.slots[2]==CRAFT_MAT_WOOL);
 assert(!memcmp(recipe_settings,AMMO_SETTINGS(&ch),sizeof(recipe_settings)));
 // An unavailable explicit choice must not silently spend a different metal.
 AMMO_SETTINGS(&ch)[2]=CRAFT_MAT_TIN;int tin_stock=GET_CRAFT_MAT((&ch),CRAFT_MAT_TIN);GET_CRAFT_MAT((&ch),CRAFT_MAT_TIN)=0;
 maple_stock=GET_CRAFT_MAT((&ch),CRAFT_MAT_MAPLE_WOOD);craft_and_wait(&ch,"arrows");
 assert(!delivered && GET_CRAFT_MAT((&ch),CRAFT_MAT_MAPLE_WOOD)==maple_stock);
 GET_CRAFT_MAT((&ch),CRAFT_MAT_TIN)=tin_stock;memcpy(AMMO_SETTINGS(&ch),recipe_settings,sizeof(recipe_settings));
 GET_CRAFT_MAT((&ch),CRAFT_MAT_IRON)=iron_stock;GET_CRAFT_MAT((&ch),CRAFT_MAT_ASH_WOOD)=ash_stock;
 int before=GET_CRAFT_MAT((&ch),CRAFT_MAT_WOOL);craft_and_wait(&ch,"begin");
 assert(delivered && GET_CRAFT_MAT((&ch),CRAFT_MAT_WOOL)==before-1);extract_obj(delivered);delivered=NULL;
 roll=false;natural_roll=2;assert(saved_ammo_plan(&ch,&p));check_skill=p.dc-20;before=GET_CRAFT_MAT((&ch),CRAFT_MAT_WOOL);craft_and_wait(&ch,"begin");
 assert(!delivered && GET_CRAFT_MAT((&ch),CRAFT_MAT_WOOL)==before);roll=true;natural_roll=10;check_skill=100;
 // Portable kit bypasses stations and tools for ammo only; fake names/types cannot.
 struct obj_data kit={0};kit.item_number=FLETCHING_KIT_VNUM;GET_OBJ_TYPE(&kit)=ITEM_CRAFTING_TOOL;ch.carrying=&kit;
 tool=station=false;craft_and_wait(&ch,"begin");assert(delivered);extract_obj(delivered);delivered=NULL;
 GET_OBJ_TYPE(&kit)=ITEM_OTHER;craft_and_wait(&ch,"begin");assert(delivered);extract_obj(delivered);delivered=NULL;
 GET_OBJ_TYPE(&kit)=ITEM_WEAPON;craft_and_wait(&ch,"begin");assert(!delivered);
 GET_OBJ_TYPE(&kit)=ITEM_OTHER;
 before=GET_CRAFT_MAT((&ch),CRAFT_MAT_ASH_WOOD);craft_and_wait(&ch,"kit");assert(!delivered && GET_CRAFT_MAT((&ch),CRAFT_MAT_ASH_WOOD)==before);
 kit.item_number=1;craft_and_wait(&ch,"begin");assert(!delivered);ch.carrying=NULL;
 tool=station=true;
 int bronze_before=GET_CRAFT_MAT((&ch),CRAFT_MAT_BRONZE),iron_before=GET_CRAFT_MAT((&ch),CRAFT_MAT_IRON);
 GET_CRAFT_MAT((&ch),CRAFT_MAT_BRONZE)=1;craft_and_wait(&ch,"kit");
 assert(!delivered && GET_CRAFT_MAT((&ch),CRAFT_MAT_ASH_WOOD)==before && GET_CRAFT_MAT((&ch),CRAFT_MAT_IRON)==iron_before);
 GET_CRAFT_MAT((&ch),CRAFT_MAT_BRONZE)=bronze_before;
 craft_and_wait(&ch,"kit");assert(delivered && GET_OBJ_TYPE(delivered)==ITEM_OTHER && GET_OBJ_VNUM(delivered)==FLETCHING_KIT_VNUM);
 assert(GET_CRAFT_MAT((&ch),CRAFT_MAT_BRONZE)==bronze_before-2);
 assert(GET_CRAFT_MAT((&ch),CRAFT_MAT_IRON)==iron_before);
 assert(GET_CRAFT_MAT((&ch),CRAFT_MAT_ASH_WOOD)==before-2);extract_obj(delivered);delivered=NULL;
 // Actual native d20 outcomes: failure refunds all, natural 1 loses all,
 // natural 20 produces 75 finite uses and charges ceil(cost/2) per material/mote.
 for(int recipe=0;recipe<NUM_AMMO_CRAFT_RECIPES;recipe++) {
   char command[100];snprintf(command,sizeof(command),"type %s",ammo_craft_recipes[recipe].name);
   craft_and_wait(&ch,command);craft_and_wait(&ch,"enhancement 3");craft_and_wait(&ch,"damage fire 6");
   const char *mote_names[]={"air","dark","earth","fire","ice","light","lightning","water"};
   snprintf(command,sizeof(command),"mote %s",mote_names[recipe%8]);
   craft_ammunition(&ch,command);
   assert(saved_ammo_plan(&ch,&p));
   assert(p.enhancement_mote==recipe%8+1);
   for(int outcome=0;outcome<3;outcome++) {
     int stock[NUM_CRAFT_MATS],motes[NUM_CRAFT_MOTES];
     for(int m=0;m<NUM_CRAFT_MATS;m++)stock[m]=GET_CRAFT_MAT((&ch),m)=1000;
     for(int m=0;m<NUM_CRAFT_MOTES;m++)motes[m]=GET_CRAFT_MOTES((&ch),m)=1000;
     natural_roll=outcome==0?2:outcome==1?1:20;
     check_skill=outcome==0?p.dc-20:100;
     craft_and_wait(&ch,"begin");
     assert((delivered!=NULL)==(outcome==2));
     for(int m=0;m<3;m++) {
       int cost=outcome==0?0:outcome==1?p.materials[m][1]:(p.materials[m][1]+1)/2;
       if(p.materials[m][1])assert(GET_CRAFT_MAT((&ch),p.materials[m][0])==stock[p.materials[m][0]]-cost);
     }
     for(int m=1;m<NUM_CRAFT_MOTES;m++) {
       int cost=outcome==0?0:outcome==1?p.motes[m]:(p.motes[m]+1)/2;
       assert(GET_CRAFT_MOTES((&ch),m)==motes[m]-cost);
     }
     if(delivered) {
       assert(GET_OBJ_VAL(delivered,AMMO_VAL_QUANTITY)==75);
       assert(GET_OBJ_VAL(delivered,AMMO_VAL_MIN)==75&&GET_OBJ_VAL(delivered,AMMO_VAL_MAX)==75);
       assert(!GET_OBJ_VAL(delivered,AMMO_VAL_UNLIMITED));extract_obj(delivered);delivered=NULL;
     }
   }
 }
 natural_roll=10;check_skill=100;
 // Throwing recipes persist their selections and consume real materials.
 for(int t=0;t<6;t++) {
   char command[100];snprintf(command,sizeof(command),"type %s",thrown_names[t]);craft_and_wait(&ch,command);
   assert(AMMO_SETTINGS(&ch)[0]==t+3 && saved_ammo_plan(&ch,&p));
   int stock[3];for(int m=0;m<3;m++)stock[m]=GET_CRAFT_MAT((&ch),p.materials[m][0]);
   craft_and_wait(&ch,"begin");assert(delivered && thrown_weapon_type(delivered)==thrown_types[t]);
   for(int m=0;m<3;m++)assert(GET_CRAFT_MAT((&ch),p.materials[m][0])==stock[m]-p.materials[m][1]);
   extract_obj(delivered);delivered=NULL;
 }
 /* Starting work does not roll or deliver; duplicate starts cannot spend again. */
 int old_rolls=rolls;
 int original_materials[NUM_CRAFT_MATS],original_motes[NUM_CRAFT_MOTES];
 for(int m=0;m<NUM_CRAFT_MATS;m++)original_materials[m]=GET_CRAFT_MAT((&ch),m);
 for(int m=0;m<NUM_CRAFT_MOTES;m++)original_motes[m]=GET_CRAFT_MOTES((&ch),m);
 craft_ammunition(&ch,"arrows");
 assert(GET_CRAFT((&ch)).craft_duration==60 && !delivered && rolls==old_rolls);
 craft_ammunition_complete(&ch);assert(!delivered && rolls==old_rolls);
 int pending[8];memcpy(pending,GET_CRAFT((&ch)).ammo_job,sizeof(pending));
 int remaining_wood=GET_CRAFT_MAT((&ch),pending[1]);
 craft_ammunition(&ch,"bolts");
 assert(!memcmp(pending,GET_CRAFT((&ch)).ammo_job,sizeof(pending)));
 assert(GET_CRAFT_MAT((&ch),pending[1])==remaining_wood);
 craft_ammunition(&ch,"cancel");
 assert(GET_CRAFT((&ch)).craft_duration==0 && GET_CRAFT((&ch)).crafting_method==0);
 for(int m=0;m<NUM_CRAFT_MATS;m++)assert(GET_CRAFT_MAT((&ch),m)==original_materials[m]);
 for(int m=0;m<NUM_CRAFT_MOTES;m++)assert(GET_CRAFT_MOTES((&ch),m)==original_motes[m]);
 craft_ammunition(&ch,"cancel");
 for(int m=0;m<NUM_CRAFT_MATS;m++)assert(GET_CRAFT_MAT((&ch),m)==original_materials[m]);
 craft_ammunition_complete(&ch);assert(!delivered && rolls==old_rolls);
 rapid=15;craft_ammunition(&ch,"arrows");assert(GET_CRAFT((&ch)).craft_duration==45);
 craft_ammunition(&ch,"cancel");
 rapid=100;craft_ammunition(&ch,"kit");assert(GET_CRAFT((&ch)).craft_duration==1);
 craft_ammunition(&ch,"cancel");rapid=0;
 for(int m=0;m<NUM_CRAFT_MATS;m++)assert(GET_CRAFT_MAT((&ch),m)==original_materials[m]);
 for(int m=0;m<NUM_CRAFT_MOTES;m++)assert(GET_CRAFT_MOTES((&ch),m)==original_motes[m]);
 craft_and_wait(&ch,"reset");assert(!AMMO_SETTINGS(&ch)[0] && !AMMO_SETTINGS(&ch)[3] && !AMMO_SETTINGS(&ch)[5]);
 // Enhancement currency is independent of all five locked elemental costs.
 const char *mote_names[]={"air","dark","earth","fire","ice","light","lightning","water"};
 for(int mote=1;mote<NUM_CRAFT_MOTES;mote++)for(int element=0;element<6;element++) {
   char command[100];craft_ammunition(&ch,"reset");
   snprintf(command,sizeof(command),"mote %s",mote_names[mote-1]);craft_ammunition(&ch,command);
   craft_ammunition(&ch,"enhancement 2");
   snprintf(command,sizeof(command),"damage %s",ammo_craft_elements[element].name);craft_ammunition(&ch,command);
   assert(saved_ammo_plan(&ch,&p));assert(p.enhancement_mote==mote);
   int enhancement_cost=craft_motes_required(0,0,0,2);
   int damage_cost=element?craft_motes_required(0,0,0,3):0;
   for(int m=1;m<NUM_CRAFT_MOTES;m++) {
     assert(p.motes[m]==(m==mote?enhancement_cost:0)+(element&&m==ammo_craft_elements[element].mote?damage_cost:0));
     GET_CRAFT_MOTES((&ch),m)=1000;
   }
   for(int m=0;m<NUM_CRAFT_MATS;m++)GET_CRAFT_MAT((&ch),m)=1000;
   assert(character_ammo_plan(&ch,"bolts 2 fire 6",&p)&&p.enhancement_mote==mote);
   craft_ammunition(&ch,"begin");
   assert(GET_CRAFT((&ch)).ammo_job_enhancement_mote==mote);
   if(mote==CRAFTING_MOTE_FIRE)GET_CRAFT((&ch)).ammo_job_enhancement_mote=0; // pre-update saved job
   // A later preference change must not change refunds or a restored timed job.
   craft_ammunition(&ch,"mote water");
   assert(load_ammunition_job(&ch,&p)&&p.enhancement_mote==mote);
   assert(AMMO_ENHANCEMENT_MOTE(&ch)==CRAFTING_MOTE_WATER);
   craft_ammunition(&ch,"cancel");
   for(int m=1;m<NUM_CRAFT_MOTES;m++)assert(GET_CRAFT_MOTES((&ch),m)==1000);
 }
 int selected=AMMO_ENHANCEMENT_MOTE(&ch);
 craft_ammunition(&ch,"mote none");assert(AMMO_ENHANCEMENT_MOTE(&ch)==selected);
 craft_ammunition(&ch,"mote invalid");assert(AMMO_ENHANCEMENT_MOTE(&ch)==selected);
 craft_ammunition(&ch,"reset");assert(!AMMO_ENHANCEMENT_MOTE(&ch));
 assert(character_ammo_plan(&ch,"arrows 2",&p)&&p.enhancement_mote==CRAFTING_MOTE_FIRE);
 // Missing crafting talents: success-only savings and Expertise critical batches.
 for(int recipe=0;recipe<NUM_AMMO_CRAFT_RECIPES;recipe++)for(int outcome=0;outcome<6;outcome++) {
   char command[100];craft_ammunition(&ch,"reset");
   snprintf(command,sizeof(command),"type %s",ammo_craft_recipes[recipe].name);craft_ammunition(&ch,command);
   craft_ammunition(&ch,"enhancement 2");craft_ammunition(&ch,"mote earth");craft_ammunition(&ch,"damage fire 6");
   assert(saved_ammo_plan(&ch,&p));
   for(int m=0;m<NUM_CRAFT_MATS;m++)GET_CRAFT_MAT((&ch),m)=1000;
   for(int m=1;m<NUM_CRAFT_MOTES;m++)GET_CRAFT_MOTES((&ch),m)=1000;
   efficient=30;expertise=5;percent_roll=outcome==0?100:outcome==5?10:1;
   natural_roll=outcome==3?1:outcome==4?2:outcome==2?20:10;
   check_skill=outcome==4?p.dc-20:100;
   craft_and_wait(&ch,"begin");
   bool success=outcome<3||outcome==5,critical=outcome==1||outcome==2;
   for(int m=0;m<3;m++)if(p.materials[m][1]) {
     int paid=success||outcome==3?p.materials[m][1]:0;
     if(critical)paid-=paid/2;
     if(success&&percent_roll<=30)paid-=paid/2;
     assert(GET_CRAFT_MAT((&ch),p.materials[m][0])==1000-paid);
   }
   for(int m=1;m<NUM_CRAFT_MOTES;m++) {
     int paid=success||outcome==3?p.motes[m]:0;if(critical)paid-=paid/2;
     assert(GET_CRAFT_MOTES((&ch),m)==1000-paid);
   }
   assert((delivered!=NULL)==success);
   if(delivered) {
     assert(GET_OBJ_VAL(delivered,AMMO_VAL_QUANTITY)==(critical?75:50));
     assert(last_xp_skill==p.recipe->skill);
     extract_obj(delivered);delivered=NULL;
   }
 }
 // Kits use woodworking talents but retain their own critical/failure rules.
 for(int outcome=0;outcome<3;outcome++) {
   craft_ammunition(&ch,"reset");efficient=30;expertise=5;percent_roll=1;
   natural_roll=outcome==1?20:outcome==2?1:10;check_skill=100;
   int mats[]={CRAFT_MAT_ASH_WOOD,CRAFT_MAT_BRONZE,CRAFT_MAT_HEMP};
   for(int i=0;i<3;i++)GET_CRAFT_MAT((&ch),mats[i])=1000;
   craft_and_wait(&ch,"kit");
   for(int i=0;i<3;i++)assert(GET_CRAFT_MAT((&ch),mats[i])==(outcome==2?998:999));
   assert((delivered!=NULL)==(outcome!=2));
   if(delivered){assert(last_xp_skill==ABILITY_CRAFT_WOODWORKING);extract_obj(delivered);delivered=NULL;}
 }
 efficient=expertise=0;check_skill=100;natural_roll=10;percent_roll=100;
 // Proficient counts once in the actual roll, including the pass/fail boundary.
 for(int recipe=0;recipe<NUM_AMMO_CRAFT_RECIPES;recipe++) {
   char command[100];craft_ammunition(&ch,"reset");
   snprintf(command,sizeof(command),"type %s",ammo_craft_recipes[recipe].name);craft_ammunition(&ch,command);
   craft_ammunition(&ch,"enhancement 3");
   assert(saved_ammo_plan(&ch,&p));check_skill=p.dc-11;assert(check_skill>=0);
   for(int bonus=0;bonus<2;bonus++) {
     for(int m=0;m<NUM_CRAFT_MATS;m++)GET_CRAFT_MAT((&ch),m)=1000;
     proficient=bonus;craft_and_wait(&ch,"begin");assert((delivered!=NULL)==(bonus==1));
     if(delivered){extract_obj(delivered);delivered=NULL;}
   }
 }
 proficient=0;check_skill=100;
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
    throwing=(root/'src/assign_wpn_armor.c').read_text(); thrown_functions=''
    for sig in ['int thrown_weapon_type(struct obj_data *obj)', 'struct obj_data *make_thrown_weapon_profile(struct obj_data *source, struct obj_data *profile)']:
        a=throwing.index(sig+'\n{');thrown_functions+=throwing[a:throwing.index('\n}',a)+2]+'\n'
    p.write_text(headers+'\n'+defines+stubs+names+chop+native+thrown_functions+function('int get_level_adjustment_by_enhancement_bonus(int bonus_amt)')+function('int craft_motes_required(int location, int modifier, int bonus_type, int enhancement)')+'\n'+function('static bool create_craft_skill_check_with_roll(struct char_data *ch, struct obj_data *obj, int skill, char *method,\n                              int exp, int dc, int *natural_roll)')+'\n'+code+loader+checks)
    subprocess.run([os.environ.get('CC','cc'),'-fsanitize=address','-g','-std=gnu99','-I'+str(root/'src'),str(p),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
print('Ammo crafting: recipes, bounds, elemental costs, enhancement, and material quality and finite template sanitization, guards, and success/failure resource consumption passed.')
