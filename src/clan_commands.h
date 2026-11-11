#ifndef KRYNN_CLAN_COMMANDS_H
#define KRYNN_CLAN_COMMANDS_H
#include <stdbool.h>
#include <stdint.h>
struct char_data;
/* Shared production adapter, injected time for deterministic verification. */
void clan_status_command(struct char_data *,const char *arguments,bool inspect,int64_t now);
#endif
