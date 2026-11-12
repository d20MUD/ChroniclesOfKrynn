#include "clan_expeditions.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
static bool fail(char *error, size_t cap, const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  if (error && cap)
    vsnprintf(error, cap, fmt, ap);
  va_end(ap);
  return false;
}
static struct json_object *get(struct json_object *o, const char *key)
{
  struct json_object *v = NULL;
  if (o)
    json_object_object_get_ex(o, key, &v);
  return v;
}
static bool integer(struct json_object *o, const char *key, int lo, int hi, int *out)
{
  struct json_object *v = get(o, key);
  int64_t n;
  if (!v || json_object_get_type(v) != json_type_int)
    return false;
  n = json_object_get_int64(v);
  if (n < lo || n > hi)
    return false;
  *out = (int)n;
  return true;
}
static bool string(struct json_object *o, const char *key, char *out, size_t cap, bool slug)
{
  struct json_object *v = get(o, key);
  const char *s;
  size_t i, n;
  if (!v || json_object_get_type(v) != json_type_string)
    return false;
  s = json_object_get_string(v);
  n = strlen(s);
  if (!n || n >= cap || n != (size_t)json_object_get_string_len(v))
    return false;
  for (i = 0; i < n; i++)
    if (slug ? !((s[i] >= 'a' && s[i] <= 'z') || (s[i] >= '0' && s[i] <= '9') || s[i] == '-')
             : ((unsigned char)s[i] < 32 || (unsigned char)s[i] > 126))
      return false;
  memcpy(out, s, n + 1);
  return true;
}
static bool same(struct clan_expedition_target a, struct clan_expedition_target b)
{
  /* A prototype belongs to only one primary objective, even across areas. */
  return a.mob == b.mob;
}
bool clan_expedition_catalog_parse(struct json_object *root, struct clan_expedition_catalog *out,
                                   char *error, size_t cap)
{
  struct clan_expedition_catalog c = {0};
  struct json_object *array = get(root, "expeditions");
  size_t i, j, k, x, y;
  int version;
  if (!out || !root || json_object_get_type(root) != json_type_object ||
      json_object_object_length(root) != 2 || !integer(root, "version", 1, 1, &version) || !array ||
      json_object_get_type(array) != json_type_array ||
      !(c.count = json_object_array_length(array)) || c.count > CLAN_EXPEDITION_MAX)
    return fail(error, cap, "expeditions: expected version 1 and 1-%d definitions",
                CLAN_EXPEDITION_MAX);
  for (i = 0; i < c.count; i++)
  {
    struct clan_expedition_definition *d = &c.entries[i];
    struct json_object *o = json_object_array_get_idx(array, i), *objectives = get(o, "objectives");
    if (!o || json_object_get_type(o) != json_type_object || json_object_object_length(o) != 9 ||
        !string(o, "id", d->id, sizeof(d->id), true) ||
        !string(o, "name", d->name, sizeof(d->name), false) ||
        !string(o, "shared_reward_id", d->shared_reward_id, sizeof(d->shared_reward_id), true) ||
        !string(o, "personal_reward_id", d->personal_reward_id, sizeof(d->personal_reward_id),
                true) ||
        !integer(o, "family", 1, 2, &d->family) ||
        !integer(o, "min_clan_level", 1, 20, &d->min_clan_level) ||
        !integer(o, "min_player_level", 1, 30, &d->min_player_level) ||
        !integer(o, "max_player_level", d->min_player_level, 30, &d->max_player_level) ||
        !objectives || json_object_get_type(objectives) != json_type_array ||
        json_object_array_length(objectives) != 3)
      return fail(error, cap, "expedition[%zu] %s: invalid fields/band/objectives", i, d->id);
    /* Reward IDs select fixed policy families, never client-supplied amounts. */
    if (strcmp(d->shared_reward_id, d->family == 1 ? "stronghold-shared-v1" : "hunt-shared-v1") ||
        strcmp(d->personal_reward_id,
               d->family == 1 ? "stronghold-personal-v1" : "hunt-personal-v1"))
      return fail(error, cap, "%s: unknown reward ID", d->id);
    for (j = 0; j < i; j++)
      if (!strcmp(d->id, c.entries[j].id))
        return fail(error, cap, "%s: duplicate ID", d->id);
    for (j = 0; j < 3; j++)
    {
      struct clan_expedition_objective *objective = &d->objectives[j];
      struct json_object *v = json_object_array_get_idx(objectives, j),
                         *targets = get(v, "targets");
      int quota = d->family == 2 ? 10 : j == 0 ? 20 : 1;
      if (!v || json_object_get_type(v) != json_type_object || json_object_object_length(v) != 2 ||
          !integer(v, "quota", quota, quota, &objective->quota) || !targets ||
          json_object_get_type(targets) != json_type_array ||
          !(objective->count = json_object_array_length(targets)) ||
          objective->count > CLAN_EXPEDITION_TARGET_MAX ||
          (d->family == 1 && j > 0 && objective->count < 2))
        return fail(error, cap, "%s objective %zu: invalid quota/alternatives", d->id, j);
      for (k = 0; k < objective->count; k++)
      {
        struct clan_expedition_target *t = &objective->targets[k];
        struct json_object *ref = json_object_array_get_idx(targets, k);
        if (!ref || json_object_get_type(ref) != json_type_object ||
            json_object_object_length(ref) != 2 || !integer(ref, "area", 0, 99999999, &t->area) ||
            !integer(ref, "mob", 1, 99999999, &t->mob))
          return fail(error, cap, "%s objective %zu target %zu: invalid reference", d->id, j, k);
        for (x = 0; x <= j; x++)
          for (y = 0; y < (x == j ? k : d->objectives[x].count); y++)
            if (same(*t, d->objectives[x].targets[y]))
              return fail(error, cap, "%s: overlapping target %d", d->id, t->mob);
        if (d->family == 1 && t->area != d->objectives[0].targets[0].area)
          return fail(error, cap, "%s: stronghold objectives must share one area", d->id);
      }
    }
    if (d->family == 2)
    {
      int area = d->objectives[0].targets[0].area;
      bool different = false;
      for (j = 0; j < 3; j++)
        for (k = 0; k < d->objectives[j].count; k++)
          if (d->objectives[j].targets[k].area != area)
            different = true;
      /* Every possible completion must visit at least two areas. */
      if (!different)
        return fail(error, cap, "%s: hunt requires two areas", d->id);
      for (j = 0; j < d->objectives[0].count; j++)
      {
        int candidate = d->objectives[0].targets[j].area;
        bool in_second = false, in_third = false;
        for (k = 0; k < d->objectives[1].count; k++)
          if (d->objectives[1].targets[k].area == candidate)
            in_second = true;
        for (k = 0; k < d->objectives[2].count; k++)
          if (d->objectives[2].targets[k].area == candidate)
            in_third = true;
        if (in_second && in_third)
          return fail(error, cap, "%s: hunt can be completed in one area", d->id);
      }
    }
  }
  *out = c;
  if (error && cap)
    *error = 0;
  return true;
}
bool clan_expedition_catalog_validate(const struct clan_expedition_catalog *c,
                                      const struct clan_expedition_world *w, char *error,
                                      size_t cap)
{
  size_t i, j, k;
  if (!c || !c->count || c->count > CLAN_EXPEDITION_MAX || !w || !w->area || !w->target ||
      !w->available)
    return fail(error, cap, "expeditions: world validation unavailable");
  for (i = 0; i < c->count; i++)
    for (j = 0; j < 3; j++)
    {
      bool available = false;
      const struct clan_expedition_objective *o = &c->entries[i].objectives[j];
      if (!o->count || o->count > CLAN_EXPEDITION_TARGET_MAX)
        return fail(error, cap, "%s: invalid target count", c->entries[i].id);
      for (k = 0; k < o->count; k++)
      {
        struct clan_expedition_target t = o->targets[k];
        if (!w->area(t.area, w->context) || !w->target(t.area, t.mob, w->context))
          return fail(error, cap, "%s objective %zu: missing area/reset target %d/%d",
                      c->entries[i].id, j, t.area, t.mob);
        if (w->available(t.area, t.mob, w->context))
          available = true;
      }
      if (!available)
        return fail(error, cap, "%s objective %zu: no available alternative", c->entries[i].id, j);
    }
  if (error && cap)
    *error = 0;
  return true;
}

