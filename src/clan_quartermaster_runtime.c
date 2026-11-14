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
#include "clan_qm_services.h"
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
  if (!authenticated(ch) || !mysql_available)
    return blocked(ch);
  int service_recovery = clan_qm_services_recover(ch);
  if (service_recovery) return service_recovery > 0;
  if (!(connection = mysql_pool_acquire())) return blocked(ch);
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
  clan_qm_services_command(ch, argument);
}
