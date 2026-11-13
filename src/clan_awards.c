/* Phases 9–10: checked clan/personal policy, no gameplay source hooks. */
#include "clan_awards.h"
#include <string.h>

bool clan_award_level(const struct clan_progression_config *c, int64_t xp, int *out)
{
  int i, level = 1;
  int64_t threshold, maximum;
  if (!out || xp < 0 || clan_progression_config_validate(c, NULL) != CLAN_CONFIG_OK ||
      !clan_progression_points_to_eighths(c->xp_points[c->max_level - 1], &maximum) ||
      (c->rules_version == 1 && xp > maximum)) return false;
  for (i = 1; i < c->max_level; i++) {
    if (!clan_progression_points_to_eighths(c->xp_points[i], &threshold)) return false;
    if (xp < threshold) break;
    level = i + 1;
  }
  *out = level;
  return true;
}
static int64_t lesser(int64_t a, int64_t b) { return a < b ? a : b; }

bool clan_award_project(const struct clan_progression_config *c,
    int64_t xp, int64_t influence, int64_t used, int64_t weekly_influence,
    int64_t incoming, int64_t incoming_influence, struct clan_award_projection *out)
{
  struct clan_award_projection p;
  int64_t full, quarter, week_cap, stored_cap, max_xp, remaining, amount, band_used;
  memset(&p, 0, sizeof(p));
  if (!out || !clan_award_level(c, xp, &p.level_before) || influence < 0 || used < 0 ||
      weekly_influence < 0 || incoming < 0 || incoming_influence < 0 ||
      incoming > INT64_MAX - used ||
      !clan_progression_points_to_eighths(c->limits.clan_full_xp_points, &full) ||
      !clan_progression_points_to_eighths(c->limits.clan_quarter_xp_points, &quarter) ||
      !clan_progression_points_to_eighths(c->limits.clan_weekly_influence_points, &week_cap) ||
      !clan_progression_points_to_eighths(c->limits.stored_influence_points, &stored_cap) ||
      !clan_progression_points_to_eighths(c->xp_points[c->max_level-1], &max_xp) ||
      influence > stored_cap || weekly_influence > week_cap) return false;
  if (c->rules_version >= 2) {
    /* Exact whole-eighth arithmetic, rounded down conservatively. The old XP
     * column is the non-spendable lifetime Influence counter under rules 2. */
    incoming_influence /= 8;
    max_xp = INT64_MAX;
  }
  remaining = incoming;
  if (used < full) {
    p.xp_eighths = lesser(remaining, full - used);
    remaining -= p.xp_eighths;
    used += p.xp_eighths;
  }
  band_used = used > full ? used - full : 0;
  if (band_used < quarter) {
    amount = lesser(remaining, quarter - band_used);
    /* Difference of cumulative quotients retains sub-eighth band remainders
     * across events without multiplying or overflowing. */
    p.xp_eighths += (band_used + amount) / 4 - band_used / 4;
  }
  if (p.xp_eighths < incoming) p.cap_flags |= CLAN_AWARD_XP_WEEK;
  if (p.xp_eighths > max_xp - xp) {
    p.xp_eighths = max_xp - xp;
    p.cap_flags |= CLAN_AWARD_XP_LEVEL;
  }
  if (incoming_influence > week_cap - weekly_influence)
    p.cap_flags |= CLAN_AWARD_INFLUENCE_WEEK;
  if (incoming_influence > stored_cap - influence)
    p.cap_flags |= CLAN_AWARD_INFLUENCE_STORED;
  p.influence_eighths = lesser(incoming_influence,
      lesser(week_cap - weekly_influence, stored_cap - influence));
  if (c->rules_version >= 2) {
    p.xp_eighths = p.influence_eighths;
    p.cap_flags &= ~(CLAN_AWARD_XP_WEEK | CLAN_AWARD_XP_LEVEL);
    if (p.xp_eighths > INT64_MAX - xp) return false;
  }
  p.xp_after = xp + p.xp_eighths;
  p.influence_after = influence + p.influence_eighths;
  if (!clan_award_level(c, p.xp_after, &p.level_after)) return false;
  *out = p;
  return true;
}
struct shared_context {
  const struct clan_progression_config *config;
  const struct clan_progression_readiness *ready;
};
static bool decide_shared(const struct clan_reward_event *e,
    const struct clan_reward_snapshot *s, struct clan_reward_delta *out, void *context)
{
  struct shared_context *ctx = context;
  struct clan_award_projection p;
  struct clan_reward_delta d;
  const struct clan_reward_definition *source;
  int64_t expected_xp, expected_influence;
  /* Version 1 has immutable balance policy. Runtime release controls may vary,
   * but a changed balance table must get a separately supported rules version. */
  const struct clan_progression_config *policy =
      e->rules_version == 1 ? &clan_progression_v1 : &clan_progression_defaults;
  if (!ctx || !ctx->config ||
      (ctx->config->rules_version >= 2 && e->rules_version != CLAN_PROGRESSION_RULES_VERSION) ||
      (e->rules_version != 1 && e->rules_version != CLAN_PROGRESSION_RULES_VERSION) ||
      !clan_progression_feature_available(ctx->config, CLAN_FEATURE_EARNINGS,
          ctx->ready, e->clan_uid) || e->player_id || e->account_id ||
      e->requested_favor_eighths || s->clan.status != CLAN_RECORD_ACTIVE ||
      strcmp(s->clan.uid, e->clan_uid)) return false;
  source = clan_progression_reward(policy, e->source);
  if (!source || source->scope != CLAN_REWARD_SHARED ||
      !clan_progression_points_to_eighths(source->points.xp, &expected_xp) ||
      !clan_progression_points_to_eighths(source->points.influence, &expected_influence) ||
      e->requested_xp_eighths != expected_xp ||
      e->requested_influence_eighths != expected_influence ||
      !clan_award_project(policy, s->clan.xp_eighths, s->clan.influence_eighths,
          s->weekly.incoming_xp_eighths, s->weekly.influence_eighths,
          e->requested_xp_eighths, e->requested_influence_eighths, &p)) return false;
  memset(&d, 0, sizeof(d));
  d.xp_eighths = p.xp_eighths;
  d.influence_eighths = p.influence_eighths;
  d.incoming_xp_eighths = e->requested_xp_eighths;
  d.cap_flags = p.cap_flags;
  *out = d;
  return true;
}
enum clan_repository_result clan_award_complete_shared(MYSQL *db, const char *uid,
    int64_t now, const struct clan_progression_config *config,
    const struct clan_progression_readiness *ready, struct json_object **out)
{
  struct shared_context ctx = {config, ready};
  return clan_reward_complete(db, uid, now, decide_shared, &ctx, out);
}


