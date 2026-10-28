#!/usr/bin/env python3
"""Exercise production save DC branches and power-circle conversion."""
from pathlib import Path
import os
import subprocess
import tempfile

root=Path(__file__).resolve().parents[1]
magic=(root/'src/magic.c').read_text()
def function(source,signature):
    start=source.index(signature+'\n{')
    return source[start:source.index('\n}',start)+2]+'\n'
start=magic.index('  if (!spellnum && ch && casttype == CAST_SPELL')
resolve=magic[start:magic.index('  /* Irresistible Magic',start)]
start=magic.index('  /* compute DC of savingthrow')
switch=magic[start:magic.index('  if (ch && casttype == CAST_WEAPON_SPELL',start)]
start=magic.index('  if (is_spellnum_psionic(spellnum))', magic.index('  if (ch && casttype == CAST_WEAPON_SPELL',start))
psionic=magic[start:magic.index('  if (ch && IS_UNDEAD(ch)',start)]
start=magic.index('    challenge += GET_DC_BONUS(ch);')
transient=magic[start:magic.index('\n  }',start)]
headers=''.join('#include "'+h+'"\n' for h in ['conf.h','sysdep.h','structs.h','utils.h',
    'spells.h','psionics.h','spell_prep.h','perks.h'])
code=r'''
#include <assert.h>
struct player_special_data dummy_mob;
struct spell_info_type spell_info[MAX_SPELLS+1];
struct psionic_power_data psionic_powers[MAX_SPELLS+1];
static int arcane,divine,gear_bonus,epic,focused,telepathy;
void basic_mud_log(const char *fmt,...) {assert(0);}
int MAX(int a,int b) {return a>b?a:b;}
int compute_arcane_level(struct char_data *ch) {return arcane;}
int compute_divine_level(struct char_data *ch) {return divine;}
int compute_wisdom_bonus(struct char_data *ch) {return (ch->aff_abils.wis-10)/2;}
int compute_charisma_bonus(struct char_data *ch) {return (ch->aff_abils.cha-10)/2;}
int get_spell_dc_bonus(struct char_data *ch) {return gear_bonus;}
int get_feat_value(struct char_data *ch,int feat) {return feat==FEAT_EPIC_PSIONICS?epic:0;}
struct affected_type *affected_by_spell(struct char_data *ch,int spell) {return focused?(struct affected_type *)ch:NULL;}
int get_psionic_telepathy_dc_bonus(struct char_data *ch) {return telepathy;}
bool isAurakMagic(struct char_data *ch,int spell) {return FALSE;}
int get_alchemist_bomb_dc_bonus(struct char_data *ch) {return 0;}
int get_artificer_combat_calibration_rank(struct char_data *ch) {return 0;}
int get_artificer_infusion_theory_bonus(struct char_data *ch) {return 0;}
bool is_domain_spell_of_ch(struct char_data *ch,int spell) {return FALSE;}
int get_cleric_domain_focus_bonus(struct char_data *ch) {return 0;}
'''
code+=function((root/'src/utils.c').read_text(),'bool is_spellnum_psionic(int spellnum)')
code+=function((root/'src/spell_prep.c').read_text(),'int compute_powers_circle(int class, int spellnum, int metamagic)')
code+='\nstatic int dc(struct char_data *ch,int casttype,int level,int spellnum) {\nstruct char_data victim={0};struct char_data *vict=&victim;\nint challenge=10,stat_bonus=0,savethrow=0;\n'
code+=resolve+switch+psionic+'\nif(ch) {\n'+transient+'\n}\n(void)savethrow;return challenge;\n}\n'
code+=r'''
int main(void) {
  struct char_data ch={0};struct player_special_data ps={0};ch.player_specials=&ps;
  ch.aff_abils.intel=28;ch.aff_abils.wis=40;ch.aff_abils.cha=50;
  CASTING_SPELLNUM((&ch))=PSIONIC_ASSIMILATE;
  spell_info[PSIONIC_ASSIMILATE].min_level[CLASS_PSIONICIST]=17;
  GET_DC_BONUS((&ch))=2;
  assert(dc(&ch,CAST_SPELL,17,0)==30); /* 10 + ninth circle + INT 9 + feat bonus 2 */
  assert(GET_DC_BONUS((&ch))==0);
  assert(dc(&ch,CAST_SPELL,17,PSIONIC_ASSIMILATE)==28);
  /* Every power circle uses its own level, regardless of arcane/divine multiclass levels. */
  for(int circle=1;circle<=9;circle++) {
    spell_info[PSIONIC_ASSIMILATE].min_level[CLASS_PSIONICIST]=2*circle-1;
    for(int multiclass=0;multiclass<2;multiclass++) {
      arcane=multiclass*20;divine=multiclass*10;
      assert(dc(&ch,CAST_SPELL,30,0)==10+circle+9);
    }
  }
  spell_info[PSIONIC_ASSIMILATE].min_level[CLASS_PSIONICIST]=17;
  CASTING_SPELLNUM((&ch))=0;gear_bonus=3;epic=2;focused=1;
  psionic_powers[PSIONIC_ASSIMILATE].power_type=TELEPATHY;telepathy=2;
  assert(dc(&ch,CAST_SPELL,17,PSIONIC_ASSIMILATE)==37); /* 28 + gear 3 + epic 4 + telepathy 2 */
  /* Ordinary spells and fixed item/innate DCs retain their existing calculation. */
  epic=focused=telepathy=gear_bonus=0;arcane=20;divine=0;
  CASTING_SPELLNUM((&ch))=PSIONIC_ASSIMILATE;
  assert(dc(&ch,CAST_SPELL,20,SPELL_MAGIC_MISSILE)==40); /* 10 + caster contribution 10 + CHA 20 */
  assert(dc(&ch,CAST_WAND,5,0)==15);
  assert(dc(&ch,CAST_INNATE,12,0)==22);
  assert(dc(NULL,CAST_INNATE,12,0)==22);
  spell_info[SPELL_MAGIC_MISSILE].effective_level=1;
  assert(dc(&ch,CAST_SPELL,4,SPELL_MAGIC_MISSILE)==34);
  return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='psionic-save-dc-') as tmp:
    path=Path(tmp)/'check.c';path.write_text(headers+code);binary=Path(tmp)/'check'
    subprocess.run([os.environ.get('CC','cc'),'-std=gnu99','-Wall','-Wextra','-Wno-unused-parameter',
        '-I'+str(root/'src'),str(path),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
print('Psionic save DC: Assimilate DC 30, circles 1–9, INT, multiclass independence, implicit/explicit power IDs, feat/gear bonuses, and unchanged spell/item DCs passed.')
