#!/usr/bin/env python3
"""Poison ticks preserve stealth; active poison attacks still reveal their caster."""
from pathlib import Path
import os
import subprocess
import tempfile

root=Path(__file__).resolve().parents[1]
source=(root/'src/fight.c').read_text()
start=source.index('  /* Poison suffering is not an attack')
reveal=source[start:source.index('  if (GET_POS(victim) == POS_DEAD)',start)]
start=source.index('  if (victim != ch)',source.index('static int resolve_damage(struct char_data *ch,'))
attribution=source[start:source.index('  /* pets leave if attacked',start)]
headers=''.join('#include "'+h+'"\n' for h in ['conf.h','sysdep.h','structs.h','utils.h',
    'spells.h','fight.h','handler.h'])
code=r'''
#include <assert.h>
struct player_special_data dummy_mob;
struct char_data *character_list;
static int appearances;
void basic_mud_log(const char *fmt,...) {assert(0);}
void appear(struct char_data *ch,bool forced) {
  assert(!forced);appearances++;
  REMOVE_BIT_AR(AFF_FLAGS(ch),AFF_HIDE);
  REMOVE_BIT_AR(AFF_FLAGS(ch),AFF_SNEAK);
  REMOVE_BIT_AR(AFF_FLAGS(ch),AFF_INVISIBLE);
}
bool is_wall_spell(int spell) {return FALSE;}
bool set_fighting(struct char_data *ch,struct char_data *victim) {FIGHTING(ch)=victim;return TRUE;}
static void take_hit(struct char_data *ch,struct char_data *victim,int w_type) {
  bool original_self_damage=(ch==victim);
'''+attribution+r'''
  GET_HIT(victim)--;
'''+reveal+r'''
}
static void hidden(struct char_data *ch) {
  SET_BIT_AR(AFF_FLAGS(ch),AFF_HIDE);
  SET_BIT_AR(AFF_FLAGS(ch),AFF_SNEAK);
  SET_BIT_AR(AFF_FLAGS(ch),AFF_INVISIBLE);
}
static void assert_hidden(struct char_data *ch) {
  assert(AFF_FLAGGED(ch,AFF_HIDE) && AFF_FLAGGED(ch,AFF_SNEAK) && AFF_FLAGGED(ch,AFF_INVISIBLE));
}
int main(void) {
  struct char_data poisoned={0},enemy={0};
  GET_HIT(&poisoned)=100;GET_POS(&poisoned)=GET_POS(&enemy)=POS_STANDING;
  IN_ROOM(&poisoned)=IN_ROOM(&enemy)=1;
  character_list=&poisoned;poisoned.next=&enemy;
  hidden(&poisoned);hidden(&enemy);
  for(int tick=0;tick<10;tick++)take_hit(&poisoned,&poisoned,SPELL_POISON);
  assert_hidden(&poisoned);assert(GET_HIT(&poisoned)==90 && !appearances);
  /* Giving a previous attacker kill credit must not turn a tick into an attack. */
  poisoned.last_attacker=&enemy;
  take_hit(&poisoned,&poisoned,SPELL_POISON);
  assert_hidden(&poisoned);assert_hidden(&enemy);assert(!appearances && GET_HIT(&poisoned)==89);
  /* Stale or out-of-room credit also preserves the victim's stealth. */
  IN_ROOM(&enemy)=2;take_hit(&poisoned,&poisoned,SPELL_POISON);
  assert_hidden(&poisoned);assert(!poisoned.last_attacker && !appearances);
  IN_ROOM(&enemy)=1;
  /* An active poison cast still reveals its caster, not its target. */
  take_hit(&enemy,&poisoned,SPELL_POISON);
  assert(appearances==1 && !AFF_FLAGGED(&enemy,AFF_HIDE) && !AFF_FLAGGED(&enemy,AFF_INVISIBLE));
  assert_hidden(&poisoned);
  hidden(&enemy);take_hit(&enemy,&poisoned,TYPE_HIT);
  assert(appearances==2 && !AFF_FLAGGED(&enemy,AFF_HIDE));
  /* Other self-inflicted damage keeps its existing behavior. */
  poisoned.last_attacker=NULL;
  take_hit(&poisoned,&poisoned,SPELL_FIREBALL);
  assert(appearances==3 && !AFF_FLAGGED(&poisoned,AFF_HIDE));
  return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='poison-stealth-') as tmp:
    path=Path(tmp)/'check.c';path.write_text(headers+code);binary=Path(tmp)/'check'
    subprocess.run([os.environ.get('CC','cc'),'-std=gnu99','-Wall','-Wextra','-Wno-unused-parameter',
        '-I'+str(root/'src'),str(path),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
print('Poison stealth: repeated self ticks preserve hide/sneak/invisibility, damage remains, kill-credit reassignment is safe, and active attacks still reveal the attacker.')
