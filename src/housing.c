/* Neighborhood housing, adapted from d20StarWars housing service. */
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "housing.h"
#include "act.h"
#include "comm.h"
#include "constants.h"
#include "db.h"
#include "estate.h"
#include "genwld.h"
#include "handler.h"
#include "interpreter.h"

struct home_room
{
  long id, house;
  room_vnum vnum;
  struct home_room *next;
};
struct home_chest
{
  long id, house, room;
  struct obj_data *obj;
  struct home_chest *next;
};
static struct home_room *home_rooms;
static struct home_chest *home_chests;
static struct home_room *room_record(room_vnum vnum)
{
  struct home_room *r;
  for (r = home_rooms; r; r = r->next)
    if (r->vnum == vnum)
      return r;
  return NULL;
}
static struct home_room *persistent_room(long id)
{
  struct home_room *r;
  for (r = home_rooms; r; r = r->next)
    if (r->id == id)
      return r;
  return NULL;
}
static struct home_chest *chest_record(struct obj_data *obj)
{
  struct home_chest *c;
  for (c = home_chests; c; c = c->next)
    if (c->obj == obj)
      return c;
  return NULL;
}
int housing_is_room(room_vnum vnum) { return room_record(vnum) != NULL; }
static long owner(long house)
{
  MYSQL_RES *res = estate_select("SELECT owner_id FROM housing_houses WHERE house_id=%ld", house);
  MYSQL_ROW row;
  long id = -1;
  if (res)
  {
    if ((row = mysql_fetch_row(res)))
      id = atol(row[0]);
    mysql_free_result(res);
  }
  return id;
}
static int owns(struct char_data *ch, long house)
{
  return !IS_NPC(ch) && (GET_LEVEL(ch) >= LVL_GRSTAFF || owner(house) == GET_IDNUM(ch));
}
static int access_house(struct char_data *ch, long house)
{
  MYSQL_RES *res;
  int ok;
  if (IS_NPC(ch))
    return ch->master && access_house(ch->master, house);
  if (owns(ch, house))
    return TRUE;
  res = estate_select("SELECT guest_id FROM housing_guests WHERE house_id=%ld AND guest_id=%ld",
                      house, GET_IDNUM(ch));
  ok = res && mysql_num_rows(res) > 0;
  if (res)
    mysql_free_result(res);
  return ok;
}
int housing_can_enter(struct char_data *ch, room_vnum vnum)
{
  struct home_room *r = room_record(vnum);
  return r ? access_house(ch, r->house) : TRUE;
}
static room_vnum exterior(long house)
{
  MYSQL_RES *res =
      estate_select("SELECT n.entrance_room_vnum FROM housing_houses h JOIN housing_neighborhoods "
                    "n USING(neighborhood_id) WHERE h.house_id=%ld",
                    house);
  MYSQL_ROW row;
  room_vnum v = NOWHERE;
  if (res)
  {
    if ((row = mysql_fetch_row(res)))
      v = atoi(row[0]);
    mysql_free_result(res);
  }
  return v;
}
void housing_capture_location(struct char_data *ch)
{
  struct home_room *r;
  if (IS_NPC(ch) || IN_ROOM(ch) == NOWHERE)
    return;
  r = room_record(GET_ROOM_VNUM(IN_ROOM(ch)));
  ch->player_specials->housing_house_id = r ? r->house : 0;
  ch->player_specials->housing_room_id = r ? r->id : 0;
  if (r)
  {
    GET_LOADROOM(ch) = exterior(r->house);
    GET_LAST_ROOM(ch) = GET_LOADROOM(ch);
  }
}
static long price(long house, const char *column)
{
  MYSQL_RES *res = estate_select("SELECT n.%s FROM housing_houses h JOIN housing_neighborhoods n "
                                 "USING(neighborhood_id) WHERE h.house_id=%ld",
                                 column, house);
  MYSQL_ROW row;
  long value = -1;
  if (res)
  {
    if ((row = mysql_fetch_row(res)))
      value = atol(row[0]);
    mysql_free_result(res);
  }
  return value;
}
static int afford(struct char_data *ch, long amount)
{
  if (amount < 0)
  {
    send_to_char(ch, "The housing service is unavailable.\r\n");
    return FALSE;
  }
  if (!estate_player_ready(ch))
    return FALSE;
  if (GET_GOLD(ch) < amount)
  {
    send_to_char(ch, "You need %ld gold.\r\n", amount);
    return FALSE;
  }
  return TRUE;
}
static void charge(struct char_data *ch, long amount)
{
  GET_GOLD(ch) -= amount;
  estate_save_player(ch);
}

