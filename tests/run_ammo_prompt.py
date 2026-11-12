#!/usr/bin/env python3
"""Exercise the MUD-only prompt selector against real ammunition quantity rules."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1]
def function(file,signature):
    s=(root/file).read_text();start=s.index(signature)
    return s[start:s.index('\n}',start)+2]+'\n'
code=r'''
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "assign_wpn_armor.h"
#include <assert.h>
#define MAX(a,b) ((a)>(b)?(a):(b))
struct weapon_table weapon_list[NUM_WEAPON_TYPES];
struct index_data *obj_index;
obj_rnum top_of_objt=2;
'''
for file,sig in [('src/assign_wpn_armor.c','int thrown_weapon_type('),('src/assign_wpn_armor.c','int ammunition_remaining('),('src/comm.c','static bool prompt_ranged_weapon('),('src/comm.c','static void prompt_ammo_indicator(')]:code+=function(file,sig)
code+=r'''
static void check(struct char_data *ch,const char *expected) {
 char buf[64];prompt_ammo_indicator(ch,buf,sizeof(buf));assert(!strcmp(buf,expected));
}
int main(void) {
 struct char_data ch={0};struct player_special_data ps={0};
 struct obj_data bow={0},pouch={0},a={0},b={0},c={0},knife={0};
 struct index_data index[3]={{0}};obj_index=index;
 index[0].vnum=100;index[1].vnum=101;index[2].vnum=102;
 ch.player_specials=&ps;
 weapon_list[WEAPON_TYPE_LONG_BOW].weaponFlags=WEAPON_FLAG_RANGED;
 weapon_list[WEAPON_TYPE_THROWING_KNIFE].weaponFlags=WEAPON_FLAG_THROWN;
 GET_OBJ_TYPE(&bow)=ITEM_WEAPON;GET_OBJ_VAL(&bow,0)=WEAPON_TYPE_LONG_BOW;
 GET_OBJ_TYPE(&pouch)=ITEM_AMMO_POUCH;GET_EQ(&ch,WEAR_AMMO_POUCH)=&pouch;
 GET_OBJ_TYPE(&a)=GET_OBJ_TYPE(&b)=GET_OBJ_TYPE(&c)=ITEM_MISSILE;
 a.item_number=0;b.item_number=1;c.item_number=0;
 GET_OBJ_VAL(&a,AMMO_VAL_MAX)=GET_OBJ_VAL(&b,AMMO_VAL_MAX)=GET_OBJ_VAL(&c,AMMO_VAL_MAX)=100;
 GET_OBJ_VAL(&a,AMMO_VAL_QUANTITY)=7;GET_OBJ_VAL(&b,AMMO_VAL_QUANTITY)=31;GET_OBJ_VAL(&c,AMMO_VAL_QUANTITY)=5;
 pouch.contains=&a;a.next_content=&b;b.next_content=&c;
 check(&ch,"");ch.carrying=&bow;check(&ch,"Ammo:12 ");
 GET_PREFERRED_AMMO(&ch)=102;check(&ch,"Ammo:31 ");
 GET_PREFERRED_AMMO(&ch)=1000;check(&ch,"Ammo:12 ");
 GET_OBJ_VAL(&a,AMMO_VAL_QUANTITY)=0;check(&ch,"Ammo:5 ");
 GET_EQ(&ch,WEAR_AMMO_POUCH)=NULL;check(&ch,"");GET_EQ(&ch,WEAR_AMMO_POUCH)=&pouch;
 ch.carrying=NULL;GET_EQ(&ch,WEAR_WIELD_2H)=&bow;check(&ch,"Ammo:5 ");
 pouch.contains=NULL;check(&ch,"Ammo:0 ");
 GET_EQ(&ch,WEAR_WIELD_2H)=NULL;
 GET_OBJ_TYPE(&knife)=ITEM_WEAPON;GET_OBJ_VAL(&knife,0)=WEAPON_TYPE_THROWING_KNIFE;knife.item_number=2;
 pouch.contains=&knife;check(&ch,"Thrown:1 ");
 GET_OBJ_VAL(&b,0)=AMMO_TYPE_THROWN;GET_OBJ_VAL(&b,AMMO_VAL_WEAPON_TYPE)=WEAPON_TYPE_THROWING_KNIFE;
 pouch.contains=&b;b.next_content=NULL;GET_PREFERRED_AMMO(&ch)=0;check(&ch,"Thrown:31 ");
 assert(GET_OBJ_VAL(&b,AMMO_VAL_QUANTITY)==31&&pouch.contains==&b&&!THROWING(&ch));
 puts("MUD ammo prompt: preferred/fallback, stack totals, carried/equipped ranged weapons, empty/missing pouch, thrown bundles/individual weapons and no mutation passed.");return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='ammo-prompt-') as tmp:
    source=Path(tmp)/'test.c';source.write_text(code);binary=Path(tmp)/'test'
    subprocess.run(['cc','-std=gnu99','-Wall','-Wextra','-Werror','-Wno-unused-parameter','-fsanitize=address,undefined','-fno-pie','-no-pie','-Isrc',str(source),'-o',str(binary)],cwd=root,check=True)
    subprocess.run([str(binary)],check=True)
