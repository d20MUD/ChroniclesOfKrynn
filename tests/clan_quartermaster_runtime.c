/* Production coordinator with restartable independent player/inventory stores. */
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "mysql.h"
#include "clan_quartermaster.h"
#include <assert.h>
static MYSQL_POOL_CONN connection;
static bool busy, at_quartermaster = true, restriction;
static struct clan_progression_config policy;
static struct char_data character;
static struct player_special_data specials;
static struct descriptor_data descriptor;
static struct account_data account;
struct player_special_data dummy_mob;
struct index_data *obj_index;
struct obj_data *obj_proto;
obj_rnum top_of_objt;
bool mysql_available = true;
static struct clan_qm_catalog catalog;
static const char *directory;
static int saves, crash_save, fail_save, after_ack;
static const char *expected_uid;
static char messages[65536];
static int64_t clock_value = 2200010001;
time_t qm_test_time(time_t *out)
{
  if (out)
    *out = clock_value;
  return clock_value;
}
#define time qm_test_time
#include "../src/clan_quartermaster_runtime.c"
#undef time
size_t strlcpy(char *out, const char *in, size_t cap)
{
  size_t n = strlen(in);
  if (cap)
  {
    size_t c = n < cap - 1 ? n : cap - 1;
    memcpy(out, in, c);
    out[c] = 0;
  }
  return n;
}
void basic_mud_log(const char *fmt, ...)
{
  (void)fmt;
}
void PERF_prof_sect_init(struct PERF_prof_sect **p, const char *s)
{
  (void)p;
  (void)s;
}
void PERF_prof_sect_enter(struct PERF_prof_sect *p)
{
  (void)p;
}
void PERF_prof_sect_exit(struct PERF_prof_sect *p)
{
  (void)p;
}
size_t send_to_char(struct char_data *ch, const char *fmt, ...)
{
  va_list ap;
  int n;
  (void)ch;
  va_start(ap, fmt);
  n = vsnprintf(messages + strlen(messages), sizeof(messages) - strlen(messages), fmt, ap);
  va_end(ap);
  return n < 0 ? 0 : (size_t)n;
}
const struct clan_progression_config *clan_buff_runtime_policy(void)
{
  return &policy;
}
bool clan_persistence_ready(void)
{
  return true;
}
bool clan_persistence_sql_authority(void)
{
  return true;
}
bool has_quartermaster_in_room(struct char_data *ch)
{
  (void)ch;
  return at_quartermaster;
}
MYSQL_POOL_CONN *mysql_pool_acquire(void)
{
  assert(!busy);
  busy = true;
  return &connection;
}
void mysql_pool_release(MYSQL_POOL_CONN *c)
{
  assert(c == &connection && busy);
  busy = false;
  if (after_ack)
  {
    MYSQL_RES *res;
    MYSQL_ROW row;
    char query[256];
    snprintf(
        query, sizeof(query),
        "SELECT COUNT(*) FROM clan_quartermaster_operations WHERE operation_uid='%s' AND state=1",
        expected_uid);
    assert(!mysql_query(c->conn, query));
    res = mysql_store_result(c->conn);
    row = mysql_fetch_row(res);
    int n = atoi(row[0]);
    mysql_free_result(res);
    if (n)
      _exit(89);
  }
}
int can_carry_weight_limit(struct char_data *ch)
{
  (void)ch;
  return 10000;
}
int invalid_align(struct char_data *ch, struct obj_data *o)
{
  (void)ch;
  (void)o;
  return restriction;
}
int invalid_class(struct char_data *ch, struct obj_data *o)
{
  (void)ch;
  (void)o;
  return restriction;
}
int invalid_prof(struct char_data *ch, struct obj_data *o)
{
  (void)ch;
  (void)o;
  return restriction;
}
bool is_class_req_object(struct char_data *ch, struct obj_data *o, bool output)
{
  (void)ch;
  (void)o;
  (void)output;
  return !restriction;
}
bool is_class_anti_object(struct char_data *ch, struct obj_data *o, bool output)
{
  (void)ch;
  (void)o;
  (void)output;
  return restriction;
}
obj_rnum real_object(obj_vnum v)
{
  size_t i;
  for (i = 0; i < catalog.count; i++)
    if (catalog.entries[i].prototype == (int)v)
      return i;
  return NOTHING;
}
struct obj_data *read_object(obj_vnum v, int type)
{
  obj_rnum n = real_object(v);
  struct obj_data *o;
  (void)type;
  if (n == NOTHING)
    return NULL;
  o = calloc(1, sizeof(*o));
  *o = obj_proto[n];
  return o;
}
void obj_to_char(struct obj_data *o, struct char_data *ch)
{
  o->next_content = ch->carrying;
  ch->carrying = o;
  o->carried_by = ch;
}
void extract_obj(struct obj_data *o)
{
  struct obj_data **p = &character.carrying;
  while (*p && *p != o)
    p = &(*p)->next_content;
  if (*p)
    *p = o->next_content;
  free(o);
}
/* Serialization of mock objects is independently covered by production codec
 * tests. Keep the checkpoint harness focused on coordinator state transitions. */
