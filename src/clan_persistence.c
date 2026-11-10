/* Gated legacy/MySQL writers. Boot hydration/cutover remains Phase 5a. */
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "interpreter.h"
#include "clan.h"
#include "mysql.h"
#include "clan_repository.h"
#include "clan_persistence.h"
#include "clan_legacy_fields.h"
#include <json-c/json.h>
#include <limits.h>
static enum clan_store_mode mode = CLAN_STORE_LEGACY;
static bool ready = true;
static struct json_object* committed;
static struct json_object*
get(struct json_object* o, const char* name)
{
  struct json_object* v = NULL;
  json_object_object_get_ex(o, name, &v);
  return v;
}
static struct json_object*
copy(struct json_object* o)
{
  return json_tokener_parse(json_object_to_json_string_ext(o, JSON_C_TO_STRING_PLAIN));
}
static struct json_object*
by_vnum(struct json_object* root, int vnum)
{
  size_t i;
  struct json_object* a = get(root, "clans");
  for (i = 0; i < json_object_array_length(a); i++)
  {
    struct json_object* e = json_object_array_get_idx(a, i);
    if (json_object_get_int(get(get(e, "core"), "vnum")) == vnum)
      return e;
  }
  return NULL;
}
static const char*
uid_at(struct json_object* root, int vnum)
{
  struct json_object* e = by_vnum(root, vnum);
  return e ? json_object_get_string(get(get(e, "core"), "uid")) : NULL;
}
static struct json_object*
world_snapshot(int64_t now)
{
  struct json_object *root = json_object_new_object(), *clans = json_object_new_array(),
                     *claims = json_object_new_array();
  int i, j;
  struct claim_data* claim;
  json_object_object_add(root, "clans", clans);
  json_object_object_add(root, "claims", claims);
  json_object_object_add(root, "members", copy(get(committed, "members")));
  json_object_object_add(root, "written_at", json_object_new_int64(now));
  for (i = 0; i < num_of_clans; i++)
  {
    struct clan_data* c = &clan_list[i];
    struct clan_repository_record r;
    struct json_object *old = by_vnum(committed, c->vnum), *e = json_object_new_object(),
                       *legacy = json_object_new_object(), *ranks = json_object_new_array(),
                       *priv = json_object_new_array(), *edges = json_object_new_array();
    memset(&r, 0, sizeof(r));
    if (old)
    {
      if (!clan_repository_record_parse(get(old, "core"), &r))
        goto invalid;
    }
    else
    {
      if (!clan_repository_uid_generate(r.uid))
        goto invalid;
      r.status = CLAN_RECORD_ACTIVE;
      r.vnum = c->vnum;
      r.created_at = now;
    }
    if (!c->clan_name || strlen(c->clan_name) >= sizeof(r.name) ||
        (c->description && strlen(c->description) >= sizeof(r.description)) ||
        (c->abrev && strlen(c->abrev) >= sizeof(r.abbreviation)))
      goto invalid;
    strcpy(r.name, c->clan_name);
    strcpy(r.description, c->description ? c->description : "");
    strcpy(r.abbreviation, c->abrev ? c->abrev : "");
    r.leader_id = c->leader;
    r.treasury_gold = c->treasure;
    r.founded_at = c->date_founded;
    r.last_activity_at = c->last_activity;
    r.updated_at = now;
    r.state_updated_at = now;
#define ADD(name, member, tag)                                                                     \
  json_object_object_add(legacy, #name, json_object_new_int64(c->member));
    CLAN_LEGACY_FIELDS(ADD)
#undef ADD
    for (j = 0; j < c->ranks && j < MAX_CLANRANKS; j++)
    {
      if (!c->rank_name[j])
        goto invalid;
      json_object_array_add(ranks, json_object_new_string(c->rank_name[j]));
    }
    for (j = 0; j < NUM_CLAN_PRIVS; j++)
    {
      struct json_object* p = json_object_new_object();
      json_object_object_add(p, "id", json_object_new_int(j));
      json_object_object_add(p, "rank", json_object_new_int(c->privilege[j]));
      json_object_array_add(priv, p);
    }
    json_object_object_add(legacy, "ranks", ranks);
    json_object_object_add(legacy, "privileges", priv);
    json_object_object_add(legacy, "relationships", edges);
    json_object_object_add(e, "core", clan_repository_record_json(&r));
    json_object_object_add(e, "legacy", legacy);
    json_object_array_add(clans, e);
    continue;
  invalid:
    json_object_put(e);
    json_object_put(legacy);
    json_object_put(ranks);
    json_object_put(priv);
    json_object_put(edges);
    json_object_put(root);
    return NULL;
  }
  for (i = 0; i < num_of_clans; i++)
  {
    struct json_object *e = json_object_array_get_idx(clans, (size_t)i),
                       *edges = get(get(e, "legacy"), "relationships");
    for (j = 0; j < MAX_CLANS; j++)
      if (clan_list[i].allies[j] || clan_list[i].at_war[j])
      {
        struct json_object* edge = json_object_new_object();
        const char* uid;
        if (j >= num_of_clans || !(uid = uid_at(root, clan_list[j].vnum)))
        {
          json_object_put(edge);
          json_object_put(root);
          return NULL;
        }
        json_object_object_add(edge, "peer_uid", json_object_new_string(uid));
        json_object_object_add(edge, "allied", json_object_new_boolean(clan_list[i].allies[j]));
        json_object_object_add(edge, "at_war", json_object_new_boolean(clan_list[i].at_war[j]));
        json_object_array_add(edges, edge);
      }
  }
  for (claim = claim_list; claim; claim = claim->next)
  {
    struct json_object *e = json_object_new_object(), *pop = json_object_new_array();
    const char* controller = uid_at(root, claim->clan);
    if (claim->clan != NO_CLAN && claim->clan != 0 && !controller)
    {
      json_object_put(e);
      json_object_put(pop);
      json_object_put(root);
      return NULL;
    }
    json_object_object_add(e, "zone_vnum", json_object_new_int(claim->zn));
    json_object_object_add(
      e, "controller_uid", controller ? json_object_new_string(controller) : NULL);
    json_object_object_add(e, "claimant_id", json_object_new_int64(claim->claimant));
    json_object_object_add(e, "popularity", pop);
    for (j = 0; j < MAX_CLANS; j++)
      if (claim->popularity[j] != 0)
      {
        struct json_object* p = json_object_new_object();
        const char* uid;
        if (j >= num_of_clans || !(uid = uid_at(root, clan_list[j].vnum)))
        {
          json_object_put(p);
          json_object_put(e);
          json_object_put(root);
          return NULL;
        }
        json_object_object_add(p, "clan_uid", json_object_new_string(uid));
        json_object_object_add(p, "percentage", json_object_new_double(claim->popularity[j]));
        json_object_array_add(pop, p);
      }
    json_object_array_add(claims, e);
  }
  if (mode == CLAN_STORE_MYSQL && ready)
  {
    struct char_data* ch;
    for (ch = character_list; ch; ch = ch->next)
    {
      struct json_object *members = get(root, "members"), *old = NULL, *replacement;
      size_t k;
      const char* uid;
      if (IS_NPC(ch) || GET_PFILEPOS(ch) < 0)
        continue;
      uid = (GET_CLAN(ch) == 0 || GET_CLAN(ch) == NO_CLAN) ? NULL : uid_at(root, GET_CLAN(ch));
      if (GET_CLAN(ch) != 0 && GET_CLAN(ch) != NO_CLAN && !uid)
      {
        json_object_put(root);
        return NULL;
      }
      for (k = 0; k < json_object_array_length(members); k++)
        if (json_object_get_int64(get(json_object_array_get_idx(members, k), "player_id")) ==
            GET_IDNUM(ch))
        {
          old = json_object_array_get_idx(members, k);
          break;
        }
      if (!uid)
      {
        if (old)
          json_object_array_del_idx(members, k, 1);
        continue;
      }
      replacement = json_object_new_object();
      json_object_object_add(replacement, "player_id", json_object_new_int64(GET_IDNUM(ch)));
      json_object_object_add(replacement, "clan_uid", json_object_new_string(uid));
      json_object_object_add(replacement, "rank_id", json_object_new_int(GET_CLANRANK(ch)));
      json_object_object_add(replacement,
                             "legacy_points",
                             old ? json_object_get(get(old, "legacy_points"))
                                 : json_object_new_int(0));
      if (old)
        json_object_array_put_idx(members, k, replacement);
      else
        json_object_array_add(members, replacement);
    }
  }
  if (!clan_repository_snapshot_canonicalize(root))
  {
    json_object_put(root);
    return NULL;
  }
  return root;
}
bool
clan_persistence_sql_authority(void)
{
  return mode == CLAN_STORE_MYSQL;
}
bool
clan_persistence_ready(void)
{
  return mode != CLAN_STORE_MYSQL || ready;
}
bool
clan_persistence_configure(enum clan_store_mode requested)
{
  MYSQL_POOL_CONN* connection;
  struct json_object *root = NULL, *candidate;
  enum clan_repository_result status;
  size_t i;
  if (requested < CLAN_STORE_LEGACY || requested > CLAN_STORE_MYSQL)
    return false;
  if (mode == CLAN_STORE_MYSQL && requested != CLAN_STORE_MYSQL)
    return false; /* Never reopen stale files, including after failed attachment. */
  if (mode == CLAN_STORE_MYSQL && ready)
    return true;
  if (requested == CLAN_STORE_MYSQL)
  {
    mode = requested;
    ready = false; /* Pin authority BEFORE contacting storage. */
  }
  if (requested == CLAN_STORE_LEGACY)
  {
    mode = requested;
    return true;
  }
  if (!mysql_available || !(connection = mysql_pool_acquire()))
    return false;
  status = clan_repository_snapshot_read(connection->conn, &root);
  mysql_pool_release(connection);
  if (status != CLAN_REPO_OK)
    return false;
  if (requested == CLAN_STORE_MYSQL &&
      json_object_get_int(get(root, "store_mode")) != CLAN_STORE_MYSQL)
  {
    json_object_put(root);
    return false;
  }
  if (committed)
    json_object_put(committed);
  committed = root;
  candidate = world_snapshot((int64_t)time(NULL));
  if (!candidate)
  {
    json_object_put(committed);
    committed = NULL;
    return false;
  }
  /* Attach only already-reconciled metadata. No boot import or invented IDs. */
  if (json_object_array_length(get(root, "clans")) !=
      json_object_array_length(get(candidate, "clans")))
  {
    json_object_put(candidate);
    return false;
  }
  for (i = 0; i < json_object_array_length(get(root, "clans")); i++)
  {
    struct json_object *old = json_object_array_get_idx(get(root, "clans"), i),
                       *built =
                         by_vnum(candidate, json_object_get_int(get(get(old, "core"), "vnum")));
    struct clan_repository_record a, b;
    if (!built || !clan_repository_record_parse(get(old, "core"), &a) ||
        !clan_repository_record_parse(get(built, "core"), &b) || strcmp(a.name, b.name) ||
        strcmp(a.description, b.description) || strcmp(a.abbreviation, b.abbreviation) ||
        a.leader_id != b.leader_id || a.treasury_gold != b.treasury_gold ||
        a.founded_at != b.founded_at || a.last_activity_at != b.last_activity_at ||
        !json_object_equal(get(old, "legacy"), get(built, "legacy")))
    {
      json_object_put(candidate);
      return false;
    }
  }
  if (!json_object_equal(get(root, "claims"), get(candidate, "claims")))
  {
    json_object_put(candidate);
    return false;
  }
  json_object_put(candidate);
  mode = requested;
  ready = true;
  if (mode == CLAN_STORE_MYSQL)
  {
    struct char_data* ch;
    int p;
    for (p = 0; p <= top_of_p_table; p++)
    {
      size_t n;
      player_table[p].clan = NO_CLAN;
      for (n = 0; n < json_object_array_length(get(committed, "members")); n++)
      {
        struct json_object* m = json_object_array_get_idx(get(committed, "members"), n);
        if (json_object_get_int64(get(m, "player_id")) == player_table[p].id)
        {
          size_t k;
          for (k = 0; k < json_object_array_length(get(committed, "clans")); k++)
          {
            struct json_object* core =
              get(json_object_array_get_idx(get(committed, "clans"), k), "core");
            if (!strcmp(json_object_get_string(get(core, "uid")),
                        json_object_get_string(get(m, "clan_uid"))))
            {
              player_table[p].clan = json_object_get_int(get(core, "vnum"));
              break;
            }
          }
          break;
        }
      }
    }
    for (ch = character_list; ch; ch = ch->next)
      clan_persistence_reconcile_player(ch);
  }
  return true;
}
static bool
write_snapshot(struct json_object* candidate)
{
  MYSQL_POOL_CONN* connection;
  enum clan_repository_result status;
  int64_t revision;
  size_t i;
  if (!candidate)
  {
    ready = false;
    return false;
  }
  if (!ready || !mysql_available || !(connection = mysql_pool_acquire()))
  {
    ready = false;
    json_object_put(candidate);
    return false;
  }
  revision = json_object_get_int64(get(committed, "store_revision"));
  status = clan_repository_snapshot_write(connection->conn, candidate, revision, NULL);
  mysql_pool_release(connection);
  if (status != CLAN_REPO_OK)
  {
    ready = false;
    log("CLAN MYSQL: write rejected: %s; no legacy fallback", clan_repository_result_name(status));
    json_object_put(candidate);
    return false;
  }
  for (i = 0; i < json_object_array_length(get(candidate, "clans")); i++)
  {
    struct json_object *e = json_object_array_get_idx(get(candidate, "clans"), i),
                       *core = get(e, "core");
    int vnum = json_object_get_int(get(core, "vnum"));
    if (by_vnum(committed, vnum))
    {
      json_object_object_add(
        core, "revision", json_object_new_int64(json_object_get_int64(get(core, "revision")) + 1));
      json_object_object_add(
        core,
        "state_revision",
        json_object_new_int64(json_object_get_int64(get(core, "state_revision")) + 1));
    }
  }
  json_object_object_add(candidate, "store_revision", json_object_new_int64(revision + 1));
  json_object_object_add(candidate, "store_mode", json_object_new_int(CLAN_STORE_MYSQL));
  json_object_put(committed);
  committed = candidate;
  for (i = 0; i < (size_t)num_of_clans; i++)
    clan_list[i].modified = FALSE;
  return true;
}
bool
clan_persistence_save(void)
{
  if (mode != CLAN_STORE_MYSQL)
    return false;
  if (!ready)
    return false;
  return write_snapshot(world_snapshot((int64_t)time(NULL)));
}
static struct json_object*
member(struct json_object* root, int64_t id)
{
  size_t i;
  struct json_object* members = get(root, "members");
  for (i = 0; i < json_object_array_length(members); i++)
  {
    struct json_object* m = json_object_array_get_idx(members, i);
    if (json_object_get_int64(get(m, "player_id")) == id)
      return m;
  }
  return NULL;
}
bool
clan_persistence_member_update(int64_t id, int vnum, int rank)
{
  struct json_object *candidate, *members, *old, *replacement = NULL;
  const char* uid;
  size_t i;
  if (mode != CLAN_STORE_MYSQL)
    return true;
  if (!ready)
    return false;
  uid = (vnum == 0 || vnum == NO_CLAN) ? NULL : uid_at(committed, vnum);
  if (vnum != 0 && vnum != NO_CLAN && !uid)
    return false;
  old = member(committed, id);
  if ((!old && !uid) || (old && uid && !strcmp(uid, json_object_get_string(get(old, "clan_uid"))) &&
                         rank == json_object_get_int(get(old, "rank_id"))))
    return true;
  candidate = world_snapshot((int64_t)time(NULL));
  if (!candidate)
    return false;
  members = json_object_new_array();
  for (i = 0; i < json_object_array_length(get(candidate, "members")); i++)
  {
    struct json_object* m = json_object_array_get_idx(get(candidate, "members"), i);
    if (json_object_get_int64(get(m, "player_id")) != id)
      json_object_array_add(members, copy(m));
  }
  if (uid)
  {
    replacement = json_object_new_object();
    json_object_object_add(replacement, "player_id", json_object_new_int64(id));
    json_object_object_add(replacement, "clan_uid", json_object_new_string(uid));
    json_object_object_add(replacement, "rank_id", json_object_new_int(rank));
    json_object_object_add(replacement,
                           "legacy_points",
                           old ? json_object_get(get(old, "legacy_points"))
                               : json_object_new_int(0));
    json_object_array_add(members, replacement);
  }
  json_object_object_add(candidate, "members", members);
  return write_snapshot(candidate);
}
void
clan_persistence_reconcile_player(struct char_data* ch)
{
  struct json_object* m;
  size_t i;
  int vnum = NO_CLAN, rank = 0;
  if (mode != CLAN_STORE_MYSQL || !ch || IS_NPC(ch))
    return;
  m = ready ? member(committed, GET_IDNUM(ch)) : NULL;
  if (m)
    for (i = 0; i < json_object_array_length(get(committed, "clans")); i++)
    {
      struct json_object* core = get(json_object_array_get_idx(get(committed, "clans"), i), "core");
      if (!strcmp(json_object_get_string(get(core, "uid")),
                  json_object_get_string(get(m, "clan_uid"))))
      {
        vnum = json_object_get_int(get(core, "vnum"));
        rank = json_object_get_int(get(m, "rank_id"));
        break;
      }
    }
  GET_CLAN(ch) = vnum;
  GET_CLANRANK(ch) = rank; /* Personal clan points are NOT Favor. */
  if (GET_PFILEPOS(ch) >= 0 && GET_PFILEPOS(ch) <= top_of_p_table)
    player_table[GET_PFILEPOS(ch)].clan = vnum;
}
bool
clan_persistence_before_player_save(struct char_data* ch)
{
  if (mode != CLAN_STORE_MYSQL || IS_NPC(ch))
    return true;
  if (!ready)
  {
    /* An SQL outage must not prevent unrelated character progress from saving.
     * Unchanged committed mirrors (or disabled/empty mirrors) are not writes to
     * membership authority. Reject attempted changes, not ordinary saves. */
    struct json_object* old = committed ? member(committed, GET_IDNUM(ch)) : NULL;
    const char* uid = (committed && (GET_CLAN(ch) != 0 && GET_CLAN(ch) != NO_CLAN))
                        ? uid_at(committed, GET_CLAN(ch))
                        : NULL;
    if ((GET_CLAN(ch) == 0 || GET_CLAN(ch) == NO_CLAN) && GET_CLANRANK(ch) == 0)
      return true;
    if (old && uid && !strcmp(uid, json_object_get_string(get(old, "clan_uid"))) &&
        GET_CLANRANK(ch) == json_object_get_int(get(old, "rank_id")))
      return true;
  }
  if (clan_persistence_member_update(GET_IDNUM(ch), GET_CLAN(ch), GET_CLANRANK(ch)))
    return true;
  clan_persistence_reconcile_player(ch);
  send_to_char(ch, "Clan storage is unavailable; membership was not saved.\r\n");
  return false;
}
bool
clan_persistence_access(struct char_data* ch)
{
  if (clan_persistence_ready())
    return true;
  if (ch)
    send_to_char(ch, "Clan storage is unavailable. No legacy data will be used.\r\n");
  return false;
}
bool
clan_persistence_economy_available(struct char_data* ch)
{
  if (mode != CLAN_STORE_MYSQL)
    return true;
  if (ch)
    send_to_char(ch,
                 "Clan gold operations are unavailable until durable SQL/player-file receipts are "
                 "enabled.\r\n");
  return false;
}
