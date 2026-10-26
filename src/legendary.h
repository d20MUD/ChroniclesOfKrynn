#ifndef KRYNN_LEGENDARY_H
#define KRYNN_LEGENDARY_H

/* Pure definitions/rules: no world, database, command or effect mutations.
 * Persistent IDs are explicit; never derive them from names or table order. */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define LEGENDARY_DEFINITION_VERSION 3
#define LEGENDARY_BALANCE_MAX INT64_C(2000000000)
#define LEGENDARY_PASSIVE_POINTS 12
#define LEGENDARY_MAJOR_POWERS 3
#define LEGENDARY_ESSENCE_COUNT 6
#define LEGENDARY_SLOT_COUNT 10
#define LEGENDARY_PATH_COUNT 12
#define LEGENDARY_FOUNDATION_COUNT 16
#define LEGENDARY_TECHNIQUE_COUNT 18
#define LEGENDARY_SIGNATURE_COUNT 13
#define LEGENDARY_POWER_COUNT 31
#define LEGENDARY_MAX_CHANNELS 16
#define LEGENDARY_FLAG(id) (1U << (id))

enum legendary_rank { LEG_RANK_NONE=0, LEG_RANK_I=1, LEG_RANK_II=2,
  LEG_RANK_III=3, LEG_RANK_IV=4, LEG_RANK_V=5 };
enum legendary_essence { LEG_ESS_DRACONIC=0, LEG_ESS_MARTIAL=1,
  LEG_ESS_ARCANE=2, LEG_ESS_SPIRITUAL=3, LEG_ESS_PRIMAL=4, LEG_ESS_SHADOW=5 };
enum legendary_slot { LEG_SLOT_WEAPON=0, LEG_SLOT_ARMS=1, LEG_SLOT_LEGS=2,
  LEG_SLOT_HEAD=3, LEG_SLOT_BODY=4, LEG_SLOT_ABOUT=5, LEG_SLOT_SHIELD=6,
  LEG_SLOT_HELD=7, LEG_SLOT_FEET=8, LEG_SLOT_HANDS=9 };
enum legendary_family { LEG_FAMILY_NONE=0, LEG_FAMILY_ASSAULT=1,
  LEG_FAMILY_GUARD=2, LEG_FAMILY_FOCUS=3 };
enum legendary_channel { LEG_CHANNEL_PRIMARY=0, LEG_CHANNEL_SUPPORTING=1 };
enum legendary_path { LEG_PATH_NONE=0, LEG_PATH_DRAGONBANE=1, LEG_PATH_GUARDIAN=2,
  LEG_PATH_SPELLWEAVER=3, LEG_PATH_PURSUER=4, LEG_PATH_LIFEBINDER=5,
  LEG_PATH_TRICKSTER=6, LEG_PATH_IRONWILL=7, LEG_PATH_WAYFARER=8,
  LEG_PATH_RUNEKEEPER=9, LEG_PATH_COMMANDER=10, LEG_PATH_SLAYER=11,
  LEG_PATH_RENEWAL=12 };
enum legendary_foundation_id { LEG_F01=1, LEG_F02=2, LEG_F03=3, LEG_F04=4,
  LEG_F05=5, LEG_F06=6, LEG_F07=7, LEG_F08=8, LEG_F09=9, LEG_F10=10,
  LEG_F11=11, LEG_F12=12, LEG_F13=13, LEG_F14=14, LEG_F15=15, LEG_F16=16 };
enum legendary_power_id { LEG_T01=101, LEG_T02=102, LEG_T03=103, LEG_T04=104,
  LEG_T05=105, LEG_T06=106, LEG_T07=107, LEG_T08=108, LEG_T09=109, LEG_T10=110,
  LEG_T11=111, LEG_T12=112, LEG_T13=113, LEG_T14=114, LEG_T15=115, LEG_T16=116,
  LEG_T17=117, LEG_T18=118, LEG_S01=201, LEG_S02=202, LEG_S03=203, LEG_S04=204,
  LEG_S05=205, LEG_S06=206, LEG_S07=207, LEG_S08=208, LEG_S09=209, LEG_S10=210,
  LEG_S11=211, LEG_S12=212, LEG_S13=213 };
