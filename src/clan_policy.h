#ifndef KRYNN_CLAN_POLICY_H
#define KRYNN_CLAN_POLICY_H
#include "clan_progression.h"
/* Server-owned startup configuration, never client input. Missing files leave
 * gameplay OFF. Invalid/oversized files fail without changing the output. */
#define CLAN_POLICY_MAX_BYTES 8192
bool clan_policy_parse(const char *, size_t, bool sql_ready,
    struct clan_progression_config *, char *reason, size_t reason_size);
bool clan_policy_load(const char *path, bool sql_ready,
    struct clan_progression_config *, bool *present, char *reason, size_t reason_size);
#endif
