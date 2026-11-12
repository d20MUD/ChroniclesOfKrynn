#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "mysql.h"
#include "clan.h"
#include "clan_buff_runtime.h"
#include "clan_persistence.h"
#include "clan_expedition_enrollment.h"
#include "crafting_new.h"
#include <assert.h>
static struct clan_progression_config policy;
static MYSQL_POOL_CONN connection;
static bool busy, quartermaster = true;
static char messages[65536];
static unsigned sequence = 100000;
struct room_data *world;
room_rnum top_of_world;
struct player_special_data dummy_mob;
bool mysql_available = true;
time_t expedition_test_time(time_t *out)
{
  if (out)
    *out = 2200610101;
  return 2200610101;
}
const struct clan_progression_config *clan_buff_runtime_policy(void)
{
  return &policy;
}
bool clan_persistence_sql_authority(void)
{
  return true;
}
bool clan_persistence_ready(void)
{
  return true;
}
bool clan_persistence_refresh_progression(void)
{
  assert(!busy);
  return true;
}
MYSQL_POOL_CONN *mysql_pool_acquire(void)
{
  assert(!busy);
  busy = true;
  return &connection;
}
void mysql_pool_release(MYSQL_POOL_CONN *c)
{
  assert(c == &connection && busy);
  busy = false;
}
bool legendary_uuid_generate(char out[33])
{
  snprintf(out, 33, "%032x", ++sequence);
  return true;
}
size_t send_to_char(struct char_data *ch, const char *fmt, ...)
{
  va_list ap;
  int n;
  (void)ch;
  va_start(ap, fmt);
  n = vsnprintf(messages + strlen(messages), sizeof(messages) - strlen(messages), fmt, ap);
  va_end(ap);
  return n < 0 ? 0 : (size_t)n;
}
void basic_mud_log(const char *fmt, ...)
{
  (void)fmt;
}
void PERF_prof_sect_init(struct PERF_prof_sect **p, const char *s)
{
  (void)p;
  (void)s;
}
void PERF_prof_sect_enter(struct PERF_prof_sect *p)
{
  (void)p;
}
void PERF_prof_sect_exit(struct PERF_prof_sect *p)
{
  (void)p;
}
bool has_quartermaster_in_room(struct char_data *ch)
{
  (void)ch;
  return quartermaster;
}
static bool area(int a, void *c)
{
  (void)a;
  (void)c;
  return true;
}
static bool target(int a, int m, void *c)
{
  (void)a;
  (void)m;
  (void)c;
  return true;
}
struct clan_expedition_world clan_expedition_world_refs(void)
{
  struct clan_expedition_world refs = {area, target, target, NULL};
  return refs;
}
static void command(struct char_data *ch, const char *s, const char *expected)
{
  *messages = 0;
  do_clanexpeditions(ch, (char *)s, 0, 0);
  assert(strstr(messages, expected));
  assert(!busy);
}
int main(int argc, char **argv)
{
  struct char_data leader = {0}, ally = {0};
  struct player_special_data ps = {0}, aps = {0};
  struct descriptor_data desc = {0}, ad = {0};
  struct account_data account = {0}, aa = {0};
  struct room_data rooms[1] = {{0}};
  struct json_object *outing = NULL;
  char uid[33], join[80];
  assert(argc == 2 || argc == 3);
  connection.conn = mysql_init(NULL);
  assert(mysql_real_connect(connection.conn, NULL, "root", "", "clan_test", 0, argv[1], 0));
  policy = clan_progression_defaults;
  policy.controls.store_mode = CLAN_STORE_MYSQL;
  policy.controls.mode = CLAN_PROGRESSION_LIVE;
  policy.controls.public_release = true;
  policy.controls.contracts = true;
  world = rooms;
  world[0].people = &leader;
  leader.next_in_room = &ally;
  leader.player_specials = &ps;
  leader.desc = &desc;
  desc.character = &leader;
  desc.account = &account;
  account.id = 77;
  STATE(&desc) = CON_PLAYING;
  ally.player_specials = &aps;
  ally.desc = &ad;
  ad.character = &ally;
  ad.account = &aa;
  aa.id = 80;
  STATE(&ad) = CON_PLAYING;
  GET_IDNUM((&leader)) = 5000000007;
  GET_IDNUM((&ally)) = 5000000010;
  GET_LEVEL((&leader)) = GET_LEVEL((&ally)) = 27;
  IN_ROOM((&leader)) = IN_ROOM((&ally)) = 0;
  GET_POS((&leader)) = GET_POS((&ally)) = POS_STANDING;
  if (argc == 3)
  {
    char claim[80];
    policy.controls.expeditions = true;
    command(&leader, "claims", argv[2]);
    snprintf(claim, sizeof(claim), "claim %s", argv[2]);
    command(&leader, claim, "Favor credited");
    command(&leader, claim, "Favor credited");
    mysql_close(connection.conn);
    puts("Expedition claim commands: completion discovery, exact Favor, replay and safe SQL-pool "
         "release passed.");
    return 0;
  }
  command(&leader, "list", "unavailable");
  policy.controls.expeditions = true;
  command(&leader, "list", "icewall-assault-v1");
  command(&leader, "create", "Usage");
  quartermaster = false;
  command(&leader, "create icewall-assault-v1", "rejected");
  quartermaster = true;
  command(&leader, "create icewall-assault-v1", "created");
  assert(clan_expedition_read(connection.conn, "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", &outing) ==
         CLAN_REPO_OK);
  {
    struct json_object *v = NULL;
    assert(json_object_object_get_ex(outing, "outing_uid", &v));
    snprintf(uid, 33, "%s", json_object_get_string(v));
  }
  json_object_put(outing);
  snprintf(join, sizeof(join), "join %s", uid);
  command(&leader, "active", uid);
  command(&leader, "start", "rejected");
  aa.id = 77;
  command(&ally, join, "rejected");
  aa.id = 80;
  command(&ally, join, "join recorded");
  leader.next_in_room = NULL;
  command(&leader, "start", "rejected");
  leader.next_in_room = &ally;
  GET_LEVEL((&ally)) = 1;
  command(&leader, "start", "rejected");
  GET_LEVEL((&ally)) = 27;
  command(&leader, "start", "start recorded");
  command(&leader, "active", uid);
  command(&leader, "abandon", "abandon recorded");
  assert(clan_expedition_read(connection.conn, "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", &outing) ==
         CLAN_REPO_NOT_FOUND);
  mysql_close(connection.conn);
  puts("Phase 34 game commands: gates, opt-in, account dedup, current level/presence, start, "
       "stable readout and abandonment passed.");
  return 0;
}
