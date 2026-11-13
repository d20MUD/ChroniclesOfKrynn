#include "clan_quartermaster.h"
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
  char input[65536];
  size_t n = fread(input, 1, sizeof(input) - 1, stdin);
  MYSQL *db;
  struct json_object *req, *out = NULL, *result;
  struct clan_qm_catalog catalog;
  struct clan_progression_config config = clan_progression_v1;
  enum clan_repository_result r = CLAN_REPO_INVALID;
  assert(argc == 3);
  input[n] = 0;
  req = json_tokener_parse(input);
  assert(req);
  db = mysql_init(NULL);
  assert(db && mysql_real_connect(db, NULL, "root", "", "clan_test", 0, argv[1], 0));
  config.controls.store_mode = CLAN_STORE_MYSQL;
  config.controls.mode = CLAN_PROGRESSION_LIVE;
  config.controls.public_release = true;
  config.controls.quartermaster = !num(req, "disabled");
  if (!strcmp(argv[2], "parse"))
    r = clan_qm_parse(get(req, "catalog"), &catalog) ? CLAN_REPO_OK : CLAN_REPO_INVALID;
  else if (!strcmp(argv[2], "prepare"))
  {
    if (clan_qm_parse(get(req, "catalog"), &catalog))
      r = clan_qm_prepare(db, &catalog.entries[num(req, "index")], str(req, "uid"),
                          str(req, "clan"), num(req, "player"), num(req, "account"),
                          num(req, "level"), num(req, "gold"), str(req, "payload"), num(req, "now"),
                          &config);
  }
  else if (!strcmp(argv[2], "read"))
    r = clan_qm_operation(db, num(req, "player"), get(req, "pending") ? NULL : str(req, "uid"),
                          &out);
  else if (!strcmp(argv[2], "ack"))
    r = clan_qm_ack(db, num(req, "player"), str(req, "uid"));
  result = json_object_new_object();
  json_object_object_add(result, "result", json_object_new_string(clan_repository_result_name(r)));
  if (out)
    json_object_object_add(result, "data", out);
  puts(json_object_to_json_string_ext(result, JSON_C_TO_STRING_PLAIN));
  json_object_put(result);
  json_object_put(req);
  mysql_close(db);
  return 0;
}
