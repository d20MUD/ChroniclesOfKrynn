/* Production Phase 3 configuration/release-gate behavior; no live DB/world. */
#include "clan_progression.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

static const char uid_a[] = "0123456789abcdef0123456789abcdef";
static const char uid_b[] = "fedcba9876543210fedcba9876543210";

static void rejected(struct clan_progression_config *c, enum clan_progression_config_error expected)
{
  const char *reason = NULL;
  assert(clan_progression_config_validate(c, &reason) == expected);
  assert(reason && *reason);
}

static void defaults(void)
{
  const struct clan_progression_config *c = &clan_progression_v1;
  const char *reason = "must be cleared";
  int level;
  assert(clan_progression_config_validate(c, &reason) == CLAN_CONFIG_OK && !reason);
  assert(c->controls.store_mode == CLAN_STORE_LEGACY && c->controls.mode == CLAN_PROGRESSION_OFF);
  assert(!c->controls.buffs && !c->controls.contracts && !c->controls.expeditions &&
         !c->controls.quartermaster && !c->controls.web && !c->controls.public_release && !c->controls.pilot_count);
  for (level = 1; level <= 20; ++level)
    assert(c->xp_points[level - 1] == 100 * level * (level - 1));
  assert(c->initial_slots == 2 && c->max_slots == 3 && c->third_slot_level == 10 && c->combat_slots == 1);
  assert(c->xp_points[19] == 38000 && c->standings[CLAN_FAVOR_EXALTED].favor_points == 6000);
  assert(c->limits.account_full_xp_points == 600 && c->limits.account_half_xp_points == 600);
  assert(c->limits.clan_full_xp_points == 6000 && c->limits.clan_quarter_xp_points == 6000);
  assert(c->limits.clan_weekly_influence_points == 5000 && c->limits.stored_influence_points == 20000);
  assert(c->day_seconds == 86400 && c->week_seconds == 604800 && c->weekly_reset_iso_weekday == 1);
  assert(c->contracts.expedition_min_accounts == 2 && c->contracts.expedition_max_accounts == 6);
  assert(c->contracts.expedition_expiry_seconds == 7200 && c->contracts.delivery_ammo_shots == 100);
  assert(clan_progression_buff(c, CLAN_BUFF_ARTISAN)->tiers[0].unlock_level == 3);
  assert(clan_progression_buff(c, CLAN_BUFF_VITALITY)->category == CLAN_CATEGORY_COMBAT);
  assert(clan_progression_buff(c, CLAN_BUFF_ARCANE)->tiers[2].amount == 20);
  assert(clan_progression_buff(c, CLAN_BUFF_PSIONIC)->tiers[2].amount == 30);
  assert(clan_progression_buff(c, CLAN_BUFF_SCHOLAR)->tiers[2].amount == 10);
  assert(clan_progression_buff(c, CLAN_BUFF_ARTISAN)->tiers[2].amount == 4);
  assert(clan_progression_reward(c, CLAN_SOURCE_EXPEDITION_PERSONAL)->points.xp == 0);
  assert(clan_progression_reward(c, CLAN_SOURCE_EXPEDITION_PERSONAL)->points.favor == 100);
  assert(clan_progression_reward(c, CLAN_SOURCE_KILL)->weekly_claims == 200);
  assert(clan_progression_reward(c, CLAN_SOURCE_ASSAULT_SHARED)->points.xp == 600);
  for (level = 1; level <= CLAN_PROGRESSION_BUFF_COUNT; ++level) {
    const struct clan_buff_definition *b = clan_progression_buff(c, (enum clan_buff_family)level);
    assert(b && b->duration_seconds == 86400 && b->downtime_seconds == 43200);
    assert(b->duration_seconds + b->downtime_seconds == 129600);
    assert(b->tiers[0].influence_points == 100 && b->tiers[1].influence_points == 150 && b->tiers[2].influence_points == 225);
    assert(b->tiers[1].unlock_level == 5 && b->tiers[2].unlock_level == 20);
  }
}

