#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "clan.h"
#include "clan_commands.h"
#include "clan_persistence.h"
#include "clan_status.h"
#include "clan_awards.h"
#include "mysql.h"
#include <json-c/json.h>
#include <errno.h>
#include <inttypes.h>
#include <time.h>
static bool positive_id(const char *s,int64_t *out)
{
  char *end;intmax_t id;
  if (!s || !*s || *s<'0' || *s>'9') return false;
  errno=0;id=strtoimax(s,&end,10);
  if (errno || *end || id<=0 || id>INT64_MAX) return false;
  *out=(int64_t)id;return true;
}
void clan_status_command(struct char_data *ch,const char *arguments,bool inspect,int64_t now)
{
  MYSQL_POOL_CONN *connection;
  struct clan_status_report *report;
  enum clan_repository_result status;
  char uid[64]="",player_arg[64],account_arg[64],extra[2],text[16384];
  const char *selected=NULL;
  int64_t player,account;
  if (!ch || IS_NPC(ch)) return;
  if (inspect && GET_LEVEL(ch)<LVL_GRSTAFF) {
    send_to_char(ch,"Staff permission is required for progression inspection.\r\n");return;
  }
  if (inspect) {
    if (sscanf(arguments?arguments:"","%63s %63s %63s %1s",player_arg,account_arg,uid,extra)!=3 ||
        !positive_id(player_arg,&player) || !positive_id(account_arg,&account) || !clan_progression_uid_valid(uid)) {
      send_to_char(ch,"Usage: clan inspect <character-id> <account-id> <clan-incarnation-uid>\r\n");return;
    }
    selected=uid;
  } else {
    int tokens=sscanf(arguments?arguments:"","%63s %1s",uid,extra);
    if (tokens>1 || (tokens==1 && !clan_progression_uid_valid(uid))) {
      send_to_char(ch,"Usage: clan progress [clan-incarnation-uid]. See clan favor for your history.\r\n");return;
    }
    if (tokens==1) selected=uid;
    player=GET_IDNUM(ch);
    if (!ch->desc || !ch->desc->account || ch->desc->account->id<=0) {
      send_to_char(ch,"Your authenticated account identity is unavailable; no character fallback is used.\r\n");return;
    }
    account=ch->desc->account->id;
  }
  if (!clan_persistence_sql_authority() || !clan_persistence_ready()) {
    send_to_char(ch,"Clan progression readouts require available authoritative SQL storage.\r\n");return;
  }
  if (!mysql_available || !(connection=mysql_pool_acquire())) {
    send_to_char(ch,"Clan progression storage is unavailable.\r\n");return;
  }
  report=calloc(1,sizeof(*report));
  if (!report) {mysql_pool_release(connection);return;}
  status=clan_repository_status_read(connection->conn,player,account,selected,inspect,now,report);
  mysql_pool_release(connection);
  if (status==CLAN_REPO_OK && clan_status_render(&clan_progression_defaults,report,text,sizeof(text))) {
    if (inspect) send_to_char(ch,"Staff inspection: character #%" PRId64 "; explicit account #%" PRId64 ".\r\n",player,account);
    send_to_char(ch,"%s",text);
  } else if (status==CLAN_REPO_NOT_FOUND || status==CLAN_REPO_INELIGIBLE)
    send_to_char(ch,"No accessible clan record. Use clan favor for your retained Favor history.\r\n");
  else send_to_char(ch,"Clan progression could not be read safely (%s).\r\n",status==CLAN_REPO_OK?"invalid state":clan_repository_result_name(status));
  free(report);
}
ACMDU(do_clanprogress) {clan_status_command(ch,argument,false,(int64_t)time(NULL));}
ACMDU(do_claninspect) {clan_status_command(ch,argument,true,(int64_t)time(NULL));}
ACMDU(do_clanfavor)
{
  MYSQL_POOL_CONN *connection;
  struct json_object *root=NULL,*entries,*cursor=NULL;
  enum clan_repository_result status;
  char after[64]="",extra[2];size_t i;
  int tokens;
  if (!ch || IS_NPC(ch)) return;
  tokens=sscanf(argument?argument:"","%63s %1s",after,extra);
  if (tokens>1 || (tokens==1 && !clan_progression_uid_valid(after))) {
    send_to_char(ch,"Usage: clan favor [after-incarnation-uid]\r\n");return;
  }
  if (!clan_persistence_sql_authority() || !clan_persistence_ready() || !mysql_available ||
      !(connection=mysql_pool_acquire())) {
    send_to_char(ch,"Clan Favor history requires available authoritative SQL storage.\r\n");return;
  }
  status=clan_repository_favor_history(connection->conn,GET_IDNUM(ch),tokens==1?after:NULL,&root);
  mysql_pool_release(connection);
  if (status!=CLAN_REPO_OK) {send_to_char(ch,"Clan Favor history is unavailable (%s).\r\n",clan_repository_result_name(status));return;}
  json_object_object_get_ex(root,"entries",&entries);
  send_to_char(ch,"Your retained Favor history:\r\n");
  for (i=0;i<json_object_array_length(entries);i++) {
    struct json_object *e=json_object_array_get_idx(entries,i),*name=NULL,*uid=NULL,*favor=NULL,*deleted=NULL;
    enum clan_favor_standing standing;
    int64_t n;
    static const char *fraction[]={"",".125",".25",".375",".5",".625",".75",".875"};
    json_object_object_get_ex(e,"name",&name);json_object_object_get_ex(e,"uid",&uid);
    json_object_object_get_ex(e,"favor_eighths",&favor);json_object_object_get_ex(e,"deleted",&deleted);
    n=json_object_get_int64(favor);clan_award_standing(n,&standing);
    send_to_char(ch,"  %s%s: %" PRId64 "%s (%s); %s\r\n",json_object_get_string(name),json_object_get_boolean(deleted)?" [disbanded]":"",
        n/8,fraction[n%8],clan_progression_defaults.standings[standing].name,json_object_get_string(uid));
  }
  if (!json_object_array_length(entries)) send_to_char(ch,"No saved Favor records.\r\n");
  send_to_char(ch,"Use clan progress <incarnation-uid> to inspect your own retained standing.\r\n");
  if (json_object_object_get_ex(root,"next_uid",&cursor) && cursor)
    send_to_char(ch,"More: clan favor %s\r\n",json_object_get_string(cursor));
  json_object_put(root);
}
