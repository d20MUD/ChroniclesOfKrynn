#!/usr/bin/env python3
"""Exercise legacy-bow retirement, saved-instance preservation and Strength damage."""
from pathlib import Path
import os
import subprocess
import tempfile
root=Path(__file__).resolve().parents[1]
def source(f):return (root/'src'/f).read_text()
def function(text,name):
 a=text.index(name);return text[a:text.index('\n}',a)+2]
w=source('assign_wpn_armor.c');t=source('treasure.c');fight=source('fight.c')
headers=['conf.h','sysdep.h','structs.h','utils.h','interpreter.h','assign_wpn_armor.h','db.h','treasure.h','crafting_recipes.h','crafting_new.h']
code='\n'.join('#include "'+h+'"' for h in headers)+r'''
#include <assert.h>
#include <stdarg.h>
struct weapon_table weapon_list[NUM_WEAPON_TYPES];
struct obj_data *obj_proto;
obj_rnum top_of_objt=0;
static char menu[32000];
#define legendary_tree_present(obj) false
#define send_to_char(...) ((void)0)
#define is_throwing_weapon(ch,obj) false
int MIN(int a,int b) {return a<b?a:b;}
size_t write_to_output(struct descriptor_data *d,const char *fmt,...) {
 va_list args;va_start(args,fmt);int n=vsnprintf(menu+strlen(menu),sizeof(menu)-strlen(menu),fmt,args);va_end(args);return n;
}
'''
for name in ['bool is_legacy_composite_bow(','int current_weapon_type(','static char *current_bow_description(','void retire_new_composite_bow(']:code+='\n'+function(w,name)
for name in ['void set_weapon_object_for_restore(','void set_weapon_object(']:code+='\n'+function(t,name)
code+=r'''
struct obj_data *read_object_for_restore(obj_vnum nr,int mode) {
 struct obj_data *obj=malloc(sizeof(*obj));*obj=obj_proto[0];return obj;
}
'''
code+='\n'+function(source('db.c'),'struct obj_data *read_object(obj_vnum')
code+='\n'+function(source('oedit.c'),'void oedit_disp_weapon_type_menu(')
code+='\n'+function(source('crafting_new.c'),'bool is_valid_craft_weapon(')
code+=r'''
struct craft_recipe_data crafting_recipes[NUM_CRAFTING_RECIPES];
static int draw_count,draw_mode;
int craft_recipe_by_type(int type) {return ITEM_WEAPON;}
int dice(int number,int size) {draw_count++;return draw_count==1?1:draw_mode?1:draw_count==2?1:3;}
'''
for name in ['int select_random_craft_recipe(','int select_stable_craft_recipe(']:
 code+='\n'+function(source('crafting_new.c'),name)

a=fight.index('  case ATTACK_TYPE_RANGED:',fight.index('int compute_damage_bonus('));b=fight.index('\n    if (vict)',a)
code+='\nstatic int ranged_strength(struct obj_data *wielded,int str_bonus) { int dambonus=0;bool display_mode=false;struct char_data *ch=NULL;switch(ATTACK_TYPE_RANGED) {\n'+fight[a:b]+'\nbreak; }return dambonus;}\n'
code+='\n'+function(source('genobj.c'),'static int update_all_objects(struct obj_data *refobj)\n{')
code+=r'''
static int weapon_draw;
static int retired_ids[]={45,46,66,67,68,69,70,71,72,73};
int rand_number(int low,int high) {return weapon_draw<10?retired_ids[weapon_draw++]:WEAPON_TYPE_LONG_BOW;}
'''
a=t.index('  do { roll = rand_number(1, NUM_WEAPON_TYPES - 1); }');b=t.index('\n',a)
code+='\n'+function(w,'bool is_pure_thrown_weapon_type(')
code+='\nstatic int treasure_weapon_type(void) {int roll;\n'+t[a:b]+'\nreturn roll;}\n'