char *estate_pack(struct obj_data *o)
{
  struct json_object *j = json_object_new_object();
  char *s;
  json_object_object_add(j, "prototype", json_object_new_int(GET_OBJ_VNUM(o)));
  json_object_object_add(j, "bound", json_object_new_int(GET_OBJ_BOUND_ID(o)));
  json_object_object_add(j, "uid", json_object_new_string(o->clan_qm_uid));
  s = strdup(json_object_to_json_string(j));
  json_object_put(j);
  return s;
}
struct obj_data *estate_unpack(const char *s)
{
  struct json_object *j = json_tokener_parse(s);
  struct obj_data *o;
  if (!j)
    return NULL;
  o = read_object(num(j, "prototype"), VIRTUAL);
  if (o)
  {
    GET_OBJ_BOUND_ID(o) = num(j, "bound");
    GET_OBJ_COST(o) = 0;
    strlcpy(o->clan_qm_uid, str(j, "uid"), 33);
  }
  json_object_put(j);
  return o;
}
static bool checkpoint(struct char_data *ch, bool inventory)
{
  struct json_object *j = json_object_new_object(), *a;
  char path[512];
  struct obj_data *o;
  assert(!busy);
  saves++;
  if (saves == fail_save)
  {
    json_object_put(j);
    return false;
  }
  if (inventory)
  {
    json_object_object_add(j, "uid", json_object_new_string(PS(ch)->clan_qm_inventory));
    a = json_object_new_array();
    for (o = ch->carrying; o; o = o->next_content)
    {
      char *s = estate_pack(o);
      json_object_array_add(a, json_tokener_parse(s));
      free(s);
    }
    json_object_object_add(j, "items", a);
  }
  else
  {
    json_object_object_add(j, "uid", json_object_new_string(PS(ch)->clan_qm_lock));
    json_object_object_add(j, "paid", json_object_new_boolean(PS(ch)->clan_qm_paid));
    json_object_object_add(j, "gold", json_object_new_int(GET_GOLD(ch)));
  }
  snprintf(path, sizeof(path), "%s/%s.json", directory, inventory ? "inventory" : "player");
  assert(!json_object_to_file(path, j));
  json_object_put(j);
  if (saves == crash_save)
    _exit(88);
  if (fail_save < 0 && saves == -fail_save)
    return false;
  return true;
}
bool save_char_checked(struct char_data *ch, int mode)
{
  (void)mode;
  return checkpoint(ch, false);
}
bool Crash_crashsave_checked(struct char_data *ch)
{
  return checkpoint(ch, true);
}
static void load(void)
{
  char path[512];
  struct json_object *j;
  size_t i;
  snprintf(path, sizeof(path), "%s/player.json", directory);
  j = json_object_from_file(path);
  if (j)
  {
    strlcpy(specials.clan_qm_lock, str(j, "uid"), 33);
    specials.clan_qm_paid = num(j, "paid");
    GET_GOLD((&character)) = num(j, "gold");
    json_object_put(j);
  }
  snprintf(path, sizeof(path), "%s/inventory.json", directory);
  j = json_object_from_file(path);
  if (j)
  {
    strlcpy(specials.clan_qm_inventory, str(j, "uid"), 33);
    for (i = 0; i < json_object_array_length(get(j, "items")); i++)
    {
      struct obj_data *o =
          estate_unpack(json_object_to_json_string(json_object_array_get_idx(get(j, "items"), i)));
      assert(o);
      obj_to_char(o, &character);
    }
    json_object_put(j);
  }
}
int main(int argc, char **argv)
{
  struct json_object *root, *output;
  size_t i;
  char command[256];
  bool ok = true;
  int count = 0;
  struct obj_data *o;
  assert(argc >= 5);
  directory = argv[2];
  expected_uid = argv[4];
  if (argc > 5)
    crash_save = atoi(argv[5]);
  if (argc > 6)
    fail_save = atoi(argv[6]);
  if (argc > 7)
    after_ack = atoi(argv[7]);
  connection.conn = mysql_init(NULL);
  assert(mysql_real_connect(connection.conn, NULL, "root", "", argc > 8 ? argv[8] : "clan_test", 0, argv[1], 0));
  root = json_object_from_file("clan/quartermaster.json");
  assert(root && clan_qm_parse(root, &catalog));
  json_object_put(root);
  obj_proto = calloc(catalog.count, sizeof(*obj_proto));
  obj_index = calloc(catalog.count, sizeof(*obj_index));
  top_of_objt = catalog.count - 1;
  for (i = 0; i < catalog.count; i++)
  {
    struct clan_qm_item *d = &catalog.entries[i];
    o = &obj_proto[i];
    obj_index[i].vnum = d->prototype;
    o->item_number = i;
    GET_OBJ_TYPE(o) = d->type;
    GET_OBJ_LEVEL(o) = d->level;
    GET_OBJ_WEIGHT(o) = d->weight;
    GET_OBJ_COST(o) = d->value;
    memcpy(o->obj_flags.value, d->values, sizeof(d->values));
    GET_OBJ_BOUND_ID(o) = NOBODY;
  }
  character.player_specials = &specials;
  character.desc = &descriptor;
  descriptor.character = &character;
  descriptor.account = &account;
  STATE(&descriptor) = CON_PLAYING;
  account.id = 77;
  GET_IDNUM((&character)) = 1007;
  GET_LEVEL((&character)) = 10;
  GET_GOLD((&character)) = 10000;
  policy = clan_progression_v1;
  policy.controls.store_mode = CLAN_STORE_MYSQL;
  policy.controls.mode = CLAN_PROGRESSION_LIVE;
  policy.controls.public_release = true;
  policy.controls.quartermaster = true;
  load();
  if (!strcmp(argv[3], "begin"))
  {
    struct json_object *quote;
    impl_do_clanquartermaster_(&character, "buy field-map", 0, 0);
    quote = json_tokener_parse(specials.clan_qm_quote);
    assert(quote);
    json_object_object_add(quote, "uid", json_object_new_string(argv[4]));
    strlcpy(specials.clan_qm_quote, json_object_to_json_string(quote),
            sizeof(specials.clan_qm_quote));
    json_object_put(quote);
    snprintf(command, sizeof(command), "confirm field-map %s", argv[4]);
    impl_do_clanquartermaster_(&character, command, 0, 0);
  }
  else if (!strcmp(argv[3], "command"))
  {
    impl_do_clanquartermaster_(&character, argv[4], 0, 0);
  }
  else if (!strcmp(argv[3], "restricted"))
  {
    restriction = true;
    impl_do_clanquartermaster_(&character, "buy field-map", 0, 0);
  }
  else if (!strcmp(argv[3], "named"))
  {
    impl_do_clanquartermaster_(&character, "list", 0, 0);
    assert(strstr(messages, "Abanasinia field map") && !strstr(messages, "Budget:"));
    *messages = 0;
    impl_do_clanquartermaster_(&character, "buy Abanasinia field map", 0, 0);
    assert(strstr(messages, "quartermaster confirm") && !strstr(messages, "confirm field-map"));
    assert(!character.carrying && GET_GOLD((&character)) == 10000);
    impl_do_clanquartermaster_(&character, "confirm", 0, 0);
    assert(character.carrying && GET_GOLD((&character)) == 9985);
    impl_do_clanquartermaster_(&character, "confirm", 0, 0);
    assert(GET_GOLD((&character)) == 9985 && !character.carrying->next_content);
  }
  else if (!strcmp(argv[3], "quotes"))
  {
    struct json_object *quote;
    char requested[33];
    impl_do_clanquartermaster_(&character, "buy field-map", 0, 0);
    quote = json_tokener_parse(specials.clan_qm_quote);
    assert(quote);
    strlcpy(requested, str(quote, "uid"), 33);
    json_object_put(quote);
    clock_value += 60;
    snprintf(command, sizeof(command), "confirm field-map %s", requested);
    impl_do_clanquartermaster_(&character, command, 0, 0);
    assert(!character.carrying && GET_GOLD((&character)) == 10000 && strstr(messages, "expired"));
    impl_do_clanquartermaster_(&character, "buy field-map", 0, 0);
    quote = json_tokener_parse(specials.clan_qm_quote);
    assert(quote);
    strlcpy(requested, str(quote, "uid"), 33);
    json_object_object_add(get(quote, "terms"), "price", json_object_new_int(1));
    strlcpy(specials.clan_qm_quote, json_object_to_json_string(quote),
            sizeof(specials.clan_qm_quote));
    json_object_put(quote);
    snprintf(command, sizeof(command), "confirm field-map %s", requested);
    impl_do_clanquartermaster_(&character, command, 0, 0);
    assert(!character.carrying && GET_GOLD((&character)) == 10000);
  }
  else if (!strcmp(argv[3], "world"))
  {
    assert(clan_qm_catalog_load(&catalog));
    obj_index[0].vnum = 0;
    assert(!clan_qm_catalog_load(&catalog));
    obj_index[0].vnum = catalog.entries[0].prototype;
    obj_proto[0].activate_spell[0] = 1;
    assert(!clan_qm_catalog_load(&catalog));
    obj_proto[0].activate_spell[0] = 0;
    obj_proto[1].affected[0].modifier = 1;
    assert(!clan_qm_catalog_load(&catalog));
    obj_proto[1].affected[0].modifier = 0;
    assert(clan_qm_catalog_load(&catalog));
  }
  else
  {
    policy.controls.quartermaster = false;
    ok = clan_qm_recover(&character);
  }
  for (o = character.carrying; o; o = o->next_content)
  {
    count++;
    assert(GET_OBJ_BOUND_ID(o) == 1007 && GET_OBJ_COST(o) == 0);
  }
  output = json_object_new_object();
  json_object_object_add(output, "ok", json_object_new_boolean(ok));
  json_object_object_add(output, "gold", json_object_new_int(GET_GOLD((&character))));
  json_object_object_add(output, "items", json_object_new_int(count));
  json_object_object_add(output, "messages", json_object_new_string(messages));
  json_object_object_add(output, "locked", json_object_new_boolean(*specials.clan_qm_lock != 0));
  puts(json_object_to_json_string(output));
  json_object_put(output);
  while (character.carrying)
    extract_obj(character.carrying);
  free(obj_proto);
  free(obj_index);
  mysql_close(connection.conn);
  return 0;
}
