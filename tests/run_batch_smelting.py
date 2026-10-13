#!/usr/bin/env python3
"""Exercise batch preparation, stations, material accounting, and completion."""
from pathlib import Path
import os
import re
import subprocess
import tempfile

root=Path(__file__).resolve().parents[1]
source=(root/'src/crafting_new.c').read_text()
recipes=(root/'src/crafting_recipes.c').read_text()
def function(text,name):
    start=re.search(r'^(?:static )?(?:void|int|bool) '+name+r'\(',text,re.M).start()
    brace=text.index('\n{',start)
    return text[start:text.index('\n}',brace)+2]+'\n'
start=source.index('#define MAX_REFINING_BATCHES')
end=source.index('void show_refine_noargs',start)
code=source[start:end]+''.join(function(source,n) for n in
    ['newcraft_refine','is_refine_ready','happy_hour_quantity_bonus','apply_happy_hour_yield','craft_refine_complete','canonical_craft_material','material_grade','craft_group_by_material'])
code+=''.join(function(recipes,n) for n in ['refine_recipe','initialize_refining_recipes','populate_refining_recipes'])
constants=(root/'src/constants.c').read_text()
a=constants.index('const char *crafting_materials[] ='); b=constants.index('CHECK_TABLE_SIZE',a)
code=constants[a:b]+code
headers='\n'.join('#include "'+h+'"' for h in
    ['conf.h','sysdep.h','structs.h','utils.h','db.h','comm.h','spells.h','interpreter.h','crafting_new.h','crafting_recipes.h','constants.h'])
