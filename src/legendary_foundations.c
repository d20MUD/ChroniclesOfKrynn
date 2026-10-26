/* Runtime contributions only. Never persist these as base stats or item applies. */
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "spells.h"
#include "fight.h"
#include "legendary_equipment.h"
#include "legendary_foundations.h"
#include "legendary_techniques.h"

struct legendary_foundation_state {
  bool busy,syncing,rerun;
  int deflection;
  unsigned long long plan;
  int ability[6],move,psp,save[3],resist[5];
};
static const int elements[]={DAM_ACID,DAM_COLD,DAM_ELECTRIC,DAM_FIRE,DAM_SOUND};
static const int saves[]={SAVING_FORT,SAVING_REFL,SAVING_WILL};
static bool managed(int id){return (id>=LEG_F04&&id<=LEG_F07)||(id>=LEG_F09&&id<=LEG_F12);}
static unsigned long long plan_hash(const struct legendary_loadout *v)
{
  unsigned long long h=0;int i;
  for(i=0;i<v->count;i++) {
    const struct legendary_equipped_channel *e=&v->channels[i];
    if(e->reason==LEG_EQUIPPED&&e->item_passive)
      h=h*1099511628211ULL+(unsigned long long)(e->item_passive*10+e->rank);
    if(e->passive.reason==LEG_PASSIVE_ACTIVE&&managed(e->foundation))
      h=h*1099511628211ULL+(unsigned long long)(e->foundation*100000+e->subtype*100+e->passive.value);
  }
  return h;
}
static struct legendary_foundation_state *state(struct char_data *ch)
{
  if(!ch->legendary_foundations)ch->legendary_foundations=calloc(1,sizeof(*ch->legendary_foundations));
  return ch->legendary_foundations;
}
int legendary_foundation_value(struct char_data *ch,int id,int subtype)
{
  const struct legendary_loadout *v;int i,total=0;
  if(!ch||IS_NPC(ch))return 0;
  v=legendary_loadout_get(ch);if(!v->ready)return 0;
  for(i=0;i<v->count;i++)if(v->channels[i].foundation==id&&v->channels[i].subtype==subtype&&
    v->channels[i].passive.reason==LEG_PASSIVE_ACTIVE)total+=v->channels[i].passive.value;
  return total;
}
int legendary_foundation_resistance_base(struct char_data *ch,int type)
{
  int i,value=GET_RESISTANCES(ch,type);
  if(ch->legendary_foundations)for(i=0;i<5;i++)if(elements[i]==type)value-=ch->legendary_foundations->resist[i];
  return value;
}
void legendary_foundations_sync(struct char_data *ch,const struct legendary_loadout *v)
{
  struct legendary_foundation_state *s;unsigned long long hash;
  if(!ch||IS_NPC(ch)||!v)return;
  s=ch->legendary_foundations;if(s&&(s->busy||s->syncing))return;
  hash=plan_hash(v);if(!s&&!hash)return;
  if(!s&&!(s=state(ch)))return;
  if(hash==s->plan)return;
  s->syncing=true;affect_total(ch);s->syncing=false;
}
void legendary_foundations_refresh(struct char_data *ch){if(ch&&!IS_NPC(ch))(void)legendary_loadout_get(ch);}
bool legendary_foundations_begin(struct char_data *ch)
{
  int i;if(!ch||IS_NPC(ch))return true;
  if(!ch->legendary_foundations)for(i=0;i<NUM_WEARS;i++)if(GET_EQ(ch,i)&&*GET_EQ(ch,i)->legendary_uuid){state(ch);break;}
  if(ch->legendary_foundations) {
    if(ch->legendary_foundations->busy){ch->legendary_foundations->rerun=true;return false;}
    ch->legendary_foundations->busy=true;
  }
  return true;
}
void legendary_foundations_reset(struct char_data *ch)
{
  struct legendary_foundation_state *s=ch->legendary_foundations;
  if(!s)return;
  memset(s->ability,0,sizeof(s->ability));memset(s->save,0,sizeof(s->save));memset(s->resist,0,sizeof(s->resist));s->move=s->psp=0;
}
static int ability(struct char_data *ch,int id,int add)
{
  switch(id){case 0:ch->aff_abils.str+=add;return ch->aff_abils.str;
    case 1:ch->aff_abils.dex+=add;return ch->aff_abils.dex;
    case 2:ch->aff_abils.con+=add;return ch->aff_abils.con;
    case 3:ch->aff_abils.intel+=add;return ch->aff_abils.intel;
    case 4:ch->aff_abils.wis+=add;return ch->aff_abils.wis;
    default:ch->aff_abils.cha+=add;return ch->aff_abils.cha;}
}
void legendary_foundations_apply(struct char_data *ch)
{
  struct legendary_foundation_state *s;const struct legendary_loadout *v;
  int base[6],base_save[3],base_move,base_psp,i,amount;bool changed=false;
  if(!ch||IS_NPC(ch))return;
  v=legendary_loadout_get(ch);if(!ch->legendary_foundations&&!plan_hash(v))return;
  s=state(ch);if(!s)return;s->plan=plan_hash(v);
  base_move=GET_MAX_MOVE(ch);base_psp=GET_MAX_PSP(ch);
  for(i=0;i<6;i++){base[i]=ability(ch,i,0);amount=legendary_foundation_value(ch,LEG_F07,i);ability(ch,i,amount);if(amount)changed=true;}
  for(i=0;i<3;i++){base_save[i]=GET_SAVE(ch,saves[i]);amount=legendary_foundation_value(ch,LEG_F04+i,0)+legendary_item_passive_value(ch,14+i);GET_SAVE(ch,saves[i])+=amount;if(amount)changed=true;}
  amount=legendary_foundation_value(ch,LEG_F10,0)+legendary_item_passive_value(ch,3);GET_MAX_MOVE(ch)+=amount;if(amount)changed=true;
  amount=legendary_resource_class(ch)?legendary_foundation_value(ch,LEG_F11,0)+legendary_item_passive_value(ch,2):0;GET_MAX_PSP(ch)+=amount;if(amount)changed=true;
  for(i=0;i<5;i++) {
    amount=legendary_foundation_value(ch,LEG_F12,i);
    amount=MIN(amount,MAX(0,75-GET_RESISTANCES(ch,elements[i])));
    GET_RESISTANCES(ch,elements[i])+=amount;s->resist[i]=amount;
  }
  if(changed)compute_char_cap(ch,0);
  for(i=0;i<6;i++)s->ability[i]=ability(ch,i,0)-base[i];
  for(i=0;i<3;i++)s->save[i]=GET_SAVE(ch,saves[i])-base_save[i];
  s->move=GET_MAX_MOVE(ch)-base_move;s->psp=GET_MAX_PSP(ch)-base_psp;
}
void legendary_foundations_end(struct char_data *ch)
{
  if(!ch||IS_NPC(ch)||!ch->legendary_foundations)return;
  /* Raising a maximum never raises the corresponding current pool. */
  if(!ch->legendary_serializing){GET_HIT(ch)=MIN(GET_HIT(ch),GET_MAX_HIT(ch));GET_MOVE(ch)=MIN(GET_MOVE(ch),GET_MAX_MOVE(ch));GET_PSP(ch)=MIN(GET_PSP(ch),GET_MAX_PSP(ch));}
  if(ch->legendary_foundations) {
    bool again=ch->legendary_foundations->rerun;
    ch->legendary_foundations->rerun=false;ch->legendary_foundations->busy=false;
    if(again)affect_total(ch);
  }
}
void legendary_foundations_free(struct char_data *ch)
{if(ch){free(ch->legendary_foundations);ch->legendary_foundations=NULL;}}
int legendary_foundation_actual(struct char_data *ch,int id,int subtype)
{
  struct legendary_foundation_state *s;legendary_foundations_refresh(ch);s=ch->legendary_foundations;
  if(id==LEG_F03){(void)compute_armor_class(NULL,ch,false,MODE_ARMOR_CLASS_NORMAL);return ch->legendary_foundations?ch->legendary_foundations->deflection:0;}
  if(id==LEG_F07)return s&&subtype>=0&&subtype<6?s->ability[subtype]:0;
  if(id>=LEG_F04&&id<=LEG_F06)return s?s->save[id-LEG_F04]:0;
  if(id==LEG_F10)return s?s->move:0;
  if(id==LEG_F11)return s?s->psp:0;
  if(id==LEG_F12)return s&&subtype>=0&&subtype<5?s->resist[subtype]:0;
  return legendary_foundation_value(ch,id,subtype);
}
int legendary_foundation_channel_actual(struct char_data *ch,const char *uuid,int channel)
{
  const struct legendary_loadout *v=legendary_loadout_get(ch);int i,id=0,sub=0,remaining,amount;
  for(i=0;i<v->count;i++)if(v->channels[i].channel==channel&&!strcmp(v->channels[i].uuid,uuid)) {
    id=v->channels[i].foundation;sub=v->channels[i].subtype;break;
  }
  if(!id)return 0;
  remaining=legendary_foundation_actual(ch,id,sub);
  for(i=0;i<v->count;i++) {
    const struct legendary_equipped_channel *e=&v->channels[i];
    if(e->foundation!=id||e->subtype!=sub||e->passive.reason!=LEG_PASSIVE_ACTIVE)continue;
    amount=MIN(remaining,e->passive.value);remaining-=amount;
    if(e->channel==channel&&!strcmp(e->uuid,uuid))return amount;
  }
  return 0;
}
int legendary_foundation_deflection(struct char_data *ch,int existing)
{
  struct affected_type *a;struct obj_data *o;int i,j,penalty=0;
  int value=legendary_foundation_value(ch,LEG_F03,0),result;
  if(!value){if(ch->legendary_foundations)ch->legendary_foundations->deflection=0;return existing;}
  /* Preserve penalties while competing only with positive deflection. */
  for(a=ch->affected;a;a=a->next)if(a->bonus_type==BONUS_TYPE_DEFLECTION&&a->modifier<0) {
    if(a->location==APPLY_AC_NEW)penalty+=a->modifier;
    else if(a->location==APPLY_AC)penalty+=a->modifier/10;
  }
  for(i=0;i<NUM_WEARS;i++)if((o=GET_EQ(ch,i)))for(j=0;j<MAX_OBJ_AFFECT;j++) {
    if(o->affected[j].bonus_type!=BONUS_TYPE_DEFLECTION||o->affected[j].modifier>=0)continue;
    if(o->affected[j].location==APPLY_AC_NEW)penalty+=o->affected[j].modifier;
    else if(o->affected[j].location==APPLY_AC)penalty+=o->affected[j].modifier/10;
  }
  result=MAX(existing-penalty,value)+penalty;
  if(state(ch))ch->legendary_foundations->deflection=result-existing;
  return result;
}
int legendary_foundation_weapon(struct char_data *ch,struct obj_data *weapon,bool damage)
{
  if(!weapon||(GET_OBJ_TYPE(weapon)!=ITEM_WEAPON&&GET_OBJ_TYPE(weapon)!=ITEM_FIREWEAPON))return 0;
  return legendary_foundation_value(ch,damage?LEG_F02:LEG_F01,0);
}
int legendary_foundation_skill(struct char_data *ch,int skill)
{
  static const int skills[]={ABILITY_PERCEPTION,ABILITY_STEALTH,ABILITY_CONCENTRATION,ABILITY_HEAL,ABILITY_SPELLCRAFT,ABILITY_USE_MAGIC_DEVICE};
  int i,value=0;if(!legendary_trained_skill(ch,skill))return 0;
  value=legendary_foundation_value(ch,LEG_F08,skill);
  for(i=0;i<6;i++)if(skill==skills[i])value+=legendary_item_passive_value(ch,6+i);
  return value;
}
int legendary_foundation_maneuver(struct char_data *ch,int maneuver)
{
  switch(maneuver){case COMBAT_MANEUVER_TYPE_KNOCKDOWN:case COMBAT_MANEUVER_TYPE_FORCED_MOVEMENT:
    return legendary_foundation_value(ch,LEG_F13,0)+legendary_item_passive_value(ch,13);
    case COMBAT_MANEUVER_TYPE_DISARM:return legendary_foundation_value(ch,LEG_F14,0)+legendary_item_passive_value(ch,12);default:return 0;}
}
int legendary_foundation_walk(struct char_data *ch,int cost,bool mounted)
{
  int reduction;
  if(mounted||cost<=0)return cost;
  reduction=legendary_foundation_value(ch,LEG_F15,0)+legendary_item_passive_value(ch,5);
  return MIN(reduction?MAX(1,(int)((int64_t)cost*(100-reduction)/100)):cost,legendary_technique_walk(ch,cost,mounted));
}
int legendary_foundation_regen(struct char_data *ch)
{return !FIGHTING(ch)?legendary_foundation_value(ch,LEG_F16,0)+legendary_item_passive_value(ch,4):0;}
void legendary_foundation_hp(struct char_data *ch,int ordinary_max)
{
  int bonus=legendary_foundation_value(ch,LEG_F09,0)+legendary_item_passive_value(ch,1);
  GET_MAX_HIT(ch)=ordinary_max+bonus;
  if(!ch->legendary_serializing&&(bonus||ch->legendary_foundations))GET_HIT(ch)=MIN(GET_HIT(ch),GET_MAX_HIT(ch));
}