bool clan_award_standing(int64_t favor, enum clan_favor_standing *out)
{
  int i;
  enum clan_favor_standing standing = CLAN_FAVOR_UNPROVEN;
  if (!out || favor < 0) return false;
  for (i = 1; i < CLAN_PROGRESSION_STANDING_COUNT; i++) {
    int64_t threshold;
    if (!clan_progression_points_to_eighths(
        clan_progression_defaults.standings[i].favor_points, &threshold)) return false;
    if (favor < threshold) break;
    standing = clan_progression_defaults.standings[i].standing;
  }
  *out = standing;
  return true;
}
const char *clan_award_explanation(int flag)
{
  switch (flag) {
  case CLAN_AWARD_XP_WEEK: return "Clan weekly XP reduced or exhausted.";
  case CLAN_AWARD_XP_LEVEL: return "Clan has reached its maximum XP level.";
  case CLAN_AWARD_INFLUENCE_WEEK: return "Clan weekly Influence allowance exhausted.";
  case CLAN_AWARD_INFLUENCE_STORED: return "Clan Influence storage is full.";
  case CLAN_AWARD_ACCOUNT_BAND: return "Account weekly contribution reduced or exhausted.";
  case CLAN_AWARD_SOURCE_LIMIT: return "Account weekly source claim limit reached.";
  case CLAN_AWARD_FAVOR_WEEK: return "Account weekly Favor allowance exhausted.";
  case CLAN_AWARD_SOURCE_REDUCED: return "Source credit reduced before account limits.";
  default: return NULL;
  }
}
bool clan_award_personal_project(const struct clan_reward_event *e,
    const struct clan_reward_snapshot *s, struct clan_reward_delta *out)
{
  const struct clan_progression_config *c =
      e && e->rules_version == 1 ? &clan_progression_v1 : &clan_progression_defaults;
  const struct clan_reward_definition *source;
  struct clan_reward_delta d;
  struct clan_award_projection clan;
  struct clan_reset_period period;
  int64_t xp, influence, favor, expected_xp, expected_influence, expected_favor;
  int64_t full, half, favor_cap, used, remaining, amount, adjusted;
  if (!out || !s || !clan_reward_event_valid(e) ||
      !(source = clan_progression_reward(c, e->source)) ||
      source->scope != CLAN_REWARD_ACCOUNT || s->clan.status != CLAN_RECORD_ACTIVE ||
      strcmp(s->clan.uid, e->clan_uid) ||
      !clan_progression_week_at(e->occurred_at, &period) ||
      !clan_progression_account_window_valid(&s->account) ||
      s->account.account_id != e->account_id || s->account.starts_at != period.starts_at ||
      s->account.base_xp_eighths % 8 || s->favor.character_id != e->player_id ||
      strcmp(s->favor.clan_uid, e->clan_uid) || s->favor.favor_eighths < 0 ||
      s->weekly.starts_at != period.starts_at || strcmp(s->weekly.clan_uid, e->clan_uid) ||
      !clan_progression_points_to_eighths(source->points.xp, &expected_xp) ||
      !clan_progression_points_to_eighths(source->points.influence, &expected_influence) ||
      !clan_progression_points_to_eighths(source->points.favor, &expected_favor) ||
      !clan_progression_points_to_eighths(c->limits.account_full_xp_points, &full) ||
      !clan_progression_points_to_eighths(c->limits.account_half_xp_points, &half) ||
      !clan_progression_points_to_eighths(c->limits.account_favor_points, &favor_cap) ||
      s->account.favor_eighths > favor_cap) return false;
  xp = e->requested_xp_eighths;
  influence = e->requested_influence_eighths;
  favor = e->requested_favor_eighths;
  memset(&d, 0, sizeof(d));
  if (xp != expected_xp || influence != expected_influence || favor != expected_favor) {
    /* Only the trusted ordinary-kill adapter may freeze prototype reductions. */
    if (e->source != CLAN_SOURCE_KILL ||
        !((xp == expected_xp/2 && influence == expected_influence/2 && favor == expected_favor/2) ||
          (!xp && !influence && !favor))) return false;
    d.cap_flags |= CLAN_AWARD_SOURCE_REDUCED;
  }
  if (s->account.claims[e->source] >= source->weekly_claims) {
    d.cap_flags |= CLAN_AWARD_SOURCE_LIMIT;
    xp = influence = favor = 0;
  } else if (xp || influence || favor) {
    d.account_claims = 1;
    d.account_base_xp_eighths = xp;
    if (xp) {
      used = s->account.base_xp_eighths;
      if (xp > INT64_MAX - used) return false;
      remaining = xp;
      adjusted = 0;
      if (used < full) {
        amount = lesser(remaining, full - used);
        adjusted += amount;
        remaining -= amount;
        used += amount;
      }
      used = used > full ? used - full : 0;
      if (used < half) adjusted += lesser(remaining, half - used)/2;
      if (adjusted < xp) d.cap_flags |= CLAN_AWARD_ACCOUNT_BAND;
      /* Version-1 tuples have XP:Influence:Favor = 2:1:1. Full/half
       * source credit and whole-point base usage preserve exact eighths. */
      xp = adjusted;
      influence = favor = adjusted/2;
    }
  }
  if (!clan_award_project(c, s->clan.xp_eighths, s->clan.influence_eighths,
      s->weekly.incoming_xp_eighths, s->weekly.influence_eighths, xp, influence, &clan))
    return false;
  d.xp_eighths = clan.xp_eighths;
  d.influence_eighths = clan.influence_eighths;
  d.incoming_xp_eighths = xp;
  d.cap_flags |= clan.cap_flags;
  if (c->rules_version >= 2) favor /= 4;
  if (favor > favor_cap - s->account.favor_eighths) {
    favor = favor_cap - s->account.favor_eighths;
    d.cap_flags |= CLAN_AWARD_FAVOR_WEEK;
  }
  if (favor > INT64_MAX - s->favor.favor_eighths) return false;
  d.favor_eighths = favor;
  *out = d;
  return true;
}
static bool decide_personal(const struct clan_reward_event *e,
    const struct clan_reward_snapshot *s, struct clan_reward_delta *out, void *context)
{
  struct shared_context *ctx = context;
  if (!ctx || !ctx->config || (ctx->config->rules_version >= 2 && e->rules_version != CLAN_PROGRESSION_RULES_VERSION) ||
      !clan_progression_feature_available(ctx->config, CLAN_FEATURE_EARNINGS,
      ctx->ready, e->clan_uid)) return false;
  return clan_award_personal_project(e, s, out);
}
enum clan_repository_result clan_award_complete_personal(MYSQL *db, const char *uid,
    int64_t now, const struct clan_progression_config *config,
    const struct clan_progression_readiness *ready, struct json_object **out)
{
  struct shared_context ctx = {config, ready};
  return clan_reward_complete(db, uid, now, decide_personal, &ctx, out);
}
