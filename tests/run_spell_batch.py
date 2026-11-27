#!/usr/bin/env python3
"""Exercise production spell-batch rules and room lifecycle with native structs.

Extract the new rule blocks from magic.c; no duplicated spell formulas in the
adapter. Full renderer/registration checks live in run_spellhelp.py.
"""
from pathlib import Path
import re, subprocess, tempfile
ROOT=Path(__file__).resolve().parents[1]
source=(ROOT/'src/magic.c').read_text()
def function(signature):
    start=source.index(signature+'\n{')
    return source[start:source.index('\n}',start)+2]
a=source.index('  case SPELL_EAR_PIERCING_SCREAM:',source.index('int mag_damage('))
b=source.index('    /*******************************************',a)
cases=source[a:b]
a=source.index('  else if (batch_damage_spell(spellnum))')
b=source.index('  else if (dam && (save != -1))',a)
resolve=source[a:b]
a=source.index('  case PSIONIC_RECONSTRUCTION:',source.index('void mag_points('))
b=source.index('  case PSIONIC_BODY_ADJUSTMENT:',a)
repair=source[a:b]
a=source.index('  case PSIONIC_DESTINY_DISSONANCE:',source.index('void mag_affects_full('))
b=source.index('    /*******************************************',a) if '    /*******************************************' in source[a:a+1800] else -1
# Stop at the next case rather than depending on the neighboring old spell.
b=source.index('\n  case ',a+10)
destiny=source[a:b]
code=r'''
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "spells.h"
#include "handler.h"
#include "fight.h"
#include "dg_scripts.h"
#include "domains_schools.h"
#include "evolutions.h"
#include "psionics.h"
#include "spell_prep.h"
#include "oasis.h"
#include "epic_feats.h"
#include "epic_feats2.h"
#include "craft.h"
#include "assign_wpn_armor.h"
#include <assert.h>
struct spell_info_type spell_info[TOP_SKILL_DEFINE+1];
struct psionic_power_data psionic_powers[MAX_SPELLS+1];
int epic_feat_rank(struct char_data *ch,int f){return 1;}
int d20(struct char_data *ch){return 20;}
void save_char(struct char_data *ch,int mode){}
bool is_a_known_spell(struct char_data *ch,int cls,int spell){return false;}
int compute_powers_circle(int cls,int spell,int meta){return (spell_info[spell].min_level[cls]+1)/2;}
int get_class_highest_circle(struct char_data *ch,int cls){return 9;}
struct room_data rooms[2];struct room_data *world=rooms;room_rnum top_of_world=1;
struct char_data *character_list;
static struct char_data *caster_lookup;
static int incorporeal, death_immune, crit_immune, test_armor_type, drop_calls, inventory_calls, roll_index, roll_count;
static int rolls[4];
static struct affected_type joined[8];
bool is_incorporeal(struct char_data *ch){return incorporeal;}
sbyte is_immune_death_magic(struct char_data *ch,struct char_data *v,sbyte display){return death_immune;}
int is_immune_to_crits(struct char_data *ch,struct char_data *v){return crit_immune;}
int compute_gear_armor_type(struct char_data *ch){return test_armor_type;}
int get_char_affect_modifier(struct char_data *ch,int spell,int loc){
 struct affected_type *a;for(a=ch->affected;a;a=a->next)if(a->spell==spell&&a->location==loc)return a->modifier;return 0;
}
struct obj_data *unequip_char(struct char_data *ch,int slot){struct obj_data *o=GET_EQ(ch,slot);GET_EQ(ch,slot)=NULL;return o;}
void obj_to_room(struct obj_data *o,room_rnum room){drop_calls++;o->in_room=room;}
void obj_to_char(struct obj_data *o,struct char_data *ch){inventory_calls++;o->carried_by=ch;o->in_room=NOWHERE;}

static int last_bit, last_duration, joins, save_calls, touch_roll, resist_power;
static int immune_blind, immune_stun, immune_sick, immune_cold, feat;
static int damage_calls, area_calls, kill_target, healing, save_only, extract_caster;
static struct char_data *excluded;
struct sivak_form *active_sivak_form(struct char_data *ch){return NULL;}
int char_has_evolution(struct char_data *ch,int evo){return 0;}
int get_feat_value(struct char_data *ch,int f){return (f==FEAT_COLD_IMMUNITY&&immune_cold)||f==feat;}
bool can_blind(struct char_data *ch){return !immune_blind;}
bool can_stun(struct char_data *ch){return !immune_stun;}
bool can_nauseate(struct char_data *ch){return !immune_sick;}
bool affected_by_spell(struct char_data *ch,int s){struct affected_type *a;for(a=ch->affected;a;a=a->next)if(a->spell==s)return true;return false;}
int dice(int n,int size){return roll_index<roll_count?rolls[roll_index++]:n*size;}
int MAX(int a,int b){return a>b?a:b;}int MIN(int a,int b){return a<b?a:b;}
void new_affect(struct affected_type *af){memset(af,0,sizeof(*af));}
void affect_join(struct char_data *ch,struct affected_type *af,bool a,bool b,bool c,bool d){
 int i;if(joins<8)joined[joins]=*af;joins++;last_duration=af->duration;
 for(i=0;i<NUM_AFF_FLAGS;i++)if(IS_SET_AR(af->bitvector,i))last_bit=i;
}
size_t send_to_char(struct char_data *ch,const char *s,...){return 0;}
const char *act(const char *s,int hide,struct char_data *ch,struct obj_data *o,void *v,int type){return s;}
int attack_roll(struct char_data *ch,struct char_data *v,int type,int touch,int n){return touch_roll;}
int attack_roll_with_critical(struct char_data *ch,struct char_data *v,int type,int touch,int n,int threshold){return touch_roll;}
bool power_resistance(struct char_data *ch,struct char_data *v,int mod){return resist_power;}
struct char_data *find_char(long id){
 int i;struct char_data *v;
 if(caster_lookup&&GET_ID(caster_lookup)==id)return caster_lookup;
 for(i=0;i<2;i++)for(v=rooms[i].people;v;v=v->next_in_room)if(GET_ID(v)==id)return v;
 return NULL;
}
int aoeOK(struct char_data *ch,struct char_data *v,int spell){return ch!=v&&v!=excluded;}
int damage(struct char_data *ch,struct char_data *victim,int dam,int attacktype,int type,int display){return dam;}
int mag_damage(int level,struct char_data *ch,struct char_data *v,struct obj_data *o,int spell,int meta,int save,int cast){
 damage_calls++;if(extract_caster){caster_lookup=NULL;rooms[0].people=v;character_list=NULL;}if(kill_target)GET_POS(v)=POS_DEAD;return kill_target?-1:1;
}
void mag_areas(int level,struct char_data *ch,struct obj_data *o,int spell,int meta,int save,int cast){area_calls++;}
static bool savingthrow(struct char_data *ch,struct char_data *v,int save,int mod,int cast,int level,int school){save_calls++;return resist_power||save_only;}
'''
# savingthrow is declared in spells.h as a macro; use the production signature.
code=code.replace('static bool savingthrow(', 'bool batch_test_savingthrow(')
code+='\nint savingthrow_full(struct char_data *ch,struct char_data *v,int save,int mod,int cast,int level,int school,int spell){return batch_test_savingthrow(ch,v,save,mod,cast,level,school); }\n'
for sig in ['static bool batch_bone_target(struct char_data *victim, bool carapace)',
            'static int burning_disarm_slot(struct char_data *victim)',
            'static void second_batch_damage_condition(struct char_data *ch, struct char_data *victim,\n                                           int spell, int level, bool saved, bool critical)',
            'static void corrosive_aura_tick(struct char_data *caster)',
            'static void corrosive_aura_update(void)',
            'static int batch_alignment_scale(int spell, struct char_data *victim)',
            'static bool batch_damage_spell(int spell)',
            'static bool batch_area_limit(int spell, int *targets)',
            'static void batch_damage_condition(struct char_data *victim, int spell,\n                                   bool saved, bool critical)',
            'void mag_hail_of_crystals(int level, struct char_data *ch, struct char_data *victim,\n                          struct obj_data *obj, int metamagic, int savetype, int casttype)',
            'static void frost_fall_tick(struct raff_node *raff)']:
    code+='\n'+function(sig)+'\n'
