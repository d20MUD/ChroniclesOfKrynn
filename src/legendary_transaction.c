/* Phase 4: a server-only transaction coordinator. No command or earning hooks. */
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "estate.h"
#include "legendary_identity.h"
#include "legendary_transaction.h"
#include <limits.h>
#include <errno.h>
#define CONFIG_VERSION LEGENDARY_DEFINITION_VERSION
struct legendary_quote {
  char id[33];
  long player;
  time_t expires;
  int64_t version;
  int gold;
  bool used, has_item, new_identity;
  struct obj_data *object;
  long object_id;
  int previous_bound;
  bool (*participate)(struct char_data *,const char *,void *);
  void *context;
  int channel, kind;
  struct legendary_wallet before, after, delta;
  struct legendary_item_record before_item, after_item;
};
#ifdef LEGENDARY_TRANSACTION_TEST
int legendary_transaction_fault;
int legendary_transaction_crash;
static bool checkpoint_stop(int n)
{if(legendary_transaction_crash==n)_exit(90+n);return legendary_transaction_fault==n;}
int legendary_transaction_clock_offset;
#define STOP(n) checkpoint_stop(n)
#else
#define STOP(n) 0
#endif
static time_t transaction_now(void)
{
#ifdef LEGENDARY_TRANSACTION_TEST
  return time(NULL)+legendary_transaction_clock_offset;
#else
  return time(NULL);
#endif
}
static bool changed(void) { return mysql_affected_rows(conn)==1; }
static bool economic_schema_ready(void)
{
  MYSQL_RES *res;
  MYSQL_ROW row;
  bool ok=false;
  if(!legendary_schema_ready())return false;
  res=estate_select("SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() "
    "AND table_name IN ('estate_deliveries','player_save_objs') AND engine='InnoDB'");
  if(res){row=mysql_fetch_row(res);ok=row && row[0] && !strcmp(row[0],"2");mysql_free_result(res);}
  return ok;
}
static bool pending_message(struct char_data *ch)
{
  send_to_char(ch,"Legendary operation %s is pending recovery. Please try again later.\r\n",
    ch->player_specials->legendary_operation);
  return false;
}
bool legendary_transaction_locked(const struct char_data *ch)
{
  return ch && !IS_NPC(ch) && ch->player_specials && *ch->player_specials->legendary_operation;
}
static bool wallet_load(long player, struct legendary_wallet *wallet, int64_t *version, bool lock)
{
  MYSQL_RES *res;
  MYSQL_ROW row;
  unsigned long long n[10];
  int i;
  res=estate_select("SELECT infusion_xp,fragments,catalysts,draconic,martial,arcane,spiritual,primal,shadow,version "
    "FROM legendary_characters WHERE player_id=%ld%s",player,lock?" FOR UPDATE":"");
  if (!res) return false;
  row=mysql_fetch_row(res);
  if (!row) {mysql_free_result(res);return false;}
  for(i=0;i<10;i++) {
    char *end;
    if (!row[i] || !*row[i] || row[i][0]=='-') break;
    errno=0;n[i]=strtoull(row[i],&end,10);
    if (errno || *end || n[i]>(i==9?(unsigned long long)INT64_MAX:2000000000ULL)) break;
  }
  mysql_free_result(res);
  if(i!=10 || n[9]==INT64_MAX) return false;
  memset(wallet,0,sizeof(*wallet));wallet->xp=n[0];wallet->fragments=n[1];wallet->catalysts=n[2];
  for(i=0;i<6;i++)wallet->essences[i]=n[i+3];
  *version=n[9];return true;
}
static bool balance_add(int64_t a,int64_t b,int64_t *out)
{
  if(b < -2000000000LL || b>2000000000LL || a+b<0 || a+b>2000000000LL)return false;
  *out=a+b;return true;
}
static bool apply_delta(const struct legendary_wallet *a,const struct legendary_wallet *d,
    struct legendary_wallet *b)
{
  int i;
  *b=*a;
  if(d->gold>0 || d->gold < -INT_MAX ||
     !balance_add(a->xp,d->xp,&b->xp) || !balance_add(a->fragments,d->fragments,&b->fragments) ||
     !balance_add(a->catalysts,d->catalysts,&b->catalysts) || a->gold+d->gold<0)return false;
  b->gold=a->gold+d->gold;
  for(i=0;i<6;i++)if(!balance_add(a->essences[i],d->essences[i],&b->essences[i]))return false;
  return true;
}
static void find_uuid(struct obj_data *list,const char *uuid,struct obj_data **found,int *count,int depth)
{
  struct obj_data *obj;
  if(depth>64){*count=2;return;}
  for(obj=list;obj;obj=obj->next_content) {
    if(!strcmp(obj->legendary_uuid,uuid)){*found=obj;(*count)++;}
    find_uuid(obj->contains,uuid,found,count,depth+1);
    find_uuid(obj->sheath_primary,uuid,found,count,depth+1);
    find_uuid(obj->sheath_secondary,uuid,found,count,depth+1);
  }
}
static int item_occurrences(struct char_data *ch,const char *uuid,struct obj_data **found)
{
  int count=0,i;
  *found=NULL;
  find_uuid(ch->carrying,uuid,found,&count,0);
  for(i=0;i<NUM_WEARS;i++)if(GET_EQ(ch,i))find_uuid(GET_EQ(ch,i),uuid,found,&count,0);
  return count;
}
static struct obj_data *owned_item(struct char_data *ch,const char *uuid)
{
  struct obj_data *found;
  int count=item_occurrences(ch,uuid,&found);
  return count==1 && found && GET_OBJ_BOUND_ID(found)==GET_IDNUM(ch)?found:NULL;
}
static struct obj_data *new_candidate(struct char_data *ch,const struct legendary_quote *q)
{
  struct obj_data *obj;
  int i;
  for(obj=ch->carrying;obj;obj=obj->next_content)
    if(obj==q->object && GET_ID(obj)==q->object_id && !*obj->legendary_uuid &&
       GET_OBJ_BOUND_ID(obj)==q->previous_bound && (int64_t)GET_OBJ_VNUM(obj)==q->after_item.prototype)return obj;
  for(i=0;i<NUM_WEARS;i++) {
    obj=GET_EQ(ch,i);
    if(obj && obj==q->object && GET_ID(obj)==q->object_id && !*obj->legendary_uuid &&
       GET_OBJ_BOUND_ID(obj)==q->previous_bound && (int64_t)GET_OBJ_VNUM(obj)==q->after_item.prototype)return obj;
  }
  return NULL;
}
/* Verify the UUID actually made it into the ordinary saved-object journal.
 * A detached object/preview can never authorize progression. */