static char *description_text(const char *text)
{
  size_t n = strlen(text);
  char *result = malloc(n + 3);
  if (!result)
    return NULL;
  memcpy(result, text, n);
  if (!n || text[n - 1] != '\n')
  {
    result[n++] = '\r';
    result[n++] = '\n';
  }
  result[n] = 0;
  return result;
}
static room_rnum make_room(long house, long id, const char *title, const char *description,
                           int entrance)
{
  struct room_data data;
  struct home_room *record;
  room_rnum nr, er;
  room_vnum vnum;
  if ((record = persistent_room(id)))
    return real_room(record->vnum);
  er = real_room(exterior(house));
  if (er == NOWHERE)
    return NOWHERE;
  /* Append above every existing VNUM. This preserves Krynn's sorted room array. */
  if (world[top_of_world].number >= 98999999)
    return NOWHERE;
  vnum = MAX(2000000, world[top_of_world].number + 1);
  if (vnum >= 99000000)
    return NOWHERE;
  memset(&data, 0, sizeof(data));
  data.number = vnum;
  data.zone = world[er].zone;
  data.name = (char *)title;
  data.description = description_text(description);
  if (!data.description)
    return NOWHERE;
  data.sector_type = SECT_INSIDE;
  SET_BIT_AR(data.room_flags, ROOM_HOUSE);
  SET_BIT_AR(data.room_flags, ROOM_INDOORS);
  nr = add_runtime_room(&data);
  free(data.description);
  if (nr == NOWHERE)
    return NOWHERE;
  CREATE(record, struct home_room, 1);
  record->id = id;
  record->house = house;
  record->vnum = vnum;
  record->next = home_rooms;
  home_rooms = record;
#ifdef CAMPAIGN_FR
  if (entrance)
  {
    CREATE(world[nr].dir_option[OUT], struct room_direction_data, 1);
    world[nr].dir_option[OUT]->key = NOTHING;
    world[nr].dir_option[OUT]->to_room = real_room(exterior(house));
  }
#else
  (void)entrance; /* Krynn has ten directions; house leave supplies the exterior exit. */
#endif
  return nr;
}
static void connect_rooms(long from, int dir, long to)
{
  struct home_room *a = persistent_room(from), *b = persistent_room(to);
  room_rnum r;
  if (!a || !b || dir < 0 || dir >= NUM_OF_DIRS)
    return;
  r = real_room(a->vnum);
  if (!world[r].dir_option[dir])
    CREATE(world[r].dir_option[dir], struct room_direction_data, 1);
  world[r].dir_option[dir]->key = NOTHING;
  world[r].dir_option[dir]->to_room = real_room(b->vnum);
}
static void create_chest(MYSQL_ROW row)
{
  struct home_chest *c;
  struct home_room *r = persistent_room(atol(row[2]));
  char name[512];
  struct obj_data *obj;
  if (!r)
    return;
  for (c = home_chests; c; c = c->next)
    if (c->id == atol(row[0]))
      break;
  if (c && c->obj)
    return;
  obj = create_obj();
  obj->item_number = NOTHING;
  snprintf(name, sizeof(name), "%s %s", row[0], row[3]);
  obj->name = strdup(name);
  obj->short_description = strdup(row[4]);
  obj->description = strdup(row[5]);
  GET_OBJ_BOUND_ID(obj) = NOBODY;
  GET_OBJ_TYPE(obj) = ITEM_CONTAINER;
  GET_OBJ_VAL(obj, 0) = INT_MAX;
  GET_OBJ_WEIGHT(obj) = 1000;
  SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_NORENT);
  if (!c)
  {
    CREATE(c, struct home_chest, 1);
    c->next = home_chests;
    home_chests = c;
  }
  c->id = atol(row[0]);
  c->house = atol(row[1]);
  c->room = r->id;
  c->obj = obj;
  obj_to_room(obj, real_room(r->vnum));
}
static room_rnum materialize(long house)
{
  MYSQL_RES *res;
  MYSQL_ROW row;
  room_rnum entrance = NOWHERE;
  struct home_room *r;
  res = estate_select("SELECT room_id,title,description,is_entrance FROM housing_rooms WHERE "
                      "house_id=%ld ORDER BY room_order",
                      house);
  if (!res)
    return NOWHERE;
  while ((row = mysql_fetch_row(res)))
  {
    room_rnum nr = make_room(house, atol(row[0]), row[1], row[2], atoi(row[3]));
    if (nr == NOWHERE)
    {
      mysql_free_result(res);
      return NOWHERE;
    }
    if (atoi(row[3]))
      entrance = nr;
  }
  mysql_free_result(res);
  res = estate_select("SELECT e.from_room_id,e.direction,e.to_room_id FROM housing_room_exits e "
                      "JOIN housing_rooms r ON r.room_id=e.from_room_id WHERE r.house_id=%ld",
                      house);
  if (!res)
    return NOWHERE;
  while ((row = mysql_fetch_row(res)))
    connect_rooms(atol(row[0]), atoi(row[1]), atol(row[2]));
  mysql_free_result(res);
  res =
      estate_select("SELECT e.room_id,e.keyword,e.description FROM housing_room_extra_descriptions "
                    "e JOIN housing_rooms r USING(room_id) WHERE r.house_id=%ld",
                    house);
  if (!res)
    return NOWHERE;
  /* Reload extras once: newly materialized rooms have no extras yet. */
  for (r = home_rooms; r; r = r->next)
    if (r->house == house)
    {
      struct extra_descr_data *e = world[real_room(r->vnum)].ex_description, *next;
      while (e)
      {
        next = e->next;
        free(e->keyword);
        free(e->description);
        free(e);
        e = next;
      }
      world[real_room(r->vnum)].ex_description = NULL;
    }
  while ((row = mysql_fetch_row(res)))
    if ((r = persistent_room(atol(row[0]))))
    {
      struct extra_descr_data *e;
      room_rnum nr = real_room(r->vnum);
      CREATE(e, struct extra_descr_data, 1);
      e->keyword = strdup(row[1]);
      e->description = description_text(row[2]);
      e->next = world[nr].ex_description;
      world[nr].ex_description = e;
    }
  mysql_free_result(res);
  res =
      estate_select("SELECT chest_id,house_id,room_id,keywords,short_description,room_description "
                    "FROM housing_storage_chests WHERE house_id=%ld",
                    house);
  if (!res)
    return NOWHERE;
  while ((row = mysql_fetch_row(res)))
    create_chest(row);
  mysql_free_result(res);
  return entrance;
}
room_rnum housing_login_room(struct char_data *ch)
{
  long id = ch->player_specials->housing_house_id;
  struct home_room *r;
  room_rnum entry;
  if (!id || !estate_init() || !access_house(ch, id))
    return NOWHERE;
  entry = materialize(id);
  r = persistent_room(ch->player_specials->housing_room_id);
  return r && r->house == id ? real_room(r->vnum) : entry;
}
static int storage_permission(struct char_data *ch, struct home_chest *c)
{
  if (!c || !owns(ch, c->house) || IN_ROOM(ch) != IN_ROOM(c->obj))
  {
    send_to_char(ch, "Only the house owner can use chest storage, while in its room.\r\n");
    return FALSE;
  }
  return TRUE;
}
static int storable(struct obj_data *obj)
{
  struct obj_data *it;
  if (OBJ_FLAGGED(obj, ITEM_NORENT) || OBJ_FLAGGED(obj, ITEM_NODROP) ||
      GET_OBJ_TYPE(obj) == ITEM_MONEY)
    return FALSE;
  for (it = obj->contains; it; it = it->next_content)
    if (!storable(it))
      return FALSE;
  if (obj->sheath_primary && !storable(obj->sheath_primary))
    return FALSE;
  if (obj->sheath_secondary && !storable(obj->sheath_secondary))
    return FALSE;
  return TRUE;
}
int housing_put(struct char_data *ch, struct obj_data *obj, struct obj_data *container)
{
  struct home_chest *c = chest_record(container);
  char *packed, *q, *name, *keywords;
  int ok;
  unsigned long long escrow;
  if (!c)
    return FALSE;
  if (!storage_permission(ch, c))
    return TRUE;
  if (obj->carried_by != ch || !storable(obj))
  {
    send_to_char(ch, "That item cannot be stored.\r\n");
    return TRUE;
  }
  escrow = estate_prepare_escrow(ch, obj);
  if (!escrow)
  {
    send_to_char(ch, "Your inventory could not be saved; deposit cancelled.\r\n");
    return TRUE;
  }
  packed = estate_pack(obj);
  if (!packed)
  {
    send_to_char(ch, "Unable to serialize that item.\r\n");
    return TRUE;
  }
  q = estate_quote(packed);
  name = estate_quote(obj->short_description);
  keywords = estate_quote(obj->name);
  free(packed);
  ok = q && name && keywords && estate_query("START TRANSACTION") &&
       estate_query("INSERT INTO housing_chest_items(chest_id,item_name,keywords,serialized_obj) "
                    "VALUES(%ld,'%s','%s','%s')",
                    c->id, name, keywords, q) &&
       estate_commit_escrow(escrow) && estate_query("COMMIT");
  if (!ok)
    estate_query("ROLLBACK");
  free(q);
  free(name);
  free(keywords);
  if (ok)
  {
    act("You store $p in $P.", FALSE, ch, obj, container, TO_CHAR);
    obj_from_char(obj);
    extract_obj(obj);
    estate_recover(ch);
  }
  else
    send_to_char(ch, "Storage failed; you still have your item.\r\n");
  return TRUE;
}
int housing_contents(struct char_data *ch, struct obj_data *container)
{
  struct home_chest *c = chest_record(container);
  MYSQL_RES *res;
  MYSQL_ROW row;
  int count = 0;
  if (!c)
    return FALSE;
  if (!storage_permission(ch, c))
    return TRUE;
  res = estate_select(
      "SELECT item_id,item_name FROM housing_chest_items WHERE chest_id=%ld ORDER BY item_id",
      c->id);
  if (!res)
  {
    send_to_char(ch, "Storage is unavailable.\r\n");
    return TRUE;
  }
  send_to_char(ch, "%s contains:\r\n", container->short_description);
  while ((row = mysql_fetch_row(res)))
  {
    send_to_char(ch, "  [%s] %s\r\n", row[0], row[1]);
    count++;
  }
  if (!count)
    send_to_char(ch, "  Nothing.\r\n");
  mysql_free_result(res);
  return TRUE;
}
int housing_get(struct char_data *ch, struct obj_data *container, const char *argument, int amount)
{
  struct home_chest *c = chest_record(container);
  MYSQL_RES *res;
  MYSQL_ROW row;
  int mode, number = 1, found = 0;
  char arg[MAX_INPUT_LENGTH], *match;
  struct obj_data *obj;
  if (!c)
    return FALSE;
  if (!storage_permission(ch, c))
    return TRUE;
  strlcpy(arg, argument, sizeof(arg));
  mode = find_all_dots(arg);
  match = arg;
  if (mode == FIND_INDIV)
    number = get_number(&match);
  if (number < 1)
    number = 1;
  if (mode != FIND_INDIV && amount == 1)
    amount = INT_MAX;
  res = estate_select("SELECT item_id,keywords,serialized_obj FROM housing_chest_items WHERE "
                      "chest_id=%ld ORDER BY item_id",
                      c->id);
  if (!res)
  {
    send_to_char(ch, "Storage is unavailable.\r\n");
    return TRUE;
  }
  while (amount > 0 && (row = mysql_fetch_row(res)))
  {
    if (mode != FIND_ALL && strcmp(match, row[0]) && !isname(match, row[1]))
      continue;
    if (mode == FIND_INDIV && --number > 0)
      continue;
    obj = estate_unpack(row[2]);
    if (!obj)
    {
      send_to_char(ch, "That stored item could not be restored; it remains in storage.\r\n");
      break;
    }
    if (!CAN_CARRY_OBJ(ch, obj))
    {
      extract_obj(obj);
      send_to_char(ch, "You cannot carry that item.\r\n");
      break;
    }
    if (!estate_query("START TRANSACTION"))
    {
      extract_obj(obj);
      break;
    }
    if (!estate_query("DELETE FROM housing_chest_items WHERE item_id=%s AND chest_id=%ld", row[0],
                      c->id) ||
        mysql_affected_rows(conn) != 1 || !estate_queue_delivery(ch, obj, 0) ||
        !estate_query("COMMIT"))
    {
      estate_query("ROLLBACK");
      extract_obj(obj);
      break;
    }
    extract_obj(obj);
    estate_recover(ch);
    found++;
    amount--;
    if (mode == FIND_INDIV && amount == 0)
      break;
    number = 1;
  }
  mysql_free_result(res);
  if (found)
    estate_save_player(ch);
  else
    send_to_char(ch, "No item retrieved.\r\n");
  return TRUE;
}
static struct home_chest *find_chest(struct char_data *ch, long id)
{
  struct home_chest *c;
  for (c = home_chests; c; c = c->next)
    if (c->obj && c->id == id && IN_ROOM(c->obj) == IN_ROOM(ch))
      return c;
  return NULL;
}
static void chest_command(struct char_data *ch, struct home_room *r, const char *argument)
{
  char verb[MAX_INPUT_LENGTH], idtext[MAX_INPUT_LENGTH];
  const char *text;
  struct home_chest *c;
  long amount, id;
  MYSQL_RES *res;
  MYSQL_ROW row;
  text = one_argument(argument, verb, sizeof(verb));
  text = one_argument(text, idtext, sizeof(idtext));
  id = atol(idtext);
  if (!*verb || !str_cmp(verb, "list"))
  {
    res = estate_select("SELECT chest_id,room_id,short_description FROM housing_storage_chests "
                        "WHERE house_id=%ld ORDER BY chest_id",
                        r->house);
    if (res)
    {
      while ((row = mysql_fetch_row(res)))
        send_to_char(ch, "Chest %s, room %s: %s\r\n", row[0], row[1], row[2]);
      mysql_free_result(res);
    }
    return;
  }
  if (!owns(ch, r->house))
  {
    send_to_char(ch, "Only the owner can manage chests.\r\n");
    return;
  }
  if (!str_cmp(verb, "buy"))
  {
    amount = price(r->house, "chest_price");
    if (!afford(ch, amount))
      return;
    if (estate_query("INSERT INTO housing_storage_chests(house_id,room_id) VALUES(%ld,%ld)",
                     r->house, r->id))
    {
      charge(ch, amount);
      materialize(r->house);
      send_to_char(ch, "Storage chest created.\r\n");
    }
    return;
  }
  if (!str_cmp(verb, "move"))
  {
    for (c = home_chests; c; c = c->next)
      if (c->obj && c->id == id && c->house == r->house)
        break;
    if (c &&
        estate_query(
            "UPDATE housing_storage_chests SET room_id=%ld WHERE chest_id=%ld AND house_id=%ld",
            r->id, id, r->house))
    {
      obj_from_room(c->obj);
      obj_to_room(c->obj, IN_ROOM(ch));
      c->room = r->id;
      send_to_char(ch, "Chest moved.\r\n");
    }
    else
      send_to_char(ch, "No such chest in your house.\r\n");
    return;
  }
  materialize(r->house);
  c = find_chest(ch, id);
  if (!c)
  {
    send_to_char(ch, "No such chest here.\r\n");
    return;
  }
  if (!str_cmp(verb, "contents"))
  {
    housing_contents(ch, c->obj);
    return;
  }
  if (!str_cmp(verb, "withdraw"))
  {
    housing_get(ch, c->obj, text, 1);
    return;
  }
  if (!str_cmp(verb, "deposit"))
  {
    char item[MAX_INPUT_LENGTH];
    struct obj_data *obj;
    strlcpy(item, text, sizeof(item));
    obj = get_obj_in_list_vis(ch, item, NULL, ch->carrying);
    if (obj)
      housing_put(ch, obj, c->obj);
    else
      send_to_char(ch, "You aren't carrying that.\r\n");
    return;
  }
  if (!str_cmp(verb, "keywords") || !str_cmp(verb, "short") || !str_cmp(verb, "room"))
  {
    const char *column = !str_cmp(verb, "keywords") ? "keywords"
                         : !str_cmp(verb, "short")  ? "short_description"
                                                    : "room_description";
    char *q;
    skip_spaces_c(&text);
    if (!*text || strlen(text) > 255)
    {
      send_to_char(ch, "Text must be 1-255 characters.\r\n");
      return;
    }
    amount = price(r->house, "chest_customization_price");
    if (!afford(ch, amount))
      return;
    q = estate_quote(text);
    if (!q)
      return;
    if (estate_query(
            "UPDATE housing_storage_chests SET %s='%s' WHERE chest_id=%ld AND house_id=%ld", column,
            q, id, r->house))
    {
      char **field;
      char buf[512];
      field = !str_cmp(verb, "keywords") ? &c->obj->name
              : !str_cmp(verb, "short")  ? &c->obj->short_description
                                         : &c->obj->description;
      free(*field);
      if (!str_cmp(verb, "keywords"))
      {
        snprintf(buf, sizeof(buf), "%ld %s", id, text);
        *field = strdup(buf);
      }
      else
        *field = strdup(text);
      charge(ch, amount);
      send_to_char(ch, "Chest updated.\r\n");
    }
    free(q);
    return;
  }
  send_to_char(
      ch,
      "Chest commands: list, buy, contents, deposit, withdraw, keywords, short, room, move.\r\n");
}

