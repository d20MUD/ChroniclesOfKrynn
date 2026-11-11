/* Checked legacy/MySQL writers; explicit boot hydration lives in clan_boot.c. */
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
static void (*member_invalidation)(struct char_data *, const char *);
void clan_persistence_set_member_invalidation(void (*callback)(struct char_data *,const char *))
{member_invalidation=callback;}
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
world_snapshot_extra(int64_t now, struct clan_data *extra)
{
  struct json_object *root = json_object_new_object(), *clans = json_object_new_array(),
                     *claims = json_object_new_array();
  int i, j;
  struct claim_data* claim;
  json_object_object_add(root, "clans", clans);
  json_object_object_add(root, "claims", claims);
  json_object_object_add(root, "members", copy(get(committed, "members")));
  json_object_object_add(root, "written_at", json_object_new_int64(now));
  for (i = 0; i < num_of_clans + (extra ? 1 : 0); i++)
  {
    struct clan_data* c = i < num_of_clans ? &clan_list[i] : extra;
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
static struct json_object *world_snapshot(int64_t now)
{ return world_snapshot_extra(now, NULL); }

void
clan_persistence_pin_mysql(void)
{
  mode = CLAN_STORE_MYSQL;
  ready = false;
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
  /* Membership replacements may append a lower player ID. Keep the cached
   * committed ordering identical to checked SQL reads before later refreshes. */
  if (!clan_repository_snapshot_canonicalize(candidate)) {
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
clan_persistence_refresh_progression(void)
{
  MYSQL_POOL_CONN *connection;
  struct json_object *root = NULL, *expected = NULL;
  enum clan_repository_result status;
  size_t i, j;
  static const char *fields[] = {
    "xp_eighths", "influence_eighths", "state_revision", "state_updated_at"
  };
  if (mode != CLAN_STORE_MYSQL) return true;
  if (!committed || !mysql_available || !(connection = mysql_pool_acquire())) goto fail;
  status = clan_repository_snapshot_read(connection->conn, &root);
  mysql_pool_release(connection);
  if (status != CLAN_REPO_OK) goto fail;
  expected = copy(committed);
  if (!expected) goto fail;
  for (i = 0; i < json_object_array_length(get(root, "clans")); i++) {
    struct json_object *entry = json_object_array_get_idx(get(root, "clans"), i);
    struct json_object *core = get(entry, "core"), *old;
    struct clan_repository_record before, after;
    old = get(by_vnum(expected, json_object_get_int(get(core, "vnum"))), "core");
    if (!old || !clan_repository_record_parse(old, &before) ||
        !clan_repository_record_parse(core, &after) ||
        after.state_revision < before.state_revision ||
        after.state_updated_at < before.state_updated_at ||
        (after.state_revision == before.state_revision &&
         (after.xp_eighths != before.xp_eighths ||
          after.influence_eighths != before.influence_eighths ||
          after.state_updated_at != before.state_updated_at))) goto fail;
    for (j = 0; j < sizeof(fields)/sizeof(fields[0]); j++)
      json_object_object_add(old, fields[j], json_object_get(get(core, fields[j])));
  }
  /* written_at is a proposed save timestamp, not authoritative stored metadata. */
  json_object_object_del(expected, "written_at");
  if (!json_object_equal(expected, root)) goto fail;
  json_object_put(expected);
  json_object_put(committed);
  committed = root;
  ready = true;
  return true;
fail:
  if (expected) json_object_put(expected);
  if (root) json_object_put(root);
  ready = false;
  return false;
}
bool
clan_persistence_save(void)
{
  if (mode != CLAN_STORE_MYSQL)
    return false;
  if (!ready || !clan_persistence_refresh_progression())
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
  if (!clan_persistence_refresh_progression())
    return false;
  uid = (vnum == 0 || vnum == (int)NO_CLAN) ? NULL : uid_at(committed, vnum);
  if (vnum != 0 && vnum != (int)NO_CLAN && !uid)
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
  if (member_invalidation)
    member_invalidation(ch, m && rank>0 ? json_object_get_string(get(m, "clan_uid")) : NULL);
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
bool clan_persistence_progression_access(struct char_data *ch, const char *uid)
{
  MYSQL_POOL_CONN *connection;
  enum clan_member_access access;
  enum clan_repository_result status;
  if (!ch || IS_NPC(ch) || mode != CLAN_STORE_MYSQL || !ready ||
      !clan_progression_uid_valid(uid)) return false;
  if (!mysql_available || !(connection = mysql_pool_acquire())) {
    ready = false;
    return false;
  }
  status = clan_repository_member_access(connection->conn, GET_IDNUM(ch), uid, &access);
  mysql_pool_release(connection);
  if (status != CLAN_REPO_OK) {
    if (status != CLAN_REPO_NOT_FOUND) ready = false;
    return false;
  }
  return access == CLAN_ACCESS_ACCEPTED;
}
bool clan_persistence_create(struct clan_data *draft, struct char_data *leader)
{
  struct json_object *candidate, *membership;
  const char *uid;
  int i;
  if (mode != CLAN_STORE_MYSQL || !ready || !draft || !leader || IS_NPC(leader) ||
      GET_IDNUM(leader) <= 0 || num_of_clans >= MAX_CLANS ||
      draft->leader != GET_IDNUM(leader) || !clan_persistence_refresh_progression() ||
      by_vnum(committed, draft->vnum) || member(committed, GET_IDNUM(leader))) return false;
  for (i = 0; i < MAX_CLANS; i++)
    if (draft->allies[i] || draft->at_war[i]) return false;
  clan_persistence_reconcile_player(leader);
  candidate = world_snapshot_extra((int64_t)time(NULL), draft);
  if (!candidate) return false;
  uid = uid_at(candidate, draft->vnum);
  if (!uid) { json_object_put(candidate);return false; }
  membership = json_object_new_object();
  json_object_object_add(membership, "player_id", json_object_new_int64(GET_IDNUM(leader)));
  json_object_object_add(membership, "clan_uid", json_object_new_string(uid));
  json_object_object_add(membership, "rank_id", json_object_new_int(1));
  json_object_object_add(membership, "legacy_points", json_object_new_int(0));
  json_object_array_add(get(candidate, "members"), membership);
  return write_snapshot(candidate);
}
bool clan_persistence_disband(int vnum)
{
  struct json_object *candidate;
  struct char_data *ch;
  const char *uid;
  int p;
  if (mode != CLAN_STORE_MYSQL || !ready || !clan_persistence_refresh_progression()) return false;
  uid = uid_at(committed, vnum);
  if (!uid) return false;
  candidate = world_snapshot((int64_t)time(NULL));
  if (!candidate) return false;
  if (!clan_repository_snapshot_disband(candidate, uid)) {
    json_object_put(candidate);
    return false;
  }
  if (!write_snapshot(candidate)) return false;
  /* SQL is authoritative. Offline player files are reconciled on load; do not
   * make an SQL tombstone depend on writing every character file. */
  for (p = 0; p <= top_of_p_table; p++)
    if (player_table[p].clan == vnum) player_table[p].clan = NO_CLAN;
  for (ch = character_list; ch; ch = ch->next) {
    if (!IS_NPC(ch) && GET_CLAN(ch) == (clan_vnum)vnum)
      send_to_char(ch, "Your clan has been disbanded. You are no longer a member.\r\n");
    clan_persistence_reconcile_player(ch);
  }
  return true;
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
