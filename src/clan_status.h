#ifndef KRYNN_CLAN_STATUS_H
#define KRYNN_CLAN_STATUS_H
#include "clan_rewards.h"
#include "clan_buff_queries.h"
struct clan_status_report {
  struct clan_repository_record clan;
  struct clan_account_window account;
  struct clan_favor_state favor;
  struct clan_weekly_window weekly;
  enum clan_member_access membership;
  bool full_clan_access, favor_present, account_present, weekly_present, receipt_present;
  int64_t queried_at;
  struct clan_buff_state buffs[CLAN_PROGRESSION_BUFF_COUNT];
  size_t buff_count;
  struct clan_reward_delta recent;
};
/* Trusted server read API. inspect is an authorization decision made by the
 * command adapter, never a player-supplied flag. No nested transaction allowed.
 * A NULL UID selects SQL membership; explicit historical UID requires own Favor
 * unless inspect is authorized. Outputs unchanged on error, no rows created. */
enum clan_repository_result clan_repository_status_read(MYSQL *, int64_t player_id,
    int64_t account_id, const char *clan_uid, bool inspect, int64_t now,
    struct clan_status_report *out);
enum clan_repository_result clan_repository_favor_history(MYSQL *, int64_t player_id,
    const char *after_uid, struct json_object **out);
/* Pure, bounded text with exact eighth-points and UTC periods. */
bool clan_status_render(const struct clan_progression_config *,
    const struct clan_status_report *, char *out, size_t capacity);
#endif
