#include "clan_contract_rotation.h"
#include "clan_delivery.h"
#include "crafting_recipes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <inttypes.h>

static struct json_object *get(struct json_object *o, const char *key)
{
  struct json_object *v = NULL;
  if (o)
    json_object_object_get_ex(o, key, &v);
  return v;
}
static int64_t num(struct json_object *o, const char *key)
{
  return json_object_get_int64(get(o, key));
}
static const char *str(struct json_object *o, const char *key)
{
  const char *s = json_object_get_string(get(o, key));
  return s ? s : "";
}
static bool slug(const char *s)
{
  size_t n;
  if (!s || (n = strlen(s)) < 1 || n > 32)
    return false;
  for (; *s; s++)
    if (!(*s >= 'a' && *s <= 'z') && !(*s >= '0' && *s <= '9') && *s != '-')
      return false;
  return true;
}
bool clan_delivery_definition_valid(const struct clan_delivery_definition *d)
{
  size_t i, n;
  if (!d || !memchr(d->id, 0, sizeof(d->id)) || !memchr(d->name, 0, sizeof(d->name)) ||
      !slug(d->id) || (n = strlen(d->name)) < 1 || d->min_level < 1 || d->min_level > 20)
    return false;
  for (i = 0; i < n; i++)
    if ((unsigned char)d->name[i] < 32 || (unsigned char)d->name[i] > 126)
      return false;
  if (d->kind == CLAN_DELIVERY_MATERIAL)
    return d->resource > 0 && d->resource < 256 && d->grade >= 1 && d->grade <= 10 &&
           d->quantity == 50 && d->min_enhancement == 0;
  if (d->kind == CLAN_DELIVERY_CRAFTED)
    return d->resource > CRAFT_RECIPE_NONE && d->resource < NUM_CRAFTING_RECIPES &&
           d->grade == 0 && d->quantity >= 1 && d->quantity <= 20 && d->min_enhancement == 0;
  return d->kind == CLAN_DELIVERY_AMMO && d->resource >= 1 && d->resource <= 3 && d->grade == 0 &&
         d->quantity == 100 && d->min_enhancement >= 0 && d->min_enhancement <= 8;
}
struct json_object *clan_delivery_definition_json(const struct clan_delivery_definition *d)
{
  struct json_object *o;
  if (!clan_delivery_definition_valid(d))
    return NULL;
  o = json_object_new_object();
  json_object_object_add(o, "id", json_object_new_string(d->id));
  json_object_object_add(o, "name", json_object_new_string(d->name));
#define N(k) json_object_object_add(o, #k, json_object_new_int(d->k))
  N(kind);
  N(resource);
  N(grade);
  N(quantity);
  N(min_level);
  N(min_enhancement);
#undef N
  return o;
}
bool clan_delivery_definition_parse(struct json_object *o, struct clan_delivery_definition *out)
{
  struct clan_delivery_definition d = {0};
  const char *keys[] = {"kind", "resource", "grade", "quantity", "min_level", "min_enhancement"};
  size_t i;
  if (!out || !o || json_object_get_type(o) != json_type_object ||
      json_object_object_length(o) != 8 || json_object_get_type(get(o, "id")) != json_type_string ||
      json_object_get_type(get(o, "name")) != json_type_string || strlen(str(o, "id")) > 32 ||
      strlen(str(o, "name")) > 80)
    return false;
  for (i = 0; i < 6; i++)
    if (json_object_get_type(get(o, keys[i])) != json_type_int || num(o, keys[i]) < 0 ||
        num(o, keys[i]) > 1000)
      return false;
  snprintf(d.id, sizeof(d.id), "%s", str(o, "id"));
  snprintf(d.name, sizeof(d.name), "%s", str(o, "name"));
  d.kind = num(o, "kind");
  d.resource = num(o, "resource");
  d.grade = num(o, "grade");
  d.quantity = num(o, "quantity");
  d.min_level = num(o, "min_level");
  d.min_enhancement = num(o, "min_enhancement");
  if (!clan_delivery_definition_valid(&d))
    return false;
  *out = d;
  return true;
}
bool clan_delivery_catalog_parse(struct json_object *o, struct clan_delivery_catalog *out)
{
  struct clan_delivery_catalog c = {0};
  struct json_object *a = get(o, "deliveries");
  size_t i, j;
  if (!out || json_object_get_type(o) != json_type_object || json_object_object_length(o) != 2 ||
      json_object_get_type(get(o, "version")) != json_type_int || num(o, "version") != 1 ||
      json_object_get_type(a) != json_type_array || !(c.count = json_object_array_length(a)) ||
      c.count > CLAN_DELIVERY_LIMIT)
    return false;
  for (i = 0; i < c.count; i++)
  {
    if (!clan_delivery_definition_parse(json_object_array_get_idx(a, i), &c.entries[i]))
      return false;
    for (j = 0; j < i; j++)
      if (!strcmp(c.entries[i].id, c.entries[j].id))
        return false;
  }
  *out = c;
  return true;
}
static bool sql(MYSQL *db, const char *fmt, ...)
{
  char buf[16384];
  va_list ap;
  int n;
  va_start(ap, fmt);
  n = vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  return db && n >= 0 && (size_t)n < sizeof(buf) && mysql_query(db, buf) == 0;
}
static bool scalar(MYSQL *db, int64_t *out)
{
  MYSQL_RES *r = mysql_store_result(db);
  MYSQL_ROW row;
  char *end;
  bool ok = false;
  if (!r)
    return false;
  row = mysql_fetch_row(r);
  if (row && row[0])
  {
    *out = strtoll(row[0], &end, 10);
    ok = !*end;
  }
  mysql_free_result(r);
  return ok;
}
static bool encode(struct json_object *o, char *out, size_t cap)
{
  const unsigned char *p =
      (const unsigned char *)json_object_to_json_string_ext(o, JSON_C_TO_STRING_PLAIN);
  size_t i, n = strlen((const char *)p);
  static const char h[] = "0123456789abcdef";
  if (n * 2 + 1 > cap)
    return false;
  for (i = 0; i < n; i++)
  {
    out[2 * i] = h[p[i] >> 4];
    out[2 * i + 1] = h[p[i] & 15];
  }
  out[n * 2] = 0;
  return true;
}
static bool clan_delivery_schema_ready_full(MYSQL *db)
{
  int64_t n;
  return sql(db, "SELECT COUNT(*) FROM information_schema.TABLES WHERE TABLE_SCHEMA=DATABASE() "
                 "AND TABLE_NAME IN "
                 "('clan_delivery_schema','clan_delivery_contracts','clan_delivery_operations','"
                 "clan_delivery_claims') AND ENGINE='InnoDB'") &&
         scalar(db, &n) && n == 4 &&
         sql(db, "SELECT COUNT(*) FROM clan_delivery_schema WHERE version=1") && scalar(db, &n) &&
         n == 1 &&
         sql(db, "SELECT COUNT(*) FROM information_schema.TRIGGERS WHERE TRIGGER_SCHEMA=DATABASE() "
                 "AND TRIGGER_NAME IN "
                 "('cp_delivery_terms','cp_delivery_operation_facts','cp_delivery_contract_"
                 "retained','cp_delivery_operation_retained','cp_delivery_claim_retained','cp_"
                 "delivery_claim_immutable')") &&
         scalar(db, &n) && n == 6 &&
         sql(db,
             "SELECT COUNT(*) FROM (SELECT "
             "TABLE_NAME,INDEX_NAME,NON_UNIQUE,GROUP_CONCAT(COLUMN_NAME ORDER BY SEQ_IN_INDEX) AS "
             "cols FROM information_schema.STATISTICS WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME "
             "IN ('clan_delivery_contracts','clan_delivery_operations','clan_delivery_claims') "
             "GROUP BY TABLE_NAME,INDEX_NAME,NON_UNIQUE) AS keys_checked WHERE NON_UNIQUE=0 AND "
             "((TABLE_NAME='clan_delivery_contracts' AND ((INDEX_NAME='PRIMARY' AND "
             "cols='accepted_uid') OR (INDEX_NAME='cp_delivery_slot' AND cols='active_player'))) "
             "OR (TABLE_NAME='clan_delivery_operations' AND ((INDEX_NAME='PRIMARY' AND "
             "cols='operation_uid') OR (INDEX_NAME='cp_delivery_hold' AND cols='held_player') OR "
             "(INDEX_NAME='cp_delivery_event' AND cols='accepted_uid'))) OR "
             "(TABLE_NAME='clan_delivery_claims' AND ((INDEX_NAME='PRIMARY' AND "
             "cols='account_id,week_start,definition_id') OR (INDEX_NAME='cp_delivery_claim' AND "
             "cols='operation_uid'))))") &&
         scalar(db, &n) && n == 7;
}
static enum clan_repository_result clan_delivery_schema_ready_validate(MYSQL *db)
{
  return clan_delivery_schema_ready_full(db) ? CLAN_REPO_OK : CLAN_REPO_SCHEMA;
}
bool clan_delivery_schema_ready(MYSQL *db)
{
  return clan_repository_schema_extension_check(db, clan_delivery_schema_ready_validate) == CLAN_REPO_OK;
}
enum clan_repository_result clan_delivery_claim_history(MYSQL *db, int64_t account, int64_t now,
                                                        struct json_object **out)
{
  struct clan_reset_period week;
  struct json_object *a;
  MYSQL_RES *result;
  MYSQL_ROW row;
  if (account < 1 || !out || !clan_progression_week_at(now, &week))
    return CLAN_REPO_INVALID;
  if (!clan_delivery_schema_ready(db))
    return CLAN_REPO_SCHEMA;
  if (!sql(db,
           "SELECT definition_id FROM clan_delivery_claims WHERE account_id=%" PRId64
           " AND week_start=%" PRId64 " ORDER BY definition_id LIMIT 3",
           account, week.starts_at))
    return CLAN_REPO_UNAVAILABLE;
  result = mysql_store_result(db);
  if (!result)
    return CLAN_REPO_UNAVAILABLE;
  a = json_object_new_array();
  while ((row = mysql_fetch_row(result)))
    json_object_array_add(a, json_object_new_string(row[0]));
  mysql_free_result(result);
  if (json_object_array_length(a) > 2)
  {
    json_object_put(a);
    return CLAN_REPO_SCHEMA;
  }
  *out = a;
  return CLAN_REPO_OK;
}
static enum clan_repository_result begin(MYSQL *db)
{
  int64_t n;
  if (!clan_delivery_schema_ready(db) || clan_repository_schema_check(db) != CLAN_REPO_OK)
    return CLAN_REPO_SCHEMA;
  if (db->server_status & SERVER_STATUS_IN_TRANS)
    return CLAN_REPO_CONFLICT;
  if (!sql(db, "START TRANSACTION"))
    return CLAN_REPO_UNAVAILABLE;
  if (!sql(db, "SELECT store_mode FROM clan_store_control WHERE singleton=1 FOR UPDATE") ||
      !scalar(db, &n) || n != 2)
  {
    sql(db, "ROLLBACK");
    return CLAN_REPO_UNAVAILABLE;
  }
  return CLAN_REPO_OK;
}
static enum clan_repository_result end(MYSQL *db, enum clan_repository_result r)
{
  if (r != CLAN_REPO_OK)
  {
    sql(db, "ROLLBACK");
    return r;
  }
  return sql(db, "COMMIT") ? CLAN_REPO_OK : CLAN_REPO_COMMIT_UNKNOWN;
}
static bool member(MYSQL *db, int64_t player, const char *uid)
{
  int64_t n;
  return sql(db,
             "SELECT COUNT(*) FROM clan_memberships m JOIN clan_progression_legacy l ON "
             "l.clan_uid=m.clan_uid "
             "WHERE m.player_id=%" PRId64 " AND m.clan_uid='%s' AND m.rank_id BETWEEN 1 AND "
             "JSON_LENGTH(JSON_EXTRACT(l.details,'$.ranks'))",
             player, uid) &&
         scalar(db, &n) && n == 1;
}
static bool claims(MYSQL *db, int64_t account, int64_t week, const char *definition)
{
  int64_t n;
  return slug(definition) &&
         sql(db,
             "SELECT COUNT(*)+2*COALESCE(SUM(definition_id='%s'),0) "
             "FROM clan_delivery_claims WHERE account_id=%" PRId64 " AND week_start=%" PRId64,
             definition, account, week) &&
         scalar(db, &n) && n < 2;
}
static struct json_object *contract_row(MYSQL *db)
{
  MYSQL_RES *r = mysql_store_result(db);
  MYSQL_ROW row;
  struct json_object *o = NULL;
  if (!r)
    return NULL;
  row = mysql_fetch_row(r);
  if (row)
  {
    o = json_object_new_object();
    json_object_object_add(o, "accepted_uid", json_object_new_string(row[0]));
    json_object_object_add(o, "clan_uid", json_object_new_string(row[1]));
    json_object_object_add(o, "player_id", json_object_new_int64(atoll(row[2])));
    json_object_object_add(o, "account_id", json_object_new_int64(atoll(row[3])));
    json_object_object_add(o, "definition", json_tokener_parse(row[4]));
    json_object_object_add(o, "expires_at", json_object_new_int64(atoll(row[5])));
    json_object_object_add(o, "week_start", json_object_new_int64(atoll(row[6])));
    json_object_object_add(o, "state", json_object_new_int(atoi(row[7])));
  }
  mysql_free_result(r);
  return o;
}
enum clan_repository_result clan_delivery_active(MYSQL *db, int64_t player, int64_t now,
                                                 struct json_object **out)
{
  struct json_object *o;
  if (player < 1 || now < 0 || !out)
    return CLAN_REPO_INVALID;
  if (!clan_delivery_schema_ready(db))
    return CLAN_REPO_SCHEMA;
  if (!sql(db,
           "SELECT accepted_uid,clan_uid,player_id,account_id,terms,expires_at,week_start,state "
           "FROM clan_delivery_contracts "
           "WHERE player_id=%" PRId64 " AND state=0",
           player))
    return CLAN_REPO_UNAVAILABLE;
  o = contract_row(db);
  if (!o)
    return CLAN_REPO_NOT_FOUND;
  *out = o;
  return CLAN_REPO_OK;
}
/* Lazy per-character reset. Retained claims and committed/pending rewards
 * are never deleted or moved into a new week. Safe to retry after lost ACK. */
