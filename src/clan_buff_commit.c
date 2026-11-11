#define _POSIX_C_SOURCE 200809L
#include "clan_buff_activation.h"
#include <json-c/json.h>
#include <errno.h>
#include <ctype.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef CLAN_BUFF_ACTIVATION_TEST
#include <unistd.h>
int clan_buff_activation_fault,clan_buff_activation_crash;
static bool stop(int n) {
  if(clan_buff_activation_crash==n)_exit(100+n);
  return clan_buff_activation_fault==n;
}
#else
static bool stop(int n) {(void)n;return false;}
#endif
static bool sql(MYSQL *db,const char *fmt,...)
{
  char text[8192];int n;va_list ap;
  va_start(ap,fmt);n=vsnprintf(text,sizeof(text),fmt,ap);va_end(ap);
  return n>=0 && (size_t)n<sizeof(text) && !mysql_query(db,text);
}
static bool integer(const char *s,int64_t *out)
{
  char *end;intmax_t n;
  if(!s||!*s)return false;
  errno=0;n=strtoimax(s,&end,10);
  if(errno||*end||n<0||n>INT64_MAX)return false;
  *out=(int64_t)n;return true;
}
static bool scalar(MYSQL *db,const char *query,int64_t *n)
{
  MYSQL_RES *res;MYSQL_ROW row;bool ok;
  if(mysql_query(db,query)||!(res=mysql_store_result(db)))return false;
  row=mysql_fetch_row(res);ok=row&&integer(row[0],n)&&!mysql_fetch_row(res);
  mysql_free_result(res);return ok;
}
static void canonical(char *out,const char *in)
{
  while(*in) {unsigned char c=(unsigned char)*in++;if(!isspace(c)&&c!='`'&&c!='('&&c!=')')*out++=(char)tolower(c);}
  *out=0;
}
static bool contract(MYSQL *db)
{
  static const struct {const char *table,*fragment;} fragments[]={
    {"clan_buff_activations","PRIMARY KEY (operation_uid)"},
    {"clan_buff_management","PRIMARY KEY (clan_uid)"},
    {"clan_buff_activation_schema","PRIMARY KEY (singleton)"},
    {"clan_buff_activations","CONSTRAINT cba_clan FOREIGN KEY(clan_uid) REFERENCES clan_progression_clans(clan_uid)"},
    {"clan_buff_management","CONSTRAINT cbm_clan FOREIGN KEY(clan_uid) REFERENCES clan_progression_clans(clan_uid)"},
    {"clan_buff_management","CONSTRAINT cbm_privilege CHECK(privilege_id=21)"},
    {"clan_buff_management","CONSTRAINT cbm_rank CHECK(max_rank BETWEEN 0 AND 15)"},
    {"clan_buff_activations","CONSTRAINT cba_request CHECK(actor_id>0 AND family_id BETWEEN 1 AND 5 AND tier BETWEEN 1 AND 3 AND slot_id BETWEEN 1 AND 3 AND rules_version=1)"},
    {"clan_buff_activations","CONSTRAINT cba_json CHECK(JSON_VALID(receipt))"}
  };
  size_t i;MYSQL_RES *res;MYSQL_ROW row;bool ok;
  char actual[32768],expected[4096];
  for(i=0;i<sizeof(fragments)/sizeof(*fragments);i++) {
    if(!sql(db,"SHOW CREATE TABLE %s",fragments[i].table)||!(res=mysql_store_result(db)))return false;
    row=mysql_fetch_row(res);ok=row&&row[1]&&strlen(row[1])<sizeof(actual);
    if(ok) {canonical(actual,row[1]);canonical(expected,fragments[i].fragment);ok=strstr(actual,expected)!=NULL;}
    mysql_free_result(res);if(!ok)return false;
  }
  for(i=0;i<2;i++) {
    const char *name=i?"cba_retained":"cba_immutable";
    const char *body=i?"BEGIN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Buff activation history is retained'; END":"BEGIN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Buff activation receipts are immutable'; END";
    if(!sql(db,"SELECT ACTION_STATEMENT FROM information_schema.TRIGGERS WHERE TRIGGER_SCHEMA=DATABASE() AND TRIGGER_NAME='%s'",name)||!(res=mysql_store_result(db)))return false;
    row=mysql_fetch_row(res);ok=row&&row[0]&&strlen(row[0])<sizeof(actual);
    if(ok){canonical(actual,row[0]);canonical(expected,body);ok=!strcmp(actual,expected);}
    mysql_free_result(res);if(!ok)return false;
  }
  return true;
}
enum clan_repository_result clan_buff_activation_schema_check(MYSQL *db)
{
  int64_t n;
  enum clan_repository_result result=clan_repository_schema_check(db);
  if(result!=CLAN_REPO_OK)return result;
  if(!scalar(db,"SELECT version FROM clan_buff_activation_schema WHERE singleton=1",&n)||n!=1 ||
     !scalar(db,"SELECT COUNT(*) FROM information_schema.TABLES WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME IN ('clan_buff_activations','clan_buff_management','clan_buff_activation_schema') AND ENGINE='InnoDB'",&n)||n!=3 ||
     !scalar(db,"SELECT COUNT(*) FROM information_schema.TRIGGERS WHERE TRIGGER_SCHEMA=DATABASE() AND EVENT_OBJECT_TABLE='clan_buff_activations' AND ACTION_TIMING='BEFORE' AND ((TRIGGER_NAME='cba_immutable' AND EVENT_MANIPULATION='UPDATE') OR (TRIGGER_NAME='cba_retained' AND EVENT_MANIPULATION='DELETE')) AND ACTION_STATEMENT LIKE '%SIGNAL%45000%'",&n)||n!=2 ||
     !scalar(db,"SELECT COUNT(*) FROM information_schema.TABLE_CONSTRAINTS WHERE CONSTRAINT_SCHEMA=DATABASE() AND CONSTRAINT_TYPE='CHECK' AND ((TABLE_NAME='clan_buff_activations' AND CONSTRAINT_NAME IN ('cba_uid','cba_request','cba_expected','cba_json')) OR (TABLE_NAME='clan_buff_management' AND CONSTRAINT_NAME IN ('cbm_privilege','cbm_rank')))",&n)||n!=6)
    return CLAN_REPO_SCHEMA;
  return contract(db)?CLAN_REPO_OK:CLAN_REPO_SCHEMA;
}
static struct json_object *receipt(const struct clan_buff_request *r,const struct clan_buff_decision *d,int64_t balance)
{
  struct json_object *o=json_object_new_object();
#define S(k,v) json_object_object_add(o,k,json_object_new_string(v))
#define N(k,v) json_object_object_add(o,k,json_object_new_int64(v))
  S("operation_uid",r->operation_uid);S("clan_uid",r->clan_uid);N("actor_id",r->actor_id);
  N("family",r->family);N("tier",r->tier);N("slot",r->slot);N("rules_version",r->rules_version);
  S("expected_activation_uid",r->expected_activation_uid);
  S("replaced_activation_uid",d->replaced_activation_uid);N("replaced_family",d->replaced_family);
  N("paid_influence_eighths",d->activation.paid_influence_eighths);
  N("started_at",d->activation.started_at);N("expires_at",d->activation.expires_at);
  N("next_available_at",d->activation.next_available_at);
  N("influence_before",balance);N("influence_after",balance-d->activation.paid_influence_eighths);
#undef S
#undef N
  return o;
}
static bool receipt_valid(struct json_object *o,const struct clan_buff_request *r)
{
  static const char *keys[]={"actor_id","family","tier","slot","rules_version","replaced_family",
      "paid_influence_eighths","started_at","expires_at","next_available_at","influence_before","influence_after"};
  int64_t n[12];size_t i;struct json_object *v;
  const char *names[]={"operation_uid","clan_uid","expected_activation_uid","replaced_activation_uid"};
  const char *expected[]={r->operation_uid,r->clan_uid,r->expected_activation_uid,r->expected_activation_uid};
  if(!o||!json_object_is_type(o,json_type_object)||json_object_object_length(o)!=16)return false;
  for(i=0;i<4;i++) {
    if(!json_object_object_get_ex(o,names[i],&v)||!json_object_is_type(v,json_type_string)||
        json_object_get_string_len(v)!=(int)strlen(expected[i])||strcmp(json_object_get_string(v),expected[i]))return false;
  }
  for(i=0;i<12;i++) {
    if(!json_object_object_get_ex(o,keys[i],&v)||!json_object_is_type(v,json_type_int)||(n[i]=json_object_get_int64(v))<0)return false;
  }
  return n[0]==r->actor_id&&n[1]==r->family&&n[2]==r->tier&&n[3]==r->slot&&n[4]==r->rules_version&&
      n[5]<=CLAN_PROGRESSION_BUFF_COUNT&&((n[5]>0)==(r->expected_activation_uid[0]!=0))&&
      n[8]>n[7]&&n[9]>=n[8]&&n[10]>=n[6]&&n[11]==n[10]-n[6];
}
static void receipt_decision(const struct clan_buff_request *r,struct json_object *o,struct clan_buff_decision *d)
{
  struct json_object *v;
  memset(d,0,sizeof(*d));strcpy(d->activation.clan_uid,r->clan_uid);strcpy(d->activation.activation_uid,r->operation_uid);
  d->activation.family=r->family;d->activation.tier=r->tier;d->activation.slot=r->slot;d->activation.rules_version=r->rules_version;
  strcpy(d->replaced_activation_uid,r->expected_activation_uid);
#define READ(key,field) json_object_object_get_ex(o,key,&v);d->field=json_object_get_int64(v)
  READ("replaced_family",replaced_family);READ("paid_influence_eighths",activation.paid_influence_eighths);
  READ("started_at",activation.started_at);READ("expires_at",activation.expires_at);READ("next_available_at",activation.next_available_at);
#undef READ
}
/* Check immutable request fields before accepting a retry as the same action. */
static enum clan_repository_result replay(MYSQL *db,const struct clan_buff_request *r,struct json_object **out)
{
  MYSQL_RES *res;MYSQL_ROW row;int64_t n[5];size_t i;bool matches;
  struct json_object *o;
  if(!sql(db,"SELECT clan_uid,actor_id,family_id,tier,slot_id,rules_version,expected_activation_uid,receipt FROM clan_buff_activations WHERE operation_uid='%s'",r->operation_uid) || !(res=mysql_store_result(db)))return CLAN_REPO_UNAVAILABLE;
  row=mysql_fetch_row(res);
  if(!row){mysql_free_result(res);return CLAN_REPO_NOT_FOUND;}
  for(i=0;i<5;i++)if(!integer(row[i+1],&n[i])){mysql_free_result(res);return CLAN_REPO_SCHEMA;}
  matches=row[0]&&row[6]&&!strcmp(row[0],r->clan_uid)&&!strcmp(row[6],r->expected_activation_uid)&&
      n[0]==r->actor_id&&n[1]==r->family&&n[2]==r->tier&&n[3]==r->slot&&n[4]==r->rules_version;
  o=row[7]?json_tokener_parse(row[7]):NULL;
  mysql_free_result(res);
  if(!o||!json_object_is_type(o,json_type_object)){if(o)json_object_put(o);return CLAN_REPO_SCHEMA;}
  if(!matches){json_object_put(o);return CLAN_REPO_CONFLICT;}
  if(!receipt_valid(o,r)){json_object_put(o);return CLAN_REPO_SCHEMA;}
  *out=o;return CLAN_REPO_OK;
}
enum clan_repository_result clan_buff_locked_snapshot(MYSQL *db,const struct clan_buff_request *r,struct clan_buff_activation_snapshot *s)
{
  MYSQL_RES *res;MYSQL_ROW row;
  enum clan_repository_result result;
  int64_t rank,ranks,maximum;size_t i;
  memset(s,0,sizeof(*s));
  if(!sql(db,"SELECT clan_uid FROM clan_progression_state WHERE clan_uid='%s' FOR UPDATE",r->clan_uid)||!(res=mysql_store_result(db)))return CLAN_REPO_UNAVAILABLE;
  mysql_free_result(res);
  result=clan_repository_load(db,r->clan_uid,&s->clan);
  if(result!=CLAN_REPO_OK)return result;
  if(!sql(db,"SELECT m.rank_id,JSON_LENGTH(JSON_EXTRACT(l.details,'$.ranks')),COALESCE(p.max_rank,0) FROM clan_memberships m JOIN clan_progression_legacy l ON l.clan_uid=m.clan_uid LEFT JOIN clan_buff_management p ON p.clan_uid=m.clan_uid WHERE m.clan_uid='%s' AND m.player_id=%" PRId64 " FOR UPDATE",r->clan_uid,r->actor_id)||!(res=mysql_store_result(db)))return CLAN_REPO_UNAVAILABLE;
  row=mysql_fetch_row(res);
  if(row) {
    if(!integer(row[0],&rank)||!integer(row[1],&ranks)||!integer(row[2],&maximum)||ranks<1||ranks>15||maximum>ranks||rank>ranks){mysql_free_result(res);return CLAN_REPO_SCHEMA;}
    s->accepted_member=rank>0;
    s->may_manage=s->accepted_member&&(r->actor_id==s->clan.leader_id||(maximum>0&&rank<=maximum));
  }
  mysql_free_result(res);
  if(!sql(db,"SELECT activation_uid,family_id,tier,slot_id,rules_version,started_at,expires_at,next_available_at,paid_influence_eighths,canceled FROM clan_buff_families WHERE clan_uid='%s' ORDER BY family_id FOR UPDATE",r->clan_uid)||!(res=mysql_store_result(db)))return CLAN_REPO_UNAVAILABLE;
  while((row=mysql_fetch_row(res))) {
    struct clan_buff_state *b;int64_t n[9];
    if(s->count==CLAN_PROGRESSION_BUFF_COUNT||!row[0]||!clan_progression_uid_valid(row[0])){mysql_free_result(res);return CLAN_REPO_SCHEMA;}
    for(i=0;i<9;i++)if(!integer(row[i+1],&n[i])){mysql_free_result(res);return CLAN_REPO_SCHEMA;}
    if(n[0]>5||n[1]>3||n[2]>3||n[3]>INT32_MAX||n[8]>1){mysql_free_result(res);return CLAN_REPO_SCHEMA;}
    b=&s->rows[s->count++];strcpy(b->clan_uid,r->clan_uid);strcpy(b->activation_uid,row[0]);
    b->family=(enum clan_buff_family)n[0];b->tier=(int)n[1];b->slot=(int)n[2];b->rules_version=(int)n[3];
    b->started_at=n[4];b->expires_at=n[5];b->next_available_at=n[6];b->paid_influence_eighths=n[7];b->canceled=n[8]!=0;
  }
  mysql_free_result(res);return CLAN_REPO_OK;
}
enum clan_repository_result clan_buff_activation_commit(MYSQL *db,
    const struct clan_progression_config *c,const struct clan_buff_request *r,int64_t now,
    struct clan_buff_decision *decision,struct json_object **out)
{
  enum clan_repository_result result;
  struct clan_buff_activation_snapshot s;
  struct clan_buff_decision d;
  struct clan_progression_readiness ready={true,false,false,false,CLAN_PROGRESSION_SCHEMA_VERSION};
  struct json_object *o=NULL;
  int64_t mode;
  const struct clan_buff_state *b;
  char *quoted=NULL;const char *json;
  if(!out||!decision||!clan_buff_request_valid(r)||now<0 || clan_progression_config_validate(c,NULL)!=CLAN_CONFIG_OK)return CLAN_REPO_INVALID;
  result=clan_buff_activation_schema_check(db);if(result!=CLAN_REPO_OK)return result;
  if(db->server_status&SERVER_STATUS_IN_TRANS)return CLAN_REPO_INVALID;
  if(!sql(db,"START TRANSACTION"))return CLAN_REPO_UNAVAILABLE;
  if(!scalar(db,"SELECT store_mode FROM clan_store_control WHERE singleton=1 FOR UPDATE",&mode)){result=CLAN_REPO_UNAVAILABLE;goto abort;}
  if(mode!=CLAN_STORE_MYSQL){result=CLAN_REPO_CONFLICT;goto abort;}
  result=replay(db,r,&o);
  if(result==CLAN_REPO_OK) {
    if(!sql(db,"ROLLBACK")){json_object_put(o);return CLAN_REPO_UNAVAILABLE;}
    receipt_decision(r,o,&d);*decision=d;*out=o;return CLAN_REPO_OK;
  }
  if(result!=CLAN_REPO_NOT_FOUND)goto abort;
  result=clan_buff_locked_snapshot(db,r,&s);if(result!=CLAN_REPO_OK)goto abort;
  clan_buff_validate(c,&ready,r,&s,now,&d);
  if(d.reasons) {
    if(!sql(db,"ROLLBACK"))return CLAN_REPO_UNAVAILABLE;
    *decision=d;return CLAN_REPO_INELIGIBLE;
  }
  b=&d.activation;
  if(s.clan.state_revision==INT64_MAX){result=CLAN_REPO_INVALID;goto abort;}
  if(!sql(db,"UPDATE clan_progression_state SET influence_eighths=influence_eighths-%" PRId64 ",revision=revision+1,updated_at_utc=%" PRId64 " WHERE clan_uid='%s' AND revision=%" PRId64 " AND influence_eighths>=%" PRId64,
      b->paid_influence_eighths,now,r->clan_uid,s.clan.state_revision,b->paid_influence_eighths)||mysql_affected_rows(db)!=1)goto failed;
  if(stop(1))goto failed;
  if(d.replaced_family && (!sql(db,"UPDATE clan_buff_families SET canceled=1 WHERE clan_uid='%s' AND family_id=%d AND activation_uid='%s' AND canceled=0",r->clan_uid,d.replaced_family,d.replaced_activation_uid)||mysql_affected_rows(db)!=1))goto failed;
  if(stop(2))goto failed;
  {
    size_t i;bool existing=false;
    for(i=0;i<s.count;i++)if(s.rows[i].family==r->family)existing=true;
    if(existing) {
      if(!sql(db,"UPDATE clan_buff_families SET activation_uid='%s',tier=%d,slot_id=%d,rules_version=%d,started_at=%" PRId64 ",expires_at=%" PRId64 ",next_available_at=%" PRId64 ",paid_influence_eighths=%" PRId64 ",canceled=0 WHERE clan_uid='%s' AND family_id=%d",
          r->operation_uid,r->tier,r->slot,r->rules_version,now,b->expires_at,b->next_available_at,b->paid_influence_eighths,r->clan_uid,r->family)||mysql_affected_rows(db)!=1)goto failed;
    } else if(!sql(db,"INSERT INTO clan_buff_families(clan_uid,activation_uid,family_id,tier,slot_id,rules_version,started_at,expires_at,next_available_at,paid_influence_eighths,canceled) VALUES('%s','%s',%d,%d,%d,%d,%" PRId64 ",%" PRId64 ",%" PRId64 ",%" PRId64 ",0)",
          r->clan_uid,r->operation_uid,r->family,r->tier,r->slot,r->rules_version,now,b->expires_at,b->next_available_at,b->paid_influence_eighths))goto failed;
  }
  if(stop(3))goto failed;
  o=receipt(r,&d,s.clan.influence_eighths);
  json=json_object_to_json_string_ext(o,JSON_C_TO_STRING_PLAIN);quoted=malloc(strlen(json)*2+1);
  if(!quoted)goto failed;
  mysql_real_escape_string(db,quoted,json,strlen(json));
  if(!sql(db,"INSERT INTO clan_buff_activations(operation_uid,clan_uid,actor_id,family_id,tier,slot_id,rules_version,expected_activation_uid,receipt) VALUES('%s','%s',%" PRId64 ",%d,%d,%d,%d,'%s','%s')",
      r->operation_uid,r->clan_uid,r->actor_id,r->family,r->tier,r->slot,r->rules_version,r->expected_activation_uid,quoted))goto failed;
  free(quoted);quoted=NULL;
  if(stop(4)||stop(5))goto failed;
  if(!sql(db,"COMMIT")){clan_buff_invalidate(r->clan_uid);json_object_put(o);return CLAN_REPO_COMMIT_UNKNOWN;}
  clan_buff_invalidate(r->clan_uid);
  if(stop(6)){json_object_put(o);return CLAN_REPO_COMMIT_UNKNOWN;}
  *decision=d;*out=o;return CLAN_REPO_OK;
failed:result=(mysql_errno(db)==1062||mysql_errno(db)==1205||mysql_errno(db)==1213)?CLAN_REPO_CONFLICT:CLAN_REPO_UNAVAILABLE;
abort:
  free(quoted);if(o)json_object_put(o);
  sql(db,"ROLLBACK");return result;
}
