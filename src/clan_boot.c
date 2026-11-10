/* Explicit clan boot authority. No implicit import, cutover or legacy fallback.
 */
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "clan.h"
#include "mysql.h"
#include "clan_repository.h"
#include "clan_persistence.h"
#include "clan_legacy_fields.h"
#include "dotenv.h"
#include <json-c/json.h>
#include <limits.h>

static struct clan_buff_state
  timers[CLAN_PROGRESSION_MAX_PILOT_CLANS * CLAN_PROGRESSION_BUFF_COUNT];
static size_t timer_count;
bool clan_persistence_boot_environment(void)
{
  const char *selection = getenv("CLAN_STORE_MODE");
  if (!selection) {
    selection = get_env_value("CLAN_STORE_MODE");
    if (!*selection) selection = NULL;
  }
  return clan_persistence_boot(selection);
}
static struct json_object*
field(struct json_object* o, const char* key)
{
  struct json_object* v = NULL;
  json_object_object_get_ex(o, key, &v);
  return v;
}
static int
uid_index(struct json_object* clans, const char* uid)
{
  size_t i;
  if (!uid)
    return -1;
  for (i = 0; i < json_object_array_length(clans); i++)
    if (!strcmp(json_object_get_string(field(
                  field(json_object_array_get_idx(clans, i), "core"), "uid")),
                uid))
      return (int)i;
  return -1;
}
static void
release_clans(struct clan_data* clans, size_t n)
{
  size_t i, j;
  for (i = 0; i < n; i++) {
    free(clans[i].clan_name);
    free(clans[i].description);
    free(clans[i].abrev);
    for (j = 0; j < MAX_CLANRANKS; j++)
      free(clans[i].rank_name[j]);
  }
  free(clans);
}
/* No stat effects are connected. Future services must additionally check
 * current membership, slot/rules eligibility, clock, feature gates and storage
 * health. */
