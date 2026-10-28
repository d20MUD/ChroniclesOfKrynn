#!/usr/bin/env python3
"""Clone needs no nonexistent component and still respects summon restrictions."""
from pathlib import Path
import os
import subprocess
import tempfile

root=Path(__file__).resolve().parents[1]
source=(root/'src/magic.c').read_text()
start=source.index('  case SPELL_CLONE:',source.index('void mag_summons('))
setup=source[start:source.index('  case VAMPIRE_ABILITY_CHILDREN_OF_THE_NIGHT:',start)]
start=source.index('  /* start off with some possible fail conditions */',start)
gates=source[start:source.index('  /* new limit cap on certain mobiles */',start)]
start=source.index('    /* All other summons use the general follower limit check */',start)
follower=source[start:source.index('    break;',start)]
headers=''.join('#include "'+h+'"\n' for h in ['conf.h','sysdep.h','structs.h','utils.h','comm.h','spells.h'])
code=r'''
#include <assert.h>
struct player_special_data dummy_mob;
static int rolled,material_checks,spawns,last_mob;
static bool allow_follower=TRUE,superior;
static const char *mag_summon_fail_msgs[10];
void basic_mud_log(const char *fmt,...) {assert(0);}
int rand_number(int low,int high) {return low==2?2:rolled;}
size_t send_to_char(struct char_data *ch,const char *fmt,...) {return 0;}
bool has_summoner_superior_summoner(struct char_data *ch) {return superior;}
bool can_add_follower(struct char_data *ch,int mob) {assert(mob==MOB_CLONE);return allow_follower;}
/* If the obsolete gate returns, the empty-inventory case must fail this test. */
static int mag_materials(struct char_data *ch,IDXTYPE a,IDXTYPE b,IDXTYPE c,int extract,int verbose) {
  material_checks++;return FALSE;
}
static void attempt_clone(struct char_data *ch) {
  int msg=0,fmsg=0,mob_num=0,pfail=0;
  switch(SPELL_CLONE) {
'''+setup+r'''
  default:assert(0);
  }
'''+gates+follower+r'''
  assert(msg==11 && mob_num==MOB_CLONE && pfail==0);
  spawns++;last_mob=mob_num;
}
int main(void) {
  struct char_data ch={0};struct player_special_data ps={0};struct obj_data inventory={0};
  ch.player_specials=&ps;
  for(rolled=0;rolled<=101;rolled++) {
    int previous=spawns;attempt_clone(&ch);
    assert(spawns==previous+1 && last_mob==MOB_CLONE && !material_checks && !ch.carrying);
  }
  ch.carrying=&inventory;int previous=spawns;attempt_clone(&ch);
  assert(spawns==previous+1 && ch.carrying==&inventory && !material_checks);
  allow_follower=FALSE;previous=spawns;attempt_clone(&ch);assert(spawns==previous);
  allow_follower=TRUE;SET_BIT_AR(AFF_FLAGS(&ch),AFF_CHARM);
  attempt_clone(&ch);assert(spawns==previous);
  REMOVE_BIT_AR(AFF_FLAGS(&ch),AFF_CHARM);superior=TRUE;allow_follower=FALSE;
  attempt_clone(&ch);assert(spawns==previous);
  (void)mag_materials;
  return 0;
}
'''
# The selected summon prototype must actually exist in the shipped world data.
assert '\n#10\n' in (root/'lib/world/mob/0.mob').read_text()
with tempfile.TemporaryDirectory(prefix='clone-summoning-') as tmp:
    path=Path(tmp)/'check.c';path.write_text(headers+code);binary=Path(tmp)/'check'
    subprocess.run([os.environ.get('CC','cc'),'-std=gnu99','-Wall','-Wextra','-Wno-unused-parameter',
        '-I'+str(root/'src'),str(path),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
print('Clone: no component gate, empty/occupied inventory, all failure-roll values, existing prototype, charm restriction, and follower limits passed.')
