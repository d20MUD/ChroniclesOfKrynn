#!/usr/bin/env python3
"""Check shared ranged/bomb accuracy bonuses and prevent duplicate weapon focus."""
from pathlib import Path
import os
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
fight = (root / 'src/fight.c').read_text()
start = fight.index('  case ATTACK_TYPE_RANGED:', fight.index('int compute_attack_bonus_full('))
ranged = fight[start:fight.index('  case ATTACK_TYPE_UNARMED:', start)]
start = fight.index('  if (attack_type != ATTACK_TYPE_RANGED && attack_type != ATTACK_TYPE_ELDRITCH_BLAST &&\n      HAS_FEAT(ch, FEAT_WEAPON_FOCUS))')
focus = fight[start:fight.index('  /* epic weapon specialization */', start)]
start = fight.index('int bomb_attack_roll(')
bomb_wrapper = fight[start:fight.index('\n}', start)+2]
# Use the production constants, including the legacy bomb alias.
headers = '\n'.join('#include "'+name+'"' for name in ['conf.h','sysdep.h','structs.h','utils.h'])
stubs = r'''
#include <assert.h>
#undef HAS_FEAT
#undef HAS_COMBAT_FEAT
#undef KNOWS_DISCOVERY
#undef GET_DEX_BONUS
#define HAS_FEAT(ch,feat) owned(feat)
#define HAS_COMBAT_FEAT(ch,feat,family) focus_owned(feat,family)
#define KNOWS_DISCOVERY(ch,discovery) precise
#define GET_DEX_BONUS(ch) 4
static int precise, craftsman, savant, equipped_family=WEAPON_FAMILY_RANGED;
static int selected_family=WEAPON_FAMILY_RANGED;
static bool has_focus, has_greater, has_superior;
static struct { int weaponFamily; } weapon_list[NUM_WEAPON_TYPES];
static int owned(int feat) {
  if(feat==FEAT_WEAPON_FOCUS) return has_focus;
  if(feat==FEAT_GREATER_WEAPON_FOCUS) return has_greater;
  if(feat==FEAT_SUPERIOR_WEAPON_FOCUS || feat==FEAT_WEAPON_OF_CHOICE) return has_superior;
  return 0;
}
static int feat_to_cfeat(int feat) { return feat; }
static int focus_owned(int feat,int family) { return family==selected_family && owned(feat); }
static int get_alchemist_bomb_precision_bonus(struct char_data *ch) { return craftsman; }
static int get_bombardier_savant_attack_bonus(struct char_data *ch) { return savant; }
static void send_to_char(struct char_data *ch,const char *fmt,...) {}
static int captured_type, captured_extra;
static bool captured_show;
static int perform_attack_roll(struct char_data *ch,struct char_data *victim,int type,
                               int touch,int number,int extra,bool show) {
  assert(touch && number==1);
  captured_type=type; captured_extra=extra; captured_show=show;
  return 7;
}
'''
code = r'''
static int accuracy(int attack_type,bool armed,bool display) {
  struct char_data *ch=NULL;
  struct obj_data obj={0}, *wielded=armed ? &obj : NULL;
  int calc_bab=10, bonuses[NUM_BONUS_TYPES]={0};
  GET_OBJ_TYPE(&obj)=ITEM_WEAPON;
  GET_OBJ_VAL(&obj,0)=WEAPON_TYPE_LONG_SWORD;
  weapon_list[WEAPON_TYPE_LONG_SWORD].weaponFamily=equipped_family;
  weapon_list[WEAPON_TYPE_UNARMED].weaponFamily=WEAPON_FAMILY_MONK;
  switch(attack_type) {
''' + ranged + r'''
  default: break;
  }
''' + focus + r'''
  return calc_bab+bonuses[BONUS_TYPE_UNDEFINED];
}
''' + bomb_wrapper
checks = r'''
int main(void) {
  _Static_assert(ATTACK_TYPE_BOMB_TOSS==ATTACK_TYPE_RANGED,"Bombs must use ranged attacks");
  for(int armed=0;armed<2;armed++) for(int display=0;display<2;display++) {
    precise=craftsman=savant=0; has_focus=has_greater=has_superior=false;
    assert(accuracy(ATTACK_TYPE_RANGED,armed,display)==14);
    has_focus=true;
    assert(accuracy(ATTACK_TYPE_RANGED,armed,display)==15);
    has_greater=has_superior=true;
    assert(accuracy(ATTACK_TYPE_RANGED,armed,display)==17);
    precise=1; craftsman=3; savant=6;
    assert(accuracy(ATTACK_TYPE_RANGED,armed,display)==28);
    assert(accuracy(ATTACK_TYPE_BOMB_TOSS,armed,display)==28);
    equipped_family=WEAPON_FAMILY_HEAVY_BLADE;
    assert(accuracy(ATTACK_TYPE_RANGED,armed,display)==28);
    selected_family=WEAPON_FAMILY_HEAVY_BLADE;
    assert(accuracy(ATTACK_TYPE_RANGED,armed,display)==25);
    assert(accuracy(ATTACK_TYPE_PRIMARY,true,display)==13);
    selected_family=equipped_family=WEAPON_FAMILY_RANGED;
  }
  assert(bomb_attack_roll(NULL,NULL)==7);
  assert(captured_type==ATTACK_TYPE_RANGED && captured_extra==0 && captured_show);
  return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='ranged-bombs-') as tmp:
    path=Path(tmp)/'check.c'
    path.write_text(headers+stubs+code+checks)
    binary=Path(tmp)/'check'
    subprocess.run([os.environ.get('CC','cc'),'-std=gnu11','-Wall','-Wextra',
                    '-Wno-unused-parameter','-I'+str(root/'src'),str(path),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
print('Shared ranged/bomb accuracy, focus stacking, bomb bonuses, melee isolation, and roll routing passed.')
