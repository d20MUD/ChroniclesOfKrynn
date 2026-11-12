#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "clan.h"
#include "clan_delivery.h"
#include "clan_delivery_runtime.h"
#include "clan_buff_runtime.h"
#include "clan_persistence.h"
#include "crafting_new.h"
#include "constants.h"
#include "spec_abilities.h"
#include "mysql.h"
#include <inttypes.h>
#include <limits.h>
#define PS(ch) ((ch)->player_specials)
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
static bool terms_valid(const struct clan_delivery_definition *d)
{
  return clan_delivery_definition_valid(d) &&
         (d->kind != CLAN_DELIVERY_MATERIAL ||
          (d->resource < NUM_CRAFT_MATS && material_grade(d->resource) == d->grade));
}
static bool catalog_load(struct clan_delivery_catalog *c)
{
  struct json_object *o = json_object_from_file("clan/deliveries.json");
  size_t i;
  bool ok = o && clan_delivery_catalog_parse(o, c);
  if (o)
    json_object_put(o);
  if (ok)
    for (i = 0; i < c->count; i++)
      if (!terms_valid(&c->entries[i]))
        return false;
  return ok;
}
static bool ammo_eligible(struct obj_data *o, const struct clan_delivery_definition *d)
{
  return o && GET_OBJ_TYPE(o) == ITEM_MISSILE && GET_OBJ_VAL(o, 0) == d->resource &&
         GET_OBJ_VAL(o, 4) >= d->min_enhancement && GET_OBJ_VAL(o, AMMO_VAL_MAX) > 0 &&
         GET_OBJ_VAL(o, AMMO_VAL_QUANTITY) > 0 &&
         GET_OBJ_VAL(o, AMMO_VAL_QUANTITY) <= GET_OBJ_VAL(o, AMMO_VAL_MAX) &&
         !GET_OBJ_VAL(o, AMMO_VAL_UNLIMITED) && !GET_OBJ_VAL(o, AMMO_VAL_RETURNING) &&
         !obj_has_special_ability(o, WEAPON_SPECAB_RETURNING) && !*o->clan_delivery_uid &&
         !o->estate_delivery_id && !o->legendary_quarantined;
}
/* Only directly carried bundles or the equipped ammo pouch. Nested bags and
 * other containers are excluded, so the displayed source is predictable. */
static int ammo_collect(struct char_data *ch, const struct clan_delivery_definition *d,
                        struct obj_data **items, int *amounts)
{
  struct obj_data *o, *pouch = GET_EQ(ch, WEAR_AMMO_POUCH);
  int n = 0, left = d->quantity, pass;
  for (pass = 0; pass < 2; pass++)
    for (o = pass ? ch->carrying : (pouch ? pouch->contains : NULL); o && left; o = o->next_content)
    {
      if (n >= 100)
        return 0;
      if (ammo_eligible(o, d))
      {
        items[n] = o;
        amounts[n] = MIN(left, GET_OBJ_VAL(o, AMMO_VAL_QUANTITY));
        left -= amounts[n++];
      }
    }
  return left ? 0 : n;
}
static bool goods_available(struct char_data *ch, const struct clan_delivery_definition *d)
{
  struct obj_data *items[100];
  int amounts[100];
  return terms_valid(d) &&
         (d->kind == CLAN_DELIVERY_MATERIAL ? GET_CRAFT_MAT(ch, d->resource) >= d->quantity
                                            : ammo_collect(ch, d, items, amounts) > 0);
}
/* Includes zero-shot placeholders. They must survive until the terminal SQL
 * receipt; extracting them earlier would destroy the exact refund identity. */
