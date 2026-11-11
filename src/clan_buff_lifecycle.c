#define _POSIX_C_SOURCE 200809L
#include "clan_buff_lifecycle.h"
#include "clan_awards.h"
#include <json-c/json.h>
#include <errno.h>
#include <ctype.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef CLAN_BUFF_LIFECYCLE_TEST
#include <unistd.h>
int clan_buff_lifecycle_fault,clan_buff_lifecycle_crash;
static bool stop(int n) {if(clan_buff_lifecycle_crash==n)_exit(110+n);return clan_buff_lifecycle_fault==n;}
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
static enum clan_repository_result begin(MYSQL *db)
{
  enum clan_repository_result result=clan_buff_activation_schema_check(db);
  MYSQL_RES *res;MYSQL_ROW row;int64_t n;size_t i;bool ok;
  char actual[16384],expected[1024];
  static const char *fragments[]={"PRIMARY KEY(event_kind,operation_uid)","UNIQUE KEY cble_one_transition(activation_uid)",
    "CONSTRAINT cble_clan FOREIGN KEY(clan_uid) REFERENCES clan_progression_clans(clan_uid)",
    "CONSTRAINT cble_event CHECK((event_kind=1 AND actor_id>0) OR (event_kind=2 AND actor_id=0))",
    "CONSTRAINT cble_time CHECK(occurred_at>=0)","CONSTRAINT cble_json CHECK(JSON_VALID(receipt))"};
  if(result!=CLAN_REPO_OK)return result;
  if(db->server_status&SERVER_STATUS_IN_TRANS)return CLAN_REPO_INVALID;
  if(!scalar(db,"SELECT version FROM clan_buff_lifecycle_schema WHERE singleton=1",&n)||n!=1)return CLAN_REPO_SCHEMA;
  if(!sql(db,"SHOW CREATE TABLE clan_buff_lifecycle_events")||!(res=mysql_store_result(db)))return CLAN_REPO_SCHEMA;
  row=mysql_fetch_row(res);ok=row&&row[1]&&strlen(row[1])<sizeof(actual);
  if(ok) {canonical(actual,row[1]);ok=strstr(actual,"engine=innodb")!=NULL;}
  mysql_free_result(res);
  for(i=0;ok&&i<sizeof(fragments)/sizeof(*fragments);i++){canonical(expected,fragments[i]);ok=strstr(actual,expected)!=NULL;}
  if(!ok)return CLAN_REPO_SCHEMA;
  for(i=0;i<2;i++) {
    const char *body=i?"BEGIN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Buff lifecycle history is retained'; END":"BEGIN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Buff lifecycle events are immutable'; END";
    if(!sql(db,"SELECT ACTION_STATEMENT,EVENT_MANIPULATION,ACTION_TIMING FROM information_schema.TRIGGERS WHERE TRIGGER_SCHEMA=DATABASE() AND EVENT_OBJECT_TABLE='clan_buff_lifecycle_events' AND TRIGGER_NAME='%s'",i?"cble_retained":"cble_immutable")||!(res=mysql_store_result(db)))return CLAN_REPO_SCHEMA;
    row=mysql_fetch_row(res);ok=row&&row[0]&&row[1]&&row[2]&&strlen(row[0])<sizeof(actual);
    if(ok){canonical(actual,row[0]);canonical(expected,body);ok=!strcmp(actual,expected)&&!strcmp(row[1],i?"DELETE":"UPDATE")&&!strcmp(row[2],"BEFORE");}
    mysql_free_result(res);if(!ok)return CLAN_REPO_SCHEMA;
  }
  if(!sql(db,"START TRANSACTION"))return CLAN_REPO_UNAVAILABLE;
  if(!scalar(db,"SELECT store_mode FROM clan_store_control WHERE singleton=1 FOR UPDATE",&n)||n!=CLAN_STORE_MYSQL){sql(db,"ROLLBACK");return CLAN_REPO_CONFLICT;}
  return CLAN_REPO_OK;
}
static struct json_object *proof(const char *operation,const struct clan_buff_state *b,int kind,int64_t actor,int64_t now)
{
  struct json_object *o=json_object_new_object();
#define S(k,v) json_object_object_add(o,k,json_object_new_string(v))
#define N(k,v) json_object_object_add(o,k,json_object_new_int64(v))
  S("operation_uid",operation);S("clan_uid",b->clan_uid);S("activation_uid",b->activation_uid);
  N("event_kind",kind);N("actor_id",actor);N("family",b->family);N("tier",b->tier);N("slot",b->slot);
  N("occurred_at",kind==2?b->expires_at:now);N("processed_at",now);
  N("expires_at",b->expires_at);N("next_available_at",b->next_available_at);N("paid_influence_eighths",b->paid_influence_eighths);
#undef S
#undef N
  return o;
}
static bool transition(MYSQL *db,const char *operation,const struct clan_buff_state *b,int kind,int64_t actor,int64_t now,struct json_object **out)
{
  struct json_object *o=proof(operation,b,kind,actor,now);const char *text=json_object_to_json_string_ext(o,JSON_C_TO_STRING_PLAIN);
  char *quoted=malloc(strlen(text)*2+1);bool ok=false;
  if(!quoted){json_object_put(o);return false;}
  mysql_real_escape_string(db,quoted,text,strlen(text));
  if(sql(db,"UPDATE clan_buff_families SET canceled=1 WHERE clan_uid='%s' AND family_id=%d AND activation_uid='%s' AND canceled=0",b->clan_uid,b->family,b->activation_uid)&&mysql_affected_rows(db)==1&&!stop(1))
    ok=sql(db,"INSERT INTO clan_buff_lifecycle_events(operation_uid,clan_uid,activation_uid,actor_id,event_kind,occurred_at,receipt) VALUES('%s','%s','%s',%" PRId64 ",%d,%" PRId64 ",'%s')",operation,b->clan_uid,b->activation_uid,actor,kind,kind==2?b->expires_at:now,quoted)&&!stop(2);
  free(quoted);if(ok)*out=o;else json_object_put(o);return ok;
}
static bool cancel_proof_valid(struct json_object *o,const struct clan_buff_cancel_request *r)
{
  static const char *keys[]={"event_kind","actor_id","family","tier","slot","occurred_at","processed_at","expires_at","next_available_at","paid_influence_eighths"};
  const char *names[]={"operation_uid","clan_uid","activation_uid"};
  const char *values[]={r->operation_uid,r->clan_uid,r->activation_uid};
  struct json_object *v;int64_t n[10];size_t i;
  if(!o||!json_object_is_type(o,json_type_object)||json_object_object_length(o)!=13)return false;
  for(i=0;i<3;i++)if(!json_object_object_get_ex(o,names[i],&v)||!json_object_is_type(v,json_type_string)||json_object_get_string_len(v)!=32||strcmp(json_object_get_string(v),values[i]))return false;
  for(i=0;i<10;i++)if(!json_object_object_get_ex(o,keys[i],&v)||!json_object_is_type(v,json_type_int)||(n[i]=json_object_get_int64(v))<0)return false;
  return n[0]==1&&n[1]==r->actor_id&&n[2]>=1&&n[2]<=5&&n[3]>=1&&n[3]<=3&&n[4]>=1&&n[4]<=3&&n[6]>=n[5]&&n[7]>n[5]&&n[8]>=n[7]&&n[9]>0;
}
enum clan_repository_result clan_buff_cancel(MYSQL *db,const struct clan_progression_config *c,
    const struct clan_buff_cancel_request *r,int64_t now,unsigned int *reasons,struct json_object **out)
{
  enum clan_repository_result result;
  struct clan_buff_request request;
  struct clan_buff_activation_snapshot s;
  struct clan_progression_readiness ready={true,false,false,false,CLAN_PROGRESSION_SCHEMA_VERSION};
  struct json_object *o=NULL;
  MYSQL_RES *res;MYSQL_ROW row;unsigned int why=0;size_t i;int64_t actor;
  if(!r||!out||!reasons||now<0||!memchr(r->operation_uid,0,33)||!memchr(r->clan_uid,0,33)||!memchr(r->activation_uid,0,33)||
      !clan_progression_uid_valid(r->operation_uid)||!clan_progression_uid_valid(r->clan_uid)||!clan_progression_uid_valid(r->activation_uid)||r->actor_id<=0||clan_progression_config_validate(c,NULL)!=CLAN_CONFIG_OK)return CLAN_REPO_INVALID;
  result=begin(db);if(result!=CLAN_REPO_OK)return result;
  if(!sql(db,"SELECT clan_uid,activation_uid,actor_id,receipt FROM clan_buff_lifecycle_events WHERE event_kind=1 AND operation_uid='%s'",r->operation_uid)||!(res=mysql_store_result(db)))goto failed;
  row=mysql_fetch_row(res);
  if(row) {
    bool matches=row[0]&&row[1]&&integer(row[2],&actor)&&!strcmp(row[0],r->clan_uid)&&!strcmp(row[1],r->activation_uid)&&actor==r->actor_id;
    o=row[3]?json_tokener_parse(row[3]):NULL;mysql_free_result(res);
    if(!matches){result=CLAN_REPO_CONFLICT;goto abort;}
    if(!cancel_proof_valid(o,r)){result=CLAN_REPO_SCHEMA;goto abort;}
    if(!sql(db,"ROLLBACK"))goto failed;
    *out=o;*reasons=0;return CLAN_REPO_OK;
  }
  mysql_free_result(res);
  memset(&request,0,sizeof(request));strcpy(request.clan_uid,r->clan_uid);request.actor_id=r->actor_id;
  result=clan_buff_locked_snapshot(db,&request,&s);if(result!=CLAN_REPO_OK)goto abort;
  if(!clan_progression_feature_available(c,CLAN_FEATURE_BUFFS,&ready,r->clan_uid))why|=CLAN_BUFF_DISABLED;
  {
    int level;struct clan_buff_view view;
    if(!clan_repository_record_valid(&s.clan)||!clan_award_level(c,s.clan.xp_eighths,&level)||
        !clan_buff_query(c,s.clan.uid,(enum clan_progression_record_status)s.clan.status,level,s.rows,s.count,now,&view)) {result=CLAN_REPO_SCHEMA;goto abort;}
  }
  if(s.clan.status!=CLAN_RECORD_ACTIVE)why|=CLAN_BUFF_DELETED;
  if(!s.accepted_member)why|=CLAN_BUFF_NOT_MEMBER;
  if(!s.may_manage)why|=CLAN_BUFF_PERMISSION;
  for(i=0;i<s.count;i++)if(!strcmp(s.rows[i].activation_uid,r->activation_uid))break;
  if(i==s.count)why|=CLAN_BUFF_STALE;
  else if(!clan_repository_buff_valid(&s.rows[i])){result=CLAN_REPO_SCHEMA;goto abort;}
  else if(now<s.rows[i].started_at)why|=CLAN_BUFF_CLOCK;
  else if(!clan_repository_buff_active(&s.rows[i],now))why|=CLAN_BUFF_STALE;
  if(why){if(!sql(db,"ROLLBACK"))return CLAN_REPO_UNAVAILABLE;*reasons=why;return CLAN_REPO_INELIGIBLE;}
  if(!transition(db,r->operation_uid,&s.rows[i],1,r->actor_id,now,&o)||stop(3))goto failed;
  if(!sql(db,"COMMIT")){result=CLAN_REPO_COMMIT_UNKNOWN;goto uncertain;}
  clan_buff_invalidate(r->clan_uid);
  if(stop(4)){json_object_put(o);return CLAN_REPO_COMMIT_UNKNOWN;}
  *out=o;*reasons=0;return CLAN_REPO_OK;
failed:result=CLAN_REPO_UNAVAILABLE;
abort:sql(db,"ROLLBACK");if(o)json_object_put(o);return result;
uncertain:clan_buff_invalidate(r->clan_uid);if(o)json_object_put(o);return result;
}
enum clan_repository_result clan_buff_lifecycle_reconcile(MYSQL *db,int64_t now,
    struct json_object **out,struct clan_buff_state *rows,size_t capacity,size_t *count)
{
  struct clan_buff_state expired[125],timers[125];size_t n=0,total=0,i,j;
  struct json_object *root=NULL,*o=NULL;
  MYSQL_RES *res;MYSQL_ROW row;enum clan_repository_result result;
  if(!out||!rows||!count||now<0)return CLAN_REPO_INVALID;
  result=begin(db);if(result!=CLAN_REPO_OK)return result;
  if(!sql(db,"SELECT clan_uid,activation_uid,family_id,tier,slot_id,rules_version,started_at,expires_at,next_available_at,paid_influence_eighths,canceled FROM clan_buff_families WHERE canceled=0 AND expires_at<=%" PRId64 " ORDER BY clan_uid,family_id LIMIT 125 FOR UPDATE",now)||!(res=mysql_store_result(db)))goto failed;
  while((row=mysql_fetch_row(res))) {
    struct clan_buff_state *b=&expired[n];int64_t v[9];
    if(n==125||!row[0]||!row[1]||!clan_progression_uid_valid(row[0])||!clan_progression_uid_valid(row[1])){mysql_free_result(res);result=CLAN_REPO_SCHEMA;goto abort;}
    for(j=0;j<9;j++)if(!integer(row[j+2],&v[j])){mysql_free_result(res);result=CLAN_REPO_SCHEMA;goto abort;}
    if(v[0]>5||v[1]>3||v[2]>3||v[3]>INT32_MAX||v[8]!=0){mysql_free_result(res);result=CLAN_REPO_SCHEMA;goto abort;}
    memset(b,0,sizeof(*b));strcpy(b->clan_uid,row[0]);strcpy(b->activation_uid,row[1]);
    b->family=(enum clan_buff_family)v[0];b->tier=(int)v[1];b->slot=(int)v[2];b->rules_version=(int)v[3];
    b->started_at=v[4];b->expires_at=v[5];b->next_available_at=v[6];b->paid_influence_eighths=v[7];
    if(!clan_repository_buff_valid(b)){mysql_free_result(res);result=CLAN_REPO_SCHEMA;goto abort;}n++;
  }
  mysql_free_result(res);
  for(i=0;i<n;i++) {
    if(!transition(db,expired[i].activation_uid,&expired[i],2,0,now,&o))goto failed;
    json_object_put(o);o=NULL;
  }
  result=clan_repository_snapshot_read_locked(db,&root);if(result!=CLAN_REPO_OK)goto abort;
  if(!clan_repository_snapshot_valid(root)){result=CLAN_REPO_SCHEMA;goto abort;}
  result=clan_repository_buffs_read(db,timers,125,&total);if(result!=CLAN_REPO_OK)goto abort;
  if(total>capacity){result=CLAN_REPO_CAPACITY;goto abort;}
  if(stop(3))goto failed;
  if(!sql(db,"COMMIT")){for(i=0;i<n;i++)clan_buff_invalidate(expired[i].clan_uid);result=CLAN_REPO_COMMIT_UNKNOWN;goto abort;}
  for(i=0;i<n;i++)clan_buff_invalidate(expired[i].clan_uid);
  if(stop(4)){json_object_put(root);return CLAN_REPO_COMMIT_UNKNOWN;}
  memcpy(rows,timers,total*sizeof(*rows));*count=total;*out=root;return CLAN_REPO_OK;
failed:result=CLAN_REPO_UNAVAILABLE;
abort:sql(db,"ROLLBACK");if(root)json_object_put(root);if(o)json_object_put(o);return result;
}
