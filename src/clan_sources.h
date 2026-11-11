#ifndef KRYNN_CLAN_SOURCES_H
#define KRYNN_CLAN_SOURCES_H
#include "clan_combat.h"
struct char_data;
/* Server facts only. No SQL in action collection. */
void clan_source_action(struct char_data *actor,struct char_data *victim,
    enum clan_combat_action_kind kind,struct char_data *ally,int amount);
void clan_source_support(struct char_data *actor,struct char_data *ally,
    enum clan_combat_action_kind kind,int amount);
void clan_source_damage(struct char_data *actor,struct char_data *victim,int amount,bool attack);
void clan_source_death(struct char_data *victim,struct char_data *killer);
void clan_source_forget(struct char_data *victim);
void clan_source_tick(int64_t now);
/* Capture membership before the successful turn-in; enqueue only after normal
 * rewards and order clearing. Persist before accepting a SQL receipt. */
void clan_source_supply_identity(struct char_data *ch,char uid[33]);
void clan_source_supply_complete(struct char_data *ch,const char uid[33]);
#endif
