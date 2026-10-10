/* Native C adaptation of d20StarWars persistent housing/auction services.
 * Objects use Krynn's complete object record format rather than SW JSON. */
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "estate.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include <stdarg.h>
extern int estate_write_record(struct obj_data *, FILE *);
extern obj_save_data *objsave_parse_objects(FILE *);

static int execute(const char *format, va_list ap)
{
  char *sql = NULL;
  int ok;
  if (!conn || !mysql_available)
    return FALSE;
  if (vasprintf(&sql, format, ap) < 0)
    return FALSE;
  ok = mysql_query(conn, sql) == 0;
  if (!ok)
    log("SYSERR: Estate database: %s", mysql_error(conn));
  free(sql);
  return ok;
}
int estate_query(const char *format, ...)
{
  va_list ap;
  int ok;
  va_start(ap, format);
  ok = execute(format, ap);
  va_end(ap);
  return ok;
}
MYSQL_RES *estate_select(const char *format, ...)
{
  va_list ap;
  int ok;
  va_start(ap, format);
  ok = execute(format, ap);
  va_end(ap);
  return ok ? mysql_store_result(conn) : NULL;
}
char *estate_quote(const char *value)
{
  size_t n = value ? strlen(value) : 0;
  char *s = malloc(2 * n + 1);
  if (!s || !conn)
  {
    free(s);
    return NULL;
  }
  mysql_real_escape_string(conn, s, value ? value : "", n);
  return s;
}

/* Length framing keeps customized multiline records and nested/sheathed items intact. */
static int pack_node(FILE *out, struct obj_data *obj, int depth)
{
  FILE *record;
  char *text = NULL;
  size_t size = 0;
  struct obj_data *it;
  int children = 0, weight, ok;
  if (depth > 64 || !obj)
    return FALSE;
  for (it = obj->contains; it; it = it->next_content)
    children++;
  if (children > 10000)
    return FALSE;
  record = open_memstream(&text, &size);
  if (!record)
    return FALSE;
  weight = GET_OBJ_WEIGHT(obj);
  for (it = obj->contains; it; it = it->next_content)
    GET_OBJ_WEIGHT(obj) -= GET_OBJ_WEIGHT(it);
  ok = estate_write_record(obj, record);
  GET_OBJ_WEIGHT(obj) = weight;
  if (fclose(record))
    ok = FALSE;
  if (size > 1048576)
    ok = FALSE;
  if (ok)
  {
    fprintf(out, "EOBJ1 %zu %d %d %d\n", size, children, !!obj->sheath_primary,
            !!obj->sheath_secondary);
    if (fwrite(text, 1, size, out) != size)
      ok = FALSE;
  }
  free(text);
  for (it = obj->contains; ok && it; it = it->next_content)
    ok = pack_node(out, it, depth + 1);
  if (ok && obj->sheath_primary)
    ok = pack_node(out, obj->sheath_primary, depth + 1);
  if (ok && obj->sheath_secondary)
    ok = pack_node(out, obj->sheath_secondary, depth + 1);
  return ok;
}
char *estate_pack(struct obj_data *obj)
{
  char *s = NULL;
  size_t n = 0;
  FILE *fp = open_memstream(&s, &n);
  int ok;
  if (!fp)
    return NULL;
  ok = pack_node(fp, obj, 0);
  if (fclose(fp))
    ok = FALSE;
  if (!ok)
  {
    free(s);
    return NULL;
  }
  return s;
}
static struct obj_data *unpack_node(FILE *in, int depth)
{
  size_t n;
  int count, primary, secondary, i;
  char header[128], *s;
  FILE *record;
  obj_save_data *loaded, *next;
  struct obj_data *obj, *child;
  if (depth > 64 || !fgets(header, sizeof(header), in) ||
      sscanf(header, "EOBJ1 %zu %d %d %d", &n, &count, &primary, &secondary) != 4 || n > 1048576 ||
      !n || count < 0 || count > 10000 || primary < 0 || primary > 1 || secondary < 0 ||
      secondary > 1)
    return NULL;
  s = malloc(n + 1);
  if (!s)
    return NULL;
  if (fread(s, 1, n, in) != n)
  {
    free(s);
    return NULL;
  }
  s[n] = 0;
  record = fmemopen(s, n, "r");
  if (!record)
  {
    free(s);
    return NULL;
  }
  loaded = objsave_parse_objects(record);
  fclose(record);
  free(s);
  if (!loaded)
    return NULL;
  obj = loaded->obj;
  next = loaded->next;
  free(loaded);
  if (next || !obj)
  {
    while (next)
    {
      loaded = next;
      next = next->next;
      if (loaded->obj)
        extract_obj(loaded->obj);
      free(loaded);
    }
    if (obj)
      extract_obj(obj);
    return NULL;
  }
  for (i = 0; i < count; i++)
  {
    child = unpack_node(in, depth + 1);
    if (!child)
    {
      extract_obj(obj);
      return NULL;
    }
    obj_to_obj(child, obj);
  }
  /* obj_to_obj prepends; restore the original contents order for numeric selectors. */
  {
    struct obj_data *previous = NULL, *it = obj->contains, *next_item;
    while (it)
    {
      next_item = it->next_content;
      it->next_content = previous;
      previous = it;
      it = next_item;
    }
    obj->contains = previous;
  }
  if (primary)
  {
    obj->sheath_primary = unpack_node(in, depth + 1);
    if (!obj->sheath_primary)
    {
      extract_obj(obj);
      return NULL;
    }
  }
  if (secondary)
  {
    obj->sheath_secondary = unpack_node(in, depth + 1);
    if (!obj->sheath_secondary)
    {
      extract_obj(obj);
      return NULL;
    }
  }
  return obj;
}
struct obj_data *estate_unpack(const char *text)
{
  FILE *in;
  struct obj_data *obj;
  if (!text)
    return NULL;
  in = fmemopen((void *)text, strlen(text), "r");
  if (!in)
    return NULL;
  obj = unpack_node(in, 0);
  if (obj && fgetc(in) != EOF)
  {
    extract_obj(obj);
    obj = NULL;
  }
  fclose(in);
  return obj;
}
void estate_save_player(struct char_data *ch)
{
  Crash_crashsave(ch);
  save_char(ch, 0);
}

