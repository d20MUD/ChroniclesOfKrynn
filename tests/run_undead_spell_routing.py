#!/usr/bin/env python3
"""Production undead conversion must precede the effective routine selection."""
from pathlib import Path
import os
import subprocess
import tempfile
root=Path(__file__).resolve().parents[1]
source=(root/'src/spell_parser.c').read_text()
magic=(root/'src/magic.c').read_text()
harm=magic.split('  case SPELL_HARM:\n',1)[1].split('    break;',1)[0]
assert 'element = DAM_UNHOLY;' in harm and 'element = DAM_HOLY;' not in harm
start=source.index('  if (cvict && IS_UNDEAD(cvict))', source.index('static int resolve_call_magic(',source.index('int call_magic(')))
end=source.index('  if ((casttype != CAST_WEAPON_POISON)',start)
production=source[start:end]
headers=''.join('#include "'+h+'"\n' for h in ['conf.h','sysdep.h','structs.h','utils.h','spells.h'])
code=r'''
#include <assert.h>
#undef IS_UNDEAD
#define IS_UNDEAD(ch) (undead)
static int undead,grouped;
int is_player_grouped(struct char_data *ch, struct char_data *victim) {return grouped;}
struct spell_info_type spell_info[TOP_SPELL_DEFINE+1];
static int route(struct char_data *caster, struct char_data *cvict, int spellnum, int *result) {
  int routines=-1;
'''+production+r'''
  *result=spellnum;return routines;
}
int main(void) {
  struct char_data caster={0},victim={0};struct player_special_data ps={0};
  int result,routines,i;
  const int healing[]={SPELL_CURE_LIGHT,SPELL_CURE_MODERATE,SPELL_CURE_SERIOUS,SPELL_CURE_CRITIC,SPELL_HEAL};
  const int damaging[]={SPELL_CAUSE_LIGHT_WOUNDS,SPELL_CAUSE_MODERATE_WOUNDS,SPELL_CAUSE_SERIOUS_WOUNDS,SPELL_CAUSE_CRITICAL_WOUNDS,SPELL_HARM};
  caster.player_specials=&ps;
  for(i=0;i<5;i++){spell_info[healing[i]].routines=MAG_POINTS;spell_info[damaging[i]].routines=MAG_DAMAGE;}
  undead=1;
  for(grouped=0;grouped<=1;grouped++) {
    for(i=0;i<5;i++) {
      routines=route(&caster,&victim,damaging[i],&result);
      assert(result==healing[i] && routines==MAG_POINTS);
      routines=route(&caster,&victim,healing[i],&result);
      assert(result==healing[i] && routines==MAG_POINTS);
    }
  }
  undead=0;
  routines=route(&caster,&victim,SPELL_HARM,&result);
  assert(result==SPELL_HARM && routines==MAG_DAMAGE);
  spell_info[PSIONIC_DEADLY_FEAR].routines=MAG_DAMAGE;
  GET_AUGMENT_PSP(&caster)=8;
  routines=route(&caster,&victim,PSIONIC_DEADLY_FEAR,&result);
  assert(routines==MAG_AREAS);
  return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='undead-routing-') as tmp:
    path=Path(tmp)/'check.c';path.write_text(headers+code);binary=Path(tmp)/'check'
    subprocess.run([os.environ.get('CC','cc'),'-std=gnu99','-Wall','-Wextra','-Wno-unused-parameter','-I'+str(root/'src'),str(path),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
print('Undead routing: converted Harm/cause spells select healing routines; living Harm and augmented Deadly Fear retain correct dispatch.')
