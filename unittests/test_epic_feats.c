/* Component tests use the production epic module and production queued PSP
 * payment code; only the surrounding MUD services are replaced. */
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "handler.h"
#include "spells.h"
#include "spell_prep.h"
#include "psionics.h"
#include "feats.h"
#include "mud_event.h"
#include "epic_feats.h"
#include "epic_feats2.h"
#include "epic_feats3.h"
#include "evolutions.h"
#include "alchemy.h"
#include "legendary_techniques.h"
#include "legendary_identity.h"
#include <assert.h>

struct config_data config_info;
struct player_special_data dummy_mob;
int dice(int n,int size) { return n*size; }
int MAX(int a,int b) { return a>b?a:b; }
int MIN(int a,int b) { return a<b?a:b; }
void basic_mud_log(const char *format, ...) { (void)format; }
struct spell_info_type spell_info[TOP_SPELL_DEFINE + 1];
struct psionic_power_data psionic_powers[TOP_SPELL_DEFINE + 1];
static struct char_data pc;
static struct char_data *test_pc = &pc;
static struct player_special_data specials;
static struct descriptor_data descriptor;
static struct level_data levelup;
static int saves, roll_value, rolls, legendary_discount, consumed;
struct index_data *mob_index;
mob_rnum top_of_mobt = -1;
struct room_data *world;
struct char_data *character_list;
static struct room_data test_rooms[1];
static bool twin, known, save_result;
static int save_attempts;
struct mud_event_data *char_has_mud_event(struct char_data *ch,event_id id)
{ (void)ch;return twin && id==ePURGEMOB?(struct mud_event_data *)1:NULL; }
bool is_summon_creature_mob(int vnum) { return vnum==123; }
void new_affect(struct affected_type *af) { memset(af,0,sizeof(*af)); }
void affect_to_char(struct char_data *ch,struct affected_type *af)
{
  struct affected_type *copy=malloc(sizeof(*copy));assert(copy);
  *copy=*af;copy->next=ch->affected;ch->affected=copy;
}


size_t send_to_char(struct char_data *ch, const char *format, ...) { (void)ch; (void)format; return 0; }
void save_char(struct char_data *ch, int mode) { (void)ch; (void)mode; ++saves; }
int d20(struct char_data *ch) { (void)ch; ++rolls; return roll_value; }
int has_feat_requirement_check(struct char_data *ch, int feat)
{
  return HAS_REAL_FEAT(ch,feat) + (ch->desc && LEVELUP(ch) ? LEVELUP(ch)->feats[feat] : 0);
}
int get_class_highest_circle(struct char_data *ch, int class)
{
  int cap=class==CLASS_PSIONICIST?9:epic_capacity_circle(class);
  return CLASS_LEVEL(ch,class)>=20?cap:0;
}
bool is_a_known_spell(struct char_data *ch,int class,int spell)
{ (void)ch;(void)class;(void)spell;return known; }
int compute_powers_circle(int class,int spell,int metamagic)
{ (void)class;(void)spell;(void)metamagic;return 1; }
int bloodline_affect_bonus(struct char_data *ch,int spell)
{ (void)ch;(void)spell;return 0; }
bool is_fear_spell(int spell) { return spell==SPELL_FEAR; }
bool affected_by_spell(struct char_data *ch,int spell)
{ (void)ch;return spell==SKILL_RAGE; }

bool legendary_uuid_generate(char out[33])
{ memset(out,'a',32);out[32]=0;return TRUE; }
bool legendary_effect_peek(struct char_data *ch,int key,struct legendary_activation *a,
                           struct legendary_effect_spec *spec)
{
  (void)ch;(void)spec;
  if (key!=LEG_T16 || !legendary_discount) return FALSE;
  memset(a,0,sizeof(*a));a->resolved.values.resource_discount=legendary_discount;return TRUE;
}
int legendary_budget_available(struct char_data *ch,int kind)
{ (void)ch;(void)kind;return 20; }
bool legendary_effect_consume(struct char_data *ch,int key)
{ (void)ch;(void)key;++consumed;return TRUE; }
int legendary_resource_pay(struct char_data *ch,int cost,int discount)
{
  int paid=MAX(1,cost-discount);
  if (GET_PSP(ch)<paid) return -1;
  GET_PSP(ch)-=paid;return paid;
}

int savingthrow_flatdc(struct char_data *ch,struct char_data *vict,int type,int mod,int dc,int cast,int school,int spell)
{(void)ch;(void)vict;(void)type;(void)mod;(void)cast;(void)school;(void)spell;assert(dc==20);save_attempts++;return save_result;}
void affect_from_char(struct char_data *ch,int spell)
{struct affected_type **at=&ch->affected,*af;while((af=*at)) {if(af->spell==spell){*at=af->next;free(af);}else at=&af->next;}}

static int random_value=45;
int rand_number(int low,int high) {(void)low;(void)high;return random_value;}
int compute_caster_level(struct char_data *ch) {(void)ch;return 30;}
int is_player_grouped(struct char_data *a,struct char_data *b) {return a==b || (a->group && a->group==b->group);}
const char *act(const char *s,int hide,struct char_data *ch,struct obj_data *obj,void *vict,int type)
{(void)s;(void)hide;(void)ch;(void)obj;(void)vict;(void)type;return "";}
void affect_remove(struct char_data *ch,struct affected_type *which)
{struct affected_type **at=&ch->affected;while(*at && *at!=which)at=&(*at)->next;if(*at){*at=which->next;free(which);}}

static bool pet_save_ok=TRUE;
static int manifested, manifested_spell, manifested_level;
bool save_char_pets_checked(struct char_data *ch) {(void)ch;return pet_save_ok;}
int legendary_temporary_remaining(struct char_data *ch) {(void)ch;return 0;}
void legendary_temporary_clear(struct char_data *ch) {(void)ch;}
void mag_affects(int level,struct char_data *ch,struct char_data *vict,struct obj_data *obj,int spell,int save,int cast,int meta)
{
  struct affected_type af={0};(void)obj;(void)save;(void)meta;
  assert(cast==CAST_INNATE && GET_AUGMENT_PSP(ch)==0);manifested++;manifested_spell=spell;manifested_level=level;
  af.spell=spell;af.location=APPLY_AC_NEW;af.duration=20;affect_to_char(vict,&af);
}

