#!/usr/bin/env python3
"""Verify Thorn PCs ignore lunar slots and spell modifiers, including stale saves."""
from pathlib import Path
import os
import subprocess
import tempfile

root=Path(__file__).resolve().parents[1]
moon=(root/'src/moon_bonus_spells.c').read_text()
magic=(root/'src/magic.c').read_text()
headers='\n'.join('#include "'+name+'"' for name in
                  ['conf.h','sysdep.h','structs.h','utils.h','db.h','moon_bonus_spells.h'])
stubs=r'''
#include <assert.h>
struct config_data config_info;
struct weather_data weather_info;
int MAX(int a,int b) { return a>b?a:b; }
int compute_arcane_level(struct char_data *ch) {
  if(IS_NPC(ch)) return GET_LEVEL(ch);
  return CLASS_LEVEL(ch,CLASS_WIZARD)+CLASS_LEVEL(ch,CLASS_SORCERER);
}
'''
blocks=[]
pos=0
while True:
    a=magic.find('  if (is_moon_phase_affected(ch))',pos)
    if a<0: break
    b=magic.index('\n  }',a)+4
    block=magic[a:b]
    pos=b
    var='savethrow' if 'savethrow +=' in block else 'level'
    blocks.append(f'int lunar_{len(blocks)}(struct char_data *ch) {{ int {var}=10;\n'+block+f'\nreturn {var}; }}\n')
assert len(blocks)==8, 'Every lunar spell modifier must use the eligibility helper'
checks=r'''
static void stale_pool(struct char_data *ch) {
  ch->player_specials->saved.moon_bonus_spells=2;
  ch->player_specials->saved.moon_bonus_spells_used=1;
  ch->player_specials->saved.moon_bonus_regen_timer=1;
}
static void empty_pool(struct char_data *ch) {
  assert(!ch->player_specials->saved.moon_bonus_spells);
  assert(!ch->player_specials->saved.moon_bonus_spells_used);
  assert(!ch->player_specials->saved.moon_bonus_regen_timer);
}
int main(void) {
  struct char_data ch={0}; struct player_special_data special={0};
  ch.player_specials=&special;
  CONFIG_ARCANE_MOON_PHASES=1;
  assert(!is_moon_phase_affected(NULL));
  for(int alignment=-1000;alignment<=1000;alignment+=1000) {
    GET_ALIGNMENT(&ch)=alignment;
    for(int phase=-1;phase<=1;phase++) {
      weather_info.moons.solinari_sp=weather_info.moons.lunitari_sp=weather_info.moons.nuitari_sp=2;
      weather_info.moons.solinari_lv=weather_info.moons.lunitari_lv=weather_info.moons.nuitari_lv=phase;
      weather_info.moons.solinari_st=weather_info.moons.lunitari_st=weather_info.moons.nuitari_st=phase;
      CLASS_LEVEL((&ch),CLASS_WIZARD)=10;
      CLASS_LEVEL((&ch),CLASS_SORCERER)=5;
      CLASS_LEVEL((&ch),CLASS_KNIGHT_OF_THE_THORN)=0;
      assert(is_arcane_caster(&ch) && is_moon_phase_affected(&ch));
      init_moon_bonus_spells(&ch);
      assert(get_available_moon_bonus_spells(&ch)==2);
      assert(use_moon_bonus_spell(&ch));
      assert(get_available_moon_bonus_spells(&ch)==1);
      ch.player_specials->saved.moon_bonus_regen_timer=1;
      regenerate_moon_bonus_spell(&ch);
      assert(get_available_moon_bonus_spells(&ch)==2);
      LUNAR_CONTROL
      for(int thorn=1;thorn<=3;thorn++) {
        CLASS_LEVEL((&ch),CLASS_KNIGHT_OF_THE_THORN)=thorn;
        assert(is_arcane_caster(&ch) && !is_moon_phase_affected(&ch));
        stale_pool(&ch);
        assert(!get_max_moon_bonus_spells(&ch));
        assert(!get_available_moon_bonus_spells(&ch));
        assert(!get_used_moon_bonus_spells(&ch));
        assert(!has_moon_bonus_spells(&ch) && !use_moon_bonus_spell(&ch));
        LUNAR_THORN
        update_moon_bonus_spells(&ch); empty_pool(&ch);
        stale_pool(&ch); init_moon_bonus_spells(&ch); empty_pool(&ch);
        stale_pool(&ch); regenerate_moon_bonus_spell(&ch); empty_pool(&ch);
        stale_pool(&ch); reset_moon_bonus_spells(&ch); empty_pool(&ch);
      }
    }
  }
  CLASS_LEVEL((&ch),CLASS_KNIGHT_OF_THE_THORN)=0;
  CLASS_LEVEL((&ch),CLASS_WIZARD)=CLASS_LEVEL((&ch),CLASS_SORCERER)=0;
  assert(!is_moon_phase_affected(&ch));
  stale_pool(&ch); update_moon_bonus_spells(&ch); empty_pool(&ch);
  SET_BIT_AR(MOB_FLAGS(&ch),MOB_ISNPC); GET_LEVEL(&ch)=10;
  assert(is_moon_phase_affected(&ch));
  assert(!has_moon_bonus_spells(&ch));
  return 0;
}
'''
checks=checks.replace('LUNAR_CONTROL','\n'.join(f'assert(lunar_{i}(&ch)==10+phase);' for i in range(len(blocks))))
checks=checks.replace('LUNAR_THORN','\n'.join(f'assert(lunar_{i}(&ch)==10);' for i in range(len(blocks))))
with tempfile.TemporaryDirectory(prefix='thorn-moons-') as tmp:
    path=Path(tmp)/'check.c'; path.write_text(headers+stubs+moon+'\n'+''.join(blocks)+checks)
    binary=Path(tmp)/'check'
    subprocess.run([os.environ.get('CC','cc'),'-std=gnu99','-Wall','-Wextra',
                    '-I'+str(root/'src'),str(path),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
print('Thorn moon exemption: all alignments, positive/negative phases, eight spell modifiers, slot use, stale pools, and regeneration passed.')
