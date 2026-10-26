#ifndef KRYNN_LEGENDARY_EQUIPMENT_H
#define KRYNN_LEGENDARY_EQUIPMENT_H
#include "legendary.h"
struct char_data;
struct obj_data;
/* Owner storage/recovery is distinct from sale, transfer, and conversion. */
enum legendary_route { LEG_ROUTE_OWNER, LEG_ROUTE_TRANSFER, LEG_ROUTE_TRANSFORM };
bool legendary_tree_present(const struct obj_data *);
bool legendary_tree_allowed(struct char_data *, struct obj_data *, enum legendary_route);
bool legendary_tree_check(struct char_data *, struct obj_data *, enum legendary_route);
void legendary_holder_check(struct char_data *, struct obj_data *);
bool legendary_storage_check(struct char_data *, struct obj_data *, struct obj_data *);
bool legendary_equipment_eligible(struct char_data *, struct obj_data *, int slot, int channel);
int legendary_equipment_slot(struct obj_data *);
bool legendary_resource_class(struct char_data *);
bool legendary_supported_skill(int skill);
bool legendary_trained_skill(struct char_data *, int skill);

enum legendary_dormant { LEG_EQUIPPED=0, LEG_BAD_IDENTITY, LEG_BAD_OWNER,
  LEG_BAD_EQUIPMENT, LEG_BAD_GRIP, LEG_NOT_EQUIPPED, LEG_BAD_CONFIG,
  LEG_NO_SELECTION, LEG_BAD_CAPABILITY, LEG_DUPLICATE_POWER };
struct legendary_equipped_channel {
  char uuid[33];
  int channel, wear, rank, path, family, foundation, subtype, technique, signature;
  int modifier, modifier_target, reason, item_passive;
  int64_t version;
  struct legendary_passive_result passive;
};
struct legendary_equipped_major { char uuid[33]; int channel, kind, power, reason; };
struct legendary_loadout {
  bool ready;
  int count, points, major_count;
  struct legendary_equipped_channel channels[LEGENDARY_MAX_CHANNELS];
  struct legendary_equipped_major majors[LEGENDARY_MAJOR_POWERS];
};
int legendary_item_passive_value(struct char_data *,int id);
const char *legendary_dormant_reason(int);
const struct legendary_loadout *legendary_loadout_get(struct char_data *);
/* Explicit configuration refresh may read SQL. Stable combat reads never do. */
void legendary_loadout_refresh(struct char_data *);
void legendary_loadout_combat_start(struct char_data *);
void legendary_loadout_invalidate(struct char_data *);
void legendary_loadout_free(struct char_data *);
/* Future effects lease their source. Revoke must remove only that effect and
 * may recalculate affects/invalidate the view, but must not attach sources,
 * free the character, or change persistent cooldowns or rolling budgets. */
bool legendary_source_attach(struct char_data *, const char *uuid, int channel,
    int power, void (*revoke)(struct char_data *, void *), void *context);
void legendary_source_forget(struct char_data *, void *context);
#endif
