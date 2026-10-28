#!/usr/bin/env python3
"""Verify displayed armor failure matches the actual percent-roll boundaries."""
from pathlib import Path
import os
import subprocess
import tempfile

root=Path(__file__).resolve().parents[1]
armor=(root/'src/assign_wpn_armor.c').read_text()
parser=(root/'src/spell_parser.c').read_text()
start=armor.index('int compute_gear_spell_failure(struct char_data *ch)\n{')
compute=armor[start:armor.index('\n}',start)+2]
start=parser.index('  /* armor arcane failure check,')
branch=parser[start:parser.index('  // increase epic spell skill',start)]
headers=''.join('#include "'+h+'"\n' for h in ['conf.h','sysdep.h','structs.h','utils.h',
    'spells.h','handler.h','comm.h','assign_wpn_armor.h','perks.h'])
code=r'''
#include <assert.h>
struct player_special_data dummy_mob;
struct armor_table armor_list[NUM_SPEC_ARMOR_TYPES];
static int roll=1,armored,training,mastery,ignore,battle,oak,iron,test_armor_type,shield_type;
static bool insight,atwill;
void basic_mud_log(const char *fmt,...) {assert(0);}
int get_feat_value(struct char_data *ch,int feat) {
  switch(feat) {
  case FEAT_ARMORED_SPELLCASTING:return armored;
  case FEAT_ARCANE_ARMOR_TRAINING:return training;
  case FEAT_ARCANE_ARMOR_MASTERY:return mastery;
  case FEAT_IGNORE_SPELL_FAILURE:return ignore;
  case FEAT_BATTLE_CASTER:return battle;
  default:return 0;
  }
}
bool affected_by_spell(struct char_data *ch,int spell) {
  return spell==PSIONIC_OAK_BODY?oak:spell==PSIONIC_BODY_OF_IRON?iron:FALSE;
}
bool has_summoner_archmages_insight(struct char_data *ch) {return insight;}
sbyte canCastAtWill(struct char_data *ch,int spell) {return atwill;}
int compute_gear_armor_type(struct char_data *ch) {return test_armor_type;}
int compute_gear_shield_type(struct char_data *ch) {return shield_type;}
int rand_number(int low,int high) {assert(low==1 && high==100);return roll;}
size_t send_to_char(struct char_data *ch,const char *fmt,...) {return 0;}
const char *act(const char *s,int hide,struct char_data *ch,struct obj_data *obj,void *vict,int type) {return s;}
'''+compute+'\nstatic int cast(struct char_data *caster,int spellnum,int casttype) {\n'+branch+'return 1;\n}\n'
code+=r'''
static int failures(struct char_data *ch,int spell,int casttype) {
  int failed=0;
  for(roll=1;roll<=100;roll++)failed+=!cast(ch,spell,casttype);
  return failed;
}
int main(void) {
  struct char_data ch={0};struct player_special_data ps={0};struct obj_data gear={0};
  ch.player_specials=&ps;GET_OBJ_VAL(&gear,1)=1;
  const int classes[]={CLASS_WIZARD,CLASS_SORCERER,CLASS_BARD,CLASS_SUMMONER,CLASS_WARLOCK};
  for(int c=0;c<5;c++) {
    CASTING_CLASS(&ch)=classes[c];armored=1;
    assert(compute_gear_spell_failure(&ch)==0);
    assert(failures(&ch,SPELL_MAGIC_MISSILE,CAST_SPELL)==0);
    /* Worn accessories, even in body slots, are not armor. */
    for(int slot=0;slot<NUM_WEARS;slot++)GET_EQ((&ch),slot)=&gear;
    GET_OBJ_TYPE(&gear)=ITEM_WORN;armor_list[1].spellFail=100;
    assert(compute_gear_spell_failure(&ch)==0);
    assert(failures(&ch,SPELL_MAGIC_MISSILE,CAST_SPELL)==0);
    for(int slot=0;slot<NUM_WEARS;slot++)GET_EQ((&ch),slot)=NULL;
  }
  CASTING_CLASS(&ch)=CLASS_WIZARD;GET_EQ((&ch),WEAR_BODY)=&gear;
  GET_OBJ_TYPE(&gear)=ITEM_ARMOR;
  for(int percent=0;percent<=100;percent++) {
    armor_list[1].spellFail=percent;
    int expected=percent>5?percent-5:0;
    assert(compute_gear_spell_failure(&ch)==expected);
    assert(failures(&ch,SPELL_MAGIC_MISSILE,CAST_SPELL)==expected);
  }
  armor_list[1].spellFail=100;insight=TRUE;
  assert(compute_gear_spell_failure(&ch)==0 && failures(&ch,SPELL_MAGIC_MISSILE,CAST_SPELL)==0);
  insight=FALSE;atwill=TRUE;
  assert(!failures(&ch,SPELL_MAGIC_MISSILE,CAST_SPELL));atwill=FALSE;
  assert(!failures(&ch,SPELL_MAGIC_MISSILE,CAST_DEVICE));
  assert(!failures(&ch,SPELL_MAGIC_MISSILE,CAST_INNATE));
  assert(!failures(&ch,SPELL_GREATER_RUIN,CAST_SPELL));
  GET_EQ((&ch),WEAR_BODY)=NULL;oak=TRUE;
  assert(compute_gear_spell_failure(&ch)==20);
  assert(failures(&ch,SPELL_MAGIC_MISSILE,CAST_SPELL)==20);
  oak=FALSE;iron=TRUE;
  assert(compute_gear_spell_failure(&ch)==30);
  iron=FALSE;
  assert(compute_gear_spell_failure(&ch)==0);
  return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='arcane-spell-failure-') as tmp:
    path=Path(tmp)/'check.c';path.write_text(headers+code);binary=Path(tmp)/'check'
    subprocess.run([os.environ.get('CC','cc'),'-std=gnu99','-Wall','-Wextra','-Wno-unused-parameter',
        '-I'+str(root/'src'),str(path),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
print('Arcane failure: zero armor/accessory-only with Armored Spellcasting, all percent-roll boundaries, exemptions, and body transformation penalties passed.')
