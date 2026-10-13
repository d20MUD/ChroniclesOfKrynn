#!/usr/bin/env python3
"""Happy-hour XP and yields across production refining, harvesting and butchering."""
from pathlib import Path
import os
import re
import subprocess
import tempfile

root=Path(__file__).resolve().parents[1]
craft=(root/'src/crafting_new.c').read_text()
def function(source,name):
    start=re.search(r'^(?:static )?(?:void|int|bool) '+name+r'\([^;]*?\)\n\{',source,re.M).start()
    return source[start:source.index('\n}',start)+2]+'\n'
code=''.join(function(craft,n) for n in ['canonical_craft_material','material_grade',
    'harvesting_skill_by_material','get_insightful_talent_bonus','gain_craft_exp',
    'happy_hour_quantity_bonus','apply_happy_hour_yield','craft_refine_complete',
    'harvest_complete','butcher_complete'])
code+=function((root/'src/limits.c').read_text(),'clear_happy_hour_bonuses')
headers='\n'.join('#include "'+h+'"' for h in ['conf.h','sysdep.h','structs.h','utils.h',
    'spells.h','comm.h','db.h','constants.h','crafting_new.h','talents.h'])
stubs=r'''
#include <assert.h>
#define REFINE_BASE_EXP 10
#define HARVEST_BASE_EXP 20
#define HARVEST_BASE_DC 5
#define HARVEST_BASE_AMOUNT dice(2,2)
#define HARVEST_MOTE_CHANCE 20
#define HARVEST_MOTE_DICE_SIZE 4
#define HARVEST_MOTE_MULTIPLIER 150
struct happyhour happy_data;
struct player_special_data dummy_mob;
struct room_data rooms[1];
struct room_data *world=rooms;
const char *ability_names[NUM_ABILITIES+1];
const char *crafting_materials[NUM_CRAFT_MATS+1];
const char *crafting_motes[NUM_CRAFT_MOTES+1];
static struct obj_data corpse;
static int roll=10, efficient, expertise, mote_rank, skill_value=30;
static char output[32768];
void basic_mud_log(const char *fmt,...) { assert(0); }
int MAX(int a,int b) { return a>b?a:b; }
int MIN(int a,int b) { return a<b?a:b; }
int d20(struct char_data *ch) { return roll; }
int dice(int count,int sides) { return count*sides; }
int rand_number(int low,int high) { return low; }
int get_craft_skill_value(struct char_data *ch,int skill) { return skill_value; }
int craft_skill_level_exp(struct char_data *ch,int level) { return 10000000; }
int get_proficient_talent_bonus(struct char_data *ch,int skill) { return 0; }
int get_efficient_talent_bonus(struct char_data *ch,int skill) { return efficient; }
int get_talent_rank(struct char_data *ch,int talent) {
  return talent==TALENT_BUTCHERING_EXPERTISE ? expertise : talent==TALENT_WATER_MOTE_SYNERGY ? mote_rank : 0;
}
void gain_talent_point(struct char_data *ch,int amount) { assert(0); }
const char *harvest_node_name(room_rnum room) { return "a node"; }
static struct obj_data *find_butcherable_corpse_in_room(struct char_data *ch) { return &corpse; }
static int get_corpse_level(struct obj_data *obj) { return 10; }
static int get_corpse_race_type(struct obj_data *obj) { return RACE_TYPE_ANIMAL; }
void reset_current_craft(struct char_data *ch,char *arg,bool verbose,bool reimburse) {
  GET_CRAFT(ch).refining_result[0]=GET_CRAFT(ch).refining_result[1]=0;
}
const char *act(const char *s,int hide,struct char_data *ch,struct obj_data *obj,void *vict,int type) { return s; }
size_t send_to_char(struct char_data *ch,const char *fmt,...) {
  va_list ap; va_start(ap,fmt); int n=vsnprintf(output+strlen(output),sizeof(output)-strlen(output),fmt,ap); va_end(ap); return n;
}
static void reset(struct char_data *ch) {
  memset(ch->player_specials,0,sizeof(*ch->player_specials)); memset(&corpse,0,sizeof(corpse));
  output[0]=0; efficient=expertise=mote_rank=0; roll=10; skill_value=30;
}
'''
checks=r'''
int main(void) {
  struct char_data ch={0}; struct player_special_data ps={0}; ch.player_specials=&ps;
  for(int i=0;i<=NUM_ABILITIES;i++) ability_names[i]="skill";
  for(int i=0;i<NUM_CRAFT_MATS;i++) crafting_materials[i]="material";
  for(int i=0;i<NUM_CRAFT_MOTES;i++) crafting_motes[i]="mote";
  HAPPY_CRAFTING_EXP=25; HAPPY_HARVESTING_EXP=50; HAPPY_HARVESTING_MATERIALS=25;
  HAPPY_HARVESTING_MOTES_OBTAINED=25;
  const int skills[]={ABILITY_HARVEST_MINING,ABILITY_CRAFT_ALCHEMY,ABILITY_CRAFT_TAILORING};
  for(int i=0;i<3;i++) {
    reset(&ch); GET_CRAFT((&ch)).refining_result[0]=CRAFT_MAT_STEEL;
    GET_CRAFT((&ch)).refining_result[1]=4; GET_CRAFT((&ch)).refining_batch_quantity=4;
    GET_CRAFT((&ch)).dc=20; GET_CRAFT((&ch)).skill_type=skills[i];
    craft_refine_complete(&ch);
    assert(GET_CRAFT_MAT((&ch),CRAFT_MAT_STEEL)==5);
    assert(GET_CRAFT_SKILL_EXP((&ch),skills[i])==(i==0?180:150));
  }
  reset(&ch); roll=20; GET_CRAFT((&ch)).refining_result[0]=CRAFT_MAT_STEEL;
  GET_CRAFT((&ch)).refining_result[1]=4; GET_CRAFT((&ch)).refining_batch_quantity=4;
  GET_CRAFT((&ch)).dc=20; GET_CRAFT((&ch)).skill_type=ABILITY_CRAFT_ALCHEMY;
  craft_refine_complete(&ch); assert(GET_CRAFT_MAT((&ch),CRAFT_MAT_STEEL)==6);
  assert(GET_CRAFT_SKILL_EXP((&ch),ABILITY_CRAFT_ALCHEMY)==187);
  const int materials[]={CRAFT_MAT_STONE,CRAFT_MAT_LOW_GRADE_HIDE,CRAFT_MAT_ASH_WOOD,CRAFT_MAT_HEMP};
  for(int i=0;i<4;i++) {
    reset(&ch); rooms[0].harvest_material=materials[i]; rooms[0].harvest_material_amount=10;
    harvest_complete(&ch);
    assert(GET_CRAFT_MAT((&ch),materials[i])==5 && rooms[0].harvest_material_amount==6);
    assert(GET_CRAFT_SKILL_EXP((&ch),harvesting_skill_by_material(materials[i]))==67);
  }
  reset(&ch); roll=20; rooms[0].harvest_material=CRAFT_MAT_IRONWOOD; rooms[0].harvest_material_amount=4;
  harvest_complete(&ch); assert(GET_CRAFT_MAT((&ch),CRAFT_MAT_IRONWOOD)==10);
  assert(rooms[0].harvest_material==CRAFT_MAT_NONE);
  /* Depletion must not discard the node's grade for the critical mote roll. */
  assert(GET_CRAFT_MOTES((&ch),NUM_CRAFT_MOTES-1)==37);
  reset(&ch); roll=20; mote_rank=2; rooms[0].harvest_material=CRAFT_MAT_IRONWOOD;
  rooms[0].harvest_material_amount=4; harvest_complete(&ch);
  assert(GET_CRAFT_MOTES((&ch),CRAFTING_MOTE_WATER)==40);
  reset(&ch); GET_CRAFT((&ch)).butcher_material=CRAFT_MAT_LOW_GRADE_HIDE;
  butcher_complete(&ch);
  assert(GET_CRAFT_MAT((&ch),CRAFT_MAT_LOW_GRADE_HIDE)==2);
  assert(GET_CRAFT_SKILL_EXP((&ch),ABILITY_HARVEST_BUTCHERING)==45);
  reset(&ch); roll=20; efficient=100; expertise=1;
  GET_CRAFT((&ch)).butcher_material=CRAFT_MAT_LOW_GRADE_HIDE; butcher_complete(&ch);
  assert(GET_CRAFT_MAT((&ch),CRAFT_MAT_LOW_GRADE_HIDE)==6);
  assert(GET_CRAFT_SKILL_EXP((&ch),ABILITY_HARVEST_BUTCHERING)==60);
  reset(&ch); roll=1; GET_CRAFT((&ch)).butcher_material=CRAFT_MAT_LOW_GRADE_HIDE; butcher_complete(&ch);
  assert(!GET_CRAFT_MAT((&ch),CRAFT_MAT_LOW_GRADE_HIDE) && !GET_CRAFT_SKILL_EXP((&ch),ABILITY_HARVEST_BUTCHERING));
  assert(happy_hour_quantity_bonus(1,25)==1 && happy_hour_quantity_bonus(4,25)==1);
  assert(!happy_hour_quantity_bonus(0,25) && !happy_hour_quantity_bonus(4,0));
  assert(!happy_hour_quantity_bonus(INT_MAX,1000));
  HAPPY_EXP=100; HAPPY_TREASURE=50; HAPPY_TIME=1; clear_happy_hour_bonuses();
  struct happyhour empty={0}; assert(!memcmp(&happy_data,&empty,sizeof(empty)));
  reset(&ch); rooms[0].harvest_material=CRAFT_MAT_HEMP; rooms[0].harvest_material_amount=10;
  harvest_complete(&ch); assert(GET_CRAFT_MAT((&ch),CRAFT_MAT_HEMP)==4);
  assert(GET_CRAFT_SKILL_EXP((&ch),ABILITY_HARVEST_GATHERING)==45);
  return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='happy-crafting-') as tmp:
    path=Path(tmp)/'check.c'; path.write_text(headers+stubs+code+checks)
    binary=Path(tmp)/'check'
    subprocess.run([os.environ.get('CC','cc'),'-std=gnu99','-Wall','-Wextra','-Wno-unused-parameter',
        '-I'+str(root/'src'),str(path),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
print('Happy crafting: single XP bonus, refining/harvesting/butchering yields, criticals/talents, mote grade on depletion, rounding and happy-hour cleanup passed.')
