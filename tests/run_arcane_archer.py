#!/usr/bin/env python3
"""Exercise Arcane Archer accuracy and readied Death Arrow consumption."""
from pathlib import Path
import os
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
fight = (root / 'src/fight.c').read_text()
offense = (root / 'src/act.offensive.c').read_text()

def block(source, marker):
    start = source.index(marker)
    opening = source.index('{', start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]

attack = block(fight, 'int compute_attack_bonus_full(')
a = attack.index('    if (GET_OBJ_VAL(ammo, 0) == AMMO_TYPE_ARROW)')
accuracy = block(attack[a:], 'if (GET_OBJ_VAL')
miss = block(fight, 'void handle_missed_attack(')
a = miss.index('  if (attack_type == ATTACK_TYPE_RANGED && missile &&')
consume = block(miss[a:], 'if (attack_type')
for name in ['can_arrowswarm', 'can_seekerarrow', 'can_deatharrow']:
    assert '!= AMMO_TYPE_ARROW' in block(offense, 'ACMDCHECK(' + name + ')')
seeker = block(offense, 'void perform_seekerarrow(')
assert seeker.index('combat_attack_in_range') < seeker.index('start_daily_use_cooldown')
assert 'combat_attack_in_range' in block(offense, 'static int arrowswarm_callback(')
assert 'if (!aoe_effect' in block(offense, 'ACMD(do_arrowswarm)')
success = block(fight, 'int handle_successful_attack(')
assert 'savingthrow_flatdc(ch, victim, SAVING_FORT, 0, deatharrow_dc,' in success
assert '!is_immune_death_magic(ch, victim, false)' in success

code = r'''
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "spells.h"
#include <assert.h>
#define send_to_char(...) ((void)0)
static int rank, consumed;
int get_feat_value(struct char_data *ch, int feat) {return rank;}
int affected_by_spell(struct char_data *ch, int spell) {return 1;}
void affect_from_char(struct char_data *ch, int spell) {consumed++;}
static int accuracy(struct char_data *ch, struct obj_data *ammo) {
  int calc_bab=10; bool display=false;
''' + accuracy + r'''
  return calc_bab;
}
static void miss(struct char_data *ch, struct obj_data *missile, int attack_type) {
''' + consume + r'''
}
int main(void) {
  struct char_data ch={0}; struct obj_data ammo={0};
  for(rank=0;rank<=5;rank++) {
    GET_OBJ_VAL(&ammo,0)=AMMO_TYPE_ARROW;assert(accuracy(&ch,&ammo)==10+rank);
    GET_OBJ_VAL(&ammo,0)=AMMO_TYPE_BOLT;assert(accuracy(&ch,&ammo)==10);
    GET_OBJ_VAL(&ammo,0)=AMMO_TYPE_THROWN;assert(accuracy(&ch,&ammo)==10);
  }
  GET_OBJ_VAL(&ammo,0)=AMMO_TYPE_ARROW;
  miss(&ch,&ammo,ATTACK_TYPE_PRIMARY);assert(consumed==0);
  miss(&ch,NULL,ATTACK_TYPE_RANGED);assert(consumed==0);
  GET_OBJ_VAL(&ammo,0)=AMMO_TYPE_BOLT;
  miss(&ch,&ammo,ATTACK_TYPE_RANGED);assert(consumed==0);
  GET_OBJ_VAL(&ammo,0)=AMMO_TYPE_ARROW;
  miss(&ch,&ammo,ATTACK_TYPE_RANGED);assert(consumed==1);
  puts("PASS: Enhance Arrow ranks and ammo types, Death Arrow miss consumption, bow prerequisites, range/cooldown ordering and death save routing.");
}
'''
with tempfile.TemporaryDirectory(prefix='arcane-archer-') as directory:
    source = Path(directory) / 'archer.c'
    binary = Path(directory) / 'archer'
    source.write_text(code)
    subprocess.run([os.environ.get('CC', 'cc'), '-std=c99', '-I'+str(root/'src'), str(source), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