static void identities_and_units(void)
{
  int64_t units = -7;
  char nonterminated[CLAN_PROGRESSION_UID_SIZE];
  struct clan_progression_config c = clan_progression_v1;
  assert(clan_progression_uid_valid(uid_a));
  assert(!clan_progression_uid_valid(NULL));
  assert(!clan_progression_uid_valid(""));
  assert(!clan_progression_uid_valid("0123"));
  assert(!clan_progression_uid_valid("00000000000000000000000000000000"));
  assert(!clan_progression_uid_valid("0123456789ABCDEF0123456789ABCDEF"));
  assert(!clan_progression_uid_valid("0123456789abcdef0123456789abcde'"));
  assert(!clan_progression_uid_valid("0123456789abcdef0123456789abcdefx"));
  memset(nonterminated, 'a', sizeof(nonterminated));
  assert(!clan_progression_uid_valid(nonterminated));
  assert(!clan_progression_points_to_eighths(-1, &units) && units == -7);
  assert(!clan_progression_points_to_eighths(INT64_MAX, &units) && units == -7);
  assert(!clan_progression_points_to_eighths(1, NULL));
  assert(clan_progression_points_to_eighths(0, &units) && units == 0);
  assert(clan_progression_points_to_eighths(INT64_MAX / 8, &units) && units == (INT64_MAX / 8) * 8);
  c.controls.pilot_count = CLAN_PROGRESSION_MAX_PILOT_CLANS + 1;
  rejected(&c, CLAN_CONFIG_IDENTITY);
  c = clan_progression_v1; c.controls.pilot_count = 1;
  memcpy(c.controls.pilot_clans[0], nonterminated, sizeof(nonterminated));
  rejected(&c, CLAN_CONFIG_IDENTITY);
  strcpy(c.controls.pilot_clans[0], uid_a);
  c.controls.pilot_count = 2; strcpy(c.controls.pilot_clans[1], uid_a);
  rejected(&c, CLAN_CONFIG_IDENTITY);
}

