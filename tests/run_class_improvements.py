#!/usr/bin/env python3
"""Exercise production class feature and performance commands with native data types."""
from pathlib import Path
import os
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
module = (root/'src/class_improvements.c').read_text()
module = '\n'.join(line for line in module.splitlines() if not line.startswith('#include'))
bard = (root/'src/bardic_performance.c').read_text()
bard = bard[bard.index('ACMD(do_perform)'):bard.index('/* function for processing individual effects')]
code = r'''
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "spells.h"
#include "comm.h"
#include "handler.h"
#include "fight.h"
#include "class.h"
#include "bardic_performance.h"
#include "mud_event.h"
#include "actions.h"
#include "assign_wpn_armor.h"
#include "perks.h"
#include "class_improvements.h"
#include "db.h"
#include "dg_scripts.h"
#include "domains_schools.h"
#include "missions.h"
#include <assert.h>

#undef ACMD
#define ACMD(name) ACMD_DECL(name)
int compute_dexterity_bonus(struct char_data *ch) {return (ch->aff_abils.dex-10)/2;}
int compute_charisma_bonus(struct char_data *ch) {return (ch->aff_abils.cha-10)/2;}
struct player_special_data dummy_mob;
struct room_data *world;
struct weapon_table weapon_list[NUM_WEAPON_TYPES];
int performance_info[MAX_PERFORMANCES][PERFORMANCE_INFO_FIELDS];
static int immunity_checks;
static int gear, die_roll=20, damage_roll=4, distance=0, actions, attacks, saves, stops;
static bool free_hand=true, valid_song=true, dual, swift=true, standard=true, move_action=true;
static bool blind, immune, casting, resistance, saved, feat=true, feat_riposte=true;
static char output[32768];
static struct char_data *target;
static struct obj_data sword;
static struct affected_type bonus_af;
static struct char_data chars[4];
static struct player_special_data players[4];
static struct group_data group;
static struct room_data room;
static const char *names[MAX_PERFORMANCES]={"song of healing","song of heroism","song of revelation"};
#undef HAS_FREE_HAND
#define HAS_FREE_HAND(ch) free_hand
#undef IS_CASTING
#define IS_CASTING(ch) casting
#undef CAN_SEE
#define CAN_SEE(a,b) (!blind)
#define compute_gear_armor_type(ch) gear
#define char_has_mud_event(ch,event) false
#define has_bard_master_of_motifs(ch) dual
#define d20(ch) die_roll
#define dice(n,s) damage_roll
#define is_mission_mob(a,b) true
#define mag_resistance(a,b,c) resistance
#define savingthrow_flatdc(...) (saves++, saved)
#define combat_distance(a,b) distance
#define combat_melee_range(a,b) 5
#define combat_attack_in_range(a,b,c,d) (distance<=5)
#define get_wielded(ch,type) (GET_EQ(ch,WEAR_WIELD_1))
#define affected_by_spell(ch,spell) ((spell)==SKILL_DEFENSIVE_STANCE && GET_SKILL(ch,1))
#define skill_name(n) names[n]
#define epic_third_stop_music(ch) (stops++)
#define epic_third_stop_song(ch,spell) (stops++)
#define list_available_performances(ch) send_to_char(ch,"Available performances\r\n")
#undef USE_SWIFT_ACTION
#undef USE_MOVE_ACTION
#undef USE_STANDARD_ACTION
#define USE_SWIFT_ACTION(ch) (swift=false, actions++)
#define USE_MOVE_ACTION(ch) (move_action=false, actions++)
#define USE_STANDARD_ACTION(ch) (standard=false, actions++)
#define is_action_available(ch,type,msg) ((type)==atSWIFT ? swift : (type)==atMOVE ? move_action : standard)
#undef IS_IMMUNE_CRITS
#define IS_IMMUNE_CRITS(a,b) (immunity_checks++, immune)
int MAX(int a,int b) {return a>b?a:b;}
int MIN(int a,int b) {return a<b?a:b;}
void basic_mud_log(const char *format,...) {(void)format;}
int get_feat_value(struct char_data *ch,int f) {(void)ch;return f==FEAT_RIPOSTE ? feat_riposte : feat;}
size_t send_to_char(struct char_data *ch,const char *format,...) {
  va_list ap;(void)ch;va_start(ap,format);
  vsnprintf(output+strlen(output),sizeof(output)-strlen(output),format,ap);va_end(ap);return strlen(output);
}
const char *act(const char *s,int hide,struct char_data *ch,struct obj_data *obj,void *vict,int type) {
  (void)s;(void)hide;(void)ch;(void)obj;(void)vict;(void)type;return s;
}
void skip_spaces_c(const char **p) {while(isspace((unsigned char)**p))++*p;}
const char *one_argument(const char *p,char *arg,size_t size) {
 size_t i=0;skip_spaces_c(&p);while(*p&&!isspace((unsigned char)*p)){if(i+1<size)arg[i++]=*p;++p;}arg[i]=0;skip_spaces_c(&p);return p;
}
int is_abbrev(const char *a,const char *b) {return *a && strlen(a)<=strlen(b)&&!strncasecmp(a,b,strlen(a));}
struct char_data *get_char_room_vis(struct char_data *ch,char *name,int *n) {(void)ch;(void)name;(void)n;return target;}
struct char_data *find_char(long n) {for(int i=0;i<4;i++)if(GET_ID(&chars[i])==n)return &chars[i];return NULL;}
int can_perform(struct char_data *ch,int n,bool check,bool silent) {(void)ch;(void)n;(void)check;(void)silent;return valid_song;}
static int counter_roll;
int hit(struct char_data *ch,struct char_data *vict,int type,int dam,int penalty,int attack_type) {
 (void)type;(void)dam;assert(penalty==-2);assert(attack_type==ATTACK_TYPE_PRIMARY);attacks++;
 assert(class_reaction_active());
 assert(class_reaction_allow_hit());assert(!class_reaction_allow_hit());
 assert(!class_measured_parry(ch,vict,counter_roll,false,attack_type));
 assert(class_opening_begin(ch,vict,TYPE_UNDEFINED,attack_type)==0);
 return 0;
}
'''
code += module + '\n' + bard
fight = (root/'src/fight.c').read_text()
pa = fight.index('  if (vict && !IS_NPC(ch) && HAS_FEAT(ch, FEAT_PRECISE_STRIKE)')
pb = fight.index('\n\n  if (HAS_FEAT(ch, FEAT_MASTER_OF_THE_MIND))', pa)
code += r'''
static int ordinary_precise_bonus(struct char_data *ch,struct char_data *vict) {
 int dambonus=0;bool display_mode=false;
''' + fight[pa:pb] + r'''
 return dambonus;
}
'''

