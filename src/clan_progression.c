/* Clan progression Phase 3. Pure configuration: deliberately not wired into
 * game events or storage. All default release controls are fail-closed. */
#include "clan_progression.h"
#include "campaign.h"
#include <string.h>

#define TIER(level, value, cost) {level, value, INT64_C(cost)}
#define BUFF(id, cat, label, first, a, b, c) \
  {id, cat, label, INT64_C(86400), INT64_C(43200), \
   {TIER(first, a, 100), TIER(5, b, 150), TIER(20, c, 225)}}
#define REWARD(id, scope, xp, influence, favor, claims) \
  {id, scope, {INT64_C(xp), INT64_C(influence), INT64_C(favor)}, claims}

const struct clan_progression_config clan_progression_defaults = {
  .rules_version = CLAN_PROGRESSION_RULES_VERSION,
  .schema_version = CLAN_PROGRESSION_SCHEMA_VERSION,
  .max_level = 20, .initial_slots = 2, .max_slots = 3, .third_slot_level = 10, .combat_slots = 1,
  .day_seconds = INT64_C(86400), .week_seconds = INT64_C(604800), .weekly_reset_iso_weekday = 1,
  .xp_points = {0, 200, 600, 1200, 2000, 3000, 4200, 5600, 7200, 9000,
                11000, 13200, 15600, 18200, 21000, 24000, 27200, 30600, 34200, 38000},
  .buffs = {
    BUFF(CLAN_BUFF_SCHOLAR, CLAN_CATEGORY_GROWTH, "Scholar's Encouragement", 1, 5, 8, 10),
    BUFF(CLAN_BUFF_ARCANE, CLAN_CATEGORY_RECOVERY, "Arcane Readiness", 1, 10, 15, 20),
    BUFF(CLAN_BUFF_PSIONIC, CLAN_CATEGORY_RECOVERY, "Psionic Focus", 1, 10, 20, 30),
    BUFF(CLAN_BUFF_VITALITY, CLAN_CATEGORY_COMBAT, "Company Vitality", 1, 5, 10, 15),
    BUFF(CLAN_BUFF_ARTISAN, CLAN_CATEGORY_CRAFT, "Artisan's Guidance", 3, 2, 3, 4)
  },
  .standings = {
    {CLAN_FAVOR_UNPROVEN, "Unproven", 0, 1},
    {CLAN_FAVOR_RECOGNIZED, "Recognized", 100, 2},
    {CLAN_FAVOR_TRUSTED, "Trusted", 500, 7},
    {CLAN_FAVOR_PROVEN, "Proven", 1500, 8},
    {CLAN_FAVOR_DISTINGUISHED, "Distinguished", 3500, 12},
    {CLAN_FAVOR_EXALTED, "Exalted", 6000, 18}
  },
  .rewards = {
    REWARD(CLAN_SOURCE_KILL, CLAN_REWARD_ACCOUNT, 2, 1, 1, 200),
    REWARD(CLAN_SOURCE_SUPPLY_ORDER, CLAN_REWARD_ACCOUNT, 100, 50, 50, 3),
    REWARD(CLAN_SOURCE_DELIVERY, CLAN_REWARD_ACCOUNT, 100, 50, 50, 2),
    REWARD(CLAN_SOURCE_GATHERING, CLAN_REWARD_ACCOUNT, 100, 50, 50, 2),
    REWARD(CLAN_SOURCE_BOUNTY, CLAN_REWARD_ACCOUNT, 100, 50, 50, 2),
    REWARD(CLAN_SOURCE_EXPEDITION_PERSONAL, CLAN_REWARD_ACCOUNT, 0, 0, 100, 2),
    REWARD(CLAN_SOURCE_BOUNTY_SHARED, CLAN_REWARD_SHARED, 200, 100, 0, 2),
    REWARD(CLAN_SOURCE_ASSAULT_SHARED, CLAN_REWARD_SHARED, 600, 300, 0, 1),
    REWARD(CLAN_SOURCE_HUNT_SHARED, CLAN_REWARD_SHARED, 600, 300, 0, 1)
  },
  .limits = {
    .account_full_xp_points = 600, .account_half_xp_points = 600, .account_favor_points = 600,
    .clan_full_xp_points = 6000, .clan_quarter_xp_points = 6000,
    .clan_weekly_influence_points = 5000, .stored_influence_points = 20000,
    .prototype_daily_full_kills = 20, .prototype_daily_half_kills = 20,
    .pending_per_character = 100, .pending_per_server = 10000
  },
  .participation = {
    .victim_level_tolerance = 3, .member_level_tolerance = 5, .recent_seconds = 120,
    .damage_percent = 5, .healing_percent = 10, .mitigation_percent = 5, .useful_actions = 2
  },
  .contracts = {
    .personal_slots = 3, .bounty_kills = 20, .gathering_base_units = 20,
    .delivery_material_units = 50, .delivery_ammo_shots = 100,
    .expedition_min_accounts = 2, .expedition_max_accounts = 6, .active_outings = 1,
    .assault_defenders = 20, .assault_named_targets = 2,
    .hunt_categories = 3, .hunt_areas = 2, .hunt_kills_per_category = 10,
    .personal_objective_percent = 50, .expedition_expiry_seconds = 7200
  },
  .controls = {
    .store_mode = CLAN_STORE_LEGACY, .mode = CLAN_PROGRESSION_OFF,
    .buffs = false, .contracts = false, .expeditions = false,
    .quartermaster = false, .web = false, .public_release = false, .pilot_count = 0
  }
};
#undef TIER
#undef BUFF
#undef REWARD

