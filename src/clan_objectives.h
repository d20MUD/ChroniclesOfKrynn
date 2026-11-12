#ifndef KRYNN_CLAN_OBJECTIVES_H
#define KRYNN_CLAN_OBJECTIVES_H
#include "clan_awards.h"
#include <json-c/json.h>
#define CLAN_OBJECTIVE_MAX 32
struct clan_objective_definition
{
  char id[33], name[81];
  int family, target, grade, required, min_clan_level, min_player_level, max_player_level;
};
struct clan_objective_catalog
{
  size_t count;
  struct clan_objective_definition entries[CLAN_OBJECTIVE_MAX];
};
/* Trusted occurrence, captured from the real harvest/combat hook. Never derive
 * these facts from inventory totals, commands, client data or character XP. */
struct clan_objective_event
{
  char event_uid[33], contract_uid[33], clan_uid[33];
  int64_t player_id, account_id, occurred_at, room;
  int family, target, grade, base_units;
};
bool clan_objective_definition_parse(struct json_object *, struct clan_objective_definition *);
struct json_object *clan_objective_definition_json(const struct clan_objective_definition *);
bool clan_objective_catalog_parse(struct json_object *, struct clan_objective_catalog *);
void clan_objective_catalog_rotate(struct clan_objective_catalog *, int64_t now, int player_level);
bool clan_objective_schema_ready(MYSQL *);
enum clan_repository_result clan_objective_expire(MYSQL *, int64_t player, int64_t now);
enum clan_repository_result clan_objective_accept(MYSQL *, const struct clan_objective_definition *,
                                                  const char *uid, const char *clan, int64_t player,
                                                  int64_t account, int level, int64_t now);
enum clan_repository_result clan_objective_active(MYSQL *, int64_t player, int family,
                                                  struct json_object **);
enum clan_repository_result clan_objective_abandon(MYSQL *, int64_t player, int family);
enum clan_repository_result clan_objective_claims(MYSQL *, int64_t account, int family, int64_t now,
                                                  struct json_object **);
/* Freezes the accepted contract identity at occurrence, with current accepted
 * membership and exact target checks. A later handoff cannot select a new one. */
enum clan_repository_result clan_objective_capture(MYSQL *, struct clan_objective_event *,
                                                   int player_level);
/* Event dedup, bounded progress and final pending reward are one transaction.
 * Saved server events retain their occurrence identity during offline retries. */
enum clan_repository_result clan_objective_progress(MYSQL *, const struct clan_objective_event *,
                                                    int64_t now);
enum clan_repository_result clan_objective_reward_complete(MYSQL *, const char *uid, int64_t now,
                                                           struct json_object **);
#ifdef CLAN_REWARD_TEST
extern int clan_objective_crash;
#endif
#endif
