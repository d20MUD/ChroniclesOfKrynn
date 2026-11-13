#ifndef KRYNN_CLAN_WEB_H
#define KRYNN_CLAN_WEB_H
#include "clan_buff_activation.h"
#include <json-c/json.h>
#define CLAN_WEB_MAX_BYTES 49152
struct clan_web_snapshot
{
  struct clan_buff_activation_snapshot buffs;
  struct clan_favor_state favor;
  struct clan_account_window account;
  bool buff_storage_ready;
};
/* Bounded private view: supplied IDs are authenticated server facts, never client
 * selectors. Reads one member/clan/account and at most five family rows. */
enum clan_repository_result clan_web_read(MYSQL *, const struct clan_progression_config *,
                                          int64_t player, int64_t account, int64_t now,
                                          struct json_object **);
/* Pure serialization and existing activation validation; no receipts minted. */
struct json_object *clan_web_render(const struct clan_progression_config *,
                                    const struct clan_web_snapshot *, int64_t now);
struct json_object *clan_web_status(const char *status, const char *reason, int64_t now);
struct char_data;
void clan_web_update(struct char_data *, bool force);
struct descriptor_data;
void clan_web_descriptor_update(struct descriptor_data *, bool force);
#endif
