#ifndef KRYNN_CLAN_PROGRESSION_H
#define KRYNN_CLAN_PROGRESSION_H

/* Phase 3: definitions, immutable defaults, validation and release gates only.
 * No world/SQL access, persistence, rewards, timers, commands or stat effects. */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define CLAN_PROGRESSION_RULES_VERSION 2
/* Required schema contract; checked detection lives in clan_repository. */
#define CLAN_PROGRESSION_SCHEMA_VERSION 6
#define CLAN_PROGRESSION_UID_SIZE 33
#define CLAN_PROGRESSION_MAX_LEVEL 20
#define CLAN_PROGRESSION_MAX_SLOTS 3
#define CLAN_PROGRESSION_BUFF_COUNT 5
#define CLAN_PROGRESSION_TIER_COUNT 3
#define CLAN_PROGRESSION_STANDING_COUNT 6
#define CLAN_PROGRESSION_REWARD_COUNT 9
#define CLAN_PROGRESSION_MAX_PILOT_CLANS 25
#define CLAN_PROGRESSION_UNITS_PER_POINT INT64_C(8)

/* Persistent IDs are explicit. Do not reuse retired values or derive IDs from
 * display names, array positions, clan vnums, runtime rnums or officer rank. */
enum clan_buff_family {
  CLAN_BUFF_NONE = 0, CLAN_BUFF_SCHOLAR = 1, CLAN_BUFF_ARCANE = 2,
  CLAN_BUFF_PSIONIC = 3, CLAN_BUFF_VITALITY = 4, CLAN_BUFF_ARTISAN = 5
};
enum clan_buff_category {
  CLAN_CATEGORY_GROWTH = 1, CLAN_CATEGORY_RECOVERY = 2,
  CLAN_CATEGORY_COMBAT = 3, CLAN_CATEGORY_CRAFT = 4
};
enum clan_favor_standing {
  CLAN_FAVOR_UNPROVEN = 0, CLAN_FAVOR_RECOGNIZED = 1,
  CLAN_FAVOR_TRUSTED = 2, CLAN_FAVOR_PROVEN = 3,
  CLAN_FAVOR_DISTINGUISHED = 4, CLAN_FAVOR_EXALTED = 5
};
enum clan_reward_source {
  CLAN_SOURCE_NONE = 0, CLAN_SOURCE_KILL = 1, CLAN_SOURCE_SUPPLY_ORDER = 2,
  CLAN_SOURCE_DELIVERY = 3, CLAN_SOURCE_GATHERING = 4, CLAN_SOURCE_BOUNTY = 5,
  CLAN_SOURCE_EXPEDITION_PERSONAL = 6, CLAN_SOURCE_BOUNTY_SHARED = 7,
  CLAN_SOURCE_ASSAULT_SHARED = 8, CLAN_SOURCE_HUNT_SHARED = 9
};
enum clan_reward_scope { CLAN_REWARD_ACCOUNT = 1, CLAN_REWARD_SHARED = 2 };
enum clan_store_mode { CLAN_STORE_LEGACY = 0, CLAN_STORE_MYSQL_SHADOW = 1, CLAN_STORE_MYSQL = 2 };
enum clan_progression_mode { CLAN_PROGRESSION_OFF = 0, CLAN_PROGRESSION_SHADOW = 1, CLAN_PROGRESSION_LIVE = 2 };
enum clan_progression_feature {
  CLAN_FEATURE_EARNINGS = 0, CLAN_FEATURE_BUFFS = 1, CLAN_FEATURE_CONTRACTS = 2,
  CLAN_FEATURE_EXPEDITIONS = 3, CLAN_FEATURE_QUARTERMASTER = 4,
  CLAN_FEATURE_WEB = 5, CLAN_FEATURE_COUNT = 6
};
enum clan_progression_record_status { CLAN_RECORD_ACTIVE = 1, CLAN_RECORD_DELETED = 2 };
enum clan_progression_config_error {
  CLAN_CONFIG_OK = 0, CLAN_CONFIG_MISSING, CLAN_CONFIG_VERSION,
  CLAN_CONFIG_CONTROLS, CLAN_CONFIG_IDENTITY, CLAN_CONFIG_LEVELS,
  CLAN_CONFIG_SLOTS, CLAN_CONFIG_BUFFS, CLAN_CONFIG_STANDINGS,
  CLAN_CONFIG_REWARDS, CLAN_CONFIG_LIMITS, CLAN_CONFIG_CLOCK,
  CLAN_CONFIG_PARTICIPATION, CLAN_CONFIG_CONTRACTS
};

/* Stored balances use eighth-points. Configuration tables below use WHOLE
 * points and must pass checked conversion before entering a stored balance. */
struct clan_progression_identity { char uid[CLAN_PROGRESSION_UID_SIZE]; int vnum; };
struct clan_progression_state {
  struct clan_progression_identity identity;
  enum clan_progression_record_status status;
  int64_t xp_eighths, influence_eighths, revision;
};
/* Separate family rows survive cancellation/replacement and slot expiry. */
struct clan_buff_state {
  char clan_uid[CLAN_PROGRESSION_UID_SIZE], activation_uid[CLAN_PROGRESSION_UID_SIZE];
  enum clan_buff_family family;
  int tier, slot, rules_version; /* Buff effect/receipt format remains version 1. */
  int64_t started_at, expires_at, next_available_at, paid_influence_eighths;
  bool canceled;
};
struct clan_favor_state {
  char clan_uid[CLAN_PROGRESSION_UID_SIZE];
  int64_t character_id, favor_eighths, revision;
};
/* Keyed by account_data.id and occurrence-time Monday, never character/clan.
 * claims is indexed by explicit account-source IDs 1..6; slot 0 stays zero.
 * Shared sources have no per-account claim counter in this record. */
