#include "clan_buff_queries.h"
#include <string.h>

bool clan_buff_query(const struct clan_progression_config *c, const char *uid,
    enum clan_progression_record_status status, int level, const struct clan_buff_state *rows,
    size_t count, int64_t now, struct clan_buff_view *out)
{
  struct clan_buff_view v;
  bool seen[CLAN_PROGRESSION_BUFF_COUNT] = {false};
  size_t i, j, k;
  unsigned int free_slots = 0;
  if (!out || !clan_progression_uid_valid(uid) || now < 0 ||
      (status != CLAN_RECORD_ACTIVE && status != CLAN_RECORD_DELETED) ||
      clan_progression_config_validate(c, NULL) != CLAN_CONFIG_OK ||
      level < 1 || level > c->max_level || (count && !rows)) return false;
  memset(&v, 0, sizeof(v));
  if (status == CLAN_RECORD_ACTIVE)
    v.slots_total = level >= c->third_slot_level ? c->max_slots : c->initial_slots;
  for (i = 0; i < CLAN_PROGRESSION_BUFF_COUNT; i++) {
    struct clan_buff_family_view *f = &v.families[i];
    f->family = c->buffs[i].family;
    f->category = c->buffs[i].category;
    if (status == CLAN_RECORD_ACTIVE)
      for (j = 0; j < CLAN_PROGRESSION_TIER_COUNT; j++)
        if (level >= c->buffs[i].tiers[j].unlock_level) f->unlocked_tier = (int)j + 1;
  }
  for (k = 0; k < count; k++) {
    const struct clan_buff_state *b = &rows[k];
    struct clan_buff_family_view *f;
    if (!memchr(b->clan_uid, 0, sizeof(b->clan_uid)) ||
        !clan_progression_uid_valid(b->clan_uid)) return false;
    if (strcmp(uid, b->clan_uid)) continue;
    for (i = 0; i < CLAN_PROGRESSION_BUFF_COUNT; i++)
      if (v.families[i].family == b->family) break;
    if (i == CLAN_PROGRESSION_BUFF_COUNT || seen[i] ||
        !memchr(b->activation_uid, 0, sizeof(b->activation_uid)) ||
        !clan_progression_uid_valid(b->activation_uid) ||
        b->tier < 1 || b->tier > CLAN_PROGRESSION_TIER_COUNT || b->slot < 1 ||
        b->slot > CLAN_PROGRESSION_MAX_SLOTS || b->rules_version != 1 ||
        b->started_at < 0 || b->started_at > now || b->expires_at <= b->started_at ||
        b->next_available_at < b->expires_at || b->paid_influence_eighths < 0) return false;
    for (j = 0; j < k; j++)
      if (!strcmp(rows[j].clan_uid, uid) && !strcmp(rows[j].activation_uid, b->activation_uid))
        return false;
    seen[i] = true;
    f = &v.families[i];
    f->expires_at = b->expires_at;
    f->next_available_at = b->next_available_at;
    f->cooling_down = now < b->next_available_at;
    f->active = status == CLAN_RECORD_ACTIVE && !b->canceled && now < b->expires_at;
    if (f->active) {
      strcpy(f->activation_uid, b->activation_uid);
      if (b->slot > v.slots_total || b->tier > f->unlocked_tier || v.slots[b->slot-1]) return false;
      f->active_tier = b->tier;
      f->active_slot = b->slot;
      v.slots[b->slot-1] = b->family;
      v.slots_used++;
      if (f->category == CLAN_CATEGORY_COMBAT) v.combat_used++;
    }
  }
  if (v.slots_used > v.slots_total || v.combat_used > c->combat_slots) return false;
  for (i = 0; i < (size_t)v.slots_total; i++)
    if (!v.slots[i]) free_slots |= 1U << i;
  for (i = 0; i < CLAN_PROGRESSION_BUFF_COUNT; i++) {
    struct clan_buff_family_view *f = &v.families[i];
    f->category_available = status == CLAN_RECORD_ACTIVE &&
        (f->category != CLAN_CATEGORY_COMBAT || v.combat_used < c->combat_slots);
    if (f->unlocked_tier && !f->active && !f->cooling_down && f->category_available)
      f->available_slots = free_slots;
  }
  *out = v;
  return true;
}
