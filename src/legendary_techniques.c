/* Live catalog adapters. Values always come from legendary_resolve_power. */
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "act.h"
#include "dg_event.h"
#include "actions.h"
#include "spells.h"
#include "fight.h"
#include "constants.h"
#include "spec_procs.h"
#include "house.h"
#include "lists.h"
#include "movement_validation.h"
#include "legendary_techniques.h"
#include "legendary_equipment.h"
#include "legendary_identity.h"
#include "legendary_store.h"
#include <limits.h>
#define OPENING_MARK 303
static bool true_lance(const struct legendary_activation *,struct obj_data *);
static int signature_damage(struct char_data *,struct char_data *,int,int,struct legendary_attack *);
int legendary_script_depth;
static int64_t next_affect_round;
static bool locked(struct affected_type *af)
{return af->duration<0||(af->legendary_flags&LEG_CONDITION_LOCKED);}
static struct affected_type *fear(struct char_data *ch)
{
  struct affected_type *af,*oldest=NULL;
  for(af=ch->affected;af;af=af->next)if(!locked(af)&&IS_SET_AR(af->bitvector,AFF_FEAR)&&
      (af->spell==SPELL_FEAR||af->spell==SPELL_DRAGONFEAR))oldest=af;
  return oldest;
}
static struct affected_type *bleed(struct char_data *ch)
{
  struct affected_type *af,*oldest=NULL;
  for(af=ch->affected;af;af=af->next)if(!locked(af)&&IS_SET_AR(af->bitvector,AFF_BLEED)&&
      (af->spell==SKILL_BLEEDING_ATTACK||af->spell==ABILITY_BLEEDING_CRITICAL||
       af->spell==SKILL_TIGER_CLAW||af->spell==EVOLUTION_BLEED_EFFECT))oldest=af;
  return oldest;
}
void legendary_affect_schedule(int pulses){next_affect_round=legendary_effects_now()+(pulses+PASSES_PER_SEC-1)/PASSES_PER_SEC;}
void legendary_affect_round(void){next_affect_round=legendary_effects_now()+PULSE_VIOLENCE/PASSES_PER_SEC;}
void legendary_condition_origin(struct affected_type *af,struct char_data *source)
{
  (void)source;
  if(legendary_script_depth)af->legendary_flags|=LEG_CONDITION_LOCKED;
}
void legendary_condition_expire(struct char_data *ch)
{
  struct affected_type *af,*next;
  for(af=ch->affected;af;af=next){next=af->next;
    if(af->legendary_expiry>0&&af->legendary_expiry<=legendary_effects_now())affect_remove(ch,af);
  }
}
static bool able(struct char_data *ch)
{return ch&&GET_HIT(ch)>0&&GET_POS(ch)>POS_SLEEPING&&!AFF_FLAGGED(ch,AFF_STUN)&&
 !AFF_FLAGGED(ch,AFF_PARALYZED)&&!AFF_FLAGGED(ch,AFF_DAZED)&&!AFF_FLAGGED(ch,AFF_PINNED);}
static bool recoverable(struct char_data *ch)
{return able(ch)&&ch->legendary_knockdown&&GET_POS(ch)==POS_SITTING&&
 !AFF_FLAGGED(ch,AFF_GRAPPLED)&&!AFF_FLAGGED(ch,AFF_ENTANGLED)&&!RIDING(ch)&&!RIDDEN_BY(ch)&&
 !SITTING(ch)&&GET_WAIT_STATE(ch)==0&&!IS_CASTING(ch)&&is_action_available(ch,atSTANDARD,false);}
