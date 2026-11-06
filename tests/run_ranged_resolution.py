#!/usr/bin/env python3
"""Check production shot context lifetime, nested attacks and typed damage routing."""
from pathlib import Path
import os
import subprocess
import tempfile
root=Path(__file__).resolve().parents[1]
fight=(root/'src/fight.c').read_text()
def function(name):
    start=fight.index(name)
    return fight[start:fight.index('\n}',start)+2]
a=fight.index('struct ranged_shot_context\n')
b=fight.index('static bool manyshot_ready(',a)
context=fight[a:b]
a=fight.index('      if (!victim_is_dead && attack_type == ATTACK_TYPE_RANGED &&\n          ammo_damage_type')
b=fight.index('      if (!victim_is_dead && current_shot',a)
bonus=fight[a:b]
code=r'''
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "spells.h"
#include <assert.h>
#define HIT_MISS 0
#define MIN(a,b) ((a)<(b)?(a):(b))
static struct obj_data *last_missile;
static struct obj_data source, projectile[2];
static int mode, depth, resolved, removed, captured_type, captured_damage;
static bool ready;
static bool manyshot_ready(struct char_data *ch,struct obj_data *obj) {return false;}
static struct obj_data *get_wielded(struct char_data *ch,int type) {return NULL;}
static bool can_fire_ammo(struct char_data *ch,bool silent) { return ready; }
static void extract_obj(struct obj_data *obj);
static int resolve_weapon_hit(struct char_data *,struct char_data *,int,int,int,int);
''' + context + function('int hit(struct char_data *ch,') + r'''
static void extract_obj(struct obj_data *obj) {
  removed++;
  forget_ranged_projectile(obj);
}
static int resolve_weapon_hit(struct char_data *ch,struct char_data *victim,
                              int type,int dam_type,int penalty,int attack_type) {
  int level=depth++;
  resolved++;
  struct obj_data *shot=&projectile[level];
  current_shot->projectile=shot;last_missile=shot;
  current_shot->unlimited=(mode==1);
  ready=false;
  assert(ranged_ammunition(ch)==shot); /* Bonuses survive the final shot. */
  if(mode==2) { forget_ranged_projectile(shot); }
  else if(mode==3 && level==0) {
    assert(hit(ch,victim,type,dam_type,penalty,attack_type)==9);
    assert(current_shot->projectile==shot && last_missile==shot);
  }
  depth--;
  return 9;
}
int dice(int n,int size) { assert(n==1); return size; }
static int damage(struct char_data *ch,struct char_data *victim,int amount,
                  int w_type,int dam_type,int attack_type) {
  captured_type=dam_type;captured_damage=amount;return amount==8 ? -1 : amount;
}
static bool elemental(struct char_data *ch,struct char_data *victim,int attack_type,
                      int ammo_damage_type,int ammo_damage_die,bool victim_is_dead) {
  int w_type=1;
''' + bonus + r'''
  return victim_is_dead;
}
static struct char_data ch,victim;
int main(void) {
  GET_EQ(&ch,WEAR_AMMO_POUCH)=&source;
  assert(hit(NULL,&victim,0,0,0,ATTACK_TYPE_RANGED)==HIT_MISS);
  IN_ROOM(&ch)=1;IN_ROOM(&victim)=2;
  assert(hit(&ch,&victim,0,0,0,ATTACK_TYPE_RANGED)==HIT_MISS && !resolved);
  IN_ROOM(&victim)=1;
  for(mode=0;mode<4;mode++) {
    memset(projectile,0,sizeof(projectile));
    for(int i=0;i<2;i++) IN_ROOM(&projectile[i])=mode==1 ? 1 : NOWHERE;
    removed=resolved=0;
    assert(hit(&ch,&victim,0,0,0,ATTACK_TYPE_RANGED)==9);
    assert(!current_shot && !last_missile);
    assert(removed==(mode==2?0:mode==3?2:1));
    assert(resolved==(mode==3?2:1));
  }
  for(int type=1;type<NUM_DAM_TYPES;type++) {
    captured_damage=0;
    assert(!elemental(&ch,&victim,ATTACK_TYPE_RANGED,type,6,false));
    assert(captured_type==type && captured_damage==6);
  }
  captured_damage=0;
  elemental(&ch,&victim,ATTACK_TYPE_PRIMARY,1,6,false);
  elemental(&ch,&victim,ATTACK_TYPE_RANGED,0,6,false);
  elemental(&ch,&victim,ATTACK_TYPE_RANGED,1,0,false);
  elemental(&ch,&victim,ATTACK_TYPE_RANGED,1,6,true);
  assert(captured_damage==0);
  assert(elemental(&ch,&victim,ATTACK_TYPE_RANGED,2,8,false));
  puts("PASS: same-room gate, last-shot context, nested shots, unlimited and orphan cleanup, elemental routing and lethal bonus damage.");
}
'''
with tempfile.TemporaryDirectory(prefix='ranged-resolution-') as directory:
    source=Path(directory)/'resolution.c';binary=Path(directory)/'resolution'
    source.write_text(code)
    subprocess.run([os.environ.get('CC','cc'),'-std=c11','-g','-fsanitize=address,undefined',
                    '-fno-omit-frame-pointer','-I'+str(root/'src'),str(source),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