static void invalid_configuration(void)
{
  struct clan_progression_config c;
  const char *reason = NULL;
  assert(clan_progression_config_validate(NULL, &reason) == CLAN_CONFIG_MISSING && reason);
#define BAD(change, error) do { c = clan_progression_v1; change; rejected(&c, error); } while (0)
  BAD(c.rules_version++, CLAN_CONFIG_VERSION);
  BAD(c.schema_version++, CLAN_CONFIG_VERSION);
  BAD(c.controls.store_mode = (enum clan_store_mode)-1, CLAN_CONFIG_CONTROLS);
  BAD(c.controls.mode = (enum clan_progression_mode)99, CLAN_CONFIG_CONTROLS);
  BAD(c.controls.mode = CLAN_PROGRESSION_LIVE, CLAN_CONFIG_CONTROLS);
  BAD(c.controls.buffs = true, CLAN_CONFIG_CONTROLS);
  BAD(c.max_level = 21, CLAN_CONFIG_LEVELS);
  BAD(c.xp_points[0] = 1, CLAN_CONFIG_LEVELS);
  BAD(c.xp_points[4] = c.xp_points[3], CLAN_CONFIG_LEVELS);
  BAD(c.xp_points[19] = INT64_MAX, CLAN_CONFIG_LEVELS);
  BAD(c.xp_points[19] = -1, CLAN_CONFIG_LEVELS);
  BAD(c.max_slots = 4, CLAN_CONFIG_SLOTS);
  BAD(c.initial_slots = 0, CLAN_CONFIG_SLOTS);
  BAD(c.combat_slots = 2, CLAN_CONFIG_SLOTS);
  BAD(c.third_slot_level = 21, CLAN_CONFIG_SLOTS);
  BAD(c.buffs[0].family = CLAN_BUFF_NONE, CLAN_CONFIG_BUFFS);
  BAD(c.buffs[0].family = (enum clan_buff_family)99, CLAN_CONFIG_BUFFS);
  BAD(c.buffs[1].family = c.buffs[0].family, CLAN_CONFIG_BUFFS);
  BAD(c.buffs[3].category = CLAN_CATEGORY_RECOVERY, CLAN_CONFIG_BUFFS);
  BAD(c.buffs[0].name = NULL, CLAN_CONFIG_BUFFS);
  BAD(c.buffs[0].duration_seconds = 0, CLAN_CONFIG_BUFFS);
  BAD(c.buffs[0].downtime_seconds = 0, CLAN_CONFIG_BUFFS);
  BAD(c.buffs[0].downtime_seconds = -1, CLAN_CONFIG_BUFFS);
  BAD(c.buffs[0].duration_seconds = INT64_MAX, CLAN_CONFIG_BUFFS);
  BAD(c.buffs[0].tiers[0].influence_points = -1, CLAN_CONFIG_BUFFS);
  BAD(c.buffs[0].tiers[0].influence_points = 0, CLAN_CONFIG_BUFFS);
  BAD(c.buffs[0].tiers[2].influence_points = INT64_MAX / 8, CLAN_CONFIG_BUFFS);
  BAD(c.buffs[0].tiers[1].unlock_level = 1, CLAN_CONFIG_BUFFS);
  BAD(c.buffs[1].tiers[2].amount = 21, CLAN_CONFIG_BUFFS);
  BAD(c.buffs[2].tiers[2].amount = 31, CLAN_CONFIG_BUFFS);
  BAD(c.buffs[3].tiers[2].amount = 16, CLAN_CONFIG_BUFFS);
  BAD(c.buffs[4].tiers[2].amount = 5, CLAN_CONFIG_BUFFS);
  BAD(c.buffs[0].tiers[2].amount = 11, CLAN_CONFIG_BUFFS);
  BAD(c.standings[2].favor_points = c.standings[1].favor_points, CLAN_CONFIG_STANDINGS);
  BAD(c.standings[2].standing = CLAN_FAVOR_PROVEN, CLAN_CONFIG_STANDINGS);
  BAD(c.standings[2].stock_clan_level = 21, CLAN_CONFIG_STANDINGS);
  BAD(c.rewards[1].source = c.rewards[0].source, CLAN_CONFIG_REWARDS);
  BAD(c.rewards[0].source = (enum clan_reward_source)-1, CLAN_CONFIG_REWARDS);
  BAD(c.rewards[0].points.xp = -1, CLAN_CONFIG_REWARDS);
  BAD(c.rewards[0].points.xp = INT64_MAX / 8, CLAN_CONFIG_REWARDS);
  BAD(c.rewards[0].weekly_claims = 0, CLAN_CONFIG_REWARDS);
  BAD(c.rewards[0].weekly_claims = INT_MAX; c.rewards[0].points.xp = INT64_MAX / 8, CLAN_CONFIG_REWARDS);
  BAD(c.rewards[0].points.influence = 2, CLAN_CONFIG_REWARDS);
  BAD(c.rewards[5].points.xp = 100, CLAN_CONFIG_REWARDS);
  BAD(c.rewards[6].points.favor = 100, CLAN_CONFIG_REWARDS);
  BAD(c.rewards[6].scope = CLAN_REWARD_ACCOUNT, CLAN_CONFIG_REWARDS);
  BAD(c.limits.account_full_xp_points = 0, CLAN_CONFIG_LIMITS);
  BAD(c.limits.account_full_xp_points = INT64_MAX / 8, CLAN_CONFIG_LIMITS);
  BAD(c.limits.clan_quarter_xp_points = INT64_MAX, CLAN_CONFIG_LIMITS);
  BAD(c.limits.stored_influence_points = 1, CLAN_CONFIG_LIMITS);
  BAD(c.limits.pending_per_server = 1, CLAN_CONFIG_LIMITS);
  BAD(c.day_seconds = 0, CLAN_CONFIG_CLOCK);
  BAD(c.week_seconds = 86400, CLAN_CONFIG_CLOCK);
  BAD(c.weekly_reset_iso_weekday = 7, CLAN_CONFIG_CLOCK);
  BAD(c.participation.useful_actions = 1, CLAN_CONFIG_PARTICIPATION);
  BAD(c.participation.recent_seconds = 0, CLAN_CONFIG_PARTICIPATION);
  BAD(c.participation.healing_percent = 101, CLAN_CONFIG_PARTICIPATION);
  BAD(c.contracts.expedition_min_accounts = 1, CLAN_CONFIG_CONTRACTS);
  BAD(c.contracts.expedition_max_accounts = 7, CLAN_CONFIG_CONTRACTS);
  BAD(c.contracts.delivery_ammo_shots = 0, CLAN_CONFIG_CONTRACTS);
  BAD(c.contracts.expedition_expiry_seconds = INT64_MAX, CLAN_CONFIG_CONTRACTS);
  BAD(c.contracts.personal_objective_percent = 49, CLAN_CONFIG_CONTRACTS);
#undef BAD
}

