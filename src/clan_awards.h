#ifndef KRYNN_CLAN_AWARDS_H
#define KRYNN_CLAN_AWARDS_H
#include "clan_rewards.h"
/* Version-1 receipt explanations. Reserve new bits rather than reusing them. */
enum clan_award_cap {
  CLAN_AWARD_XP_WEEK = 1, CLAN_AWARD_XP_LEVEL = 2,
  CLAN_AWARD_INFLUENCE_WEEK = 4, CLAN_AWARD_INFLUENCE_STORED = 8,
  CLAN_AWARD_ACCOUNT_BAND = 16, CLAN_AWARD_SOURCE_LIMIT = 32,
  CLAN_AWARD_FAVOR_WEEK = 64, CLAN_AWARD_SOURCE_REDUCED = 128
};
struct clan_award_projection {
  int64_t xp_eighths, influence_eighths, xp_after, influence_after;
  int level_before, level_after, cap_flags;
};
/* Pure queries; negative/corrupt/overflow inputs fail without changing output.
 * Level is derived, never independently saved. XP above the level cap is corrupt.
 * Incoming XP is already account-adjusted; personal projection supplies account/Favor policy. */
bool clan_award_level(const struct clan_progression_config *, int64_t xp, int *out);
bool clan_award_project(const struct clan_progression_config *,
    int64_t xp, int64_t influence, int64_t weekly_incoming, int64_t weekly_influence,
    int64_t incoming_xp, int64_t incoming_influence, struct clan_award_projection *out);
/* Trusted SQL service restricted to shared clan-only sources. Personal events
 * use the separate account/Favor service below. No prepare/eligibility
 * hook is connected. Pending decisions require enabled earnings/readiness and
 * frozen version-1 rules/default policy. Committed replays retain their receipt,
 * even after disabling earnings. Caller owns the exclusive SQL connection and
 * must refresh the runtime persistence cache after OK or COMMIT_UNKNOWN. */
enum clan_repository_result clan_award_complete_shared(MYSQL *, const char *operation_uid,
    int64_t now, const struct clan_progression_config *,
    const struct clan_progression_readiness *, struct json_object **out);
/* Pure version-1 personal decision for previews and locked receipt commits.
 * Trusted source adapters freeze eligible full/half/zero kill tuples; all other
 * sources require their exact table tuple. Daily prototype eligibility remains
 * a later source adapter's responsibility, not inferred from requested amounts.
 * Preview is advisory: commit recalculates under locks. Outputs stay unchanged
 * on invalid facts, unsupported rules, corrupt counters or arithmetic overflow. */
bool clan_award_personal_project(const struct clan_reward_event *,
    const struct clan_reward_snapshot *, struct clan_reward_delta *out);
/* Cumulative Favor standing, independent of membership or current clan level. */
bool clan_award_standing(int64_t favor_eighths, enum clan_favor_standing *out);
/* One stable explanation per individual cap bit; NULL for unknown/composite. */
const char *clan_award_explanation(int flag);
enum clan_repository_result clan_award_complete_personal(MYSQL *, const char *operation_uid,
    int64_t now, const struct clan_progression_config *,
    const struct clan_progression_readiness *, struct json_object **out);
#endif
