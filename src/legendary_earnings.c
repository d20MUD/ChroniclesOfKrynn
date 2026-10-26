/* Phase 5. Authoritative source receipts, capped personal awards and participation. */
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "estate.h"
#include "legendary_identity.h"
#include "legendary_transaction.h"
#include "legendary_earnings.h"
#include <errno.h>
#include <limits.h>
#ifdef LEGENDARY_EARNINGS_TEST
int64_t legendary_earnings_test_time;
#endif
static int64_t now_utc(void)
{
#ifdef LEGENDARY_EARNINGS_TEST
  if(legendary_earnings_test_time)return legendary_earnings_test_time;
#endif
  return (int64_t)time(NULL);
}
static bool num(const char *s,int64_t *n)
{
  char *end; unsigned long long value;
  if(!s || !*s || *s<'0' || *s>'9')return false;
  errno=0;value=strtoull(s,&end,10);
  if(errno || *end || value>INT64_MAX)return false;
  *n=value;return true;
}
bool legendary_earnings_available(void)
{
  MYSQL_RES *r; MYSQL_ROW row; bool ok=false;
  static time_t checked_until;
  static bool cached;
  time_t now=time(NULL);
  /* A short readiness cache keeps normal combat and legacy servers off SQL.
   * Every actual ledger commit still revalidates the schema through Phase 4. */
  if(!conn || !mysql_available)return false;
  if(now<checked_until)return cached;
  r=estate_select("SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='legendary_schema'");
  if(r){row=mysql_fetch_row(r);ok=row && !strcmp(row[0],"1");mysql_free_result(r);}
  cached=ok && legendary_schema_ready();checked_until=now+5;return cached;
}
static bool combat_ready;
void legendary_earnings_maintenance(void)
{
  combat_ready=legendary_earnings_available();
}
/* Builders opt a true encounter into legendary materials with MOB_LEGENDARY_BOSS.
 * Tier comes from its native level and an eligible death rolls one of the six
 * essences.  The award is persisted before it is granted, so retries cannot
 * reroll it.  No mutable SQL registry can silently turn an unrelated mobile
 * into a material source. */
