#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "interpreter.h"
#include "clan.h"
#include "clan_objective_runtime.h"
#include "clan_buff_runtime.h"
#include "clan_persistence.h"
#include "clan_status.h"
#include "crafting_new.h"
#include "constants.h"
#include "mysql.h"
#include <inttypes.h>
#include "clan_display.h"
#define PENDING(ch, f)                                                                             \
  ((ch)->player_specials->clan_objective_pending[(f) == CLAN_SOURCE_GATHERING ? 0 : 1])
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
static bool configured(void)
{
  const struct clan_progression_config *c = clan_buff_runtime_policy();
  return clan_progression_campaign_supported() && c->controls.contracts &&
         c->controls.store_mode == CLAN_STORE_MYSQL && c->controls.mode == CLAN_PROGRESSION_LIVE &&
         clan_persistence_sql_authority();
}
static bool available(MYSQL *db, const char *uid)
{
  struct clan_progression_readiness ready = {.schema_ready = true,
                                             .schema_version = CLAN_PROGRESSION_SCHEMA_VERSION};
  ready.contracts_ready = clan_objective_schema_ready(db);
  return clan_progression_feature_available(clan_buff_runtime_policy(), CLAN_FEATURE_CONTRACTS,
                                            &ready, uid);
}
static bool earnings_allowed(const char *uid)
{
  struct clan_progression_readiness ready = {.schema_ready = true,
                                             .schema_version = CLAN_PROGRESSION_SCHEMA_VERSION};
  return clan_progression_feature_available(clan_buff_runtime_policy(), CLAN_FEATURE_EARNINGS,
                                            &ready, uid);
}
static bool authenticated(struct char_data *ch)
{
  return ch && !IS_NPC(ch) && ch->desc && STATE(ch->desc) == CON_PLAYING && ch->desc->account &&
         ch->desc->account->id > 0;
}
bool clan_objective_catalog_load(struct clan_objective_catalog *c)
{
  struct json_object *o = json_object_from_file("clan/objectives.json");
  size_t i;
  bool ok = o && clan_objective_catalog_parse(o, c);
  if (o)
    json_object_put(o);
  if (ok)
    for (i = 0; i < c->count; i++)
    {
      const struct clan_objective_definition *d = &c->entries[i];
      if (d->family == CLAN_SOURCE_GATHERING)
      {
        if (d->target >= NUM_CRAFT_MATS || material_grade(d->target) != d->grade)
          return false;
      }
      else if (real_mobile(d->target) == NOBODY)
        return false;
    }
  return ok;
}
static bool decode(struct char_data *ch, int family, struct clan_objective_event *e)
{
  char extra[2];
  memset(e, 0, sizeof(*e));
  e->player_id = GET_IDNUM(ch);
  return sscanf(PENDING(ch, family),
                "%32s %32s %32s %" SCNd64 " %" SCNd64 " %" SCNd64 " %d %d %d %d %1s", e->event_uid,
                e->contract_uid, e->clan_uid, &e->account_id, &e->occurred_at, &e->room, &e->family,
                &e->target, &e->grade, &e->base_units, extra) == 10 &&
         e->family == family && clan_progression_uid_valid(e->event_uid) &&
         clan_progression_uid_valid(e->contract_uid) && clan_progression_uid_valid(e->clan_uid);
}
static bool encode(struct char_data *ch, const struct clan_objective_event *e)
{
  int n = snprintf(PENDING(ch, e->family), 256,
                   "%s %s %s %" PRId64 " %" PRId64 " %" PRId64 " %d %d %d %d", e->event_uid,
                   e->contract_uid, e->clan_uid, e->account_id, e->occurred_at, e->room, e->family,
                   e->target, e->grade, e->base_units);
  return n > 0 && n < 256;
}
bool clan_objective_flush(struct char_data *ch)
{
  int family;
  bool changed = false;
  if (!ch || IS_NPC(ch) || !ch->player_specials)
    return true;
  for (family = CLAN_SOURCE_GATHERING; family <= CLAN_SOURCE_BOUNTY; family++)
    if (*PENDING(ch, family))
    {
      struct clan_objective_event e;
      MYSQL_POOL_CONN *connection;
      enum clan_repository_result r;
      struct json_object *receipt = NULL;
      if (!decode(ch, family, &e) || !clan_persistence_sql_authority() ||
          !clan_persistence_ready() || !mysql_available || !save_char_checked(ch, 0) ||
          !(connection = mysql_pool_acquire()))
        return false;
      r = clan_objective_progress(connection->conn, &e, time(NULL));
      if (r == CLAN_REPO_OK && earnings_allowed(e.clan_uid))
        clan_objective_reward_complete(connection->conn, e.contract_uid, time(NULL), &receipt);
      mysql_pool_release(connection);
      if (receipt)
      {
        struct json_object *delta = get(get(receipt, "decision"), "delta");
        send_to_char(
            ch, "Your %s contract is complete: %.3f lifetime Influence, %.3f Influence, %.3f Favor%s.\r\n",
            family == CLAN_SOURCE_GATHERING ? "gathering" : "bounty",
            num(delta, "xp_eighths") / 8.0, num(delta, "influence_eighths") / 8.0,
            num(delta, "favor_eighths") / 8.0, num(delta, "cap_flags") ? " (limits applied)" : "");
        json_object_put(receipt);
      }
      if (r != CLAN_REPO_OK && r != CLAN_REPO_INELIGIBLE && r != CLAN_REPO_DELETED)
        return false;
      if (r != CLAN_REPO_OK)
        send_to_char(ch, "Your saved contract occurrence no longer has an eligible active "
                         "objective; no credit was added.\r\n");
      *PENDING(ch, family) = 0;
      changed = true;
      if (!save_char_checked(ch, 0))
        return false;
    }
  if (changed)
    clan_persistence_refresh_progression();
  return true;
}
bool clan_objective_harvest_ready(struct char_data *ch)
{
  if (!clan_objective_flush(ch))
  {
    send_to_char(ch,
                 "A saved contract event is awaiting storage recovery; harvesting is paused.\r\n");
    return false;
  }
  if (configured() && (!mysql_available || !clan_persistence_ready()))
  {
    send_to_char(
        ch,
        "Contract storage is unavailable; harvesting is paused to preserve fresh progress.\r\n");
    return false;
  }
  return true;
}
bool clan_objective_harvest_begin(struct char_data *ch, int material,
                                  struct clan_objective_event *e)
{
  MYSQL_POOL_CONN *connection;
  struct clan_status_report report;
  enum clan_repository_result r;
  memset(e, 0, sizeof(*e));
  if (!clan_objective_harvest_ready(ch))
    return false;
  if (!configured() || !authenticated(ch) || material <= 0 || material >= NUM_CRAFT_MATS)
    return true;
  if (!(connection = mysql_pool_acquire()))
    return false;
  r = clan_repository_status_read(connection->conn, GET_IDNUM(ch), ch->desc->account->id, NULL,
                                  false, time(NULL), &report);
  bool admitted = r == CLAN_REPO_OK && report.full_clan_access &&
                  report.membership == CLAN_ACCESS_ACCEPTED && earnings_allowed(report.clan.uid);
  if (admitted && !available(connection->conn, report.clan.uid))
    r = CLAN_REPO_SCHEMA;
  if (r == CLAN_REPO_OK && admitted)
  {
    e->player_id = GET_IDNUM(ch);
    e->account_id = ch->desc->account->id;
    strlcpy(e->clan_uid, report.clan.uid, 33);
    e->family = CLAN_SOURCE_GATHERING;
    e->target = material;
    e->grade = material_grade(material);
    e->base_units = 1;
    e->occurred_at = time(NULL);
    e->room = GET_ROOM_VNUM(IN_ROOM(ch));
    if (!clan_repository_uid_generate(e->event_uid))
      r = CLAN_REPO_UNAVAILABLE;
    else
      r = clan_objective_capture(connection->conn, e, GET_LEVEL(ch));
  }
  mysql_pool_release(connection);
  if (r == CLAN_REPO_UNAVAILABLE || r == CLAN_REPO_COMMIT_UNKNOWN || r == CLAN_REPO_SCHEMA)
  {
    send_to_char(ch,
                 "Contract storage could not validate this occurrence; nothing was harvested.\r\n");
    return false;
  }
  if (r != CLAN_REPO_OK || !*e->contract_uid)
    memset(e, 0, sizeof(*e));
  return true;
}
static void record(struct char_data *ch, const struct clan_objective_event *e)
{
  if (!ch || IS_NPC(ch) || !*e->contract_uid)
    return;
  if (*PENDING(ch, e->family) && !clan_objective_flush(ch))
  {
    send_to_char(ch, "A prior saved contract event is still pending; this occurrence could not be "
                     "recorded.\r\n");
    return;
  }
  if (!encode(ch, e))
    return;
  /* Fresh material gains and their source marker share this checkpoint. A
  * failed save never feeds SQL. A later retry saves the same occurrence first. */
  if (!clan_objective_flush(ch))
    send_to_char(
        ch, "Contract progress is awaiting a safe checkpoint/retry when storage recovers.\r\n");
}
void clan_objective_harvest_finish(struct char_data *ch, struct clan_objective_event *e, int units)
{
  if (e && e->family == CLAN_SOURCE_GATHERING && units > 0 && units <= 20)
  {
    e->base_units = units;
    e->occurred_at = time(NULL);
    record(ch, e);
  }
}
bool clan_objective_bounty_capture(MYSQL *db, struct char_data *ch, int prototype,
                                   const char *encounter, const char *clan, int64_t now,
                                   struct clan_objective_event *e)
{
  memset(e, 0, sizeof(*e));
  if (!configured() || !authenticated(ch) || !available(db, clan))
    return false;
  strlcpy(e->event_uid, encounter, 33);
  strlcpy(e->clan_uid, clan, 33);
  e->player_id = GET_IDNUM(ch);
  e->account_id = ch->desc->account->id;
  e->family = CLAN_SOURCE_BOUNTY;
  e->target = prototype;
  e->grade = 0;
  e->base_units = 1;
  e->occurred_at = now;
  e->room = GET_ROOM_VNUM(IN_ROOM(ch));
  return clan_objective_capture(db, e, GET_LEVEL(ch)) == CLAN_REPO_OK;
}
void clan_objective_bounty_record(struct char_data *ch, const struct clan_objective_event *e)
{
  record(ch, e);
}
ACMD(do_clancontracts)
{
  char family_arg[24] = "", command[24] = "list", id[80] = "";
  int family, tokens;
  int64_t now = time(NULL);
  struct clan_objective_catalog catalog;
  struct clan_status_report report;
  MYSQL_POOL_CONN *connection;
  enum clan_repository_result r;
  struct json_object *o = NULL, *claims = NULL;
  size_t i;
  if (!authenticated(ch))
    return;
  tokens = sscanf(argument ? argument : "", "%23s %23s %79[^\r\n]", family_arg, command, id);
  family = !strcmp(family_arg, "gathering") ? CLAN_SOURCE_GATHERING
           : !strcmp(family_arg, "bounty")  ? CLAN_SOURCE_BOUNTY
                                            : 0;
  if (!family || tokens > 3 || (!strcmp(command, "accept") ? tokens != 3 : tokens > 2) ||
      (strcmp(command, "list") && strcmp(command, "active") && strcmp(command, "accept") &&
       strcmp(command, "abandon") && strcmp(command, "retry")))
  {
    send_to_char(ch, "Usage: clan contracts <gathering|bounty> list | active | accept <name> | "
                     "abandon | retry\r\n");
    return;
  }
  if (!clan_objective_flush(ch))
  {
    send_to_char(ch, "A contract occurrence is still awaiting safe storage recovery.\r\n");
    return;
  }
  if (!strcmp(command, "retry"))
  {
    send_to_char(ch, "Saved contract occurrences have been retried.\r\n");
    return;
  }
  if (!clan_persistence_sql_authority() || !clan_persistence_ready() || !mysql_available ||
      !(connection = mysql_pool_acquire()))
  {
    send_to_char(ch, "Contracts require available authoritative SQL storage.\r\n");
    return;
  }
  r = clan_repository_status_read(connection->conn, GET_IDNUM(ch), ch->desc->account->id, NULL,
                                  false, now, &report);
  bool enabled = r == CLAN_REPO_OK && report.full_clan_access &&
                 report.membership == CLAN_ACCESS_ACCEPTED &&
                 available(connection->conn, report.clan.uid) && clan_objective_catalog_load(&catalog);
  if (!enabled)
  {
    mysql_pool_release(connection);
    send_to_char(ch, "Gathering/bounty contracts are unavailable: accepted membership, available "
                     "contracts are required.\r\n");
    return;
  }
  r = clan_objective_expire(connection->conn, GET_IDNUM(ch), now);
  if (r != CLAN_REPO_OK)
  {
    mysql_pool_release(connection);
    send_to_char(ch, "Contract reset is unavailable; retry later.\r\n");
    return;
  }
  clan_objective_catalog_rotate(&catalog, now, GET_LEVEL(ch));
  if (!strcmp(command, "list"))
  {
    struct clan_reset_period rotation_week;
    if (clan_progression_week_at(now, &rotation_week))
      send_to_char(ch, "\r\n\tCClan Contracts\tn\r\n  New choices: %s\r\n",
                   clan_display_time(rotation_week.ends_at, (char[64]){0}));
    r = clan_objective_claims(connection->conn, ch->desc->account->id, family, now, &claims);
    mysql_pool_release(connection);
    if (r != CLAN_REPO_OK)
    {
      send_to_char(ch, "Contract availability cannot be read safely.\r\n");
      return;
    }
    int clan_level = 0;
    clan_award_level(clan_buff_runtime_policy(), report.clan.xp_eighths, &clan_level);
    send_to_char(ch, "  %s  |  %zu / 2 completed this week\r\n"
                     "  Reward: 12.5 Favor and 6.25 Influence, before limits.\r\n\r\n",
                 family == CLAN_SOURCE_GATHERING ? "Gathering" : "Bounties",
                 json_object_array_length(claims));
    for (i = 0; i < catalog.count; i++)
      if (catalog.entries[i].family == family)
      {
        struct clan_objective_definition *d = &catalog.entries[i];
        size_t j;
        bool done = false;
        for (j = 0; j < json_object_array_length(claims); j++)
          if (!strcmp(d->id, json_object_get_string(json_object_array_get_idx(claims, j))))
            done = true;
        send_to_char(ch, "  \tW%s\tn\r\n    Goal: 20 %s; ", d->name,
                     family == CLAN_SOURCE_GATHERING ? "fresh base harvest units"
                                                     : "qualifying designated kills");
        if (family == CLAN_SOURCE_GATHERING)
          send_to_char(ch, "%s, exact grade %d; ", crafting_materials[d->target], d->grade);
        else
          send_to_char(ch, "the named target; ");
        send_to_char(ch, "clan level %d, character levels %d-%d%s%s.\r\n", d->min_clan_level,
                     d->min_player_level, d->max_player_level,
                     done                                    ? " [completed]"
                     : json_object_array_length(claims) >= 2 ? " [account limit]"
                                                             : "",
                     clan_level < d->min_clan_level || GET_LEVEL(ch) < d->min_player_level ||
                             GET_LEVEL(ch) > d->max_player_level
                         ? " [locked]"
                         : "");
      }
    send_to_char(
        ch, "Accept by name: clan contracts %s accept <name>\r\n"
            "Visit a quartermaster to accept. Failed/bonus harvests "
            "do not count; bounties require participation, not a killing blow.\r\n", family_arg);
    if (family == CLAN_SOURCE_BOUNTY)
      send_to_char(ch, "The first completion of each bounty also earns 12.5 shared Influence "
                       "(up to two bounties per clan each week).\r\n");
    json_object_put(claims);
    return;
  }
  if (!strcmp(command, "accept"))
  {
    char uid[33];
    int best = 0, choice = -1;
    for (i = 0; i < catalog.count; i++) if (catalog.entries[i].family == family) {
      int score = clan_display_match(id, catalog.entries[i].name, catalog.entries[i].id);
      if (score > best) { best = score; choice = (int)i; }
      else if (score && score == best) choice = -2;
    }
    if (choice < 0) {
      mysql_pool_release(connection);
      send_to_char(ch, choice == -2 ? "That name matches several contracts. Please use more of the name.\r\n"
                                    : "No contract by that name. Use clan contracts %s list.\r\n", family_arg);
      return;
    }
    i = (size_t)choice;
    if (!has_quartermaster_in_room(ch) || FIGHTING(ch) || GET_CRAFT(ch).craft_duration > 0)
      r = CLAN_REPO_INELIGIBLE;
    else if (i == catalog.count || !clan_repository_uid_generate(uid))
      r = CLAN_REPO_INVALID;
    else
      r = clan_objective_accept(connection->conn, &catalog.entries[i], uid, report.clan.uid,
                                GET_IDNUM(ch), ch->desc->account->id, GET_LEVEL(ch), now);
  }
  else if (!strcmp(command, "abandon"))
    r = clan_objective_abandon(connection->conn, GET_IDNUM(ch), family);
  else
    r = clan_objective_active(connection->conn, GET_IDNUM(ch), family, &o);
  mysql_pool_release(connection);
  if (r != CLAN_REPO_OK)
    send_to_char(ch,
                 "Contract request rejected (%s). Check quartermaster, membership, level, active "
                 "slot and weekly limits.\r\n",
                 clan_repository_result_name(r));
  else if (o)
  {
    send_to_char(ch,
                 "\r\n\tW%s\tn\r\n  Progress: %" PRId64 " / 20  (%" PRId64 " remaining)\r\n  Expires: %s"
                 "%s%s.\r\n",
                 str(get(o, "definition"), "name"), num(o, "progress"), 20 - num(o, "progress"),
                 clan_display_time(num(o, "expires_at"), (char[64]){0}),
                 now >= num(o, "expires_at") && num(o, "state") == 0 ? " [expired]" : "",
                 num(o, "state") == 1 ? " [reward pending]" : "");
    json_object_put(o);
  }
  else
    send_to_char(ch, "%s\r\n", !strcmp(command, "accept") ? "Contract accepted. Your progress will save automatically." : "Contract abandoned.");
}