static bool ammo_markers(struct char_data *ch, const char *uid, bool refund, bool cleanup)
{
  struct obj_data *o, *next, *pouch = GET_EQ(ch, WEAR_AMMO_POUCH);
  int total = 0, pass;
  for (pass = 0; pass < 2; pass++)
    for (o = pass ? ch->carrying : (pouch ? pouch->contains : NULL); o; o = o->next_content)
    {
      if (!strcmp(o->clan_delivery_uid, uid))
      {
        if (o->clan_delivery_amount < 1 || o->clan_delivery_amount > 100 ||
            GET_OBJ_TYPE(o) != ITEM_MISSILE || GET_OBJ_VAL(o, AMMO_VAL_QUANTITY) < 0 ||
            GET_OBJ_VAL(o, AMMO_VAL_QUANTITY) > INT_MAX - o->clan_delivery_amount)
          return false;
        total += o->clan_delivery_amount;
      }
    }
  if (total != 100)
    return false;
  if (!refund && !cleanup)
    return true;
  for (pass = 0; pass < 2; pass++)
    for (o = pass ? ch->carrying : (pouch ? pouch->contains : NULL); o; o = next)
    {
      next = o->next_content;
      if (strcmp(o->clan_delivery_uid, uid))
        continue;
      if (refund)
        GET_OBJ_VAL(o, AMMO_VAL_QUANTITY) += o->clan_delivery_amount;
      *o->clan_delivery_uid = 0;
      o->clan_delivery_amount = 0;
      if (cleanup && GET_OBJ_VAL(o, AMMO_VAL_QUANTITY) == 0)
        extract_obj(o);
    }
  return true;
}
static bool checkpoint(struct char_data *ch, int kind)
{
  return kind == CLAN_DELIVERY_MATERIAL ? save_char_checked(ch, 0) : Crash_crashsave_checked(ch);
}
static bool goods_pay(struct char_data *ch, const struct clan_delivery_definition *d,
                      const char *uid)
{
  struct obj_data *items[100];
  int amounts[100], i, n;
  if (!goods_available(ch, d))
    return false;
  if (d->kind == CLAN_DELIVERY_MATERIAL)
  {
    GET_CRAFT_MAT(ch, d->resource) -= d->quantity;
    PS(ch)->clan_delivery_stage = 1;
  }
  else
  {
    n = ammo_collect(ch, d, items, amounts);
    if (!n)
      return false;
    for (i = 0; i < n; i++)
    {
      GET_OBJ_VAL(items[i], AMMO_VAL_QUANTITY) -= amounts[i];
      strlcpy(items[i]->clan_delivery_uid, uid, 33);
      items[i]->clan_delivery_amount = amounts[i];
    }
    strlcpy(PS(ch)->clan_delivery_inventory, uid, 33);
    PS(ch)->clan_delivery_inventory_stage = 1;
  }
  return checkpoint(ch, d->kind);
}
static bool recovery_error(struct char_data *ch)
{
  send_to_char(ch, "Your clan delivery is awaiting safe storage recovery. No new command was "
                   "performed; try again shortly or contact staff.\r\n");
  return false;
}
bool clan_delivery_recover(struct char_data *ch)
{
  MYSQL_POOL_CONN *connection;
  struct json_object *op = NULL, *receipt = NULL;
  struct clan_delivery_definition d;
  enum clan_repository_result r, completion_result;
  char uid[33];
  int stage, terminal;
  bool committed;
  int64_t now = time(NULL);
  if (!ch || IS_NPC(ch) || !ch->player_specials)
    return true;
  if (PS(ch)->clan_delivery_checked && !*PS(ch)->clan_delivery_lock)
    return true;
  if (!clan_persistence_sql_authority())
    return *PS(ch)->clan_delivery_lock ? recovery_error(ch) : true;
  if (!mysql_available || !(connection = mysql_pool_acquire()))
    return recovery_error(ch);
  r = clan_delivery_operation(connection->conn, GET_IDNUM(ch),
                              *PS(ch)->clan_delivery_lock ? PS(ch)->clan_delivery_lock : NULL, &op);
  mysql_pool_release(connection);
  if (r == CLAN_REPO_NOT_FOUND || r == CLAN_REPO_SCHEMA)
  {
    if (*PS(ch)->clan_delivery_lock)
    {
      /* Only a durable unpaid intent can safely lack its SQL participant. */
      if (r == CLAN_REPO_SCHEMA || PS(ch)->clan_delivery_stage != 0 ||
          !strcmp(PS(ch)->clan_delivery_inventory, PS(ch)->clan_delivery_lock))
        return recovery_error(ch);
      *PS(ch)->clan_delivery_lock = 0;
      if (!save_char_checked(ch, 0))
        return recovery_error(ch);
    }
    PS(ch)->clan_delivery_checked = true;
    return true;
  }
  if (r != CLAN_REPO_OK)
    return recovery_error(ch);
  strlcpy(uid, str(op, "operation_uid"), 33);
  if (!clan_delivery_definition_parse(get(op, "definition"), &d) || !terms_valid(&d))
  {
    json_object_put(op);
    return recovery_error(ch);
  }
  stage = d.kind == CLAN_DELIVERY_MATERIAL ? PS(ch)->clan_delivery_stage
                                           : (!strcmp(PS(ch)->clan_delivery_inventory, uid)
                                                  ? PS(ch)->clan_delivery_inventory_stage
                                                  : 0);
  terminal = num(op, "operation_state");
  if (!mysql_available || !(connection = mysql_pool_acquire()))
  {
    json_object_put(op);
    return recovery_error(ch);
  }
  r = clan_reward_load(connection->conn, uid, &receipt);
  mysql_pool_release(connection);
  if (r != CLAN_REPO_OK)
  {
    json_object_put(op);
    return recovery_error(ch);
  }
  committed = num(receipt, "status") == 1;
  if (!committed && num(receipt, "status") == 0 && stage == 1)
  {
    if ((d.kind == CLAN_DELIVERY_AMMO && !ammo_markers(ch, uid, false, false)) ||
        !checkpoint(ch, d.kind))
      goto blocked;
    if (!(connection = mysql_pool_acquire()))
      goto blocked;
    json_object_put(receipt);
    receipt = NULL;
    r = clan_delivery_complete(connection->conn, op, now, &receipt);
    completion_result = r;
    mysql_pool_release(connection);
    if (r != CLAN_REPO_OK)
    {
      /* Resolve ambiguous commit before making any refund decision. */
      if (receipt)
      {
        json_object_put(receipt);
        receipt = NULL;
      }
      if (!(connection = mysql_pool_acquire()))
        goto blocked;
      r = clan_reward_load(connection->conn, uid, &receipt);
      mysql_pool_release(connection);
      if (r != CLAN_REPO_OK)
        goto blocked;
      /* Transient infrastructure failures retain paid goods for retry. */
      if (num(receipt, "status") == 0 &&
          (completion_result != CLAN_REPO_INVALID && completion_result != CLAN_REPO_INELIGIBLE &&
           completion_result != CLAN_REPO_DELETED))
        goto blocked;
    }
    committed = num(receipt, "status") == 1;
  }
  if (committed)
  {
    if (stage == 1)
    {
      if (d.kind == CLAN_DELIVERY_AMMO && !ammo_markers(ch, uid, false, true))
        goto blocked;
      if (d.kind == CLAN_DELIVERY_MATERIAL)
        PS(ch)->clan_delivery_stage = 3;
      else
        PS(ch)->clan_delivery_inventory_stage = 3;
      stage = 3;
    }
    if (stage != 3 || !checkpoint(ch, d.kind))
      goto blocked;
  }
  else
  {
    if (stage == 1)
    {
      if (d.kind == CLAN_DELIVERY_MATERIAL)
      {
        if (GET_CRAFT_MAT(ch, d.resource) > INT_MAX - d.quantity)
          goto blocked;
        GET_CRAFT_MAT(ch, d.resource) += d.quantity;
        PS(ch)->clan_delivery_stage = 2;
      }
      else
      {
        if (!ammo_markers(ch, uid, true, false))
          goto blocked;
        PS(ch)->clan_delivery_inventory_stage = 2;
      }
      stage = 2;
    }
    if (stage != 0 && stage != 2)
      goto blocked;
    if (!checkpoint(ch, d.kind))
      goto blocked;
    if (!(connection = mysql_pool_acquire()))
      goto blocked;
    r = clan_delivery_cancel(connection->conn, uid, now);
    mysql_pool_release(connection);
    if (r != CLAN_REPO_OK)
      goto blocked;
  }
  /* File acknowledgements precede releasing SQL's per-character hold. */
  if (committed && terminal != 2)
  {
    if (!(connection = mysql_pool_acquire()))
      goto blocked;
    r = clan_delivery_ack(connection->conn, uid);
    mysql_pool_release(connection);
    if (r != CLAN_REPO_OK)
      goto blocked;
  }
  *PS(ch)->clan_delivery_lock = 0;
  PS(ch)->clan_delivery_stage = 0;
  if (!save_char_checked(ch, 0))
    goto blocked;
  PS(ch)->clan_delivery_checked = true;
  if (committed)
  {
    clan_persistence_refresh_progression();
    send_to_char(ch, "Clan delivery complete. Its recorded reward has been applied once.\r\n");
  }
  else
    send_to_char(ch, "Clan delivery canceled safely; unpaid or refunded goods were retained. "
                     "Accept a new contract to try again.\r\n");
  json_object_put(receipt);
  json_object_put(op);
  return true;
blocked:
  /* Keep recovery enabled even if the final player-file write was ambiguous. */
  PS(ch)->clan_delivery_checked = false;
  if (receipt)
    json_object_put(receipt);
  json_object_put(op);
  return recovery_error(ch);
}
static void requirements(struct char_data *ch, const struct clan_delivery_definition *d)
{
  static const char *ammo[] = {"", "arrows", "bolts", "bullets"};
  if (d->kind == CLAN_DELIVERY_MATERIAL)
    send_to_char(ch, "%d units of %s (exact grade %d)", d->quantity,
                 crafting_materials[d->resource], d->grade);
  else
    send_to_char(ch,
                 "%d remaining finite shots of %s, enhancement +%d or better (no "
                 "infinite/Returning ammunition)",
                 d->quantity, ammo[d->resource], d->min_enhancement);
}
ACMDU(do_clandeliveries)
{
  char command[32] = "list", arg[80] = "", extra[2];
  int tokens;
  int64_t now = time(NULL);
  MYSQL_POOL_CONN *connection;
  struct clan_status_report report;
  struct clan_delivery_catalog catalog;
  struct clan_progression_readiness ready = {.schema_ready = true,
                                             .schema_version = CLAN_PROGRESSION_SCHEMA_VERSION};
  const struct clan_progression_config *config = clan_buff_runtime_policy();
  enum clan_repository_result r;
  struct json_object *o = NULL, *receipt = NULL;
  struct clan_delivery_definition d;
  size_t i;
  if (!ch || IS_NPC(ch) || !ch->desc || !ch->desc->account || ch->desc->account->id <= 0)
    return;
  if (!clan_delivery_recover(ch))
    return;
  tokens = sscanf(argument ? argument : "", "%31s %79s %1s", command, arg, extra);
  if (tokens > 2)
    goto usage;
  if (!strcmp(command, "retry"))
  {
    PS(ch)->clan_delivery_checked = false;
    clan_delivery_recover(ch);
    return;
  }
  if (strcmp(command, "list") && strcmp(command, "active") && strcmp(command, "accept") &&
      strcmp(command, "abandon") && strcmp(command, "turnin") && strcmp(command, "confirm"))
    goto usage;
  if ((!strcmp(command, "accept") || !strcmp(command, "confirm")) ? tokens != 2 : tokens == 2)
    goto usage;
  if (!clan_persistence_sql_authority() || !clan_persistence_ready() || !mysql_available ||
      !(connection = mysql_pool_acquire()))
  {
    send_to_char(ch, "Clan contracts require available authoritative SQL storage.\r\n");
    return;
  }
  ready.contracts_ready = clan_delivery_schema_ready(connection->conn) && catalog_load(&catalog);
  r = clan_repository_status_read(connection->conn, GET_IDNUM(ch), ch->desc->account->id, NULL,
                                  false, now, &report);
  mysql_pool_release(connection);
  if (r != CLAN_REPO_OK || !report.full_clan_access || report.membership != CLAN_ACCESS_ACCEPTED ||
      !clan_progression_feature_available(config, CLAN_FEATURE_CONTRACTS, &ready, report.clan.uid))
  {
    send_to_char(ch, "Delivery contracts are unavailable: accepted membership, enabled pilot "
                     "contracts, and validated definitions/storage are required.\r\n");
    return;
  }
  r = clan_delivery_expire(connection->conn, GET_IDNUM(ch), now);
  if (r != CLAN_REPO_OK)
  {
    mysql_pool_release(connection);
    send_to_char(ch, "Contract reset is unavailable; retry later.\r\n");
    return;
  }
  clan_delivery_catalog_rotate(&catalog, now);
  if (!strcmp(command, "list"))
  {
    struct clan_reset_period rotation_week;
    if (clan_progression_week_at(now, &rotation_week))
      send_to_char(ch, "Weekly choices rotate at UTC epoch %" PRId64
                       ". Unfinished contracts expire; pending rewards recover in their original week.\r\n",
                   rotation_week.ends_at);
    int level = 0;
    clan_award_level(config, report.clan.xp_eighths, &level);
    if (!(connection = mysql_pool_acquire()))
    {
      recovery_error(ch);
      return;
    }
    r = clan_delivery_claim_history(connection->conn, ch->desc->account->id, now, &o);
    mysql_pool_release(connection);
    if (r != CLAN_REPO_OK)
    {
      send_to_char(ch, "Delivery availability cannot be read safely.\r\n");
      return;
    }
    send_to_char(ch, "Account delivery completions this UTC week: %zu / 2.\r\n",
                 json_object_array_length(o));
    send_to_char(ch, "Delivery contracts: one active delivery slot. Two DIFFERENT definitions per "
                     "account per UTC week, across clans/alts.\r\n"
                     "Base credit: 100 clan XP / 50 Influence / 50 Favor; account, clan and Favor "
                     "caps can reduce this. Preview before confirming.\r\n");
    for (i = 0; i < catalog.count; i++)
    {
      size_t j;
      bool claimed = false;
      d = catalog.entries[i];
      for (j = 0; j < json_object_array_length(o); j++)
        if (!strcmp(d.id, json_object_get_string(json_object_array_get_idx(o, j))))
          claimed = true;
      send_to_char(ch, "  %s - %s [clan level %d%s%s]: ", d.id, d.name, d.min_level,
                   level < d.min_level ? ", locked" : "",
                   claimed                            ? ", completed this week"
                   : json_object_array_length(o) >= 2 ? ", account limit reached"
                                                      : "");
      requirements(ch, &d);
      send_to_char(ch, ".\r\n");
    }
    json_object_put(o);
    o = NULL;
    send_to_char(
        ch,
        "Accept/turn in at a quartermaster. Purchased or transferred goods qualify. Ammo is taken "
        "from your equipped pouch and direct inventory.\r\n"
        "clan deliveries accept <id> | active | abandon | turnin | confirm <quote-id> | retry\r\n");
    return;
  }
  if (strcmp(command, "active") && strcmp(command, "abandon") &&
      (!has_quartermaster_in_room(ch) || FIGHTING(ch) || GET_CRAFT(ch).craft_duration > 0))
  {
    send_to_char(ch, "Visit a quartermaster, outside combat and timed crafting, to accept or turn "
                     "in deliveries.\r\n");
    return;
  }
  if (!strcmp(command, "confirm"))
  {
    o = json_tokener_parse(PS(ch)->clan_delivery_quote);
    if (!o || strcmp(arg, str(o, "operation_uid")) || now >= num(o, "confirm_before") ||
        !clan_delivery_definition_parse(get(o, "definition"), &d) || !goods_available(ch, &d) ||
        num(o, "player_id") != GET_IDNUM(ch) || num(o, "account_id") != ch->desc->account->id ||
        strcmp(str(o, "clan_uid"), report.clan.uid))
    {
      if (o)
        json_object_put(o);
      send_to_char(ch, "Quote expired or goods/membership changed. Use clan deliveries turnin for "
                       "a new preview. Nothing was consumed.\r\n");
      return;
    }
    strlcpy(PS(ch)->clan_delivery_lock, arg, 33);
    PS(ch)->clan_delivery_stage = 0;
    PS(ch)->clan_delivery_checked = false;
    if (!save_char_checked(ch, 0))
    {
      json_object_put(o);
      recovery_error(ch);
      return;
    }
    if (!(connection = mysql_pool_acquire()))
    {
      json_object_put(o);
      recovery_error(ch);
      return;
    }
    r = clan_delivery_prepare(connection->conn, o, now, &receipt);
    mysql_pool_release(connection);
    if (receipt)
      json_object_put(receipt);
    *PS(ch)->clan_delivery_quote = 0;
    if (r == CLAN_REPO_OK)
      goods_pay(ch, &d, arg);
    json_object_put(o);
    clan_delivery_recover(ch);
    return;
  }
  if (!(connection = mysql_pool_acquire()))
  {
    recovery_error(ch);
    return;
  }
  if (!strcmp(command, "accept"))
  {
    char uid[33];
    for (i = 0; i < catalog.count; i++)
      if (!strcmp(catalog.entries[i].id, arg))
        break;
    if (i == catalog.count || !clan_repository_uid_generate(uid))
      r = CLAN_REPO_INVALID;
    else
      r = clan_delivery_accept(connection->conn, &catalog.entries[i], uid, report.clan.uid,
                               GET_IDNUM(ch), ch->desc->account->id, now);
  }
  else if (!strcmp(command, "abandon"))
    r = clan_delivery_abandon(connection->conn, GET_IDNUM(ch), now);
  else if (!strcmp(command, "active"))
    r = clan_delivery_active(connection->conn, GET_IDNUM(ch), now, &o);
  else
    r = clan_delivery_quote(connection->conn, GET_IDNUM(ch), ch->desc->account->id, now, &o);
  mysql_pool_release(connection);
  if (r != CLAN_REPO_OK)
    send_to_char(ch,
                 "Delivery request rejected (%s). Check your active slot, weekly limits, "
                 "membership and caps. Nothing was consumed.\r\n",
                 clan_repository_result_name(r));
  else if (!o)
    send_to_char(ch, "Delivery %s recorded. No goods consumed or rewards awarded.\r\n", command);
  else if (clan_delivery_definition_parse(get(o, "definition"), &d) && terms_valid(&d))
  {
    send_to_char(ch, "%s: ", d.name);
    requirements(ch, &d);
    send_to_char(ch, ". Expires at UTC epoch %" PRId64 "%s.\r\n", num(o, "expires_at"),
                 now >= num(o, "expires_at") ? " [expired]" : "");
    if (!strcmp(command, "turnin"))
    {
      struct json_object *delta = get(o, "delta");
      const char *encoded = json_object_to_json_string_ext(o, JSON_C_TO_STRING_PLAIN);
      if (!goods_available(ch, &d))
        send_to_char(ch, "You do not have the required eligible goods. Nothing was consumed.\r\n");
      else if (strlen(encoded) < sizeof(PS(ch)->clan_delivery_quote))
      {
        strlcpy(PS(ch)->clan_delivery_quote, encoded, sizeof(PS(ch)->clan_delivery_quote));
        send_to_char(ch,
                     "Exact credit: %.3f clan XP / %.3f Influence / %.3f Favor%s.\r\nConfirm "
                     "within 60 seconds: clan deliveries confirm %s\r\n",
                     num(delta, "xp_eighths") / 8.0, num(delta, "influence_eighths") / 8.0,
                     num(delta, "favor_eighths") / 8.0,
                     num(delta, "cap_flags") ? " (reduced by caps)" : "", str(o, "operation_uid"));
      }
    }
  }
  if (o)
    json_object_put(o);
  return;
usage:
  send_to_char(ch, "Usage: clan deliveries list | accept <id> | active | abandon | turnin | "
                   "confirm <quote-id> | retry\r\n");
}
