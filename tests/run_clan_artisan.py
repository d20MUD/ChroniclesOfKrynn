#!/usr/bin/env python3
"""Production craft check: Artisan tiers, non-clan stacking, ranks/XP and criticals."""
import pathlib,subprocess,tempfile
root=pathlib.Path(__file__).resolve().parents[1];s=(root/'src/crafting_new.c').read_text()
def function(name):
 a=s.index(name);a=s.rfind('\n',0,a)+1
 return s[a:s.index('\n}',a)+2]
header=r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#define TRUE true
#define FALSE false
#define START_CRAFT_ABILITIES 34
#define END_CRAFT_ABILITIES 46
#define START_HARVEST_ABILITIES 48
#define END_HARVEST_ABILITIES 53
#define FEAT_CRAFT_WONDEROUS_ITEM 1
#define FEAT_CRAFT_MAGICAL_ARMS_AND_ARMOR 2
#define FEAT_EXEMPLAR 4
#define FEAT_IMPROVED_JACK_OF_ALL_TRADES 8
#define CRAFT_TYPE_MISC 1
#define CRAFT_TYPE_WEAPON 2
#define CRAFT_TYPE_ARMOR 3
#define ITEM_MASTERWORK 1
struct char_data {int npc,rank,feats,clan_artisan_bonus;int64_t clan_artisan_expires_at;struct {int craft_specialization[2],crafting_item_type;} craft;};
struct obj_data {int extra;};
#define IS_NPC(ch) ((ch)->npc)
#define GET_ABILITY(ch,skill) ((ch)->rank)
#define GET_CRAFT(ch) ((ch)->craft)
#define HAS_FEAT(ch,f) ((ch)->feats&(f))
#define GET_OBJ_EXTRA(obj) ((obj)->extra)
#define SET_BIT_AR(v,f) ((v)|=(f))
static int64_t now=100;
#define time(p) now
static int natural=10,proficient,elbow,jack,efficient,xp,reset;
static int d20(struct char_data *ch){(void)ch;return natural;}
static int get_proficient_talent_bonus(struct char_data *ch,int skill){(void)ch;(void)skill;return proficient;}
static int get_elbow_grease_bonus(struct char_data *ch){(void)ch;return elbow;}
static int get_jack_of_all_trades_bonus(struct char_data *ch){(void)ch;return jack;}
static bool artificer_efficient_crafter_applies(struct char_data *ch){(void)ch;return efficient;}
static const char *object_short_description(struct obj_data *obj){(void)obj;return "test item";}
static void send_to_char(struct char_data *ch,const char *f,...){(void)ch;(void)f;}
static void reset_current_craft(struct char_data *ch,void *obj,bool a,bool b){(void)ch;(void)obj;(void)a;(void)b;reset++;}
static void gain_craft_exp(struct char_data *ch,int exp,int skill,bool fail){(void)ch;(void)skill;assert(fail);xp+=exp;}
'''
main=r'''
int main(void){
 struct char_data ch={.rank=10,.clan_artisan_expires_at=200};struct obj_data obj={0};
 for(int bonus=2;bonus<=4;bonus++)for(int skill=34;skill<=46;skill++){
  ch.clan_artisan_bonus=bonus;
  assert(get_craft_skill_value(&ch,skill)==10);
  assert(create_craft_skill_check_with_roll(&ch,&obj,skill,"craft",25,20+bonus,NULL));
  xp=0;assert(!create_craft_skill_check_with_roll(&ch,&obj,skill,"craft",25,21+bonus,NULL));assert(xp==25&&ch.rank==10);
 }
 ch.clan_artisan_bonus=4;ch.craft.craft_specialization[0]=34;ch.craft.crafting_item_type=CRAFT_TYPE_WEAPON;ch.feats=FEAT_CRAFT_MAGICAL_ARMS_AND_ARMOR;
 proficient=2;elbow=2;jack=3;efficient=1;
 // 10 ranks + 5 specialization + 2 talent + 5 feat + 2 elbow + 3 jack + 2 artificer + 4 clan = 33.
 assert(create_craft_skill_check_with_roll(&ch,&obj,34,"craft",25,43,NULL));
 assert(!create_craft_skill_check_with_roll(&ch,&obj,34,"craft",25,44,NULL));
 assert(get_craft_skill_value(&ch,34)==15&&ch.rank==10);
 now=200;assert(get_craft_production_bonus(&ch,34)==0);now=100;
 assert(get_craft_production_bonus(&ch,47)==0&&get_craft_production_bonus(&ch,0)==0);
 ch.npc=1;assert(!get_craft_production_bonus(&ch,34));ch.npc=0;
 // Repeated checks never mutate or compound raw ranks/XP, and natural rolls retain their semantics.
 natural=1;int rolled=0;assert(!create_craft_skill_check_with_roll(&ch,&obj,34,"ammo",25,40,&rolled));assert(rolled==1&&reset==1);
 natural=20;assert(create_craft_skill_check_with_roll(&ch,&obj,34,"ammo",25,40,&rolled));assert(rolled==20&&obj.extra==ITEM_MASTERWORK);
 assert(ch.rank==10&&ch.clan_artisan_bonus==4);
 puts("Artisan production checks: all professions/tiers, additive stacking, expiry, ranks/XP isolation and natural rolls passed");
}
'''
with tempfile.TemporaryDirectory(prefix='clan-artisan-') as tmp:
 p=pathlib.Path(tmp);(p/'test.c').write_text(header+'\n'.join(function(n) for n in ['int get_craft_production_bonus(', 'int get_craft_skill_value(', 'static bool create_craft_skill_check_with_roll('])+main)
 subprocess.run(['cc','-std=gnu99','-Wall','-Wextra','-Werror','-Wno-unused-parameter','-fsanitize=address,undefined','-fno-pie','-no-pie',str(p/'test.c'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