a = fight.index('    /* Resolve the original target')
b = fight.index('    /* OK, attack should be a success', a)
code += r'''
#define HIT_MISS 0
#define epic_feat_rank(ch,f) 0
static bool native_avoid, epic_dodge, critical_seen;
static int avoidance_calls;
#define native_weapon_avoidance(...) (avoidance_calls++, native_avoid ? -1 : 0)
#define epic_third_dodge(...) epic_dodge
static long after_successful_roll(struct char_data *ch,struct char_data *victim,bool is_critical) {
 int type=TYPE_UNDEFINED,attack_type=ATTACK_TYPE_PRIMARY,calc_bab=10,modified_diceroll=5;
 bool weapon_defense=true;
''' + fight[a:b] + r'''
 critical_seen=is_critical;
 return GET_ID(victim);
}
'''
players_source = (root/'src/players.c').read_text()
a = players_source.index('        if (!strcmp(tag, "CiCd"))')
b = players_source.index('        if (!strcmp(tag, "CbFt"))', a)
loader = players_source[a:b]
saver = next(line for line in players_source.splitlines() if 'BUFFER_WRITE("CiCd:' in line)
code += r'''
static char save_buffer[1024];
#define BUFFER_WRITE(...) snprintf(save_buffer+strlen(save_buffer),sizeof(save_buffer)-strlen(save_buffer),__VA_ARGS__)
static void save_ready(struct char_data *ch) {int i;save_buffer[0]=0;
''' + saver + r'''
}
static void load_ready(struct char_data *ch,const char *line) {
 const char *tag="CiCd";
 do {
''' + loader + r'''
 } while(0);
}
'''