bool clan_progression_uid_valid(const char *uid)
{
  size_t i;
  bool nonzero = false;
  if (!uid) return false;
  for (i = 0; i < CLAN_PROGRESSION_UID_SIZE - 1; ++i) {
    char c = uid[i];
    if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return false;
    if (c != '0') nonzero = true;
  }
  return nonzero && uid[CLAN_PROGRESSION_UID_SIZE - 1] == '\0';
}

bool clan_progression_points_to_eighths(int64_t points, int64_t *out)
{
  if (!out || points < 0 || points > INT64_MAX / CLAN_PROGRESSION_UNITS_PER_POINT) return false;
  *out = points * CLAN_PROGRESSION_UNITS_PER_POINT;
  return true;
}

static bool points_valid(int64_t points)
{
  int64_t unused;
  return clan_progression_points_to_eighths(points, &unused);
}

static enum clan_progression_config_error invalid(const char **reason,
    enum clan_progression_config_error code, const char *message)
{
  if (reason) *reason = message;
  return code;
}

const struct clan_buff_definition *clan_progression_buff(
    const struct clan_progression_config *config, enum clan_buff_family family)
{
  size_t i;
  if (!config || family < CLAN_BUFF_SCHOLAR || family > CLAN_BUFF_ARTISAN) return NULL;
  for (i = 0; i < CLAN_PROGRESSION_BUFF_COUNT; ++i)
    if (config->buffs[i].family == family) return &config->buffs[i];
  return NULL;
}

const struct clan_reward_definition *clan_progression_reward(
    const struct clan_progression_config *config, enum clan_reward_source source)
{
  size_t i;
  if (!config || source < CLAN_SOURCE_KILL || source > CLAN_SOURCE_HUNT_SHARED) return NULL;
  for (i = 0; i < CLAN_PROGRESSION_REWARD_COUNT; ++i)
    if (config->rewards[i].source == source) return &config->rewards[i];
  return NULL;
}