static bool weapon_ready(struct char_data *ch)
{
  static const int slots[]={WEAR_WIELD_1,WEAR_WIELD_2H,WEAR_WIELD_OFFHAND};int i;
  for(i=0;i<3;i++)if(GET_EQ(ch,slots[i])&&(GET_OBJ_TYPE(GET_EQ(ch,slots[i]))==ITEM_WEAPON||GET_OBJ_TYPE(GET_EQ(ch,slots[i]))==ITEM_FIREWEAPON))return true;
  return false;
}
static bool check(const struct legendary_activation *a)
{
  switch(a->power){
    case LEG_T01:case LEG_T03:return weapon_ready(a->source);
    case LEG_T07:case LEG_T14:return a->source!=a->target;
    case LEG_T08:return recoverable(a->target);
    case LEG_T09:return GET_HIT(a->target)<GET_MAX_HIT(a->target);
    case LEG_T10:return able(a->target)&&fear(a->target)!=NULL;
    case LEG_T11:return bleed(a->target)!=NULL;
    case LEG_T15:case LEG_T16:return legendary_resource_class(a->source);
    default:return true;
  }
}
static void prepared(const struct legendary_activation *a,int key,int magnitude,int duration,int flags)
{
  struct legendary_effect_spec s={0};s.key=key;s.magnitude=magnitude;s.duration=duration;s.flags=flags;s.charges=1;
  (void)legendary_effect_prepare(a,&s);
}
static void apply(const struct legendary_activation *a)
{
  const struct legendary_values *v=&a->resolved.values;
  struct legendary_effect_spec s={0};struct affected_type *af;int64_t expiry,now=legendary_effects_now();int i;
  int flags=a->target!=a->source?LEG_EFFECT_ROOM_BOUND:0;
  s.key=a->power;s.duration=v->window?v->window:v->duration;s.magnitude=v->check_bonus?v->check_bonus:1;s.charges=1;s.flags=flags;
  switch(a->power){
    case LEG_T04:{
      static const int types[]={DAM_FIRE,DAM_COLD,DAM_ACID,DAM_ELECTRIC,DAM_SOUND};
      static const char *names[]={"fire","cold","acid","electricity","sonic"};bool immune=false;
      for(i=0;i<5;i++)send_to_char(a->source,"%s: %d%% resistance\r\n",names[i],compute_damtype_reduction(a->target,types[i],a->source,TYPE_UNDEFINED));
      immune=IS_NPC(a->target)&&MOB_FLAGGED(a->target,MOB_LEGENDARY_BOSS);
      send_to_char(a->source,"Legendary-boss hard-control protection: %s. Other immunities depend on the condition.\r\n",immune?"yes":"no");
      if(v->physical_info)send_to_char(a->source,"Physical reduction (slash/pierce/bludgeon): %d/%d/%d.\r\n",compute_damage_reduction(a->target,DAM_SLICE),compute_damage_reduction(a->target,DAM_PUNCTURE),compute_damage_reduction(a->target,DAM_BLUDGEON));
      return;
    }
    case LEG_T05:case LEG_T07:case LEG_T17:
      s.kind=a->power==LEG_T07?LEG_EFFECT_REDIRECT:LEG_EFFECT_REDUCTION;
      s.percent=v->percent;s.maximum=v->pool;s.flags|=a->power==LEG_T17?LEG_EFFECT_BREATH:LEG_EFFECT_PHYSICAL;
      (void)legendary_effect_prepare(a,&s);return;
    case LEG_T08:
      char_from_furniture(a->target);a->target->legendary_knockdown=false;
      change_position(a->target,FIGHTING(a->target)?POS_FIGHTING:POS_STANDING);
      if(FIGHTING(a->target))attacks_of_opportunity(a->target,0);
      if(v->check_bonus)prepared(a,LEG_T08,v->check_bonus,v->duration,flags);
      return;
    case LEG_T09:(void)legendary_heal(a->source,a->target,v->heal);return;
    case LEG_T10:
      af=fear(a->target);if(!af)return;
      if(!next_affect_round)next_affect_round=now+PULSE_VIOLENCE/PASSES_PER_SEC;
      expiry=af->legendary_expiry?af->legendary_expiry:next_affect_round+(int64_t)af->duration*(PULSE_VIOLENCE/PASSES_PER_SEC);
      if(expiry-now<=v->fear_seconds)affect_remove(a->target,af);else af->legendary_expiry=expiry-v->fear_seconds;
      return;
    case LEG_T11:af=bleed(a->target);if(af)affect_remove(a->target,af);(void)legendary_heal(a->source,a->target,v->heal);return;
    case LEG_T13:
      legendary_effect_clear_source(a->source,LEG_T13);s.flags=LEG_EFFECT_SOURCE_STATIONARY;
      (void)legendary_effect_prepare(a,&s);legendary_track(a->source);return;
    default:(void)legendary_effect_prepare(a,&s);return;
  }
}
void legendary_techniques_init(void)
{
  static bool initialized;int id,target;
  legendary_signatures_init();
  if(initialized)return;
  initialized=true;
  for(id=LEG_T01;id<=LEG_T18;id++){
    target=id==LEG_T04||id==LEG_T13?LEG_TARGET_ENEMY:
      id==LEG_T07||id==LEG_T09||id==LEG_T14?LEG_TARGET_ALLY:LEG_TARGET_SELF;
    (void)legendary_activation_register(id,target,check,apply);
  }
}
int legendary_technique_attack(struct char_data *ch,struct char_data *vict,struct obj_data *weapon,struct legendary_attack *ctx)
{
  struct legendary_activation a;int bonus=0;
  memset(ctx,0,sizeof(*ctx));
  if(!legendary_target_legal(ch,vict,LEG_TARGET_ENEMY)||!weapon||(GET_OBJ_TYPE(weapon)!=ITEM_WEAPON&&GET_OBJ_TYPE(weapon)!=ITEM_FIREWEAPON))return 0;
  if(legendary_effect_peek(ch,LEG_T01,&ctx->strike,NULL)){
    ctx->measured=true;bonus+=ctx->strike.resolved.values.attack;legendary_effect_consume(ch,LEG_T01);
  }
  if(legendary_effect_peek(ch,LEG_T14,&a,NULL)){bonus+=a.resolved.values.check_bonus;legendary_effect_consume(ch,LEG_T14);}
  if(legendary_effect_peek(vict,OPENING_MARK,&a,NULL)&&a.source!=ch&&GROUP(ch)&&GROUP(ch)==GROUP(a.source)&&
      IN_ROOM(a.source)==IN_ROOM(ch)){
    bonus+=a.resolved.values.attack;legendary_effect_clear(vict,OPENING_MARK);
  }
  ctx->opening=legendary_effect_peek(ch,LEG_T03,&ctx->mark,NULL);
  if(legendary_effect_from(ch,LEG_S09,&ctx->hunt)&&ctx->hunt.target==vict){
    ctx->hunted=true;bonus+=ctx->hunt.resolved.values.attack;legendary_effect_clear_source(ch,LEG_S09);
  }
  if(legendary_effect_from(ch,LEG_S13,&ctx->rally)){
    ctx->rallying=true;legendary_effect_clear_source(ch,LEG_S13);
  }
  ctx->grounding=legendary_effect_peek(ch,LEG_S11,&ctx->ground,NULL)&&true_lance(&ctx->ground,weapon);
  return bonus;
}
int legendary_technique_damage(struct char_data *ch,struct char_data *vict,int ordinary,int type,struct legendary_attack *ctx)
{
  int bonus=0,resistance;
  if(!ctx||!legendary_target_legal(ch,vict,LEG_TARGET_ENEMY))return 0;
  if(ordinary<=0)return signature_damage(ch,vict,0,type,ctx);
  /* A source lease must still exist even though the attack consumed its charge. */
  if(ctx->measured){
    const struct legendary_loadout *load=legendary_loadout_get(ch);int i,j;
    for(i=0;i<load->major_count;i++)if(load->majors[i].reason==LEG_EQUIPPED&&load->majors[i].power==LEG_T01&&!strcmp(load->majors[i].uuid,ctx->strike.uuid)&&load->majors[i].channel==ctx->strike.channel)break;
    for(j=0;j<load->count;j++)if(load->channels[j].channel==ctx->strike.channel&&!strcmp(load->channels[j].uuid,ctx->strike.uuid)&&load->channels[j].version==ctx->strike.source_version)break;
    if(i<load->major_count&&j<load->count){
      resistance=MAX(0,MIN(100,compute_damtype_reduction(vict,type,ch,TYPE_UNDEFINED)));
      bonus=(int)((int64_t)ctx->strike.resolved.values.damage*(100-resistance)/100);
      bonus=legendary_budget_debit(ch,LEG_BUDGET_OFFENSE,MIN(bonus,MAX(0,GET_HIT(vict)-ordinary)));
    }
    ctx->measured=false;
  }
  if(ctx->opening&&legendary_effect_consume(ch,LEG_T03)){
    struct legendary_activation mark=ctx->mark;mark.target=vict;
    legendary_effect_clear(vict,OPENING_MARK);
    prepared(&mark,OPENING_MARK,mark.resolved.values.attack,mark.resolved.values.duration,LEG_EFFECT_ROOM_BOUND);
    ctx->opening=false;
  }
  bonus+=signature_damage(ch,vict,ordinary+bonus,type,ctx);
  return bonus;
}
int legendary_technique_maneuver(struct char_data *ch,int kind,bool defense)
{
  int value=0;
  if(kind==COMBAT_MANEUVER_TYPE_KNOCKDOWN||kind==COMBAT_MANEUVER_TYPE_DISARM||kind==COMBAT_MANEUVER_TYPE_FORCED_MOVEMENT)value+=legendary_effect_value(ch,LEG_T02);
  if(defense&&(kind==COMBAT_MANEUVER_TYPE_KNOCKDOWN||kind==COMBAT_MANEUVER_TYPE_FORCED_MOVEMENT))value+=legendary_effect_value(ch,LEG_T06);
  if(defense&&GROUP(ch)&&(kind==COMBAT_MANEUVER_TYPE_KNOCKDOWN||kind==COMBAT_MANEUVER_TYPE_FORCED_MOVEMENT))value+=legendary_effect_value(ch,LEG_S02);
  if(defense&&kind==COMBAT_MANEUVER_TYPE_KNOCKDOWN)value+=legendary_effect_value(ch,LEG_T08);
  return value;
}
int legendary_technique_skill(struct char_data *ch,int skill,bool consume)
{
  struct legendary_activation a;int bonus=0;
  if(!consume&&skill==ABILITY_CONCENTRATION)bonus+=legendary_effect_value(ch,LEG_S08);
  if(!consume&&skill==ABILITY_STEALTH)bonus+=legendary_effect_value(ch,LEG_T18);
  if(consume&&legendary_supported_skill(skill)&&legendary_effect_peek(ch,LEG_T14,&a,NULL)){
    bonus+=a.resolved.values.check_bonus;legendary_effect_consume(ch,LEG_T14);
  }
  return bonus;
}
int legendary_technique_walk(struct char_data *ch,int cost,bool mounted)
{
  int percent=0;struct legendary_activation a;
  if(legendary_effect_peek(ch,LEG_T12,&a,NULL))percent=a.resolved.values.percent;
  if(legendary_effect_peek(ch,LEG_S03,&a,NULL))percent=MAX(percent,a.resolved.values.percent);
  return mounted||cost<=0||!percent?cost:MAX(1,(int)((int64_t)cost*(100-percent)/100));
}
void legendary_track(struct char_data *ch)
{
  struct legendary_activation a;int dir;
  if(!legendary_effect_from(ch,LEG_T13,&a)){send_to_char(ch,"You have no active pursuit mark.\r\n");return;}
  if(IN_ROOM(ch)==IN_ROOM(a.target)){send_to_char(ch,CAN_SEE(ch,a.target)?"Your marked quarry is here.\r\n":"Your quarry's direction is obscured.\r\n");return;}
  for(dir=0;dir<NUM_OF_DIRS;dir++)if(legendary_exit_traversable(ch,dir)&&EXIT(ch,dir)->to_room==IN_ROOM(a.target)&&CAN_SEE(ch,a.target)&&
      (!room_is_dark(IN_ROOM(a.target))||CAN_SEE_IN_DARK(ch))){send_to_char(ch,"Your marked quarry is %s.\r\n",dirs[dir]);return;}
  send_to_char(ch,"Your quarry's direction is obscured.\r\n");
}
struct payment {struct payment *next;struct char_data *ch;int spell,cost;bool paid,effect;char id[33];};
static struct payment *payments;
void legendary_cast_clear(struct char_data *ch)
{struct payment **at=&payments,*p;while((p=*at))if(p->ch==ch){*at=p->next;free(p);}else at=&p->next;}
bool legendary_cast_queue(struct char_data *ch,int spell,int cost)
{
  struct payment *p;if(IS_CASTING(ch))return false;legendary_cast_clear(ch);if(IS_NPC(ch)||cost<0)return false;
  p=calloc(1,sizeof(*p));if(!p)return false;p->ch=ch;p->spell=spell;p->cost=cost;
  if(!legendary_uuid_generate(p->id)){free(p);return false;}p->next=payments;payments=p;return true;
}
static int discount_power(struct char_data *ch,int cost,int *discount)
{
  struct legendary_activation a;int key=0;*discount=0;
  if(cost<20)return 0;
  if(legendary_effect_peek(ch,LEG_T16,&a,NULL)){key=LEG_T16;*discount=a.resolved.values.resource_discount;}
  if(legendary_effect_peek(ch,LEG_S07,&a,NULL)&&a.resolved.values.resource_discount>*discount){key=LEG_S07;*discount=a.resolved.values.resource_discount;}
  return key;
}
bool legendary_technique_affordable(struct char_data *ch,int cost)
{
  int discount;if(GET_PSP(ch)>=cost)return true;
  (void)discount_power(ch,cost,&discount);
  discount=MIN(discount,legendary_budget_available(ch,LEG_BUDGET_RESOURCE));
  return GET_PSP(ch)>=MAX(1,cost-discount);
}
int legendary_technique_pay(struct char_data *ch,int spell,int cost)
{
  int discount,key=discount_power(ch,cost,&discount),paid=legendary_resource_pay(ch,cost,discount);
  if(paid>=0&&paid<cost&&key)legendary_effect_consume(ch,key);
  (void)spell;return paid;
}
bool legendary_cast_commit(struct char_data *ch,int spell)
{
  struct payment *p;for(p=payments;p;p=p->next)if(p->ch==ch&&p->spell==spell){
    if(p->paid)return true;
    if(p->cost>0){int paid=legendary_technique_pay(ch,spell,p->cost);if(paid<0)return false;p->cost=paid;}
    p->paid=true;return true;
  }
  return true; /* Prepared/Vancian and item spells never invent a point payment. */
}
void legendary_cast_failed(struct char_data *ch,int spell)
{
  struct payment *p;struct legendary_activation a;
  for(p=payments;p;p=p->next)if(p->ch==ch&&p->spell==spell&&p->paid&&!p->effect&&p->cost>=20&&legendary_effect_peek(ch,LEG_T15,&a,NULL)){
    int refund=MIN(a.resolved.values.refund_cap,(int)((int64_t)p->cost*a.resolved.values.refund_percent/100));
    (void)legendary_resource_restore(ch,refund);legendary_effect_consume(ch,LEG_T15);p->effect=true;break;
  }
}
void legendary_cast_effect(struct char_data *ch,int spell,int casttype)
{struct payment *p;if(casttype!=CAST_SPELL)return;for(p=payments;p;p=p->next)if(p->ch==ch&&p->spell==spell)p->effect=true;}
/* Read-only traversal predicate: never run room triggers, spend movement, roll
 * swimming/climbing checks or open a door just to disclose a direction. */
