#ifndef KRYNN_CLAN_CONTRACT_ROTATION_H
#define KRYNN_CLAN_CONTRACT_ROTATION_H
#include "clan_progression.h"
#include <string.h>
/* Immutable IDs define the order, never catalog file order, player or clan.
 * Each unlock/difficulty cohort retains at least two alternatives. Singleton
 * unlocks remain available. Rotation changes admission only, never frozen terms. */
static inline bool clan_contract_rotates_in(const char *id, const char *const *ids, size_t count,
                                            int64_t now)
{
  struct clan_reset_period week;
  size_t i, rank = 0, take;
  if (!id || !ids || !count || !clan_progression_week_at(now, &week))
    return false;
  for (i = 0; i < count; i++)
    if (strcmp(ids[i], id) < 0)
      rank++;
  for (i = 0; i < count; i++)
    if (!strcmp(ids[i], id))
      break;
  if (i == count)
    return false;
  take = count > 2 ? count - 1 : count;
  return (rank + count - (size_t)((week.starts_at / 604800) % count)) % count < take;
}
#endif
