/* Integration harness: real estate services/MySQL/object codec, small game adapter. */
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "deities.h"
#include "religion.h"
#include "restring.h"
#include "assign_wpn_armor.h"
#include "clan.h"
#include "quest.h"
#include "act.h"
#include "auction_house.h"
#include "comm.h"
#include "constants.h"
#include "db.h"
#include "estate.h"
#include "player_shop.h"
#include "handler.h"
#include "housing.h"
#include "interpreter.h"
#include <assert.h>
#include <json-c/json.h>
#ifdef LEGENDARY_PERSISTENCE_TEST
#include "legendary_identity.h"
#endif
#ifndef ESTATE_REAL_CODEC
struct index_data *obj_index;
obj_rnum top_of_objt = -1;
#endif
struct aq_data *aquest_table;
qst_rnum total_quests;
qst_rnum real_quest(qst_vnum vnum) {
  for(int i=0;i<total_quests;i++)if(QST_NUM(i)==vnum)return i;
  return NOTHING;
}
void clear_quest(struct char_data *ch,int index) {
  GET_QUEST(ch,index)=NOTHING;GET_QUEST_COUNTER(ch,index)=0;GET_QUEST_TIME(ch,index)=-1;
}
void set_quest(struct char_data *ch,qst_rnum r,int index) {
  GET_QUEST(ch,index)=QST_NUM(r);GET_QUEST_COUNTER(ch,index)=QST_QUANTITY(r);GET_QUEST_TIME(ch,index)=QST_TIME(r);
}
bool quest_church_eligible(struct char_data *ch,qst_rnum r,char **reason) {
  (void)reason;return GET_QUEST(ch,RELIGION_QUEST_SLOT)==NOTHING && GET_LEVEL(ch)>=QST_MINLEVEL(r) && GET_LEVEL(ch)<=QST_MAXLEVEL(r);
}
MYSQL *conn;
bool mysql_available = TRUE;
struct room_data *world;
room_rnum top_of_world;
struct player_special_data dummy_mob;
struct deity_info deity_list[NUM_DEITIES];
struct char_data *mob_proto;
struct index_data *mob_index;
mob_rnum top_of_mobt = -1;
mob_rnum real_mobile(mob_vnum vnum) { return mob_proto && vnum>=5000 && vnum<=5001 ? vnum-5000 : NOBODY; }
struct command_info *complete_cmd_info;
const char *dirs[] = {"north",     "east",      "south",     "west",      "up", "down",
                      "northwest", "northeast", "southeast", "southwest", "\n"};
const char *item_types[NUM_ITEM_TYPES] = {[ITEM_WEAPON] = "Weapon", [ITEM_ARMOR] = "Armor"};
const char *wear_bits[NUM_ITEM_WEARS] = {
    [ITEM_WEAR_BODY] = "Body", [ITEM_WEAR_WIELD] = "Wield", [ITEM_WEAR_ON_BACK] = "On-Back"};
size_t wear_bits_count = NUM_ITEM_WEARS;
int rev_dir[] = {2, 3, 0, 1, 5, 4, 8, 9, 6, 7};
static time_t church_clock;
time_t religion_task_test_time(void) {return church_clock ? church_clock : time(NULL);}
bool room_is_dark(room_rnum room) {(void)room;return false;}
bool char_has_ultra(struct char_data *ch) {(void)ch;return false;}
bool char_has_infra(struct char_data *ch) {(void)ch;return false;}
bool is_outdoors(struct char_data *ch) {(void)ch;return false;}
bool has_blindsight(struct char_data *ch) {(void)ch;return false;}
bool has_true_sight(struct char_data *ch) {(void)ch;return false;}
bool can_see_hidden(struct char_data *ch,struct char_data *vict) {(void)ch;(void)vict;return true;}
static char output[32768];
static int fail_church_saves;
static int restring_clan_policy, restring_clan_favor=5;
static bool restring_clan_member=true;
static int temple_edit_allowed = TRUE;
int can_edit_zone(struct char_data *ch, zone_rnum zone) {(void)ch;(void)zone;return temple_edit_allowed;}
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
/* Ordinary wear adapter shared by legendary integration fixtures. */
bool legendary_test_wear_allowed = true;
int invalid_align(struct char_data *ch, struct obj_data *o) { (void)ch; (void)o; return 0; }
int invalid_race(struct char_data *ch, struct obj_data *o) { (void)ch; (void)o; return 0; }
int invalid_class(struct char_data *ch, struct obj_data *o) { (void)ch; (void)o; return !legendary_test_wear_allowed; }
bool is_class_req_object(struct char_data *ch, struct obj_data *o, bool output) { (void)ch; (void)o; (void)output; return true; }
bool is_class_anti_object(struct char_data *ch, struct obj_data *o, bool output) { (void)ch; (void)o; (void)output; return false; }
int hands_needed_full(struct char_data *ch, struct obj_data *o, int feats) { (void)ch; (void)o; (void)feats; return 1; }
bool is_two_handed_ranged_weapon(struct obj_data *o) { return GET_OBJ_TYPE(o)==ITEM_FIREWEAPON; }
bool is_caster_class(int c) { return c==CLASS_WIZARD; }
bool is_riding_dragon_mount(struct char_data *ch) { return ch && RIDING(ch) && IS_NPC(RIDING(ch)) && GET_REAL_RACE(RIDING(ch))==RACE_TYPE_DRAGON && IN_ROOM(ch)==IN_ROOM(RIDING(ch)); }
/* Surrounding stat rebuild adapter; legendary apply/caps bookkeeping is real. */
#include "legendary_foundations.h"
#include "spells.h"
static int test_ability_cap=32,test_pool_cap=10000,test_save_cap=15;
void compute_char_cap(struct char_data *ch,int mode)
{
  int i;(void)mode;
  ch->aff_abils.str=ch->aff_abils.str>test_ability_cap?test_ability_cap:ch->aff_abils.str;
  ch->aff_abils.dex=ch->aff_abils.dex>test_ability_cap?test_ability_cap:ch->aff_abils.dex;
  ch->aff_abils.con=ch->aff_abils.con>test_ability_cap?test_ability_cap:ch->aff_abils.con;
  ch->aff_abils.intel=ch->aff_abils.intel>test_ability_cap?test_ability_cap:ch->aff_abils.intel;
  ch->aff_abils.wis=ch->aff_abils.wis>test_ability_cap?test_ability_cap:ch->aff_abils.wis;
  ch->aff_abils.cha=ch->aff_abils.cha>test_ability_cap?test_ability_cap:ch->aff_abils.cha;
  if(GET_MAX_MOVE(ch)>test_pool_cap)GET_MAX_MOVE(ch)=test_pool_cap;
  if(GET_MAX_PSP(ch)>test_pool_cap)GET_MAX_PSP(ch)=test_pool_cap;
  for(i=0;i<NUM_OF_SAVING_THROWS;i++)if(GET_SAVE(ch,i)>test_save_cap)GET_SAVE(ch,i)=test_save_cap;
}
void affect_total(struct char_data *ch)
{
  int i;if(!legendary_foundations_begin(ch))return;ch->aff_abils=ch->real_abils;
  GET_MAX_MOVE(ch)=GET_REAL_MAX_MOVE(ch);GET_MAX_PSP(ch)=GET_REAL_MAX_PSP(ch);GET_MAX_HIT(ch)=GET_REAL_MAX_HIT(ch);
  for(i=0;i<NUM_OF_SAVING_THROWS;i++)GET_SAVE(ch,i)=GET_REAL_SAVE(ch,i);
  for(i=0;i<NUM_DAM_TYPES;i++)GET_RESISTANCES(ch,i)=GET_REAL_RESISTANCES(ch,i);
  legendary_foundations_reset(ch);compute_char_cap(ch,0);legendary_foundations_apply(ch);
  legendary_foundation_hp(ch,GET_REAL_MAX_HIT(ch));legendary_foundations_end(ch);
}
int compute_armor_class(struct char_data *a,struct char_data *ch,int touch,int mode)
{(void)a;(void)touch;(void)mode;return 10+legendary_foundation_deflection(ch,0);}
int MAX(int a, int b) { return a > b ? a : b; }
int MIN(int a, int b) { return a < b ? a : b; }
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
static bool preserve_argument_separator;
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
  /* Exercise production separator behavior in the restring editor tests. */
  if (!preserve_argument_separator)
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
char *object_short_description(const struct obj_data *obj) { return obj->short_description; }
void extract_obj(struct obj_data *obj)
{
#ifdef LEGENDARY_PERSISTENCE_TEST
  legendary_object_unload(obj);
#endif
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
  snprintf(out, n, "%s.%s", name, mode == CRASH_FILE ? "objs" : "plr");
  return TRUE;
}
#include "durable_file.h"
bool save_char_checked(struct char_data *ch, int mode)
{
  char path[128], data[256];
  if (fail_church_saves) {--fail_church_saves;return false;}
  int len;
  (void)mode;
  get_filename(path, sizeof(path), PLR_FILE, GET_NAME(ch));
  len=snprintf(data,sizeof(data),"Gold: %d\nECrd: %llu\nEDbt: %llu\nLgTx: %s\n", GET_GOLD(ch),
      ch->player_specials->estate_credit_receipt,ch->player_specials->estate_debit_receipt,
      ch->player_specials->legendary_operation);
  return durable_file_replace(path,data,(size_t)len);
}
void save_char(struct char_data *ch, int mode) { assert(save_char_checked(ch,mode)); }

