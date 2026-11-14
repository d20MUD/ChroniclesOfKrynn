#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "mysql.h"
#include "clan_access.h"
#include "clan_persistence.h"
#include "clan_awards.h"
#include <inttypes.h>

bool clan_access_check(struct char_data *ch, const struct clan_requirement *r, bool explain)
{
  MYSQL_POOL_CONN *connection;
  MYSQL_RES *result;
  MYSQL_ROW row;
  enum clan_favor_standing favor = CLAN_FAVOR_UNPROVEN;
  char query[1024];
  bool allowed = false;
  if (!r || (!r->clan && !r->rank && !r->favor) || IS_NPC(ch))
    return true;
  if (r->clan < 0 || r->clan > 65534 || r->rank < 0 || r->rank > 15 || r->favor < 0 || r->favor > 5)
    goto denied;
  if (!clan_persistence_sql_authority() || !clan_persistence_ready() || !mysql_available ||
      !(connection = mysql_pool_acquire()))
    goto denied;
  /* A single snapshot checks the current incarnation, accepted officer rank,
   * and personal Favor. Never authorize from stale player-file mirrors. */
  snprintf(query, sizeof(query),
           "SELECT c.clan_vnum,m.rank_id,COALESCE(f.favor_eighths,0) FROM clan_memberships m "
           "JOIN clan_progression_clans c ON c.clan_uid=m.clan_uid AND c.record_status=1 "
           "JOIN clan_progression_legacy l ON l.clan_uid=c.clan_uid "
           "JOIN clan_store_control sc ON sc.singleton=1 AND sc.store_mode=2 "
           "LEFT JOIN clan_personal_favor f ON f.clan_uid=m.clan_uid AND f.player_id=m.player_id "
           "WHERE m.player_id=%" PRId64
           " AND m.rank_id BETWEEN 1 AND JSON_LENGTH(JSON_EXTRACT(l.details,'$.ranks'))",
           (int64_t)GET_IDNUM(ch));
  if (!mysql_query(connection->conn, query) && (result = mysql_store_result(connection->conn)))
  {
    row = mysql_fetch_row(result);
    if (row && row[0] && row[1] && row[2] && clan_award_standing(strtoll(row[2], NULL, 10), &favor))
      allowed = (!r->clan || atoi(row[0]) == r->clan) && (!r->rank || atoi(row[1]) <= r->rank) &&
                (!r->favor || (int)favor >= r->favor);
    mysql_free_result(result);
  }
  mysql_pool_release(connection);
denied:
  if (!allowed && explain)
    send_to_char(
        ch,
        "You do not meet this item's clan membership, officer rank, or Favor requirements.\r\n");
  return allowed;
}
