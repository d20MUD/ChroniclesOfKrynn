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
#include "clan_display.h"
#include "clan_buff_runtime.h"
static bool positive_id(const char *s,int64_t *out)
{
  char *end;intmax_t id;
  if (!s || !*s || *s<'0' || *s>'9') return false;
  errno=0;id=strtoimax(s,&end,10);
  if (errno || *end || id<=0 || id>INT64_MAX) return false;
  *out=(int64_t)id;return true;
}
/* Search only this character's retained history; never expose another clan's
 * balances. Resolve across every page and reject duplicate historical names. */
static enum clan_repository_result history_name(MYSQL *db, int64_t player,
                                                const char *name, char uid[64])
{
  char after[33] = "", selected[33] = "";
  int best = 0; bool ambiguous = false;
  size_t page;
  for (page = 0; page < 100; page++) {
    struct json_object *root = NULL, *entries = NULL, *next = NULL;
    enum clan_repository_result r = clan_repository_favor_history(db, player, *after ? after : NULL, &root);
    size_t i;
    if (r != CLAN_REPO_OK) return r;
    json_object_object_get_ex(root, "entries", &entries);
    for (i = 0; i < json_object_array_length(entries); i++) {
      struct json_object *e = json_object_array_get_idx(entries, i), *n = NULL, *id = NULL;
      json_object_object_get_ex(e, "name", &n); json_object_object_get_ex(e, "uid", &id);
      int score = clan_display_match(name, json_object_get_string(n), json_object_get_string(id));
      if (score > best) { best = score; ambiguous = false; snprintf(selected, sizeof(selected), "%s", json_object_get_string(id)); }
      else if (score && score == best) ambiguous = true;
    }
    if (json_object_object_get_ex(root, "next_uid", &next) && next)
      snprintf(after, sizeof(after), "%s", json_object_get_string(next));
    else *after = 0;
    json_object_put(root);
    if (!*after) break;
  }
  if (*after) return CLAN_REPO_CAPACITY;
  if (ambiguous) return CLAN_REPO_CONFLICT;
  if (!best) return CLAN_REPO_NOT_FOUND;
  snprintf(uid, 64, "%s", selected);
  return CLAN_REPO_OK;
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
    if (arguments && *arguments) {
      if (strlen(arguments)>=sizeof(uid)) {
        send_to_char(ch,"Usage: clan progress [clan name].\r\n"); return;
      }
      snprintf(uid,sizeof(uid),"%s",arguments); selected=uid;
    }
    player=GET_IDNUM(ch);
    if (!ch->desc || !ch->desc->account || ch->desc->account->id<=0) {
      send_to_char(ch,"Your account identity is unavailable. Please reconnect and try again.\r\n");return;
    }
    account=ch->desc->account->id;
  }
  if (!clan_persistence_sql_authority() || !clan_persistence_ready()) {
    send_to_char(ch,"Clan progress is unavailable right now. Please try again shortly.\r\n");return;
  }
  if (!mysql_available || !(connection=mysql_pool_acquire())) {
    send_to_char(ch,"Clan progression storage is unavailable.\r\n");return;
  }
  report=calloc(1,sizeof(*report));
  if (!report) {mysql_pool_release(connection);return;}
  if (!inspect && selected && !clan_progression_uid_valid(uid)) {
    enum clan_repository_result current_status=clan_repository_status_read(connection->conn,player,account,NULL,false,now,report);
    int current_match=current_status==CLAN_REPO_OK ? clan_display_match(uid,report->clan.name,"") : 0;
    char current_uid[33]="";
    if (current_match) snprintf(current_uid,sizeof(current_uid),"%s",report->clan.uid);
    status=history_name(connection->conn,player,uid,uid);
    if (status==CLAN_REPO_NOT_FOUND && current_match) {snprintf(uid,sizeof(uid),"%s",current_uid);status=CLAN_REPO_OK;}
    if (status!=CLAN_REPO_OK) {
      mysql_pool_release(connection); free(report);
      send_to_char(ch,status==CLAN_REPO_CONFLICT ? "More than one saved clan matches that name. Please use a more specific name.\r\n"
                                                : "No saved standing for that clan. Use clan favor to see your history.\r\n");
      return;
    }
  }
  status=clan_repository_status_read(connection->conn,player,account,selected,inspect,now,report);
  mysql_pool_release(connection);
  if (status==CLAN_REPO_OK && clan_status_render(clan_buff_runtime_policy(),report,text,sizeof(text))) {
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
  int tokens, page=1, current;
  if (!ch || IS_NPC(ch)) return;
  tokens=sscanf(argument?argument:"","%63s %1s",after,extra);
  if (tokens>1 || (tokens==1 && !clan_progression_uid_valid(after) &&
      (!positive_id(after,(int64_t[1]){0}) || strlen(after)>3))) {
    send_to_char(ch,"Usage: clan favor [page]\r\n");return;
  }
  if (!clan_persistence_sql_authority() || !clan_persistence_ready() || !mysql_available ||
      !(connection=mysql_pool_acquire())) {
    send_to_char(ch,"Your Favor history is unavailable right now. Please try again shortly.\r\n");return;
  }
  if (tokens==1 && !clan_progression_uid_valid(after)) { page=atoi(after); *after=0; }
  for (current=1; current<=page; current++) {
    status=clan_repository_favor_history(connection->conn,GET_IDNUM(ch),*after?after:NULL,&root);
    if (status!=CLAN_REPO_OK || current==page) break;
    if (!json_object_object_get_ex(root,"next_uid",&cursor) || !cursor) break;
    snprintf(after,sizeof(after),"%s",json_object_get_string(cursor));
    json_object_put(root); root=NULL;
  }
  mysql_pool_release(connection);
  if (status!=CLAN_REPO_OK) {send_to_char(ch,"Clan Favor history is unavailable (%s).\r\n",clan_repository_result_name(status));return;}
  json_object_object_get_ex(root,"entries",&entries);
  send_to_char(ch,"\r\n\tCYour Clan Favor\tn\r\n  Favor is permanent and never spent.\r\n\r\n");
  for (i=0;i<json_object_array_length(entries);i++) {
    struct json_object *e=json_object_array_get_idx(entries,i),*name=NULL,*uid=NULL,*favor=NULL,*deleted=NULL;
    enum clan_favor_standing standing;
    int64_t n;
    static const char *fraction[]={"",".125",".25",".375",".5",".625",".75",".875"};
    json_object_object_get_ex(e,"name",&name);json_object_object_get_ex(e,"uid",&uid);
    json_object_object_get_ex(e,"favor_eighths",&favor);json_object_object_get_ex(e,"deleted",&deleted);
    n=json_object_get_int64(favor);clan_award_standing(n,&standing);
    send_to_char(ch,"  \tW%s\tn%s\r\n    %" PRId64 "%s Favor  |  %s\r\n",json_object_get_string(name),json_object_get_boolean(deleted)?" [disbanded]":"",
        n/8,fraction[n%8],clan_progression_defaults.standings[standing].name);
  }
  if (!json_object_array_length(entries)) send_to_char(ch,"No saved Favor records.\r\n");
  send_to_char(ch,"Use clan progress <clan name> to view your standing.\r\n");
  if (json_object_object_get_ex(root,"next_uid",&cursor) && cursor)
    send_to_char(ch,"Next page: clan favor %d\r\n",page+1);
  json_object_put(root);
}
