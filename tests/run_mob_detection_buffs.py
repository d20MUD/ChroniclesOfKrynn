#!/usr/bin/env python3
"""Verify useful mob detection buffs and complete campaign spell pools."""
from pathlib import Path
import os
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'src/mob_spells.c').read_text()
def function(signature):
    start = source.index(signature + '\n{')
    return source[start:source.index('\n}', start) + 2]
start = source.index('#if defined(CAMPAIGN_DL)')
arrays = source[start:source.index('/* Wizard/Sorcerer group buff', start)]
functions = function('bool mob_can_auto_buff_with_spell(int spellnum)')
functions += function('bool wizard_is_long_duration_buff(int spellnum)')
headers = '\n'.join('#include "' + h + '"' for h in
                    ['conf.h', 'sysdep.h', 'structs.h', 'utils.h', 'spells.h'])
checks = r'''
#include <assert.h>
struct spell_info_type spell_info[NUM_SPELLS + 1];
#define SINFO spell_info[spellnum]
'''
main = r'''
int main(void) {
  int i, invis = 0, seeing = 0;
  for (i = 0; i < SPELLUP_SPELLS; i++) {
    int spell = valid_spellup_spell[i];
    assert(spell > 0 && spell < NUM_SPELLS);
    assert(spell != SPELL_DETECT_MAGIC && spell != SPELL_DETECT_POISON && spell != SPELL_DETECT_ALIGN);
    invis += spell == SPELL_DETECT_INVIS;
    seeing += spell == SPELL_TRUE_SEEING;
  }
  assert(invis == 1 && seeing == 1);
  assert(!mob_can_auto_buff_with_spell(SPELL_DETECT_MAGIC));
  assert(!mob_can_auto_buff_with_spell(SPELL_DETECT_POISON));
  assert(!mob_can_auto_buff_with_spell(SPELL_DETECT_ALIGN));
  assert(!mob_can_auto_buff_with_spell(SPELL_INVISIBLE));
  assert(mob_can_auto_buff_with_spell(SPELL_DETECT_INVIS));
  assert(mob_can_auto_buff_with_spell(SPELL_TRUE_SEEING));
  spell_info[SPELL_DETECT_INVIS].routines = MAG_AFFECTS;
  spell_info[SPELL_TRUE_SEEING].routines = MAG_AFFECTS;
  spell_info[SPELL_DETECT_MAGIC].routines = MAG_AFFECTS;
  assert(wizard_is_long_duration_buff(SPELL_DETECT_INVIS));
  assert(wizard_is_long_duration_buff(SPELL_TRUE_SEEING));
  assert(!wizard_is_long_duration_buff(SPELL_DETECT_MAGIC));
  spell_info[SPELL_TRUE_SEEING].violent = TRUE;
  assert(!wizard_is_long_duration_buff(SPELL_TRUE_SEEING));
  return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='mob-detection-buffs-') as tmp:
    for dl in [False, True]:
        mode = '\n#undef CAMPAIGN_DL\n' + ('#define CAMPAIGN_DL\n' if dl else '')
        path = Path(tmp) / 'check.c'
        path.write_text(headers + mode + '#include "mob_spells.h"\n' + arrays + checks + functions + main)
        binary = Path(tmp) / 'check'
        subprocess.run([os.environ.get('CC', 'cc'), '-std=gnu99', '-Wall', '-Wextra',
                        '-I' + str(root / 'src'), str(path), '-o', str(binary)], check=True)
        subprocess.run([str(binary)], check=True)
print('Mob detection buffs: both campaigns, pool sizes, utility exclusions, and useful pre-buffs passed.')