int housing_command(struct char_data *ch, const char *argument)
{
  char verb[MAX_INPUT_LENGTH], arg[MAX_INPUT_LENGTH];
  const char *text;
  struct home_room *r;
  MYSQL_RES *res;
  MYSQL_ROW row;
  long neighborhood = 0, house = 0, amount, id;
  int dir;
  if (IS_NPC(ch) || IN_ROOM(ch) == NOWHERE)
    return TRUE;
  r = room_record(GET_ROOM_VNUM(IN_ROOM(ch)));
  /* Preserve the old house guest command for legacy fixed-room houses. */
  if (!r && ROOM_FLAGGED(IN_ROOM(ch), ROOM_HOUSE))
    return FALSE;
  if (!estate_init())
  {
    send_to_char(ch, "The housing database is unavailable.\r\n");
    return TRUE;
  }
  text = one_argument(argument, verb, sizeof(verb));
  skip_spaces_c(&text);
  if (!*verb)
  {
    send_to_char(
        ch, "House: list, buy <name>, enter <id>, leave, info, guest <player>, build <direction>, "
            "title <text>, description <text>, extra <keyword> <text>, chest <command>.\r\n");
    return TRUE;
  }
  res = estate_select("SELECT neighborhood_id,house_price FROM housing_neighborhoods WHERE "
                      "entrance_room_vnum=%d AND active=1",
                      GET_ROOM_VNUM(IN_ROOM(ch)));
  amount = -1;
  if (res)
  {
    if ((row = mysql_fetch_row(res)))
    {
      neighborhood = atol(row[0]);
      amount = atol(row[1]);
    }
    mysql_free_result(res);
  }
  if (!str_cmp(verb, "list"))
  {
    if (!neighborhood)
    {
      send_to_char(ch, "You must be at a neighborhood entrance.\r\n");
      return TRUE;
    }
    res = estate_select("SELECT h.house_id,h.name,h.owner_name,COUNT(r.room_id) FROM "
                        "housing_houses h LEFT JOIN housing_rooms r USING(house_id) WHERE "
                        "h.neighborhood_id=%ld GROUP BY h.house_id ORDER BY h.house_id",
                        neighborhood);
    if (res)
    {
      while ((row = mysql_fetch_row(res)))
        send_to_char(ch, "[%s] %s (%s; %s rooms)\r\n", row[0], row[1], row[2], row[3]);
      mysql_free_result(res);
    }
    return TRUE;
  }
  if (!str_cmp(verb, "buy"))
  {
    char default_name[128], *q, *name;
    if (!neighborhood)
    {
      send_to_char(ch, "You must be at a neighborhood entrance.\r\n");
      return TRUE;
    }
    if (!*text)
    {
      snprintf(default_name, sizeof(default_name), "%s's Home", GET_NAME(ch));
      text = default_name;
    }
    if (strlen(text) > 100 || !afford(ch, amount))
      return TRUE;
    q = estate_quote(text);
    name = estate_quote(GET_NAME(ch));
    if (!q || !name)
    {
      free(q);
      free(name);
      return TRUE;
    }
    if (estate_query("START TRANSACTION") &&
        estate_query("INSERT INTO housing_houses(neighborhood_id,owner_id,owner_name,name) "
                     "VALUES(%ld,%ld,'%s','%s')",
                     neighborhood, GET_IDNUM(ch), name, q))
    {
      house = (long)mysql_insert_id(conn);
      if (estate_query(
              "INSERT INTO housing_rooms(house_id,room_order,title,description,is_entrance) "
              "VALUES(%ld,0,'An Unfinished Room','This room awaits your imagination.\r\n',1)",
              house) &&
          estate_query("COMMIT"))
      {
        charge(ch, amount);
        send_to_char(ch, "House %ld purchased. Use house enter %ld.\r\n", house, house);
      }
      else
        estate_query("ROLLBACK");
    }
    else
    {
      estate_query("ROLLBACK");
      send_to_char(ch, "Purchase failed. You may already own a house in this neighborhood.\r\n");
    }
    free(q);
    free(name);
    return TRUE;
  }
  if (!str_cmp(verb, "enter"))
  {
    room_rnum nr;
    house = atol(text);
    if (!neighborhood || exterior(house) != GET_ROOM_VNUM(IN_ROOM(ch)) || !access_house(ch, house))
    {
      send_to_char(ch, "You cannot enter that house from here.\r\n");
      return TRUE;
    }
    nr = materialize(house);
    if (nr != NOWHERE)
    {
      char_from_room(ch);
      char_to_room(ch, nr);
      housing_capture_location(ch);
      estate_save_player(ch);
      look_at_room(ch, 0);
    }
    else
      send_to_char(ch, "House could not be loaded.\r\n");
    return TRUE;
  }
  if (!r)
  {
    send_to_char(ch, "You must be inside a neighborhood house.\r\n");
    return TRUE;
  }
  if (!str_cmp(verb, "leave"))
  {
    room_rnum nr = real_room(exterior(r->house));
    if (nr != NOWHERE)
    {
      char_from_room(ch);
      char_to_room(ch, nr);
      housing_capture_location(ch);
      estate_save_player(ch);
      look_at_room(ch, 0);
    }
    return TRUE;
  }
  if (!str_cmp(verb, "info"))
  {
    res = estate_select("SELECT h.house_id,h.name,h.owner_name,n.name FROM housing_houses h JOIN "
                        "housing_neighborhoods n USING(neighborhood_id) WHERE h.house_id=%ld",
                        r->house);
    if (res)
    {
      if ((row = mysql_fetch_row(res)))
        send_to_char(ch, "House %s: %s\r\nOwner: %s. Neighborhood: %s. Room ID: %ld.\r\n", row[0],
                     row[1], row[2], row[3], r->id);
      mysql_free_result(res);
    }
    return TRUE;
  }
  if (!str_cmp(verb, "chest"))
  {
    chest_command(ch, r, text);
    return TRUE;
  }
  if (!owns(ch, r->house))
  {
    send_to_char(ch, "Only the owner can change this house.\r\n");
    return TRUE;
  }
  if (!str_cmp(verb, "guest"))
  {
    id = get_id_by_name(text);
    if (id < 0 || id == owner(r->house))
    {
      send_to_char(ch, "Choose an existing player other than the owner.\r\n");
      return TRUE;
    }
    res = estate_select("SELECT guest_id FROM housing_guests WHERE house_id=%ld AND guest_id=%ld",
                        r->house, id);
    if (res)
    {
      int exists = mysql_num_rows(res) > 0;
      mysql_free_result(res);
      if (estate_query(exists ? "DELETE FROM housing_guests WHERE house_id=%ld AND guest_id=%ld"
                              : "INSERT INTO housing_guests(house_id,guest_id) VALUES(%ld,%ld)",
                       r->house, id))
        send_to_char(ch, "Guest access %s.\r\n", exists ? "removed" : "granted");
    }
    return TRUE;
  }
  if (!str_cmp(verb, "build"))
  {
    room_rnum nr;
    long new_id;
    int order = 0;
    for (dir = 0; dir < NUM_OF_DIRS; dir++)
      if (is_abbrev(text, dirs[dir]))
        break;
    if (!*text || dir >= NUM_OF_DIRS || dir >= 10 || world[IN_ROOM(ch)].dir_option[dir])
    {
      send_to_char(ch, "Choose an unused cardinal, diagonal, up or down exit.\r\n");
      return TRUE;
    }
    amount = price(r->house, "room_price");
    if (!afford(ch, amount))
      return TRUE;
    res = estate_select(
        "SELECT COALESCE(MAX(room_order),0)+1 FROM housing_rooms WHERE house_id=%ld", r->house);
    if (!res)
      return TRUE;
    if ((row = mysql_fetch_row(res)))
      order = atoi(row[0]);
    mysql_free_result(res);
    if (!estate_query("START TRANSACTION"))
      return TRUE;
    if (!estate_query(
            "INSERT INTO housing_rooms(house_id,room_order,title,description) VALUES(%ld,%d,'An "
            "Unfinished Room','This room awaits your imagination.\r\n')",
            r->house, order))
      goto build_fail;
    new_id = (long)mysql_insert_id(conn);
    if (!estate_query("INSERT INTO housing_room_exits(from_room_id,direction,to_room_id) "
                      "VALUES(%ld,%d,%ld),(%ld,%d,%ld)",
                      r->id, dir, new_id, new_id, rev_dir[dir], r->id))
      goto build_fail;
    if (!estate_query("COMMIT"))
      goto build_fail;
    charge(ch, amount);
    nr = make_room(r->house, new_id, "An Unfinished Room", "This room awaits your imagination.\r\n",
                   FALSE);
    if (nr == NOWHERE)
    {
      send_to_char(ch, "Your new room is saved; re-enter the house to load it.\r\n");
      return TRUE;
    }
    connect_rooms(r->id, dir, new_id);
    connect_rooms(new_id, rev_dir[dir], r->id);
    send_to_char(ch, "Room built %s.\r\n", dirs[dir]);
    return TRUE;
  build_fail:
    estate_query("ROLLBACK");
    send_to_char(ch, "Room construction failed.\r\n");
    return TRUE;
  }
  if (!str_cmp(verb, "title") || !str_cmp(verb, "description") || !str_cmp(verb, "extra"))
  {
    int extra = !str_cmp(verb, "extra"), title = !str_cmp(verb, "title");
    char *q, *key = NULL;
    const char *column = title ? "title" : "description";
    if (extra)
    {
      text = one_argument(text, arg, sizeof(arg));
      skip_spaces_c(&text);
      key = estate_quote(arg);
    }
    if (!*text || strlen(text) > (title ? 200 : 4000) || (extra && (!*arg || strlen(arg) > 100)))
    {
      free(key);
      send_to_char(ch, "Invalid description length.\r\n");
      return TRUE;
    }
    amount = price(r->house, title   ? "title_price"
                             : extra ? "extra_description_price"
                                     : "description_price");
    if (!afford(ch, amount))
    {
      free(key);
      return TRUE;
    }
    q = estate_quote(text);
    if (!q || (extra && !key))
    {
      free(q);
      free(key);
      return TRUE;
    }
    if (extra ? estate_query(
                    "INSERT INTO housing_room_extra_descriptions(room_id,keyword,description) "
                    "VALUES(%ld,'%s','%s')",
                    r->id, key, q)
              : estate_query("UPDATE housing_rooms SET %s='%s' WHERE room_id=%ld AND house_id=%ld",
                             column, q, r->id, r->house))
    {
      if (!extra)
      {
        char **field = title ? &world[IN_ROOM(ch)].name : &world[IN_ROOM(ch)].description;
        free(*field);
        *field = title ? strdup(text) : description_text(text);
      }
      else
        materialize(r->house);
      charge(ch, amount);
      send_to_char(ch, "Room updated.\r\n");
    }
    free(q);
    free(key);
    return TRUE;
  }
  send_to_char(ch, "Unknown house command. Type house for help.\r\n");
  return TRUE;
}
int housing_admin(struct char_data *ch, const char *argument)
{
  char verb[MAX_INPUT_LENGTH], sub[MAX_INPUT_LENGTH], arg[MAX_INPUT_LENGTH];
  const char *text;
  MYSQL_RES *res;
  MYSQL_ROW row;
  room_vnum vnum;
  char *q;
  text = one_argument(argument, verb, sizeof(verb));
  if (str_cmp(verb, "neighborhood") && str_cmp(verb, "houses"))
    return FALSE;
  if (!estate_init())
  {
    send_to_char(ch, "Estate database is unavailable.\r\n");
    return TRUE;
  }
  if (!str_cmp(verb, "houses"))
  {
    long id = atol(text);
    res = estate_select("SELECT house_id,neighborhood_id,owner_name,name FROM housing_houses WHERE "
                        "(%ld=0 OR neighborhood_id=%ld) ORDER BY house_id",
                        id, id);
    if (res)
    {
      while ((row = mysql_fetch_row(res)))
        send_to_char(ch, "House %s, neighborhood %s: %s (%s)\r\n", row[0], row[1], row[3], row[2]);
      mysql_free_result(res);
    }
    return TRUE;
  }
  text = one_argument(text, sub, sizeof(sub));
  if (!str_cmp(sub, "list"))
  {
    res = estate_select("SELECT neighborhood_id,entrance_room_vnum,name FROM housing_neighborhoods "
                        "ORDER BY neighborhood_id");
    if (res)
    {
      while ((row = mysql_fetch_row(res)))
        send_to_char(ch, "[%s] room %s: %s\r\n", row[0], row[1], row[2]);
      mysql_free_result(res);
    }
    return TRUE;
  }
  if (!str_cmp(sub, "add"))
  {
    text = one_argument(text, arg, sizeof(arg));
    skip_spaces_c(&text);
    vnum = atoi(arg);
    if (real_room(vnum) == NOWHERE || housing_is_room(vnum) || !*text || strlen(text) > 100)
    {
      send_to_char(ch, "Usage: hcontrol neighborhood add <existing room vnum> <name>\r\n");
      return TRUE;
    }
    q = estate_quote(text);
    if (q)
    {
      if (estate_query("INSERT INTO housing_neighborhoods(name,entrance_room_vnum) VALUES('%s',%d)",
                       q, vnum))
        send_to_char(ch, "Neighborhood %llu created.\r\n",
                     (unsigned long long)mysql_insert_id(conn));
      free(q);
    }
    return TRUE;
  }
  send_to_char(
      ch,
      "hcontrol neighborhood list | add <room vnum> <name>; hcontrol houses [neighborhood ID]\r\n");
  return TRUE;
}
/* Chest objects are fixtures; their contents live in MySQL immediately.
 * Loose floor objects have the same nonpersistent behavior as the SW system. */
