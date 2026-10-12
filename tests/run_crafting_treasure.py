#!/usr/bin/env python3
"""Verify rare resource drops, every material grade boundary, and treasure gating."""
from pathlib import Path
import os
import subprocess
import tempfile

root=Path(__file__).resolve().parents[1]
treasure=(root/'src/treasure.c').read_text()
craft=(root/'src/crafting_new.c').read_text()
def function(source, signature):
    a=source.index(signature+'\n{'); return source[a:source.index('\n}',a)+2]
a=treasure.index('#define TREASURE_MATERIAL_CHANCE')
b=treasure.index('/* this function determines whether',a)
code=treasure[a:b]+function(craft,'int material_grade(int material)')+function(treasure,'void determine_treasure(struct char_data *ch, struct char_data *mob)')
headers='\n'.join('#include "'+name+'"' for name in
                  ['conf.h','sysdep.h','structs.h','utils.h','comm.h','db.h','constants.h','treasure.h','crafting_new.h','account.h'])
stubs=r'''
#include <assert.h>
#include <limits.h>
struct happyhour happy_data;
const char *crafting_materials[NUM_CRAFT_MATS];
const char *crafting_motes[NUM_CRAFT_MOTES];
static int numbers[8], used, length, saves, messages, treasure_roll=100, magic_items;
#undef IS_STAFF_EVENT
#define IS_STAFF_EVENT 0
int MIN(int a,int b) { return a<b?a:b; }
int MAX(int a,int b) { return a>b?a:b; }
int rand_number(int low,int high) {
  assert(used<length); int result=numbers[used++]; assert(result>=low && result<=high); return result;
}
static void sequence(int a,int b,int c,int d,int e,int count) {
  numbers[0]=a; numbers[1]=b; numbers[2]=c; numbers[3]=d; numbers[4]=e; used=0; length=count;
}
void save_char(struct char_data *ch,int load_room) { saves++; }
const char *act(const char *text,int hide,struct char_data *ch,struct obj_data *obj,void *vict,int type) {
  messages++; return text;
}
int dice(int count,int size) { return size==100 ? treasure_roll : size==3 ? 3 : 10; }
void check_event_drops(struct char_data *ch,struct char_data *mob) {}
int quick_grade_check(int level) { return GRADE_MUNDANE; }
void award_magic_item(int count,struct char_data *ch,int grade,int level) { magic_items+=count; }
int get_account_reward_bonus(struct char_data *ch,int bonus) { return 0; }
int apply_account_reward_bonus(struct char_data *ch,int amount,int bonus) { return amount; }
'''
checks=r'''
int main(void) {
  struct char_data ch={0}, mob={0}; struct player_special_data special={0};
  ch.player_specials=&special; SET_BIT_AR(MOB_FLAGS(&mob),MOB_ISNPC);
  for(int i=0;i<NUM_CRAFT_MATS;i++) crafting_materials[i]="material";
  for(int i=0;i<NUM_CRAFT_MOTES;i++) crafting_motes[i]="mote";
  /* Tier probabilities over all 100 roll outcomes, including exact level boundaries. */
  int levels[]={1,4,5,9,10,19,20,29,30,40};
  int expected[][7]={{0,100,0,0,0,0,0},{0,100,0,0,0,0,0},
                    {0,18,82,0,0,0,0},{0,18,82,0,0,0,0},
                    {0,18,20,62,0,0,0},{0,18,20,62,0,0,0},
                    {0,18,20,30,32,0,0},{0,18,20,30,32,0,0},
                    {0,18,20,30,20,10,2},{0,18,20,30,20,10,2}};
  for(int level=0;level<10;level++) {
    int counts[7]={0};
    for(int roll=1;roll<=100;roll++) {
      sequence(roll,0,0,0,0,2);
      int material=random_treasure_material(levels[level]);
      int grade=material_grade(material);
      assert(material>0 && material<NUM_CRAFT_MATS && grade>0 && grade<=6);
      counts[grade]++;
      assert(levels[level]>=30 || grade<5);
      /* Every eligible material in a selected grade can actually be returned. */
      int index=0;
      for(int candidate=1;candidate<NUM_CRAFT_MATS;candidate++) if(material_grade(candidate)==grade) {
        sequence(roll,index++,0,0,0,2);
        assert(random_treasure_material(levels[level])==candidate);
      }
    }
    for(int grade=1;grade<=6;grade++) assert(counts[grade]==expected[level][grade]);
  }
  sequence(0,0,0,0,0,0); assert(random_treasure_material(0)==CRAFT_MAT_NONE);
  GET_LEVEL(&mob)=30;
  sequence(21,11,0,0,0,2); award_random_crafting_treasure(&ch,&mob);
  assert(!saves && !messages && used==2);
  sequence(20,2,0,10,1,5); award_random_crafting_treasure(&ch,&mob);
  assert(GET_CRAFT_MAT((&ch),CRAFT_MAT_DRAGONMETAL)==1 && GET_CRAFT_MOTES((&ch),1)==1);
  assert(saves==1 && messages==2 && used==5);
  GET_LEVEL(&mob)=29;
  sequence(1,1,0,11,0,4); award_random_crafting_treasure(&ch,&mob);
  assert(GET_CRAFT_MAT((&ch),CRAFT_MAT_DRAGONMETAL)==1);
  assert(GET_CRAFT_MAT((&ch),CRAFT_MAT_STEEL)==1 && saves==2);
  sequence(21,10,8,0,0,3); award_random_crafting_treasure(&ch,&mob);
  assert(GET_CRAFT_MOTES((&ch),8)==1 && saves==3);
  GET_LEVEL(&mob)=30;
  GET_CRAFT_MAT((&ch),CRAFT_MAT_DRAGONMETAL)=INT_MAX; GET_CRAFT_MOTES((&ch),1)=INT_MAX;
  sequence(1,1,0,1,1,5); award_random_crafting_treasure(&ch,&mob);
  assert(GET_CRAFT_MAT((&ch),CRAFT_MAT_DRAGONMETAL)==INT_MAX && GET_CRAFT_MOTES((&ch),1)==INT_MAX && saves==3);
  sequence(0,0,0,0,0,0);
  award_random_crafting_treasure(NULL,&mob); award_random_crafting_treasure(&ch,NULL);
  SET_BIT_AR(MOB_FLAGS(&ch),MOB_ISNPC); award_random_crafting_treasure(&ch,&mob);
  REMOVE_BIT_AR(MOB_FLAGS(&ch),MOB_ISNPC); assert(!used);
  /* No resources can drop without first passing the ordinary treasure roll. */
  memset(&special,0,sizeof(special)); saves=messages=0;
  determine_treasure(&ch,&mob); assert(!used && !saves);
  treasure_roll=5; sequence(20,2,0,10,8,5);
  determine_treasure(&ch,&mob);
  assert(GET_CRAFT_MAT((&ch),CRAFT_MAT_DRAGONMETAL)==1 && GET_CRAFT_MOTES((&ch),8)==1 && saves==1);
  assert(GET_GOLD(&ch)>0 && magic_items==0 && used==5);
  return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='crafting-treasure-') as tmp:
    path=Path(tmp)/'check.c'; path.write_text(headers+stubs+code+checks)
    binary=Path(tmp)/'check'
    subprocess.run([os.environ.get('CC','cc'),'-std=gnu99','-Wall','-Wextra','-Wno-unused-parameter',
                    '-I'+str(root/'src'),str(path),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
print('Rare crafting treasure: all material outcomes, level gates, chance boundaries, mote types, persistence, overflow, and treasure gating passed.')