static int reactions,reaction_spell;
bool has_blindsight(struct char_data *ch) {(void)ch;return FALSE;}
bool has_true_sight(struct char_data *ch) {(void)ch;return FALSE;}
bool can_see_hidden(struct char_data *ch,struct char_data *vict) {(void)ch;(void)vict;return TRUE;}
bool char_has_infra(struct char_data *ch) {(void)ch;return FALSE;}
bool char_has_ultra(struct char_data *ch) {(void)ch;return FALSE;}
bool is_outdoors(struct char_data *ch) {(void)ch;return TRUE;}
bool room_is_dark(room_rnum room) {(void)room;return FALSE;}
bool is_spellnum_psionic(int spell) {return spell>=PSIONIC_POWER_START && spell<=PSIONIC_POWER_END;}
int aoeOK(struct char_data *ch,struct char_data *vict,int spell) {(void)ch;(void)vict;(void)spell;return TRUE;}
int practiced_spellcaster_level(struct char_data *ch,int cl,int level) {(void)ch;(void)cl;return level;}
struct char_data *find_char(long id)
{struct char_data *ch;for(ch=character_list;ch;ch=ch->next)if(GET_ID(ch)==id)return ch;return NULL;}
int call_magic(struct char_data *ch,struct char_data *vict,struct obj_data *obj,int spell,int meta,int level,int cast)
{(void)vict;(void)obj;(void)meta;(void)level;assert(cast==CAST_SPELL && ch->epic_reaction_depth==1 && CASTING_SPELLNUM(ch)==spell && CASTING_METAMAGIC(ch)==0);reactions++;reaction_spell=spell;return 1;}
void affect_join(struct char_data *ch,struct affected_type *af,bool a,bool b,bool c,bool d)
{(void)a;(void)b;(void)c;(void)d;affect_to_char(ch,af);}

static void fixture(void)
{
  int i;
  memset(&pc,0,sizeof(pc));memset(&specials,0,sizeof(specials));
  memset(&descriptor,0,sizeof(descriptor));memset(&levelup,0,sizeof(levelup));
  pc.player_specials=&specials;GET_LEVEL(test_pc)=30;
  GET_REAL_INT(test_pc)=GET_REAL_WIS(test_pc)=GET_REAL_CON(test_pc)=30;
  GET_REAL_STR(test_pc)=GET_REAL_DEX(test_pc)=GET_REAL_CHA(test_pc)=30;
  for(i=0;i<NUM_CLASSES;i++) CLASS_LEVEL(test_pc,i)=30;
  for(i=1;i<NUM_FEATS;i++) HAS_REAL_FEAT(test_pc,i)=1;
  for(i=FEAT_EPIC_POWER_KNOWLEDGE;i<=FEAT_RUINOUS_RAGE;i++) HAS_REAL_FEAT(test_pc,i)=0;
  for(i=1;i<=MAX_ABILITIES;i++) GET_ABILITY(test_pc,i)=30;
  config_info.extra.crafting_system=CRAFTING_SYSTEM_MOTES;
  saves=rolls=legendary_discount=consumed=0;roll_value=16;
  twin=known=save_result=FALSE;save_attempts=0;world=test_rooms;memset(test_rooms,0,sizeof(test_rooms));
}

static void eligibility_and_ranks(void)
{
  int i;
  fixture();
  for(i=FEAT_EPIC_POWER_KNOWLEDGE;i<=FEAT_RUINOUS_RAGE;i++)
  {
    assert(epic_feat_available(&pc,i));
    assert(!epic_feat_rank(&pc,i));
    HAS_REAL_FEAT(test_pc,i)=1;
    assert(epic_feat_rank(&pc,i)==1);
    if(i!=FEAT_EPIC_POWER_KNOWLEDGE && i!=FEAT_EPIC_EVOLUTION_RESERVE && i!=FEAT_IMPROVED_SPELL_CAPACITY)
      assert(!epic_feat_available(&pc,i));
    HAS_REAL_FEAT(test_pc,i)=0;
  }
  GET_LEVEL(test_pc)=20;assert(!epic_feat_available(&pc,FEAT_EPIC_FORTITUDE));
  GET_LEVEL(test_pc)=21;HAS_REAL_FEAT(test_pc,FEAT_EPIC_POWER_KNOWLEDGE)=1;
  assert(!epic_feat_available(&pc,FEAT_EPIC_POWER_KNOWLEDGE));
  GET_LEVEL(test_pc)=24;assert(epic_feat_available(&pc,FEAT_EPIC_POWER_KNOWLEDGE));
  HAS_REAL_FEAT(test_pc,FEAT_EPIC_POWER_KNOWLEDGE)=2;
  assert(!epic_feat_available(&pc,FEAT_EPIC_POWER_KNOWLEDGE));
  GET_LEVEL(test_pc)=27;assert(epic_feat_available(&pc,FEAT_EPIC_POWER_KNOWLEDGE));
  HAS_REAL_FEAT(test_pc,FEAT_EPIC_POWER_KNOWLEDGE)=3;
  assert(!epic_feat_available(&pc,FEAT_EPIC_POWER_KNOWLEDGE));
  GET_LEVEL(test_pc)=26;HAS_REAL_FEAT(test_pc,FEAT_EPIC_EVOLUTION_RESERVE)=1;
  assert(!epic_feat_available(&pc,FEAT_EPIC_EVOLUTION_RESERVE));
  GET_LEVEL(test_pc)=27;assert(epic_feat_available(&pc,FEAT_EPIC_EVOLUTION_RESERVE));
  HAS_REAL_FEAT(test_pc,FEAT_EPIC_EVOLUTION_RESERVE)=2;
  assert(!epic_feat_available(&pc,FEAT_EPIC_EVOLUTION_RESERVE));
  HAS_REAL_FEAT(test_pc,FEAT_EPIC_FORTITUDE)=1;GET_REAL_CON(test_pc)=20;
  assert(!epic_feat_rank(&pc,FEAT_EPIC_FORTITUDE));
  GET_REAL_CON(test_pc)=21;assert(epic_feat_rank(&pc,FEAT_EPIC_FORTITUDE)==1);
  pc.desc=&descriptor;LEVELUP(test_pc)=&levelup;
  levelup.feats[FEAT_EPIC_EVOLUTION_RESERVE]=1;
  HAS_REAL_FEAT(test_pc,FEAT_EPIC_EVOLUTION_RESERVE)=0;
  assert(epic_study_rank(&pc,FEAT_EPIC_EVOLUTION_RESERVE)==1);
  assert(!epic_feat_rank(&pc,FEAT_EPIC_EVOLUTION_RESERVE));
}

