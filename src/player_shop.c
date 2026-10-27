/* Persistent, fixed-price player stalls. Ownership uses stable character and
 * account IDs. Item/currency transfers share the estate recovery journal. */
#include "conf.h"
#include "sysdep.h"
#include "legendary_equipment.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "estate.h"
#include "handler.h"
#include "interpreter.h"
#include "player_shop.h"
#include "spells.h"

#define STALL_MAX_PRICE 99999999L
/* Retain historical leases/stock/sales without keeping a market in a room. */
#define STALL_REMOVED_ROOM -1

/* The standalone SQL migration is generated from these same definitions. */
static const char *stall_schema[] = {
    "CREATE TABLE IF NOT EXISTS player_shop_stalls (stall_id BIGINT PRIMARY KEY AUTO_INCREMENT,"
    "room_vnum INT NOT NULL,weekly_rent INT NOT NULL DEFAULT 1000,capacity INT NOT NULL DEFAULT 20,"
    "enabled TINYINT NOT NULL DEFAULT 1,KEY room(room_vnum)) "
    "ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci",
    "CREATE TABLE IF NOT EXISTS player_shop_leases (lease_id BIGINT PRIMARY KEY AUTO_INCREMENT,"
    "stall_id BIGINT NOT NULL,held_account BIGINT NULL UNIQUE,held_stall BIGINT NULL UNIQUE,"
    "owner_id BIGINT NOT NULL,owner_name VARCHAR(64) NOT NULL,name VARCHAR(80) NOT NULL,"
    "expires_at DATETIME NOT NULL,created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
    "KEY owner(owner_id),FOREIGN KEY(stall_id) REFERENCES player_shop_stalls(stall_id)) "
    "ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci",
    "CREATE TABLE IF NOT EXISTS player_shop_listings (listing_id BIGINT PRIMARY KEY AUTO_INCREMENT,"
    "lease_id BIGINT NOT NULL,item_name VARCHAR(255) NOT NULL,keywords TEXT NOT NULL,"
    "price BIGINT NOT NULL,serialized_obj LONGTEXT NOT NULL,"
    "state ENUM('active','sold','withdrawn') NOT NULL DEFAULT 'active',"
    "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,KEY stock(lease_id,state),"
    "FOREIGN KEY(lease_id) REFERENCES player_shop_leases(lease_id)) "
    "ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci",
    "CREATE TABLE IF NOT EXISTS player_shop_sales (sale_id BIGINT PRIMARY KEY AUTO_INCREMENT,"
    "listing_id BIGINT NOT NULL UNIQUE,seller_id BIGINT NOT NULL,buyer_id BIGINT NOT NULL,"
    "price BIGINT NOT NULL,paid TINYINT NOT NULL DEFAULT 0,"
    "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,KEY proceeds(seller_id,paid),"
    "FOREIGN KEY(listing_id) REFERENCES player_shop_listings(listing_id)) "
    "ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci",
    "CREATE TABLE IF NOT EXISTS player_shop_events (event_id BIGINT PRIMARY KEY AUTO_INCREMENT,"
    "stall_id BIGINT NOT NULL,actor_id BIGINT NOT NULL,action VARCHAR(32) NOT NULL,"
    "amount BIGINT NOT NULL DEFAULT 0,created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
    "KEY history(stall_id,event_id)) "
    "ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci", NULL};

struct stall_room
{
  room_vnum vnum;
  struct stall_room *next;
};
static struct stall_room *stall_rooms;
static int stall_ready;

