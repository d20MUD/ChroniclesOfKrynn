#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "clan_expeditions.h"
static bool area_exists(int area, void *context)
{
  (void)context;
  return real_zone(area) != NOWHERE;
}
static bool target_exists(int area, int mob, void *context)
{
  zone_rnum zone = real_zone(area);
  mob_rnum target = real_mobile(mob);
  int i;
  (void)context;
  if (zone == NOWHERE || target == NOBODY || !zone_table[zone].cmd)
    return false;
  for (i = 0; zone_table[zone].cmd[i].command != 'S'; i++)
  {
    const struct reset_com *cmd = &zone_table[zone].cmd[i];
    if (cmd->command == 'M' && cmd->arg1 == target && cmd->arg4 > 0 && cmd->arg3 >= 0 &&
        cmd->arg3 <= top_of_world && world[cmd->arg3].zone == zone)
      return true;
  }
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
  struct clan_expedition_world refs = {area_exists, target_exists, target_exists, NULL};
  return refs;
}