static void crafting_and_charges(void)
{
  fixture();HAS_REAL_FEAT(test_pc,FEAT_EPIC_ARTISANS_INSIGHT)=1;
  EPIC_TIMERS(test_pc).armed[0]=time(NULL)+30;
  assert(epic_artisan_roll(&pc,12,20,30,TRUE)==12);assert(rolls==0);
  assert(epic_artisan_roll(&pc,1,20,30,TRUE)==16);assert(rolls==1);
  assert(!EPIC_TIMERS(test_pc).armed[0]);assert(saves==1);
  assert(epic_artisan_roll(&pc,1,20,30,TRUE)==1);assert(rolls==1);
  EPIC_TIMERS(test_pc).armed[0]=time(NULL)-1;
  assert(epic_artisan_roll(&pc,1,20,30,TRUE)==1);
  EPIC_TIMERS(test_pc).armed[0]=time(NULL)+30;
  assert(epic_artisan_roll(&pc,20,0,30,FALSE)==16); /* golem has no natural-20 auto success */
  HAS_REAL_FEAT(test_pc,FEAT_EPIC_EFFICIENT_ITEM_CREATION)=1;EPIC_CHOICES(test_pc).craft_category=1;
  assert(epic_craft_seconds(&pc,100,0,1,TRUE)==70);
  assert(epic_craft_seconds(&pc,100,80,1,TRUE)==50);
  assert(epic_craft_seconds(&pc,11,0,1,FALSE)==9);
  assert(epic_craft_seconds(&pc,100,0,2,TRUE)==90);
  assert(epic_craft_seconds(&pc,100,0,0,FALSE)==100);
  HAS_REAL_FEAT(test_pc,FEAT_CRAFT_WONDEROUS_ITEM)=0;
  assert(epic_craft_seconds(&pc,100,0,1,FALSE)==100);
  HAS_REAL_FEAT(test_pc,FEAT_EPIC_METAMAGIC_SCIENCE)=1;EPIC_TIMERS(test_pc).armed[1]=time(NULL)+18;
  assert(epic_trigger_cost(&pc,1,FALSE)==1);assert(epic_trigger_cost(&pc,4,FALSE)==3);
  assert(EPIC_TIMERS(test_pc).armed[1]>0);assert(epic_trigger_cost(&pc,4,TRUE)==3);
  assert(epic_trigger_cost(&pc,4,TRUE)==4);
  HAS_REAL_FEAT(test_pc,FEAT_EPIC_DEVICE_PENETRATION)=1;EPIC_TIMERS(test_pc).armed[2]=time(NULL)+18;
  epic_device_begin(&pc);assert(!EPIC_TIMERS(test_pc).armed[2]);
  pc.epic_spell_depth=pc.epic_device_depth;pc.epic_cast_context=CAST_DEVICE;
  assert(epic_device_penetration(&pc)==4);assert(epic_device_penetration(&pc)==0);
  epic_device_end(&pc);assert(!pc.epic_device_check);
}

static void power_cost_and_choices(void)
{
  fixture();HAS_REAL_FEAT(test_pc,FEAT_EPIC_AUGMENTATION_EFFICIENCY)=1;
  assert(epic_augmentation_discount(&pc,PSIONIC_POWER_START,0)==0);
  assert(epic_augmentation_discount(&pc,PSIONIC_POWER_START,1)==1);
  assert(epic_augmentation_discount(&pc,PSIONIC_POWER_START,6)==2);
  assert(!epic_augmentation_discount(&pc,PSIONIC_IMPALE_MIND,6));
  GET_AUGMENT_PSP(test_pc)=6;GET_PSP(test_pc)=20;
  assert(legendary_cast_queue(&pc,PSIONIC_POWER_START,13));
  assert(legendary_cast_commit(&pc,PSIONIC_POWER_START));assert(GET_PSP(test_pc)==9);
  assert(legendary_cast_commit(&pc,PSIONIC_POWER_START));assert(GET_PSP(test_pc)==9);
  legendary_cast_clear(&pc);
  legendary_discount=5;GET_PSP(test_pc)=30;
  assert(legendary_cast_queue(&pc,PSIONIC_POWER_START,25));
  assert(legendary_cast_commit(&pc,PSIONIC_POWER_START));assert(GET_PSP(test_pc)==10);
  assert(consumed==1); /* five wins, never seven */
  legendary_cast_clear(&pc);
  GET_PSP(test_pc)=10;legendary_discount=0;
  assert(legendary_cast_queue(&pc,PSIONIC_POWER_START,13));
  assert(!legendary_cast_commit(&pc,PSIONIC_POWER_START));assert(GET_PSP(test_pc)==10);
  legendary_cast_clear(&pc);
  assert(epic_capacity_choice(&pc,CLASS_SUMMONER));assert(epic_capacity_circle(CLASS_SUMMONER)==6);
  assert(!epic_capacity_choice(&pc,CLASS_PSIONICIST));assert(!epic_capacity_choice(&pc,CLASS_ARTIFICER));
  psionic_powers[PSIONIC_POWER_START].psp_cost=1;
  assert(epic_power_choice(&pc,PSIONIC_POWER_START));
  assert(!epic_power_choice(&pc,PSIONIC_IMPALE_MIND));
  assert(!epic_power_choice(&pc,-1));
  pc.desc=&descriptor;LEVELUP(test_pc)=&levelup;
  levelup.epic_choices.powers[0]=PSIONIC_POWER_START;
  assert(!epic_power_choice(&pc,PSIONIC_POWER_START));
}

