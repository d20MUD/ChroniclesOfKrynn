/* Integration harness: real estate services/MySQL/object codec, small game adapter. */
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "act.h"
#include "auction_house.h"
#include "comm.h"
#include "constants.h"
#include "db.h"
#include "estate.h"
#include "handler.h"
#include "housing.h"
#include "interpreter.h"
#include <assert.h>
MYSQL *conn;
bool mysql_available = TRUE;
struct room_data *world;
room_rnum top_of_world;
struct player_special_data dummy_mob;
struct command_info *complete_cmd_info;
const char *dirs[] = {"north",     "east",      "south",     "west",      "up", "down",
                      "northwest", "northeast", "southeast", "southwest", "\n"};
const char *item_types[NUM_ITEM_TYPES] = {[ITEM_WEAPON] = "Weapon", [ITEM_ARMOR] = "Armor"};
const char *wear_bits[NUM_ITEM_WEARS] = {
    [ITEM_WEAR_BODY] = "Body", [ITEM_WEAR_WIELD] = "Wield", [ITEM_WEAR_ON_BACK] = "On-Back"};
size_t wear_bits_count = NUM_ITEM_WEARS;
int rev_dir[] = {2, 3, 0, 1, 5, 4, 8, 9, 6, 7};
static char output[32768];
void basic_mud_log(const char *format, ...)
{
  va_list ap;
  va_start(ap, format);
  vfprintf(stderr, format, ap);
  va_end(ap);
  fputc('\n', stderr);
}
size_t send_to_char(struct char_data *ch, const char *format, ...)
{
  va_list ap;
  (void)ch;
  va_start(ap, format);
  vsnprintf(output + strlen(output), sizeof(output) - strlen(output), format, ap);
  va_end(ap);
  return strlen(output);
}
const char *act(const char *s, int hide, struct char_data *ch, struct obj_data *obj, void *vict,
                int type)
{
  (void)hide;
  (void)ch;
  (void)obj;
  (void)vict;
  (void)type;
  send_to_char(ch, "%s\n", s);
  return s;
}
int MAX(int a, int b) { return a > b ? a : b; }
size_t strlcpy(char *dst, const char *src, size_t n)
{
  size_t len = strlen(src);
  if (n)
  {
    size_t copy = len < n - 1 ? len : n - 1;
    memcpy(dst, src, copy);
    dst[copy] = 0;
  }
  return len;
}
size_t strlcat(char *dst, const char *src, size_t n)
{
  size_t len = strlen(dst);
  return len + strlcpy(dst + len, src, n - len);
}
void skip_spaces_c(const char **s)
{
  while (**s && isspace((unsigned char)**s))
    (*s)++;
}
void skip_spaces(char **s)
{
  while (**s && isspace((unsigned char)**s))
    (*s)++;
}
const char *one_argument(const char *s, char *arg, size_t n)
{
  size_t i = 0;
  skip_spaces_c(&s);
  while (*s && !isspace((unsigned char)*s))
  {
    if (i + 1 < n)
      arg[i++] = *s;
    s++;
  }
  arg[i] = 0;
  skip_spaces_c(&s);
  return s;
}
int is_abbrev(const char *a, const char *b) { return *a && !strncasecmp(a, b, strlen(a)); }
int isname(const char *a, const char *b)
{
  char word[512];
  while (*b)
  {
    b = one_argument(b, word, sizeof(word));
    if (!strcasecmp(a, word))
      return TRUE;
  }
  return FALSE;
}
int find_all_dots(char *s)
{
  if (!strcmp(s, "all"))
    return FIND_ALL;
  if (!strncmp(s, "all.", 4))
  {
    memmove(s, s + 4, strlen(s + 4) + 1);
    return FIND_ALLDOT;
  }
  return FIND_INDIV;
}
int get_number(char **s)
{
  char *dot = strchr(*s, '.');
  int n;
  if (!dot)
    return 1;
  n = atoi(*s);
  *s = dot + 1;
  return n;
}
long get_id_by_name(const char *s) { return !strcasecmp(s, "guest") ? 3 : -1; }
room_rnum real_room(room_vnum v)
{
  room_rnum i;
  for (i = 0; i <= top_of_world; i++)
    if (world[i].number == v)
      return i;
  return NOWHERE;
}
room_rnum add_runtime_room(struct room_data *r)
{
  top_of_world++;
  world = realloc(world, (top_of_world + 1) * sizeof(*world));
  world[top_of_world] = *r;
  world[top_of_world].name = strdup(r->name);
  world[top_of_world].description = strdup(r->description);
  return top_of_world;
}
int delete_runtime_room(room_rnum r)
{
  int i;
  assert(!world[r].people && !world[r].contents);
  free(world[r].name);
  free(world[r].description);
  for (i = 0; i < NUM_OF_DIRS; i++)
    free(world[r].dir_option[i]);
  memmove(world + r, world + r + 1, (top_of_world - r) * sizeof(*world));
  top_of_world--;
  return TRUE;
}
void look_at_room(struct char_data *ch, int ignore)
{
  (void)ch;
  (void)ignore;
}
void char_from_room(struct char_data *ch)
{
  struct char_data **p = &world[IN_ROOM(ch)].people;
  while (*p && *p != ch)
    p = &(*p)->next_in_room;
  if (*p)
    *p = ch->next_in_room;
  IN_ROOM(ch) = NOWHERE;
  ch->next_in_room = NULL;
}
void char_to_room(struct char_data *ch, room_rnum r)
{
  IN_ROOM(ch) = r;
  ch->next_in_room = world[r].people;
  world[r].people = ch;
}
struct obj_data *create_obj(void)
{
  struct obj_data *obj = calloc(1, sizeof(*obj));
  obj->item_number = NOTHING;
  IN_ROOM(obj) = NOWHERE;
  return obj;
}
void obj_to_char(struct obj_data *obj, struct char_data *ch)
{
  obj->carried_by = ch;
  obj->next_content = ch->carrying;
  ch->carrying = obj;
  IS_CARRYING_N(ch)++;
  IS_CARRYING_W(ch) += GET_OBJ_WEIGHT(obj);
}
void obj_from_char(struct obj_data *obj)
{
  struct char_data *ch = obj->carried_by;
  struct obj_data **p = &ch->carrying;
  while (*p && *p != obj)
    p = &(*p)->next_content;
  assert(*p);
  *p = obj->next_content;
  obj->next_content = NULL;
  obj->carried_by = NULL;
  IS_CARRYING_N(ch)--;
  IS_CARRYING_W(ch) -= GET_OBJ_WEIGHT(obj);
}
void obj_to_obj(struct obj_data *obj, struct obj_data *container)
{
  obj->in_obj = container;
  obj->next_content = container->contains;
  container->contains = obj;
  GET_OBJ_WEIGHT(container) += GET_OBJ_WEIGHT(obj);
}
void obj_from_obj(struct obj_data *obj)
{
  struct obj_data *parent = obj->in_obj, **p = &parent->contains;
  while (*p && *p != obj)
    p = &(*p)->next_content;
  assert(*p);
  *p = obj->next_content;
  parent->obj_flags.weight -= obj->obj_flags.weight;
  obj->next_content = NULL;
  obj->in_obj = NULL;
}
struct obj_data *unequip_char(struct char_data *ch, int pos)
{
  struct obj_data *obj = GET_EQ(ch, pos);
  GET_EQ(ch, pos) = NULL;
  obj->worn_by = NULL;
  obj->worn_on = -1;
  return obj;
}
void obj_to_room(struct obj_data *obj, room_rnum r)
{
  IN_ROOM(obj) = r;
  obj->next_content = world[r].contents;
  world[r].contents = obj;
}
void obj_from_room(struct obj_data *obj)
{
  struct obj_data **p = &world[IN_ROOM(obj)].contents;
  while (*p && *p != obj)
    p = &(*p)->next_content;
  assert(*p);
  *p = obj->next_content;
  obj->next_content = NULL;
  IN_ROOM(obj) = NOWHERE;
}
void extract_obj(struct obj_data *obj)
{
  housing_object_extracted(obj);
  struct obj_data *it, *next;
  if (obj->carried_by)
    obj_from_char(obj);
  if (IN_ROOM(obj) != NOWHERE)
    obj_from_room(obj);
  for (it = obj->contains; it; it = next)
  {
    next = it->next_content;
    it->in_obj = NULL;
    extract_obj(it);
  }
  if (obj->sheath_primary)
    extract_obj(obj->sheath_primary);
  if (obj->sheath_secondary)
    extract_obj(obj->sheath_secondary);
  free(obj->name);
  free(obj->short_description);
  free(obj->description);
  free(obj);
}
struct obj_data *get_obj_in_list_vis(struct char_data *ch, char *s, int *number,
                                     struct obj_data *list)
{
  int n = get_number(&s);
  (void)ch;
  (void)number;
  for (; list; list = list->next_content)
    if (isname(s, list->name) && !--n)
      return list;
  return NULL;
}
int can_carry_weight_limit(struct char_data *ch)
{
  (void)ch;
  return 100000;
}
int get_filename(char *out, size_t n, int mode, const char *name)
{
  (void)mode;
  snprintf(out, n, "%s.plr", name);
  return TRUE;
}
void save_char(struct char_data *ch, int mode)
{
  char path[128];
  FILE *f;
  (void)mode;
  get_filename(path, sizeof(path), PLR_FILE, GET_NAME(ch));
  f = fopen(path, "w");
  assert(f);
  fprintf(f, "Gold: %d\nECrd: %llu\nEDbt: %llu\n", GET_GOLD(ch),
          ch->player_specials->estate_credit_receipt, ch->player_specials->estate_debit_receipt);
  fclose(f);
}
static void save_item(struct char_data *ch, struct obj_data *obj)
{
  char *name, *data, *q;
  struct obj_data *it;
  data = estate_pack(obj);
  assert(data);
  q = estate_quote(data);
  name = estate_quote(GET_NAME(ch));
  assert(estate_query(
      "INSERT INTO player_save_objs(name,serialized_obj) VALUES('%s','EId : %llu\\n%s')", name,
      obj->estate_delivery_id, q));
  free(name);
  free(q);
  free(data);
  for (it = obj->contains; it; it = it->next_content)
    save_item(ch, it);
}
void Crash_crashsave(struct char_data *ch)
{
  char *q = estate_quote(GET_NAME(ch));
  struct obj_data *it;
  int i;
  assert(estate_query("DELETE FROM player_save_objs WHERE name='%s'", q));
  free(q);
  for (it = ch->carrying; it; it = it->next_content)
    save_item(ch, it);
  for (i = 0; i < NUM_WEARS; i++)
    if (GET_EQ(ch, i))
      save_item(ch, GET_EQ(ch, i));
}
int call_magic(struct char_data *ch, struct char_data *v, struct obj_data *o, int spell, int level,
               int cast, int extra)
{
  (void)v;
  (void)spell;
  (void)level;
  (void)cast;
  (void)extra;
  send_to_char(ch, "IDENTIFY %s\n", o->short_description);
  return 1;
}
/* Lightweight record adapter; runner also tests the actual production codec. */
#ifndef ESTATE_REAL_CODEC
int estate_write_record(struct obj_data *obj, FILE *fp)
{
  fprintf(fp, "%llu %d %d %d %d %d %d %d %d %d %d\n%s\n%s\n", obj->estate_delivery_id,
          GET_OBJ_WEIGHT(obj), GET_OBJ_TYPE(obj), obj->affected[0].location,
          obj->affected[0].modifier, obj->affected[0].specific, GET_OBJ_WEAR(obj)[0],
          GET_OBJ_WEAR(obj)[1], GET_OBJ_WEAR(obj)[2], GET_OBJ_WEAR(obj)[3], GET_OBJ_BOUND_ID(obj),
          obj->name, obj->short_description);
  return TRUE;
}
obj_save_data *objsave_parse_objects(FILE *fp)
{
  char line[512];
  struct obj_data *obj = create_obj();
  obj_save_data *data = calloc(1, sizeof(*data));
  assert(fgets(line, sizeof(line), fp));
  assert(sscanf(line, "%llu %d %hhd %hhd %d %d %d %d %d %d %d", &obj->estate_delivery_id,
                &GET_OBJ_WEIGHT(obj), &GET_OBJ_TYPE(obj), &obj->affected[0].location,
                &obj->affected[0].modifier, &obj->affected[0].specific, &GET_OBJ_WEAR(obj)[0],
                &GET_OBJ_WEAR(obj)[1], &GET_OBJ_WEAR(obj)[2], &GET_OBJ_WEAR(obj)[3],
                &GET_OBJ_BOUND_ID(obj)) == 11);
  assert(fgets(line, sizeof(line), fp));
  line[strcspn(line, "\n")] = 0;
  obj->name = strdup(line);
  assert(fgets(line, sizeof(line), fp));
  line[strcspn(line, "\n")] = 0;
  obj->short_description = strdup(line);
  obj->description = strdup(line);
  SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_IDENTIFIED);
  data->obj = obj;
  return data;
}
#endif
static void init_player(struct char_data *ch, const char *name, long id)
{
  memset(ch, 0, sizeof(*ch));
  ch->player_specials = calloc(1, sizeof(*ch->player_specials));
  ch->player.name = strdup(name);
  GET_IDNUM(ch) = id;
  GET_LEVEL(ch) = id == 4 ? LVL_GRSTAFF : 20;
  GET_GOLD(ch) = 500000;
  IN_ROOM(ch) = NOWHERE;
  ch->real_abils.dex = 18;
  char_to_room(ch, 1);
}
static long scalar(const char *sql)
{
  MYSQL_RES *res = estate_select("%s", sql);
  MYSQL_ROW row;
  long value;
  assert(res);
  row = mysql_fetch_row(res);
  assert(row);
  value = atol(row[0]);
  mysql_free_result(res);
  return value;
}
static void house(struct char_data *ch, const char *cmd)
{
  output[0] = 0;
  assert(housing_command(ch, cmd));
}
static void auction(struct char_data *ch, const char *cmd)
{
  output[0] = 0;
  auction_house_command(ch, cmd);
}
static struct obj_data *item(const char *name)
{
  struct obj_data *obj = create_obj();
  obj->name = strdup(name);
  obj->short_description = strdup(name);
  obj->description = strdup(name);
  GET_OBJ_BOUND_ID(obj) = NOBODY;
  GET_OBJ_TYPE(obj) = ITEM_WEAPON;
  GET_OBJ_WEIGHT(obj) = 5;
  SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_IDENTIFIED);
  SET_BIT_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_TAKE);
  SET_BIT_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_ON_BACK);
  return obj;
}
int main(int argc, char **argv)
{
  struct char_data seller, buyer, guest, admin;
  struct obj_data *obj, *chest, *restored;
  char *packed;
  assert(argc == 2);
  conn = mysql_init(NULL);
  assert(mysql_real_connect(conn, NULL, "root", NULL, "estate_test", 0, argv[1], 0));
  assert(estate_init());
  assert(estate_query(
      "CREATE TABLE player_save_objs(idnum INT PRIMARY KEY AUTO_INCREMENT,name "
      "VARCHAR(64),serialized_obj LONGTEXT,creation_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP)"));
  top_of_world = 1;
  world = calloc(2, sizeof(*world));
  world[0].number = 0;
  world[1].number = 2200;
  init_player(&seller, "seller", 1);
  init_player(&buyer, "buyer", 2);
  init_player(&guest, "guest", 3);
  init_player(&admin, "admin", 4);
  assert(housing_admin(&admin, "neighborhood add 2200 Test Neighborhood"));
  assert(scalar("SELECT COUNT(*) FROM housing_neighborhoods") == 1);
#ifdef ESTATE_REAL_CODEC
  /* Exercise SQL persistence too, including customization and typed affections. */
  obj = item("engraved dagger");
  free(obj->short_description);
  obj->short_description = strdup("a smith's engraved dagger");
  obj->action_description = strdup("A smith's inscription.\nA second line.\n");
  obj->affected[0].location = 1;
  obj->affected[0].modifier = 4;
  obj->affected[0].specific = 7;
  obj->affected[0].bonus_type = BONUS_TYPE_ENHANCEMENT;
  obj->estate_delivery_id = 123456;
  GET_OBJ_VAL(obj, 15) = 42;
  obj->special_abilities = calloc(1, sizeof(*obj->special_abilities));
  obj->special_abilities->ability = 11;
  obj->special_abilities->next = calloc(1, sizeof(*obj->special_abilities));
  obj->special_abilities->next->ability = 22;
  GET_OBJ_PROF(obj) = 9;
  GET_OBJ_TIMER(obj) = 17;
  obj->weapon_poison.poison = 11;
  obj->weapon_poison.poison_hits = 3;
  obj->wpn_spells[1].spellnum = 6;
  obj->wpn_spells[1].uses_left = 5;
  obj->tinker_bonus = 4;
  {
    FILE *record = tmpfile();
    MYSQL_RES *res;
    MYSQL_ROW row;
    obj_save_data *loaded;
    assert(record && objsave_save_obj_record_db(obj, &seller, NOWHERE, record, 3));
    fclose(record);
    res = estate_select("SELECT serialized_obj FROM player_save_objs WHERE name='seller'");
    assert(res);
    row = mysql_fetch_row(res);
    assert(row);
    record = fmemopen(row[0], strlen(row[0]), "r");
    assert(record);
    loaded = objsave_parse_objects(record);
    fclose(record);
    assert(loaded && !loaded->next);
    assert(loaded->locate == 3);
    restored = loaded->obj;
    free(loaded);
    mysql_free_result(res);
    assert(!strcmp(restored->short_description, "a smith's engraved dagger"));
    assert(strstr(restored->action_description, "A second line."));
    assert(restored->estate_delivery_id == 123456 && restored->affected[0].specific == 7);
    assert(GET_OBJ_VAL(restored, 15) == 42 && GET_OBJ_BOUND_ID(restored) == (int)NOBODY);
    assert(restored->special_abilities && restored->special_abilities->next &&
           restored->special_abilities->ability == 11 &&
           restored->special_abilities->next->ability == 22);
    assert(GET_OBJ_PROF(restored) == 9 && GET_OBJ_TIMER(restored) == 17);
    {
      obj_save_data *database = objsave_parse_objects_db("seller", NOWHERE);
      assert(database && !database->next);
      assert(database->locate == 3 && database->db_idnum > 0 &&
             database->obj->estate_delivery_id == 123456);
      assert(strstr(database->obj->action_description, "A second line.") &&
             database->obj->special_abilities->next);
      extract_obj(database->obj);
      free(database);
    }

    assert(restored->weapon_poison.poison_hits == 3 && restored->wpn_spells[1].uses_left == 5 &&
           restored->tinker_bonus == 4);
    extract_obj(restored);
    extract_obj(obj);
    assert(estate_query("DELETE FROM player_save_objs"));
  }
#endif
  house(&seller, "buy Test Home");
  assert(scalar("SELECT COUNT(*) FROM housing_houses") == 1);
  assert(GET_GOLD(&seller) == 400000);
  house(&seller, "buy Another");
  assert(scalar("SELECT COUNT(*) FROM housing_houses") == 1);
  assert(GET_GOLD(&seller) == 400000);
  house(&buyer, "enter 1");
  assert(IN_ROOM(&buyer) == 1);
  house(&seller, "enter 1");
  assert(housing_is_room(GET_ROOM_VNUM(IN_ROOM(&seller))));
  house(&seller, "build north");
  assert(scalar("SELECT COUNT(*) FROM housing_rooms") == 2);
  assert(scalar("SELECT COUNT(*) FROM housing_room_exits") == 2);
  house(&seller, "build north");
  assert(scalar("SELECT COUNT(*) FROM housing_rooms") == 2);
  house(&seller, "title A Fine Hall");
  assert(!strcmp(world[IN_ROOM(&seller)].name, "A Fine Hall"));
  house(&seller, "description My custom hall.");
  house(&seller, "extra painting A bright painting.");
  house(&seller, "guest guest");
  house(&guest, "enter 1");
  assert(IN_ROOM(&guest) == IN_ROOM(&seller));
  house(&seller, "chest buy");
  chest = world[IN_ROOM(&seller)].contents;
  assert(chest);
  house(&seller, "chest keywords 1 lockbox");
  assert(isname("lockbox", chest->name));
  obj = item("dagger");
  obj->affected[0].location = 1;
  obj->affected[0].modifier = 4;
  obj->affected[0].specific = 7;
  obj_to_obj(item("gem"), obj);
  obj_to_obj(item("bluegem"), obj);
  obj->sheath_primary = item("blade");
  packed = estate_pack(obj);
  assert(packed);
  restored = estate_unpack(packed);
  free(packed);
  assert(restored && restored->contains && restored->sheath_primary);
  assert(GET_OBJ_WEIGHT(restored) == GET_OBJ_WEIGHT(obj));
  assert(restored->affected[0].specific == 7);
  assert(!strcmp(restored->contains->name, obj->contains->name) &&
         restored->contains->next_content);
  extract_obj(restored);
  obj_to_char(obj, &seller);
  assert(housing_put(&guest, obj, chest));
  assert(obj->carried_by == &seller);
  assert(housing_put(&seller, obj, chest));
  assert(scalar("SELECT COUNT(*) FROM housing_chest_items") == 1);
  assert(housing_get(&seller, chest, "dagger", 1));
  assert(seller.carrying && seller.carrying->contains);
  assert(scalar("SELECT COUNT(*) FROM housing_chest_items") == 0);
  assert(scalar("SELECT COUNT(*) FROM estate_deliveries WHERE delivered=0") == 0);
  house(&seller, "leave");
  house(&guest, "leave");
  housing_save_all();
  assert(top_of_world == 1);
  house(&seller, "enter 1");
  assert(!strcmp(world[IN_ROOM(&seller)].name, "A Fine Hall"));
  assert(world[IN_ROOM(&seller)].ex_description);
  house(&seller, "leave");
  /* Native prototypes also use zero as an unassigned binding ID. */
  GET_OBJ_BOUND_ID(seller.carrying)=0;
  auction(&seller, "sell dagger 1234");
  assert(!seller.carrying);
  assert(scalar("SELECT COUNT(*) FROM auction_listings WHERE listing_state='active'") == 1);
  auction(&buyer, "list weapons DAGGER");
  assert(strstr(output, "dagger"));
  auction(&buyer, "list on-back DAGGER");
  assert(strstr(output, "dagger"));
  auction(&buyer, "view 1");
  assert(strstr(output, "IDENTIFY"));
  auction(&seller, "buy 1");
  assert(!seller.carrying);
  auction(&buyer, "buy 1");
  assert(buyer.carrying && buyer.carrying->contains && buyer.carrying->sheath_primary);
  assert(GET_GOLD(&buyer) == 498766);
  auction(&buyer, "buy 1");
  assert(GET_GOLD(&buyer) == 498766);
  {
    int before = GET_GOLD(&seller);
    auction_house_collect(&seller);
    assert(GET_GOLD(&seller) == before + 1234);
    auction_house_collect(&seller);
    assert(GET_GOLD(&seller) == before + 1234);
  }
  estate_recover(&buyer);
  assert(IS_CARRYING_N(&buyer) == 1 && GET_GOLD(&buyer) == 498766);
  auction(&buyer, "sell dagger 2000");
  assert(!buyer.carrying);
  auction(&buyer, "reprice 2 2500");
  assert(scalar("SELECT price FROM auction_listings WHERE listing_id=2") == 2500);
  assert(estate_query(
      "UPDATE auction_listings SET expires_at=DATE_SUB(NOW(),INTERVAL 1 DAY) WHERE listing_id=2"));
  auction(&seller, "buy 2");
  assert(!seller.carrying);
  auction(&buyer, "recover 2");
  assert(buyer.carrying);
  assert(scalar("SELECT COUNT(*) FROM auction_listings WHERE listing_state='expired'") == 1);
  auction(&buyer, "sell dagger 3000");
  auction(&buyer, "cancel 3");
  assert(buyer.carrying);
  /* Simulate interruption after delivery commit, before currency/inventory save. */
  obj = item("recovered");
  assert(estate_query("START TRANSACTION"));
  assert(estate_queue_delivery(&buyer, obj, 100));
  assert(estate_query("COMMIT"));
  extract_obj(obj);
  estate_recover(&buyer);
  assert(IS_CARRYING_N(&buyer) == 2 && GET_GOLD(&buyer) == 498666);
  estate_recover(&buyer);
  assert(IS_CARRYING_N(&buyer) == 2 && GET_GOLD(&buyer) == 498666);
  /* SQL acknowledgement failure must not expose an item that could be given away. */
  assert(estate_query("CREATE TRIGGER reject_delivery_ack BEFORE UPDATE ON estate_deliveries FOR "
                      "EACH ROW BEGIN IF NEW.delivered=1 THEN SIGNAL SQLSTATE '45000' SET "
                      "MESSAGE_TEXT='test delivery failure'; END IF; END"));
  obj = item("delayed");
  assert(estate_query("START TRANSACTION"));
  assert(estate_queue_delivery(&buyer, obj, 50));
  assert(estate_query("COMMIT"));
  extract_obj(obj);
  estate_recover(&buyer);
  assert(IS_CARRYING_N(&buyer) == 2 && GET_GOLD(&buyer) == 498616);
  assert(scalar("SELECT COUNT(*) FROM estate_deliveries WHERE delivered=0") == 1);
  assert(estate_query("DROP TRIGGER reject_delivery_ack"));
  estate_recover(&buyer);
  assert(IS_CARRYING_N(&buyer) == 3 && GET_GOLD(&buyer) == 498616);
  estate_recover(&buyer);
  assert(IS_CARRYING_N(&buyer) == 3 && GET_GOLD(&buyer) == 498616);
  /* Simulate committed outgoing escrow while the old player inventory is still saved. */
  obj = item("escrowed");
  obj_to_char(obj, &seller);
  {
    unsigned long long id = estate_prepare_escrow(&seller, obj);
    assert(id);
    assert(estate_query("START TRANSACTION"));
    assert(estate_commit_escrow(id));
    assert(estate_query("COMMIT"));
  }
  estate_recover(&seller);
  assert(!seller.carrying);
  assert(scalar("SELECT COUNT(*) FROM estate_outgoing WHERE escrow_state='committed'") == 0);
  /* Replaying an unacknowledged seller payout must respect the saved receipt. */
  assert(estate_query("UPDATE auction_credit_settlements SET settlement_state='pending'"));
  {
    int before = GET_GOLD(&seller);
    auction_house_collect(&seller);
    assert(GET_GOLD(&seller) == before);
  }
  assert(!estate_unpack("EOBJ1 999999999 1 0 0\n"));
  puts("PASS: neighborhood purchase/build/guests/reload, chest nesting/sheaths, auction "
       "escrow/search/expiry/cancel/reprice, payouts and delivery recovery");
  mysql_close(conn);
  return 0;
}