stubs=r'''
#include <assert.h>
#define REFINE_BASE_EXP 10
struct refine_recipe_data refining_recipes[NUM_REFINING_RECIPES];
struct room_data *world;
room_rnum top_of_world;
struct happyhour happy_data;
struct player_special_data dummy_mob;
void basic_mud_log(const char *fmt,...) { assert(0); }
const char *ability_names[NUM_ABILITIES+1];
const char *extra_bits[NUM_ITEM_FLAGS];
static int die=10, skill_value=30, xp, xp_skill, checks;
static char output[32768];
int MIN(int a,int b) { return a<b?a:b; }
int MAX(int a,int b) { return a>b?a:b; }
int is_abbrev(const char *a,const char *b) { return *a && !strncasecmp(a,b,strlen(a)); }
const char *one_argument(const char *argument,char *first,size_t n) {
  while (isspace((unsigned char)*argument)) argument++;
  size_t used=0;
  while (*argument && !isspace((unsigned char)*argument)) {
    if (used+1<n) first[used++]=tolower((unsigned char)*argument);
    argument++;
  }
  first[used]=0; return argument; /* Preserve production's leading-space return. */
}
size_t send_to_char(struct char_data *ch,const char *fmt,...) {
  va_list ap; va_start(ap,fmt); int n=vsnprintf(output+strlen(output),sizeof(output)-strlen(output),fmt,ap); va_end(ap); return n;
}
const char *act(const char *s,int hide,struct char_data *ch,struct obj_data *obj,void *vict,int type) { return s; }
void show_refine_noargs(struct char_data *ch) {}
void text_line(struct char_data *ch,const char *s,int length,char first,char second) {}
void draw_line(struct char_data *ch,int length,char first,char second) {}
int d20(struct char_data *ch) { checks++; return die; }
int get_craft_skill_value(struct char_data *ch,int skill) { return skill_value; }
int get_proficient_talent_bonus(struct char_data *ch,int skill) { return 0; }
void gain_craft_exp(struct char_data *ch,int exp,int abil,bool verbose) { xp+=exp; xp_skill=abil; }
void reset_current_craft(struct char_data *ch,char *arg,bool verbose,bool reimburse) {
  for (int i=0;i<3;i++) {
    if (reimburse && GET_CRAFT(ch).refining_materials[i][0])
      GET_CRAFT_MAT(ch,GET_CRAFT(ch).refining_materials[i][0])+=GET_CRAFT(ch).refining_materials[i][1];
    GET_CRAFT(ch).refining_materials[i][0]=GET_CRAFT(ch).refining_materials[i][1]=0;
  }
  GET_CRAFT(ch).refining_result[0]=GET_CRAFT(ch).refining_result[1]=0;
  GET_CRAFT(ch).refining_batch_quantity=1; GET_CRAFT(ch).craft_duration=0;
}
'''
checks=r'''
int main(void) {
  struct char_data ch={0}; struct player_special_data ps={0};
  struct room_data rooms[1]={0}; struct obj_data smelter={0};
  ch.player_specials=&ps; IN_ROOM(&ch)=0; world=rooms;
  extra_bits[ITEM_CRAFTING_SMELTER]="Crafting-Smelter";
  extra_bits[ITEM_CRAFTING_LOOM]="Crafting-Loom";
  ability_names[ABILITY_HARVEST_MINING]="mining"; populate_refining_recipes();
  SET_BIT_AR(GET_OBJ_EXTRA(&smelter),ITEM_CRAFTING_SMELTER); rooms[0].contents=&smelter;
  GET_CRAFT_MAT((&ch),CRAFT_MAT_IRON)=100; GET_CRAFT_MAT((&ch),CRAFT_MAT_COAL)=100;
  newcraft_refine(&ch,"smelt steel 10");
  assert(GET_CRAFT((&ch)).craft_duration==100 && GET_CRAFT((&ch)).refining_batch_quantity==10);
  assert(GET_CRAFT_MAT((&ch),CRAFT_MAT_IRON)==90 && GET_CRAFT_MAT((&ch),CRAFT_MAT_COAL)==90);
  output[0]=0; newcraft_refine(&ch,"show"); assert(strstr(output,"10/10 iron units"));
  GET_CRAFT((&ch)).craft_duration=0; craft_refine_complete(&ch);
  assert(GET_CRAFT_MAT((&ch),CRAFT_MAT_STEEL)==10 && xp==300 && checks==1 && xp_skill==ABILITY_HARVEST_MINING);
  newcraft_refine(&ch,"smelt steel all"); assert(GET_CRAFT((&ch)).refining_batch_quantity==20);
  newcraft_refine(&ch,"remove"); assert(GET_CRAFT_MAT((&ch),CRAFT_MAT_IRON)==90);
  GET_CRAFT_MAT((&ch),CRAFT_MAT_COAL)=3; newcraft_refine(&ch,"smelt steel all");
  assert(GET_CRAFT((&ch)).refining_batch_quantity==3 && GET_CRAFT_MAT((&ch),CRAFT_MAT_COAL)==0);
  newcraft_refine(&ch,"remove");
  const char *bad[]={"smelt steel 0","smelt steel -1","smelt steel 21","smelt steel 1x","smelt steel 999999999999999999999999","smelt satin 1","smelt steel 4","smelt unknown 2"};
  for(int i=0;i<sizeof(bad)/sizeof(bad[0]);i++) {
    newcraft_refine(&ch,bad[i]); assert(!GET_CRAFT((&ch)).refining_result[0]);
    assert(GET_CRAFT_MAT((&ch),CRAFT_MAT_COAL)==3 && GET_CRAFT_MAT((&ch),CRAFT_MAT_IRON)==90);
  }
  rooms[0].contents=NULL; newcraft_refine(&ch,"smelt steel 2");
  assert(!GET_CRAFT((&ch)).refining_result[0] && GET_CRAFT_MAT((&ch),CRAFT_MAT_COAL)==3);
  rooms[0].contents=&smelter;
  newcraft_refine(&ch,"add steel 2"); assert(!GET_CRAFT((&ch)).craft_duration);
  GET_CRAFT((&ch)).refining_materials[0][1]=1; assert(is_refine_ready(&ch,FALSE));
  GET_CRAFT((&ch)).refining_materials[0][1]=2;
  rooms[0].contents=NULL; newcraft_refine(&ch,"begin"); assert(!GET_CRAFT((&ch)).craft_duration);
  rooms[0].contents=&smelter; newcraft_refine(&ch,"begin"); assert(GET_CRAFT((&ch)).craft_duration==20);
  newcraft_refine(&ch,"smelt steel 1"); assert(GET_CRAFT_MAT((&ch),CRAFT_MAT_COAL)==1);
  newcraft_refine(&ch,"remove");
  GET_CRAFT_MAT((&ch),CRAFT_MAT_ZINC)=10; GET_CRAFT_MAT((&ch),CRAFT_MAT_SILVER)=10;
  newcraft_refine(&ch,"smelt cold iron 2"); assert(GET_CRAFT((&ch)).refining_result[0]==CRAFT_MAT_COLD_IRON);
  newcraft_refine(&ch,"remove");
  newcraft_refine(&ch,"smelt \"cold iron\" 2"); assert(GET_CRAFT((&ch)).refining_batch_quantity==2);
  newcraft_refine(&ch,"remove");
  GET_CRAFT_MAT((&ch),CRAFT_MAT_COPPER)=6;
  newcraft_refine(&ch,"smelt brass 3"); assert(GET_CRAFT((&ch)).refining_result[1]==6 && GET_CRAFT((&ch)).craft_duration==60);
  GET_CRAFT((&ch)).craft_duration=0; die=20; craft_refine_complete(&ch);
  assert(GET_CRAFT_MAT((&ch),CRAFT_MAT_BRASS)==7);
  newcraft_refine(&ch,"smelt steel 2"); GET_CRAFT((&ch)).craft_duration=0; die=1; craft_refine_complete(&ch);
  assert(!GET_CRAFT((&ch)).refining_result[0] && GET_CRAFT_MAT((&ch),CRAFT_MAT_COAL)==1);
  GET_CRAFT_MAT((&ch),CRAFT_MAT_COAL)=10;
  newcraft_refine(&ch,"smelt steel 2"); GET_CRAFT((&ch)).craft_duration=0; die=2; skill_value=1;
  craft_refine_complete(&ch); assert(GET_CRAFT((&ch)).refining_result[1]==2); newcraft_refine(&ch,"remove");
  GET_CRAFT_MAT((&ch),CRAFT_MAT_DRAGONBLOOD)=9; GET_CRAFT_MAT((&ch),CRAFT_MAT_IRON)=4; GET_CRAFT_MAT((&ch),CRAFT_MAT_STEEL)=10;
  newcraft_refine(&ch,"smelt dragonmetal all");
  assert(GET_CRAFT((&ch)).refining_batch_quantity==5 && GET_CRAFT((&ch)).refining_materials[1][0]==CRAFT_MAT_STEEL);
  newcraft_refine(&ch,"remove"); assert(GET_CRAFT_MAT((&ch),CRAFT_MAT_STEEL)==10);
  /* Non-smelting batches keep using their own station and proportional recipe output. */
  REMOVE_BIT_AR(GET_OBJ_EXTRA(&smelter),ITEM_CRAFTING_SMELTER);
  SET_BIT_AR(GET_OBJ_EXTRA(&smelter),ITEM_CRAFTING_LOOM);
  GET_CRAFT_MAT((&ch),CRAFT_MAT_FLAX)=9;
  newcraft_refine(&ch,"add linen all");
  assert(GET_CRAFT((&ch)).refining_batch_quantity==4 && GET_CRAFT_MAT((&ch),CRAFT_MAT_FLAX)==1);
  newcraft_refine(&ch,"begin"); assert(GET_CRAFT((&ch)).craft_duration==80);
  newcraft_refine(&ch,"remove"); assert(GET_CRAFT_MAT((&ch),CRAFT_MAT_FLAX)==9);
  /* One-command batches cover every real recipe, not only mining/smelting. */
  die=10; skill_value=30;
  for (int recipe=1; recipe<NUM_REFINING_RECIPES; recipe++) {
    memset(&smelter,0,sizeof(smelter));
    SET_BIT_AR(GET_OBJ_EXTRA(&smelter),refining_recipes[recipe].crafting_station_flag);
    for (int material=1;material<NUM_CRAFT_MATS;material++) GET_CRAFT_MAT((&ch),material)=50;
    char command[128]; snprintf(command,sizeof(command),"batch %s 2",crafting_materials[refining_recipes[recipe].result[0]]);
    newcraft_refine(&ch,command);
    int units=2*refining_recipes[recipe].result[1];
    assert(GET_CRAFT((&ch)).refining_batch_quantity==2 && GET_CRAFT((&ch)).craft_duration==10*units);
    for (int slot=0;slot<3;slot++) {
      int material=GET_CRAFT((&ch)).refining_materials[slot][0];
      if (material) assert(GET_CRAFT_MAT((&ch),material)==50-2*refining_recipes[recipe].materials[slot][1]);
    }
    GET_CRAFT((&ch)).craft_duration=0; xp=0; craft_refine_complete(&ch);
    assert(GET_CRAFT_MAT((&ch),refining_recipes[recipe].result[0])==50+units);
    assert(xp==(10+refining_recipes[recipe].dc)*units && xp_skill==refining_recipes[recipe].skill);
  }
  memset(&smelter,0,sizeof(smelter)); SET_BIT_AR(GET_OBJ_EXTRA(&smelter),ITEM_CRAFTING_ALCHEMY_LAB);
  GET_CRAFT_MAT((&ch),CRAFT_MAT_CATALYST_FRAGMENT)=37;
  newcraft_refine(&ch,"batch catalyst all"); assert(GET_CRAFT((&ch)).refining_batch_quantity==3 && GET_CRAFT((&ch)).craft_duration==30);
  newcraft_refine(&ch,"remove"); assert(GET_CRAFT_MAT((&ch),CRAFT_MAT_CATALYST_FRAGMENT)==37);
  return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='batch-smelting-') as tmp:
    path=Path(tmp)/'check.c'; path.write_text(headers+stubs+code+checks)
    binary=Path(tmp)/'check'
    subprocess.run([os.environ.get('CC','cc'),'-std=gnu99','-Wall','-Wextra','-Wno-unused-parameter','-Wno-sign-compare',
                    '-I'+str(root/'src'),str(path),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
print('Batch refining: quantities/all, multi-word names, station/readiness checks, reservations/refunds, yields/XP, failures, dragonmetal, every recipe/station, and one-command catalyst batches passed.')
