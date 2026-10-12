#!/usr/bin/env python3
"""Exercise real wield command, equipment checks, grip bonuses and size limits."""
from pathlib import Path
import os
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
item = (root/'src/act.item.c').read_text()
utils = (root/'src/utils.c').read_text()
fight = (root/'src/fight.c').read_text()
def function(source, name):
    start = re.search(r'^(?:static )?(?:int|bool|void) '+name+r'\([^;]*?\)\n\{',source,re.M).start()
    return source[start:source.index('\n}',start)+2]+'\n'
code = ''.join(function(item,n) for n in ['hands_used','hands_have','hands_available',
    'wield_size_difference','hands_needed_full','hands_needed','is_wielding_type',
    'perform_wear','perform_wield_at','perform_wield'])
start=item.index('ACMD(do_wield)\n{')
code+=item[start:item.index('\n}',start)+2]+'\n'
code+=function(utils,'is_weapon_wielded_two_handed')
handler=(root/'src/handler.c').read_text()
start=handler.index('struct obj_data *unequip_char(')
code+=handler[start:handler.index('\n}',start)+2]+'\n'
start=fight.index('  /* strength bonus */'); end=fight.index('  case ATTACK_TYPE_OFFHAND:',start)
code+='static int strength_bonus(struct char_data *ch,struct obj_data *wielded,int str_bonus) {\n'
code+='int attack_type=ATTACK_TYPE_PRIMARY, dambonus=0, display_mode=0; const char *strength="Strength";\n'
code+=fight[start:end]+'}\nreturn dambonus;\n}\n'
headers='\n'.join('#include "'+h+'"' for h in ['conf.h','sysdep.h','structs.h','utils.h',
    'comm.h','db.h','spells.h','act.h','interpreter.h','handler.h','class.h','clan.h',
    'spec_procs.h','assign_wpn_armor.h','dg_scripts.h'])
