#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "interpreter.h"
#include "clan.h"
#include "clan_expedition_enrollment.h"
#include "clan_buff_runtime.h"
#include "clan_persistence.h"
#include "clan_status.h"
#include "mysql.h"
#include "crafting_new.h"
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
static bool authenticated(struct char_data *ch)
{
  return ch && !IS_NPC(ch) && ch->desc && STATE(ch->desc) == CON_PLAYING && ch->desc->account &&
         ch->desc->account->id > 0;
}
static struct clan_expedition_member identity(struct char_data *ch)
{
  struct clan_expedition_member m = {GET_IDNUM(ch), ch->desc->account->id, GET_LEVEL(ch)};
  return m;
}
/* Online presence is checked only at enrollment/start, never used as saved identity. */
static bool ready_actor(struct char_data *ch)
{
  return authenticated(ch) && IN_ROOM(ch) != NOWHERE && IN_ROOM(ch) <= top_of_world &&
         !FIGHTING(ch) && GET_CRAFT(ch).craft_duration <= 0;
}
ACMD(do_clanexpeditions)
{
  char command[24] = "list", arg[80] = "", extra[2], error[256];
  int tokens;
  size_t i;
  int64_t now = time(NULL);
  MYSQL_POOL_CONN *connection;
  struct clan_status_report report;
  struct clan_expedition_catalog catalog;
  struct clan_expedition_world refs = clan_expedition_world_refs();
  struct clan_progression_readiness ready = {.schema_ready = true,
                                             .schema_version = CLAN_PROGRESSION_SCHEMA_VERSION,
                                             .contracts_ready = true};
  const struct clan_progression_config *config = clan_buff_runtime_policy();
  struct clan_expedition_member self, members[6];
  struct json_object *root, *outing = NULL;
  enum clan_repository_result r;
  if (!authenticated(ch))
    return;
  tokens = sscanf(argument ? argument : "", "%23s %79s %1s", command, arg, extra);
  if (tokens > 2 ||
      ((!strcmp(command, "create") || !strcmp(command, "join") || !strcmp(command, "claim"))
           ? tokens != 2
           : tokens > 1) ||
      (strcmp(command, "list") && strcmp(command, "create") && strcmp(command, "join") &&
       strcmp(command, "active") && strcmp(command, "start") && strcmp(command, "leave") &&
       strcmp(command, "abandon") && strcmp(command, "claims") && strcmp(command, "claim")))
  {
    send_to_char(ch, "Usage: clan expeditions list | create <definition> | join <outing-id> | "
                     "active | start | leave | abandon | claims | claim <outing-id>\r\n");
    return;
  }
  if (!clan_persistence_sql_authority() || !clan_persistence_ready() || !mysql_available ||
      !(connection = mysql_pool_acquire()))
  {
    send_to_char(ch, "Expeditions require authoritative SQL storage.\r\n");
    return;
  }
  r = clan_repository_status_read(connection->conn, GET_IDNUM(ch), ch->desc->account->id, NULL,
                                  false, now, &report);
  ready.expeditions_ready = clan_expedition_progress_ready(connection->conn);
  if (r != CLAN_REPO_OK || !report.full_clan_access || report.membership != CLAN_ACCESS_ACCEPTED ||
      !clan_progression_feature_available(config, CLAN_FEATURE_EXPEDITIONS, &ready,
                                          report.clan.uid))
  {
    mysql_pool_release(connection);
    send_to_char(ch, "Expeditions are unavailable: accepted membership, enabled pilot expeditions "
                     "and validated storage are required.\r\n");
    return;
  }
  root = json_object_from_file("clan/expeditions.json");
  if (!root || !clan_expedition_catalog_parse(root, &catalog, error, sizeof(error)) ||
      !clan_expedition_catalog_validate(&catalog, &refs, error, sizeof(error)))
  {
    if (root)
      json_object_put(root);
    mysql_pool_release(connection);
    send_to_char(ch, "Expedition definitions/world references are unavailable.\r\n");
    return;
  }
  json_object_put(root);
  self = identity(ch);
  if (!strcmp(command, "list"))
  {
    mysql_pool_release(connection);
    send_to_char(
        ch,
        "Expeditions: one recruiting/active outing per clan; 2-6 distinct accepted accounts.\r\n");
    for (i = 0; i < catalog.count; i++)
      send_to_char(ch, "  %s - %s: clan level %d; character levels %d-%d.\r\n",
                   catalog.entries[i].id, catalog.entries[i].name,
                   catalog.entries[i].min_clan_level, catalog.entries[i].min_player_level,
                   catalog.entries[i].max_player_level);
    send_to_char(ch, "Create/join/start at a quartermaster, outside combat/crafting. Members opt "
                     "in individually. Start requires the full eligible roster in this room. "
                     "Expiry is the earlier of two hours from start or Monday 00:00 UTC.\r\n");
    return;
  }
  if (!strcmp(command, "claims"))
  {
    struct json_object *claims = NULL;
    r = clan_expedition_claim_list(connection->conn, report.clan.uid, self.player, self.account,
                                   now, &claims);
    mysql_pool_release(connection);
    if (r != CLAN_REPO_OK)
      send_to_char(ch, "Expedition claims are unavailable.\r\n");
    else
    {
      send_to_char(ch, "Your expedition completions (100 base Favor each, subject to weekly caps; "
                       "one per family/account/week):\r\n");
      for (i = 0; i < json_object_array_length(claims); i++)
      {
        struct json_object *v = json_object_array_get_idx(claims, i);
        send_to_char(ch, "  %s: %s; %s; claim expiry UTC epoch %" PRId64 ".\r\n",
                     str(v, "outing_uid"),
                     num(v, "family") == 1 ? "Stronghold Assault" : "Regional Hunt",
                     num(v, "state") == 2   ? "claimed"
                     : num(v, "state") == 1 ? "reward pending"
                     : num(v, "inactive")   ? "ineligible after departure"
                                            : "unclaimed",
                     num(v, "expires_at"));
      }
      json_object_put(claims);
    }
    return;
  }
  if (!strcmp(command, "claim"))
  {
    struct json_object *receipt = NULL, *paid = NULL;
    if (!ready_actor(ch) || !has_quartermaster_in_room(ch))
      r = CLAN_REPO_INELIGIBLE;
    else
      r = clan_expedition_claim_prepare(connection->conn, arg, report.clan.uid, &self, now, config,
                                        &receipt);
    if (r == CLAN_REPO_OK)
      r = clan_expedition_reward_complete(
          connection->conn, str(get(receipt, "event"), "operation_uid"), now, config, &paid);
    if (receipt)
      json_object_put(receipt);
    if (paid)
    {
      int64_t favor = num(get(get(paid, "decision"), "delta"), "favor_eighths");
      send_to_char(ch, "Expedition Favor credited: %.3f.\r\n", (double)favor / 8.0);
      json_object_put(paid);
    }
    mysql_pool_release(connection);
    if (r == CLAN_REPO_OK)
      clan_persistence_refresh_progression();
    else
      send_to_char(
          ch,
          "Expedition claim is not available (%s); accepted pending receipts may be retried.\r\n",
          clan_repository_result_name(r));
    return;
  }
  if (!strcmp(command, "create"))
  {
    char uid[33];
    if (!ready_actor(ch) || !has_quartermaster_in_room(ch))
      r = CLAN_REPO_INELIGIBLE;
    else if (!clan_repository_uid_generate(uid))
      r = CLAN_REPO_UNAVAILABLE;
    else
    {
      r = clan_expedition_create(connection->conn, &catalog, &refs, arg, uid, report.clan.uid,
                                 &self, now, config);
      if (r == CLAN_REPO_OK)
        send_to_char(
            ch, "Recruiting outing %s created. Share this ID with eligible clan members.\r\n", uid);
    }
  }
  else if (!strcmp(command, "join"))
  {
    if (!ready_actor(ch) || !has_quartermaster_in_room(ch))
      r = CLAN_REPO_INELIGIBLE;
    else
      r = clan_expedition_enroll(connection->conn, arg, report.clan.uid, &self, now, config);
  }
  else
  {
    r = clan_expedition_read(connection->conn, report.clan.uid, &outing);
    if (r == CLAN_REPO_OK && !strcmp(command, "active"))
    {
      send_to_char(ch, "Outing %s: %s; %s%s. Expires UTC epoch %" PRId64 ".\r\n",
                   str(outing, "outing_uid"), str(get(outing, "definition"), "name"),
                   num(outing, "state") == 0 ? "recruiting" : "started",
                   num(outing, "roster_locked") ? ", roster sealed" : "",
                   num(outing, "expires_at"));
      struct json_object *counts = NULL;
      if (clan_expedition_progress_read(connection->conn, str(outing, "outing_uid"), &counts) ==
          CLAN_REPO_OK)
      {
        struct json_object *objectives = get(get(outing, "definition"), "objectives");
        for (i = 0; i < 3; i++)
          send_to_char(ch, "  Objective %zu: %d / %" PRId64 " qualifying kills.\r\n", i + 1,
                       json_object_get_int(json_object_array_get_idx(counts, i)),
                       num(json_object_array_get_idx(objectives, i), "quota"));
        json_object_put(counts);
      }
      for (i = 0; i < json_object_array_length(get(outing, "roster")); i++)
      {
        struct json_object *v = json_object_array_get_idx(get(outing, "roster"), i);
        send_to_char(ch, "  Character %" PRId64 "%s\r\n", num(v, "player"),
                     num(v, "inactive") ? " [removed/ineligible]" : "");
      }
      if (now >= num(outing, "expires_at"))
        send_to_char(ch, "This outing has expired; it cannot start or earn credit.\r\n");
    }
    else if (r == CLAN_REPO_OK && !strcmp(command, "start"))
    {
      size_t n = 0;
      struct char_data *other;
      struct json_object *a = get(outing, "roster");
      if (!ready_actor(ch) || !has_quartermaster_in_room(ch))
        r = CLAN_REPO_INELIGIBLE;
      else
      {
        for (i = 0; i < json_object_array_length(a); i++)
        {
          struct json_object *v = json_object_array_get_idx(a, i);
          if (num(v, "inactive"))
            continue;
          for (other = world[IN_ROOM(ch)].people; other; other = other->next_in_room)
            if (ready_actor(other) && GET_IDNUM(other) == num(v, "player") &&
                other->desc->account->id == num(v, "account"))
              break;
          if (!other || n >= 6)
          {
            r = CLAN_REPO_INELIGIBLE;
            break;
          }
          members[n++] = identity(other);
        }
        if (r == CLAN_REPO_OK)
        {
          /* Revalidate frozen terms, even if the catalog changed after create. */
          struct json_object *frozen = json_object_new_object(), *a1 = json_object_new_array();
          struct clan_expedition_catalog saved;
          json_object_object_add(frozen, "version", json_object_new_int(1));
          json_object_array_add(a1, json_object_get(get(outing, "definition")));
          json_object_object_add(frozen, "expeditions", a1);
          if (!clan_expedition_catalog_parse(frozen, &saved, error, sizeof(error)) ||
              !clan_expedition_catalog_validate(&saved, &refs, error, sizeof(error)))
            r = CLAN_REPO_INELIGIBLE;
          else
            r = clan_expedition_start(connection->conn, str(outing, "outing_uid"), report.clan.uid,
                                      self.player, members, n, now, config);
          json_object_put(frozen);
        }
      }
    }
    else if (r == CLAN_REPO_OK)
      r = clan_expedition_leave(connection->conn, str(outing, "outing_uid"), report.clan.uid, &self,
                                !strcmp(command, "abandon"), now, config);
  }
  if (outing)
    json_object_put(outing);
  mysql_pool_release(connection);
  if (r == CLAN_REPO_OK)
  {
    if (strcmp(command, "active") && strcmp(command, "create"))
      send_to_char(ch, "Expedition %s recorded.\r\n", command);
  }
  else
    send_to_char(ch,
                 "Expedition %s rejected (%s). Check eligibility, roster, expiry and the "
                 "one-outing limit.\r\n",
                 command, clan_repository_result_name(r));
}
