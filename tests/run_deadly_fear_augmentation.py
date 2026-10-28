#!/usr/bin/env python3
"""Exercise production Deadly Fear dispatch and area target iteration."""
from pathlib import Path
import os
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
parser = (root / 'src/spell_parser.c').read_text()
magic = (root / 'src/magic.c').read_text()
start = parser.index('  routines = spell_info[spellnum].routines;', parser.index('static int resolve_call_magic(', parser.index('int call_magic(')))
setup = parser[start:parser.index('\n  level = apply_arcane_supremacy', start)]
start = parser.index('  if (IS_SET(routines, MAG_DAMAGE))', parser.index('/* the rest of the routine handling follows: */'))
dispatch = parser[start:parser.index('  if (IS_SET(routines, MAG_SUMMONS))', start)]
start = magic.index('  for (tch = world[IN_ROOM(ch)].people;', magic.index('void mag_areas('))
loop = magic[start:magic.index('\n}', start)]
# Exercise the real per-target loop with harmless unrelated spell IDs.
headers = ''.join('#include "' + name + '"\n' for name in ['conf.h', 'sysdep.h', 'structs.h', 'utils.h', 'spells.h'])
code = r'''
#include <assert.h>
struct player_special_data dummy_mob;
struct spell_info_type spell_info[TOP_SPELL_DEFINE + 1];
struct room_data *world;
static int hits[4], area_calls;
int mag_damage(int level, struct char_data *ch, struct char_data *victim, struct obj_data *obj,
               int spellnum, int metamagic, int savetype, int casttype) {
  assert(victim); hits[GET_LEVEL(victim)]++; return 0;
}
int aoeOK(struct char_data *ch, struct char_data *victim, int spellnum) {
  return victim != ch && GET_LEVEL(victim) != 3;
}
#define mag_affects(...) ((void)0)
#define mag_unaffects(...) ((void)0)
#define mag_loops(...) ((void)0)
#define mag_points(...) ((void)0)
#define mag_alter_objs(...) ((void)0)
#define mag_groups(...) ((void)0)
#define mag_masses(...) ((void)0)
#undef IS_UNDEAD
#define IS_UNDEAD(...) 0
#define is_player_grouped(...) 0
#define hit(...) ((void)0)
#define perform_knockdown(...) ((void)0)
#define spatial_visual_meteor_impact(...) ((void)0)
#define spatial_audio_test_sound_effect(...) ((void)0)
#define get_modified_elevation(...) 0
#define ZONE_FLAGGED(...) 0
#define X_LOC(...) 0
#define Y_LOC(...) 0
void mag_areas(int level, struct char_data *ch, struct obj_data *obj, int spellnum,
               int metamagic, int savetype, int casttype) {
  struct char_data *tch, *next_tch;
  int temp_class=0, temp_meta=metamagic, isEffect=0, is_eff_and_dam=0, is_uneffect=0;
  area_calls++;
''' + loop + r'''
}
static int manifest(struct char_data *caster, struct char_data *cvict, int spellnum) {
  struct obj_data *ovict=NULL;
  int routines, spell_level=20, metamagic=0, savetype=SAVING_WILL, casttype=CAST_SPELL;
''' + setup + dispatch + r'''
  return routines;
}
int main(void) {
  struct char_data caster={0}, first={0}, second={0}, protected={0};
  struct player_special_data ps={0}; struct room_data room={0};
  caster.player_specials=&ps; world=&room; IN_ROOM(&caster)=0;
  GET_LEVEL(&first)=1; GET_LEVEL(&second)=2; GET_LEVEL(&protected)=3;
  room.people=&caster; caster.next_in_room=&first; first.next_in_room=&second;
  second.next_in_room=&protected;
  spell_info[PSIONIC_DEADLY_FEAR].routines=MAG_DAMAGE;
  for(int augment=0; augment<=20; augment++) {
    memset(hits,0,sizeof(hits)); area_calls=0; GET_AUGMENT_PSP(&caster)=augment;
    int routines=manifest(&caster,&first,PSIONIC_DEADLY_FEAR);
    assert(hits[0]==0 && hits[1]==1 && hits[3]==0);
    assert(hits[2]==(augment>=8)); assert(area_calls==(augment>=8));
    assert(!!IS_SET(routines,MAG_AREAS)==(augment>=8));
    assert(spell_info[PSIONIC_DEADLY_FEAR].routines==MAG_DAMAGE);
  }
  return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='deadly-fear-') as tmp:
    source = Path(tmp) / 'check.c'
    source.write_text(headers + code)
    binary = Path(tmp) / 'check'
    subprocess.run([os.environ.get('CC', 'cc'), '-std=gnu99', '-w', '-I'+str(root/'src'), str(source), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print('Deadly Fear: 0–7 augment single target; 8–20 area, no duplicate hits, protected targets excluded, metadata preserved.')
