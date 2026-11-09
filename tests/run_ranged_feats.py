#!/usr/bin/env python3
"""Exercise PF1 ranged feat rules, Manyshot ammunition and crossbow reload actions."""
from pathlib import Path
import os
import subprocess
import tempfile
root=Path(__file__).resolve().parents[1]
fight=(root/'src/fight.c').read_text()
weapons=(root/'src/assign_wpn_armor.c').read_text()
def function(text,name):
    a=text.index(name)
    return text[a:text.index('\n}',a)+2]
feat_source=(root/'src/feats.c').read_text()
for requirement in [
    'feat_prereq_attribute(FEAT_MANYSHOT, AB_DEX, 17)',
    'feat_prereq_bab(FEAT_MANYSHOT, 6)',
    'feat_prereq_attribute(FEAT_IMPROVED_PRECISE_SHOT, AB_DEX, 19)',
    'feat_prereq_bab(FEAT_IMPROVED_PRECISE_SHOT, 11)',
    'feat_prereq_feat(FEAT_PRECISE_SHOT, FEAT_POINT_BLANK_SHOT, 1)',
    'feat_prereq_bab(FEAT_QUICK_DRAW, 1)',
    'feat_prereq_bab(FEAT_DEADLY_AIM, 1)',
]:
    assert requirement in feat_source,requirement
assert 'feato(FEAT_QUICK_DRAW, "quick draw", TRUE, TRUE' in feat_source
headers='\n'.join('#include "'+h+'"' for h in ['conf.h','sysdep.h','structs.h','utils.h','spells.h','assign_wpn_armor.h','spec_abilities.h','fight.h','mud_event.h','actions.h'])
code='#define _POSIX_C_SOURCE 200809L\n'+headers+r'''
#include <assert.h>
#define act(...) ((void)0)
#define send_to_char(...) ((void)0)
#undef USE_MOVE_ACTION
#undef USE_FULL_ROUND_ACTION
#define USE_MOVE_ACTION(ch) (move_used++,move_ready=false)
#define USE_FULL_ROUND_ACTION(ch) (move_used++,standard_used++,move_ready=false,standard_ready=false)
#define is_action_available(ch,action,show) ((action)==atMOVE?move_ready:standard_ready)
static bool move_ready=true,standard_ready=true;
static int move_used,standard_used;
struct weapon_table weapon_list[NUM_WEAPON_TYPES];
struct index_data *obj_index;
obj_rnum top_of_objt=-1;
static int feats[NUM_FEATS], base_bab, distance, concealment;
static struct obj_data *last_missile;
static int computed, reduction_calls, packets, values[4], types[4], property_calls;
static struct char_data *last_victim;
struct char_data *find_char(long id) {return last_victim;}
int process_weapon_abilities(struct obj_data *weapon,struct char_data *ch,struct char_data *victim,int method,const char *command) {assert(method==ACTMTD_ON_HIT);property_calls++;return 1;}
int MIN(int a,int b) {return a<b?a:b;}
int MAX(int a,int b) {return a>b?a:b;}
int get_feat_value(struct char_data *ch,int feat) {return feats[feat];}
int BAB(struct char_data *ch) {return base_bab;}
int combat_distance(struct char_data *ch,struct char_data *victim) {return distance;}
int compute_concealment(struct char_data *victim,struct char_data *ch) {return concealment;}
bool obj_has_special_ability(struct obj_data *obj,int ability) {return false;}
static void obj_from_obj(struct obj_data *obj) {
  assert(obj->in_obj);obj->in_obj->contains=obj->next_content;obj->in_obj=NULL;obj->next_content=NULL;
}
static void obj_to_char(struct obj_data *obj,struct char_data *ch) {obj->carried_by=ch;}
struct obj_data *create_obj(void) {struct obj_data *obj=calloc(1,sizeof(*obj));IN_ROOM(obj)=NOWHERE;return obj;}
int dice(int number,int size) {return number*size;}
'''
for name in ['bool is_throwing_weapon(','bool ammunition_is_unlimited(','static bool usable_ammunition(','bool has_ammo_in_pouch(','struct obj_data *take_ammo_projectile(','bool reload_weapon(','bool ready_to_reload(']:
    code+='\n'+function(weapons,name)
