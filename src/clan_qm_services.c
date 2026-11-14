#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "handler.h"
#include "interpreter.h"
#include "mysql.h"
#include "clan.h"
#include "clan_persistence.h"
#include "clan_quartermaster.h"
#include "clan_qm_services.h"
#include "clan_enchant.h"
#include "clan_awards.h"
#include "clan_buff_runtime.h"
#include "crafting_new.h"
#include "db.h"
#include "spec_abilities.h"
#include "assign_wpn_armor.h"
#include <inttypes.h>
#include <limits.h>
#define PS(ch) ((ch)->player_specials)
struct service_config
{
  int mote, qp, enhance, reroll, motes[6], quests[6];
};
struct service_operation
{
  char uid[33], clan[33], weapon[33], word[24];
  int kind, quantity, mote, price, effect, race, reroll, state;
  int64_t player, account, week;
};
static const char *mote_names[] = {"none", "air",   "dark",      "earth", "fire",
                                   "ice",  "light", "lightning", "water"};
static bool configuration(struct service_config *c)
{
  struct json_object *o = json_object_from_file("clan/quartermaster_services.json"), *v, *a;
  const char *keys[] = {"mote_price", "quest_point_price", "enhancement_price", "reroll_price"};
  int *prices[] = {&c->mote, &c->qp, &c->enhance, &c->reroll};
  int i, j;
  bool ok = false;
  if (!o || !json_object_object_get_ex(o, "version", &v) || json_object_get_int(v) != 1)
    goto done;
  for (i = 0; i < 4; i++)
  {
    if (!json_object_object_get_ex(o, keys[i], &v) || !json_object_is_type(v, json_type_int) ||
        json_object_get_int64(v) < 1 || json_object_get_int64(v) > INT_MAX / 1000)
      goto done;
    *prices[i] = json_object_get_int(v);
  }
  if (c->reroll <= c->enhance)
    goto done;
  for (j = 0; j < 2; j++)
  {
    if (!json_object_object_get_ex(o, j ? "weekly_quest_points" : "weekly_motes", &a) ||
        !json_object_is_type(a, json_type_array) || json_object_array_length(a) != 6)
      goto done;
    for (i = 0; i < 6; i++)
    {
      int *limits = j ? c->quests : c->motes;
      v = json_object_array_get_idx(a, i);
      if (!json_object_is_type(v, json_type_int) || json_object_get_int64(v) < 0 ||
          json_object_get_int64(v) > 1000 || (i && json_object_get_int(v) < limits[i - 1]))
        goto done;
      limits[i] = json_object_get_int(v);
    }
  }
  ok = true;
done:
  if (o)
    json_object_put(o);
  return ok;
}
static bool authenticated(struct char_data *ch)
{
  return ch && !IS_NPC(ch) && ch->desc && ch->desc->account && STATE(ch->desc) == CON_PLAYING &&
         GET_IDNUM(ch) > 0 && ch->desc->account->id > 0;
}
static bool available(MYSQL *db)
{
  /* Absence is a disabled optional service, never a reason to block ordinary login. */
  MYSQL_RES *res;
  MYSQL_ROW row;
  bool ok = false;
  if (mysql_query(
          db,
          "SELECT (SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() "
          "AND table_name='clan_quartermaster_services' AND engine='InnoDB')=1 AND "
          "(SELECT COUNT(*) FROM information_schema.triggers WHERE trigger_schema=DATABASE() "
          "AND event_object_table='clan_quartermaster_services' AND trigger_name IN "
          "('cqs_no_delete','cqs_immutable'))=2") ||
      !(res = mysql_store_result(db)))
    return false;
  row = mysql_fetch_row(res);
  ok = row && row[0] && atoi(row[0]) == 1;
  mysql_free_result(res);
  return ok;
}
static int operation(MYSQL *db, struct char_data *ch, struct service_operation *op)
{
  char q[1024];
  MYSQL_RES *res;
  MYSQL_ROW r;
  if (*PS(ch)->clan_qm_lock)
  {
    if (!clan_progression_uid_valid(PS(ch)->clan_qm_lock))
      return -1;
    snprintf(q, sizeof(q),
             "SELECT "
             "operation_uid,clan_uid,player_id,account_id,week_start,kind,quantity,mote,price,"
             "weapon_uid,effect,command_word,bane_race,reroll,state FROM "
             "clan_quartermaster_services WHERE operation_uid='%s' AND player_id=%" PRId64,
             PS(ch)->clan_qm_lock, (int64_t)GET_IDNUM(ch));
  }
  else
    snprintf(q, sizeof(q),
             "SELECT "
             "operation_uid,clan_uid,player_id,account_id,week_start,kind,quantity,mote,price,"
             "weapon_uid,effect,command_word,bane_race,reroll,state FROM "
             "clan_quartermaster_services WHERE active_player=%" PRId64,
             (int64_t)GET_IDNUM(ch));
  if (mysql_query(db, q) || !(res = mysql_store_result(db)))
    return -1;
  r = mysql_fetch_row(res);
  if (!r)
  {
    mysql_free_result(res);
    return 0;
  }
  memset(op, 0, sizeof(*op));
  strlcpy(op->uid, r[0], 33);
  strlcpy(op->clan, r[1], 33);
  op->player = strtoll(r[2], NULL, 10);
  op->account = strtoll(r[3], NULL, 10);
  op->week = strtoll(r[4], NULL, 10);
  op->kind = atoi(r[5]);
  op->quantity = atoi(r[6]);
  op->mote = atoi(r[7]);
  op->price = atoi(r[8]);
  strlcpy(op->weapon, r[9], 33);
  op->effect = atoi(r[10]);
  strlcpy(op->word, r[11], 24);
  op->race = atoi(r[12]);
  op->reroll = atoi(r[13]);
  op->state = atoi(r[14]);
  mysql_free_result(res);
  if (op->account != ch->desc->account->id || !clan_progression_uid_valid(op->uid) ||
      op->price < 1 || op->quantity < 1 || op->quantity > 1000 || op->kind < 1 || op->kind > 3 ||
      (op->kind == 1 && (op->mote < 1 || op->mote >= NUM_CRAFT_MOTES)) ||
      (op->kind == 3 &&
       (!clan_progression_uid_valid(op->weapon) || !clan_enchant_choice(op->effect))))
    return -1;
  return 1;
}
static struct obj_data *weapon_identity(struct char_data *ch, const char *uid)
{
  struct obj_data *o, *found = NULL;
  for (o = ch->carrying; o; o = o->next_content)
    if (!strcmp(o->clan_enchantment.uid, uid))
    {
      if (found)
        return NULL;
      found = o;
    }
  return found;
}
int clan_qm_services_recover(struct char_data *ch)
{
  MYSQL_POOL_CONN *connection;
  struct service_operation op;
  struct obj_data *weapon = NULL;
  char q[256];
  int found;
  if (!authenticated(ch) || !clan_persistence_sql_authority() || !mysql_available)
    return 0;
  if (!(connection = mysql_pool_acquire()))
    return -1;
  if ((connection->conn->server_status & SERVER_STATUS_IN_TRANS) ||
      !(connection->conn->server_status & SERVER_STATUS_AUTOCOMMIT))
  {
    mysql_pool_release(connection);
    return -1;
  }
  if (!available(connection->conn))
  {
    mysql_pool_release(connection);
    return 0;
  }
  found = operation(connection->conn, ch, &op);
  mysql_pool_release(connection);
  if (found <= 0)
    return found;
  if (op.kind == 3 && !(weapon = weapon_identity(ch, op.weapon)))
    goto fail;
  if (!*PS(ch)->clan_qm_lock)
  {
    strlcpy(PS(ch)->clan_qm_lock, op.uid, 33);
    PS(ch)->clan_qm_paid = false;
  }
  if (strcmp(PS(ch)->clan_qm_lock, op.uid))
    goto fail;
  if (!PS(ch)->clan_qm_paid)
  {
    if (op.state || GET_GOLD(ch) < op.price)
      goto fail;
    if (op.kind == 1 && GET_CRAFT_MOTES(ch, op.mote) > INT_MAX - op.quantity)
      goto fail;
    if (op.kind == 2 && GET_QUESTPOINTS(ch) > INT_MAX - op.quantity)
      goto fail;
    GET_GOLD(ch) -= op.price;
    if (op.kind == 1)
      GET_CRAFT_MOTES(ch, op.mote) += op.quantity;
    if (op.kind == 2)
      GET_QUESTPOINTS(ch) += op.quantity;
    PS(ch)->clan_qm_paid = true;
  }
  if (!save_char_checked(ch, 0))
    goto fail;
  if (strcmp(PS(ch)->clan_qm_inventory, op.uid))
  {
    if (op.state)
      goto fail;
    if (weapon)
    {
      clan_enchant_activate(weapon, false);
      weapon->clan_enchantment.effect = op.effect;
      weapon->clan_enchantment.bane_race = op.race;
      strlcpy(weapon->clan_enchantment.word, op.word, 24);
      strlcpy(weapon->clan_enchantment.receipt, op.uid, 33);
      if (op.reroll)
        weapon->clan_enchantment.reroll_week = op.week;
      clan_enchant_activate(weapon, false);
    }
    strlcpy(PS(ch)->clan_qm_inventory, op.uid, 33);
  }
  if (weapon && strcmp(weapon->clan_enchantment.receipt, op.uid))
    goto fail;
  if (!Crash_crashsave_checked(ch))
    goto fail;
  if (!(connection = mysql_pool_acquire()))
    goto fail;
  if ((connection->conn->server_status & SERVER_STATUS_IN_TRANS) ||
      !(connection->conn->server_status & SERVER_STATUS_AUTOCOMMIT))
  {
    mysql_pool_release(connection);
    goto fail;
  }
  snprintf(q, sizeof(q),
           "UPDATE clan_quartermaster_services SET state=1 WHERE operation_uid='%s' AND "
           "player_id=%" PRId64,
           op.uid, op.player);
  found = !mysql_query(connection->conn, q);
  mysql_pool_release(connection);
  if (!found)
    goto fail;
  *PS(ch)->clan_qm_lock = 0;
  PS(ch)->clan_qm_paid = false;
  if (!save_char_checked(ch, 0))
  {
    strlcpy(PS(ch)->clan_qm_lock, op.uid, 33);
    PS(ch)->clan_qm_paid = true;
    goto fail;
  }
  PS(ch)->clan_qm_checked = true;
  *PS(ch)->clan_qm_quote = 0;
  if (weapon)
    send_to_char(ch, "\tGYour weapon gains %s.\tn Command word: \tW%s\tn (currently inactive).\r\n",
                 clan_enchant_choice(op.effect)->name, op.word);
  else
    send_to_char(ch, "\tGYour quartermaster purchase is complete. Favor was not spent.\tn\r\n");
  return 1;
fail:
  PS(ch)->clan_qm_checked = false;
  send_to_char(ch,
               "Your quartermaster purchase is awaiting storage recovery. Please try again.\r\n");
  return -1;
}
static bool standing(MYSQL *db, struct char_data *ch, struct clan_status_report *r, int *rank)
{
  enum clan_favor_standing s;
  struct clan_progression_readiness ready = {true, true, true, true,
                                             CLAN_PROGRESSION_SCHEMA_VERSION};
  if (clan_repository_status_read(db, GET_IDNUM(ch), ch->desc->account->id, NULL, false, time(NULL),
                                  r) != CLAN_REPO_OK ||
      !r->full_clan_access || r->membership != CLAN_ACCESS_ACCEPTED ||
      !clan_progression_feature_available(clan_buff_runtime_policy(), CLAN_FEATURE_QUARTERMASTER,
                                          &ready, r->clan.uid) ||
      !clan_award_standing(r->favor.favor_eighths, &s))
    return false;
  *rank = s;
  return true;
}
static bool used(MYSQL *db, int64_t player, int64_t week, int *motes, int *qp)
{
  char q[384];
  MYSQL_RES *res;
  MYSQL_ROW row;
  snprintf(q, sizeof(q),
           "SELECT COALESCE(SUM(CASE WHEN kind=1 THEN quantity ELSE 0 END),0),COALESCE(SUM(CASE "
           "WHEN kind=2 THEN quantity ELSE 0 END),0) FROM clan_quartermaster_services WHERE "
           "player_id=%" PRId64 " AND week_start=%" PRId64,
           player, week);
  if (mysql_query(db, q) || !(res = mysql_store_result(db)))
    return false;
  row = mysql_fetch_row(res);
  if (row)
  {
    *motes = atoi(row[0]);
    *qp = atoi(row[1]);
  }
  mysql_free_result(res);
  return row != NULL;
}
static bool suitable(struct obj_data *o, bool reroll)
{
  if (!o || GET_OBJ_TYPE(o) != ITEM_WEAPON || GET_OBJ_VAL(o, 0) <= WEAPON_TYPE_UNARMED ||
      GET_OBJ_VAL(o, 0) >= NUM_WEAPON_TYPES || o->contains || o->special_abilities ||
      *o->legendary_uuid || o->legendary_quarantined || *o->clan_delivery_uid ||
      OBJ_FLAGGED(o, ITEM_NORENT) || o->proto_script || o->script || o->has_spells)
    return false;
  if (reroll)
    return clan_enchant_choice(o->clan_enchantment.effect) != NULL;
  return !o->clan_enchantment.effect && !OBJ_FLAGGED(o, ITEM_FLAMING) &&
         !OBJ_FLAGGED(o, ITEM_FROST) && !OBJ_FLAGGED(o, ITEM_SHOCK) &&
         !OBJ_FLAGGED(o, ITEM_CORROSIVE) && !OBJ_FLAGGED(o, ITEM_KI_FOCUS) &&
         !OBJ_FLAGGED(o, ITEM_VICIOUS) && !OBJ_FLAGGED(o, ITEM_VORPAL) &&
         !OBJ_FLAGGED(o, ITEM_DISRUPTION) && !OBJ_FLAGGED(o, ITEM_SEEKING) &&
         !OBJ_FLAGGED(o, ITEM_ADAPTIVE) && !OBJ_FLAGGED(o, ITEM_AGILE) &&
         !OBJ_FLAGGED(o, ITEM_DEFENDING);
}
static bool compatible(const struct obj_data *weapon, int effect)
{
  int type = GET_OBJ_VAL(weapon, 0);
  if (type <= WEAPON_TYPE_UNARMED || type >= NUM_WEAPON_TYPES)
    return false;
  int flags = weapon_list[type].weaponFlags, damage_types = weapon_list[type].damageTypes;
  switch (effect)
  {
  case WEAPON_SPECAB_ADAPTIVE:
    return (flags & WEAPON_FLAG_RANGED) != 0;
  case WEAPON_SPECAB_SEEKING:
    return (flags & (WEAPON_FLAG_RANGED | WEAPON_FLAG_THROWN)) != 0;
  case WEAPON_SPECAB_RETURNING:
    return (flags & WEAPON_FLAG_THROWN) != 0;
  case WEAPON_SPECAB_THROWING:
    return !(flags & (WEAPON_FLAG_RANGED | WEAPON_FLAG_THROWN));
  case WEAPON_SPECAB_AGILE:
    return (flags & (WEAPON_FLAG_LIGHT | WEAPON_FLAG_FINESSE)) != 0;
  case WEAPON_SPECAB_KEEN:
    return (damage_types & (DAMAGE_TYPE_PIERCING | DAMAGE_TYPE_SLASHING)) != 0;
  case WEAPON_SPECAB_VORPAL:
    return (damage_types & DAMAGE_TYPE_SLASHING) != 0;
  case WEAPON_SPECAB_DISRUPTION:
    return (damage_types & DAMAGE_TYPE_BLUDGEONING) != 0;
  case WEAPON_SPECAB_DANCING:
  case WEAPON_SPECAB_MIGHTY_CLEAVING:
  case WEAPON_SPECAB_KI_FOCUS:
    return !(flags & WEAPON_FLAG_RANGED);
  default:
    return true;
  }
}
/* Randomness is generated once, after accepting the quote, and frozen in SQL.
 * Previewing never reveals an outcome, so cancelling cannot fish for effects. */
