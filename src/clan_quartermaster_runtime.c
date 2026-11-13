#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "constants.h"
#include "class.h"
#include "clan.h"
#include "clan_quartermaster.h"
#include "clan_awards.h"
#include <limits.h>
#include "clan_buff_runtime.h"
#include "clan_persistence.h"
#include "crafting_new.h"
#include "estate.h"
#include <inttypes.h>
#include "clan_display.h"
#define PS(ch) ((ch)->player_specials)
static struct json_object *get(struct json_object *o, const char *k)
{
  struct json_object *v = NULL;
  if (o)
    json_object_object_get_ex(o, k, &v);
  return v;
}
static const char *str(struct json_object *o, const char *k)
{
  const char *v = json_object_get_string(get(o, k));
  return v ? v : "";
}
static int64_t num(struct json_object *o, const char *k)
{
  return json_object_get_int64(get(o, k));
}
/* Initial budget deliberately admits no spells, affects, scripts or enhancement.
 * Future activated utility must receive its own explicit budget/catalog revision. */
static bool template_valid(const struct clan_qm_item *d, struct obj_data *o, bool purchased)
{
  int i;
  if (!o || GET_OBJ_VNUM(o) != (obj_vnum)d->prototype || GET_OBJ_TYPE(o) != d->type ||
      GET_OBJ_LEVEL(o) != d->level || GET_OBJ_WEIGHT(o) != d->weight ||
      GET_OBJ_COST(o) != (purchased ? 0 : d->value) || o->has_spells || o->special_abilities ||
      o->proto_script || o->script || o->contains || o->sheath_primary || o->sheath_secondary ||
      o->trap || o->sbinfo || o->tinker_bonus || *o->legendary_uuid || o->legendary_quarantined ||
      obj_index[GET_OBJ_RNUM(o)].func)
    return false;
  for (i = 0; i < 16; i++)
    if (GET_OBJ_VAL(o, i) != d->values[i])
      return false;
  for (i = 0; i < MAX_OBJ_AFFECT; i++)
    if (o->affected[i].modifier)
      return false;
  for (i = 0; i < AF_ARRAY_MAX; i++)
    if (o->obj_flags.bitvector[i] || o->obj_flags.bitvector2[i])
      return false;
  for (i = 0; i < MAX_WEAPON_SPELLS; i++)
    if (o->wpn_spells[i].spellnum)
      return false;
  for (i = 0; i < MAX_WEAPON_CHANNEL_SPELLS; i++)
    if (o->channel_spells[i].spellnum)
      return false;
  for (i = 0; i < 5; i++)
    if (o->activate_spell[i])
      return false;
  return true;
}
bool clan_qm_catalog_load(struct clan_qm_catalog *c)
{
  struct json_object *o = json_object_from_file("clan/quartermaster.json");
  size_t i;
  bool ok = o && clan_qm_parse(o, c);
  if (o)
    json_object_put(o);
  if (ok)
    for (i = 0; i < c->count; i++)
    {
      obj_rnum n = real_object(c->entries[i].prototype);
      if (n == NOTHING || !template_valid(&c->entries[i], &obj_proto[n], false))
        return false;
    }
  return ok;
}
void clan_qm_boot_validate(void)
{
  struct clan_qm_catalog c;
  if (clan_qm_catalog_load(&c))
    log("CLAN: validated %zu quartermaster templates (purchases remain gated)", c.count);
  else
    log("CLAN: quartermaster catalog unavailable or outside approved item budgets");
}
static bool blocked(struct char_data *ch)
{
  send_to_char(ch, "Your quartermaster purchase awaits safe storage recovery. Try again shortly or "
                   "contact staff.\r\n");
  PS(ch)->clan_qm_checked = false;
  return false;
}
static bool authenticated(struct char_data *ch)
{
  return ch && !IS_NPC(ch) && ch->player_specials && ch->desc && STATE(ch->desc) == CON_PLAYING &&
         ch->desc->account && ch->desc->account->id > 0 && GET_IDNUM(ch) > 0;
}
/* Character currency checkpoint precedes inventory delivery; inventory's own
 * receipt survives a crash between its save and the final SQL acknowledgement.
 * No commands execute until both checkpoints and the SQL ack are durable. */
