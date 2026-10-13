#!/usr/bin/env python3
"""Check harvesting Insightful ranks and XP stacking using production functions."""
from pathlib import Path
import os
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'src/crafting_new.c').read_text()
def function(signature):
    start = source.index(signature + '\n{')
    return source[start:source.index('\n}', start) + 2]
code = function('int get_insightful_talent_bonus(struct char_data *ch, int skill)')
code += function('void gain_craft_exp(struct char_data *ch, int exp, int abil, bool verbose)')
headers = '\n'.join('#include "' + h + '"' for h in
                   ['conf.h', 'sysdep.h', 'structs.h', 'utils.h', 'spells.h',
                    'comm.h', 'constants.h', 'crafting_new.h', 'talents.h'])
stubs = r'''
#include <assert.h>
struct happyhour happy_data;
struct player_special_data dummy_mob;
void basic_mud_log(const char *fmt, ...) { assert(0); }
const char *ability_names[NUM_ABILITIES + 1];
static int rank, expected_talent;
static char output[8192];
int MAX(int a, int b) { return a > b ? a : b; }
int get_talent_rank(struct char_data *ch, int talent) {
  if (expected_talent) assert(talent == expected_talent);
  return rank;
}
int get_craft_skill_value(struct char_data *ch, int skill) { return 5; }
int craft_skill_level_exp(struct char_data *ch, int level) { return 21000; }
void gain_talent_point(struct char_data *ch, int amount) { assert(0); }
size_t send_to_char(struct char_data *ch, const char *fmt, ...) {
  va_list ap; va_start(ap,fmt); int n=vsnprintf(output+strlen(output),sizeof(output)-strlen(output),fmt,ap);
  va_end(ap); return n;
}
'''
checks = r'''
int main(void) {
  struct char_data ch = {0}; struct player_special_data ps = {0};
  int skills[] = {ABILITY_HARVEST_MINING, ABILITY_HARVEST_HUNTING,
    ABILITY_HARVEST_FORESTRY, ABILITY_HARVEST_GATHERING, ABILITY_HARVEST_BUTCHERING};
  int talents[] = {TALENT_INSIGHTFUL_MINING, TALENT_INSIGHTFUL_HUNTING,
    TALENT_INSIGHTFUL_FORESTRY, TALENT_INSIGHTFUL_GATHERING, TALENT_INSIGHTFUL_BUTCHERING};
  const char *names[] = {"mining", "hunting", "forestry", "gathering", "butchering"};
  int i;
  ch.player_specials = &ps;
  for (i = 0; i < 5; i++) {
    ability_names[skills[i]] = names[i];
    expected_talent = talents[i];
    for (rank = 0; rank <= 5; rank++) {
      assert(get_insightful_talent_bonus(&ch, skills[i]) == rank * 20);
      ps.saved.ability_exp[skills[i]] = 0;
      output[0] = 0;
      gain_craft_exp(&ch, 70, skills[i], FALSE);
      assert(ps.saved.ability_exp[skills[i]] == 70 + 14 * rank);
      char expected[128];
      snprintf(expected,sizeof(expected),"XP %d | TNL %d (next rank: 6).",70+14*rank,21000-(70+14*rank));
      assert(strstr(output,expected));
      assert(output[0]==toupper((unsigned char)names[i][0]));
    }
  }
  expected_talent = TALENT_INSIGHTFUL_MINING; rank = 1;
  ps.saved.craft_data.craft_specialization[0] = ABILITY_HARVEST_MINING;
  happy_data.harvesting_exp_rate = 50;
  ps.saved.ability_exp[ABILITY_HARVEST_MINING] = 0;
  gain_craft_exp(&ch, 70, ABILITY_HARVEST_MINING, TRUE);
  assert(ps.saved.ability_exp[ABILITY_HARVEST_MINING] == 70 + 70 + 14 + 35);
  expected_talent = TALENT_INSIGHTFUL_WOODWORKING; rank = 5;
  assert(get_insightful_talent_bonus(&ch, ABILITY_CRAFT_WOODWORKING) == 25);
  assert(get_insightful_talent_bonus(&ch, -1) == 0);
  expected_talent=0; rank=0;
  memset(&ps,0,sizeof(ps)); happy_data.crafting_exp_rate=25; happy_data.harvesting_exp_rate=50;
  for(int skill=ABILITY_CRAFT_WOODWORKING;skill<=ABILITY_CRAFT_COOKING;skill++) {
    ability_names[skill]="crafting"; gain_craft_exp(&ch,100,skill,FALSE);
    assert(GET_CRAFT_SKILL_EXP((&ch),skill)==125);
  }
  for(int skill=ABILITY_HARVEST_MINING;skill<=ABILITY_HARVEST_BUTCHERING;skill++) {
    ability_names[skill]="harvesting"; gain_craft_exp(&ch,100,skill,FALSE);
    assert(GET_CRAFT_SKILL_EXP((&ch),skill)==150);
  }
  return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='insightful-harvesting-') as tmp:
    path = Path(tmp) / 'check.c'
    path.write_text(headers + stubs + code + checks)
    binary = Path(tmp) / 'check'
    subprocess.run([os.environ.get('CC', 'cc'), '-std=gnu99', '-Wall', '-Wextra',
                    '-Wno-unused-parameter', '-I' + str(root / 'src'),
                    str(path), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print('Insightful harvesting: all five skills/ranks, XP awards, bonus stacking, crafting rate, and current XP/TNL feedback passed.')
