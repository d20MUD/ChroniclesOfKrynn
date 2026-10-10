#!/usr/bin/env python3
"""Exercise mounted spell access using production helpers and game structures."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
src = root / "src"

def function(filename, signature):
    text = (src / filename).read_text()
    start = text.index(signature)
    return text[start:text.index("\n}", start) + 2]

code = '\n'.join('#include "' + h + '"' for h in
                 ['conf.h', 'sysdep.h', 'structs.h', 'utils.h', 'spells.h', 'db.h'])
code += r'''
#include <assert.h>
struct index_data *mob_index;
mob_rnum top_of_mobt = 0;
static int uses = 1;
int daily_uses_remaining(struct char_data *ch, int feat) {
  assert(feat == FEAT_DRAGOON_POINTS);
  return uses;
}
'''
code += function('utils.c', 'bool is_dragon_rider_mount(struct char_data *ch)')
code += function('utils.c', 'bool is_riding_dragon_mount(struct char_data *ch)')
code += function('spell_parser.c', 'bool isDragonRiderMagic(struct char_data *ch, int spellnum)')
code += r'''
int main(void) {
  struct char_data rider = {0}, dragon = {0}, other = {0};
  struct player_special_data special = {0};
  struct index_data index = {0};
  mob_index = &index;
  index.vnum = 40401;
  dragon.nr = 0;
  SET_BIT_AR(MOB_FLAGS(&dragon), MOB_ISNPC);
  rider.player_specials = &special;
  CLASS_LEVEL((&rider), CLASS_DRAGONRIDER) = 1;
  HAS_REAL_FEAT(&rider, FEAT_DRAGON_FLIGHT) = 1;
  HAS_REAL_FEAT(&rider, FEAT_DRAGOON_POINTS) = 1;
  RIDING(&rider) = &dragon;
  dragon.master = &rider;
  rider.in_room = dragon.in_room = 1;
  assert(isDragonRiderMagic(&rider, SPELL_OVERLAND_FLIGHT));
  RIDING(&rider) = NULL;
  assert(!isDragonRiderMagic(&rider, SPELL_OVERLAND_FLIGHT));
  RIDING(&rider) = &dragon;
  dragon.master = &other;
  assert(!isDragonRiderMagic(&rider, SPELL_OVERLAND_FLIGHT));
  dragon.master = &rider;
  dragon.in_room = 2;
  assert(!isDragonRiderMagic(&rider, SPELL_OVERLAND_FLIGHT));
  dragon.in_room = 1;
  index.vnum = 20803;
  assert(!isDragonRiderMagic(&rider, SPELL_OVERLAND_FLIGHT));
  index.vnum = 40401;
  HAS_REAL_FEAT(&rider, FEAT_DRAGON_FLIGHT) = 0;
  assert(!isDragonRiderMagic(&rider, SPELL_OVERLAND_FLIGHT));
  HAS_REAL_FEAT(&rider, FEAT_DRAGON_FLIGHT) = 1;
  CLASS_LEVEL((&rider), CLASS_DRAGONRIDER) = 0;
  assert(!isDragonRiderMagic(&rider, SPELL_OVERLAND_FLIGHT));
  CLASS_LEVEL((&rider), CLASS_DRAGONRIDER) = 1;
  uses = 0;
  assert(!isDragonRiderMagic(&rider, SPELL_OVERLAND_FLIGHT));
  uses = -1;
  assert(!isDragonRiderMagic(&rider, SPELL_OVERLAND_FLIGHT));
  uses = 1;
  HAS_REAL_FEAT(&rider, FEAT_DRAGOON_POINTS) = 0;
  assert(!isDragonRiderMagic(&rider, SPELL_OVERLAND_FLIGHT));
  HAS_REAL_FEAT(&rider, FEAT_DRAGOON_POINTS) = 1;
  HAS_REAL_FEAT(&rider, FEAT_ADEPT_RIDER) = 1;
  RIDING(&rider) = NULL;
  assert(isDragonRiderMagic(&rider, SPELL_DARKNESS));
  assert(!isDragonRiderMagic(NULL, SPELL_OVERLAND_FLIGHT));
  assert(!isDragonRiderMagic(&dragon, SPELL_OVERLAND_FLIGHT));
  puts("Dragon Flight access checks passed.");
  return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='dragon-flight-') as temp:
    source = Path(temp) / 'check.c'
    exe = Path(temp) / 'check'
    source.write_text(code)
    subprocess.run(['cc', '-I', str(src), '-fsanitize=address,undefined', '-g',
                    '-o', str(exe), str(source)], check=True)
    subprocess.run([str(exe)], check=True)
