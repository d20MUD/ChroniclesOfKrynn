#!/usr/bin/env python3
"""Check every configured background skill pair and the shared modifier hook."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'src/backgrounds.c').read_text()
def function(signature):
    start = source.index(signature)
    return source[start:source.index('\n}', start) + 2]

skills = (root / 'src/spec_procs.c').read_text()
start = skills.index('int compute_ability_full(')
calculation = skills[start:skills.index('\n}', start) + 2]
assert calculation.count('value += background_skill_bonus(ch, abilityNum);') == 1
assert 'background_list[GET_BACKGROUND(ch)]' not in calculation

code = r'''
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "spells.h"
#include "backgrounds.h"
#include <assert.h>
#include <limits.h>
struct background_data background_list[NUM_BACKGROUNDS];
'''
for signature in ['static void backgroundo(', 'int background_skill_bonus(',
                  'void initialize_background_list(', 'void assign_backgrounds(']:
    code += '\n' + function(signature)
code += r'''
int main(void) {
  struct char_data player = {0}, npc = {0};
  struct player_special_data special = {0};
  player.player_specials = &special;
  /* NPCs need no player_specials allocation to safely query this modifier. */
  SET_BIT_AR(MOB_FLAGS(&npc), MOB_ISNPC);
  const int expected[NUM_BACKGROUNDS][2] = {
    [BACKGROUND_ACOLYTE] = {ABILITY_SENSE_MOTIVE, ABILITY_RELIGION},
    [BACKGROUND_CHARLATAN] = {ABILITY_SLEIGHT_OF_HAND, ABILITY_BLUFF},
    [BACKGROUND_CRIMINAL] = {ABILITY_BLUFF, ABILITY_STEALTH},
    [BACKGROUND_ENTERTAINER] = {ABILITY_ACROBATICS, ABILITY_PERFORM},
    [BACKGROUND_FOLK_HERO] = {ABILITY_HANDLE_ANIMAL, ABILITY_SURVIVAL},
    [BACKGROUND_GLADIATOR] = {ABILITY_ACROBATICS, ABILITY_PERFORM},
    [BACKGROUND_TRADER] = {ABILITY_SENSE_MOTIVE, ABILITY_DIPLOMACY},
    [BACKGROUND_HERMIT] = {ABILITY_HEAL, ABILITY_RELIGION},
    [BACKGROUND_SQUIRE] = {ABILITY_HISTORY, ABILITY_DIPLOMACY},
    [BACKGROUND_NOBLE] = {ABILITY_HISTORY, ABILITY_DIPLOMACY},
    [BACKGROUND_OUTLANDER] = {ABILITY_ATHLETICS, ABILITY_SURVIVAL},
    [BACKGROUND_PIRATE] = {ABILITY_ATHLETICS, ABILITY_PERCEPTION},
    [BACKGROUND_SAGE] = {ABILITY_ARCANA, ABILITY_HISTORY},
    [BACKGROUND_SAILOR] = {ABILITY_ATHLETICS, ABILITY_PERCEPTION},
    [BACKGROUND_SOLDIER] = {ABILITY_ATHLETICS, ABILITY_INTIMIDATE},
    [BACKGROUND_URCHIN] = {ABILITY_SLEIGHT_OF_HAND, ABILITY_STEALTH},
  };
  assign_backgrounds();
  for (int bg = BACKGROUND_NONE; bg < NUM_BACKGROUNDS; bg++) {
    GET_BACKGROUND((&player)) = bg;
    for (int skill = 1; skill <= NUM_ABILITIES; skill++) {
      int bonus = expected[bg][0] == skill || expected[bg][1] == skill ? 2 : 0;
      assert(background_skill_bonus(&player, skill) == bonus);
      assert(background_skill_bonus(&npc, skill) == 0);
    }
  }
  GET_BACKGROUND((&player)) = BACKGROUND_ACOLYTE;
  assert(background_skill_bonus(&player, ABILITY_RELIGION) == 2);
  assert(background_skill_bonus(&player, 0) == 0);
  assert(background_skill_bonus(&player, -1) == 0);
  assert(background_skill_bonus(&player, NUM_ABILITIES + 1) == 0);
  assert(background_skill_bonus(NULL, ABILITY_RELIGION) == 0);
  int invalid[] = {-1, INT_MIN, NUM_BACKGROUNDS, INT_MAX};
  for (unsigned int i = 0; i < sizeof(invalid)/sizeof(invalid[0]); i++) {
    GET_BACKGROUND((&player)) = invalid[i];
    assert(background_skill_bonus(&player, ABILITY_RELIGION) == 0);
  }
  puts("Background skills: all 16 skill pairs grant +2, unrelated skills grant zero, and none/NPC/invalid values are safe.");
  return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='background-skills-') as directory:
    path = Path(directory) / 'check.c'
    binary = Path(directory) / 'check'
    path.write_text(code)
    subprocess.run(['cc', '-std=gnu99', '-I' + str(root / 'src'),
                    '-fsanitize=address,undefined', str(path), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