code+=r'''
bool can_fire_ammo(struct char_data *ch,bool silent) {
  return has_ammo_in_pouch(ch,GET_EQ(ch,WEAR_WIELD_1),silent);
}
'''
a=fight.index('struct ranged_shot_context\n');b=fight.index('/* Distances are symmetric',a)
code+='\n'+fight[a:b]
for name in ['int point_blank_shot_bonus(','int deadly_aim_step(','int ranged_range_penalty(','int ranged_concealment(']:
    code+='\n'+function(fight,name)
code+=r'''
int compute_hit_damage(struct char_data *ch,struct char_data *victim,int w_type,int roll,
                       int mode,bool critical,int attack_type,int damage_type) {
  assert(!critical && attack_type==ATTACK_TYPE_RANGED);
  assert(current_shot->resolving_extra);
  assert(ranged_ammunition(ch)==current_shot->extra_projectile);
  computed++;return 20;
}
int handle_warding(struct char_data *ch,struct char_data *victim,int amount) {return amount;}
int apply_damage_reduction(struct char_data *ch,struct char_data *victim,struct obj_data *weapon,
                           int amount,bool display,int type) {reduction_calls++;return amount-7;}
int damage(struct char_data *ch,struct char_data *victim,int amount,int w_type,int type,int attack_type) {
  last_victim=victim;assert(packets<4);values[packets]=amount;types[packets++]=type;return amount;
}
'''
code+='\n'+function(fight,'static bool resolve_manyshot_arrow(')
a=fight.index('    current_shot->unlimited = ammunition_is_unlimited(missile, wielded);')
b=fight.index('    /* if this was a weapon that was loaded, unload */',a)
code+='\nstatic void commit_shot(struct char_data *ch) { struct obj_data *wielded=GET_EQ(ch,WEAR_WIELD_1), *ammo_pouch=GET_EQ(ch,WEAR_AMMO_POUCH), *missile=ammo_pouch->contains;\n'+fight[a:b]+'\n}\n'
a=fight.index('    if (AFF_FLAGGED(ch, AFF_RAPID_SHOT) && HAS_FEAT(ch, FEAT_RAPID_SHOT)',fight.index('int perform_attacks('))
b=fight.index('  /* -- finished processing base number of attacks',a)
rapid=fight[a:b]
# Strip the enclosing can_fire_ammo block's closing brace before the thrown cap.
rapid=rapid.replace('\n  }\n  if (is_throwing_weapon', '\n  if (is_throwing_weapon',1)
code+=r'''
struct obj_data *is_using_ranged_weapon(struct char_data *ch,bool silent) {return GET_EQ(ch,WEAR_WIELD_1);}
static int volley_penalty;
static int volley_count(struct char_data *ch,int bonus_mainhand_attacks) {
  int ranged_attacks=1+bonus_mainhand_attacks, penalty=0,attacks_at_max_bab=0,drop_an_attack_at_max_bab=0;
'''+rapid+'\nvolley_penalty=penalty;return ranged_attacks;\n}\n'
a=fight.index('  if (attack_type == ATTACK_TYPE_RANGED &&\n      is_throwing_weapon(ch, get_wielded(ch, ATTACK_TYPE_RANGED))')
b=fight.index('  GET_ATTACKS_THIS_ROUND(ch)++;',a)
code+=r'''
#define HIT_MISS 0
struct obj_data *get_wielded(struct char_data *ch,int attack_type) {return GET_EQ(ch,WEAR_WIELD_1);}
static bool attempt_throw(struct char_data *ch,int attack_type) {
'''+fight[a:b]+'\nGET_ATTACKS_THIS_ROUND(ch)++;return true;\n}\n'
code+=r'''
static struct char_data ch,victim;
static struct player_special_data special;
static struct obj_data bow,ammo,pouch,extra;
static void dispose(struct obj_data *obj) {free(obj->name);free(obj->short_description);free(obj->description);free(obj);}
int main(void) {
  ch.player_specials=&special;GET_POS(&victim)=POS_FIGHTING;
  distance=30;assert(!point_blank_shot_bonus(&ch,&victim));
  feats[FEAT_POINT_BLANK_SHOT]=1;
  for(int d=0;d<=30;d++) {distance=d;assert(point_blank_shot_bonus(&ch,&victim)==1);}
  distance=31;assert(!point_blank_shot_bonus(&ch,&victim));
  distance=-1;assert(!point_blank_shot_bonus(&ch,&victim));
  SET_BIT_AR(AFF_FLAGS(&ch),AFF_DEADLY_AIM);feats[FEAT_DEADLY_AIM]=1;
  int bab_values[]={1,3,4,7,8,12,20};int steps[]={1,1,2,2,3,4,6};
  for(int i=0;i<7;i++) {base_bab=bab_values[i];assert(deadly_aim_step(&ch)==steps[i]);}
  REMOVE_BIT_AR(AFF_FLAGS(&ch),AFF_DEADLY_AIM);assert(!deadly_aim_step(&ch));
  GET_OBJ_TYPE(&bow)=ITEM_WEAPON;GET_OBJ_VAL(&bow,0)=WEAPON_TYPE_LONG_BOW;
  weapon_list[WEAPON_TYPE_LONG_BOW].range=100;
  distance=100;assert(!ranged_range_penalty(&ch,&victim,&bow));
  distance=101;assert(ranged_range_penalty(&ch,&victim,&bow)==-2);
  distance=201;assert(ranged_range_penalty(&ch,&victim,&bow)==-4);
  feats[FEAT_FAR_SHOT]=1;assert(ranged_range_penalty(&ch,&victim,&bow)==-2);
  concealment=20;assert(ranged_concealment(&ch,&victim,ATTACK_TYPE_RANGED)==20);
  feats[FEAT_IMPROVED_PRECISE_SHOT]=1;
  assert(!ranged_concealment(&ch,&victim,ATTACK_TYPE_RANGED));
  assert(!ranged_concealment(&ch,&victim,ATTACK_TYPE_PSIONICS));
  assert(ranged_concealment(&ch,&victim,ATTACK_TYPE_PRIMARY)==20);
  concealment=50;assert(ranged_concealment(&ch,&victim,ATTACK_TYPE_RANGED)==50);
  concealment=100;assert(ranged_concealment(&ch,&victim,ATTACK_TYPE_RANGED)==100);
  GET_EQ(&ch,WEAR_WIELD_1)=&bow;GET_EQ(&ch,WEAR_AMMO_POUCH)=&pouch;
  pouch.contains=&ammo;ammo.in_obj=&pouch;GET_OBJ_TYPE(&ammo)=ITEM_MISSILE;
  GET_OBJ_VAL(&ammo,0)=AMMO_TYPE_ARROW;GET_OBJ_VAL(&ammo,AMMO_VAL_MAX)=20;
  GET_OBJ_VAL(&ammo,AMMO_VAL_QUANTITY)=3;
  ammo.name="arrows";ammo.short_description="arrows";ammo.description="Arrows.";
  assert(!manyshot_ready(&ch,&bow));feats[FEAT_MANYSHOT]=1;assert(manyshot_ready(&ch,&bow));
  GET_OBJ_VAL(&bow,0)=WEAPON_TYPE_LIGHT_CROSSBOW;assert(!manyshot_ready(&ch,&bow));
  GET_OBJ_VAL(&bow,0)=WEAPON_TYPE_LONG_BOW;
  GET_OBJ_VAL(&ammo,AMMO_VAL_QUANTITY)=1;assert(!manyshot_ready(&ch,&bow));
  GET_OBJ_TYPE(&extra)=ITEM_MISSILE;GET_OBJ_VAL(&extra,0)=AMMO_TYPE_ARROW;
  ammo.next_content=&extra;assert(manyshot_ready(&ch,&bow));
  struct obj_data wrong={0};GET_OBJ_TYPE(&wrong)=ITEM_MISSILE;GET_OBJ_VAL(&wrong,0)=AMMO_TYPE_BOLT;
  ammo.next_content=&wrong;wrong.next_content=&extra;assert(manyshot_ready(&ch,&bow));
  ammo.next_content=NULL;
  GET_OBJ_VAL(&ammo,AMMO_VAL_QUANTITY)=3;
  GET_OBJ_VAL(&ammo,AMMO_VAL_DAMAGE_TYPE)=DAM_FIRE;GET_OBJ_VAL(&ammo,AMMO_VAL_DAMAGE_DIE)=6;
  struct ranged_shot_context context={0};context.attacker=&ch;context.manyshot=true;
  current_shot=&context;commit_shot(&ch);
  assert(GET_OBJ_VAL(&ammo,AMMO_VAL_QUANTITY)==1);
  assert(context.projectile && context.extra_projectile && context.projectile!=context.extra_projectile);
  assert(!resolve_manyshot_arrow(&ch,&victim,&bow,TYPE_PIERCE,DAM_PUNCTURE));
  assert(computed==1 && reduction_calls==1 && packets==2 && property_calls==1);
  assert(values[0]==13 && types[0]==DAM_PUNCTURE && values[1]==6 && types[1]==DAM_FIRE);
  assert(!context.resolving_extra);
  dispose(context.projectile);dispose(context.extra_projectile);current_shot=NULL;
  /* Multiple thrown attacks require Quick Draw, and Rapid Shot requires a full attack. */
  GET_OBJ_VAL(&bow,0)=WEAPON_TYPE_DAGGER;weapon_list[WEAPON_TYPE_DAGGER].weaponFlags=WEAPON_FLAG_THROWN;
  THROWING(&ch)=true;SET_BIT_AR(AFF_FLAGS(&ch),AFF_RAPID_SHOT);feats[FEAT_RAPID_SHOT]=1;
  assert(volley_count(&ch,3)==1 && volley_penalty==0);
  GET_ATTACKS_THIS_ROUND(&ch)=0;
  assert(attempt_throw(&ch,ATTACK_TYPE_RANGED));assert(!attempt_throw(&ch,ATTACK_TYPE_RANGED));
  feats[FEAT_QUICK_DRAW]=1;
  assert(volley_count(&ch,3)==5 && volley_penalty==-2);
  for(int i=0;i<5;i++) assert(attempt_throw(&ch,ATTACK_TYPE_RANGED));
  move_ready=false;assert(volley_count(&ch,3)==4 && volley_penalty==0);move_ready=true;
  THROWING(&ch)=false;GET_OBJ_VAL(&bow,0)=WEAPON_TYPE_LONG_BOW;
  assert(volley_count(&ch,1)==3 && volley_penalty==-2);
  feats[FEAT_MANYSHOT]=0;assert(volley_count(&ch,1)==3);feats[FEAT_MANYSHOT]=1;
  REMOVE_BIT_AR(AFF_FLAGS(&ch),AFF_RAPID_SHOT);
  assert(volley_count(&ch,1)==2 && volley_penalty==0);

  /* A free reload is what permits successive shots with a light crossbow. */
  int crossbows[]={WEAPON_TYPE_HAND_CROSSBOW,WEAPON_TYPE_LIGHT_CROSSBOW,WEAPON_TYPE_HEAVY_CROSSBOW};
  for(int i=0;i<3;i++) {
    GET_OBJ_VAL(&bow,0)=crossbows[i];GET_OBJ_VAL(&bow,5)=0;
    move_ready=standard_ready=true;move_used=standard_used=0;feats[FEAT_RAPID_RELOAD]=0;
    assert(ready_to_reload(&ch,&bow,true));
    assert(move_used==1 && standard_used==(i==2));
    GET_OBJ_VAL(&bow,5)=0;move_ready=standard_ready=true;move_used=standard_used=0;
    feats[FEAT_RAPID_RELOAD]=1;assert(ready_to_reload(&ch,&bow,true));
    assert(move_used==(i==2) && !standard_used);
  }
  puts("PASS: PBS 30-foot boundary, BAB-scaled Deadly Aim, Far Shot increments, partial/total concealment, bow-only Manyshot with two ammo/one roll/no extra critical or precision, separate DR/resistance and Rapid Reload actions.");
}
'''
with tempfile.TemporaryDirectory(prefix='ranged-feats-') as directory:
    cfile=Path(directory)/'feats.c';binary=Path(directory)/'feats'
    cfile.write_text(code)
    subprocess.run([os.environ.get('CC','cc'),'-std=c11','-g','-fsanitize=address,undefined',
                    '-fno-omit-frame-pointer','-I'+str(root/'src'),str(cfile),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
