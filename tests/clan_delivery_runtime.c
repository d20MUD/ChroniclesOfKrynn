#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "mysql.h"
#include "clan_delivery.h"
#include "clan_buff_runtime.h"
#include "clan_persistence.h"
#include "crafting_new.h"
#include "spec_abilities.h"
#include <assert.h>
#include <unistd.h>
static MYSQL_POOL_CONN connection;
static struct char_data character;
static struct player_special_data specials;
static struct obj_data pouch, arrows[3];
static bool busy;
static bool at_quartermaster;
static struct clan_progression_config policy;
static char messages[65536];
const char *crafting_materials[NUM_CRAFT_MATS] = {[3] = "bronze", [21] = "ash wood"};
static const char *directory;
static int crash_save, save_count, fail_save;
static int64_t clock_value = 2200000001;
struct player_special_data dummy_mob;
bool mysql_available = true;
time_t delivery_test_time(time_t *out)
{
  if (out)
    *out = clock_value;
  return clock_value;
}
#define time delivery_test_time
#include "../src/clan_delivery_runtime.c"
#undef time
int MIN(int a, int b)
{
  return a < b ? a : b;
}
int MAX(int a, int b)
{
  return a > b ? a : b;
}
size_t strlcpy(char *dst, const char *src, size_t size)
{
  size_t n = strlen(src);
  if (size)
  {
    size_t c = n < size - 1 ? n : size - 1;
    memcpy(dst, src, c);
    dst[c] = 0;
  }
  return n;
}
const struct clan_progression_config *clan_buff_runtime_policy(void)
{
  return &policy;
}
bool clan_persistence_ready(void)
{
  return true;
}
bool has_quartermaster_in_room(struct char_data *ch)
{
  (void)ch;
  return at_quartermaster;
}
bool clan_persistence_sql_authority(void)
{
  return true;
}
bool clan_persistence_refresh_progression(void)
{
  assert(!busy);
  return true;
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
}
size_t send_to_char(struct char_data *ch, const char *fmt, ...)
{
  va_list ap;
  size_t used = strlen(messages);
  (void)ch;
  va_start(ap, fmt);
  int n = vsnprintf(messages + used, sizeof(messages) - used, fmt, ap);
  va_end(ap);
  return n < 0 ? 0 : (size_t)n;
}
bool obj_has_special_ability(struct obj_data *o, int ability)
{
  struct obj_special_ability *s;
  for (s = o->special_abilities; s; s = s->next)
    if (s->ability == ability)
      return true;
  return false;
}
int material_grade(int material)
{
  return material == 21 ? 1 : material == 3 ? 2 : 0;
}
void extract_obj(struct obj_data *o)
{
  struct obj_data **p = &pouch.contains;
  while (*p && *p != o)
    p = &(*p)->next_content;
  if (*p)
    *p = o->next_content;
  else
  {
    p = &character.carrying;
    while (*p && *p != o)
      p = &(*p)->next_content;
    if (*p)
      *p = o->next_content;
  }
  o->next_content = NULL;
}
static bool write_checkpoint(struct char_data *ch, bool inventory)
{
  struct json_object *o = json_object_new_object(), *a;
  char path[512];
  int i;
  assert(!busy);
  save_count++;
  if (save_count == fail_save)
  {
    json_object_put(o);
    return false;
  }
  if (inventory)
  {
    json_object_object_add(o, "uid", json_object_new_string(PS(ch)->clan_delivery_inventory));
    json_object_object_add(o, "stage", json_object_new_int(PS(ch)->clan_delivery_inventory_stage));
    a = json_object_new_array();
    for (i = 0; i < 3; i++)
    {
      struct json_object *e = json_object_new_object();
      bool present=false; struct obj_data *it;
      for(it=ch->carrying;it;it=it->next_content) if(it==&arrows[i])present=true;
      json_object_object_add(e,"present",json_object_new_boolean(present));
      json_object_object_add(e, "qty",
                             json_object_new_int(GET_OBJ_VAL(&arrows[i], AMMO_VAL_QUANTITY)));
      json_object_object_add(e, "uid", json_object_new_string(arrows[i].clan_delivery_uid));
      json_object_object_add(e, "amount", json_object_new_int(arrows[i].clan_delivery_amount));
      json_object_array_add(a, e);
    }
    json_object_object_add(o, "items", a);
  }
  else
  {
    json_object_object_add(o, "uid", json_object_new_string(PS(ch)->clan_delivery_lock));
    json_object_object_add(o, "stage", json_object_new_int(PS(ch)->clan_delivery_stage));
    json_object_object_add(o, "materials", json_object_new_int(GET_CRAFT_MAT(ch, 21)));
  }
  snprintf(path, sizeof(path), "%s/%s.json", directory, inventory ? "inventory" : "player");
  assert(json_object_to_file(path, o) == 0);
  json_object_put(o);
  if (save_count == crash_save)
    _exit(88);
  return true;
}
bool save_char_checked(struct char_data *ch, int mode)
{
  (void)mode;
  return write_checkpoint(ch, false);
}
bool Crash_crashsave_checked(struct char_data *ch)
{
  return write_checkpoint(ch, true);
}
static void load_checkpoint(void)
{
  char path[512];
  struct json_object *o;
  int i;
  snprintf(path, sizeof(path), "%s/player.json", directory);
  o = json_object_from_file(path);
  if (o)
  {
    strlcpy(specials.clan_delivery_lock, str(o, "uid"), 33);
    specials.clan_delivery_stage = num(o, "stage");
    GET_CRAFT_MAT((&character), 21) = num(o, "materials");
    json_object_put(o);
  }
  snprintf(path, sizeof(path), "%s/inventory.json", directory);
  o = json_object_from_file(path);
  if (o)
  {
    strlcpy(specials.clan_delivery_inventory, str(o, "uid"), 33);
    specials.clan_delivery_inventory_stage = num(o, "stage");
    struct obj_data **tail=&character.carrying;
    bool crafted=GET_OBJ_TYPE(&arrows[0])==ITEM_WEAPON;
    if(crafted)character.carrying=NULL;
    for (i = 0; i < 3; i++)
    {
      struct json_object *e = json_object_array_get_idx(get(o, "items"), i);
      if(crafted) {
        arrows[i].next_content=NULL;
        if(json_object_get_boolean(get(e,"present"))) { *tail=&arrows[i];tail=&arrows[i].next_content; }
      }
      GET_OBJ_VAL(&arrows[i], AMMO_VAL_QUANTITY) = num(e, "qty");
      strlcpy(arrows[i].clan_delivery_uid, str(e, "uid"), 33);
      arrows[i].clan_delivery_amount = num(e, "amount");
    }
    json_object_put(o);
  }
}
int main(int argc, char **argv)
{
  struct json_object *q = NULL, *r = NULL, *output;
  struct clan_delivery_definition def = {
      "native-material-v1", "Native delivery", 1, 21, 1, 50, 1, 0};
  char uid[33];
  bool ammo, crafted, ok;
  int i, total = 0;
  assert(argc >= 5);
  directory = argv[2];
  ammo = !strcmp(argv[3], "ammo");
  crafted = !strcmp(argv[3], "crafted");
  populate_crafting_recipes();
  if (argc > 5)
    crash_save = atoi(argv[5]);
  if (argc > 6)
    fail_save = atoi(argv[6]);
  if (argc > 7)
    clock_value = atoll(argv[7]);
  if (argc > 8)
    clan_reward_crash = atoi(argv[8]);
  character.player_specials = &specials;
  GET_IDNUM((&character)) = 5000000007;
  GET_CRAFT_MAT((&character), 21) = 200;
  GET_EQ((&character), WEAR_AMMO_POUCH) = &pouch;
  pouch.contains = &arrows[0];
  for (i = 0; i < 3; i++)
  {
    GET_OBJ_TYPE(&arrows[i]) = ITEM_MISSILE;
    GET_OBJ_VAL(&arrows[i], 0) = 1;
    GET_OBJ_VAL(&arrows[i], AMMO_VAL_MAX) = 50;
    GET_OBJ_VAL(&arrows[i], AMMO_VAL_QUANTITY) = 50;
    arrows[i].next_content = i < 2 ? &arrows[i + 1] : NULL;
  }
  if (ammo)
  {
    def.kind = 2;
    def.resource = 1;
    def.grade = 0;
    def.quantity = 100;
  }
  if (crafted) {
    def.kind=CLAN_DELIVERY_CRAFTED; def.resource=CRAFT_RECIPE_WEAPON_DAGGER;
    def.grade=0; def.quantity=2;
    pouch.contains=NULL; character.carrying=&arrows[0];
    for(i=0;i<3;i++) {
      GET_OBJ_TYPE(&arrows[i])=ITEM_WEAPON;
      GET_OBJ_VAL(&arrows[i],0)=crafting_recipes[def.resource].object_subtype;
      SET_BIT_AR(GET_OBJ_EXTRA(&arrows[i]),ITEM_CRAFTED);
    }
  }
  connection.conn = mysql_init(NULL);
  assert(mysql_real_connect(connection.conn, NULL, "root", "", "clan_test", 0, argv[1], 0));
  if (!strcmp(argv[4], "commands"))
  {
    struct descriptor_data descriptor = {0};
    struct account_data account = {0};
    char confirm[80];
    character.desc = &descriptor;
    descriptor.character = &character;
    descriptor.account = &account;
    account.id = 77;
    STATE(&descriptor) = CON_PLAYING;
    policy = clan_progression_v1;
    impl_do_clandeliveries_(&character, "list", 0, 0);
    assert(strstr(messages, "unavailable"));
    *messages = 0;
    policy.controls.store_mode = CLAN_STORE_MYSQL;
    policy.controls.mode = CLAN_PROGRESSION_LIVE;
    policy.controls.public_release = true;
    policy.controls.contracts = true;
    impl_do_clandeliveries_(&character, "list", 0, 0);
    assert(strstr(messages, "Ash shaft supplies") && !strstr(messages, "ash-shafts-v1") && strstr(messages, "0 / 2"));
    *messages = 0;
    impl_do_clandeliveries_(&character, "accept Ash shaft supplies", 0, 0);
    assert(strstr(messages, "Visit a quartermaster"));
    *messages = 0;
    at_quartermaster = true;
    impl_do_clandeliveries_(&character, "accept Ash shaft supplies", 0, 0);
    assert(strstr(messages, "accept recorded"));
    *messages = 0;
    impl_do_clandeliveries_(&character, "turnin", 0, 0);
    assert(strstr(messages, "Reward: 6.25 Influence"));
    assert(GET_CRAFT_MAT((&character), 21) == 200);
    struct json_object *quote = json_tokener_parse(specials.clan_delivery_quote);
    assert(quote);
    snprintf(confirm, sizeof(confirm), "confirm %s", str(quote, "operation_uid"));
    json_object_put(quote);
    *messages = 0;
    impl_do_clandeliveries_(&character, "confirm", 0, 0);
    assert(GET_CRAFT_MAT((&character), 21) == 150 && strstr(messages, "delivery complete"));
    impl_do_clandeliveries_(&character, confirm, 0, 0);
    assert(GET_CRAFT_MAT((&character), 21) == 150);
    *messages = 0;
    impl_do_clandeliveries_(&character, "list", 0, 0);
    assert(strstr(messages, "1 / 2") && strstr(messages, "completed this week"));
    mysql_close(connection.conn);
    puts("{}");
    return 0;
  }
  if (!strcmp(argv[4], "goods"))
  {
    struct obj_special_ability returning = {0};
    returning.ability = WEAPON_SPECAB_RETURNING;
    assert(clan_delivery_goods_available(&character, &def));
    if (crafted) {
      struct clan_delivery_catalog live_catalog;
      assert(clan_delivery_catalog_load(&live_catalog) && live_catalog.count==17);
      struct clan_delivery_definition invalid=def;invalid.resource=38;
      assert(!terms_valid(&invalid));
      int recipe_ids[]={CRAFT_RECIPE_ARMOR_LEATHER,CRAFT_RECIPE_ARMOR_SMALL_SHIELD,
                        CRAFT_RECIPE_MISC_BOOTS,CRAFT_RECIPE_INSTRUMENT_FLUTE};
      for(int rindex=0;rindex<4;rindex++) {
        struct clan_delivery_definition other=def;other.resource=recipe_ids[rindex];
        const struct craft_recipe_data *recipe=&crafting_recipes[other.resource];
        for(int item=0;item<3;item++) {
          GET_OBJ_TYPE(&arrows[item])=recipe->object_type;
          GET_OBJ_VAL(&arrows[item],recipe->object_type==ITEM_ARMOR?1:0)=recipe->object_subtype;
          if(recipe->object_type==ITEM_WORN)SET_BIT_AR(GET_OBJ_WEAR(&arrows[item]),recipe->object_subtype);
        }
        assert(clan_delivery_goods_available(&character,&other));
      }
      for(int item=0;item<3;item++) {
        GET_OBJ_TYPE(&arrows[item])=ITEM_WEAPON;
        GET_OBJ_VAL(&arrows[item],0)=crafting_recipes[def.resource].object_subtype;
      }
      assert(clan_delivery_goods_available(&character,&def));
      character.carrying=&arrows[2];pouch.contains=&arrows[0];arrows[1].next_content=NULL;
      assert(!clan_delivery_goods_available(&character,&def));
      pouch.contains=NULL;character.carrying=&arrows[0];arrows[1].next_content=&arrows[2];
      GET_OBJ_VAL(&arrows[0],4)=1; GET_OBJ_VAL(&arrows[1],4)=1;
      assert(!clan_delivery_goods_available(&character,&def));
      GET_OBJ_VAL(&arrows[0],4)=GET_OBJ_VAL(&arrows[1],4)=0;
      REMOVE_BIT_AR(GET_OBJ_EXTRA(&arrows[0]),ITEM_CRAFTED);
      REMOVE_BIT_AR(GET_OBJ_EXTRA(&arrows[1]),ITEM_CRAFTED);
      assert(!clan_delivery_goods_available(&character,&def));
      SET_BIT_AR(GET_OBJ_EXTRA(&arrows[0]),ITEM_CRAFTED);
      SET_BIT_AR(GET_OBJ_EXTRA(&arrows[1]),ITEM_CRAFTED);
      GET_OBJ_BOUND_ID(&arrows[0])=GET_OBJ_BOUND_ID(&arrows[1])=123;
      assert(!clan_delivery_goods_available(&character,&def));
      GET_OBJ_BOUND_ID(&arrows[0])=GET_OBJ_BOUND_ID(&arrows[1])=0;
      assert(goods_pay(&character,&def,"dddddddddddddddddddddddddddddddd"));
      assert(crafted_markers(&character,&def,"dddddddddddddddddddddddddddddddd",true,false));
      assert(clan_delivery_goods_count(&character,&def)==2);
    }
    else     if (ammo)
    {
      GET_OBJ_VAL(&arrows[0], AMMO_VAL_UNLIMITED) = 1;
      GET_OBJ_VAL(&arrows[1], AMMO_VAL_RETURNING) = 1;
      assert(!clan_delivery_goods_available(&character, &def));
      GET_OBJ_VAL(&arrows[0], AMMO_VAL_UNLIMITED) = 0;
      GET_OBJ_VAL(&arrows[1], AMMO_VAL_RETURNING) = 0;
      arrows[0].special_abilities = &returning;
      arrows[1].special_abilities = &returning;
      assert(!clan_delivery_goods_available(&character, &def));
      arrows[0].special_abilities = arrows[1].special_abilities = NULL;
      GET_OBJ_VAL(&arrows[0], AMMO_VAL_QUANTITY) = 25;
      GET_OBJ_VAL(&arrows[1], AMMO_VAL_QUANTITY) = 30;
      GET_OBJ_VAL(&arrows[2], AMMO_VAL_QUANTITY) = 45;
      assert(clan_delivery_goods_available(&character, &def));
      assert(goods_pay(&character, &def, "dddddddddddddddddddddddddddddddd"));
      assert(ammo_markers(&character, "dddddddddddddddddddddddddddddddd", true, false));
      assert(GET_OBJ_VAL(&arrows[0], AMMO_VAL_QUANTITY) == 25 &&
             GET_OBJ_VAL(&arrows[1], AMMO_VAL_QUANTITY) == 30 &&
             GET_OBJ_VAL(&arrows[2], AMMO_VAL_QUANTITY) == 45);
    }
    else
    {
      GET_CRAFT_MAT((&character), 21) = 49;
      assert(!clan_delivery_goods_available(&character, &def));
    }
    mysql_close(connection.conn);
    puts("{}");
    return 0;
  }
  if (!strcmp(argv[4], "begin"))
  {
    assert(clan_repository_uid_generate(uid));
    assert(clan_delivery_accept(connection.conn, &def, uid, "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
                                GET_IDNUM((&character)), 77, clock_value) == CLAN_REPO_OK);
    assert(clan_delivery_quote(connection.conn, GET_IDNUM((&character)), 77, clock_value, &q) ==
           CLAN_REPO_OK);
    strlcpy(specials.clan_delivery_lock, str(q, "operation_uid"), 33);
    assert(save_char_checked(&character, 0));
    assert(clan_delivery_prepare(connection.conn, q, clock_value, &r) == CLAN_REPO_OK);
    json_object_put(r);
    /* Initial failure keeps the in-memory checkpoint for a later recovery. */
    goods_pay(&character, &def, str(q, "operation_uid"));
    json_object_put(q);
  }
  else
    load_checkpoint();
  ok = clan_delivery_recover(&character);
  for (i = 0; i < 3; i++)
    total += GET_OBJ_VAL(&arrows[i], AMMO_VAL_QUANTITY);
  output = json_object_new_object();
  json_object_object_add(output, "ok", json_object_new_boolean(ok));
  json_object_object_add(output, "materials", json_object_new_int(GET_CRAFT_MAT((&character), 21)));
  json_object_object_add(output, "shots", json_object_new_int(total));
  int present=0; struct obj_data *it;
  for(it=crafted?character.carrying:pouch.contains;it;it=it->next_content)present++;
  if(crafted)json_object_object_add(output,"items",json_object_new_int(present));
  puts(json_object_to_json_string(output));
  json_object_put(output);
  mysql_close(connection.conn);
  return 0;
}