static bool item_checkpoint(struct char_data *ch,const char *uuid)
{
  MYSQL_RES *res;
  MYSQL_ROW row;
  char *name;
  bool ok=false;
  if(!owned_item(ch,uuid))return false;
  if(!Crash_crashsave_checked(ch))return false;
  name=estate_quote(GET_NAME(ch));
  if(!name)return false;
  res=estate_select("SELECT COUNT(*) FROM player_save_objs WHERE name='%s' AND "
    "serialized_obj LIKE '%%LgId: 1 %s%%'",name,uuid);
  free(name);
  if(res){row=mysql_fetch_row(res);ok=row && row[0] && !strcmp(row[0],"1");mysql_free_result(res);}
  return ok;
}
static bool item_matches(const struct legendary_item_record *a,const struct legendary_item_record *b)
{
  int i;
  if(strcmp(a->uuid,b->uuid)||a->owner!=b->owner||a->prototype!=b->prototype||a->slot!=b->slot||
     a->state!=b->state||a->version!=b->version)return false;
  for(i=0;i<2;i++)if(a->channels[i].present!=b->channels[i].present ||
    (a->channels[i].present && a->channels[i].version!=b->channels[i].version))return false;
  return true;
}
struct legendary_quote *legendary_transaction_quote(struct char_data *ch,
    const struct legendary_transaction_plan *plan)
{
  struct legendary_quote *q;
  if(!ch || IS_NPC(ch) || !plan || (plan->item && FIGHTING(ch)) || legendary_transaction_locked(ch) ||
    plan->kind<LEG_OP_PURCHASE || plan->kind>LEG_OP_GRANT || !economic_schema_ready() ||
    !estate_player_ready(ch))return NULL;
  q=calloc(1,sizeof(*q));if(!q)return NULL;
  q->player=GET_IDNUM(ch);q->gold=GET_GOLD(ch);q->delta=plan->delta;q->kind=plan->kind;
  q->participate=plan->participate;q->context=plan->context;
  q->expires=transaction_now()+60;
  if(!legendary_uuid_generate(q->id)||!wallet_load(q->player,&q->before,&q->version,false))goto fail;
  q->before.gold=q->gold;
  if(!apply_delta(&q->before,&q->delta,&q->after))goto fail;
  if(plan->item) {
    if(plan->channel<0 || plan->channel>1 || plan->item->owner!=q->player)goto fail;
    q->has_item=true;q->channel=plan->channel;q->after_item=*plan->item;
    if(!*plan->item->uuid) {
      q->new_identity=true;q->object=plan->object;
      if(!q->object || plan->channel || plan->kind!=LEG_OP_PURCHASE ||
         plan->item->version || plan->item->channels[1].present)goto fail;
      q->object_id=GET_ID(q->object);q->previous_bound=GET_OBJ_BOUND_ID(q->object);
      if((q->previous_bound < -1 || (q->previous_bound > 0 && q->previous_bound!=q->player)) || !new_candidate(ch,q) ||
         !legendary_uuid_generate(q->after_item.uuid))goto fail;
      q->before_item=q->after_item;
      memset(q->before_item.channels,0,sizeof(q->before_item.channels));
      q->before_item.state=LEG_RECORD_PENDING;
    } else {
      if(!owned_item(ch,plan->item->uuid) || owned_item(ch,plan->item->uuid)->legendary_quarantined ||
         legendary_store_load(plan->item->uuid,&q->before_item)!=LEG_LOAD_OK ||
         q->before_item.state==LEG_RECORD_DELETED || q->before_item.version!=plan->item->version ||
         q->before_item.owner!=plan->item->owner || q->before_item.prototype!=plan->item->prototype ||
         q->before_item.slot!=plan->item->slot)goto fail;
    }
    if(q->after_item.state==LEG_RECORD_DELETED || !legendary_record_valid(&q->after_item) || !q->after_item.channels[q->channel].present ||
       q->after_item.channels[q->channel].version!=q->before_item.channels[q->channel].version)goto fail;
  }
  return q;
fail:
  free(q);return NULL;
}
void legendary_transaction_discard(struct legendary_quote *q){free(q);}
const char *legendary_transaction_id(const struct legendary_quote *q){return q?q->id:NULL;}
static bool wallet_save(const struct legendary_quote *q)
{
  const struct legendary_wallet *w=&q->after;
  return estate_query("UPDATE legendary_characters SET infusion_xp=%lld,fragments=%lld,catalysts=%lld,"
    "draconic=%lld,martial=%lld,arcane=%lld,spiritual=%lld,primal=%lld,shadow=%lld,version=version+1 "
    "WHERE player_id=%ld AND version=%lld",(long long)w->xp,(long long)w->fragments,(long long)w->catalysts,
    (long long)w->essences[0],(long long)w->essences[1],(long long)w->essences[2],
    (long long)w->essences[3],(long long)w->essences[4],(long long)w->essences[5],q->player,
    (long long)q->version) && changed();
}
/* No write from this point is considered successful merely because the client
 * saw an acknowledgment. Every restart uses SQL state and the saved receipt. */
