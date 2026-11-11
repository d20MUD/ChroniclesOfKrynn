#!/usr/bin/env python3
"""Exercise the production XP award path and Scholar source boundary natively."""
import pathlib, subprocess, tempfile
root=pathlib.Path(__file__).resolve().parents[1]
s=(root/'src/limits.c').read_text()
def function(name):
 a=s.index(name);a=s.rfind('\n',0,a)+1
 return s[a:s.index('\n}',a)+2]
mode_defines='\n'.join(l for l in (root/'src/mudlim.h').read_text().splitlines() if l.startswith('#define GAIN_EXP_MODE_'))
header=r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))
#define LEG_XP_NONE 0
#define LEG_XP_KILL 1
#define LEG_XP_QUEST 2
#define LEG_XP_TRAP 3
#define LEG_XP_DAMAGE 4
#define FEAT_ADAPTABILITY 1
#define FEAT_BG_HERMIT 2
#define AFF_CHARM 1
#define LVL_IMMORT 31
#define NEWBIE_LEVEL 5
#define NEWBIE_EXP 100
#define PLR_NOWIZLIST 1
#define ACCOUNT_BONUS_EXPERIENCE 1
#define MIN_NUM_MOBS_TO_KILL_5 100
#define MIN_NUM_MOBS_TO_KILL_10 100
#define MIN_NUM_MOBS_TO_KILL_15 100
#define MIN_NUM_MOBS_TO_KILL_20 100
#define MIN_NUM_MOBS_TO_KILL_25 100
#define CONFIG_NO_MORT_TO_IMMORT 0
#define CONFIG_MAX_EXP_LOSS 10000
static int cap=INT_MAX,happy=0,account_percent=0,leadership=100,legendary_calls;
#define CONFIG_MAX_EXP_GAIN cap
#define IS_HAPPYHOUR happy
#define IS_HAPPYEXP happy
#define HAPPY_EXP 20
struct legendary_xp_context {int origin;bool eligible;};
struct descriptor {void *account;};
struct char_data {int npc,level,clan_scholar_percent,feat;int64_t clan_scholar_expires_at;long exp;struct char_data *master;struct {bool post_combat_messages;int post_combat_exp,post_combat_account_exp;} char_specials;struct descriptor *desc;};
#define IS_NPC(ch) ((ch)->npc)
#define GET_LEVEL(ch) ((ch)->level)
#define GET_EXP(ch) ((ch)->exp)
#define AFF_FLAGGED(ch,f) 0
#define PLR_FLAGGED(ch,f) 0
#define HAS_FEAT(ch,f) ((ch)->feat==(f))
#define GET_ACCEXP_DESC(ch) 0
static int64_t now=100;
#define time(p) now
#define send_to_char(...) ((void)0)
#define rand_number(a,b) 0
#define change_account_xp(ch,x) ((void)0)
static long level_exp(struct char_data *ch,int level){(void)ch;return (long)level*1000000;}
static int leadership_exp_multiplier(struct char_data *ch){(void)ch;return leadership;}
static int get_party_size_same_room(struct char_data *ch){(void)ch;return 1;}
static int apply_account_reward_bonus(struct char_data *ch,int gain,int kind){(void)ch;(void)kind;return gain+(int)((int64_t)gain*account_percent/100);}
static void legendary_credit_xp(struct char_data *ch,int gain,const struct legendary_xp_context *ctx){(void)ch;(void)gain;(void)ctx;legendary_calls++;}
static void check_stage_advancement(struct char_data *ch,int *points){(void)ch;(void)points;}
static long get_minimum_experience_floor(struct char_data *ch){(void)ch;return 0;}
static void run_autowiz(void){}
static void legendary_xp_damage_context(struct legendary_xp_context *s,struct char_data *victim){s->origin=LEG_XP_DAMAGE;s->eligible=victim&&victim->npc;}
int gain_exp_with_source(struct char_data *,int,int,const struct legendary_xp_context *);
'''
main=r'''
static struct char_data player(void){return (struct char_data){.level=12,.clan_scholar_percent=10,.clan_scholar_expires_at=200};}
int main(void){
 struct legendary_xp_context kill={LEG_XP_KILL,true}, damage={LEG_XP_DAMAGE,true}, quest={LEG_XP_QUEST,true}, invalid={LEG_XP_KILL,false};
 struct char_data ch=player(),victim={.npc=1};
 for(int tier=0;tier<3;tier++){
  int percentages[]={5,8,10};int p=percentages[tier];
  ch=player();ch.clan_scholar_percent=p;assert(gain_exp_with_source(&ch,1000,GAIN_EXP_MODE_SOLO,&kill)==1000+10*p);
  ch=player();ch.clan_scholar_percent=p;assert(gain_exp_with_source(&ch,1000,GAIN_EXP_MODE_GROUP,&kill)==1000+10*p);
  ch=player();ch.clan_scholar_percent=p;assert(gain_exp_from_damage(&ch,1000,&victim)==1000+10*p);
 }
 // Explicit provenance: every context-free mode, quests, mismatches and PvP excluded.
 for(int mode=0;mode<=10;mode++) {ch=player();assert(gain_exp(&ch,1000,mode)==1000);}
 ch=player();assert(gain_exp_with_source(&ch,1000,GAIN_EXP_MODE_QUEST,&quest)==1000);
 ch=player();assert(gain_exp_with_source(&ch,1000,GAIN_EXP_MODE_SOLO,&invalid)==1000);
 ch=player();assert(gain_exp_with_source(&ch,1000,GAIN_EXP_MODE_DAMAGE,&kill)==1000);
 ch=player();assert(gain_exp_with_source(&ch,1000,GAIN_EXP_MODE_GROUP,&damage)==1000);
 victim.npc=0;ch=player();assert(gain_exp_from_damage(&ch,1000,&victim)==1000);
 // Existing modifiers retain their order; Scholar applies once after them.
 leadership=110;happy=1;account_percent=10;ch=player();
 assert(gain_exp_with_source(&ch,1000,GAIN_EXP_MODE_SOLO,&kill)==1597); // 1100 -> 1320 -> 1452 -> 1597
 leadership=100;happy=0;account_percent=0;
 ch=player();ch.level=4;assert(gain_exp_with_source(&ch,1000,GAIN_EXP_MODE_SOLO,&kill)==2200);
 cap=1050;ch=player();assert(gain_exp_with_source(&ch,1000,GAIN_EXP_MODE_SOLO,&kill)==1050);cap=INT_MAX;
 // Existing ordinary per-mode caps still precede happy/account and Scholar.
 ch=player();assert(gain_exp_with_source(&ch,100000,GAIN_EXP_MODE_SOLO,&kill)==11000);
 ch=player();assert(gain_exp_with_source(&ch,100000,GAIN_EXP_MODE_DAMAGE,&damage)==2750);
 // No bonus at exact expiry, with disabled cache, or on NPC recipients.
 now=200;ch=player();assert(gain_exp_with_source(&ch,1000,GAIN_EXP_MODE_SOLO,&kill)==1000);now=100;
 ch=player();ch.clan_scholar_percent=0;assert(gain_exp_with_source(&ch,1000,GAIN_EXP_MODE_SOLO,&kill)==1000);
 ch=player();ch.npc=1;assert(gain_exp_with_source(&ch,1000,GAIN_EXP_MODE_SOLO,&kill)==500);
 // Capped/max-level character XP rejection never mutates the immutable source.
 ch=player();ch.exp=15000000;assert(gain_exp_with_source(&ch,1000,GAIN_EXP_MODE_SOLO,&kill)==0);assert(kill.eligible);
 ch=player();ch.level=30;assert(gain_exp_with_source(&ch,1000,GAIN_EXP_MODE_SOLO,&kill)==1100);
 ch=player();ch.exp=10000;assert(gain_exp_with_source(&ch,-1000,GAIN_EXP_MODE_DEATH,&kill)==-1000);
 ch=player();assert(scholar_adventuring_xp(&ch,INT_MAX,GAIN_EXP_MODE_SOLO,&kill)==INT_MAX);
 // Damage awards don't gain a legendary kill receipt; each legitimate award scales once.
 legendary_calls=0;victim.npc=1;ch=player();assert(gain_exp_from_damage(&ch,1000,&victim)==1100);assert(!legendary_calls);
 assert(gain_exp_with_source(&ch,1000,GAIN_EXP_MODE_SOLO,&kill)==1100);assert(legendary_calls==1&&ch.exp==2200);
 puts("Scholar production XP: tiers, provenance/exclusions, modifier order, caps, losses and independent source facts passed");
}
'''
with tempfile.TemporaryDirectory(prefix='clan-scholar-') as tmp:
 p=pathlib.Path(tmp);(p/'test.c').write_text(header+mode_defines+'\n'+'\n'.join(function(n) for n in ['int gain_exp_from_damage(', 'static int scholar_adventuring_xp(', 'int gain_exp(', 'int gain_exp_with_source('])+main)
 subprocess.run(['cc','-std=gnu99','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-pie','-no-pie',str(p/'test.c'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