static bool combat_boss(const struct char_data *boss)
{return combat_ready&&boss&&IS_NPC(boss)&&MOB_FLAGGED(boss,MOB_LEGENDARY_BOSS);}
static int boss_tier(const struct char_data *boss)
{return GET_LEVEL(boss)>=30?3:GET_LEVEL(boss)>=25?2:1;}
static int boss_essence(const struct char_data *boss)
{(void)boss;return rand_number(0,LEGENDARY_ESSENCE_COUNT-1);}
bool legendary_balance_load(struct char_data *ch,struct legendary_balance *out)
{
  MYSQL_RES *r; MYSQL_ROW row; int i; int64_t n[14]; struct legendary_balance b;
  if(!ch || IS_NPC(ch) || GET_IDNUM(ch)<=0 || !out || !legendary_earnings_available())return false;
  if(!estate_query("INSERT IGNORE INTO legendary_characters(player_id) VALUES(%ld)",GET_IDNUM(ch)))return false;
  r=estate_select("SELECT infusion_xp,fragments,catalysts,draconic,martial,arcane,spiritual,primal,shadow,version,"
    "COALESCE(DATEDIFF(allowance_date,'1970-01-01'),0),fragments_today,essences_today,catalysts_today "
    "FROM legendary_characters WHERE player_id=%ld",GET_IDNUM(ch));
  if(!r)return false;
  row=mysql_fetch_row(r);
  for(i=0;row && i<14;i++)if(!num(row[i],&n[i]))break;
  mysql_free_result(r);if(!row || i!=14)return false;
  for(i=0;i<9;i++)if(n[i]>LEGENDARY_BALANCE_MAX)return false;
  if(n[9]==INT64_MAX || n[11]>100 || n[12]>20 || n[13]>1)return false;
  memset(&b,0,sizeof(b));b.wallet.xp=n[0];b.wallet.fragments=n[1];b.wallet.catalysts=n[2];
  b.wallet.gold=GET_GOLD(ch);for(i=0;i<6;i++)b.wallet.essences[i]=n[3+i];
  b.version=n[9];b.utc_day=now_utc()/86400;
  if(n[10]>b.utc_day)return false; /* Do not refill allowance on clock rollback. */
  if(n[10]==b.utc_day){b.fragments_today=n[11];b.essences_today=n[12];b.catalysts_today=n[13];}
  *out=b;return true;
}
static struct char_data *credit_owner(struct char_data *ch)
{
  int depth=0;
  while(ch && IS_NPC(ch) && ch->master && depth++<8)ch=ch->master;
  return ch && !IS_NPC(ch)?ch:NULL;
}
static bool recipient(struct char_data *ch)
{
  return ch && !IS_NPC(ch) && GET_LEVEL(ch)==30 && GET_HIT(ch)>0 && GET_POS(ch)>POS_DEAD &&
    ch->desc && STATE(ch->desc)==CON_PLAYING;
}
static bool real_victim(struct char_data *victim)
{
  return victim && IS_MOB(victim) && !victim->master && !AFF_FLAGGED(victim,AFF_CHARM) &&
    !MOB_FLAGGED(victim,MOB_C_FAMILIAR) && !MOB_FLAGGED(victim,MOB_C_ANIMAL) &&
    !MOB_FLAGGED(victim,MOB_C_MOUNT) && !MOB_FLAGGED(victim,MOB_EIDOLON) &&
    !MOB_FLAGGED(victim,MOB_GOLEM) && !MOB_FLAGGED(victim,MOB_SUMMON_SOLAR) &&
    !victim->legendary_test_encounter && !victim->legendary_owned_instance;
}
static bool instance_id(struct char_data *victim)
{
  return legendary_uuid_valid(victim->legendary_encounter_id) || legendary_uuid_generate(victim->legendary_encounter_id);
}
void legendary_xp_kill_context(struct legendary_xp_context *ctx,struct char_data *victim)
{
  memset(ctx,0,sizeof(*ctx));ctx->origin=LEG_XP_KILL;
  if(real_victim(victim) && instance_id(victim)) {
    memcpy(ctx->award_id,victim->legendary_encounter_id,33);ctx->source_vnum=GET_MOB_VNUM(victim);ctx->eligible=true;
  }
}
void legendary_xp_quest_context(struct legendary_xp_context *ctx,int vnum)
{
  memset(ctx,0,sizeof(*ctx));ctx->origin=LEG_XP_QUEST;ctx->source_vnum=vnum;
  ctx->eligible=vnum>=0 && legendary_uuid_generate(ctx->award_id);
}
void legendary_xp_trap_context(struct legendary_xp_context *ctx,struct trap_data *trap)
{
  memset(ctx,0,sizeof(*ctx));ctx->origin=LEG_XP_TRAP;
  if(trap && IS_SET(trap->flags,TRAP_FLAG_DISARMED) &&
    (legendary_uuid_valid(trap->legendary_disarm_id) || legendary_uuid_generate(trap->legendary_disarm_id))) {
    memcpy(ctx->award_id,trap->legendary_disarm_id,33);ctx->source_vnum=MAX(0,trap->trigger_vnum);ctx->eligible=true;
  }
}
struct award {
  char id[33]; int kind,vnum,essence; int64_t occurred,xp;
  int fragments,essences,catalysts;
};
struct grant_context {
  struct legendary_balance before;
  struct award award;
  int64_t received_xp,now;
  int fragments,essences,catalysts;
  bool exchange,quest_duplicate;
};
static int64_t cap(int64_t request,int64_t balance,int remaining)
{
  int64_t room=LEGENDARY_BALANCE_MAX-balance;
  if(request>room)request=room;
  if(remaining>=0 && request>remaining)request=remaining;
  return request;
}
/* One output cap implementation for boss grants and exchanges alike. */
static void cap_outputs(struct grant_context *g)
{
  const struct legendary_balance *b=&g->before;
  g->received_xp=cap(g->award.xp,b->wallet.xp,-1);
  g->fragments=cap(g->award.fragments,b->wallet.fragments,100-b->fragments_today);
  g->essences=cap(g->award.essences,b->wallet.essences[g->award.essence],20-b->essences_today);
  g->catalysts=cap(g->award.catalysts,b->wallet.catalysts,1-b->catalysts_today);
}
static bool exists(const char *sql,bool *result)
{
  MYSQL_RES *r=estate_select("%s",sql); MYSQL_ROW row;
  if(!r)return false;
  row=mysql_fetch_row(r);*result=row && row[0] && strcmp(row[0],"0");mysql_free_result(r);return row!=NULL;
}
static bool prior_quest(long player,int vnum,int64_t day,bool *found)
{
  char sql[512];
  snprintf(sql,sizeof(sql),"SELECT COUNT(*) FROM legendary_quest_xp_claims WHERE player_id=%ld AND quest_vnum=%d "
    "AND award_date=DATE_ADD('1970-01-01',INTERVAL %lld DAY)",player,vnum,(long long)day);
  return exists(sql,found);
}
static bool prior_boss(long player,int vnum,int64_t now,bool *found)
{
  char sql[512];
  snprintf(sql,sizeof(sql),"SELECT COUNT(*) FROM legendary_boss_claims WHERE player_id=%ld AND boss_vnum=%d AND "
    "awarded_utc>%lld AND (fragments>0 OR essences>0 OR catalysts>0)",player,vnum,(long long)(now>72000?now-72000:0));
  return exists(sql,found);
}
static bool grant_participant(struct char_data *ch,const char *operation,void *context)
{
  struct grant_context *g=context; MYSQL_RES *r; MYSQL_ROW row; int64_t version; bool ok=false,found;
  long id=GET_IDNUM(ch);
  if(now_utc()/86400!=g->before.utc_day)return false;
  r=estate_select("SELECT version FROM legendary_characters WHERE player_id=%ld",id);
  if(r){row=mysql_fetch_row(r);ok=row && num(row[0],&version) && version==g->before.version;mysql_free_result(r);}
  if(!ok)return false;
  if(!g->exchange) {
    /* Queue row is claimed in the same transaction as balances and counters. */
    if(!estate_query("UPDATE legendary_awards SET state='credited',operation_id='%s',received_xp=%lld,"
       "received_fragments=%d,received_essences=%d,received_catalysts=%d WHERE player_id=%ld AND "
       "source_kind=%d AND award_id='%s' AND state='pending'",operation,(long long)g->received_xp,
       g->fragments,g->essences,g->catalysts,id,g->award.kind,g->award.id) || mysql_affected_rows(conn)!=1)return false;
    if(g->award.kind==LEG_XP_QUEST && !g->quest_duplicate) {
      if(!estate_query("INSERT IGNORE INTO legendary_quest_xp_claims(player_id,quest_vnum,award_date,operation_id) "
        "VALUES(%ld,%d,DATE_ADD('1970-01-01',INTERVAL %lld DAY),'%s')",id,g->award.vnum,
        (long long)(g->award.occurred/86400),operation) || mysql_affected_rows(conn)!=1)return false;
    }
    if(g->award.kind==4) {
      if(g->fragments || g->essences || g->catalysts) {
        if(!prior_boss(id,g->award.vnum,g->now,&found) || found)return false;
      }
      if(!estate_query("INSERT IGNORE INTO legendary_boss_claims(player_id,death_id,boss_vnum,awarded_utc,essence_id,fragments,essences,catalysts) "
         "VALUES(%ld,'%s',%d,%lld,%d,%d,%d,%d)",id,g->award.id,g->award.vnum,(long long)g->now,
         g->award.essence,g->fragments,g->essences,g->catalysts) || mysql_affected_rows(conn)!=1)return false;
    }
  }
  return estate_query("UPDATE legendary_characters SET allowance_date=DATE_ADD('1970-01-01',INTERVAL %lld DAY),"
    "fragments_today=%d,essences_today=%d,catalysts_today=%d WHERE player_id=%ld AND version=%lld",
    (long long)g->before.utc_day,g->before.fragments_today+g->fragments,
    g->before.essences_today+g->essences,g->before.catalysts_today+g->catalysts,id,(long long)g->before.version);
}
static bool prepare_grant(struct char_data *ch,struct grant_context *g,struct legendary_transaction_plan *plan)
{
  bool blocked=false;
  if(!legendary_balance_load(ch,&g->before))return false;
  g->now=now_utc();cap_outputs(g);
  if(g->award.kind==LEG_XP_QUEST) {
    if(!prior_quest(GET_IDNUM(ch),g->award.vnum,g->award.occurred/86400,&blocked))return false;
    g->quest_duplicate=blocked;if(blocked)g->received_xp=0;
  }
  if(g->award.kind==4) {
    if(!prior_boss(GET_IDNUM(ch),g->award.vnum,g->now,&blocked))return false;
    /* Never bank capped/late material awards into a later UTC allowance. */
    if(blocked || g->award.occurred/86400!=g->before.utc_day)g->fragments=g->essences=g->catalysts=0;
  }
  memset(plan,0,sizeof(*plan));plan->kind=LEG_OP_GRANT;
  plan->delta.xp=g->received_xp;plan->delta.fragments=g->fragments;
  plan->delta.essences[g->award.essence]=g->essences;plan->delta.catalysts=g->catalysts;
  plan->participate=grant_participant;plan->context=g;return true;
}
static bool notify_awards(struct char_data *ch)
{
  MYSQL_RES *r; MYSQL_ROW row;
  r=estate_select("SELECT source_kind,award_id,xp,fragments,essences,catalysts,received_xp,received_fragments,"
    "received_essences,received_catalysts,essence_id FROM legendary_awards WHERE player_id=%ld AND state='credited' ORDER BY occurred_utc,award_id",GET_IDNUM(ch));
  if(!r)return false;
  while((row=mysql_fetch_row(r))) {
    int64_t n[8],essence=0; int i; bool valid=legendary_uuid_valid(row[1]) && num(row[10],&essence) && essence<6;
    for(i=0;i<8;i++)if(!num(row[i+2],&n[i]))valid=false;
    if(!valid || !estate_query("UPDATE legendary_awards SET state='notified' WHERE player_id=%ld AND source_kind=%d "
      "AND award_id='%s' AND state='credited'",GET_IDNUM(ch),atoi(row[0]),row[1])){mysql_free_result(r);return false;}
    if(mysql_affected_rows(conn)!=1)continue;
    if(atoi(row[0])==4) {
      if(!n[5] && !n[6] && !n[7])
        send_to_char(ch,"No dragonmetal materials received: allowance/capacity limits, boss cooldown or a closed source day.\r\n");
      else send_to_char(ch,"Dragonmetal reward: %lld fragments, %lld %s essences, %lld catalysts received%s.\r\n",
        (long long)n[5],(long long)n[6],legendary_essence_names[essence],(long long)n[7],
        n[1]!=n[5] || n[2]!=n[6] || n[3]!=n[7]?" (partial award: allowance or balance limits)":"");
    } else if(n[4]) {
      send_to_char(ch,"You receive %lld infusion XP%s.\r\n",(long long)n[4],n[4]<n[0]?" (balance overflow discarded)":"");
    } else if(n[0]) {
      send_to_char(ch,"No infusion XP credited: balance full or this quest already credited for that UTC day.\r\n");
    }
  }
  mysql_free_result(r);return true;
}
bool legendary_earnings_recover(struct char_data *ch)
{
  MYSQL_RES *r; MYSQL_ROW row; struct grant_context g; struct legendary_transaction_plan plan;
  struct legendary_quote *quote; int i; int64_t n[8];
  if(!ch || IS_NPC(ch))return true;
  if(!legendary_transaction_recover(ch))return false;
  if(!legendary_earnings_available()) {
    if(ch->legendary_awards_pending)
      send_to_char(ch,"A legendary source award is awaiting database recovery.\r\n");
    return !ch->legendary_awards_pending; /* Uninstalled legacy characters need no schema. */
  }
  ch->legendary_awards_pending=true;
  r=estate_select("SELECT source_kind,award_id,source_vnum,occurred_utc,xp,fragments,essences,catalysts,essence_id "
     "FROM legendary_awards WHERE player_id=%ld AND state='pending' ORDER BY occurred_utc,award_id",GET_IDNUM(ch));
  if(!r)return false;
  while((row=mysql_fetch_row(r))) {
    memset(&g,0,sizeof(g));
    if(!legendary_uuid_valid(row[1]))goto fail;
    for(i=0;i<9;i++)if(i!=1 && !num(row[i],&n[i>1?i-1:i]))goto fail;
    if(n[0]<1 || n[0]>4 || n[1]>INT_MAX || n[3]>INT_MAX || n[4]>100 || n[5]>20 || n[6]>1 || n[7]>5)goto fail;
    g.award.kind=n[0];memcpy(g.award.id,row[1],33);g.award.vnum=n[1];g.award.occurred=n[2];g.award.xp=n[3];
    g.award.fragments=n[4];g.award.essences=n[5];g.award.catalysts=n[6];g.award.essence=n[7];
    if(!prepare_grant(ch,&g,&plan) || !(quote=legendary_transaction_quote(ch,&plan)))goto fail;
    {bool ok=legendary_transaction_confirm(ch,quote);legendary_transaction_discard(quote);if(!ok)goto fail;}
  }
  mysql_free_result(r);
  if(!notify_awards(ch))return false;
  ch->legendary_awards_pending=false;return true;
fail:
  mysql_free_result(r);send_to_char(ch,"A legendary source award is awaiting persistence; retry after recovery.\r\n");return false;
}
static bool queue_award(struct char_data *ch,const struct award *a)
{
  if(!legendary_uuid_valid(a->id) || !legendary_earnings_available())return false;
  if(!estate_query("INSERT IGNORE INTO legendary_awards(player_id,source_kind,award_id,source_vnum,occurred_utc,"
     "xp,fragments,essences,catalysts,essence_id) VALUES(%ld,%d,'%s',%d,%lld,%lld,%d,%d,%d,%d)",
     GET_IDNUM(ch),a->kind,a->id,a->vnum,(long long)a->occurred,(long long)a->xp,
     a->fragments,a->essences,a->catalysts,a->essence)) {
    send_to_char(ch,"Legendary source %s could not be saved; no legendary award has been applied.\r\n",a->id);return false;
  }
  ch->legendary_awards_pending=true;
  return legendary_earnings_recover(ch);
}
void legendary_credit_xp(struct char_data *ch,int gain,const struct legendary_xp_context *ctx)
{
  struct award a;
  if(!ch || IS_NPC(ch) || GET_LEVEL(ch)!=30 || gain<=0 || !ctx || !ctx->eligible ||
     ctx->origin<LEG_XP_KILL || ctx->origin>LEG_XP_TRAP || ctx->source_vnum<0 ||
     !legendary_feature_enabled(LEG_FEATURE_EARNINGS))return;
  memset(&a,0,sizeof(a));a.kind=ctx->origin;a.vnum=ctx->source_vnum;a.xp=gain;a.occurred=now_utc();
  memcpy(a.id,ctx->award_id,33);queue_award(ch,&a);
}

