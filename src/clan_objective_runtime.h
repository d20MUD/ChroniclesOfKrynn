#ifndef KRYNN_CLAN_OBJECTIVE_RUNTIME_H
#define KRYNN_CLAN_OBJECTIVE_RUNTIME_H
#include "clan_objectives.h"
struct char_data;
bool clan_objective_flush(struct char_data *);
bool clan_objective_harvest_ready(struct char_data *);
bool clan_objective_harvest_begin(struct char_data *, int material, struct clan_objective_event *);
void clan_objective_harvest_finish(struct char_data *, struct clan_objective_event *,
                                   int base_units);
bool clan_objective_bounty_capture(MYSQL *, struct char_data *, int prototype,
                                   const char *encounter, const char *clan, int64_t now,
                                   struct clan_objective_event *);
void clan_objective_bounty_record(struct char_data *, const struct clan_objective_event *);
#endif