static void draw(struct service_operation *op, int rank, const struct obj_data *weapon)
{
  size_t i;
  int total = 0, roll;
  const struct clan_enchant_choice *choice = NULL;
  char words[128], *word;
  int n;
  for (i = 0; i < clan_enchant_count; i++)
    if (clan_enchant_choices[i].rank <= rank && compatible(weapon, clan_enchant_choices[i].effect))
      total += 6 - clan_enchant_choices[i].rank;
  roll = rand_number(1, total);
  for (i = 0; i < clan_enchant_count; i++)
    if (clan_enchant_choices[i].rank <= rank && compatible(weapon, clan_enchant_choices[i].effect))
    {
      roll -= 6 - clan_enchant_choices[i].rank;
      if (roll <= 0)
      {
        choice = &clan_enchant_choices[i];
        break;
      }
    }
  op->effect = choice->effect;
  strlcpy(words, choice->words, sizeof(words));
  n = rand_number(0, 2);
  word = strtok(words, " ");
  while (n-- && word)
    word = strtok(NULL, " ");
  strlcpy(op->word, word, 24);
  op->race = rand_number(1, NUM_RACE_TYPES - 1);
}
/* Serialize acceptance on the same store-control row used by clan membership
 * changes. Weekly allowance, membership and the receipt commit together. */
