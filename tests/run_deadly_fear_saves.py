#!/usr/bin/env python3
"""Run Deadly Fear's production save branch with controlled save outcomes."""
from pathlib import Path
import os
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'src/magic.c').read_text()
start = source.index('  else if (spellnum == PSIONIC_DEADLY_FEAR)\n')
branch = source[start:source.index('  else if (spellnum == PSIONIC_PSYCHIC_CRUSH)', start)]
headers = ''.join('#include "'+h+'"\n' for h in ['conf.h', 'sysdep.h', 'structs.h', 'utils.h', 'spells.h', 'domains_schools.h'])
code = r'''
#include <assert.h>
static int will_pass, fort_pass, rolls, expected_bonus;
int savingthrow(struct char_data *ch, struct char_data *vict, int type, int modifier,
                int casttype, int level, int school) {
  assert(type == (rolls == 0 ? SAVING_WILL : SAVING_FORT));
  assert(casttype == CAST_SPELL && level == 17 && school == NOSCHOOL);
  assert(modifier == 3);
  assert(GET_DC_BONUS(ch) == expected_bonus);
  GET_DC_BONUS(ch) = 0;
  return rolls++ == 0 ? will_pass : fort_pass;
}
int dice(int number, int size) {assert(number==3 && size==6); return 12;}
static int resolve(struct char_data *ch, struct char_data *victim, int bonus) {
  int spellnum=PSIONIC_DEADLY_FEAR, race_bonus=1, dc_mod=2;
  int casttype=CAST_SPELL, level=17, dam=1+bonus;
  if (0) {}
''' + branch + r'''
  return dam;
}
int main(void) {
  struct char_data ch={0}, victim={0}; struct player_special_data ps={0};
  ch.player_specials=&ps; GET_HIT(&victim)=500;
  for(int augment=0; augment<=20; augment++) {
    for(will_pass=0; will_pass<=1; will_pass++) {
      for(fort_pass=0; fort_pass<=1; fort_pass++) {
        rolls=0; GET_DC_BONUS(&ch)=3; expected_bonus=3+augment/2;
        int damage=resolve(&ch,&victim,augment/2);
        assert(rolls==(will_pass ? 1 : 2));
        assert(damage==(will_pass ? 0 : fort_pass ? 12 : 600));
      }
    }
  }
  return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='deadly-fear-saves-') as tmp:
    path = Path(tmp)/'check.c'
    path.write_text(headers+code)
    binary = Path(tmp)/'check'
    subprocess.run([os.environ.get('CC','cc'), '-std=gnu99', '-Wall', '-Wextra', '-Wno-unused-parameter', '-I'+str(root/'src'), str(path), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print('Deadly Fear saves: Will negates; failed Will leads to Fortitude; Fortitude success deals 3d6, failure is lethal; both DC bonuses match at 0–20 augmentation.')