enum clan_progression_config_error clan_progression_config_validate(
    const struct clan_progression_config *c, const char **reason)
{
  size_t i, j;
  bool buff_seen[CLAN_PROGRESSION_BUFF_COUNT + 1] = {false};
  bool reward_seen[CLAN_PROGRESSION_REWARD_COUNT + 1] = {false};
  const struct clan_progression_limits *l;
  const struct clan_contract_rules *q;
  const struct clan_participation_rules *p;
  if (reason) *reason = NULL;
#define REQUIRE(condition, code, message) do { if (!(condition)) return invalid(reason, code, message); } while (0)
  REQUIRE(c, CLAN_CONFIG_MISSING, "Missing clan progression configuration.");
  REQUIRE(c->rules_version == CLAN_PROGRESSION_RULES_VERSION &&
          c->schema_version == CLAN_PROGRESSION_SCHEMA_VERSION,
          CLAN_CONFIG_VERSION, "Unsupported clan rules or schema version.");
  REQUIRE(c->controls.store_mode >= CLAN_STORE_LEGACY && c->controls.store_mode <= CLAN_STORE_MYSQL &&
          c->controls.mode >= CLAN_PROGRESSION_OFF && c->controls.mode <= CLAN_PROGRESSION_LIVE,
          CLAN_CONFIG_CONTROLS, "Invalid clan storage or progression mode.");
  REQUIRE(c->controls.mode != CLAN_PROGRESSION_LIVE || c->controls.store_mode == CLAN_STORE_MYSQL,
          CLAN_CONFIG_CONTROLS, "Live clan progression requires MySQL authority.");
  REQUIRE(!(c->controls.buffs || c->controls.contracts || c->controls.expeditions ||
          c->controls.quartermaster || c->controls.web) || c->controls.mode == CLAN_PROGRESSION_LIVE,
          CLAN_CONFIG_CONTROLS, "Clan gameplay features require live progression.");
  REQUIRE(!c->controls.expeditions || c->controls.contracts,
          CLAN_CONFIG_CONTROLS, "Expeditions require clan contracts.");
  REQUIRE(c->controls.pilot_count <= CLAN_PROGRESSION_MAX_PILOT_CLANS,
          CLAN_CONFIG_IDENTITY, "Too many pilot clan identities.");
  for (i = 0; i < c->controls.pilot_count; ++i) {
    REQUIRE(clan_progression_uid_valid(c->controls.pilot_clans[i]),
            CLAN_CONFIG_IDENTITY, "Invalid pilot clan incarnation identity.");
    for (j = 0; j < i; ++j)
      REQUIRE(strcmp(c->controls.pilot_clans[i], c->controls.pilot_clans[j]),
              CLAN_CONFIG_IDENTITY, "Duplicate pilot clan identity.");
  }
  REQUIRE(c->max_level == CLAN_PROGRESSION_MAX_LEVEL && c->xp_points[0] == 0,
          CLAN_CONFIG_LEVELS, "Clan XP table must cover levels 1 through 20, starting at zero.");
  for (i = 0; i < CLAN_PROGRESSION_MAX_LEVEL; ++i)
    REQUIRE(points_valid(c->xp_points[i]) && (!i || c->xp_points[i] > c->xp_points[i - 1]),
            CLAN_CONFIG_LEVELS, "Clan XP thresholds must increase and fit eighth-point storage.");
  REQUIRE(c->initial_slots >= 1 && c->initial_slots <= c->max_slots &&
          c->max_slots <= CLAN_PROGRESSION_MAX_SLOTS && c->combat_slots == 1 &&
          c->third_slot_level >= 2 && c->third_slot_level <= c->max_level,
          CLAN_CONFIG_SLOTS, "Invalid clan buff slot or combat-category limits.");
  for (i = 0; i < CLAN_PROGRESSION_BUFF_COUNT; ++i) {
    const struct clan_buff_definition *b = &c->buffs[i];
    int maximum = 0, category = 0;
    REQUIRE(b->family >= CLAN_BUFF_SCHOLAR && b->family <= CLAN_BUFF_ARTISAN && !buff_seen[b->family],
            CLAN_CONFIG_BUFFS, "Invalid or duplicate stable buff family ID.");
    buff_seen[b->family] = true;
    switch (b->family) {
      case CLAN_BUFF_SCHOLAR: maximum = 10; category = CLAN_CATEGORY_GROWTH; break;
      case CLAN_BUFF_ARCANE: maximum = 20; category = CLAN_CATEGORY_RECOVERY; break;
      case CLAN_BUFF_PSIONIC: maximum = 30; category = CLAN_CATEGORY_RECOVERY; break;
      case CLAN_BUFF_VITALITY: maximum = 15; category = CLAN_CATEGORY_COMBAT; break;
      case CLAN_BUFF_ARTISAN: maximum = 4; category = CLAN_CATEGORY_CRAFT; break;
      default: break;
    }
    REQUIRE(b->name && *b->name && (int)b->category == category && b->duration_seconds > 0 &&
            b->downtime_seconds > 0 && b->duration_seconds <= INT64_MAX - b->downtime_seconds,
            CLAN_CONFIG_BUFFS, "Invalid buff label, category, duration or actual downtime.");
    for (j = 0; j < CLAN_PROGRESSION_TIER_COUNT; ++j) {
      const struct clan_buff_tier *t = &b->tiers[j];
      REQUIRE(t->unlock_level >= 1 && t->unlock_level <= c->max_level &&
              t->amount > 0 && t->amount <= maximum && points_valid(t->influence_points) &&
              t->influence_points > 0 && t->influence_points <= INT64_MAX / (8 * c->max_slots),
              CLAN_CONFIG_BUFFS, "Invalid buff tier unlock, effect or Influence price.");
      if (j) REQUIRE(t->unlock_level > b->tiers[j - 1].unlock_level &&
                     t->amount >= b->tiers[j - 1].amount &&
                     t->influence_points >= b->tiers[j - 1].influence_points,
                     CLAN_CONFIG_BUFFS, "Buff tiers must increase in unlock level without decreasing effect or price.");
    }
  }
  for (i = 0; i < CLAN_PROGRESSION_STANDING_COUNT; ++i) {
    const struct clan_standing_definition *s = &c->standings[i];
    REQUIRE((int)s->standing == (int)i && s->name && *s->name && points_valid(s->favor_points) &&
            s->stock_clan_level >= 1 && s->stock_clan_level <= c->max_level &&
            (i ? s->favor_points > c->standings[i - 1].favor_points &&
                 s->stock_clan_level >= c->standings[i - 1].stock_clan_level : s->favor_points == 0),
            CLAN_CONFIG_STANDINGS, "Invalid Favor standing IDs, thresholds or stock gates.");
  }
  for (i = 0; i < CLAN_PROGRESSION_REWARD_COUNT; ++i) {
    const struct clan_reward_definition *r = &c->rewards[i];
    REQUIRE(r->source >= CLAN_SOURCE_KILL && r->source <= CLAN_SOURCE_HUNT_SHARED && !reward_seen[r->source],
            CLAN_CONFIG_REWARDS, "Invalid or duplicate reward source ID.");
    reward_seen[r->source] = true;
    REQUIRE(points_valid(r->points.xp) && points_valid(r->points.influence) && points_valid(r->points.favor) &&
            r->weekly_claims > 0 &&
            r->points.xp <= INT64_MAX / (8 * (int64_t)r->weekly_claims) &&
            r->points.influence <= INT64_MAX / (8 * (int64_t)r->weekly_claims) &&
            r->points.favor <= INT64_MAX / (8 * (int64_t)r->weekly_claims),
            CLAN_CONFIG_REWARDS, "Invalid or overflowing reward tuple or source claim budget.");
    if (r->source <= CLAN_SOURCE_BOUNTY)
      REQUIRE(r->scope == CLAN_REWARD_ACCOUNT && r->points.xp > 0 &&
              r->points.xp == 2 * r->points.influence && r->points.influence == r->points.favor &&
              r->points.xp % 2 == 0,
              CLAN_CONFIG_REWARDS, "Account reward ratios must preserve exact eighth-point rounding.");
    else if (r->source == CLAN_SOURCE_EXPEDITION_PERSONAL)
      REQUIRE(r->scope == CLAN_REWARD_ACCOUNT && !r->points.xp && !r->points.influence && r->points.favor > 0,
              CLAN_CONFIG_REWARDS, "Personal expedition rewards grant Favor only.");
    else REQUIRE(r->scope == CLAN_REWARD_SHARED && r->points.xp > 0 && r->points.influence > 0 && !r->points.favor,
                 CLAN_CONFIG_REWARDS, "Shared rewards grant clan resources, not personal Favor.");
  }
  l = &c->limits;
  REQUIRE(points_valid(l->account_full_xp_points) && points_valid(l->account_half_xp_points) &&
          l->account_full_xp_points > 0 && l->account_half_xp_points > 0 &&
          l->account_full_xp_points <= INT64_MAX / 8 - l->account_half_xp_points &&
          points_valid(l->account_favor_points) && l->account_favor_points > 0 &&
          points_valid(l->clan_full_xp_points) && points_valid(l->clan_quarter_xp_points) &&
          l->clan_full_xp_points > 0 && l->clan_quarter_xp_points > 0 &&
          l->clan_full_xp_points <= INT64_MAX / 8 - l->clan_quarter_xp_points &&
          points_valid(l->clan_weekly_influence_points) && l->clan_weekly_influence_points > 0 &&
          points_valid(l->stored_influence_points) && l->stored_influence_points >= l->clan_weekly_influence_points &&
          l->prototype_daily_full_kills > 0 && l->prototype_daily_half_kills > 0 &&
          l->pending_per_character > 0 && l->pending_per_server >= l->pending_per_character,
          CLAN_CONFIG_LIMITS, "Invalid contribution bands, capacities or pending-operation bounds.");
  REQUIRE(c->day_seconds == 86400 && c->week_seconds == 604800 && c->weekly_reset_iso_weekday == 1,
          CLAN_CONFIG_CLOCK, "Version 1 requires UTC daily and Monday weekly windows.");
  p = &c->participation;
  REQUIRE(p->victim_level_tolerance >= 0 && p->member_level_tolerance >= 0 && p->recent_seconds > 0 &&
          p->damage_percent > 0 && p->damage_percent <= 100 &&
          p->healing_percent > 0 && p->healing_percent <= 100 &&
          p->mitigation_percent > 0 && p->mitigation_percent <= 100 && p->useful_actions >= 2,
          CLAN_CONFIG_PARTICIPATION, "Invalid all-level encounter participation thresholds.");
  q = &c->contracts;
  REQUIRE(q->personal_slots >= 1 && q->personal_slots <= 3 && q->bounty_kills > 0 &&
          q->gathering_base_units > 0 && q->delivery_material_units > 0 && q->delivery_ammo_shots > 0 &&
          q->expedition_min_accounts >= 2 && q->expedition_max_accounts >= q->expedition_min_accounts &&
          q->expedition_max_accounts <= 6 && q->active_outings == 1 && q->assault_defenders > 0 &&
          q->assault_named_targets > 0 && q->hunt_categories >= 2 && q->hunt_areas >= 2 &&
          q->hunt_areas <= q->hunt_categories && q->hunt_kills_per_category > 0 &&
          q->personal_objective_percent >= 50 && q->personal_objective_percent <= 100 &&
          q->expedition_expiry_seconds > 0 && q->expedition_expiry_seconds <= c->day_seconds,
          CLAN_CONFIG_CONTRACTS, "Invalid contract quotas, expedition participants or expiry.");
#undef REQUIRE
  return CLAN_CONFIG_OK;
}

