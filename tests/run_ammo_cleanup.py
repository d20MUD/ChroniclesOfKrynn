#!/usr/bin/env python3
"""Run native preference, depletion and shot cleanup under memory sanitizers."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1]
weapons=(root/'src/assign_wpn_armor.c').read_text();fight=(root/'src/fight.c').read_text();handler=(root/'src/handler.c').read_text()
# Extraction must invalidate combat references before any extraction callbacks.
start=handler.index('void extract_obj(struct obj_data *obj)\n{')
assert handler.index('forget_ranged_projectile(obj);',start)<handler.index('legendary_object_unload(obj);',start)
assert 'release_ranged_shot(&context);' in fight

def function(text,name):
 a=text.index(name);return text[a:text.index('\n}',a)+2]
code=r'''
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "assign_wpn_armor.h"
#include "spec_abilities.h"
#include <assert.h>
#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))
#define send_to_char(...) ((void)0)
#define act(...) ((void)0)
struct weapon_table weapon_list[NUM_WEAPON_TYPES];
obj_rnum top_of_objt=1;
struct index_data indices[2];struct index_data *obj_index=indices;
static int freed;
static bool cascade;
static struct obj_data *last_missile;
void extract_obj(struct obj_data *obj);
bool obj_has_special_ability(struct obj_data *obj,int ability){return false;}
struct obj_data *create_obj(void){struct obj_data *obj=calloc(1,sizeof(*obj));IN_ROOM(obj)=NOWHERE;return obj;}
static void obj_from_obj(struct obj_data *obj){
 struct obj_data **link=&obj->in_obj->contains;
 while(*link && *link!=obj)link=&(*link)->next_content;
 assert(*link==obj);*link=obj->next_content;obj->next_content=NULL;obj->in_obj=NULL;
}
'''
a=fight.index('struct ranged_shot_context\n');b=fight.index('static struct obj_data *ranged_ammunition(',a)
code+=fight[a:b]
code+=r'''
void extract_obj(struct obj_data *obj){
 // Match native extraction's first operation before testing callback-visible references.
 forget_ranged_projectile(obj);
 for(struct ranged_shot_context *ctx=current_shot;ctx;ctx=ctx->previous){assert(ctx->projectile!=obj && ctx->extra_projectile!=obj);}
 if(cascade && current_shot && current_shot->extra_projectile){cascade=false;extract_obj(current_shot->extra_projectile);}
 assert(last_missile!=obj);assert(!obj->in_obj && !obj->next_content);
 free(obj->name);free(obj->short_description);free(obj->description);free(obj);freed++;
}
'''
for name in ['bool is_throwing_weapon(', 'bool ammunition_is_unlimited(', 'static bool usable_ammunition(', 'bool has_ammo_in_pouch(', 'struct obj_data *take_ammo_projectile(']:code+=function(weapons,name)
code+=function(fight,'static void release_ranged_shot(')
code+=r'''
static struct obj_data *bundle(int rnum,int count){
 struct obj_data *obj=create_obj();obj->item_number=rnum;GET_OBJ_TYPE(obj)=ITEM_MISSILE;
 GET_OBJ_VAL(obj,0)=AMMO_TYPE_ARROW;GET_OBJ_VAL(obj,4)=rnum+2;GET_OBJ_VAL(obj,AMMO_VAL_MAX)=50;GET_OBJ_VAL(obj,AMMO_VAL_QUANTITY)=count;
 obj->name=strdup("arrows");obj->short_description=strdup("arrows");obj->description=strdup("Arrows.");return obj;
}
int main(void){
 struct char_data ch={0};struct player_special_data ps={0};struct obj_data pouch={0},weapon={0};ch.player_specials=&ps;
 indices[0].vnum=100;indices[1].vnum=200;GET_EQ(&ch,WEAR_AMMO_POUCH)=&pouch;GET_OBJ_VAL(&weapon,0)=WEAPON_TYPE_LONG_BOW;
 GET_PREFERRED_AMMO(&ch)=201;
 struct obj_data *fallback=bundle(0,2),*preferred=bundle(1,1);pouch.contains=fallback;fallback->in_obj=&pouch;fallback->next_content=preferred;preferred->in_obj=&pouch;
 struct ranged_shot_context outer={0},inner={0};outer.attacker=&ch;current_shot=&outer;
 assert(has_ammo_in_pouch(&ch,&weapon,true) && pouch.contains==preferred);
 outer.projectile=take_ammo_projectile(pouch.contains,&weapon);last_missile=outer.projectile;
 assert(outer.projectile==preferred && GET_OBJ_VAL(outer.projectile,4)==3 && pouch.contains==fallback);
 assert(!preferred->in_obj && !preferred->next_content);assert(GET_PREFERRED_AMMO(&ch)==201);
 // The preferred stack is gone; the next shot safely selects another usable stack.
 assert(has_ammo_in_pouch(&ch,&weapon,true) && pouch.contains==fallback);
 inner.attacker=&ch;inner.previous=&outer;current_shot=&inner;
 inner.projectile=take_ammo_projectile(pouch.contains,&weapon);last_missile=inner.projectile;
 release_ranged_shot(&inner);assert(!inner.projectile && !inner.extra_projectile && !last_missile);
 current_shot=&outer;last_missile=outer.projectile;assert(GET_OBJ_VAL(last_missile,4)==3);
 release_ranged_shot(&outer);assert(!outer.projectile && !outer.extra_projectile && !last_missile && freed==2);
 outer.projectile=take_ammo_projectile(pouch.contains,&weapon);last_missile=outer.projectile;
 assert(!pouch.contains);release_ranged_shot(&outer);assert(!last_missile && freed==3);
 assert(!has_ammo_in_pouch(&ch,&weapon,true));assert(GET_PREFERRED_AMMO(&ch)==201);
 // Replacement stacks reuse the saved ID; no freed stack address is retained.
 preferred=bundle(1,2);pouch.contains=preferred;preferred->in_obj=&pouch;
 assert(has_ammo_in_pouch(&ch,&weapon,true));outer.projectile=take_ammo_projectile(pouch.contains,&weapon);
 outer.extra_projectile=take_ammo_projectile(pouch.contains,&weapon);last_missile=outer.projectile;
 assert(!pouch.contains);release_ranged_shot(&outer);assert(freed==5 && !last_missile && !outer.projectile && !outer.extra_projectile);
 // Even an accidentally aliased extra projectile cannot be freed twice.
 outer.projectile=bundle(0,1);outer.extra_projectile=outer.projectile;last_missile=outer.projectile;
 release_ranged_shot(&outer);assert(freed==6 && !last_missile);
 // An extraction callback can remove the other arrow during primary cleanup.
 outer.projectile=bundle(0,1);outer.extra_projectile=bundle(1,1);last_missile=outer.projectile;cascade=true;
 release_ranged_shot(&outer);assert(freed==8 && !last_missile && !outer.projectile && !outer.extra_projectile);
 // A proc may extract the current arrow before the usual end-of-shot cleanup.
 outer.projectile=bundle(0,1);last_missile=outer.projectile;extract_obj(outer.projectile);
 assert(!outer.projectile && !last_missile && freed==9);release_ranged_shot(&outer);assert(freed==9);
 current_shot=NULL;return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='ammo-cleanup-') as tmp:
 p=Path(tmp)/'test.c';binary=Path(tmp)/'test';p.write_text(code)
 subprocess.run(['cc','-fsanitize=address,undefined','-g','-std=gnu99','-I'+str(root/'src'),str(p),'-o',str(binary)],check=True)
 subprocess.run([str(binary)],check=True)
print('Ammo cleanup: preferred-stack depletion, safe fallback, final-shot bonuses, nested shots, Manyshot exhaustion, replacement preference, cleared references and no double frees/leaks passed.')