stubs=r'''
#include <assert.h>
#undef ACMD
#define ACMD(name) ACMD_DECL(name)
#undef GET_SIZE
#define GET_SIZE(ch) char_size
#undef GET_RACE
#define GET_RACE(ch) RACE_HUMAN
#undef HAS_FEAT
#define HAS_FEAT(ch,feat) (feat==FEAT_MONKEY_GRIP ? monkey : feat==FEAT_POWERFUL_BUILD ? powerful : 0)
#undef KNOWS_DISCOVERY
#define KNOWS_DISCOVERY(ch,id) 0
#undef IS_ANIMAL
#define IS_ANIMAL(ch) 0
#undef IS_WILDSHAPED
#define IS_WILDSHAPED(ch) shaped
#undef IS_MORPHED
#define IS_MORPHED(ch) 0
#undef CCNRM
#define CCNRM(ch,level) ""
#define affected_by_spell(ch,spell) 0
#define is_using_double_weapon(ch) 0
struct weapon_table weapon_list[NUM_WEAPON_TYPES];
struct player_special_data dummy_mob;
static int char_size=SIZE_MEDIUM, monkey, powerful, shaped, trigger=1, class_ok=1;
static char output[8192];
struct room_data rooms[1];
struct room_data *world=rooms;
static int removed_modifier;
void core_dump_real(const char *file,int line) { assert(0); }
int apply_ac(struct char_data *ch,int slot) { return 0; }
int calculate_best_mod(struct char_data *ch,int loc,int bonus,int eq,int spell) { return 0; }
void aff_apply_modify(struct char_data *ch,byte loc,sh_int mod,const char *msg) {}
void affect_modify_ar(struct char_data *ch,byte loc,sh_int mod,int bits[],bool add) { assert(!add); removed_modifier+=mod; }
void affect_total(struct char_data *ch) {}
int MAX(int a,int b) { return a>b?a:b; }
void basic_mud_log(const char *fmt,...) { assert(0); }
int compute_ability(struct char_data *ch,int ability) { return 0; }
struct obj_data *is_using_ranged_weapon(struct char_data *ch,bool melee) { return NULL; }
bool is_two_handed_ranged_weapon(struct obj_data *obj) { return false; }
bool is_class_req_object(struct char_data *ch,struct obj_data *obj,bool out) { return class_ok; }
bool is_class_anti_object(struct char_data *ch,struct obj_data *obj,bool out) { return false; }
bool violates_only_equip_one(struct char_data *ch,struct obj_data *obj) { return false; }
char *get_name_by_id(long id) { return "Owner"; }
char *CAP(char *text) { return text; }
int wear_otrigger(struct obj_data *obj,struct char_data *ch,int slot) { return trigger; }
void wear_message(struct char_data *ch,struct obj_data *obj,int slot) {}
void obj_from_char(struct obj_data *obj) { obj->carried_by->carrying=NULL; obj->carried_by=NULL; }
void equip_char(struct char_data *ch,struct obj_data *obj,int slot) { GET_EQ(ch,slot)=obj; obj->worn_by=ch; obj->worn_on=slot; }
const char *act(const char *text,int hide,struct char_data *ch,struct obj_data *obj,void *vict,int type) { return text; }
size_t send_to_char(struct char_data *ch,const char *fmt,...) {
  va_list ap; va_start(ap,fmt); int n=vsnprintf(output+strlen(output),sizeof(output)-strlen(output),fmt,ap); va_end(ap); return n;
}
const char *one_argument(const char *arg,char *out,size_t size) {
  while(isspace((unsigned char)*arg)) arg++;
  size_t n=0; while(*arg && !isspace((unsigned char)*arg)) { if(n+1<size) out[n++]=*arg; arg++; }
  out[n]=0; return arg;
}
struct obj_data *get_obj_in_list_vis(struct char_data *ch,char *name,int *num,struct obj_data *list) { return !strcmp(name,"staff") ? list : NULL; }
static void reset(struct char_data *ch,struct obj_data *staff) {
  memset(ch->equipment,0,sizeof(ch->equipment)); ch->carrying=staff; staff->carried_by=ch;
  staff->worn_by=NULL; staff->worn_on=-1; output[0]=0;
}
'''
checks=r'''
int main(void) {
  struct char_data ch={0}; struct player_special_data ps={0}; struct obj_data staff={0}, shield={0}, sword={0};
  ch.player_specials=&ps; GET_LEVEL(&ch)=30;
  GET_OBJ_TYPE(&staff)=ITEM_WEAPON; GET_OBJ_VAL(&staff,0)=WEAPON_TYPE_QUARTERSTAFF;
  GET_OBJ_SIZE(&staff)=SIZE_LARGE; GET_OBJ_BOUND_ID(&staff)=NOBODY;
  SET_BIT_AR(GET_OBJ_WEAR(&staff),ITEM_WEAR_WIELD);
  reset(&ch,&staff); do_wield(&ch,"staff onehand",0,0);
  assert(GET_EQ(&ch,WEAR_WIELD_1)==&staff && hands_used(&ch)==1 && hands_available(&ch)==1);
  assert(!is_weapon_wielded_two_handed(&staff,&ch) && strength_bonus(&ch,&staff,4)==4);
  GET_EQ(&ch,WEAR_SHIELD)=&shield;
  assert(hands_used(&ch)==2 && !is_weapon_wielded_two_handed(&staff,&ch));
  reset(&ch,&staff); GET_EQ(&ch,WEAR_SHIELD)=&shield; do_wield(&ch,"staff onehand",0,0);
  assert(GET_EQ(&ch,WEAR_WIELD_1)==&staff && hands_used(&ch)==2);
  reset(&ch,&staff); do_wield(&ch,"staff twohand",0,0);
  assert(GET_EQ(&ch,WEAR_WIELD_2H)==&staff && hands_used(&ch)==2);
  assert(is_weapon_wielded_two_handed(&staff,&ch) && strength_bonus(&ch,&staff,4)==6);
  staff.affected[0].location=APPLY_STR; staff.affected[0].modifier=4;
  removed_modifier=0; assert(unequip_char(&ch,WEAR_WIELD_2H)==&staff && removed_modifier==8);
  reset(&ch,&staff); do_wield(&ch,"staff onehand",0,0);
  removed_modifier=0; assert(unequip_char(&ch,WEAR_WIELD_1)==&staff && removed_modifier==4);
  staff.affected[0].modifier=0;
  reset(&ch,&staff); GET_EQ(&ch,WEAR_SHIELD)=&shield; do_wield(&ch,"staff twohand",0,0);
  assert(ch.carrying==&staff && !GET_EQ(&ch,WEAR_WIELD_2H));
  reset(&ch,&staff); trigger=0; do_wield(&ch,"staff onehand",0,0); assert(ch.carrying==&staff); trigger=1;
  class_ok=0; do_wield(&ch,"staff twohand",0,0); assert(ch.carrying==&staff); class_ok=1;
  do_wield(&ch,"staff banana",0,0); assert(ch.carrying==&staff);
  do_wield(&ch,"staff onehand extra",0,0); assert(ch.carrying==&staff);
  shaped=1; do_wield(&ch,"staff onehand",0,0); assert(ch.carrying==&staff); shaped=0;
  char_size=SIZE_SMALL; do_wield(&ch,"staff onehand",0,0); assert(ch.carrying==&staff);
  do_wield(&ch,"staff twohand",0,0); assert(GET_EQ(&ch,WEAR_WIELD_2H)==&staff);
  reset(&ch,&staff); char_size=SIZE_TINY; assert(hands_needed(&ch,&staff)==-1);
  do_wield(&ch,"staff twohand",0,0); assert(ch.carrying==&staff);
  monkey=1; assert(hands_needed(&ch,&staff)==2); monkey=0;
  char_size=SIZE_MEDIUM; assert(wield_size_difference(&ch,&staff)==0);
  sword=staff; GET_OBJ_VAL(&sword,0)=WEAPON_TYPE_GREAT_SWORD;
  assert(hands_needed(&ch,&sword)==2 && wield_size_difference(&ch,&sword)==1);
  reset(&ch,&sword); do_wield(&ch,"staff onehand",0,0); assert(ch.carrying==&sword);
  do_wield(&ch,"staff",0,0); assert(GET_EQ(&ch,WEAR_WIELD_2H)==&sword);
  reset(&ch,&staff); do_wield(&ch,"staff",0,0); assert(GET_EQ(&ch,WEAR_WIELD_1)==&staff);
  return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='quarterstaff-grip-') as tmp:
    path=Path(tmp)/'check.c'; path.write_text(headers+stubs+code+checks)
    binary=Path(tmp)/'check'
    subprocess.run([os.environ.get('CC','cc'),'-std=gnu99','-Wall','-Wextra',
        '-Wno-unused-parameter','-Wno-sign-compare','-I'+str(root/'src'),str(path),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
print('Quarterstaff grips: command selection, hand limits, shields, triggers, size/feat checks, strength and two-hand affection bonuses passed.')
