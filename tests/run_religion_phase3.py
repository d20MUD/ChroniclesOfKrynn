#!/usr/bin/env python3
"""Native matching/typed stacking plus authored wardrobe/topology checks."""
import importlib.util
import json
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile
root=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('wardrobes',root/'examples/religion_phase3/build_wardrobes.py')
wardrobes=importlib.util.module_from_spec(spec);spec.loader.exec_module(wardrobes)
rows=list(wardrobes.entries())
assert len(rows)==4704 and len({e['id'] for e in rows})==4704
coverage={(e['org'],e['rank'],e['value'],e['position']) for e in rows}
assert len(coverage)==4704
for e in rows:
 assert len(e['short'])<=150 and len(e['keywords'])<=100 and len(e['inspection'])<2048
 assert e['deity_name'] in e['short'] and e['rank'] in range(1,9)
 assert e['kind']==('symbol' if e['position']==3 else 'armor')
 assert not any(s in e['inspection'].lower() for s in ('placeholder','todo','invincible','best-in-slot'))
# All added restricted rooms have one controlled entry and an outward route.
world=(root/'examples/religion_phase3/699.wld').read_text();rooms={}
for entry in re.split(r'(?m)^#',world)[1:]:
 vnum,body=entry.split('\n',1);_,_,rest=body.split('~',2)
 rooms[int(vnum)]=(int(rest.strip().splitlines()[0].split()[1]),{int(d):int(v) for d,v in re.findall(r'(?m)^D(\d+)\n[^~]*~\n[^~]*~\n0 -1 (\d+)',rest)})
assert len(rooms)==20
for start,sanctuary in [(69905,69909),(69915,69919)]:
 for i in range(3):
  flags,exits=rooms[start+i]
  assert all(flags&(1<<bit) for bit in (19,21,24))
  inward=[(v,d) for v,(_,es) in rooms.items() for d,dest in es.items() if dest==start+i]
  # Every room has the controlled inward edge, while inner rooms return outward.
  assert (sanctuary,0) in inward if i==0 else (start+i-1,1) in inward
  assert exits[2 if i==0 else 3]==(sanctuary if i==0 else start+i-1)
for _,exits in rooms.values():assert all(v in rooms for v in exits.values())
mobs=(root/'examples/religion_phase3/699.mob').read_text()
for vnum,deity,rank,bit in [(69904,2,3,8192),(69905,2,6,16384),(69906,2,8,16384),(69907,8,3,8192),(69908,8,6,16384),(69909,8,8,16384)]:
 block=mobs.split(f'#{vnum}\n')[1].split('\nE\n')[0]
 assert f'10 {bit} 0 65536 0 0 0 0 0 E' in block
 assert f'MobDeity: {deity}' in block and f'TempleRank: {rank}' in block