#ifndef LEGENDARY_TRANSACTION_TEST
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

#endif
bool Crash_crashsave_checked(struct char_data *ch)
{
#ifdef LEGENDARY_TRANSACTION_TEST
  extern bool test_real_crashsave_checked(struct char_data *);
  return test_real_crashsave_checked(ch);
#else

  char *q = estate_quote(GET_NAME(ch));
  assert(estate_query("START TRANSACTION"));
  struct obj_data *it;
  int i;
  assert(estate_query("DELETE FROM player_save_objs WHERE name='%s'", q));
  free(q);
  for (it = ch->carrying; it; it = it->next_content)
    save_item(ch, it);
  for (i = 0; i < NUM_WEARS; i++)
    if (GET_EQ(ch, i))
      save_item(ch, GET_EQ(ch, i));
  assert(estate_query("COMMIT"));
  return true;
#endif
}
void Crash_crashsave(struct char_data *ch) { assert(Crash_crashsave_checked(ch)); }
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
  for(int i=0;i<MAX_CURRENT_QUESTS;i++)clear_quest(ch,i);
  ch->real_abils.dex = 18;
  ch->real_abils.str=ch->real_abils.con=ch->real_abils.intel=ch->real_abils.wis=ch->real_abils.cha=10;
  ch->aff_abils=ch->real_abils;
  GET_REAL_MAX_HIT(ch)=GET_MAX_HIT(ch)=GET_HIT(ch)=100;
  GET_REAL_MAX_MOVE(ch)=GET_MAX_MOVE(ch)=GET_MOVE(ch)=100;
  GET_REAL_MAX_PSP(ch)=GET_MAX_PSP(ch)=GET_PSP(ch)=100;
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
static void stall(struct char_data *ch, const char *format, ...)
{
  char command[1024];
  va_list ap;
  va_start(ap, format);
  vsnprintf(command, sizeof(command), format, ap);
  va_end(ap);
  output[0] = 0;
  player_shop_command(ch, command);
}
static void test_stalls(struct char_data *seller, struct char_data *buyer,
                        struct char_data *guest, struct char_data *staff)
{
  struct descriptor_data sd = {0}, bd = {0}, gd = {0};
  struct account_data sa = {0}, ba = {0}, ga = {0};
  struct obj_data *obj;
  long first, second;
  int gold, items;
  sa.id = 1; ba.id = 2; ga.id = 1;
  sd.account = &sa; bd.account = &ba; gd.account = &ga;
  seller->desc = &sd; buyer->desc = &bd; guest->desc = &gd;
  char_from_room(seller); char_to_room(seller, 1);
  char_from_room(buyer); char_to_room(buyer, 1);
  char_from_room(guest); char_to_room(guest, 1);
  world[1].name = "Test Market";
  assert(player_shop_init());
  stall(seller, "admin add 2200 3");
  assert(scalar("SELECT COUNT(*) FROM player_shop_stalls") == 0);
  stall(staff, "admin add 2200 3");
  assert(scalar("SELECT COUNT(*) FROM player_shop_stalls") == 3);
  gold = GET_GOLD(seller);
  stall(seller, "rent 1junk");
  assert(GET_GOLD(seller) == gold);
  stall(seller, "rent 1");
  assert(GET_GOLD(seller) == gold - 1000);
  assert(scalar("SELECT COUNT(*) FROM player_shop_leases") == 1);
  stall(guest, "rent 2"); /* Same account, different character. */
  assert(scalar("SELECT COUNT(*) FROM player_shop_leases") == 1);
  stall(seller, "name Smith's Emporium");
  assert(strstr(output, "recorded"));
  player_shop_show_room(buyer);
  assert(strstr(output, "Smith's Emporium"));
  obj = item("stallblade");
#ifdef ESTATE_REAL_CODEC
  obj->religion_requirement=(struct religion_requirement){1,4};
#endif
  obj->affected[0].location = APPLY_FEAT;
  obj->affected[0].modifier = FEAT_WEAPON_FOCUS;
  obj->affected[0].specific = 3;
  obj_to_char(obj, seller);
  stall(seller, "sell stallblade 0");
  assert(scalar("SELECT COUNT(*) FROM player_shop_listings") == 0);
  SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_QUEST);
  stall(seller, "sell stallblade 25000");
  assert(scalar("SELECT COUNT(*) FROM player_shop_listings") == 0);
  REMOVE_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_QUEST);
  GET_OBJ_BOUND_ID(obj) = 999;
  stall(seller, "sell stallblade 25000");
  assert(scalar("SELECT COUNT(*) FROM player_shop_listings") == 0);
  GET_OBJ_BOUND_ID(obj) = NOBODY;
  stall(staff, "admin set 1 1000 1");
  stall(seller, "sell stallblade 25000");
  assert(strstr(output, "Listing #1 prepared"));
  assert(scalar("SELECT COUNT(*) FROM player_shop_listings") == 1);
  first = scalar("SELECT MAX(listing_id) FROM player_shop_listings");
  obj = item("spareblade"); obj_to_char(obj, seller);
  stall(seller, "sell spareblade 100");
  assert(scalar("SELECT COUNT(*) FROM player_shop_listings") == 1);
  stall(buyer, "reprice %ld 10", first);
  assert(scalar("SELECT price FROM player_shop_listings LIMIT 1") == 25000);
  stall(seller, "reprice %ld 18000", first);
  stall(seller, "reprice %ld 18000", first); /* Unchanged price is valid. */
  assert(strstr(output, "recorded"));
  stall(buyer, "search STALLBLADE");
  assert(strstr(output, "stallblade") && strstr(output, "2200"));
  stall(buyer, "inspect %ld", first);
  assert(strstr(output, "IDENTIFY"));
  char_from_room(buyer); char_to_room(buyer, 0);
  items = IS_CARRYING_N(buyer); gold = GET_GOLD(buyer);
  stall(buyer, "buy %ld", first);
  assert(IS_CARRYING_N(buyer) == items && GET_GOLD(buyer) == gold);
  char_from_room(buyer); char_to_room(buyer, 1);
  GET_GOLD(buyer) = 1;
  stall(buyer, "buy %ld", first);
  assert(IS_CARRYING_N(buyer) == items);
  GET_GOLD(buyer) = gold;
#ifdef ESTATE_REAL_CODEC
  GET_DEITY(buyer)=0;stall(buyer, "buy %ld", first);
  assert(GET_GOLD(buyer)==gold && IS_CARRYING_N(buyer)==items);
  GET_DEITY(buyer)=2;buyer->player_specials->saved.religion_standing[2]=336;
  stall(buyer, "buy %ld", first);
  assert(GET_GOLD(buyer)==gold && scalar("SELECT COUNT(*) FROM player_shop_sales")==0);
  GET_DEITY(buyer)=1;buyer->player_specials->saved.religion_standing[1]=36;
  stall(buyer, "buy %ld", first);
  assert(GET_GOLD(buyer)==gold && IS_CARRYING_N(buyer)==items);
  assert(scalar("SELECT COUNT(*) FROM player_shop_listings WHERE state='active'")==1);
  buyer->player_specials->saved.religion_standing[1]=72;
