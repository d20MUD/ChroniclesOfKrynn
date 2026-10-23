#!/usr/bin/env python3
"""Exercise new spell effects, cleansing and shared immunity/save helpers."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
magic = (root / 'src/magic.c').read_text()
utils = (root / 'src/utils.c').read_text()
spells = (root / 'src/spells.h').read_text()

def function(source, signature):
    start = source.index(signature)
    end = source.index('\n}', start) + 2
    return source[start:end]

ids = '\n'.join(line for line in spells.splitlines() if re.match(
    r'#define (SPELL_PROTECTION_FROM_POISON|SPELL_PANACEA|SPELL_POISON|SPELL_POISON_BREATHE|SPELL_POISON_BREATH|POISON_TYPE_\w+|WEAPON_POISON_BLACK_ADDER_VENOM)\s', line))
effects = magic[magic.index('  case SPELL_PROTECTION_FROM_POISON:'):magic.index('  case SPELL_DEATH_WARD: // necromancy')]
cure_start = magic.index('  if (spellnum == SPELL_PANACEA)', magic.index('void mag_unaffects('))
cure = magic[cure_start:magic.index('  switch (spellnum)', cure_start)]
save_start = magic.index('  if (ch && (casttype == CAST_WEAPON_POISON || is_poison_spell(spellnum)))')
save_bonus = magic[save_start:magic.index('  if (ch && IS_FRIGHTENED(ch))', save_start)]
helpers = '\n'.join(function(utils, sig) for sig in [
    'int get_poison_save_mod(', 'bool can_poison(', 'bool can_disease(', 'bool is_poison_spell('])

harness = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#define APPLY_SPECIAL 1
#define AFF_POISON 1
#define AFF_DISEASE 2
#define CAST_WEAPON_POISON 1
#define SAVING_POISON 3
#define SAVING_FORT 0
#define RACE_DWARF 1
#define RACE_DUERGAR 2
#define RACE_CRYSTAL_DWARF 3
#define HAS_FEAT(...) 0
#define HAS_REAL_FEAT(...) 0
#define HAS_EVOLUTION(...) 0
#define KNOWS_DISCOVERY(...) 0
#define GET_RACE(...) 0
#define IS_CONSTRUCT(...) 0
#define IS_UNDEAD(...) 0
#define PSIONIC_OAK_BODY 10001
#define PSIONIC_BODY_OF_IRON 10002
#define PSIONIC_SHADOW_BODY 10003
#define IS_SET_AR(bits, bit) ((bits) & (1 << (bit)))
#define REMOVE_BIT_AR(bits, bit) ((bits) &= ~(1 << (bit)))
#define AFF_FLAGS(ch) ((ch)->flags)
struct affected_type { int spell, location, modifier, duration, bitvector; struct affected_type *next; };
struct char_data { int flags; struct affected_type *affected; };
static bool affected_by_spell(struct char_data *ch, int spell) {
  for (struct affected_type *af = ch->affected; af; af = af->next)
    if (af->spell == spell) return true;
  return false;
}
static int get_evolution_appearance_save_bonus(struct char_data *ch) { return 0; }
static void affect_remove(struct char_data *ch, struct affected_type *af) {
  struct affected_type **link = &ch->affected;
  while (*link != af) link = &(*link)->next;
  *link = af->next; free(af);
}
''' + ids + '\n' + helpers + r'''
static void cast_effect(struct char_data *victim, int spellnum, int level) {
  struct affected_type af[1] = {{0}};
  const char *to_vict = 0, *to_room = 0;
  switch (spellnum) {
''' + effects + r'''
  }
  struct affected_type *record = malloc(sizeof(*record));
  *record = af[0]; record->spell = spellnum;
  record->next = victim->affected; victim->affected = record;
}
static void cleanse(struct char_data *victim) {
  int spellnum = SPELL_PANACEA; struct affected_type *af;
''' + cure + r'''
}
static int bonus(struct char_data *ch, struct char_data *vict, int type, int casttype, int spellnum) {
  int savethrow = 0;
''' + save_bonus + r'''
  return savethrow;
}
static void tick(struct char_data *ch) {
  struct affected_type *af = ch->affected, *next;
  for (; af; af = next) { next = af->next; if (--af->duration == 0) affect_remove(ch, af); }
}
int main(void) {
  struct char_data caster = {0}, target = {0};
  assert(can_poison(&target) && can_disease(&target));
  cast_effect(&target, SPELL_PROTECTION_FROM_POISON, 7);
  assert(target.affected->duration == 700);
  assert(bonus(&caster, &target, SAVING_POISON, 0, 0) == 5);
  assert(bonus(&caster, &target, SAVING_FORT, 0, SPELL_POISON) == 5);
  assert(bonus(&caster, &target, SAVING_FORT, 0, SPELL_POISON_BREATH) == 5);
  assert(bonus(&caster, &target, SAVING_FORT, CAST_WEAPON_POISON, SPELL_POISON) == 5);
  assert(bonus(&caster, &target, SAVING_FORT, CAST_WEAPON_POISON, 0) == 5);
  assert(bonus(&caster, &target, SAVING_FORT, 0, 0) == 0);
  assert(can_poison(&target) && can_disease(&target));
  cast_effect(&target, SPELL_POISON, 7);
  target.affected->bitvector = 1 << AFF_POISON;
  cast_effect(&target, 9000, 7);
  target.affected->bitvector = 1 << AFF_DISEASE;
  cast_effect(&target, SPELL_POISON_BREATHE, 7); /* poison without AFF_POISON */
  target.flags = (1 << AFF_POISON) | (1 << AFF_DISEASE);
  cast_effect(&target, SPELL_PANACEA, 3); /* engine applies buff before cleansing */
  cleanse(&target);
  assert(!affected_by_spell(&target, SPELL_POISON));
  assert(!affected_by_spell(&target, SPELL_POISON_BREATHE));
  assert(!affected_by_spell(&target, 9000));
  assert(target.flags == 0);
  assert(affected_by_spell(&target, SPELL_PROTECTION_FROM_POISON));
  assert(!can_poison(&target) && !can_disease(&target));
  tick(&target); tick(&target);
  assert(!can_poison(&target) && !can_disease(&target));
  tick(&target);
  assert(can_poison(&target) && can_disease(&target));
  cleanse(&target); /* healthy target: retain unrelated buffs */
  assert(affected_by_spell(&target, SPELL_PROTECTION_FROM_POISON));
  affect_remove(&target, target.affected);
  assert(get_poison_save_mod(&caster, &target) == 0);
  puts("Poison protection and panacea checks passed.");
}
'''
# Class assignments are character levels: divine circle 2 is level 3,
# inquisitor circle 2 is level 4, and divine circle 5 is level 9.
classes = (root / 'src/class.c').read_text()
for cls, spell, level in [
    ('CLERIC', 'PROTECTION_FROM_POISON', 3), ('DRUID', 'PROTECTION_FROM_POISON', 3),
    ('INQUISITOR', 'PROTECTION_FROM_POISON', 4), ('CLERIC', 'PANACEA', 9), ('DRUID', 'PANACEA', 9)]:
    assert f'spell_assignment(CLASS_{cls}, SPELL_{spell}, {level});' in classes
with tempfile.TemporaryDirectory() as tmp:
    c = Path(tmp) / 'protection.c'
    c.write_text(harness)
    exe = Path(tmp) / 'protection'
    subprocess.run(['cc', '-std=c99', '-fsanitize=address,undefined', str(c), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
