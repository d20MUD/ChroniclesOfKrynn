#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "protocol.h"
#include "mysql.h"
#include "clan_web.h"
#include "clan_web_content.h"
#include "clan_buff_runtime.h"
#include "clan_persistence.h"
void clan_web_update(struct char_data *ch, bool force)
{
  struct descriptor_data *d;
  struct json_object *o = NULL;
  MYSQL_POOL_CONN *connection;
  int64_t now = time(NULL), account;
  const struct clan_progression_config *policy = clan_buff_runtime_policy();
  if (!ch || IS_NPC(ch) || !(d = ch->desc) || !d->pProtocol || !d->pProtocol->pVariables ||
      (!d->pProtocol->bMSDP && !d->pProtocol->bGMCP))
    return;
  if (!force && !d->pProtocol->pVariables[eMSDP_CLAN_PROGRESS]->bReport)
    return;
  account = d->account ? d->account->id : 0;
  if (!force && d->clan_web_player == GET_IDNUM(ch) && d->clan_web_account == account &&
      now >= d->clan_web_updated_at && now - d->clan_web_updated_at < 2)
    return;
  d->clan_web_player = GET_IDNUM(ch);
  d->clan_web_account = account;
  d->clan_web_updated_at = now;
  if (STATE(d) != CON_PLAYING || account <= 0)
    o = clan_web_status("unavailable", "An authenticated character is required.", now);
  else if (!policy->controls.web)
    o = clan_web_status("disabled", "Clan web reports are disabled.", now);
  else if (!clan_persistence_sql_authority() || !clan_persistence_ready() || !mysql_available)
    o = clan_web_status("unavailable", "Authoritative clan storage is unavailable.", now);
  else
  {
    connection = mysql_pool_acquire();
    if (connection)
    {
      clan_web_read(connection->conn, policy, GET_IDNUM(ch), account, now, &o);
      mysql_pool_release(connection);
    }
    if (!o)
      o = clan_web_status("unavailable", "Clan state could not be read safely. Try again shortly.",
                          now);
  }
  clan_web_content_enrich(ch, o, now);
  MSDPSetString(d, eMSDP_CLAN_PROGRESS, json_object_to_json_string_ext(o, JSON_C_TO_STRING_PLAIN));
  json_object_put(o);
}

/* Protocol SEND/REPORT must also clear stale private values at account menus
 * and while switched; there may be no authenticated character to refresh. */
void clan_web_descriptor_update(struct descriptor_data *d, bool force)
{
  struct json_object *o;
  if (!d || !d->pProtocol || !d->pProtocol->pVariables)
    return;
  if (!d->character || IS_NPC(d->character) || d->character->desc != d || STATE(d) != CON_PLAYING)
  {
    o = clan_web_status("unavailable", "An authenticated character is required.",
                        (int64_t)time(NULL));
    MSDPSetString(d, eMSDP_CLAN_PROGRESS,
                  json_object_to_json_string_ext(o, JSON_C_TO_STRING_PLAIN));
    json_object_put(o);
    d->clan_web_player = 0;
    d->clan_web_account = 0;
    d->clan_web_updated_at = 0;
    return;
  }
  clan_web_update(d->character, force);
}
