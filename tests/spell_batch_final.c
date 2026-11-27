/* Execute the production final batch with native structs and controlled engine services. */
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "spells.h"
#include "fight.h"
#include "mud_event.h"
#include "actions.h"
#include "assign_wpn_armor.h"
#include "domains_schools.h"
#include "psionics.h"
#include "grapple.h"
#include "dg_scripts.h"
#include "perfmon.h"
#include "spell_batch_final.h"
#include "religion.h"
#include <assert.h>

struct spell_info_type spell_info[TOP_SKILL_DEFINE+1];
struct char_data *character_list;
struct room_data rooms[2], *world=rooms;
room_rnum top_of_world=1;
struct weapon_table weapon_list[NUM_WEAPON_TYPES];
int test_native_dr(struct char_data *,struct char_data *,struct obj_data *,int,int);
int get_feat_value(struct char_data *ch,int f){return 0;}
int char_has_evolution(struct char_data *ch,int f){return 0;}
int get_effective_draconic_bloodline_level(struct char_data *ch){return 0;}
bool has_perk(struct char_data *ch,int p){return false;}
bool has_blackguard_dark_channel(struct char_data *ch){return false;}
bool is_bare_handed(struct char_data *ch){return false;}
bool is_monk_weapon(struct obj_data *obj){return false;}
int get_monk_dr_bypass(struct char_data *ch){return 0;}
int epic_rage_penetration(struct char_data *ch,int type){return 0;}
static int church_dr;
int religion_blessing_value(struct char_data *ch,enum church_component kind){return church_dr;}
void send_combat_roll_info(struct char_data *ch,const char *format,...){}
const char *get_wearoff(int spell){return "The ward ends.";}
static int save_result, resist_result, immune, incorporeal, flat_dr, damage_calls, last_damage;
static int action_available=1, action_spent, roll=20, escape_bonus, kill_on_damage, exclude_aoe;
static struct char_data *target_lookup;
void PERF_prof_sect_init(struct PERF_prof_sect **p,const char *s){}
void PERF_prof_sect_enter(struct PERF_prof_sect *p){}
void PERF_prof_sect_exit(struct PERF_prof_sect *p){}
int MAX(int a,int b){return a>b?a:b;}int MIN(int a,int b){return a<b?a:b;}
int dice(int n,int sides){return n*sides;}
int d20(struct char_data *ch){return roll;}
int compute_strength_bonus(struct char_data *ch){return 0;}
int compute_wisdom_bonus(struct char_data *ch){return 3;}
int compute_ability(struct char_data *ch,int skill){return escape_bonus;}
int str_cmp(const char *a,const char *b){while(*a&&tolower(*a)==tolower(*b)){a++;b++;}return tolower(*a)-tolower(*b);}
const char *one_argument(const char *s,char *out,size_t len){size_t n=0;while(*s==' ')s++;while(*s&&*s!=' '){if(n+1<len)out[n++]=*s;s++;}out[n]=0;while(*s==' ')s++;return s;}
size_t send_to_char(struct char_data *ch,const char *format,...){return 0;}
const char *act(const char *s,int hide,struct char_data *ch,struct obj_data *obj,void *v,int type){return s;}
bool is_action_available(struct char_data *ch,action_type type,bool message){return action_available;}
void start_action_cooldown(struct char_data *ch,action_type type,int duration){action_spent++;}
int compute_current_size(struct char_data *ch){return ch->points.size;}
bool is_incorporeal(struct char_data *ch){return incorporeal;}
sbyte is_immune_mind_affecting(struct char_data *ch,struct char_data *v,sbyte display){return immune;}
bool can_blind(struct char_data *ch){return !immune;}
int mag_resistance(struct char_data *ch,struct char_data *v,int mod){return resist_result;}
int savingthrow_full(struct char_data *ch,struct char_data *v,int type,int mod,int cast,int level,int school,int spell){return save_result;}
int savingthrow_flatdc(struct char_data *ch,struct char_data *v,int type,int mod,int dc,int cast,int school,int spell){assert(dc>=10);return save_result;}
int compute_damage_reduction(struct char_data *ch,int type){HIT_BY_CRITICAL(ch)=0;HIT_BY_SNEAK_ATTACK(ch)=0;return flat_dr;}
struct obj_data *get_wielded(struct char_data *ch,int type){return type==ATTACK_TYPE_UNARMED?NULL:GET_EQ(ch,WEAR_WIELD_1);}
struct char_data *get_char_room_vis(struct char_data *ch,char *name,int *number){return target_lookup;}
bool set_fighting(struct char_data *ch,struct char_data *v){FIGHTING(ch)=v;return true;}
struct char_data *find_char(long id){struct char_data *ch;for(ch=character_list;ch;ch=ch->next)if(GET_ID(ch)==id)return ch;return NULL;}
int aoeOK(struct char_data *ch,struct char_data *v,int spell){return ch!=v&&!exclude_aoe;}
void new_affect(struct affected_type *af){memset(af,0,sizeof(*af));}
bool affected_by_spell(struct char_data *ch,int spell){struct affected_type *a;for(a=ch->affected;a;a=a->next)if(a->spell==spell)return true;return false;}
void affect_from_char(struct char_data *ch,int spell){
 struct affected_type **a=&ch->affected;struct damage_reduction_type **d=&GET_DR(ch);
 while(*a){if((*a)->spell==spell){struct affected_type *old=*a;*a=old->next;free(old);}else a=&(*a)->next;}
 while(*d){if((*d)->spell==spell){struct damage_reduction_type *old=*d;*d=old->next;free(old);}else d=&(*d)->next;}
}
void affect_join(struct char_data *ch,struct affected_type *af,bool a,bool b,bool c,bool d){
 struct affected_type **p=&ch->affected,*n;
 while(*p){if((*p)->spell==af->spell&&(*p)->location==af->location){n=*p;*p=n->next;free(n);break;}p=&(*p)->next;}
 n=malloc(sizeof(*n));*n=*af;n->next=ch->affected;ch->affected=n;
}
int damage(struct char_data *ch,struct char_data *v,int amount,int spell,int type,int offhand){
 damage_calls++;last_damage=amount;
 if(kill_on_damage){struct char_data **p=&character_list;while(*p&&*p!=v)p=&(*p)->next;if(*p)*p=v->next;affect_from_char(v,PSIONIC_CRYSTALLINE_BONDS);return -1;}
 return amount;
}
static struct affected_type *effect(struct char_data *ch,int spell,int location){struct affected_type *a;for(a=ch->affected;a;a=a->next)if(a->spell==spell&&a->location==location)return a;return NULL;}
static void clear(struct char_data *ch){while(ch->affected)affect_from_char(ch,ch->affected->spell);assert(!GET_DR(ch));memset(AFF_FLAGS(ch),0,sizeof(ch->char_specials.saved.affected_by));}