bool legendary_transaction_confirm(struct char_data *ch,struct legendary_quote *q)
{
  struct legendary_wallet wallet;
  struct legendary_item_record current;
  int64_t version;
  unsigned long long receipt=0;
  MYSQL_RES *res;
  MYSQL_ROW row;
  bool valid;
  int i;
  long long ess=0;
  char uuid_sql[36], receipt_sql[32], payload[1536];
  if(!ch || IS_NPC(ch) || !q || q->used || (q->has_item && FIGHTING(ch)) || q->player!=GET_IDNUM(ch) ||
    legendary_transaction_locked(ch) || transaction_now()>=q->expires || GET_GOLD(ch)!=q->gold ||
    !economic_schema_ready() || !estate_player_ready(ch))return false;
  if(q->new_identity && !new_candidate(ch,q))return false;
  q->used=true;
  for(i=0;i<6;i++)if(q->delta.essences[i]<0)ess-=q->delta.essences[i];
  if(STOP(1))return false;
  if(q->has_item) snprintf(uuid_sql,sizeof(uuid_sql),"'%s'",q->after_item.uuid);
  else strcpy(uuid_sql,"NULL");
  { /* Persist exact ledger changes and the selected channel for audits. */
    const struct legendary_channel_record *c=q->has_item?&q->after_item.channels[q->channel]:NULL;
    snprintf(payload,sizeof(payload),
      "v1 delta %lld %lld %lld %lld %lld %lld %lld %lld %lld %lld item %lld channel "
      "%d %d %d %d %d %d %d %d %d %u %lld %lld %lld %lld %lld %lld passive %d",
      (long long)q->delta.xp,(long long)q->delta.gold,(long long)q->delta.fragments,
      (long long)q->delta.catalysts,(long long)q->delta.essences[0],(long long)q->delta.essences[1],
      (long long)q->delta.essences[2],(long long)q->delta.essences[3],(long long)q->delta.essences[4],
      (long long)q->delta.essences[5],(long long)(q->has_item?q->before_item.version:0),
      c?c->rank:0,c?c->path:0,c?c->family:0,c?c->foundation:0,c?c->subtype:0,
      c?c->technique:0,c?c->signature:0,c?c->modifier:0,c?c->modifier_target:0,c?c->first_choices:0,
      (long long)(c?c->version:0),(long long)(c?c->paid.xp:0),(long long)(c?c->paid.gold:0),
      (long long)(c?c->paid.fragments:0),(long long)(c?c->paid.essences:0),(long long)(c?c->paid.catalysts:0),c?c->item_passive:0);
  }
  /* Persist the lock BEFORE creating any SQL operation or identity. Thus a
   * prepared SQL row can never become an orphan merely because a save failed. */
  snprintf(ch->player_specials->legendary_operation,33,"%s",q->id);
  if(STOP(2) || !save_char_checked(ch,0) || STOP(3))return pending_message(ch);
  /* Unique held_player serializes all kinds, including grants/exchanges. */
  if(!estate_query("INSERT INTO legendary_operations(operation_id,player_id,held_player,uuid,channel_id,kind,"
    "before_version,after_version,gold_before,gold_after,xp_cost,gold_cost,fragment_cost,essence_cost,catalyst_cost,"
    "recovery_payload,quote_expires,config_version,new_identity,previous_bound) VALUES('%s',%ld,%ld,%s,%s,'%s',%lld,%lld,%d,%lld,"
    "%lld,%lld,%lld,%lld,%lld,'%s',%lld,%d,%d,%d)",q->id,q->player,q->player,
    uuid_sql,
    q->has_item?(q->channel?"1":"0"):"NULL",
    (const char *[]) {"purchase","reshape","exchange","grant"}[q->kind],
    (long long)q->version,(long long)q->version+1,q->gold,(long long)q->after.gold,
    (long long)(q->delta.xp<0?-q->delta.xp:0),(long long)-q->delta.gold,
    (long long)(q->delta.fragments<0?-q->delta.fragments:0),ess,
    (long long)(q->delta.catalysts<0?-q->delta.catalysts:0),payload,(long long)q->expires,CONFIG_VERSION,q->new_identity?1:0,q->previous_bound)) {
    return pending_message(ch);
  }
  if(q->new_identity) {
    struct obj_data *obj=new_candidate(ch,q);
    if(!obj || !legendary_store_insert(&q->before_item))return pending_message(ch);
    memcpy(obj->legendary_uuid,q->after_item.uuid,33);
    GET_OBJ_BOUND_ID(obj)=q->player;
  }
  if(q->has_item && (!owned_item(ch,q->after_item.uuid) ||
     owned_item(ch,q->after_item.uuid)->legendary_quarantined))return pending_message(ch);
  if(q->has_item && (!item_checkpoint(ch,q->after_item.uuid) || STOP(4)))return pending_message(ch);
  if(!estate_query("START TRANSACTION"))return pending_message(ch);
  res=estate_select("SELECT state FROM legendary_operations WHERE operation_id='%s' AND held_player=%ld FOR UPDATE",
    q->id,q->player);
  valid=false;
  if(res){row=mysql_fetch_row(res);valid=row && !strcmp(row[0],"prepared");mysql_free_result(res);}
  if(!valid || !wallet_load(q->player,&wallet,&version,true) || version!=q->version || GET_GOLD(ch)!=q->gold ||
     transaction_now()>=q->expires)goto rollback;
  if(q->has_item) {
    res=estate_select("SELECT uuid FROM legendary_items WHERE uuid='%s' FOR UPDATE",q->after_item.uuid);
    if(!res)goto rollback;
    mysql_free_result(res);
    if(legendary_store_load(q->after_item.uuid,&current)!=LEG_LOAD_OK || !item_matches(&current,&q->before_item))goto rollback;
  }
  if((q->participate && !q->participate(ch,q->id,q->context)) || STOP(5) || !wallet_save(q) || STOP(6))goto rollback;
  if(q->has_item) {
    int64_t expected=q->before_item.channels[q->channel].present?q->before_item.channels[q->channel].version:-1;
    if(!legendary_store_save_channel(&q->after_item,q->channel,expected) || STOP(7) ||
       !legendary_store_set_state(q->after_item.uuid,q->player,q->before_item.version,LEG_RECORD_PENDING))goto rollback;
  }
  if(STOP(8))goto rollback;
  if(q->delta.gold && !(receipt=estate_queue_money_receipt(q->player,(long)-q->delta.gold)))goto rollback;
  if(receipt) snprintf(receipt_sql,sizeof(receipt_sql),"%llu",receipt);
  else strcpy(receipt_sql,"NULL");
  if(STOP(9) || !estate_query("UPDATE legendary_operations SET state='committed',estate_delivery_id=%s "
     "WHERE operation_id='%s' AND state='prepared'",
     receipt_sql,q->id) || !changed() || STOP(10))goto rollback;
  if(!estate_query("COMMIT") || STOP(11))return pending_message(ch);
  return legendary_transaction_recover(ch);
rollback:
  estate_query("ROLLBACK");
  return pending_message(ch);
}

