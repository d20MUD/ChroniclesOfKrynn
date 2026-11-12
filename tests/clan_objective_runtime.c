#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "mysql.h"
#include "clan_sources.h"
#include "clan.h"
#include "clan_buff_runtime.h"
#include "clan_persistence.h"
#include "clan_objective_runtime.h"
#include "clan_expedition_enrollment.h"
#include "crafting_new.h"
#include <assert.h>
#include <unistd.h>
static struct clan_progression_config policy;
static MYSQL_POOL_CONN connection;
static bool busy, save_ok = true, at_quartermaster = true;
static char messages[65536];
static int save_count, crash_save;
static int64_t clock_value = 2200000001;
static const char *checkpoint_path;
static unsigned sequence;
struct char_data *character_list;
struct player_special_data dummy_mob;
struct zone_data *zone_table;
struct room_data *world;
room_rnum top_of_world;
struct index_data *mob_index;
mob_rnum top_of_mobt;
bool mysql_available = true;
const char *crafting_materials[NUM_CRAFT_MATS] = {
    [21] = "ash wood", [26] = "hemp", [22] = "maple wood"};
time_t objective_test_time(time_t *out)
{
  if (out)
    *out = clock_value;
  return clock_value;
}
int MIN(int a, int b)
{
  return a < b ? a : b;
}
int MAX(int a, int b)
{
  return a > b ? a : b;
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
  size_t used = strlen(messages);
  (void)ch;
  va_start(ap, fmt);
  int n = vsnprintf(messages + used, sizeof(messages) - used, fmt, ap);
  va_end(ap);
  return n < 0 ? 0 : (size_t)n;
}
void basic_mud_log(const char *fmt, ...)
{
  (void)fmt;
}
size_t strlcpy(char *dst, const char *src, size_t size)
{
  size_t n = strlen(src);
  if (size)
  {
    size_t take = n < size - 1 ? n : size - 1;
    memcpy(dst, src, take);
    dst[take] = 0;
  }
  return n;
}
int material_grade(int id)
{
  return id == 1 || id == 21 || id == 26 ? 1 : id == 22 ? 2 : 0;
}
mob_rnum real_mobile(mob_vnum v)
{
  return v > 0 ? 0 : NOBODY;
}
bool has_quartermaster_in_room(struct char_data *ch)
{
  (void)ch;
  return at_quartermaster;
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
bool save_char_checked(struct char_data *ch, int mode)
{
  struct json_object *o;
  char file[1024];
  (void)mode;
  assert(!busy);
  save_count++;
  if (!save_ok)
    return false;
  o = json_object_new_object();
  json_object_object_add(o, "gather",
                         json_object_new_string(ch->player_specials->clan_objective_pending[0]));
  json_object_object_add(o, "bounty",
                         json_object_new_string(ch->player_specials->clan_objective_pending[1]));
  json_object_object_add(o, "materials", json_object_new_int(GET_CRAFT_MAT(ch, 21)));
  snprintf(file, sizeof(file), "%s-%ld", checkpoint_path, GET_IDNUM(ch));
  assert(!json_object_to_file(file, o));
  json_object_put(o);
  if (save_count == crash_save)
    _exit(88);
  return true;
}
void save_char(struct char_data *ch, int mode)
{
  save_char_checked(ch, mode);
}
static struct json_object *get(struct json_object *o, const char *key)
{
  struct json_object *v = NULL;
  json_object_object_get_ex(o, key, &v);
  return v;
}
static int64_t count(const char *query)
{
  assert(!mysql_query(connection.conn, query));
  MYSQL_RES *r = mysql_store_result(connection.conn);
  MYSQL_ROW row = mysql_fetch_row(r);
  assert(row);
  int64_t n = atoll(row[0]);
  mysql_free_result(r);
  return n;
}
static void accept(struct char_data *ch, int family)
{
  char uid[33];
  struct clan_objective_definition d = {"native-gather-v1", "Fresh ash", 4, 21, 1, 20, 1, 1, 30};
  if (family == 5)
  {
    strcpy(d.id, "native-bounty-v1");
    strcpy(d.name, "Designated bandits");
    d.family = 5;
    d.target = 3705;
    d.grade = 0;
  }
  assert(clan_repository_uid_generate(uid));
  assert(clan_objective_accept(connection.conn, &d, uid, "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
                               GET_IDNUM(ch), ch->desc->account->id, GET_LEVEL(ch),
                               clock_value) == CLAN_REPO_OK);
}
static void load_checkpoint(struct char_data *ch)
{
  char file[1024];
  snprintf(file, sizeof(file), "%s-%ld", checkpoint_path, GET_IDNUM(ch));
  struct json_object *o = json_object_from_file(file);
  assert(o);
  strlcpy(ch->player_specials->clan_objective_pending[0], json_object_get_string(get(o, "gather")),
          256);
  strlcpy(ch->player_specials->clan_objective_pending[1], json_object_get_string(get(o, "bounty")),
          256);
  GET_CRAFT_MAT(ch, 21) = json_object_get_int(get(o, "materials"));
  json_object_put(o);
}
static bool expedition_area(int area, void *context) {(void)area;(void)context;return true;}
static bool expedition_target(int area,int mob,void *context) {(void)area;(void)mob;(void)context;return true;}
int main(int argc, char **argv)
{
  struct char_data ch = {0}, ally = {0}, npc = {0};
  struct player_special_data ps = {0}, aps = {0};
  struct descriptor_data desc = {0}, ad = {0};
  struct account_data account = {0}, aa = {0};
  struct room_data rooms[1] = {{0}};
  struct index_data mobs[1] = {{0}};
  struct group_data group = {0};
  assert(argc >= 4);
  checkpoint_path = argv[2];
  const char *mode = argv[3];
  if (argc > 4)
    clock_value = atoll(argv[4]);
  if (argc > 5)
    crash_save = atoi(argv[5]);
  if (argc > 6)
    clan_objective_crash = atoi(argv[6]);
  for (const char *p = mode; *p; p++)
    sequence = sequence * 31 + (unsigned char)*p;
  sequence += (unsigned)clock_value;
  connection.conn = mysql_init(NULL);
  assert(mysql_real_connect(connection.conn, NULL, "root", "", "clan_test", 0, argv[1], 0));
  policy = clan_progression_defaults;
  policy.controls.store_mode = CLAN_STORE_MYSQL;
  policy.controls.mode = CLAN_PROGRESSION_LIVE;
  policy.controls.public_release = true;
  policy.controls.contracts = true;
  ch.player_specials = &ps;
  ch.desc = &desc;
  desc.character = &ch;
  desc.account = &account;
  account.id = 77;
  STATE(&desc) = CON_PLAYING;
  GET_IDNUM((&ch)) = 5000000007;
  GET_LEVEL((&ch)) = 7;
  GET_HIT((&ch)) = 100;
  GET_MAX_HIT((&ch)) = 100;
  GET_POS((&ch)) = POS_STANDING;
  IN_ROOM((&ch)) = 0;
  ally.player_specials = &aps;
  ally.desc = &ad;
  ad.character = &ally;
  ad.account = &aa;
  aa.id = !strcmp(mode, "alt") ? 77 : 88;
  STATE(&ad) = CON_PLAYING;
  GET_IDNUM((&ally)) = 5000000012;
  GET_LEVEL((&ally)) = 7;
  GET_HIT((&ally)) = 100;
  GET_MAX_HIT((&ally)) = 100;
  GET_POS((&ally)) = POS_STANDING;
  IN_ROOM((&ally)) = 0;
  world = rooms;
  world[0].number = 4300;
  mob_index = mobs;
  mob_index[0].vnum = 3705;
  SET_BIT_AR(MOB_FLAGS((&npc)), MOB_ISNPC);
  npc.nr = 0;
  GET_LEVEL((&npc)) = 7;
  GET_MAX_HIT((&npc)) = GET_HIT((&npc)) = 1000;
  IN_ROOM((&npc)) = 0;
  character_list = &ch;
  ch.next = &ally;
  world[0].people = &ch;
  ch.next_in_room = &ally;
  ally.next_in_room = &npc;
  GROUP((&ch)) = GROUP((&ally)) = &group;
  if (!strcmp(mode, "commands"))
  {
    policy.controls.contracts = false;
    do_clancontracts(&ch, "gathering list", 0, 0);
    assert(strstr(messages, "unavailable"));
    *messages = 0;
    policy.controls.contracts = true;
    do_clancontracts(&ch, "gathering list", 0, 0);
    assert(strstr(messages, "fresh-ash-v1") && strstr(messages, "exact grade 1"));
    *messages = 0;
    at_quartermaster = false;
    do_clancontracts(&ch, "gathering accept fresh-ash-v1", 0, 0);
    assert(strstr(messages, "rejected"));
    *messages = 0;
    at_quartermaster = true;
    do_clancontracts(&ch, "gathering accept fresh-ash-v1", 0, 0);
    assert(strstr(messages, "accept recorded"));
    *messages = 0;
    GET_CRAFT_MAT((&ch), 21) = 10000;
    do_clancontracts(&ch, "gathering active", 0, 0);
    assert(strstr(messages, "0 / 20"));
    *messages = 0;
    struct clan_objective_event e;
    assert(clan_objective_harvest_begin(&ch, 21, &e));
    clan_objective_harvest_finish(&ch, &e, 4);
    do_clancontracts(&ch, "gathering active", 0, 0);
    assert(strstr(messages, "4 / 20") && strstr(messages, "16 remaining"));
    *messages = 0;
    do_clancontracts(&ch, "gathering abandon", 0, 0);
    assert(strstr(messages, "abandon recorded"));
    *messages = 0;
    do_clancontracts(&ch, "gathering accept fresh-ash-v1", 0, 0);
    do_clancontracts(&ch, "gathering active", 0, 0);
    assert(strstr(messages, "0 / 20"));
    *messages = 0;
    do_clancontracts(&ch, "bounty list", 0, 0);
    assert(strstr(messages, "sewer-bandits-v1") && strstr(messages, "two shared bounty"));
    *messages = 0;
  }
  else if (!strncmp(mode, "expedition", 10))
  {
    struct zone_data zones[1]={{0}};zone_table=zones;zones[0].number=141;
    struct json_object *root=json_object_from_file("clan/expeditions.json");
    struct clan_expedition_catalog catalog;struct clan_expedition_world refs={expedition_area,expedition_target,expedition_target,NULL};
    struct clan_expedition_member members[2]={{5000000007,77,27},{5000000012,88,27}};
    char uid[33];assert(root&&clan_expedition_catalog_parse(root,&catalog,NULL,0));json_object_put(root);
    policy.controls.expeditions=true;
    GET_LEVEL((&ch))=GET_LEVEL((&ally))=GET_LEVEL((&npc))=27;
    assert(legendary_uuid_generate(uid));
    assert(clan_expedition_create(connection.conn,&catalog,&refs,"icewall-assault-v1",uid,"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",&members[0],clock_value,&policy)==CLAN_REPO_OK);
    assert(clan_expedition_enroll(connection.conn,uid,"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",&members[1],clock_value,&policy)==CLAN_REPO_OK);
    assert(clan_expedition_start(connection.conn,uid,"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",members[0].player,members,2,clock_value,&policy)==CLAN_REPO_OK);
    FIGHTING((&ch))=FIGHTING((&ally))=&npc;
    int64_t shared_before=count("SELECT COUNT(*) FROM clan_reward_receipts WHERE source_id=8 AND receipt_status=1");
    int64_t expedition_before=count("SELECT COUNT(*) FROM clan_expedition_events");
    int64_t completions_before=count("SELECT COUNT(*) FROM clan_expedition_completions");
    int64_t entitlements_before=count("SELECT COUNT(*) FROM clan_expedition_entitlements");
    for(int i=0;i<22;i++)
    {
      mob_index[0].vnum=i<20?14100:i==20?14001:14000;
      clan_source_damage(&ch,&npc,100,true);
      if(!strcmp(mode,"expedition-heal"))clan_source_support(&ally,&ch,CLAN_COMBAT_HEAL,10);
      else clan_source_damage(&ally,&npc,100,true);
      clan_source_death(&npc,&ch);clan_source_death(&npc,&ch);clock_value++;
    }
    assert(count("SELECT COUNT(*) FROM clan_expedition_events")==expedition_before+22);
    assert(count("SELECT COUNT(*) FROM clan_expedition_completions")==completions_before+1);
    assert(count("SELECT COUNT(*) FROM clan_expedition_entitlements")==entitlements_before+2);
    clan_source_tick(clock_value+31);
    assert(count("SELECT COUNT(*) FROM clan_reward_receipts WHERE source_id=8 AND receipt_status=1")==shared_before+1);
  }
  else if (!strcmp(mode, "recover"))
  {
    load_checkpoint(&ch);
    assert(clan_objective_flush(&ch));
    assert(clan_objective_flush(&ch));
  }
  else if (!strcmp(mode, "gather-schema"))
  {
    struct clan_objective_event e;
    accept(&ch, 4);
    assert(!mysql_query(connection.conn, "DROP TRIGGER co_event_immutable"));
    assert(!clan_objective_harvest_begin(&ch, 21, &e));
    assert(!GET_CRAFT_MAT((&ch), 21));
  }
  else if (!strncmp(mode, "gather", 6))
  {
    accept(&ch, 4);
    int loops = !strcmp(mode, "gather") ? 5 : 1;
    int64_t prior_events = count("SELECT COUNT(*) FROM clan_objective_events");
    if (!strcmp(mode, "gather-failed-save"))
      save_ok = false;
    for (int i = 0; i < loops; i++)
    {
      struct clan_objective_event e;
      assert(clan_objective_harvest_begin(&ch, 21, &e));
      assert(*e.contract_uid);
      GET_CRAFT_MAT((&ch), 21) += 40;
      clan_objective_harvest_finish(&ch, &e, 4); /* 4 base, 36 bonus never counts. */
    }
    if (!save_ok)
    {
      assert(*ps.clan_objective_pending[0]);
      assert(count("SELECT COUNT(*) FROM clan_objective_events") == prior_events);
      save_ok = true;
      assert(clan_objective_flush(&ch));
    }
  }
  else
  {
    accept(&ch, 5);
    if (!strcmp(mode, "alt"))
      accept(&ally, 5);
    FIGHTING((&ch)) = &npc;
    for (int i = 0; i < 20; i++)
    {
      if (!strcmp(mode, "owned"))
        npc.legendary_owned_instance = true;
      if (!strcmp(mode, "trivial"))
        GET_LEVEL((&npc)) = 1;
      if (!strcmp(mode, "wrong-target"))
        mob_index[0].vnum = 12310;
      if (!strcmp(mode, "disabled"))
        policy.controls.contracts = false;
      if (!strcmp(mode, "support"))
      {
        FIGHTING((&ally)) = &npc;
        clan_source_action(&ch, &npc, CLAN_COMBAT_DEFENSE, &ally, 1);
        clock_value += 6;
        clan_source_action(&ch, &npc, CLAN_COMBAT_DEFENSE, &ally, 1);
      }
      else if (!strcmp(mode, "heal"))
      {
        FIGHTING((&ally)) = &npc;
        clan_source_support(&ch, &ally, CLAN_COMBAT_HEAL, 10);
      }
      else if (!strcmp(mode, "tank"))
      {
        FIGHTING((&npc)) = &ch;
        clan_source_damage(&npc, &ch, 1, true);
        clan_source_damage(&npc, &ch, 1, true);
      }
      else
      {
        clan_source_damage(&ch, &npc, 50, true);
        if (!strcmp(mode, "alt"))
        {
          FIGHTING((&ally)) = &npc;
          clan_source_damage(&ally, &npc, 50, true);
        }
      }
      clan_source_death(&npc, &ch);
      clan_source_death(&npc, &ch);
      clock_value++;
    }
    clan_source_tick(clock_value + 31);
  }
  printf(
      "{\"progress\":%lld,\"materials\":%d,\"completed\":%lld}\n",
      (long long)count(
          "SELECT COALESCE(SUM(progress),0) FROM clan_objective_contracts WHERE state IN(0,1,2)"),
      GET_CRAFT_MAT((&ch), 21),
      (long long)count("SELECT COUNT(*) FROM clan_objective_contracts WHERE state=2"));
  mysql_close(connection.conn);
  return 0;
}
