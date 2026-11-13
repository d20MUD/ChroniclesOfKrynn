#include "clan_web.h"
#include "clan_web_content.h"
#include "clan_awards.h"
#include "clan_buff_lifecycle.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
static void number(struct json_object *o, const char *k, int64_t n)
{
  json_object_object_add(o, k, json_object_new_int64(n));
}
static void decimal(struct json_object *o, const char *k, int64_t n)
{
  char buf[32];
  snprintf(buf, sizeof(buf), "%" PRId64, n);
  json_object_object_add(o, k, json_object_new_string(buf));
}
static void text(struct json_object *o, const char *k, const char *s)
{
  json_object_object_add(o, k, json_object_new_string(s));
}
static void boolean(struct json_object *o, const char *k, bool b)
{
  json_object_object_add(o, k, json_object_new_boolean(b));
}
struct json_object *clan_web_status(const char *status, const char *reason, int64_t now)
{
  struct json_object *o = json_object_new_object();
  number(o, "version", 1);
  number(o, "server_time", now);
  text(o, "status", status);
  text(o, "reason", reason);
  return o;
}
static struct json_object *reasons(unsigned int bits)
{
  unsigned int bit;
  struct json_object *a = json_object_new_array();
  for (bit = 1; bit <= CLAN_BUFF_CLOCK; bit <<= 1)
    if (bits & bit)
      json_object_array_add(a, json_object_new_string(clan_buff_rejection_text(bit)));
  return a;
}
struct json_object *clan_web_render(const struct clan_progression_config *c,
                                    const struct clan_web_snapshot *s, int64_t now)
{
  struct json_object *o, *clan, *favor, *contribution, *sources, *families, *unlocks;
  struct clan_buff_view view;
  struct clan_reset_period week;
  enum clan_favor_standing standing;
  int level;
  size_t i, j, k;
  struct clan_progression_readiness ready = {0};
  if (!s || clan_progression_config_validate(c, NULL) != CLAN_CONFIG_OK ||
      !clan_repository_record_valid(&s->buffs.clan) || !s->buffs.accepted_member ||
      !clan_award_level(c, s->buffs.clan.xp_eighths, &level) ||
      !clan_award_standing(s->favor.favor_eighths, &standing) ||
      strcmp(s->favor.clan_uid, s->buffs.clan.uid) || s->favor.character_id <= 0 ||
      !clan_progression_week_at(now, &week) ||
      !clan_progression_account_window_valid(&s->account) ||
      s->account.starts_at != week.starts_at || now > INT64_C(9007199254740991) ||
      !clan_buff_query(c, s->buffs.clan.uid, s->buffs.clan.status, level, s->buffs.rows,
                       s->buffs.count, now, &view))
    return NULL;
  ready.schema_ready = true;
  ready.schema_version = CLAN_PROGRESSION_SCHEMA_VERSION;
  if (!clan_progression_feature_available(c, CLAN_FEATURE_WEB, &ready, s->buffs.clan.uid))
    return clan_web_status("disabled", "Clan web reports are disabled for this clan.", now);
  o = clan_web_status("ready", "", now);
  clan = json_object_new_object();
  favor = json_object_new_object();
  contribution = json_object_new_object();
  sources = json_object_new_array();
  families = json_object_new_array();
  unlocks = json_object_new_array();
  json_object_object_add(o, "clan", clan);
  json_object_object_add(o, "favor", favor);
  json_object_object_add(o, "contribution", contribution);
  json_object_object_add(o, "buffs", families);
  json_object_object_add(o, "next_unlocks", unlocks);
  decimal(o, "character_id", s->favor.character_id);
  boolean(o, "buff_state_available", s->buff_storage_ready);
  text(clan, "uid", s->buffs.clan.uid);
  text(clan, "name", s->buffs.clan.name);
  number(clan, "level", level);
  decimal(clan, "xp_eighths", s->buffs.clan.xp_eighths);
  decimal(clan, "influence_eighths", s->buffs.clan.influence_eighths);
  decimal(clan, "revision", s->buffs.clan.state_revision);
  if (level < c->max_level)
    decimal(clan, "next_level_xp_eighths", c->xp_points[level] * 8);
  else
    json_object_object_add(clan, "next_level_xp_eighths", NULL);
  decimal(favor, "eighths", s->favor.favor_eighths);
  text(favor, "standing", c->standings[standing].name);
  if (standing < CLAN_FAVOR_EXALTED)
    decimal(favor, "next_standing_eighths", c->standings[standing + 1].favor_points * 8);
  else
    json_object_object_add(favor, "next_standing_eighths", NULL);
  boolean(o, "may_manage", s->buffs.may_manage);
  number(o, "slots_total", view.slots_total);
  number(o, "slots_used", view.slots_used);
  number(o, "combat_used", view.combat_used);
  number(contribution, "week_ends_at", week.ends_at);
  decimal(contribution, "base_xp_eighths", s->account.base_xp_eighths);
  decimal(contribution, "full_xp_eighths", c->limits.account_full_xp_points * 8);
  decimal(contribution, "half_xp_eighths",
          (c->limits.account_full_xp_points + c->limits.account_half_xp_points) * 8);
  decimal(contribution, "favor_eighths", s->account.favor_eighths);
  decimal(contribution, "favor_cap_eighths", c->limits.account_favor_points * 8);
  json_object_object_add(contribution, "sources", sources);
  for (i = 1; i <= CLAN_SOURCE_EXPEDITION_PERSONAL; i++)
  {
    const struct clan_reward_definition *r = clan_progression_reward(c, i);
    struct json_object *source = json_object_new_object();
    static const char *names[] = {"",          "Kills",    "Supply orders", "Deliveries",
                                  "Gathering", "Bounties", "Expeditions"};
    number(source, "id", i);
    text(source, "name", names[i]);
    decimal(source, "used", s->account.claims[i]);
    number(source, "limit", r->weekly_claims);
    json_object_array_add(sources, source);
  }
  if (level < c->third_slot_level)
  {
    struct json_object *u = json_object_new_object();
    number(u, "level", c->third_slot_level);
    text(u, "name", "Third buff slot");
    json_object_array_add(unlocks, u);
  }
  for (i = 0; i < CLAN_PROGRESSION_BUFF_COUNT; i++)
  {
    const struct clan_buff_definition *b = &c->buffs[i];
    const struct clan_buff_family_view *f = &view.families[i];
    struct json_object *family = json_object_new_object(), *tiers = json_object_new_array(),
                       *cancel = json_object_new_object();
    unsigned int cancel_bits = 0;
    const char *activation = "";
    number(family, "family", b->family);
    text(family, "name", b->name);
    text(family, "category",
         b->category == CLAN_CATEGORY_COMBAT     ? "combat"
         : b->category == CLAN_CATEGORY_GROWTH   ? "growth"
         : b->category == CLAN_CATEGORY_RECOVERY ? "recovery"
                                                 : "craft");
    boolean(family, "active", f->active);
    boolean(family, "cooling_down", f->cooling_down);
    number(family, "tier", f->active_tier);
    number(family, "slot", f->active_slot);
    number(family, "expires_at", f->expires_at);
    number(family, "next_available_at", f->next_available_at);
    for (k = 0; k < s->buffs.count; k++)
      if (s->buffs.rows[k].family == b->family)
        activation = s->buffs.rows[k].activation_uid;
    text(family, "activation_uid", activation);
    if (!s->buff_storage_ready)
      cancel_bits |= CLAN_BUFF_CORRUPT;
    if (!clan_progression_feature_available(c, CLAN_FEATURE_BUFFS, &ready, s->buffs.clan.uid))
      cancel_bits |= CLAN_BUFF_DISABLED;
    if (!s->buffs.may_manage)
      cancel_bits |= CLAN_BUFF_PERMISSION;
    if (!f->active)
      cancel_bits |= CLAN_BUFF_STALE;
    boolean(cancel, "enabled", cancel_bits == 0);
    json_object_object_add(cancel, "reasons", reasons(cancel_bits));
    json_object_object_add(family, "cancel", cancel);
    json_object_object_add(family, "tiers", tiers);
    for (j = 0; j < CLAN_PROGRESSION_TIER_COUNT; j++)
    {
      struct json_object *tier = json_object_new_object(), *slots = json_object_new_array();
      number(tier, "tier", j + 1);
      number(tier, "unlock_level", b->tiers[j].unlock_level);
      decimal(tier, "cost_eighths", b->tiers[j].influence_points * 8);
      json_object_object_add(tier, "slots", slots);
      json_object_array_add(tiers, tier);
      for (k = 1; k <= CLAN_PROGRESSION_MAX_SLOTS; k++)
      {
        struct clan_buff_request request = {0};
        struct clan_buff_decision decision;
        struct json_object *slot = json_object_new_object();
        size_t n;
        strcpy(request.operation_uid, "11111111111111111111111111111111");
        strcpy(request.clan_uid, s->buffs.clan.uid);
        request.actor_id = s->favor.character_id;
        request.family = b->family;
        request.tier = j + 1;
        request.slot = k;
        request.rules_version = 1;
        for (n = 0; n < s->buffs.count; n++)
          if (clan_repository_buff_active(&s->buffs.rows[n], now) &&
              s->buffs.rows[n].slot == (int)k)
            strcpy(request.expected_activation_uid, s->buffs.rows[n].activation_uid);
        clan_buff_validate(c, &ready, &request, &s->buffs, now, &decision);
        if (!s->buff_storage_ready)
          decision.reasons |= CLAN_BUFF_CORRUPT;
        number(slot, "slot", k);
        boolean(slot, "enabled", decision.reasons == 0);
        text(slot, "replace_activation_uid", request.expected_activation_uid);
        json_object_object_add(slot, "reasons", reasons(decision.reasons));
        json_object_array_add(slots, slot);
      }
    }
    for (j = 0; j < 3; j++)
      if (b->tiers[j].unlock_level > level)
      {
        struct json_object *u = json_object_new_object();
        char name[160];
        snprintf(name, sizeof(name), "%s tier %zu", b->name, j + 1);
        number(u, "level", b->tiers[j].unlock_level);
        text(u, "name", name);
        json_object_array_add(unlocks, u);
        break;
      }
    json_object_array_add(families, family);
  }
  if (strlen(json_object_to_json_string_ext(o, JSON_C_TO_STRING_PLAIN)) > CLAN_WEB_MAX_BYTES)
  {
    json_object_put(o);
    return NULL;
  }
  return o;
}
static bool sql(MYSQL *, const char *, ...) __attribute__((format(printf, 2, 3)));
static bool sql(MYSQL *db, const char *format, ...)
{
  char query[4096];
  va_list ap;
  int n;
  va_start(ap, format);
  n = vsnprintf(query, sizeof(query), format, ap);
  va_end(ap);
  return n >= 0 && (size_t)n < sizeof(query) && !mysql_query(db, query);
}
static bool integer(const char *s, int64_t *n)
{
  char *end;
  if (!s || !*s)
    return false;
  *n = strtoll(s, &end, 10);
  return !*end && *n >= 0;
}
enum clan_repository_result clan_web_read(MYSQL *db, const struct clan_progression_config *c,
                                          int64_t player, int64_t account, int64_t now,
                                          struct json_object **out)
{
  struct clan_web_snapshot s = {0};
  MYSQL_RES *res = NULL;
  MYSQL_ROW row;
  char uid[33];
  int64_t rank, ranks, n;
  enum clan_repository_result r;
  struct json_object *o = NULL;
  size_t i;
  if (!out || player < 1 || account < 1 || now < 0 ||
      clan_progression_config_validate(c, NULL) != CLAN_CONFIG_OK)
    return CLAN_REPO_INVALID;
  if (!c->controls.web)
  {
    *out = clan_web_status("disabled", "Clan web reports are disabled.", now);
    return CLAN_REPO_OK;
  }
  if (clan_repository_schema_check(db) != CLAN_REPO_OK)
    return CLAN_REPO_SCHEMA;
  if (db->server_status & SERVER_STATUS_IN_TRANS)
    return CLAN_REPO_CONFLICT;
  s.buff_storage_ready = clan_buff_lifecycle_schema_check(db) == CLAN_REPO_OK;
  if (!sql(db, "SET TRANSACTION ISOLATION LEVEL REPEATABLE READ") ||
      !sql(db, "START TRANSACTION WITH CONSISTENT SNAPSHOT"))
    return CLAN_REPO_UNAVAILABLE;
  r = CLAN_REPO_UNAVAILABLE;
  if (!sql(db, "SELECT store_mode FROM clan_store_control WHERE singleton=1") ||
      !(res = mysql_store_result(db)))
    goto done;
  row = mysql_fetch_row(res);
  if (!row || !integer(row[0], &n) || n != 2)
    goto done;
  mysql_free_result(res);
  res = NULL;
  if (!sql(db,
           "SELECT m.clan_uid,m.rank_id,JSON_LENGTH(JSON_EXTRACT(l.details,'$.ranks')) FROM "
           "clan_memberships m JOIN clan_progression_legacy l ON l.clan_uid=m.clan_uid JOIN "
           "clan_progression_clans c ON c.clan_uid=m.clan_uid WHERE m.player_id=%" PRId64
           " AND c.record_status=1 LIMIT 1",
           player) ||
      !(res = mysql_store_result(db)))
    goto done;
  row = mysql_fetch_row(res);
  if (!row)
  {
    o = clan_web_status("non_member", "Current accepted clan membership is required.", now);
    r = CLAN_REPO_OK;
    goto done;
  }
  if (!clan_progression_uid_valid(row[0]) || !integer(row[1], &rank) || !integer(row[2], &ranks) ||
      ranks < 1 || ranks > 15 || rank > ranks)
  {
    r = CLAN_REPO_SCHEMA;
    goto done;
  }
  strcpy(uid, row[0]);
  mysql_free_result(res);
  res = NULL;
  if (!rank)
  {
    o = clan_web_status("non_member", "Your clan application has not been accepted.", now);
    r = CLAN_REPO_OK;
    goto done;
  }
  s.buffs.accepted_member = true;
  r = clan_repository_load(db, uid, &s.buffs.clan);
  if (r != CLAN_REPO_OK)
    goto done;
  if (s.buff_storage_ready)
  {
    r = CLAN_REPO_UNAVAILABLE;
    if (!sql(db, "SELECT max_rank FROM clan_buff_management WHERE clan_uid='%s'", uid) ||
        !(res = mysql_store_result(db)))
      goto done;
    row = mysql_fetch_row(res);
    n = 0;
    if (row && (!integer(row[0], &n) || n > ranks))
    {
      r = CLAN_REPO_SCHEMA;
      goto done;
    }
    s.buffs.may_manage = player == s.buffs.clan.leader_id || (n > 0 && rank <= n);
    mysql_free_result(res);
    res = NULL;
    if (!sql(db,
             "SELECT "
             "activation_uid,family_id,tier,slot_id,rules_version,started_at,expires_at,next_"
             "available_at,paid_influence_eighths,canceled FROM clan_buff_families WHERE "
             "clan_uid='%s' ORDER BY family_id LIMIT 6",
             uid) ||
        !(res = mysql_store_result(db)))
      goto done;
    while ((row = mysql_fetch_row(res)))
    {
      struct clan_buff_state *b;
      int64_t v[9];
      if (s.buffs.count == 5 || !clan_progression_uid_valid(row[0]))
      {
        r = CLAN_REPO_SCHEMA;
        goto done;
      }
      for (i = 0; i < 9; i++)
        if (!integer(row[i + 1], &v[i]))
        {
          r = CLAN_REPO_SCHEMA;
          goto done;
        }
      if (v[0] > 5 || v[1] > 3 || v[2] > 3 || v[3] > INT32_MAX || v[8] > 1)
      {
        r = CLAN_REPO_SCHEMA;
        goto done;
      }
      b = &s.buffs.rows[s.buffs.count++];
      strcpy(b->clan_uid, uid);
      strcpy(b->activation_uid, row[0]);
      b->family = v[0];
      b->tier = v[1];
      b->slot = v[2];
      b->rules_version = v[3];
      b->started_at = v[4];
      b->expires_at = v[5];
      b->next_available_at = v[6];
      b->paid_influence_eighths = v[7];
      b->canceled = v[8];
    }
    mysql_free_result(res);
    res = NULL;
  }
  strcpy(s.favor.clan_uid, uid);
  s.favor.character_id = player;
  r = clan_repository_favor_load(db, player, uid, &s.favor);
  if (r != CLAN_REPO_OK && r != CLAN_REPO_NOT_FOUND)
    goto done;
  clan_progression_account_window_init(account, now, &s.account);
  r = clan_repository_account_window_load(db, account, now, &s.account);
  if (r != CLAN_REPO_OK && r != CLAN_REPO_NOT_FOUND)
    goto done;
  o = clan_web_render(c, &s, now);
  if (o && !strcmp(json_object_get_string(json_object_object_get(o, "status")), "ready"))
    json_object_object_add(o, "content", clan_web_content_read(db, c, uid, player, account, now));
  r = o ? CLAN_REPO_OK : CLAN_REPO_SCHEMA;
done:
  if (res)
    mysql_free_result(res);
  if (!sql(db, r == CLAN_REPO_OK ? "COMMIT" : "ROLLBACK"))
  {
    if (o)
      json_object_put(o);
    return CLAN_REPO_UNAVAILABLE;
  }
  if (r == CLAN_REPO_OK)
    *out = o;
  else if (o)
    json_object_put(o);
  return r;
}
