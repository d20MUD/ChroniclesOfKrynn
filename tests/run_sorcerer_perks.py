#!/usr/bin/env python3
"""Exercise copied perk definitions, separate purchase rules, effects and migration."""
from pathlib import Path
import os
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'src/perks.c').read_text()

def function(signature):
    start = source.index(signature + '\n{')
    return source[start:source.index('\n}', start) + 2] + '\n'

headers = ''.join('#include "' + h + '"\n' for h in
                  ['conf.h', 'sysdep.h', 'structs.h', 'utils.h', 'comm.h',
                   'db.h', 'spells.h', 'perks.h', 'constants.h', 'class.h'])
code = r'''
#include <assert.h>
struct player_special_data dummy_mob;
void basic_mud_log(const char *fmt, ...) { assert(0); }
struct perk_data perk_list[NUM_PERKS];
struct class_table class_list[NUM_CLASSES];
const char *class_names[NUM_CLASSES+1];
int MAX(int a, int b) { return a > b ? a : b; }
#define PERK_TIER_2_MIN_LEVEL 6
#define PERK_TIER_3_MIN_LEVEL 12
#define PERK_TIER_4_MIN_LEVEL 18
'''
start = source.index('struct perk_tier_range\n{')
end = source.index('static int get_perk_tier_by_id', start)
code += source[start:end]
for signature in [
    'static int normalize_perk_class_id(int class_id)',
    'void migrate_sorcerer_perks(struct char_data *ch)',
    'int class_to_perk_class(int class_type, int which_perk)',
    'static bool is_loremaster_perk_candidate_class(int class_id)',
    'static int get_loremaster_advancement_count(struct char_data *ch, int class_id)',
    'static int get_loremaster_perk_award_class(struct char_data *ch)',
    'static void get_award_perk_classes(struct char_data *ch, int award_class, int *perk_class_1,\n                                   int *perk_class_2)',
    'static bool character_can_use_perk_class(struct char_data *ch, int class_id)',
    'static bool should_show_perk_class(struct char_data *ch, int class_id)',
    'static int get_perk_tier_by_id(int perk_id)',
    'static int get_perk_tier_min_level(int tier)',
    'struct perk_data *get_perk_by_id(int perk_id)',
    'int get_class_perks(int class_id, int *perk_ids, int max_perks)',
    'struct char_perk_data *find_char_perk(struct char_data *ch, int perk_id, int class_id)',
    'int get_perk_rank(struct char_data *ch, int perk_id, int class_id)',
    'int get_total_perk_ranks(struct char_data *ch, int perk_id)',
    'bool has_perk(struct char_data *ch, int perk_id)',
    'int get_arcane_perk_ranks(struct char_data *ch, int wizard_perk_id)',
    'bool has_arcane_perk(struct char_data *ch, int wizard_perk_id)',
    'int get_perk_points(struct char_data *ch, int class_id)',
    'bool spend_perk_points(struct char_data *ch, int class_id, int amount)',
    'bool can_purchase_perk(struct char_data *ch, int perk_id, int class_id, char *error_msg,\n                       size_t error_len)',
    'void define_wizard_perks(void)',
    'static void define_wizard_controller_perks(void)',
    'void define_wizard_versatile_caster_perks(void)',
    'void define_sorcerer_perks(void)',
    'static void define_sorcerer_controller_perks(void)',
    'void define_sorcerer_versatile_caster_perks(void)',
    'int get_wizard_spell_power_bonus(struct char_data *ch)',
]:
    code += function(signature)
