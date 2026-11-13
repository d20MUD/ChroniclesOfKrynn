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
#include "clan_display.h"
static int number(const char *s,int maximum)
{
  int n=0;
  if(!*s)return -1;
  for(;*s;s++){if(*s<'0'||*s>'9'||n>maximum)return -1;n=n*10+*s-'0';}
  return n<=maximum?n:-1;
}
static void usage(struct char_data *ch)
{
  send_to_char(ch,"\r\n  What do these buffs do? Type help clanbuffs.\r\n\r\n  clan buffs list\r\n  clan buffs activate <name> <tier> <slot>\r\n"
                  "  clan buffs replace <name> <tier> <slot>\r\n"
                  "  clan buffs cancel <name or slot>\r\n"
                  "  clan buffs permission <maximum rank; 0 for leader only>\r\n");
}
static void rejection(struct char_data *ch,unsigned int reasons)
{
  unsigned int bit;
  for(bit=1;bit<=CLAN_BUFF_CLOCK;bit<<=1)if(reasons&bit)
    send_to_char(ch,"%s\r\n",clan_buff_rejection_text(bit));
}
void clan_buff_command(struct char_data *ch,const char *argument,int64_t now)
{
  char tokens[9][64]={{0}},operation[33]="",uid[33]="",expected[33]="";
  const char *p=argument?argument:"";int count=0,action=0,family=0,tier=0,slot=0,rank=0;
  bool explicit_retry=false, auto_replace=false;
  char cancel_name[512]="";
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
  /* Names may contain spaces: the final two words are tier and slot.
   * Existing numeric retry commands remain accepted for uncertain operations. */
  if (count>=4 && (!strcmp(tokens[0],"activate") || !strcmp(tokens[0],"replace")) && number(tokens[1],5)<1) {
    char name[512]=""; int i, best=0, found=-1;
    for (i=1;i<count-2;i++) { if (i>1) strcat(name," "); strcat(name,tokens[i]); }
    for (i=0;i<CLAN_PROGRESSION_BUFF_COUNT;i++) {
      int score=clan_display_match(name,policy->buffs[i].name,"");
      if (score>best) {best=score;found=i;}
      else if (score && score==best) found=-2;
    }
    if (found<0) {send_to_char(ch,"Please use a unique buff name from clan buffs.\r\n");return;}
    snprintf(tokens[1],sizeof(tokens[1]),"%d",policy->buffs[found].family);
    memmove(tokens[2],tokens[count-2],sizeof(tokens[2])); memmove(tokens[3],tokens[count-1],sizeof(tokens[3])); count=4;
  }
  if (count>=2 && !strcmp(tokens[0],"cancel") && !clan_progression_uid_valid(tokens[1])) {
    int i; for(i=1;i<count;i++){if(i>1)strcat(cancel_name," ");strcat(cancel_name,tokens[i]);}
    count=2;
  }
  if(!count||!strcmp(tokens[0],"list")){if(count>1){usage(ch);return;}}
  else if(!strcmp(tokens[0],"activate")||!strcmp(tokens[0],"replace")) {
    bool replacing=!strcmp(tokens[0],"replace");
    if(replacing) {
      if(count==4) auto_replace=true;
      else if(count!=5||!clan_progression_uid_valid(tokens[4])){usage(ch);return;}
      else strcpy(expected,tokens[4]);
    } else if(count!=4&&count!=6&&count!=7){usage(ch);return;}
    action=1;family=number(tokens[1],5);tier=number(tokens[2],3);slot=number(tokens[3],3);
    if(family<1||tier<1||slot<1){usage(ch);return;}
    if(!replacing&&count>=6){explicit_retry=true;if(!clan_progression_uid_valid(tokens[4])||!clan_progression_uid_valid(tokens[5])||(count==7&&!clan_progression_uid_valid(tokens[6]))){usage(ch);return;}strcpy(operation,tokens[4]);strcpy(uid,tokens[5]);if(count==7)strcpy(expected,tokens[6]);}
  } else if(!strcmp(tokens[0],"cancel")) {
    if((count!=2&&count!=4)||(!*cancel_name&&!clan_progression_uid_valid(tokens[1]))){usage(ch);return;}
    action=2;if(count==4){explicit_retry=true;if(!clan_progression_uid_valid(tokens[2])||!clan_progression_uid_valid(tokens[3])){usage(ch);return;}strcpy(operation,tokens[2]);strcpy(uid,tokens[3]);}
  } else if(!strcmp(tokens[0],"permission")) {
    if(count!=2||(rank=number(tokens[1],15))<0){usage(ch);return;}action=3;
  } else {usage(ch);return;}
  if(!ch->desc||!ch->desc->account||ch->desc->account->id<=0){send_to_char(ch,"An authenticated account is required.\r\n");return;}
  if(!clan_persistence_sql_authority()||!clan_persistence_ready()||!mysql_available||!(connection=mysql_pool_acquire())) {
    send_to_char(ch,"Clan buffs are temporarily unavailable. Nothing was changed.\r\n");return;
  }
  if(!explicit_retry) {
    result=clan_repository_status_read(connection->conn,GET_IDNUM(ch),ch->desc->account->id,NULL,false,now,&report);
    if(result!=CLAN_REPO_OK||!report.full_clan_access||report.membership!=CLAN_ACCESS_ACCEPTED){send_to_char(ch,"Current accepted clan membership is required (%s).\r\n",clan_repository_result_name(result));goto done;}
    strcpy(uid,report.clan.uid);
  }
  if (auto_replace || *cancel_name) {
    int i, matched=-1, best=0;
    for (i=0;i<(int)report.buff_count;i++) {
      const struct clan_buff_state *active=&report.buffs[i];
      const struct clan_buff_definition *d=clan_progression_buff(policy,active->family);
      char slot_name[16]; snprintf(slot_name,sizeof(slot_name),"%d",active->slot);
      if (active->canceled || active->expires_at<=now) continue;
      int score=auto_replace ? (active->slot==slot ? 2 : 0) : clan_display_match(cancel_name,d->name,slot_name);
      if(score>best){best=score;matched=i;}
      else if(score && score==best)matched=-2;
    }
    if(matched<0){send_to_char(ch,"No unique active buff matches. Check clan buffs and use its name or slot.\r\n");goto done;}
    if(auto_replace)strcpy(expected,report.buffs[matched].activation_uid);
    else strcpy(tokens[1],report.buffs[matched].activation_uid);
  }
  if(!action) {
    int i,j; char when[64];
    send_to_char(ch,"\r\n\tC%s - Clan Buffs\tn\r\n"
                    "  Activate a buff by name, tier and slot.\r\n\r\n",report.clan.name);
    for(i=0;i<CLAN_PROGRESSION_BUFF_COUNT;i++) {
      const struct clan_buff_definition *b=&policy->buffs[i];
      send_to_char(ch,"  \tW%s\tn\r\n",b->name);
      for(j=0;j<3;j++)send_to_char(ch,"    Tier %d  |  Clan level %-2d  |  %" PRId64 " Influence\r\n",j+1,b->tiers[j].unlock_level,b->tiers[j].influence_points);
      for(j=0;j<(int)report.buff_count;j++) if(report.buffs[j].family==b->family) {
        const struct clan_buff_state *active=&report.buffs[j];
        if(!active->canceled && active->expires_at>now)
          send_to_char(ch,"    \tGActive: tier %d in slot %d\tn - ends %s\r\n",active->tier,active->slot,clan_display_time(active->expires_at,when));
        if(active->next_available_at>now)
          send_to_char(ch,"    Ready again: %s\r\n",clan_display_time(active->next_available_at,when));
      }
      send_to_char(ch,"\r\n");
    }
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
    request.family=family;request.tier=tier;request.slot=slot;request.rules_version=1;
    strcpy(request.expected_activation_uid,expected);
    result=clan_buff_activation_commit(connection->conn,policy,&request,now,&decision,&receipt);
    if(result==CLAN_REPO_INELIGIBLE)rejection(ch,decision.reasons);
  } else {
    struct clan_buff_cancel_request request={0};unsigned int reasons=0;
    strcpy(request.clan_uid,uid);strcpy(request.operation_uid,operation);strcpy(request.activation_uid,tokens[1]);request.actor_id=GET_IDNUM(ch);
    result=clan_buff_cancel(connection->conn,policy,&request,now,&reasons,&receipt);
    if(result==CLAN_REPO_INELIGIBLE)rejection(ch,reasons);
  }
  if(result==CLAN_REPO_COMMIT_UNKNOWN) {
    send_to_char(ch,"The result could not be confirmed. To check this same request safely, use:\r\n");
    if(action==1)send_to_char(ch,"clan buffs activate %d %d %d %s %s%s%s\r\n",family,tier,slot,operation,uid,*expected?" ":"",expected);
    else send_to_char(ch,"clan buffs cancel %s %s %s\r\n",tokens[1],operation,uid);
  } else if(result==CLAN_REPO_OK)
    send_to_char(ch,action==1?"\tGYour clan buff is active.\tn\r\n":"The buff has been cancelled. Its cooldown still applies.\r\n");
  else if(result!=CLAN_REPO_INELIGIBLE)
    send_to_char(ch,"The buff could not be changed. Check clan buffs and try again shortly.\r\n");
  if(receipt) {
    char when[64];
    if(json_object_object_get_ex(receipt,"paid_influence_eighths",&value))
      send_to_char(ch,"Cost: %" PRId64 " Influence. Cancellation refunds nothing.\r\n",json_object_get_int64(value)/8);
    if(json_object_object_get_ex(receipt,"expires_at",&value))
      send_to_char(ch,"Ends: %s.\r\n",clan_display_time(json_object_get_int64(value),when));
    if(json_object_object_get_ex(receipt,"next_available_at",&value))
      send_to_char(ch,"Ready again: %s.\r\n",clan_display_time(json_object_get_int64(value),when));
    json_object_put(receipt);
  }
done:
  mysql_pool_release(connection);
  /* Invalidation callbacks did not use the owned connection. Hydrate only now. */
  clan_buff_runtime_tick(now);
}
ACMD(do_clanbuffs) {clan_buff_command(ch,argument,(int64_t)time(NULL));}
