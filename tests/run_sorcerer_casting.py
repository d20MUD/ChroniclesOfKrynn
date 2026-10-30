#!/usr/bin/env python3
"""Regress Aurak source precedence and sorcerer selections versus casting slots."""
from pathlib import Path
import os
import subprocess
import tempfile
root = Path(__file__).resolve().parents[1]
prep = (root / 'src/spell_prep.c').read_text()
parser = (root / 'src/spell_parser.c').read_text()
constants = (root / 'src/constants.c').read_text()
def function(text, signature):
    a = text.index(signature + '\n{')
    return text[a:text.index('\n}', a) + 2] + '\n'
headers = ''.join('#include "' + h + '"\n' for h in [
    'conf.h', 'sysdep.h', 'structs.h', 'utils.h', 'comm.h', 'db.h', 'spells.h',
    'perks.h', 'constants.h', 'class.h', 'spell_prep.h', 'domains_schools.h'])
code = r'''
#include <assert.h>
struct player_special_data dummy_mob;
#define DEBUGMODE 0
int get_feat_value(struct char_data *ch, int feat) { return HAS_REAL_FEAT(ch, feat); }
void basic_mud_log(const char *fmt, ...) { assert(0); }
int MAX(int a, int b) { return a > b ? a : b; }
static int highest = 1, racial = 1;
int compute_bonus_caster_level(struct char_data *ch, int class) { return 0; }
int get_class_highest_circle(struct char_data *ch, int class) {
  return CLASS_LEVEL(ch, class) ? highest : 0;
}
int compute_spells_circle(struct char_data *ch, int class, int spell, int meta, int domain) { return 1; }
int get_bonus_spells_by_circle_and_class(struct char_data *ch, int class, int circle) { return 0; }
int has_loremaster_secret(struct char_data *ch, int secret) { return 0; }
int get_druid_bonus_spell_slots(struct char_data *ch) { return 0; }
int get_summoner_extended_spell_knowledge_bonus_spells(struct char_data *ch, int circle) { return 0; }
bool has_summoner_extended_spell_knowledge_2(struct char_data *ch) { return false; }
bool has_summoner_spell_weaver(struct char_data *ch) { return false; }
size_t send_to_char(struct char_data *ch, const char *fmt, ...) { return 0; }
bool has_epic_power(struct char_data *ch, int spell) { return false; }
int is_domain_spell_of_ch(struct char_data *ch, int spell) { return false; }
bool isAurakMagic(struct char_data *ch, int spell) { return racial; }
'''
for name in ['spell_bonus', 'ranger_slots', 'paladin_slots', 'cleric_slots', 'druid_slots',
             'wizard_slots', 'sorcerer_slots', 'sorcerer_known', 'bard_slots',
             'alchemist_slots', 'summoner_slots', 'inquisitor_slots']:
    a = constants.index(('int ' if name == 'spell_bonus' else 'const int ') + name + '[')
    code += constants[a:constants.index(';', a) + 1] + '\n'
for signature in [
    'int compute_slots_by_circle(struct char_data *ch, int class, int circle)',
    'int compute_sorcerer_known_limit(struct char_data *ch, int circle)',
    'bool is_sorc_bloodline_spell(int bloodline, int spellnum)',
    'int get_sorc_bloodline(struct char_data *ch)',
    'bool is_a_known_spell(struct char_data *ch, int class, int spellnum)',
    'int is_spell_in_prep_queue(struct char_data *ch, int class, int spellnum, int metamagic)',
    'bool is_spell_in_collection(struct char_data *ch, int class, int spellnum, int metamagic)',
    'int count_known_spells_by_circle(struct char_data *ch, int class, int circle)',
]:
    code += function(prep, signature)
