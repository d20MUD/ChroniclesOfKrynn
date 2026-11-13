#include "clan_contract_rotation.h"
#include "clan_objectives.h"
#include "clan_status.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <inttypes.h>
#ifdef CLAN_REWARD_TEST
#include <unistd.h>
int clan_objective_crash;
#endif
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
static bool definition_valid(const struct clan_objective_definition *d)
{
  size_t i;
  if (!d || !memchr(d->id, 0, 33) || !memchr(d->name, 0, 81) || !slug(d->id) || !*d->name ||
      d->required != 20 || d->min_clan_level < 1 || d->min_clan_level > 20 ||
      d->min_player_level < 1 || d->max_player_level > 30 ||
      d->min_player_level > d->max_player_level)
    return false;
  for (i = 0; d->name[i]; i++)
    if ((unsigned char)d->name[i] < 32 || (unsigned char)d->name[i] > 126)
      return false;
  return d->family == CLAN_SOURCE_GATHERING
             ? (d->target > 0 && d->target < 75 && d->grade >= 1 && d->grade <= 10)
             : d->family == CLAN_SOURCE_BOUNTY && d->target > 0 && d->target <= 99999999 &&
                   d->grade == 0;
}
struct json_object *clan_objective_definition_json(const struct clan_objective_definition *d)
{
  struct json_object *o;
  if (!definition_valid(d))
    return NULL;
  o = json_object_new_object();
  json_object_object_add(o, "id", json_object_new_string(d->id));
  json_object_object_add(o, "name", json_object_new_string(d->name));
#define N(k) json_object_object_add(o, #k, json_object_new_int(d->k))
  N(family);
  N(target);
  N(grade);
  N(required);
  N(min_clan_level);
  N(min_player_level);
  N(max_player_level);
#undef N
  return o;
}
bool clan_objective_definition_parse(struct json_object *o, struct clan_objective_definition *out)
{
  struct clan_objective_definition d = {0};
  size_t i;
  const char *keys[] = {"family",         "target",           "grade",           "required",
                        "min_clan_level", "min_player_level", "max_player_level"};
  if (!out || !o || json_object_get_type(o) != json_type_object ||
      json_object_object_length(o) != 9 || json_object_get_type(get(o, "id")) != json_type_string ||
      json_object_get_type(get(o, "name")) != json_type_string || strlen(str(o, "id")) > 32 ||
      strlen(str(o, "name")) > 80 ||
      strlen(str(o, "id")) != (size_t)json_object_get_string_len(get(o, "id")) ||
      strlen(str(o, "name")) != (size_t)json_object_get_string_len(get(o, "name")))
    return false;
  for (i = 0; i < 7; i++)
    if (json_object_get_type(get(o, keys[i])) != json_type_int || num(o, keys[i]) < 0 ||
        num(o, keys[i]) > 99999999)
      return false;
  snprintf(d.id, 33, "%s", str(o, "id"));
  snprintf(d.name, 81, "%s", str(o, "name"));
  d.family = num(o, "family");
  d.target = num(o, "target");
  d.grade = num(o, "grade");
  d.required = num(o, "required");
  d.min_clan_level = num(o, "min_clan_level");
  d.min_player_level = num(o, "min_player_level");
  d.max_player_level = num(o, "max_player_level");
  if (!definition_valid(&d))
    return false;
  *out = d;
  return true;
}
bool clan_objective_catalog_parse(struct json_object *o, struct clan_objective_catalog *out)
{
  struct clan_objective_catalog c = {0};
  struct json_object *a = get(o, "objectives");
  size_t i, j;
  if (!out || json_object_get_type(o) != json_type_object || json_object_object_length(o) != 2 ||
      json_object_get_type(get(o, "version")) != json_type_int || num(o, "version") != 1 ||
      json_object_get_type(a) != json_type_array || !(c.count = json_object_array_length(a)) ||
      c.count > CLAN_OBJECTIVE_MAX)
    return false;
  for (i = 0; i < c.count; i++)
  {
    if (!clan_objective_definition_parse(json_object_array_get_idx(a, i), &c.entries[i]))
      return false;
    for (j = 0; j < i; j++)
      if (!strcmp(c.entries[i].id, c.entries[j].id))
        return false;
  }
  *out = c;
  return true;
}
bool clan_objective_schema_ready(MYSQL *db)
{
  int64_t n;
  return sql(db, "SELECT COUNT(*) FROM information_schema.TABLES WHERE TABLE_SCHEMA=DATABASE() AND "
                 "TABLE_NAME IN "
                 "('clan_objective_schema','clan_objective_contracts','clan_objective_events','"
                 "clan_objective_claims','clan_bounty_shared_claims') AND ENGINE='InnoDB'") &&
         scalar(db, &n) && n == 5 &&
         sql(db, "SELECT COUNT(*) FROM clan_objective_schema WHERE version=1") && scalar(db, &n) &&
         n == 1 &&
         sql(db,
             "SELECT COUNT(*) FROM information_schema.TRIGGERS WHERE TRIGGER_SCHEMA=DATABASE() AND "
             "TRIGGER_NAME IN "
             "('co_terms','co_contract_retained','co_event_retained','co_event_immutable','co_"
             "claim_retained','co_claim_immutable','co_shared_retained','co_shared_immutable')") &&
         scalar(db, &n) && n == 8 &&
         sql(db, "SELECT COUNT(*) FROM (SELECT "
                 "TABLE_NAME,INDEX_NAME,NON_UNIQUE,GROUP_CONCAT(COLUMN_NAME ORDER BY SEQ_IN_INDEX) "
                 "AS cols FROM information_schema.STATISTICS WHERE TABLE_SCHEMA=DATABASE() AND "
                 "TABLE_NAME IN "
                 "('clan_objective_contracts','clan_objective_events','clan_objective_claims','"
                 "clan_bounty_shared_claims') GROUP BY TABLE_NAME,INDEX_NAME,NON_UNIQUE) AS "
                 "checked_keys WHERE NON_UNIQUE=0 AND ((TABLE_NAME='clan_objective_contracts' AND "
                 "((INDEX_NAME='PRIMARY' AND cols='contract_uid') OR (INDEX_NAME='co_slot' AND "
                 "cols='active_player,family'))) OR (TABLE_NAME='clan_objective_events' AND "
                 "((INDEX_NAME='PRIMARY' AND cols='contract_uid,event_uid') OR "
                 "(INDEX_NAME='co_event_account' AND cols='account_id,family,event_uid'))) OR "
                 "(TABLE_NAME='clan_objective_claims' AND ((INDEX_NAME='PRIMARY' AND "
                 "cols='account_id,week_start,family,definition_id') OR "
                 "(INDEX_NAME='co_claim_contract' AND cols='contract_uid'))) OR "
                 "(TABLE_NAME='clan_bounty_shared_claims' AND ((INDEX_NAME='PRIMARY' AND "
                 "cols='clan_uid,week_start,definition_id') OR (INDEX_NAME='co_shared_operation' "
                 "AND cols='operation_uid'))))") &&
         scalar(db, &n) && n == 8;
}
static enum clan_repository_result begin(MYSQL *db)
{
  int64_t n;
  if (!clan_objective_schema_ready(db) || clan_repository_schema_check(db) != CLAN_REPO_OK)
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
static bool family_valid(int family)
{
  return family == CLAN_SOURCE_GATHERING || family == CLAN_SOURCE_BOUNTY;
}
static bool quota(MYSQL *db, int64_t account, int family, int64_t week, const char *definition,
                  bool *allowed)
{
  int64_t n;
  if (!slug(definition) ||
      !sql(db,
           "SELECT COUNT(*)+2*COALESCE(SUM(definition_id='%s'),0) FROM clan_objective_claims WHERE "
           "account_id=%" PRId64 " AND family=%d AND week_start=%" PRId64,
           definition, account, family, week) ||
      !scalar(db, &n))
    return false;
  *allowed = n < 2;
  return true;
}
static bool allowance(MYSQL *db, int64_t account, int family, int64_t week, const char *definition)
{
  bool allowed = false;
  return quota(db, account, family, week, definition, &allowed) && allowed;
}

#define COLUMNS                                                                                    \
  "contract_uid,clan_uid,player_id,account_id,family,terms,accepted_at,expires_at,week_start,"     \
  "progress,state"
static struct json_object *row_contract(MYSQL *db)
{
  MYSQL_RES *res = mysql_store_result(db);
  MYSQL_ROW row;
  struct json_object *o = NULL;
  int i;
  static const char *keys[] = {"contract_uid", "clan_uid",   "player_id",   "account_id",
                               "family",       "definition", "accepted_at", "expires_at",
                               "week_start",   "progress",   "state"};
  if (!res)
    return NULL;
  row = mysql_fetch_row(res);
  if (row)
  {
    o = json_object_new_object();
    for (i = 0; i < 11; i++)
      json_object_object_add(o, keys[i],
                             i < 2    ? json_object_new_string(row[i])
                             : i == 5 ? json_tokener_parse(row[i])
                                      : json_object_new_int64(atoll(row[i])));
  }
  mysql_free_result(res);
  return o;
}
enum clan_repository_result clan_objective_active(MYSQL *db, int64_t player, int family,
                                                  struct json_object **out)
{
  struct json_object *o;
  if (player < 1 || !family_valid(family) || !out)
    return CLAN_REPO_INVALID;
  if (!clan_objective_schema_ready(db))
    return CLAN_REPO_SCHEMA;
  if (!sql(db,
           "SELECT " COLUMNS " FROM clan_objective_contracts WHERE active_player=%" PRId64
           " AND family=%d",
           player, family))
    return CLAN_REPO_UNAVAILABLE;
  o = row_contract(db);
  if (!o)
    return CLAN_REPO_NOT_FOUND;
  *out = o;
  return CLAN_REPO_OK;
}
enum clan_repository_result clan_objective_claims(MYSQL *db, int64_t account, int family,
                                                  int64_t now, struct json_object **out)
{
  struct clan_reset_period w;
  MYSQL_RES *res;
  MYSQL_ROW row;
  struct json_object *a;
  if (account < 1 || !family_valid(family) || !out || !clan_progression_week_at(now, &w))
    return CLAN_REPO_INVALID;
  if (!clan_objective_schema_ready(db))
    return CLAN_REPO_SCHEMA;
  if (!sql(db,
           "SELECT definition_id FROM clan_objective_claims WHERE account_id=%" PRId64
           " AND family=%d AND week_start=%" PRId64 " ORDER BY definition_id LIMIT 3",
           account, family, w.starts_at))
    return CLAN_REPO_UNAVAILABLE;
  res = mysql_store_result(db);
  if (!res)
    return CLAN_REPO_UNAVAILABLE;
  a = json_object_new_array();
  while ((row = mysql_fetch_row(res)))
    json_object_array_add(a, json_object_new_string(row[0]));
  mysql_free_result(res);
  if (json_object_array_length(a) > 2)
  {
    json_object_put(a);
    return CLAN_REPO_SCHEMA;
  }
  *out = a;
  return CLAN_REPO_OK;
}
/* Lazy per-character reset. Retained claims and committed/pending rewards
 * are never deleted or moved into a new week. Safe to retry after lost ACK. */
enum clan_repository_result clan_objective_expire(MYSQL *db, int64_t player, int64_t now)
{
  enum clan_repository_result r;
  struct clan_reset_period week;
  if (player < 1 || !clan_progression_week_at(now, &week))
    return CLAN_REPO_INVALID;
  r = begin(db);
  if (r != CLAN_REPO_OK)
    return r;
  if (!sql(db, "UPDATE clan_objective_contracts SET state=4 WHERE player_id=%" PRId64
               " AND state=0 AND expires_at<=%" PRId64
               "", player, now))
    return end(db, CLAN_REPO_UNAVAILABLE);
  return end(db, CLAN_REPO_OK);
}
enum clan_repository_result clan_objective_accept(MYSQL *db,
                                                  const struct clan_objective_definition *d,
                                                  const char *uid, const char *clan, int64_t player,
                                                  int64_t account, int level, int64_t now)
{
  enum clan_repository_result r;
  struct clan_reset_period w;
  struct clan_repository_record c;
  int clan_level;
  struct json_object *terms;
  char encoded[4096];
  bool ok;
  if (!definition_valid(d) || !clan_progression_uid_valid(uid) ||
      !clan_progression_uid_valid(clan) || player < 1 || account < 1 ||
      !clan_progression_week_at(now, &w))
    return CLAN_REPO_INVALID;
  terms = clan_objective_definition_json(d);
  ok = encode(terms, encoded, sizeof(encoded));
  json_object_put(terms);
  if (!ok)
    return CLAN_REPO_INVALID;
  r = begin(db);
  if (r != CLAN_REPO_OK)
    return r;
  if (!member(db, player, clan) || clan_repository_load(db, clan, &c) != CLAN_REPO_OK ||
      c.status != CLAN_RECORD_ACTIVE ||
      !clan_award_level(&clan_progression_defaults, c.xp_eighths, &clan_level) ||
      clan_level < d->min_clan_level || level < d->min_player_level ||
      level > d->max_player_level || !allowance(db, account, d->family, w.starts_at, d->id))
    return end(db, CLAN_REPO_INELIGIBLE);
  if (!sql(db,
           "UPDATE clan_objective_contracts SET state=4 WHERE player_id=%" PRId64
           " AND family=%d AND state=0 AND expires_at<=%" PRId64,
           player, d->family, now))
    return end(db, CLAN_REPO_UNAVAILABLE);
  if (!sql(db,
           "INSERT INTO "
           "clan_objective_contracts(contract_uid,clan_uid,player_id,account_id,family,definition_"
           "id,terms,accepted_at,expires_at,week_start) VALUES('%s','%s',%" PRId64 ",%" PRId64
           ",%d,'%s',_utf8mb4 X'%s',%" PRId64 ",%" PRId64 ",%" PRId64 ")",
           uid, clan, player, account, d->family, d->id, encoded, now, w.ends_at, w.starts_at))
    return end(db, CLAN_REPO_CONFLICT);
  return end(db, CLAN_REPO_OK);
}
enum clan_repository_result clan_objective_abandon(MYSQL *db, int64_t player, int family)
{
  enum clan_repository_result r;
  if (player < 1 || !family_valid(family))
    return CLAN_REPO_INVALID;
  r = begin(db);
  if (r != CLAN_REPO_OK)
    return r;
  int64_t pending;
  if (!sql(db,
           "SELECT COUNT(*) FROM clan_objective_contracts WHERE player_id=%" PRId64
           " AND family=%d AND state=1",
           player, family) ||
      !scalar(db, &pending))
    return end(db, CLAN_REPO_UNAVAILABLE);
  if (pending)
    return end(db, CLAN_REPO_CONFLICT);
  if (!sql(db,
           "UPDATE clan_objective_contracts SET state=3 WHERE player_id=%" PRId64
           " AND family=%d AND state=0",
           player, family))
    return end(db, CLAN_REPO_UNAVAILABLE);
  return end(db, CLAN_REPO_OK);
}
static bool event_valid(const struct clan_objective_event *e, bool frozen)
{
  return e && clan_progression_uid_valid(e->event_uid) &&
         (!frozen || clan_progression_uid_valid(e->contract_uid)) &&
         clan_progression_uid_valid(e->clan_uid) && e->player_id > 0 && e->account_id > 0 &&
         e->occurred_at >= 0 && e->room >= 0 && family_valid(e->family) && e->target > 0 &&
         e->base_units > 0 && e->base_units <= 20 &&
         (e->family == CLAN_SOURCE_GATHERING ? (e->target < 75 && e->grade >= 1 && e->grade <= 10)
                                             : (e->grade == 0 && e->base_units == 1));
}
static bool matches(struct json_object *o, const struct clan_objective_event *e)
{
  struct clan_objective_definition d;
  return o && num(o, "player_id") == e->player_id && num(o, "account_id") == e->account_id &&
         !strcmp(str(o, "clan_uid"), e->clan_uid) && num(o, "family") == e->family &&
         num(o, "accepted_at") <= e->occurred_at && num(o, "expires_at") > e->occurred_at &&
         clan_objective_definition_parse(get(o, "definition"), &d) && d.target == e->target &&
         d.grade == e->grade;
}
enum clan_repository_result clan_objective_capture(MYSQL *db, struct clan_objective_event *e,
                                                   int level)
{
  struct json_object *o = NULL;
  struct clan_objective_definition d;
  enum clan_repository_result r;
  if (!event_valid(e, false))
    return CLAN_REPO_INVALID;
  r = clan_objective_active(db, e->player_id, e->family, &o);
  if (r != CLAN_REPO_OK)
    return r;
  bool ok = matches(o, e) && num(o, "state") == 0 && member(db, e->player_id, e->clan_uid) &&
            clan_objective_definition_parse(get(o, "definition"), &d) &&
            level >= d.min_player_level && level <= d.max_player_level;
  if (ok)
    snprintf(e->contract_uid, 33, "%s", str(o, "contract_uid"));
  json_object_put(o);
  return ok ? CLAN_REPO_OK : CLAN_REPO_INELIGIBLE;
}
/* Caller already holds the control lock. SQL receipts are server-generated and
 * atomically backed by final objective progress (or a unique shared claim). */
static bool reward_insert(MYSQL *db, const struct clan_reward_event *e, int64_t now)
{
  struct clan_reset_period w;
  int64_t count;
  if (!clan_reward_event_valid(e) || !clan_progression_week_at(e->occurred_at, &w) ||
      now < e->occurred_at)
    return false;
  if (!sql(db, "SELECT COUNT(*) FROM clan_reward_receipts WHERE receipt_status=0") ||
      !scalar(db, &count) || count >= clan_progression_defaults.limits.pending_per_server)
    return false;
  if (e->player_id &&
      (!sql(db,
            "SELECT COUNT(*) FROM clan_reward_receipts WHERE receipt_status=0 AND "
            "player_id=%" PRId64,
            e->player_id) ||
       !scalar(db, &count) || count >= clan_progression_defaults.limits.pending_per_character))
    return false;
  char beneficiary[33];
  if (e->player_id)
    snprintf(beneficiary, 33, "%" PRId64, e->player_id);
  else
    snprintf(beneficiary, 33, "%s", e->clan_uid);
  return sql(db,
             "INSERT INTO "
             "clan_reward_receipts(operation_uid,event_uid,clan_uid,beneficiary_key,account_id,"
             "player_id,source_id,occurred_at_utc,starts_at_utc,rules_version,requested_xp_eighths,"
             "requested_influence_eighths,requested_favor_eighths,accepted_at_utc) "
             "VALUES('%s','%s','%s','%s',%" PRId64 ",%" PRId64 ",%d,%" PRId64 ",%" PRId64
             ",%d,%" PRId64 ",%" PRId64 ",%" PRId64 ",%" PRId64 ")",
             e->operation_uid, e->event_uid, e->clan_uid, beneficiary, e->account_id, e->player_id,
             e->source, e->occurred_at, w.starts_at, e->rules_version, e->requested_xp_eighths,
             e->requested_influence_eighths, e->requested_favor_eighths, now);
}
enum clan_repository_result clan_objective_progress(MYSQL *db, const struct clan_objective_event *e,
                                                    int64_t now)
{
  enum clan_repository_result r;
  struct json_object *o;
  int64_t n;
  MYSQL_RES *res;
  MYSQL_ROW row;
  bool same;
  struct clan_repository_record clan;
  if (!event_valid(e, true) || now < e->occurred_at)
    return CLAN_REPO_INVALID;
  r = begin(db);
  if (r != CLAN_REPO_OK)
    return r;
  if (!sql(db,
           "SELECT "
           "e.contract_uid,e.player_id,e.occurred_at,e.target,e.grade,e.base_units,e.room,c.clan_"
           "uid FROM "
           "clan_objective_events e JOIN clan_objective_contracts c ON "
           "c.contract_uid=e.contract_uid WHERE e.account_id=%" PRId64
           " AND e.family=%d AND e.event_uid='%s'",
           e->account_id, e->family, e->event_uid))
    return end(db, CLAN_REPO_UNAVAILABLE);
  res = mysql_store_result(db);
  if (!res)
    return end(db, CLAN_REPO_UNAVAILABLE);
  row = mysql_fetch_row(res);
  if (row)
  {
    same = !strcmp(row[0], e->contract_uid) && atoll(row[1]) == e->player_id &&
           atoll(row[2]) == e->occurred_at && atoi(row[3]) == e->target &&
           atoi(row[4]) == e->grade && atoi(row[5]) == e->base_units && atoll(row[6]) == e->room &&
           !strcmp(row[7], e->clan_uid);
    mysql_free_result(res);
    return end(db, same ? CLAN_REPO_OK : CLAN_REPO_CONFLICT);
  }
  mysql_free_result(res);
  if (!sql(db,
           "SELECT " COLUMNS " FROM clan_objective_contracts WHERE contract_uid='%s' FOR UPDATE",
           e->contract_uid))
    return end(db, CLAN_REPO_UNAVAILABLE);
  o = row_contract(db);
  if (!o)
    return end(db, CLAN_REPO_NOT_FOUND);
  same = matches(o, e) && num(o, "state") == 0;
  int progress = num(o, "progress");
  json_object_put(o);
  if (!same)
    return end(db, CLAN_REPO_INELIGIBLE);
  r = clan_repository_load(db, e->clan_uid, &clan);
  if (r != CLAN_REPO_OK || clan.status != CLAN_RECORD_ACTIVE)
    return end(db, CLAN_REPO_DELETED);
  if (!sql(db,
           "INSERT INTO clan_objective_events VALUES('%s','%s',%" PRId64 ",%d,%" PRId64 ",%" PRId64
           ",%d,%d,%d,%" PRId64 ")",
           e->contract_uid, e->event_uid, e->account_id, e->family, e->player_id, e->occurred_at,
           e->target, e->grade, e->base_units, e->room))
    return end(db, CLAN_REPO_CONFLICT);
  n = progress + e->base_units;
  if (n > 20)
    n = 20;
  if (!sql(db,
           "UPDATE clan_objective_contracts SET progress=%" PRId64
           ",state=%d WHERE contract_uid='%s' AND state=0",
           n, n == 20 ? 1 : 0, e->contract_uid))
    return end(db, CLAN_REPO_UNAVAILABLE);
  if (n == 20)
  {
    struct clan_reward_event reward = {0};
    snprintf(reward.operation_uid, 33, "%s", e->contract_uid);
    snprintf(reward.event_uid, 33, "%s", e->contract_uid);
    snprintf(reward.clan_uid, 33, "%s", e->clan_uid);
    reward.account_id = e->account_id;
    reward.player_id = e->player_id;
    reward.occurred_at = e->occurred_at;
    reward.source = e->family;
    reward.rules_version = CLAN_PROGRESSION_RULES_VERSION;
    reward.requested_xp_eighths = 800;
    reward.requested_influence_eighths = 400;
    reward.requested_favor_eighths = 400;
    if (!reward_insert(db, &reward, now))
      return end(db, CLAN_REPO_UNAVAILABLE);
  }
#ifdef CLAN_REWARD_TEST
  if (clan_objective_crash == 1)
    _exit(91);
#endif
  r = end(db, CLAN_REPO_OK);
#ifdef CLAN_REWARD_TEST
  if (clan_objective_crash == 2)
    _exit(92);
#endif
  return r;
}
struct completion_context
{
  struct json_object *contract;
  bool allowed;
  int64_t now;
};
static bool decide(const struct clan_reward_event *e, const struct clan_reward_snapshot *s,
                   struct clan_reward_delta *d, void *context)
{
  struct completion_context *c = context;
  if (!family_valid(e->source))
    return false;
  if (!c->allowed)
  {
    memset(d, 0, sizeof(*d));
    d->cap_flags = CLAN_AWARD_SOURCE_LIMIT;
    return true;
  }
  return clan_award_personal_project(e, s, d);
}
static bool shared_bounty(MYSQL *db, const struct clan_reward_event *personal,
                          const char *definition, int64_t week, int64_t now)
{
  int64_t n;
  struct clan_reward_event e = {0};
  MYSQL_RES *res;
  MYSQL_ROW row;
  if (!sql(db,
           "SELECT COUNT(*)+2*COALESCE(SUM(definition_id='%s'),0) FROM clan_bounty_shared_claims "
           "WHERE clan_uid='%s' AND week_start=%" PRId64,
           definition, personal->clan_uid, week) ||
      !scalar(db, &n))
    return false;
  if (n >= 2)
    return true;
  if (!sql(db,
           "SELECT LEFT(SHA2(CONCAT('bounty-v1:', '%s', ':', '%s', ':', '%" PRId64 "'),256),32)",
           personal->clan_uid, definition, week))
    return false;
  res = mysql_store_result(db);
  if (!res)
    return false;
  row = mysql_fetch_row(res);
  if (!row || !row[0])
  {
    mysql_free_result(res);
    return false;
  }
  snprintf(e.operation_uid, 33, "%s", row[0]);
  mysql_free_result(res);
  snprintf(e.event_uid, 33, "%s", e.operation_uid);
  snprintf(e.clan_uid, 33, "%s", personal->clan_uid);
  e.source = CLAN_SOURCE_BOUNTY_SHARED;
  e.rules_version = CLAN_PROGRESSION_RULES_VERSION;
  e.occurred_at = personal->occurred_at;
  e.requested_xp_eighths = 1600;
  e.requested_influence_eighths = 800;
  return reward_insert(db, &e, now) &&
         sql(db, "INSERT INTO clan_bounty_shared_claims VALUES('%s',%" PRId64 ",'%s','%s')",
             e.clan_uid, week, definition, e.operation_uid);
}
static bool finish_contract(MYSQL *db, const struct clan_reward_event *e,
                            const struct clan_reward_snapshot *s, const struct clan_reward_delta *d,
                            void *context)
{
  struct completion_context *c = context;
  struct json_object *o;
  bool allowed, ok;
  int64_t week;
  char definition[33];
  (void)s;
  if (!family_valid(e->source) ||
      !sql(db,
           "SELECT " COLUMNS " FROM clan_objective_contracts WHERE contract_uid='%s' FOR UPDATE",
           e->event_uid))
    return false;
  o = row_contract(db);
  if (!o)
    return false;
  ok = num(o, "state") == 1 && num(o, "progress") == 20 && num(o, "player_id") == e->player_id &&
       num(o, "account_id") == e->account_id && num(o, "family") == e->source &&
       !strcmp(str(o, "clan_uid"), e->clan_uid);
  week = num(o, "week_start");
  snprintf(definition, 33, "%s", str(get(o, "definition"), "id"));
  json_object_put(o);
  if (!ok)
    return false;
  if (!quota(db, e->account_id, e->source, week, definition, &allowed))
    return false;
  if (allowed != c->allowed)
    return false;
  if (d->account_claims)
  {
    if (!allowed ||
        !sql(db, "INSERT INTO clan_objective_claims VALUES(%" PRId64 ",%" PRId64 ",%d,'%s','%s')",
             e->account_id, week, e->source, definition, e->event_uid))
      return false;
    if (e->source == CLAN_SOURCE_BOUNTY && !shared_bounty(db, e, definition, week, c->now))
      return false;
  }
  return sql(db, "UPDATE clan_objective_contracts SET state=2 WHERE contract_uid='%s' AND state=1",
             e->event_uid) &&
         mysql_affected_rows(db) == 1;
}
enum clan_repository_result clan_objective_reward_complete(MYSQL *db, const char *uid, int64_t now,
                                                           struct json_object **out)
{
  struct json_object *receipt = NULL, *event;
  struct completion_context c = {0};
  enum clan_repository_result r;
  if (!clan_progression_uid_valid(uid) || !out || now < 0)
    return CLAN_REPO_INVALID;
  if (!clan_objective_schema_ready(db))
    return CLAN_REPO_SCHEMA;
  r = clan_reward_load(db, uid, &receipt);
  if (r != CLAN_REPO_OK)
    return r;
  event = get(receipt, "event");
  if (!family_valid(num(event, "source")))
  {
    json_object_put(receipt);
    return CLAN_REPO_INVALID;
  }
  if (num(receipt, "status") == 1)
  {
    *out = receipt;
    return CLAN_REPO_OK;
  }
  json_object_put(receipt);
  if (!sql(db, "SELECT " COLUMNS " FROM clan_objective_contracts WHERE contract_uid='%s'", uid))
    return CLAN_REPO_UNAVAILABLE;
  c.contract = row_contract(db);
  if (!c.contract)
    return CLAN_REPO_NOT_FOUND;
  c.now = now;
  if (!quota(db, num(c.contract, "account_id"), num(c.contract, "family"),
             num(c.contract, "week_start"), str(get(c.contract, "definition"), "id"), &c.allowed))
  {
    json_object_put(c.contract);
    return CLAN_REPO_UNAVAILABLE;
  }
  r = clan_reward_complete_objective(db, uid, now, decide, finish_contract, &c, out);
  json_object_put(c.contract);
  return r;
}

/* Keep independent unlock/difficulty cohorts viable during rotation. */
void clan_objective_catalog_rotate(struct clan_objective_catalog *c, int64_t now, int level)
{
  struct clan_objective_catalog original = *c;
  size_t i, j;
  c->count = 0;
  for (i = 0; i < original.count; i++)
  {
    const char *ids[32];
    size_t count = 0;
    const struct clan_objective_definition *d = &original.entries[i];
    for (j = 0; j < original.count; j++)
    {
      const struct clan_objective_definition *other = &original.entries[j];
      bool eligible = level >= d->min_player_level && level <= d->max_player_level;
      bool other_eligible = level >= other->min_player_level && level <= other->max_player_level;
      if (d->family == other->family && d->min_clan_level == other->min_clan_level &&
          eligible == other_eligible)
        ids[count++] = other->id;
    }
    if (clan_contract_rotates_in(d->id, ids, count, now))
      c->entries[c->count++] = *d;
  }
}