enum legendary_modifier { LEG_MOD_NONE=0, LEG_MOD_ENDURING=1,
  LEG_MOD_READY=2, LEG_MOD_DISTANT_AID=3 };
enum legendary_ability { LEG_ABILITY_STR=0, LEG_ABILITY_DEX=1, LEG_ABILITY_CON=2,
  LEG_ABILITY_INT=3, LEG_ABILITY_WIS=4, LEG_ABILITY_CHA=5 };
enum legendary_element { LEG_ELEMENT_ACID=0, LEG_ELEMENT_COLD=1,
  LEG_ELEMENT_ELECTRICITY=2, LEG_ELEMENT_FIRE=3, LEG_ELEMENT_SONIC=4 };
enum legendary_cooldown_family { LEG_CD_OFFENSE=0, LEG_CD_UTILITY=1,
  LEG_CD_PROTECTION=2, LEG_CD_RESTORATIVE=3, LEG_CD_RESOURCE=4, LEG_CD_CONTROL=5 };
enum legendary_reshape { LEG_RESHAPE_FOUNDATION=0, LEG_RESHAPE_TECHNIQUE=1,
  LEG_RESHAPE_SIGNATURE=2, LEG_RESHAPE_MODIFIER=3, LEG_RESHAPE_PATH_FAMILY=4 };
enum legendary_item_kind { LEG_ITEM_WEAPON=1, LEG_ITEM_ARMOR=2,
  LEG_ITEM_SHIELD=3, LEG_ITEM_FOCUS=4, LEG_ITEM_OTHER=5 };
enum legendary_passive_reason { LEG_PASSIVE_ACTIVE=0, LEG_PASSIVE_INELIGIBLE=1,
  LEG_PASSIVE_INVALID=2, LEG_PASSIVE_CAPPED=3, LEG_PASSIVE_BUDGET=4, LEG_PASSIVE_COMBAT=5 };

struct legendary_cost { int64_t xp, gold, fragments, essences, catalysts; };
struct legendary_wallet {
  int64_t xp, gold, fragments, essences[LEGENDARY_ESSENCE_COUNT], catalysts;
};
struct legendary_foundation_def {
  int id; const char *code, *name, *description;
  int points, value, ceiling, copies, distinct_subtypes;
};
/* One additional item passive, selected on the primary track. */
#define LEGENDARY_ITEM_PASSIVE_COUNT 16
struct legendary_item_passive_def {
  int id; const char *code, *name, *description;
  unsigned int slots; int per_rank, ceiling;
};
extern const struct legendary_item_passive_def legendary_item_passives[16];
const struct legendary_item_passive_def *legendary_item_passive(int id);
bool legendary_item_passive_available(int slot,int id);
int legendary_item_passive_amount(int id,int rank);

struct legendary_values {
  int attack, damage, check_bonus, ac, percent, pool, heal, temporary_hp;
  int duration, window, interval, ticks, tick_heal, charges, targets;
  int resource_min, resource_discount, refund_percent, refund_cap;
  int fear_seconds, save_dc, control_seconds, physical_info;
};
struct legendary_power_def {
  int id; const char *code, *name, *description;
  int minimum_rank, cooldown, family;
  unsigned int modifiers;
  bool requires_dragonlance;
  struct legendary_values base, final;
};
struct legendary_resolved_power {
  struct legendary_values values;
  int cooldown, family_cooldown;
  bool target_ally_instead;
};
struct legendary_path_def { int id; const char *name; int essence; int powers[4]; };
struct legendary_slot_def {
  int id; const char *name;
  int foundations[4], techniques[4], signatures[3];
};
struct legendary_rules {
  int daily_fragments, daily_essences, daily_catalysts;
  int global_cooldown, quote_seconds, boss_cooldown_seconds;
  int participation_actions, participation_recent, encounter_expiry;
  int offense_limit, prevention_limit, healing_limit, rolling_seconds;
  int resource_limit, resource_seconds, essence_exchange_ratio;
  int catalyst_exchange_fragments, catalyst_exchange_essences;
};
struct legendary_boss_reward { int fragments, essences, catalysts; };