code += function(parser, 'static bool has_class_spell_source(struct char_data *ch, int spellnum)')
# Exercise the production Aurak branch of canCastAtWill without unrelated race/warlock stubs.
a = parser.index('  if (isAurakMagic(ch, spellnum) && !has_class_spell_source')
b = parser.index('    return true;', a) + len('    return true;')
code += 'static bool aurak_at_will(struct char_data *ch, int spellnum) {\n' + parser[a:b] + '\nreturn false;\n}\n'
# Check that racial casts do not extract slots before dispatch.
a = parser.index('          if (!(treat_as_at_will && isAurakMagic(ch, spellnum)))')
b = parser.index('          if (treat_as_at_will)', a)
code += r'''
static int extracted;
int spell_prep_gen_extract(struct char_data *ch, int spell, int meta) { extracted++; return CLASS_SORCERER; }
int spell_prep_gen_extract_by_class(struct char_data *ch, int class, int spell, int meta) { extracted++; return class; }
static void extract_source(struct char_data *ch, int spellnum, bool treat_as_at_will) {
  int slot_class = CLASS_UNDEFINED, slot_spellnum = spellnum, slot_metamagic = 0, ch_class = CLASS_UNDEFINED;
''' + parser[a:b] + '(void)ch_class; }\n'
code += r'''
int main(void) {
  struct char_data ch = {0}; struct player_special_data ps = {0};
  struct known_spell_data spells[6] = {0};
  struct prep_collection_spell_data prepared = {0};
  ch.player_specials = &ps;
  ps.saved.class_level[CLASS_SORCERER] = 1; GET_LEVEL(&ch) = 1; GET_CHA(&ch) = 20;
  int first_slots = compute_slots_by_circle(&ch, CLASS_SORCERER, 1);
  int first_known = compute_sorcerer_known_limit(&ch, 1);
  assert(first_slots == sorcerer_slots[1][1] + spell_bonus[20][1]);
  assert(first_known == sorcerer_known[1][1] + spell_bonus[20][1]);
  ps.saved.class_level[CLASS_SORCERER] = 2; GET_LEVEL(&ch) = 2;
  assert(compute_slots_by_circle(&ch, CLASS_SORCERER, 1) == first_slots + 1);
  assert(compute_sorcerer_known_limit(&ch, 1) == first_known);
  assert(compute_slots_by_circle(&ch, CLASS_SORCERER, 2) == 0);
  assert(compute_sorcerer_known_limit(&ch, 2) == 0);
  assert(compute_sorcerer_known_limit(&ch, 0) == 0);
  /* Reproduce screenshot: four marked known spells include free Mage Armor. */
  SET_FEAT(&ch, FEAT_SORCERER_BLOODLINE_DRACONIC, 1);
  int ids[] = {SPELL_MAGIC_MISSILE, SPELL_MAGE_ARMOR, SPELL_NEGATIVE_ENERGY_RAY,
               SPELL_SUMMON_CREATURE_1, SPELL_AURAK_ENERGY_RAY, SPELL_AURAK_BREATH};
  for (int i = 0; i < 6; i++) {
    spells[i].spell = ids[i]; spells[i].next = i < 5 ? &spells[i+1] : NULL;
  }
  KNOWN_SPELLS(&ch, CLASS_SORCERER) = spells;
  assert(count_known_spells_by_circle(&ch, CLASS_SORCERER, 1) == 3);
  assert(!aurak_at_will(&ch, SPELL_MAGIC_MISSILE));
  /* Having a known class spell stays decisive even when all slots are recovering. */
  assert(!aurak_at_will(&ch, SPELL_MAGIC_MISSILE));
  assert(aurak_at_will(&ch, SPELL_DISGUISE_SELF));
  assert(aurak_at_will(&ch, SPELL_AURAK_ENERGY_RAY));
  assert(aurak_at_will(&ch, SPELL_AURAK_BREATH));
  extracted = 0;
  extract_source(&ch, SPELL_AURAK_BREATH, true); assert(extracted == 0);
  extract_source(&ch, SPELL_MAGIC_MISSILE, false); assert(extracted == 1);
  /* Prepared class magic and its recovery queue also take priority. */
  ps.saved.class_level[CLASS_WIZARD] = 1;
  prepared.spell = SPELL_DISGUISE_SELF;
  SPELL_COLLECTION(&ch, CLASS_WIZARD) = &prepared;
  assert(!aurak_at_will(&ch, SPELL_DISGUISE_SELF));
  SPELL_COLLECTION(&ch, CLASS_WIZARD) = NULL;
  SPELL_PREP_QUEUE(&ch, CLASS_WIZARD) = &prepared;
  assert(!aurak_at_will(&ch, SPELL_DISGUISE_SELF));
  SPELL_PREP_QUEUE(&ch, CLASS_WIZARD) = NULL;
  assert(aurak_at_will(&ch, SPELL_DISGUISE_SELF));
  return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='sorcerer-casting-') as tmp:
    path = Path(tmp) / 'check.c'; path.write_text(headers + code)
    binary = Path(tmp) / 'check'
    subprocess.run([os.environ.get('CC', 'cc'), '-std=gnu99', '-Wall', '-Wextra',
                    '-Wno-unused-parameter', '-I' + str(root / 'src'), str(path),
                    '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print('Sorcerer casting: level 1/2 slot progression, stable known limits, bonus bloodline count, stale racial selections, and class/racial precedence passed.')
