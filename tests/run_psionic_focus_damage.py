#!/usr/bin/env python3
"""Exercise production psionic damage bonuses and prevent the old 0.10 multiplier."""
from pathlib import Path
import os
import subprocess
import tempfile

root=Path(__file__).resolve().parents[1]
magic=(root/'src/magic.c').read_text()
start=magic.index('  if (spellnum > 0 && spellnum < NUM_SPELLS)',magic.index('int mag_damage('))
feats=magic[start:magic.index('  // vampire bonuses',start)]
start=magic.index('  if (spellnum >= PSIONIC_POWER_START && spellnum <= PSIONIC_POWER_END)',
                  magic.index('  // resistances to magic',magic.index('int mag_damage(')))
focus=magic[start:magic.index('  // dwarven racial bonus',start)]
headers=''.join('#include "'+h+'"\n' for h in ['conf.h','sysdep.h','structs.h','utils.h',
    'spells.h','handler.h','psionics.h','perks.h'])
code=r'''
#include <assert.h>
struct player_special_data dummy_mob;
struct psionic_power_data psionic_powers[MAX_SPELLS+1];
static bool focused;
static int enhanced,epic_damage,epic_psionics;
void basic_mud_log(const char *fmt,...) {assert(0);}
bool affected_by_spell(struct char_data *ch,int spell) {
  assert(spell==PSIONIC_ABILITY_PSIONIC_FOCUS);return focused;
}
int get_feat_value(struct char_data *ch,int feat) {
  if(feat==FEAT_ENHANCED_POWER_DAMAGE)return enhanced;
  if(feat==FEAT_EPIC_POWER_DAMAGE)return epic_damage;
  if(feat==FEAT_EPIC_PSIONICS)return epic_psionics;
  return 0;
}
bool has_mind_spike_ii_bonus(struct char_data *ch,int augment) {return FALSE;}
int dice(int count,int sides) {return count*sides;}
static int power_damage(struct char_data *ch,int spellnum,int dam,int num_dice,int element) {
  int size_dice=6,bonus=0;
'''+feats+focus+r'''
  (void)bonus;return dam;
}
int main(void) {
  struct char_data ch={0};struct player_special_data ps={0};ch.player_specials=&ps;
  const int bases[]={0,1,9,10,11,99,100,123,10000};
  for(int n=0;n<9;n++) {
    int base=bases[n];focused=FALSE;
    assert(power_damage(&ch,PSIONIC_ASSIMILATE,base,18,DAM_CHAOS)==base);
    focused=TRUE;
    assert(power_damage(&ch,PSIONIC_ASSIMILATE,base,18,DAM_CHAOS)==base+base/10);
    assert(power_damage(&ch,SPELL_MAGIC_MISSILE,base,5,DAM_FORCE)==base);
    assert(power_damage(&ch,WARLOCK_ELDRITCH_BLAST,base,5,DAM_FORCE)==base);
  }
  for(int rank=0;rank<=5;rank++) for(int focus=0;focus<2;focus++) {
    epic_psionics=rank;focused=focus;
    int before_focus=100*(100+rank*(focus?20:10))/100;
    assert(power_damage(&ch,PSIONIC_ASSIMILATE,100,18,DAM_CHAOS)==
           (focus?before_focus+before_focus/10:before_focus));
  }
  /* Flat per-die bonuses apply before both percentage multipliers. */
  enhanced=2;epic_damage=1;epic_psionics=1;focused=TRUE;
  assert(power_damage(&ch,PSIONIC_ASSIMILATE,100,18,DAM_CHAOS)==250); /* (100+36+54)*1.2*1.1 */
  enhanced=epic_damage=epic_psionics=0;
  const int elements[]={DAM_FIRE,DAM_ACID,DAM_COLD,DAM_SOUND,DAM_ELECTRIC,DAM_CHAOS};
  for(int n=0;n<6;n++)
    assert(power_damage(&ch,PSIONIC_ASSIMILATE,100,18,elements[n])==110);
  return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='psionic-focus-damage-') as tmp:
    path=Path(tmp)/'check.c';path.write_text(headers+code);binary=Path(tmp)/'check'
    subprocess.run([os.environ.get('CC','cc'),'-std=gnu99','-Wall','-Wextra','-Wno-unused-parameter',
        '-I'+str(root/'src'),str(path),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
print('Psionic focus damage: +10%, rounding/zero damage, Epic Psionics stacking, per-die bonuses, and unaffected spells/invocations passed.')