code += r'''
int main(void) {
  struct char_data ch = {0};
  struct player_special_data ps = {0};
  struct char_perk_data owned = {0}, other = {0};
  int wizard[100], sorcerer[100], first, second;
  char error[1024];
  ch.player_specials = &ps;
  GET_LEVEL(&ch) = 20;
  CLASS_LEVEL((&ch), CLASS_SORCERER) = 20;
  define_wizard_perks(); define_wizard_controller_perks(); define_wizard_versatile_caster_perks();
  define_sorcerer_perks(); define_sorcerer_controller_perks(); define_sorcerer_versatile_caster_perks();
  int n = get_class_perks(CLASS_WIZARD, wizard, 100);
  assert(n > 40 && get_class_perks(CLASS_SORCERER, sorcerer, 100) == n);
  for (int i = 0; i < n; i++) {
    struct perk_data *w = get_perk_by_id(wizard[i]), *s = get_perk_by_id(sorcerer[i]);
    assert(s->id == w->id + 700 && s->associated_class == CLASS_SORCERER);
    assert(!strcmp(w->name, s->name));
    assert(w->name != s->name && w->description != s->description);
    assert(w->cost == s->cost && w->max_rank == s->max_rank);
    assert(w->perk_category == s->perk_category && w->effect_type == s->effect_type);
    assert(w->effect_value == s->effect_value && w->effect_modifier == s->effect_modifier);
    assert(w->prerequisite_rank == s->prerequisite_rank);
    assert(s->prerequisite_perk == (w->prerequisite_perk >= 100 ? w->prerequisite_perk + 700 : w->prerequisite_perk));
    assert(get_perk_tier_by_id(w->id) == get_perk_tier_by_id(s->id));
  }
  get_award_perk_classes(&ch, CLASS_SORCERER, &first, &second);
  assert(first == CLASS_SORCERER && second == CLASS_SORCERER);
  assert(should_show_perk_class(&ch, CLASS_SORCERER));
  assert(!character_can_use_perk_class(&ch, CLASS_WIZARD));
  /* Move the old shared pool and purchases once, preserving their values. */
  ps.saved.perk_points[CLASS_WIZARD] = 30;
  ps.saved.perk_points[CLASS_SORCERER] = 2;
  owned.perk_id = PERK_WIZARD_SPELL_POWER_1; owned.perk_class = CLASS_WIZARD;
  owned.current_rank = 5; ps.saved.perks = &owned;
  migrate_sorcerer_perks(&ch);
  assert(owned.perk_id == PERK_SORCERER_SPELL_POWER_1 && owned.perk_class == CLASS_SORCERER);
  assert(owned.current_rank == 5 && ps.saved.perk_points[CLASS_SORCERER] == 32);
  assert(ps.saved.perk_points[CLASS_WIZARD] == 0 && ps.saved.sorcerer_perks_migrated);
  migrate_sorcerer_perks(&ch); assert(ps.saved.perk_points[CLASS_SORCERER] == 32);
  assert(!has_perk(&ch, PERK_WIZARD_SPELL_POWER_1));
  assert(has_arcane_perk(&ch, PERK_WIZARD_SPELL_POWER_1));
  assert(get_wizard_spell_power_bonus(&ch) == 5);
  assert(can_purchase_perk(&ch, PERK_SORCERER_SPELL_POWER_2, CLASS_SORCERER, error, sizeof(error)));
  assert(!can_purchase_perk(&ch, PERK_WIZARD_SPELL_POWER_2, CLASS_SORCERER, error, sizeof(error)));
  assert(spend_perk_points(&ch, CLASS_SORCERER, 3));
  assert(ps.saved.perk_points[CLASS_SORCERER] == 29 && ps.saved.perk_points[CLASS_WIZARD] == 0);
  /* Wizard ranks cannot fulfill sorcerer prerequisites. */
  CLASS_LEVEL((&ch), CLASS_WIZARD) = 1;
  owned.perk_id = PERK_WIZARD_SPELL_POWER_1; owned.perk_class = CLASS_WIZARD;
  assert(!can_purchase_perk(&ch, PERK_SORCERER_SPELL_POWER_2, CLASS_SORCERER, error, sizeof(error)));
  other.perk_id = PERK_SORCERER_SPELL_POWER_1; other.perk_class = CLASS_SORCERER;
  other.current_rank = 5; owned.next = &other;
  assert(get_arcane_perk_ranks(&ch, PERK_WIZARD_SPELL_POWER_1) == 5);
  assert(can_purchase_perk(&ch, PERK_SORCERER_SPELL_POWER_2, CLASS_SORCERER, error, sizeof(error)));
  GET_LEVEL(&ch) = 5;
  assert(!can_purchase_perk(&ch, PERK_SORCERER_SPELL_POWER_2, CLASS_SORCERER, error, sizeof(error)));
  assert(strstr(error, "level 6"));
  /* Mixed characters retain the ambiguous historical pool and purchases. */
  ps.saved.sorcerer_perks_migrated = FALSE; ps.saved.perk_points[CLASS_WIZARD] = 7;
  migrate_sorcerer_perks(&ch);
  assert(owned.perk_id == PERK_WIZARD_SPELL_POWER_1 && ps.saved.perk_points[CLASS_WIZARD] == 7);
  for (int i = 0; i < NUM_PERKS; i++) {
    free(perk_list[i].name); free(perk_list[i].description); free(perk_list[i].special_description);
  }
  return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='sorcerer-perks-') as tmp:
    path = Path(tmp) / 'check.c'
    path.write_text(headers + code)
    binary = Path(tmp) / 'check'
    subprocess.run([os.environ.get('CC', 'cc'), '-std=gnu99', '-Wall', '-Wextra',
                    '-Wno-unused-parameter', '-I' + str(root / 'src'), str(path),
                    '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print('Sorcerer perks: definition parity, IDs, tiers, class access, point pools, prerequisites, effects, and migration passed.')