struct clan_account_window {
  int64_t account_id, starts_at, base_xp_eighths, favor_eighths, revision;
  int64_t claims[CLAN_SOURCE_EXPEDITION_PERSONAL + 1];
};
struct clan_reset_period { int64_t starts_at, ends_at; };
struct clan_weekly_window {
  char clan_uid[CLAN_PROGRESSION_UID_SIZE];
  int64_t starts_at, incoming_xp_eighths, influence_eighths, revision;
};
typedef int64_t (*clan_progression_clock_fn)(void *context);

struct clan_buff_tier {
  int unlock_level, amount;
  int64_t influence_points;
};
struct clan_buff_definition {
  enum clan_buff_family family;
  enum clan_buff_category category;
  const char *name;
  int64_t duration_seconds, downtime_seconds;
  struct clan_buff_tier tiers[CLAN_PROGRESSION_TIER_COUNT];
};
struct clan_standing_definition {
  enum clan_favor_standing standing;
  const char *name;
  int64_t favor_points;
  int stock_clan_level;
};
/* Frozen contribution weights, converted to the smaller rules-2 earning scale
 * only at the authoritative award boundary. These are not display amounts. */
struct clan_reward_points { int64_t xp, influence, favor; };
struct clan_reward_definition {
  enum clan_reward_source source;
  enum clan_reward_scope scope;
  struct clan_reward_points points;
  int weekly_claims;
};
struct clan_progression_controls {
  enum clan_store_mode store_mode;
  enum clan_progression_mode mode;
  bool buffs, contracts, expeditions, quartermaster, web, public_release;
  size_t pilot_count;
  char pilot_clans[CLAN_PROGRESSION_MAX_PILOT_CLANS][CLAN_PROGRESSION_UID_SIZE];
};
struct clan_progression_limits {
  int64_t account_full_xp_points, account_half_xp_points, account_favor_points;
  int64_t clan_full_xp_points, clan_quarter_xp_points;
  int64_t clan_weekly_influence_points, stored_influence_points;
  int prototype_daily_full_kills, prototype_daily_half_kills;
  int pending_per_character, pending_per_server;
};
struct clan_participation_rules {
  int victim_level_tolerance, member_level_tolerance;
  int64_t recent_seconds;
  int damage_percent, healing_percent, mitigation_percent, useful_actions;
};
struct clan_contract_rules {
  int personal_slots, bounty_kills, gathering_base_units, delivery_material_units, delivery_ammo_shots;
  int expedition_min_accounts, expedition_max_accounts, active_outings;
  int assault_defenders, assault_named_targets, hunt_categories, hunt_areas, hunt_kills_per_category;
  int personal_objective_percent;
  int64_t expedition_expiry_seconds;
};
struct clan_progression_config {
  int rules_version, schema_version, max_level;
  int initial_slots, max_slots, third_slot_level, combat_slots;
  int64_t day_seconds, week_seconds;
  int weekly_reset_iso_weekday; /* Monday=1; UTC boundaries, not local DST. */
  int64_t xp_points[CLAN_PROGRESSION_MAX_LEVEL]; /* rank thresholds; lifetime Influence in rules 2, index = rank - 1 */
  struct clan_buff_definition buffs[CLAN_PROGRESSION_BUFF_COUNT];
  struct clan_standing_definition standings[CLAN_PROGRESSION_STANDING_COUNT];
  struct clan_reward_definition rewards[CLAN_PROGRESSION_REWARD_COUNT];
  struct clan_progression_limits limits;
  struct clan_participation_rules participation;
  struct clan_contract_rules contracts;
  struct clan_progression_controls controls;
};
/* Trusted future repository/content readiness. Not a client-supplied payload.
 * A release gate does not replace member, permission, resource or claim checks. */
struct clan_progression_readiness {
  bool schema_ready, contracts_ready, expeditions_ready, catalog_ready;
  int schema_version;
};

extern const struct clan_progression_config clan_progression_defaults;
/* Frozen historical receipt policy; never edit or use for new events. */
extern const struct clan_progression_config clan_progression_v1;
/* UTC arithmetic, half-open periods, no local time/DST or wall-clock reads.
 * Occurrence time must be >=0 and the next boundary representable in int64.
 * Epoch's first week starts Monday 1969-12-29 (-259200). Outputs unchanged
 * on failure. Feed a trusted/injected clock's result, not client timestamps. */
bool clan_progression_week_at(int64_t occurred_at, struct clan_reset_period *out);
bool clan_progression_day_at(int64_t occurred_at, struct clan_reset_period *out);
bool clan_progression_account_window_init(int64_t account_id, int64_t occurred_at,
    struct clan_account_window *out);
bool clan_progression_account_window_valid(const struct clan_account_window *window);
bool clan_progression_uid_valid(const char *uid);
bool clan_progression_points_to_eighths(int64_t points, int64_t *out);
enum clan_progression_config_error clan_progression_config_validate(
    const struct clan_progression_config *config, const char **reason);
const struct clan_buff_definition *clan_progression_buff(
    const struct clan_progression_config *config, enum clan_buff_family family);
const struct clan_reward_definition *clan_progression_reward(
    const struct clan_progression_config *config, enum clan_reward_source source);
bool clan_progression_campaign_supported(void);
bool clan_progression_feature_available(const struct clan_progression_config *config,
    enum clan_progression_feature feature, const struct clan_progression_readiness *ready,
    const char *clan_uid);

#endif