static int refresh_rooms(void)
{
  MYSQL_RES *res = estate_select("SELECT DISTINCT room_vnum FROM player_shop_stalls WHERE room_vnum>=0");
  MYSQL_ROW row;
  struct stall_room *room;
  if (!res)
    return FALSE;
  while (stall_rooms)
  {
    room = stall_rooms;
    stall_rooms = room->next;
    free(room);
  }
  while ((row = mysql_fetch_row(res)))
  {
    CREATE(room, struct stall_room, 1);
    room->vnum = atoi(row[0]);
    room->next = stall_rooms;
    stall_rooms = room;
  }
  mysql_free_result(res);
  return TRUE;
}
int player_shop_init(void)
{
  int i, present;
  char table[80];
  MYSQL_RES *res;
  if (!estate_init())
    return FALSE;
  if (stall_ready)
    return TRUE;
  for (i = 0; stall_schema[i]; i++)
  {
    if (sscanf(stall_schema[i], "CREATE TABLE IF NOT EXISTS %79s", table) != 1)
      return FALSE;
    res = estate_select("SELECT 1 FROM information_schema.tables WHERE table_schema=DATABASE() "
                        "AND table_name='%s'", table);
    if (!res)
      return FALSE;
    present = mysql_num_rows(res) != 0;
    mysql_free_result(res);
    if (!present && !estate_query("%s", stall_schema[i]))
      return FALSE;
  }
  stall_ready = refresh_rooms();
  return stall_ready;
}
static int expire_leases(void)
{
  /* Historical leases and their stock survive repossession. NULL unique keys
   * release both the account and stall without exposing an old tenant's goods. */
  return estate_query("UPDATE player_shop_leases SET held_account=NULL,held_stall=NULL "
                      "WHERE held_account IS NOT NULL AND expires_at<=DATE_SUB(NOW(),INTERVAL 48 HOUR)");
}
static int number(const char *s, long maximum, long *out)
{
  char *end;
  long value;
  errno = 0;
  if (!*s || !isdigit((unsigned char)*s))
    return FALSE;
  value = strtol(s, &end, 10);
  while (isspace((unsigned char)*end))
    end++;
  if (errno || *end || value < 1 || value > maximum)
    return FALSE;
  *out = value;
  return TRUE;
}
static room_vnum here(struct char_data *ch)
{
  return IN_ROOM(ch) == NOWHERE ? NOWHERE : GET_ROOM_VNUM(IN_ROOM(ch));
}
static int account_id(struct char_data *ch)
{
  return ch->desc && ch->desc->account ? ch->desc->account->id : 0;
}
static int audit(struct char_data *ch, long stall, const char *action, long amount)
{
  return estate_query("INSERT INTO player_shop_events(stall_id,actor_id,action,amount) "
                      "VALUES(%ld,%ld,'%s',%ld)", stall, GET_IDNUM(ch), action, amount);
}
static void failed(struct char_data *ch)
{
  estate_query("ROLLBACK");
  send_to_char(ch, "Stall transaction could not be completed; any committed transfer remains recoverable.\r\n");
}
static int finish(struct char_data *ch)
{
  if (!estate_query("COMMIT"))
  {
    failed(ch);
    return FALSE;
  }
  estate_recover(ch);
  send_to_char(ch, "Stall transaction recorded.\r\n");
  return TRUE;
}
static int clean_name(const char *s)
{
  const unsigned char *p = (const unsigned char *)s;
  if (!*s || strlen(s) > 80)
    return FALSE;
  for (; *p; p++)
    if (*p < 32 || *p == 127 || *p == '$')
      return FALSE;
  return TRUE;
}
static int sellable(struct obj_data *obj)
{
  if (legendary_tree_present(obj)) return FALSE;
  return OBJ_FLAGGED(obj, ITEM_IDENTIFIED) && !OBJ_FLAGGED(obj, ITEM_NORENT) &&
         !OBJ_FLAGGED(obj, ITEM_NOSELL) && !OBJ_FLAGGED(obj, ITEM_QUEST) && !OBJ_FLAGGED(obj, ITEM_NODROP) &&
         GET_OBJ_BOUND_ID(obj) <= 0 && GET_OBJ_TYPE(obj) != ITEM_MONEY &&
         !obj->contains && !obj->sheath_primary && !obj->sheath_secondary;
}
static const char *market_name(room_vnum vnum)
{
  room_rnum room = real_room(vnum);
  return room == NOWHERE ? "Unavailable market" : world[room].name;
}
static void list_stalls(struct char_data *ch, int directory)
{
  MYSQL_RES *res;
  MYSQL_ROW row;
  res = estate_select("SELECT s.stall_id,s.room_vnum,s.weekly_rent,s.capacity,s.enabled,"
                      "COALESCE(l.name,'Available'),COALESCE(l.owner_name,''),"
                      "COALESCE(l.expires_at,''),COALESCE(l.expires_at>NOW(),0) "
                      "FROM player_shop_stalls s LEFT JOIN player_shop_leases l "
                      "ON l.held_stall=s.stall_id WHERE s.room_vnum>=0 AND (%d=1 OR s.room_vnum=%d) "
                      "ORDER BY s.room_vnum,s.stall_id LIMIT 200", directory, here(ch));
  if (!res)
    return;
  send_to_char(ch, "Stall  Weekly rent  Capacity  Status / Name / Owner / Lease expiry\r\n");
  while ((row = mysql_fetch_row(res)))
    send_to_char(ch, "#%s: %s gold, %s items, %s | %s | %s %s [%s, room %s]\r\n",
                 row[0], row[2], row[3], !atoi(row[4]) ? "disabled" :
                 !*row[6] ? "available" : atoi(row[8]) ? "open" : "renewal grace",
                 row[5], row[6], row[7], market_name(atoi(row[1])), row[1]);
  mysql_free_result(res);
}
void player_shop_show_room(struct char_data *ch)
{
  struct stall_room *room;
  MYSQL_RES *res;
  MYSQL_ROW row;
  if (IS_NPC(ch) || !player_shop_init())
    return;
  for (room = stall_rooms; room && room->vnum != here(ch); room = room->next)
    ;
  if (!room)
    return;
  res = estate_select("SELECT s.stall_id,l.name,l.expires_at>NOW() FROM player_shop_stalls s "
                      "LEFT JOIN player_shop_leases l ON l.held_stall=s.stall_id AND "
                      "l.expires_at>DATE_SUB(NOW(),INTERVAL 48 HOUR) "
                      "WHERE s.room_vnum=%d AND s.enabled=1 ORDER BY s.stall_id", here(ch));
  if (!res)
    return;
  while ((row = mysql_fetch_row(res)))
  {
    if (row[1] && atoi(row[2]))
      send_to_char(ch, "A stall merchant tends %s (stall #%s). [stall browse %s]\r\n", row[1], row[0], row[0]);
    else if (row[1])
      send_to_char(ch, "Stall #%s, %s, is closed pending rent renewal.\r\n", row[0], row[1]);
    else
      send_to_char(ch, "Stall #%s is available to rent. [stall rent %s]\r\n", row[0], row[0]);
  }
  mysql_free_result(res);
}
static void stock_list(struct char_data *ch, const char *verb, const char *text)
{
  MYSQL_RES *res;
  MYSQL_ROW row;
  char *q;
  long stall = 0;
  int mine = !str_cmp(verb, "info"), search = !str_cmp(verb, "search");
  if (!mine && !search && !number(text, LONG_MAX, &stall))
  {
    send_to_char(ch, "Use stall browse <stall number>.\r\n");
    return;
  }
  q = estate_quote(search ? text : "");
  if (!q)
    return;
  res = estate_select("SELECT i.listing_id,i.item_name,i.price,s.stall_id,s.room_vnum,l.name,"
                      "l.expires_at,(l.held_stall IS NOT NULL AND l.expires_at>NOW() AND s.enabled=1),"
                      "l.owner_name FROM player_shop_listings i JOIN player_shop_leases l USING(lease_id) "
                      "JOIN player_shop_stalls s ON s.stall_id=l.stall_id WHERE i.state='active' AND "
                      "((%d=1 AND l.owner_id=%ld) OR (%d=0 AND l.held_stall IS NOT NULL AND "
                      "l.expires_at>NOW() AND s.enabled=1 AND ((%d=1 AND "
                      "(i.item_name LIKE '%%%s%%' OR i.keywords LIKE '%%%s%%')) OR "
                      "(%d=0 AND s.stall_id=%ld AND s.room_vnum=%d)))) "
                      "ORDER BY s.stall_id,i.listing_id LIMIT 200",
                      mine, GET_IDNUM(ch), mine, search, q, q, search, stall, here(ch));
  free(q);
  if (!res)
    return;
  if (mine)
  {
    MYSQL_RES *lease = estate_select("SELECT s.stall_id,l.name,l.expires_at,s.weekly_rent,s.capacity "
                                    "FROM player_shop_leases l JOIN player_shop_stalls s USING(stall_id) "
                                    "WHERE l.owner_id=%ld AND l.held_stall IS NOT NULL", GET_IDNUM(ch));
    if (lease)
    {
      while ((row = mysql_fetch_row(lease)))
        send_to_char(ch, "Stall #%s: %s; expires %s; rent %s gold/week; capacity %s.\r\n",
                     row[0], row[1], row[2], row[3], row[4]);
      mysql_free_result(lease);
    }
  }
  send_to_char(ch, "Listing  Gold  Item / Stall / Market\r\n");
  while ((row = mysql_fetch_row(res)))
    send_to_char(ch, "#%s: %s gold | %s | #%s %s (%s) | %s, room %s%s\r\n",
                 row[0], row[2], row[1], row[3], row[5], row[8], market_name(atoi(row[4])),
                 row[4], atoi(row[7]) ? "" : " [closed; withdraw to reclaim]");
  mysql_free_result(res);
}
static void rent_stall(struct char_data *ch, long stall)
{
  MYSQL_RES *res;
  MYSQL_ROW row;
  long cost;
  char *owner;
  int available;
  if (account_id(ch) <= 0 || !estate_player_ready(ch))
  {
    send_to_char(ch, "A logged-in account is required to rent a stall.\r\n");
    return;
  }
  owner = estate_quote(GET_NAME(ch));
  if (!owner)
    return;
  if (!estate_query("START TRANSACTION"))
    goto fail;
  res = estate_select("SELECT weekly_rent FROM player_shop_stalls WHERE stall_id=%ld AND "
                      "room_vnum=%d AND enabled=1 FOR UPDATE", stall, here(ch));
  if (!res)
    goto fail;
  row = mysql_fetch_row(res);
  cost = row ? atol(row[0]) : 0;
  mysql_free_result(res);
  res = estate_select("SELECT lease_id FROM player_shop_leases WHERE held_stall=%ld OR "
                      "held_account=%d FOR UPDATE", stall, account_id(ch));
  available = res && !mysql_num_rows(res);
  if (res)
    mysql_free_result(res);
  if (cost < 1 || cost > STALL_MAX_PRICE || GET_GOLD(ch) < cost || !available)
  {
    send_to_char(ch, "Rent requires an available local stall, enough gold, and no other stall on your account.\r\n");
    goto fail;
  }
  if (!estate_query("INSERT INTO player_shop_leases(stall_id,held_stall,held_account,owner_id,"
                    "owner_name,name,expires_at) VALUES(%ld,%ld,%d,%ld,'%s','A player stall',"
                    "DATE_ADD(NOW(),INTERVAL 7 DAY))", stall, stall, account_id(ch), GET_IDNUM(ch),
                    owner) || !estate_queue_money(GET_IDNUM(ch), cost) ||
      !audit(ch, stall, "rent", cost))
    goto fail;
  free(owner);
  finish(ch);
  return;
fail:
  free(owner);
  failed(ch);
}
/* Lock the current lease together with its stall. All stock, lease and pricing
 * changes use this lock, so capacity and expiry cannot change mid-transaction. */
