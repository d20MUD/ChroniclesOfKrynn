#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "clan_expeditions.h"
#include <assert.h>
struct zone_data *zone_table;
zone_rnum top_of_zone_table;
struct room_data *world;
room_rnum top_of_world;
struct char_data *mob_proto, *character_list;
struct player_special_data dummy_mob;
zone_rnum real_zone(zone_vnum area)
{
  return area == 10 ? 0 : area == 11 ? 1 : NOWHERE;
}
mob_rnum real_mobile(mob_vnum mob)
{
  return mob >= 100 && mob < 106 ? mob - 100 : NOBODY;
}
void basic_mud_log(const char *format, ...)
{
  (void)format;
}
#include "../src/clan_expedition_world.c"
int main(void)
{
  struct zone_data zones[2] = {{0}};
  struct room_data rooms[2] = {{0}};
  struct char_data prototypes[6] = {{0}}, npcs[3] = {{0}};
  struct reset_com commands[7] = {{0}};
  struct clan_expedition_catalog c = {0};
  struct clan_expedition_world refs = clan_expedition_world_refs();
  char error[256];
  size_t i;
  zone_table = zones;
  world = rooms;
  top_of_world = 1;
  mob_proto = prototypes;
  zones[0].number = 10;
  zones[0].name = "Test stronghold";
  zones[0].cmd = commands;
  rooms[0].zone = 0;
  rooms[1].zone = 1;
  for (i = 0; i < 6; i++)
  {
    commands[i].command = 'M';
    commands[i].arg1 = i;
    commands[i].arg3 = 0;
    commands[i].arg4 = 100;
    SET_BIT_AR(MOB_FLAGS(&prototypes[i]), MOB_ISNPC);
    prototypes[i].player.short_descr = "sentinel";
  }
  commands[6].command = 'S';
  c.count = 1;
  strcpy(c.entries[0].id, "sentinels-v1");
  c.entries[0].family = 1;
  for (i = 0; i < 3; i++)
  {
    c.entries[0].objectives[i].quota = i ? 1 : 20;
    c.entries[0].objectives[i].count = 2;
    c.entries[0].objectives[i].targets[0] = (struct clan_expedition_target){10, 100 + 2 * i};
    c.entries[0].objectives[i].targets[1] = (struct clan_expedition_target){10, 101 + 2 * i};
  }
  assert(refs.target(10, 100, NULL));
  assert(!refs.available(10, 100, NULL));
  assert(!clan_expedition_catalog_validate(&c, &refs, error, sizeof(error)));
  for (i = 0; i < 3; i++)
  {
    SET_BIT_AR(MOB_FLAGS(&npcs[i]), MOB_ISNPC);
    GET_MOB_RNUM(&npcs[i]) = 1 + 2 * i;
    IN_ROOM(&npcs[i]) = 0;
    GET_HIT(&npcs[i]) = 10;
    GET_POS(&npcs[i]) = POS_STANDING;
    if (i < 2)
      npcs[i].next = &npcs[i + 1];
  }
  character_list = npcs;
  assert(clan_expedition_catalog_validate(
      &c, &refs, error, sizeof(error))); /* every primary unavailable, all fallbacks alive */
  GET_HIT(&npcs[2]) = 0;
  assert(!clan_expedition_catalog_validate(&c, &refs, error, sizeof(error)));
  GET_HIT(&npcs[2]) = 10;
  npcs[2].master = &npcs[0];
  assert(!refs.available(10, 105, NULL));
  npcs[2].master = NULL;
  IN_ROOM(&npcs[2]) = 1;
  assert(!refs.available(10, 105, NULL));
  IN_ROOM(&npcs[2]) = 0;
  SET_BIT_AR(ROOM_FLAGS(0), ROOM_PEACEFUL);
  assert(!refs.target(10, 100, NULL));
  REMOVE_BIT_AR(ROOM_FLAGS(0), ROOM_PEACEFUL);
  SET_BIT_AR(MOB_FLAGS(&prototypes[0]), MOB_NOKILL);
  assert(!refs.target(10, 100, NULL));
  REMOVE_BIT_AR(MOB_FLAGS(&prototypes[0]), MOB_NOKILL);
  SET_BIT_AR(zones[0].zone_flags, ZONE_CLOSED);
  assert(!refs.area(10, NULL));
  REMOVE_BIT_AR(zones[0].zone_flags, ZONE_CLOSED);
  assert(!strcmp(clan_expedition_target_name(100), "sentinel"));
  assert(clan_expedition_catalog_validate(&c, &refs, error, sizeof(error)));
  puts("Staging world adapter: loaded spawns, live fallback availability, missing/dead/controlled "
       "targets, closed/peaceful areas and names passed.");
  return 0;
}
