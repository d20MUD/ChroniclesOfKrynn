/* Persistent fixed-price auction house adapted from d20StarWars.
 * Monetary identity is the stable player ID, never a mutable name. */
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "auction_house.h"
#include "comm.h"
#include "constants.h"
#include "db.h"
#include "estate.h"
#include "handler.h"
#include "interpreter.h"
#include "spec_procs.h"
#include "spells.h"

#define AUCTION_MAX_PRICE 99999999
static int valid_price(const char *text, long *price)
{
  char *end;
  long n;
  errno = 0;
  n = strtol(text, &end, 10);
  while (*end && isspace((unsigned char)*end))
    end++;
  if (errno || !*text || *end || n < 1 || n > AUCTION_MAX_PRICE)
    return FALSE;
  *price = n;
  return TRUE;
}
static int transferable(struct obj_data *obj)
{
  struct obj_data *it;
  if (OBJ_FLAGGED(obj, ITEM_NORENT) || OBJ_FLAGGED(obj, ITEM_NOSELL) ||
      OBJ_FLAGGED(obj, ITEM_NODROP) || GET_OBJ_BOUND_ID(obj) > 0 ||
      GET_OBJ_TYPE(obj) == ITEM_MONEY)
    return FALSE;
  for (it = obj->contains; it; it = it->next_content)
    if (!transferable(it))
      return FALSE;
  if (obj->sheath_primary && !transferable(obj->sheath_primary))
    return FALSE;
  if (obj->sheath_secondary && !transferable(obj->sheath_secondary))
    return FALSE;
  return TRUE;
}
/* Payouts are processed in ID order. The receipt is saved with the gold value,
 * before the SQL acknowledgement, so a retry cannot pay the same sale twice. */
