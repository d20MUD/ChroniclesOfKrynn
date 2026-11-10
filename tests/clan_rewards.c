#include "clan_rewards.h"
#include "clan_awards.h"
#include <json-c/json.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static struct json_object *get(struct json_object *o,const char *key)
{struct json_object *v=NULL;json_object_object_get_ex(o,key,&v);return v;}
static int64_t n(struct json_object *o,const char *key){return json_object_get_int64(get(o,key));}
static void uid(char out[33],struct json_object *o,const char *key)
{const char *value=json_object_get_string(get(o,key));snprintf(out,33,"%s",value?value:"");}
static int calls;
static bool decide(const struct clan_reward_event *event,const struct clan_reward_snapshot *snapshot,struct clan_reward_delta *delta,void *context)
{
 struct json_object *request=context,*d=get(request,"delta");(void)event;
 assert(snapshot->clan.status==CLAN_RECORD_ACTIVE);calls++;
 if(json_object_get_boolean(get(request,"reject_callback")))return false;
#define F(key) delta->key=n(d,#key)
 F(xp_eighths);F(influence_eighths);F(favor_eighths);F(account_base_xp_eighths);F(account_claims);F(incoming_xp_eighths);
#undef F
 delta->cap_flags=(int)n(d,"cap_flags");return true;
}
int main(int argc,char **argv)
{
 char input[8192],operation[33];size_t size;struct json_object *request,*out,*result;MYSQL *db;enum clan_repository_result status=CLAN_REPO_INVALID;
 assert(argc==3);size=fread(input,1,sizeof(input)-1,stdin);input[size]=0;request=json_tokener_parse(input);assert(request);
 db=mysql_init(NULL);assert(db&&mysql_real_connect(db,NULL,"root","","clan_test",0,argv[1],0));assert(!mysql_query(db,"SET SESSION sql_mode='STRICT_ALL_TABLES,NO_ENGINE_SUBSTITUTION'"));
 clan_reward_fault=(int)n(request,"fault");clan_reward_crash=(int)n(request,"crash");
 uid(operation,request,"operation_uid");out=json_object_new_object();json_object_object_add(out,"untouched",json_object_new_boolean(true));
 {struct json_object *original=out;
 if(!strcmp(argv[2],"prepare") || !strcmp(argv[2],"prepare-member")){
  struct clan_reward_event e;memset(&e,0,sizeof(e));uid(e.operation_uid,request,"operation_uid");uid(e.event_uid,request,"event_uid");uid(e.clan_uid,request,"clan_uid");
  e.source=(enum clan_reward_source)n(request,"source");e.rules_version=(int)n(request,"rules_version");
#define F(key) e.key=n(request,#key)
  F(account_id);F(player_id);F(occurred_at);F(requested_xp_eighths);F(requested_influence_eighths);F(requested_favor_eighths);
#undef F
  status=!strcmp(argv[2],"prepare-member") ? clan_reward_prepare_member(db,&e,n(request,"now"),&out) : clan_reward_prepare(db,&e,n(request,"now"),&out);
 }else if(!strcmp(argv[2],"access")) {
  enum clan_member_access access;
  char clan_uid[33];uid(clan_uid,request,"clan_uid");
  status=clan_repository_member_access(db,n(request,"player_id"),clan_uid,&access);
  if(status==CLAN_REPO_OK)out=json_object_new_int(access);
 }else if(!strcmp(argv[2],"disband")) {
  char clan_uid[33];uid(clan_uid,request,"clan_uid");
  status=clan_repository_disband(db,clan_uid,n(request,"revision"),n(request,"now"));
  if(status==CLAN_REPO_OK)out=json_object_new_boolean(true);
 }else if((!strcmp(argv[2],"policy") || !strcmp(argv[2],"personal"))) {
  struct clan_progression_config config=clan_progression_defaults;
  struct clan_progression_readiness ready={true,false,false,false,CLAN_PROGRESSION_SCHEMA_VERSION};
  if(!json_object_get_boolean(get(request,"disabled"))) {
   config.controls.store_mode=CLAN_STORE_MYSQL;config.controls.mode=CLAN_PROGRESSION_LIVE;
   config.controls.public_release=true;
  }
  if(json_object_get_boolean(get(request,"unready")))ready.schema_ready=false;
  status=!strcmp(argv[2],"personal")
    ? clan_award_complete_personal(db,operation,n(request,"now"),&config,&ready,&out)
    : clan_award_complete_shared(db,operation,n(request,"now"),&config,&ready,&out);
 }else if(!strcmp(argv[2],"complete"))status=clan_reward_complete(db,operation,n(request,"now"),decide,request,&out);
 else if(!strcmp(argv[2],"load"))status=clan_reward_load(db,operation,&out);
 else if(!strcmp(argv[2],"cancel"))status=clan_reward_cancel(db,operation,(int)n(request,"reason"),n(request,"now"),&out);
 else if(!strcmp(argv[2],"pending"))status=clan_reward_pending(db,n(request,"player_id"),json_object_get_string(get(request,"cursor")),(size_t)n(request,"limit"),&out);
 if(status!=CLAN_REPO_OK)assert(out==original);else json_object_put(original);
 }
 result=json_object_new_object();json_object_object_add(result,"result",json_object_new_string(clan_repository_result_name(status)));json_object_object_add(result,"calls",json_object_new_int(calls));json_object_object_add(result,"receipt",out);
 puts(json_object_to_json_string_ext(result,JSON_C_TO_STRING_PLAIN));json_object_put(result);json_object_put(request);mysql_close(db);return 0;
}