struct json_object *clan_expedition_definition_json(const struct clan_expedition_definition *d)
{
  struct json_object *o, *a;
  size_t i, j;
  if (!d || !memchr(d->id, 0, sizeof(d->id)) || !memchr(d->name, 0, sizeof(d->name)) ||
      !memchr(d->shared_reward_id, 0, sizeof(d->shared_reward_id)) ||
      !memchr(d->personal_reward_id, 0, sizeof(d->personal_reward_id)))
    return NULL;
  o = json_object_new_object();
  a = json_object_new_array();
#define S(k) json_object_object_add(o, #k, json_object_new_string(d->k))
#define N(k) json_object_object_add(o, #k, json_object_new_int(d->k))
  S(id);
  S(name);
  S(shared_reward_id);
  S(personal_reward_id);
  N(family);
  N(min_clan_level);
  N(min_player_level);
  N(max_player_level);
#undef S
#undef N
  json_object_object_add(o, "objectives", a);
  for (i = 0; i < 3; i++)
  {
    struct json_object *v = json_object_new_object(), *targets = json_object_new_array();
    if (!d->objectives[i].count || d->objectives[i].count > CLAN_EXPEDITION_TARGET_MAX)
    {
      json_object_put(v);
      json_object_put(targets);
      json_object_put(o);
      return NULL;
    }
    json_object_object_add(v, "quota", json_object_new_int(d->objectives[i].quota));
    json_object_object_add(v, "targets", targets);
    json_object_array_add(a, v);
    for (j = 0; j < d->objectives[i].count; j++)
    {
      struct json_object *t = json_object_new_object();
      json_object_object_add(t, "area", json_object_new_int(d->objectives[i].targets[j].area));
      json_object_object_add(t, "mob", json_object_new_int(d->objectives[i].targets[j].mob));
      json_object_array_add(targets, t);
    }
  }
  return o;
}