int main(void){
 struct char_data c={0},v={0},ally={0};struct player_special_data cp={0},vp={0},ap={0};
 struct obj_data sword={0};struct affected_type af={0};struct damage_reduction_type other={0};
 int i;
 c.player_specials=&cp;v.player_specials=&vp;ally.player_specials=&ap;
 GET_ID(&c)=11;GET_ID(&v)=12;GET_ID(&ally)=13;c.next=&v;v.next=&ally;character_list=&c;
 IN_ROOM(&c)=IN_ROOM(&v)=IN_ROOM(&ally)=0;GET_POS(&c)=GET_POS(&v)=GET_POS(&ally)=POS_STANDING;
 c.points.size=v.points.size=SIZE_MEDIUM;CASTING_CLASS(&c)=CLASS_CLERIC;
 for(i=555;i<=564;i++)spell_info[i].name="test spell";
 spell_info[PSIONIC_CRYSTALLINE_BONDS].name="crystalline bonds";spell_info[PSIONIC_BATTLESENSE].name="battlesense";
 final_batch_affect(3,&c,&c,SPELL_DEFENDING_BONE,0);
 assert(GET_DR(&c)&&GET_DR(&c)->amount==5&&GET_DR(&c)->max_damage==15);
 assert(GET_DR(&c)->bypass_cat[0]==DR_BYPASS_CAT_DAMTYPE&&GET_DR(&c)->bypass_val[0]==DR_DAMTYPE_BLUDGEONING);
 assert(effect(&c,SPELL_DEFENDING_BONE,APPLY_DR)->duration==3600);
 final_batch_affect(20,&c,&c,SPELL_DEFENDING_BONE,0);assert(GET_DR(&c)->max_damage==50&&!GET_DR(&c)->next);
 GET_OBJ_TYPE(&sword)=ITEM_WEAPON;weapon_list[GET_WEAPON_TYPE(&sword)].damageTypes=DAMAGE_TYPE_SLASHING;
 assert(test_native_dr(&v,&c,&sword,20,ATTACK_TYPE_PRIMARY)==15&&GET_DR(&c)->max_damage==45);
 GET_DR(&c)->max_damage=2;assert(test_native_dr(&v,&c,&sword,20,ATTACK_TYPE_PRIMARY)==18&&!GET_DR(&c)&&!c.affected);
 final_batch_affect(20,&c,&c,SPELL_DEFENDING_BONE,0);
 assert(test_native_dr(&v,&c,NULL,20,ATTACK_TYPE_UNARMED)==20&&GET_DR(&c)->max_damage==50);
 weapon_list[GET_WEAPON_TYPE(&sword)].damageTypes=DAMAGE_TYPE_BLUDGEONING;
 assert(test_native_dr(&v,&c,&sword,20,ATTACK_TYPE_PRIMARY)==20&&GET_DR(&c)->max_damage==50);
 weapon_list[GET_WEAPON_TYPE(&sword)].damageTypes=DAMAGE_TYPE_SLASHING;
 HIT_BY_CRITICAL(&c)=1;HIT_BY_SNEAK_ATTACK(&c)=1;flat_dr=3;assert(final_batch_other_dr(&c));assert(HIT_BY_CRITICAL(&c)&&HIT_BY_SNEAK_ATTACK(&c));
 clear(&c);final_batch_affect(3,&c,&c,SPELL_DEFENDING_BONE,0);assert(!c.affected);flat_dr=0;
 church_dr=3;assert(final_batch_other_dr(&c));final_batch_affect(3,&c,&c,SPELL_DEFENDING_BONE,0);assert(!c.affected);church_dr=0;
 GET_DR(&c)=&other;other.spell=SPELL_STONESKIN;other.amount=10;final_batch_affect(3,&c,&c,SPELL_DEFENDING_BONE,0);assert(!c.affected);GET_DR(&c)=NULL;
 final_batch_affect(14,&c,&c,SPELL_RIGHTEOUS_MIGHT,0);assert(GET_DR(&c)->amount==5&&GET_DR(&c)->max_damage==-1&&GET_DR(&c)->bypass_val[0]==DR_ALIGNTYPE_EVIL);
 assert(effect(&c,SPELL_RIGHTEOUS_MIGHT,APPLY_SIZE)->modifier==1&&effect(&c,SPELL_RIGHTEOUS_MIGHT,APPLY_STR)->modifier==4&&effect(&c,SPELL_RIGHTEOUS_MIGHT,APPLY_DEX)->modifier==-2);
 final_batch_affect(15,&c,&c,SPELL_RIGHTEOUS_MIGHT,0);assert(GET_DR(&c)->amount==10&&!GET_DR(&c)->next);
 cp.saved.channel_energy_type=CHANNEL_ENERGY_TYPE_NEGATIVE;final_batch_affect(15,&c,&c,SPELL_RIGHTEOUS_MIGHT,0);assert(GET_DR(&c)->bypass_val[0]==DR_ALIGNTYPE_GOOD);clear(&c);
 af.spell=SPELL_ENLARGE_PERSON;af.location=APPLY_SIZE;affect_join(&c,&af,0,0,0,0);final_batch_affect(15,&c,&c,SPELL_RIGHTEOUS_MIGHT,0);assert(!affected_by_spell(&c,SPELL_RIGHTEOUS_MIGHT));clear(&c);
 for(i=0;i<4;i++){
 c.spell_fervor_choice=i;final_batch_affect(7,&c,&v,SPELL_BLESSING_OF_FERVOR,0);assert(final_batch_fervor(&v,i));
 if(i==FERVOR_DEFENSE)assert(final_batch_bonus(&v,APPLY_HITROLL)==2&&final_batch_bonus(&v,APPLY_AC_NEW)==2&&final_batch_bonus(&v,APPLY_SAVING_REFL)==2);
 if(i==FERVOR_SPEED)assert(final_batch_bonus(&v,APPLY_MOVE_SPEED)==30);
 SET_BIT_AR(AFF_FLAGS(&v),AFF_HASTE);assert(!final_batch_fervor(&v,i)&&!final_batch_bonus(&v,APPLY_MOVE_SPEED));REMOVE_BIT_AR(AFF_FLAGS(&v),AFF_HASTE);clear(&v);
 }
 GET_AUGMENT_PSP(&c)=8;
 for(i=0;i<4;i++){c.spell_battlesense_choice=i;final_batch_affect(9,&c,&v,PSIONIC_BATTLESENSE,0);assert(effect(&v,PSIONIC_BATTLESENSE,APPLY_SPECIAL)->modifier==5);
 assert(final_batch_bonus(&v, i==0?APPLY_HITROLL:i==1?APPLY_AC_NEW:i==2?APPLY_DAMROLL:APPLY_MOVE_SPEED)==(i==2?10:i==3?25:5));clear(&v);}
 immune=1;final_batch_affect(9,&c,&v,PSIONIC_BATTLESENSE,0);assert(!v.affected);immune=0;
 final_batch_affect(8,&c,&c,SPELL_DAZZLING_BLADE,0);assert(!c.affected);
 GET_ID(&sword)=99;GET_OBJ_TYPE(&sword)=ITEM_WEAPON;GET_OBJ_MATERIAL(&sword)=MATERIAL_IRON;GET_EQ(&c,WEAR_WIELD_1)=&sword;
 final_batch_affect(12,&c,&c,SPELL_DAZZLING_BLADE,0);assert(final_batch_blade_bonus(&c)==5);GET_EQ(&c,WEAR_WIELD_1)=NULL;assert(!final_batch_blade_bonus(&c));GET_EQ(&c,WEAR_WIELD_1)=&sword;
 target_lookup=&v;save_result=0;do_spellchoice(&c,"blade enemy",0,0);assert(!affected_by_spell(&c,SPELL_DAZZLING_BLADE));assert(IS_SET_AR(effect(&v,SPELL_DAZZLING_BLADE_FLASH,APPLY_NONE)->bitvector,AFF_BLIND));clear(&v);
 final_batch_affect(1,&c,&c,SPELL_DAZZLING_BLADE,0);save_result=1;do_spellchoice(&c,"blade enemy",0,0);assert(IS_SET_AR(effect(&v,SPELL_DAZZLING_BLADE_FLASH,APPLY_NONE)->bitvector,AFF_DAZZLED));clear(&v);save_result=0;
 final_batch_affect(10,&c,&c,SPELL_THORN_BODY,0);assert(final_batch_natural_hit(&c,&v,ATTACK_TYPE_UNARMED)==6);assert(final_batch_thorns(&v,&c,ATTACK_TYPE_UNARMED)==16);
 assert(final_batch_thorns(&v,&c,ATTACK_TYPE_RANGED)==0);GRAPPLE_ATTACKER(&c)=&v;assert(final_batch_thorns(&v,&c,ATTACK_TYPE_UNARMED)==22);GRAPPLE_ATTACKER(&c)=NULL;
 GET_EQ(&v,WEAR_WIELD_1)=&sword;weapon_list[GET_WEAPON_TYPE(&sword)].weaponFlags=WEAPON_FLAG_REACH;assert(!final_batch_thorns(&v,&c,ATTACK_TYPE_PRIMARY));weapon_list[GET_WEAPON_TYPE(&sword)].weaponFlags=0;GET_EQ(&v,WEAR_WIELD_1)=NULL;clear(&c);
 final_batch_affect(4,&c,&c,SPELL_VINE_STRIKE,0);
 SET_BIT_AR(AFF_FLAGS(&v),AFF_FREE_MOVEMENT);assert(final_batch_natural_hit(&c,&v,ATTACK_TYPE_UNARMED)==6&&!v.affected);REMOVE_BIT_AR(AFF_FLAGS(&v),AFF_FREE_MOVEMENT);final_batch_round();assert(final_batch_natural_hit(&c,&v,ATTACK_TYPE_PRIMARY)==0);assert(final_batch_natural_hit(&c,&v,ATTACK_TYPE_UNARMED)==6&&affected_by_spell(&v,SPELL_VINE_STRIKE_BIND));assert(!final_batch_natural_hit(&c,&v,ATTACK_TYPE_UNARMED));
 assert(final_batch_struggle(&v)&&!v.affected);final_batch_round();save_result=1;assert(final_batch_natural_hit(&c,&v,ATTACK_TYPE_UNARMED)==6&&affected_by_spell(&v,SPELL_VINE_STRIKE_RESIST));clear(&v);clear(&c);save_result=0;
 resist_result=1;final_batch_affect(7,&c,&v,SPELL_DEBILITATING_PORTENT,0);assert(!v.affected);resist_result=0;
 final_batch_affect(7,&c,&v,SPELL_DEBILITATING_PORTENT,0);assert(final_batch_outgoing(&v,20,SPELL_FIREBALL)==10);save_result=1;assert(final_batch_outgoing(&v,20,SPELL_FIREBALL)==20);save_result=0;
 target_lookup=&v;do_spellchoice(&ally,"portent enemy",0,0);assert(!final_batch_cancel_critical(&v));do_spellchoice(&c,"portent enemy",0,0);assert(final_batch_cancel_critical(&v)&&!v.affected);assert(final_batch_outgoing(&v,20,TYPE_HIT)==10&&!v.spell_portent_half_critical);
 GET_AUGMENT_PSP(&c)=4;c.spell_bonds_size_choice=0;v.points.size=SIZE_LARGE;
 final_batch_affect(7,&c,&v,PSIONIC_CRYSTALLINE_BONDS,0);assert(!v.affected);c.spell_bonds_size_choice=1;
 final_batch_affect(7,&c,&v,PSIONIC_CRYSTALLINE_BONDS,0);assert(v.affected&&last_damage==18&&effect(&v,PSIONIC_CRYSTALLINE_BONDS,APPLY_SPECIAL)->modifier==1);clear(&v);
 v.points.size=SIZE_MEDIUM;c.spell_bonds_size_choice=0;save_result=1;final_batch_affect(7,&c,&v,PSIONIC_CRYSTALLINE_BONDS,0);assert(!v.affected);save_result=0;resist_result=1;
 SET_BIT_AR(AFF_FLAGS(&v),AFF_FREE_MOVEMENT);final_batch_affect(7,&c,&v,PSIONIC_CRYSTALLINE_BONDS,0);assert(!v.affected);REMOVE_BIT_AR(AFF_FLAGS(&v),AFF_FREE_MOVEMENT);
 final_batch_affect(7,&c,&v,PSIONIC_CRYSTALLINE_BONDS,0);assert(v.affected&&last_damage==18);resist_result=0;
 assert(IS_SET_AR(effect(&v,PSIONIC_CRYSTALLINE_BONDS,APPLY_SPECIAL)->bitvector,AFF_ENTANGLED)&&IS_SET_AR(effect(&v,PSIONIC_CRYSTALLINE_BONDS,APPLY_SPECIAL)->bitvector,AFF_SILENCED));
 damage_calls=0;final_batch_round();assert(damage_calls==1&&last_damage==18&&final_batch_bonds_dc(&v)==29);
 SET_BIT_AR(rooms[0].room_flags,ROOM_PEACEFUL);final_batch_round();assert(damage_calls==1);REMOVE_BIT_AR(rooms[0].room_flags,ROOM_PEACEFUL);
 exclude_aoe=1;final_batch_round();assert(damage_calls==1);exclude_aoe=0;
 IN_ROOM(&c)=1;final_batch_round();assert(damage_calls==1);IN_ROOM(&c)=0;
 action_available=0;assert(final_batch_struggle(&v)&&v.affected);action_available=1;roll=1;escape_bonus=0;assert(final_batch_struggle(&v)&&v.affected);roll=20;escape_bonus=20;assert(final_batch_struggle(&v)&&!v.affected);
 final_batch_affect(7,&c,&v,PSIONIC_CRYSTALLINE_BONDS,0);kill_on_damage=1;final_batch_round();assert(!find_char(12));kill_on_damage=0;c.next=&v;character_list=&c;
 do_spellchoice(&c,"fervor speed",0,0);assert(c.spell_fervor_choice==FERVOR_SPEED);do_spellchoice(&c,"battlesense armor",0,0);assert(c.spell_battlesense_choice==BATTLESENSE_ARMOR);do_spellchoice(&c,"bonds size",0,0);assert(c.spell_bonds_size_choice);
 clear(&c);clear(&v);clear(&ally);
 puts("PASS: final batch native DR pools, size exclusivity, group modes/Haste, weapon binding/discharge, natural-hit and reach/grapple hooks, save protection/escape, portent arming, bonds augmentation/concentration, periodic protections and extraction safety");
 return 0;
}