static bool prepare(MYSQL *db, struct char_data *ch, struct service_operation *op,
                    const struct service_config *c)
{
  int rank, motes, qp;
  char q[2048];
  MYSQL_RES *res;
  MYSQL_ROW row;
  if ((db->server_status & SERVER_STATUS_IN_TRANS) ||
      !(db->server_status & SERVER_STATUS_AUTOCOMMIT))
    return false;
  if (mysql_query(db, "START TRANSACTION"))
    return false;
  if (mysql_query(db, "SELECT store_mode FROM clan_store_control WHERE singleton=1 FOR UPDATE") ||
      !(res = mysql_store_result(db)))
    goto rollback;
  row = mysql_fetch_row(res);
  bool authority = row && atoi(row[0]) == 2;
  mysql_free_result(res);
  if (!authority)
    goto rollback;
  /* status_read starts its own transaction; use the already checked standing
  * before this lock, then validate current membership/Favor in one locked read. */
  snprintf(q, sizeof(q),
           "SELECT m.clan_uid,COALESCE(f.favor_eighths,0) FROM clan_memberships m JOIN "
           "clan_progression_clans c ON c.clan_uid=m.clan_uid AND c.record_status=1 LEFT JOIN "
           "clan_personal_favor f ON f.clan_uid=m.clan_uid AND f.player_id=m.player_id WHERE "
           "m.player_id=%" PRId64 " AND m.rank_id>0 FOR UPDATE",
           op->player);
  if (mysql_query(db, q) || !(res = mysql_store_result(db)))
    goto rollback;
  row = mysql_fetch_row(res);
  enum clan_favor_standing s;
  bool member =
      row && !strcmp(row[0], op->clan) && clan_award_standing(strtoll(row[1], NULL, 10), &s);
  mysql_free_result(res);
  if (!member)
    goto rollback;
  rank = s;
  if ((op->kind == 1 && GET_CRAFT_MOTES(ch, op->mote) > INT_MAX - op->quantity) ||
      (op->kind == 2 && GET_QUESTPOINTS(ch) > INT_MAX - op->quantity))
    goto rollback;
  if (GET_GOLD(ch) < op->price || !used(db, op->player, op->week, &motes, &qp))
    goto rollback;
  if ((op->kind == 1 && op->quantity > c->motes[rank] - motes) ||
      (op->kind == 2 && op->quantity > c->quests[rank] - qp) || (op->kind == 3 && rank < 1))
    goto rollback;
  if (op->kind == 3)
    draw(op, rank, weapon_identity(ch, op->weapon));
  snprintf(
      q, sizeof(q),
      "INSERT INTO "
      "clan_quartermaster_services(operation_uid,player_id,account_id,clan_uid,week_start,kind,"
      "quantity,mote,price,weapon_uid,effect,command_word,bane_race,reroll) VALUES('%s',%" PRId64
      ",%" PRId64 ",'%s',%" PRId64 ",%d,%d,%d,%d,'%s',%d,'%s',%d,%d)",
      op->uid, op->player, op->account, op->clan, op->week, op->kind, op->quantity, op->mote,
      op->price, op->weapon, op->effect, op->word, op->race, op->reroll);
  if (mysql_query(db, q))
    goto rollback;
  /* On uncertain COMMIT, command recovery queries the SAME receipt. */
  if (mysql_query(db, "COMMIT"))
  {
    /* If the server rejected COMMIT while retaining the transaction, abort it
     * before any pooled reader could mistake an uncommitted row for a receipt.
     * An already committed receipt is unaffected by ROLLBACK. */
    mysql_query(db, "ROLLBACK");
    return false;
  }
  return true;
rollback:
  mysql_query(db, "ROLLBACK");
  return false;
}
static void service_info(struct char_data *ch, const struct service_config *c)
{
  size_t i;
  int rank;
  send_to_char(ch,
               "\r\n\tCClan Quartermaster\tn\r\n"
               "  Crafting motes: \tW%d gold each\tn\r\n  Quest points:   \tW%d gold each\tn\r\n"
               "  Enchant weapon: \tW%d gold\tn (Favor rank 1+)\r\n  Reroll weapon:  \tW%d gold\tn "
               "(once per weapon per week)\r\n\r\n"
               "\tWFavor rank     Weekly motes     Weekly quest points\tn\r\n",
               c->mote, c->qp, c->enhance, c->reroll);
  for (rank = 0; rank < 6; rank++)
    send_to_char(ch, "     %d              %2d                  %2d\r\n", rank, c->motes[rank],
                 c->quests[rank]);
  send_to_char(
      ch, "\r\nAllowances are per character across all clans; they reset with clan week.\r\n"
          "Favor unlocks services and is never spent. Effects roll only from ranks you "
          "have\r\nunlocked and suited to your weapon; each rank 1 effect is five times as likely "
          "as a rank 5 effect.\r\n"
          "One clan enchantment per weapon; rerolls replace it and can roll the same effect.\r\n"
          "Use: clan quartermaster motes <type> <quantity>\r\n"
          "     clan quartermaster quest <quantity>\r\n"
          "     clan quartermaster enhance <carried weapon>\r\n"
          "     clan quartermaster reroll <carried weapon>\r\n"
          "     clan quartermaster inspect <weapon>\r\n"
          "     clan quartermaster confirm\r\n"
          "Equip the weapon, then utter its command word to toggle its effect.\r\n\r\n\tWWeapon "
          "effects (minimum Favor rank)\tn\r\n");
  for (rank = 1; rank <= 5; rank++)
  {
    int column = 0;
    send_to_char(ch, "\r\n  \tCFavor rank %d\tn\r\n", rank);
    for (i = 0; i < clan_enchant_count; i++)
      if (clan_enchant_choices[i].rank == rank)
      {
        send_to_char(ch, "    %-18s", clan_enchant_choices[i].name);
        if (++column % 3 == 0)
          send_to_char(ch, "\r\n");
      }
    if (column % 3)
      send_to_char(ch, "\r\n");
  }
  send_to_char(ch, "\r\nFor effect descriptions: \tWhelp clanquartermaster\tn\r\n");
}
void clan_qm_services_command(struct char_data *ch, const char *argument)
{
  char command[32] = "list", arg[MAX_INPUT_LENGTH] = "", type[32], amount[16], extra[2];
  struct service_config c;
  struct service_operation op = {0};
  struct clan_status_report report;
  struct clan_reset_period week;
  MYSQL_POOL_CONN *connection;
  int rank, motes, qp, n = 1;
  struct obj_data *weapon = NULL;
  struct json_object *quote = NULL, *v;
  bool confirmed = false, accepted = false;
  if (!authenticated(ch))
    return;
  while (isspace((unsigned char)*argument))
    argument++;
  if (*argument)
  {
    size_t length = strcspn(argument, " \t\r\n");
    if (length >= sizeof(command))
      goto usage;
    memcpy(command, argument, length);
    command[length] = '\0';
    argument += length;
    while (isspace((unsigned char)*argument))
      argument++;
    if (strlen(argument) >= sizeof(arg))
      goto usage;
    strlcpy(arg, argument, sizeof(arg));
  }
  if (!configuration(&c))
  {
    send_to_char(ch, "Quartermaster services are temporarily unavailable.\r\n");
    return;
  }
  if (!strcmp(command, "info") || !strcmp(command, "help"))
  {
    service_info(ch, &c);
    return;
  }
  if (!strcmp(command, "inspect"))
  {
    if (!clan_progression_week_at(time(NULL), &week))
      goto unavailable;
    weapon = get_obj_in_list_vis(ch, arg, &n, ch->carrying);
    if (!weapon)
      for (n = 0; n < NUM_WEARS; n++)
        if (GET_EQ(ch, n) && isname(arg, GET_EQ(ch, n)->name))
        {
          weapon = GET_EQ(ch, n);
          break;
        }
    if (!weapon || !clan_enchant_choice(weapon->clan_enchantment.effect))
      send_to_char(ch, "That weapon has no clan enchantment.\r\n");
    else
      send_to_char(
          ch, "\tW%s\tn\r\n  Effect: %s (%s)\r\n  Command word: %s\r\n  Reroll: %s\r\n",
          weapon->short_description, clan_enchant_choice(weapon->clan_enchantment.effect)->name,
          weapon->clan_enchantment.active ? "active" : "inactive", weapon->clan_enchantment.word,
          weapon->clan_enchantment.reroll_week == week.starts_at ? "used this week" : "available");
    return;
  }
  if (!clan_qm_recover(ch))
    return;
  if (!clan_persistence_sql_authority() || !clan_persistence_ready() || !mysql_available ||
      !(connection = mysql_pool_acquire()))
    goto unavailable;
  if (!available(connection->conn) || !standing(connection->conn, ch, &report, &rank) ||
      !clan_progression_week_at(time(NULL), &week) ||
      !used(connection->conn, GET_IDNUM(ch), week.starts_at, &motes, &qp))
  {
    mysql_pool_release(connection);
    goto unavailable;
  }
  mysql_pool_release(connection);
  if (!strcmp(command, "list"))
  {
    service_info(ch, &c);
    send_to_char(ch, "\r\n\tWYour Favor rank: %d\tn  Motes left: %d  Quest points left: %d\r\n",
                 rank, MAX(0, c.motes[rank] - motes), MAX(0, c.quests[rank] - qp));
    return;
  }
  if (!has_quartermaster_in_room(ch) || FIGHTING(ch) || GET_CRAFT(ch).craft_duration > 0)
  {
    send_to_char(ch,
                 "Visit a quartermaster outside combat and crafting to use these services.\r\n");
    return;
  }
  if (!strcmp(command, "confirm"))
  {
    quote = json_tokener_parse(PS(ch)->clan_qm_quote);
    if (!quote || !json_object_object_get_ex(quote, "service", &v) || !json_object_get_boolean(v))
      goto badquote;
#define READNUM(key, field)                                                                        \
  if (!json_object_object_get_ex(quote, key, &v))                                                  \
    goto badquote;                                                                                 \
  op.field = json_object_get_int64(v)
#define READSTR(key, field)                                                                        \
  if (!json_object_object_get_ex(quote, key, &v))                                                  \
    goto badquote;                                                                                 \
  strlcpy(op.field, json_object_get_string(v), sizeof(op.field))
    READNUM("kind", kind);
    READNUM("quantity", quantity);
    READNUM("mote", mote);
    READNUM("price", price);
    READNUM("reroll", reroll);
    READNUM("week", week);
    READSTR("uid", uid);
    READSTR("clan", clan);
    READSTR("weapon", weapon);
    if (!clan_progression_uid_valid(op.uid) || strcmp(op.clan, report.clan.uid) ||
        op.week != week.starts_at)
      goto badquote;
    if (!json_object_object_get_ex(quote, "expires", &v) || json_object_get_int64(v) <= time(NULL))
      goto badquote;
    confirmed = true;
  }
  else
  {
    op.player = GET_IDNUM(ch);
    op.account = ch->desc->account->id;
    op.week = week.starts_at;
    strlcpy(op.clan, report.clan.uid, 33);
    if (!strcmp(command, "motes"))
    {
      if (sscanf(arg, "%31s %15s %1s", type, amount, extra) != 2 || strlen(amount) > 4 ||
          strspn(amount, "0123456789") != strlen(amount))
        goto usage;
      for (op.mote = 1; op.mote < NUM_CRAFT_MOTES; op.mote++)
        if (!strcasecmp(type, mote_names[op.mote]))
          break;
      if (op.mote == NUM_CRAFT_MOTES)
        goto usage;
      op.quantity = atoi(amount);
      op.kind = 1;
    }
    else if (!strcmp(command, "quest"))
    {
      if (sscanf(arg, "%15s %1s", amount, extra) != 1 || strlen(amount) > 4 ||
          strspn(amount, "0123456789") != strlen(amount))
        goto usage;
      op.quantity = atoi(amount);
      op.kind = 2;
    }
    else if (!strcmp(command, "enhance") || !strcmp(command, "reroll"))
    {
      weapon = get_obj_in_list_vis(ch, arg, &n, ch->carrying);
      op.kind = 3;
      op.quantity = 1;
      op.reroll = !strcmp(command, "reroll");
      if (!weapon || GET_OBJ_TYPE(weapon) != ITEM_WEAPON)
        goto badweapon;
      if (!suitable(weapon, op.reroll))
        goto badweapon;
      if (GET_OBJ_BOUND_ID(weapon) > 0 && GET_OBJ_BOUND_ID(weapon) != (int)NOBODY &&
          GET_OBJ_BOUND_ID(weapon) != GET_IDNUM(ch))
        goto badweapon;
      if (!*weapon->clan_enchantment.uid)
      {
        if (!clan_repository_uid_generate(weapon->clan_enchantment.uid))
          goto unavailable;
        if (!Crash_crashsave_checked(ch))
        {
          *weapon->clan_enchantment.uid = 0;
          goto unavailable;
        }
      }
      strlcpy(op.weapon, weapon->clan_enchantment.uid, 33);
    }
    else
      goto usage;
    if (op.quantity < 1 || op.quantity > 1000)
      goto usage;
    op.price = op.kind == 1   ? c.mote * op.quantity
               : op.kind == 2 ? c.qp * op.quantity
               : op.reroll    ? c.reroll
                              : c.enhance;
    if (!clan_repository_uid_generate(op.uid))
      goto unavailable;
  }
  op.player = GET_IDNUM(ch);
  op.account = ch->desc->account->id;
  if (op.kind == 3)
  {
    weapon = weapon_identity(ch, op.weapon);
    if (weapon && GET_OBJ_BOUND_ID(weapon) > 0 && GET_OBJ_BOUND_ID(weapon) != (int)NOBODY &&
        GET_OBJ_BOUND_ID(weapon) != GET_IDNUM(ch))
      goto badweapon;
    if (!weapon || rank < 1 ||
        (!suitable(weapon, op.reroll) ||
         (op.reroll && weapon->clan_enchantment.reroll_week == week.starts_at)))
      goto badweapon;
  }
  if ((op.kind == 1 && op.quantity > c.motes[rank] - motes) ||
      (op.kind == 2 && op.quantity > c.quests[rank] - qp))
  {
    send_to_char(ch, "That exceeds your character's remaining weekly allowance.\r\n");
    goto done;
  }
  if (GET_GOLD(ch) < op.price)
  {
    send_to_char(ch, "You need %d gold for this service.\r\n", op.price);
    goto done;
  }
  if (confirmed)
  {
    if (!(connection = mysql_pool_acquire()))
      goto unavailable;
    accepted = prepare(connection->conn, ch, &op, &c);
    mysql_pool_release(connection);
    /* Even when COMMIT is uncertain, recovery resolves the accepted operation
    * before any other command. Keep the quote until its fate is known. */
    PS(ch)->clan_qm_checked = false;
    if (accepted || clan_qm_services_recover(ch) == 1)
    {
      *PS(ch)->clan_qm_quote = 0;
      if (accepted)
        clan_qm_services_recover(ch);
    }
    else
      send_to_char(ch, "The purchase could not be accepted. Check your weekly allowance, gold and "
                       "reroll availability; retry the same confirmation.\r\n");
  }
  else
  {
    quote = json_object_new_object();
    json_object_object_add(quote, "service", json_object_new_boolean(true));
    json_object_object_add(quote, "expires", json_object_new_int64(time(NULL) + 60));
#define NUM(key, field) json_object_object_add(quote, key, json_object_new_int64(op.field))
#define STR(key, field) json_object_object_add(quote, key, json_object_new_string(op.field))
    NUM("kind", kind);
    NUM("quantity", quantity);
    NUM("mote", mote);
    NUM("price", price);
    NUM("reroll", reroll);
    NUM("week", week);
    STR("uid", uid);
    STR("clan", clan);
    STR("weapon", weapon);
    strlcpy(PS(ch)->clan_qm_quote, json_object_to_json_string_ext(quote, JSON_C_TO_STRING_PLAIN),
            sizeof(PS(ch)->clan_qm_quote));
    send_to_char(ch,
                 "\r\n\tCQuartermaster Preview\tn\r\n  %s%s\r\n  Gold: \tY%d\tn\r\n  Favor spent: "
                 "none\r\nConfirm with: \tWclan quartermaster confirm\tn\r\n",
                 op.kind == 1   ? mote_names[op.mote]
                 : op.kind == 2 ? "Quest points"
                                : weapon->short_description,
                 op.kind == 1   ? " motes"
                 : op.kind == 3 ? (op.reroll ? " (reroll)" : " (enchant)")
                                : "",
                 op.price);
    if (op.kind != 3)
      send_to_char(ch, "  Quantity: %d\r\n", op.quantity);
  }
  goto done;
badquote:
  send_to_char(ch, "Preview a quartermaster service first. Quotes expire after 60 seconds or the "
                   "weekly reset.\r\n");
  goto done;
badweapon:
  send_to_char(ch, "Choose an eligible carried weapon. New enchantments need Favor rank 1 and a "
                   "weapon without special abilities; rerolls need an existing clan enchantment "
                   "and an unused weekly reroll.\r\n");
  goto done;
usage:
  send_to_char(ch,
               "Use clan quartermaster info for service names, prices, limits and commands.\r\n");
  goto done;
unavailable:
  send_to_char(ch, "Quartermaster services could not be verified. Accepted clan membership and "
                   "available storage are required.\r\n");
done:
  if (quote)
    json_object_put(quote);
}
