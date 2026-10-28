#!/usr/bin/env python3
"""Exercise Holy Weapon rank totals and the successful-melee-hit trigger."""
from pathlib import Path
import os
import subprocess
import tempfile

root=Path(__file__).resolve().parents[1]
fight=(root/'src/fight.c').read_text()
perks=(root/'src/perks.c').read_text()
def function(signature):
    start=perks.index(signature+'\n{')
    return perks[start:perks.index('\n}',start)+2]+'\n'
start=fight.index('  /* Battle Cleric Holy Weapon applies to every melee hit,')
trigger=fight[start:fight.index('  /* weapon spells - deprecated,',start)]
headers=''.join('#include "'+h+'"\n' for h in ['conf.h','sysdep.h','structs.h','utils.h',
    'spells.h','handler.h','perks.h','spec_abilities.h'])
code=r'''
#include <assert.h>
struct player_special_data dummy_mob;
static int rank1,rank2,calls,last_amount,last_type,last_spell;
static bool rank3,wrath,avatar,kills;
void basic_mud_log(const char *fmt,...) {assert(0);}
int get_total_perk_ranks(struct char_data *ch,int perk) {
  return perk==PERK_CLERIC_HOLY_WEAPON_1?rank1:perk==PERK_CLERIC_HOLY_WEAPON_2?rank2:0;
}
bool has_perk(struct char_data *ch,int perk) {
  return perk==PERK_CLERIC_HOLY_WEAPON_3?rank3:perk==PERK_CLERIC_DIVINE_WRATH?wrath:FALSE;
}
bool affected_by_spell(struct char_data *ch,int spell) {return spell==PERK_CLERIC_AVATAR_OF_WAR && avatar;}
int damage(struct char_data *ch,struct char_data *vict,int amount,int spell,int type,int hand) {
  assert(ch && vict);calls++;last_amount=amount;last_type=type;last_spell=spell;return kills?-1:amount;
}
'''
for signature in ['int get_cleric_holy_weapon_bonus(struct char_data *ch)',
                  'bool has_divine_wrath(struct char_data *ch)',
                  'int get_cleric_total_holy_weapon_bonus(struct char_data *ch)']:
    code+=function(signature)
code+='\nstatic bool landed_hit(struct char_data *ch,struct char_data *victim,int attack_type,bool victim_is_dead) {\n'+trigger+'return victim_is_dead;\n}\n'
code+=r'''
int main(void) {
  struct char_data ch={0},victim={0};struct player_special_data ps={0};struct obj_data weapon={0},gloves={0};
  ch.player_specials=&ps;
  const int melee[]={ATTACK_TYPE_PRIMARY,ATTACK_TYPE_OFFHAND,ATTACK_TYPE_TWOHAND,
    ATTACK_TYPE_UNARMED,ATTACK_TYPE_PRIMARY_SNEAK,ATTACK_TYPE_OFFHAND_SNEAK,
    ATTACK_TYPE_PRIMARY_EVO_BITE,ATTACK_TYPE_PRIMARY_EVO_CLAWS};
  for(rank1=0;rank1<=3;rank1++)for(rank2=0;rank2<=2;rank2++)
    for(int tier3=0;tier3<2;tier3++)for(int capstone=0;capstone<2;capstone++) {
      rank3=tier3;wrath=capstone;
      int expected=rank1*2+rank2*3+tier3*5+capstone*10;
      for(int equipment=0;equipment<3;equipment++) {
        GET_EQ((&ch),WEAR_WIELD_1)=equipment==1?&weapon:NULL;
        GET_EQ((&ch),WEAR_HANDS)=equipment==2?&gloves:NULL;
        for(unsigned int n=0;n<sizeof(melee)/sizeof(*melee);n++) {
          calls=0;assert(!landed_hit(&ch,&victim,melee[n],FALSE));
          assert(calls==(expected>0));
          if(expected)assert(last_amount==expected && last_type==DAM_HOLY && last_spell==TYPE_SPECAB_HOLY);
        }
      }
    }
  rank1=3;rank2=2;rank3=wrath=avatar=TRUE;
  assert(get_cleric_total_holy_weapon_bonus(&ch)==37);
  calls=0;landed_hit(&ch,&victim,ATTACK_TYPE_UNARMED,FALSE);assert(calls==1 && last_amount==37);
  calls=0;
  landed_hit(&ch,&victim,ATTACK_TYPE_RANGED,FALSE);
  landed_hit(&ch,&victim,ATTACK_TYPE_PSIONICS,FALSE);
  landed_hit(&ch,&victim,ATTACK_TYPE_ELDRITCH_BLAST,FALSE);
  landed_hit(&ch,&victim,ATTACK_TYPE_UNARMED,TRUE);
  landed_hit(&ch,NULL,ATTACK_TYPE_UNARMED,FALSE);
  landed_hit(NULL,&victim,ATTACK_TYPE_UNARMED,FALSE);
  SET_BIT_AR(MOB_FLAGS(&ch),MOB_ISNPC);
  landed_hit(&ch,&victim,ATTACK_TYPE_UNARMED,FALSE);assert(!calls);
  REMOVE_BIT_AR(MOB_FLAGS(&ch),MOB_ISNPC);kills=TRUE;
  assert(landed_hit(&ch,&victim,ATTACK_TYPE_UNARMED,FALSE));assert(calls==1);
  return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='holy-weapon-unarmed-') as tmp:
    path=Path(tmp)/'check.c';path.write_text(headers+code);binary=Path(tmp)/'check'
    subprocess.run([os.environ.get('CC','cc'),'-std=gnu99','-Wall','-Wextra','-Wno-unused-parameter',
        '-I'+str(root/'src'),str(path),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
print('Holy Weapon: all ranks, fists/weapons/gloves, offhand/natural attacks, holy damage type, Divine Wrath/Avatar, excluded non-melee attacks, and lethal proc handling passed.')
