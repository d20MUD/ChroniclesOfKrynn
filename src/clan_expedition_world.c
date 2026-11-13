#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "clan_expeditions.h"
static bool area_exists(int area, void *context)
{
  (void)context;
  zone_rnum zone = real_zone(area);
  return zone != NOWHERE && !ZONE_FLAGGED(zone, ZONE_CLOSED);
}
static bool target_exists(int area, int mob, void *context)
{
  zone_rnum zone = real_zone(area);
  mob_rnum target = real_mobile(mob);
  int i;
  (void)context;
  if (zone == NOWHERE || target == NOBODY || !zone_table[zone].cmd ||
      ZONE_FLAGGED(zone, ZONE_CLOSED) || MOB_FLAGGED(&mob_proto[target], MOB_NOKILL) ||
      MOB_FLAGGED(&mob_proto[target], MOB_NOFIGHT) || AFF_FLAGGED(&mob_proto[target], AFF_CHARM))
    return false;
  for (i = 0; zone_table[zone].cmd[i].command != 'S'; i++)
  {
    const struct reset_com *cmd = &zone_table[zone].cmd[i];
    if (cmd->command == 'M' && cmd->arg1 >= 0 && (mob_rnum)cmd->arg1 == target && cmd->arg4 > 0 &&
        cmd->arg3 >= 0 && (room_rnum)cmd->arg3 <= top_of_world && world[cmd->arg3].zone == zone &&
        !ROOM_FLAGGED(cmd->arg3, ROOM_PEACEFUL) && !ROOM_FLAGGED(cmd->arg3, ROOM_DEATH))
      return true;
  }
  return false;
}
static bool target_available(int area, int mob, void *context)
{
  struct char_data *npc;
  mob_rnum target = real_mobile(mob);
  zone_rnum zone = real_zone(area);
  if (!target_exists(area, mob, context))
    return false;
  for (npc = character_list; npc; npc = npc->next)
    if (IS_NPC(npc) && GET_MOB_RNUM(npc) == target && GET_HIT(npc) > 0 && GET_POS(npc) > POS_DEAD &&
        IN_ROOM(npc) != NOWHERE && IN_ROOM(npc) <= top_of_world &&
        world[IN_ROOM(npc)].zone == zone && !ROOM_FLAGGED(IN_ROOM(npc), ROOM_PEACEFUL) &&
        !ROOM_FLAGGED(IN_ROOM(npc), ROOM_DEATH) && !MOB_FLAGGED(npc, MOB_NOKILL) &&
        !MOB_FLAGGED(npc, MOB_NOFIGHT) && !npc->master && !AFF_FLAGGED(npc, AFF_CHARM) &&
        !npc->legendary_owned_instance)
      return true;
  return false;
}
void clan_expedition_boot_validate(void)
{
  struct clan_expedition_catalog catalog;
  struct clan_expedition_world refs = {area_exists, target_exists, target_exists, NULL};
  struct json_object *root = json_object_from_file("clan/expeditions.json");
  char error[256];
  if (!root)
    log("CLAN: expedition definitions unavailable: clan/expeditions.json (no enrollment enabled)");
  else if (!clan_expedition_catalog_parse(root, &catalog, error, sizeof(error)) ||
           !clan_expedition_catalog_validate(&catalog, &refs, error, sizeof(error)))
    log("CLAN: expedition validation failed: %s", error);
  else
    log("CLAN: validated %zu expedition definitions (no enrollment enabled)", catalog.count);
  if (root)
    json_object_put(root);
}

struct clan_expedition_world clan_expedition_world_refs(void)
{
  struct clan_expedition_world refs = {area_exists, target_exists, target_available, NULL};
  return refs;
}

const char *clan_expedition_target_name(int mob)
{
  mob_rnum target = real_mobile(mob);
  return target != NOBODY && mob_proto[target].player.short_descr
             ? mob_proto[target].player.short_descr
             : "unknown enemy";
}
const char *clan_expedition_area_name(int area)
{
  zone_rnum zone = real_zone(area);
  return zone != NOWHERE && zone_table[zone].name ? zone_table[zone].name : "unknown area";
}