int estate_init(void)
{
  static int ready = FALSE;
  const char *tables[] = {
      "CREATE TABLE IF NOT EXISTS housing_neighborhoods (neighborhood_id BIGINT PRIMARY KEY "
      "AUTO_INCREMENT,name VARCHAR(100) NOT NULL,entrance_room_vnum INT NOT NULL "
      "UNIQUE,house_price BIGINT NOT NULL DEFAULT 100000,room_price BIGINT NOT NULL DEFAULT "
      "50000,title_price BIGINT NOT NULL DEFAULT 500,description_price BIGINT NOT NULL DEFAULT "
      "1000,extra_description_price BIGINT NOT NULL DEFAULT 750,chest_price BIGINT NOT NULL "
      "DEFAULT 10000,chest_customization_price BIGINT NOT NULL DEFAULT 500,active TINYINT NOT NULL "
      "DEFAULT 1) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci",
      "CREATE TABLE IF NOT EXISTS housing_houses (house_id BIGINT PRIMARY KEY "
      "AUTO_INCREMENT,neighborhood_id BIGINT NOT NULL,owner_id BIGINT NOT NULL,owner_name "
      "VARCHAR(64) NOT NULL,name VARCHAR(100) NOT NULL,UNIQUE KEY "
      "owner_neighborhood(neighborhood_id,owner_id),FOREIGN KEY(neighborhood_id) REFERENCES "
      "housing_neighborhoods(neighborhood_id)) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 "
      "COLLATE=utf8mb4_unicode_ci",
      "CREATE TABLE IF NOT EXISTS housing_rooms (room_id BIGINT PRIMARY KEY "
      "AUTO_INCREMENT,house_id BIGINT NOT NULL,room_order INT NOT NULL,title VARCHAR(200) NOT "
      "NULL,description TEXT NOT NULL,is_entrance TINYINT NOT NULL DEFAULT 0,UNIQUE KEY "
      "room_order(house_id,room_order),UNIQUE KEY house_room(house_id,room_id),FOREIGN "
      "KEY(house_id) REFERENCES housing_houses(house_id) ON DELETE CASCADE) ENGINE=InnoDB DEFAULT "
      "CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci",
      "CREATE TABLE IF NOT EXISTS housing_room_exits (from_room_id BIGINT NOT NULL,direction INT "
      "NOT NULL,to_room_id BIGINT NOT NULL,PRIMARY KEY(from_room_id,direction),FOREIGN "
      "KEY(from_room_id) REFERENCES housing_rooms(room_id) ON DELETE CASCADE,FOREIGN "
      "KEY(to_room_id) REFERENCES housing_rooms(room_id) ON DELETE CASCADE) ENGINE=InnoDB DEFAULT "
      "CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci",
      "CREATE TABLE IF NOT EXISTS housing_room_extra_descriptions (extra_description_id BIGINT "
      "PRIMARY KEY AUTO_INCREMENT,room_id BIGINT NOT NULL,keyword VARCHAR(100) NOT "
      "NULL,description TEXT NOT NULL,FOREIGN KEY(room_id) REFERENCES housing_rooms(room_id) ON "
      "DELETE CASCADE) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci",
      "CREATE TABLE IF NOT EXISTS housing_guests (house_id BIGINT NOT NULL,guest_id BIGINT NOT "
      "NULL,PRIMARY KEY(house_id,guest_id),FOREIGN KEY(house_id) REFERENCES "
      "housing_houses(house_id) ON DELETE CASCADE) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 "
      "COLLATE=utf8mb4_unicode_ci",
      "CREATE TABLE IF NOT EXISTS housing_storage_chests (chest_id BIGINT PRIMARY KEY "
      "AUTO_INCREMENT,house_id BIGINT NOT NULL,room_id BIGINT NOT NULL,keywords VARCHAR(255) NOT "
      "NULL DEFAULT 'storage chest',short_description VARCHAR(255) NOT NULL DEFAULT 'a storage "
      "chest',room_description VARCHAR(255) NOT NULL DEFAULT 'A sturdy storage chest rests "
      "here.',FOREIGN KEY(house_id,room_id) REFERENCES housing_rooms(house_id,room_id) ON DELETE "
      "CASCADE) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci",
      "CREATE TABLE IF NOT EXISTS housing_chest_items (item_id BIGINT UNSIGNED PRIMARY KEY "
      "AUTO_INCREMENT,chest_id BIGINT NOT NULL,item_name VARCHAR(255) NOT NULL,keywords TEXT NOT "
      "NULL,serialized_obj LONGTEXT NOT NULL,KEY chest(chest_id),FOREIGN KEY(chest_id) REFERENCES "
      "housing_storage_chests(chest_id) ON DELETE CASCADE) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 "
      "COLLATE=utf8mb4_unicode_ci",
      "CREATE TABLE IF NOT EXISTS auction_listings (listing_id BIGINT UNSIGNED PRIMARY KEY "
      "AUTO_INCREMENT,seller_id BIGINT NOT NULL,seller_name VARCHAR(64) NOT NULL,item_name "
      "VARCHAR(255) NOT NULL,keywords TEXT NOT NULL,price BIGINT NOT NULL,item_type INT NOT "
      "NULL,wear_flags VARCHAR(128) NOT NULL,serialized_obj LONGTEXT NOT NULL,listing_state "
      "ENUM('active','sold','cancelled','expired') NOT NULL DEFAULT 'active',buyer_id BIGINT "
      "NULL,expires_at TIMESTAMP NOT NULL,created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,KEY "
      "active(listing_state,expires_at),KEY seller(seller_id,listing_state)) ENGINE=InnoDB DEFAULT "
      "CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci",
      "CREATE TABLE IF NOT EXISTS auction_credit_settlements (settlement_id BIGINT UNSIGNED "
      "PRIMARY KEY AUTO_INCREMENT,listing_id BIGINT UNSIGNED NOT NULL UNIQUE,beneficiary_id BIGINT "
      "NOT NULL,amount BIGINT NOT NULL,settlement_state ENUM('pending','paid') NOT NULL DEFAULT "
      "'pending',KEY beneficiary(beneficiary_id,settlement_state),FOREIGN KEY(listing_id) "
      "REFERENCES auction_listings(listing_id)) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 "
      "COLLATE=utf8mb4_unicode_ci",
      "CREATE TABLE IF NOT EXISTS estate_deliveries (delivery_id BIGINT UNSIGNED PRIMARY KEY "
      "AUTO_INCREMENT,player_id BIGINT NOT NULL,debit BIGINT NOT NULL DEFAULT 0,serialized_obj "
      "LONGTEXT NOT NULL,delivered TINYINT NOT NULL DEFAULT 0,KEY pending(player_id,delivered)) "
      "ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci",
      "CREATE TABLE IF NOT EXISTS estate_outgoing (escrow_id BIGINT UNSIGNED PRIMARY KEY "
      "AUTO_INCREMENT,player_id BIGINT NOT NULL,escrow_state ENUM('prepared','committed','saved') "
      "NOT NULL DEFAULT 'prepared',KEY pending(player_id,escrow_state)) ENGINE=InnoDB DEFAULT "
      "CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci",
      NULL};
  int i;
  if (ready)
    return conn && mysql_available;
  for (i = 0; tables[i]; i++)
  {
    char table[100];
    MYSQL_RES *exists;
    int present;
    if (sscanf(tables[i], "CREATE TABLE IF NOT EXISTS %99s", table) != 1)
      return FALSE;
    exists = estate_select(
        "SELECT 1 FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='%s'",
        table);
    if (!exists)
      return FALSE;
    present = mysql_num_rows(exists) > 0;
    mysql_free_result(exists);
    /* A preinstalled schema needs only ordinary read/write privileges. */
    if (!present && !estate_query("%s", tables[i]))
      return FALSE;
  }
  ready = TRUE;
  return TRUE;
}

