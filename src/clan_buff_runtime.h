#ifndef KRYNN_CLAN_BUFF_RUNTIME_H
#define KRYNN_CLAN_BUFF_RUNTIME_H
#include "clan_progression.h"
/* Trusted server configuration only. Defaults OFF. Invalid configuration clears
 * all effects and fails closed. No player command or implicit enablement. */
bool clan_buff_runtime_configure(const struct clan_progression_config *);
const struct clan_progression_config *clan_buff_runtime_policy(void);
struct char_data;
/* Defer only HP clamping while restoring equipment; finish before world scripts.
 * Both normal login and copyover use enter_player_game's common restore path. */
void clan_buff_runtime_login_begin(struct char_data *);
void clan_buff_runtime_login_end(struct char_data *, int64_t now);
/* Heartbeat/login/copyover: reload current SQL membership and saved timers,
 * expire due activations, and invalidate changed member maxima. */
void clan_buff_runtime_tick(int64_t now);
#endif
