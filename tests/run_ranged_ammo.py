#!/usr/bin/env python3
"""Exercise production ammunition matching, quantities, loading and projectile properties."""
from pathlib import Path
import os
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'src/assign_wpn_armor.c').read_text()
def function(name):
    start = source.index(name)
    return source[start:source.index('\n}', start) + 2]
load = (root / 'src/db.c').read_text()
a = load.index('  if (GET_OBJ_TYPE(obj) == ITEM_MISSILE && GET_OBJ_VAL(obj, AMMO_VAL_MAX) > 0)')
b = load.index('  /* going to put some caps', a)
code = '''#define _POSIX_C_SOURCE 200809L
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "assign_wpn_armor.h"
#include "spec_abilities.h"
#include <assert.h>
#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))
#define act(...) ((void)0)
#define send_to_char(...) ((void)0)
struct weapon_table weapon_list[NUM_WEAPON_TYPES];
static bool main_returning;
bool obj_has_special_ability(struct obj_data *obj,int ability) { return obj && main_returning && ability==WEAPON_SPECAB_RETURNING; }
static int roll;
int rand_number(int low, int high) { assert(low<=high); return roll ? high : low; }
struct obj_data *create_obj(void) { return calloc(1,sizeof(struct obj_data)); }
static void obj_from_obj(struct obj_data *o) { assert(o->in_obj); o->in_obj->contains=o->next_content; o->in_obj=NULL; o->next_content=NULL; }
''' + function('bool is_throwing_weapon(') + '\n' + function('bool ammunition_is_unlimited(') + '\n' + function('bool has_ammo_in_pouch(') + '\n' + function('struct obj_data *take_ammo_projectile(')
code += '\nstatic void initialize(struct obj_data *obj) {\n' + load[a:b] + '\n}\n'
code += r'''
static struct char_data ch;
static struct obj_data pouch, ammo, weapon;
static void dispose(struct obj_data *shot) {
  free(shot->name);free(shot->short_description);free(shot->description);free(shot);
}
int main(void) {
  GET_EQ(&ch,WEAR_AMMO_POUCH)=&pouch;
  pouch.contains=&ammo;ammo.in_obj=&pouch;
  ammo.name="arrows";ammo.short_description="a bundle of arrows";ammo.description="Arrows.";
  GET_OBJ_TYPE(&ammo)=ITEM_MISSILE; GET_OBJ_TYPE(&weapon)=ITEM_WEAPON;
  int ammo_types[]={AMMO_TYPE_ARROW,AMMO_TYPE_BOLT,AMMO_TYPE_BULLET};
  int weapon_types[]={WEAPON_TYPE_LONG_BOW,WEAPON_TYPE_LIGHT_CROSSBOW,WEAPON_TYPE_SLING};
  for(int i=0;i<3;i++) for(int j=0;j<3;j++) {
    GET_OBJ_VAL(&ammo,0)=ammo_types[i]; GET_OBJ_VAL(&weapon,0)=weapon_types[j];
    assert(has_ammo_in_pouch(&ch,&weapon,true)==(i==j));
  }
  GET_OBJ_VAL(&ammo,0)=AMMO_TYPE_ARROW;GET_OBJ_VAL(&weapon,0)=WEAPON_TYPE_LONG_BOW;
  GET_OBJ_VAL(&ammo,4)=3;GET_OBJ_VAL(&ammo,AMMO_VAL_MIN)=2;
  GET_OBJ_VAL(&ammo,AMMO_VAL_MAX)=5;GET_OBJ_VAL(&ammo,AMMO_VAL_DAMAGE_TYPE)=1;
  GET_OBJ_VAL(&ammo,AMMO_VAL_DAMAGE_DIE)=6;
  initialize(&ammo);assert(GET_OBJ_VAL(&ammo,AMMO_VAL_QUANTITY)==2);
  roll=1;initialize(&ammo);assert(GET_OBJ_VAL(&ammo,AMMO_VAL_QUANTITY)==5);
  for(int i=5;i>1;i--) {
    assert(has_ammo_in_pouch(&ch,&weapon,true));
    struct obj_data *shot=take_ammo_projectile(&ammo,&weapon);
    assert(shot!=&ammo && pouch.contains==&ammo);
    assert(GET_OBJ_VAL(&ammo,AMMO_VAL_QUANTITY)==i-1);
    assert(GET_OBJ_VAL(shot,4)==3 && GET_OBJ_VAL(shot,AMMO_VAL_DAMAGE_TYPE)==1);
    assert(GET_OBJ_VAL(shot,AMMO_VAL_DAMAGE_DIE)==6);
    assert(!GET_OBJ_VAL(shot,AMMO_VAL_UNLIMITED) && !GET_OBJ_VAL(shot,AMMO_VAL_MAX));
    dispose(shot);
  }
  assert(take_ammo_projectile(&ammo,&weapon)==&ammo && !pouch.contains);
  assert(!has_ammo_in_pouch(&ch,&weapon,true));
  pouch.contains=&ammo;ammo.in_obj=&pouch;
  GET_OBJ_VAL(&ammo,AMMO_VAL_MAX)=5; GET_OBJ_VAL(&ammo,AMMO_VAL_QUANTITY)=0;
  assert(!has_ammo_in_pouch(&ch,&weapon,true));
  GET_OBJ_VAL(&ammo,AMMO_VAL_UNLIMITED)=1;
  for(int i=0;i<100;i++) {
    assert(has_ammo_in_pouch(&ch,&weapon,true));
    struct obj_data *shot=take_ammo_projectile(&ammo,&weapon);
    assert(shot!=&ammo && ammo.in_obj==&pouch);
    assert(!GET_OBJ_VAL(shot,AMMO_VAL_UNLIMITED));
    assert(GET_OBJ_VAL(&ammo,AMMO_VAL_QUANTITY)==0);
    assert(GET_OBJ_VAL(shot,4)==3 && GET_OBJ_VAL(shot,AMMO_VAL_DAMAGE_DIE)==6);
    dispose(shot);
  }
  GET_OBJ_VAL(&ammo,AMMO_VAL_UNLIMITED)=0;GET_OBJ_VAL(&ammo,AMMO_VAL_MAX)=0;
  assert(has_ammo_in_pouch(&ch,&weapon,true)); /* legacy single projectile */
  assert(take_ammo_projectile(&ammo,&weapon)==&ammo && !pouch.contains);
  pouch.contains=&ammo;ammo.in_obj=&pouch;
  GET_OBJ_VAL(&ammo,0)=AMMO_TYPE_THROWN;
  GET_OBJ_VAL(&ammo,AMMO_VAL_WEAPON_TYPE)=WEAPON_TYPE_DAGGER;
  GET_OBJ_VAL(&weapon,0)=WEAPON_TYPE_DAGGER;
  weapon_list[WEAPON_TYPE_DAGGER].weaponFlags=WEAPON_FLAG_THROWN;
  weapon_list[WEAPON_TYPE_SHURIKEN].weaponFlags=WEAPON_FLAG_THROWN;
  assert(!has_ammo_in_pouch(&ch,&weapon,true));
  THROWING(&ch)=true;assert(has_ammo_in_pouch(&ch,&weapon,true));
  GET_OBJ_VAL(&ammo,AMMO_VAL_WEAPON_TYPE)=WEAPON_TYPE_SHURIKEN;
  assert(!has_ammo_in_pouch(&ch,&weapon,true));
  GET_OBJ_VAL(&weapon,0)=WEAPON_TYPE_SHURIKEN;
  assert(has_ammo_in_pouch(&ch,&weapon,true));
  GET_OBJ_VAL(&ammo,AMMO_VAL_MAX)=10;GET_OBJ_VAL(&ammo,AMMO_VAL_QUANTITY)=0;
  assert(!has_ammo_in_pouch(&ch,&weapon,true));
  for(int returning_source=0;returning_source<2;returning_source++) {
    GET_OBJ_VAL(&ammo,AMMO_VAL_RETURNING)=returning_source==0;
    main_returning=returning_source==1;
    for(int i=0;i<20;i++) {
      assert(has_ammo_in_pouch(&ch,&weapon,true));
      struct obj_data *shot=take_ammo_projectile(&ammo,&weapon);
      assert(shot!=&ammo && pouch.contains==&ammo);
      assert(GET_OBJ_VAL(&ammo,AMMO_VAL_QUANTITY)==0);
      assert(!GET_OBJ_VAL(shot,AMMO_VAL_RETURNING) && !GET_OBJ_VAL(shot,AMMO_VAL_UNLIMITED));
      dispose(shot);
    }
  }
  THROWING(&ch)=false;SET_BIT_AR(MOB_FLAGS(&ch),MOB_ISNPC);SET_BIT_AR(MOB_FLAGS(&ch),MOB_RANGED_ATTACK);
  assert(is_throwing_weapon(&ch,&weapon));
  assert(has_ammo_in_pouch(&ch,&weapon,true));
  REMOVE_BIT_AR(MOB_FLAGS(&ch),MOB_RANGED_ATTACK);REMOVE_BIT_AR(MOB_FLAGS(&ch),MOB_ISNPC);
  THROWING(&ch)=true;
  GET_OBJ_VAL(&ammo,AMMO_VAL_RETURNING)=0;main_returning=false;
  assert(!has_ammo_in_pouch(&ch,&weapon,true));
  main_returning=true;GET_OBJ_VAL(&ammo,0)=AMMO_TYPE_ARROW;
  assert(!ammunition_is_unlimited(&ammo,&weapon));
  puts("PASS: arrow/bolt/bullet matching, load bounds, finite and unlimited consumption, legacy ammo, thrown subtype matching, Returning from either source and shot bonuses.");
}
'''
with tempfile.TemporaryDirectory(prefix='ranged-ammo-') as directory:
    cfile = Path(directory) / 'ammo.c'
    binary = Path(directory) / 'ammo'
    cfile.write_text(code)
    subprocess.run([os.environ.get('CC','cc'),'-std=c11','-g','-fsanitize=address,undefined',
                    '-fno-omit-frame-pointer','-I'+str(root/'src'),str(cfile),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
