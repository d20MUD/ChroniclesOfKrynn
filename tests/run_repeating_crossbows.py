#!/usr/bin/env python3
"""Run the production repeater proc with deterministic rolls and defenses."""
from pathlib import Path
import os
import subprocess
import tempfile
root=Path(__file__).resolve().parents[1]
fight=(root/'src/fight.c').read_text()
a=fight.index('static void try_repeater_bonus_attack(')
helper=fight[a:fight.index('\n}',a)+2]
assert 'try_repeater_bonus_attack(ch, penalty);' in fight
assert fight.count('ch->char_specials.repeater_bonus_checked = FALSE;') == 2
code=r'''
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "fight.h"
#include "spells.h"
#include <assert.h>
#undef PRF_FLAGGED
#define PRF_FLAGGED(ch,flag) false
#undef GET_WEAPON_TYPE
#define GET_WEAPON_TYPE(obj) GET_OBJ_VAL(obj,0)
#define send_to_char(...) ((void)0)
static struct obj_data weapon;
static int roll=10, rolls, shots, last_penalty;
static bool proficient=true, ammo=true, valid=true, in_range=true;
struct obj_data *is_using_ranged_weapon(struct char_data *ch,bool silent) {return &weapon;}
int is_proficient_with_weapon(struct char_data *ch,int type) {return proficient;}
int valid_fight_cond(struct char_data *ch,bool verbose) {return valid;}
bool can_fire_ammo(struct char_data *ch,bool silent) {return ammo;}
bool combat_attack_in_range(struct char_data *ch,struct char_data *vict,int type,struct obj_data *obj) {return in_range;}
int rand_number(int low,int high) {rolls++;return roll;}
bool auto_reload_weapon(struct char_data *ch,bool silent) {return false;}
int hit(struct char_data *ch,struct char_data *vict,int type,int dam,int penalty,int attack) {
  assert(attack==ATTACK_TYPE_RANGED);assert(!ch->char_specials.manyshot_pending);
  assert(GET_OBJ_VAL(&weapon,5)>0);GET_OBJ_VAL(&weapon,5)--;
  shots++;last_penalty=penalty;return 0;
}
''' + helper + r'''
int main(void) {
  struct char_data ch={0},vict={0};
  ch.char_specials.fighting=&vict;ch.char_specials.saved.act[0]=0;
  GET_OBJ_TYPE(&weapon)=ITEM_WEAPON;
  int types[]={WEAPON_TYPE_LIGHT_REP_XBOW,WEAPON_TYPE_HEAVY_REP_XBOW};
  for(int t=0;t<2;t++) {
    GET_OBJ_VAL(&weapon,0)=types[t];GET_OBJ_VAL(&weapon,5)=5;
    ch.char_specials.repeater_bonus_checked=false;roll=10;int before=shots;
    ch.char_specials.manyshot_pending=true;
    try_repeater_bonus_attack(&ch,-2);assert(shots==before+1 && last_penalty==-2);
    assert(GET_OBJ_VAL(&weapon,5)==4);
    int attempts=rolls;try_repeater_bonus_attack(&ch,0);assert(rolls==attempts && shots==before+1);
    ch.char_specials.repeater_bonus_checked=false;roll=11;
    try_repeater_bonus_attack(&ch,0);assert(shots==before+1);
    roll=1;try_repeater_bonus_attack(&ch,0);assert(shots==before+1);
  }
  for(int condition=0;condition<8;condition++) {
    ch.char_specials.repeater_bonus_checked=false;GET_OBJ_VAL(&weapon,5)=5;
    GET_OBJ_VAL(&weapon,0)=WEAPON_TYPE_LIGHT_REP_XBOW;
    proficient=ammo=valid=in_range=true;FIGHTING(&ch)=&vict;
    if(condition==0)proficient=false;
    if(condition==1)ammo=false;
    if(condition==2)valid=false;
    if(condition==3)in_range=false;
    if(condition==4)GET_OBJ_VAL(&weapon,5)=0;
    if(condition==5)GET_OBJ_VAL(&weapon,0)=WEAPON_TYPE_LIGHT_CROSSBOW;
    if(condition==6)FIGHTING(&ch)=NULL;
    if(condition==7)SET_BIT_AR(AFF_FLAGS(&ch),AFF_STAGGERED);
    int before=shots;try_repeater_bonus_attack(&ch,0);assert(shots==before);
  }
  puts("PASS: both repeaters, 10 percent boundary, one roll/extra shot per round, normal penalty and load consumption, proficiency/ammo/range/live-target gates.");
}
'''
with tempfile.TemporaryDirectory(prefix='repeater-') as directory:
    source=Path(directory)/'repeater.c';binary=Path(directory)/'repeater'
    source.write_text(code)
    subprocess.run([os.environ.get('CC','cc'),'-std=c99','-I'+str(root/'src'),str(source),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
