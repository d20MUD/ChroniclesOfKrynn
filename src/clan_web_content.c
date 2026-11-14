#include "clan_web_content.h"
#include "clan_delivery.h"
#include "clan_objectives.h"
#include "clan_expedition_enrollment.h"
#include "clan_quartermaster.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
static struct json_object *get(struct json_object *o, const char *k)
{
  struct json_object *v = NULL;
  if (o)
    json_object_object_get_ex(o, k, &v);
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
static void text(struct json_object *o, const char *k, const char *s)
{
  json_object_object_add(o, k, json_object_new_string(s));
}
static void integer(struct json_object *o, const char *k, int64_t n)
{
  json_object_object_add(o, k, json_object_new_int64(n));
}
static void flag(struct json_object *o, const char *k, bool b)
{
  json_object_object_add(o, k, json_object_new_boolean(b));
}
static bool sql(MYSQL *, const char *, ...) __attribute__((format(printf, 2, 3)));
static bool sql(MYSQL *db, const char *f, ...)
{
  char query[4096];
  int n;
  va_list ap;
  va_start(ap, f);
  n = vsnprintf(query, sizeof(query), f, ap);
  va_end(ap);
  return n >= 0 && (size_t)n < sizeof(query) && !mysql_query(db, query);
}
static struct json_object *section(bool ready)
{
  struct json_object *o = json_object_new_object();
  flag(o, "ready", ready);
  json_object_object_add(o, "choices", json_object_new_array());
  json_object_object_add(o, "actions", json_object_new_array());
  json_object_object_add(o, "reasons", json_object_new_array());
  json_object_object_add(o, "notes", json_object_new_array());
  return o;
}
static void unavailable(struct json_object *o)
{
  flag(o, "ready", false);
}
static bool claims(MYSQL *db, struct json_object *section, int family, int64_t account, int64_t now)
{
  struct json_object *a = NULL;
  enum clan_repository_result r = family == 3 ? clan_delivery_claim_history(db, account, now, &a)
                                              : clan_objective_claims(db, account, family, now, &a);
  if (r != CLAN_REPO_OK)
    return false;
  json_object_object_add(section, "completed_ids", a);
  return true;
}
static bool contract(MYSQL *db, struct json_object *s, const char *clan, int64_t player,
                     int64_t account, int family, int64_t now)
{
  MYSQL_RES *res;
  MYSQL_ROW row;
  struct json_object *d, *active;
  struct clan_delivery_definition delivery;
  struct clan_objective_definition objective;
  int progress = 0, required, state;
  const char *query =
      family == 3 ? "SELECT accepted_uid,clan_uid,account_id,terms,expires_at,state,0 FROM "
                    "clan_delivery_contracts WHERE active_player=%" PRId64 " LIMIT 1"
                  : "SELECT contract_uid,clan_uid,account_id,terms,expires_at,state,progress FROM "
                    "clan_objective_contracts WHERE active_player=%" PRId64
                    " AND family=%d LIMIT 1";
  if (!claims(db, s, family, account, now) ||
      !(family == 3 ? sql(db, query, player) : sql(db, query, player, family)) ||
      !(res = mysql_store_result(db)))
    return false;
  row = mysql_fetch_row(res);
  if (!row)
  {
    mysql_free_result(res);
    return true;
  }
  d = row[3] ? json_tokener_parse(row[3]) : NULL;
  if (!clan_progression_uid_valid(row[0]) || !clan_progression_uid_valid(row[1]) ||
      atoll(row[2]) != account ||
      !(family == 3 ? clan_delivery_definition_parse(d, &delivery)
                    : clan_objective_definition_parse(d, &objective)))
  {
    if (d)
      json_object_put(d);
    mysql_free_result(res);
    return false;
  }
  required = family == 3 ? delivery.quantity : objective.required;
  progress = atoi(row[6]);
  state = atoi(row[5]);
  if (progress < 0 || progress > required || state < 0 || state > 1 ||
      (family != 3 && objective.family != family))
  {
    json_object_put(d);
    mysql_free_result(res);
    return false;
  }
  active = json_object_new_object();
  text(active, "uid", row[0]);
  text(active, "name", family == 3 ? delivery.name : objective.name);
  text(active, "definition_id", family == 3 ? delivery.id : objective.id);
  integer(active, "progress", progress);
  integer(active, "required", required);
  integer(active, "expires_at", atoll(row[4]));
  flag(active, "own_clan", !strcmp(row[1], clan));
  text(active, "state",
       state == 1             ? "reward_pending"
       : now >= atoll(row[4]) ? "expired"
                              : "active");
  json_object_object_add(active, "definition", d);
  json_object_object_add(s, "active", active);
  mysql_free_result(res);
  return true;
}
static bool expedition(MYSQL *db, struct json_object *s, const char *clan, int64_t player,
                       int64_t account, int64_t now)
{
  struct json_object *o = NULL, *counts = NULL, *raw = NULL, *a = json_object_new_array();
  enum clan_repository_result r;
  size_t i;
  MYSQL_RES *res;
  MYSQL_ROW row;
  struct clan_reset_period week;
  json_object_object_add(s, "claims", a);
  r = clan_expedition_read(db, clan, &o);
  if (r != CLAN_REPO_OK && r != CLAN_REPO_NOT_FOUND)
    return false;
  if (o)
  {
    struct clan_expedition_catalog parsed;
    struct json_object *wrapper = json_object_new_object(), *defs = json_object_new_array();
    bool valid;
    integer(wrapper, "version", 1);
    json_object_object_add(wrapper, "expeditions", defs);
    json_object_array_add(defs, json_object_get(get(o, "definition")));
    valid = clan_expedition_catalog_parse(wrapper, &parsed, NULL, 0);
    json_object_put(wrapper);
    if (!valid || clan_expedition_progress_read(db, str(o, "outing_uid"), &counts) != CLAN_REPO_OK)
    {
      json_object_put(o);
      return false;
    }
    raw = json_object_new_object();
    text(raw, "uid", str(o, "outing_uid"));
    text(raw, "name", parsed.entries[0].name);
    text(raw, "state",
         now >= num(o, "expires_at") ? "expired"
         : num(o, "state") == 0      ? "recruiting"
                                     : "started");
    integer(raw, "expires_at", num(o, "expires_at"));
    flag(raw, "sealed", num(o, "roster_locked"));
    flag(raw, "leader", num(o, "leader_id") == player);
    integer(raw, "roster_count", 0);
    flag(raw, "enrolled", false);
    flag(raw, "removed", false);
    flag(raw, "account_in_roster", false);
    for (i = 0; i < json_object_array_length(get(o, "roster")); i++)
    {
      struct json_object *m = json_object_array_get_idx(get(o, "roster"), i);
      if (num(m, "account") == account)
        flag(raw, "account_in_roster", true);
      if (!num(m, "inactive"))
        integer(raw, "roster_count", num(raw, "roster_count") + 1);
      if (num(m, "player") == player && num(m, "account") == account)
      {
        flag(raw, "enrolled", !num(m, "inactive"));
        flag(raw, "removed", num(m, "inactive"));
      }
    }
    if (!sql(db,
             "SELECT COUNT(*) FROM clan_expedition_events WHERE outing_uid='%s' AND "
             "JSON_CONTAINS(participants,'%" PRId64 "')",
             str(o, "outing_uid"), player) ||
        !(res = mysql_store_result(db)))
    {
      json_object_put(o);
      json_object_put(raw);
      json_object_put(counts);
      return false;
    }
    row = mysql_fetch_row(res);
    integer(raw, "personal_credit", row ? atoll(row[0]) : 0);
    mysql_free_result(res);
    json_object_object_add(raw, "definition", json_object_get(get(o, "definition")));
    json_object_object_add(raw, "counts", counts);
    json_object_object_add(raw, "objectives", json_object_new_array());
    json_object_object_add(s, "active", raw);
    json_object_put(o);
  }
  clan_progression_week_at(now, &week);
  if (!sql(db,
           "SELECT e.outing_uid,c.family,c.expires_at,e.state,r.inactive,EXISTS(SELECT 1 FROM "
           "clan_expedition_personal_claims p WHERE p.account_id=e.account_id AND "
           "p.week_start=c.week_start AND p.family=c.family) FROM clan_expedition_entitlements e "
           "JOIN clan_expedition_completions c ON c.outing_uid=e.outing_uid JOIN "
           "clan_expedition_outings o ON o.outing_uid=e.outing_uid JOIN clan_expedition_roster r "
           "ON r.outing_uid=e.outing_uid AND r.player_id=e.player_id WHERE o.clan_uid='%s' AND "
           "e.player_id=%" PRId64 " AND e.account_id=%" PRId64 " AND (c.expires_at>=%" PRId64
           " OR e.state=1) ORDER BY c.completed_at DESC,e.outing_uid LIMIT 16",
           clan, player, account, week.starts_at) ||
      !(res = mysql_store_result(db)))
    return false;
  while ((row = mysql_fetch_row(res)))
  {
    struct json_object *claim = json_object_new_object();
    if (!clan_progression_uid_valid(row[0]))
    {
      json_object_put(claim);
      mysql_free_result(res);
      return false;
    }
    text(claim, "uid", row[0]);
    integer(claim, "family", atoi(row[1]));
    integer(claim, "expires_at", atoll(row[2]));
    text(claim, "state",
         atoi(row[3]) == 2   ? "claimed"
         : atoi(row[3]) == 1 ? "reward_pending"
                             : "unclaimed");
    flag(claim, "removed", atoi(row[4]));
    flag(claim, "account_claimed", atoi(row[5]));
    json_object_object_add(claim, "actions", json_object_new_array());
    json_object_array_add(a, claim);
  }
  mysql_free_result(res);
  return true;
}
struct json_object *clan_web_content_read(MYSQL *db, const struct clan_progression_config *config,
                                          const char *clan, int64_t player, int64_t account,
                                          int64_t now)
{
  struct json_object *root = json_object_new_object(), *contracts = json_object_new_array(), *s;
  int i;
  bool dr = clan_delivery_schema_ready(db), orr = clan_objective_schema_ready(db),
       er = clan_expedition_progress_ready(db), qr = clan_qm_schema_ready(db);
  MYSQL_RES *res;
  MYSQL_ROW row;
  struct clan_reset_period week;
  (void)config;
  integer(root, "version", 1);
  flag(root, "context_ready", false);
  json_object_object_add(root, "contracts", contracts);
  for (i = 3; i <= 5; i++)
  {
    bool ready = i == 3 ? dr : orr;
    s = section(ready);
    text(s, "family", i == 3 ? "delivery" : i == 4 ? "gathering" : "bounty");
    json_object_object_add(s, "completed_ids", json_object_new_array());
    if (ready && !contract(db, s, clan, player, account, i, now))
      unavailable(s);
    if (i == 3 && ready)
    {
      if (!sql(db,
               "SELECT operation_uid FROM clan_delivery_operations WHERE held_player=%" PRId64
               " LIMIT 1",
               player) ||
          !(res = mysql_store_result(db)))
        unavailable(s);
      else
      {
        flag(s, "recovery_pending", mysql_fetch_row(res) != NULL);
        mysql_free_result(res);
      }
    }
    json_object_array_add(contracts, s);
  }
  s = section(er);
  if (er && !expedition(db, s, clan, player, account, now))
    unavailable(s);
  json_object_object_add(root, "expedition", s);
  if (qr) {
    if (!sql(db, "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='clan_quartermaster_services' AND engine='InnoDB'") ||
        !(res=mysql_store_result(db))) qr=false;
    else { row=mysql_fetch_row(res); qr=row && atoi(row[0])==1; mysql_free_result(res); }
  }
  s = section(qr);
  flag(s, "purchase_pending", false);
  if (qr)
  {
    if (!sql(db,
             "SELECT operation_uid FROM clan_quartermaster_operations WHERE active_player=%" PRId64
             " UNION ALL SELECT operation_uid FROM clan_quartermaster_services WHERE active_player=%" PRId64 " LIMIT 1",
             player, player) ||
        !(res = mysql_store_result(db)))
      unavailable(s);
    else
    {
      row = mysql_fetch_row(res);
      flag(s, "purchase_pending", row != NULL);
      mysql_free_result(res);
    }
  }
  json_object_object_add(root, "quartermaster", s);
  clan_progression_week_at(now, &week);
  if (sql(db,
          "SELECT incoming_xp_eighths,influence_eighths FROM clan_weekly_windows WHERE "
          "clan_uid='%s' AND starts_at_utc=%" PRId64,
          clan, week.starts_at) &&
      (res = mysql_store_result(db)))
  {
    row = mysql_fetch_row(res);
    integer(root, "weekly_xp_eighths", row ? atoll(row[0]) : 0);
    integer(root, "weekly_influence_eighths", row ? atoll(row[1]) : 0);
    mysql_free_result(res);
  }
  else
    text(root, "limits_warning",
         "Clan weekly caps could not be read; previews remain authoritative.");
  return root;
}
