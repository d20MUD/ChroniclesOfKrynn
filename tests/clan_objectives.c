#include "clan_objectives.h"
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
static int64_t n(struct json_object *o, const char *key)
{
  return json_object_get_int64(get(o, key));
}
static const char *s(struct json_object *o, const char *key)
{
  const char *v = json_object_get_string(get(o, key));
  return v ? v : "";
}
static void check_rotation(const struct clan_objective_catalog *original)
{
  int level, week, family;
  size_t i;
  for (level = 1; level <= 30; level++)
    for (week = 0; week < 12; week++)
    {
      struct clan_objective_catalog rotated = *original, reversed = *original;
      for (i = 0; i < original->count; i++)
        reversed.entries[i] = original->entries[original->count-i-1];
      clan_objective_catalog_rotate(&rotated, INT64_C(2200000001)+week*INT64_C(604800), level);
      clan_objective_catalog_rotate(&reversed, INT64_C(2200000001)+week*INT64_C(604800), level);
      assert(rotated.count == reversed.count);
      for (i = 0; i < rotated.count; i++)
      {
        size_t j;
        for (j = 0; j < reversed.count; j++)
          if (!strcmp(rotated.entries[i].id, reversed.entries[j].id)) break;
        assert(j < reversed.count);
      }
      for (family = 4; family <= 5; family++)
      {
        size_t before = 0, after = 0;
        for (i = 0; i < original->count; i++)
        {
          const struct clan_objective_definition *d = &original->entries[i];
          if (d->family == family && d->min_clan_level == 1 &&
              level >= d->min_player_level && level <= d->max_player_level) before++;
        }
        for (i = 0; i < rotated.count; i++)
        {
          const struct clan_objective_definition *d = &rotated.entries[i];
          if (d->family == family && d->min_clan_level == 1 &&
              level >= d->min_player_level && level <= d->max_player_level) after++;
        }
        assert(after == (before > 2 ? before-1 : before));
      }
    }
}
int main(int argc, char **argv)
{
  char buf[16384];
  size_t size;
  MYSQL *db;
  struct json_object *req, *out = NULL, *result;
  enum clan_repository_result r = CLAN_REPO_INVALID;
  struct clan_objective_definition d;
  struct clan_objective_catalog catalog;
  assert(argc == 3);
  size = fread(buf, 1, sizeof(buf) - 1, stdin);
  buf[size] = 0;
  req = json_tokener_parse(buf);
  assert(req);
  db = mysql_init(NULL);
  assert(db && mysql_real_connect(db, NULL, "root", "", "clan_test", 0, argv[1], 0));
  clan_reward_fault = n(req, "fault");
  clan_reward_crash = n(req, "crash");
  clan_objective_crash = n(req, "objective_crash");
  if (!strcmp(argv[2], "catalog"))
  {
    r = clan_objective_catalog_parse(req, &catalog) ? CLAN_REPO_OK : CLAN_REPO_INVALID;
    if (r == CLAN_REPO_OK) check_rotation(&catalog);
  }
  else if (!strcmp(argv[2], "accept"))
  {
    if (clan_objective_definition_parse(get(req, "definition"), &d))
      r = clan_objective_accept(db, &d, s(req, "contract_uid"), s(req, "clan_uid"),
                                n(req, "player_id"), n(req, "account_id"), n(req, "level"),
                                n(req, "now"));
  }
  else if (!strcmp(argv[2], "expire"))
    r = clan_objective_expire(db, n(req, "player_id"), n(req, "now"));
  else if (!strcmp(argv[2], "active"))
    r = clan_objective_active(db, n(req, "player_id"), n(req, "family"), &out);
  else if (!strcmp(argv[2], "abandon"))
    r = clan_objective_abandon(db, n(req, "player_id"), n(req, "family"));
  else if (!strcmp(argv[2], "claims"))
    r = clan_objective_claims(db, n(req, "account_id"), n(req, "family"), n(req, "now"), &out);
  else if (!strcmp(argv[2], "progress") || !strcmp(argv[2], "capture"))
  {
    struct clan_objective_event e = {0};
    snprintf(e.event_uid, 33, "%s", s(req, "event_uid"));
    snprintf(e.contract_uid, 33, "%s", s(req, "contract_uid"));
    snprintf(e.clan_uid, 33, "%s", s(req, "clan_uid"));
#define F(k) e.k = n(req, #k)
    F(player_id);
    F(account_id);
    F(occurred_at);
    F(room);
    F(family);
    F(target);
    F(grade);
    F(base_units);
#undef F
    r = !strcmp(argv[2], "progress") ? clan_objective_progress(db, &e, n(req, "now"))
                                     : clan_objective_capture(db, &e, n(req, "level"));
    if (r == CLAN_REPO_OK)
      out = json_object_new_string(e.contract_uid);
  }
  else if (!strcmp(argv[2], "complete"))
    r = clan_objective_reward_complete(db, s(req, "contract_uid"), n(req, "now"), &out);
  else if (!strcmp(argv[2], "generic") || !strcmp(argv[2], "shared"))
  {
    struct clan_progression_config c = clan_progression_defaults;
    struct clan_progression_readiness ready = {.schema_ready = true,
                                               .schema_version = CLAN_PROGRESSION_SCHEMA_VERSION};
    c.controls.store_mode = CLAN_STORE_MYSQL;
    c.controls.mode = CLAN_PROGRESSION_LIVE;
    c.controls.public_release = true;
    r = !strcmp(argv[2], "shared") ? clan_award_complete_shared(db, s(req, "contract_uid"),
                                                                n(req, "now"), &c, &ready, &out)
                                   : clan_award_complete_personal(db, s(req, "contract_uid"),
                                                                  n(req, "now"), &c, &ready, &out);
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
