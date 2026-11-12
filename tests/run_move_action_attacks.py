#!/usr/bin/env python3
"""Exercise production single-attack execution after spending a move action."""
from pathlib import Path
import subprocess
import tempfile
root = Path(__file__).resolve().parents[1]
s = (root / 'src/fight.c').read_text()
def function(signature):
    a = s.index(signature)
    return s[a:s.index('\n}', a) + 2]
# Execute the real routine's action gates; the full-attack body is a sentinel.
a = s.index('int perform_attacks(')
b = s.index('  /** BEGIN PROCESS OF COUNTING ATTACKS', a)
routing = s[a:b] + '  return 42;\n}\n'
headers = '\n'.join('#include "' + h + '"' for h in
                    ['conf.h', 'sysdep.h', 'structs.h', 'utils.h', 'spells.h',
                     'fight.h', 'mud_event.h', 'actions.h'])
code = headers + r'''
#include <assert.h>
#undef CHECK_PLAYER_SPECIAL
#define CHECK_PLAYER_SPECIAL(ch,var) (var)
#define NORMAL_ATTACK_ROUTINE 0
#define RETURN_NUM_ATTACKS 1
#define DISPLAY_ROUTINE_POTENTIAL 2
#define HIT_NEED_RELOAD -1
#define HIT_MISS 0
#define PHASE_0 0
#define PHASE_1 1
#define PHASE_2 2
#define PHASE_3 3
static bool move_ready, standard_ready=true, ranged, blast, reload, empty_launcher;
static int hits, blasts;
static struct obj_data weapon;
#define is_action_available(ch,action,msg) ((action)==atMOVE ? move_ready : standard_ready)
#define send_to_char(...) ((void)0)
#define guard_check(...) ((void)0)
#define get_wielded(...) NULL
#define combat_distance(...) 50
#define combat_melee_range(...) 5
#define get_throwing_source(...) NULL
#define valid_fight_cond(...) true
#undef HAS_FEAT
#define HAS_FEAT(ch,feat) blast
#define call_magic(...) (blasts++)
#define can_fire_ammo(...) ranged
#define ranged_weapon_melee_type(...) (empty_launcher ? WEAPON_TYPE_QUARTERSTAFF : 0)
#define is_throwing_weapon(...) false
#define is_using_ranged_weapon(...) ((ranged || empty_launcher) ? &weapon : NULL)
#define is_reloading_weapon(...) false
#define weapon_is_loaded(...) true
#define auto_reload_weapon(...) ((void)0)
#define compute_attack_bonus(...) 0
#define compute_hit_damage(...) ((void)0)
#define stop_fighting(...) ((void)0)
#define hit(ch,...) (hits++,GET_ATTACKS_THIS_ROUND(ch)++,reload ? HIT_NEED_RELOAD : HIT_MISS)
''' + function('static int perform_staggered_attack(') + '\n' + routing + r'''
int main(void) {
 struct char_data ch={0},victim={0};
 struct player_special_data specials={0};
 ch.player_specials=&specials;FIGHTING(&ch)=&victim;GET_POS(&ch)=POS_FIGHTING;
 for(int npc=0;npc<2;npc++) {
  if(npc) SET_BIT_AR(MOB_FLAGS(&ch),MOB_ISNPC);
  for(int kind=0;kind<4;kind++) {
   if(npc && kind==2) continue;
   ranged=kind==1;blast=kind==2;empty_launcher=kind==3;FIRING(&ch)=ranged;
   SET_BIT_AR(PRF_FLAGS(&ch),PRF_AUTOBLAST);
   GET_ELDRITCH_SHAPE(&ch)=0;
   for(int phase0=0;phase0<2;phase0++) {
    move_ready=false;GET_ATTACKS_THIS_ROUND(&ch)=0;hits=blasts=0;
    assert(perform_attacks(&ch,RETURN_NUM_ATTACKS,1)==1);
    assert(hits+blasts==0);
    if(!npc && ranged)FIRING(&ch)=false; // quiver refilled after melee fallback
    perform_attacks(&ch,NORMAL_ATTACK_ROUTINE,phase0 ? 0 : 1);
    perform_attacks(&ch,NORMAL_ATTACK_ROUTINE,1);
    perform_attacks(&ch,NORMAL_ATTACK_ROUTINE,2);
    perform_attacks(&ch,NORMAL_ATTACK_ROUTINE,3);
    assert(hits+blasts==1 && GET_ATTACKS_THIS_ROUND(&ch)==1);
   }
   standard_ready=false;GET_ATTACKS_THIS_ROUND(&ch)=0;hits=blasts=0;
   perform_attacks(&ch,NORMAL_ATTACK_ROUTINE,1);assert(hits+blasts==0);
   standard_ready=true;move_ready=true;
   assert(perform_attacks(&ch,NORMAL_ATTACK_ROUTINE,1)==42);
  }
 }
 puts("PASS: PC/NPC melee, ranged and eldritch single attacks, all phases, repeated calls, standard-action gating and full-attack eligibility.");
}
'''
with tempfile.TemporaryDirectory(prefix='move-action-attacks-') as directory:
    p=Path(directory);(p/'test.c').write_text(code)
    subprocess.run(['cc','-std=gnu99','-g','-fsanitize=address,undefined',
                    '-fno-pie','-no-pie','-I'+str(root/'src'),str(p/'test.c'),'-o',str(p/'test')],check=True)
    subprocess.run([str(p/'test')],check=True)