bool legendary_exit_traversable(struct char_data *ch,int dir)
{
  struct room_direction_data *ex;room_rnum to;int sector,i;bool boat=false;struct obj_data *obj;
  if(!ch||IN_ROOM(ch)==NOWHERE||dir<0||dir>=NUM_OF_DIRS||!CAN_GO(ch,dir))return false;
  ex=EXIT(ch,dir);to=ex->to_room;
  if(to==NOWHERE||to>top_of_world||IS_SET(ex->exit_info,EX_HIDDEN|EX_HIDDEN_MEDIUM|EX_HIDDEN_HARD|EX_HIDDEN_EASY))return false;
  if(ROOM_FLAGGED(to,ROOM_STAFFROOM)||ROOM_FLAGGED(to,ROOM_DEATH)||ROOM_FLAGGED(to,ROOM_FOG)||ROOM_FLAGGED(IN_ROOM(ch),ROOM_FOG))return false;
  if(ZONE_FLAGGED(GET_ROOM_ZONE(to),ZONE_CLOSED)||ZONE_FLAGGED(GET_ROOM_ZONE(to),ZONE_NOIMMORT))return false;
  if(ROOM_FLAGGED(IN_ROOM(ch),ROOM_ATRIUM)&&!House_can_enter(ch,GET_ROOM_VNUM(to)))return false;
  if(ROOM_FLAGGED(to,ROOM_TUNNEL)&&(RIDING(ch)||num_pc_in_room(&world[to])>=CONFIG_TUNNEL_SIZE))return false;
  if(AFF_FLAGGED(ch,AFF_GRAPPLED)||AFF_FLAGGED(ch,AFF_ENTANGLED)||AFF_FLAGGED(ch,AFF_PINNED))return false;
  sector=SECT(to);
  if((sector==SECT_FLYING||sector==SECT_UD_NOGROUND||SECT(IN_ROOM(ch))==SECT_FLYING||SECT(IN_ROOM(ch))==SECT_UD_NOGROUND)&&(!has_flight(ch)||(RIDING(ch)&&!has_flight(RIDING(ch)))))return false;
  if((sector==SECT_UNDERWATER||SECT(IN_ROOM(ch))==SECT_UNDERWATER)&&!has_scuba(ch,to))return false;
  for(obj=ch->carrying;obj;obj=obj->next_content)if(GET_OBJ_TYPE(obj)==ITEM_BOAT)boat=true;
  for(i=0;i<NUM_WEARS;i++)if(GET_EQ(ch,i)&&GET_OBJ_TYPE(GET_EQ(ch,i))==ITEM_BOAT)boat=true;
  if(sector==SECT_WATER_SWIM||sector==SECT_WATER_NOSWIM||sector==SECT_UD_WATER||sector==SECT_UD_NOSWIM||sector==SECT_UNDERWATER){
    if(!boat&&!has_flight(ch)&&!AFF_FLAGGED(ch,AFF_WATERWALK)&&!AFF_FLAGGED(ch,AFF_LEVITATE)){
      if(sector==SECT_WATER_NOSWIM||sector==SECT_UD_NOSWIM)return false;
      if(!is_natural_swimmer(ch)&&20+compute_ability(ch,ABILITY_ATHLETICS)<13+zone_table[GET_ROOM_ZONE(to)].min_level)return false;
    }
    /* A mount must have a guaranteed water route; tracking never rolls a
     * mount's swimming check or spends its action. */
    if(RIDING(ch)&&!has_flight(RIDING(ch))&&!AFF_FLAGGED(RIDING(ch),AFF_WATERWALK)&&!AFF_FLAGGED(RIDING(ch),AFF_LEVITATE)&&!is_natural_swimmer(RIDING(ch)))return false;
  }
  if(ROOM_FLAGGED(to,ROOM_CLIMB_NEEDED)&&!has_flight(ch)&&
     20+compute_ability(ch,ABILITY_ATHLETICS)<13+zone_table[GET_ROOM_ZONE(to)].min_level-(affected_by_spell(ch,SPELL_GRASP)?2:0))return false;
  return true;
}

