#!/usr/bin/env python3
"""Run production distance, reach, movement, mob flag and PC advance rules."""
from pathlib import Path
import os
import subprocess
import tempfile
root=Path(__file__).resolve().parents[1]
fight=(root/'src/fight.c').read_text()
offense=(root/'src/act.offensive.c').read_text()
def function(text,name):
    a=text.index(name)
    return text[a:text.index('\n}',a)+2]
a=fight.index('/* Distances are symmetric')
b=fight.index('/* head of l-list',a)
production=fight[a:b]
# Check lifecycle hooks and ordering against the real entry points.
assert 'start_combat_distance(ch, vict);' in function(fight,'bool set_fighting(')
assert 'clear_combat_distances(ch, FALSE);' in function(fight,'void stop_fighting(')
assert 'clear_combat_distances(ch, TRUE);' in function((root/'src/handler.c').read_text(),'void char_from_room(')
turn=function(fight,'void perform_violence(')
assert turn.index('auto_advance_mob(ch, phase);') < turn.index('epic_summon_combat_turn(ch)')
assert 'combat_distance(ch, FIGHTING(ch)) > 5' in turn
assert 'combat_attack_in_range(ch, victim, attack_type, wielded)' in function(fight,'static int resolve_weapon_hit(struct char_data *ch, struct char_data *victim, int type, int dam_type')
assert 'do_combat_advance, 0, 0' in (root/'src/interpreter.c').read_text()
headers='\n'.join('#include "'+h+'"' for h in ['conf.h','sysdep.h','structs.h','utils.h','spells.h','assign_wpn_armor.h','fight.h','interpreter.h','mud_event.h','actions.h'])
code=headers+r'''
#include <assert.h>
#undef ACMD
#define ACMD(name) void name(struct char_data *ch,const char *argument,int cmd,int subcmd)
#undef CREATE
#define CREATE(ptr,type,count) do { (ptr)=calloc((count),sizeof(type));assert(ptr); } while(0)
#undef PERS
#define PERS(ch, viewer) "opponent"
#define act(...) ((void)0)
#define send_to_char(...) ((void)0)
#undef USE_MOVE_ACTION
#define USE_MOVE_ACTION(ch) (used_move++,move_ready=false)
#define is_action_available(ch,action,show) move_ready
#define one_argument(text,buf,len) snprintf(buf,len,"%s",text)
#define get_char_vis(...) selected
static int used_move, staff_calls;
static bool move_ready=true, event_stunned, grounded;
#define char_has_mud_event(ch,event) event_stunned
#define legendary_grounded(ch) grounded
static struct char_data *selected;
struct room_data *world;
room_rnum top_of_world;
struct weapon_table weapon_list[NUM_WEAPON_TYPES];
int get_feat_value(struct char_data *ch,int feat) {return 0;}
int BAB(struct char_data *ch) {return 0;}
int MAX(int a,int b) {return a>b?a:b;}
int MIN(int a,int b) {return a<b?a:b;}
int get_speed(struct char_data *ch,sbyte display) {return GET_MOVE(ch);}
struct obj_data *get_wielded(struct char_data *ch,int attack_type) {
  return GET_EQ(ch,WEAR_WIELD_1) ? GET_EQ(ch,WEAR_WIELD_1) : GET_EQ(ch,WEAR_WIELD_2H);
}
struct obj_data *is_using_ranged_weapon(struct char_data *ch,bool silent) {
  struct obj_data *weapon=get_wielded(ch,ATTACK_TYPE_PRIMARY);
  if(weapon && GET_OBJ_TYPE(weapon)==ITEM_WEAPON &&
     weapon_list[GET_OBJ_VAL(weapon,0)].weaponFlags&WEAPON_FLAG_RANGED) return weapon;
  return NULL;
}
ACMD(do_advance) {staff_calls++;}
'''
utils=(root/'src/utils.c').read_text()
code+='\n'+function(utils,'int default_room_size(')+'\n'+function(utils,'int get_room_size(')
code+='\n'+function((root/'src/assign_wpn_armor.c').read_text(),'bool is_throwing_weapon(')
code+='\n'+production+'\n'+function(offense,'ACMD(do_combat_advance)')
code+=r'''
static struct char_data pc, mob, other, follower;
static struct player_special_data pc_specials;
static struct room_data rooms[2];
static struct obj_data bow,sword,polearm;
static void reset_pairs(void) {
  clear_combat_distances(&pc,true); clear_combat_distances(&mob,true);
  clear_combat_distances(&other,true);clear_combat_distances(&follower,true);
  assert(!combat_distances);
}
int main(void) {
  world=rooms;top_of_world=1;rooms[0].sector_type=SECT_FIELD;
  pc.player_specials=&pc_specials;
  struct char_data *chars[]={&pc,&mob,&other,&follower};
  for(int i=0;i<4;i++) {IN_ROOM(chars[i])=0;GET_POS(chars[i])=POS_FIGHTING;GET_MOVE(chars[i])=30;}
  SET_BIT_AR(MOB_FLAGS(&mob),MOB_ISNPC);SET_BIT_AR(MOB_FLAGS(&other),MOB_ISNPC);
  SET_BIT_AR(MOB_FLAGS(&follower),MOB_ISNPC);SET_BIT_AR(AFF_FLAGS(&follower),AFF_CHARM);
  follower.master=&pc;
  GET_OBJ_TYPE(&bow)=GET_OBJ_TYPE(&sword)=GET_OBJ_TYPE(&polearm)=ITEM_WEAPON;
  GET_OBJ_VAL(&bow,0)=WEAPON_TYPE_LONG_BOW;GET_OBJ_VAL(&sword,0)=WEAPON_TYPE_LONG_SWORD;
  GET_OBJ_VAL(&polearm,0)=WEAPON_TYPE_HALBERD;
  weapon_list[WEAPON_TYPE_LONG_BOW].weaponFlags=WEAPON_FLAG_RANGED;
  weapon_list[WEAPON_TYPE_HALBERD].weaponFlags=WEAPON_FLAG_REACH;
  GET_EQ(&pc,WEAR_WIELD_1)=&sword;
  assert(combat_distance(&pc,&mob)==0);
  GET_EQ(&pc,WEAR_WIELD_1)=&bow;
  assert(combat_distance(&pc,&mob)==50 && combat_distance(&mob,&pc)==50);
  FIGHTING(&pc)=&mob;FIGHTING(&mob)=&pc;
  start_combat_distance(&pc,&mob);start_combat_distance(&mob,&pc);
  assert(combat_distances && !combat_distances->next);
  assert(!combat_attack_in_range(&mob,&pc,ATTACK_TYPE_PRIMARY,NULL));
  assert(combat_attack_in_range(&pc,&mob,ATTACK_TYPE_RANGED,&bow));
  auto_advance_mob(&mob,2);auto_advance_mob(&mob,3);assert(combat_distance(&pc,&mob)==50);
  auto_advance_mob(&mob,1);assert(combat_distance(&pc,&mob)==20);
  assert(!combat_attack_in_range(&mob,&pc,ATTACK_TYPE_RANGED,NULL));
  SET_BIT_AR(MOB_FLAGS(&mob),MOB_RANGED_ATTACK);
  assert(combat_attack_in_range(&mob,&pc,ATTACK_TYPE_PRIMARY,NULL));
  auto_advance_mob(&mob,1);assert(combat_distance(&pc,&mob)==5);
  REMOVE_BIT_AR(MOB_FLAGS(&mob),MOB_RANGED_ATTACK);
  assert(combat_attack_in_range(&mob,&pc,ATTACK_TYPE_PRIMARY,NULL));
  reset_pairs();
  start_combat_distance(&pc,&mob);start_combat_distance(&pc,&other);
  advance_combat_distance(&mob,&pc,5);
  assert(combat_distance(&pc,&mob)==20 && combat_distance(&pc,&other)==50);
  FIGHTING(&pc)=NULL;clear_combat_distances(&pc,false);
  assert(find_combat_distance(&pc,&mob) && !find_combat_distance(&pc,&other));
  FIGHTING(&mob)=NULL;clear_combat_distances(&mob,false);assert(!combat_distances);
  /* Already engaged pairs never reset when equipment or target selection changes. */
  GET_EQ(&pc,WEAR_WIELD_1)=&bow;start_combat_distance(&pc,&mob);
  GET_EQ(&pc,WEAR_WIELD_1)=&polearm;
  assert(combat_melee_range(&pc,&polearm)==10 && combat_melee_range(&pc,&sword)==5);
  assert(combat_melee_range(&mob,&polearm)==5);
  FIGHTING(&pc)=&mob;FIGHTING(&mob)=&pc;
  used_move=0;move_ready=true;selected=&mob;
  do_combat_advance(&pc,"",0,0);
  assert(used_move==1 && combat_distance(&pc,&mob)==20);
  do_combat_advance(&pc,"",0,0);assert(used_move==1 && combat_distance(&pc,&mob)==20);
  move_ready=true;do_combat_advance(&pc,"",0,0);
  assert(used_move==2 && combat_distance(&pc,&mob)==10);
  assert(combat_attack_in_range(&pc,&mob,ATTACK_TYPE_PRIMARY,&polearm));
  assert(!combat_attack_in_range(&pc,&mob,ATTACK_TYPE_UNARMED,NULL));
  assert(!combat_attack_in_range(&pc,&mob,ATTACK_TYPE_PRIMARY_EVO_BITE,&polearm));
  move_ready=true;do_combat_advance(&pc,"",0,0);assert(used_move==2 && move_ready);
  GET_EQ(&pc,WEAR_WIELD_1)=&sword;do_combat_advance(&pc,"",0,0);
  assert(used_move==3 && combat_distance(&pc,&mob)==5);
  reset_pairs();
  /* Charmies and flagged ranged mobs always close too. */
  GET_EQ(&pc,WEAR_WIELD_1)=&bow;FIGHTING(&follower)=&pc;
  start_combat_distance(&follower,&pc);
  auto_advance_mob(&follower,1);assert(combat_distance(&follower,&pc)==20);
  reset_pairs();
  FIGHTING(&pc)=&mob;FIGHTING(&mob)=&pc;start_combat_distance(&pc,&mob);
  SET_BIT_AR(AFF_FLAGS(&mob),AFF_PARALYZED);auto_advance_mob(&mob,1);
  assert(combat_distance(&pc,&mob)==50);REMOVE_BIT_AR(AFF_FLAGS(&mob),AFF_PARALYZED);
  GET_MOVE(&mob)=0;auto_advance_mob(&mob,1);assert(combat_distance(&pc,&mob)==50);
  event_stunned=true;GET_MOVE(&mob)=30;auto_advance_mob(&mob,1);
  assert(combat_distance(&pc,&mob)==50);event_stunned=false;
  grounded=true;auto_advance_mob(&mob,1);assert(combat_distance(&pc,&mob)==50);grounded=false;
  GET_MOVE(&mob)=12;auto_advance_mob(&mob,1);assert(combat_distance(&pc,&mob)==38);
  /* Bounds, departure and PC command authorization. */
  IN_ROOM(&mob)=1;assert(combat_distance(&pc,&mob)==-1);
  assert(!combat_attack_in_range(&pc,&mob,ATTACK_TYPE_RANGED,&bow));
  IN_ROOM(&mob)=0;reset_pairs();
  FIGHTING(&pc)=NULL;GET_LEVEL(&pc)=1;do_combat_advance(&pc,"",0,0);assert(!staff_calls);
  GET_LEVEL(&pc)=LVL_GRSTAFF;do_combat_advance(&pc,"target 10",0,0);assert(staff_calls==1);
  assert(!combat_distances);
  rooms[0].room_size=INT_MAX;GET_EQ(&pc,WEAR_WIELD_1)=&bow;
  assert(combat_distance(&pc,&mob)==INT_MAX);
  assert(advance_combat_distance(&mob,&pc,5)==12);
  reset_pairs();
  puts("PASS: ranged/melee starts, symmetric multi-opponent distance, per-round mob/charmie movement, ranged mob flag, 5/10-foot reach, advance actions, immobilization, cleanup and staff authorization.");
}
'''
with tempfile.TemporaryDirectory(prefix='combat-distance-') as directory:
    cfile=Path(directory)/'distance.c';binary=Path(directory)/'distance'
    cfile.write_text(code)
    subprocess.run([os.environ.get('CC','cc'),'-std=c11','-g','-fsanitize=address,undefined',
                    '-fno-omit-frame-pointer','-I'+str(root/'src'),str(cfile),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
