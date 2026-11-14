#ifndef KRYNN_CLAN_REPOSITORY_H
#define KRYNN_CLAN_REPOSITORY_H
#include "clan_progression.h"
#include <mariadb/mysql.h>

/* The caller owns an exclusively acquired existing pool connection. These
 * functions never reconnect, fall back to files, or enable feature gates. */
enum clan_repository_result
{
  CLAN_REPO_OK,
  CLAN_REPO_NOT_FOUND,
  CLAN_REPO_INVALID,
  CLAN_REPO_UNAVAILABLE,
  CLAN_REPO_SCHEMA,
  CLAN_REPO_CONFLICT,
  CLAN_REPO_DELETED,
  CLAN_REPO_COMMIT_UNKNOWN,
  CLAN_REPO_CAPACITY,
  CLAN_REPO_INELIGIBLE
};
struct clan_repository_record
{
  char uid[33], name[61], description[49152], abbreviation[6];
  int vnum, status;
  int64_t leader_id, treasury_gold, founded_at, last_activity_at;
  int64_t created_at, updated_at, deleted_at, revision;
  int64_t xp_eighths, influence_eighths, state_revision, state_updated_at;
};
const char*
clan_repository_result_name(enum clan_repository_result result);
bool
clan_repository_uid_generate(char out[33]);
bool
clan_repository_record_valid(const struct clan_repository_record* record);
/* Runtime-only successful schema cache; disabled by default for offline tools.
 * Every call still checks connection/session/database/schema-version identity.
 * Invalidate before closing a connection or applying an intentional migration;
 * NULL invalidates every entry. Full metadata proofs expire after 60 seconds. */
void clan_repository_schema_cache_enable(bool enabled);
void clan_repository_schema_cache_invalidate(MYSQL* db);
/* Optional metadata proofs share the runtime cache and its invalidation rules.
 * Validators must inspect schema only, never mutable balances or receipts. */
enum clan_repository_result clan_repository_schema_extension_check(
    MYSQL* db, enum clan_repository_result (*validate)(MYSQL*));
enum clan_repository_result
clan_repository_schema_check(MYSQL* db);
enum clan_repository_result
clan_repository_load(MYSQL* db, const char* uid, struct clan_repository_record* out);
enum clan_repository_result
clan_repository_load_vnum(MYSQL* db, int vnum, struct clan_repository_record* out);
/* Creation always starts XP/Influence/revisions at zero. Supply a stable random
 * UID once and reuse it after an uncertain commit; read before retrying. */
enum clan_repository_result
clan_repository_create(MYSQL* db, const struct clan_repository_record* record);
/* Whole core/state update in one transaction with BOTH revision predicates.
 * Timestamp/identity/deleted-state checks happen before any write. */
enum clan_repository_result
clan_repository_save(MYSQL* db,
                     const struct clan_repository_record* record,
                     int64_t expected_revision,
                     int64_t expected_state_revision);
struct json_object;
/* Full snapshots own JSON references: caller releases returned trees with
 * json_object_put. Members include applicants; claims retain UID popularity. */
struct json_object*
clan_repository_record_json(const struct clan_repository_record* record);
bool
clan_repository_record_parse(struct json_object* object, struct clan_repository_record* record);
bool
clan_repository_snapshot_valid(struct json_object* snapshot);
bool
clan_repository_legacy_valid(struct json_object* details, const char* uid);
enum clan_repository_result
clan_repository_snapshot_read(MYSQL* db, struct json_object** out);
enum clan_repository_result
clan_repository_snapshot_write(MYSQL* db,
                               struct json_object* snapshot,
                               int64_t expected_store_revision,
                               struct json_object* import_manifest);
/* Internal service composition: caller already owns a transaction and the
 * global store-control lock. No nested BEGIN/COMMIT; output unchanged on error. */
enum clan_repository_result clan_repository_snapshot_read_locked(MYSQL *, struct json_object **);
/* Canonicalize owned trees; rank ordering is preserved. */
bool clan_repository_snapshot_canonicalize(struct json_object *snapshot);
/* Offline, quiesced cutover only. NULL digest explicitly selects a fresh,
 * empty store; otherwise require the reviewed import receipt. Revision CAS. */
enum clan_repository_result clan_repository_cutover(MYSQL *db,
    int64_t expected_revision, const char *source_digest);
/* Family history remains persisted after expiry/cancellation. Output unchanged
 * on error; no rows means no activations, never synthesized cooldowns. */
enum clan_repository_result clan_repository_buffs_read(MYSQL *db,
    struct clan_buff_state *out, size_t capacity, size_t *count);
bool clan_repository_buff_valid(const struct clan_buff_state *);
bool clan_repository_buff_active(const struct clan_buff_state *buff, int64_t now);
/* No membership dependency: former-clan standings remain addressable. Missing
 * rows return NOT_FOUND, not a persisted/inferred award. Revision -1 inserts. */
enum clan_repository_result clan_repository_favor_load(MYSQL *db,
    int64_t character_id, const char *clan_uid, struct clan_favor_state *out);
enum clan_repository_result clan_repository_favor_save(MYSQL *db,
    const struct clan_favor_state *state, int64_t expected_revision, int64_t now);
/* Uses occurrence time to select a retained account-global week. NOT_FOUND
 * leaves output unchanged; initialize a proposed zero row with the pure helper.
 * Saving a new week never deletes/resets an older week. Revision -1 inserts.
 * No gameplay hooks: Phase 8 must compose counters/balances with receipts. */
enum clan_repository_result clan_repository_account_window_load(MYSQL *db,
    int64_t account_id, int64_t occurred_at, struct clan_account_window *out);
enum clan_repository_result clan_repository_account_window_save(MYSQL *db,
    const struct clan_account_window *window, int64_t expected_revision, int64_t written_at);
/* Current accepted membership is separate from historical Favor ownership. */
enum clan_member_access {
  CLAN_ACCESS_ACCEPTED, CLAN_ACCESS_APPLICANT, CLAN_ACCESS_FORMER, CLAN_ACCESS_DELETED
};
enum clan_repository_result clan_repository_member_access(MYSQL *, int64_t player_id,
    const char *clan_uid, enum clan_member_access *out);
/* Prune a private candidate only; discard it on failure. Retains all historical
 * SQL rows when snapshot_write tombstones the omitted incarnation. */
bool clan_repository_snapshot_disband(struct json_object *, const char *clan_uid);
enum clan_repository_result clan_repository_disband(MYSQL *, const char *clan_uid,
    int64_t expected_store_revision, int64_t now);
#endif