/* Signatures share the same activation transactions, leases and family clocks. */
#define GROUNDING_LOCK 311
static bool true_lance(const struct legendary_activation *a,struct obj_data *weapon)
{
  struct legendary_registry_record r;
  return weapon&&weapon->legendary_uuid[0]&&!strcmp(weapon->legendary_uuid,a->uuid)&&
    GET_OBJ_RNUM(weapon)!=NOTHING&&
    legendary_registry_cached(GET_OBJ_VNUM(weapon),true,&r)&&r.enabled;
}
bool legendary_activation_live(const struct legendary_activation *a)
{
  const struct legendary_loadout *load=legendary_loadout_get(a->source);int i,j;
  for(i=0;i<load->major_count;i++)if(load->majors[i].reason==LEG_EQUIPPED&&
      load->majors[i].power==a->power&&load->majors[i].channel==a->channel&&
      !strcmp(load->majors[i].uuid,a->uuid))break;
  for(j=0;j<load->count;j++)if(load->channels[j].channel==a->channel&&
      !strcmp(load->channels[j].uuid,a->uuid)&&load->channels[j].version==a->source_version)break;
  if(i>=load->major_count||j>=load->count)return false;
  if(legendary_power(a->power)->requires_dragonlance&&!true_lance(a,GET_EQ(a->source,load->channels[j].wear)))return false;
  return true;
}
static bool signature_check(const struct legendary_activation *a)
{
  switch(a->power){
    case LEG_S02:return GROUP(a->source)&&a->target==a->source;
    case LEG_S03:return a->target==a->source;
    case LEG_S05:return GET_HIT(a->target)<GET_MAX_HIT(a->target);
    case LEG_S07:return legendary_resource_class(a->source);
    case LEG_S08:case LEG_S13:return a->source!=a->target;
    case LEG_S11:case LEG_S12:{int i;static const int slots[]={WEAR_WIELD_1,WEAR_WIELD_OFFHAND,WEAR_WIELD_2H};
      if(a->power==LEG_S12&&(!RIDING(a->source)||RIDING(a->source)->master!=a->source||
         GET_HIT(RIDING(a->source))<=0||!IS_NPC(RIDING(a->source))||
         GET_REAL_RACE(RIDING(a->source))!=RACE_TYPE_DRAGON||!is_riding_dragon_mount(a->source)))return false;
      for(i=0;i<3;i++)if(true_lance(a,GET_EQ(a->source,slots[i])))return true;
      return false;}
    default:return true;
  }
}
static void signature_recipient(const struct legendary_activation *a)
{
  const struct legendary_values *v=&a->resolved.values;struct legendary_effect_spec s={0};
  s.key=a->power;s.duration=v->window?v->window:v->duration;
  s.magnitude=v->check_bonus?v->check_bonus:v->ac?v->ac:1;s.charges=v->charges;
  s.flags=LEG_EFFECT_ROOM_BOUND;
  switch(a->power){
    case LEG_S01:case LEG_S10:case LEG_S12:
      s.kind=LEG_EFFECT_REDUCTION;s.percent=v->percent;s.maximum=v->pool;
      s.flags|=a->power==LEG_S10?LEG_EFFECT_BREATH:LEG_EFFECT_PHYSICAL;
      if(a->power==LEG_S01)s.pool=v->pool;
      if(a->power==LEG_S12)s.flags|=LEG_EFFECT_MOUNT;
      break;
    case LEG_S02:s.flags|=LEG_EFFECT_GROUP;break;
    case LEG_S03:s.flags=LEG_EFFECT_GROUP;break;
    case LEG_S05:
      (void)legendary_heal(a->source,a->target,v->heal);
      s.kind=LEG_EFFECT_HEAL_TICKS;s.magnitude=v->tick_heal;s.interval=v->interval;s.ticks=v->ticks;break;
    case LEG_S08:(void)legendary_temporary_hp(a,v->temporary_hp,s.duration);break;
    default:break;
  }
  (void)legendary_effect_prepare(a,&s);
}
static void signature_apply(const struct legendary_activation *a)
{
  struct legendary_activation copy=*a;struct item_data *entry;struct char_data *member;int n=1;
  if(a->power==LEG_S09||a->power==LEG_S13)legendary_effect_clear_source(a->source,a->power);
  signature_recipient(a);
  if(a->power==LEG_S12){copy.target=RIDING(a->source);signature_recipient(&copy);}
  if((a->power==LEG_S02||a->power==LEG_S03)&&GROUP(a->source)&&GROUP(a->source)->members){
    for(entry=GROUP(a->source)->members->pFirstItem;entry;entry=entry->pNextItem){
      member=entry->pContent;
      if(n>=a->resolved.values.targets)continue;
      if(member==a->source||!legendary_target_legal(a->source,member,LEG_TARGET_ALLY))continue;
      copy.target=member;signature_recipient(&copy);n++;
    }
  }
}
void legendary_signatures_init(void)
{
  static bool initialized;int id,target;if(initialized)return;initialized=true;
  for(id=LEG_S01;id<=LEG_S13;id++){
    target=id==LEG_S09?LEG_TARGET_ENEMY:
      id==LEG_S01||id==LEG_S04||id==LEG_S05||id==LEG_S08||id==LEG_S10||id==LEG_S13?LEG_TARGET_ALLY:LEG_TARGET_SELF;
    (void)legendary_activation_register(id,target,signature_check,signature_apply);
  }
}
int legendary_signature_dodge(struct char_data *ch,bool consume)
{
  int value=legendary_effect_value(ch,LEG_S06);
  if(consume&&value)legendary_effect_consume(ch,LEG_S06);
  return value;
}
int legendary_signature_save(struct char_data *ch,int type,int spell)
{
  if(type!=SAVING_WILL)return 0;
  switch(spell){
    case SPELL_FEAR:case SPELL_DRAGONFEAR:case SPELL_SCARE:case PSIONIC_DEADLY_FEAR:
    case SPELL_CHARM:case SPELL_CHARM_MONSTER:case SPELL_MASS_CHARM_MONSTER:case SPELL_CHARM_ANIMAL:
    case SPELL_CONFUSION:case SPELL_DOMINATE_PERSON:case SPELL_MASS_DOMINATION:case WARLOCK_CHARM:
    case ABILITY_VAMPIRIC_DOMINATION:case PSYCHOKINETIC_FEAR:case RACIAL_LICH_FEAR:
    case EVOLUTION_FRIGHTFUL_EFFECT:case AFFECT_AURA_OF_TERROR:case AFFECT_BLACKGUARD_FEAR:
    case PSIONIC_MOMENT_OF_TERROR:case WARLOCK_FRIGHTFUL_BLAST:case SKILL_SONG_OF_FEAR:
    case BOMB_AFFECT_CONFUSION:return legendary_effect_value(ch,LEG_S04);
    default:return 0;
  }
}
bool legendary_grounded(struct char_data *ch)
{return !legendary_script_depth&&(IS_NPC(ch)||GET_LEVEL(ch)<LVL_IMMORT)&&legendary_effect_value(ch,GROUNDING_LOCK)>0;}