static long own_lease(struct char_data *ch, long *stall, long *cost, long *capacity, int active)
{
  MYSQL_RES *res = estate_select("SELECT l.lease_id,s.stall_id,s.weekly_rent,s.capacity "
                                "FROM player_shop_leases l JOIN player_shop_stalls s USING(stall_id) "
                                "WHERE l.owner_id=%ld AND l.held_stall IS NOT NULL AND s.room_vnum=%d "
                                "AND s.enabled=1 AND (%d=0 OR l.expires_at>NOW()) FOR UPDATE",
                                GET_IDNUM(ch), here(ch), active);
  MYSQL_ROW row;
  long id = 0;
  if (!res)
    return 0;
  row = mysql_fetch_row(res);
  if (row)
  {
    id = atol(row[0]);
    *stall = atol(row[1]);
    *cost = atol(row[2]);
    *capacity = atol(row[3]);
  }
  mysql_free_result(res);
  if (!id)
    send_to_char(ch, "Visit your enabled, rented stall; stocking and customization also require paid rent.\r\n");
  return id;
}
static void manage_stall(struct char_data *ch, const char *verb, const char *text)
{
  long lease, stall, rent, capacity, count;
  char *name;
  if (!estate_player_ready(ch) || !estate_query("START TRANSACTION"))
    return;
  lease = own_lease(ch, &stall, &rent, &capacity, !str_cmp(verb, "name"));
  if (!lease)
    goto fail;
  if (!str_cmp(verb, "renew"))
  {
    if (GET_GOLD(ch) < rent)
    {
      send_to_char(ch, "Renewal costs %ld gold; you do not have enough.\r\n", rent);
      goto fail;
    }
    if (*text || rent < 1 || rent > STALL_MAX_PRICE ||
        !estate_query("UPDATE player_shop_leases SET expires_at=DATE_ADD(GREATEST(expires_at,NOW()),"
                      "INTERVAL 7 DAY) WHERE lease_id=%ld", lease) ||
        !estate_queue_money(GET_IDNUM(ch), rent) || !audit(ch, stall, "renew", rent))
      goto fail;
  }
  else if (!str_cmp(verb, "name"))
  {
    if (!clean_name(text))
    {
      send_to_char(ch, "Use a stall name of 1-80 characters without control codes or dollar signs.\r\n");
      goto fail;
    }
    name = estate_quote(text);
    if (!name)
      goto fail;
    count = estate_query("UPDATE player_shop_leases SET name='%s' WHERE lease_id=%ld", name, lease);
    free(name);
    if (!count || !audit(ch, stall, "name", 0))
      goto fail;
  }
  else /* Voluntary closure: goods remain in the old lease for recovery. */
  {
    if (!estate_query("UPDATE player_shop_leases SET held_stall=NULL,held_account=NULL WHERE lease_id=%ld", lease) ||
        !audit(ch, stall, "close", 0))
      goto fail;
  }
  finish(ch);
  return;
fail:
  failed(ch);
}
static void sell_item(struct char_data *ch, const char *text)
{
  char arg[MAX_INPUT_LENGTH];
  char *packed = NULL, *payload = NULL, *name = NULL, *keywords = NULL;
  long price, lease, stall, rent, capacity, count;
  unsigned long long escrow, listing;
  MYSQL_RES *res;
  MYSQL_ROW row;
  struct obj_data *obj;
  text = one_argument(text, arg, sizeof(arg));
  skip_spaces_c(&text);
  if (!number(text, STALL_MAX_PRICE, &price))
  {
    send_to_char(ch, "Use stall sell <inventory item> <price: 1-99999999>.\r\n");
    return;
  }
  obj = get_obj_in_list_vis(ch, arg, NULL, ch->carrying);
  if (!obj || !sellable(obj))
  {
    send_to_char(ch, "Carry an identified, unbound, transferable item. Empty containers and sheaths first.\r\n");
    return;
  }
  escrow = estate_prepare_escrow(ch, obj);
  if (!escrow)
    return;
  packed = estate_pack(obj);
  payload = packed ? estate_quote(packed) : NULL;
  name = estate_quote(obj->short_description);
  keywords = estate_quote(obj->name);
  free(packed);
  if (!payload || !name || !keywords || !estate_query("START TRANSACTION"))
    goto fail;
  lease = own_lease(ch, &stall, &rent, &capacity, TRUE);
  if (!lease)
    goto fail;
  res = estate_select("SELECT COUNT(*) FROM player_shop_listings WHERE lease_id=%ld AND state='active'", lease);
  if (!res)
    goto fail;
  row = mysql_fetch_row(res);
  count = row ? atol(row[0]) : capacity;
  mysql_free_result(res);
  if (count >= capacity)
  {
    send_to_char(ch, "Your stall is full.\r\n");
    goto fail;
  }
  if (!estate_query("INSERT INTO player_shop_listings(lease_id,item_name,keywords,price,serialized_obj) "
                    "VALUES(%ld,'%s','%s',%ld,'%s')", lease, name, keywords, price, payload))
    goto fail;
  listing = mysql_insert_id(conn);
  if (!estate_commit_escrow(escrow) || !audit(ch, stall, "stock", price))
    goto fail;
  send_to_char(ch, "Listing #%llu prepared.\r\n", listing);
  if (finish(ch))
    send_to_char(ch, "Your item is now offered for %ld gold.\r\n", price);
  goto done;
fail:
  failed(ch);
done:
  free(payload);
  free(name);
  free(keywords);
}
static void listing_action(struct char_data *ch, const char *verb, long id, const char *text)
{
  MYSQL_RES *res;
  MYSQL_ROW row;
  struct obj_data *obj = NULL;
  long seller, stall, price, vnum, new_price = 0;
  int open, buying = !str_cmp(verb, "buy"), withdraw = !str_cmp(verb, "withdraw");
  int inspect = !str_cmp(verb, "inspect") || !str_cmp(verb, "view");
  char *payload;
  if (!inspect && !buying && !withdraw && !number(text, STALL_MAX_PRICE, &new_price))
  {
    send_to_char(ch, "Use stall reprice <listing ID> <price: 1-99999999>.\r\n");
    return;
  }
  if (!estate_player_ready(ch) || !estate_query("START TRANSACTION"))
    return;
  res = estate_select("SELECT l.owner_id,s.stall_id,i.price,s.room_vnum,"
                      "(s.enabled=1 AND l.held_stall IS NOT NULL AND l.expires_at>NOW()),i.serialized_obj "
                      "FROM player_shop_listings i JOIN player_shop_leases l USING(lease_id) "
                      "JOIN player_shop_stalls s ON s.stall_id=l.stall_id "
                      "WHERE i.listing_id=%ld AND i.state='active' FOR UPDATE", id);
  if (!res)
    goto fail;
  row = mysql_fetch_row(res);
  if (!row)
  {
    mysql_free_result(res);
    send_to_char(ch, "There is no unsold listing with that ID.\r\n");
    goto fail;
  }
  seller = atol(row[0]);
  stall = atol(row[1]);
  price = atol(row[2]);
  vnum = atol(row[3]);
  open = atoi(row[4]);
  payload = strdup(row[5]);
  mysql_free_result(res);
  /* Owners may recover old stock anywhere, even after a new tenant rents the stall. */
  if ((!withdraw && (vnum != here(ch) || !open)) ||
      (withdraw && seller != GET_IDNUM(ch)) ||
      (!inspect && !buying && seller != GET_IDNUM(ch)) ||
      (buying && (seller == GET_IDNUM(ch) || price < 1 || price > STALL_MAX_PRICE || GET_GOLD(ch) < price)))
  {
    free(payload);
    send_to_char(ch, "Visit the open stall with enough gold to buy; only its owner can manage stock.\r\n");
    goto fail;
  }
  if (!inspect && !buying && !withdraw)
  {
    free(payload);
    if (!estate_query("UPDATE player_shop_listings SET price=%ld WHERE listing_id=%ld", new_price, id) ||
        !audit(ch, stall, "reprice", new_price))
      goto fail;
    finish(ch);
    return;
  }
  obj = estate_unpack(payload);
  free(payload);
  if (!obj)
    goto fail;
  if (inspect)
  {
    estate_query("ROLLBACK");
    call_magic(ch, NULL, obj, SPELL_IDENTIFY, 0, GET_LEVEL(ch), CAST_SPELL);
    extract_obj(obj);
    return;
  }
  if (!CAN_CARRY_OBJ(ch, obj))
  {
    send_to_char(ch, "You cannot carry that item.\r\n");
    goto fail;
  }
  if (!estate_query("UPDATE player_shop_listings SET state='%s' WHERE listing_id=%ld AND state='active'",
                    buying ? "sold" : "withdrawn", id) || mysql_affected_rows(conn) != 1)
    goto fail;
  if (buying && !estate_query("INSERT INTO player_shop_sales(listing_id,seller_id,buyer_id,price) "
                              "VALUES(%ld,%ld,%ld,%ld)", id, seller, GET_IDNUM(ch), price))
    goto fail;
  if (!estate_queue_delivery(ch, obj, buying ? price : 0) ||
      !audit(ch, stall, buying ? "buy" : "withdraw", buying ? price : 0))
    goto fail;
  extract_obj(obj);
  finish(ch);
  return;
fail:
  if (obj)
    extract_obj(obj);
  failed(ch);
}
static void earnings(struct char_data *ch, int collect)
{
  MYSQL_RES *res;
  MYSQL_ROW row;
  long amount;
  unsigned long long sale;
  if (collect && (!estate_player_ready(ch) || !estate_query("START TRANSACTION")))
    return;
  res = estate_select("SELECT sale_id,price FROM player_shop_sales WHERE seller_id=%ld AND paid=0 "
                      "ORDER BY sale_id%s", GET_IDNUM(ch), collect ? " FOR UPDATE" : "");
  if (!res)
  {
    if (collect)
      failed(ch);
    return;
  }
  while ((row = mysql_fetch_row(res)))
  {
    sale = strtoull(row[0], NULL, 10);
    amount = atol(row[1]);
    send_to_char(ch, "Sale #%llu: %ld gold %s.\r\n", sale, amount, collect ? "queued for collection" : "uncollected");
    if (collect && (!estate_queue_money(GET_IDNUM(ch), -amount) ||
                    !estate_query("UPDATE player_shop_sales SET paid=1 WHERE sale_id=%llu", sale)))
    {
      mysql_free_result(res);
      failed(ch);
      return;
    }
  }
  mysql_free_result(res);
  if (collect)
    finish(ch);
}
static void admin(struct char_data *ch, const char *text)
{
  char verb[MAX_INPUT_LENGTH], arg[MAX_INPUT_LENGTH], args[4][MAX_INPUT_LENGTH];
  long id, count = 1, rent = 1000, capacity = 20;
  int i;
  MYSQL_RES *res;
  MYSQL_ROW row;
  if (GET_LEVEL(ch) < LVL_GRSTAFF)
  {
    send_to_char(ch, "Only senior staff can configure stalls.\r\n");
    return;
  }
  text = one_argument(text, verb, sizeof(verb));
  text = one_argument(text, arg, sizeof(arg));
  if (!number(arg, INT_MAX, &id))
    goto usage;
  for (i = 0; i < 4; i++)
    text = one_argument(text, args[i], sizeof(args[i]));
  if (!str_cmp(verb, "delete-room"))
  {
    if (*args[0])
      goto usage;
    if (!estate_query("START TRANSACTION"))
      return;
    /* Match rent's stall-first locking order, then release all reservations.
     * Keep foreign-key targets so former tenants can retrieve stock/proceeds. */
    res = estate_select("SELECT stall_id FROM player_shop_stalls WHERE room_vnum=%ld "
                        "ORDER BY stall_id FOR UPDATE", id);
    if (!res)
      goto fail;
    count = mysql_num_rows(res);
    if (!count)
    {
      mysql_free_result(res);
      estate_query("ROLLBACK");
      send_to_char(ch, "There are no stalls configured in room %ld.\r\n", id);
      return;
    }
    while ((row = mysql_fetch_row(res)))
      if (!audit(ch, atol(row[0]), "delete-room", id))
      {
        mysql_free_result(res);
        goto fail;
      }
    mysql_free_result(res);
    if (!estate_query("UPDATE player_shop_leases l JOIN player_shop_stalls s USING(stall_id) "
                      "SET l.held_account=NULL,l.held_stall=NULL WHERE s.room_vnum=%ld", id) ||
        !estate_query("UPDATE player_shop_stalls SET enabled=0,room_vnum=%d WHERE room_vnum=%ld",
                      STALL_REMOVED_ROOM, id))
      goto fail;
    if (finish(ch))
    {
      if (!refresh_rooms())
        stall_ready = FALSE;
      send_to_char(ch, "Removed %ld stalls from room %ld. Owners retain their items and earnings.\r\n", count, id);
    }
    return;
  }
  if (!str_cmp(verb, "history"))
  {
    res = estate_select("SELECT actor_id,action,amount,created_at FROM player_shop_events "
                        "WHERE stall_id=%ld ORDER BY event_id DESC LIMIT 100", id);
    if (res)
    {
      while ((row = mysql_fetch_row(res)))
        send_to_char(ch, "Player %s: %s, %s gold, %s\r\n", row[0], row[1], row[2], row[3]);
      mysql_free_result(res);
    }
    return;
  }
  if (str_cmp(verb, "add"))
  {
    res = estate_select("SELECT stall_id FROM player_shop_stalls WHERE stall_id=%ld AND room_vnum>=0", id);
    if (!res)
      return;
    i = mysql_num_rows(res) != 0;
    mysql_free_result(res);
    if (!i)
    {
      send_to_char(ch, "There is no active stall with that ID.\r\n");
      return;
    }
  }
  if (!str_cmp(verb, "add"))
  {
    if (real_room(id) == NOWHERE || !number(args[0], 100, &count) ||
        (*args[1] && !number(args[1], STALL_MAX_PRICE, &rent)) ||
        (*args[2] && !number(args[2], 200, &capacity)) || *args[3])
      goto usage;
    if (!estate_query("START TRANSACTION"))
      return;
    for (i = 0; i < count; i++)
    {
      if (!estate_query("INSERT INTO player_shop_stalls(room_vnum,weekly_rent,capacity) VALUES(%ld,%ld,%ld)",
                        id, rent, capacity) || !audit(ch, (long)mysql_insert_id(conn), "create", rent))
        goto fail;
    }
  }
  else if (!str_cmp(verb, "set"))
  {
    if (!number(args[0], STALL_MAX_PRICE, &rent) || !number(args[1], 200, &capacity) || *args[2])
      goto usage;
    if (!estate_query("START TRANSACTION") ||
        !estate_query("UPDATE player_shop_stalls SET weekly_rent=%ld,capacity=%ld WHERE stall_id=%ld", rent, capacity, id) ||
        !audit(ch, id, "configure", rent))
      goto fail;
  }
  else if (!str_cmp(verb, "enable") || !str_cmp(verb, "disable"))
  {
    if (!estate_query("START TRANSACTION") ||
        !estate_query("UPDATE player_shop_stalls SET enabled=%d WHERE stall_id=%ld", !str_cmp(verb, "enable"), id) ||
        !audit(ch, id, verb, 0))
      goto fail;
  }
  else if (!str_cmp(verb, "evict"))
  {
    if (!estate_query("START TRANSACTION") ||
        !estate_query("UPDATE player_shop_leases SET held_account=NULL,held_stall=NULL WHERE held_stall=%ld", id) ||
        !audit(ch, id, "evict", 0))
      goto fail;
  }
  else
    goto usage;
  if (finish(ch) && !refresh_rooms())
    stall_ready = FALSE;
  return;
fail:
  failed(ch);
  return;
usage:
  send_to_char(ch, "stall admin add <room vnum> <count: 1-100> [weekly rent] [capacity: 1-200]\r\n"
                   "stall admin set <stall ID> <weekly rent> <capacity>\r\n"
                   "stall admin delete-room <room vnum> (preserves owner items and earnings)\r\n"
                   "stall admin enable|disable|evict|history <stall ID>\r\n");
}
void player_shop_command(struct char_data *ch, const char *argument)
{
  char verb[MAX_INPUT_LENGTH], arg[MAX_INPUT_LENGTH];
  const char *text;
  long id;
  if (IS_NPC(ch))
    return;
  if (!player_shop_init() || !expire_leases())
  {
    send_to_char(ch, "Player stalls are unavailable while the database is offline.\r\n");
    return;
  }
  text = one_argument(argument, verb, sizeof(verb));
  skip_spaces_c(&text);
  if (!*verb)
  {
    send_to_char(ch, "stall list | directory | search [keywords] | browse <stall ID> | inspect <listing ID>\r\n"
                     "stall rent <stall ID> | info | name <name> | renew | close\r\n"
                     "stall sell <item> <price> | reprice <listing ID> <price> | withdraw <listing ID>\r\n"
                     "stall buy <listing ID> | earnings | collect\r\n"
                     "Rent lasts 7 real days. Sales close on expiry; the stall is reserved for 48 more hours.\r\n");
    return;
  }
  if (!str_cmp(verb, "admin"))
    admin(ch, text);
  else if (!str_cmp(verb, "list") || !str_cmp(verb, "directory"))
    list_stalls(ch, !str_cmp(verb, "directory"));
  else if (!str_cmp(verb, "browse") || !str_cmp(verb, "search") || !str_cmp(verb, "info"))
    stock_list(ch, verb, text);
  else if (!str_cmp(verb, "name") || !str_cmp(verb, "renew") || !str_cmp(verb, "close"))
    manage_stall(ch, verb, text);
  else if (!str_cmp(verb, "sell"))
    sell_item(ch, text);
  else if (!str_cmp(verb, "earnings") || !str_cmp(verb, "collect"))
    earnings(ch, !str_cmp(verb, "collect"));
  else if (!str_cmp(verb, "rent") || !str_cmp(verb, "buy") || !str_cmp(verb, "withdraw") ||
           !str_cmp(verb, "reprice") || !str_cmp(verb, "inspect") || !str_cmp(verb, "view"))
  {
    text = one_argument(text, arg, sizeof(arg));
    skip_spaces_c(&text);
    if (!number(arg, LONG_MAX, &id) || (*text && str_cmp(verb, "reprice")))
      send_to_char(ch, "Specify a valid numeric stall or listing ID.\r\n");
    else if (!str_cmp(verb, "rent"))
      rent_stall(ch, id);
    else
      listing_action(ch, verb, id, text);
  }
  else
    send_to_char(ch, "Unknown stall command. Type stall for help.\r\n");
}
ACMD(do_stall)
{
  player_shop_command(ch, argument);
}
