#ifndef KRYNN_CLAN_BUFF_ACTIVATION_H
#define KRYNN_CLAN_BUFF_ACTIVATION_H
#include "clan_buff_queries.h"
#include "clan_repository.h"
/* New privilege ID; legacy IDs 0..20 are unchanged. Persisted independently
 * until the Phase 24 command/editor integration. Missing row = leader only. */
#define CLAN_BUFF_MANAGEMENT_PRIVILEGE 21
#define CLAN_BUFF_ACTIVATION_SCHEMA 1
struct clan_buff_request {
  char operation_uid[33], clan_uid[33], expected_activation_uid[33];
  int64_t actor_id;
  enum clan_buff_family family;
  int tier, slot, rules_version;
};
enum clan_buff_rejection {
  CLAN_BUFF_INVALID=1U, CLAN_BUFF_DISABLED=2U, CLAN_BUFF_DELETED=4U,
  CLAN_BUFF_NOT_MEMBER=8U, CLAN_BUFF_PERMISSION=16U, CLAN_BUFF_LOCKED=32U,
  CLAN_BUFF_BAD_SLOT=64U, CLAN_BUFF_ALREADY_ACTIVE=128U, CLAN_BUFF_COOLDOWN=256U,
  CLAN_BUFF_UNAFFORDABLE=512U, CLAN_BUFF_STALE=1024U, CLAN_BUFF_SLOT_OCCUPIED=2048U,
  CLAN_BUFF_COMBAT_LIMIT=4096U, CLAN_BUFF_CORRUPT=8192U, CLAN_BUFF_CLOCK=16384U
};
struct clan_buff_activation_snapshot {
  struct clan_repository_record clan;
  bool accepted_member, may_manage;
  struct clan_buff_state rows[CLAN_PROGRESSION_BUFF_COUNT];
  size_t count;
};
struct clan_buff_decision {
  unsigned int reasons; /* all applicable rejections; zero means permitted */
  struct clan_buff_state activation;
  enum clan_buff_family replaced_family;
  char replaced_activation_uid[33];
};
bool clan_buff_request_valid(const struct clan_buff_request *);
/* Pure validation. No mutation, cost, permission inference or wall-clock read.
 * The snapshot's permissions/readiness are trusted server facts. */
void clan_buff_validate(const struct clan_progression_config *,
    const struct clan_progression_readiness *, const struct clan_buff_request *,
    const struct clan_buff_activation_snapshot *, int64_t now, struct clan_buff_decision *);
const char *clan_buff_rejection_text(unsigned int reason);
/* Caller owns an exclusive pool connection; no nested transaction. The config
 * and now are server-owned. Reuse the exact request/operation ID on retry.
 * OK returns immutable receipt JSON owned by caller. INELIGIBLE returns a pure
 * decision with reasons. Other failures leave both outputs unchanged. Committed
 * retries return their original receipt even after departure/config changes.
 * No effects, cache mutation, commands, auto-migration or live enablement. */
enum clan_repository_result clan_buff_activation_commit(MYSQL *,
    const struct clan_progression_config *, const struct clan_buff_request *, int64_t now,
    struct clan_buff_decision *, struct json_object **receipt);
/* Notification only: callback must not query SQL on the still-owned connection.
 * Successful/ambiguous commits invalidate cached views; no guessed effects. */
void clan_buff_set_invalidation(void (*callback)(const char *, void *), void *context);
void clan_buff_invalidate(const char *clan_uid);
/* Internal locked service composition, not player-facing authorization. */
enum clan_repository_result clan_buff_activation_schema_check(MYSQL *);
enum clan_repository_result clan_buff_locked_snapshot(MYSQL *, const struct clan_buff_request *,
    struct clan_buff_activation_snapshot *);
#ifdef CLAN_BUFF_ACTIVATION_TEST
/* Fault/crash stages: debit, replacement, family, receipt, precommit, postcommit. */
extern int clan_buff_activation_fault, clan_buff_activation_crash;
#endif
#endif
