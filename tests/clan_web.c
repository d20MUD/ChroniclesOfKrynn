#include "clan_web.h"
#include "clan_buff_lifecycle.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static struct json_object *get(struct json_object *o, const char *k)
{
  struct json_object *v = NULL;
  json_object_object_get_ex(o, k, &v);
  return v;
}
static int64_t num(struct json_object *o, const char *k)
{
  return json_object_get_int64(get(o, k));
}
static const char *str(struct json_object *o, const char *k)
{
  return json_object_get_string(get(o, k));
}
int main(int argc, char **argv)
{
  char input[32768];
  size_t n = fread(input, 1, sizeof(input) - 1, stdin);
  struct json_object *req, *out = NULL, *result;
  MYSQL *db;
  enum clan_repository_result r;
  struct clan_progression_config c = clan_progression_v1;
  assert(argc == 3);
  input[n] = 0;
  req = json_tokener_parse(input);
  assert(req);
  db = mysql_init(NULL);
  assert(db && mysql_real_connect(db, NULL, "root", "", "clan_test", 0, argv[1], 0));
  c.controls.store_mode = CLAN_STORE_MYSQL;
  c.controls.mode = CLAN_PROGRESSION_LIVE;
  c.controls.public_release = true;
  c.controls.web = !num(req, "disabled");
  c.controls.buffs = !num(req, "buffs_disabled");
  if (!strcmp(argv[2], "read"))
    r = clan_web_read(db, &c, num(req, "player"), num(req, "account"), num(req, "now"), &out);
  else if (!strcmp(argv[2], "activate"))
  {
    struct clan_buff_request request = {0};
    struct clan_buff_decision decision;
    strcpy(request.operation_uid, str(req, "operation"));
    strcpy(request.clan_uid, str(req, "clan"));
    request.actor_id = num(req, "player");
    request.family = num(req, "family");
    request.tier = num(req, "tier");
    request.slot = num(req, "slot");
    request.rules_version = 1;
    if (get(req, "expected"))
      strcpy(request.expected_activation_uid, str(req, "expected"));
    r = clan_buff_activation_commit(db, &c, &request, num(req, "now"), &decision, &out);
  }
  else
  {
    struct clan_buff_cancel_request request = {0};
    unsigned int reasons;
    strcpy(request.operation_uid, str(req, "operation"));
    strcpy(request.clan_uid, str(req, "clan"));
    strcpy(request.activation_uid, str(req, "activation"));
    request.actor_id = num(req, "player");
    r = clan_buff_cancel(db, &c, &request, num(req, "now"), &reasons, &out);
  }
  result = json_object_new_object();
  json_object_object_add(result, "result", json_object_new_string(clan_repository_result_name(r)));
  if (out)
    json_object_object_add(result, "data", out);
  puts(json_object_to_json_string_ext(result, JSON_C_TO_STRING_PLAIN));
  json_object_put(req);
  json_object_put(result);
  mysql_close(db);
  return 0;
}