for filename,signature in [('epic_feats.c','int epic_augmentation_discount('),
                           ('epic_feats.c','bool epic_power_choice('),
                           ('epic_feats2.c','int epic_rebuttal_roll(')]:
    text=(ROOT/'src'/filename).read_text();a=text.index(signature)
    code+='\n'+text[a:text.index('\n}',a)+2]+'\n'
code+='''
static int configure(int spellnum,int level,struct char_data *ch,struct char_data *victim,int *out){
 int element=0,save=0,num_dice=0,size_dice=0,bonus=0,mag_resist=TRUE,mag_resist_bonus=0,casttype=CAST_SPELL;
 bool batch_critical=false;
 switch(spellnum){
'''+cases+'''
 default:return 0;}
 out[0]=num_dice;out[1]=size_dice;out[2]=bonus;out[3]=save;out[4]=element;out[5]=mag_resist;
 out[6]=mag_resist_bonus;out[7]=batch_critical;
 return 1;
}
static int resolve_damage(int spellnum,int dam,int save,struct char_data *ch,struct char_data *victim){
 int race_bonus=0,dc_mod=0,casttype=CAST_SPELL,level=10,spell_school=EVOCATION,element=DAM_LIGHT;
 bool batch_critical=false;
 if(false){}
'''+resolve+'''
 return dam;
}
static void repair_plan(int level,struct char_data *ch,struct char_data *victim){
 const char *to_char=NULL,*to_vict=NULL,*to_notvict=NULL;
 healing=0;
 switch(PSIONIC_RECONSTRUCTION){
'''+repair+'''
 }
}
static void destiny_plan(int level,struct char_data *ch,struct char_data *victim){
 struct affected_type af[MAX_SPELL_AFFECTS]={{0}};
 const char *to_vict=NULL,*to_room=NULL;
 int casttype=CAST_SPELL;
 switch(PSIONIC_DESTINY_DISSONANCE){
'''+destiny+'''
 }
 if(af[0].duration)affect_join(victim,&af[0],false,false,false,false);
}
'''
a=source.index('  case PSIONIC_NATURAL_HEALING:',source.index('void mag_points('))
b=source.index('  case PSIONIC_RECONSTRUCTION:',a)
code+='\nstatic void natural_plan(int level,struct char_data *ch,struct char_data *victim){\n const char *to_char=NULL,*to_vict=NULL,*to_notvict=NULL;healing=0;\n switch(PSIONIC_NATURAL_HEALING){\n'+source[a:b]+'\n}}\n'
a=source.index('  case SPELL_BURST_OF_SPEED:',source.index('void mag_affects_full('))
b=source.index('  case PSIONIC_DESTINY_DISSONANCE:',a)
code+='\nstatic void second_affect_plan(int spellnum,int level,struct char_data *ch,struct char_data *victim){\n struct affected_type af[MAX_SPELL_AFFECTS]={{0}};const char *to_vict=NULL,*to_room=NULL;int casttype=CAST_SPELL;af[0].spell=spellnum;\n switch(spellnum){\n'+source[a:b]+'\n}if(af[0].duration)affect_join(victim,&af[0],false,false,false,false);\n}\n'
utils=(ROOT/'src/utils.c').read_text();a=utils.index('bool fortification_blocks(')
code+='\n'+utils[a:utils.index('\n}',a)+2]+'\n'
fight=(ROOT/'src/fight.c').read_text();movement=(ROOT/'src/movement.c').read_text();speed=(ROOT/'src/movement_cost.c').read_text()
a=fight.index('  if (victim && affected_by_spell(victim, PSIONIC_EXPOSE_WEAKNESS))')
b=fight.index('\n\n',a)
code+='\nstatic int expose_confirmation(struct char_data *victim){int confirm_roll=10;\n'+fight[a:b]+'\nreturn confirm_roll;}\n'
a=fight.index('  if (affected_by_spell(ch, PSIONIC_EXPOSE_WEAKNESS) &&')
b=fight.index('\n\n',a)
code+='\nstatic bool expose_flanked(struct char_data *ch){\n'+fight[a:b]+'\nreturn false;}\n'
a=fight.index('  if (((AFF_FLAGGED(ch, AFF_FLAT_FOOTED) ||')
b=fight.index('\n  {',a)
code+='\nbool has_uncanny_dodge(struct char_data *ch){return false;}\nstatic bool expose_dex(struct char_data *ch){\n'+fight[a:b]+'\n{return false;}return true;}\n'
a=movement.index('  if (need_specials_check == 3 && GET_POS(ch) > POS_DEAD &&')
b=movement.index('\n  {',a)
code+='\nstatic bool movement_provokes(struct char_data *ch,int need_specials_check){\n'+movement[a:b]+'\n{return true;}return false;}\n'
a=speed.index('  speed += get_char_affect_modifier(ch, SPELL_BURST_OF_SPEED, APPLY_SPECIAL);')
b=speed.index('\n',a)
x=speed.index('  else if (AFF_FLAGGED(ch, AFF_SLOW))');y=speed.index('  else if (AFF_FLAGGED(ch, AFF_ENTANGLED))',x)
code+='\nstatic int batch_speed(struct char_data *ch){int speed=30;\n'+speed[a:b]+'\nif(false){}\n'+speed[x:y]+'\nreturn speed;}\n'
code+=r'''
static void reset(void){last_bit=-1;joins=save_calls=damage_calls=area_calls=0;feat=0;resist_power=0;}
static void second_batch_checks(void){
 struct char_data c={0},v={0},ally={0};struct player_special_data cp={0},vp={0};
 struct obj_data metal={0},wood={0};struct affected_type aura={0},expose={0};
 int p[8],i;
 c.player_specials=&cp;v.player_specials=&vp;
 SET_BIT_AR(MOB_FLAGS(&c),MOB_ISNPC);SET_BIT_AR(MOB_FLAGS(&v),MOB_ISNPC);
 GET_REAL_RACE(&c)=GET_REAL_RACE(&v)=RACE_TYPE_HUMANOID;
 GET_ID(&c)=101;GET_ID(&v)=102;GET_ID(&ally)=103;
 IN_ROOM(&c)=IN_ROOM(&v)=IN_ROOM(&ally)=0;GET_POS(&c)=GET_POS(&v)=POS_STANDING;
 GET_OBJ_MATERIAL(&metal)=MATERIAL_IRON;GET_OBJ_MATERIAL(&wood)=MATERIAL_WOOD;
 GET_EQ(&v,WEAR_WIELD_1)=&wood;GET_EQ(&v,WEAR_HOLD_1)=&metal;
 assert(burning_disarm_slot(&v)==WEAR_HOLD_1);
 assert(configure(SPELL_BURNING_DISARM,20,&c,&v,p)&&p[0]==5&&p[1]==4&&p[3]==SAVING_REFL);
 SET_BIT_AR(GET_OBJ_EXTRA(&metal),ITEM_NODROP);
 assert(configure(SPELL_BURNING_DISARM,1,&c,&v,p)&&p[0]==1&&p[3]==-1);
 reset();second_batch_damage_condition(&c,&v,SPELL_BURNING_DISARM,1,true,false);
 assert(!drop_calls&&!inventory_calls&&GET_EQ(&v,WEAR_HOLD_1)==&metal);
 REMOVE_BIT_AR(GET_OBJ_EXTRA(&metal),ITEM_NODROP);
 resist_power=1;assert(resolve_damage(SPELL_BURNING_DISARM,20,SAVING_REFL,&c,&v)==0);
 assert(!drop_calls&&inventory_calls==1&&!GET_EQ(&v,WEAR_HOLD_1)&&metal.in_room==NOWHERE&&metal.carried_by==&v);
 assert(!configure(SPELL_BURNING_DISARM,1,&c,&v,p));
 reset();rooms[0].sector_type=SECT_UNDERWATER;
 assert(resolve_damage(SPELL_BURNING_DISARM,20,-1,&c,&v)==10);rooms[0].sector_type=SECT_INSIDE;
 assert(configure(SPELL_BONESHAKER,10,&c,&v,p)&&p[0]==8&&p[1]==6);
 assert(configure(SPELL_BONESHATTER,30,&c,&v,p)&&p[0]==15);
 {int races[]={RACE_TYPE_OOZE,RACE_TYPE_PLANT,RACE_TYPE_ELEMENTAL,RACE_TYPE_CONSTRUCT};
  for(i=0;i<4;i++){GET_REAL_RACE(&v)=races[i];assert(!batch_bone_target(&v,true)&&!batch_bone_target(&v,false));}}
 GET_REAL_RACE(&v)=RACE_TYPE_UNDEAD;assert(!batch_bone_target(&v,false)&&batch_bone_target(&v,true));
 GET_REAL_RACE(&v)=RACE_TYPE_VERMIN;assert(!batch_bone_target(&v,false)&&batch_bone_target(&v,true));
 GET_REAL_RACE(&v)=RACE_TYPE_HUMANOID;incorporeal=1;assert(!batch_bone_target(&v,true));incorporeal=0;
 reset();second_batch_damage_condition(&c,&v,SPELL_BONESHATTER,7,true,false);
 assert(joins==1&&joined[0].modifier==2&&last_duration==70&&last_bit==AFF_FATIGUED);
 reset();second_batch_damage_condition(&c,&v,SPELL_BONESHATTER,7,false,false);
 assert(joins==3&&joined[0].modifier==6&&joined[1].location==APPLY_STR&&joined[1].modifier==-4&&joined[2].location==APPLY_DEX);
 v.affected=&joined[0];reset();second_batch_damage_condition(&c,&v,SPELL_BONESHATTER,7,true,false);assert(!joins);v.affected=NULL;
 touch_roll=-1;assert(!configure(SPELL_STRICKEN_HEART,5,&c,&v,p));
 touch_roll=999;assert(configure(SPELL_STRICKEN_HEART,5,&c,&v,p)&&p[0]==4&&p[7]);
 touch_roll=0;assert(configure(SPELL_STRICKEN_HEART,5,&c,&v,p)&&p[0]==2);
 death_immune=1;assert(!configure(SPELL_STRICKEN_HEART,5,&c,&v,p));death_immune=0;
 reset();second_batch_damage_condition(&c,&v,SPELL_STRICKEN_HEART,5,false,true);assert(last_bit==AFF_STAGGERED&&last_duration==10);
 reset();crit_immune=1;second_batch_damage_condition(&c,&v,SPELL_STRICKEN_HEART,5,false,false);assert(!joins);crit_immune=0;
 GET_AUGMENT_PSP(&c)=0;natural_plan(10,&c,&c);assert(healing==3);
 GET_AUGMENT_PSP(&c)=4;natural_plan(10,&c,&c);assert(healing==15);
 natural_plan(10,&c,&v);assert(!healing);
 GET_REAL_RACE(&c)=RACE_TYPE_CONSTRUCT;natural_plan(10,&c,&c);assert(!healing);GET_REAL_RACE(&c)=RACE_TYPE_HUMANOID;
 reset();test_armor_type=ARMOR_TYPE_LIGHT;second_affect_plan(SPELL_BURST_OF_SPEED,5,&c,&c);assert(joined[0].modifier==20&&last_duration==1);
 reset();test_armor_type=ARMOR_TYPE_MEDIUM;second_affect_plan(SPELL_BURST_OF_SPEED,5,&c,&c);assert(joined[0].modifier==10);
 reset();second_affect_plan(SPELL_BURST_OF_SPEED,5,&c,&v);assert(!joins);
 reset();GET_AUGMENT_PSP(&c)=4;second_affect_plan(PSIONIC_CORROSIVE_AURA,7,&c,&c);
 assert(joined[0].modifier==5&&joined[0].epic_cl==7&&last_duration==7);
 {struct affected_type burst={0},exhaust={0};
 burst.spell=SPELL_BURST_OF_SPEED;burst.location=APPLY_SPECIAL;burst.modifier=20;c.affected=&burst;
 assert(batch_speed(&c)==50&&!movement_provokes(&c,3));burst.modifier=10;assert(batch_speed(&c)==40);
 c.affected=NULL;assert(batch_speed(&c)==30&&movement_provokes(&c,3));
 exhaust.spell=SPELL_BONESHATTER;exhaust.location=APPLY_SPECIAL;exhaust.modifier=6;c.affected=&exhaust;
 assert(batch_speed(&c)==15);SET_BIT_AR(AFF_FLAGS(&c),AFF_SLOW);assert(batch_speed(&c)==15);REMOVE_BIT_AR(AFF_FLAGS(&c),AFF_SLOW);
 exhaust.modifier=2;assert(batch_speed(&c)==30);c.affected=NULL;
 }

 aura=joined[0];c.affected=&aura;GET_AUGMENT_PSP(&c)=0;GET_DC_BONUS(&c)=0;
 assert(configure(PSIONIC_CORROSIVE_AURA_DAMAGE,7,&c,&v,p)&&p[0]==5&&GET_DC_BONUS(&c)==2);
 reset();GET_AUGMENT_PSP(&c)=0;second_affect_plan(PSIONIC_EXPOSE_WEAKNESS,5,&c,&v);assert(joins==1&&joined[0].modifier==0);
 reset();GET_AUGMENT_PSP(&c)=6;second_affect_plan(PSIONIC_EXPOSE_WEAKNESS,5,&c,&v);assert(joins==1&&joined[0].modifier==6&&last_bit==AFF_FLAT_FOOTED);
 expose=joined[0];v.affected=&expose;
 assert(expose_confirmation(&v)==14&&!expose_dex(&v)&&!expose_flanked(&v));
 feat=FEAT_UNCANNY_DODGE;assert(expose_dex(&v));feat=0;
 expose.modifier=0;assert(expose_flanked(&v)&&expose_dex(&v));
 feat=FEAT_IMPROVED_UNCANNY_DODGE;assert(!expose_flanked(&v));feat=0;expose.modifier=6;
GET_AUGMENT_PSP(&c)=0;reset();second_affect_plan(PSIONIC_EXPOSE_WEAKNESS,5,&c,&v);assert(!joins);
 v.affected=NULL;reset();resist_power=1;second_affect_plan(PSIONIC_EXPOSE_WEAKNESS,5,&c,&v);assert(!joins);
 reset();save_only=1;second_affect_plan(PSIONIC_EXPOSE_WEAKNESS,5,&c,&v);assert(!joins&&save_calls==1);save_only=0;
 reset();roll_count=2;roll_index=0;rolls[0]=1;rolls[1]=4;assert(fortification_blocks(&v,4)&&roll_index==1);
 v.affected=&expose;roll_index=0;assert(!fortification_blocks(&v,4)&&roll_index==2);
 roll_index=0;rolls[1]=1;assert(fortification_blocks(&v,4));roll_count=0;v.affected=NULL;
 reset();caster_lookup=&c;character_list=&c;rooms[0].people=&c;c.next_in_room=&v;v.next_in_room=&ally;excluded=&ally;
 cp.casting_class=CLASS_WIZARD;CASTING_CLASS(&c)=CLASS_CLERIC;CASTING_SPELLNUM(&c)=SPELL_FIREBALL;GET_DC_BONUS(&c)=9;
 corrosive_aura_update();assert(damage_calls==1&&cp.casting_class==CLASS_WIZARD&&CASTING_CLASS(&c)==CLASS_CLERIC&&CASTING_SPELLNUM(&c)==SPELL_FIREBALL&&GET_DC_BONUS(&c)==9);
 reset();aura.duration=0;corrosive_aura_tick(&c);assert(!damage_calls);aura.duration=7;
 SET_BIT_AR(rooms[0].room_flags,ROOM_PEACEFUL);corrosive_aura_tick(&c);assert(!damage_calls);REMOVE_BIT_AR(rooms[0].room_flags,ROOM_PEACEFUL);
 SET_BIT_AR(rooms[0].room_flags,ROOM_NOMAGIC);corrosive_aura_tick(&c);assert(!damage_calls);REMOVE_BIT_AR(rooms[0].room_flags,ROOM_NOMAGIC);
 rooms[0].people=NULL;rooms[1].people=&c;IN_ROOM(&c)=IN_ROOM(&v)=IN_ROOM(&ally)=1;
 corrosive_aura_tick(&c);assert(damage_calls==1);
 reset();kill_target=1;corrosive_aura_tick(&c);assert(damage_calls==1);kill_target=0;
 reset();GET_POS(&v)=POS_STANDING;IN_ROOM(&c)=IN_ROOM(&v)=IN_ROOM(&ally)=0;rooms[1].people=NULL;rooms[0].people=&c;
 extract_caster=1;corrosive_aura_update();assert(damage_calls==1);extract_caster=0;rooms[0].people=NULL;
 c.affected=NULL;character_list=caster_lookup=NULL;excluded=NULL;rooms[1].people=NULL;
 puts("PASS: second batch metal equipment/drop protections, anatomy, exhaustion, touch/critical immunities, self healing, armor speed, augmentation, exposure fortification and moving aura/state restoration");
}
int main(void){
 second_batch_checks();reset();
 struct char_data ch={0},v={0},other={0};struct player_special_data cp={0},vp={0};
 struct raff_node patch={0};int p[8],n,i;
 ch.player_specials=&cp;v.player_specials=&vp;GET_ID(&ch)=77;caster_lookup=&ch;
 GET_POS(&ch)=POS_STANDING;GET_POS(&v)=POS_STANDING;IN_ROOM(&ch)=0;
 SET_BIT_AR(MOB_FLAGS(&v),MOB_ISNPC);GET_REAL_RACE(&v)=RACE_TYPE_HUMANOID;
 /* All nine alignment combinations: each burst protects its own axis. */
 for(i=-1;i<=1;i++)for(n=-1;n<=1;n++){
  const int alignment[3][3]={{-900,-700,-500},{-200,0,200},{500,700,900}};
  GET_ALIGNMENT(&v)=alignment[i+1][n+1];
  assert(batch_alignment_scale(SPELL_HOLY_SMITE,&v)==(i==1?0:i==-1?2:1));
  assert(batch_alignment_scale(SPELL_UNHOLY_BLIGHT,&v)==(i==-1?0:i==1?2:1));
  assert(batch_alignment_scale(SPELL_ORDERS_WRATH,&v)==(n==1?0:n==-1?2:1));
  assert(batch_alignment_scale(SPELL_CHAOS_HAMMER,&v)==(n==-1?0:n==1?2:1));
 }
 reset();GET_ALIGNMENT(&v)=-1000;
 assert(resolve_damage(SPELL_HOLY_SMITE,40,SAVING_WILL,&ch,&v)==40&&save_calls==1&&last_bit==AFF_BLIND);
 reset();resist_power=1;assert(resolve_damage(SPELL_HOLY_SMITE,40,SAVING_WILL,&ch,&v)==20&&save_calls==1&&joins==0);
 reset();GET_ALIGNMENT(&v)=0;assert(resolve_damage(SPELL_HOLY_SMITE,40,SAVING_WILL,&ch,&v)==20&&joins==0);
 resist_power=1;assert(resolve_damage(SPELL_HOLY_SMITE,40,SAVING_WILL,&ch,&v)==10);
 reset();resist_power=1;GET_ALIGNMENT(&v)=-1000;
 assert(resolve_damage(SPELL_BURST_OF_RADIANCE,40,SAVING_REFL,&ch,&v)==40&&last_bit==AFF_DAZZLED);
 reset();GET_ALIGNMENT(&v)=1000;assert(resolve_damage(SPELL_BURST_OF_RADIANCE,40,SAVING_REFL,&ch,&v)==0&&last_bit==AFF_BLIND);
 reset();immune_blind=1;resolve_damage(SPELL_BURST_OF_RADIANCE,40,SAVING_REFL,&ch,&v);assert(!joins);immune_blind=0;
 reset();resolve_damage(SPELL_EAR_PIERCING_SCREAM,30,SAVING_FORT,&ch,&v);assert(last_bit==AFF_DAZED&&last_duration==1);
 reset();immune_stun=1;resolve_damage(SPELL_EAR_PIERCING_SCREAM,30,SAVING_FORT,&ch,&v);assert(!joins);immune_stun=0;
 reset();resist_power=1;feat=FEAT_IMPROVED_EVASION;assert(resolve_damage(SPELL_FROST_FALL_TICK,20,SAVING_REFL,&ch,&v)==0);
 reset();touch_roll=-1;assert(!configure(SPELL_FRIGID_TOUCH,10,&ch,&v,p));
 touch_roll=999;assert(configure(SPELL_FRIGID_TOUCH,10,&ch,&v,p)&&p[0]==8&&p[7]);
 reset();batch_damage_condition(&v,SPELL_FRIGID_TOUCH,false,true);assert(last_bit==AFF_STAGGERED&&last_duration==10);
 reset();immune_cold=1;batch_damage_condition(&v,SPELL_FRIGID_TOUCH,false,false);assert(!joins);immune_cold=0;
 assert(configure(SPELL_EAR_PIERCING_SCREAM,1,&ch,&v,p)&&p[0]==1);
 assert(configure(SPELL_EAR_PIERCING_SCREAM,30,&ch,&v,p)&&p[0]==5);
 GET_AUGMENT_PSP(&ch)=4;
 for(i=0;i<4;i++){
  int energies[]={DAM_FIRE,DAM_COLD,DAM_ELECTRIC,DAM_SOUND};GET_PSIONIC_ENERGY_TYPE(&ch)=energies[i];GET_DC_BONUS(&ch)=0;
  assert(configure(PSIONIC_ENERGY_MISSILE,10,&ch,&v,p)&&p[0]==7&&p[1]==6&&p[4]==energies[i]);
  assert(p[3]==(i==1?SAVING_FORT:SAVING_REFL));assert(GET_DC_BONUS(&ch)==(i==2?4:2));
 }
 GET_PSIONIC_ENERGY_TYPE(&ch)=DAM_ACID;assert(configure(PSIONIC_ENERGY_CONE,10,&ch,&v,p)&&p[4]==DAM_FIRE&&p[0]==9);
 assert(configure(PSIONIC_HAIL_OF_CRYSTALS,10,&ch,&v,p)&&p[0]==13&&!p[5]);
 assert(configure(PSIONIC_HAIL_OF_CRYSTALS_IMPACT,10,&ch,&v,p)&&p[0]==5&&p[3]==-1);
 n=0;for(i=0;i<5;i++)assert(batch_area_limit(PSIONIC_ENERGY_MISSILE,&n));assert(!batch_area_limit(PSIONIC_ENERGY_MISSILE,&n));
 n=0;for(i=0;i<20;i++)assert(batch_area_limit(PSIONIC_ENERGY_CONE,&n));
 reset();touch_roll=0;destiny_plan(7,&ch,&v);assert(last_bit==AFF_SICKENED&&last_duration==7&&!save_calls);
 reset();touch_roll=-1;destiny_plan(7,&ch,&v);assert(!joins);
 reset();touch_roll=0;resist_power=1;destiny_plan(7,&ch,&v);assert(!joins);
 reset();GET_REAL_RACE(&v)=RACE_TYPE_CONSTRUCT;GET_HIT(&v)=1;repair_plan(7,&ch,&v);assert(healing==47);
 GET_HIT(&v)=0;repair_plan(7,&ch,&v);assert(!healing);GET_HIT(&v)=1;
 feat=FEAT_IRON_GOLEM_IMMUNITY;repair_plan(7,&ch,&v);assert(!healing);feat=0;
 GET_REAL_RACE(&v)=RACE_TYPE_HUMANOID;repair_plan(7,&ch,&v);assert(!healing);
 reset();touch_roll=-1;mag_hail_of_crystals(9,&ch,&v,NULL,0,0,CAST_SPELL);assert(!damage_calls&&area_calls==1);
 reset();touch_roll=0;kill_target=1;mag_hail_of_crystals(9,&ch,&v,NULL,0,0,CAST_SPELL);assert(damage_calls==1&&area_calls==1);kill_target=0;
 reset();GET_POS(&v)=POS_STANDING;patch.room=0;patch.caster_uid=77;patch.caster_class=CLASS_WIZARD;patch.caster_level=8;
 rooms[0].people=&ch;ch.next_in_room=&v;v.next_in_room=&other;excluded=&other;
 GET_DC_BONUS(&ch)=7;CASTING_SPELLNUM(&ch)=SPELL_FIREBALL;cp.casting_class=CLASS_SORCERER;
 frost_fall_tick(&patch);assert(damage_calls==1&&GET_DC_BONUS(&ch)==7&&CASTING_SPELLNUM(&ch)==SPELL_FIREBALL&&cp.casting_class==CLASS_SORCERER);
 reset();patch.caster_uid=999;frost_fall_tick(&patch);assert(!damage_calls&&patch.timer==0);
 reset();patch.caster_uid=77;IN_ROOM(&ch)=1;frost_fall_tick(&patch);assert(!damage_calls&&patch.timer==0);
 /* Appended ordinary powers retain epic-feat support; actual epic powers do not. */
 psionic_powers[PSIONIC_ENERGY_MISSILE].psp_cost=3;
 spell_info[PSIONIC_ENERGY_MISSILE].min_level[CLASS_PSIONICIST]=3;
 assert(epic_augmentation_discount(&ch,PSIONIC_ENERGY_MISSILE,4)==2);
 assert(epic_power_choice(&ch,PSIONIC_ENERGY_MISSILE));
 psionic_powers[PSIONIC_IMPALE_MIND].is_epic=true;
 assert(!epic_augmentation_discount(&ch,PSIONIC_IMPALE_MIND,4));
 assert(!epic_power_choice(&ch,PSIONIC_IMPALE_MIND));
 spell_info[PSIONIC_HAIL_OF_CRYSTALS_IMPACT].no_player=true;
 assert(!epic_power_choice(&ch,PSIONIC_HAIL_OF_CRYSTALS_IMPACT));
 ch.epic_cast_context=CAST_SPELL;ch.epic_spell_depth=1;ch.epic_spell_context=PSIONIC_ENERGY_MISSILE;
 EPIC_SECOND(&ch).rebuttal=time(NULL)+60;
 assert(epic_rebuttal_roll(&ch,10,1)==20&&!EPIC_SECOND(&ch).rebuttal);
 puts("PASS: native batch dice, shared saves, alignments, immunity, touch misses/criticals, augmentation, target caps, construct repair, hail impact/explosion and frost caster lifetime");
 return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='spell-batch-') as directory:
    tmp=Path(directory);(tmp/'test.c').write_text(code)
    subprocess.run(['cc','-std=gnu99','-g','-fsanitize=address,undefined','-fno-pie','-no-pie','-I'+str(ROOT/'src'),str(tmp/'test.c'),'-o',str(tmp/'test')],check=True)
    subprocess.run([str(tmp/'test')],check=True)
# Exact class assignments are part of the proposal's public access contract.
classes=(ROOT/'src/class.c').read_text()
expected={'SPELL_EAR_PIERCING_SCREAM':{'WIZARD':1,'SORCERER':1,'BARD':1,'INQUISITOR':1},
'SPELL_FRIGID_TOUCH':{'WIZARD':3,'SORCERER':4,'DRUID':3},
'SPELL_FROST_FALL':{'WIZARD':3,'SORCERER':4,'DRUID':3},
'SPELL_BURST_OF_RADIANCE':{'WIZARD':3,'SORCERER':4,'DRUID':3,'CLERIC':3}}
for sym in ['SPELL_HOLY_SMITE','SPELL_UNHOLY_BLIGHT','SPELL_ORDERS_WRATH','SPELL_CHAOS_HAMMER']:
    expected[sym]={'CLERIC':7,'INQUISITOR':10}
for sym,level in [('ENERGY_MISSILE',3),('ENERGY_CONE',5),('DESTINY_DISSONANCE',1),('RECONSTRUCTION',3),('HAIL_OF_CRYSTALS',9)]:
    expected['PSIONIC_'+sym]={'PSIONICIST':level}
expected.update({
'SPELL_BURNING_DISARM':{'WIZARD':1,'SORCERER':1,'CLERIC':1,'DRUID':1},
'SPELL_BONESHAKER':{'WIZARD':3,'SORCERER':4,'CLERIC':3,'INQUISITOR':4,'BLACKGUARD':6},
'SPELL_BONESHATTER':{'WIZARD':7,'SORCERER':8,'CLERIC':9},
'SPELL_STRICKEN_HEART':{'WIZARD':3,'SORCERER':4,'INQUISITOR':4},
'SPELL_BURST_OF_SPEED':{'ALCHEMIST':7,'BLACKGUARD':12,'INQUISITOR':7,'PALADIN':12,'RANGER':12},
'PSIONIC_NATURAL_HEALING':{'PSIONICIST':1},'PSIONIC_CORROSIVE_AURA':{'PSIONICIST':1},'PSIONIC_EXPOSE_WEAKNESS':{'PSIONICIST':5}})
for symbol,want in expected.items():
    got={c:int(n) for c,n in re.findall(r'spell_assignment\(CLASS_(\w+),\s*'+symbol+r',\s*(\d+)\)',classes)}
    assert got==want,(symbol,got,want)
print('PASS: exact class-level assignments for all 21 additions')
