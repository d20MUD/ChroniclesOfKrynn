#!/usr/bin/env python3
"""Run real launcher selection, pairing, reload and offhand scheduling code."""
from pathlib import Path
import subprocess,tempfile,os
root=Path(__file__).resolve().parents[1]
w=(root/'src/assign_wpn_armor.c').read_text();f=(root/'src/fight.c').read_text()
def fn(s,name):
 a=s.index(name);return s[a:s.index('\n}',a)+2]
code='\n'.join('#include "'+h+'"' for h in ['conf.h','sysdep.h','structs.h','utils.h','spells.h','fight.h','assign_wpn_armor.h'])+r'''
#include <assert.h>
#define send_to_char test_send_to_char
static size_t test_send_to_char(struct char_data *ch,const char *fmt,...) {return 0;}
#undef CHECK_PLAYER_SPECIAL
#define CHECK_PLAYER_SPECIAL(ch,var) (var)
#define MODE_IMP_2_WPN 2
#define MODE_GREAT_2_WPN 3
#define MODE_EPIC_2_WPN 4
#define NORMAL_ATTACK_ROUTINE 0
#define RETURN_NUM_ATTACKS 1
#define DISPLAY_ROUTINE_POTENTIAL 2
#define HIT_NEED_RELOAD -2
#define PHASE_0 0
struct weapon_table weapon_list[NUM_WEAPON_TYPES];
static int skills[5], ammo, shots, penalties[8], shown;
static bool kill_target;
struct obj_data *get_throwing_weapon(struct char_data *ch) {return NULL;}
bool hoopak_uses_ranged(struct char_data *ch,struct obj_data *obj) {return false;}
bool is_throwing_weapon(struct char_data *ch,struct obj_data *obj) {return false;}
bool is_using_double_weapon(struct char_data *ch) {return false;}
int is_skilled_dualer(struct char_data *ch,int mode) {return skills[mode];}
'''
for name in ['bool is_hand_crossbow(', 'bool dual_hand_crossbows(', 'bool ranged_wield_compatible(', 'struct obj_data *is_using_ranged_weapon(', 'bool reload_weapon(']:code+=fn(w,name)+'\n'
code+=fn(f,'struct obj_data *get_wielded(struct char_data *ch, /* Wielder */')+'\n'
code+=r'''
bool process_load_weapon(struct char_data *ch,struct obj_data *obj,bool silent) {
 if (!obj || GET_OBJ_VAL(obj,5)>0) return false;
 return reload_weapon(ch,obj,silent);
}
'''+fn(w,'bool auto_reload_weapon(')+r'''
bool can_fire_ammo(struct char_data *ch,bool silent) {return ammo>0;}
int hit(struct char_data *ch,struct char_data *victim,int type,int damage_type,int penalty,int attack_type) {
 struct obj_data *obj=get_wielded(ch,attack_type);
 assert(obj==GET_EQ(ch,WEAR_WIELD_OFFHAND));
 assert(is_using_ranged_weapon(ch,true)==obj);
 if (!GET_OBJ_VAL(obj,5)) return HIT_NEED_RELOAD;
 GET_OBJ_VAL(obj,5)--;ammo--;penalties[shots++]=penalty;
 if(kill_target)FIGHTING(ch)=NULL;
 return 0;
}
int compute_attack_bonus(struct char_data *ch,struct char_data *victim,int type) {assert(ch->char_specials.ranged_offhand);shown++;return 10;}
int compute_hit_damage(struct char_data *ch,struct char_data *victim,int type,int roll,int mode,bool critical,int attack_type,int reduction) {assert(get_wielded(ch,attack_type)==GET_EQ(ch,WEAR_WIELD_OFFHAND));return 1;}
'''+fn(f,'static int hand_crossbow_offhand_attacks(')+r'''
int main(void) {
 struct char_data ch={0},enemy={0};struct player_special_data ps={0};ch.player_specials=&ps;
 struct obj_data main={0},off={0},other={0};
 GET_OBJ_TYPE(&main)=GET_OBJ_TYPE(&off)=GET_OBJ_TYPE(&other)=ITEM_WEAPON;
 GET_OBJ_VAL(&main,0)=GET_OBJ_VAL(&off,0)=WEAPON_TYPE_HAND_CROSSBOW;
 GET_OBJ_VAL(&other,0)=WEAPON_TYPE_LONG_SWORD;
 weapon_list[WEAPON_TYPE_HAND_CROSSBOW].weaponFlags=WEAPON_FLAG_RANGED;
 weapon_list[WEAPON_TYPE_LONG_BOW].weaponFlags=WEAPON_FLAG_RANGED;
 GET_EQ(&ch,WEAR_WIELD_1)=&main;
 assert(ranged_wield_compatible(&ch,&off));assert(!ranged_wield_compatible(&ch,&other));
 GET_OBJ_VAL(&other,0)=WEAPON_TYPE_LONG_BOW;assert(!ranged_wield_compatible(&ch,&other));
 GET_EQ(&ch,WEAR_WIELD_OFFHAND)=&off;assert(dual_hand_crossbows(&ch));
 assert(get_wielded(&ch,ATTACK_TYPE_RANGED)==&main);
 assert(is_using_ranged_weapon(&ch,true)==&main);
 assert(auto_reload_weapon(&ch,true));assert(GET_OBJ_VAL(&main,5)==1&&GET_OBJ_VAL(&off,5)==1);
 assert(!auto_reload_weapon(&ch,true));
 FIGHTING(&ch)=&enemy;ammo=20;
 assert(hand_crossbow_offhand_attacks(&ch,RETURN_NUM_ATTACKS,0,0)==1);
 assert(!shots&&!ch.char_specials.ranged_offhand);
 hand_crossbow_offhand_attacks(&ch,NORMAL_ATTACK_ROUTINE,1,0);assert(!shots);
 hand_crossbow_offhand_attacks(&ch,NORMAL_ATTACK_ROUTINE,2,-2);
 assert(shots==1&&ammo==19&&GET_OBJ_VAL(&off,5)==0&&GET_OBJ_VAL(&main,5)==1&&penalties[0]==-2);
 assert(!ch.char_specials.ranged_offhand&&get_wielded(&ch,ATTACK_TYPE_RANGED)==&main);
 hand_crossbow_offhand_attacks(&ch,NORMAL_ATTACK_ROUTINE,2,0);assert(shots==1);
 SET_BIT_AR(PRF_FLAGS(&ch),PRF_AUTORELOAD);
 skills[2]=skills[3]=skills[4]=1;shots=0;GET_OBJ_VAL(&off,5)=1;
 assert(hand_crossbow_offhand_attacks(&ch,RETURN_NUM_ATTACKS,0,0)==4);
 hand_crossbow_offhand_attacks(&ch,NORMAL_ATTACK_ROUTINE,0,-2);
 assert(shots==4&&penalties[0]==-2&&penalties[1]==-7&&penalties[2]==-12&&penalties[3]==-2);
 assert(GET_OBJ_VAL(&main,5)==1&&GET_OBJ_VAL(&off,5)==1&&!ch.char_specials.ranged_offhand);
 hand_crossbow_offhand_attacks(&ch,DISPLAY_ROUTINE_POTENTIAL,0,0);assert(shown==4);
 shots=0;ammo=1;GET_OBJ_VAL(&off,5)=1;
 hand_crossbow_offhand_attacks(&ch,NORMAL_ATTACK_ROUTINE,0,0);
 assert(shots==1&&ammo==0&&!ch.char_specials.ranged_offhand);
 ammo=20;shots=0;kill_target=true;hand_crossbow_offhand_attacks(&ch,NORMAL_ATTACK_ROUTINE,0,0);
 assert(shots==1&&!ch.char_specials.ranged_offhand);
 GET_EQ(&ch,WEAR_WIELD_1)=NULL;
 assert(get_wielded(&ch,ATTACK_TYPE_RANGED)==&off);
 assert(is_using_ranged_weapon(&ch,true)==&off);
 GET_EQ(&ch,WEAR_WIELD_OFFHAND)=NULL;assert(!dual_hand_crossbows(&ch));
 assert(!hand_crossbow_offhand_attacks(&ch,RETURN_NUM_ATTACKS,0,0));
 puts("PASS: hand-crossbow pairing, launcher selection, independent reloads, phase scheduling, feat attacks, ammo exhaustion, target death and context cleanup");
}
'''
with tempfile.TemporaryDirectory(prefix='dual-crossbows-') as directory:
 p=Path(directory);(p/'test.c').write_text(code)
 subprocess.run([os.environ.get('CC','cc'),'-std=c11','-g','-fsanitize=address,undefined','-I'+str(root/'src'),str(p/'test.c'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
