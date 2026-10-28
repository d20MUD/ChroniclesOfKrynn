#!/usr/bin/env python3
"""Exercise production Elbow Grease scaling and item check outcomes."""
from pathlib import Path
import os
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]

def function(file, signature):
    source = (root / 'src' / file).read_text()
    start = source.index(signature + '\n{')
    return source[start:source.index('\n}', start) + 2]

headers = ''.join(f'#include "{h}"\n' for h in
                  ['conf.h', 'sysdep.h', 'structs.h', 'utils.h', 'spells.h',
                   'comm.h', 'crafting_new.h', 'feats.h'])
code = r'''
#include <assert.h>
struct player_special_data dummy_mob;
void basic_mud_log(const char *fmt, ...) { assert(0); }
static int feat, roll = 10;
static char output[4096];
int get_feat_value(struct char_data *ch, int n) { return n == FEAT_ELBOW_GREASE ? feat : 0; }
int d20(struct char_data *ch) { return roll; }
int get_proficient_talent_bonus(struct char_data *ch, int skill) { return 0; }
bool artificer_efficient_crafter_applies(struct char_data *ch) { return FALSE; }
void gain_craft_exp(struct char_data *ch, int exp, int abil, bool verbose) {}
void reset_current_craft(struct char_data *ch, char *arg, bool a, bool b) { assert(0); }
size_t send_to_char(struct char_data *ch, const char *fmt, ...) {
  va_list ap; va_start(ap, fmt); int n = vsnprintf(output, sizeof(output), fmt, ap); va_end(ap); return n;
}
'''
code += function('utils.c', 'int get_elbow_grease_bonus(struct char_data *ch)')
code += function('crafting_new.c', 'int get_craft_skill_value(struct char_data *ch, int skill_num)')
code += function('crafting_new.c', 'bool create_craft_skill_check(struct char_data *ch, struct obj_data *obj, int skill, char *method,\n                              int exp, int dc)')
code += r'''
int main(void) {
  struct char_data ch = {0}; struct player_special_data ps = {0}; struct obj_data obj = {0};
  int skill = ABILITY_CRAFT_WOODWORKING;
  ch.player_specials = &ps; obj.short_description = "test item";
  SET_ABILITY((&ch), skill, 5);
  for (int level = 0; level <= 20; ++level) {
    CLASS_LEVEL((&ch), CLASS_ARTIFICER) = level;
    for (feat = 0; feat <= 1; ++feat) {
      int expected = !feat || !level ? 0 : level >= 10 ? 6 : level >= 6 ? 4 : 2;
      assert(get_elbow_grease_bonus(&ch) == expected);
      assert(get_craft_skill_value(&ch, skill) == 5);
      assert(create_craft_skill_check(&ch, &obj, skill, "craft", 0, 15 + expected));
      assert(!create_craft_skill_check(&ch, &obj, skill, "craft", 0, 16 + expected));
      assert(GET_ABILITY((&ch), skill) == 5);
    }
  }
  return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='elbow-grease-') as tmp:
    source = Path(tmp) / 'check.c'
    source.write_text(headers + code)
    binary = Path(tmp) / 'check'
    subprocess.run([os.environ.get('CC', 'cc'), '-std=gnu99', '-Wno-unused-parameter',
                    '-I' + str(root / 'src'), str(source), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print('Elbow Grease: feat gating, levels 0–20, item check boundaries, and unchanged skill ranks passed.')
