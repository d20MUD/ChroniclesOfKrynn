/* Actual service commands and recovery against independent saved checkpoints. */
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "mysql.h"
#include "spec_abilities.h"
#include "assign_wpn_armor.h"
#include "clan_qm_services.h"
#include "clan_access.h"
#include <assert.h>
static MYSQL_POOL_CONN connection;
static struct char_data character;
static struct player_special_data specials;
static struct descriptor_data descriptor;
static struct account_data account;
static struct obj_data weapon;
static struct clan_progression_config policy;
static const char *directory;
static int saves, crash_save, fail_save;
static bool busy, quartermaster = true;
static char messages[65536];
struct player_special_data dummy_mob;
bool mysql_available = true;
struct special_ability_info_type special_ability_info[NUM_SPECABS];
struct weapon_table weapon_list[NUM_WEAPON_TYPES];
static int64_t clock_value = 2200010001;
time_t service_test_time(time_t *out)
{
  if (out)
    *out = clock_value;
  return clock_value;
}
#define time service_test_time
#include "../src/clan_qm_services.c"
#undef time
int MAX(int a, int b)
{
  return a > b ? a : b;
}
int MIN(int a, int b)
{
  return a < b ? a : b;
}
size_t strlcpy(char *out, const char *in, size_t cap)
{
  size_t n = strlen(in);
  if (cap)
  {
    size_t c = MIN(n, cap - 1);
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
const char *act(const char *s, int hide, struct char_data *ch, struct obj_data *o, void *v,
                int type)
{
  (void)hide;
  (void)ch;
  (void)o;
  (void)v;
  (void)type;
  return s;
}
bool clan_persistence_ready(void)
{
  return true;
}
bool clan_persistence_sql_authority(void)
{
  return true;
}
const struct clan_progression_config *clan_buff_runtime_policy(void)
{
  return &policy;
}
bool has_quartermaster_in_room(struct char_data *ch)
{
  (void)ch;
  return quartermaster;
}
MYSQL_POOL_CONN *mysql_pool_acquire(void)
{
  assert(!busy);
  busy = true;
  return &connection;
}
void mysql_pool_release(MYSQL_POOL_CONN *p)
{
  assert(p == &connection && busy);
  busy = false;
}
bool clan_qm_recover(struct char_data *ch)
{
  return clan_qm_services_recover(ch) >= 0;
}
int rand_number(int low, int high)
{
  assert(high >= low);
  return low + rand() % (high - low + 1);
}
struct obj_data *get_obj_in_list_vis(struct char_data *ch, char *name, int *number,
                                     struct obj_data *list)
{
  (void)ch;
  (void)number;
  return list && !strcmp(name, "sword") ? list : NULL;
}
int isname(const char *str, const char *namelist)
{
  return !strcmp(str, namelist);
}
static bool checkpoint(struct char_data *ch, bool inventory)
{
  struct json_object *j = json_object_new_object();
  char path[512];
  assert(!busy);
  saves++;
  if (saves == fail_save)
  {
    json_object_put(j);
    return false;
  }
  if (inventory)
  {
    json_object_object_add(j, "receipt", json_object_new_string(PS(ch)->clan_qm_inventory));
    json_object_object_add(j, "weapon_uid", json_object_new_string(weapon.clan_enchantment.uid));
    json_object_object_add(j, "weapon_receipt",
                           json_object_new_string(weapon.clan_enchantment.receipt));
    json_object_object_add(j, "effect", json_object_new_int(weapon.clan_enchantment.effect));
    json_object_object_add(j, "word", json_object_new_string(weapon.clan_enchantment.word));
    json_object_object_add(j, "week", json_object_new_int64(weapon.clan_enchantment.reroll_week));
  }
  else
  {
    json_object_object_add(j, "lock", json_object_new_string(PS(ch)->clan_qm_lock));
    json_object_object_add(j, "paid", json_object_new_boolean(PS(ch)->clan_qm_paid));
    json_object_object_add(j, "gold", json_object_new_int(GET_GOLD(ch)));
    json_object_object_add(j, "qp", json_object_new_int(GET_QUESTPOINTS(ch)));
    json_object_object_add(j, "motes",
                           json_object_new_int(GET_CRAFT_MOTES(ch, CRAFTING_MOTE_FIRE)));
  }
  snprintf(path, sizeof(path), "%s/%s.json", directory, inventory ? "inventory" : "player");
  assert(!json_object_to_file(path, j));
  json_object_put(j);
  if (saves == crash_save)
    _exit(88);
  if (saves == -fail_save)
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
static int64_t number_field(struct json_object *j, const char *key)
{
  struct json_object *v = NULL;
  json_object_object_get_ex(j, key, &v);
  return json_object_get_int64(v);
}
static void string_field(struct json_object *j, const char *key, char *out, size_t cap)
{
  struct json_object *v = NULL;
  json_object_object_get_ex(j, key, &v);
  const char *s = json_object_get_string(v);
  strlcpy(out, s ? s : "", cap);
}
static void load_files(void)
{
  char path[512];
  struct json_object *j;
  snprintf(path, sizeof(path), "%s/player.json", directory);
  j = json_object_from_file(path);
  if (j)
  {
    string_field(j, "lock", specials.clan_qm_lock, 33);
    specials.clan_qm_paid = number_field(j, "paid");
    GET_GOLD((&character)) = number_field(j, "gold");
    GET_QUESTPOINTS((&character)) = number_field(j, "qp");
    GET_CRAFT_MOTES((&character), CRAFTING_MOTE_FIRE) = number_field(j, "motes");
    json_object_put(j);
  }
  snprintf(path, sizeof(path), "%s/inventory.json", directory);
  j = json_object_from_file(path);
  if (j)
  {
    string_field(j, "receipt", specials.clan_qm_inventory, 33);
    string_field(j, "weapon_uid", weapon.clan_enchantment.uid, 33);
    string_field(j, "weapon_receipt", weapon.clan_enchantment.receipt, 33);
    string_field(j, "word", weapon.clan_enchantment.word, 24);
    weapon.clan_enchantment.effect = number_field(j, "effect");
    weapon.clan_enchantment.reroll_week = number_field(j, "week");
    json_object_put(j);
  }
}
int main(int argc, char **argv)
{
  struct json_object *out;
  int mode;
  assert(argc >= 5);
  directory = argv[2];
  mode = atoi(argv[3]);
  if (argc > 5)
    crash_save = atoi(argv[5]);
  if (argc > 6)
    fail_save = atoi(argv[6]);
  if (argc > 7)
    clock_value = strtoll(argv[7], NULL, 10);
  connection.conn = mysql_init(NULL);
  assert(mysql_real_connect(connection.conn, NULL, "root", "", "clan_test", 0, argv[1], 0));
  character.player_specials = &specials;
  character.desc = &descriptor;
  descriptor.character = &character;
  descriptor.account = &account;
  STATE(&descriptor) = CON_PLAYING;
  account.id = 77;
  GET_IDNUM((&character)) = argc > 8 ? atol(argv[8]) : 1007;
  GET_GOLD((&character)) = 10000000;
  GET_LEVEL((&character)) = 20;
  GET_OBJ_TYPE((&weapon)) = ITEM_WEAPON;
  GET_OBJ_VAL((&weapon), 0) = WEAPON_TYPE_LONG_SWORD;
  weapon_list[WEAPON_TYPE_LONG_SWORD].damageTypes = DAMAGE_TYPE_SLASHING;
  weapon_list[WEAPON_TYPE_LONG_SWORD].weaponFlags = WEAPON_FLAG_MARTIAL;
  GET_OBJ_LEVEL((&weapon)) = 20;
  weapon.name = "sword";
  weapon.short_description = "a test sword";
  weapon.carried_by = &character;
  character.carrying = &weapon;
  policy = clan_progression_v1;
  policy.controls.store_mode = CLAN_STORE_MYSQL;
  policy.controls.mode = CLAN_PROGRESSION_LIVE;
  policy.controls.public_release = true;
  policy.controls.quartermaster = true;
  load_files();
  srand(123);
  if (mode == 0)
    clan_qm_services_command(&character, argv[4]);
  if (mode == 1)
  {
    clan_qm_services_command(&character, argv[4]);
    assert(*specials.clan_qm_quote);
    clan_qm_services_command(&character, "confirm");
  }
  if (mode == 2)
    clan_qm_services_recover(&character);
  if (mode == 3)
  {
    struct clan_requirement r = {7, 2, 1};
    assert(clan_access_check(&character, &r, false));
    r.clan = 2;
    assert(!clan_access_check(&character, &r, false));
    r.clan = 0;
    r.rank = 1;
    assert(clan_access_check(&character, &r, false) == (GET_IDNUM((&character)) == 1007));
    r.favor = 5;
    assert(!clan_access_check(&character, &r, false));
  }
  if (mode == 4)
  {
    int i;
    for (i = 1; i <= 5; i++)
    {
      int draws;
      struct service_operation op = {0};
      for (draws = 0; draws < 1000; draws++)
      {
        draw(&op, i, &weapon);
        assert(clan_enchant_choice(op.effect)->rank <= i);
        assert(*op.word);
      }
    }
  }
  if (mode == 5)
  {
    const struct clan_enchant_choice *c = clan_enchant_choice(weapon.clan_enchantment.effect);
    assert(c);
    clan_enchant_activate(&weapon, true);
    assert(weapon.clan_enchantment.active);
    clan_enchant_process(&weapon, &character, NULL, ACTMTD_COMMAND_WORD,
                         weapon.clan_enchantment.word);
    assert(!weapon.clan_enchantment.active);
  }
  out = json_object_new_object();
#define OUT(key, val) json_object_object_add(out, key, json_object_new_int64(val))
  OUT("gold", GET_GOLD((&character)));
  OUT("qp", GET_QUESTPOINTS((&character)));
  OUT("motes", GET_CRAFT_MOTES((&character), CRAFTING_MOTE_FIRE));
  OUT("effect", weapon.clan_enchantment.effect);
  OUT("locked", *specials.clan_qm_lock != 0);
  OUT("week", weapon.clan_enchantment.reroll_week);
  json_object_object_add(out, "messages", json_object_new_string(messages));
  puts(json_object_to_json_string(out));
  json_object_put(out);
  mysql_close(connection.conn);
  return 0;
}
