#ifndef KRYNN_CLAN_EXPEDITION_ENROLLMENT_H
#define KRYNN_CLAN_EXPEDITION_ENROLLMENT_H
#include "clan_expeditions.h"
#include "clan_repository.h"
#define CLAN_EXPEDITION_ROSTER_MAX 6
/* Trusted authenticated server character, never client-supplied identity/level. */
struct clan_expedition_member
{
  int64_t player, account;
  int level;
};
bool clan_expedition_schema_ready(MYSQL *);
enum clan_repository_result
clan_expedition_create(MYSQL *, const struct clan_expedition_catalog *,
                       const struct clan_expedition_world *, const char *definition,
                       const char *outing, const char *clan, const struct clan_expedition_member *,
                       int64_t now, const struct clan_progression_config *);
enum clan_repository_result clan_expedition_enroll(MYSQL *, const char *outing, const char *clan,
                                                   const struct clan_expedition_member *,
                                                   int64_t now,
                                                   const struct clan_progression_config *);
enum clan_repository_result clan_expedition_read(MYSQL *, const char *clan, struct json_object **);
/* Start requires fresh server snapshots of the entire enrolled roster. Seal is
 * reserved for the first counted objective, not a player command. Both recheck
 * every member and freeze no substitute identities. */
enum clan_repository_result clan_expedition_start(MYSQL *, const char *outing, const char *clan,
                                                  int64_t actor,
                                                  const struct clan_expedition_member *,
                                                  size_t count, int64_t now,
                                                  const struct clan_progression_config *);
enum clan_repository_result clan_expedition_seal(MYSQL *, const char *outing, const char *clan,
                                                 const struct clan_expedition_member *,
                                                 size_t count, int64_t now,
                                                 const struct clan_progression_config *);
enum clan_repository_result clan_expedition_leave(MYSQL *, const char *outing, const char *clan,
                                                  const struct clan_expedition_member *,
                                                  bool abandon, int64_t now,
                                                  const struct clan_progression_config *);
#include "clan_combat.h"
bool clan_expedition_progress_ready(MYSQL *);
/* Trusted combat collector facts. Area is resolved from the victim's real room.
 * The same pure combat policy qualifies all roles; clients cannot supply it. */
enum clan_repository_result clan_expedition_combat(MYSQL *, const struct clan_combat_encounter *,
                                                   int area, int64_t now,
                                                   const struct clan_progression_config *);
enum clan_repository_result clan_expedition_progress_read(MYSQL *, const char *outing,
                                                          struct json_object **);
enum clan_repository_result clan_expedition_claim_list(MYSQL *, const char *clan, int64_t player,
                                                       int64_t account, int64_t now,
                                                       struct json_object **);
enum clan_repository_result
clan_expedition_claim_prepare(MYSQL *, const char *outing, const char *clan,
                              const struct clan_expedition_member *, int64_t now,
                              const struct clan_progression_config *, struct json_object **);
enum clan_repository_result clan_expedition_reward_complete(MYSQL *, const char *operation,
                                                            int64_t now,
                                                            const struct clan_progression_config *,
                                                            struct json_object **);
#ifdef CLAN_REWARD_TEST
extern int clan_expedition_crash;
#endif
#endif
