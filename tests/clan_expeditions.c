#include "clan_expeditions.h"
#include "clan_contract_rotation.h"
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
static bool area(int a, void *ctx)
{
  struct json_object *list = get(ctx, "areas");
  size_t i;
  for (i = 0; i < json_object_array_length(list); i++)
    if (json_object_get_int(json_object_array_get_idx(list, i)) == a)
      return true;
  return false;
}
static bool ref(int a, int m, void *ctx, const char *key)
{
  struct json_object *list = get(ctx, key);
  size_t i;
  for (i = 0; i < json_object_array_length(list); i++)
  {
    struct json_object *v = json_object_array_get_idx(list, i);
    if (json_object_get_int(get(v, "area")) == a && json_object_get_int(get(v, "mob")) == m)
      return true;
  }
  return false;
}
static bool target(int a, int m, void *ctx)
{
  return ref(a, m, ctx, "targets");
}
static bool available(int a, int m, void *ctx)
{
  return ref(a, m, ctx, "available");
}
int main(int argc, char **argv)
{
  if (argc > 1 && !strcmp(argv[1], "rotation"))
  {
    const char *ids[] = {"d", "b", "a", "c"}, *reordered[] = {"c", "a", "b", "d"};
    int week, seen[4] = {0};
    size_t i, n;
    for (week = 0; week < 104; week++)
    {
      int64_t now = INT64_C(2200000001) + week * INT64_C(604800);
      struct clan_reset_period w;
      assert(clan_progression_week_at(now, &w));
      n = 0;
      for (i = 0; i < 4; i++)
      {
        bool chosen = clan_contract_rotates_in(ids[i], ids, 4, now);
        assert(chosen == clan_contract_rotates_in(ids[i], reordered, 4, now));
        assert(chosen == clan_contract_rotates_in(ids[i], ids, 4, w.starts_at));
        assert(chosen == clan_contract_rotates_in(ids[i], ids, 4, w.ends_at - 1));
        assert(clan_contract_rotates_in(ids[i], ids, 4, w.ends_at) ==
               clan_contract_rotates_in(ids[i], ids, 4, w.ends_at + 604799));
        n += chosen;
        seen[i] += chosen;
      }
      assert(n == 3);
      assert(clan_contract_rotates_in("d", ids, 2, now));
      assert(clan_contract_rotates_in("b", ids, 2, now));
      assert(!clan_contract_rotates_in("missing", ids, 4, now));
    }
    for (i = 0; i < 4; i++)
      assert(seen[i] == 78);
    assert(!clan_contract_rotates_in("a", ids, 4, -1));
    puts("rotation passed");
    return 0;
  }
  {
    char input[65536], error[256];
    size_t n = fread(input, 1, sizeof(input) - 1, stdin);
    struct json_object *root, *result;
    struct clan_expedition_catalog c, unchanged;
    struct clan_expedition_world w = {area, target, available, NULL};
    bool ok;
    input[n] = 0;
    root = json_tokener_parse(input);
    assert(root);
    memset(&c, 0xa5, sizeof(c));
    unchanged = c;
    ok = clan_expedition_catalog_parse(get(root, "catalog"), &c, error, sizeof(error));
    if (!ok)
      assert(!memcmp(&c, &unchanged, sizeof(c)));
    w.context = root;
    if (ok)
      ok = clan_expedition_catalog_validate(&c, &w, error, sizeof(error));
    result = json_object_new_object();
    json_object_object_add(result, "ok", json_object_new_boolean(ok));
    json_object_object_add(result, "error", json_object_new_string(error));
    puts(json_object_to_json_string(result));
    json_object_put(result);
    json_object_put(root);
  }
  return 0;
}