bool clan_qm_recover(struct char_data *ch)
{
  MYSQL_POOL_CONN *connection;
  struct json_object *op = NULL;
  struct clan_qm_item d;
  struct obj_data *o = NULL, *it;
  enum clan_repository_result r;
  char uid[33];
  int count = 0, state;
  bool fresh = false;
  if (!ch || IS_NPC(ch) || !ch->player_specials)
    return true;
  if (PS(ch)->clan_qm_checked && !*PS(ch)->clan_qm_lock)
    return true;
  if (!clan_persistence_sql_authority())
    return *PS(ch)->clan_qm_lock ? blocked(ch) : true;
  if (!authenticated(ch) || !mysql_available || !(connection = mysql_pool_acquire()))
    return blocked(ch);
  r = clan_qm_operation(connection->conn, GET_IDNUM(ch),
                        *PS(ch)->clan_qm_lock ? PS(ch)->clan_qm_lock : NULL, &op);
  mysql_pool_release(connection);
  if (r == CLAN_REPO_NOT_FOUND || r == CLAN_REPO_SCHEMA)
  {
    if (*PS(ch)->clan_qm_lock)
      return blocked(ch);
    PS(ch)->clan_qm_checked = true;
    return true;
  }
  if (r != CLAN_REPO_OK)
    return blocked(ch);
  strlcpy(uid, str(op, "operation_uid"), 33);
  state = num(op, "state");
  if (!clan_qm_item_parse(get(op, "terms"), &d) || num(op, "player") != GET_IDNUM(ch) ||
      num(op, "account") != ch->desc->account->id)
    goto fail;
  if (!*PS(ch)->clan_qm_lock)
  {
    strlcpy(PS(ch)->clan_qm_lock, uid, 33);
    PS(ch)->clan_qm_paid = false;
  }
  if (strcmp(PS(ch)->clan_qm_lock, uid))
    goto fail;
  if (!PS(ch)->clan_qm_paid)
  {
    if (state != 0 || !strcmp(PS(ch)->clan_qm_inventory, uid) || GET_GOLD(ch) < d.price)
      goto fail;
    GET_GOLD(ch) -= d.price;
    PS(ch)->clan_qm_paid = true;
  }
  /* Retry uncertain writes with the same in-memory receipt and debit. */
  if (!save_char_checked(ch, 0))
    goto fail;
  for (it = ch->carrying; it; it = it->next_content)
    if (!strcmp(it->clan_qm_uid, uid))
    {
      o = it;
      count++;
    }
  if (count > 1)
    goto fail;
  if (!strcmp(PS(ch)->clan_qm_inventory, uid))
  {
    if (!o)
      goto fail;
  }
  else
  {
    if (state != 0)
      goto fail;
    if (!o)
    {
      o = estate_unpack(str(op, "payload"));
      fresh = true;
      if (!o)
        goto fail;
    }
  }
  if (!template_valid(&d, o, true) || strcmp(o->clan_qm_uid, uid) ||
      GET_OBJ_BOUND_ID(o) != GET_IDNUM(ch))
  {
    if (fresh)
      extract_obj(o);
    goto fail;
  }
  if (fresh)
    obj_to_char(o, ch);
  strlcpy(PS(ch)->clan_qm_inventory, uid, 33);
  if (!Crash_crashsave_checked(ch))
    goto fail;
  if (!(connection = mysql_pool_acquire()))
    goto fail;
  r = clan_qm_ack(connection->conn, GET_IDNUM(ch), uid);
  mysql_pool_release(connection);
  if (r != CLAN_REPO_OK)
    goto fail;
  *PS(ch)->clan_qm_lock = 0;
  PS(ch)->clan_qm_paid = false;
  if (!save_char_checked(ch, 0))
  { /* Retain identity in memory until the clear is durable. */
    strlcpy(PS(ch)->clan_qm_lock, uid, 33);
    PS(ch)->clan_qm_paid = true;
    goto fail;
  }
  PS(ch)->clan_qm_checked = true;
  json_object_put(op);
  send_to_char(ch, "Your quartermaster purchase is delivered. Favor was not spent.\r\n");
  return true;
fail:
  json_object_put(op);
  return blocked(ch);
}
ACMDU(do_clanquartermaster)
{
  char command[32] = "list", id[160] = "", uid[33] = "";
  int tokens, clanlevel;
  size_t i;
  struct clan_qm_catalog catalog;
  struct clan_status_report report;
  struct clan_progression_readiness ready = {0};
  const struct clan_progression_config *config = clan_buff_runtime_policy();
  MYSQL_POOL_CONN *connection;
  enum clan_repository_result r;
  struct obj_data *o;
  char *payload;
  struct json_object *previous = NULL;
  int64_t now = time(NULL);
  if (!authenticated(ch))
    return;
  tokens = sscanf(argument, "%31s %159[^\r\n]", command, id);
  if (!strcmp(command, "confirm")) {
    struct json_object *quote = json_tokener_parse(PS(ch)->clan_qm_quote);
    if (tokens == 1 && quote) {
      strlcpy(id, str(get(quote, "terms"), "id"), sizeof(id));
      strlcpy(uid, str(quote, "uid"), sizeof(uid));
      tokens = 3;
    } else {
      char legacy_id[64], extra[2];
      if (sscanf(id, "%63s %32s %1s", legacy_id, uid, extra) == 2) {
        strlcpy(id, legacy_id, sizeof(id)); tokens = 3;
      }
    }
    if (quote) json_object_put(quote);
  }
  if (tokens > 3 || (!strcmp(command, "list") && tokens > 1) ||
      (strcmp(command, "list") && strcmp(command, "buy") && strcmp(command, "commission") &&
       strcmp(command, "confirm")) ||
      ((!strcmp(command, "buy") || !strcmp(command, "commission")) && tokens != 2) ||
      (!strcmp(command, "confirm") && (tokens != 3 || !clan_progression_uid_valid(uid))))
  {
    send_to_char(ch, "Usage: clan quartermaster list | buy <name> | commission <name> "
                     "| confirm\r\n");
    return;
  }
  if (!clan_qm_recover(ch))
    return;
  if (!clan_persistence_sql_authority() || !clan_persistence_ready() || !mysql_available ||
      !(connection = mysql_pool_acquire()))
  {
    send_to_char(ch, "Quartermaster storage is unavailable.\r\n");
    return;
  }
  if (!strcmp(command, "confirm"))
  {
    r = clan_qm_operation(connection->conn, GET_IDNUM(ch), uid, &previous);
    if (r == CLAN_REPO_OK)
    {
      bool same = !strcmp(str(get(previous, "terms"), "id"), id) &&
                  num(previous, "account") == ch->desc->account->id;
      bool pending = num(previous, "state") == 0;
      json_object_put(previous);
      mysql_pool_release(connection);
      if (same && pending)
      {
        PS(ch)->clan_qm_checked = false;
        clan_qm_recover(ch);
        return;
      }
      send_to_char(ch, same ? "That purchase has already been delivered; no additional gold or "
                              "item was issued.\r\n"
                            : "That confirmation belongs to a different purchase.\r\n");
      return;
    }
    if (r != CLAN_REPO_NOT_FOUND)
    {
      mysql_pool_release(connection);
      send_to_char(ch, "Purchase storage could not be verified. Retry the same confirmation.\r\n");
      return;
    }
  }
  ready.schema_ready = clan_qm_schema_ready(connection->conn);
  ready.schema_version = CLAN_PROGRESSION_SCHEMA_VERSION;
  ready.catalog_ready = clan_qm_catalog_load(&catalog);
  r = clan_repository_status_read(connection->conn, GET_IDNUM(ch), ch->desc->account->id, NULL,
                                  false, now, &report);
  if (r != CLAN_REPO_OK || !report.full_clan_access || report.membership != CLAN_ACCESS_ACCEPTED ||
      !clan_progression_feature_available(config, CLAN_FEATURE_QUARTERMASTER, &ready,
                                          report.clan.uid) ||
      !clan_award_level(config, report.clan.xp_eighths, &clanlevel))
  {
    mysql_pool_release(connection);
    send_to_char(ch, "Quartermaster purchases require accepted clan membership. "
                     "If you are a member, the service may be temporarily unavailable.\r\n");
    return;
  }
  if (!strcmp(command, "list"))
  {
    mysql_pool_release(connection);
    send_to_char(ch, "\r\n\tCClan Quartermaster\tn\r\n  Favor unlocks items; only gold is spent.\r\n\r\n");
    for (i = 0; i < catalog.count; i++)
    {
      struct clan_qm_item *d = &catalog.entries[i];
      send_to_char(ch,
                   "  \tW%s\tn%s\r\n    Price: %d gold\r\n    Requires: character level %d, clan level %d, %d Favor\r\n",
                   d->name,
                   (clanlevel < d->clan_level || report.favor.favor_eighths < (int64_t)clan_qm_favor_requirement(d)*8 || GET_LEVEL(ch) < d->level) ? " [locked]" : "", d->price, d->level, d->clan_level, clan_qm_favor_requirement(d));
      send_to_char(ch,"    %s\r\n\r\n",d->coverage);
    }
    send_to_char(ch, "Browse by name: clan quartermaster buy <name>\r\n"
                     "Prices are fixed; Favor is never spent. Bound purchases have no resale value "
                     "and remain usable after departure under normal item restrictions.\r\n");
    return;
  }
  int choice = CLAN_DISPLAY_FIND(id, catalog, struct clan_qm_item);
  if (choice < 0) {
    mysql_pool_release(connection);
    send_to_char(ch, choice == -2 ? "That name matches several items. Please use more of the name.\r\n"
                                  : "No item by that name. Use clan quartermaster list.\r\n");
    return;
  }
  i = (size_t)choice;
  if (i == catalog.count || !has_quartermaster_in_room(ch) || FIGHTING(ch) ||
      GET_CRAFT(ch).craft_duration > 0 || GET_IDNUM(ch) > INT_MAX ||
      IS_CARRYING_N(ch) >= CAN_CARRY_N(ch) ||
      IS_CARRYING_W(ch) + (i < catalog.count ? catalog.entries[i].weight : 0) > CAN_CARRY_W(ch))
  {
    mysql_pool_release(connection);
    send_to_char(ch, "Visit a quartermaster outside combat/crafting, with room to carry the item, "
                     "and choose an item by name.\r\n");
    return;
  }
  o = read_object(catalog.entries[i].prototype, VIRTUAL);
  if (!o)
  {
    mysql_pool_release(connection);
    return;
  }
  if (GET_LEVEL(ch) < catalog.entries[i].level || invalid_align(ch, o) ||
      !is_class_req_object(ch, o, false) || is_class_anti_object(ch, o, false) ||
      invalid_class(ch, o) || invalid_prof(ch, o))
  {
    extract_obj(o);
    mysql_pool_release(connection);
    send_to_char(ch, "Your character does not meet the item's normal restrictions.\r\n");
    return;
  }
  if (clanlevel < catalog.entries[i].clan_level ||
      report.favor.favor_eighths < (int64_t)clan_qm_favor_requirement(&catalog.entries[i]) * 8 ||
      GET_GOLD(ch) < catalog.entries[i].price)
  {
    extract_obj(o);
    mysql_pool_release(connection);
    send_to_char(ch, "You need more gold, clan levels or Favor for that item.\r\n");
    return;
  }
  if (strcmp(command, "confirm"))
  {
    struct json_object *quote;
    extract_obj(o);
    mysql_pool_release(connection);
    if (!clan_repository_uid_generate(uid))
      return;
    quote = json_object_new_object();
    json_object_object_add(quote, "terms", clan_qm_terms(&catalog.entries[i]));
    json_object_object_add(quote, "uid", json_object_new_string(uid));
    json_object_object_add(quote, "clan", json_object_new_string(report.clan.uid));
    json_object_object_add(quote, "expires", json_object_new_int64(now + 60));
    strlcpy(PS(ch)->clan_qm_quote, json_object_to_json_string_ext(quote, JSON_C_TO_STRING_PLAIN),
            sizeof(PS(ch)->clan_qm_quote));
    json_object_put(quote);
    send_to_char(ch,
                 "%s costs %d gold; Favor is not spent. Confirm within 60 seconds: clan "
                 "quartermaster confirm\r\n",
                 catalog.entries[i].name, catalog.entries[i].price);
    return;
  }
  else
  {
    struct json_object *quote = json_tokener_parse(PS(ch)->clan_qm_quote),
                       *terms = clan_qm_terms(&catalog.entries[i]);
    bool valid = quote && !strcmp(str(quote, "uid"), uid) &&
                 !strcmp(str(quote, "clan"), report.clan.uid) && now < num(quote, "expires") &&
                 json_object_equal(get(quote, "terms"), terms);
    if (quote)
      json_object_put(quote);
    json_object_put(terms);
    if (!valid)
    {
      extract_obj(o);
      mysql_pool_release(connection);
      send_to_char(
          ch, "Preview this purchase again; its price confirmation has expired or changed.\r\n");
      return;
    }
  }
  GET_OBJ_BOUND_ID(o) = GET_IDNUM(ch);
  GET_OBJ_COST(o) = 0;
  strlcpy(o->clan_qm_uid, uid, 33);
  if (!template_valid(&catalog.entries[i], o, true))
  {
    extract_obj(o);
    mysql_pool_release(connection);
    send_to_char(ch, "That item is unavailable pending catalog review.\r\n");
    return;
  }
  payload = estate_pack(o);
  extract_obj(o);
  if (!payload)
  {
    mysql_pool_release(connection);
    return;
  }
  r = clan_qm_prepare(connection->conn, &catalog.entries[i], uid, report.clan.uid, GET_IDNUM(ch),
                      ch->desc->account->id, GET_LEVEL(ch), GET_GOLD(ch), payload, now, config);
  free(payload);
  mysql_pool_release(connection);
  /* Even an uncertain commit is resolved from the original UID before commands
  * resume. A rejected purchase does not touch currency or inventory. */
  if (r == CLAN_REPO_OK || r == CLAN_REPO_UNAVAILABLE)
  {
    PS(ch)->clan_qm_checked = false;
    if (!clan_qm_recover(ch))
      return;
  }
  else
    send_to_char(ch, "Purchase rejected (%s). Check gold, Favor, clan level and membership.\r\n",
                 clan_repository_result_name(r));
}
