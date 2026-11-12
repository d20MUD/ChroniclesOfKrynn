#include "clan_expedition_enrollment.h"
#include "clan_awards.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
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
static struct clan_expedition_member member(struct json_object *o)
{
  struct clan_expedition_member m = {num(o, "player"), num(o, "account"), num(o, "level")};
  return m;
}
static bool area(int a, void *c)
{
  (void)a;
  return !num(c, "bad_world");
}
static bool target(int a, int m, void *c)
{
  (void)a;
  (void)m;
  return !num(c, "bad_world");
}
int main(int argc, char **argv)
{
  MYSQL *db;
  char input[32768];
  size_t n;
  struct json_object *req, *out = NULL, *result;
  struct clan_expedition_catalog catalog;
  struct clan_expedition_world refs = {area, target, target, NULL};
  struct clan_expedition_member self, members[6];
  struct clan_progression_config config = clan_progression_defaults;
  enum clan_repository_result r = CLAN_REPO_INVALID;
  size_t i, count;
  assert(argc == 3);
  n = fread(input, 1, sizeof(input) - 1, stdin);
  input[n] = 0;
  req = json_tokener_parse(input);
  assert(req);
  db = mysql_init(NULL);
  assert(db && mysql_real_connect(db, NULL, "root", "", "clan_test", 0, argv[1], 0));
  config.controls.store_mode = CLAN_STORE_MYSQL;
  config.controls.mode = CLAN_PROGRESSION_LIVE;
  config.controls.public_release = true;
  config.controls.contracts = true;
  config.controls.expeditions = !num(req, "disabled");
  refs.context = req;
  self = member(req);
  if (!strcmp(argv[2], "create"))
  {
    if (clan_expedition_catalog_parse(get(req, "catalog"), &catalog, NULL, 0))
      r = clan_expedition_create(db, &catalog, &refs, str(req, "definition"), str(req, "outing"),
                                 str(req, "clan"), &self, num(req, "now"), &config);
  }
  else if (!strcmp(argv[2], "join"))
    r = clan_expedition_enroll(db, str(req, "outing"), str(req, "clan"), &self, num(req, "now"),
                               &config);
  else if (!strcmp(argv[2], "combat"))
  {
    struct clan_combat_encounter e = {0};
    struct clan_combat_actor actors[6] = {{0}};
    struct clan_combat_action actions[18] = {{0}};
    count = json_object_array_length(get(req, "members"));
    assert(count <= 6);
    e.actors = actors;
    e.actor_count = count;
    e.actions = actions;
    e.action_count = count * 3;
    e.normal_death = e.world_npc = true;
    e.room = 100;
    e.start_max_hp = 1000;
    e.victim_level = 27;
    e.started_at = num(req, "now") - 10;
    e.occurred_at = num(req, "occurred_at");
    if (!e.occurred_at)
      e.occurred_at = num(req, "now");
    e.victim_prototype = num(req, "target");
    snprintf(e.encounter_uid, 33, "%s", str(req, "event"));
    e.owned = num(req, "owned");
    e.test = num(req, "test");
    e.victim_level = num(req, "trivial") ? 1 : 27;
    for (i = 0; i < count; i++)
    {
      struct json_object *m = json_object_array_get_idx(get(req, "members"), i);
      actors[i].owner = -1;
      actors[i].player_id = num(m, "player");
      actors[i].account_id = num(m, "account");
      actors[i].level = num(m, "level");
      actors[i].room = 100;
      actors[i].non_clan_max_hp = 500;
      actors[i].accepted = actors[i].alive = actors[i].playing = true;
      snprintf(actors[i].clan_uid, 33, "%s", get(m, "clan") ? str(m, "clan") : str(req, "clan"));
      for (size_t j = 0; j < 3; j++)
      {
        struct clan_combat_action *a = &actions[3 * i + j];
        a->actor = i;
        a->ally = (i + 1) % count;
        a->kind = num(m, "kind");
        a->application = 3 * i + j + 1;
        a->at = e.occurred_at - 1;
        a->round = j + 1;
        a->room = 100;
        a->amount = num(m, "inactive") ? 0 : 100;
        a->available = 1000;
        a->useful = a->ally_engaged = true;
      }
    }
#ifdef CLAN_REWARD_TEST
    clan_expedition_crash = num(req, "expedition_crash");
#endif
    r = clan_expedition_combat(db, &e, num(req, "area"), num(req, "now"), &config);
  }
  else if (!strcmp(argv[2], "progress"))
    r = clan_expedition_progress_read(db, str(req, "outing"), &out);
  else if (!strcmp(argv[2], "claims"))
    r = clan_expedition_claim_list(db, str(req, "clan"), self.player, self.account, num(req, "now"),
                                   &out);
  else if (!strcmp(argv[2], "claim"))
    r = clan_expedition_claim_prepare(db, str(req, "outing"), str(req, "clan"), &self,
                                      num(req, "now"), &config, &out);
  else if (!strcmp(argv[2], "complete"))
  {
#ifdef CLAN_REWARD_TEST
    clan_reward_fault = num(req, "fault");
#endif
    r = clan_expedition_reward_complete(db, str(req, "operation"), num(req, "now"), &config, &out);
  }
  else if (!strcmp(argv[2], "generic"))
  {
    struct clan_progression_readiness ready = {.schema_ready = true,
                                               .schema_version = CLAN_PROGRESSION_SCHEMA_VERSION};
    r = clan_award_complete_personal(db, str(req, "operation"), num(req, "now"), &config, &ready,
                                     &out);
  }
  else if (!strcmp(argv[2], "read"))
    r = clan_expedition_read(db, str(req, "clan"), &out);
  else if (!strcmp(argv[2], "start") || !strcmp(argv[2], "seal"))
  {
    count = json_object_array_length(get(req, "members"));
    assert(count <= 6);
    for (i = 0; i < count; i++)
      members[i] = member(json_object_array_get_idx(get(req, "members"), i));
    if (!strcmp(argv[2], "start"))
      r = clan_expedition_start(db, str(req, "outing"), str(req, "clan"), self.player, members,
                                count, num(req, "now"), &config);
    else
      r = clan_expedition_seal(db, str(req, "outing"), str(req, "clan"), members, count,
                               num(req, "now"), &config);
  }
  else if (!strcmp(argv[2], "leave") || !strcmp(argv[2], "abandon"))
    r = clan_expedition_leave(db, str(req, "outing"), str(req, "clan"), &self,
                              !strcmp(argv[2], "abandon"), num(req, "now"), &config);
  result = json_object_new_object();
  json_object_object_add(result, "result", json_object_new_string(clan_repository_result_name(r)));
  json_object_object_add(result, "data", out);
  puts(json_object_to_json_string(result));
  json_object_put(result);
  json_object_put(req);
  mysql_close(db);
  return 0;
}
