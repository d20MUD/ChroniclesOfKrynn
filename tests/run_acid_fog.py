#!/usr/bin/env python3
"""Prevent acid-fog ticks from re-entering area/room spell dispatch."""
from pathlib import Path
import os
import subprocess
import tempfile

root=Path(__file__).resolve().parents[1]
limits=(root/'src/limits.c').read_text()
magic=(root/'src/magic.c').read_text()
start=limits.index('void proc_d20_round(void)\n{')
# Execute the production character loop through its acid-fog step, before unrelated round effects.
round_code=limits[start:limits.index('    /* Cowering:',start)]+'  }\n}\n'
start=magic.index('  case SPELL_ACID_FOG:', magic.index('int mag_damage('))
damage_case=magic[start:magic.index('  case SPELL_CHAIN_LIGHTNING:',start)]
headers=''.join('#include "'+h+'"\n' for h in ['conf.h','sysdep.h','structs.h','utils.h',
    'spells.h','comm.h','handler.h'])
code=r'''
#include <assert.h>
struct player_special_data dummy_mob;
struct char_data *character_list;
static struct char_data people[6];
static struct affected_type fog[6];
static int active[6],hits[6],casts;
static int index_of(struct char_data *ch) {assert(ch>=people && ch<people+6);return ch-people;}
void basic_mud_log(const char *fmt,...) {assert(0);}
bool affected_by_spell(struct char_data *ch,int spell) {
  assert(spell==SPELL_ACID_FOG);int index=index_of(ch);return active[index]!=0;
}
void affect_from_char(struct char_data *ch,int spell) {
  assert(spell==SPELL_ACID_FOG);active[index_of(ch)]=0;
}
size_t send_to_char(struct char_data *ch,const char *fmt,...) {return 0;}
const char *act(const char *s,int hide,struct char_data *ch,struct obj_data *obj,void *vict,int type) {return s;}
int call_magic(struct char_data *ch,struct char_data *vict,struct obj_data *obj,int spell,int meta,int level,int casttype) {
  casts++;
  /* Model the old area dispatch: each recast infects bystanders and renews existing effects. */
  for(int n=0;n<6;n++) {active[n]=1;fog[n].duration++;}
  return 0;
}
int mag_damage(int level,struct char_data *ch,struct char_data *victim,struct obj_data *wpn,
               int spellnum,int metamagic,int savetype,int casttype) {
  assert(ch==victim && wpn==NULL && spellnum==SPELL_ACID_FOG);
  assert(casttype==CAST_SPELL && metamagic==0);
  int save=savetype,mag_resist=0,element=0,num_dice=0,size_dice=0;
  switch(spellnum) {
'''+damage_case+r'''
  default: assert(0);
  }
  assert(num_dice==2 && size_dice==6 && element==DAM_ACID);
  assert(save==SAVING_FORT && mag_resist);
  hits[index_of(victim)]++;
  return num_dice*size_dice;
}
'''
code+=round_code
code+=r'''
int main(void) {
  struct player_special_data ps[6]={0};
  for(int n=0;n<6;n++) {
    people[n].player_specials=&ps[n];people[n].next=n<5?&people[n+1]:NULL;
    if(n!=1)SET_BIT_AR(MOB_FLAGS(&people[n]),MOB_ISNPC);
    fog[n].duration=10;
  }
  character_list=people;
  /* One player and three NPCs carry the effect. Two bystanders do not. */
  active[0]=active[1]=active[2]=active[3]=1;
  for(int round=1;round<=10;round++) {
    proc_d20_round();
    assert(casts==0);
    for(int n=0;n<6;n++) {
      assert(hits[n]==(n<4?round:0));
      assert(active[n]==(n<4));
      assert(fog[n].duration==10); /* Damage must not reapply or extend the effect. */
    }
  }
  SET_BIT_AR(AFF_FLAGS(&people[2]),AFF_WIND_WALL);
  proc_d20_round();assert(!active[2] && hits[2]==10 && !casts);
  affect_from_char(&people[1],SPELL_ACID_FOG);
  proc_d20_round();assert(hits[1]==11 && hits[2]==10);
  assert(hits[0]==12 && hits[3]==12 && !hits[4] && !hits[5]);
  assert(!active[4] && !active[5] && !casts);
  return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='acid-fog-') as tmp:
    path=Path(tmp)/'check.c';path.write_text(headers+code);binary=Path(tmp)/'check'
    subprocess.run([os.environ.get('CC','cc'),'-std=gnu99','-Wall','-Wextra',
        '-Wno-unused-parameter','-Wno-unused-variable','-I'+str(root/'src'),str(path),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
print('Acid fog: repeated player/NPC ticks, no area recasts, unaffected bystanders, no duration renewal, 2d6 acid setup, wind-wall removal, and ended effects passed.')