#endif
  IS_CARRYING_N(buyer) = 10000;
  stall(buyer, "buy %ld", first);
  assert(GET_GOLD(buyer) == gold);
  IS_CARRYING_N(buyer) = items;
  stall(seller, "buy %ld", first);
  assert(scalar("SELECT COUNT(*) FROM player_shop_sales") == 0);
  /* A competing transaction holding the listing blocks the purchase. It must
   * leave stock, money and proceeds unchanged when the lock times out. */
  {
    MYSQL *other = mysql_init(NULL);
    MYSQL_RES *locked;
    char sql[160];
    assert(mysql_real_connect(other, NULL, "root", NULL, "estate_test", 0,
                              getenv("STALL_TEST_SOCKET"), 0));
    assert(mysql_query(other, "START TRANSACTION") == 0);
    snprintf(sql, sizeof(sql), "SELECT listing_id FROM player_shop_listings WHERE listing_id=%ld FOR UPDATE", first);
    assert(mysql_query(other, sql) == 0);
    locked = mysql_store_result(other);
    assert(locked); mysql_free_result(locked);
    assert(estate_query("SET SESSION innodb_lock_wait_timeout=1"));
    stall(buyer, "buy %ld", first);
    assert(GET_GOLD(buyer) == gold && IS_CARRYING_N(buyer) == items);
    assert(mysql_query(other, "ROLLBACK") == 0);
    mysql_close(other);
  }
  stall(buyer, "buy %ld", first);
  assert(GET_GOLD(buyer) == gold - 18000 && IS_CARRYING_N(buyer) == items + 1);
  assert(buyer->carrying->affected[0].specific == 3);
  assert(buyer->carrying->affected[0].modifier == FEAT_WEAPON_FOCUS);
  stall(guest, "buy %ld", first);
  assert(scalar("SELECT COUNT(*) FROM player_shop_sales") == 1);
  stall(seller, "earnings");
  assert(strstr(output, "18000"));
  gold = GET_GOLD(seller);
  GET_GOLD(seller) = INT_MAX;
  stall(seller, "collect");
  assert(GET_GOLD(seller) == INT_MAX);
  assert(scalar("SELECT COUNT(*) FROM estate_deliveries WHERE delivered=0") == 1);
  GET_GOLD(seller) = gold;
  estate_recover(seller);
  assert(GET_GOLD(seller) == gold + 18000);
  estate_recover(seller);
  stall(seller, "collect");
  assert(GET_GOLD(seller) == gold + 18000);
  stall(seller, "sell spareblade 500");
  second = scalar("SELECT MAX(listing_id) FROM player_shop_listings");
  assert(second > first);
  assert(estate_query("UPDATE player_shop_leases SET expires_at=DATE_SUB(NOW(),INTERVAL 1 DAY) WHERE held_stall=1"));
  stall(buyer, "buy %ld", second);
  assert(scalar("SELECT COUNT(*) FROM player_shop_sales") == 1);
  gold = GET_GOLD(seller);
  stall(seller, "renew");
  assert(GET_GOLD(seller) == gold - 1000);
  assert(scalar("SELECT COUNT(*) FROM player_shop_leases WHERE expires_at>NOW()") == 1);
  assert(estate_query("UPDATE player_shop_leases SET expires_at=DATE_SUB(NOW(),INTERVAL 49 HOUR) WHERE held_stall=1"));
  stall(buyer, "rent 1");
  assert(scalar("SELECT COUNT(*) FROM player_shop_leases") == 2);
  stall(buyer, "browse 1");
  assert(!strstr(output, "spareblade"));
  stall(buyer, "withdraw %ld", second);
  assert(scalar("SELECT COUNT(*) FROM player_shop_listings WHERE state='active'") == 1);
  char_from_room(seller); char_to_room(seller, 0);
  items = IS_CARRYING_N(seller);
  stall(seller, "withdraw %ld", second);
  assert(IS_CARRYING_N(seller) == items + 1);
  stall(staff, "admin disable 1");
  gold = GET_GOLD(buyer);
  stall(buyer, "renew");
  assert(GET_GOLD(buyer) == gold);
  stall(staff, "admin enable 1");
  assert(estate_query("UPDATE player_shop_leases SET expires_at=DATE_SUB(NOW(),INTERVAL 1 DAY) WHERE held_stall=1"));
  stall(buyer, "close");
  assert(scalar("SELECT COUNT(*) FROM player_shop_leases WHERE held_stall IS NOT NULL") == 0);
  ga.id = 3;
  stall(guest, "rent 1");
  stall(staff, "admin evict 1");
  assert(scalar("SELECT COUNT(*) FROM player_shop_leases WHERE held_stall IS NOT NULL") == 0);
  stall(staff, "admin history 1");
  assert(strstr(output, "evict") && strstr(output, "buy"));
  /* Committed currency payment, interrupted before save/acknowledgement. */
  gold = GET_GOLD(seller);
  assert(estate_query("START TRANSACTION"));
  assert(estate_queue_money(GET_IDNUM(seller), 100));
  assert(estate_query("COMMIT"));
  assert(estate_query("CREATE TRIGGER reject_stall_ack BEFORE UPDATE ON estate_deliveries FOR "
                      "EACH ROW BEGIN IF NEW.delivered=1 THEN SIGNAL SQLSTATE '45000' SET "
                      "MESSAGE_TEXT='test money ack failure'; END IF; END"));
  estate_recover(seller);
  assert(GET_GOLD(seller) == gold - 100);
  estate_recover(seller);
  assert(GET_GOLD(seller) == gold - 100);
  assert(estate_query("DROP TRIGGER reject_stall_ack"));
  estate_recover(seller);
  assert(GET_GOLD(seller) == gold - 100);
  assert(scalar("SELECT COUNT(*) FROM estate_deliveries WHERE delivered=0") == 0);
  /* Room deletion retires every slot atomically, preserving stock and proceeds. */
  char_from_room(seller); char_to_room(seller, 1);
  stall(seller, "rent 2");
  obj = item("retirementstock"); obj_to_char(obj, seller);
  stall(seller, "sell retirementstock 50");
  first = scalar("SELECT MAX(listing_id) FROM player_shop_listings");
  stall(buyer, "buy %ld", first);
  obj = item("retirementspare"); obj_to_char(obj, seller);
  stall(seller, "sell retirementspare 75");
  second = scalar("SELECT MAX(listing_id) FROM player_shop_listings");
  stall(seller, "admin delete-room 2200");
  assert(scalar("SELECT COUNT(*) FROM player_shop_stalls WHERE room_vnum=2200") == 3);
  stall(staff, "admin delete-room 2200 extra");
  assert(scalar("SELECT COUNT(*) FROM player_shop_stalls WHERE room_vnum=2200") == 3);
  assert(estate_query("CREATE TRIGGER reject_market_delete BEFORE UPDATE ON player_shop_stalls "
                      "FOR EACH ROW BEGIN IF NEW.room_vnum=-1 THEN SIGNAL SQLSTATE '45000' SET "
                      "MESSAGE_TEXT='test market deletion failure'; END IF; END"));
  stall(staff, "admin delete-room 2200");
  assert(scalar("SELECT COUNT(*) FROM player_shop_stalls WHERE room_vnum=2200") == 3);
  assert(scalar("SELECT COUNT(*) FROM player_shop_leases WHERE held_stall=2") == 1);
  assert(scalar("SELECT COUNT(*) FROM player_shop_events WHERE action='delete-room'") == 0);
  assert(estate_query("DROP TRIGGER reject_market_delete"));
  stall(staff, "admin delete-room 2200");
  assert(strstr(output, "Removed 3 stalls"));
  assert(scalar("SELECT COUNT(*) FROM player_shop_stalls WHERE room_vnum=2200") == 0);
  assert(scalar("SELECT COUNT(*) FROM player_shop_leases WHERE held_stall IS NOT NULL") == 0);
  stall(buyer, "directory"); assert(!strstr(output, "Test Market"));
  output[0] = 0; player_shop_show_room(buyer); assert(!*output);
  stall(staff, "admin enable 2");
  assert(scalar("SELECT enabled FROM player_shop_stalls WHERE stall_id=2") == 0);
  stall(buyer, "rent 2");
  assert(scalar("SELECT COUNT(*) FROM player_shop_leases WHERE held_stall IS NOT NULL") == 0);
  stall(buyer, "buy %ld", second);
  assert(scalar("SELECT COUNT(*) FROM player_shop_listings WHERE state='active'") == 1);
  char_from_room(seller); char_to_room(seller, 0);
  items = IS_CARRYING_N(seller);
  stall(seller, "info"); assert(strstr(output, "retirementspare"));
  stall(seller, "withdraw %ld", second); assert(IS_CARRYING_N(seller) == items + 1);
  gold = GET_GOLD(seller); stall(seller, "collect"); assert(GET_GOLD(seller) == gold + 50);
  stall(staff, "admin history 2"); assert(strstr(output, "delete-room"));
  stall(staff, "admin delete-room 2200"); assert(strstr(output, "no stalls configured"));
  stall(staff, "admin add 2200 1");
  stall(buyer, "directory"); assert(strstr(output, "Test Market") && !strstr(output, "#2:"));
  seller->desc = buyer->desc = guest->desc = NULL;
  puts("PASS: player stalls: account limits, local purchases, typed gear, capacity, rent/grace, "
       "tenant isolation, recovery, proceeds, staff controls and idempotent money journal");
}
static void church_accept(struct char_data *ch,int slot)
{
  char text[80];
  snprintf(text,sizeof(text),"accept %d",slot);religion_tasks_command(ch,text);
  religion_tasks_command(ch,"confirm");
}
static void church_ready(struct char_data *ch)
{
  assert(GET_QUEST(ch,RELIGION_QUEST_SLOT)!=NOTHING);
  GET_QUEST_COUNTER(ch,RELIGION_QUEST_SLOT)=0;
  religion_task_objective_complete(ch);
}
static void test_church_tasks(struct char_data *ch,struct char_data *admin)
{
  FILE *registry=fopen("test-churches.json","w");
  int i;
  char text[80];
  assert(registry);
  fputs("{\"version\":1,\"temples\":[{\"id\":42,\"deity\":1,\"name\":\"East Church\",\"entrance\":2200,\"priest\":5000,\"enabled\":true,\"hospitality\":true},"
        "{\"id\":43,\"deity\":2,\"name\":\"West Church\",\"entrance\":2200,\"priest\":5001,\"enabled\":true,\"hospitality\":true}]}",registry);
  fclose(registry);
  mob_proto=calloc(2,sizeof(*mob_proto));mob_index=calloc(2,sizeof(*mob_index));top_of_mobt=1;
  for(i=0;i<2;i++) {
    mob_index[i].vnum=5000+i;mob_proto[i].nr=i;
    SET_BIT_AR(MOB_FLAGS(&mob_proto[i]),MOB_ISNPC);
    mob_proto[i].mob_deity=i+1;mob_proto[i].mob_temple_rank=8;
    mob_proto[i].player.short_descr=i ? "the western priest" : "the eastern priest";
    GET_POS(&mob_proto[i])=POS_STANDING;char_to_room(&mob_proto[i],1);
  }
  assert(religion_load_temples("test-churches.json"));
  aquest_table=calloc(8,sizeof(*aquest_table));total_quests=8;
  for(i=0;i<8;i++) {
    QST_NUM(i)=700+i;QST_TYPE(i)=AQ_ROOM_FIND;QST_MASTER(i)=i<4 ? 5000 : 5001;
    QST_FLAGS(i)=AQ_REPEATABLE;QST_TARGET(i)=2200;
    QST_PREV(i)=QST_NEXT(i)=QST_DIAGN(i)=QST_PREREQ(i)=NOTHING;
    QST_OBJ(i)=NOTHING;QST_FOLLOWER(i)=NOBODY;QST_RACE(i)=RACE_UNDEFINED;
    QST_TIME(i)=-1;QST_QUANTITY(i)=1;QST_MINLEVEL(i)=1;QST_MAXLEVEL(i)=50;
    QST_NAME(i)="Church pilgrimage";QST_INFO(i)="Visit the shrine and return to this priest.";QST_DESC(i)="Visit the shrine.";
    snprintf(text,sizeof(text),"%d %d",i%4+1,700+i);
    religion_tasks_admin(admin,i<4 ? 42:43,i<4 ? 1:2,i<4 ? 5000:5001,text);
  }
  assert(scalar("SELECT COUNT(*) FROM religion_task_config")==8);
  QST_GOLD(0)=100;religion_tasks_admin(admin,42,1,5000,"4 700");QST_GOLD(0)=0;
  assert(scalar("SELECT quest_vnum FROM religion_task_config WHERE deity=1 AND slot=4")==703);
  religion_tasks_admin(admin,42,1,5000,"2 700"); /* duplicate template rolls back slot replacement */
  assert(scalar("SELECT quest_vnum FROM religion_task_config WHERE deity=1 AND slot=2")==701);

  religion_rank_admin(admin,"1 2 Keeper of Mercy");
  religion_tasks_boot();assert(!strcmp(religion_rank_title(1,2),"Keeper of Mercy"));
  GET_DEITY(ch)=1;GET_POS(ch)=POS_STANDING;
  ch->player_specials->saved.religion_standing[1]=ch->player_specials->saved.religion_standing[2]=0;
  for(i=0;i<NORMAL_CURRENT_QUESTS;i++)GET_QUEST(ch,i)=9000+i;
  church_clock=1700438399; /* Sunday 23:59:59 UTC; Monday boundary is next second. */
  church_accept(ch,1);assert(GET_QUEST(ch,RELIGION_QUEST_SLOT)==700);
  religion_tasks_command(ch,"accept 2");assert(scalar("SELECT COUNT(*) FROM religion_task_active")==1);
  for(i=0;i<NORMAL_CURRENT_QUESTS;i++)assert(GET_QUEST(ch,i)==9000+i);
  church_ready(ch);
  assert(estate_query("CREATE TRIGGER test_church_completion BEFORE INSERT ON religion_task_completions FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='church completion test failure'"));
  religion_tasks_command(ch,"turnin");assert(scalar("SELECT COUNT(*) FROM religion_task_completions")==0);
  assert(ch->player_specials->saved.religion_standing[1]==0);
  assert(estate_query("DROP TRIGGER test_church_completion"));
  GET_DEITY(ch)=2;religion_tasks_command(ch,"turnin");assert(scalar("SELECT COUNT(*) FROM religion_task_completions")==0);
  GET_DEITY(ch)=1;fail_church_saves=1;religion_tasks_command(ch,"turnin");assert(ch->player_specials->saved.religion_standing[1]==2);
  religion_tasks_command(ch,"turnin");assert(scalar("SELECT COUNT(*) FROM religion_task_completions")==1);
  ch->player_specials->saved.religion_standing[1]=0;religion_tasks_restore(ch);
  assert(ch->player_specials->saved.religion_standing[1]==2);
  church_accept(ch,1);assert(scalar("SELECT story_only FROM religion_task_active")==1);church_ready(ch);
  ++church_clock;religion_tasks_command(ch,"turnin");assert(ch->player_specials->saved.religion_standing[1]==2);
  for(i=2;i<=4;i++) {church_accept(ch,i);church_ready(ch);religion_tasks_command(ch,"turnin");}
  assert(ch->player_specials->saved.religion_standing[1]==8);
  assert(scalar("SELECT COUNT(*) FROM religion_task_completions WHERE award>0")==4);
  GET_DEITY(ch)=2;church_accept(ch,1);
  assert(scalar("SELECT story_only FROM religion_task_active")==1);church_ready(ch);
  religion_tasks_command(ch,"turnin");assert(ch->player_specials->saved.religion_standing[2]==0);
  GET_DEITY(ch)=1;church_clock+=604800-1;
  religion_tasks_command(ch,"accept 1");++church_clock;religion_tasks_command(ch,"confirm");
  assert(scalar("SELECT COUNT(*) FROM religion_task_active")==0); /* preview expired at reset */
  church_accept(ch,2);church_clock+=604800;church_ready(ch);religion_tasks_command(ch,"turnin");
  assert(ch->player_specials->saved.religion_standing[1]==10); /* active task crosses weeks */
  church_accept(ch,1);church_ready(ch);religion_tasks_command(ch,"turnin");
  assert(religion_rank(ch)==2 && !strcmp(religion_rank_title(1,2),"Keeper of Mercy"));
  church_accept(ch,3);clear_quest(ch,RELIGION_QUEST_SLOT);religion_tasks_restore(ch);
  assert(GET_QUEST(ch,RELIGION_QUEST_SLOT)==702); /* missing character save recovers enrollment */
  QST_QUANTITY(2)=2;GET_QUEST_COUNTER(ch,RELIGION_QUEST_SLOT)=0;religion_task_objective_complete(ch);
  assert(scalar("SELECT ready FROM religion_task_active")==0);
  religion_tasks_command(ch,"abandon");QST_QUANTITY(2)=1;
  assert(estate_query("UPDATE religion_standing SET standing=334 WHERE player_id=%ld AND deity=1",GET_IDNUM(ch)));
  religion_tasks_restore(ch);church_accept(ch,4);church_ready(ch);religion_tasks_command(ch,"turnin");
  assert(ch->player_specials->saved.religion_standing[1]==336 && religion_rank(ch)==8);
  church_clock+=604800;
  church_accept(ch,3);assert(scalar("SELECT story_only FROM religion_task_active")==1);
  religion_tasks_command(ch,"abandon");
  religion_tasks_command(ch,"accept 1");church_clock+=121;religion_tasks_command(ch,"confirm");
  assert(scalar("SELECT COUNT(*) FROM religion_task_active")==0);
  assert(scalar("SELECT COUNT(*) FROM (SELECT deity,week_id,COUNT(*) n FROM religion_task_offers GROUP BY deity,week_id HAVING n>4) counts")==0);
  /* Config changes do not reroll the persisted week's offerings. */
  religion_tasks_admin(admin,42,1,5000,"1 0");religion_tasks_command(ch,"list");
  assert(scalar("SELECT COUNT(*) FROM religion_task_offers WHERE temple_id=42 AND quest_vnum=700 AND week_id=(SELECT MAX(week_id) FROM religion_task_weeks WHERE deity=1)")==1);
  for(i=0;i<MAX_CURRENT_QUESTS;i++)clear_quest(ch,i);
  GET_DEITY(ch)=0;church_clock=0;
  char_from_room(&mob_proto[0]);char_from_room(&mob_proto[1]);
  puts("PASS: church board confirmations, dedicated slot, atomic Standing, weekly caps across faiths, story-only rollover, template edits, reload and rank cap");
}