struct legendary_exchange { struct legendary_quote *quote; struct grant_context grant; struct legendary_wallet delta; };
struct legendary_exchange *legendary_exchange_quote(struct char_data *ch,int from,int to,int quantity)
{
  struct legendary_exchange *x; struct legendary_transaction_plan plan; int64_t payment[6]; int i;
  if(!recipient(ch) || FIGHTING(ch) || quantity<=0 || quantity>20 || to < -1 || to>5 ||
    (to>=0 && (from<0 || from>5 || to==from)))return NULL;
  x=calloc(1,sizeof(*x));if(!x)return NULL;
  x->grant.exchange=true;x->grant.award.essence=to<0?0:to;
  if(!legendary_balance_load(ch,&x->grant.before))goto fail;
  if(to<0){x->grant.award.catalysts=quantity;}else{x->grant.award.essences=quantity;}
  cap_outputs(&x->grant);
  if(x->grant.essences!=x->grant.award.essences || x->grant.catalysts!=x->grant.award.catalysts)goto fail;
  memset(&plan,0,sizeof(plan));plan.kind=LEG_OP_EXCHANGE;plan.context=&x->grant;plan.participate=grant_participant;
  if(to<0) {
    if(!legendary_allocate_essences(x->grant.before.wallet.essences,0,20LL*quantity,0,payment))goto fail;
    plan.delta.fragments=-100LL*quantity;plan.delta.catalysts=quantity;
    for(i=0;i<6;i++)plan.delta.essences[i]=-payment[i];
  } else {plan.delta.essences[from]=-3LL*quantity;plan.delta.essences[to]=quantity;}
  x->delta=plan.delta;
  x->quote=legendary_transaction_quote(ch,&plan);if(!x->quote)goto fail;return x;
fail:
  free(x);return NULL;
}
bool legendary_exchange_confirm(struct char_data *ch,struct legendary_exchange *x)
{
  return x && recipient(ch) && !FIGHTING(ch) && legendary_transaction_confirm(ch,x->quote);
}
void legendary_exchange_discard(struct legendary_exchange *x)
{
  if(x){legendary_transaction_discard(x->quote);free(x);}
}
const struct legendary_wallet *legendary_exchange_delta(const struct legendary_exchange *x){return x?&x->delta:NULL;}
const char *legendary_exchange_id(const struct legendary_exchange *x){return x?legendary_transaction_id(x->quote):NULL;}