void housing_save_all(void)
{
  struct home_room *r, *it, **link;
  struct home_chest **clink, *c;
  long house;
  int busy;
  room_rnum nr;
  r = home_rooms;
  while (r)
  {
    house = r->house;
    busy = FALSE;
    for (it = home_rooms; it; it = it->next)
      if (it->house == house)
      {
        struct obj_data *obj;
        nr = real_room(it->vnum);
        if (nr == NOWHERE)
          continue;
        if (world[nr].people || world[nr].room_affections || world[nr].events)
          busy = TRUE;
        for (obj = world[nr].contents; obj; obj = obj->next_content)
          if (!chest_record(obj))
            busy = TRUE;
      }
    if (busy)
    {
      r = r->next;
      continue;
    }
    for (clink = &home_chests; *clink;)
    {
      c = *clink;
      if (c->house != house)
      {
        clink = &c->next;
        continue;
      }
      *clink = c->next;
      if (c->obj)
        extract_obj(c->obj);
      free(c);
    }
    for (link = &home_rooms; *link;)
    {
      it = *link;
      if (it->house != house)
      {
        link = &it->next;
        continue;
      }
      nr = real_room(it->vnum);
      *link = it->next;
      if (nr != NOWHERE)
        delete_runtime_room(nr);
      free(it);
    }
    r = home_rooms;
  }
}

void housing_object_extracted(struct obj_data *obj)
{
  struct home_chest *c = chest_record(obj);
  if (c)
    c->obj = NULL;
}