code+=r'''
struct obj_data *object_list;
static void destroy(struct obj_data *obj) {
 if(obj->name!=obj_proto[0].name)free(obj->name);
 if(obj->short_description!=obj_proto[0].short_description)free(obj->short_description);
 if(obj->description!=obj_proto[0].description)free(obj->description);
 free(obj);
}
int main(void) {
 struct obj_data proto={0};obj_proto=&proto;proto.item_number=0;
 GET_OBJ_TYPE(&proto)=ITEM_WEAPON;
 proto.name="composite long bow (5)";proto.short_description="a composite long bow (5)";
 proto.description="A composite long bow (5) lies here.";
 GET_OBJ_VAL(&proto,0)=WEAPON_TYPE_COMPOSITE_LONGBOW_5;
 GET_OBJ_VAL(&proto,4)=4;GET_OBJ_VAL(&proto,1)=3;
 struct obj_data *saved=read_object_for_restore(1,0),*fresh=read_object(1,0);
 assert(GET_OBJ_VAL(saved,0)==WEAPON_TYPE_COMPOSITE_LONGBOW_5);
 assert(!strcmp(saved->name,"composite long bow (5)"));
 assert(GET_OBJ_VAL(fresh,0)==WEAPON_TYPE_LONG_BOW);
 assert(!strcmp(fresh->name,"long bow"));assert(!strcmp(fresh->short_description,"a long bow"));
 assert(!strcmp(fresh->description,"A long bow lies here."));
 assert(GET_OBJ_VAL(fresh,4)==4 && GET_OBJ_VAL(fresh,1)==3);
 assert(GET_OBJ_VAL(&proto,0)==WEAPON_TYPE_COMPOSITE_LONGBOW_5);
 assert(!strcmp(proto.name,"composite long bow (5)"));
 retire_new_composite_bow(fresh);retire_new_composite_bow(NULL);
 destroy(fresh);destroy(saved);
 int legacy_count=0;
 for(int i=1;i<NUM_WEAPON_TYPES;i++) {
  struct obj_data obj={0};char name[64];snprintf(name,sizeof(name),"weapon_%d",i);weapon_list[i].name=strdup(name);
  bool legacy=(i==45 || i==46 || (i>=66 && i<=73));
  assert(is_legacy_composite_bow(i)==legacy);assert(is_valid_craft_weapon(i)==!legacy);
  set_weapon_object_for_restore(&obj,i);assert(GET_OBJ_VAL(&obj,0)==i);
  set_weapon_object(&obj,i);assert(GET_OBJ_VAL(&obj,0)==current_weapon_type(i));
  if(legacy) {legacy_count++;assert(!is_legacy_composite_bow(GET_OBJ_VAL(&obj,0)));}
 }
 assert(legacy_count==10);
 assert(treasure_weapon_type()==WEAPON_TYPE_LONG_BOW && weapon_draw==10);
 for(int i=0;i<NUM_CRAFTING_RECIPES;i++) {crafting_recipes[i].object_type=ITEM_WEAPON;crafting_recipes[i].practical_type=WEAPON_TYPE_LONG_BOW;}
 crafting_recipes[1].practical_type=WEAPON_TYPE_COMPOSITE_LONGBOW_5;
 crafting_recipes[2].practical_type=WEAPON_TYPE_COMPOSITE_SHORTBOW_5;
 assert(select_random_craft_recipe()==3);assert(draw_count==3);
 draw_count=0;draw_mode=1;assert(select_random_craft_recipe()==CRAFT_RECIPE_NONE);
 for(int seed=0;seed<10000;seed++) {int r=select_stable_craft_recipe(seed);assert(r==0 || !is_legacy_composite_bow(crafting_recipes[r].practical_type));}
 weapon_list[0].name="undefined";
 oedit_disp_weapon_type_menu(NULL);
 for(int i=1;i<NUM_WEAPON_TYPES;i++) {
  char entry[80];snprintf(entry,sizeof(entry),"%3d) %-27s",i,weapon_list[i].name);
  assert((strstr(menu,entry)!=NULL)==!is_legacy_composite_bow(i));free((void*)weapon_list[i].name);
 }
 struct obj_data bow={0};GET_OBJ_TYPE(&bow)=ITEM_WEAPON;
 int normal[]={WEAPON_TYPE_LONG_BOW,WEAPON_TYPE_SHORT_BOW};
 for(int i=0;i<2;i++) {GET_OBJ_VAL(&bow,0)=normal[i];assert(ranged_strength(&bow,17)==17);assert(ranged_strength(&bow,-3)==-3);}
 for(int i=1;i<NUM_WEAPON_TYPES;i++) if(is_legacy_composite_bow(i)) {
  GET_OBJ_VAL(&bow,0)=i;int rating=i==45||i==46?1:i<=69?i-64:i-68;
  assert(ranged_strength(&bow,17)==rating);assert(ranged_strength(&bow,1)==1);assert(ranged_strength(&bow,-2)==-2);
 }
 GET_OBJ_VAL(&bow,0)=WEAPON_TYPE_HEAVY_CROSSBOW;assert(ranged_strength(&bow,17)==0);
 GET_OBJ_VAL(&bow,0)=WEAPON_TYPE_COMPOSITE_LONGBOW_5;SET_BIT_AR(GET_OBJ_EXTRA(&bow),ITEM_ADAPTIVE);assert(ranged_strength(&bow,17)==17);
 /* A prototype maintenance edit keeps legacy owners legacy while fresh
  * ordinary instances cannot become legacy again. */
 saved=read_object_for_restore(1,0);fresh=read_object(1,0);
 /* update_all_objects replaces strings; release the fresh owned copies first. */
 free(fresh->name);free(fresh->short_description);free(fresh->description);
 fresh->name=fresh->short_description=fresh->description=NULL;
 object_list=saved;saved->next=fresh;fresh->next=NULL;
 assert(update_all_objects(&proto)==2);
 assert(GET_OBJ_VAL(saved,0)==WEAPON_TYPE_COMPOSITE_LONGBOW_5);
 assert(GET_OBJ_VAL(fresh,0)==WEAPON_TYPE_LONG_BOW);
 destroy(saved);destroy(fresh);
 puts("Legacy bow policy, fresh/restored objects, menus, crafting and Strength damage passed.");
}
'''
# Saved inventories and persisted cargo use the restoration entry point.
assert 'read_object(' not in source('objsave.c')
assert 'set_weapon_object_for_restore(obj, GET_OBJ_VAL(obj, 0))' in source('db.c')
with tempfile.TemporaryDirectory(prefix='legacy-bows-') as directory:
 p=Path(directory);(p/'test.c').write_text(code)
 subprocess.run(['gcc','-std=gnu11','-g','-fsanitize=address,undefined','-fno-pie','-no-pie','-I.', '-Isrc',str(p/'test.c'),'-o',str(p/'test')],cwd=root,check=True)
 subprocess.run([str(p/'test')],env={**os.environ,'ASAN_OPTIONS':'detect_leaks=1'},check=True)
