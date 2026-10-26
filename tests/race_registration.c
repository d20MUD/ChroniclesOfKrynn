/* Exercise production race registration without a running game or database. */
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "race.h"
#include <assert.h>

static void check_race(int race, const char *name, int cost, int feat_count)
{
  struct race_feat_assign *feat;
  int count = 0;
  assert(race_list[race].is_pc);
  assert(strcmp(race_list[race].type, name) == 0);
  assert(race_list[race].type_color != NULL);
  assert(race_list[race].descrip != NULL && *race_list[race].descrip);
  assert(race_list[race].unlock_cost == cost);
  for (feat = race_list[race].featassign_list; feat; feat = feat->next)
  {
    assert(feat->level_received == 1);
    assert(feat->feat_num > 0 && feat->feat_num < NUM_FEATS);
    count++;
  }
  assert(count == feat_count);
}

int main(void)
{
  assign_races();
  check_race(DL_RACE_AURAK_DRACONIAN, "Aurak Draconian", 15000, 10);
  check_race(DL_RACE_SIVAK_DRACONIAN, "Sivak Draconian", 8000, 9);
  check_race(DL_RACE_IRDA, "Irda", 5000, 4);
  assert(race_list[DL_RACE_AURAK_DRACONIAN].ability_mods[2] == 4);
  assert(race_list[DL_RACE_SIVAK_DRACONIAN].ability_mods[0] == 2);
  assert(race_list[DL_RACE_IRDA].ability_mods[3] == 2);
  puts("Advanced race registration passed.");
  return 0;
}