bool legendary_transaction_recover(struct char_data *ch)
{
  MYSQL_RES *res;
  MYSQL_ROW row;
  char id[33],uuid[33],state[16];
  unsigned long long receipt=0;
  bool has_item=false, new_identity=false;
  int previous_bound=0;
  struct legendary_item_record item;
  struct obj_data *obj=NULL;
  if(!legendary_transaction_locked(ch))return true; /* legacy players: no schema dependency */
  snprintf(id,sizeof(id),"%s",ch->player_specials->legendary_operation);
  if(!legendary_uuid_valid(id) || !economic_schema_ready())return pending_message(ch);
  res=estate_select("SELECT state,uuid,estate_delivery_id,new_identity,previous_bound FROM legendary_operations WHERE operation_id='%s' AND player_id=%ld",
    id,GET_IDNUM(ch));
  if(!res)return pending_message(ch);
  row=mysql_fetch_row(res);
  if(!row) {
    /* Crash between the durable lock and the first SQL insert: no debit or
     * identity can have been written. Only clear if no other operation holds
     * this character; a conflicting writer requires reconciliation. */
    bool clear=false;
    mysql_free_result(res);
    res=estate_select("SELECT COUNT(*) FROM legendary_operations WHERE held_player=%ld",GET_IDNUM(ch));
    if(res){row=mysql_fetch_row(res);clear=row && !strcmp(row[0],"0");mysql_free_result(res);}
    if(!clear)return pending_message(ch);
    ch->player_specials->legendary_operation[0]='\0';
    if(!save_char_checked(ch,0)) {
      memcpy(ch->player_specials->legendary_operation,id,33);return pending_message(ch);
    }
    return true;
  }
  snprintf(state,sizeof(state),"%s",row[0]);uuid[0]='\0';
  if(row[1]){snprintf(uuid,sizeof(uuid),"%s",row[1]);has_item=true;}
  if(row[2])receipt=strtoull(row[2],NULL,10);
  new_identity=row[3] && !strcmp(row[3],"1");
  if(row[4])previous_bound=atoi(row[4]);
  mysql_free_result(res);
  if(!strcmp(state,"prepared")) {
    if(STOP(12) || !estate_query("UPDATE legendary_operations SET state='cancelled',held_player=NULL "
        "WHERE operation_id='%s' AND state='prepared'",id) || !changed() || STOP(13))return pending_message(ch);
    strcpy(state,"cancelled");
  }
  if(!strcmp(state,"committed")) {
    if(has_item && (!(obj=owned_item(ch,uuid)) || !item_checkpoint(ch,uuid)))return pending_message(ch);
    if(STOP(14))return pending_message(ch);
    estate_recover(ch); /* same ordered receipt stream as all other estate money */
    if(receipt) {
      bool delivered=false;
      res=estate_select("SELECT delivered FROM estate_deliveries WHERE delivery_id=%llu AND player_id=%ld "
        "AND serialized_obj=''",receipt,GET_IDNUM(ch));
      if(res){row=mysql_fetch_row(res);delivered=row && !strcmp(row[0],"1");mysql_free_result(res);}
      if(!delivered || ch->player_specials->estate_debit_receipt<receipt)return pending_message(ch);
    }
    if(!save_char_checked(ch,0) || STOP(15) || !estate_query("START TRANSACTION"))return pending_message(ch);
    /* Lock operation before item, matching confirmation order. */
    res=estate_select("SELECT state FROM legendary_operations WHERE operation_id='%s' FOR UPDATE",id);
    if(!res)goto rollback;
    row=mysql_fetch_row(res);
    if(!row || strcmp(row[0],"committed")){mysql_free_result(res);goto rollback;}
    mysql_free_result(res);
    if(has_item) {
      if(legendary_store_load(uuid,&item)!=LEG_LOAD_OK || item.owner!=GET_IDNUM(ch) || item.state!=LEG_RECORD_PENDING ||
        !legendary_store_set_state(uuid,item.owner,item.version,LEG_RECORD_ACTIVE))goto rollback;
    }
    if(STOP(16) || !estate_query("UPDATE legendary_operations SET state='applied',held_player=NULL "
      "WHERE operation_id='%s' AND state='committed'",id) || !changed() || STOP(17))goto rollback;
    if(!estate_query("COMMIT") || STOP(18))return pending_message(ch);
    strcpy(state,"applied");
  }
  if(strcmp(state,"applied") && strcmp(state,"cancelled"))return pending_message(ch);
  if(new_identity && !strcmp(state,"cancelled")) {
    int count=item_occurrences(ch,uuid,&obj);
    if(count>1 || (obj && GET_OBJ_BOUND_ID(obj)!=GET_IDNUM(ch)))return pending_message(ch);
    if(obj) {legendary_object_clear_identity(obj);GET_OBJ_BOUND_ID(obj)=previous_bound;}
    /* Even when a prior attempt already cleared memory, repeat the checkpoint
     * before releasing the durable lock. No persisted UUID may be left behind. */
    if(!Crash_crashsave_checked(ch) || !estate_query("UPDATE legendary_items SET state='deleted',"
       "deleted_at=UTC_TIMESTAMP(),version=version+1 WHERE uuid='%s' AND owner_id=%ld AND state='pending'",
       uuid,GET_IDNUM(ch)))return pending_message(ch);
    has_item=false;
  }
  if(has_item) {
    obj=owned_item(ch,uuid);
    if(!obj || legendary_store_load(uuid,&item)!=LEG_LOAD_OK) return pending_message(ch);
    if(item.state==LEG_RECORD_ACTIVE) {
      legendary_object_unload(obj);obj->legendary_quarantined=false;
      if(!legendary_object_verify(obj))return pending_message(ch);
    }
    if(!item_checkpoint(ch,uuid))return pending_message(ch);
  }
  /* Clear only after the SQL outcome is terminal. On any ambiguous save keep
   * the lock in memory and repeat this harmless checkpoint on the next retry. */
  ch->player_specials->legendary_operation[0]='\0';
  if(STOP(19) || !save_char_checked(ch,0) || STOP(20)) {
    memcpy(ch->player_specials->legendary_operation,id,33);return pending_message(ch);
  }
  return true;
rollback:
  estate_query("ROLLBACK");return pending_message(ch);
}
