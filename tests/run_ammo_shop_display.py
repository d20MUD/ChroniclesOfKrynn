#!/usr/bin/env python3
"""Exercise real shop rows, transaction labels and quantity-aware stock grouping."""
from pathlib import Path
import subprocess
import tempfile
root=Path(__file__).resolve().parents[1]
def function(file,name):
    text=(root/file).read_text();start=text.index(name)
    return text[start:text.index('\n}',start)+2]
code=r'''
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "spec_abilities.h"
#include <assert.h>
#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))
static struct obj_data prototype;
struct obj_data *obj_proto=&prototype;
obj_rnum top_of_objt=0;
static int products[]={0,NOTHING};
#define SHOP_PRODUCT(shop,index) products[index]
#define buy_price(...) 20
#define count_color_chars(name) 0
static const char *drinks[]={"water"};
char *CAP(char *text){if(*text)*text=toupper((unsigned char)*text);return text;}
bool is_ranged_projectile(const struct obj_data *obj){return false;}
bool obj_has_special_ability(struct obj_data *obj,int ability){return false;}
size_t strlcpy(char *dst,const char *src,size_t size){
  size_t length=strlen(src);if(size){size_t count=MIN(length,size-1);memcpy(dst,src,count);dst[count]=0;}return length;
}
'''
for file,name in [('src/assign_wpn_armor.c','bool ammunition_is_unlimited('),('src/assign_wpn_armor.c','int ammunition_remaining('),('src/assign_wpn_armor.c','void format_ammunition_quantity('),('src/utils.c','char *object_short_description('),('src/shop.c','static int same_obj(struct obj_data *obj1, struct obj_data *obj2)\n{'),('src/shop.c','static int same_shop_item(struct obj_data *obj1, struct obj_data *obj2)\n{'),('src/shop.c','static int shop_producing(struct obj_data *item, int shop_nr)\n{'),('src/shop.c','static char *times_message(struct obj_data *obj, char *name, int num)\n{'),('src/shop.c','static char *list_object(struct obj_data *obj, int cnt, int aindex, int shop_nr,\n')]:
    code+='\n'+function(file,name)
code+=r'''
int main(void){
  struct obj_data ammo={0},other;
  GET_OBJ_TYPE(&ammo)=ITEM_MISSILE;GET_OBJ_RNUM(&ammo)=0;
  GET_OBJ_VAL(&ammo,AMMO_VAL_MAX)=40;GET_OBJ_VAL(&ammo,AMMO_VAL_QUANTITY)=40;
  ammo.name="arrows";ammo.short_description="a bundle of simple arrows";
  prototype=ammo;GET_OBJ_VAL(&prototype,AMMO_VAL_QUANTITY)=0;
  assert(shop_producing(&ammo,0)); // Prototype quantity must not disable produced stock.
  assert(strstr(list_object(&ammo,1,1,0,NULL,NULL),"simple arrows (40)"));
  assert(strstr(list_object(&ammo,1,1,0,NULL,NULL),"Unlimited")); // Shop stock, not ammo supply.
  assert(strstr(times_message(&ammo,NULL,2),"simple arrows (40) (x 2)"));
  other=ammo;assert(same_shop_item(&ammo,&other));
  GET_OBJ_VAL(&other,AMMO_VAL_QUANTITY)=17;assert(!same_shop_item(&ammo,&other));
  GET_OBJ_VAL(&ammo,AMMO_VAL_QUANTITY)=39;
  assert(strstr(list_object(&ammo,1,1,0,NULL,NULL),"simple arrows (39)"));
  assert(!strcmp(ammo.short_description,"a bundle of simple arrows"));
  GET_OBJ_VAL(&ammo,AMMO_VAL_UNLIMITED)=1;
  assert(strstr(list_object(&ammo,1,1,0,NULL,NULL),"simple arrows (unlimited)"));
  GET_OBJ_VAL(&ammo,AMMO_VAL_UNLIMITED)=0;GET_OBJ_VAL(&ammo,AMMO_VAL_QUANTITY)=0;
  assert(strstr(list_object(&ammo,1,1,0,NULL,NULL),"simple arrows (0)"));
  GET_OBJ_VAL(&ammo,AMMO_VAL_MAX)=0;
  assert(strstr(list_object(&ammo,1,1,0,NULL,NULL),"simple arrows (1)"));
  char long_name[141];memset(long_name,'a',140);long_name[140]=0;ammo.short_description=long_name;
  GET_OBJ_VAL(&ammo,AMMO_VAL_MAX)=40;GET_OBJ_VAL(&ammo,AMMO_VAL_QUANTITY)=40;
  assert(strstr(list_object(&ammo,1,1,0,NULL,NULL)," (40)"));
  puts("Ammo shop rows, transaction labels, stock grouping and unmodified base names passed");
}
'''
with tempfile.TemporaryDirectory() as temp:
    source=Path(temp)/'shop.c';binary=Path(temp)/'shop';source.write_text(code)
    subprocess.run(['cc','-std=gnu11','-I'+str(root/'src'),str(source),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
