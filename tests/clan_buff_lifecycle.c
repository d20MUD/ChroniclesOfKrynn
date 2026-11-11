#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "mysql.h"
#include "clan_persistence.h"
#include "clan_buff_runtime.h"
#include "clan_buff_lifecycle.h"
#include "clan_buff_commands.h"
#include "perfmon.h"
#include "legendary.h"
#include <json-c/json.h>
#include <assert.h>
#include <limits.h>
void PERF_prof_sect_init(struct PERF_prof_sect **p,const char *id){(void)id;*p=NULL;}
void PERF_prof_sect_enter(struct PERF_prof_sect *p){(void)p;}
void PERF_prof_sect_exit(struct PERF_prof_sect *p){(void)p;}
static char command_output[32768];
size_t send_to_char(struct char_data *ch,const char *format,...)
{
  size_t used=strlen(command_output);int n;va_list args;(void)ch;
  va_start(args,format);n=vsnprintf(command_output+used,sizeof(command_output)-used,format,args);va_end(args);
  assert(n>=0&&(size_t)n<sizeof(command_output)-used);return (size_t)n;
}
bool legendary_uuid_generate(char out[33]){strcpy(out,"0123456789abcdef0123456789abcdef");return true;}
struct char_data *character_list;
struct player_special_data dummy_mob;
bool mysql_available=true;
static MYSQL_POOL_CONN connection;
static void (*member_callback)(struct char_data *,const char *);
static int64_t test_now;
static int rebuilds;
static int flat_bonus=25,passive_bonus=5;
static bool authority=true,ready=true;
time_t clan_test_time(time_t *out){if(out)*out=(time_t)test_now;return (time_t)test_now;}
int MIN(int a,int b){return a<b?a:b;}
int legendary_foundation_value(struct char_data *ch,int id,int subtype){(void)id;(void)subtype;return ch->legendary_serializing?0:flat_bonus;}
int legendary_item_passive_value(struct char_data *ch,int id){(void)id;return ch->legendary_serializing?0:passive_bonus;}
void legendary_foundation_hp(struct char_data *,int);
void calculate_max_hp(struct char_data *ch,bool display){(void)display;rebuilds++;GET_REAL_MAX_HIT(ch)=100;legendary_foundation_hp(ch,100);}
bool clan_persistence_sql_authority(void){return authority;}
bool clan_persistence_ready(void){return ready;}
void clan_persistence_set_member_invalidation(void (*callback)(struct char_data *,const char *)){member_callback=callback;}
MYSQL_POOL_CONN *mysql_pool_acquire(void){return &connection;}
void mysql_pool_release(MYSQL_POOL_CONN *c){assert(c==&connection);}
static int notifications;
static void notified(const char *uid,void *context){(void)context;assert(uid&&strlen(uid)==32);notifications++;}
static struct clan_progression_config live(void)
{
  struct clan_progression_config c=clan_progression_defaults;
  c.controls.store_mode=CLAN_STORE_MYSQL;c.controls.mode=CLAN_PROGRESSION_LIVE;c.controls.buffs=true;c.controls.public_release=true;
  return c;
}
static void artisan_runtime(int64_t now)
{
  struct char_data ch={0};struct player_special_data ps={0};struct clan_progression_config c=live();
  ch.player_specials=&ps;GET_IDNUM((&ch))=5000000007;character_list=&ch;test_now=now;
  clan_buff_runtime_tick(now);assert(!ch.clan_artisan_bonus);
  assert(clan_buff_runtime_configure(&c));clan_buff_runtime_tick(now);
  assert(ch.clan_scholar_percent==5); /* Distinct families coexist, no summing. */
  assert(ch.clan_artisan_bonus==2&&ch.clan_artisan_expires_at==now+86400);
  member_callback(&ch,NULL);assert(!ch.clan_artisan_bonus);
  clan_buff_runtime_tick(++now);assert(ch.clan_artisan_bonus==2);
  mysql_available=false;clan_buff_runtime_tick(++now);assert(!ch.clan_artisan_bonus);
  mysql_available=true;clan_buff_runtime_tick(++now);assert(ch.clan_artisan_bonus==2);
  c.controls.buffs=false;assert(clan_buff_runtime_configure(&c));assert(!ch.clan_artisan_bonus);
  c.controls.buffs=true;assert(clan_buff_runtime_configure(&c));clan_buff_runtime_tick(++now);assert(ch.clan_artisan_bonus==2);
  clan_buff_runtime_tick(ch.clan_artisan_expires_at);assert(!ch.clan_artisan_bonus);
  character_list=NULL;puts("Artisan SQL default gates, membership, outage, disable and absolute expiry passed");
}
static void scholar_runtime(int64_t now)
{
  struct char_data ch={0};struct player_special_data ps={0};struct clan_progression_config c=live();
  ch.player_specials=&ps;GET_IDNUM((&ch))=5000000007;character_list=&ch;test_now=now;
  clan_buff_runtime_tick(now);assert(!ch.clan_scholar_percent);
  assert(clan_buff_runtime_configure(&c));clan_buff_runtime_tick(now);
  assert(ch.clan_scholar_percent==5&&ch.clan_scholar_expires_at==now+86400);
  member_callback(&ch,NULL);assert(!ch.clan_scholar_percent);
  clan_buff_runtime_tick(++now);assert(ch.clan_scholar_percent==5);
  mysql_available=false;clan_buff_runtime_tick(++now);assert(!ch.clan_scholar_percent);
  mysql_available=true;clan_buff_runtime_tick(++now);assert(ch.clan_scholar_percent==5);
  c.controls.buffs=false;assert(clan_buff_runtime_configure(&c));assert(!ch.clan_scholar_percent);
  c.controls.buffs=true;assert(clan_buff_runtime_configure(&c));clan_buff_runtime_tick(++now);assert(ch.clan_scholar_percent==5);
  clan_buff_runtime_tick(ch.clan_scholar_expires_at);assert(!ch.clan_scholar_percent);
  character_list=NULL;puts("Scholar SQL default gates, membership, outage, disable and absolute expiry passed");
}
static void psionic_runtime(MYSQL *db,int64_t now)
{
  struct char_data ch={0};struct player_special_data ps={0};struct clan_progression_config c=live();
  struct clan_buff_request r={0};struct clan_buff_decision decision;struct json_object *receipt=NULL;
  ch.player_specials=&ps;GET_IDNUM((&ch))=5000000007;character_list=&ch;test_now=now;
  assert(clan_buff_runtime_configure(&c));clan_buff_runtime_tick(now);
  assert(ch.clan_psionic_percent==10&&ch.clan_psionic_expires_at==now+86400);
  assert(!strcmp(ch.clan_psionic_activation_uid,"bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"));
  ch.clan_psionic_remainder=70;clan_buff_runtime_tick(++now);assert(ch.clan_psionic_remainder==70);
  member_callback(&ch,NULL);assert(!ch.clan_psionic_percent&&!ch.clan_psionic_remainder);
  clan_buff_runtime_tick(++now);assert(ch.clan_psionic_percent==10);
  ch.clan_psionic_remainder=80;mysql_available=false;clan_buff_runtime_tick(++now);assert(!ch.clan_psionic_percent&&!ch.clan_psionic_remainder);
  mysql_available=true;clan_buff_runtime_tick(++now);assert(ch.clan_psionic_percent==10&&!ch.clan_psionic_remainder);
  ch.clan_psionic_remainder=90;c.controls.buffs=false;assert(clan_buff_runtime_configure(&c));assert(!ch.clan_psionic_remainder&&!ch.clan_psionic_percent);
  c.controls.buffs=true;assert(clan_buff_runtime_configure(&c));clan_buff_runtime_tick(++now);
  ch.clan_psionic_remainder=60;
  strcpy(r.clan_uid,"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");strcpy(r.operation_uid,"cccccccccccccccccccccccccccccccc");
  r.actor_id=5000000007;r.family=CLAN_BUFF_PSIONIC;r.tier=1;r.slot=1;r.rules_version=1;
  now=2200907200;test_now=now;
  assert(clan_buff_activation_commit(db,&c,&r,now,&decision,&receipt)==CLAN_REPO_OK);json_object_put(receipt);
  clan_buff_runtime_tick(now);assert(ch.clan_psionic_percent==10&&!ch.clan_psionic_remainder);
  assert(!strcmp(ch.clan_psionic_activation_uid,r.operation_uid));
  ch.clan_psionic_remainder=90;clan_buff_runtime_tick(ch.clan_psionic_expires_at);assert(!ch.clan_psionic_percent&&!ch.clan_psionic_remainder);
  character_list=NULL;puts("Psionic SQL hydration, identity/remainder, departure, disable, outage and expiry passed");
}
static void arcane_runtime(int64_t now)
{
  struct char_data ch={0};struct player_special_data ps={0};struct clan_progression_config c=live();
  ch.player_specials=&ps;GET_IDNUM((&ch))=5000000007;character_list=&ch;
  clan_buff_runtime_tick(now);assert(ch.clan_arcane_percent==0);
  assert(clan_buff_runtime_configure(&c));clan_buff_runtime_tick(now);
  assert(ch.clan_arcane_percent==10&&ch.clan_arcane_expires_at==now+86400);
  member_callback(&ch,NULL);assert(ch.clan_arcane_percent==0);
  clan_buff_runtime_tick(++now);assert(ch.clan_arcane_percent==10);
  mysql_available=false;clan_buff_runtime_tick(++now);assert(ch.clan_arcane_percent==0);
  mysql_available=true;clan_buff_runtime_tick(++now);assert(ch.clan_arcane_percent==10);
  clan_buff_runtime_tick(ch.clan_arcane_expires_at);assert(ch.clan_arcane_percent==0);
  character_list=NULL;puts("Arcane runtime eligibility and expiry passed");
}
static void runtime(MYSQL *db,int64_t now)
{
  struct char_data ch={0};struct player_special_data ps={0};struct clan_progression_config c=live();
  ch.player_specials=&ps;GET_IDNUM((&ch))=5000000007;GET_HIT((&ch))=80;character_list=&ch;
  test_now=now;calculate_max_hp(&ch,false);assert(GET_MAX_HIT((&ch))==130);
  /* Default OFF makes no SQL request or temporary bonus. */
  clan_buff_runtime_tick(now);assert(ch.clan_vitality_percent==0);
  assert(clan_buff_runtime_configure(&c));
  clan_buff_runtime_login_begin(&ch);assert(ch.clan_buff_login_pending);
  GET_HIT((&ch))=136;calculate_max_hp(&ch,false);assert(GET_HIT((&ch))==136);
  clan_buff_runtime_login_end(&ch,now);assert(!ch.clan_buff_login_pending&&GET_MAX_HIT((&ch))==136&&GET_HIT((&ch))==136);
  GET_HIT((&ch))=80;clan_buff_runtime_tick(now);
  assert(ch.clan_vitality_percent==5&&GET_MAX_HIT((&ch))==136&&GET_HIT((&ch))==80);
  assert(GET_REAL_MAX_HIT((&ch))==100);
  GET_HIT((&ch))=136;calculate_max_hp(&ch,false);calculate_max_hp(&ch,false);
  assert(GET_MAX_HIT((&ch))==136&&GET_HIT((&ch))==136);
  ch.clan_vitality_percent=10;calculate_max_hp(&ch,false);assert(GET_MAX_HIT((&ch))==143&&GET_HIT((&ch))==136);
  ch.clan_vitality_percent=15;
  for(int i=0;i<20;i++){calculate_max_hp(&ch,false);assert(GET_MAX_HIT((&ch))==149&&GET_HIT((&ch))==136);}
  ch.clan_vitality_percent=5;calculate_max_hp(&ch,false);
  ch.legendary_serializing=true;calculate_max_hp(&ch,false);
  assert(GET_MAX_HIT((&ch))==100&&GET_HIT((&ch))==136&&ch.clan_vitality_applied==6);
  ch.legendary_serializing=false;calculate_max_hp(&ch,false);assert(GET_HIT((&ch))==136);
  test_now++;clan_buff_runtime_tick(test_now);assert(GET_HIT((&ch))==136);
  assert(!mysql_query(db,"UPDATE clan_memberships SET rank_id=0 WHERE player_id=5000000007"));
  assert(member_callback);member_callback(&ch,NULL);
  assert(GET_MAX_HIT((&ch))==130&&GET_HIT((&ch))==130);
  clan_buff_runtime_tick(++test_now);assert(ch.clan_vitality_percent==0);
  assert(!mysql_query(db,"UPDATE clan_memberships SET rank_id=1 WHERE player_id=5000000007"));
  member_callback(&ch,"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");clan_buff_runtime_tick(++test_now);
  assert(GET_MAX_HIT((&ch))==136&&GET_HIT((&ch))==130);
  mysql_available=false;clan_buff_runtime_tick(++test_now);assert(GET_MAX_HIT((&ch))==130);
  mysql_available=true;clan_buff_runtime_tick(++test_now);assert(GET_MAX_HIT((&ch))==136&&GET_HIT((&ch))==130);
  c.controls.buffs=false;assert(clan_buff_runtime_configure(&c));assert(GET_MAX_HIT((&ch))==130);
  c.controls.buffs=true;assert(clan_buff_runtime_configure(&c));clan_buff_runtime_tick(++test_now);assert(GET_MAX_HIT((&ch))==136&&GET_HIT((&ch))==130);
  GET_HIT((&ch))=-7;calculate_max_hp(&ch,false);assert(GET_HIT((&ch))==-7);
  legendary_foundation_hp(&ch,INT_MAX);assert(GET_MAX_HIT((&ch))==INT_MAX&&GET_HIT((&ch))==-7);
  calculate_max_hp(&ch,false);
  flat_bonus=passive_bonus=0;GET_HIT((&ch))=10;legendary_foundation_hp(&ch,10);
  assert(GET_MAX_HIT((&ch))==10&&ch.clan_vitality_was_active&&ch.clan_vitality_applied==0);
  ch.clan_vitality_percent=0;legendary_foundation_hp(&ch,4);assert(GET_HIT((&ch))==4);
  flat_bonus=25;passive_bonus=5;ch.clan_vitality_percent=5;calculate_max_hp(&ch,false);
  test_now=ch.clan_vitality_expires_at;
  /* Absolute expiry gates HP before SQL maintenance has run. */
  GET_HIT((&ch))=136;calculate_max_hp(&ch,false);assert(GET_MAX_HIT((&ch))==130&&GET_HIT((&ch))==130);
  clan_buff_runtime_tick(test_now);assert(ch.clan_vitality_percent==0);
  clan_buff_runtime_login_begin(&ch);GET_HIT((&ch))=136;
  calculate_max_hp(&ch,false);assert(GET_HIT((&ch))==136);
  clan_buff_runtime_login_end(&ch,test_now);assert(GET_MAX_HIT((&ch))==130&&GET_HIT((&ch))==130);
  clan_buff_runtime_tick(++test_now);assert(GET_HIT((&ch))==130);
  character_list=NULL;
  puts("Runtime/HP: default gates, SQL reconciliation, departure/rejoin, outage/disable, no healing/compounding, serialization, overflow, negative HP and exact expiry passed.");
}
int main(int argc,char **argv)
{
  MYSQL *db;struct json_object *input,*v,*o=NULL,*answer;
  struct clan_progression_config c=live();char text[4096];const char *mode;
  struct clan_buff_request r;struct clan_buff_decision d;
  struct clan_buff_cancel_request cancel;
  struct clan_buff_state rows[125];size_t count=0;unsigned int reasons=0x5a5a;
  enum clan_repository_result result;int64_t now;
  assert(argc==2&&fgets(text,sizeof(text),stdin));input=json_tokener_parse(text);assert(input);
  assert(json_object_object_get_ex(input,"mode",&v));mode=json_object_get_string(v);
  assert(json_object_object_get_ex(input,"now",&v));now=json_object_get_int64(v);test_now=now;
  db=mysql_init(NULL);assert(mysql_real_connect(db,NULL,"root",NULL,"clan_test",0,argv[1],0));connection.conn=db;
  if(!strcmp(mode,"buff-command")) {
    struct char_data ch={0};struct descriptor_data desc={0};struct account_data account={0};
    const char *args="";struct json_object *answer=json_object_new_object();
    GET_IDNUM((&ch))=5000000007;ch.desc=&desc;desc.character=&ch;desc.account=&account;account.id=77;
    if(json_object_object_get_ex(input,"actor",&v))GET_IDNUM((&ch))=json_object_get_int64(v);
    if(json_object_object_get_ex(input,"account",&v))account.id=json_object_get_int(v);
    if(json_object_object_get_ex(input,"disabled",&v))c.controls.buffs=!json_object_get_boolean(v);
    if(json_object_object_get_ex(input,"args",&v))args=json_object_get_string(v);
    assert(clan_buff_runtime_configure(&c));clan_buff_command(&ch,args,now);
    json_object_object_add(answer,"text",json_object_new_string(command_output));puts(json_object_to_json_string(answer));
    json_object_put(answer);json_object_put(input);mysql_close(db);return 0;
  }
  if(!strcmp(mode,"artisan-runtime")){artisan_runtime(now);json_object_put(input);mysql_close(db);return 0;}
  if(!strcmp(mode,"scholar-runtime")){scholar_runtime(now);json_object_put(input);mysql_close(db);return 0;}
  if(!strcmp(mode,"psionic-runtime")){psionic_runtime(db,now);json_object_put(input);mysql_close(db);return 0;}
  if(!strcmp(mode,"arcane-runtime")){arcane_runtime(now);json_object_put(input);mysql_close(db);return 0;}
  if(!strcmp(mode,"runtime")){runtime(db,now);json_object_put(input);mysql_close(db);return 0;}
  if(json_object_object_get_ex(input,"fault",&v))clan_buff_lifecycle_fault=json_object_get_int(v);
  if(json_object_object_get_ex(input,"crash",&v))clan_buff_lifecycle_crash=json_object_get_int(v);
  if(json_object_object_get_ex(input,"disabled",&v))c.controls.buffs=!json_object_get_boolean(v);
  clan_buff_set_invalidation(notified,NULL);
  memset(&r,0,sizeof(r));strcpy(r.clan_uid,"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");r.actor_id=5000000007;r.family=4;r.tier=1;r.slot=1;r.rules_version=1;
  if(json_object_object_get_ex(input,"slot",&v))r.slot=json_object_get_int(v);
  if(json_object_object_get_ex(input,"family",&v))r.family=json_object_get_int(v);
  if(json_object_object_get_ex(input,"operation_uid",&v))strcpy(r.operation_uid,json_object_get_string(v));
  if(json_object_object_get_ex(input,"actor",&v))r.actor_id=json_object_get_int64(v);
  if(!strcmp(mode,"activate"))result=clan_buff_activation_commit(db,&c,&r,now,&d,&o);
  else if(!strcmp(mode,"cancel")) {
    memset(&cancel,0,sizeof(cancel));strcpy(cancel.operation_uid,r.operation_uid);strcpy(cancel.clan_uid,r.clan_uid);cancel.actor_id=r.actor_id;
    assert(json_object_object_get_ex(input,"activation_uid",&v));strcpy(cancel.activation_uid,json_object_get_string(v));
    result=clan_buff_cancel(db,&c,&cancel,now,&reasons,&o);
  } else {result=clan_buff_lifecycle_reconcile(db,now,&o,rows,125,&count);}
  answer=json_object_new_object();json_object_object_add(answer,"result",json_object_new_string(clan_repository_result_name(result)));
  json_object_object_add(answer,"reasons",json_object_new_int64(reasons));json_object_object_add(answer,"count",json_object_new_int64(count));
  json_object_object_add(answer,"notifications",json_object_new_int(notifications));
  if(o)json_object_object_add(answer,"receipt",o);
  if(result!=CLAN_REPO_OK&&result!=CLAN_REPO_INELIGIBLE)assert(!o&&reasons==0x5a5a);
  puts(json_object_to_json_string_ext(answer,JSON_C_TO_STRING_PLAIN));json_object_put(answer);json_object_put(input);mysql_close(db);return 0;
}