static void narrow_save_bonus(void)
{
  struct affected_type af;
  fixture();HAS_REAL_FEAT(test_pc,FEAT_EPIC_MENTAL_BASTION)=1;
  assert(epic_mind_save_bonus(&pc,SAVING_WILL,SPELL_CHARM)==4);
  assert(!epic_mind_save_bonus(&pc,SAVING_WILL,SPELL_HOLD_PERSON));
  memset(&af,0,sizeof(af));af.location=APPLY_SAVING_WILL;af.bonus_type=BONUS_TYPE_INSIGHT;
  af.modifier=3;pc.affected=&af;
  assert(epic_mind_save_bonus(&pc,SAVING_WILL,SPELL_CHARM)==1);
  af.modifier=6;assert(!epic_mind_save_bonus(&pc,SAVING_WILL,SPELL_FEAR));
  pc.affected=NULL;specials.saved.flash_insight_bonus=5;
  specials.saved.flash_insight_expires=time(NULL)+30;
  assert(!epic_mind_save_bonus(&pc,SAVING_WILL,SPELL_CHARM));
  specials.saved.flash_insight_survival_only=TRUE;
  assert(epic_mind_save_bonus(&pc,SAVING_WILL,SPELL_CHARM)==4);
  HAS_REAL_FEAT(test_pc,FEAT_RUINOUS_RAGE)=1;
  assert(epic_rage_penetration(&pc,ATTACK_TYPE_PRIMARY)==5);
  assert(!epic_rage_penetration(&pc,ATTACK_TYPE_RANGED));
  assert(!epic_rage_penetration(&pc,ATTACK_TYPE_PSIONICS));
}

static void companion_ward(void)
{
  struct char_data pet, *p=&pet;
  struct follow_type follower;
  struct affected_type *af;
  time_t expiry;
  fixture();memset(&pet,0,sizeof(pet));memset(&follower,0,sizeof(follower));
  pet.player_specials=&dummy_mob;pet.master=&pc;pet.nr=NOBODY;
  SET_BIT_AR(MOB_FLAGS(p),MOB_ISNPC);SET_BIT_AR(MOB_FLAGS(p),MOB_EIDOLON);
  SET_BIT_AR(AFF_FLAGS(p),AFF_CHARM);pet.in_room=0;
  follower.follower=&pet;pc.followers=&follower;
  HAS_REAL_FEAT(test_pc,FEAT_EPIC_ADAPTIVE_EIDOLON_WARD)=1;
  HAS_REAL_FEAT(test_pc,FEAT_EPIC_PLANAR_ANCHORAGE)=1;
  assert(epic_planar_save_bonus(&pet,SAVING_WILL,SPELL_CHARM)==4);
  assert(!epic_planar_save_bonus(&pet,SAVING_WILL,SPELL_FEAR));
  EPIC_TIMERS(test_pc).ward_expiry=expiry=time(NULL)+30;
  EPIC_TIMERS(test_pc).ward_type=DAM_FIRE;
  epic_restore_ward(&pc,&pet);
  assert(pet.affected && pet.affected->legendary_expiry==expiry);
  assert(epic_eidolon_resistance(&pet,DAM_FIRE)==20);
  assert(!epic_eidolon_resistance(&pet,DAM_COLD));
  twin=TRUE;assert(!epic_eidolon_resistance(&pet,DAM_FIRE));twin=FALSE;
  epic_snapshot_ward(&pc);assert(EPIC_TIMERS(test_pc).ward_expiry==expiry);
  af=pet.affected;pet.affected=NULL;free(af); /* native dispel removed the affect */
  epic_snapshot_ward(&pc);assert(!EPIC_TIMERS(test_pc).ward_expiry);
  EPIC_TIMERS(test_pc).ward_expiry=time(NULL)-1;
  epic_restore_ward(&pc,&pet);assert(!pet.affected);
  SET_BIT_AR(MOB_FLAGS(p),MOB_GOLEM);
  HAS_REAL_FEAT(test_pc,FEAT_EPIC_ANCHORED_CHASSIS)=1;
  assert(epic_chassis_bonus(&pet,COMBAT_MANEUVER_TYPE_KNOCKDOWN)==4);
  assert(epic_chassis_bonus(&pet,COMBAT_MANEUVER_TYPE_FORCED_MOVEMENT)==4);
  assert(!epic_chassis_bonus(&pet,COMBAT_MANEUVER_TYPE_GRAPPLE));
}