/* Confirm the currency receipt in the saved character file before acknowledging SQL. */
int estate_receipt_saved(struct char_data *ch)
{
  char filename[1024], line[256];
  FILE *fp;
  unsigned long long credit = 0, debit = 0;
  int gold = 0;
  if (!get_filename(filename, sizeof(filename), PLR_FILE, GET_NAME(ch)) ||
      !(fp = fopen(filename, "r")))
    return FALSE;
  while (fgets(line, sizeof(line), fp))
  {
    if (!strncmp(line, "ECrd:", 5))
      credit = strtoull(line + 5, NULL, 10);
    else if (!strncmp(line, "EDbt:", 5))
      debit = strtoull(line + 5, NULL, 10);
    else if (!strncmp(line, "Gold:", 5))
      gold = atoi(line + 5);
  }
  fclose(fp);
  return gold == GET_GOLD(ch) && credit == ch->player_specials->estate_credit_receipt &&
         debit == ch->player_specials->estate_debit_receipt;
}
int estate_queue_delivery(struct char_data *ch, struct obj_data *obj, long debit)
{
  char *packed, *q;
  unsigned long long id;
  if (!estate_query(
          "INSERT INTO estate_deliveries(player_id,debit,serialized_obj) VALUES(%ld,%ld,'')",
          GET_IDNUM(ch), debit))
    return FALSE;
  id = mysql_insert_id(conn);
  obj->estate_delivery_id = id;
  packed = estate_pack(obj);
  if (!packed)
    return FALSE;
  q = estate_quote(packed);
  free(packed);
  if (!q)
    return FALSE;
  if (!estate_query("UPDATE estate_deliveries SET serialized_obj='%s' WHERE delivery_id=%llu", q,
                    id))
  {
    free(q);
    return FALSE;
  }
  free(q);
  return TRUE;
}
static struct obj_data *find_outgoing(struct obj_data *, unsigned long long, struct obj_data **);
static void recover_outgoing(struct char_data *ch);
static void remove_transfer_object(struct char_data *ch, struct obj_data *obj,
                                   struct obj_data *sheath)
{
  if (obj->carried_by)
    obj_from_char(obj);
  else if (obj->in_obj)
    obj_from_obj(obj);
  else if (sheath)
  {
    if (sheath->sheath_primary == obj)
      sheath->sheath_primary = NULL;
    else if (sheath->sheath_secondary == obj)
      sheath->sheath_secondary = NULL;
  }
  else if (obj->worn_by)
    unequip_char(ch, obj->worn_on);
  extract_obj(obj);
}
void estate_recover(struct char_data *ch)
{
  MYSQL_RES *res, *saved;
  MYSQL_ROW row;
  unsigned long long id;
  long debit;
  int i, exists, fresh;
  struct obj_data *obj, *sheath;
  char *name;
  if (IS_NPC(ch) || !estate_init())
    return;
  recover_outgoing(ch);
  res = estate_select("SELECT delivery_id,debit,serialized_obj FROM estate_deliveries WHERE "
                      "player_id=%ld AND delivered=0 ORDER BY delivery_id",
                      GET_IDNUM(ch));
  if (!res)
    return;
  while ((row = mysql_fetch_row(res)))
  {
    id = strtoull(row[0], NULL, 10);
    debit = atol(row[1]);
    sheath = NULL;
    obj = find_outgoing(ch->carrying, id, &sheath);
    for (i = 0; i < NUM_WEARS && !obj; i++)
      if (GET_EQ(ch, i))
        obj = find_outgoing(GET_EQ(ch, i), id, &sheath);
    fresh = obj == NULL;
    if (fresh)
      obj = estate_unpack(row[2]);
    if (!obj)
    {
      log("SYSERR: Unable to recover estate delivery %llu", id);
      break;
    }
    if (id > ch->player_specials->estate_debit_receipt)
    {
      if (debit > GET_GOLD(ch))
      {
        remove_transfer_object(ch, obj, sheath);
        send_to_char(ch, "An auction delivery is pending; you need %ld gold to finish it.\r\n",
                     debit);
        break;
      }
      GET_GOLD(ch) -= debit;
      ch->player_specials->estate_debit_receipt = id;
    }
    if (fresh)
      obj_to_char(obj, ch);
    estate_save_player(ch);
    if (!estate_receipt_saved(ch))
      goto pending;
    name = estate_quote(GET_NAME(ch));
    if (!name)
      goto pending;
    saved = estate_select("SELECT 1 FROM player_save_objs WHERE name='%s' AND serialized_obj LIKE "
                          "'%%EId : %llu\\n%%' LIMIT 1",
                          name, id);
    free(name);
    exists = saved && mysql_num_rows(saved) > 0;
    if (saved)
      mysql_free_result(saved);
    if (!exists ||
        !estate_query(
            "UPDATE estate_deliveries SET delivered=1 WHERE delivery_id=%llu AND player_id=%ld", id,
            GET_IDNUM(ch)))
      goto pending;
    if (fresh)
      send_to_char(ch, "You receive %s from persistent storage.\r\n", obj->short_description);
    continue;
  pending:
    /* Keep an unacknowledged item in the journal, out of reach of give/drop/sell.
     * Ordinary inventory saves may continue; recovery will recreate it once. */
    remove_transfer_object(ch, obj, sheath);
    send_to_char(ch, "Your delivery remains in storage until your player data can be saved.\r\n");
    break;
  }
  mysql_free_result(res);
}