struct activity {long player; int actions; int64_t last; struct activity *next;};
struct encounter {char id[33]; int64_t last; struct activity *members; struct encounter *next;};
static struct encounter *encounters;
static void free_encounter(struct encounter *e)
{
  struct activity *a,*next;
  for(a=e->members;a;a=next){next=a->next;free(a);}free(e);
}
void legendary_activity_expire(void)
{
  struct encounter **p=&encounters,*e;
  while(*p){e=*p;if(now_utc()-e->last>legendary_rules.encounter_expiry){*p=e->next;free_encounter(e);}else p=&e->next;}
}
static struct encounter *encounter_for(struct char_data *boss,bool create)
{
  struct encounter *e;
  legendary_activity_expire();
  if(!instance_id(boss))return NULL;
  for(e=encounters;e;e=e->next)if(!strcmp(e->id,boss->legendary_encounter_id))return e;
  if(!create)return NULL;
  e=calloc(1,sizeof(*e));if(!e)return NULL;
  memcpy(e->id,boss->legendary_encounter_id,33);e->last=now_utc();e->next=encounters;encounters=e;return e;
}
static struct activity *member(struct encounter *e,long id)
{
  struct activity *a;
  for(a=e->members;a;a=a->next)if(a->player==id)return a;
  return NULL;
}
void legendary_record_action(struct char_data *actor,struct char_data *boss,enum legendary_action kind,
    struct char_data *ally,int effect)
{
  struct char_data *owner=credit_owner(actor),*supported=credit_owner(ally);
  struct encounter *e;struct activity *a,*engaged;
  int64_t now=now_utc();
  if(effect<=0 || kind<LEG_ACTION_DAMAGE || kind>LEG_ACTION_DEFENSE || !recipient(owner) || !real_victim(boss) ||
     IN_ROOM(boss)==NOWHERE || IN_ROOM(owner)!=IN_ROOM(boss) || !combat_boss(boss))return;
  e=encounter_for(boss,true);if(!e)return;
  if(kind==LEG_ACTION_HEAL || kind==LEG_ACTION_INTERCEPT || kind==LEG_ACTION_DEFENSE) {
    if(!supported || GET_HIT(ally)<=0 || GET_HIT(supported)<=0 || (owner!=supported && (!GROUP(owner) || GROUP(owner)!=GROUP(supported))) ||
       IN_ROOM(supported)!=IN_ROOM(boss))return;
    engaged=member(e,GET_IDNUM(supported));
    if((!engaged || now-engaged->last>120) && FIGHTING(ally)!=boss && FIGHTING(supported)!=boss)return;
  }
  a=member(e,GET_IDNUM(owner));
  if(!a){a=calloc(1,sizeof(*a));if(!a)return;a->player=GET_IDNUM(owner);a->next=e->members;e->members=a;}
  /* Count every useful event.  Endgame combat commonly produces several real
   * packets inside six seconds, so timing gates made normal participation look
   * like a tag.  A single incidental event still cannot qualify: all three
   * required actions must fall inside the short recent-activity window. */
  if(a->actions && now-a->last>legendary_rules.participation_recent)a->actions=0;
  if(a->actions<legendary_rules.participation_actions)a->actions++;
  a->last=now;e->last=now;
}
void legendary_record_support(struct char_data *actor,struct char_data *ally,enum legendary_action kind,int effect)
{
  struct char_data *boss;
  if(!ally || IN_ROOM(ally)==NOWHERE || effect<=0)return;
  for(boss=world[IN_ROOM(ally)].people;boss;boss=boss->next_in_room)
    if(real_victim(boss))legendary_record_action(actor,boss,kind,ally,effect);
}
void legendary_boss_death(struct char_data *boss,struct char_data *killer)
{
  struct char_data *owner=credit_owner(killer),*ch;
  struct encounter *e;struct activity *a;
  struct award award;int64_t now=now_utc();
  if(boss && owner && GET_LEVEL(owner)>=LVL_IMMORT)boss->legendary_test_encounter=true;
  if(!real_victim(boss) || !owner || GET_LEVEL(owner)>=LVL_IMMORT || IN_ROOM(boss)==NOWHERE ||
     !legendary_feature_enabled(LEG_FEATURE_EARNINGS) || !combat_boss(boss) ||
     !(e=encounter_for(boss,false)) || !legendary_earnings_available())return;
  memset(&award,0,sizeof(award));award.kind=4;award.vnum=GET_MOB_VNUM(boss);award.essence=boss_essence(boss);
  award.fragments=boss_tier(boss)*10;award.essences=boss_tier(boss)*2;award.catalysts=boss_tier(boss)==3;
  award.occurred=now;memcpy(award.id,boss->legendary_encounter_id,33);
  for(ch=world[IN_ROOM(boss)].people;ch;ch=ch->next_in_room) {
    if(!recipient(ch) || IN_ROOM(owner)!=IN_ROOM(boss) ||
      (ch!=owner && (!GROUP(owner) || GROUP(ch)!=GROUP(owner))))continue;
    a=member(e,GET_IDNUM(ch));
    if(a && a->actions>=legendary_rules.participation_actions &&
       now-a->last<=legendary_rules.participation_recent)queue_award(ch,&award);
  }
}