/* The later live adapter must supply actual checked facts, not infer these
 * from names. ordinary_allowed covers existing proficiency/class/wear rules. */
struct legendary_item_facts {
  int character_level, item_level, kind;
  int64_t character_id, bound_owner;
  unsigned int wearable_slots;
  bool npc, permanent, temporary, expiring, transformation, ordinary_allowed;
  bool two_hand_slot, mandatory_two_hand_ranged, actually_wielded;
  bool offhand_occupied, has_resource;
};
struct legendary_passive_request {
  int id, subtype; /* F07: 0..5 ability; F12: 0..4 element; F08: validated skill ID. */
  bool eligible; /* Includes typed stacking, skill training, owner/equip and ordinary caps. */
  int current_resistance; /* Nonlegendary percentage, F12 only. */
};
struct legendary_passive_result { int reason, value, points; };

extern const struct legendary_rules legendary_rules;
extern const char *const legendary_rank_names[6];
extern const char *const legendary_essence_names[6];
extern const char *const legendary_family_names[4];
extern const char *const legendary_modifier_names[4];
extern const struct legendary_cost legendary_rank_costs[6];
extern const struct legendary_cost legendary_reshape_costs[5];
extern const struct legendary_boss_reward legendary_boss_rewards[3];
extern const struct legendary_foundation_def legendary_foundations[16];
extern const struct legendary_power_def legendary_powers[31];
extern const struct legendary_path_def legendary_paths[12];
extern const struct legendary_slot_def legendary_slots[10];

bool legendary_add(int64_t a, int64_t b, int64_t *out);
bool legendary_subtract(int64_t a, int64_t b, int64_t *out);
bool legendary_to_int(int64_t value, int *out);
bool legendary_cost_between(int from_rank, int to_rank, struct legendary_cost *out);
bool legendary_pair_cost(int primary_rank, int supporting_rank, struct legendary_cost *out);
bool legendary_allocate_essences(const int64_t balances[6], int matching_type,
                                  int64_t amount, int64_t matching_required, int64_t payment[6]);
bool legendary_plan_essences(const int64_t balances[6], int matching_type,
                             int64_t amount, int64_t payment[6]);
bool legendary_can_afford(const struct legendary_wallet *wallet,
                         const struct legendary_cost *cost, int path, bool all_matching, int64_t payment[6]);
const struct legendary_foundation_def *legendary_foundation(int id);
const struct legendary_power_def *legendary_power(int id);
const struct legendary_path_def *legendary_path(int id);
const struct legendary_slot_def *legendary_slot(int id);
int legendary_parse_code(const char *code);
int legendary_family_slot(int family);
bool legendary_item_eligible(const struct legendary_item_facts *facts, int slot);
bool legendary_supporting_active(const struct legendary_item_facts *facts);
bool legendary_power_available(int slot, int path, int power, bool dragonlance);
bool legendary_modifier_allowed(int power, int modifier);
/* allowed() checks power-level compatibility; resolve() also checks rank.
 * Enduring cannot improve T08 until its rank-V timed benefit exists. */
bool legendary_resolve_power(int power, int rank, int modifier,
                             struct legendary_resolved_power *out);
/* Requests are already in saved priority order. At most MAX_CHANNELS entries.
 * False leaves output untouched; inactive requests have a reason and zero value. */
bool legendary_plan_passives(const struct legendary_passive_request *requests, size_t count,
                             struct legendary_passive_result *out, int *points_used);
/* Pure checks only. Called by tests now; startup validation will be wired later. */
bool legendary_validate_definitions(char *error, size_t error_size);

#endif