enum clan_repository_result clan_delivery_expire(MYSQL *db, int64_t player, int64_t now)
{
  enum clan_repository_result r;
  struct clan_reset_period week;
  if (player < 1 || !clan_progression_week_at(now, &week))
    return CLAN_REPO_INVALID;
  r = begin(db);
  if (r != CLAN_REPO_OK)
    return r;
  if (!sql(db, "UPDATE clan_delivery_contracts SET state=3 WHERE player_id=%" PRId64
               " AND state=0 AND expires_at<=%" PRId64
               " AND NOT EXISTS (SELECT 1 FROM clan_delivery_operations o WHERE o.accepted_uid=clan_delivery_contracts.accepted_uid AND o.state=0)", player, now))
    return end(db, CLAN_REPO_UNAVAILABLE);
  return end(db, CLAN_REPO_OK);
}
enum clan_repository_result clan_delivery_accept(MYSQL *db,
                                                 const struct clan_delivery_definition *d,
                                                 const char *uid, const char *clan, int64_t player,
                                                 int64_t account, int64_t now)
{
  enum clan_repository_result r;
  struct clan_reset_period week;
  struct clan_repository_record c;
  int level;
  int64_t n;
  struct json_object *terms;
  char encoded[4096];
  if (!clan_delivery_definition_valid(d) || !clan_progression_uid_valid(uid) ||
      !clan_progression_uid_valid(clan) || player < 1 || account < 1 ||
      !clan_progression_week_at(now, &week))
    return CLAN_REPO_INVALID;
  terms = clan_delivery_definition_json(d);
  bool ok = encode(terms, encoded, sizeof(encoded));
  json_object_put(terms);
  if (!ok)
    return CLAN_REPO_INVALID;
  r = begin(db);
  if (r != CLAN_REPO_OK)
    return r;
  if (!member(db, player, clan) || clan_repository_load(db, clan, &c) != CLAN_REPO_OK ||
      c.status != CLAN_RECORD_ACTIVE ||
      !clan_award_level(&clan_progression_defaults, c.xp_eighths, &level) || level < d->min_level ||
      !claims(db, account, week.starts_at, d->id))
    return end(db, CLAN_REPO_INELIGIBLE);
  if (!sql(db, "SELECT COUNT(*) FROM clan_delivery_operations WHERE held_player=%" PRId64,
           player) ||
      !scalar(db, &n) || n)
    return end(db, CLAN_REPO_CONFLICT);
  if (!sql(db,
           "UPDATE clan_delivery_contracts SET state=3 WHERE player_id=%" PRId64
           " AND state=0 AND expires_at<=%" PRId64,
           player, now))
    return end(db, CLAN_REPO_UNAVAILABLE);
  if (!sql(db,
           "INSERT INTO "
           "clan_delivery_contracts(accepted_uid,clan_uid,player_id,account_id,definition_id,terms,"
           "accepted_at,expires_at,week_start) "
           "VALUES('%s','%s',%" PRId64 ",%" PRId64 ",'%s',_utf8mb4 X'%s',%" PRId64 ",%" PRId64
           ",%" PRId64 ")",
           uid, clan, player, account, d->id, encoded, now, week.ends_at, week.starts_at))
    return end(db, CLAN_REPO_CONFLICT);
  return end(db, CLAN_REPO_OK);
}
enum clan_repository_result clan_delivery_abandon(MYSQL *db, int64_t player, int64_t now)
{
  enum clan_repository_result r;
  int64_t n;
  if (player < 1 || now < 0)
    return CLAN_REPO_INVALID;
  r = begin(db);
  if (r != CLAN_REPO_OK)
    return r;
  if (!sql(db, "SELECT COUNT(*) FROM clan_delivery_operations WHERE held_player=%" PRId64,
           player) ||
      !scalar(db, &n) || n)
    return end(db, CLAN_REPO_CONFLICT);
  if (!sql(db, "UPDATE clan_delivery_contracts SET state=2 WHERE player_id=%" PRId64 " AND state=0",
           player))
    return end(db, CLAN_REPO_UNAVAILABLE);
  return end(db, CLAN_REPO_OK);
}
static void event_from_quote(struct json_object *q, struct clan_reward_event *e)
{
  memset(e, 0, sizeof(*e));
  snprintf(e->operation_uid, 33, "%s", str(q, "operation_uid"));
  snprintf(e->event_uid, 33, "%s", str(q, "accepted_uid"));
  snprintf(e->clan_uid, 33, "%s", str(q, "clan_uid"));
  e->player_id = num(q, "player_id");
  e->account_id = num(q, "account_id");
  e->occurred_at = num(q, "occurred_at");
  e->source = CLAN_SOURCE_DELIVERY;
  e->rules_version = CLAN_PROGRESSION_RULES_VERSION;
  e->requested_xp_eighths = 800;
  e->requested_influence_eighths = 400;
  e->requested_favor_eighths = 400;
}
static struct json_object *delta_json(const struct clan_reward_delta *d)
{
  struct json_object *o = json_object_new_object();
#define N(k) json_object_object_add(o, #k, json_object_new_int64(d->k))
  N(xp_eighths);
  N(influence_eighths);
  N(favor_eighths);
  N(account_base_xp_eighths);
  N(account_claims);
  N(incoming_xp_eighths);
  N(cap_flags);
#undef N
  return o;
}
static bool exact(const struct clan_reward_delta *d, struct json_object *q)
{
  struct json_object *o = delta_json(d);
  bool same = json_object_equal(o, get(q, "delta"));
  json_object_put(o);
  return same;
}
enum clan_repository_result clan_delivery_quote(MYSQL *db, int64_t player, int64_t account,
                                                int64_t now, struct json_object **out)
{
  enum clan_repository_result r;
  struct json_object *q = NULL;
  struct clan_status_report report;
  struct clan_reward_event e;
  struct clan_reward_snapshot s = {0};
  struct clan_reward_delta d;
  char uid[33];
  if (player < 1 || account < 1 || now < 0 || !out || now > INT64_MAX - 60)
    return CLAN_REPO_INVALID;
  r = clan_delivery_active(db, player, now, &q);
  if (r != CLAN_REPO_OK)
    return r;
  r = clan_repository_status_read(db, player, account, NULL, false, now, &report);
  if (r != CLAN_REPO_OK || !report.full_clan_access || report.membership != CLAN_ACCESS_ACCEPTED ||
      strcmp(report.clan.uid, str(q, "clan_uid")) || num(q, "account_id") != account ||
      now >= num(q, "expires_at") ||
      !claims(db, account, num(q, "week_start"), str(get(q, "definition"), "id")))
  {
    json_object_put(q);
    return CLAN_REPO_INELIGIBLE;
  }
  if (!clan_repository_uid_generate(uid))
  {
    json_object_put(q);
    return CLAN_REPO_UNAVAILABLE;
  }
  json_object_object_add(q, "operation_uid", json_object_new_string(uid));
  json_object_object_add(q, "occurred_at", json_object_new_int64(now));
  json_object_object_add(q, "confirm_before", json_object_new_int64(now + 60));
  event_from_quote(q, &e);
  s.clan = report.clan;
  s.account = report.account;
  s.weekly = report.weekly;
  s.favor = report.favor;
  s.account_present = report.account_present;
  s.weekly_present = report.weekly_present;
  s.favor_present = report.favor_present;
  if (!clan_award_personal_project(&e, &s, &d) ||
      !(d.xp_eighths || d.influence_eighths || d.favor_eighths))
  {
    json_object_put(q);
    return CLAN_REPO_CAPACITY;
  }
  json_object_object_add(q, "delta", delta_json(&d));
  *out = q;
  return CLAN_REPO_OK;
}
struct participant_context
{
  struct json_object *quote;
  int64_t now;
};
static bool participate(MYSQL *db, const struct clan_reward_event *e,
                        const struct clan_reward_snapshot *s, const struct clan_reward_delta *delta,
                        void *context)
{
  struct participant_context *ctx = context;
  struct json_object *q = ctx->quote, *stored = NULL;
  struct clan_reward_delta projected;
  struct clan_delivery_definition definition;
  char encoded[8192];
  bool ok;
  if (e->source != CLAN_SOURCE_DELIVERY ||
      !clan_delivery_definition_parse(get(q, "definition"), &definition) ||
      !claims(db, e->account_id, num(q, "week_start"), definition.id))
    return false;
  if (!sql(db,
           "SELECT accepted_uid,clan_uid,player_id,account_id,terms,expires_at,week_start,state "
           "FROM clan_delivery_contracts "
           "WHERE accepted_uid='%s' FOR UPDATE",
           e->event_uid))
    return false;
  stored = contract_row(db);
  ok = stored && num(stored, "state") == 0 && num(stored, "player_id") == e->player_id &&
       num(stored, "account_id") == e->account_id &&
       !strcmp(str(stored, "clan_uid"), e->clan_uid) &&
       json_object_equal(get(stored, "definition"), get(q, "definition")) &&
       num(stored, "week_start") == num(q, "week_start") &&
       num(stored, "expires_at") == num(q, "expires_at");
  if (stored)
    json_object_put(stored);
  if (!ok)
    return false;
  if (!delta)
  {
    if (ctx->now >= num(q, "confirm_before") || ctx->now >= num(q, "expires_at") ||
        ctx->now < e->occurred_at || !clan_award_personal_project(e, s, &projected) ||
        !exact(&projected, q) ||
        !(projected.xp_eighths || projected.influence_eighths || projected.favor_eighths) ||
        !encode(q, encoded, sizeof(encoded)))
      return false;
    return sql(db,
               "INSERT INTO clan_delivery_operations(operation_uid,accepted_uid,player_id,quote) "
               "VALUES('%s','%s',%" PRId64 ",_utf8mb4 X'%s')",
               e->operation_uid, e->event_uid, e->player_id, encoded);
  }
  if (!exact(delta, q))
    return false;
  if (!sql(db, "UPDATE clan_delivery_operations SET state=1 WHERE operation_uid='%s' AND state=0",
           e->operation_uid) ||
      mysql_affected_rows(db) != 1)
    return false;
  return sql(db, "INSERT INTO clan_delivery_claims VALUES(%" PRId64 ",%" PRId64 ",'%s','%s')",
             e->account_id, num(q, "week_start"), definition.id, e->operation_uid) &&
         sql(db, "UPDATE clan_delivery_contracts SET state=1 WHERE accepted_uid='%s' AND state=0",
             e->event_uid) &&
         mysql_affected_rows(db) == 1;
}
static bool decide(const struct clan_reward_event *e, const struct clan_reward_snapshot *s,
                   struct clan_reward_delta *d, void *ctx)
{
  (void)ctx;
  return e->source == CLAN_SOURCE_DELIVERY && clan_award_personal_project(e, s, d);
}
enum clan_repository_result clan_delivery_prepare(MYSQL *db, struct json_object *quote, int64_t now,
                                                  struct json_object **out)
{
  struct clan_reward_event e;
  struct participant_context ctx = {quote, now};
  if (!clan_delivery_schema_ready(db))
    return CLAN_REPO_SCHEMA;
  event_from_quote(quote, &e);
  return clan_reward_prepare_delivery(db, &e, now, participate, &ctx, out);
}
enum clan_repository_result clan_delivery_operation(MYSQL *db, int64_t player, const char *uid,
                                                    struct json_object **out)
{
  MYSQL_RES *r;
  MYSQL_ROW row;
  struct json_object *o;
  if (player < 1 || !out || (uid && !clan_progression_uid_valid(uid)))
    return CLAN_REPO_INVALID;
  if (!clan_delivery_schema_ready(db))
    return CLAN_REPO_SCHEMA;
  if (uid)
  {
    if (!sql(db,
             "SELECT quote,state FROM clan_delivery_operations WHERE player_id=%" PRId64
             " AND operation_uid='%s'",
             player, uid))
      return CLAN_REPO_UNAVAILABLE;
  }
  else if (!sql(db, "SELECT quote,state FROM clan_delivery_operations WHERE held_player=%" PRId64,
                player))
    return CLAN_REPO_UNAVAILABLE;
  r = mysql_store_result(db);
  if (!r)
    return CLAN_REPO_UNAVAILABLE;
  row = mysql_fetch_row(r);
  if (!row)
  {
    mysql_free_result(r);
    return CLAN_REPO_NOT_FOUND;
  }
  o = json_tokener_parse(row[0]);
  if (o)
    json_object_object_add(o, "operation_state", json_object_new_int(atoi(row[1])));
  mysql_free_result(r);
  if (!o)
    return CLAN_REPO_SCHEMA;
  *out = o;
  return CLAN_REPO_OK;
}
enum clan_repository_result clan_delivery_complete(MYSQL *db, struct json_object *operation,
                                                   int64_t now, struct json_object **out)
{
  struct json_object *frozen = NULL;
  struct participant_context ctx;
  enum clan_repository_result r;
  if (!operation || !clan_delivery_schema_ready(db))
    return CLAN_REPO_SCHEMA;
  r = clan_delivery_operation(db, num(operation, "player_id"), str(operation, "operation_uid"),
                              &frozen);
  if (r != CLAN_REPO_OK)
    return r;
  ctx.quote = frozen;
  ctx.now = now;
  r = clan_reward_complete_delivery(db, str(frozen, "operation_uid"), now, decide, participate,
                                    &ctx, out);
  json_object_put(frozen);
  return r;
}
enum clan_repository_result clan_delivery_cancel(MYSQL *db, const char *uid, int64_t now)
{
  /* Cancel receipt first. A crash between these commits is recovered by reading
   * the terminal receipt; no worker can complete a canceled event. */
  struct json_object *receipt = NULL;
  enum clan_repository_result r;
  if (!clan_progression_uid_valid(uid) || !clan_delivery_schema_ready(db))
    return CLAN_REPO_INVALID;
  r = clan_reward_cancel(db, uid, 29, now, &receipt);
  if (receipt)
    json_object_put(receipt);
  if (r != CLAN_REPO_OK)
    return r;
  r = begin(db);
  if (r != CLAN_REPO_OK)
    return r;
  if (!sql(db,
           "UPDATE clan_delivery_contracts c JOIN clan_delivery_operations o ON "
           "o.accepted_uid=c.accepted_uid "
           "SET c.state=2 WHERE o.operation_uid='%s' AND o.state=0 AND c.state=0",
           uid) ||
      !sql(db, "UPDATE clan_delivery_operations SET state=3 WHERE operation_uid='%s' AND state=0",
           uid))
    return end(db, CLAN_REPO_UNAVAILABLE);
  return end(db, CLAN_REPO_OK);
}
enum clan_repository_result clan_delivery_ack(MYSQL *db, const char *uid)
{
  enum clan_repository_result r;
  if (!clan_progression_uid_valid(uid))
    return CLAN_REPO_INVALID;
  r = begin(db);
  if (r != CLAN_REPO_OK)
    return r;
  if (!sql(db, "UPDATE clan_delivery_operations SET state=2 WHERE operation_uid='%s' AND state=1",
           uid))
    return end(db, CLAN_REPO_UNAVAILABLE);
  return end(db, CLAN_REPO_OK);
}

/* Keep independent unlock/difficulty cohorts viable during rotation. */
void clan_delivery_catalog_rotate(struct clan_delivery_catalog *c, int64_t now)
{
  struct clan_delivery_catalog original = *c;
  size_t i, j;
  c->count = 0;
  for (i = 0; i < original.count; i++)
  {
    const char *ids[32];
    size_t count = 0;
    const struct clan_delivery_definition *d = &original.entries[i];
    for (j = 0; j < original.count; j++)
    {
      const struct clan_delivery_definition *other = &original.entries[j];
      if (d->min_level == other->min_level)
        ids[count++] = other->id;
    }
    if (clan_contract_rotates_in(d->id, ids, count, now))
      c->entries[c->count++] = *d;
  }
}
