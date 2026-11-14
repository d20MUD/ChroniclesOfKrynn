#!/usr/bin/env python3
"""Actual enchantment activation and the completed cold/stamina callbacks."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1]
s=(root/'src/spec_abilities.c').read_text()
def function(signature):
 a=s.index(signature);return s[a:s.index('\n}',a)+2]
code='\n'.join('#include "'+h+'"' for h in ['conf.h','sysdep.h','structs.h','utils.h','comm.h','db.h','spec_abilities.h','spells.h','assign_wpn_armor.h','fight.h','clan_enchant.h'])+r'''
#include <assert.h>
struct special_ability_info_type special_ability_info[NUM_SPECABS];
struct weapon_table weapon_list[NUM_WEAPON_TYPES];
struct player_special_data dummy_mob;
static int damage_count,damage_amount,damage_kind;
int MIN(int a,int b){return a<b?a:b;}
int MAX(int a,int b){return a>b?a:b;}
int dice(int n,int size){return n*size;}
int damage(struct char_data *ch,struct char_data *v,int amount,int type,int kind,int ranged){damage_count++;damage_amount=amount;damage_kind=kind;return amount;}
size_t strlcpy(char *out,const char *in,size_t cap){size_t n=strlen(in);if(cap){size_t c=n<cap-1?n:cap-1;memcpy(out,in,c);out[c]=0;}return n;}
size_t send_to_char(struct char_data *ch,const char *format,...){return 0;}
const char *act(const char *s,int hide,struct char_data *ch,struct obj_data *o,void *v,int type){return s;}
bool Crash_crashsave_checked(struct char_data *ch){return true;}
'''
for name in ['bool obj_has_special_ability(', 'struct obj_special_ability *get_obj_special_ability(', 'WEAPON_SPECIAL_ABILITY(weapon_specab_frost)', 'WEAPON_SPECIAL_ABILITY(weapon_specab_icy_burst)', 'WEAPON_SPECIAL_ABILITY(weapon_specab_invigorating)']:
 code+='\n'+function(name)
code+=r'''
int main(void){
 struct obj_data o={0};struct char_data ch={0},victim={0};struct player_special_data ps={0};
 ch.player_specials=&ps;GET_OBJ_TYPE(&o)=ITEM_WEAPON;GET_OBJ_VAL(&o,0)=WEAPON_TYPE_LONG_SWORD;
 weapon_list[WEAPON_TYPE_LONG_SWORD].critMult=3;
 o.clan_enchantment.effect=WEAPON_SPECAB_ICY_BURST;strcpy(o.clan_enchantment.word,"blizzard");
 special_ability_info[WEAPON_SPECAB_ICY_BURST].activation_method=ACTMTD_ON_HIT|ACTMTD_ON_CRIT|ACTMTD_COMMAND_WORD;
 special_ability_info[WEAPON_SPECAB_ICY_BURST].special_ability_proc=weapon_specab_icy_burst;
 assert(!obj_has_special_ability(&o,WEAPON_SPECAB_ICY_BURST));
 clan_enchant_process(&o,&ch,&victim,ACTMTD_ON_CRIT,NULL);assert(damage_count==0);
 assert(clan_enchant_process(&o,&ch,NULL,ACTMTD_COMMAND_WORD,"blizzard"));
 assert(o.clan_enchantment.active && OBJ_FLAGGED(&o,ITEM_FROST));
 assert(obj_has_special_ability(&o,WEAPON_SPECAB_ICY_BURST));
 assert(get_obj_special_ability(&o,WEAPON_SPECAB_ICY_BURST)->ability==WEAPON_SPECAB_ICY_BURST);
 clan_enchant_process(&o,&ch,&victim,ACTMTD_ON_HIT,NULL);assert(damage_count==1 && damage_amount==6 && damage_kind==DAM_COLD);
 clan_enchant_process(&o,&ch,&victim,ACTMTD_ON_CRIT,NULL);assert(damage_count==2 && damage_amount==20 && damage_kind==DAM_COLD);
 clan_enchant_process(&o,&ch,NULL,ACTMTD_COMMAND_WORD,"blizzard");
 assert(!o.clan_enchantment.active && !OBJ_FLAGGED(&o,ITEM_FROST));
 clan_enchant_process(&o,&ch,&victim,ACTMTD_ON_HIT,NULL);assert(damage_count==2);
 o.clan_enchantment.effect=WEAPON_SPECAB_INVIGORATING;clan_enchant_activate(&o,true);
 special_ability_info[WEAPON_SPECAB_INVIGORATING].activation_method=ACTMTD_ON_HIT|ACTMTD_ON_CRIT;
 special_ability_info[WEAPON_SPECAB_INVIGORATING].special_ability_proc=weapon_specab_invigorating;
 GET_MOVE(&ch)=10;GET_MAX_MOVE(&ch)=100;
 clan_enchant_process(&o,&ch,&victim,ACTMTD_ON_HIT,NULL);assert(GET_MOVE(&ch)==11);
 clan_enchant_process(&o,&ch,&victim,ACTMTD_ON_CRIT,NULL);assert(GET_MOVE(&ch)==14);
 o.clan_enchantment.effect=WEAPON_SPECAB_ANARCHIC;clan_enchant_activate(&o,true);
 assert(obj_has_special_ability(&o,WEAPON_SPECAB_ANARCHIC));clan_enchant_activate(&o,false);
 assert(!obj_has_special_ability(&o,WEAPON_SPECAB_ANARCHIC));
 puts("Clan enchantments: saved activation gates passive/on-hit/critical effects; Icy Burst cold damage and Invigorating stamina recovery passed.");
}
'''
with tempfile.TemporaryDirectory(prefix='clan-enchant-') as d:
 p=Path(d);(p/'test.c').write_text(code)
 subprocess.run(['cc','-std=gnu99','-Wno-unused-parameter','-fsanitize=address,undefined','-fno-pie','-no-pie','-Isrc',str(p/'test.c'),'src/clan_enchant.c','-o',str(p/'test')],cwd=root,check=True)
 subprocess.run([str(p/'test')],check=True)
