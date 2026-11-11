#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "clan.h"
#include "mysql.h"
#include "clan_persistence.h"
#include "clan_buff_commands.h"
#include "clan_buff_runtime.h"
#include "clan_buff_lifecycle.h"
#include "clan_status.h"
#include "legendary_identity.h"
#include <inttypes.h>
#include <json-c/json.h>
static int number(const char *s,int maximum)
{
  int n=0;
  if(!*s)return -1;
  for(;*s;s++){if(*s<'0'||*s>'9'||n>maximum)return -1;n=n*10+*s-'0';}
  return n<=maximum?n:-1;
}
static void usage(struct char_data *ch)
{
  send_to_char(ch,"clan buffs list\r\nclan buffs activate <family 1-5> <tier 1-3> <slot 1-3> [operation-uid clan-uid [replace-activation-uid]]\r\nclan buffs replace <family 1-5> <tier 1-3> <slot 1-3> <active-uid>\r\nclan buffs cancel <activation-uid> [operation-uid clan-uid]\r\nclan buffs permission <maximum-rank; 0 leader-only>\r\n");
}
static const char *stamp(int64_t value,char out[64])
{
  time_t t=(time_t)value;struct tm date;
  if((int64_t)t==value&&gmtime_r(&t,&date)&&strftime(out,64,"%Y-%m-%d %H:%M:%S UTC",&date))return out;
  return "unavailable timestamp";
}
static void rejection(struct char_data *ch,unsigned int reasons)
{
  unsigned int bit;
  for(bit=1;bit<=CLAN_BUFF_CLOCK;bit<<=1)if(reasons&bit)
    send_to_char(ch,"%s\r\n",clan_buff_rejection_text(bit));
}
void clan_buff_command(struct char_data *ch,const char *argument,int64_t now)
{
  char tokens[9][64]={{0}},operation[33]="",uid[33]="",expected[33]="",output[16384];
  const char *p=argument?argument:"";int count=0,action=0,family=0,tier=0,slot=0,rank=0;
  bool explicit_retry=false;
  MYSQL_POOL_CONN *connection;struct clan_status_report report;
  const struct clan_progression_config *policy=clan_buff_runtime_policy();
  enum clan_repository_result result;struct json_object *receipt=NULL,*value;
  if(!ch||IS_NPC(ch))return;
  while(*p) {
    size_t length=0;while(isspace((unsigned char)*p))p++;if(!*p)break;
    if(count==9){usage(ch);return;}
    while(*p&&!isspace((unsigned char)*p)) {
      if(length>=63){usage(ch);return;}tokens[count][length++]=*p++;
    }
    count++;
  }
  if(!count||!strcmp(tokens[0],"list")){if(count>1){usage(ch);return;}}
  else if(!strcmp(tokens[0],"activate")||!strcmp(tokens[0],"replace")) {
    bool replacing=!strcmp(tokens[0],"replace");
    if(replacing) {
      if(count!=5||!clan_progression_uid_valid(tokens[4])){usage(ch);return;}
      strcpy(expected,tokens[4]);
    } else if(count!=4&&count!=6&&count!=7){usage(ch);return;}
    action=1;family=number(tokens[1],5);tier=number(tokens[2],3);slot=number(tokens[3],3);
    if(family<1||tier<1||slot<1){usage(ch);return;}
    if(!replacing&&count>=6){explicit_retry=true;if(!clan_progression_uid_valid(tokens[4])||!clan_progression_uid_valid(tokens[5])||(count==7&&!clan_progression_uid_valid(tokens[6]))){usage(ch);return;}strcpy(operation,tokens[4]);strcpy(uid,tokens[5]);if(count==7)strcpy(expected,tokens[6]);}
  } else if(!strcmp(tokens[0],"cancel")) {
    if((count!=2&&count!=4)||!clan_progression_uid_valid(tokens[1])){usage(ch);return;}
    action=2;if(count==4){explicit_retry=true;if(!clan_progression_uid_valid(tokens[2])||!clan_progression_uid_valid(tokens[3])){usage(ch);return;}strcpy(operation,tokens[2]);strcpy(uid,tokens[3]);}
  } else if(!strcmp(tokens[0],"permission")) {
    if(count!=2||(rank=number(tokens[1],15))<0){usage(ch);return;}action=3;
  } else {usage(ch);return;}
  if(!ch->desc||!ch->desc->account||ch->desc->account->id<=0){send_to_char(ch,"An authenticated account is required.\r\n");return;}
  if(!clan_persistence_sql_authority()||!clan_persistence_ready()||!mysql_available||!(connection=mysql_pool_acquire())) {
    send_to_char(ch,"Authoritative clan SQL storage is unavailable; nothing was changed.\r\n");return;
  }
  if(!explicit_retry) {
    result=clan_repository_status_read(connection->conn,GET_IDNUM(ch),ch->desc->account->id,NULL,false,now,&report);
    if(result!=CLAN_REPO_OK||!report.full_clan_access||report.membership!=CLAN_ACCESS_ACCEPTED){send_to_char(ch,"Current accepted clan membership is required (%s).\r\n",clan_repository_result_name(result));goto done;}
    strcpy(uid,report.clan.uid);
  }
  if(!action) {
    int i,j;
    if(!clan_status_render(policy,&report,output,sizeof(output))){send_to_char(ch,"Buff state could not be read safely.\r\n");goto done;}
    send_to_char(ch,"%s",output);
    for(i=0;i<CLAN_PROGRESSION_BUFF_COUNT;i++) {
      const struct clan_buff_definition *b=&policy->buffs[i];
      send_to_char(ch,"Family %d: %s",b->family,b->name);
      for(j=0;j<3;j++)send_to_char(ch,"; tier %d: level %d, cost %" PRId64 " Influence",j+1,b->tiers[j].unlock_level,b->tiers[j].influence_points);
      send_to_char(ch,"\r\n");
    }
    for(i=0;i<(int)report.buff_count;i++)if(!report.buffs[i].canceled&&report.buffs[i].expires_at>now)
      send_to_char(ch,"Slot %d activation UID: %s\r\n",report.buffs[i].slot,report.buffs[i].activation_uid);
    usage(ch);goto done;
  }
  result=clan_buff_lifecycle_schema_check(connection->conn);
  if(result!=CLAN_REPO_OK) {
    send_to_char(ch,"Buff lifecycle storage is not ready (%s); nothing was changed.\r\n",clan_repository_result_name(result));goto done;
  }
  if(action==3) {
    result=clan_buff_management_set(connection->conn,policy,uid,GET_IDNUM(ch),rank);
    send_to_char(ch,"Buff permission update: %s. Only the accepted clan leader may delegate, within existing ranks; buffs must be enabled.\r\n",clan_repository_result_name(result));goto done;
  }
  if(!operation[0]&&!legendary_uuid_generate(operation)){send_to_char(ch,"Could not create a retry identity; nothing changed.\r\n");goto done;}
  if(action==1) {
    struct clan_buff_request request={0};struct clan_buff_decision decision={0};
    strcpy(request.clan_uid,uid);strcpy(request.operation_uid,operation);request.actor_id=GET_IDNUM(ch);
    request.family=family;request.tier=tier;request.slot=slot;request.rules_version=policy->rules_version;
    strcpy(request.expected_activation_uid,expected);
    send_to_char(ch,"Exact retry: clan buffs activate %d %d %d %s %s%s%s\r\n",family,tier,slot,operation,uid,expected[0]?" ":"",expected);
    result=clan_buff_activation_commit(connection->conn,policy,&request,now,&decision,&receipt);
    if(result==CLAN_REPO_INELIGIBLE)rejection(ch,decision.reasons);
  } else {
    struct clan_buff_cancel_request request={0};unsigned int reasons=0;
    strcpy(request.clan_uid,uid);strcpy(request.operation_uid,operation);strcpy(request.activation_uid,tokens[1]);request.actor_id=GET_IDNUM(ch);
    send_to_char(ch,"Exact retry: clan buffs cancel %s %s %s\r\n",tokens[1],operation,uid);
    result=clan_buff_cancel(connection->conn,policy,&request,now,&reasons,&receipt);
    if(result==CLAN_REPO_INELIGIBLE)rejection(ch,reasons);
  }
  send_to_char(ch,"Buff operation: %s.%s\r\n",clan_repository_result_name(result),result==CLAN_REPO_COMMIT_UNKNOWN?" Use the exact retry above; do not generate a new operation ID.":"");
  if(receipt) {
    char when[64];
    if(json_object_object_get_ex(receipt,"paid_influence_eighths",&value))
      send_to_char(ch,"Original price: %" PRId64 " Influence. Cancellation refunds nothing.\r\n",json_object_get_int64(value)/8);
    if(json_object_object_get_ex(receipt,"expires_at",&value))
      send_to_char(ch,"Original expiration: %s.\r\n",stamp(json_object_get_int64(value),when));
    if(json_object_object_get_ex(receipt,"next_available_at",&value))
      send_to_char(ch,"Family next available: %s.\r\n",stamp(json_object_get_int64(value),when));
    json_object_put(receipt);
  }
done:
  mysql_pool_release(connection);
  /* Invalidation callbacks did not use the owned connection. Hydrate only now. */
  clan_buff_runtime_tick(now);
}
ACMD(do_clanbuffs) {clan_buff_command(ch,argument,(int64_t)time(NULL));}
