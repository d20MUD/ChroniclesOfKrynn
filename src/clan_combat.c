#include "clan_combat.h"
#include <limits.h>
#include <string.h>
static int owner(const struct clan_combat_encounter *e,int index)
{
  size_t depth;
  for(depth=0;depth<e->actor_count;depth++) {
    if(index<0||(size_t)index>=e->actor_count)return -1;
    if(e->actors[index].owner==-1)return index;
    index=e->actors[index].owner;
  }
  return -1;
}
static int64_t add(int64_t a,int64_t b){return b>INT64_MAX-a?INT64_MAX:a+b;}
static int64_t threshold(int64_t base,int percent)
{return base/100*percent+(base%100*percent+99)/100;}
static bool recent(const struct clan_combat_encounter *e,const struct clan_combat_action *a,int64_t window)
{return a->at>=e->started_at&&a->at<=e->occurred_at&&e->occurred_at-a->at<=window&&a->room==e->room;}
bool clan_combat_query(const struct clan_progression_config *config,const struct clan_combat_encounter *e,
    struct clan_combat_result *out,size_t capacity)
{
  struct clan_combat_result result[CLAN_COMBAT_MAX_ACTORS];
  const struct clan_participation_rules *rules;
  size_t i,j,k;int highest=0,owners[CLAN_COMBAT_MAX_ACTORS];
  if(!e||!out||!e->actors||!e->actor_count||e->actor_count>CLAN_COMBAT_MAX_ACTORS||capacity<e->actor_count||
      e->action_count>CLAN_COMBAT_MAX_ACTIONS||(e->action_count&&!e->actions)||e->room<0||e->started_at<0||
      e->occurred_at<e->started_at||e->start_max_hp<=0||e->victim_level<1||e->victim_prototype<=0||
      !clan_progression_uid_valid(e->encounter_uid)||
      clan_progression_config_validate(config,NULL)!=CLAN_CONFIG_OK)return false;
  rules=&config->participation;
  memset(result,0,sizeof(result));
  for(i=0;i<e->actor_count;i++) {
    owners[i]=owner(e,(int)i);
    if(owners[i]<0||e->actors[i].level<1||e->actors[i].non_clan_max_hp<=0||
        !memchr(e->actors[i].clan_uid,0,33))return false;
    if(e->actors[i].participated) {
      int level=e->actors[owners[i]].level;if(level>highest)highest=level;
    }
    /* Two records for the same player are corrupt, not two contribution claims. */
    if(e->actors[i].owner==-1)for(j=0;j<i;j++)
      if(e->actors[j].owner==-1&&e->actors[i].player_id>0&&e->actors[j].player_id==e->actors[i].player_id)return false;
  }
  for(i=0;i<e->action_count;i++) {
    const struct clan_combat_action *a=&e->actions[i];int resolved=a->actor>=0&&(size_t)a->actor<e->actor_count?owners[a->actor]:-1;
    if(resolved<0||a->kind<CLAN_COMBAT_DAMAGE||a->kind>CLAN_COMBAT_HOSTILE_HIT||!a->application||
        a->at<e->started_at||a->at>e->occurred_at||a->round<0||a->amount<0||a->available<0||
        a->ally< -1||(a->ally>=0&&(size_t)a->ally>=e->actor_count))return false;
    if(a->room==e->room&&e->actors[resolved].level>highest)highest=e->actors[resolved].level;
  }
  for(i=0;i<e->actor_count;i++) {
    const struct clan_combat_actor *p=&e->actors[i];
    int64_t damage=0,healing[CLAN_COMBAT_MAX_ACTORS]={0},mitigation[CLAN_COMBAT_MAX_ACTORS]={0};
    int64_t rounds[CLAN_COMBAT_MAX_ACTIONS];uint64_t applications[CLAN_COMBAT_MAX_ACTIONS];
    size_t useful=0,hits=0;bool meaningful=false;
    struct clan_combat_result *r=&result[i];r->player_id=p->player_id;r->account_id=p->account_id;r->highest_level=highest;strcpy(r->clan_uid,p->clan_uid);
    if(!e->normal_death||!e->world_npc||e->owned||e->charmed||e->summoned||e->pet||e->test||e->staff_test||e->manipulated)r->reasons|=CLAN_COMBAT_BAD_VICTIM;
    if(e->victim_level<(highest>rules->victim_level_tolerance?highest-rules->victim_level_tolerance:1))r->reasons|=CLAN_COMBAT_TRIVIAL;
    if(p->owner!=-1||p->player_id<=0||p->account_id<=0||!p->accepted||!clan_progression_uid_valid(p->clan_uid))r->reasons|=CLAN_COMBAT_IDENTITY;
    if(!p->alive||!p->playing||p->room!=e->room)r->reasons|=CLAN_COMBAT_INACTIVE;
    if(highest-p->level>rules->member_level_tolerance)r->reasons|=CLAN_COMBAT_LEVEL;
    for(j=0;j<e->action_count;j++) {
      const struct clan_combat_action *a=&e->actions[j];int64_t amount;bool duplicate=false;
      if(owners[a->actor]!=(int)i||!recent(e,a,rules->recent_seconds))continue;
      /* Repeated packets from a single application cannot inflate any total. */
      for(k=0;k<j;k++)if(owners[e->actions[k].actor]==(int)i&&e->actions[k].application==a->application&&
          ((e->actions[k].kind==a->kind&&e->actions[k].ally==a->ally)||
           ((a->kind==CLAN_COMBAT_SUPPORT||a->kind==CLAN_COMBAT_DEFENSE)&&
            (e->actions[k].kind==CLAN_COMBAT_SUPPORT||e->actions[k].kind==CLAN_COMBAT_DEFENSE)))){duplicate=true;break;}
      if(duplicate)continue;
      amount=a->amount<a->available?a->amount:a->available;
      if(a->kind==CLAN_COMBAT_DAMAGE)damage=add(damage,amount);
      else if(a->kind==CLAN_COMBAT_HOSTILE_HIT) {if(a->ally_engaged&&amount>0)hits++;}
      else if(a->kind==CLAN_COMBAT_HEAL||a->kind==CLAN_COMBAT_MITIGATE) {
        if(a->ally<0||!a->ally_engaged)continue;
        if(a->kind==CLAN_COMBAT_HEAL)healing[a->ally]=add(healing[a->ally],amount);
        else mitigation[a->ally]=add(mitigation[a->ally],amount);
      } else if(a->useful&&(a->kind==CLAN_COMBAT_SUPPORT||(a->ally>=0&&a->ally_engaged))) {
        for(k=0;k<useful;k++)if(rounds[k]==a->round||applications[k]==a->application)break;
        if(k==useful){rounds[useful]=a->round;applications[useful++]=a->application;}
      }
    }
    meaningful=damage>=threshold(e->start_max_hp,rules->damage_percent)||useful>=(size_t)rules->useful_actions||hits>=2;
    for(j=0;j<e->actor_count;j++)if(healing[j]>=threshold(e->actors[j].non_clan_max_hp,rules->healing_percent)||
        mitigation[j]>=threshold(e->actors[j].non_clan_max_hp,rules->mitigation_percent))meaningful=true;
    if(!meaningful)r->reasons|=CLAN_COMBAT_NO_PARTICIPATION;
  }
  /* One deterministic eligible character represents each authoritative account. */
  for(i=0;i<e->actor_count;i++)if(!result[i].reasons)
    for(j=0;j<e->actor_count;j++)if(i!=j&&!(result[j].reasons&~CLAN_COMBAT_ACCOUNT_DUPLICATE)&&
        result[i].account_id==result[j].account_id&&result[j].player_id<result[i].player_id)
      result[i].reasons|=CLAN_COMBAT_ACCOUNT_DUPLICATE;
  memcpy(out,result,e->actor_count*sizeof(*out));return true;
}