static void test_church_blessing(struct char_data *ch)
{
 static const int thresholds[]={0,0,12,36,72,120,180,252,336};
 static const int expected[9][9]={{0},{1,0,0,0,0,0,0,0,0},{1,1,0,0,0,0,0,0,0},
 {2,1,15,0,0,0,0,0,0},{2,2,20,5,5,0,0,0,0},{3,2,25,5,5,1,0,0,0},
 {3,3,30,7,10,1,1,0,0},{4,3,35,7,10,2,2,50,0},{4,4,40,10,15,2,3,75,2}};
 int rank,i;time_t expires;
 char_to_room(&mob_proto[0],1);char_to_room(&mob_proto[1],1);
 church_clock=1800000000;GET_DEITY(ch)=1;GET_POS(ch)=POS_STANDING;
 assert(estate_query("DELETE FROM religion_blessings"));
 for(rank=1;rank<=8;rank++) {
  ch->player_specials->saved.religion_standing[1]=thresholds[rank];
  assert(estate_query("REPLACE INTO religion_blessings VALUES(%ld,1,%d,%lld,%lld)",GET_IDNUM(ch),rank,(long long)church_clock+86400,(long long)church_clock+86400));
  religion_blessing_restore(ch);
  for(i=0;i<9;i++)assert(religion_blessing_value(ch,i)==expected[rank][i]);
  ch->legendary_serializing=true;
  for(i=0;i<9;i++)assert(!religion_blessing_value(ch,i));
  religion_blessing_tick(ch); /* Serialization must not remove the durable package. */
  ch->legendary_serializing=false;
  assert(religion_blessing_value(ch,CHURCH_SAVE)==expected[rank][CHURCH_SAVE]);
 }
 expires=ch->player_specials->saved.church_expires;
 ch->player_specials->saved.religion_standing[1]=0;religion_blessing_tick(ch);
 assert(ch->player_specials->saved.church_rank==1 && ch->player_specials->saved.church_expires==expires);
 ch->player_specials->saved.religion_standing[1]=336;religion_blessing_tick(ch);
 assert(religion_blessing_value(ch,CHURCH_HP)==0); /* No automatic upgrade. */
 religion_blessing_restore(ch);assert(ch->player_specials->saved.church_rank==1);
 assert(religion_blessing_remove(ch));religion_blessing_restore(ch);
 assert(!religion_blessing_value(ch,CHURCH_SAVE) && ch->player_specials->saved.church_available==expires);
 religion_blessing_command(ch,"bless","");religion_blessing_command(ch,"bless","confirm");
 assert(!religion_blessing_value(ch,CHURCH_SAVE)); /* Removal retains cooldown. */
 church_clock=expires;religion_blessing_command(ch,"bless","");
 assert(ch->player_specials->church_preview_until==church_clock+120);
 religion_blessing_command(ch,"bless","confirm");assert(religion_blessing_value(ch,CHURCH_HP)==40);
 expires=ch->player_specials->saved.church_expires;
 religion_blessing_command(ch,"bless","confirm");assert(ch->player_specials->saved.church_expires==expires);
 GET_DEITY(ch)=2;religion_blessing_tick(ch);GET_DEITY(ch)=1;religion_blessing_restore(ch);
 assert(!religion_blessing_value(ch,CHURCH_SAVE));
 assert(ch->player_specials->saved.church_available==expires);
 church_clock=expires;religion_blessing_command(ch,"bless","");
 assert(estate_query("CREATE TRIGGER reject_blessing BEFORE UPDATE ON religion_blessings FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='test blessing failure'"));
 religion_blessing_command(ch,"bless","confirm");assert(!religion_blessing_value(ch,CHURCH_SAVE));
 assert(ch->player_specials->saved.church_available==expires);
 assert(estate_query("DROP TRIGGER reject_blessing"));
 religion_blessing_command(ch,"bless","");religion_blessing_command(ch,"bless","confirm");
 expires=ch->player_specials->saved.church_expires;
 church_clock=expires+1;religion_blessing_restore(ch);assert(!religion_blessing_value(ch,CHURCH_SAVE));
 puts("PASS: exact blessing rows, immutable expiry, correction trimming, no rank-up upgrade, shared cooldown, faith removal, relog expiry and failed application");
}
#ifdef ESTATE_REAL_CODEC
static void edit_input(struct char_data *ch,const char *s){preserve_argument_separator=true;assert(restring_editor_input(ch,s));preserve_argument_separator=false;}
static void edit_field(struct char_data *ch,int n,const char *s){char a[8];snprintf(a,sizeof(a),"%d",n);edit_input(ch,a);edit_input(ch,s);if(n==8)edit_input(ch,"@");}
static void test_restrings(struct char_data *ch,struct char_data *admin)
{
 struct obj_data *obj,*loaded;char *packed;int before,initial_gold=GET_GOLD(ch);
 extern int restring_test_stop_after_commit;
 struct descriptor_data cd={0},ad={0};
 ch->desc=&cd;admin->desc=&ad;cd.character=ch;ad.character=admin;
 assert(estate_query("DELETE FROM restring_catalogue"));
 GET_DEITY(ch)=1;GET_POS(ch)=POS_STANDING;ch->player_specials->saved.religion_standing[1]=0;
 do_restringedit(admin,"",0,0);edit_input(admin,"2");edit_input(admin,"1");edit_input(admin,"a");
 edit_field(admin,1,"1");edit_field(admin,3,"neck");edit_field(admin,4,"symbol   0  ");
 {
  struct json_object *entry=NULL,*value=NULL;
  assert(json_object_object_get_ex(admin->player_specials->restring_editor,"entry",&entry));
  assert(json_object_object_get_ex(entry,"value",&value)&&json_object_get_int(value)==0);
  edit_input(admin,"4");edit_input(admin,"symbol garbage");
  assert(json_object_object_get_ex(entry,"value",&value)&&json_object_get_int(value)==0);
  edit_input(admin,"symbol 0");
 }
 edit_field(admin,3,"body");edit_field(admin,4,"armor 0");
 edit_field(admin,5,"robe vestment");edit_field(admin,6,"a blue service robe");
 edit_field(admin,7,"A blue service robe lies here.");edit_field(admin,8,"Fine blue stitching surrounds a plain emblem.");
 edit_input(admin,"s");assert(scalar("SELECT COUNT(*) FROM restring_catalogue WHERE draft IS NOT NULL AND published IS NULL")==1);
 edit_input(admin,"p");assert(scalar("SELECT COUNT(*) FROM restring_catalogue WHERE published IS NOT NULL AND draft IS NULL")==1);
 /* A draft edit keeps the published revision and player text intact. */
 edit_field(admin,6,"an unfinished draft robe");edit_input(admin,"s");
 assert(scalar("SELECT COUNT(*) FROM restring_catalogue WHERE JSON_UNQUOTE(JSON_EXTRACT(published,'$.short'))='a blue service robe'")==1);
 edit_input(admin,"b");edit_input(admin,"yes");edit_input(admin,"q");
 obj=item("robe");obj->id=80001;GET_OBJ_TYPE(obj)=ITEM_WORN;GET_OBJ_LEVEL(obj)=5;
 SET_BIT_AR(GET_OBJ_WEAR(obj),ITEM_WEAR_BODY);obj_to_char(obj,ch);GET_GOLD(ch)=5000;
 obj->ex_description=calloc(1,sizeof(*obj->ex_description));obj->ex_description->keyword=strdup("secret");obj->ex_description->description=strdup("Old secret text.");
 before=GET_GOLD(ch);do_restringapply(ch,"robe religion 2",0,0);assert(!ch->player_specials->restring_quote);
 do_restringapply(ch,"robe religion 1",0,0);assert(ch->player_specials->restring_quote);
 do_restringapply(ch,"confirm",0,0);assert(GET_GOLD(ch)==before-500);
 assert(!strcmp(obj->short_description,"a blue service robe") && !obj->ex_description && obj->restring_extras_cleared);
 assert(obj->religion_requirement.deity==1 && obj->religion_requirement.rank==1 && GET_OBJ_LEVEL(obj)==5 && obj->id==80001);
 assert(!strcmp(obj->restring_inspection,"Fine blue stitching surrounds a plain emblem.\r\n"));
 do_restringapply(ch,"confirm",0,0);assert(GET_GOLD(ch)==before-500);
 packed=estate_pack(obj);assert(packed && strstr(packed,"REmp: 1") && strstr(packed,"RIns:"));loaded=estate_unpack(packed);free(packed);
 assert(loaded && loaded->restring_extras_cleared && !loaded->ex_description && !strcmp(loaded->restring_inspection,obj->restring_inspection));extract_obj(loaded);
 before=GET_GOLD(ch);do_restringapply(ch,"robe religion 1",0,0);do_restringapply(ch,"confirm",0,0);assert(GET_GOLD(ch)==before); /* No-op. */
 /* An object edit invalidates a quote, as does a new published revision. */
 do_restringapply(ch,"robe religion 1",0,0);GET_OBJ_LEVEL(obj)=6;do_restringapply(ch,"confirm",0,0);assert(GET_GOLD(ch)==before);GET_OBJ_LEVEL(obj)=5;
 do_restringapply(ch,"robe religion 1",0,0);assert(estate_query("UPDATE restring_catalogue SET revision=revision+1"));do_restringapply(ch,"confirm",0,0);assert(GET_GOLD(ch)==before);
 do_restringedit(admin,"",0,0);edit_input(admin,"2");edit_input(admin,"1");
 {char line[64];snprintf(line,sizeof(line),"e %ld",scalar("SELECT id FROM restring_catalogue LIMIT 1"));edit_input(admin,line);}
 edit_field(admin,6,"a newly embroidered robe");edit_input(admin,"p");edit_input(admin,"q");
 do_restringapply(ch,"robe religion 1",0,0);
 assert(estate_query("CREATE TRIGGER reject_restring BEFORE INSERT ON estate_deliveries FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='test appearance failure'"));
 do_restringapply(ch,"confirm",0,0);assert(GET_GOLD(ch)==before && !strcmp(obj->short_description,"a blue service robe"));
 assert(estate_query("DROP TRIGGER reject_restring"));estate_recover(ch);ch->player_specials->restring_pending=false;
 do_restringapply(ch,"robe religion 1",0,0);do_restringapply(ch,"confirm",0,0);assert(GET_GOLD(ch)==before-500 && !strcmp(obj->short_description,"a newly embroidered robe"));
 estate_recover(ch);assert(GET_GOLD(ch)==before-500);
 /* A crash after COMMIT reloads the old escrow-tagged save, removes it,
  * delivers the committed appearance and applies its fee exactly once. */
 packed=estate_pack(obj);loaded=estate_unpack(packed);free(packed);assert(loaded);
 before=GET_GOLD(ch);
 assert(estate_query("UPDATE restring_catalogue SET revision=revision+1,published=JSON_SET(published,'$.short','a crash-recovered robe')"));
 do_restringapply(ch,"robe religion 1",0,0);restring_test_stop_after_commit=1;
 do_restringapply(ch,"confirm",0,0);restring_test_stop_after_commit=0;
 assert(ch->player_specials->restring_pending && GET_GOLD(ch)==before);
 loaded->estate_delivery_id=(unsigned long long)scalar("SELECT MAX(escrow_id) FROM estate_outgoing") | (1ULL<<63);
 extract_obj(obj);obj_to_char(loaded,ch);estate_recover(ch);
 obj=NULL;for(loaded=ch->carrying;loaded;loaded=loaded->next_content)if(!strcmp(loaded->short_description,"a crash-recovered robe"))obj=loaded;
 assert(obj && GET_GOLD(ch)==before-500 && !obj->ex_description && obj->restring_extras_cleared);
 estate_recover(ch);assert(GET_GOLD(ch)==before-500);ch->player_specials->restring_pending=false;
 /* Long multiline inspection text uses bounded codec records. */
 memset(obj->restring_inspection,'x',sizeof(obj->restring_inspection)-1);obj->restring_inspection[sizeof(obj->restring_inspection)-1]=0;
 obj->restring_inspection[500]='\n';packed=estate_pack(obj);loaded=estate_unpack(packed);free(packed);
 assert(loaded && !strcmp(loaded->restring_inspection,obj->restring_inspection));extract_obj(loaded);
 /* Shared clan application uses personal Favor, preserving the existing
  * officer gate and the previously stamped religion requirement. */
 SET_BIT_AR(MOB_FLAGS(&mob_proto[0]),MOB_QUARTERMASTER);
 clan_list=calloc(1,sizeof(*clan_list));num_of_clans=1;clan_list[0].vnum=1;clan_list[0].clan_name="Test Clan";
 do_restringedit(admin,"",0,0);edit_input(admin,"1");edit_input(admin,"3");edit_input(admin,"a");
 edit_field(admin,1,"1");edit_field(admin,3,"body");edit_field(admin,4,"armor 0");
 edit_field(admin,5,"robe clan");edit_field(admin,6,"a clan service robe");edit_field(admin,7,"A clan service robe lies here.");edit_field(admin,8,"A woven clan emblem.");edit_input(admin,"p");edit_input(admin,"q");
 obj->id=80001;obj->clan_requirement.rank=2;restring_clan_policy=1;restring_clan_favor=2;
 before=GET_GOLD(ch);do_restringapply(ch,"robe clan 3",0,0);assert(!ch->player_specials->restring_quote);
 restring_clan_favor=3;do_restringapply(ch,"robe clan 3",0,0);assert(ch->player_specials->restring_quote);
 restring_clan_member=false;do_restringapply(ch,"confirm",0,0);assert(GET_GOLD(ch)==before);
 restring_clan_member=true;do_restringapply(ch,"robe clan 3",0,0);do_restringapply(ch,"confirm",0,0);
 assert(GET_GOLD(ch)==before-500 && obj->clan_requirement.clan==1 && obj->clan_requirement.favor==3 && obj->clan_requirement.rank==2);
 assert(obj->religion_requirement.deity==1 && obj->religion_requirement.rank==1);
 REMOVE_BIT_AR(MOB_FLAGS(&mob_proto[0]),MOB_QUARTERMASTER);
 restring_clan_policy=0;free(clan_list);clan_list=NULL;num_of_clans=0;
 /* Deletion is confirmed, audited and does not rewrite completed items. */
 do_restringedit(admin,"",0,0);edit_input(admin,"2");edit_input(admin,"1");
 {char line[64];snprintf(line,sizeof(line),"d %ld",scalar("SELECT id FROM restring_catalogue LIMIT 1"));edit_input(admin,line);}edit_input(admin,"yes");edit_input(admin,"q");
 assert(scalar("SELECT COUNT(*) FROM restring_catalogue WHERE deleted=1")==1 && scalar("SELECT COUNT(*) FROM restring_audit WHERE action='delete'")==1);
 assert(!strcmp(obj->short_description,"a clan service robe"));
 restring_cleanup(ch);extract_obj(obj);GET_GOLD(ch)=initial_gold;GET_DEITY(ch)=0;estate_save_player(ch);ch->desc=admin->desc=NULL;
 puts("PASS: shared appearance menus, draft isolation, publication, eligibility, exact ranks, fees, no-op, stale quotes, native codec, extra clearing, failure rollback, replay and deletion audit");
}
#endif

