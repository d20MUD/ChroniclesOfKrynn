#ifndef KRYNN_LEGENDARY_FOUNDATIONS_H
#define KRYNN_LEGENDARY_FOUNDATIONS_H
#include <stdbool.h>
#include "legendary.h"
struct char_data;
struct obj_data;
struct legendary_loadout;
int legendary_foundation_value(struct char_data *, int id, int subtype);
int legendary_foundation_resistance_base(struct char_data *, int type);
void legendary_foundations_sync(struct char_data *, const struct legendary_loadout *);
void legendary_foundations_refresh(struct char_data *);
bool legendary_foundations_begin(struct char_data *);
void legendary_foundations_reset(struct char_data *);
void legendary_foundations_apply(struct char_data *);
void legendary_foundations_end(struct char_data *);
void legendary_foundations_free(struct char_data *);
int legendary_foundation_channel_actual(struct char_data *, const char *uuid, int channel);
int legendary_foundation_actual(struct char_data *, int id, int subtype);
int legendary_foundation_deflection(struct char_data *, int existing);
int legendary_foundation_weapon(struct char_data *, struct obj_data *, bool damage);
int legendary_foundation_skill(struct char_data *, int skill);
int legendary_foundation_maneuver(struct char_data *, int maneuver);
int legendary_foundation_walk(struct char_data *, int cost, bool mounted);
int legendary_foundation_regen(struct char_data *);
void legendary_foundation_hp(struct char_data *, int ordinary_max);
#endif
