#!/usr/bin/env python3
"""Check real crafting/harvest XP awards and rank progression without skill modifiers."""
from pathlib import Path
import os
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'src/crafting_new.c').read_text()


def function(signature):
    start = source.index(signature)
    return source[start:source.index('\n}', start) + 2]


# Both score rows must use the same remaining-XP definition as the award message.
assert source.count('MAX(0, craft_skill_level_exp(ch, base_rank + 1) - GET_CRAFT_SKILL_EXP(ch, abil))') == 2
headers = ['conf.h', 'sysdep.h', 'structs.h', 'utils.h', 'spells.h',
           'comm.h', 'crafting_new.h', 'talents.h']
code = '\n'.join('#include "' + header + '"' for header in headers)
code += r'''
#include <assert.h>
struct happyhour happy_data;
struct player_special_data dummy_mob;
const char *ability_names[NUM_ABILITIES + 1];
static char output[8192];
static int talent_points, insightful;
void basic_mud_log(const char *format, ...) {assert(0);}
int MAX(int a, int b) {return a > b ? a : b;}
int get_insightful_talent_bonus(struct char_data *ch, int ability) {return insightful;}
void gain_talent_point(struct char_data *ch, int amount) {talent_points += amount;}
/* Effective values must never be queried to determine progression. */
int get_craft_skill_value(struct char_data *ch, int ability) {assert(0); return 99;}
size_t send_to_char(struct char_data *ch, const char *format, ...) {
  va_list args; va_start(args, format);
  int n = vsnprintf(output + strlen(output), sizeof(output) - strlen(output), format, args);
  va_end(args); return n;
}
'''
code += function('int craft_skill_level_exp(') + '\n' + function('void gain_craft_exp(')
code += r'''
static void reset(struct char_data *ch) {
  memset(ch->player_specials, 0, sizeof(*ch->player_specials));
  GET_CRAFT(ch).craft_specialization[0] = GET_CRAFT(ch).craft_specialization[1] = -1;
  memset(&happy_data, 0, sizeof(happy_data));
  talent_points = insightful = 0; output[0] = '\0';
}
int main(void) {
  struct char_data ch = {0}; struct player_special_data ps = {0}; ch.player_specials = &ps;
  for (int i = 0; i <= NUM_ABILITIES; i++) ability_names[i] = "skill";
  ability_names[ABILITY_CRAFT_WOODWORKING] = "woodworking";
  int skills[] = {ABILITY_CRAFT_WOODWORKING, ABILITY_CRAFT_ALCHEMY,
    ABILITY_HARVEST_MINING, ABILITY_HARVEST_FORESTRY, ABILITY_HARVEST_GATHERING,
    ABILITY_HARVEST_HUNTING, ABILITY_HARVEST_SURVEYING, ABILITY_HARVEST_BUTCHERING};
  for (unsigned i = 0; i < sizeof(skills) / sizeof(skills[0]); i++) {
    int skill = skills[i]; reset(&ch);
    GET_CRAFT((&ch)).craft_specialization[0] = skill;
    GET_CRAFT_SKILL_EXP((&ch), skill) = 980;
    gain_craft_exp(&ch, 10, skill, true);
    assert(GET_CRAFT_SKILL_EXP((&ch), skill) == 1000); /* specialization still doubles XP */
    assert(GET_ABILITY((&ch), skill) == 1 && talent_points == 1);
    assert(strstr(output, "increased from 0 to 1!"));
    assert(strstr(output, "XP 1000 | TNL 2000 (next rank: 2)"));
    GET_CRAFT_SKILL_EXP((&ch), skill) = 2980; output[0] = '\0';
    gain_craft_exp(&ch, 10, skill, true);
    assert(GET_ABILITY((&ch), skill) == 2 && talent_points == 2);
    assert(strstr(output, "increased from 1 to 2!"));
    assert(strstr(output, "XP 3000 | TNL 3000 (next rank: 3)"));
  }
  /* The reported failure: 20 specialized XP at the first threshold is rank 1, not 6. */
  reset(&ch); GET_CRAFT((&ch)).craft_specialization[1] = ABILITY_CRAFT_WOODWORKING;
  GET_CRAFT_SKILL_EXP((&ch), ABILITY_CRAFT_WOODWORKING) = 980;
  gain_craft_exp(&ch, 10, ABILITY_CRAFT_WOODWORKING, true);
  assert(strstr(output, "Woodworking: XP 1000 | TNL 2000 (next rank: 2)"));
  assert(!strstr(output, "next rank: 6"));
  /* XP multipliers still apply, but never change which rank is next. */
  for (unsigned i = 0; i < 2; i++) {
    int skill = i ? ABILITY_HARVEST_MINING : ABILITY_CRAFT_WOODWORKING;
    reset(&ch); GET_CRAFT((&ch)).craft_specialization[0] = skill;
    insightful = 50; HAPPY_CRAFTING_EXP = HAPPY_HARVESTING_EXP = 50;
    GET_CRAFT_SKILL_EXP((&ch), skill) = 969;
    gain_craft_exp(&ch, 10, skill, false);
    assert(GET_CRAFT_SKILL_EXP((&ch), skill) == 999);
    assert(GET_ABILITY((&ch), skill) == 0 && talent_points == 0);
    assert(strstr(output, "TNL 1 (next rank: 1)"));
    gain_craft_exp(&ch, 10, skill, false);
    assert(GET_ABILITY((&ch), skill) == 1 && talent_points == 1);
  }
  puts("PASS: base-rank craft/harvest thresholds, rank assignment, TNL and unchanged XP bonuses.");
}
'''
with tempfile.TemporaryDirectory(prefix='craft-ranks-') as directory:
    cfile = Path(directory) / 'test.c'
    binary = Path(directory) / 'test'
    cfile.write_text(code)
    subprocess.run([os.environ.get('CC', 'cc'), '-std=c11', '-g',
                    '-fsanitize=address,undefined', '-fno-omit-frame-pointer',
                    '-I' + str(root / 'src'), str(cfile), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