int main(int argc, char **argv)
{
  struct char_data seller, buyer, guest, admin;
  struct obj_data *obj, *chest, *restored;
  char *packed;
  deity_list[1].pantheon=DEITY_PANTHEON_ALL;deity_list[1].name="Test Faith";
  deity_list[2].pantheon=DEITY_PANTHEON_ALL;deity_list[2].name="Other Faith";
  assert(argc == 2);
  assert(setenv("STALL_TEST_SOCKET", argv[1], 1) == 0);
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
  /* Temple chests use the production SQL escrow/delivery path and native codec. */
  temple_edit_allowed = FALSE;
  temple_chest_admin(&admin, "1 4 3");
  assert(scalar("SELECT COUNT(*) FROM temple_chests") == 0);
  temple_edit_allowed = TRUE;
  temple_chest_admin(&admin, "999999999999999999999999 4 3");
  assert(scalar("SELECT COUNT(*) FROM temple_chests") == 0);
  temple_chest_admin(&admin, "1 0 3");
  assert(scalar("SELECT COUNT(*) FROM temple_chests") == 0);
  temple_chest_admin(&admin, "1 4 3");
  assert(scalar("SELECT COUNT(*) FROM temple_chests") == 1);
  chest = world[1].contents;
  assert(chest && GET_OBJ_TYPE(chest) == ITEM_CONTAINER && !CAN_WEAR(chest, ITEM_WEAR_TAKE));
  temple_chests_boot(); assert(world[1].contents == chest && !chest->next_content);
  output[0]=0;assert(housing_contents(&admin,chest));assert(strstr(output,"Only members"));
  assert(chest->religion_requirement.deity==1 && chest->religion_requirement.rank==4);
  obj = item("templeblade");obj_to_char(obj, &seller);
  GET_DEITY((&seller))=2;seller.player_specials->saved.religion_standing[2]=336;
  output[0]=0;assert(housing_contents(&seller,chest));assert(strstr(output,"Only members"));
  assert(housing_put(&seller,obj,chest));assert(obj->carried_by==&seller);
  GET_DEITY((&seller))=1;seller.player_specials->saved.religion_standing[1]=36;
  assert(housing_put(&seller,obj,chest));assert(obj->carried_by==&seller);
  seller.player_specials->saved.religion_standing[1]=72;
  assert(estate_query("CREATE TRIGGER test_temple_insert BEFORE INSERT ON temple_chest_items FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='test temple deposit failure'"));
  assert(housing_put(&seller,obj,chest));assert(obj->carried_by==&seller);
  assert(scalar("SELECT COUNT(*) FROM temple_chest_items")==0);
  assert(estate_query("DROP TRIGGER test_temple_insert"));
  assert(housing_put(&seller,obj,chest));
  assert(scalar("SELECT COUNT(*) FROM temple_chest_items")==1);
  obj=item("templebox");obj_to_obj(item("templegem"),obj);obj_to_char(obj,&seller);
  assert(housing_put(&seller,obj,chest));
  assert(scalar("SELECT SUM(item_units) FROM temple_chest_items")==3);
  obj=item("excess");obj_to_char(obj,&seller);
  assert(housing_put(&seller,obj,chest));assert(obj->carried_by==&seller);
  assert(scalar("SELECT COUNT(*) FROM temple_chest_items")==2);
  temple_chest_admin(&admin,"1 4 2");
  assert(scalar("SELECT item_limit FROM temple_chests")==3);
  extract_obj(chest);temple_chests_boot();chest=world[1].contents;
  assert(chest && scalar("SELECT COUNT(*) FROM temple_chest_items")==2);
  output[0]=0;assert(housing_contents(&seller,chest));assert(strstr(output,"templeblade"));
  seller.player_specials->saved.religion_standing[1]=36;
  assert(housing_get(&seller,chest,"all",1));
  assert(scalar("SELECT COUNT(*) FROM temple_chest_items")==2);
  GET_DEITY((&seller))=2;
  assert(housing_get(&seller,chest,"all",1));
  assert(scalar("SELECT COUNT(*) FROM temple_chest_items")==2);
  GET_DEITY((&seller))=1;seller.player_specials->saved.religion_standing[1]=336;
  assert(estate_query("CREATE TRIGGER test_temple_delete BEFORE DELETE ON temple_chest_items FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='test temple failure'"));
  assert(housing_get(&seller,chest,"templeblade",1));
  assert(scalar("SELECT COUNT(*) FROM temple_chest_items")==2);
  assert(estate_query("DROP TRIGGER test_temple_delete"));
  assert(housing_get(&seller,chest,"all",1));
  assert(scalar("SELECT COUNT(*) FROM temple_chest_items")==0);
  assert(scalar("SELECT COUNT(*) FROM estate_deliveries WHERE delivered=0")==0);
  while(seller.carrying)extract_obj(seller.carrying);
  estate_save_player(&seller);
  assert(estate_query("DELETE FROM player_save_objs WHERE name='seller'"));
  extract_obj(chest);
  GET_DEITY((&seller))=0;
  /* Both hospitality gifts and the cooldown commit together, including partial
   * delivery failure, stale character files and idempotent delivery recovery. */
  {
    struct obj_data *meal=item("templemeal"), *drink=item("templedrink");
    GET_OBJ_TYPE(meal)=ITEM_FOOD;GET_OBJ_TYPE(drink)=ITEM_DRINK;
    GET_OBJ_VAL(meal,0)=GET_OBJ_VAL(drink,0)=100;
    meal->affected[0].location=APPLY_STR;meal->affected[0].modifier=2;
    meal->affected[0].bonus_type=BONUS_TYPE_FOOD;
    drink->affected[0].location=APPLY_CHA;drink->affected[0].modifier=2;
    drink->affected[0].bonus_type=BONUS_TYPE_DRINK;
    assert(estate_query("CREATE TRIGGER test_hospitality_delivery BEFORE INSERT ON estate_deliveries "
      "FOR EACH ROW BEGIN IF NEW.player_id=3 AND (SELECT COUNT(*) FROM estate_deliveries WHERE player_id=3)>0 "
      "THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='test second hospitality gift failure'; END IF; END"));
    assert(!estate_hospitality_claim(&guest,meal,drink,1700000000,86400));
    assert(scalar("SELECT COUNT(*) FROM estate_deliveries WHERE player_id=3")==0);
    assert(scalar("SELECT available_after FROM religion_hospitality WHERE player_id=3")==0);
    assert(!guest.player_specials->saved.religion_hospitality_after);
    assert(estate_query("DROP TRIGGER test_hospitality_delivery"));
    assert(estate_hospitality_claim(&guest,meal,drink,1700000000,86400));
    assert(scalar("SELECT COUNT(*) FROM estate_deliveries WHERE player_id=3")==2);
    assert(guest.player_specials->saved.religion_hospitality_after==1700086400);
    guest.player_specials->saved.religion_hospitality_after=0;
    assert(!estate_hospitality_claim(&guest,meal,drink,1700000001,86400));
    assert(guest.player_specials->saved.religion_hospitality_after==1700086400);
    assert(scalar("SELECT COUNT(*) FROM estate_deliveries WHERE player_id=3")==2);
    assert(IS_CARRYING_N(&guest)==2);estate_recover(&guest);assert(IS_CARRYING_N(&guest)==2);
#ifdef ESTATE_REAL_CODEC
    assert(guest.carrying->affected[0].bonus_type==BONUS_TYPE_DRINK);
    assert(guest.carrying->next_content->affected[0].bonus_type==BONUS_TYPE_FOOD);
    assert(GET_OBJ_VAL(guest.carrying,0)==100);
#endif
    assert(estate_hospitality_claim(&guest,meal,drink,1700086400,86400));
    estate_recover(&guest);assert(IS_CARRYING_N(&guest)==4);
    extract_obj(meal);extract_obj(drink);
    while(guest.carrying)extract_obj(guest.carrying);
    estate_save_player(&guest);
  }
  test_church_tasks(&seller,&admin);
  test_church_blessing(&seller);
#ifdef ESTATE_REAL_CODEC
  test_restrings(&seller,&admin);
#endif
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
  strcpy(obj->clan_delivery_uid,"dddddddddddddddddddddddddddddddd");
  obj->clan_delivery_amount=25;
  strcpy(obj->clan_qm_uid,"cccccccccccccccccccccccccccccccc");
  obj->clan_requirement = (struct clan_requirement){7, 3, 4};
  obj->religion_requirement = (struct religion_requirement){1, 4};
  strcpy(obj->clan_enchantment.uid,"eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee");
  strcpy(obj->clan_enchantment.receipt,"ffffffffffffffffffffffffffffffff");
  strcpy(obj->clan_enchantment.word,"inferno");
  obj->clan_enchantment.effect=9; obj->clan_enchantment.active=1;
  obj->clan_enchantment.reroll_week=2200000000;
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
    assert(!strcmp(restored->clan_delivery_uid,"dddddddddddddddddddddddddddddddd") && restored->clan_delivery_amount==25);
    assert(!strcmp(restored->clan_qm_uid,"cccccccccccccccccccccccccccccccc"));
    assert(restored->religion_requirement.deity==1 && restored->religion_requirement.rank==4);
    assert(restored->clan_requirement.clan==7 && restored->clan_requirement.rank==3 && restored->clan_requirement.favor==4);
    assert(!strcmp(restored->clan_enchantment.uid,"eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee") &&
           !strcmp(restored->clan_enchantment.receipt,"ffffffffffffffffffffffffffffffff") &&
           !strcmp(restored->clan_enchantment.word,"inferno") && restored->clan_enchantment.effect==9 &&
           restored->clan_enchantment.active==1 && restored->clan_enchantment.reroll_week==2200000000);
    assert(restored->estate_delivery_id == 123456 && restored->affected[0].specific == 7);
    assert(GET_OBJ_VAL(restored, 15) == 42 && GET_OBJ_BOUND_ID(restored) == (int)NOBODY);
    assert(restored->special_abilities && restored->special_abilities->next &&
           restored->special_abilities->ability == 11 &&
           restored->special_abilities->next->ability == 22);
    assert(GET_OBJ_PROF(restored) == 9 && GET_OBJ_TIMER(restored) == 17);
    {
      obj_save_data *database = objsave_parse_objects_db("seller", NOWHERE);
      assert(database && !database->next);
      assert(database->obj->religion_requirement.deity==1 && database->obj->religion_requirement.rank==4);
      assert(!strcmp(database->obj->clan_delivery_uid,"dddddddddddddddddddddddddddddddd") && database->obj->clan_delivery_amount==25);
      assert(!strcmp(database->obj->clan_qm_uid,"cccccccccccccccccccccccccccccccc"));
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
  {
    char command[128];
    long gold = GET_GOLD(&seller);
    house(&seller, "name");
    assert(strstr(output, "Usage:"));
    house(&seller, "name    ");
    assert(strstr(output, "Usage:"));
    assert(scalar("SELECT COUNT(*) FROM housing_houses WHERE name='Test Home'") == 1);
    strcpy(command, "name ");
    memset(command + 5, 'x', 100);
    command[105] = '\0';
    house(&seller, command);
    assert(scalar("SELECT LENGTH(name) FROM housing_houses WHERE house_id=1") == 100);
    command[105] = 'x';
    command[106] = '\0';
    house(&seller, command);
    assert(strstr(output, "Usage:"));
    assert(scalar("SELECT LENGTH(name) FROM housing_houses WHERE house_id=1") == 100);
    house(&seller, "rename Rose's Retreat");
    assert(strstr(output, "now named Rose's Retreat"));
    assert(scalar("SELECT COUNT(*) FROM housing_houses WHERE name='Rose''s Retreat'") == 1);
    house(&seller, "info");
    assert(strstr(output, "Rose's Retreat"));
    assert(!strcmp(world[IN_ROOM(&seller)].name, "An Unfinished Room"));
    assert(GET_GOLD(&seller) == gold);
  }
  {
    const char *commands[]={"sector Inside","sector City","sector Field","sector Forest","sector Water (Swim)","sector Underwater","sector Cave"};
    int sectors[]={SECT_INSIDE,SECT_CITY,SECT_FIELD,SECT_FOREST,SECT_WATER_SWIM,SECT_UNDERWATER,SECT_CAVE};
    int index;
    for(index=0;index<7;index++) {
      house(&seller,commands[index]);assert(world[IN_ROOM(&seller)].sector_type==sectors[index]);
      assert(!!ROOM_FLAGGED(IN_ROOM(&seller),ROOM_INDOORS)==(index==0||index>=5));
      assert(scalar("SELECT sector_type FROM housing_room_settings WHERE room_id=1")==sectors[index]);
      assert(ROOM_FLAGGED(IN_ROOM(&seller),ROOM_HOUSE));
    }
    house(&seller,"sector lava");assert(world[IN_ROOM(&seller)].sector_type==SECT_CAVE);
    house(&seller,"sector Forest");
    world[IN_ROOM(&seller)].sector_type=SECT_INSIDE;SET_BIT_AR(world[IN_ROOM(&seller)].room_flags,ROOM_INDOORS);
    house(&seller,"leave");house(&seller,"enter 1");
    assert(world[IN_ROOM(&seller)].sector_type==SECT_FOREST&&!ROOM_FLAGGED(IN_ROOM(&seller),ROOM_INDOORS));
  }
  house(&seller, "build north");
  assert(scalar("SELECT COUNT(*) FROM housing_rooms") == 2);
  assert(scalar("SELECT COUNT(*) FROM housing_room_exits") == 2);
  house(&seller, "build north");
  assert(scalar("SELECT COUNT(*) FROM housing_rooms") == 2);
  {
    room_rnum origin=IN_ROOM(&seller),north=world[origin].dir_option[NORTH]->to_room,east,northeast;
    long gold;
    char_from_room(&seller);char_to_room(&seller,north);house(&seller,"build east");
    northeast=world[north].dir_option[EAST]->to_room;
    char_from_room(&seller);char_to_room(&seller,origin);house(&seller,"build east");
    east=world[origin].dir_option[EAST]->to_room;
    char_from_room(&seller);char_to_room(&seller,east);
    world[northeast].dir_option[SOUTH]=calloc(1,sizeof(struct room_direction_data));
    world[northeast].dir_option[SOUTH]->to_room=east;
    world[northeast].dir_option[SOUTH]->key=777;
    world[northeast].dir_option[SOUTH]->exit_info=EX_ISDOOR;
    gold=GET_GOLD(&seller);GET_GOLD(&seller)=0;house(&seller,"build north");
    assert(world[northeast].dir_option[SOUTH]->key==777&&world[northeast].dir_option[SOUTH]->exit_info==EX_ISDOOR);
    assert(world[east].dir_option[NORTH]->to_room==northeast);
    assert(world[northeast].dir_option[SOUTH]->to_room==east);
    assert(scalar("SELECT COUNT(*) FROM housing_rooms")==4&&GET_GOLD(&seller)==0);
    GET_GOLD(&seller)=gold;
    char_from_room(&seller);char_to_room(&seller,origin);house(&seller,"build northeast");
    assert(world[origin].dir_option[NORTHEAST]->to_room==northeast);
    assert(world[northeast].dir_option[SOUTHWEST]->to_room==origin);
    assert(scalar("SELECT COUNT(*) FROM housing_rooms")==4&&GET_GOLD(&seller)==gold);
    house(&seller,"build up");
    {
      room_rnum upstairs=world[origin].dir_option[UP]->to_room,above_east;
      char_from_room(&seller);char_to_room(&seller,upstairs);house(&seller,"build east");
      above_east=world[upstairs].dir_option[EAST]->to_room;
      char_from_room(&seller);char_to_room(&seller,above_east);gold=GET_GOLD(&seller);
      house(&seller,"build down");
      assert(world[above_east].dir_option[DOWN]->to_room==east&&world[east].dir_option[UP]->to_room==above_east);
      assert(scalar("SELECT COUNT(*) FROM housing_rooms")==6&&GET_GOLD(&seller)==gold);
    }
    house(&seller,"leave");house(&seller,"enter 1");
    assert(world[IN_ROOM(&seller)].dir_option[NORTHEAST]->to_room==northeast);
  }
  house(&seller, "title A Fine Hall");
  assert(!strcmp(world[IN_ROOM(&seller)].name, "A Fine Hall"));
  {
    room_rnum entrance=IN_ROOM(&seller),untouched=world[entrance].dir_option[EAST]->to_room;
    long gold=GET_GOLD(&seller),normal=scalar("SELECT description_price FROM housing_neighborhoods WHERE neighborhood_id=1");
    long revision=normal/10+(normal%10!=0);
    house(&seller,"description My custom hall.");assert(GET_GOLD(&seller)==gold-normal);
    assert(scalar("SELECT COUNT(*) FROM housing_room_description_edits WHERE room_id=1")==1);
    gold=GET_GOLD(&seller);house(&seller,"description My corrected hall.");assert(GET_GOLD(&seller)==gold-revision);
    house(&seller,"leave");house(&seller,"enter 1");gold=GET_GOLD(&seller);
    house(&seller,"description This room awaits your imagination.");
    house(&seller,"description My custom hall.");assert(GET_GOLD(&seller)==gold-2*revision);
    // A new room still pays the full initial customization price.
    char_from_room(&seller);char_to_room(&seller,untouched);gold=GET_GOLD(&seller);
    house(&seller,"description An eastern chamber.");assert(GET_GOLD(&seller)==gold-normal);
    // Old customized rooms qualify even without the new persistent marker.
    {
      room_rnum legacy=world[entrance].dir_option[NORTH]->to_room;
      assert(estate_query("UPDATE housing_rooms SET description='A legacy custom room.' WHERE room_id=2"));
      char_from_room(&seller);char_to_room(&seller,legacy);gold=GET_GOLD(&seller);
      house(&seller,"description A corrected legacy room.");assert(GET_GOLD(&seller)==gold-revision);
    }
    house(&seller,"leave");house(&seller,"enter 1");gold=GET_GOLD(&seller);
    house(&seller,"description");assert(GET_GOLD(&seller)==gold);
    GET_GOLD(&seller)=revision-1;house(&seller,"description Cannot afford this.");
    assert(GET_GOLD(&seller)==revision-1&&!strcmp(world[IN_ROOM(&seller)].description,"My custom hall.\r\n"));GET_GOLD(&seller)=gold;
    // A failed marker write rolls back the text update and never charges gold.
    assert(estate_query("CREATE TRIGGER test_description_save BEFORE INSERT ON housing_room_description_edits FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='test description save failure'"));
    house(&seller,"description This must roll back.");
    assert(GET_GOLD(&seller)==gold&&!strcmp(world[IN_ROOM(&seller)].description,"My custom hall.\r\n"));
    assert(scalar("SELECT COUNT(*) FROM housing_rooms WHERE room_id=1 AND description='My custom hall.'")==1);
    assert(estate_query("DROP TRIGGER test_description_save"));
  }
  house(&seller, "extra painting A bright painting.");
  house(&seller, "guest guest");
  house(&guest, "enter 1");
  house(&guest,"sector Underwater");assert(strstr(output,"Only the owner"));
  assert(world[IN_ROOM(&guest)].sector_type==SECT_FOREST);
  assert(IN_ROOM(&guest) == IN_ROOM(&seller));
  house(&guest, "name Stolen Home");
  assert(strstr(output, "Only the owner"));
  assert(scalar("SELECT COUNT(*) FROM housing_houses WHERE name='Rose''s Retreat'") == 1);
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
  house(&seller, "name Outside Home");
  assert(strstr(output, "inside a neighborhood house"));
  house(&seller, "list");
  assert(strstr(output, "Rose's Retreat"));
  house(&seller, "enter 1");
  house(&seller, "info");
  assert(strstr(output, "Rose's Retreat"));
  assert(!strcmp(world[IN_ROOM(&seller)].name, "A Fine Hall"));
  assert(world[IN_ROOM(&seller)].ex_description);
  house(&seller, "leave");
#ifdef ESTATE_REAL_CODEC
  seller.carrying->contains->religion_requirement=(struct religion_requirement){1,4};
#endif
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
#ifdef ESTATE_REAL_CODEC
  auction(&buyer,"buy 1");assert(!buyer.carrying && GET_GOLD(&buyer)==500000);
  GET_DEITY((&buyer))=2;buyer.player_specials->saved.religion_standing[2]=336;
  auction(&buyer,"buy 1");assert(!buyer.carrying && GET_GOLD(&buyer)==500000);
  GET_DEITY((&buyer))=1;buyer.player_specials->saved.religion_standing[1]=36;
  auction(&buyer,"buy 1");assert(!buyer.carrying && GET_GOLD(&buyer)==500000);
  assert(scalar("SELECT COUNT(*) FROM auction_listings WHERE listing_state='active'")==1);
  buyer.player_specials->saved.religion_standing[1]=72;
#endif
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
  puts("PASS: hospitality atomic rewards/cooldown/recovery, temple chest access/capacity/reload/failure recovery, neighborhood purchase/build/guests/reload, chest nesting/sheaths, auction "
       "escrow/search/expiry/cancel/reprice, payouts and delivery recovery");
  test_stalls(&seller, &buyer, &guest, &admin);
  mysql_close(conn);
  return 0;
}

/* Clan access is exercised against real SQL by run_clan_qm_services.py. */
bool clan_access_check(struct char_data *ch,const struct clan_requirement *r,bool explain)
{ (void)ch;(void)explain;return !restring_clan_policy || ((!r->clan || (restring_clan_member && r->clan==1)) && (!r->favor || r->favor<=restring_clan_favor)); }

/* Native surrounding adapters for the shared appearance service. */
void free_ex_descriptions(struct extra_descr_data *e)
{ while(e){struct extra_descr_data *next=e->next;free(e->keyword);free(e->description);free(e);e=next;} }

struct weapon_table weapon_list[NUM_WEAPON_TYPES];
struct armor_table armor_list[NUM_SPEC_ARMOR_TYPES];
const char *weapon_family[NUM_WEAPON_FAMILIES+1];
const char *armor_type[NUM_SPEC_ARMOR_TYPES];
struct clan_data *clan_list;
int num_of_clans;
clan_rnum real_clan(clan_vnum v) { return v==1?0:NO_CLAN; }
bool can_edit_clan(struct char_data *ch,clan_vnum v) { (void)ch;return v==1; }
bool has_quartermaster_in_room(struct char_data *ch) { (void)ch;return true; }
struct obj_data *get_obj_in_equip_vis(struct char_data *ch,char *arg,int *number,struct obj_data *equipment[])
{ int i;(void)number;for(i=0;i<NUM_WEARS;i++)if(equipment[i] && isname(arg,equipment[i]->name))return equipment[i];return NULL; }

struct PERF_prof_sect;
void PERF_prof_sect_init(struct PERF_prof_sect **p,const char *id){(void)p;(void)id;}
void PERF_prof_sect_enter(struct PERF_prof_sect *p){(void)p;}
void PERF_prof_sect_exit(struct PERF_prof_sect *p){(void)p;}
