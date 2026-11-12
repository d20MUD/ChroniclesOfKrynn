#ifndef KRYNN_CLAN_DELIVERY_H
#define KRYNN_CLAN_DELIVERY_H
#include "clan_awards.h"
#include <json-c/json.h>
#include "clan_status.h"
#define CLAN_DELIVERY_LIMIT 32
#define CLAN_DELIVERY_MATERIAL 1
#define CLAN_DELIVERY_AMMO 2
struct clan_delivery_definition
{
  char id[33], name[81];
  int kind, resource, grade, quantity, min_level, min_enhancement;
};
struct clan_delivery_catalog
{
  size_t count;
  struct clan_delivery_definition entries[CLAN_DELIVERY_LIMIT];
};
/* Strict bounded definitions. Material grade is verified by the game adapter. */
bool clan_delivery_definition_valid(const struct clan_delivery_definition *);
bool clan_delivery_catalog_parse(struct json_object *, struct clan_delivery_catalog *);
struct json_object *clan_delivery_definition_json(const struct clan_delivery_definition *);
bool clan_delivery_definition_parse(struct json_object *, struct clan_delivery_definition *);
void clan_delivery_catalog_rotate(struct clan_delivery_catalog *, int64_t now);
bool clan_delivery_schema_ready(MYSQL *);
enum clan_repository_result clan_delivery_claim_history(MYSQL *, int64_t account, int64_t now,
                                                        struct json_object **);
/* Accepted terms are immutable, weekly expiry is explicit. No goods touched. */
enum clan_repository_result clan_delivery_expire(MYSQL *, int64_t player, int64_t now);
enum clan_repository_result clan_delivery_accept(MYSQL *, const struct clan_delivery_definition *,
                                                 const char *accepted_uid, const char *clan_uid,
                                                 int64_t player, int64_t account, int64_t now);
enum clan_repository_result clan_delivery_active(MYSQL *, int64_t player, int64_t now,
                                                 struct json_object **);
enum clan_repository_result clan_delivery_abandon(MYSQL *, int64_t player, int64_t now);
/* quote JSON freezes event, accepted definition, exact payout and 60s confirmation
 * deadline. prepare rechecks membership, definitions, caps and expiry under lock. */
enum clan_repository_result clan_delivery_quote(MYSQL *, int64_t player, int64_t account,
                                                int64_t now, struct json_object **);
enum clan_repository_result clan_delivery_prepare(MYSQL *, struct json_object *quote, int64_t now,
                                                  struct json_object **);
enum clan_repository_result clan_delivery_operation(MYSQL *, int64_t player, const char *uid,
                                                    struct json_object **);
/* Only call complete after the paid goods checkpoint is durable. Receipt and
 * unique definition claim commit atomically; terminal receipts replay verbatim. */
enum clan_repository_result clan_delivery_complete(MYSQL *, struct json_object *operation,
                                                   int64_t now, struct json_object **);
/* Caller first checkpoints a refund (or verifies unpaid). No resource mutation. */
enum clan_repository_result clan_delivery_cancel(MYSQL *, const char *uid, int64_t now);
enum clan_repository_result clan_delivery_ack(MYSQL *, const char *uid);
#endif