void auction_house_collect(struct char_data *ch)
{
  MYSQL_RES *res;
  MYSQL_ROW row;
  unsigned long long receipt, id;
  long amount;
  int changed = FALSE;
  if (IS_NPC(ch) || !estate_init())
    return;
  receipt = ch->player_specials->estate_credit_receipt;
  res = estate_select("SELECT settlement_id,amount FROM auction_credit_settlements WHERE "
                      "beneficiary_id=%ld AND settlement_state='pending' ORDER BY settlement_id",
                      GET_IDNUM(ch));
  if (!res)
    return;
  while ((row = mysql_fetch_row(res)))
  {
    id = strtoull(row[0], NULL, 10);
    amount = atol(row[1]);
    if (id > receipt)
    {
      if (amount < 1 || amount > INT_MAX - GET_GOLD(ch))
      {
        send_to_char(ch, "Auction proceeds remain pending: make room in your gold balance.\r\n");
        break;
      }
      GET_GOLD(ch) += amount;
      ch->player_specials->estate_credit_receipt = id;
      estate_save_player(ch);
      if (!estate_receipt_saved(ch))
      {
        GET_GOLD(ch) -= amount;
        ch->player_specials->estate_credit_receipt = receipt;
        break;
      }
      receipt = id;
      changed = TRUE;
      send_to_char(ch, "You collect %ld gold in auction proceeds.\r\n", amount);
    }
    if (!estate_query("UPDATE auction_credit_settlements SET settlement_state='paid' WHERE "
                      "settlement_id=%llu AND beneficiary_id=%ld",
                      id, GET_IDNUM(ch)))
      break;
  }
  mysql_free_result(res);
  (void)changed;
}
static void filter_key(const char *text, char *key, size_t size)
{
  size_t n = 0;
  while (*text && n + 1 < size)
  {
    unsigned char c = (unsigned char)*text++;
    if (isalnum(c))
      key[n++] = tolower(c);
  }
  if (n > 1 && key[n - 1] == 's')
    n--;
  key[n] = 0;
}
static void list_auctions(struct char_data *ch, const char *argument)
{
  char filter[MAX_INPUT_LENGTH], key[MAX_INPUT_LENGTH], candidate[MAX_INPUT_LENGTH], wear_sql[256];
  const char *text;
  char *q;
  int type = -1, mine = 0, wear = -1, i;
  MYSQL_RES *res;
  MYSQL_ROW row;
  int count = 0;
  text = one_argument(argument, filter, sizeof(filter));
  skip_spaces_c(&text);
  filter_key(filter, key, sizeof(key));
  if (!str_cmp(key, "mine"))
    mine = 1;
  else if (*key && str_cmp(key, "all"))
  {
    for (i = 0; i < NUM_ITEM_TYPES; i++)
    {
      if (!item_types[i])
        continue;
      filter_key(item_types[i], candidate, sizeof(candidate));
      if (!strncmp(candidate, key, strlen(key)))
      {
        type = i;
        break;
      }
    }
    if (type < 0)
      for (i = 1; i < (int)wear_bits_count; i++)
      {
        if (!wear_bits[i])
          continue;
        filter_key(wear_bits[i], candidate, sizeof(candidate));
        if (!strncmp(candidate, key, strlen(key)))
        {
          wear = i;
          break;
        }
      }
    if (type < 0 && wear < 0)
      text = argument;
  }
  strlcpy(wear_sql, "1", sizeof(wear_sql));
  if (wear >= 0)
    snprintf(wear_sql, sizeof(wear_sql),
             "(CAST(SUBSTRING_INDEX(SUBSTRING_INDEX(wear_flags,' ',%d),' ',-1) AS UNSIGNED) & "
             "%llu) != 0",
             wear / 32 + 1, 1ULL << (wear % 32));
  q = estate_quote(text);
  if (!q)
    return;
  res = estate_select(
      "SELECT listing_id,item_name,price,seller_name,expires_at,expires_at<=NOW() FROM "
      "auction_listings WHERE listing_state='active' AND (%d=1 OR expires_at>NOW()) AND (%d=0 OR "
      "seller_id=%ld) AND (%d=-1 OR item_type=%d) AND (%s) AND (item_name LIKE '%%%s%%' OR "
      "keywords LIKE '%%%s%%') ORDER BY listing_id LIMIT 200",
      mine, mine, GET_IDNUM(ch), type, type, wear_sql, q, q);
  free(q);
  if (!res)
  {
    send_to_char(ch, "Auction listings are unavailable.\r\n");
    return;
  }
  send_to_char(ch, "ID      Gold       Seller             Item / Expiry\r\n");
  while ((row = mysql_fetch_row(res)))
  {
    send_to_char(ch, "%-7s %-10s %-18s %s [%s%s]\r\n", row[0], row[2], row[3], row[1], row[4],
                 atoi(row[5]) ? "; recoverable" : "");
    count++;
  }
  if (!count)
    send_to_char(ch, "No auction listings matched.\r\n");
  mysql_free_result(res);
}
void auction_house_command(struct char_data *ch, const char *argument)
{
  char verb[MAX_INPUT_LENGTH], arg[MAX_INPUT_LENGTH];
  const char *text;
  MYSQL_RES *res;
  MYSQL_ROW row;
  long price, id;
  struct obj_data *obj;
  if (IS_NPC(ch))
    return;
  if (!estate_init())
  {
    send_to_char(ch, "The auction house database is unavailable.\r\n");
    return;
  }
  text = one_argument(argument, verb, sizeof(verb));
  skip_spaces_c(&text);
  if (!*verb)
  {
    send_to_char(
        ch,
        "Auction house: list [all|mine|item type|wear slot] [keywords], view <ID>, sell <item> "
        "<price>, buy <ID>, cancel <ID>, recover <expired ID>, reprice <ID> <price>, collect.\r\n");
    return;
  }
  if (!str_cmp(verb, "list"))
  {
    list_auctions(ch, text);
    return;
  }
  if (!str_cmp(verb, "collect"))
  {
    auction_house_collect(ch);
    return;
  }
  if (!str_cmp(verb, "sell"))
  {
    char *packed, *q, *name, *keywords, *seller, bits[128];
    unsigned long long escrow, listing;
    text = one_argument(text, arg, sizeof(arg));
    skip_spaces_c(&text);
    if (!valid_price(text, &price))
    {
      send_to_char(ch, "Set a price from 1 to %d gold.\r\n", AUCTION_MAX_PRICE);
      return;
    }
    obj = get_obj_in_list_vis(ch, arg, NULL, ch->carrying);
    if (!obj)
    {
      send_to_char(ch, "You are not carrying that item.\r\n");
      return;
    }
    if (!OBJ_FLAGGED(obj, ITEM_IDENTIFIED) || !transferable(obj))
    {
      send_to_char(ch, "Only identified, unbound, transferable items can be auctioned.\r\n");
      return;
    }
    escrow = estate_prepare_escrow(ch, obj);
    if (!escrow)
    {
      send_to_char(ch, "Your inventory could not be saved; listing cancelled.\r\n");
      return;
    }
    packed = estate_pack(obj);
    if (!packed)
    {
      send_to_char(ch, "The item could not be serialized.\r\n");
      return;
    }
    q = estate_quote(packed);
    name = estate_quote(obj->short_description);
    keywords = estate_quote(obj->name);
    seller = estate_quote(GET_NAME(ch));
    free(packed);
    snprintf(bits, sizeof(bits), "%llu %llu %llu %llu",
             (unsigned long long)(unsigned int)GET_OBJ_WEAR(obj)[0],
             (unsigned long long)(unsigned int)GET_OBJ_WEAR(obj)[1],
             (unsigned long long)(unsigned int)GET_OBJ_WEAR(obj)[2],
             (unsigned long long)(unsigned int)GET_OBJ_WEAR(obj)[3]);
    if (q && name && keywords && seller && estate_query("START TRANSACTION") &&
        estate_query("INSERT INTO "
                     "auction_listings(seller_id,seller_name,item_name,keywords,price,item_type,"
                     "wear_flags,serialized_obj,expires_at) "
                     "VALUES(%ld,'%s','%s','%s',%ld,%d,'%s','%s',DATE_ADD(NOW(),INTERVAL 14 DAY))",
                     GET_IDNUM(ch), seller, name, keywords, price, GET_OBJ_TYPE(obj), bits, q))
    {
      listing = mysql_insert_id(conn);
      if (!estate_commit_escrow(escrow) || !estate_query("COMMIT"))
      {
        estate_query("ROLLBACK");
        free(q);
        free(name);
        free(keywords);
        free(seller);
        send_to_char(ch, "Listing failed; you keep your item.\r\n");
        return;
      }
      act("You place $p in auction escrow.", FALSE, ch, obj, 0, TO_CHAR);
      obj_from_char(obj);
      extract_obj(obj);
      estate_recover(ch);
      send_to_char(ch, "Listing %llu created for %ld gold, expiring in 14 days.\r\n", listing,
                   price);
    }
    else
    {
      estate_query("ROLLBACK");
      send_to_char(ch, "Listing failed; you keep your item.\r\n");
    }
    free(q);
    free(name);
    free(keywords);
    free(seller);
    return;
  }
  text = one_argument(text, arg, sizeof(arg));
  id = atol(arg);
  if (id < 1)
  {
    send_to_char(ch, "Specify a valid listing ID.\r\n");
    return;
  }
  if (!str_cmp(verb, "view") || !str_cmp(verb, "stats"))
  {
    res = estate_select("SELECT serialized_obj FROM auction_listings WHERE listing_id=%ld AND "
                        "listing_state='active'",
                        id);
    if (res)
    {
      row = mysql_fetch_row(res);
      obj = row ? estate_unpack(row[0]) : NULL;
      mysql_free_result(res);
      if (obj)
      {
        call_magic(ch, NULL, obj, SPELL_IDENTIFY, 0, GET_LEVEL(ch), CAST_SPELL);
        extract_obj(obj);
      }
      else
        send_to_char(ch, "Listing cannot be viewed.\r\n");
    }
    return;
  }
  if (!str_cmp(verb, "reprice"))
  {
    skip_spaces_c(&text);
    if (!valid_price(text, &price))
    {
      send_to_char(ch, "Invalid price.\r\n");
      return;
    }
    if (estate_query("UPDATE auction_listings SET price=%ld WHERE listing_id=%ld AND seller_id=%ld "
                     "AND listing_state='active' AND expires_at>NOW()",
                     price, id, GET_IDNUM(ch)) &&
        mysql_affected_rows(conn) == 1)
      send_to_char(ch, "Price updated.\r\n");
    else
      send_to_char(ch, "That active listing is not yours.\r\n");
    return;
  }
  if (!str_cmp(verb, "buy") || !str_cmp(verb, "cancel") || !str_cmp(verb, "recover"))
  {
    int buying = !str_cmp(verb, "buy"), expired, ok;
    long seller;
    char *payload = NULL;
    if (!estate_player_ready(ch) || !estate_query("START TRANSACTION"))
      return;
    res =
        estate_select("SELECT seller_id,price,serialized_obj,expires_at<=NOW() FROM "
                      "auction_listings WHERE listing_id=%ld AND listing_state='active' FOR UPDATE",
                      id);
    if (!res)
      goto fail;
    row = mysql_fetch_row(res);
    if (!row)
    {
      mysql_free_result(res);
      send_to_char(ch, "No active listing with that ID.\r\n");
      goto fail;
    }
    seller = atol(row[0]);
    price = atol(row[1]);
    expired = atoi(row[3]);
    payload = strdup(row[2]);
    mysql_free_result(res);
    if ((buying && (expired || seller == GET_IDNUM(ch) || GET_GOLD(ch) < price)) ||
        (!buying && (seller != GET_IDNUM(ch) || (!str_cmp(verb, "recover") && !expired))))
    {
      send_to_char(ch, "That transaction is not available to you.\r\n");
      free(payload);
      goto fail;
    }
    obj = estate_unpack(payload);
    free(payload);
    if (!obj)
    {
      send_to_char(ch, "The item could not be restored; escrow is unchanged.\r\n");
      goto fail;
    }
    if (!CAN_CARRY_OBJ(ch, obj))
    {
      extract_obj(obj);
      send_to_char(ch, "You cannot carry that item.\r\n");
      goto fail;
    }
    ok = estate_query("UPDATE auction_listings SET listing_state='%s',buyer_id=%ld WHERE "
                      "listing_id=%ld AND listing_state='active'",
                      buying    ? "sold"
                      : expired ? "expired"
                                : "cancelled",
                      GET_IDNUM(ch), id);
    if (ok && buying)
      ok = estate_query("INSERT INTO auction_credit_settlements(listing_id,beneficiary_id,amount) "
                        "VALUES(%ld,%ld,%ld)",
                        id, seller, price);
    /* A committed delivery is recovered on login if the player save is interrupted. */
    if (ok)
      ok = estate_queue_delivery(ch, obj, buying ? price : 0);
    if (!ok || !estate_query("COMMIT"))
    {
      extract_obj(obj);
      goto fail;
    }
    extract_obj(obj);
    estate_recover(ch);
    if (estate_player_ready(ch))
      send_to_char(ch, "Auction transaction completed.\r\n");
    else
      send_to_char(ch, "The auction is recorded; your item remains pending delivery.\r\n");
    return;
  fail:
    estate_query("ROLLBACK");
    send_to_char(ch, "Auction transaction could not be completed.\r\n");
    return;
  }
  send_to_char(ch, "Unknown auction command. Type auctionhouse for help.\r\n");
}
/* This spec can be attached to either a room or an auctioneer mobile in OLC. */
SPECIAL(mysql_auction_house)
{
  char buffer[MAX_INPUT_LENGTH + 16];
  if (!cmd || IS_NPC(ch))
    return FALSE;
  if (CMD_IS("auctionhouse"))
  {
    auction_house_command(ch, argument);
    return TRUE;
  }
  if (CMD_IS("list") || CMD_IS("sell") || CMD_IS("buy") || CMD_IS("view") || CMD_IS("stats") ||
      CMD_IS("recover") || CMD_IS("cancel") || CMD_IS("reprice") || CMD_IS("collect"))
  {
    snprintf(buffer, sizeof(buffer), "%s %s", CMD_NAME, argument);
    auction_house_command(ch, buffer);
    return TRUE;
  }
  return FALSE;
}
ACMD(do_auctionhouse)
{
  /* Keep purchases local to an assigned auctioneer; the spec receives this first. */
  if (GET_LEVEL(ch) >= LVL_GRSTAFF)
    auction_house_command(ch, argument);
  else
    send_to_char(ch, "Visit an auction house to use this command.\r\n");
}
