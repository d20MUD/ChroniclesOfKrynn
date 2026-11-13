/* Exercise production eligibility composition without a live world or writes. */
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "clan_web_content.h"
#include "clan_delivery.h"
#include "crafting_recipes.h"
#include "clan_objectives.h"
#include "clan_quartermaster.h"
#include "clan_expeditions.h"
#include <assert.h>
const char *crafting_materials[NUM_CRAFT_MATS] = {[3] = "bronze", [21] = "ash"};
struct obj_data *obj_proto;
static struct clan_progression_config policy;
static struct clan_qm_catalog catalog;
static bool at_qm = true, restricted = false;
const struct clan_progression_config *clan_buff_runtime_policy(void)
{
  return &policy;
}
bool has_quartermaster_in_room(struct char_data *ch)
{
  return at_qm;
}
int can_carry_weight_limit(struct char_data *ch)
{
  return 10000;
}
int invalid_align(struct char_data *ch, struct obj_data *o)
{
  return restricted;
}
int invalid_prof(struct char_data *ch, struct obj_data *o)
{
  return restricted;
}
int invalid_class(struct char_data *ch, struct obj_data *o)
{
  return restricted;
}
bool is_class_req_object(struct char_data *ch, struct obj_data *o, bool output)
{
  return !restricted;
}
bool is_class_anti_object(struct char_data *ch, struct obj_data *o, bool output)
{
  return restricted;
}
obj_rnum real_object(obj_vnum v)
{
  size_t i;
  for (i = 0; i < catalog.count; i++)
    if (catalog.entries[i].prototype == (int)v)
      return i;
  return NOTHING;
}
bool clan_qm_catalog_load(struct clan_qm_catalog *out)
{
  *out = catalog;
  return true;
}
bool clan_delivery_catalog_load(struct clan_delivery_catalog *out)
{
  struct json_object *o = json_object_from_file("clan/deliveries.json");
  bool ok = clan_delivery_catalog_parse(o, out);
  json_object_put(o);
  return ok;
}
bool clan_objective_catalog_load(struct clan_objective_catalog *out)
{
  struct json_object *o = json_object_from_file("clan/objectives.json");
  bool ok = clan_objective_catalog_parse(o, out);
  json_object_put(o);
  return ok;
}
int clan_delivery_goods_count(struct char_data *ch, const struct clan_delivery_definition *d)
{
  return d->quantity;
}
bool clan_delivery_goods_available(struct char_data *ch, const struct clan_delivery_definition *d)
{
  return true;
}
static bool area(int n, void *context)
{
  return true;
}
static bool target(int a, int n, void *context)
{
  return true;
}
struct clan_expedition_world clan_expedition_world_refs(void)
{
  struct clan_expedition_world w = {area, target, target, NULL};
  return w;
}
const char *clan_expedition_target_name(int n)
{
  return "Designated target";
}
const char *clan_expedition_area_name(int n)
{
  return "Test area";
}
int main(int argc, char **argv)
{
  populate_crafting_recipes();
  struct char_data ch = {0};
  struct player_special_data ps = {0};
  struct json_object *o, *items;
  assert(argc == 3);
  o = json_object_from_file(argv[1]);
  assert(o);
  items = json_object_from_file("clan/quartermaster.json");
  assert(items && clan_qm_parse(items, &catalog));
  json_object_put(items);
  obj_proto = calloc(catalog.count, sizeof(*obj_proto));
  assert(obj_proto);
  policy = clan_progression_v1;
  policy.controls.mode = CLAN_PROGRESSION_LIVE;
  policy.controls.store_mode = CLAN_STORE_MYSQL;
  policy.controls.public_release = true;
  policy.controls.contracts = policy.controls.expeditions = policy.controls.quartermaster = true;
  ch.player_specials = &ps;
  GET_IDNUM(&ch) = 1007;
  GET_LEVEL(&ch) = 10;
  GET_GOLD(&ch) = 10000;
  if (!strcmp(argv[2], "away"))
    at_qm = false;
  if (!strcmp(argv[2], "restricted"))
    restricted = true;
  if (!strcmp(argv[2], "poor"))
    GET_GOLD(&ch) = 0;
  if (!strcmp(argv[2], "disabled"))
    policy.controls.mode = CLAN_PROGRESSION_OFF;
  if (!strcmp(argv[2], "preview"))
  {
    struct json_object *q = json_object_new_object(), *clan = NULL, *value = NULL;
    json_object_object_get_ex(o, "clan", &clan);
    json_object_object_get_ex(clan, "uid", &value);
    snprintf(ps.clan_delivery_quote, sizeof(ps.clan_delivery_quote),
             "{\"player_id\":1007,\"clan_uid\":\"%s\",\"operation_uid\":"
             "\"11111111111111111111111111111111\",\"accepted_uid\":"
             "\"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\",\"confirm_before\":2200000061,\"delta\":{\"xp_"
             "eighths\":800,\"influence_eighths\":400,\"favor_eighths\":400}}",
             json_object_get_string(value));
    json_object_object_add(q, "terms", clan_qm_terms(&catalog.entries[0]));
    json_object_object_add(q, "uid", json_object_new_string("22222222222222222222222222222222"));
    json_object_object_add(q, "clan", json_object_get(value));
    json_object_object_add(q, "expires", json_object_new_int64(2200000061));
    snprintf(ps.clan_qm_quote, sizeof(ps.clan_qm_quote), "%s",
             json_object_to_json_string_ext(q, JSON_C_TO_STRING_PLAIN));
    json_object_put(q);
  }
  clan_web_content_enrich(&ch, o, 2200000001);
  puts(json_object_to_json_string_ext(o, JSON_C_TO_STRING_PLAIN));
  json_object_put(o);
  free(obj_proto);
  return 0;
}