bool clan_progression_campaign_supported(void)
{
#if defined(CAMPAIGN_DL) && !defined(CAMPAIGN_FR)
  return true;
#else
  return false;
#endif
}

bool clan_progression_feature_available(const struct clan_progression_config *config,
    enum clan_progression_feature feature, const struct clan_progression_readiness *ready,
    const char *clan_uid)
{
  size_t i;
  bool permitted;
  if (!clan_progression_campaign_supported() || !ready || !ready->schema_ready ||
      ready->schema_version != CLAN_PROGRESSION_SCHEMA_VERSION ||
      feature < CLAN_FEATURE_EARNINGS || feature >= CLAN_FEATURE_COUNT ||
      !clan_progression_uid_valid(clan_uid) ||
      clan_progression_config_validate(config, NULL) != CLAN_CONFIG_OK) return false;
  if (config->controls.store_mode != CLAN_STORE_MYSQL || config->controls.mode != CLAN_PROGRESSION_LIVE) return false;
  permitted = config->controls.public_release;
  for (i = 0; !permitted && i < config->controls.pilot_count; ++i)
    permitted = !strcmp(config->controls.pilot_clans[i], clan_uid);
  if (!permitted) return false;
  switch (feature) {
    case CLAN_FEATURE_EARNINGS: return true;
    case CLAN_FEATURE_BUFFS: return config->controls.buffs;
    case CLAN_FEATURE_CONTRACTS: return config->controls.contracts && ready->contracts_ready;
    case CLAN_FEATURE_EXPEDITIONS: return config->controls.expeditions && config->controls.contracts &&
        ready->contracts_ready && ready->expeditions_ready;
    case CLAN_FEATURE_QUARTERMASTER: return config->controls.quartermaster && ready->catalog_ready;
    case CLAN_FEATURE_WEB: return config->controls.web;
    default: return false;
  }
}