static void second_batch(void)
{
  struct char_data pet,*p=&pet;
  struct follow_type f={0};
  struct affected_type af={0};
  struct epic_second_attack attack;
  struct obj_data weapon={0};
  int i;
  fixture();known=TRUE;GET_ALIGNMENT(test_pc)=1000;
  for(i=FEAT_EPIC_RESISTANCE_REBUTTAL;i<=FEAT_EPIC_AUGMENTED_ALCHEMY;i++)
  {HAS_REAL_FEAT(test_pc,i)=0;assert(epic_feat_available(&pc,i));HAS_REAL_FEAT(test_pc,i)=1;assert(epic_feat_rank(&pc,i)==1);}
  HAS_REAL_FEAT(test_pc,FEAT_GREAT_SMITING)=0;assert(epic_feat_available(&pc,FEAT_GREAT_SMITING));
  HAS_REAL_FEAT(test_pc,FEAT_GREAT_SMITING)=1;assert(!epic_feat_available(&pc,FEAT_GREAT_SMITING));
  GET_LEVEL(test_pc)=26;assert(!epic_feat_available(&pc,FEAT_EPIC_INSPIRATION));
  GET_LEVEL(test_pc)=27;CLASS_LEVEL(test_pc,CLASS_BARD)=22;assert(!epic_feat_available(&pc,FEAT_EPIC_INSPIRATION));
  CLASS_LEVEL(test_pc,CLASS_BARD)=23;assert(epic_feat_available(&pc,FEAT_EPIC_INSPIRATION));
  HAS_REAL_FEAT(test_pc,FEAT_EPIC_INSPIRATION)=2;assert(epic_feat_rank(&pc,FEAT_EPIC_INSPIRATION)==2);
  EPIC_SECOND(test_pc).rebuttal=time(NULL)+18;pc.epic_cast_context=CAST_SPELL;
  pc.epic_spell_context=PSIONIC_INERTIAL_ARMOR;pc.epic_spell_depth=1;
  assert(epic_rebuttal_roll(&pc,0,3)==3 && rolls==0);
  pc.epic_spell_depth=2;assert(epic_rebuttal_roll(&pc,30,3)==3 && rolls==0);
  pc.epic_spell_depth=1;assert(epic_rebuttal_roll(&pc,30,3)==16 && rolls==1);
  assert(!EPIC_SECOND(test_pc).rebuttal);assert(epic_rebuttal_roll(&pc,30,3)==3);
  memset(&pet,0,sizeof(pet));pet.player_specials=&dummy_mob;pet.master=&pc;
  SET_BIT_AR(MOB_FLAGS(p),MOB_ISNPC);SET_BIT_AR(AFF_FLAGS(p),AFF_CHARM);
  SET_BIT_AR(MOB_FLAGS(p),MOB_GOLEM);GET_HIT(p)=100;pet.in_room=pc.in_room=0;
  strcpy(pet.epic_pet_uuid,"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
  strcpy(EPIC_SECOND(test_pc).golem_uuid,pet.epic_pet_uuid);
  f.follower=&pet;pc.followers=&f;
  EPIC_SECOND(test_pc).aegis_expiry=time(NULL)+18;EPIC_SECOND(test_pc).aegis_hp=30;
  assert(epic_temporary_absorb(&pc,12)==0 && EPIC_SECOND(test_pc).aegis_hp==18);
  assert(epic_temporary_absorb(&pc,25)==7 && !EPIC_SECOND(test_pc).aegis_hp && GET_HIT(p)==100);
  EPIC_SECOND(test_pc).aegis_hp=30;pet.in_room=1;
  assert(epic_temporary_absorb(&pc,12)==12 && !EPIC_SECOND(test_pc).aegis_hp);pet.in_room=0;
  af.spell=AFFECT_EPIC_SHARED_VIGOR;af.modifier=5;af.legendary_expiry=time(NULL)+60;
  affect_to_char(&pet,&af);assert(epic_temporary_absorb(&pet,3)==0);
  assert(epic_temporary_absorb(&pet,3)==1);assert(GET_HIT(p)==100);
  affect_from_char(&pet,AFFECT_EPIC_SHARED_VIGOR);
  EPIC_SECOND(test_pc).bomb_slot=2;EPIC_SECOND(test_pc).bomb_expiry=time(NULL)+18;
  assert(!epic_bomb_consume(&pc,0));assert(EPIC_SECOND(test_pc).bomb_slot==2);
  assert(epic_bomb_consume(&pc,1)==24);assert(!EPIC_SECOND(test_pc).bomb_slot);
  EPIC_SECOND(test_pc).bomb_slot=2;epic_bomb_changed(&pc,1);assert(!EPIC_SECOND(test_pc).bomb_slot);
  epic_summon_created(&pc,&pet,SPELL_ANIMATE_DEAD,60);assert(!pet.epic_summon_expiry);
  EPIC_CHOICES(test_pc).summon_doctrine=2;epic_summon_created(&pc,&pet,SPELL_SUMMON_CREATURE_5,60);
  assert(epic_summon_doctrine(&pet,2));pet.epic_summon_expiry=time(NULL)-1;assert(!epic_summon_doctrine(&pet,2));
  twin=TRUE;pet.epic_summon_expiry=time(NULL)+60;
  strcpy(EPIC_SECOND(test_pc).command_uuid,pet.epic_pet_uuid);EPIC_SECOND(test_pc).command_expiry=time(NULL)+12;
  pet.epic_attack_depth=1;
  assert(epic_second_attack_begin(&pet,TYPE_UNDEFINED,ATTACK_TYPE_PRIMARY,&attack)==4);
  assert(attack.command_damage==8 && !EPIC_SECOND(test_pc).command_expiry);
  assert(!epic_second_attack_begin(&pet,TYPE_UNDEFINED,ATTACK_TYPE_PRIMARY,&attack) && !attack.command_damage);
  twin=FALSE;REMOVE_BIT_AR(MOB_FLAGS(p),MOB_GOLEM);SET_BIT_AR(MOB_FLAGS(p),MOB_EIDOLON);
  GET_POS(test_pc)=POS_STANDING;GET_HIT(test_pc)=100;pet.epic_lethal_packet=TRUE;
  assert(!epic_resilient_bond(&pet));assert(save_attempts==1 && EPIC_SECOND(test_pc).ready[E2_BOND]>time(NULL));
  save_result=TRUE;assert(!epic_resilient_bond(&pet) && save_attempts==1);
  EPIC_SECOND(test_pc).ready[E2_BOND]=0;twin=TRUE;assert(!epic_resilient_bond(&pet) && save_attempts==1);
  twin=FALSE;pet.epic_lethal_packet=FALSE;assert(!epic_resilient_bond(&pet));
  pet.epic_lethal_packet=TRUE;assert(epic_resilient_bond(&pet) && save_attempts==2);
  strcpy(weapon.epic_binding_id,"bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb");strcpy(EPIC_SECOND(test_pc).weapon_id,weapon.epic_binding_id);
  EPIC_SECOND(test_pc).infusion_expiry=time(NULL)+18;pc.legendary_serializing=TRUE;
  epic_infusion_unwield(&pc,&weapon);assert(EPIC_SECOND(test_pc).infusion_expiry);
  pc.legendary_serializing=FALSE;epic_infusion_unwield(&pc,&weapon);assert(!EPIC_SECOND(test_pc).infusion_expiry);
}

static void third_batch(void)
{
  struct char_data other={0};
  struct affected_type af={0},*a;
  struct obj_data weapon={0};
  int i;
  fixture();known=TRUE;GET_POS(test_pc)=POS_STANDING;GET_HIT(test_pc)=100;
  GET_IDNUM(test_pc)=123;other.player_specials=&dummy_mob;other.in_room=0;
  HAS_REAL_FEAT(test_pc,FEAT_SNEAK_ATTACK)=8;SET_COMBAT_FEAT(test_pc,CFEAT_WEAPON_FOCUS,1);
  for(i=FEAT_EPIC_PSIONIC_CONTINGENCY;i<=FEAT_EPIC_ANATOMICAL_SAVANT;i++)
  {HAS_REAL_FEAT(test_pc,i)=0;assert(epic_feat_available(&pc,i));HAS_REAL_FEAT(test_pc,i)=1;assert(epic_feat_rank(&pc,i)==1);assert(!epic_feat_available(&pc,i));}
  GET_LEVEL(test_pc)=20;assert(!epic_feat_rank(&pc,FEAT_EPIC_DODGE));GET_LEVEL(test_pc)=30;
  HAS_REAL_FEAT(test_pc,FEAT_EPIC_DODGE)=2;assert(epic_feat_rank(&pc,FEAT_EPIC_DODGE)==1);
  assert(epic_third_weapon_choice(&pc,1,FALSE));assert(!epic_third_weapon_choice(&pc,2,FALSE));assert(!epic_third_weapon_choice(&pc,-1,TRUE));
  pc.epic_spell_depth=1;epic_third_provenance(&pc,&other,&af,SPELL_BLUR,12,CAST_SPELL);
  assert(af.epic_source==123 && af.epic_cl==12 && af.epic_ward==4);
  other.affected=&af;assert(epic_third_dispel_cl(&pc,&other)==16);
  assert(epic_third_dispel_cl(&other,&other)==30);other.affected=NULL;
  epic_third_provenance(&pc,&other,&af,SPELL_BLUR,12,CAST_WAND);assert(!af.epic_ward);
  pc.epic_reaction_depth=1;epic_third_provenance(&pc,&other,&af,SPELL_BLUR,12,CAST_SPELL);assert(!af.epic_ward);pc.epic_reaction_depth=0;
  epic_third_failed_save(&pc,&other,SPELL_POISON,SAVING_POISON,27);
  epic_third_provenance(&pc,&other,&af,SPELL_POISON,12,CAST_SPELL);
  assert(af.epic_save_dc==27 && af.epic_save_type==SAVING_POISON);
  other.epic_last_save_dc=0;other.epic_last_save_spell=0;memset(&af,0,sizeof(af));
  epic_third_provenance(&pc,&other,&af,SPELL_POISON,12,CAST_SPELL);assert(!af.epic_save_dc);
  memset(&af,0,sizeof(af));af.spell=AFFECT_EPIC_SENTINEL;af.duration=10;af.legendary_expiry=time(NULL)+60;affect_to_char(&pc,&af);
  assert(epic_third_sentinel(&pc,SPELL_HOLD_PERSON,SAVING_WILL,4,20,0)==-1);assert(pc.affected);
  assert(epic_third_sentinel(&pc,SPELL_CHARM,SAVING_WILL,4,20,0)==1);assert(!pc.affected);
  assert(epic_third_sentinel(&pc,SPELL_CHARM,SAVING_WILL,4,20,0)==-1);
  roll_value=1;affect_to_char(&pc,&af);assert(!epic_third_sentinel(&pc,SPELL_FEAR,SAVING_WILL,100,20,0));assert(!pc.affected);roll_value=16;
  E3V(test_pc,E3_ASPECT_OLD)=EVOLUTION_SCENT;E3V(test_pc,E3_ASPECT_NEW)=EVOLUTION_GILLS;E3V(test_pc,E3_ASPECT_END)=time(NULL)+60;
  assert(epic_third_evolution(&pc,EVOLUTION_SCENT,1)==0);assert(epic_third_evolution(&pc,EVOLUTION_GILLS,0)==1);
  assert(epic_third_evolution(&pc,EVOLUTION_GILLS,2)==2);assert(!HAS_REAL_EVOLUTION(test_pc,EVOLUTION_GILLS));
  E3V(test_pc,E3_ASPECT_END)=time(NULL)-1;assert(epic_third_evolution(&pc,EVOLUTION_SCENT,1)==1);
  other.epic_attack_depth=1;other.epic_attack_critical=TRUE;
  assert(!epic_third_dodge(&other,&pc,TYPE_HIT,DAM_BLUDGEON));assert(!EPIC_THIRD(test_pc).ready[E3_DODGE]);
  other.epic_attack_critical=FALSE;SET_BIT_AR(AFF_FLAGS(test_pc),AFF_ENTANGLED);
  assert(!epic_third_dodge(&other,&pc,TYPE_HIT,DAM_BLUDGEON));REMOVE_BIT_AR(AFF_FLAGS(test_pc),AFF_ENTANGLED);
  assert(!epic_third_dodge(&other,&pc,TYPE_HIT,DAM_FIRE));
  assert(epic_third_dodge(&other,&pc,TYPE_HIT,DAM_RESERVED_DBC));assert(other.epic_attack_dodged);
  assert(!epic_third_dodge(&other,&pc,TYPE_HIT,DAM_BLUDGEON));
  GET_OBJ_TYPE(&weapon)=ITEM_WEAPON;GET_OBJ_VAL(&weapon,0)=1;EPIC_CHOICES(test_pc).anatomy_weapon=2;pc.epic_attack_depth=1;
  HAS_REAL_FEAT(&other,FEAT_ESSENCE_OF_UNDEATH)=1;
  assert(epic_third_precision_immunity(&other)==EPIC_PRECISION_ORDINARY);
  assert(epic_third_anatomy(&pc,&other,&weapon));assert(!epic_third_anatomy(&pc,&other,&weapon));
  EPIC_THIRD(test_pc).ready[E3_ANATOMY]=0;random_value=80;
  assert(!epic_third_anatomy(&pc,&other,&weapon));assert(EPIC_THIRD(test_pc).ready[E3_ANATOMY]>time(NULL));random_value=45;
  memset(&af,0,sizeof(af));af.spell=SKILL_DANCE_OF_PROTECTION;af.location=APPLY_AC_NEW;af.duration=12;af.modifier=4;
  epic_third_performance(&pc,&af,0);assert(af.epic_flags==E3_PERFORMANCE);affect_to_char(&pc,&af);
  world[0].people=&pc;IS_PERFORMING(test_pc)=TRUE;
  epic_third_stop_song(&pc,SKILL_SONG_OF_HEROISM);assert(!(pc.affected->epic_flags&E3_TAIL));
  epic_third_stop_song(&pc,SKILL_DANCE_OF_PROTECTION);a=pc.affected;assert((a->epic_flags&E3_TAIL) && a->legendary_expiry==a->epic_baseline+18);
  epic_third_stop_music(&pc);assert(a->legendary_expiry==a->epic_baseline+18);
  memset(&af,0,sizeof(af));af.spell=SKILL_DANCE_OF_PROTECTION;af.location=APPLY_DR;af.duration=12;
  epic_third_performance(&pc,&af,2);assert(!af.epic_flags);
  character_list=&pc;SET_BIT_AR(AFF_FLAGS(test_pc),AFF_SILENCED);epic_third_update();assert(pc.affected && !pc.affected->epic_flags && pc.affected->legendary_expiry==pc.affected->epic_baseline);affect_from_char(&pc,SKILL_DANCE_OF_PROTECTION);
  REMOVE_BIT_AR(AFF_FLAGS(test_pc),AFF_SILENCED);
  EPIC_THIRD(test_pc).ready[E3_CONTINGENCY]=time(NULL)+300;E3V(test_pc,E3_ASPECT_END)=time(NULL)+60;
  epic_third_respec(&pc);assert(!E3V(test_pc,E3_ASPECT_END) && EPIC_THIRD(test_pc).ready[E3_CONTINGENCY]>time(NULL));
  character_list=NULL;
}

static void third_damage_reactions(void)
{
  struct char_data ally={0},foe={0},pet={0},*p=&pet;
  struct follow_type follower={0};
  struct affected_type af={0};
  fixture();known=TRUE;GET_POS(test_pc)=POS_STANDING;GET_HIT(test_pc)=100;
  GET_IDNUM(test_pc)=123;ally.player_specials=&dummy_mob;foe.player_specials=&dummy_mob;
  GET_IDNUM(&ally)=124;ally.in_room=pet.in_room=pc.in_room=0;
  pc.group=ally.group=(struct group_data *)1;
  pet.player_specials=&dummy_mob;pet.master=&pc;pet.nr=NOBODY;
  SET_BIT_AR(MOB_FLAGS(p),MOB_ISNPC);SET_BIT_AR(MOB_FLAGS(p),MOB_EIDOLON);SET_BIT_AR(AFF_FLAGS(p),AFF_CHARM);
  follower.follower=&pet;pc.followers=&follower;world[0].people=&pc;
  E3V(test_pc,E3_INTER_TARGET)=124;E3V(test_pc,E3_INTER_END)=time(NULL)+18;
  af.spell=AFFECT_EPIC_INTERCESSION;af.duration=3;af.legendary_expiry=time(NULL)+18;affect_to_char(&pc,&af);
  GET_HIT(p)=20;assert(epic_third_intercept(&foe,&ally,30,0)==30 && pc.affected);
  GET_HIT(p)=21;assert(epic_third_intercept(&foe,&ally,30,LEG_ORIGIN_BONUS)==30 && GET_HIT(p)==21);
  assert(epic_third_intercept(&foe,&ally,30,0)==10 && GET_HIT(p)==1);assert(!pc.affected && !E3V(test_pc,E3_INTER_END));
  assert(epic_third_intercept(&foe,&ally,30,0)==30);
  E3V(test_pc,E3_INTER_END)=time(NULL)+18;affect_to_char(&pc,&af);GET_HIT(p)=21;pet_save_ok=FALSE;
  assert(epic_third_intercept(&foe,&ally,30,0)==30 && !pc.affected);pet_save_ok=TRUE;
  pc.group=NULL;GET_MAX_HIT(test_pc)=200;GET_HIT(test_pc)=50;GET_PSP(test_pc)=100;GET_AUGMENT_PSP(test_pc)=9;
  E3V(test_pc,E3_POWER)=PSIONIC_INERTIAL_ARMOR;E3V(test_pc,E3_POWER_ML)=18;
  af.spell=AFFECT_EPIC_CONTINGENCY;af.duration=50;af.legendary_expiry=time(NULL)+300;affect_to_char(&pc,&af);manifested=0;
  GET_HIT(test_pc)=0;epic_third_after_damage(&foe,&pc,1,0);assert(!manifested);
  GET_HIT(test_pc)=50;epic_third_after_damage(&foe,&pc,1,LEG_ORIGIN_BONUS);assert(!manifested);
  epic_third_after_damage(&foe,&pc,1,0);assert(manifested==1 && manifested_spell==PSIONIC_INERTIAL_ARMOR && manifested_level==18);
  assert(GET_HIT(test_pc)==50 && GET_PSP(test_pc)==100 && GET_AUGMENT_PSP(test_pc)==9);
  assert(pc.affected && pc.affected->spell==PSIONIC_INERTIAL_ARMOR && pc.affected->duration==10 && pc.affected->legendary_expiry<=time(NULL)+60);
  epic_third_after_damage(&foe,&pc,1,0);assert(manifested==1);
  affect_from_char(&pc,PSIONIC_INERTIAL_ARMOR);
}

static void third_cast_reactions(void)
{
  struct char_data caster={0};
  struct affected_type af={0};
  fixture();known=TRUE;GET_POS(test_pc)=GET_POS(&caster)=POS_STANDING;
  GET_HIT(test_pc)=GET_HIT(&caster)=100;
  caster.player_specials=&dummy_mob;GET_IDNUM(&caster)=777;GET_ID(&caster)=1777;GET_ID(test_pc)=1123;
  pc.next=&caster;character_list=&pc;pc.next_in_room=&caster;world[0].people=&pc;
  pc.group=caster.group=(struct group_data *)1;
  E3V(test_pc,E3_STOW_TARGET)=777;E3V(test_pc,E3_STOW_SPELL)=SPELL_BLUR;E3V(test_pc,E3_STOW_END)=time(NULL)+18;
  caster.epic_cast_serial=1001;af.epic_cast_serial=1000;af.epic_source=777;af.spell=SPELL_BLUR;af.duration=11;af.epic_ward=4;
  affect_to_char(&caster,&af);
  epic_third_cast_done(&caster,&caster,SPELL_BLUR,12,CAST_WAND,TRUE);assert(E3V(test_pc,E3_STOW_END)>time(NULL) && !pc.affected);
  epic_third_cast_done(&caster,&pc,SPELL_BLUR,12,CAST_SPELL,TRUE);assert(E3V(test_pc,E3_STOW_END)>time(NULL));
  caster.affected->epic_cast_serial=1001;
  epic_third_cast_done(&caster,&caster,SPELL_BLUR,12,CAST_SPELL,TRUE);
  assert(!E3V(test_pc,E3_STOW_END) && pc.affected && pc.affected->duration==5 && !pc.affected->epic_ward);
  assert(pc.affected->epic_cast_serial==0);affect_from_char(&pc,SPELL_BLUR);affect_from_char(&caster,SPELL_BLUR);
  caster.group=NULL;reactions=0;spell_info[SPELL_MAGIC_MISSILE].violent=TRUE;
  E3V(test_pc,E3_OP_TARGET)=1777;E3V(test_pc,E3_OP_SPELL)=SPELL_SHOCKING_GRASP;E3V(test_pc,E3_OP_CLASS)=CLASS_WIZARD;E3V(test_pc,E3_OP_END)=time(NULL)+18;
  epic_third_cast_done(&caster,&pc,SPELL_MAGIC_MISSILE,12,CAST_SPELL,FALSE);assert(!reactions && E3V(test_pc,E3_OP_END)>time(NULL));
  caster.epic_reaction_depth=1;epic_third_cast_done(&caster,&pc,SPELL_MAGIC_MISSILE,12,CAST_SPELL,TRUE);assert(!reactions);caster.epic_reaction_depth=0;
  epic_third_cast_done(&caster,&pc,SPELL_MAGIC_MISSILE,12,CAST_SPELL,TRUE);
  assert(reactions==1 && reaction_spell==SPELL_SHOCKING_GRASP && !E3V(test_pc,E3_OP_END));
  epic_third_cast_done(&caster,&pc,SPELL_MAGIC_MISSILE,12,CAST_SPELL,TRUE);assert(reactions==1);
  character_list=NULL;pc.next=NULL;
}

static void discipline_focus(void)
{
  int roll, dc, before, after;
  fixture();
  CLASS_LEVEL(test_pc,CLASS_PSIONICIST)=20;
  GET_ABILITY(test_pc,ABILITY_SPELLCRAFT)=20;
  GET_LEVEL(test_pc)=26;
  GET_REAL_INT(test_pc)=23;
  EPIC_CHOICES(test_pc).discipline=TELEPATHY;
  HAS_REAL_FEAT(test_pc,FEAT_EPIC_DISCIPLINE_FOCUS)=2;
  psionic_powers[PSIONIC_POWER_START].power_type=TELEPATHY;
  assert(epic_discipline_dc(test_pc,PSIONIC_POWER_START)==1);
  assert(!epic_feat_available(test_pc,FEAT_EPIC_DISCIPLINE_FOCUS));
  GET_LEVEL(test_pc)=27;
  assert(epic_discipline_dc(test_pc,PSIONIC_POWER_START)==2);
  HAS_REAL_FEAT(test_pc,FEAT_EPIC_DISCIPLINE_FOCUS)=1;
  assert(epic_feat_available(test_pc,FEAT_EPIC_DISCIPLINE_FOCUS));
  HAS_REAL_FEAT(test_pc,FEAT_EPIC_DISCIPLINE_FOCUS)=2;
  psionic_powers[PSIONIC_POWER_START].power_type=PSYCHOKINESIS;
  assert(!epic_discipline_dc(test_pc,PSIONIC_POWER_START));
  psionic_powers[PSIONIC_POWER_START].power_type=TELEPATHY;
  psionic_powers[PSIONIC_POWER_START].is_epic=TRUE;
  assert(!epic_discipline_dc(test_pc,PSIONIC_POWER_START));
  assert(!epic_discipline_dc(test_pc,SPELL_MAGIC_MISSILE));
  psionic_powers[PSIONIC_POWER_START].is_epic=FALSE;
  GET_REAL_INT(test_pc)=22;
  assert(!epic_discipline_dc(test_pc,PSIONIC_POWER_START));
  /* Enumerate native natural-one failure / natural-twenty success. */
  for(dc=20;dc<=35;dc+=5)
  {
    before=after=0;
    for(roll=1;roll<=20;roll++)
    {
      before += roll!=1 && (roll==20 || roll+15>=dc);
      after += roll!=1 && (roll==20 || roll+15>=dc+2);
    }
    assert(before-after>=0 && before-after<=2);
  }
}

int main(void)
{
  discipline_focus();third_cast_reactions();third_damage_reactions();third_batch();second_batch();eligibility_and_ranks();crafting_and_charges();power_cost_and_choices();narrow_save_bonus();companion_ward();
  puts("Epic batches 1, 2 and 3: prerequisites, rank gates, resistance arms, finite shields, summon doctrines, attack orders, bond saves, bomb binding, crafting, PSP and companion persistence passed.");
  return 0;
}