static void release_gates(void)
{
  struct clan_progression_config c = clan_progression_v1;
  struct clan_progression_readiness ready = {true, true, true, true, CLAN_PROGRESSION_SCHEMA_VERSION};
  bool supported = clan_progression_campaign_supported();
  int feature;
#ifdef CLAN_TEST_DRAGONLANCE
  assert(supported);
#else
  assert(!supported);
#endif
  for (feature = 0; feature < CLAN_FEATURE_COUNT; ++feature)
    assert(!clan_progression_feature_available(&c, (enum clan_progression_feature)feature, &ready, uid_a));
  c.controls.store_mode = CLAN_STORE_MYSQL; c.controls.mode = CLAN_PROGRESSION_SHADOW;
  c.controls.public_release = true;
  assert(clan_progression_config_validate(&c, NULL) == CLAN_CONFIG_OK);
  assert(!clan_progression_feature_available(&c, CLAN_FEATURE_EARNINGS, &ready, uid_a));
  c.controls.mode = CLAN_PROGRESSION_LIVE; c.controls.public_release = false;
  assert(clan_progression_config_validate(&c, NULL) == CLAN_CONFIG_OK);
  assert(!clan_progression_feature_available(&c, CLAN_FEATURE_EARNINGS, &ready, uid_a));
  c.controls.pilot_count = 1; strcpy(c.controls.pilot_clans[0], uid_a);
  assert(clan_progression_feature_available(&c, CLAN_FEATURE_EARNINGS, &ready, uid_a) == supported);
  assert(!clan_progression_feature_available(&c, CLAN_FEATURE_EARNINGS, &ready, uid_b));
  assert(!clan_progression_feature_available(&c, CLAN_FEATURE_BUFFS, &ready, uid_a));
  c.controls.buffs = c.controls.contracts = c.controls.expeditions = c.controls.quartermaster = c.controls.web = true;
  assert(clan_progression_config_validate(&c, NULL) == CLAN_CONFIG_OK);
  for (feature = 0; feature < CLAN_FEATURE_COUNT; ++feature)
    assert(clan_progression_feature_available(&c, (enum clan_progression_feature)feature, &ready, uid_a) == supported);
  ready.schema_ready = false;
  assert(!clan_progression_feature_available(&c, CLAN_FEATURE_BUFFS, &ready, uid_a));
  ready.schema_ready = true; ready.schema_version++;
  assert(!clan_progression_feature_available(&c, CLAN_FEATURE_EARNINGS, &ready, uid_a));
  ready.schema_version = CLAN_PROGRESSION_SCHEMA_VERSION; ready.contracts_ready = false;
  assert(!clan_progression_feature_available(&c, CLAN_FEATURE_CONTRACTS, &ready, uid_a));
  assert(!clan_progression_feature_available(&c, CLAN_FEATURE_EXPEDITIONS, &ready, uid_a));
  ready.contracts_ready = true; ready.expeditions_ready = false; ready.catalog_ready = false;
  assert(!clan_progression_feature_available(&c, CLAN_FEATURE_EXPEDITIONS, &ready, uid_a));
  assert(!clan_progression_feature_available(&c, CLAN_FEATURE_QUARTERMASTER, &ready, uid_a));
  c.controls.public_release = true;
  assert(clan_progression_feature_available(&c, CLAN_FEATURE_EARNINGS, &ready, uid_b) == supported);
  assert(!clan_progression_feature_available(&c, CLAN_FEATURE_EARNINGS, &ready, ""));
  assert(!clan_progression_feature_available(NULL, CLAN_FEATURE_EARNINGS, &ready, uid_a));
  assert(!clan_progression_feature_available(&c, CLAN_FEATURE_EARNINGS, NULL, uid_a));
  assert(!clan_progression_feature_available(&c, (enum clan_progression_feature)99, &ready, uid_a));
  c.buffs[0].downtime_seconds = 0;
  assert(!clan_progression_feature_available(&c, CLAN_FEATURE_EARNINGS, &ready, uid_a));
}

static void lookup_order(void)
{
  struct clan_progression_config c = clan_progression_v1;
  struct clan_buff_definition buff = c.buffs[0];
  struct clan_reward_definition reward = c.rewards[0];
  c.buffs[0] = c.buffs[4]; c.buffs[4] = buff;
  c.rewards[0] = c.rewards[8]; c.rewards[8] = reward;
  assert(clan_progression_config_validate(&c, NULL) == CLAN_CONFIG_OK);
  assert(clan_progression_buff(&c, CLAN_BUFF_ARTISAN)->tiers[0].amount == 2);
  assert(clan_progression_reward(&c, CLAN_SOURCE_KILL)->points.xp == 2);
  assert(!clan_progression_buff(NULL, CLAN_BUFF_SCHOLAR));
  assert(!clan_progression_buff(&c, CLAN_BUFF_NONE));
  assert(!clan_progression_reward(NULL, CLAN_SOURCE_KILL));
  assert(!clan_progression_reward(&c, (enum clan_reward_source)100));
}

int main(void)
{
  defaults(); identities_and_units(); invalid_configuration(); release_gates(); lookup_order();
  puts("Clan progression Phase 3: defaults, invalid/overflowing configuration, identities, stable IDs and disabled/campaign/schema/pilot/content gates passed.");
  return 0;
}