/* Unix epoch is Thursday. Its Monday is -3 days. Modulo the timestamp
 * BEFORE adding the offset to avoid overflow near INT64_MAX. */
bool clan_progression_week_at(int64_t occurred_at, struct clan_reset_period *out)
{
  struct clan_reset_period period;
  int64_t offset;
  if (!out || occurred_at < 0) return false;
  offset = (occurred_at % INT64_C(604800) + INT64_C(259200)) % INT64_C(604800);
  period.starts_at = occurred_at - offset;
  if (period.starts_at > INT64_MAX - INT64_C(604800)) return false;
  period.ends_at = period.starts_at + INT64_C(604800);
  *out = period;
  return true;
}
bool clan_progression_day_at(int64_t occurred_at, struct clan_reset_period *out)
{
  struct clan_reset_period period;
  if (!out || occurred_at < 0) return false;
  period.starts_at = occurred_at - occurred_at % INT64_C(86400);
  if (period.starts_at > INT64_MAX - INT64_C(86400)) return false;
  period.ends_at = period.starts_at + INT64_C(86400);
  *out = period;
  return true;
}
bool clan_progression_account_window_init(int64_t account_id, int64_t occurred_at,
    struct clan_account_window *out)
{
  struct clan_reset_period period;
  struct clan_account_window window;
  if (account_id <= 0 || !out || !clan_progression_week_at(occurred_at, &period)) return false;
  memset(&window, 0, sizeof(window));
  window.account_id = account_id;
  window.starts_at = period.starts_at;
  *out = window;
  return true;
}
bool clan_progression_account_window_valid(const struct clan_account_window *w)
{
  struct clan_reset_period period;
  size_t i;
  if (!w || w->account_id <= 0 || w->starts_at < -INT64_C(259200) ||
      !clan_progression_week_at(w->starts_at < 0 ? 0 : w->starts_at, &period) ||
      period.starts_at != w->starts_at || w->base_xp_eighths < 0 ||
      w->favor_eighths < 0 || w->revision < 0 || w->claims[0]) return false;
  for (i = 1; i <= CLAN_SOURCE_EXPEDITION_PERSONAL; i++)
    if (w->claims[i] < 0) return false;
  return true;
}