code += r'''
static void reset(void) {
 memset(chars,0,sizeof(chars));memset(players,0,sizeof(players));memset(&room,0,sizeof(room));
 world=&room;world[0].people=&chars[0];
 for(int i=0;i<4;i++) {
   struct char_data *ch=&chars[i];ch->player_specials=&players[i];GET_ID(ch)=i+1;
   IN_ROOM(ch)=0;GET_HIT(ch)=100;GET_POS(ch)=POS_FIGHTING;
   ch->next_in_room=i<3 ? &chars[i+1] : NULL;
   ch->group=&group;GET_EQ(ch,WEAR_WIELD_1)=&sword;
   GET_PERFORMING(ch)=-1;GET_PERFORMANCE_VAR(ch,2)=-1;
 }
 GET_OBJ_TYPE(&sword)=ITEM_WEAPON;GET_OBJ_VAL(&sword,0)=WEAPON_TYPE_LONG_SWORD;
 memset(&bonus_af,0,sizeof(bonus_af));
 for(int i=0;i<4;i++) chars[i].affected=&bonus_af;
 bonus_af.location=APPLY_HITROLL;bonus_af.bonus_type=BONUS_TYPE_COMPETENCE;
 gear=0;bonus_af.modifier=0;die_roll=20;distance=0;actions=attacks=saves=stops=0;
 free_hand=valid_song=swift=standard=move_action=feat=feat_riposte=true;
 dual=blind=immune=casting=resistance=saved=false;output[0]=0;target=&chars[1];
 for(int i=3;i<MAX_PERFORMANCES;i++) {names[i]="unused";performance_info[i][PERFORMANCE_SKILLNUM]=i;}
 for(int i=0;i<3;i++) {performance_info[i][PERFORMANCE_SKILLNUM]=i;performance_info[i][PERFORMANCE_FEATNUM]=10+i;performance_info[i][PERFORMANCE_AOE]=PERFORM_AOE_GROUP;}
}
#define LEVEL(ch,cls,n) (CLASS_LEVEL((ch),cls)=(n))
int main(void) {
 struct char_data *bard=&chars[0],*ally=&chars[1],*enemy=&chars[2],*guard=&chars[3];
 reset();
 do_perform(bard,"song of healing",0,0);assert(IS_PERFORMING(bard)&&GET_PERFORMING(bard)==0&&actions==1);
 do_perform(bard,"typo",0,0);assert(GET_PERFORMING(bard)==0&&stops==0&&actions==1);
 standard=true;valid_song=false;do_perform(bard,"song of heroism",0,0);assert(GET_PERFORMING(bard)==0&&actions==1);
 valid_song=true;feat=false;do_perform(bard,"song of heroism",0,0);assert(GET_PERFORMING(bard)==0);
 feat=true;standard=move_action=false;do_perform(bard,"song of heroism",0,0);assert(GET_PERFORMING(bard)==0);
 standard=move_action=true;dual=true;GET_PERFORMANCE_VAR(bard,3)=1;
 do_perform(bard,"song of heroism",0,0);assert(GET_PERFORMING(bard)==0&&GET_PERFORMANCE_VAR(bard,2)==1&&GET_PERFORMANCE_VAR(bard,3)==1);
 move_action=standard=true;do_perform(bard,"2 song of revelation",0,0);assert(GET_PERFORMING(bard)==0&&GET_PERFORMANCE_VAR(bard,2)==2);
 move_action=standard=true;do_perform(bard,"1 song of heroism",0,0);assert(GET_PERFORMING(bard)==1&&GET_PERFORMANCE_VAR(bard,2)==2);
 do_perform(bard,"status",0,0);assert(strstr(output,"Slot 1")&&strstr(output,"Slot 2"));
 do_perform(bard,"stop",0,0);assert(!IS_PERFORMING(bard)&&GET_PERFORMANCE_VAR(bard,2)==-1);
 LEVEL(bard,CLASS_BARD,15);IS_PERFORMING(bard)=true;GET_PERFORMING(bard)=0;
 assert(class_rhythm_bonus(bard,APPLY_HITROLL)==3);bonus_af.modifier=2;assert(class_rhythm_bonus(bard,APPLY_HITROLL)==1);
 bonus_af.modifier=4;bonus_af.location=APPLY_DAMROLL;assert(!class_rhythm_bonus(bard,APPLY_DAMROLL));bonus_af.modifier=0;
 valid_song=false;assert(!class_rhythm_bonus(bard,APPLY_HITROLL));valid_song=true;
 do_rally(bard,"ally",0,0);assert(actions==5);assert(CI_STATE(ally).rally_bonus==3&&CI_READY(bard,CI_RALLY)>time(NULL));
 assert(class_rally_save(ally,SAVING_WILL)==3);bonus_af.modifier=4;bonus_af.location=APPLY_SAVING_WILL;bonus_af.bonus_type=BONUS_TYPE_MORALE;assert(!class_rally_save(ally,SAVING_WILL));bonus_af.modifier=0;
 int old_actions=actions;swift=true;do_rally(bard,"ally",0,0);assert(actions==old_actions);
 CI_STATE(ally).rally_until=0;assert(!class_rally_save(ally,SAVING_WILL));
 reset();LEVEL(ally,CLASS_DUELIST,8);ally->epic_attack_depth=1;
 assert(class_duelist_gear(ally));free_hand=false;assert(!class_duelist_gear(ally));free_hand=true;
 LEVEL(enemy,CLASS_DUELIST,8);enemy->epic_attack_depth=1;
 FIGHTING(ally)=enemy;target=enemy;do_opening(ally,"enemy",0,0);assert(actions==1);
 assert(class_opening_begin(ally,enemy,TYPE_ATTACK_OF_OPPORTUNITY,ATTACK_TYPE_PRIMARY)==0);
 assert(class_opening_begin(ally,enemy,TYPE_UNDEFINED,ATTACK_TYPE_PRIMARY)==2);
 assert(!CI_STATE(ally).opening_until);assert(class_opening_accuracy(ally,enemy,0)==2);assert(class_opening_accuracy(ally,enemy,4)==0);assert(class_opening_damage(ally,enemy)==4);assert(!class_opening_damage(ally,enemy));
 CI_STATE(ally).opening_target=GET_ID(enemy);CI_STATE(ally).opening_until=time(NULL)+10;
 class_features_depart(enemy);assert(!CI_STATE(ally).opening_until);
 assert(class_seam_damage(ally,enemy,ATTACK_TYPE_PRIMARY)==0);
 immunity_checks=0;assert(ordinary_precise_bonus(ally,enemy)==8&&!CI_STATE(ally).precision_blocked&&immunity_checks==1);
 immune=true;immunity_checks=0;assert(ordinary_precise_bonus(ally,enemy)==0&&CI_STATE(ally).precision_blocked&&immunity_checks==1);
 assert(class_seam_damage(ally,enemy,ATTACK_TYPE_PRIMARY)==4&&immunity_checks==1);assert(!class_seam_damage(ally,enemy,ATTACK_TYPE_RANGED));
 immune=false;die_roll=20;ally->aff_abils.dex=18;assert(class_measured_parry(enemy,ally,1,false,ATTACK_TYPE_PRIMARY));assert(attacks==1);
 assert(!class_measured_parry(enemy,ally,1,false,ATTACK_TYPE_PRIMARY));
 CI_READY(ally,CI_PARRY)=0;assert(!class_measured_parry(enemy,ally,1,true,ATTACK_TYPE_PRIMARY));
 die_roll=1;assert(!class_measured_parry(enemy,ally,100,false,ATTACK_TYPE_PRIMARY));assert(CI_READY(ally,CI_PARRY)>time(NULL));
 reset();LEVEL(guard,CLASS_STALWART_DEFENDER,10);GET_SKILL(guard,1)=1;GUARDING(guard)=ally;
 assert(class_guard_ac(ally)==3);distance=10;assert(!class_guard_ac(ally));distance=0;
 do_intercept(guard,"",0,0);enemy->epic_attack_depth=1;
 assert(class_intercept(enemy,ally)==guard);assert(class_intercept(enemy,ally)==ally);
 class_recovery_start(guard);assert(CI_STATE(guard).recovery_hp==20);
 assert(class_recovery_absorb(guard,12,TYPE_SLASH,ATTACK_TYPE_PRIMARY,0)==0);
 assert(class_recovery_absorb(guard,12,SPELL_FIREBALL,ATTACK_TYPE_PRIMARY,0)==12);
 assert(class_recovery_absorb(guard,12,TYPE_SLASH,ATTACK_TYPE_PRIMARY,1)==12);
 assert(class_recovery_absorb(guard,12,TYPE_SLASH,ATTACK_TYPE_PRIMARY,0)==4);
 assert(class_recovery_absorb(guard,12,TYPE_SLASH,ATTACK_TYPE_PRIMARY,0)==12);
 GET_SKILL(guard,1)=0;assert(!class_guard_ac(ally));
 reset();LEVEL(bard,CLASS_BARD,11);IS_PERFORMING(bard)=true;GET_PERFORMING(bard)=2;
 performance_info[2][PERFORMANCE_SKILLNUM]=SKILL_SONG_OF_REVELATION;
 FIGHTING(ally)=enemy;
 assert(class_revelation(ally,enemy)&&saves==1);assert(class_revelation(ally,enemy)&&saves==1);
 CI_STATE(enemy).revelation_check=0;saved=true;assert(!class_revelation(ally,enemy)&&saves==2);
 CI_STATE(enemy).revelation_check=0;saved=false;resistance=true;assert(!class_revelation(ally,enemy)&&saves==2);
 reset();LEVEL(guard,CLASS_STALWART_DEFENDER,10);GET_SKILL(guard,1)=1;GUARDING(guard)=ally;
 CI_STATE(guard).intercept=true;enemy->epic_attack_depth=1;
 native_avoid=true;avoidance_calls=0;
 assert(!after_successful_roll(enemy,ally,true)&&avoidance_calls==1&&!CI_READY(guard,CI_INTERCEPT));
 native_avoid=false;epic_dodge=true;
 assert(!after_successful_roll(enemy,ally,true)&&!CI_READY(guard,CI_INTERCEPT));
 epic_dodge=false;assert(after_successful_roll(enemy,ally,true)==GET_ID(guard)&&critical_seen);
 CI_READY(guard,CI_INTERCEPT)=0;immune=true;
 assert(after_successful_roll(enemy,ally,true)==GET_ID(guard)&&!critical_seen);
 for(int i=0;i<CI_COOLDOWNS;i++)CI_READY(bard,i)=time(NULL)+30+i;
 save_ready(bard);memset(players[0].saved.class_improvement_ready,0,sizeof(players[0].saved.class_improvement_ready));
 char *line=save_buffer;
 for(int i=0;i<CI_COOLDOWNS;i++) {assert(!strncmp(line,"CiCd: ",6));load_ready(bard,line+6);assert(CI_READY(bard,i)>time(NULL));line=strchr(line,'\n')+1;}
 time_t ready=CI_READY(bard,0);load_ready(bard,"-1 999");load_ready(bard,"5 999");assert(CI_READY(bard,0)==ready);
 load_ready(bard,"0 -1");assert(CI_READY(bard,0)==0);
 puts("PASS: production performances, support, opening, precision fallback, parry recursion, guarding, interception/critical routing, recovery, revelation and saved cooldowns");
}
'''
with tempfile.TemporaryDirectory(prefix='class-improvements-') as temp:
    src=Path(temp)/'test.c'; binary=Path(temp)/'test'
    src.write_text(code)
    subprocess.run(['cc','-std=gnu99','-g','-Wall','-Wextra','-Wno-unused-parameter',
                    '-fsanitize=address,undefined','-fno-omit-frame-pointer','-I'+str(root/'src'),
                    str(src),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
