#include "clan_delivery.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static struct json_object *get(struct json_object *o, const char *key)
{
  struct json_object *v = NULL;
  json_object_object_get_ex(o, key, &v);
  return v;
}
static int64_t num(struct json_object *o, const char *key)
{
  return json_object_get_int64(get(o, key));
}
static const char *str(struct json_object *o, const char *key)
{
  return json_object_get_string(get(o, key));
}
int main(int argc, char **argv)
{
  MYSQL *db;
  char input[16384];
  size_t n;
  struct json_object *req, *out = NULL, *result;
  enum clan_repository_result r = CLAN_REPO_INVALID;
  struct clan_delivery_definition d;
  struct clan_delivery_catalog c;
  assert(argc == 3);
  n = fread(input, 1, sizeof(input) - 1, stdin);
  input[n] = 0;
  req = json_tokener_parse(input);
  assert(req);
  db = mysql_init(NULL);
  assert(db && mysql_real_connect(db, NULL, "root", "", "clan_test", 0, argv[1], 0));
  clan_reward_fault = num(req, "fault");
  clan_reward_crash = num(req, "crash");
  if (!strcmp(argv[2], "catalog"))
    r = clan_delivery_catalog_parse(req, &c) ? CLAN_REPO_OK : CLAN_REPO_INVALID;
  else if (!strcmp(argv[2], "accept"))
  {
    if (clan_delivery_definition_parse(get(req, "definition"), &d))
      r = clan_delivery_accept(db, &d, str(req, "accepted_uid"), str(req, "clan_uid"),
                               num(req, "player_id"), num(req, "account_id"), num(req, "now"));
  }
  else if (!strcmp(argv[2], "expire"))
    r = clan_delivery_expire(db, num(req, "player_id"), num(req, "now"));
  else if (!strcmp(argv[2], "active"))
    r = clan_delivery_active(db, num(req, "player_id"), num(req, "now"), &out);
  else if (!strcmp(argv[2], "abandon"))
    r = clan_delivery_abandon(db, num(req, "player_id"), num(req, "now"));
  else if (!strcmp(argv[2], "quote"))
    r = clan_delivery_quote(db, num(req, "player_id"), num(req, "account_id"), num(req, "now"),
                            &out);
  else if (!strcmp(argv[2], "prepare"))
    r = clan_delivery_prepare(db, get(req, "quote"), num(req, "now"), &out);
  else if (!strcmp(argv[2], "operation"))
    r = clan_delivery_operation(db, num(req, "player_id"), str(req, "operation_uid"), &out);
  else if (!strcmp(argv[2], "complete"))
    r = clan_delivery_complete(db, get(req, "quote"), num(req, "now"), &out);
  else if (!strcmp(argv[2], "cancel"))
    r = clan_delivery_cancel(db, str(req, "operation_uid"), num(req, "now"));
  else if (!strcmp(argv[2], "ack"))
    r = clan_delivery_ack(db, str(req, "operation_uid"));
  else if (!strcmp(argv[2], "generic"))
  {
    struct clan_progression_config config = clan_progression_defaults;
    struct clan_progression_readiness ready = {.schema_ready = true,
                                               .schema_version = CLAN_PROGRESSION_SCHEMA_VERSION};
    config.controls.store_mode = CLAN_STORE_MYSQL;
    config.controls.mode = CLAN_PROGRESSION_LIVE;
    config.controls.public_release = true;
    r = clan_award_complete_personal(db, str(req, "operation_uid"), num(req, "now"), &config,
                                     &ready, &out);
  }
  result = json_object_new_object();
  json_object_object_add(result, "result", json_object_new_string(clan_repository_result_name(r)));
  json_object_object_add(result, "data", out);
  puts(json_object_to_json_string_ext(result, JSON_C_TO_STRING_PLAIN));
  json_object_put(result);
  json_object_put(req);
  mysql_close(db);
  return 0;
}