/* Post-defense additions never enter critical multipliers or native proc rolls. */
static int signature_flat(struct char_data *ch,struct char_data *vict,int amount,int ordinary,int type)
{
  int resistance;
  if(ordinary<=0)return 0; /* A bonus cannot turn a fully prevented packet into damage. */
  resistance=MAX(0,MIN(100,compute_damtype_reduction(vict,type,ch,TYPE_UNDEFINED)));
  amount=(int)((int64_t)amount*(100-resistance)/100);
  return legendary_budget_debit(ch,LEG_BUDGET_OFFENSE,MIN(amount,MAX(0,GET_HIT(vict)-ordinary)));
}
static int signature_damage(struct char_data *ch,struct char_data *vict,int ordinary,int type,struct legendary_attack *ctx)
{
  int bonus=0;
  if(ctx->hunted&&ctx->hunt.target==vict&&legendary_activation_live(&ctx->hunt))
    bonus+=signature_flat(ch,vict,ctx->hunt.resolved.values.damage,ordinary,type);
  ctx->hunted=false;
  if(ctx->rallying&&legendary_activation_live(&ctx->rally)&&ctx->rally.target!=ch&&
      legendary_target_legal(ch,ctx->rally.target,LEG_TARGET_ALLY))
    (void)legendary_heal(ch,ctx->rally.target,ctx->rally.resolved.values.heal);
  ctx->rallying=false;
  if(ctx->grounding&&IS_DRAGON(vict)&&legendary_activation_live(&ctx->ground)&&legendary_effect_consume(ch,LEG_S11)){
    int roll;
    bool immune=IS_NPC(vict)&&MOB_FLAGGED(vict,MOB_LEGENDARY_BOSS);
    if(immune)bonus+=signature_flat(ch,vict,ctx->ground.resolved.values.damage,ordinary+bonus,type);
    else {
      /* Fixed DC: no caster ability, spell school, spell DC or spell bypass perks. */
      roll=d20(vict);
      if(roll==1||(roll!=20&&roll+compute_mag_saves(vict,SAVING_FORT,0)<ctx->ground.resolved.values.save_dc)){
        struct legendary_activation lock=ctx->ground;lock.target=vict;
        prepared(&lock,GROUNDING_LOCK,1,ctx->ground.resolved.values.control_seconds,LEG_EFFECT_ROOM_BOUND);
      }
    }
  }
  ctx->grounding=false;return bonus;
}
