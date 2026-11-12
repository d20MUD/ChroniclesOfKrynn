#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "mysql.h"
#include "clan_sources.h"
#include "clan_objective_runtime.h"
#include "clan_expedition_enrollment.h"
#include "clan_awards.h"
#include "clan_status.h"
#include "clan_buff_runtime.h"
#include "clan_persistence.h"
#include "legendary_identity.h"
#include <json-c/json.h>
/* Bounded live facts; no actor pointers survive a call. Victims are removed by
 * free_char, avoiding pointer reuse. Overflow fails the entire encounter closed. */
#define LIVE_ENCOUNTERS 128
struct encounter {
  struct char_data *victim;
  struct clan_combat_encounter facts;
  struct clan_combat_actor actors[CLAN_COMBAT_MAX_ACTORS];
  struct clan_combat_action actions[CLAN_COMBAT_MAX_ACTIONS];
  bool overflow;
  int64_t last;
  struct encounter *next;
};
static struct encounter *encounters;
static uint64_t application;
static bool collecting(void)
{
  const struct clan_progression_config *c=clan_buff_runtime_policy();
  return clan_progression_campaign_supported() && c->controls.mode==CLAN_PROGRESSION_LIVE && c->controls.store_mode==CLAN_STORE_MYSQL &&
    clan_persistence_sql_authority();
}
static bool enabled(void)
{return collecting()&&clan_persistence_ready()&&mysql_available;}
static struct char_data *owner(struct char_data *ch)
{
  int n=0;
  if(ch&&IS_NPC(ch)&&!ch->master&&ch->confuser_idnum>0) {
    struct char_data *pc;
    for(pc=character_list;pc;pc=pc->next)
      if(!IS_NPC(pc)&&GET_IDNUM(pc)==ch->confuser_idnum){ch=pc;break;}
  }
  while(ch&&IS_NPC(ch)&&ch->master&&n++<8)ch=ch->master;
  return ch&&!IS_NPC(ch)?ch:NULL;
}
static bool real_victim(struct char_data *v)
{
  return v&&IS_MOB(v)&&GET_MOB_VNUM(v)>0&&!v->master&&!IS_PET(v)&&
    !AFF_FLAGGED(v,AFF_CHARM)&&!v->legendary_test_encounter&&!v->legendary_owned_instance&&
    !MOB_FLAGGED(v,MOB_C_FAMILIAR)&&!MOB_FLAGGED(v,MOB_C_ANIMAL)&&
    !MOB_FLAGGED(v,MOB_C_MOUNT)&&!MOB_FLAGGED(v,MOB_EIDOLON)&&
    !MOB_FLAGGED(v,MOB_GOLEM)&&!MOB_FLAGGED(v,MOB_SUMMON_SOLAR);
}
static int actor(struct encounter *e,struct char_data *ch,bool participated)
{
  size_t i;ch=owner(ch);if(!ch||GET_IDNUM(ch)<=0)return -1;
  for(i=0;i<e->facts.actor_count;i++)if(e->actors[i].player_id==GET_IDNUM(ch))break;
  if(i==CLAN_COMBAT_MAX_ACTORS){e->overflow=true;return -1;}
  if(i==e->facts.actor_count) {
    struct clan_combat_actor *a=&e->actors[e->facts.actor_count++];
    a->owner=-1;a->player_id=GET_IDNUM(ch);a->level=GET_LEVEL(ch);
    a->non_clan_max_hp=MAX(1,GET_MAX_HIT(ch)-ch->clan_vitality_applied);
  }
  if(participated)e->actors[i].participated=true;
  if(GET_LEVEL(ch)>e->actors[i].level)e->actors[i].level=GET_LEVEL(ch);
  if(GET_LEVEL(ch)>=LVL_IMMORT)e->facts.staff_test=true;
  return (int)i;
}
static struct encounter *find(struct char_data *v,bool create)
{
  struct encounter *e;int count=0;
  for(e=encounters;e;e=e->next){if(e->victim==v)return e;count++;}
  if(!create||count>=LIVE_ENCOUNTERS||!real_victim(v)||GET_MAX_HIT(v)<=0)return NULL;
  e=calloc(1,sizeof(*e));if(!e)return NULL;
  if(!legendary_uuid_generate(e->facts.encounter_uid)){free(e);return NULL;}
  e->victim=v;e->facts.started_at=time(NULL);e->last=e->facts.started_at;
  e->facts.start_max_hp=GET_MAX_HIT(v);e->facts.victim_level=GET_LEVEL(v);
  e->facts.victim_prototype=GET_MOB_VNUM(v);e->facts.world_npc=true;
  e->facts.actors=e->actors;e->facts.actions=e->actions;
  e->next=encounters;encounters=e;return e;
}
void clan_source_forget(struct char_data *v)
{
  struct encounter **p=&encounters,*e;
  while(*p){e=*p;if(e->victim==v){*p=e->next;free(e);return;}p=&e->next;}
}
void clan_source_action(struct char_data *ch,struct char_data *v,
    enum clan_combat_action_kind kind,struct char_data *ally,int amount)
{
  struct encounter *e;struct char_data *p,*o=owner(ch);int a,b=-1;int64_t now=time(NULL);
  if(!collecting()||amount<=0||!o||!real_victim(v)||IN_ROOM(v)==NOWHERE||IN_ROOM(ch)!=IN_ROOM(v))return;
  if(!(e=find(v,true))||e->overflow)return;
  /* Include engaged outsiders for the difficulty ceiling even without membership. */
  for(p=world[IN_ROOM(v)].people;p;p=p->next_in_room)
    if(FIGHTING(p)==v)actor(e,p,true);
  if(ally) {
    struct char_data *supported=owner(ally);
    if(!supported||IN_ROOM(supported)!=IN_ROOM(v)||GET_HIT(supported)<=0||
        (o!=supported&&(!GROUP(o)||GROUP(o)!=GROUP(supported))))return;
    if(FIGHTING(ally)!=v&&FIGHTING(supported)!=v&&FIGHTING(v)!=ally&&FIGHTING(v)!=supported)return;
    b=actor(e,supported,true);if(b<0)return;
  }
  a=actor(e,o,true);if(a<0)return;
  if(e->facts.action_count==CLAN_COMBAT_MAX_ACTIONS) {
    size_t read,write=0;
    int64_t window=clan_buff_runtime_policy()->participation.recent_seconds;
    /* Old actions cannot affect eligibility. Keep owners/H for the entire
     * encounter, but let long fights reuse action storage after recency expiry. */
    for(read=0;read<e->facts.action_count;read++)
      if(now-e->actions[read].at<=window)e->actions[write++]=e->actions[read];
    e->facts.action_count=write;
    if(write==CLAN_COMBAT_MAX_ACTIONS){e->overflow=true;return;}
  }
  struct clan_combat_action *event=&e->actions[e->facts.action_count++];
  event->actor=a;event->ally=b;event->kind=kind;event->application=++application;
  event->at=now;event->round=now/6;event->room=IN_ROOM(o)==IN_ROOM(v)?IN_ROOM(v):NOWHERE;event->amount=amount;
  event->available=amount;event->useful=true;event->ally_engaged=ally!=NULL;
  e->last=now;
}
void clan_source_support(struct char_data *ch,struct char_data *ally,
    enum clan_combat_action_kind kind,int amount)
{
  struct char_data *v;
  if(!collecting()||!ally||IN_ROOM(ally)==NOWHERE||amount<=0)return;
  for(v=world[IN_ROOM(ally)].people;v;v=v->next_in_room)
    if(real_victim(v))clan_source_action(ch,v,kind,ally,amount);
}
void clan_source_damage(struct char_data *ch,struct char_data *v,int amount,bool attack)
{
  if(amount<=0||!ch||!v)return;
  clan_source_action(ch,v,CLAN_COMBAT_DAMAGE,NULL,MIN(amount,MAX(0,GET_HIT(v))));
  if(attack&&real_victim(ch)&&owner(v))
    clan_source_action(v,ch,CLAN_COMBAT_HOSTILE_HIT,v,MIN(amount,MAX(0,GET_HIT(v))));
}
static bool identity(MYSQL *db,struct char_data *ch,int64_t now,char uid[33])
{
  struct clan_status_report report;
  const struct clan_progression_config *c=clan_buff_runtime_policy();
  struct clan_progression_readiness ready={.schema_ready=true,.schema_version=CLAN_PROGRESSION_SCHEMA_VERSION};
  if(!ch||!ch->desc||STATE(ch->desc)!=CON_PLAYING||!ch->desc->account||ch->desc->account->id<=0)return false;
  if(clan_repository_status_read(db,GET_IDNUM(ch),ch->desc->account->id,NULL,false,now,&report)!=CLAN_REPO_OK||
      !report.full_clan_access||report.membership!=CLAN_ACCESS_ACCEPTED||
      !clan_progression_feature_available(c,CLAN_FEATURE_EARNINGS,&ready,report.clan.uid))return false;
  strcpy(uid,report.clan.uid);return true;
}
static void complete(MYSQL *db,const char *uid,int64_t now)
{
  struct json_object *receipt=NULL;
  struct clan_progression_readiness ready={.schema_ready=true,.schema_version=CLAN_PROGRESSION_SCHEMA_VERSION};
  struct json_object *existing=NULL,*event=NULL,*source=NULL;
  if(clan_reward_load(db,uid,&existing)==CLAN_REPO_OK) {
    json_object_object_get_ex(existing,"event",&event);
    if(event)json_object_object_get_ex(event,"source",&source);
    int kind=json_object_get_int(source);
    if(kind==CLAN_SOURCE_DELIVERY) {json_object_put(existing);return;}
    if(kind==CLAN_SOURCE_GATHERING||kind==CLAN_SOURCE_BOUNTY) {
      struct json_object *objective=NULL,*clan_uid=NULL;
      json_object_object_get_ex(event,"clan_uid",&clan_uid);
      if(!clan_progression_feature_available(clan_buff_runtime_policy(),CLAN_FEATURE_EARNINGS,&ready,json_object_get_string(clan_uid))) {json_object_put(existing);return;}
      clan_objective_reward_complete(db,uid,now,&objective);
      if(objective)json_object_put(objective);
      json_object_put(existing);return;
    }
    if(kind==CLAN_SOURCE_EXPEDITION_PERSONAL||kind==CLAN_SOURCE_ASSAULT_SHARED||kind==CLAN_SOURCE_HUNT_SHARED) {
      struct json_object *expedition=NULL;
      clan_expedition_reward_complete(db,uid,now,clan_buff_runtime_policy(),&expedition);
      if(expedition)json_object_put(expedition);
      json_object_put(existing);return;
    }
    if(kind==CLAN_SOURCE_BOUNTY_SHARED) {
      struct json_object *shared=NULL;
      clan_award_complete_shared(db,uid,now,clan_buff_runtime_policy(),&ready,&shared);
      if(shared)json_object_put(shared);
      json_object_put(existing);return;
    }
    json_object_put(existing);
  }
  enum clan_repository_result r=clan_award_complete_personal(db,uid,now,clan_buff_runtime_policy(),&ready,&receipt);
  if(receipt)json_object_put(receipt);
  if(r!=CLAN_REPO_OK&&r!=CLAN_REPO_COMMIT_UNKNOWN)
    log("CLAN: accepted source %s remains pending (%s)",uid,clan_repository_result_name(r));
}
static void prepared(MYSQL *db,struct json_object *receipt,int64_t now)
{
  struct json_object *e,*uid;
  if(receipt&&json_object_object_get_ex(receipt,"event",&e)&&json_object_object_get_ex(e,"operation_uid",&uid))
    complete(db,json_object_get_string(uid),now);
  if(receipt)json_object_put(receipt);
}
void clan_source_death(struct char_data *v,struct char_data *killer)
{
  struct encounter *e=find(v,false);struct char_data *ch;MYSQL_POOL_CONN *connection;
  struct clan_combat_result results[CLAN_COMBAT_MAX_ACTORS];size_t i;int64_t now=time(NULL);
  struct clan_objective_event bounties[CLAN_COMBAT_MAX_ACTORS]={0};
  if(!e)return;
  if(!enabled()||e->overflow||!real_victim(v)||IN_ROOM(v)==NOWHERE)goto done;
  if(owner(killer)&&GET_LEVEL(owner(killer))>=LVL_IMMORT)e->facts.staff_test=true;
  if(!(connection=mysql_pool_acquire()))goto done;
  e->facts.occurred_at=now;e->facts.room=IN_ROOM(v);e->facts.normal_death=true;
  for(i=0;i<e->facts.actor_count;i++) {
    struct clan_combat_actor *a=&e->actors[i];a->room=-1;
    for(ch=character_list;ch;ch=ch->next)if(!IS_NPC(ch)&&GET_IDNUM(ch)==a->player_id)break;
    if(!ch)continue;
    a->room=IN_ROOM(ch);a->alive=GET_HIT(ch)>0&&GET_POS(ch)>POS_DEAD;
    a->playing=ch->desc&&STATE(ch->desc)==CON_PLAYING;
    if(identity(connection->conn,ch,now,a->clan_uid)) {
      a->account_id=ch->desc->account->id;a->accepted=true;
    }
  }
  if (clan_buff_runtime_policy()->controls.expeditions)
    clan_expedition_combat(connection->conn,&e->facts,zone_table[world[IN_ROOM(v)].zone].number,
                          now,clan_buff_runtime_policy());
  if(clan_combat_query(clan_buff_runtime_policy(),&e->facts,results,CLAN_COMBAT_MAX_ACTORS))
    for(i=0;i<e->facts.actor_count;i++)if(!results[i].reasons) {
      struct clan_reward_event event={0};struct json_object *receipt=NULL;
      for(ch=character_list;ch;ch=ch->next)if(!IS_NPC(ch)&&GET_IDNUM(ch)==results[i].player_id)break;
      if(ch)clan_objective_bounty_capture(connection->conn,ch,e->facts.victim_prototype,
          e->facts.encounter_uid,results[i].clan_uid,now,&bounties[i]);
      if(!legendary_uuid_generate(event.operation_uid))continue;
      strcpy(event.event_uid,e->facts.encounter_uid);strcpy(event.clan_uid,results[i].clan_uid);
      event.account_id=results[i].account_id;event.player_id=results[i].player_id;event.occurred_at=now;
      event.source=CLAN_SOURCE_KILL;event.rules_version=1;
      event.requested_xp_eighths=16;event.requested_influence_eighths=8;event.requested_favor_eighths=8;
      enum clan_repository_result r=clan_reward_prepare_kill(connection->conn,&event,e->facts.victim_prototype,now,&receipt);
      if(r==CLAN_REPO_OK)prepared(connection->conn,receipt,now);
      else if(receipt)json_object_put(receipt);
    }
  mysql_pool_release(connection);clan_persistence_refresh_progression();
  for(i=0;i<e->facts.actor_count;i++)if(*bounties[i].contract_uid) {
    for(ch=character_list;ch;ch=ch->next)if(!IS_NPC(ch)&&GET_IDNUM(ch)==bounties[i].player_id)break;
    if(ch)clan_objective_bounty_record(ch,&bounties[i]);
  }
done:
  clan_source_forget(v);
}
void clan_source_supply_identity(struct char_data *ch,char uid[33])
{
  MYSQL_POOL_CONN *connection;uid[0]=0;
  if(!enabled()||!ch||IS_NPC(ch)||GET_CRAFT(ch).clan_supply_event[0]||!(connection=mysql_pool_acquire()))return;
  identity(connection->conn,ch,time(NULL),uid);mysql_pool_release(connection);
}
static void supply_pending(struct char_data *ch,int64_t now)
{
  MYSQL_POOL_CONN *connection;struct json_object *receipt=NULL;struct clan_reward_event event={0};
  if(!GET_CRAFT(ch).clan_supply_event[0]||!clan_progression_uid_valid(GET_CRAFT(ch).clan_supply_event)||
      !clan_progression_uid_valid(GET_CRAFT(ch).clan_supply_uid)||GET_CRAFT(ch).clan_supply_account<=0||
      GET_CRAFT(ch).clan_supply_at<=0||GET_CRAFT(ch).clan_supply_at>now||
      !clan_progression_feature_available(clan_buff_runtime_policy(),CLAN_FEATURE_EARNINGS,
        &(struct clan_progression_readiness){.schema_ready=true,.schema_version=CLAN_PROGRESSION_SCHEMA_VERSION},
        GET_CRAFT(ch).clan_supply_uid)||
      !save_char_checked(ch,0)||!(connection=mysql_pool_acquire()))return;
  strcpy(event.operation_uid,GET_CRAFT(ch).clan_supply_event);strcpy(event.event_uid,event.operation_uid);
  strcpy(event.clan_uid,GET_CRAFT(ch).clan_supply_uid);event.account_id=GET_CRAFT(ch).clan_supply_account;
  event.player_id=GET_IDNUM(ch);event.occurred_at=GET_CRAFT(ch).clan_supply_at;
  event.source=CLAN_SOURCE_SUPPLY_ORDER;event.rules_version=1;
  event.requested_xp_eighths=800;event.requested_influence_eighths=400;event.requested_favor_eighths=400;
  /* Membership was captured at successful turn-in, before the durable player
   * checkpoint. A later departure must not change this saved occurrence. */
  enum clan_repository_result r=clan_reward_prepare(connection->conn,&event,now,&receipt);
  if(r==CLAN_REPO_OK) {
    prepared(connection->conn,receipt,now);
    GET_CRAFT(ch).clan_supply_event[0]=0;
  } else {
    if(receipt)json_object_put(receipt);
    /* A tombstoned incarnation can never accept this unaccepted occurrence.
     * Release the local handoff without redirecting it to a later clan. */
    if(r==CLAN_REPO_DELETED)GET_CRAFT(ch).clan_supply_event[0]=0;
  }
  mysql_pool_release(connection);clan_persistence_refresh_progression();
  if(r==CLAN_REPO_OK||r==CLAN_REPO_DELETED)save_char(ch,0); /* lost acknowledgement replays the saved event */
}
void clan_source_supply_complete(struct char_data *ch,const char uid[33])
{
  if(!enabled()||!ch||IS_NPC(ch)||!clan_progression_uid_valid(uid)||
      GET_CRAFT(ch).clan_supply_event[0]||!ch->desc||!ch->desc->account)return;
  if(!legendary_uuid_generate(GET_CRAFT(ch).clan_supply_event))return;
  strcpy(GET_CRAFT(ch).clan_supply_uid,uid);
  GET_CRAFT(ch).clan_supply_account=ch->desc->account->id;
  GET_CRAFT(ch).clan_supply_at=time(NULL);
  supply_pending(ch,time(NULL));
}
void clan_source_tick(int64_t now)
{
  static int64_t next;static char cursor[33];struct encounter **p=&encounters,*e;struct char_data *ch;
  while(*p) {
    e=*p;
    /* An injured abandoned mob still belongs to the old encounter: discarding
     * its high-level participants would let a later low-level finisher farm it.
     * Free only a genuinely healed, disengaged encounter (or on victim free). */
    bool idle=now-e->last>600&&GET_HIT(e->victim)>=GET_MAX_HIT(e->victim)&&!FIGHTING(e->victim);
    if(idle&&IN_ROOM(e->victim)!=NOWHERE) {
      struct char_data *participant;
      for(participant=world[IN_ROOM(e->victim)].people;participant;participant=participant->next_in_room)
        if(FIGHTING(participant)==e->victim){idle=false;break;}
    }
    if(idle){*p=e->next;free(e);}else p=&e->next;
  }
  if(now<next||!enabled())return;
  next=now+30;
  for(ch=character_list;ch;ch=ch->next)if(!IS_NPC(ch)&&ch->desc&&STATE(ch->desc)==CON_PLAYING)
    {clan_objective_flush(ch);supply_pending(ch,now);}
  MYSQL_POOL_CONN *connection=mysql_pool_acquire();struct json_object *pending=NULL;
  if(!connection)return;
  if(clan_reward_pending(connection->conn,0,cursor[0]?cursor:NULL,100,&pending)==CLAN_REPO_OK) {
    size_t i;
    if(!json_object_array_length(pending))cursor[0]=0;
    for(i=0;i<json_object_array_length(pending);i++) {
      strcpy(cursor,json_object_get_string(json_object_array_get_idx(pending,i)));
      complete(connection->conn,cursor,now);
    }
    json_object_put(pending);
  }
  mysql_pool_release(connection);clan_persistence_refresh_progression();
}