/* Persist a stable identity in the player's inventory before committing an
 * outgoing transfer. A crash can then remove that exact object, not a namesake. */
unsigned long long estate_prepare_escrow(struct char_data *ch, struct obj_data *obj)
{
  unsigned long long id;
  MYSQL_RES *res;
  char *name;
  int saved;
  if (!estate_player_ready(ch))
    return 0;
  if (!estate_query("INSERT INTO estate_outgoing(player_id) VALUES(%ld)", GET_IDNUM(ch)))
    return 0;
  id = mysql_insert_id(conn);
  obj->estate_delivery_id = id | (1ULL << 63);
  estate_save_player(ch);
  name = estate_quote(GET_NAME(ch));
  if (!name)
    return 0;
  res = estate_select("SELECT 1 FROM player_save_objs WHERE name='%s' AND serialized_obj LIKE "
                      "'%%EId : %llu\\n%%' LIMIT 1",
                      name, obj->estate_delivery_id);
  free(name);
  saved = res && mysql_num_rows(res) > 0;
  if (res)
    mysql_free_result(res);
  return saved ? id : 0;
}
int estate_commit_escrow(unsigned long long id)
{
  return estate_query("UPDATE estate_outgoing SET escrow_state='committed' WHERE escrow_id=%llu "
                      "AND escrow_state='prepared'",
                      id) &&
         mysql_affected_rows(conn) == 1;
}
static struct obj_data *find_outgoing(struct obj_data *obj, unsigned long long id,
                                      struct obj_data **sheath)
{
  struct obj_data *it, *found;
  for (it = obj; it; it = it->next_content)
  {
    if (it->estate_delivery_id == id)
      return it;
    if ((found = find_outgoing(it->contains, id, sheath)))
      return found;
    if (it->sheath_primary && (found = find_outgoing(it->sheath_primary, id, sheath)))
    {
      *sheath = it;
      return found;
    }
    if (it->sheath_secondary && (found = find_outgoing(it->sheath_secondary, id, sheath)))
    {
      *sheath = it;
      return found;
    }
  }
  return NULL;
}
static void recover_outgoing(struct char_data *ch)
{
  MYSQL_RES *res, *saved;
  MYSQL_ROW row;
  unsigned long long id, identity;
  struct obj_data *obj, *sheath;
  int i, exists;
  char *name;
  res = estate_select("SELECT escrow_id FROM estate_outgoing WHERE player_id=%ld AND "
                      "escrow_state='committed' ORDER BY escrow_id",
                      GET_IDNUM(ch));
  if (!res)
    return;
  while ((row = mysql_fetch_row(res)))
  {
    id = strtoull(row[0], NULL, 10);
    identity = id | (1ULL << 63);
    sheath = NULL;
    obj = find_outgoing(ch->carrying, identity, &sheath);
    for (i = 0; i < NUM_WEARS && !obj; i++)
      if (GET_EQ(ch, i))
        obj = find_outgoing(GET_EQ(ch, i), identity, &sheath);
    if (obj)
    {
      if (obj->carried_by)
        obj_from_char(obj);
      else if (obj->in_obj)
        obj_from_obj(obj);
      else if (sheath)
      {
        if (sheath->sheath_primary == obj)
          sheath->sheath_primary = NULL;
        else if (sheath->sheath_secondary == obj)
          sheath->sheath_secondary = NULL;
      }
      else if (obj->worn_by)
        unequip_char(ch, obj->worn_on);
      extract_obj(obj);
    }
    estate_save_player(ch);
    name = estate_quote(GET_NAME(ch));
    if (!name)
      break;
    saved = estate_select("SELECT 1 FROM player_save_objs WHERE name='%s' AND serialized_obj LIKE "
                          "'%%EId : %llu\\n%%' LIMIT 1",
                          name, identity);
    free(name);
    if (!saved)
      break;
    exists = mysql_num_rows(saved) > 0;
    mysql_free_result(saved);
    if (exists)
      break;
    if (!estate_query("UPDATE estate_outgoing SET escrow_state='saved' WHERE escrow_id=%llu", id))
      break;
  }
  mysql_free_result(res);
}

int estate_player_ready(struct char_data *ch)
{
  MYSQL_RES *res;
  MYSQL_ROW row;
  int ready = FALSE;
  estate_recover(ch);
  res = estate_select(
      "SELECT (SELECT COUNT(*) FROM estate_deliveries WHERE player_id=%ld AND delivered=0)+(SELECT "
      "COUNT(*) FROM estate_outgoing WHERE player_id=%ld AND escrow_state='committed')",
      GET_IDNUM(ch), GET_IDNUM(ch));
  if (res)
  {
    row = mysql_fetch_row(res);
    ready = row && !atol(row[0]);
    mysql_free_result(res);
  }
  if (!ready)
    send_to_char(
        ch,
        "A previous transfer is still pending. Try again after your player data can be saved.\r\n");
  return ready;
}