# Compile production private matchers and the actual native typed modifier routine.
handler=(root/'src/handler.c').read_text();start=handler.index('int calculate_best_mod(')
function=handler[start:handler.index('\n}',start)+2]
fight=(root/'src/fight.c').read_text()
a=fight.index('static int apply_damage_reduction_epic(',fight.index('int apply_damage_reduction(struct'))
dr=fight[a:fight.index('\n}',a)+2]
genobj=(root/'src/genobj.c').read_text();a=genobj.index('static int update_all_objects(struct obj_data *refobj)\n{')
update=genobj[a:genobj.index('\n}',a)+2]
genolc=(root/'src/genolc.c').read_text();a=genolc.index('void free_ex_descriptions_shared(')
free_shared=genolc[a:genolc.index('\n}',a)+2]
code=r'''
#include "restring.c"
#include "spells.h"
#include <assert.h>
struct weapon_table weapon_list[NUM_WEAPON_TYPES];
struct armor_table armor_list[NUM_SPEC_ARMOR_TYPES];
struct obj_data *obj_proto;obj_rnum top_of_objt;
struct clan_data *clan_list;int num_of_clans;
struct deity_info deity_list[NUM_DEITIES];
clan_rnum real_clan(clan_vnum v){return v==1?0:NO_CLAN;}
bool religion_deity_valid(int d){return d==1;}
int religion_rank(struct char_data *ch){return 8;}
int MAX(int a,int b){return a>b?a:b;}
int MIN(int a,int b){return a<b?a:b;}
int bloodline_strength_bonus(struct char_data *ch){return 0;}
bool is_weapon_wielded_two_handed(struct obj_data *o,struct char_data *ch){return false;}
int religion_blessing_value(struct char_data *ch,enum church_component c){return c==CHURCH_SAVE?4:c==CHURCH_DR?3:0;}
struct obj_data *object_list;
bool is_legacy_composite_bow(int type){return false;}
void retire_new_composite_bow(struct obj_data *obj){}
int religion_blessing_typed(struct char_data *ch,int loc,int type){return type==BONUS_TYPE_RESISTANCE && (loc==APPLY_SAVING_FORT || loc==APPLY_SAVING_REFL || loc==APPLY_SAVING_WILL)?4:0;}
'''+free_shared+'\n'+update+'\n'+function+r'''
static int flat_dr;
int compute_damage_reduction(struct char_data *ch,int type){HIT_BY_CRITICAL(ch)=0;return flat_dr;}
bool weapon_bypasses_dr(struct obj_data *obj,struct damage_reduction_type *dr,struct char_data *ch,int type){return dr->bypass_cat[0]!=0;}
bool affected_by_spell(struct char_data *ch,int spell){return false;}
bool is_bare_handed(struct char_data *ch){return true;}
bool is_monk_weapon(struct obj_data *obj){return false;}
int get_monk_dr_bypass(struct char_data *ch){return 0;}
int epic_rage_penetration(struct char_data *ch,int type){return 0;}
const char *get_wearoff(int spell){return NULL;}
void affect_from_char(struct char_data *ch,int spell){}
void send_combat_roll_info(struct char_data *ch,const char *fmt,...){}
size_t send_to_char(struct char_data *ch,const char *fmt,...){return 0;}
'''+dr+r'''
int main(void){
 struct obj_data obj={0},gear={0};struct char_data ch={0};struct affected_type af={0};
 struct json_object *e=json_tokener_parse("{\"category\":2,\"org\":1,\"rank\":1,\"position\":5,\"kind\":\"armor\",\"value\":0,\"keywords\":\"robe\",\"short\":\"a robe\",\"room\":\"A robe lies here.\"}");
 int i,j;deity_list[1].name="Test";num_of_clans=1;clan_list=calloc(1,sizeof(*clan_list));clan_list[0].clan_name="Clan";
 GET_OBJ_TYPE(&obj)=ITEM_ARMOR;SET_BIT_AR(GET_OBJ_WEAR(&obj),ITEM_WEAR_BODY);
 for(i=0;i<4;i++){armor_list[i+1].armorType=i;GET_OBJ_VAL(&obj,1)=i+1;for(j=0;j<4;j++){setn(e,"value",j);assert(matches(&obj,e)==(i==j));}}
 GET_OBJ_TYPE(&obj)=ITEM_WORN;setn(e,"value",0);assert(matches(&obj,e));setn(e,"value",3);assert(!matches(&obj,e));
 setn(e,"position",WEAR_SHIELD);sets(e,"kind","shield");SET_BIT_AR(GET_OBJ_WEAR(&obj),ITEM_WEAR_SHIELD);GET_OBJ_TYPE(&obj)=ITEM_ARMOR;
 for(i=SPEC_ARMOR_TYPE_BUCKLER;i<=SPEC_ARMOR_TYPE_TOWER_SHIELD;i++){GET_OBJ_VAL(&obj,1)=i;for(j=SPEC_ARMOR_TYPE_BUCKLER;j<=SPEC_ARMOR_TYPE_TOWER_SHIELD;j++){setn(e,"value",j);assert(matches(&obj,e)==(i==j));}}
 sets(e,"kind","weapon");GET_OBJ_TYPE(&obj)=ITEM_WEAPON;SET_BIT_AR(GET_OBJ_WEAR(&obj),ITEM_WEAR_WIELD);GET_OBJ_VAL(&obj,0)=1;
 for(i=0;i<NUM_WEAPON_FAMILIES;i++){weapon_list[1].weaponFamily=i;for(j=0;j<NUM_WEAPON_FAMILIES;j++){setn(e,"value",j);setn(e,"position",WEAR_WIELD_1);assert(matches(&obj,e)==(i==j));}}
 for(i=WEAR_WIELD_1;i<=WEAR_WIELD_2H;i++)if(weapon_slot(i)){setn(e,"position",i);setn(e,"value",0);weapon_list[1].weaponFamily=0;assert(matches(&obj,e));obj.worn_by=&ch;obj.worn_on=i;assert(matches(&obj,e));obj.worn_on=WEAR_SHIELD;assert(!matches(&obj,e));obj.worn_by=NULL;}
 sets(e,"kind","symbol");setn(e,"position",WEAR_NECK_1);GET_OBJ_TYPE(&obj)=ITEM_TREASURE;SET_BIT_AR(GET_OBJ_WEAR(&obj),ITEM_WEAR_NECK);
 for(i=0;i<4;i++){setn(e,"value",i);assert(matches(&obj,e));obj.worn_by=&ch;obj.worn_on=WEAR_NECK_2;assert(matches(&obj,e));obj.worn_on=WEAR_BODY;assert(!matches(&obj,e));obj.worn_by=NULL;}
 setn(e,"rank",9);assert(!matches(&obj,e));setn(e,"rank",1);setn(e,"org",2);assert(!matches(&obj,e));
 GET_OBJ_LEVEL(&obj)=0;assert(price(&obj)==100);GET_OBJ_LEVEL(&obj)=30;assert(price(&obj)==3000);GET_OBJ_LEVEL(&obj)=-1;assert(price(&obj)==-1);
 /* The church bonus shares the resistance bucket with existing gear/affects. */
 assert(calculate_best_mod(&ch,APPLY_SAVING_FORT,BONUS_TYPE_RESISTANCE,-1,-1)==4);
 af.location=APPLY_SAVING_FORT;af.bonus_type=BONUS_TYPE_RESISTANCE;af.modifier=6;af.spell=1;ch.affected=&af;
 assert(calculate_best_mod(&ch,APPLY_SAVING_FORT,BONUS_TYPE_RESISTANCE,-1,-1)==6);
 af.modifier=2;assert(calculate_best_mod(&ch,APPLY_SAVING_FORT,BONUS_TYPE_RESISTANCE,-1,-1)==4);
 GET_EQ(&ch,WEAR_BODY)=&gear;gear.affected[0].location=APPLY_SAVING_FORT;gear.affected[0].bonus_type=BONUS_TYPE_RESISTANCE;gear.affected[0].modifier=8;
 assert(calculate_best_mod(&ch,APPLY_SAVING_FORT,BONUS_TYPE_RESISTANCE,-1,-1)==8);
 af.modifier=-2;assert(calculate_best_mod(&ch,APPLY_SAVING_FORT,BONUS_TYPE_RESISTANCE,-1,-1)==6);
 {
  struct char_data attacker={0};struct damage_reduction_type stronger={0};
  attacker.player_specials=calloc(1,sizeof(*attacker.player_specials));
  assert(apply_damage_reduction_epic(&attacker,&ch,NULL,20,false,0,NULL)==17);
  stronger.amount=8;stronger.max_damage=-1;GET_DR(&ch)=&stronger;
  assert(apply_damage_reduction_epic(&attacker,&ch,NULL,20,false,0,NULL)==12);
  stronger.amount=1;assert(apply_damage_reduction_epic(&attacker,&ch,NULL,20,false,0,NULL)==17);
  flat_dr=2;HIT_BY_CRITICAL(&ch)=1;
  assert(apply_damage_reduction_epic(&attacker,&ch,NULL,20,false,0,NULL)==19 && HIT_BY_CRITICAL(&ch));
  flat_dr=5;assert(apply_damage_reduction_epic(&attacker,&ch,NULL,20,false,0,NULL)==19);
  flat_dr=0;stronger.amount=8;stronger.bypass_cat[0]=1;
  assert(apply_damage_reduction_epic(&attacker,&ch,NULL,20,false,0,NULL)==17);
  free(attacker.player_specials);
 }
 {
  struct obj_data instance={0},prototype={0};struct extra_descr_data oldextra={0};
  prototype.item_number=instance.item_number=1;prototype.name="prototype";prototype.ex_description=&oldextra;
  instance.name="custom";instance.short_description="a custom symbol";instance.description="Custom.";
  instance.restring_extras_cleared=true;strcpy(instance.restring_inspection,"An independent description.");
  instance.religion_requirement.deity=1;instance.religion_requirement.rank=4;instance.clan_requirement.clan=1;
  object_list=&instance;assert(update_all_objects(&prototype)==1);
  assert(instance.restring_extras_cleared && !instance.ex_description && !strcmp(instance.name,"custom"));
  assert(instance.religion_requirement.rank==4 && instance.clan_requirement.clan==1);
 }
 {
  struct extra_descr_data prototype={0},*private=calloc(1,sizeof(*private));
  prototype.keyword=strdup("shared keyword");prototype.description=strdup("shared description");
  *private=prototype;private->next=&prototype;
  free_ex_descriptions_shared(private,&prototype);
  assert(!strcmp(prototype.keyword,"shared keyword") && !strcmp(prototype.description,"shared description"));
  free(prototype.keyword);free(prototype.description);
 }
 json_object_put(e);free(clan_list);
 puts("PASS: native armor/shield/all weapon families, explicit slots, four neck variants, invalid matches, item-level price bounds and native resistance stacking");
}
'''
with tempfile.TemporaryDirectory(prefix='religion-phase3-') as directory:
 temp=Path(directory);(temp/'test.c').write_text(code)
 flags=shlex.split(subprocess.check_output(['pkg-config','--cflags','--libs','mariadb'],text=True))
 subprocess.run([os.environ.get('CC','cc'),'-std=gnu99','-ffunction-sections','-fdata-sections','-I'+str(root/'src'),*shlex.split(os.environ.get('RELIGION_TEST_FLAGS','')),str(temp/'test.c'),'-Wl,--gc-sections',*flags,'-ljson-c','-o',str(temp/'test')],check=True)
 subprocess.run([str(temp/'test')],cwd=temp,check=True)
print('PASS: 4704 authored wardrobe entries and rank 3/6/8 archive routes')
