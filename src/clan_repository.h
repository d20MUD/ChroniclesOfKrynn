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
  CLAN_REPO_COMMIT_UNKNOWN
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
/* Canonicalize owned trees; rank ordering is preserved. */
bool clan_repository_snapshot_canonicalize(struct json_object *snapshot);
#endif
