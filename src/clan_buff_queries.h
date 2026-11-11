#ifndef KRYNN_CLAN_BUFF_QUERIES_H
#define KRYNN_CLAN_BUFF_QUERIES_H
#include "clan_progression.h"
struct clan_buff_family_view {
  enum clan_buff_family family;
  enum clan_buff_category category;
  char activation_uid[CLAN_PROGRESSION_UID_SIZE];
  int unlocked_tier, active_tier, active_slot;
  bool active, cooling_down, category_available;
  unsigned int available_slots; /* bit 0 = slot 1; no permission/price promise */
  int64_t expires_at, next_available_at;
};
struct clan_buff_view {
  int slots_total, slots_used, combat_used;
  enum clan_buff_family slots[CLAN_PROGRESSION_MAX_SLOTS];
  struct clan_buff_family_view families[CLAN_PROGRESSION_BUFF_COUNT];
};
/* Pure, output unchanged on invalid/corrupt state. Level comes from the checked
 * XP service. Input may include other incarnations, which never share timers.
 * This is placement/unlock information, not activation authorization: no release,
 * membership, permission, affordability, replacement or SQL mutation policy. */
bool clan_buff_query(const struct clan_progression_config *, const char *clan_uid,
    enum clan_progression_record_status, int level, const struct clan_buff_state *,
    size_t count, int64_t now, struct clan_buff_view *out);
#endif
