#ifndef KRYNN_CLAN_EXPEDITIONS_H
#define KRYNN_CLAN_EXPEDITIONS_H
#include <stdbool.h>
#include <stddef.h>
#include <json-c/json.h>
#define CLAN_EXPEDITION_MAX 16
#define CLAN_EXPEDITION_TARGET_MAX 8
#define CLAN_EXPEDITION_STRONGHOLD 1
#define CLAN_EXPEDITION_HUNT 2
struct clan_expedition_target
{
  int area, mob;
};
struct clan_expedition_objective
{
  int quota;
  size_t count;
  struct clan_expedition_target targets[CLAN_EXPEDITION_TARGET_MAX];
};
struct clan_expedition_definition
{
  char id[33], name[81], shared_reward_id[33], personal_reward_id[33];
  int family, min_clan_level, min_player_level, max_player_level;
  struct clan_expedition_objective objectives[3];
};
struct clan_expedition_catalog
{
  size_t count;
  struct clan_expedition_definition entries[CLAN_EXPEDITION_MAX];
};
/* area exists; target prototype has a valid reset in this area; alternative is
 * currently available. All callbacks are server-owned and required. */
struct clan_expedition_world
{
  bool (*area)(int, void *);
  bool (*target)(int, int, void *);
  bool (*available)(int, int, void *);
  void *context;
};
/* Failure never publishes a partial catalog; diagnostics identify field/ID. */
bool clan_expedition_catalog_parse(struct json_object *, struct clan_expedition_catalog *,
                                   char *error, size_t capacity);
bool clan_expedition_catalog_validate(const struct clan_expedition_catalog *,
                                      const struct clan_expedition_world *, char *, size_t);
struct json_object *clan_expedition_definition_json(const struct clan_expedition_definition *);
/* Called after world renumbering. Diagnostic only: never enables expeditions. */
void clan_expedition_boot_validate(void);
struct clan_expedition_world clan_expedition_world_refs(void);
const char *clan_expedition_target_name(int mob);
const char *clan_expedition_area_name(int area);
#endif
