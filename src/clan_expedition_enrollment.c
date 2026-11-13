#include "clan_expedition_enrollment.h"
#include "clan_awards.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
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
bool clan_expedition_schema_ready(MYSQL *db)
{
  int64_t n;
  return sql(db, "SELECT COUNT(*) FROM information_schema.TABLES WHERE TABLE_SCHEMA=DATABASE() AND "
                 "TABLE_NAME IN "
                 "('clan_expedition_schema','clan_expedition_outings','clan_expedition_roster') "
                 "AND ENGINE='InnoDB'") &&
         scalar(db, &n) && n == 3 &&
         sql(db, "SELECT COUNT(*) FROM clan_expedition_schema WHERE version=1") && scalar(db, &n) &&
         n == 1 &&
         sql(db, "SELECT COUNT(*) FROM information_schema.TRIGGERS WHERE TRIGGER_SCHEMA=DATABASE() "
                 "AND TRIGGER_NAME IN "
                 "('ce_outing_terms','ce_outing_retained','ce_roster_terms','ce_roster_retained','"
                 "ce_membership_delete','ce_membership_update','ce_roster_admission')") &&
         scalar(db, &n) && n == 7 &&
         sql(db, "SELECT COUNT(*) FROM (SELECT "
                 "TABLE_NAME,INDEX_NAME,NON_UNIQUE,GROUP_CONCAT(COLUMN_NAME ORDER BY SEQ_IN_INDEX) "
                 "cols FROM information_schema.STATISTICS WHERE TABLE_SCHEMA=DATABASE() AND "
                 "TABLE_NAME IN ('clan_expedition_outings','clan_expedition_roster') GROUP BY "
                 "TABLE_NAME,INDEX_NAME,NON_UNIQUE) k WHERE NON_UNIQUE=0 AND "
                 "((TABLE_NAME='clan_expedition_outings' AND ((INDEX_NAME='PRIMARY' AND "
                 "cols='outing_uid') OR (INDEX_NAME='ce_active' AND cols='active_clan'))) OR "
                 "(TABLE_NAME='clan_expedition_roster' AND ((INDEX_NAME='PRIMARY' AND "
                 "cols='outing_uid,player_id') OR (INDEX_NAME='ce_account' AND "
                 "cols='outing_uid,account_id'))))") &&
         scalar(db, &n) && n == 4;
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
static enum clan_repository_result audit_outing(MYSQL *, const char *, const char *, int64_t);
static enum clan_repository_result begin(MYSQL *db, const char *clan, int64_t now,
                                         const struct clan_progression_config *config)
{
  int64_t mode;
  struct clan_progression_readiness ready = {.schema_ready = true,
                                             .schema_version = CLAN_PROGRESSION_SCHEMA_VERSION,
                                             .contracts_ready = true,
                                             .expeditions_ready = true};
  if (!clan_progression_uid_valid(clan) || now < 0)
    return CLAN_REPO_INVALID;
  if (!clan_progression_feature_available(config, CLAN_FEATURE_EXPEDITIONS, &ready, clan))
    return CLAN_REPO_INELIGIBLE;
  if (!clan_expedition_schema_ready(db) || clan_repository_schema_check(db) != CLAN_REPO_OK)
    return CLAN_REPO_SCHEMA;
  if (db->server_status & SERVER_STATUS_IN_TRANS)
    return CLAN_REPO_CONFLICT;
  /* Commit reconciliation independently: a rejected/expired command must not
   * roll back expiry and strand the clan's active slot. */
  if (clan_expedition_progress_ready(db))
  {
    enum clan_repository_result reconciled = clan_expedition_reconcile_clan(db, clan, now);
    if (reconciled != CLAN_REPO_OK)
      return reconciled;
  }

  if (!sql(db, "START TRANSACTION"))
    return CLAN_REPO_UNAVAILABLE;
  if (!sql(db, "SELECT store_mode FROM clan_store_control WHERE singleton=1 FOR UPDATE") ||
      !scalar(db, &mode) || mode != 2)
    return end(db, CLAN_REPO_UNAVAILABLE);
  /* Immutable end time releases the one-clan slot, never extends an outing. */
  if (!sql(db,
           "UPDATE clan_expedition_outings SET state=4 WHERE active_clan='%s' AND "
           "expires_at<=%" PRId64,
           clan, now))
    return end(db, CLAN_REPO_UNAVAILABLE);
  return CLAN_REPO_OK;
}
static bool member(MYSQL *db, int64_t player, const char *clan)
{
  int64_t n;
  return player > 0 &&
         sql(db,
             "SELECT COUNT(*) FROM clan_memberships m JOIN clan_progression_legacy l ON "
             "l.clan_uid=m.clan_uid JOIN clan_progression_clans c ON c.clan_uid=m.clan_uid WHERE "
             "m.player_id=%" PRId64 " AND m.clan_uid='%s' AND c.record_status=1 AND m.rank_id "
             "BETWEEN 1 AND JSON_LENGTH(JSON_EXTRACT(l.details,'$.ranks'))",
             player, clan) &&
         scalar(db, &n) && n == 1;
}
static bool eligible(MYSQL *db, const struct clan_expedition_member *m, const char *clan,
                     struct json_object *terms)
{
  struct clan_repository_record c;
  int level;
  return m && m->player > 0 && m->account > 0 && m->level >= num(terms, "min_player_level") &&
         m->level <= num(terms, "max_player_level") && member(db, m->player, clan) &&
         clan_repository_load(db, clan, &c) == CLAN_REPO_OK && c.status == CLAN_RECORD_ACTIVE &&
         clan_award_level(&clan_progression_defaults, c.xp_eighths, &level) &&
         level >= num(terms, "min_clan_level");
}
static enum clan_repository_result load(MYSQL *db, const char *uid, const char *clan,
                                        struct json_object **out)
{
  MYSQL_RES *res;
  MYSQL_ROW row;
  struct json_object *o;
  if (!sql(db,
           "SELECT "
           "outing_uid,clan_uid,leader_id,terms,created_at,expires_at,week_start,state,roster_"
           "locked,started_at FROM clan_expedition_outings WHERE outing_uid='%s' AND clan_uid='%s'",
           uid, clan))
    return CLAN_REPO_UNAVAILABLE;
  res = mysql_store_result(db);
  if (!res)
    return CLAN_REPO_UNAVAILABLE;
  row = mysql_fetch_row(res);
  if (!row)
  {
    mysql_free_result(res);
    return CLAN_REPO_NOT_FOUND;
  }
  o = json_object_new_object();
  {
    const char *keys[] = {"outing_uid", "clan_uid",   "leader_id", "definition",    "created_at",
                          "expires_at", "week_start", "state",     "roster_locked", "started_at"};
    int i;
    for (i = 0; i < 10; i++)
      json_object_object_add(o, keys[i],
                             i < 2    ? json_object_new_string(row[i])
                             : i == 3 ? json_tokener_parse(row[i])
                                      : json_object_new_int64(atoll(row[i])));
  }
  mysql_free_result(res);
  if (!sql(db,
           "SELECT player_id,account_id,enrolled_at,inactive FROM clan_expedition_roster WHERE "
           "outing_uid='%s' ORDER BY player_id LIMIT 33",
           uid))
  {
    json_object_put(o);
    return CLAN_REPO_UNAVAILABLE;
  }
  res = mysql_store_result(db);
  if (!res)
  {
    json_object_put(o);
    return CLAN_REPO_UNAVAILABLE;
  }
  {
    struct json_object *a = json_object_new_array();
    json_object_object_add(o, "roster", a);
    while ((row = mysql_fetch_row(res)))
    {
      struct json_object *v = json_object_new_object();
      int i;
      const char *keys[] = {"player", "account", "enrolled_at", "inactive"};
      for (i = 0; i < 4; i++)
        json_object_object_add(v, keys[i], json_object_new_int64(atoll(row[i])));
      json_object_array_add(a, v);
    }
    mysql_free_result(res);
    if (json_object_array_length(a) > 32)
    {
      json_object_put(o);
      return CLAN_REPO_SCHEMA;
    }
  }
  *out = o;
  return CLAN_REPO_OK;
}
enum clan_repository_result clan_expedition_read(MYSQL *db, const char *clan,
                                                 struct json_object **out)
{
  MYSQL_RES *res;
  MYSQL_ROW row;
  char uid[33];
  if (!clan_progression_uid_valid(clan) || !out)
    return CLAN_REPO_INVALID;
  if (!clan_expedition_schema_ready(db))
    return CLAN_REPO_SCHEMA;
  if (!sql(db, "SELECT outing_uid FROM clan_expedition_outings WHERE active_clan='%s'", clan))
    return CLAN_REPO_UNAVAILABLE;
  res = mysql_store_result(db);
  if (!res)
    return CLAN_REPO_UNAVAILABLE;
  row = mysql_fetch_row(res);
  if (!row)
  {
    mysql_free_result(res);
    return CLAN_REPO_NOT_FOUND;
  }
  snprintf(uid, sizeof(uid), "%s", row[0]);
  mysql_free_result(res);
  return load(db, uid, clan, out);
}
static bool insert_member(MYSQL *db, const char *uid, const struct clan_expedition_member *m,
                          int64_t now)
{
  return sql(db,
             "INSERT INTO clan_expedition_roster(outing_uid,player_id,account_id,enrolled_at) "
             "VALUES('%s',%" PRId64 ",%" PRId64 ",%" PRId64 ")",
             uid, m->player, m->account, now);
}
enum clan_repository_result
clan_expedition_create(MYSQL *db, const struct clan_expedition_catalog *catalog,
                       const struct clan_expedition_world *world, const char *definition,
                       const char *uid, const char *clan, const struct clan_expedition_member *m,
                       int64_t now, const struct clan_progression_config *config)
{
  struct clan_reset_period week;
  struct clan_expedition_catalog selected = {0};
  struct json_object *terms;
  char encoded[16384];
  size_t i;
  int64_t expires;
  enum clan_repository_result r;
  if (!slug(definition) || !clan_progression_uid_valid(uid) ||
      !clan_progression_week_at(now, &week) || !m || !catalog || !catalog->count ||
      catalog->count > CLAN_EXPEDITION_MAX)
    return CLAN_REPO_INVALID;
  for (i = 0; i < catalog->count; i++)
    if (!strcmp(catalog->entries[i].id, definition))
      break;
  if (i == catalog->count)
    return CLAN_REPO_NOT_FOUND;
  selected.count = 1;
  selected.entries[0] = catalog->entries[i];
  if (!clan_expedition_catalog_validate(&selected, world, NULL, 0))
    return CLAN_REPO_INELIGIBLE;
  terms = clan_expedition_definition_json(&catalog->entries[i]);
  if (!terms)
    return CLAN_REPO_INVALID;
  if (!encode(terms, encoded, sizeof(encoded)))
  {
    json_object_put(terms);
    return CLAN_REPO_INVALID;
  }
  r = begin(db, clan, now, config);
  if (r != CLAN_REPO_OK)
  {
    json_object_put(terms);
    return r;
  }
  if (!eligible(db, m, clan, terms))
  {
    json_object_put(terms);
    return end(db, CLAN_REPO_INELIGIBLE);
  }
  json_object_put(terms);
  /* Replay a lost acknowledgement with the SAME stable ID, never allocate again. */
  {
    struct json_object *old = NULL;
    r = load(db, uid, clan, &old);
    if (r == CLAN_REPO_OK)
    {
      struct json_object *roster = get(old, "roster");
      bool enrolled = false;
      size_t j;
      for (j = 0; j < json_object_array_length(roster); j++)
      {
        struct json_object *v = json_object_array_get_idx(roster, j);
        if (num(v, "player") == m->player && num(v, "account") == m->account)
          enrolled = true;
      }
      char previous[16384];
      bool same = encode(get(old, "definition"), previous, sizeof(previous)) &&
                  !strcmp(previous, encoded) && num(old, "leader_id") == m->player &&
                  num(old, "created_at") == now &&
                  !strcmp(str(get(old, "definition"), "id"), definition) && enrolled;
      json_object_put(old);
      return end(db, same ? CLAN_REPO_OK : CLAN_REPO_CONFLICT);
    }
    if (r != CLAN_REPO_NOT_FOUND)
      return end(db, r);
  }
  expires = week.ends_at;
  if (!sql(db,
           "INSERT INTO "
           "clan_expedition_outings(outing_uid,clan_uid,leader_id,terms,created_at,expires_at,week_"
           "start) VALUES('%s','%s',%" PRId64 ",_utf8mb4 X'%s',%" PRId64 ",%" PRId64 ",%" PRId64
           ")",
           uid, clan, m->player, encoded, now, expires, week.starts_at))
    return end(db, CLAN_REPO_CONFLICT);
  if (!insert_member(db, uid, m, now))
    return end(db, CLAN_REPO_UNAVAILABLE);
  return end(db, CLAN_REPO_OK);
}
static enum clan_repository_result open_outing(MYSQL *db, const char *uid, const char *clan,
                                               int64_t now,
                                               const struct clan_progression_config *config,
                                               struct json_object **o)
{
  enum clan_repository_result r;
  if (!clan_progression_uid_valid(uid))
    return CLAN_REPO_INVALID;
  r = begin(db, clan, now, config);
  if (r != CLAN_REPO_OK)
    return r;
  r = load(db, uid, clan, o);
  if (r != CLAN_REPO_OK)
    return end(db, r);
  if (num(*o, "state") > 1 || now >= num(*o, "expires_at"))
  {
    json_object_put(*o);
    *o = NULL;
    return end(db, CLAN_REPO_INELIGIBLE);
  }
  return CLAN_REPO_OK;
}
enum clan_repository_result clan_expedition_enroll(MYSQL *db, const char *uid, const char *clan,
                                                   const struct clan_expedition_member *m,
                                                   int64_t now,
                                                   const struct clan_progression_config *config)
{
  struct json_object *o = NULL, *a;
  size_t i, n = 0;
  enum clan_repository_result r;
  if (!m)
    return CLAN_REPO_INVALID;
  r = open_outing(db, uid, clan, now, config, &o);
  if (r != CLAN_REPO_OK)
    return r;
  if (now < num(o, "created_at") || !eligible(db, m, clan, get(o, "definition")))
    r = CLAN_REPO_INELIGIBLE;
  else
  {
    a = get(o, "roster");
    for (i = 0; i < json_object_array_length(a); i++)
    {
      struct json_object *v = json_object_array_get_idx(a, i);
      if (num(v, "player") == m->player || num(v, "account") == m->account)
      {
        bool same =
            num(v, "player") == m->player && num(v, "account") == m->account && !num(v, "inactive");
        json_object_put(o);
        return end(db, same ? CLAN_REPO_OK : CLAN_REPO_CONFLICT);
      }
      if (!num(v, "inactive"))
        n++;
    }
    if (num(o, "roster_locked"))
      r = CLAN_REPO_CONFLICT;
    else if (n >= CLAN_EXPEDITION_ROSTER_MAX || json_object_array_length(a) >= 32)
      r = CLAN_REPO_CAPACITY;
    else
      r = insert_member(db, uid, m, now) ? CLAN_REPO_OK : CLAN_REPO_CONFLICT;
  }
  json_object_put(o);
  return end(db, r);
}
static bool roster_matches(MYSQL *db, struct json_object *o,
                           const struct clan_expedition_member *members, size_t count)
{
  struct json_object *a = get(o, "roster");
  size_t i, j, n = 0;
  if (!members || count < 2 || count > 6)
    return false;
  for (i = 0; i < count; i++)
    for (j = 0; j < i; j++)
      if (members[i].player == members[j].player || members[i].account == members[j].account)
        return false;
  for (i = 0; i < json_object_array_length(a); i++)
  {
    struct json_object *v = json_object_array_get_idx(a, i);
    if (num(v, "inactive"))
      continue;
    n++;
    for (j = 0; j < count; j++)
      if (members[j].player == num(v, "player") && members[j].account == num(v, "account"))
        break;
    if (j == count || !eligible(db, &members[j], str(o, "clan_uid"), get(o, "definition")))
      return false;
  }
  return n == count;
}
static enum clan_repository_result
start_or_seal(MYSQL *db, const char *uid, const char *clan, int64_t actor,
              const struct clan_expedition_member *members, size_t count, int64_t now,
              const struct clan_progression_config *config, bool seal)
{
  struct json_object *o = NULL;
  enum clan_repository_result r = open_outing(db, uid, clan, now, config, &o);
  if (r != CLAN_REPO_OK)
    return r;
  bool actor_present = seal;
  size_t i;
  if (members && count <= CLAN_EXPEDITION_ROSTER_MAX)
    for (i = 0; i < count; i++)
      if (members[i].player == actor)
        actor_present = true;
  if (now < num(o, "created_at") || (!seal && (actor != num(o, "leader_id") || !actor_present)) ||
      (seal && num(o, "state") != 1) || !roster_matches(db, o, members, count))
    r = CLAN_REPO_INELIGIBLE;
  else if (seal)
  {
    if (!sql(db, "UPDATE clan_expedition_outings SET roster_locked=1 WHERE outing_uid='%s'", uid))
      r = CLAN_REPO_UNAVAILABLE;
  }
  else if (num(o, "state") == 0)
  {
    int64_t expiry = num(o, "expires_at");
    if (expiry - now > 7200)
      expiry = now + 7200;
    if (!sql(db,
             "UPDATE clan_expedition_outings SET state=1,started_at=%" PRId64 ",expires_at=%" PRId64
             " WHERE outing_uid='%s'",
             now, expiry, uid))
      r = CLAN_REPO_UNAVAILABLE;
  }
  json_object_put(o);
  return end(db, r);
}
enum clan_repository_result clan_expedition_start(MYSQL *db, const char *uid, const char *clan,
                                                  int64_t actor,
                                                  const struct clan_expedition_member *members,
                                                  size_t count, int64_t now,
                                                  const struct clan_progression_config *config)
{
  return start_or_seal(db, uid, clan, actor, members, count, now, config, false);
}
enum clan_repository_result clan_expedition_seal(MYSQL *db, const char *uid, const char *clan,
                                                 const struct clan_expedition_member *members,
                                                 size_t count, int64_t now,
                                                 const struct clan_progression_config *config)
{
  return start_or_seal(db, uid, clan, 0, members, count, now, config, true);
}
enum clan_repository_result clan_expedition_leave(MYSQL *db, const char *uid, const char *clan,
                                                  const struct clan_expedition_member *m,
                                                  bool abandon, int64_t now,
                                                  const struct clan_progression_config *config)
{
  struct json_object *o = NULL, *a;
  size_t i;
  bool found = false;
  enum clan_repository_result r;
  if (!m)
    return CLAN_REPO_INVALID;
  r = open_outing(db, uid, clan, now, config, &o);
  if (r != CLAN_REPO_OK)
    return r;
  a = get(o, "roster");
  for (i = 0; i < json_object_array_length(a); i++)
  {
    struct json_object *v = json_object_array_get_idx(a, i);
    if (num(v, "player") == m->player && num(v, "account") == m->account && !num(v, "inactive"))
      found = true;
  }
  if (!found || !member(db, m->player, clan) || now < num(o, "created_at"))
    r = CLAN_REPO_INELIGIBLE;
  else if (!abandon && num(o, "roster_locked"))
    r = CLAN_REPO_CONFLICT;
  else if (!sql(db,
                abandon ? "UPDATE clan_expedition_outings SET state=3 WHERE outing_uid='%s'"
                        : "UPDATE clan_expedition_roster SET inactive=1 WHERE outing_uid='%s' AND "
                          "player_id=%" PRId64,
                uid, m->player))
    r = CLAN_REPO_UNAVAILABLE;
  json_object_put(o);
  return end(db, r);
}

#include <openssl/sha.h>
#ifdef CLAN_REWARD_TEST
#include <unistd.h>
int clan_expedition_crash;
#endif
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
bool clan_expedition_progress_ready(MYSQL *db)
{
  int64_t n;
  return clan_expedition_schema_ready(db) &&
         sql(db, "SELECT COUNT(*) FROM clan_expedition_schema WHERE version=2") && scalar(db, &n) &&
         n == 1 &&
         sql(db, "SELECT COUNT(*) FROM information_schema.TABLES WHERE TABLE_SCHEMA=DATABASE() AND "
                 "TABLE_NAME IN "
                 "('clan_expedition_events','clan_expedition_completions','clan_expedition_"
                 "entitlements','clan_expedition_shared_claims','clan_expedition_personal_claims') "
                 "AND ENGINE='InnoDB'") &&
         scalar(db, &n) && n == 5 &&
         sql(db,
             "SELECT COUNT(*) FROM information_schema.TRIGGERS WHERE TRIGGER_SCHEMA=DATABASE() AND "
             "TRIGGER_NAME IN "
             "('ce_event_retained','ce_event_immutable','ce_completion_retained','ce_completion_"
             "immutable','ce_entitlement_retained','ce_entitlement_terms','ce_shared_retained','ce_"
             "shared_immutable','ce_personal_retained','ce_personal_immutable')") &&
         scalar(db, &n) && n == 10 &&
         sql(db, "SELECT COUNT(*) FROM (SELECT "
                 "TABLE_NAME,INDEX_NAME,NON_UNIQUE,GROUP_CONCAT(COLUMN_NAME ORDER BY SEQ_IN_INDEX) "
                 "cols FROM information_schema.STATISTICS WHERE TABLE_SCHEMA=DATABASE() GROUP BY "
                 "TABLE_NAME,INDEX_NAME,NON_UNIQUE) k WHERE NON_UNIQUE=0 AND "
                 "((TABLE_NAME='clan_expedition_events' AND ((INDEX_NAME='PRIMARY' AND "
                 "cols='outing_uid,event_uid') OR (INDEX_NAME='ce_encounter' AND "
                 "cols='clan_uid,event_uid'))) OR (TABLE_NAME='clan_expedition_completions' AND "
                 "INDEX_NAME='PRIMARY' AND cols='outing_uid') OR "
                 "(TABLE_NAME='clan_expedition_entitlements' AND ((INDEX_NAME='PRIMARY' AND "
                 "cols='outing_uid,player_id') OR (INDEX_NAME='ce_entitlement_operation' AND "
                 "cols='operation_uid'))) OR (TABLE_NAME='clan_expedition_shared_claims' AND "
                 "((INDEX_NAME='PRIMARY' AND cols='clan_uid,week_start,family') OR "
                 "(INDEX_NAME='ce_shared_operation' AND cols='operation_uid'))) OR "
                 "(TABLE_NAME='clan_expedition_personal_claims' AND ((INDEX_NAME='PRIMARY' AND "
                 "cols='account_id,week_start,family') OR (INDEX_NAME='ce_personal_operation' AND "
                 "cols='operation_uid'))))") &&
         scalar(db, &n) && n == 9;
}
static void expedition_uid(char out[33], const char *kind, const char *outing, int64_t player)
{
  char text[128];
  unsigned char digest[SHA256_DIGEST_LENGTH];
  size_t i;
  int n = snprintf(text, sizeof(text), "expedition-v1:%s:%s:%" PRId64, kind, outing, player);
  SHA256((const unsigned char *)text, (size_t)n, digest);
  for (i = 0; i < 16; i++)
    snprintf(out + 2 * i, 3, "%02x", digest[i]);
}
static bool progress_counts(MYSQL *db, const char *uid, int counts[3])
{
  MYSQL_RES *res;
  MYSQL_ROW row;
  counts[0] = counts[1] = counts[2] = 0;
  if (!sql(db,
           "SELECT objective,COUNT(*) FROM clan_expedition_events WHERE outing_uid='%s' GROUP BY "
           "objective",
           uid))
    return false;
  res = mysql_store_result(db);
  if (!res)
    return false;
  while ((row = mysql_fetch_row(res)))
  {
    int i = atoi(row[0]), n = atoi(row[1]);
    if (i < 0 || i > 2 || n < 0 || n > 20)
    {
      mysql_free_result(res);
      return false;
    }
    counts[i] = n;
  }
  mysql_free_result(res);
  return true;
}
enum clan_repository_result clan_expedition_progress_read(MYSQL *db, const char *uid,
                                                          struct json_object **out)
{
  int counts[3], i;
  struct json_object *a;
  if (!out || !clan_progression_uid_valid(uid))
    return CLAN_REPO_INVALID;
  if (!clan_expedition_progress_ready(db))
    return CLAN_REPO_SCHEMA;
  if (!progress_counts(db, uid, counts))
    return CLAN_REPO_UNAVAILABLE;
  a = json_object_new_array();
  for (i = 0; i < 3; i++)
    json_object_array_add(a, json_object_new_int(counts[i]));
  *out = a;
  return CLAN_REPO_OK;
}
static bool finish_outing(MYSQL *db, struct json_object *o,
                          const struct clan_expedition_definition *d,
                          struct json_object *participants, int total, int64_t at, int64_t now)
{
  const char *uid = str(o, "outing_uid"), *clan = str(o, "clan_uid");
  struct clan_reset_period week;
  struct json_object *a = get(o, "roster");
  size_t i, j;
  int eligible_count = 0;
  bool eligible[32] = {false};
  int64_t n;
  if (!clan_progression_week_at(at, &week))
    return false;
  for (i = 0; i < json_object_array_length(a); i++)
  {
    struct json_object *v = json_object_array_get_idx(a, i);
    bool final = false;
    for (j = 0; j < json_object_array_length(participants); j++)
      if (json_object_get_int64(json_object_array_get_idx(participants, j)) == num(v, "player"))
        final = true;
    if (!final || num(v, "inactive") || !member(db, num(v, "player"), clan))
      continue;
    if (!sql(db,
             "SELECT COUNT(*) FROM clan_expedition_events WHERE outing_uid='%s' AND "
             "JSON_CONTAINS(participants,'%" PRId64 "')",
             uid, num(v, "player")) ||
        !scalar(db, &n))
      return false;
    if (n >= (total + 1) / 2)
    {
      eligible[i] = true;
      eligible_count++;
    }
  }
  if (eligible_count < 2)
    return false;
  if (!sql(db,
           "INSERT INTO clan_expedition_completions VALUES('%s',%d,%" PRId64 ",%" PRId64 ",%" PRId64
           ",%d)",
           uid, d->family, week.starts_at, at, week.ends_at, total))
    return false;
  for (i = 0; i < json_object_array_length(a); i++)
    if (eligible[i])
    {
      struct json_object *v = json_object_array_get_idx(a, i);
      char operation[33];
      expedition_uid(operation, "personal", uid, num(v, "player"));
      if (!sql(db,
               "INSERT INTO "
               "clan_expedition_entitlements(outing_uid,player_id,account_id,operation_uid) "
               "VALUES('%s',%" PRId64 ",%" PRId64 ",'%s')",
               uid, num(v, "player"), num(v, "account"), operation))
        return false;
    }
  if (!sql(db,
           "SELECT COUNT(*) FROM clan_expedition_shared_claims WHERE clan_uid='%s' AND "
           "week_start=%" PRId64 " AND family=%d",
           clan, week.starts_at, d->family) ||
      !scalar(db, &n))
    return false;
  if (!n)
  {
    struct clan_reward_event e = {0};
    expedition_uid(e.operation_uid, "shared", uid, 0);
    strcpy(e.event_uid, uid);
    strcpy(e.clan_uid, clan);
    e.source = d->family == 1 ? CLAN_SOURCE_ASSAULT_SHARED : CLAN_SOURCE_HUNT_SHARED;
    e.occurred_at = at;
    e.rules_version = CLAN_PROGRESSION_RULES_VERSION;
    e.requested_xp_eighths = 4800;
    e.requested_influence_eighths = 2400;
    if (!reward_insert(db, &e, now) ||
        !sql(db, "INSERT INTO clan_expedition_shared_claims VALUES('%s',%" PRId64 ",%d,'%s')", clan,
             week.starts_at, d->family, e.operation_uid))
      return false;
  }
  return sql(db, "UPDATE clan_expedition_outings SET state=3 WHERE outing_uid='%s' AND state=1",
             uid);
}
static enum clan_repository_result combat_one(MYSQL *db, const struct clan_combat_encounter *e,
                                              const struct clan_combat_result *results,
                                              const char *clan, int area, int64_t now,
                                              const struct clan_progression_config *config)
{
  enum clan_repository_result r;
  struct json_object *o = NULL, *roster, *facts, *participants;
  struct clan_expedition_catalog parsed;
  struct clan_expedition_definition *d;
  struct json_object *wrap, *defs;
  MYSQL_RES *res;
  MYSQL_ROW row;
  char uid[33] = "", encoded[8192], facts_hex[8192];
  char *old_facts = NULL;
  bool replay = false, same = false;
  int objective = -1, counts[3], total = 0, done = 0;
  size_t i, j, k;
  r = begin(db, clan, now, config);
  if (r != CLAN_REPO_OK)
    return r;
  if (!sql(db,
           "SELECT outing_uid,occurred_at,area,target,room,facts FROM clan_expedition_events WHERE "
           "clan_uid='%s' AND event_uid='%s'",
           clan, e->encounter_uid))
    return end(db, CLAN_REPO_UNAVAILABLE);
  res = mysql_store_result(db);
  if (!res)
    return end(db, CLAN_REPO_UNAVAILABLE);
  row = mysql_fetch_row(res);
  if (row)
  {
    replay = true;
    snprintf(uid, 33, "%s", row[0]);
    same = atoll(row[1]) == e->occurred_at && atoi(row[2]) == area &&
           atoll(row[3]) == e->victim_prototype && atoll(row[4]) == e->room;
    old_facts = malloc(strlen(row[5]) + 1);
    if (old_facts)
      strcpy(old_facts, row[5]);
  }
  mysql_free_result(res);
  if (replay && !old_facts)
    return end(db, CLAN_REPO_UNAVAILABLE);
  r = replay ? load(db, uid, clan, &o) : clan_expedition_read(db, clan, &o);
  if (r != CLAN_REPO_OK)
  {
    free(old_facts);
    return end(db, r);
  }
  snprintf(uid, 33, "%s", str(o, "outing_uid"));
  roster = get(o, "roster");
  facts = json_object_new_array();
  participants = json_object_new_array();
  for (i = 0; i < json_object_array_length(roster); i++)
  {
    struct json_object *v = json_object_array_get_idx(roster, i);
    for (j = 0; j < e->actor_count; j++)
      if (!results[j].reasons && results[j].player_id == num(v, "player") &&
          results[j].account_id == num(v, "account") && !strcmp(results[j].clan_uid, clan))
        break;
    if (j == e->actor_count)
      continue;
    {
      struct json_object *f = json_object_new_array();
      json_object_array_add(f, json_object_new_int64(num(v, "player")));
      json_object_array_add(f, json_object_new_int64(num(v, "account")));
      json_object_array_add(f, json_object_new_int(e->actors[j].level));
      json_object_array_add(facts, f);
    }
    if (!num(v, "inactive") && member(db, num(v, "player"), clan))
      json_object_array_add(participants, json_object_new_int64(num(v, "player")));
  }
  if (replay)
  {
    struct json_object *old = json_tokener_parse(old_facts);
    same = same && old && json_object_equal(old, facts);
    if (old)
      json_object_put(old);
    free(old_facts);
    r = same ? CLAN_REPO_OK : CLAN_REPO_CONFLICT;
    goto cleanup;
  }
  if (num(o, "state") != 1 || e->occurred_at < num(o, "started_at") ||
      e->occurred_at >= num(o, "expires_at") || json_object_array_length(participants) < 2)
  {
    r = CLAN_REPO_INELIGIBLE;
    goto cleanup;
  }
  wrap = json_object_new_object();
  defs = json_object_new_array();
  json_object_array_add(defs, json_object_get(get(o, "definition")));
  json_object_object_add(wrap, "version", json_object_new_int(1));
  json_object_object_add(wrap, "expeditions", defs);
  same = clan_expedition_catalog_parse(wrap, &parsed, NULL, 0);
  json_object_put(wrap);
  if (!same || !progress_counts(db, uid, counts))
  {
    r = CLAN_REPO_SCHEMA;
    goto cleanup;
  }
  d = &parsed.entries[0];
  /* Every credited participant must still satisfy the frozen difficulty band. */
  for (i = 0; i < json_object_array_length(facts); i++)
  {
    int level =
        json_object_get_int(json_object_array_get_idx(json_object_array_get_idx(facts, i), 2));
    if (level < d->min_player_level || level > d->max_player_level)
    {
      r = CLAN_REPO_INELIGIBLE;
      goto cleanup;
    }
  }
  for (i = 0; i < 3; i++)
  {
    total += d->objectives[i].quota;
    done += counts[i];
    if (counts[i] >= d->objectives[i].quota)
      continue;
    for (k = 0; k < d->objectives[i].count; k++)
      if (d->objectives[i].targets[k].area == area &&
          d->objectives[i].targets[k].mob == e->victim_prototype)
        objective = (int)i;
  }
  if (objective < 0)
  {
    r = CLAN_REPO_NOT_FOUND;
    goto cleanup;
  }
  if (!encode(participants, encoded, sizeof(encoded)) ||
      !encode(facts, facts_hex, sizeof(facts_hex)) ||
      !sql(db,
           "INSERT INTO clan_expedition_events VALUES('%s','%s','%s',%" PRId64 ",%d,%" PRId64
           ",%d,_utf8mb4 X'%s',_utf8mb4 X'%s',%" PRId64 ")",
           uid, clan, e->encounter_uid, e->occurred_at, area, e->victim_prototype, objective,
           encoded, facts_hex, e->room) ||
      !sql(db, "UPDATE clan_expedition_outings SET roster_locked=1 WHERE outing_uid='%s'", uid))
  {
    r = CLAN_REPO_UNAVAILABLE;
    goto cleanup;
  }
  if (done + 1 == total && !finish_outing(db, o, d, participants, total, e->occurred_at, now))
  {
    r = CLAN_REPO_INELIGIBLE;
    goto cleanup;
  }
#ifdef CLAN_REWARD_TEST
  if (clan_expedition_crash == 1)
    _exit(93);
#endif
  r = CLAN_REPO_OK;
cleanup:
  json_object_put(facts);
  json_object_put(participants);
  json_object_put(o);
  r = end(db, r);
#ifdef CLAN_REWARD_TEST
  if (r == CLAN_REPO_OK && clan_expedition_crash == 2)
    _exit(94);
#endif
  return r;
}
enum clan_repository_result clan_expedition_combat(MYSQL *db, const struct clan_combat_encounter *e,
                                                   int area, int64_t now,
                                                   const struct clan_progression_config *config)
{
  struct clan_combat_result results[CLAN_COMBAT_MAX_ACTORS];
  size_t i, j;
  enum clan_repository_result result = CLAN_REPO_NOT_FOUND, r;
  if (!e || area < 0 || now < e->occurred_at ||
      !clan_combat_query(config, e, results, CLAN_COMBAT_MAX_ACTORS))
    return CLAN_REPO_INVALID;
  if (!clan_expedition_progress_ready(db))
    return CLAN_REPO_SCHEMA;
  for (i = 0; i < e->actor_count; i++)
    if (!results[i].reasons)
    {
      for (j = 0; j < i; j++)
        if (!results[j].reasons && !strcmp(results[j].clan_uid, results[i].clan_uid))
          break;
      if (j < i)
        continue;
      r = combat_one(db, e, results, results[i].clan_uid, area, now, config);
      if (r == CLAN_REPO_OK)
        result = r;
      else if (result != CLAN_REPO_OK && r != CLAN_REPO_NOT_FOUND)
        result = r;
    }
  return result;
}
enum clan_repository_result clan_expedition_claim_list(MYSQL *db, const char *clan, int64_t player,
                                                       int64_t account, int64_t now,
                                                       struct json_object **out)
{
  MYSQL_RES *res;
  MYSQL_ROW row;
  struct json_object *a;
  if (!out || !clan_progression_uid_valid(clan) || player < 1 || account < 1 || now < 0)
    return CLAN_REPO_INVALID;
  if (!clan_expedition_progress_ready(db))
    return CLAN_REPO_SCHEMA;
  if (!sql(db,
           "SELECT e.outing_uid,c.family,c.expires_at,e.state,r.inactive FROM "
           "clan_expedition_entitlements e JOIN clan_expedition_completions c ON "
           "c.outing_uid=e.outing_uid JOIN clan_expedition_outings o ON o.outing_uid=e.outing_uid "
           "JOIN clan_expedition_roster r ON r.outing_uid=e.outing_uid AND r.player_id=e.player_id "
           "WHERE o.clan_uid='%s' AND e.player_id=%" PRId64 " AND e.account_id=%" PRId64
           " AND (c.expires_at>%" PRId64
           " OR e.state=1) ORDER BY c.completed_at DESC,e.outing_uid LIMIT 32",
           clan, player, account, now))
    return CLAN_REPO_UNAVAILABLE;
  res = mysql_store_result(db);
  if (!res)
    return CLAN_REPO_UNAVAILABLE;
  a = json_object_new_array();
  while ((row = mysql_fetch_row(res)))
  {
    struct json_object *v = json_object_new_object();
    const char *keys[] = {"outing_uid", "family", "expires_at", "state", "inactive"};
    int i;
    for (i = 0; i < 5; i++)
      json_object_object_add(
          v, keys[i], i ? json_object_new_int64(atoll(row[i])) : json_object_new_string(row[i]));
    json_object_array_add(a, v);
  }
  mysql_free_result(res);
  *out = a;
  return CLAN_REPO_OK;
}
enum clan_repository_result clan_expedition_claim_prepare(
    MYSQL *db, const char *uid, const char *clan, const struct clan_expedition_member *m,
    int64_t now, const struct clan_progression_config *config, struct json_object **out)
{
  enum clan_repository_result r;
  MYSQL_RES *res;
  MYSQL_ROW row;
  struct clan_reward_event e = {0};
  int family, state;
  int64_t week, expiry, completed;
  bool inactive;
  if (!out || !m || m->player < 1 || m->account < 1 || !clan_progression_uid_valid(uid))
    return CLAN_REPO_INVALID;
  if (!clan_expedition_progress_ready(db))
    return CLAN_REPO_SCHEMA;
  r = begin(db, clan, now, config);
  if (r != CLAN_REPO_OK)
    return r;
  r = audit_outing(db, uid, clan, now);
  if (r != CLAN_REPO_OK)
    return end(db, r);

  if (!sql(db,
           "SELECT "
           "e.operation_uid,e.state,c.family,c.week_start,c.expires_at,c.completed_at,r.inactive "
           "FROM clan_expedition_entitlements e JOIN clan_expedition_completions c ON "
           "c.outing_uid=e.outing_uid JOIN clan_expedition_outings o ON o.outing_uid=e.outing_uid "
           "JOIN clan_expedition_roster r ON r.outing_uid=e.outing_uid AND r.player_id=e.player_id "
           "WHERE e.outing_uid='%s' AND o.clan_uid='%s' AND e.player_id=%" PRId64
           " AND e.account_id=%" PRId64,
           uid, clan, m->player, m->account))
    return end(db, CLAN_REPO_UNAVAILABLE);
  res = mysql_store_result(db);
  if (!res)
    return end(db, CLAN_REPO_UNAVAILABLE);
  row = mysql_fetch_row(res);
  if (!row)
  {
    mysql_free_result(res);
    return end(db, CLAN_REPO_NOT_FOUND);
  }
  snprintf(e.operation_uid, 33, "%s", row[0]);
  state = atoi(row[1]);
  family = atoi(row[2]);
  week = atoll(row[3]);
  expiry = atoll(row[4]);
  completed = atoll(row[5]);
  inactive = atoi(row[6]) != 0;
  mysql_free_result(res);
  if (state > 0)
  {
    r = clan_reward_load(db, e.operation_uid, out);
    return end(db, r);
  }
  if (inactive || now >= expiry || now < completed || !member(db, m->player, clan))
    return end(db, CLAN_REPO_INELIGIBLE);
  strcpy(e.event_uid, uid);
  strcpy(e.clan_uid, clan);
  e.player_id = m->player;
  e.account_id = m->account;
  e.occurred_at = completed;
  e.source = CLAN_SOURCE_EXPEDITION_PERSONAL;
  e.rules_version = CLAN_PROGRESSION_RULES_VERSION;
  e.requested_favor_eighths = 800;
  {
    int64_t n;
    if (!sql(db,
             "SELECT COUNT(*) FROM clan_expedition_personal_claims WHERE account_id=%" PRId64
             " AND week_start=%" PRId64 " AND family=%d",
             m->account, week, family) ||
        !scalar(db, &n))
      return end(db, CLAN_REPO_UNAVAILABLE);
    if (n)
      return end(db, CLAN_REPO_INELIGIBLE);
  }
  if (!reward_insert(db, &e, now) ||
      !sql(db,
           "INSERT INTO clan_expedition_personal_claims VALUES(%" PRId64 ",%" PRId64 ",%d,'%s')",
           m->account, week, family, e.operation_uid) ||
      !sql(db,
           "UPDATE clan_expedition_entitlements SET state=1 WHERE outing_uid='%s' AND "
           "player_id=%" PRId64 " AND state=0",
           uid, m->player))
    return end(db, CLAN_REPO_UNAVAILABLE);
  r = end(db, CLAN_REPO_OK);
  if (r != CLAN_REPO_OK)
    return r;
  return clan_reward_load(db, e.operation_uid, out);
}
static bool expedition_decide(const struct clan_reward_event *e,
                              const struct clan_reward_snapshot *s, struct clan_reward_delta *d,
                              void *context)
{
  struct clan_award_projection p;
  (void)context;
  if (e->source == CLAN_SOURCE_EXPEDITION_PERSONAL)
    return clan_award_personal_project(e, s, d);
  if ((e->source != CLAN_SOURCE_ASSAULT_SHARED && e->source != CLAN_SOURCE_HUNT_SHARED) ||
      e->player_id || e->account_id || e->requested_favor_eighths ||
      e->requested_xp_eighths != 4800 || e->requested_influence_eighths != 2400 ||
      !clan_award_project(&clan_progression_defaults, s->clan.xp_eighths, s->clan.influence_eighths,
                          s->weekly.incoming_xp_eighths, s->weekly.influence_eighths, 4800, 2400,
                          &p))
    return false;
  memset(d, 0, sizeof(*d));
  d->xp_eighths = p.xp_eighths;
  d->influence_eighths = p.influence_eighths;
  d->incoming_xp_eighths = 4800;
  d->cap_flags = p.cap_flags;
  return true;
}
static bool expedition_paid(MYSQL *db, const struct clan_reward_event *e,
                            const struct clan_reward_snapshot *s, const struct clan_reward_delta *d,
                            void *context)
{
  int64_t n;
  if (audit_outing(db, e->event_uid, e->clan_uid, e->occurred_at) != CLAN_REPO_OK)
    return false;
  (void)s;
  (void)d;
  (void)context;
  if (e->source == CLAN_SOURCE_EXPEDITION_PERSONAL)
  {
    if (!sql(db,
             "SELECT COUNT(*) FROM clan_expedition_entitlements t JOIN clan_expedition_completions "
             "c ON c.outing_uid=t.outing_uid JOIN clan_expedition_outings o ON "
             "o.outing_uid=t.outing_uid JOIN clan_expedition_personal_claims p ON "
             "p.operation_uid=t.operation_uid WHERE t.operation_uid='%s' AND t.outing_uid='%s' AND "
             "t.player_id=%" PRId64 " AND t.account_id=%" PRId64
             " AND t.state=1 AND o.clan_uid='%s' AND p.account_id=t.account_id AND "
             "p.family=c.family AND p.week_start=c.week_start AND c.completed_at=%" PRId64,
             e->operation_uid, e->event_uid, e->player_id, e->account_id, e->clan_uid,
             e->occurred_at) ||
        !scalar(db, &n) || n != 1)
      return false;
    return sql(db,
               "UPDATE clan_expedition_entitlements SET state=2 WHERE operation_uid='%s' AND "
               "state=1",
               e->operation_uid) &&
           mysql_affected_rows(db) == 1;
  }
  return sql(db,
             "SELECT COUNT(*) FROM clan_expedition_shared_claims s JOIN "
             "clan_expedition_completions c ON c.outing_uid='%s' JOIN clan_expedition_outings o ON "
             "o.outing_uid=c.outing_uid WHERE s.operation_uid='%s' AND s.clan_uid='%s' AND "
             "o.clan_uid=s.clan_uid AND s.week_start=c.week_start AND s.family=c.family AND "
             "s.family=%d AND c.completed_at=%" PRId64,
             e->event_uid, e->operation_uid, e->clan_uid,
             e->source == CLAN_SOURCE_ASSAULT_SHARED ? 1 : 2, e->occurred_at) &&
         scalar(db, &n) && n == 1;
}
enum clan_repository_result
clan_expedition_reward_complete(MYSQL *db, const char *uid, int64_t now,
                                const struct clan_progression_config *config,
                                struct json_object **out)
{
  struct json_object *receipt = NULL, *event;
  enum clan_repository_result r;
  struct clan_progression_readiness ready = {.schema_ready = true,
                                             .schema_version = CLAN_PROGRESSION_SCHEMA_VERSION,
                                             .contracts_ready = true,
                                             .expeditions_ready = true};
  if (!out || !clan_progression_uid_valid(uid) || now < 0)
    return CLAN_REPO_INVALID;
  if (!clan_expedition_progress_ready(db))
    return CLAN_REPO_SCHEMA;
  r = clan_reward_load(db, uid, &receipt);
  if (r != CLAN_REPO_OK)
    return r;
  event = get(receipt, "event");
  if (num(event, "source") != CLAN_SOURCE_EXPEDITION_PERSONAL &&
      num(event, "source") != CLAN_SOURCE_ASSAULT_SHARED &&
      num(event, "source") != CLAN_SOURCE_HUNT_SHARED)
  {
    json_object_put(receipt);
    return CLAN_REPO_INVALID;
  }
  if (!clan_progression_feature_available(config, CLAN_FEATURE_EXPEDITIONS, &ready,
                                          str(event, "clan_uid")))
  {
    json_object_put(receipt);
    return CLAN_REPO_INELIGIBLE;
  }
  json_object_put(receipt);
  return clan_reward_complete_objective(db, uid, now, expedition_decide, expedition_paid, NULL,
                                        out);
}
/* Retained-ledger validation. Never infer counters or restore missing receipts
 * from inventory, commands, online presence or a player-file snapshot. */
static enum clan_repository_result audit_outing(MYSQL *db, const char *uid, const char *clan,
                                                int64_t now)
{
  struct json_object *o = NULL, *wrap = NULL, *defs, *events = NULL, *entitlements = NULL, *roster;
  struct clan_expedition_catalog parsed;
  struct clan_reset_period week;
  enum clan_repository_result r = load(db, uid, clan, &o);
  MYSQL_RES *res;
  MYSQL_ROW row;
  size_t i, j, k, nroster;
  int counts[3] = {0}, credits[32] = {0}, total = 0;
  uint32_t final_masks[30] = {0}, entitlement_mask = 0;
  size_t final_count = 0;
  int64_t completed = 0, claim_expiry = 0, n;
  bool complete = false;
  if (r != CLAN_REPO_OK)
    return r;
  r = CLAN_REPO_CONFLICT;
  roster = get(o, "roster");
  nroster = json_object_array_length(roster);
  wrap = json_object_new_object();
  defs = json_object_new_array();
  json_object_array_add(defs, json_object_get(get(o, "definition")));
  json_object_object_add(wrap, "version", json_object_new_int(1));
  json_object_object_add(wrap, "expeditions", defs);
  if (!clan_expedition_catalog_parse(wrap, &parsed, NULL, 0) || !nroster || nroster > 32 ||
      !clan_progression_week_at(num(o, "created_at"), &week) ||
      week.starts_at != num(o, "week_start") || num(o, "expires_at") > week.ends_at ||
      num(o, "created_at") >= num(o, "expires_at") || num(o, "roster_locked") < 0 ||
      num(o, "roster_locked") > 1)
    goto done;
  if (num(o, "started_at") && (num(o, "started_at") < num(o, "created_at") ||
                               num(o, "started_at") >= num(o, "expires_at") ||
                               num(o, "expires_at") - num(o, "started_at") > 7200))
    goto done;
  for (i = 0; i < nroster; i++)
  {
    struct json_object *v = json_object_array_get_idx(roster, i);
    if (num(v, "player") < 1 || num(v, "account") < 1 ||
        num(v, "enrolled_at") < num(o, "created_at") ||
        num(v, "enrolled_at") >= num(o, "expires_at") || num(v, "inactive") < 0 ||
        num(v, "inactive") > 1)
      goto done;
    for (j = 0; j < i; j++)
    {
      struct json_object *old = json_object_array_get_idx(roster, j);
      if (num(old, "player") == num(v, "player") || num(old, "account") == num(v, "account"))
        goto done;
    }
  }
  if (!sql(db,
           "SELECT occurred_at,area,target,objective,participants,facts FROM "
           "clan_expedition_events WHERE outing_uid='%s' ORDER BY occurred_at,event_uid LIMIT 31",
           uid))
  {
    r = CLAN_REPO_UNAVAILABLE;
    goto done;
  }
  res = mysql_store_result(db);
  if (!res)
  {
    r = CLAN_REPO_UNAVAILABLE;
    goto done;
  }
  events = json_object_new_array();
  while ((row = mysql_fetch_row(res)))
  {
    struct json_object *v = json_object_new_object();
    const char *keys[] = {"at", "area", "target", "objective", "participants", "facts"};
    for (i = 0; i < 6; i++)
      json_object_object_add(
          v, keys[i], i < 4 ? json_object_new_int64(atoll(row[i])) : json_tokener_parse(row[i]));
    json_object_array_add(events, v);
  }
  mysql_free_result(res);
  if (json_object_array_length(events) > 30)
    goto done;
  for (i = 0; i < json_object_array_length(events); i++)
  {
    struct json_object *v = json_object_array_get_idx(events, i), *p = get(v, "participants"),
                       *f = get(v, "facts");
    int objective = num(v, "objective");
    bool target = false;
    uint32_t mask = 0;
    uint32_t fact_mask = 0;
    if (objective < 0 || objective > 2 || !num(o, "started_at") ||
        num(v, "at") < num(o, "started_at") || num(v, "at") >= num(o, "expires_at") ||
        !num(o, "roster_locked") || json_object_get_type(p) != json_type_array ||
        json_object_array_length(p) < 2 || json_object_array_length(p) > 6 ||
        json_object_get_type(f) != json_type_array || json_object_array_length(f) > 32)
      goto done;
    for (j = 0; j < parsed.entries[0].objectives[objective].count; j++)
      if (parsed.entries[0].objectives[objective].targets[j].area == num(v, "area") &&
          parsed.entries[0].objectives[objective].targets[j].mob == num(v, "target"))
        target = true;
    if (!target || ++counts[objective] > parsed.entries[0].objectives[objective].quota)
      goto done;
    for (j = 0; j < json_object_array_length(f); j++)
    {
      struct json_object *fact = json_object_array_get_idx(f, j);
      int level;
      if (json_object_get_type(fact) != json_type_array || json_object_array_length(fact) != 3)
        goto done;
      for (k = 0; k < nroster; k++)
        if (num(json_object_array_get_idx(roster, k), "player") ==
            json_object_get_int64(json_object_array_get_idx(fact, 0)))
          break;
      if (k == nroster || (fact_mask & (UINT32_C(1) << k)) ||
          num(json_object_array_get_idx(roster, k), "account") !=
              json_object_get_int64(json_object_array_get_idx(fact, 1)))
        goto done;
      level = json_object_get_int(json_object_array_get_idx(fact, 2));
      if (level < parsed.entries[0].min_player_level || level > parsed.entries[0].max_player_level)
        goto done;
      fact_mask |= UINT32_C(1) << k;
    }
    for (j = 0; j < json_object_array_length(p); j++)
    {
      struct json_object *player = json_object_array_get_idx(p, j);
      if (json_object_get_type(player) != json_type_int)
        goto done;
      for (k = 0; k < nroster; k++)
        if (num(json_object_array_get_idx(roster, k), "player") == json_object_get_int64(player))
          break;
      if (k == nroster || (mask & (UINT32_C(1) << k)) || !(fact_mask & (UINT32_C(1) << k)))
        goto done;
      mask |= UINT32_C(1) << k;
      credits[k]++;
    }
  }
  for (i = 0; i < 3; i++)
    total += parsed.entries[0].objectives[i].quota;
  if (num(o, "state") == 0 &&
      (num(o, "started_at") || json_object_array_length(events) || num(o, "roster_locked")))
    goto done;
  if (num(o, "state") == 1 && !num(o, "started_at"))
    goto done;
  if (!sql(db,
           "SELECT family,week_start,completed_at,expires_at,objectives FROM "
           "clan_expedition_completions WHERE outing_uid='%s'",
           uid))
  {
    r = CLAN_REPO_UNAVAILABLE;
    goto done;
  }
  res = mysql_store_result(db);
  if (!res)
  {
    r = CLAN_REPO_UNAVAILABLE;
    goto done;
  }
  row = mysql_fetch_row(res);
  if (row)
  {
    complete = true;
    completed = atoll(row[2]);
    claim_expiry = atoll(row[3]);
    bool valid = num(o, "state") == 3 && atoi(row[0]) == parsed.entries[0].family &&
                 atoll(row[1]) == week.starts_at && completed >= num(o, "started_at") &&
                 completed < num(o, "expires_at") && claim_expiry == week.ends_at &&
                 atoi(row[4]) == total && json_object_array_length(events) == (size_t)total;
    mysql_free_result(res);
    if (!valid)
      goto done;
  }
  else
    mysql_free_result(res);
  if (!complete && json_object_array_length(events) == (size_t)total)
    goto done;
  if (!sql(db,
           "SELECT player_id,account_id,operation_uid,state FROM clan_expedition_entitlements "
           "WHERE outing_uid='%s' ORDER BY player_id LIMIT 7",
           uid))
  {
    r = CLAN_REPO_UNAVAILABLE;
    goto done;
  }
  res = mysql_store_result(db);
  if (!res)
  {
    r = CLAN_REPO_UNAVAILABLE;
    goto done;
  }
  entitlements = json_object_new_array();
  while ((row = mysql_fetch_row(res)))
  {
    struct json_object *v = json_object_new_object();
    json_object_object_add(v, "player", json_object_new_int64(atoll(row[0])));
    json_object_object_add(v, "account", json_object_new_int64(atoll(row[1])));
    json_object_object_add(v, "operation", json_object_new_string(row[2]));
    json_object_object_add(v, "state", json_object_new_int(atoi(row[3])));
    json_object_array_add(entitlements, v);
  }
  mysql_free_result(res);
  if ((!complete && json_object_array_length(entitlements)) ||
      (complete &&
       (json_object_array_length(entitlements) < 2 || json_object_array_length(entitlements) > 6)))
    goto done;
  for (i = 0; i < json_object_array_length(entitlements); i++)
  {
    struct json_object *v = json_object_array_get_idx(entitlements, i), *receipt = NULL, *e;
    char expected[33];
    for (k = 0; k < nroster; k++)
      if (num(json_object_array_get_idx(roster, k), "player") == num(v, "player"))
        break;
    if (k == nroster || num(json_object_array_get_idx(roster, k), "account") != num(v, "account") ||
        credits[k] < (total + 1) / 2 || num(v, "state") < 0 || num(v, "state") > 2)
      goto done;
    entitlement_mask |= UINT32_C(1) << k;
    expedition_uid(expected, "personal", uid, num(v, "player"));
    if (strcmp(expected, str(v, "operation")))
      goto done;
    if (!sql(db, "SELECT COUNT(*) FROM clan_reward_receipts WHERE operation_uid='%s'", expected) ||
        !scalar(db, &n))
    {
      r = CLAN_REPO_UNAVAILABLE;
      goto done;
    }
    if (!num(v, "state"))
    {
      if (n)
        goto done;
      continue;
    }
    if (n != 1 || clan_reward_load(db, expected, &receipt) != CLAN_REPO_OK)
      goto done;
    e = get(receipt, "event");
    bool valid = num(receipt, "status") == num(v, "state") - 1 &&
                 num(e, "source") == CLAN_SOURCE_EXPEDITION_PERSONAL &&
                 num(e, "player_id") == num(v, "player") &&
                 num(e, "account_id") == num(v, "account") && !strcmp(str(e, "clan_uid"), clan) &&
                 !strcmp(str(e, "event_uid"), uid) && num(e, "occurred_at") == completed &&
                 num(e, "requested_favor_eighths") == 800 && !num(e, "requested_xp_eighths") &&
                 !num(e, "requested_influence_eighths");
    json_object_put(receipt);
    if (!valid)
      goto done;
    if (!sql(db,
             "SELECT COUNT(*) FROM clan_expedition_personal_claims WHERE operation_uid='%s' AND "
             "account_id=%" PRId64 " AND week_start=%" PRId64 " AND family=%d",
             expected, num(v, "account"), week.starts_at, parsed.entries[0].family) ||
        !scalar(db, &n))
    {
      r = CLAN_REPO_UNAVAILABLE;
      goto done;
    }
    if (n != 1)
      goto done;
  }
  if (complete)
  {
    bool final = false;
    for (i = 0; i < json_object_array_length(events); i++)
    {
      struct json_object *v = json_object_array_get_idx(events, i), *p = get(v, "participants");
      uint32_t mask = 0;
      if (num(v, "at") != completed)
        continue;
      for (j = 0; j < json_object_array_length(p); j++)
        for (k = 0; k < nroster; k++)
          if (num(json_object_array_get_idx(roster, k), "player") ==
              json_object_get_int64(json_object_array_get_idx(p, j)))
            mask |= UINT32_C(1) << k;
      final_masks[final_count++] = mask;
    }
    for (i = 0; i < final_count; i++)
      if ((final_masks[i] & entitlement_mask) == entitlement_mask)
        final = true;
    if (!final)
      goto done;
    if (!sql(db,
             "SELECT COUNT(*) FROM clan_expedition_shared_claims s JOIN clan_reward_receipts r ON "
             "r.operation_uid=s.operation_uid JOIN clan_expedition_completions c ON "
             "c.outing_uid=r.event_uid JOIN clan_expedition_outings o ON o.outing_uid=c.outing_uid "
             "WHERE s.clan_uid='%s' AND s.week_start=%" PRId64
             " AND s.family=%d AND o.clan_uid=s.clan_uid AND c.family=s.family AND "
             "c.week_start=s.week_start AND r.source_id=%d AND r.occurred_at_utc=c.completed_at "
             "AND r.requested_xp_eighths=4800 AND r.requested_influence_eighths=2400 AND "
             "r.requested_favor_eighths=0",
             clan, week.starts_at, parsed.entries[0].family,
             parsed.entries[0].family == 1 ? 8 : 9) ||
        !scalar(db, &n))
    {
      r = CLAN_REPO_UNAVAILABLE;
      goto done;
    }
    if (n != 1)
      goto done;
  }
  (void)now;
  r = CLAN_REPO_OK;
done:
  if (wrap)
    json_object_put(wrap);
  if (events)
    json_object_put(events);
  if (entitlements)
    json_object_put(entitlements);
  json_object_put(o);
  return r;
}
static enum clan_repository_result maintenance_begin(MYSQL *db)
{
  int64_t n;
  if (!clan_expedition_progress_ready(db) || clan_repository_schema_check(db) != CLAN_REPO_OK)
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
static enum clan_repository_result reconcile_one(MYSQL *db, const char *uid, const char *clan,
                                                 int64_t now)
{
  enum clan_repository_result r = maintenance_begin(db);
  if (r != CLAN_REPO_OK)
    return r;
  r = audit_outing(db, uid, clan, now);
  if (r != CLAN_REPO_OK)
    return end(db, r);
  if (!sql(db,
           "UPDATE clan_expedition_roster r JOIN clan_expedition_outings o ON "
           "o.outing_uid=r.outing_uid LEFT JOIN clan_memberships m ON m.player_id=r.player_id AND "
           "m.clan_uid=o.clan_uid LEFT JOIN clan_progression_legacy l ON l.clan_uid=m.clan_uid SET "
           "r.inactive=1 WHERE r.outing_uid='%s' AND r.inactive=0 AND (m.player_id IS NULL OR "
           "m.rank_id<1 OR m.rank_id>JSON_LENGTH(JSON_EXTRACT(l.details,'$.ranks')))",
           uid) ||
      !sql(db,
           "UPDATE clan_expedition_outings o JOIN clan_progression_clans c ON "
           "c.clan_uid=o.clan_uid SET o.state=IF(c.record_status=1,4,3) WHERE o.outing_uid='%s' "
           "AND o.state IN (0,1) AND (o.expires_at<=%" PRId64 " OR c.record_status<>1)",
           uid, now))
    return end(db, CLAN_REPO_UNAVAILABLE);
#ifdef CLAN_REWARD_TEST
  if (clan_expedition_crash == 3)
    _exit(95);
#endif
  r = end(db, CLAN_REPO_OK);
#ifdef CLAN_REWARD_TEST
  if (r == CLAN_REPO_OK && clan_expedition_crash == 4)
    _exit(96);
#endif
  return r;
}
enum clan_repository_result clan_expedition_reconcile_clan(MYSQL *db, const char *clan, int64_t now)
{
  MYSQL_RES *res;
  MYSQL_ROW row;
  char uid[33];
  if (!clan_progression_uid_valid(clan) || now < 0)
    return CLAN_REPO_INVALID;
  if (!clan_expedition_progress_ready(db))
    return CLAN_REPO_SCHEMA;
  if (db->server_status & SERVER_STATUS_IN_TRANS)
    return CLAN_REPO_CONFLICT;
  if (!sql(db, "SELECT outing_uid FROM clan_expedition_outings WHERE active_clan='%s'", clan))
    return CLAN_REPO_UNAVAILABLE;
  res = mysql_store_result(db);
  if (!res)
    return CLAN_REPO_UNAVAILABLE;
  row = mysql_fetch_row(res);
  if (!row)
  {
    mysql_free_result(res);
    return CLAN_REPO_OK;
  }
  snprintf(uid, sizeof(uid), "%s", row[0]);
  mysql_free_result(res);
  return reconcile_one(db, uid, clan, now);
}
enum clan_repository_result clan_expedition_reconcile_batch(MYSQL *db, const char *after,
                                                            size_t limit, int64_t now,
                                                            struct json_object **out)
{
  MYSQL_RES *res;
  MYSQL_ROW row;
  char ids[32][33], clans[32][33];
  size_t count = 0, i;
  struct json_object *a;
  if (!out || now < 0 || !limit || limit > 32 ||
      (after && *after && !clan_progression_uid_valid(after)))
    return CLAN_REPO_INVALID;
  if (!clan_expedition_progress_ready(db))
    return CLAN_REPO_SCHEMA;
  if (db->server_status & SERVER_STATUS_IN_TRANS)
    return CLAN_REPO_CONFLICT;
  if (!sql(db,
           "SELECT outing_uid,clan_uid FROM clan_expedition_outings FORCE INDEX(ce_active) WHERE "
           "active_clan IS NOT NULL AND "
           "outing_uid>'%s' ORDER BY outing_uid LIMIT %zu",
           after ? after : "", limit))
    return CLAN_REPO_UNAVAILABLE;
  res = mysql_store_result(db);
  if (!res)
    return CLAN_REPO_UNAVAILABLE;
  while ((row = mysql_fetch_row(res)))
  {
    snprintf(ids[count], 33, "%s", row[0]);
    snprintf(clans[count], 33, "%s", row[1]);
    count++;
  }
  mysql_free_result(res);
  a = json_object_new_array();
  for (i = 0; i < count; i++)
  {
    struct json_object *v = json_object_new_object();
    enum clan_repository_result r = reconcile_one(db, ids[i], clans[i], now);
    json_object_object_add(v, "outing_uid", json_object_new_string(ids[i]));
    json_object_object_add(v, "result", json_object_new_string(clan_repository_result_name(r)));
    json_object_array_add(a, v);
  }
  *out = a;
  return CLAN_REPO_OK;
}