bool
clan_persistence_boot_buff(size_t index, struct clan_buff_state* out)
{
  if (!out || !clan_persistence_sql_authority() || !clan_persistence_ready() ||
      index >= timer_count)
    return false;
  *out = timers[index];
  return true;
}
bool
clan_persistence_boot(const char* selection)
{
  enum clan_store_mode requested;
  MYSQL_POOL_CONN* connection;
  struct json_object *root = NULL, *clans, *claims;
  struct clan_data* loaded = NULL;
  struct claim_data *head = NULL, *tail = NULL, *next;
  struct clan_buff_state
    pending[CLAN_PROGRESSION_MAX_PILOT_CLANS * CLAN_PROGRESSION_BUFF_COUNT];
  size_t n = 0, i, j, count = 0;
  enum clan_repository_result result;
  if (!selection || !strcmp(selection, "legacy"))
    requested = CLAN_STORE_LEGACY;
  else if (!strcmp(selection, "shadow"))
    requested = CLAN_STORE_MYSQL_SHADOW;
  else if (!strcmp(selection, "mysql"))
    requested = CLAN_STORE_MYSQL;
  else {
    log("CLAN BOOT: invalid CLAN_STORE_MODE; expected legacy, shadow or mysql");
    return false;
  }
  /* This is a boot-only API, never a live authority toggle. */
  {
    struct char_data* ch;
    if (clan_persistence_sql_authority())
      return false;
    for (ch = character_list; ch; ch = ch->next)
      if (!IS_NPC(ch))
        return false;
  }
  if (requested != CLAN_STORE_MYSQL) {
    load_clans();
    load_claims();
    return clan_persistence_configure(requested);
  }
  clan_persistence_pin_mysql();
  timer_count = 0;
  if (!mysql_available || !(connection = mysql_pool_acquire()))
    return false;
  result = clan_repository_snapshot_read(connection->conn, &root);
  if (result == CLAN_REPO_OK)
    result = clan_repository_buffs_read(
      connection->conn, pending, sizeof(pending) / sizeof(*pending), &count);
  mysql_pool_release(connection);
  if (result != CLAN_REPO_OK ||
      json_object_get_int(field(root, "store_mode")) != CLAN_STORE_MYSQL)
    goto failed;
  for (i = 0; i < count; i++)
    for (j = 0; j < i; j++)
      if (!pending[i].canceled && !pending[j].canceled &&
          !strcmp(pending[i].clan_uid, pending[j].clan_uid) &&
          pending[i].slot == pending[j].slot &&
          pending[i].started_at < pending[j].expires_at &&
          pending[j].started_at < pending[i].expires_at)
        goto failed; /* Conflicting occupied slots cannot become benefits. */
  clans = field(root, "clans");
  claims = field(root, "claims");
  n = json_object_array_length(clans);
  if (n > MAX_CLANS)
    goto failed;
  /* SQL memberships must reference surviving stable player identities. Stale
   * clan numbers in the index are mirrors and are reconciled on attachment. */
  {
    struct json_object* members = field(root, "members");
    for (i = 0; i < json_object_array_length(members); i++) {
      int p;
      bool found = false;
      int64_t id = json_object_get_int64(
        field(json_object_array_get_idx(members, i), "player_id"));
      for (p = 0; p <= top_of_p_table; p++)
        if (player_table[p].id == id && player_table[p].name &&
            !IS_SET(player_table[p].flags, PINDEX_DELETED)) {
          found = true;
          break;
        }
      if (!found)
        goto failed;
    }
  }
  loaded = calloc(n ? n : 1, sizeof(*loaded));
  if (!loaded)
    goto failed;
  for (i = 0; i < n; i++) {
    struct json_object *entry = json_object_array_get_idx(clans, i),
                       *legacy = field(entry, "legacy"), *a;
    struct clan_repository_record r;
    struct clan_data* c = &loaded[i];
    if (!clan_repository_record_parse(field(entry, "core"), &r) ||
        r.leader_id > LONG_MAX || r.treasury_gold > LONG_MAX ||
        (sizeof(time_t) < 8 &&
         (r.founded_at > INT32_MAX || r.last_activity_at > INT32_MAX)))
      goto failed;
    c->vnum = r.vnum;
    c->leader = r.leader_id;
    c->treasure = r.treasury_gold;
    c->date_founded = r.founded_at;
    c->last_activity = r.last_activity_at;
    c->clan_name = strdup(r.name);
    c->description = strdup(r.description);
    c->abrev = strdup(r.abbreviation);
    if (!c->clan_name || !c->description || !c->abrev)
      goto failed;
#define FILL(name, member, tag)                                                \
  if (json_object_get_int64(field(legacy, #name)) > LONG_MAX)                  \
    goto failed;                                                               \
  c->member = json_object_get_int64(field(legacy, #name));
    CLAN_LEGACY_FIELDS(FILL)
#undef FILL
    a = field(legacy, "ranks");
    c->ranks = json_object_array_length(a);
    for (j = 0; j < c->ranks; j++) {
      c->rank_name[j] =
        strdup(json_object_get_string(json_object_array_get_idx(a, j)));
      if (!c->rank_name[j])
        goto failed;
    }
    a = field(legacy, "privileges");
    for (j = 0; j < json_object_array_length(a); j++) {
      struct json_object* p = json_object_array_get_idx(a, j);
      c->privilege[json_object_get_int(field(p, "id"))] =
        json_object_get_int(field(p, "rank"));
    }
    a = field(legacy, "relationships");
    for (j = 0; j < json_object_array_length(a); j++) {
      struct json_object* edge = json_object_array_get_idx(a, j);
      int k = uid_index(clans, json_object_get_string(field(edge, "peer_uid")));
      if (k < 0)
        goto failed;
      c->allies[k] = json_object_get_boolean(field(edge, "allied"));
      c->at_war[k] = json_object_get_boolean(field(edge, "at_war"));
    }
  }
  for (i = 0; i < json_object_array_length(claims); i++) {
    struct json_object *e = json_object_array_get_idx(claims, i),
                       *a = field(e, "popularity");
    int k =
      uid_index(clans, json_object_get_string(field(e, "controller_uid")));
    struct claim_data* claim = calloc(1, sizeof(*claim));
    if (!claim)
      goto failed;
    if (tail)
      tail->next = claim;
    else
      head = claim;
    tail = claim;
    if (json_object_get_int64(field(e, "claimant_id")) > LONG_MAX)
      goto failed;
    claim->zn = json_object_get_int(field(e, "zone_vnum"));
    claim->clan = k < 0 ? NO_CLAN : loaded[k].vnum;
    claim->claimant = json_object_get_int64(field(e, "claimant_id"));
    for (j = 0; j < json_object_array_length(a); j++) {
      struct json_object* p = json_object_array_get_idx(a, j);
      k = uid_index(clans, json_object_get_string(field(p, "clan_uid")));
      if (k < 0)
        goto failed;
      claim->popularity[k] =
        (float)json_object_get_double(field(p, "percentage"));
    }
  }
  /* Publish only fully decoded runtime metadata. Attachment rereads and
   * compares to reject concurrent metadata changes; startup stops on any
   * disagreement. */
  for (i = 0; i < (size_t)num_of_clans; i++) {
    free(clan_list[i].description);
    free(clan_list[i].abrev);
  }
  free_clan_list();
  free_claim_list();
  clan_list = loaded;
  loaded = NULL;
  num_of_clans = (int)n;
  claim_list = head;
  head = NULL;
  rebuild_clan_hash();
  if (!clan_persistence_configure(CLAN_STORE_MYSQL))
    goto failed;
  memcpy(timers, pending, count * sizeof(*timers));
  timer_count = count;
  json_object_put(root);
  log("CLAN BOOT: MySQL authoritative; %d clans, %lu retained family timers; "
      "progression remains disabled",
      num_of_clans,
      (unsigned long)count);
  return true;
failed:
  if (loaded)
    release_clans(loaded, n);
  while (head) {
    next = head->next;
    free(head);
    head = next;
  }
  if (root)
    json_object_put(root);
  log("CLAN BOOT: SQL unavailable, inconsistent, or not explicitly cut over; "
      "refusing legacy fallback");
  return false;
}
