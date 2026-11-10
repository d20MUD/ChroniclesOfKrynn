#ifndef KRYNN_CLAN_REWARDS_H
#define KRYNN_CLAN_REWARDS_H
#include "clan_repository.h"
/* Trusted server facts, never browser amounts/identity. One event/source/player
 * claim across all clans; shared sources use the clan incarnation as recipient.
 * Supply the same IDs after any ambiguous acknowledgement. */
struct clan_reward_event {
  char operation_uid[33], event_uid[33], clan_uid[33];
  int64_t account_id, player_id, occurred_at;
  enum clan_reward_source source;
  int rules_version;
  int64_t requested_xp_eighths, requested_influence_eighths, requested_favor_eighths;
};
struct clan_reward_snapshot {
  struct clan_repository_record clan;
  struct clan_account_window account;
  struct clan_favor_state favor;
  struct clan_weekly_window weekly;
  bool account_present, favor_present, weekly_present;
};
struct clan_reward_delta {
  int64_t xp_eighths, influence_eighths, favor_eighths;
  int64_t account_base_xp_eighths, account_claims, incoming_xp_eighths;
  int cap_flags; /* server policy's versioned explanations, retained verbatim */
};
/* Called under SQL locks only for a pending event. Pure decision only: no SQL,
 * files, live character changes or commits. May run again after a rollback.
 * Later services supply policy; this coordinator never calculates reward caps. */
typedef bool (*clan_reward_decide_fn)(const struct clan_reward_event *,
    const struct clan_reward_snapshot *, struct clan_reward_delta *, void *context);
bool clan_reward_event_valid(const struct clan_reward_event *event);
/* Returned JSON trees are owned by caller. Outputs unchanged on errors.
 * prepare durably accepts facts, not an uncommitted cap decision. Complete
 * commits decide/counters/balances/receipt together. Already committed replays
 * return the original receipt without calling decide again. */
enum clan_repository_result clan_reward_prepare(MYSQL *,const struct clan_reward_event *,
    int64_t accepted_at,struct json_object **out);
/* Acceptance for future personal source adapters: accepted membership checked
 * under the same control lock as the durable event. Replays are checked first.
 * Lower-level prepare remains trusted recovery/test infrastructure. */
enum clan_repository_result clan_reward_prepare_member(MYSQL *, const struct clan_reward_event *,
    int64_t accepted_at, struct json_object **out);
enum clan_repository_result clan_reward_load(MYSQL *,const char *operation_uid,struct json_object **out);
enum clan_repository_result clan_reward_complete(MYSQL *,const char *operation_uid,int64_t now,
    clan_reward_decide_fn decide,void *context,struct json_object **out);
/* Explicit terminal rejection of pending facts. Cannot refund committed rewards
 * or free their event key. No file mutations/acknowledgements are provided. */
enum clan_repository_result clan_reward_cancel(MYSQL *,const char *operation_uid,
    int reason,int64_t now,struct json_object **out);
/* Bounded durable recovery enumeration. player_id=0 selects all recipients;
 * cursor NULL starts iteration; limit 1..1000. Returns operation UID strings. */
enum clan_repository_result clan_reward_pending(MYSQL *,int64_t player_id,
    const char *after_uid,size_t limit,struct json_object **out);
#ifdef CLAN_REWARD_TEST
/* Process interruption boundaries: prepare commit, account stage, clan stage,
 * Favor stage, weekly stage, receipt stage, pre-COMMIT, post-COMMIT. */
extern int clan_reward_fault;
extern int clan_reward_crash;
#endif
#endif
