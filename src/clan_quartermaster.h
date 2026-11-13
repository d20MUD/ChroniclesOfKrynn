#ifndef KRYNN_CLAN_QUARTERMASTER_H
#define KRYNN_CLAN_QUARTERMASTER_H
#include "clan_status.h"
#include <json-c/json.h>
#define CLAN_QM_MAX 16
struct clan_qm_item
{
  char id[33], name[81], coverage[161], budget[241];
  int prototype, kind, clan_level, favor, level, price, value, type, weight;
  int values[16];
};
struct clan_qm_catalog
{
  size_t count;
  struct clan_qm_item entries[CLAN_QM_MAX];
};
bool clan_qm_parse(struct json_object *, struct clan_qm_catalog *);
/* Current standing gate; frozen catalog terms retain their base threshold. */
int clan_qm_favor_requirement(const struct clan_qm_item *);
struct json_object *clan_qm_terms(const struct clan_qm_item *);
bool clan_qm_item_parse(struct json_object *, struct clan_qm_item *);
bool clan_qm_schema_ready(MYSQL *);
/* Trusted adapter supplies current level and a server-created bound payload.
 * Idempotency compares every original fact; recovery never rechecks acquisition gates. */
enum clan_repository_result clan_qm_prepare(MYSQL *, const struct clan_qm_item *, const char *uid,
                                            const char *clan, int64_t player, int64_t account,
                                            int level, int gold, const char *payload, int64_t now,
                                            const struct clan_progression_config *);
enum clan_repository_result clan_qm_operation(MYSQL *, int64_t player, const char *uid,
                                              struct json_object **);
/* Only acknowledge after durable currency AND item checkpoints. */
enum clan_repository_result clan_qm_ack(MYSQL *, int64_t player, const char *uid);
struct char_data;
bool clan_qm_recover(struct char_data *);
void clan_qm_boot_validate(void);
bool clan_qm_catalog_load(struct clan_qm_catalog *);
#endif
