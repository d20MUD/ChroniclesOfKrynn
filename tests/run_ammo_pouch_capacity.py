#!/usr/bin/env python3
"""Execute the production put path to verify stack capacity independent of weight."""
from pathlib import Path
import subprocess, tempfile
root = Path(__file__).resolve().parents[1]
def function(path, signature):
    text = (root/path).read_text(); start = text.index(signature+'\n{')
    return text[start:text.index('\n}',start)+2]
code = r'''
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "spells.h"
#include <assert.h>
#undef USE_SWIFT_ACTION
#define USE_SWIFT_ACTION(ch) ((void)0)
bool is_crafting_kit(struct obj_data *obj){return true;}
static int moved;
static char message[2048];
char *CAP(char *text) {return text;}
bool housing_put(struct char_data *ch,struct obj_data *obj,struct obj_data *cont){return false;}
bool legendary_storage_check(struct char_data *ch,struct obj_data *obj,struct obj_data *cont){return true;}
int drop_otrigger(struct obj_data *obj,struct char_data *ch){return 1;}
char *get_name_by_id(long id){return NULL;}
const char *act(const char *text,int hide,struct char_data *ch,struct obj_data *obj,void *target,int type){snprintf(message,sizeof(message),"%s",text);return text;}
size_t send_to_char(struct char_data *ch,const char *format,...){return 0;}
void obj_from_char(struct obj_data *obj){}
void obj_to_obj(struct obj_data *obj,struct obj_data *cont){obj->next_content=cont->contains;cont->contains=obj;moved++;}
void update_pos(struct char_data *ch){}
int d20(struct char_data *ch){return 20;}
int compute_ability(struct char_data *ch,int ability){return 100;}
void check_room_lighting_special(room_rnum room,struct char_data *ch,struct obj_data *obj,bool out){}
'''
code += function('src/utils.c','int num_obj_in_obj(struct obj_data *obj)')
code += function('src/act.item.c','static void perform_put(struct char_data *ch, struct obj_data *obj, struct obj_data *cont)')
code += r'''
int main(void){
 struct char_data ch={0};struct obj_data pouch={0},ammo[4]={{0}},sibling={0};
 IN_ROOM(&pouch)=NOWHERE;GET_OBJ_BOUND_ID(&pouch)=NOBODY;
 GET_OBJ_TYPE(&pouch)=ITEM_AMMO_POUCH;GET_OBJ_VAL(&pouch,0)=2;
 GET_OBJ_WEIGHT(&pouch)=500;pouch.next_content=&sibling;
 for(int i=0;i<4;i++){GET_OBJ_TYPE(&ammo[i])=ITEM_MISSILE;GET_OBJ_WEIGHT(&ammo[i])=100;GET_OBJ_VAL(&ammo[i],AMMO_VAL_QUANTITY)=50;}
 // Empty pouch's own weight and inventory siblings consume no stack slots.
 perform_put(&ch,&ammo[0],&pouch);assert(moved==1);
 GET_OBJ_VAL(&ammo[1],AMMO_VAL_QUANTITY)=1;
 perform_put(&ch,&ammo[1],&pouch);assert(moved==2);
 perform_put(&ch,&ammo[2],&pouch);assert(moved==2 && strstr(message,"2 ammo stacks"));
 // Non-ammo is rejected even with space. Removing a stack frees exactly one slot.
 pouch.contains=&ammo[0];GET_OBJ_TYPE(&ammo[2])=ITEM_OTHER;
 perform_put(&ch,&ammo[2],&pouch);assert(moved==2 && strstr(message,"only put ammo"));
 GET_OBJ_TYPE(&ammo[2])=ITEM_MISSILE;
 perform_put(&ch,&ammo[2],&pouch);assert(moved==3);
 GET_OBJ_VAL(&pouch,0)=0;perform_put(&ch,&ammo[3],&pouch);assert(moved==3);
 GET_OBJ_VAL(&pouch,0)=-1;perform_put(&ch,&ammo[3],&pouch);assert(moved==4);
 // Ordinary containers retain their weight limit.
 GET_OBJ_TYPE(&pouch)=ITEM_CONTAINER;GET_OBJ_VAL(&pouch,0)=2;
 perform_put(&ch,&ammo[0],&pouch);assert(moved==4 && strstr(message,"won't fit"));
 return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='ammo-pouch-') as tmp:
    source=Path(tmp)/'test.c';binary=Path(tmp)/'test';source.write_text(code)
    subprocess.run(['cc','-g','-fsanitize=address','-std=gnu99','-I'+str(root/'src'),str(source),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
print('Ammo pouch capacity: real put path, stack boundary, weights, quantities, siblings, removal, ammo-only, zero/unlimited and ordinary containers passed.')
