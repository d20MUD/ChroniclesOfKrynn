#include "clan_quartermaster.h"
#include "clan_pacing.h"
#include "clan_awards.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <inttypes.h>
static struct json_object *get(struct json_object *o, const char *key)
{
  struct json_object *v = NULL;
  if (o)
    json_object_object_get_ex(o, key, &v);
  return v;
}
static int64_t num(struct json_object *o, const char *k)
{
  return json_object_get_int64(get(o, k));
}
static const char *str(struct json_object *o, const char *k)
{
  const char *s = json_object_get_string(get(o, k));
  return s ? s : "";
}
static bool text_field(struct json_object *o, const char *k, char *out, size_t cap, bool slug)
{
  const unsigned char *s = (const unsigned char *)str(o, k);
  size_t n = strlen((const char *)s), i;
  if (json_object_get_type(get(o, k)) != json_type_string || !n || n >= cap ||
      n != (size_t)json_object_get_string_len(get(o, k)))
    return false;
  for (i = 0; i < n; i++)
    if (s[i] < 32 || s[i] > 126 ||
        (slug && !((s[i] >= 'a' && s[i] <= 'z') || (s[i] >= '0' && s[i] <= '9') || s[i] == '-')))
      return false;
  memcpy(out, s, n + 1);
  return true;
}
bool clan_qm_item_parse(struct json_object *o, struct clan_qm_item *out)
{
  struct clan_qm_item d = {0};
  size_t i;
  struct json_object *a;
  const char *keys[] = {"prototype", "kind",  "clan_level", "favor", "level",
                        "price",     "value", "type",       "weight"};
  int *fields[] = {&d.prototype, &d.kind,  &d.clan_level, &d.favor, &d.level,
                   &d.price,     &d.value, &d.type,       &d.weight};
  if (!out || !o || json_object_get_type(o) != json_type_object ||
      json_object_object_length(o) != 14 || !text_field(o, "id", d.id, sizeof(d.id), true) ||
      !text_field(o, "name", d.name, sizeof(d.name), false) ||
      !text_field(o, "coverage", d.coverage, sizeof(d.coverage), false) ||
      !text_field(o, "budget", d.budget, sizeof(d.budget), false))
    return false;
  for (i = 0; i < 9; i++)
  {
    int64_t v = num(o, keys[i]);
    if (json_object_get_type(get(o, keys[i])) != json_type_int || v < 0 || v > 100000000)
      return false;
    *fields[i] = (int)v;
  }
  a = get(o, "values");
  if (json_object_get_type(a) != json_type_array || json_object_array_length(a) != 16)
    return false;
  for (i = 0; i < 16; i++)
  {
    struct json_object *v = json_object_array_get_idx(a, i);
    int64_t n = json_object_get_int64(v);
    if (json_object_get_type(v) != json_type_int || n < 0 || n > 100000)
      return false;
    d.values[i] = (int)n;
  }
  if (d.prototype < 1 || d.kind < 1 || d.kind > 3 || d.level < 1 || d.level > 30 || d.value < 1 ||
      d.price != d.value * d.kind || d.weight < 0 || d.weight > 100 ||
      !(d.type == 5 || d.type == 9 || d.type == 16) ||
      (d.kind == 1 && (d.clan_level != 2 || d.favor != 229 || d.type != 16)) ||
      (d.kind == 2 && (d.clan_level != 7 || d.favor != 876)) ||
      (d.kind == 3 && (d.clan_level != 8 || d.favor != 1596)) || d.values[4] != 0)
    return false;
  *out = d;
  return true;
}
int clan_qm_favor_requirement(const struct clan_qm_item *d)
{
  return d ? CLAN_FAVOR_THRESHOLD(d->favor) : 0;
}
struct json_object *clan_qm_terms(const struct clan_qm_item *d)
{
  struct json_object *o = json_object_new_object(), *a = json_object_new_array();
  size_t i;
#define S(k) json_object_object_add(o, #k, json_object_new_string(d->k))
  S(id);
  S(name);
  S(coverage);
  S(budget);
#undef S
#define N(k) json_object_object_add(o, #k, json_object_new_int(d->k))
  N(prototype);
  N(kind);
  N(clan_level);
  N(favor);
  N(level);
  N(price);
  N(value);
  N(type);
  N(weight);
#undef N
  for (i = 0; i < 16; i++)
    json_object_array_add(a, json_object_new_int(d->values[i]));
  json_object_object_add(o, "values", a);
  return o;
}
bool clan_qm_parse(struct json_object *o, struct clan_qm_catalog *out)
{
  struct clan_qm_catalog c = {0};
  struct json_object *a = get(o, "items");
  size_t i, j;
  if (!out || !o || json_object_get_type(o) != json_type_object ||
      json_object_object_length(o) != 2 ||
      json_object_get_type(get(o, "version")) != json_type_int || num(o, "version") != 1 ||
      json_object_get_type(a) != json_type_array || !(c.count = json_object_array_length(a)) ||
      c.count > CLAN_QM_MAX)
    return false;
  for (i = 0; i < c.count; i++)
  {
    if (!clan_qm_item_parse(json_object_array_get_idx(a, i), &c.entries[i]))
      return false;
    for (j = 0; j < i; j++)
      if (!strcmp(c.entries[i].id, c.entries[j].id) ||
          c.entries[i].prototype == c.entries[j].prototype)
        return false;
  }
  *out = c;
  return true;
}
static bool sql(MYSQL *, const char *, ...) __attribute__((format(printf, 2, 3)));
static bool sql(MYSQL *db, const char *fmt, ...)
{
  char query[140000];
  va_list ap;
  int n;
  va_start(ap, fmt);
  n = vsnprintf(query, sizeof(query), fmt, ap);
  va_end(ap);
  return db && n >= 0 && (size_t)n < sizeof(query) && !mysql_query(db, query);
}
static bool scalar(MYSQL *db, int64_t *v)
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
    *v = strtoll(row[0], &end, 10);
    ok = !*end;
  }
  mysql_free_result(r);
  return ok;
}
static bool clan_qm_schema_ready_full(MYSQL *db)
{
  int64_t n;
  return sql(db, "SELECT COUNT(*) FROM information_schema.TABLES WHERE TABLE_SCHEMA=DATABASE() AND "
                 "TABLE_NAME IN ('clan_quartermaster_schema','clan_quartermaster_operations') AND "
                 "ENGINE='InnoDB'") &&
         scalar(db, &n) && n == 2 &&
         sql(db, "SELECT COUNT(*) FROM clan_quartermaster_schema HAVING MIN(version)=1 AND "
                 "MAX(version)=1") &&
         scalar(db, &n) && n == 1 &&
         sql(db, "SELECT COUNT(*) FROM information_schema.TRIGGERS WHERE TRIGGER_SCHEMA=DATABASE() "
                 "AND TRIGGER_NAME IN('qm_terms','qm_retained')") &&
         scalar(db, &n) && n == 2 &&
         sql(db, "SELECT COUNT(*) FROM information_schema.STATISTICS WHERE TABLE_SCHEMA=DATABASE() "
                 "AND TABLE_NAME='clan_quartermaster_operations' AND INDEX_NAME='qm_active' AND "
                 "COLUMN_NAME='active_player' AND NON_UNIQUE=0") &&
         scalar(db, &n) && n == 1 &&
         sql(db, "SELECT COUNT(*) FROM information_schema.COLUMNS WHERE TABLE_SCHEMA=DATABASE() "
                 "AND TABLE_NAME='clan_quartermaster_operations' AND COLUMN_NAME='active_player' "
                 "AND IS_NULLABLE='YES' AND EXTRA LIKE '%%STORED GENERATED%%' AND "
                 "GENERATION_EXPRESSION IS NOT NULL") &&
         scalar(db, &n) && n == 1 &&
         sql(db, "SELECT COUNT(*) FROM information_schema.COLUMNS WHERE TABLE_SCHEMA=DATABASE() "
                 "AND TABLE_NAME='clan_quartermaster_operations'") &&
         scalar(db, &n) && n == 9 &&
         sql(db, "SELECT COUNT(*) FROM information_schema.STATISTICS WHERE TABLE_SCHEMA=DATABASE() "
                 "AND TABLE_NAME='clan_quartermaster_operations' AND INDEX_NAME='PRIMARY' AND "
                 "COLUMN_NAME='operation_uid' AND NON_UNIQUE=0") &&
         scalar(db, &n) && n == 1;
}
static enum clan_repository_result clan_qm_schema_ready_validate(MYSQL *db)
{
  return clan_qm_schema_ready_full(db) ? CLAN_REPO_OK : CLAN_REPO_SCHEMA;
}
bool clan_qm_schema_ready(MYSQL *db)
{
  return clan_repository_schema_extension_check(db, clan_qm_schema_ready_validate) == CLAN_REPO_OK;
}
static enum clan_repository_result end(MYSQL *db, enum clan_repository_result r)
{
  if (r != CLAN_REPO_OK)
  {
    sql(db, "ROLLBACK");
    return r;
  }
  return sql(db, "COMMIT") ? r : CLAN_REPO_UNAVAILABLE;
}
static enum clan_repository_result begin(MYSQL *db)
{
  int64_t n;
  if (!clan_qm_schema_ready(db) || clan_repository_schema_check(db) != CLAN_REPO_OK)
    return CLAN_REPO_SCHEMA;
  if (db->server_status & SERVER_STATUS_IN_TRANS)
    return CLAN_REPO_CONFLICT;
  if (!sql(db, "START TRANSACTION"))
    return CLAN_REPO_UNAVAILABLE;
  if (!sql(db, "SELECT store_mode FROM clan_store_control WHERE singleton=1 FOR UPDATE") ||
      !scalar(db, &n) || n != 2)
    return end(db, CLAN_REPO_UNAVAILABLE);
  return CLAN_REPO_OK;
}
enum clan_repository_result clan_qm_operation(MYSQL *db, int64_t player, const char *uid,
                                              struct json_object **out)
{
  MYSQL_RES *r;
  MYSQL_ROW row;
  struct json_object *o, *terms;
  struct clan_qm_item d;
  if (!out || player < 1 || (uid && !clan_progression_uid_valid(uid)))
    return CLAN_REPO_INVALID;
  if (!clan_qm_schema_ready(db))
    return CLAN_REPO_SCHEMA;
  if (!(uid ? sql(db,
                  "SELECT "
                  "operation_uid,clan_uid,player_id,account_id,created_at,terms,payload,state FROM "
                  "clan_quartermaster_operations WHERE operation_uid='%s' AND player_id=%" PRId64,
                  uid, player)
            : sql(db,
                  "SELECT "
                  "operation_uid,clan_uid,player_id,account_id,created_at,terms,payload,state FROM "
                  "clan_quartermaster_operations WHERE active_player=%" PRId64,
                  player)))
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
  terms = json_tokener_parse(row[5]);
  if (!clan_qm_item_parse(terms, &d) || !clan_progression_uid_valid(row[0]) ||
      !clan_progression_uid_valid(row[1]) || !row[6] || !strlen(row[6]) || strlen(row[6]) > 32768)
  {
    if (terms)
      json_object_put(terms);
    mysql_free_result(r);
    return CLAN_REPO_CONFLICT;
  }
  o = json_object_new_object();
  json_object_object_add(o, "terms", terms);
  json_object_object_add(o, "operation_uid", json_object_new_string(row[0]));
  json_object_object_add(o, "clan_uid", json_object_new_string(row[1]));
  json_object_object_add(o, "player", json_object_new_int64(atoll(row[2])));
  json_object_object_add(o, "account", json_object_new_int64(atoll(row[3])));
  json_object_object_add(o, "created_at", json_object_new_int64(atoll(row[4])));
  json_object_object_add(o, "payload", json_object_new_string(row[6]));
  json_object_object_add(o, "state", json_object_new_int(atoi(row[7])));
  mysql_free_result(r);
  *out = o;
  return CLAN_REPO_OK;
}
static void hex(const char *in, char *out)
{
  static const char h[] = "0123456789abcdef";
  while (*in)
  {
    unsigned char b = (unsigned char)*in++;
    *out++ = h[b >> 4];
    *out++ = h[b & 15];
  }
  *out = 0;
}
enum clan_repository_result clan_qm_prepare(MYSQL *db, const struct clan_qm_item *d,
                                            const char *uid, const char *clan, int64_t player,
                                            int64_t account, int level, int gold,
                                            const char *payload, int64_t now,
                                            const struct clan_progression_config *config)
{
  struct json_object *terms = NULL, *old = NULL;
  struct clan_qm_item checked;
  enum clan_repository_result r;
  int64_t n;
  struct clan_repository_record record;
  int clanlevel;
  struct clan_progression_readiness ready = {0};
  char termshex[4097], payloadhex[65537];
  const char *encoded;
  if (!d || !clan_progression_uid_valid(uid) || !clan_progression_uid_valid(clan) || player < 1 ||
      account < 1 || now < 0 || level < 1 || level > 30 || !payload || !strlen(payload) ||
      strlen(payload) > 32768)
    return CLAN_REPO_INVALID;
  terms = clan_qm_terms(d);
  if (!clan_qm_item_parse(terms, &checked))
  {
    json_object_put(terms);
    return CLAN_REPO_INVALID;
  }
  r = begin(db);
  if (r != CLAN_REPO_OK)
  {
    json_object_put(terms);
    return r;
  }
  r = clan_qm_operation(db, player, uid, &old);
  if (r == CLAN_REPO_OK)
  {
    bool same = !strcmp(str(old, "clan_uid"), clan) && num(old, "account") == account &&
                num(old, "created_at") == now && !strcmp(str(old, "payload"), payload) &&
                json_object_equal(get(old, "terms"), terms);
    json_object_put(old);
    json_object_put(terms);
    return end(db, same ? CLAN_REPO_OK : CLAN_REPO_CONFLICT);
  }
  if (r != CLAN_REPO_NOT_FOUND)
  {
    json_object_put(terms);
    return end(db, r);
  }
  ready.schema_ready = true;
  ready.schema_version = CLAN_PROGRESSION_SCHEMA_VERSION;
  ready.catalog_ready = true;
  if (!clan_progression_feature_available(config, CLAN_FEATURE_QUARTERMASTER, &ready, clan) ||
      level < d->level || gold < d->price)
  {
    json_object_put(terms);
    return end(db, CLAN_REPO_INELIGIBLE);
  }
  if (!sql(db,
           "SELECT COUNT(*) FROM clan_memberships m JOIN clan_progression_clans c ON "
           "c.clan_uid=m.clan_uid JOIN clan_progression_legacy l ON l.clan_uid=c.clan_uid LEFT "
           "JOIN clan_personal_favor f ON f.clan_uid=c.clan_uid AND f.player_id=m.player_id WHERE "
           "m.player_id=%" PRId64 " AND m.clan_uid='%s' AND m.rank_id BETWEEN 1 AND "
           "JSON_LENGTH(JSON_EXTRACT(l.details,'$.ranks')) AND "
           "c.record_status=1 AND COALESCE(f.favor_eighths,0)>=%d",
           player, clan, clan_qm_favor_requirement(d) * 8) ||
      !scalar(db, &n))
  {
    json_object_put(terms);
    return end(db, CLAN_REPO_UNAVAILABLE);
  }
  if (n != 1 || clan_repository_load(db, clan, &record) != CLAN_REPO_OK ||
      !clan_award_level(config, record.xp_eighths, &clanlevel) || clanlevel < d->clan_level)
  {
    json_object_put(terms);
    return end(db, CLAN_REPO_INELIGIBLE);
  }
  if (!sql(db,
           "SELECT COUNT(*) FROM clan_quartermaster_operations WHERE active_player=%" PRId64
           " OR operation_uid='%s'",
           player, uid) ||
      !scalar(db, &n))
  {
    json_object_put(terms);
    return end(db, CLAN_REPO_UNAVAILABLE);
  }
  if (n)
  {
    json_object_put(terms);
    return end(db, CLAN_REPO_CONFLICT);
  }
  encoded = json_object_to_json_string_ext(terms, JSON_C_TO_STRING_PLAIN);
  if (strlen(encoded) > 2048)
  {
    json_object_put(terms);
    return end(db, CLAN_REPO_INVALID);
  }
  hex(encoded, termshex);
  hex(payload, payloadhex);
  json_object_put(terms);
  if (!sql(db,
           "INSERT INTO "
           "clan_quartermaster_operations(operation_uid,clan_uid,player_id,account_id,created_at,"
           "terms,payload) VALUES('%s','%s',%" PRId64 ",%" PRId64 ",%" PRId64
           ",CONVERT(0x%s USING utf8mb4),CONVERT(0x%s USING utf8mb4))",
           uid, clan, player, account, now, termshex, payloadhex))
    return end(db, CLAN_REPO_UNAVAILABLE);
  return end(db, CLAN_REPO_OK);
}
enum clan_repository_result clan_qm_ack(MYSQL *db, int64_t player, const char *uid)
{
  enum clan_repository_result r;
  int64_t n;
  if (player < 1 || !clan_progression_uid_valid(uid))
    return CLAN_REPO_INVALID;
  r = begin(db);
  if (r != CLAN_REPO_OK)
    return r;
  if (!sql(db,
           "SELECT COUNT(*) FROM clan_quartermaster_operations WHERE operation_uid='%s' AND "
           "player_id=%" PRId64,
           uid, player) ||
      !scalar(db, &n))
    return end(db, CLAN_REPO_UNAVAILABLE);
  if (n != 1)
    return end(db, CLAN_REPO_NOT_FOUND);
  if (!sql(db,
           "UPDATE clan_quartermaster_operations SET state=1 WHERE operation_uid='%s' AND "
           "player_id=%" PRId64,
           uid, player))
    return end(db, CLAN_REPO_UNAVAILABLE);
  return end(db, CLAN_REPO_OK);
}
