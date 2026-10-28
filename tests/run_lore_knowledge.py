#!/usr/bin/env python3
"""Creature lore must use the same best knowledge skill as item lore."""
from pathlib import Path
import os
import subprocess
import tempfile

root=Path(__file__).resolve().parents[1]
source=(root/'src/act.other.c').read_text()
start=source.index('int can_lore_target(struct char_data *ch, struct char_data *target_ch, struct obj_data *target_obj,\n                    bool silent)\n{')
production=source[start:source.index('\n}',start)+2]
headers=''.join('#include "'+h+'"\n' for h in ['conf.h','sysdep.h','structs.h','utils.h',
    'spells.h','comm.h','spec_procs.h'])
code=r'''
#include <assert.h>
struct player_special_data dummy_mob;
const char *ability_names[NUM_ABILITIES+1];
static int arcana,religion,nature,spellcraft,knowledge,greater,monster,sage;
static char output[8192];
void basic_mud_log(const char *fmt,...) {assert(0);}
int compute_charisma_bonus(struct char_data *ch) {return 0;}
int compute_wisdom_bonus(struct char_data *ch) {return (ch->aff_abils.wis-10)/2;}
int compute_ability(struct char_data *ch,int skill) {
  if(skill==ABILITY_ARCANA)return arcana;
  if(skill==ABILITY_RELIGION)return religion;
  if(skill==ABILITY_NATURE)return nature;
  if(skill==ABILITY_SPELLCRAFT)return spellcraft;
  assert(0);return 0;
}
int get_feat_value(struct char_data *ch,int feat) {
  switch(feat) {
  case FEAT_KNOWLEDGE:return knowledge;
  case FEAT_GREATER_LORE:return greater;
  case FEAT_MONSTER_LORE:return monster;
  case FEAT_BG_SAGE:return sage;
  default:return 0;
  }
}
int d20(struct char_data *ch) {return 10;}
size_t send_to_char(struct char_data *ch,const char *fmt,...) {
  va_list ap;va_start(ap,fmt);int n=vsnprintf(output+strlen(output),sizeof(output)-strlen(output),fmt,ap);va_end(ap);return n;
}
/* Keep the old selector available so reintroducing the override fails at runtime. */
int get_knowledge_skill_from_creature_type(int race) {return ABILITY_ARCANA;}
'''+production+r'''
int main(void) {
  struct char_data ch={0},mob={0};struct player_special_data ps={0};struct obj_data obj={0};
  ch.player_specials=&ps;SET_BIT_AR(MOB_FLAGS(&mob),MOB_ISNPC);
  GET_LEVEL(&mob)=20;GET_OBJ_LEVEL(&obj)=20;ch.aff_abils.wis=10;
  ability_names[ABILITY_ARCANA]="Arcana";ability_names[ABILITY_RELIGION]="Religion";
  ability_names[ABILITY_NATURE]="Nature";
  const int skills[]={ABILITY_ARCANA,ABILITY_RELIGION,ABILITY_NATURE};
  const char *names[]={"Arcana","Religion","Nature"};
  for(int best=0;best<3;best++) {
    arcana=religion=nature=1;
    if(best==0)arcana=25;else if(best==1)religion=25;else nature=25;
    for(int race=0;race<NUM_RACES;race++) {
      GET_REAL_RACE(&mob)=race;output[0]=0;
      assert(can_lore_target(&ch,&mob,NULL,FALSE));
      assert(strstr(output,names[best]) && strstr(output,"25 ranks"));
    }
    output[0]=0;assert(can_lore_target(&ch,NULL,&obj,FALSE));assert(strstr(output,names[best]));
  }
  arcana=religion=nature=1;output[0]=0;
  assert(!can_lore_target(&ch,&mob,NULL,TRUE));
  assert(!strstr(output,"not extensive enough"));
  nature=20;knowledge=monster=1;ch.aff_abils.wis=18;output[0]=0;
  assert(can_lore_target(&ch,&mob,NULL,FALSE));
  assert(strstr(output,"Nature") && strstr(output,"32 ranks")); /* 20 + knowledge 8 + monster 4 */
  greater=1;spellcraft=40;output[0]=0;
  assert(can_lore_target(&ch,NULL,&obj,FALSE));assert(strstr(output,"Spellcraft with Greater Lore"));
  output[0]=0;assert(can_lore_target(&ch,&mob,NULL,FALSE));
  assert(strstr(output,"Nature") && !strstr(output,"Spellcraft"));
  /* Identified items remain free of further checks; players are invalid creature targets. */
  SET_BIT_AR(GET_OBJ_EXTRA(&obj),ITEM_IDENTIFIED);output[0]=0;
  assert(can_lore_target(&ch,NULL,&obj,FALSE) && !output[0]);
  REMOVE_BIT_AR(MOB_FLAGS(&mob),MOB_ISNPC);output[0]=0;
  assert(!can_lore_target(&ch,&mob,NULL,FALSE));assert(strstr(output,"only lore mobs"));
  (void)skills;
  return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='lore-knowledge-') as tmp:
    path=Path(tmp)/'check.c';path.write_text(headers+code);binary=Path(tmp)/'check'
    subprocess.run([os.environ.get('CC','cc'),'-std=gnu99','-Wall','-Wextra','-Wno-unused-parameter',
        '-I'+str(root/'src'),str(path),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
print('Lore: highest Arcana/Religion/Nature across creature races and items, bonuses, Greater Lore item checks, failures, and invalid targets passed.')
