#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "mysql.h"
#include "protocol.h"
#include "clan_web.h"
#include <assert.h>
static int64_t clock_value = 2200000001;
static int reads, sets;
static bool busy, fail_read;
static struct clan_progression_config policy;
static MYSQL_POOL_CONN connection;
static char reported[2048];
bool mysql_available = true;
struct player_special_data dummy_mob;
time_t web_test_time(time_t *out)
{
  if (out)
    *out = clock_value;
  return clock_value;
}
#define time web_test_time
#include "../src/clan_web_runtime.c"
#undef time
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
struct json_object *clan_web_status(const char *status, const char *reason, int64_t now)
{
  struct json_object *o = json_object_new_object();
  json_object_object_add(o, "status", json_object_new_string(status));
  json_object_object_add(o, "reason", json_object_new_string(reason));
  json_object_object_add(o, "server_time", json_object_new_int64(now));
  return o;
}
enum clan_repository_result clan_web_read(MYSQL *db, const struct clan_progression_config *c,
                                          int64_t player, int64_t account, int64_t now,
                                          struct json_object **o)
{
  (void)db;
  (void)c;
  assert(busy);
  reads++;
  if (fail_read)
    return CLAN_REPO_UNAVAILABLE;
  *o = clan_web_status("ready", player == 1007 && account == 77 ? "original" : "new subject", now);
  return CLAN_REPO_OK;
}
void MSDPSetString(struct descriptor_data *d, variable_t v, const char *s)
{
  (void)d;
  assert(!busy && v == eMSDP_CLAN_PROGRESS);
  assert(strlen(s) < sizeof(reported));
  strcpy(reported, s);
  sets++;
}
void clan_web_content_enrich(struct char_data *ch,struct json_object *o,int64_t now){(void)ch;(void)o;(void)now;assert(!busy);}
int main(void)
{
  struct char_data ch = {0};
  struct descriptor_data d = {0};
  struct player_special_data ps = {0};
  struct account_data account = {0};
  protocol_t p = {0};
  MSDP_t value = {0};
  MSDP_t *values[eMSDP_MAX] = {0};
  ch.player_specials = &ps;
  ch.desc = &d;
  d.character = &ch;
  d.account = &account;
  STATE(&d) = CON_PLAYING;
  GET_IDNUM((&ch)) = 1007;
  account.id = 77;
  d.pProtocol = &p;
  p.bMSDP = true;
  p.pVariables = values;
  values[eMSDP_CLAN_PROGRESS] = &value;
  policy = clan_progression_v1;
  clan_web_update(&ch, false);
  assert(!sets && !reads);
  clan_web_update(&ch, true);
  assert(sets == 1 && !reads && strstr(reported, "disabled"));
  value.bReport = true;
  policy.controls.web = true;
  clock_value += 2;
  clan_web_update(&ch, false);
  assert(reads == 1 && strstr(reported, "original"));
  clan_web_update(&ch, false);
  assert(reads == 1);
  clan_web_update(&ch, true);
  assert(reads == 2);
  GET_IDNUM((&ch)) = 1010;
  account.id = 80;
  clan_web_update(&ch, false);
  assert(reads == 3 && strstr(reported, "new subject") && !strstr(reported, "original"));
  fail_read = true;
  clock_value += 2;
  clan_web_update(&ch, false);
  assert(strstr(reported, "unavailable") && !strstr(reported, "subject"));
  account.id = 0;
  clan_web_update(&ch, true);
  assert(strstr(reported, "authenticated"));
  d.character = NULL;
  clan_web_descriptor_update(&d, true);
  assert(strstr(reported, "authenticated") && d.clan_web_player == 0);
  d.character = &ch;
  account.id = 77;
  STATE(&d) = CON_PLAYING;
  fail_read = false;
  clan_web_descriptor_update(&d, true);
  assert(strstr(reported, "new subject"));
  SET_BIT_AR(MOB_FLAGS(&ch), MOB_ISNPC);
  clan_web_descriptor_update(&d, true);
  assert(strstr(reported, "authenticated"));
  puts("Clan MSDP: subscription/initial force, bounded periodic refresh, identity switch, "
       "failed-read clearing and authenticated privacy passed.");
  return 0;
}
