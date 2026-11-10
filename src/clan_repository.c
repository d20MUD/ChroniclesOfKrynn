/* Checked clan persistence. No startup/import/membership/gameplay hooks. */
#include "clan_repository.h"
#include "clan_rewards.h"
#include "clan_repository_schema.inc"
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <openssl/rand.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool
query(MYSQL* db, const char* fmt, ...)
{
  char* sql;
  int n;
  va_list args, copy;
  bool ok;
  if (!db)
    return false;
  va_start(args, fmt);
  va_copy(copy, args);
  n = vsnprintf(NULL, 0, fmt, args);
  va_end(args);
  if (n < 0 || n > 1048576)
  {
    va_end(copy);
    return false;
  }
  sql = malloc((size_t)n + 1);
  if (!sql)
  {
    va_end(copy);
    return false;
  }
  vsnprintf(sql, (size_t)n + 1, fmt, copy);
  va_end(copy);
  ok = mysql_real_query(db, sql, (unsigned long)n) == 0;
  free(sql);
  return ok;
}
static enum clan_repository_result
failure(MYSQL* db)
{
  unsigned int e = db ? mysql_errno(db) : 2006;
  return (e == 1062 || e == 1205 || e == 1213)   ? CLAN_REPO_CONFLICT
         : (e == 2006 || e == 2013 || e == 2055) ? CLAN_REPO_UNAVAILABLE
                                                 : CLAN_REPO_INVALID;
}
static bool
strict_session(MYSQL* db)
{
  MYSQL_RES* result;
  MYSQL_ROW row;
  bool strict = false;
  if (!query(db, "SELECT @@SESSION.sql_mode,@@autocommit"))
    return false;
  result = mysql_store_result(db);
  if (!result)
    return false;
  row = mysql_fetch_row(result);
  if (row && row[0] && row[1])
    strict = (strstr(row[0], "STRICT_ALL_TABLES") || strstr(row[0], "STRICT_TRANS_TABLES")) &&
             !strcmp(row[1], "1");
  mysql_free_result(result);
  return strict;
}
static void
canonical(char* out, size_t size, const char* in)
{
  size_t n = 0;
  while (*in && n + 1 < size)
  {
    unsigned char c = (unsigned char)*in++;
    if (!isspace(c) && c != '`' && c != '(' && c != ')')
      out[n++] = (char)tolower(c);
  }
  out[n] = 0;
}
static bool
integer(const char* s, int64_t* out)
{
  char* end;
  intmax_t n;
  const char* p = s;
  if (!s || !*s)
    return false;
  while (*p)
    if (*p < '0' || *p++ > '9')
      return false;
  errno = 0;
  n = strtoimax(s, &end, 10);
  if (errno || *end || n < 0 || n > INT64_MAX)
    return false;
  *out = (int64_t)n;
  return true;
}
static void
hex(char* out, const char* text)
{
  static const char digits[] = "0123456789abcdef";
  while (*text)
  {
    unsigned char c = (unsigned char)*text++;
    *out++ = digits[c >> 4];
    *out++ = digits[c & 15];
  }
  *out = 0;
}
const char*
clan_repository_result_name(enum clan_repository_result r)
{
  static const char* names[] = { "ok",          "not-found",       "invalid",
                                 "unavailable", "schema-mismatch", "conflict",
                                 "deleted",     "commit-unknown", "capacity", "ineligible" };
  return r >= CLAN_REPO_OK && r <= CLAN_REPO_INELIGIBLE ? names[r] : "invalid-result";
}
bool
clan_repository_uid_generate(char out[33])
{
  unsigned char bytes[16];
  static const char digits[] = "0123456789abcdef";
  size_t i;
  if (!out || RAND_bytes(bytes, sizeof(bytes)) != 1)
    return false;
  for (i = 0; i < 16; i++)
  {
    out[2 * i] = digits[bytes[i] >> 4];
    out[2 * i + 1] = digits[bytes[i] & 15];
  }
  out[32] = 0;
  return clan_progression_uid_valid(out);
}
bool
clan_repository_record_valid(const struct clan_repository_record* r)
{
  if (!r || !memchr(r->uid, 0, sizeof(r->uid)) || !clan_progression_uid_valid(r->uid) ||
      !memchr(r->name, 0, sizeof(r->name)) || !memchr(r->description, 0, sizeof(r->description)) ||
      !memchr(r->abbreviation, 0, sizeof(r->abbreviation)) || !*r->name || r->vnum < 1 ||
      r->vnum > 65534 || (r->status != CLAN_RECORD_ACTIVE && r->status != CLAN_RECORD_DELETED))
    return false;
  return r->leader_id >= 0 && r->treasury_gold >= 0 && r->founded_at >= 0 &&
         r->last_activity_at >= 0 && r->created_at >= 0 && r->updated_at >= r->created_at &&
         r->revision >= 0 && r->state_revision >= 0 && r->xp_eighths >= 0 &&
         r->influence_eighths >= 0 && r->state_updated_at >= 0 &&
         ((r->status == CLAN_RECORD_ACTIVE && r->deleted_at == 0) ||
          (r->status == CLAN_RECORD_DELETED && r->deleted_at >= r->created_at &&
           r->updated_at >= r->deleted_at));
}
static enum clan_repository_result schema_failure(MYSQL *db, int line)
{
#ifdef CLAN_REWARD_TEST
  fprintf(stderr, "Clan schema contract failure at line %d: mysql %u %s\n", line, db ? mysql_errno(db) : 0, db ? mysql_error(db) : "no connection");
#else
  (void)db;
  (void)line;
#endif
  return CLAN_REPO_SCHEMA;
}
enum clan_repository_result
clan_repository_schema_check(MYSQL* db)
{
  MYSQL_RES* result;
  MYSQL_ROW row;
  size_t i;
  char actual[32768], expected[8192];
  /* An implicit client reconnect inside a transaction would lose locks and
   * autocommit subsequent statements. The exclusively acquired connection
   * must instead report the outage/uncertain commit to its caller. */
  my_bool reconnect = 0;
  if (db && mysql_options(db, MYSQL_OPT_RECONNECT, &reconnect))
    return CLAN_REPO_UNAVAILABLE;
  if (!db || mysql_ping(db))
    return CLAN_REPO_UNAVAILABLE;
  if (!strict_session(db))
    return schema_failure(db, __LINE__);
  if (!query(db, "SELECT version FROM clan_progression_schema WHERE singleton=1"))
    return schema_failure(db, __LINE__);
  result = mysql_store_result(db);
  if (!result)
    return schema_failure(db, __LINE__);
  row = mysql_fetch_row(result);
  if (!row || !row[0] || atoi(row[0]) != CLAN_PROGRESSION_SCHEMA_VERSION)
  {
    mysql_free_result(result);
    return schema_failure(db, __LINE__);
  }
  mysql_free_result(result);
  for (i = 0; i < sizeof(clan_schema_fragments) / sizeof(*clan_schema_fragments); i++)
  {
    if (!query(db, "SHOW CREATE TABLE %s", clan_schema_fragments[i].table))
      return schema_failure(db, __LINE__);
    result = mysql_store_result(db);
    if (!result)
      return schema_failure(db, __LINE__);
    row = mysql_fetch_row(result);
    if (!row || !row[1])
    {
      mysql_free_result(result);
      return schema_failure(db, __LINE__);
    }
    canonical(actual, sizeof(actual), row[1]);
    canonical(expected, sizeof(expected), clan_schema_fragments[i].fragment);
    if (!strstr(actual, "engine=innodb") || !strstr(actual, expected))
    {
      mysql_free_result(result);
      return schema_failure(db, __LINE__);
    }
    mysql_free_result(result);
  }
  for (i = 0; i < sizeof(clan_schema_triggers) / sizeof(*clan_schema_triggers); i++)
  {
    if (!query(db,
               "SELECT ACTION_STATEMENT,EVENT_MANIPULATION,EVENT_OBJECT_TABLE,ACTION_TIMING FROM "
               "information_schema.TRIGGERS WHERE TRIGGER_SCHEMA=DATABASE() AND TRIGGER_NAME='%s'",
               clan_schema_triggers[i].name))
      return schema_failure(db, __LINE__);
    result = mysql_store_result(db);
    if (!result)
      return schema_failure(db, __LINE__);
    row = mysql_fetch_row(result);
    if (!row || !row[0] || !row[1] || !row[2] || !row[3])
    {
      mysql_free_result(result);
      return schema_failure(db, __LINE__);
    }
    canonical(actual, sizeof(actual), row[0]);
    canonical(expected, sizeof(expected), clan_schema_triggers[i].body);
    if (strcmp(actual, expected) || strcmp(row[1], clan_schema_triggers[i].event) ||
        strcmp(row[2], clan_schema_triggers[i].table) || strcmp(row[3], "BEFORE"))
    {
      mysql_free_result(result);
      return schema_failure(db, __LINE__);
    }
    mysql_free_result(result);
  }
  for (i = 0; i < sizeof(clan_schema_columns) / sizeof(*clan_schema_columns); i++)
  {
    if (!query(db,
               "SELECT "
               "DATA_TYPE,COALESCE(CHARACTER_MAXIMUM_LENGTH,0),IS_NULLABLE,COLUMN_TYPE,COALESCE("
               "COLLATION_NAME,''),COLUMN_DEFAULT FROM information_schema.COLUMNS WHERE "
               "TABLE_SCHEMA=DATABASE() "
               "AND TABLE_NAME='%s' AND COLUMN_NAME='%s'",
               clan_schema_columns[i].table,
               clan_schema_columns[i].name))
      return schema_failure(db, __LINE__);
    result = mysql_store_result(db);
    if (!result)
      return schema_failure(db, __LINE__);
    row = mysql_fetch_row(result);
    if (!row || strcmp(row[0], clan_schema_columns[i].type) ||
        strcmp(row[2], clan_schema_columns[i].nullable) || strstr(row[3], "unsigned") ||
        (clan_schema_columns[i].length && atoi(row[1]) != clan_schema_columns[i].length) ||
        ((strstr(clan_schema_columns[i].name, "_uid") ||
          !strcmp(clan_schema_columns[i].name, "source_digest") ||
          !strcmp(clan_schema_columns[i].name, "beneficiary_key")) &&
         strcmp(row[4], "ascii_bin")))
    {
      mysql_free_result(result);
      return schema_failure(db, __LINE__);
    }
    if (clan_schema_columns[i].default_value)
    {
      size_t length;
      if (!row[5])
      {
        mysql_free_result(result);
        return schema_failure(db, __LINE__);
      }
      canonical(actual, sizeof(actual), row[5]);
      length = strlen(actual);
      if (length >= 2 && actual[0] == '\'' && actual[length - 1] == '\'')
      {
        memmove(actual, actual + 1, length - 2);
        actual[length - 2] = 0;
      }
      canonical(expected, sizeof(expected), clan_schema_columns[i].default_value);
      if (strcmp(actual, expected))
      {
        mysql_free_result(result);
        return schema_failure(db, __LINE__);
      }
    }
    else if (row[5] && strcmp(row[5], "NULL"))
    {
      mysql_free_result(result);
      return schema_failure(db, __LINE__);
    }
    mysql_free_result(result);
  }
  for (i = 0; i < sizeof(clan_schema_foreign_keys) / sizeof(*clan_schema_foreign_keys); i++)
  {
    if (!query(
          db,
          "SELECT COUNT(*) FROM information_schema.REFERENTIAL_CONSTRAINTS WHERE "
          "CONSTRAINT_SCHEMA=DATABASE() AND TABLE_NAME='%s' AND CONSTRAINT_NAME='%s' AND "
          "UPDATE_RULE IN ('RESTRICT','NO ACTION') AND DELETE_RULE IN ('RESTRICT','NO ACTION')",
          clan_schema_foreign_keys[i].table,
          clan_schema_foreign_keys[i].name))
      return schema_failure(db, __LINE__);
    result = mysql_store_result(db);
    if (!result)
      return schema_failure(db, __LINE__);
    row = mysql_fetch_row(result);
    if (!row || strcmp(row[0], "1"))
    {
      mysql_free_result(result);
      return schema_failure(db, __LINE__);
    }
    mysql_free_result(result);
  }
  if (!query(db,
             "SELECT GENERATION_EXPRESSION FROM information_schema.COLUMNS WHERE "
             "TABLE_SCHEMA=DATABASE() AND TABLE_NAME='clan_progression_clans' AND "
             "COLUMN_NAME='active_vnum'"))
    return schema_failure(db, __LINE__);
  result = mysql_store_result(db);
  if (!result)
    return schema_failure(db, __LINE__);
  row = mysql_fetch_row(result);
  if (!row || !row[0])
  {
    mysql_free_result(result);
    return schema_failure(db, __LINE__);
  }
  canonical(actual, sizeof(actual), row[0]);
  mysql_free_result(result);
  if (strcmp(actual, "casewhenrecord_status=1thenclan_vnumelsenullend"))
    return schema_failure(db, __LINE__);
  return CLAN_REPO_OK;
}
static enum clan_repository_result
load_where(MYSQL* db, const char* where, struct clan_repository_record* out)
{
  struct clan_repository_record r;
  MYSQL_RES* result;
  MYSQL_ROW row;
  unsigned long* lengths;
  int64_t v, status;
  int i;
  int64_t* numbers[] = { &v,
                         &status,
                         &r.leader_id,
                         &r.treasury_gold,
                         &r.founded_at,
                         &r.last_activity_at,
                         &r.created_at,
                         &r.updated_at,
                         &r.deleted_at,
                         &r.revision,
                         &r.xp_eighths,
                         &r.influence_eighths,
                         &r.state_revision,
                         &r.state_updated_at };
  if (!out)
    return CLAN_REPO_INVALID;
  if (!query(db,
             "SELECT "
             "c.clan_uid,c.name,c.description,c.abbreviation,c.clan_vnum,c.record_status,c.leader_"
             "player_id,c.treasury_gold,c.founded_at_utc,c.last_activity_at_utc,c.created_at_utc,c."
             "updated_at_utc,COALESCE(c.deleted_at_utc,0),c.revision,s.xp_eighths,s.influence_"
             "eighths,s.revision,s.updated_at_utc FROM clan_progression_clans c LEFT JOIN "
             "clan_progression_state s ON s.clan_uid=c.clan_uid WHERE %s",
             where))
    return failure(db);
  result = mysql_store_result(db);
  if (!result)
    return failure(db);
  row = mysql_fetch_row(result);
  if (!row)
  {
    mysql_free_result(result);
    return CLAN_REPO_NOT_FOUND;
  }
  lengths = mysql_fetch_lengths(result);
  memset(&r, 0, sizeof(r));
  if (!lengths || !row[0] || !row[1] || !row[2] || !row[3] || lengths[0] != 32 || lengths[1] > 60 ||
      lengths[2] >= sizeof(r.description) || lengths[3] > 5)
    goto invalid;
  for (i = 0; i < 4; i++)
    if (strlen(row[i]) != lengths[i])
      goto invalid;
  strcpy(r.uid, row[0]);
  strcpy(r.name, row[1]);
  strcpy(r.description, row[2]);
  strcpy(r.abbreviation, row[3]);
  for (i = 0; i < 14; i++)
    if (!integer(row[4 + i], numbers[i]))
      goto invalid;
  if (v > 65534 || status > 2)
    goto invalid;
  r.vnum = (int)v;
  r.status = (int)status;
  if (!clan_repository_record_valid(&r))
    goto invalid;
  mysql_free_result(result);
  *out = r;
  return CLAN_REPO_OK;
invalid:
  mysql_free_result(result);
  return CLAN_REPO_INVALID;
}
enum clan_repository_result
clan_repository_load(MYSQL* db, const char* uid, struct clan_repository_record* out)
{
  char where[96];
  enum clan_repository_result status;
  if (!uid || !clan_progression_uid_valid(uid))
    return CLAN_REPO_INVALID;
  status = clan_repository_schema_check(db);
  if (status != CLAN_REPO_OK)
    return status;
  snprintf(where, sizeof(where), "c.clan_uid='%s'", uid);
  return load_where(db, where, out);
}
enum clan_repository_result
clan_repository_load_vnum(MYSQL* db, int vnum, struct clan_repository_record* out)
{
  char where[96];
  enum clan_repository_result status;
  if (vnum < 1 || vnum > 65534)
    return CLAN_REPO_INVALID;
  status = clan_repository_schema_check(db);
  if (status != CLAN_REPO_OK)
    return status;
  snprintf(where, sizeof(where), "c.active_vnum=%d", vnum);
  return load_where(db, where, out);
}
static enum clan_repository_result
finish(MYSQL* db, bool success)
{
  enum clan_repository_result result;
  if (!success)
  {
    result = failure(db);
    query(db, "ROLLBACK");
    return result;
  }
  if (!query(db, "COMMIT"))
  {
    query(db, "ROLLBACK");
    return CLAN_REPO_COMMIT_UNKNOWN;
  }
  return CLAN_REPO_OK;
}
static enum clan_repository_result
finish_batch(MYSQL* db, bool batch, bool success)
{
  return batch ? (success ? CLAN_REPO_OK : failure(db)) : finish(db, success);
}
static enum clan_repository_result
create_record(MYSQL* db, const struct clan_repository_record* r, bool batch)
{
  char name[121], description[98305], abbreviation[11];
  enum clan_repository_result status;
  if (!clan_repository_record_valid(r) || r->status != CLAN_RECORD_ACTIVE || r->xp_eighths ||
      r->influence_eighths || r->revision || r->state_revision)
    return CLAN_REPO_INVALID;
  if (!batch)
  {
    status = clan_repository_schema_check(db);
    if (status != CLAN_REPO_OK)
      return status;
  }
  hex(name, r->name);
  hex(description, r->description);
  hex(abbreviation, r->abbreviation);
  if (!batch && !query(db, "START TRANSACTION"))
    return failure(db);
  if (!query(db,
             "INSERT INTO "
             "clan_progression_clans(clan_uid,clan_vnum,name,description,abbreviation,leader_"
             "player_id,treasury_gold,founded_at_utc,last_activity_at_utc,created_at_utc,updated_"
             "at_utc) VALUES('%s',%d,_utf8mb4 X'%s',_utf8mb4 X'%s',_utf8mb4 X'%s',%" PRId64
             ",%" PRId64 ",%" PRId64 ",%" PRId64 ",%" PRId64 ",%" PRId64 ")",
             r->uid,
             r->vnum,
             name,
             description,
             abbreviation,
             r->leader_id,
             r->treasury_gold,
             r->founded_at,
             r->last_activity_at,
             r->created_at,
             r->updated_at))
    return batch ? failure(db) : finish(db, false);
  return finish_batch(
    db,
    batch,
    query(db,
          "INSERT INTO clan_progression_state(clan_uid,updated_at_utc) VALUES('%s',%" PRId64 ")",
          r->uid,
          r->state_updated_at));
}
static enum clan_repository_result
save_record(MYSQL* db,
            const struct clan_repository_record* r,
            int64_t expected,
            int64_t expected_state,
            bool batch)
{
  struct clan_repository_record old;
  char name[121], description[98305], abbreviation[11], deleted[32];
  enum clan_repository_result status;
  if (!clan_repository_record_valid(r) || expected < 0 || expected_state < 0 ||
      expected == INT64_MAX || expected_state == INT64_MAX)
    return CLAN_REPO_INVALID;
  if (!batch)
  {
    status = clan_repository_schema_check(db);
    if (status != CLAN_REPO_OK)
      return status;
  }
  if (!batch && !query(db, "START TRANSACTION"))
    return failure(db);
  {
    char where[128];
    snprintf(where, sizeof(where), "c.clan_uid='%s' FOR UPDATE", r->uid);
    status = load_where(db, where, &old);
  }
  if (status != CLAN_REPO_OK)
  {
    query(db, "ROLLBACK");
    return status;
  }
  if (old.status != CLAN_RECORD_ACTIVE)
  {
    query(db, "ROLLBACK");
    return CLAN_REPO_DELETED;
  }
  if (old.revision != expected || old.state_revision != expected_state)
  {
    query(db, "ROLLBACK");
    return CLAN_REPO_CONFLICT;
  }
  if (old.vnum != r->vnum || old.created_at != r->created_at || r->updated_at < old.updated_at ||
      r->state_updated_at < old.state_updated_at)
  {
    query(db, "ROLLBACK");
    return CLAN_REPO_INVALID;
  }
  hex(name, r->name);
  hex(description, r->description);
  hex(abbreviation, r->abbreviation);
  if (r->status == CLAN_RECORD_ACTIVE)
    strcpy(deleted, "NULL");
  else
    snprintf(deleted, sizeof(deleted), "%" PRId64, r->deleted_at);
  if (!query(db,
             "UPDATE clan_progression_clans SET name=_utf8mb4 X'%s',description=_utf8mb4 "
             "X'%s',abbreviation=_utf8mb4 X'%s',leader_player_id=%" PRId64 ",treasury_gold=%" PRId64
             ",founded_at_utc=%" PRId64 ",last_activity_at_utc=%" PRId64
             ",record_status=%d,deleted_at_utc=%s,updated_at_utc=%" PRId64
             ",revision=revision+1 WHERE clan_uid='%s' AND revision=%" PRId64,
             name,
             description,
             abbreviation,
             r->leader_id,
             r->treasury_gold,
             r->founded_at,
             r->last_activity_at,
             r->status,
             deleted,
             r->updated_at,
             r->uid,
             expected) ||
      mysql_affected_rows(db) != 1)
    return batch ? failure(db) : finish(db, false);
  return finish_batch(db,
                      batch,
                      query(db,
                            "UPDATE clan_progression_state SET xp_eighths=%" PRId64
                            ",influence_eighths=%" PRId64 ",updated_at_utc=%" PRId64
                            ",revision=revision+1 WHERE clan_uid='%s' AND revision=%" PRId64,
                            r->xp_eighths,
                            r->influence_eighths,
                            r->state_updated_at,
                            r->uid,
                            expected_state) &&
                        mysql_affected_rows(db) == 1);
}

enum clan_repository_result
clan_repository_create(MYSQL* db, const struct clan_repository_record* r)
{
  return create_record(db, r, false);
}
enum clan_repository_result
clan_repository_save(MYSQL* db, const struct clan_repository_record* r, int64_t a, int64_t b)
{
  return save_record(db, r, a, b, false);
}

#include "clan_legacy_fields.h"
#include <json-c/json.h>
#include <math.h>
#define CORE_INTS(X)                                                                               \
  X(leader_id)                                                                                     \
  X(treasury_gold)                                                                                 \
  X(founded_at)                                                                                    \
  X(last_activity_at) X(created_at) X(updated_at) X(deleted_at) X(revision) X(xp_eighths)          \
    X(influence_eighths) X(state_revision) X(state_updated_at)
static struct json_object*
field(struct json_object* o, const char* name, enum json_type type)
{
  struct json_object* v = NULL;
  return o && json_object_is_type(o, json_type_object) && json_object_object_get_ex(o, name, &v) &&
             json_object_is_type(v, type)
           ? v
           : NULL;
}
static bool
object(struct json_object* o, int count)
{
  return o && json_object_is_type(o, json_type_object) && json_object_object_length(o) == count;
}
static bool
number(struct json_object* o, const char* name, int64_t* out)
{
  struct json_object* v = field(o, name, json_type_int);
  char* end;
  intmax_t n;
  if (!v)
    return false;
  errno = 0;
  n = strtoimax(json_object_get_string(v), &end, 10);
  if (errno || *end || n < INT64_MIN || n > INT64_MAX)
    return false;
  *out = (int64_t)n;
  return true;
}
static bool
text(struct json_object* o, const char* name, char* out, size_t capacity)
{
  struct json_object* v = field(o, name, json_type_string);
  const char* s;
  if (!v)
    return false;
  s = json_object_get_string(v);
  if ((size_t)json_object_get_string_len(v) >= capacity ||
      strlen(s) != (size_t)json_object_get_string_len(v))
    return false;
  strcpy(out, s);
  return true;
}
struct json_object*
clan_repository_record_json(const struct clan_repository_record* r)
{
  struct json_object* o = json_object_new_object();
#define ADD(name) json_object_object_add(o, #name, json_object_new_int64(r->name));
  CORE_INTS(ADD)
#undef ADD
  json_object_object_add(o, "uid", json_object_new_string(r->uid));
  json_object_object_add(o, "name", json_object_new_string(r->name));
  json_object_object_add(o, "description", json_object_new_string(r->description));
  json_object_object_add(o, "abbreviation", json_object_new_string(r->abbreviation));
  json_object_object_add(o, "vnum", json_object_new_int(r->vnum));
  json_object_object_add(o, "status", json_object_new_int(r->status));
  return o;
}
bool
clan_repository_record_parse(struct json_object* o, struct clan_repository_record* r)
{
  int64_t v, status;
  memset(r, 0, sizeof(*r));
  if (!object(o, 18) || !text(o, "uid", r->uid, sizeof(r->uid)) ||
      !text(o, "name", r->name, sizeof(r->name)) ||
      !text(o, "description", r->description, sizeof(r->description)) ||
      !text(o, "abbreviation", r->abbreviation, sizeof(r->abbreviation)) ||
      !number(o, "vnum", &v) || v < 1 || v > 65534 || !number(o, "status", &status) || status < 1 ||
      status > 2)
    return false;
  r->vnum = (int)v;
  r->status = (int)status;
#define READ(name)                                                                                 \
  if (!number(o, #name, &r->name))                                                                 \
    return false;
  CORE_INTS(READ)
#undef READ
  return clan_repository_record_valid(r);
}
bool
clan_repository_legacy_valid(struct json_object* o, const char* uid)
{
  struct json_object *ranks, *priv, *edges, *v;
  size_t i, j;
  int64_t n, id;
  bool seen[21] = { false };
  char peer[33];
  if (!o || !json_object_is_type(o, json_type_object) || json_object_object_length(o) != 23)
    return false;
#define CHECK(name, member, tag)                                                                   \
  if (!number(o, #name, &n) || n < 0 ||                                                            \
      n > ((!strcmp(#member, "total_deposits") || !strcmp(#member, "total_withdrawals") ||         \
            !strcmp(#member, "total_taxes_collected"))                                             \
             ? INT64_MAX                                                                           \
           : !strcmp(#member, "hall") ? 65535                                                      \
                                      : INT32_MAX))                                                \
    return false;
  CLAN_LEGACY_FIELDS(CHECK)
#undef CHECK
  ranks = field(o, "ranks", json_type_array);
  priv = field(o, "privileges", json_type_array);
  edges = field(o, "relationships", json_type_array);
  if (!ranks || json_object_array_length(ranks) < 1 || json_object_array_length(ranks) > 15 ||
      !priv || json_object_array_length(priv) != 21 || !edges ||
      json_object_array_length(edges) > 24)
    return false;
  for (i = 0; i < json_object_array_length(ranks); i++)
  {
    v = json_object_array_get_idx(ranks, i);
    if (!v || !json_object_is_type(v, json_type_string) || !json_object_get_string_len(v) ||
        json_object_get_string_len(v) > 511 ||
        strlen(json_object_get_string(v)) != (size_t)json_object_get_string_len(v))
      return false;
  }
  for (i = 0; i < 21; i++)
  {
    v = json_object_array_get_idx(priv, i);
    if (!object(v, 2) || !number(v, "id", &id) || id < 0 || id >= 21 || seen[id] ||
        !number(v, "rank", &n) || n < 0 || n > 255)
      return false;
    seen[id] = true;
  }
  for (i = 0; i < json_object_array_length(edges); i++)
  {
    v = json_object_array_get_idx(edges, i);
    if (!object(v, 3) || !text(v, "peer_uid", peer, sizeof(peer)) ||
        !clan_progression_uid_valid(peer) || !strcmp(uid, peer) ||
        !field(v, "allied", json_type_boolean) || !field(v, "at_war", json_type_boolean))
      return false;
    for (j = 0; j < i; j++)
      if (!strcmp(peer,
                  json_object_get_string(
                    field(json_object_array_get_idx(edges, j), "peer_uid", json_type_string))))
        return false;
  }
  return true;
}
static struct json_object*
snapshot_clan(struct json_object* clans, const char* uid)
{
  size_t i;
  for (i = 0; i < json_object_array_length(clans); i++)
  {
    struct json_object *e = json_object_array_get_idx(clans, i),
                       *c = field(e, "core", json_type_object),
                       *s = field(c, "uid", json_type_string);
    if (s && !strcmp(uid, json_object_get_string(s)))
      return e;
  }
  return NULL;
}
static bool
snapshot_valid(struct json_object* root)
{
  struct json_object *clans = field(root, "clans", json_type_array),
                     *members = field(root, "members", json_type_array),
                     *claims = field(root, "claims", json_type_array), *seen;
  size_t i, j;
  bool ok = false;
  char uid[33], key[64];
  int64_t id, rank, points, zone;
  struct clan_repository_record r;
  if (!clans || !members || !claims || json_object_array_length(clans) > 25)
    return false;
  for (i = 0; i < json_object_array_length(clans); i++)
  {
    struct json_object *e = json_object_array_get_idx(clans, i),
                       *legacy = field(e, "legacy", json_type_object), *edges;
    if (!object(e, 2) || !clan_repository_record_parse(field(e, "core", json_type_object), &r) ||
        r.status != CLAN_RECORD_ACTIVE || !clan_repository_legacy_valid(legacy, r.uid))
      return false;
    for (j = 0; j < i; j++)
    {
      struct clan_repository_record other;
      if (!clan_repository_record_parse(
            field(json_object_array_get_idx(clans, j), "core", json_type_object), &other) ||
          !strcmp(other.uid, r.uid) || other.vnum == r.vnum)
        return false;
    }
    edges = field(legacy, "relationships", json_type_array);
    for (j = 0; j < json_object_array_length(edges); j++)
    {
      if (!text(json_object_array_get_idx(edges, j), "peer_uid", uid, sizeof(uid)) ||
          !snapshot_clan(clans, uid))
        return false;
    }
  }
  seen = json_object_new_object();
  for (i = 0; i < json_object_array_length(members); i++)
  {
    struct json_object *e = json_object_array_get_idx(members, i), *c, *ranks;
    if (!object(e, 4) || !number(e, "player_id", &id) || id <= 0 ||
        !text(e, "clan_uid", uid, sizeof(uid)) || !(c = snapshot_clan(clans, uid)) ||
        !number(e, "rank_id", &rank) || rank < 0 || !number(e, "legacy_points", &points) ||
        points < INT32_MIN || points > INT32_MAX)
      goto done;
    ranks = field(field(c, "legacy", json_type_object), "ranks", json_type_array);
    if ((uint64_t)rank > json_object_array_length(ranks))
      goto done;
    snprintf(key, sizeof(key), "%" PRId64, id);
    if (json_object_object_get_ex(seen, key, NULL))
      goto done;
    json_object_object_add(seen, key, json_object_new_boolean(true));
  }
  json_object_put(seen);
  seen = json_object_new_object();
  for (i = 0; i < json_object_array_length(claims); i++)
  {
    struct json_object *e = json_object_array_get_idx(claims, i), *controller = NULL, *pop,
                       *pop_seen;
    if (!object(e, 4) || !number(e, "zone_vnum", &zone) || zone < 0 || zone > 65534 ||
        !number(e, "claimant_id", &id) || id < 0 ||
        !json_object_object_get_ex(e, "controller_uid", &controller) ||
        !(pop = field(e, "popularity", json_type_array)) || json_object_array_length(pop) > 25)
      goto done;
    if (controller && (!json_object_is_type(controller, json_type_string) ||
                       !snapshot_clan(clans, json_object_get_string(controller))))
      goto done;
    snprintf(key, sizeof(key), "%" PRId64, zone);
    if (json_object_object_get_ex(seen, key, NULL))
      goto done;
    json_object_object_add(seen, key, json_object_new_boolean(true));
    pop_seen = json_object_new_object();
    for (j = 0; j < json_object_array_length(pop); j++)
    {
      struct json_object *p = json_object_array_get_idx(pop, j), *pct = NULL;
      double amount;
      if (!object(p, 2) || !text(p, "clan_uid", uid, sizeof(uid)) || !snapshot_clan(clans, uid) ||
          !json_object_object_get_ex(p, "percentage", &pct) || !pct ||
          (!json_object_is_type(pct, json_type_double) && !json_object_is_type(pct, json_type_int)))
      {
        json_object_put(pop_seen);
        goto done;
      }
      amount = json_object_get_double(pct);
      if (!isfinite(amount) || amount < 0 || amount > 100 ||
          json_object_object_get_ex(pop_seen, uid, NULL))
      {
        json_object_put(pop_seen);
        goto done;
      }
      json_object_object_add(pop_seen, uid, json_object_new_boolean(true));
    }
    json_object_put(pop_seen);
  }
  ok = true;
done:
  json_object_put(seen);
  return ok;
}
static char*
json_hex(struct json_object* o)
{
  const char* s = json_object_to_json_string_ext(o, JSON_C_TO_STRING_PLAIN);
  size_t n = strlen(s);
  char* out;
  if (n > 262144)
    return NULL;
  out = malloc(n * 2 + 1);
  if (out)
    hex(out, s);
  return out;
}
static enum clan_repository_result
snapshot_read_tx(MYSQL* db, struct json_object** out)
{
  struct json_object *root = json_object_new_object(), *clans = json_object_new_array(),
                     *members = json_object_new_array(), *claims = json_object_new_array();
  MYSQL_RES* result;
  MYSQL_ROW row;
  size_t i;
  enum clan_repository_result status = CLAN_REPO_INVALID;
  json_object_object_add(root, "clans", clans);
  json_object_object_add(root, "members", members);
  json_object_object_add(root, "claims", claims);
  if (!query(db, "SELECT revision,store_mode FROM clan_store_control WHERE singleton=1"))
    goto fail;
  result = mysql_store_result(db);
  if (!result)
    goto fail;
  row = mysql_fetch_row(result);
  if (!row || !row[0] || !row[1])
  {
    mysql_free_result(result);
    goto fail;
  }
  {
    int64_t n;
    if (!integer(row[0], &n))
    {
      mysql_free_result(result);
      goto fail;
    }
    json_object_object_add(root, "store_revision", json_object_new_int64(n));
    if (!integer(row[1], &n) || n > 2)
    {
      mysql_free_result(result);
      goto fail;
    }
    json_object_object_add(root, "store_mode", json_object_new_int64(n));
  }
  mysql_free_result(result);
  if (!query(
        db, "SELECT clan_uid FROM clan_progression_clans WHERE record_status=1 ORDER BY clan_vnum"))
    goto fail;
  result = mysql_store_result(db);
  if (!result)
    goto fail;
  while ((row = mysql_fetch_row(result)))
  {
    struct json_object* e = json_object_new_object();
    json_object_object_add(e, "uid", json_object_new_string(row[0]));
    json_object_array_add(clans, e);
  }
  mysql_free_result(result);
  for (i = 0; i < json_object_array_length(clans); i++)
  {
    struct json_object *e = json_object_array_get_idx(clans, i), *legacy;
    struct clan_repository_record r;
    char uid[33], where[128];
    if (!text(e, "uid", uid, sizeof(uid)))
      goto fail;
    snprintf(where, sizeof(where), "c.clan_uid='%s'", uid);
    status = load_where(db, where, &r);
    if (status != CLAN_REPO_OK)
      goto fail;
    json_object_object_del(e, "uid");
    json_object_object_add(e, "core", clan_repository_record_json(&r));
    if (!query(db, "SELECT details FROM clan_progression_legacy WHERE clan_uid='%s'", uid))
      goto fail;
    result = mysql_store_result(db);
    if (!result)
      goto fail;
    row = mysql_fetch_row(result);
    legacy = row ? json_tokener_parse(row[0]) : NULL;
    mysql_free_result(result);
    if (!legacy)
      goto fail;
    json_object_object_add(e, "legacy", legacy);
  }
  if (!query(
        db,
        "SELECT player_id,clan_uid,rank_id,legacy_points FROM clan_memberships ORDER BY player_id"))
    goto fail;
  result = mysql_store_result(db);
  if (!result)
    goto fail;
  while ((row = mysql_fetch_row(result)))
  {
    struct json_object* e = json_object_new_object();
    int64_t id, rank;
    char* end;
    long points;
    if (!integer(row[0], &id) || !integer(row[2], &rank))
    {
      json_object_put(e);
      mysql_free_result(result);
      goto fail;
    }
    errno = 0;
    points = strtol(row[3], &end, 10);
    if (errno || *end || points < INT32_MIN || points > INT32_MAX)
    {
      json_object_put(e);
      mysql_free_result(result);
      goto fail;
    }
    json_object_object_add(e, "player_id", json_object_new_int64(id));
    json_object_object_add(e, "clan_uid", json_object_new_string(row[1]));
    json_object_object_add(e, "rank_id", json_object_new_int64(rank));
    json_object_object_add(e, "legacy_points", json_object_new_int64(points));
    json_object_array_add(members, e);
  }
  mysql_free_result(result);
  if (!query(db,
             "SELECT zone_vnum,controller_uid,claimant_id,popularity FROM clan_zone_claims ORDER "
             "BY zone_vnum"))
    goto fail;
  result = mysql_store_result(db);
  if (!result)
    goto fail;
  while ((row = mysql_fetch_row(result)))
  {
    struct json_object *e = json_object_new_object(), *pop = json_tokener_parse(row[3]);
    int64_t z, id;
    if (!integer(row[0], &z) || !integer(row[2], &id) || !pop)
    {
      json_object_put(e);
      if (pop)
        json_object_put(pop);
      mysql_free_result(result);
      goto fail;
    }
    json_object_object_add(e, "zone_vnum", json_object_new_int64(z));
    json_object_object_add(e, "controller_uid", row[1] ? json_object_new_string(row[1]) : NULL);
    json_object_object_add(e, "claimant_id", json_object_new_int64(id));
    json_object_object_add(e, "popularity", pop);
    json_object_array_add(claims, e);
  }
  mysql_free_result(result);
  if (!clan_repository_snapshot_canonicalize(root))
    goto fail;
  *out = root;
  return CLAN_REPO_OK;
fail:
  if (mysql_errno(db))
    status = failure(db);
  json_object_put(root);
  return status == CLAN_REPO_OK ? CLAN_REPO_INVALID : status;
}
enum clan_repository_result
clan_repository_snapshot_read(MYSQL* db, struct json_object** out)
{
  struct json_object* root = NULL;
  enum clan_repository_result status;
  if (!out)
    return CLAN_REPO_INVALID;
  status = clan_repository_schema_check(db);
  if (status != CLAN_REPO_OK)
    return status;
  if (!query(db, "SET TRANSACTION ISOLATION LEVEL REPEATABLE READ") ||
      !query(db, "START TRANSACTION WITH CONSISTENT SNAPSHOT"))
    return failure(db);
  status = snapshot_read_tx(db, &root);
  if (status != CLAN_REPO_OK)
  {
    query(db, "ROLLBACK");
    return status;
  }
  status = finish(db, true);
  if (status != CLAN_REPO_OK)
  {
    json_object_put(root);
    return status;
  }
  *out = root;
  return CLAN_REPO_OK;
}

bool
clan_repository_snapshot_valid(struct json_object* root)
{
  return snapshot_valid(root);
}
enum clan_repository_result
clan_repository_snapshot_write(MYSQL* db,
                               struct json_object* root,
                               int64_t expected,
                               struct json_object* manifest)
{
  struct json_object *clans, *members, *claims;
  MYSQL_RES* result;
  MYSQL_ROW row;
  enum clan_repository_result status;
  int64_t revision, mode, now;
  size_t i;
  if (expected < 0 || expected == INT64_MAX || !snapshot_valid(root) ||
      !number(root, "written_at", &now) || now < 0)
    return CLAN_REPO_INVALID;
  clans = field(root, "clans", json_type_array);
  members = field(root, "members", json_type_array);
  claims = field(root, "claims", json_type_array);
  status = clan_repository_schema_check(db);
  if (status != CLAN_REPO_OK)
    return status;
  if (!query(db, "START TRANSACTION"))
    return failure(db);
  if (!query(db, "SELECT revision,store_mode FROM clan_store_control WHERE singleton=1 FOR UPDATE"))
    goto fail;
  result = mysql_store_result(db);
  if (!result)
    goto fail;
  row = mysql_fetch_row(result);
  if (!row || !integer(row[0], &revision) || !integer(row[1], &mode))
  {
    mysql_free_result(result);
    goto invalid;
  }
  mysql_free_result(result);
  if (revision != expected)
  {
    status = CLAN_REPO_CONFLICT;
    goto abort;
  }
  if ((manifest && mode != 0) || (!manifest && mode != 2))
    goto invalid;
  if (manifest)
  {
    if (!query(db,
               "SELECT (SELECT COUNT(*) FROM clan_progression_clans)+(SELECT COUNT(*) FROM "
               "clan_memberships)+(SELECT COUNT(*) FROM clan_zone_claims)+(SELECT COUNT(*) FROM "
               "clan_import_manifests)"))
      goto fail;
    result = mysql_store_result(db);
    if (!result)
      goto fail;
    row = mysql_fetch_row(result);
    if (!row || strcmp(row[0], "0"))
    {
      mysql_free_result(result);
      status = CLAN_REPO_CONFLICT;
      goto abort;
    }
    mysql_free_result(result);
  }
  if (!query(db, "SELECT clan_uid FROM clan_progression_clans WHERE record_status=1 FOR UPDATE"))
    goto fail;
  result = mysql_store_result(db);
  if (!result)
    goto fail;
  while ((row = mysql_fetch_row(result)))
  {
    if (!snapshot_clan(clans, row[0]))
    {
      struct clan_repository_record old;
      char where[128];
      snprintf(where, sizeof(where), "c.clan_uid='%s'", row[0]);
      status = load_where(db, where, &old);
      if (status != CLAN_REPO_OK)
      {
        mysql_free_result(result);
        goto abort;
      }
      old.status = CLAN_RECORD_DELETED;
      old.deleted_at = old.updated_at = now;
      status = save_record(db, &old, old.revision, old.state_revision, true);
      if (status != CLAN_REPO_OK)
      {
        mysql_free_result(result);
        goto abort;
      }
    }
  }
  mysql_free_result(result);
  for (i = 0; i < json_object_array_length(clans); i++)
  {
    struct json_object* e = json_object_array_get_idx(clans, i);
    struct clan_repository_record r, old;
    char where[128], *encoded;
    if (!clan_repository_record_parse(field(e, "core", json_type_object), &r))
      goto invalid;
    snprintf(where, sizeof(where), "c.clan_uid='%s' FOR UPDATE", r.uid);
    status = load_where(db, where, &old);
    if (status == CLAN_REPO_NOT_FOUND)
      status = create_record(db, &r, true);
    else if (status == CLAN_REPO_OK)
      status = save_record(db, &r, r.revision, r.state_revision, true);
    if (status != CLAN_REPO_OK)
      goto abort;
    encoded = json_hex(field(e, "legacy", json_type_object));
    if (!encoded)
      goto invalid;
    if (!query(db,
               "INSERT INTO clan_progression_legacy(clan_uid,details) VALUES('%s',_utf8mb4 X'%s') "
               "ON DUPLICATE KEY UPDATE details=VALUES(details)",
               r.uid,
               encoded))
    {
      free(encoded);
      goto fail;
    }
    free(encoded);
  }
  if (!query(db, "DELETE FROM clan_memberships") || !query(db, "DELETE FROM clan_zone_claims"))
    goto fail;
  for (i = 0; i < json_object_array_length(members); i++)
  {
    struct json_object* e = json_object_array_get_idx(members, i);
    char uid[33];
    int64_t id, rank, points;
    if (!text(e, "clan_uid", uid, sizeof(uid)) || !number(e, "player_id", &id) ||
        !number(e, "rank_id", &rank) || !number(e, "legacy_points", &points))
      goto invalid;
    if (!query(
          db,
          "INSERT INTO clan_memberships(player_id,clan_uid,rank_id,legacy_points) VALUES(%" PRId64
          ",'%s',%" PRId64 ",%" PRId64 ")",
          id,
          uid,
          rank,
          points))
      goto fail;
  }
  for (i = 0; i < json_object_array_length(claims); i++)
  {
    struct json_object *e = json_object_array_get_idx(claims, i),
                       *controller = field(e, "controller_uid", json_type_string);
    char uid_sql[40], *encoded;
    int64_t zone, id;
    if (!number(e, "zone_vnum", &zone) || !number(e, "claimant_id", &id))
      goto invalid;
    if (controller)
      snprintf(uid_sql, sizeof(uid_sql), "'%s'", json_object_get_string(controller));
    else
      strcpy(uid_sql, "NULL");
    encoded = json_hex(field(e, "popularity", json_type_array));
    if (!encoded)
      goto invalid;
    if (!query(db,
               "INSERT INTO clan_zone_claims(zone_vnum,controller_uid,claimant_id,popularity) "
               "VALUES(%" PRId64 ",%s,%" PRId64 ",_utf8mb4 X'%s')",
               zone,
               uid_sql,
               id,
               encoded))
    {
      free(encoded);
      goto fail;
    }
    free(encoded);
  }
  if (manifest)
  {
    char digest[65], uid[33], *encoded;
    int64_t at;
    struct json_object* body = field(manifest, "source_manifest", json_type_object);
    size_t n;
    if (!text(manifest, "source_digest", digest, sizeof(digest)) || strlen(digest) != 64 ||
        !text(manifest, "import_uid", uid, sizeof(uid)) || !clan_progression_uid_valid(uid) ||
        !body || !number(manifest, "imported_at", &at) || at < 0)
      goto invalid;
    for (n = 0; n < 64; n++)
      if (!((digest[n] >= '0' && digest[n] <= '9') || (digest[n] >= 'a' && digest[n] <= 'f')))
        goto invalid;
    encoded = json_hex(body);
    if (!encoded)
      goto invalid;
    if (!query(db,
               "INSERT INTO "
               "clan_import_manifests(source_digest,import_uid,source_manifest,imported_at_utc) "
               "VALUES('%s','%s',_utf8mb4 X'%s',%" PRId64 ")",
               digest,
               uid,
               encoded,
               at))
    {
      free(encoded);
      goto fail;
    }
    free(encoded);
  }
  if (!query(
        db,
        "UPDATE clan_store_control SET revision=revision+1 WHERE singleton=1 AND revision=%" PRId64,
        expected) ||
      mysql_affected_rows(db) != 1)
    goto fail;
  return finish(db, true);
invalid:
  status = CLAN_REPO_INVALID;
  goto abort;
fail:
  status = failure(db);
abort:
  query(db, "ROLLBACK");
  return status;
}

static struct json_object*
sort_object(const void* p)
{
  return *(struct json_object* const*)p;
}
static int
sort_uid(const void* a, const void* b)
{
  return strcmp(json_object_get_string(field(sort_object(a), "clan_uid", json_type_string)),
                json_object_get_string(field(sort_object(b), "clan_uid", json_type_string)));
}
static int
sort_peer(const void* a, const void* b)
{
  return strcmp(json_object_get_string(field(sort_object(a), "peer_uid", json_type_string)),
                json_object_get_string(field(sort_object(b), "peer_uid", json_type_string)));
}
static int
sort_integer_key(const void* a, const void* b, const char* key)
{
  int64_t x = json_object_get_int64(field(sort_object(a), key, json_type_int)),
          y = json_object_get_int64(field(sort_object(b), key, json_type_int));
  return (x > y) - (x < y);
}
static int
sort_privilege(const void* a, const void* b)
{
  return sort_integer_key(a, b, "id");
}
static int
sort_player(const void* a, const void* b)
{
  return sort_integer_key(a, b, "player_id");
}
static int
sort_zone(const void* a, const void* b)
{
  return sort_integer_key(a, b, "zone_vnum");
}
static int
sort_clan(const void* a, const void* b)
{
  int x = json_object_get_int(
        field(field(sort_object(a), "core", json_type_object), "vnum", json_type_int)),
      y = json_object_get_int(
        field(field(sort_object(b), "core", json_type_object), "vnum", json_type_int));
  return (x > y) - (x < y);
}
bool
clan_repository_snapshot_canonicalize(struct json_object* root)
{
  struct json_object *clans, *claims;
  size_t i;
  if (!snapshot_valid(root))
    return false;
  clans = field(root, "clans", json_type_array);
  claims = field(root, "claims", json_type_array);
  json_object_array_sort(clans, sort_clan);
  json_object_array_sort(claims, sort_zone);
  json_object_array_sort(field(root, "members", json_type_array), sort_player);
  for (i = 0; i < json_object_array_length(clans); i++)
  {
    struct json_object* legacy =
      field(json_object_array_get_idx(clans, i), "legacy", json_type_object);
    json_object_array_sort(field(legacy, "privileges", json_type_array), sort_privilege);
    json_object_array_sort(field(legacy, "relationships", json_type_array), sort_peer);
  }
  for (i = 0; i < json_object_array_length(claims); i++)
    json_object_array_sort(
      field(json_object_array_get_idx(claims, i), "popularity", json_type_array), sort_uid);
  return true;
}

/* A control transition never imports implicitly or reopens legacy authority. */
enum clan_repository_result
clan_repository_cutover(MYSQL* db, int64_t expected, const char* digest)
{
  enum clan_repository_result status;
  MYSQL_RES* res;
  MYSQL_ROW row;
  size_t i;
  if (expected < 0 || expected == INT64_MAX)
    return CLAN_REPO_INVALID;
  if (digest) {
    if (strlen(digest) != 64)
      return CLAN_REPO_INVALID;
    for (i = 0; i < 64; i++)
      if (!((digest[i] >= '0' && digest[i] <= '9') ||
            (digest[i] >= 'a' && digest[i] <= 'f')))
        return CLAN_REPO_INVALID;
  }
  status = clan_repository_schema_check(db);
  if (status != CLAN_REPO_OK)
    return status;
  if (db->server_status & SERVER_STATUS_IN_TRANS)
    return CLAN_REPO_INVALID;
  if (!query(db, "START TRANSACTION"))
    return failure(db);
  if (!query(db,
             "SELECT revision,store_mode FROM clan_store_control WHERE "
             "singleton=1 FOR UPDATE"))
    goto failed;
  res = mysql_store_result(db);
  if (!res)
    goto failed;
  row = mysql_fetch_row(res);
  if (!row || strtoll(row[0], NULL, 10) != expected || strcmp(row[1], "0")) {
    mysql_free_result(res);
    status = CLAN_REPO_CONFLICT;
    goto rollback;
  }
  mysql_free_result(res);
  if (digest) {
    if (!query(db,
               "UPDATE clan_import_manifests SET import_status=2 WHERE "
               "source_digest='%s' AND import_status=1",
               digest))
      goto failed;
    if (mysql_affected_rows(db) != 1) {
      status = CLAN_REPO_CONFLICT;
      goto rollback;
    }
  } else {
    if (!query(db,
               "SELECT (SELECT COUNT(*) FROM clan_progression_clans)+(SELECT "
               "COUNT(*) FROM clan_import_manifests)+(SELECT COUNT(*) FROM "
               "clan_memberships)+(SELECT COUNT(*) FROM clan_zone_claims)"))
      goto failed;
    res = mysql_store_result(db);
    if (!res)
      goto failed;
    row = mysql_fetch_row(res);
    if (!row || strcmp(row[0], "0")) {
      mysql_free_result(res);
      status = CLAN_REPO_CONFLICT;
      goto rollback;
    }
    mysql_free_result(res);
  }
  if (!query(db,
             "UPDATE clan_store_control SET store_mode=2,revision=revision+1 "
             "WHERE singleton=1"))
    goto failed;
  return finish(db, true);
failed:
  status = failure(db);
rollback:
  query(db, "ROLLBACK");
  return status;
}

static bool
buff_valid(const struct clan_buff_state* b)
{
  return clan_progression_uid_valid(b->clan_uid) &&
         clan_progression_uid_valid(b->activation_uid) && b->family >= 1 &&
         b->family <= CLAN_PROGRESSION_BUFF_COUNT && b->tier >= 1 &&
         b->tier <= 3 && b->slot >= 1 && b->slot <= 3 &&
         b->rules_version == CLAN_PROGRESSION_RULES_VERSION &&
         b->started_at >= 0 && b->expires_at > b->started_at &&
         b->next_available_at >= b->expires_at &&
         b->paid_influence_eighths >= 0;
}
bool
clan_repository_buff_active(const struct clan_buff_state* b, int64_t now)
{
  return b && buff_valid(b) && !b->canceled && now >= b->started_at &&
         now < b->expires_at;
}
enum clan_repository_result
clan_repository_buffs_read(MYSQL* db,
                           struct clan_buff_state* out,
                           size_t capacity,
                           size_t* count)
{
  struct clan_buff_state
    pending[CLAN_PROGRESSION_MAX_PILOT_CLANS * CLAN_PROGRESSION_BUFF_COUNT];
  size_t n = 0, j;
  MYSQL_RES* res;
  MYSQL_ROW row;
  enum clan_repository_result status;
  if (!count || (!out && capacity))
    return CLAN_REPO_INVALID;
  status = clan_repository_schema_check(db);
  if (status != CLAN_REPO_OK)
    return status;
  if (!query(db,
             "SELECT "
             "b.clan_uid,activation_uid,family_id,tier,slot_id,rules_version,"
             "started_at,expires_at,next_available_at,paid_influence_eighths,"
             "canceled FROM clan_buff_families b JOIN clan_progression_clans c "
             "ON c.clan_uid=b.clan_uid WHERE c.record_status=1 ORDER BY "
             "b.clan_uid,family_id"))
    return failure(db);
  res = mysql_store_result(db);
  if (!res)
    return failure(db);
  while ((row = mysql_fetch_row(res))) {
    struct clan_buff_state b;
    int64_t v[9];
    memset(&b, 0, sizeof(b));
    if (n >= sizeof(pending) / sizeof(*pending) || !row[0] ||
        strlen(row[0]) != 32 || !row[1] || strlen(row[1]) != 32)
      goto invalid;
    for (j = 0; j < 9; j++)
      if (!integer(row[j + 2], &v[j]))
        goto invalid;
    if (v[0] > 5 || v[1] > 3 || v[2] > 3 || v[3] > INT32_MAX || v[8] > 1)
      goto invalid;
    strcpy(b.clan_uid, row[0]);
    strcpy(b.activation_uid, row[1]);
    b.family = (enum clan_buff_family)v[0];
    b.tier = (int)v[1];
    b.slot = (int)v[2];
    b.rules_version = (int)v[3];
    b.started_at = v[4];
    b.expires_at = v[5];
    b.next_available_at = v[6];
    b.paid_influence_eighths = v[7];
    b.canceled = v[8] != 0;
    if (!buff_valid(&b))
      goto invalid;
    pending[n++] = b;
  }
  mysql_free_result(res);
  if (n > capacity)
    return CLAN_REPO_INVALID;
  if (n)
    memcpy(out, pending, n * sizeof(*out));
  *count = n;
  return CLAN_REPO_OK;
invalid:
  mysql_free_result(res);
  return CLAN_REPO_SCHEMA;
}

enum clan_repository_result
clan_repository_favor_load(MYSQL* db,
                           int64_t player,
                           const char* uid,
                           struct clan_favor_state* out)
{
  enum clan_repository_result status;
  MYSQL_RES* res;
  MYSQL_ROW row;
  struct clan_favor_state state;
  if (player <= 0 || !clan_progression_uid_valid(uid) || !out)
    return CLAN_REPO_INVALID;
  status = clan_repository_schema_check(db);
  if (status != CLAN_REPO_OK)
    return status;
  if (!query(db,
             "SELECT favor_eighths,revision FROM clan_personal_favor WHERE "
             "player_id=%" PRId64 " AND clan_uid='%s'",
             player,
             uid))
    return failure(db);
  res = mysql_store_result(db);
  if (!res)
    return failure(db);
  row = mysql_fetch_row(res);
  if (!row) {
    mysql_free_result(res);
    return CLAN_REPO_NOT_FOUND;
  }
  memset(&state, 0, sizeof(state));
  state.character_id = player;
  strcpy(state.clan_uid, uid);
  if (!integer(row[0], &state.favor_eighths) ||
      !integer(row[1], &state.revision)) {
    mysql_free_result(res);
    return CLAN_REPO_SCHEMA;
  }
  mysql_free_result(res);
  *out = state;
  return CLAN_REPO_OK;
}
static enum clan_repository_result
favor_write(MYSQL* db,
                           const struct clan_favor_state* s,
                           int64_t expected,
                           int64_t now, bool batch)
{
  enum clan_repository_result status;
  MYSQL_RES* res;
  MYSQL_ROW row;
  if (!s || s->character_id <= 0 || !clan_progression_uid_valid(s->clan_uid) ||
      s->favor_eighths < 0 || expected < -1 || expected == INT64_MAX ||
      now < 0 || s->revision != (expected < 0 ? 0 : expected))
    return CLAN_REPO_INVALID;
  if (!batch) {
    status = clan_repository_schema_check(db);
    if (status != CLAN_REPO_OK) return status;
  }
  if (!db || ((db->server_status & SERVER_STATUS_IN_TRANS) != 0) != batch)
    return CLAN_REPO_INVALID;
  if (!batch && !query(db, "START TRANSACTION")) return failure(db);
  if (!query(db,
             "SELECT store_mode FROM clan_store_control WHERE singleton=1 FOR "
             "UPDATE"))
    goto failed;
  res = mysql_store_result(db);
  if (!res)
    goto failed;
  row = mysql_fetch_row(res);
  if (!row || strcmp(row[0], "2")) {
    mysql_free_result(res);
    status = CLAN_REPO_CONFLICT;
    goto rollback;
  }
  mysql_free_result(res);
  if (!query(db,
             "SELECT record_status FROM clan_progression_clans WHERE "
             "clan_uid='%s' FOR UPDATE",
             s->clan_uid))
    goto failed;
  res = mysql_store_result(db);
  if (!res)
    goto failed;
  row = mysql_fetch_row(res);
  if (!row) {
    mysql_free_result(res);
    status = CLAN_REPO_NOT_FOUND;
    goto rollback;
  }
  if (strcmp(row[0], "1")) {
    mysql_free_result(res);
    status = CLAN_REPO_DELETED;
    goto rollback;
  }
  mysql_free_result(res);
  if (expected < 0) {
    if (!query(db,
               "INSERT INTO "
               "clan_personal_favor(player_id,clan_uid,favor_eighths,revision,"
               "updated_at_utc) VALUES(%" PRId64 ",'%s',%" PRId64 ",0,%" PRId64
               ")",
               s->character_id,
               s->clan_uid,
               s->favor_eighths,
               now))
      goto failed;
  } else {
    if (!query(db,
               "UPDATE clan_personal_favor SET favor_eighths=%" PRId64
               ",revision=revision+1,updated_at_utc=%" PRId64
               " WHERE player_id=%" PRId64
               " AND clan_uid='%s' AND revision=%" PRId64
               " AND favor_eighths<=%" PRId64 " AND updated_at_utc<=%" PRId64,
               s->favor_eighths,
               now,
               s->character_id,
               s->clan_uid,
               expected,
               s->favor_eighths,
               now))
      goto failed;
    if (mysql_affected_rows(db) != 1) {
      status = CLAN_REPO_CONFLICT;
      goto rollback;
    }
  }
  return batch ? CLAN_REPO_OK : finish(db, true);
failed:
  status = failure(db);
rollback:
  if (!batch) query(db, "ROLLBACK");
  return status;
}
enum clan_repository_result clan_repository_favor_save(MYSQL *db,
    const struct clan_favor_state *record, int64_t expected, int64_t now)
{ return favor_write(db, record, expected, now, false); }


/* Retained account-global usage. Missing periods are proposed as zero by the
 * caller, never written by a read, login, or timer rollover. */
enum clan_repository_result
clan_repository_account_window_load(MYSQL* db,
                                    int64_t account,
                                    int64_t occurred_at,
                                    struct clan_account_window* out)
{
  struct clan_account_window w;
  enum clan_repository_result status;
  MYSQL_RES* res;
  MYSQL_ROW row;
  size_t i;
  if (!out || !clan_progression_account_window_init(account, occurred_at, &w))
    return CLAN_REPO_INVALID;
  status = clan_repository_schema_check(db);
  if (status != CLAN_REPO_OK)
    return status;
  if (!query(db,
             "SELECT base_xp_eighths,favor_eighths,revision,kill_claims,"
             "supply_order_claims,delivery_claims,gathering_claims,bounty_claims,expedition_claims,"
             "updated_at_utc FROM clan_account_windows WHERE account_id=%" PRId64
             " AND starts_at_utc=%" PRId64,
             account,
             w.starts_at))
    return failure(db);
  res = mysql_store_result(db);
  if (!res)
    return failure(db);
  row = mysql_fetch_row(res);
  if (!row)
  {
    mysql_free_result(res);
    return CLAN_REPO_NOT_FOUND;
  }
  {
    int64_t written_at;
    bool ok = integer(row[0], &w.base_xp_eighths) && integer(row[1], &w.favor_eighths) &&
              integer(row[2], &w.revision) && integer(row[9], &written_at);
    for (i = 1; i <= CLAN_SOURCE_EXPEDITION_PERSONAL; i++)
      if (!integer(row[i + 2], &w.claims[i]))
        ok = false;
    mysql_free_result(res);
    if (!ok || written_at < w.starts_at || !clan_progression_account_window_valid(&w))
      return CLAN_REPO_SCHEMA;
  }
  *out = w;
  return CLAN_REPO_OK;
}
static enum clan_repository_result
account_window_write(MYSQL* db,
                                    const struct clan_account_window* w,
                                    int64_t expected,
                                    int64_t now, bool batch)
{
  enum clan_repository_result status;
  MYSQL_RES* res;
  MYSQL_ROW row;
  if (!clan_progression_account_window_valid(w) || expected < -1 || expected == INT64_MAX ||
      w->revision != (expected < 0 ? 0 : expected) || now < 0 || now < w->starts_at)
    return CLAN_REPO_INVALID;
  if (!batch) {
    status = clan_repository_schema_check(db);
    if (status != CLAN_REPO_OK) return status;
  }
  if (!db || ((db->server_status & SERVER_STATUS_IN_TRANS) != 0) != batch)
    return CLAN_REPO_INVALID;
  if (!batch && !query(db, "START TRANSACTION")) return failure(db);
  if (!query(db, "SELECT store_mode FROM clan_store_control WHERE singleton=1 FOR UPDATE"))
    goto failed;
  res = mysql_store_result(db);
  if (!res)
    goto failed;
  row = mysql_fetch_row(res);
  if (!row || strcmp(row[0], "2"))
  {
    mysql_free_result(res);
    status = CLAN_REPO_CONFLICT;
    goto rollback;
  }
  mysql_free_result(res);
  if (expected < 0)
  {
    if (!query(
          db,
          "INSERT INTO clan_account_windows(account_id,starts_at_utc,base_xp_eighths,"
          "favor_eighths,revision,kill_claims,supply_order_claims,delivery_claims,gathering_claims,"
          "bounty_claims,expedition_claims,updated_at_utc) VALUES(%" PRId64 ",%" PRId64 ",%" PRId64
          ",%" PRId64 ",0,%" PRId64 ",%" PRId64 ",%" PRId64 ",%" PRId64 ",%" PRId64 ",%" PRId64
          ",%" PRId64 ")",
          w->account_id,
          w->starts_at,
          w->base_xp_eighths,
          w->favor_eighths,
          w->claims[CLAN_SOURCE_KILL],
          w->claims[CLAN_SOURCE_SUPPLY_ORDER],
          w->claims[CLAN_SOURCE_DELIVERY],
          w->claims[CLAN_SOURCE_GATHERING],
          w->claims[CLAN_SOURCE_BOUNTY],
          w->claims[CLAN_SOURCE_EXPEDITION_PERSONAL],
          now))
      goto failed;
  }
  else
  {
    /* SQL predicate enforces monotonic values as well as CAS, returning conflict
     * for stale callers rather than overwriting another character's usage. */
    if (!query(db,
               "UPDATE clan_account_windows SET base_xp_eighths=%" PRId64 ",favor_eighths=%" PRId64
               ",revision=revision+1,kill_claims=%" PRId64 ",supply_order_claims=%" PRId64
               ",delivery_claims=%" PRId64 ",gathering_claims=%" PRId64 ",bounty_claims=%" PRId64
               ",expedition_claims=%" PRId64 ",updated_at_utc=%" PRId64 " WHERE account_id=%" PRId64
               " AND starts_at_utc=%" PRId64 " AND revision=%" PRId64
               " AND base_xp_eighths<=%" PRId64 " AND favor_eighths<=%" PRId64
               " AND kill_claims<=%" PRId64 " AND supply_order_claims<=%" PRId64
               " AND delivery_claims<=%" PRId64 " AND gathering_claims<=%" PRId64
               " AND bounty_claims<=%" PRId64 " AND expedition_claims<=%" PRId64
               " AND updated_at_utc<=%" PRId64,
               w->base_xp_eighths,
               w->favor_eighths,
               w->claims[CLAN_SOURCE_KILL],
               w->claims[CLAN_SOURCE_SUPPLY_ORDER],
               w->claims[CLAN_SOURCE_DELIVERY],
               w->claims[CLAN_SOURCE_GATHERING],
               w->claims[CLAN_SOURCE_BOUNTY],
               w->claims[CLAN_SOURCE_EXPEDITION_PERSONAL],
               now,
               w->account_id,
               w->starts_at,
               expected,
               w->base_xp_eighths,
               w->favor_eighths,
               w->claims[CLAN_SOURCE_KILL],
               w->claims[CLAN_SOURCE_SUPPLY_ORDER],
               w->claims[CLAN_SOURCE_DELIVERY],
               w->claims[CLAN_SOURCE_GATHERING],
               w->claims[CLAN_SOURCE_BOUNTY],
               w->claims[CLAN_SOURCE_EXPEDITION_PERSONAL],
               now))
      goto failed;
    if (mysql_affected_rows(db) != 1)
    {
      status = CLAN_REPO_CONFLICT;
      goto rollback;
    }
  }
  return batch ? CLAN_REPO_OK : finish(db, true);
failed:
  status = failure(db);
rollback:
  if (!batch) query(db, "ROLLBACK");
  return status;
}
enum clan_repository_result clan_repository_account_window_save(MYSQL *db,
    const struct clan_account_window *record, int64_t expected, int64_t now)
{ return account_window_write(db, record, expected, now, false); }


/* Phase 8: SQL-only receipt coordinator. Legacy economic/file gates stay closed. */
#ifdef CLAN_REWARD_TEST
#include <unistd.h>
int clan_reward_fault, clan_reward_crash;
static bool
reward_stop(int point)
{
  if (clan_reward_crash == point)
    _exit(90 + point);
  return clan_reward_fault == point;
}
#else
static bool
reward_stop(int point)
{
  (void)point;
  return false;
}
#endif
static struct json_object*
reward_get(struct json_object* o, const char* key)
{
  struct json_object* v = NULL;
  json_object_object_get_ex(o, key, &v);
  return v;
}
static bool
reward_uid(const char uid[33])
{
  return uid && memchr(uid, 0, 33) && clan_progression_uid_valid(uid);
}
bool
clan_reward_event_valid(const struct clan_reward_event* e)
{
  struct clan_reset_period period;
  const struct clan_reward_definition* definition;
  if (!e || !reward_uid(e->operation_uid) || !reward_uid(e->event_uid) ||
      !reward_uid(e->clan_uid) || e->rules_version != CLAN_PROGRESSION_RULES_VERSION ||
      !clan_progression_week_at(e->occurred_at, &period) || e->requested_xp_eighths < 0 ||
      e->requested_influence_eighths < 0 || e->requested_favor_eighths < 0 ||
      !(definition = clan_progression_reward(&clan_progression_defaults, e->source)))
    return false;
  return definition->scope == CLAN_REWARD_ACCOUNT
           ? e->account_id > 0 && e->player_id > 0
           : e->account_id == 0 && e->player_id == 0 && e->requested_favor_eighths == 0;
}
static void
reward_beneficiary(const struct clan_reward_event* e, char out[33])
{
  if (e->player_id)
    snprintf(out, 33, "%" PRId64, e->player_id);
  else
    strcpy(out, e->clan_uid);
}
static struct json_object*
reward_event_json(const struct clan_reward_event* e)
{
  struct json_object* o = json_object_new_object();
  struct clan_reset_period p;
  if (!clan_reward_event_valid(e))
  {
    json_object_put(o);
    return NULL;
  }
  clan_progression_week_at(e->occurred_at, &p);
#define S(key) json_object_object_add(o, #key, json_object_new_string(e->key))
  S(operation_uid);
  S(event_uid);
  S(clan_uid);
#undef S
#define N(key) json_object_object_add(o, #key, json_object_new_int64(e->key))
  N(account_id);
  N(player_id);
  N(occurred_at);
  N(source);
  N(rules_version);
  N(requested_xp_eighths);
  N(requested_influence_eighths);
  N(requested_favor_eighths);
#undef N
  json_object_object_add(o, "starts_at", json_object_new_int64(p.starts_at));
  return o;
}
static bool
reward_event_parse(struct json_object* o, struct clan_reward_event* e)
{
  int64_t source, rules;
  if (!object(o, 12) || !text(o, "operation_uid", e->operation_uid, 33) ||
      !text(o, "event_uid", e->event_uid, 33) || !text(o, "clan_uid", e->clan_uid, 33) ||
      !number(o, "source", &source) || source < 1 || source > 9 ||
      !number(o, "rules_version", &rules) || rules < 1 || rules > INT32_MAX)
    return false;
  e->source = (enum clan_reward_source)source;
  e->rules_version = (int)rules;
#define N(key)                                                                                     \
  if (!number(o, #key, &e->key))                                                                   \
  return false
  N(account_id);
  N(player_id);
  N(occurred_at);
  N(requested_xp_eighths);
  N(requested_influence_eighths);
  N(requested_favor_eighths);
#undef N
  return clan_reward_event_valid(e);
}
static bool
reward_delta_valid(const struct clan_reward_event* e, const struct clan_reward_delta* d)
{
  if (d->xp_eighths < 0 || d->xp_eighths > e->requested_xp_eighths || d->influence_eighths < 0 ||
      d->influence_eighths > e->requested_influence_eighths || d->favor_eighths < 0 ||
      d->favor_eighths > e->requested_favor_eighths || d->account_base_xp_eighths < 0 ||
      d->account_base_xp_eighths > e->requested_xp_eighths ||
      d->incoming_xp_eighths < d->xp_eighths || d->incoming_xp_eighths > e->requested_xp_eighths ||
      d->account_claims < 0 || d->account_claims > 1 || d->cap_flags < 0)
    return false;
  return e->player_id ? d->incoming_xp_eighths <= d->account_base_xp_eighths
                      : !d->account_base_xp_eighths && !d->account_claims && !d->favor_eighths;
}
static struct json_object*
reward_delta_json(const struct clan_reward_delta* d)
{
  struct json_object* o = json_object_new_object();
#define N(key) json_object_object_add(o, #key, json_object_new_int64(d->key))
  N(xp_eighths);
  N(influence_eighths);
  N(favor_eighths);
  N(account_base_xp_eighths);
  N(account_claims);
  N(incoming_xp_eighths);
  N(cap_flags);
#undef N
  return o;
}
static bool
reward_proof_valid(const struct clan_reward_event* e, struct json_object* proof, int status)
{
  struct clan_reward_delta d;
  struct json_object *delta, *side, *claims;
  int64_t n;
  int i, j;
  const char* keys[] = {
    "clan_xp",         "clan_influence",   "clan_revision",  "favor",
    "favor_revision",  "account_base_xp",  "account_favor",  "account_revision",
    "weekly_incoming", "weekly_influence", "weekly_revision"
  };
  if (status == 2)
    return object(proof, 1) && number(proof, "reason", &n) && n > 0 && n <= INT32_MAX;
  if (!object(proof, 3) || !(delta = field(proof, "delta", json_type_object)) || !object(delta, 7))
    return false;
#define N(key)                                                                                     \
  if (!number(delta, #key, &d.key))                                                                \
  return false
  N(xp_eighths);
  N(influence_eighths);
  N(favor_eighths);
  N(account_base_xp_eighths);
  N(account_claims);
  N(incoming_xp_eighths);
#undef N
  if (!number(delta, "cap_flags", &n) || n < 0 || n > INT32_MAX)
    return false;
  d.cap_flags = (int)n;
  if (!reward_delta_valid(e, &d))
    return false;
  for (i = 0; i < 2; i++)
  {
    side = field(proof, i ? "after" : "before", json_type_object);
    if (!object(side, 12))
      return false;
    for (j = 0; j < 11; j++)
      if (!number(side, keys[j], &n) || n < ((j == 4 || j == 7 || j == 10) ? -1 : 0))
        return false;
    claims = field(side, "account_claims", json_type_array);
    if (!claims || json_object_array_length(claims) != 7)
      return false;
    for (j = 0; j < 7; j++)
    {
      struct json_object* v = json_object_array_get_idx(claims, (size_t)j);
      char* end;
      intmax_t value;
      if (!v || !json_object_is_type(v, json_type_int))
        return false;
      errno = 0;
      value = strtoimax(json_object_get_string(v), &end, 10);
      if (errno || *end || value < 0 || value > INT64_MAX || (j == 0 && value))
        return false;
    }
  }
  return true;
}
static enum clan_repository_result
reward_read_where(MYSQL* db, const char* where, struct json_object** out)
{
  MYSQL_RES* res;
  MYSQL_ROW row;
  struct clan_reward_event e;
  struct clan_reset_period p;
  struct json_object *root, *proof = NULL;
  char beneficiary[33], week[32];
  int64_t v[11];
  const int columns[] = { 3, 4, 5, 6, 8, 9, 10, 11, 12, 13, 14 };
  size_t i;
  if (!query(db,
             "SELECT "
             "operation_uid,event_uid,clan_uid,account_id,player_id,source_id,occurred_at_utc,"
             "starts_at_utc,rules_version,requested_xp_eighths,requested_influence_eighths,"
             "requested_favor_eighths,receipt_status,accepted_at_utc,committed_at_utc,decision,"
             "beneficiary_key FROM clan_reward_receipts WHERE %s",
             where))
    return failure(db);
  res = mysql_store_result(db);
  if (!res)
    return failure(db);
  row = mysql_fetch_row(res);
  if (!row)
  {
    mysql_free_result(res);
    return CLAN_REPO_NOT_FOUND;
  }
  if (mysql_num_rows(res) != 1)
  {
    mysql_free_result(res);
    return CLAN_REPO_CONFLICT;
  }
  for (i = 0; i < 3; i++)
    if (!row[i] || strlen(row[i]) != 32)
      goto invalid;
  for (i = 0; i < 11; i++)
    if (!integer(row[columns[i]], &v[i]))
      goto invalid;
  memset(&e, 0, sizeof(e));
  strcpy(e.operation_uid, row[0]);
  strcpy(e.event_uid, row[1]);
  strcpy(e.clan_uid, row[2]);
  e.account_id = v[0];
  e.player_id = v[1];
  if (v[2] > 9 || v[4] > INT32_MAX)
    goto invalid;
  e.source = (enum clan_reward_source)v[2];
  e.occurred_at = v[3];
  e.rules_version = (int)v[4];
  e.requested_xp_eighths = v[5];
  e.requested_influence_eighths = v[6];
  e.requested_favor_eighths = v[7];
  if (!clan_reward_event_valid(&e) || v[8] > 2 || v[9] < e.occurred_at)
    goto invalid;
  clan_progression_week_at(e.occurred_at, &p);
  snprintf(week, sizeof(week), "%" PRId64, p.starts_at);
  reward_beneficiary(&e, beneficiary);
  if (!row[7] || strcmp(row[7], week) || !row[16] || strcmp(row[16], beneficiary))
    goto invalid;
  if (v[8])
  {
    struct json_tokener* tok;
    if (v[10] < v[9] || !row[15] || strlen(row[15]) > 32768)
      goto invalid;
    tok = json_tokener_new();
    json_tokener_set_flags(tok, JSON_TOKENER_STRICT | JSON_TOKENER_VALIDATE_UTF8);
    proof = json_tokener_parse_ex(tok, row[15], (int)strlen(row[15]));
    if (json_tokener_get_error(tok) != json_tokener_success)
    {
      json_tokener_free(tok);
      goto invalid;
    }
    json_tokener_free(tok);
    if (!reward_proof_valid(&e, proof, (int)v[8]))
      goto invalid;
  }
  else if (v[10] || row[15])
    goto invalid;
  root = json_object_new_object();
  json_object_object_add(root, "event", reward_event_json(&e));
  json_object_object_add(root, "status", json_object_new_int64(v[8]));
  json_object_object_add(root, "accepted_at", json_object_new_int64(v[9]));
  json_object_object_add(root, "committed_at", json_object_new_int64(v[10]));
  json_object_object_add(root, "decision", proof);
  mysql_free_result(res);
  *out = root;
  return CLAN_REPO_OK;
invalid:
#ifdef CLAN_REWARD_TEST
  fprintf(stderr, "Invalid receipt %s; status=%s accepted=%s committed=%s decision=%s\n", where,
      row[12]?row[12]:"null",row[13]?row[13]:"null",row[14]?row[14]:"null",row[15]?row[15]:"null");
#endif
  if (proof)
    json_object_put(proof);
  mysql_free_result(res);
  return CLAN_REPO_SCHEMA;
}
enum clan_repository_result
clan_reward_load(MYSQL* db, const char* uid, struct json_object** out)
{
  enum clan_repository_result status;
  char where[128];
  if (!out || !clan_progression_uid_valid(uid))
    return CLAN_REPO_INVALID;
  status = clan_repository_schema_check(db);
  if (status != CLAN_REPO_OK)
    return status;
  snprintf(where, sizeof(where), "operation_uid='%s'", uid);
  return reward_read_where(db, where, out);
}
static enum clan_repository_result
reward_begin(MYSQL* db)
{
  enum clan_repository_result status = clan_repository_schema_check(db);
  MYSQL_RES* res;
  MYSQL_ROW row;
  bool ok;
  if (status != CLAN_REPO_OK)
    return status;
  if (db->server_status & SERVER_STATUS_IN_TRANS)
    return CLAN_REPO_INVALID;
  if (!query(db, "START TRANSACTION"))
    return failure(db);
  if (!query(db, "SELECT store_mode FROM clan_store_control WHERE singleton=1 FOR UPDATE"))
  {
    status = failure(db);
    goto failed;
  }
  res = mysql_store_result(db);
  if (!res)
  {
    status = failure(db);
    goto failed;
  }
  row = mysql_fetch_row(res);
  ok = row && !strcmp(row[0], "2");
  mysql_free_result(res);
  if (ok)
    return CLAN_REPO_OK;
  status = CLAN_REPO_CONFLICT;
failed:
  query(db, "ROLLBACK");
  return status;
}
static enum clan_repository_result
reward_abort(MYSQL* db, enum clan_repository_result status, struct json_object* root)
{
  query(db, "ROLLBACK");
  if (root)
    json_object_put(root);
  return status;
}
static enum clan_repository_result
reward_finish(MYSQL* db, struct json_object* root, struct json_object** out, int point)
{
  enum clan_repository_result status = finish(db, true);
  if (status != CLAN_REPO_OK)
  {
    json_object_put(root);
    return status;
  }
  if (reward_stop(point))
  {
    json_object_put(root);
    return CLAN_REPO_COMMIT_UNKNOWN;
  }
  *out = root;
  return CLAN_REPO_OK;
}
static enum clan_repository_result
reward_prepare(MYSQL* db,
                    const struct clan_reward_event* e,
                    int64_t now,
                    struct json_object** out, bool require_member)
{
  enum clan_repository_result status;
  struct json_object *root = NULL, *wanted, *stored;
  MYSQL_RES* res;
  MYSQL_ROW row;
  struct clan_repository_record clan;
  struct clan_reset_period period;
  char where[320], beneficiary[33];
  int64_t count;
  if (!out || !clan_reward_event_valid(e) || now < e->occurred_at)
    return CLAN_REPO_INVALID;
  status = reward_begin(db);
  if (status != CLAN_REPO_OK)
    return status;
  reward_beneficiary(e, beneficiary);
  clan_progression_week_at(e->occurred_at, &period);
  snprintf(
    where,
    sizeof(where),
    "operation_uid='%s' OR (event_uid='%s' AND source_id=%d AND beneficiary_key='%s') FOR UPDATE",
    e->operation_uid,
    e->event_uid,
    (int)e->source,
    beneficiary);
  status = reward_read_where(db, where, &root);
  if (status == CLAN_REPO_OK)
  {
    /* An identical event under a different operation nonce returns its first
     * receipt. A different target/request/occurrence never becomes a retry. */
    wanted = reward_event_json(e);
    stored = reward_get(root, "event");
    json_object_object_del(wanted, "operation_uid");
    {
      struct json_object* prior =
        json_tokener_parse(json_object_to_json_string_ext(stored, JSON_C_TO_STRING_PLAIN));
      bool equal;
      json_object_object_del(prior, "operation_uid");
      equal = json_object_equal(wanted, prior);
      json_object_put(wanted);
      json_object_put(prior);
      if (!equal)
        return reward_abort(db, CLAN_REPO_CONFLICT, root);
    }
    query(db, "ROLLBACK");
    *out = root;
    return CLAN_REPO_OK;
  }
  if (status != CLAN_REPO_NOT_FOUND)
    return reward_abort(db, status, NULL);
  status = clan_repository_load(db, e->clan_uid, &clan);
  if (status != CLAN_REPO_OK)
    return reward_abort(db, status, NULL);
  if (clan.status != CLAN_RECORD_ACTIVE)
    return reward_abort(db, CLAN_REPO_DELETED, NULL);
  if (require_member) {
    if (!e->player_id) return reward_abort(db, CLAN_REPO_INVALID, NULL);
    int64_t ranks;
    if (!query(db, "SELECT m.rank_id,JSON_LENGTH(JSON_EXTRACT(l.details,'$.ranks')) "
        "FROM clan_memberships m JOIN clan_progression_legacy l ON l.clan_uid=m.clan_uid "
        "WHERE m.player_id=%" PRId64 " AND m.clan_uid='%s' FOR UPDATE",
        e->player_id, e->clan_uid)) goto failed;
    res = mysql_store_result(db);
    if (!res) goto failed;
    row = mysql_fetch_row(res);
    bool accepted = row && integer(row[0], &count) && integer(row[1], &ranks) &&
        count > 0 && count <= ranks;
    mysql_free_result(res);
    if (!accepted) return reward_abort(db, CLAN_REPO_INELIGIBLE, NULL);
  }
  if (!query(db, "SELECT COUNT(*) FROM clan_reward_receipts WHERE receipt_status=0"))
    goto failed;
  res = mysql_store_result(db);
  if (!res)
    goto failed;
  row = mysql_fetch_row(res);
  if (!row || !integer(row[0], &count))
  {
    mysql_free_result(res);
    return reward_abort(db, CLAN_REPO_SCHEMA, NULL);
  }
  mysql_free_result(res);
  if (count >= clan_progression_defaults.limits.pending_per_server)
    return reward_abort(db, CLAN_REPO_CAPACITY, NULL);
  if (e->player_id)
  {
    if (!query(
          db,
          "SELECT COUNT(*) FROM clan_reward_receipts WHERE receipt_status=0 AND player_id=%" PRId64,
          e->player_id))
      goto failed;
    res = mysql_store_result(db);
    if (!res)
      goto failed;
    row = mysql_fetch_row(res);
    if (!row || !integer(row[0], &count))
    {
      mysql_free_result(res);
      return reward_abort(db, CLAN_REPO_SCHEMA, NULL);
    }
    mysql_free_result(res);
    if (count >= clan_progression_defaults.limits.pending_per_character)
      return reward_abort(db, CLAN_REPO_CAPACITY, NULL);
  }
  if (!query(db,
             "INSERT INTO "
             "clan_reward_receipts(operation_uid,event_uid,clan_uid,beneficiary_key,account_id,"
             "player_id,source_id,occurred_at_utc,starts_at_utc,rules_version,requested_xp_eighths,"
             "requested_influence_eighths,requested_favor_eighths,accepted_at_utc) "
             "VALUES('%s','%s','%s','%s',%" PRId64 ",%" PRId64 ",%d,%" PRId64 ",%" PRId64
             ",%d,%" PRId64 ",%" PRId64 ",%" PRId64 ",%" PRId64 ")",
             e->operation_uid,
             e->event_uid,
             e->clan_uid,
             beneficiary,
             e->account_id,
             e->player_id,
             (int)e->source,
             e->occurred_at,
             period.starts_at,
             e->rules_version,
             e->requested_xp_eighths,
             e->requested_influence_eighths,
             e->requested_favor_eighths,
             now))
    goto failed;
  snprintf(where, sizeof(where), "operation_uid='%s'", e->operation_uid);
  status = reward_read_where(db, where, &root);
  if (status != CLAN_REPO_OK)
    return reward_abort(db, status, NULL);
  return reward_finish(db, root, out, 1);
failed:
  return reward_abort(db, failure(db), NULL);
}
static bool
reward_lock(MYSQL* db, const char* sql)
{
  MYSQL_RES* res;
  if (!query(db, "%s", sql))
    return false;
  res = mysql_store_result(db);
  if (!res)
    return false;
  mysql_free_result(res);
  return true;
}
static bool
reward_add(int64_t* value, int64_t amount)
{
  if (amount < 0 || *value < 0 || *value > INT64_MAX - amount)
    return false;
  *value += amount;
  return true;
}
enum clan_repository_result clan_reward_prepare(MYSQL *db, const struct clan_reward_event *e,
    int64_t now, struct json_object **out)
{ return reward_prepare(db, e, now, out, false); }
enum clan_repository_result clan_reward_prepare_member(MYSQL *db, const struct clan_reward_event *e,
    int64_t now, struct json_object **out)
{ return reward_prepare(db, e, now, out, true); }

static struct json_object*
reward_snapshot_json(const struct clan_reward_snapshot* s)
{
  struct json_object *o = json_object_new_object(), *claims = json_object_new_array();
  size_t i;
#define N(key, value) json_object_object_add(o, key, json_object_new_int64(value))
  N("clan_xp", s->clan.xp_eighths);
  N("clan_influence", s->clan.influence_eighths);
  N("clan_revision", s->clan.state_revision);
  N("favor", s->favor.favor_eighths);
  N("favor_revision", s->favor_present ? s->favor.revision : -1);
  N("account_base_xp", s->account.base_xp_eighths);
  N("account_favor", s->account.favor_eighths);
  N("account_revision", s->account_present ? s->account.revision : -1);
  N("weekly_incoming", s->weekly.incoming_xp_eighths);
  N("weekly_influence", s->weekly.influence_eighths);
  N("weekly_revision", s->weekly_present ? s->weekly.revision : -1);
#undef N
  for (i = 0; i < 7; i++)
    json_object_array_add(claims, json_object_new_int64(s->account.claims[i]));
  json_object_object_add(o, "account_claims", claims);
  return o;
}
static enum clan_repository_result
reward_snapshot_load(MYSQL* db, const struct clan_reward_event* e, struct clan_reward_snapshot* s)
{
  char sql[300];
  enum clan_repository_result status;
  MYSQL_RES* res;
  MYSQL_ROW row;
  struct clan_reset_period period;
  memset(s, 0, sizeof(*s));
  clan_progression_week_at(e->occurred_at, &period);
  /* Global control serializes all authoritative writers, followed by receipt,
   * account window, clan core/state, personal Favor, clan week. */
  if (e->source == CLAN_SOURCE_EXPEDITION_PERSONAL) {
    int64_t rank, ranks;
    if (!query(db, "SELECT m.rank_id,JSON_LENGTH(JSON_EXTRACT(l.details,'$.ranks')) "
        "FROM clan_memberships m JOIN clan_progression_legacy l ON l.clan_uid=m.clan_uid "
        "WHERE m.player_id=%" PRId64 " AND m.clan_uid='%s' FOR UPDATE",
        e->player_id, e->clan_uid)) return failure(db);
    res = mysql_store_result(db);
    if (!res) return failure(db);
    row = mysql_fetch_row(res);
    bool accepted = row && integer(row[0], &rank) && integer(row[1], &ranks) &&
        rank > 0 && rank <= ranks;
    mysql_free_result(res);
    if (!accepted) return CLAN_REPO_INELIGIBLE;
  }
  if (e->player_id)
  {
    snprintf(sql,
             sizeof(sql),
             "SELECT account_id FROM clan_account_windows WHERE account_id=%" PRId64
             " AND starts_at_utc=%" PRId64 " FOR UPDATE",
             e->account_id,
             period.starts_at);
    if (!reward_lock(db, sql))
      return failure(db);
    status = clan_repository_account_window_load(db, e->account_id, e->occurred_at, &s->account);
    if (status == CLAN_REPO_NOT_FOUND)
      clan_progression_account_window_init(e->account_id, e->occurred_at, &s->account);
    else if (status != CLAN_REPO_OK)
      return status;
    else
      s->account_present = true;
  }
  snprintf(sql,
           sizeof(sql),
           "SELECT c.clan_uid FROM clan_progression_clans c JOIN clan_progression_state s ON "
           "s.clan_uid=c.clan_uid WHERE c.clan_uid='%s' FOR UPDATE",
           e->clan_uid);
  if (!reward_lock(db, sql))
    return failure(db);
  status = clan_repository_load(db, e->clan_uid, &s->clan);
  if (status != CLAN_REPO_OK)
    return status;
  if (s->clan.status != CLAN_RECORD_ACTIVE)
    return CLAN_REPO_DELETED;
  if (e->player_id)
  {
    snprintf(sql,
             sizeof(sql),
             "SELECT player_id FROM clan_personal_favor WHERE player_id=%" PRId64
             " AND clan_uid='%s' FOR UPDATE",
             e->player_id,
             e->clan_uid);
    if (!reward_lock(db, sql))
      return failure(db);
    status = clan_repository_favor_load(db, e->player_id, e->clan_uid, &s->favor);
    if (status == CLAN_REPO_NOT_FOUND)
    {
      s->favor.character_id = e->player_id;
      strcpy(s->favor.clan_uid, e->clan_uid);
    }
    else if (status != CLAN_REPO_OK)
      return status;
    else
      s->favor_present = true;
  }
  s->weekly.starts_at = period.starts_at;
  strcpy(s->weekly.clan_uid, e->clan_uid);
  if (!query(db,
             "SELECT incoming_xp_eighths,influence_eighths,revision,updated_at_utc FROM clan_weekly_windows WHERE "
             "clan_uid='%s' AND starts_at_utc=%" PRId64 " FOR UPDATE",
             e->clan_uid,
             period.starts_at))
    return failure(db);
  res = mysql_store_result(db);
  if (!res)
    return failure(db);
  row = mysql_fetch_row(res);
  if (row)
  {
    int64_t written_at;
    if (!integer(row[0], &s->weekly.incoming_xp_eighths) ||
        !integer(row[1], &s->weekly.influence_eighths) || !integer(row[2], &s->weekly.revision) ||
        !integer(row[3], &written_at) || written_at < s->weekly.starts_at)
    {
      mysql_free_result(res);
      return CLAN_REPO_SCHEMA;
    }
    s->weekly_present = true;
  }
  mysql_free_result(res);
  return CLAN_REPO_OK;
}
static enum clan_repository_result
reward_stage(MYSQL* db,
             const struct clan_reward_event* e,
             const struct clan_reward_delta* d,
             struct clan_reward_snapshot* s,
             int64_t now)
{
  enum clan_repository_result status;
  if (now < s->clan.state_updated_at)
    return CLAN_REPO_CONFLICT;
  if (e->player_id && (d->account_base_xp_eighths || d->favor_eighths || d->account_claims))
  {
    if (!reward_add(&s->account.base_xp_eighths, d->account_base_xp_eighths) ||
        !reward_add(&s->account.favor_eighths, d->favor_eighths) ||
        !reward_add(&s->account.claims[e->source], d->account_claims))
      return CLAN_REPO_INVALID;
    status = account_window_write(
      db, &s->account, s->account_present ? s->account.revision : -1, now, true);
    if (status != CLAN_REPO_OK)
      return status;
    if (s->account_present)
      s->account.revision++;
    else
      s->account_present = true;
  }
  if (reward_stop(2))
    return CLAN_REPO_UNAVAILABLE;
  if (d->xp_eighths || d->influence_eighths)
  {
    if (s->clan.state_revision == INT64_MAX || !reward_add(&s->clan.xp_eighths, d->xp_eighths) ||
        !reward_add(&s->clan.influence_eighths, d->influence_eighths))
      return CLAN_REPO_INVALID;
    if (!query(db,
               "UPDATE clan_progression_state SET xp_eighths=%" PRId64 ",influence_eighths=%" PRId64
               ",revision=revision+1,updated_at_utc=%" PRId64
               " WHERE clan_uid='%s' AND revision=%" PRId64,
               s->clan.xp_eighths,
               s->clan.influence_eighths,
               now,
               e->clan_uid,
               s->clan.state_revision))
      return failure(db);
    if (mysql_affected_rows(db) != 1)
      return CLAN_REPO_CONFLICT;
    s->clan.state_revision++;
  }
  if (reward_stop(3))
    return CLAN_REPO_UNAVAILABLE;
  if (d->favor_eighths)
  {
    if (!reward_add(&s->favor.favor_eighths, d->favor_eighths))
      return CLAN_REPO_INVALID;
    status = favor_write(db, &s->favor, s->favor_present ? s->favor.revision : -1, now, true);
    if (status != CLAN_REPO_OK)
      return status;
    if (s->favor_present)
      s->favor.revision++;
    else
      s->favor_present = true;
  }
  if (reward_stop(4))
    return CLAN_REPO_UNAVAILABLE;
  if (d->incoming_xp_eighths || d->influence_eighths)
  {
    if (s->weekly.revision == INT64_MAX ||
        !reward_add(&s->weekly.incoming_xp_eighths, d->incoming_xp_eighths) ||
        !reward_add(&s->weekly.influence_eighths, d->influence_eighths))
      return CLAN_REPO_INVALID;
    if (s->weekly_present)
    {
      if (!query(db,
                 "UPDATE clan_weekly_windows SET incoming_xp_eighths=%" PRId64
                 ",influence_eighths=%" PRId64 ",revision=revision+1,updated_at_utc=%" PRId64
                 " WHERE clan_uid='%s' AND starts_at_utc=%" PRId64 " AND revision=%" PRId64
                 " AND updated_at_utc<=%" PRId64,
                 s->weekly.incoming_xp_eighths,
                 s->weekly.influence_eighths,
                 now,
                 e->clan_uid,
                 s->weekly.starts_at,
                 s->weekly.revision,
                 now))
        return failure(db);
      if (mysql_affected_rows(db) != 1)
        return CLAN_REPO_CONFLICT;
      s->weekly.revision++;
    }
    else
    {
      if (!query(db,
                 "INSERT INTO "
                 "clan_weekly_windows(clan_uid,starts_at_utc,incoming_xp_eighths,influence_eighths,"
                 "updated_at_utc) VALUES('%s',%" PRId64 ",%" PRId64 ",%" PRId64 ",%" PRId64 ")",
                 e->clan_uid,
                 s->weekly.starts_at,
                 s->weekly.incoming_xp_eighths,
                 s->weekly.influence_eighths,
                 now))
        return failure(db);
      s->weekly_present = true;
    }
  }
  return reward_stop(5) ? CLAN_REPO_UNAVAILABLE : CLAN_REPO_OK;
}
enum clan_repository_result
clan_reward_complete(MYSQL* db,
                     const char* uid,
                     int64_t now,
                     clan_reward_decide_fn decide,
                     void* context,
                     struct json_object** out)
{
  enum clan_repository_result status;
  struct json_object *root = NULL, *proof, *before;
  struct clan_reward_event e;
  struct clan_reward_snapshot snapshot;
  struct clan_reward_delta delta;
  char where[128], encoded[65537];
  int64_t receipt_status;
  if (!out || !clan_progression_uid_valid(uid) || now < 0)
    return CLAN_REPO_INVALID;
  status = reward_begin(db);
  if (status != CLAN_REPO_OK)
    return status;
  snprintf(where, sizeof(where), "operation_uid='%s' FOR UPDATE", uid);
  status = reward_read_where(db, where, &root);
  if (status != CLAN_REPO_OK)
    return reward_abort(db, status, NULL);
  number(root, "status", &receipt_status);
  if (receipt_status == 1)
  {
    query(db, "ROLLBACK");
    *out = root;
    return CLAN_REPO_OK;
  }
  if (receipt_status != 0)
    return reward_abort(db, CLAN_REPO_CONFLICT, root);
  if (!decide || now < json_object_get_int64(reward_get(root, "accepted_at")) ||
      !reward_event_parse(reward_get(root, "event"), &e))
    return reward_abort(db, CLAN_REPO_INVALID, root);
  status = reward_snapshot_load(db, &e, &snapshot);
  if (status != CLAN_REPO_OK)
    return reward_abort(db, status, root);
  memset(&delta, 0, sizeof(delta));
  if (!decide(&e, &snapshot, &delta, context) || !reward_delta_valid(&e, &delta))
    return reward_abort(db, CLAN_REPO_INVALID, root);
  before = reward_snapshot_json(&snapshot);
  status = reward_stage(db, &e, &delta, &snapshot, now);
  if (status != CLAN_REPO_OK)
  {
    json_object_put(before);
    return reward_abort(db, status, root);
  }
  proof = json_object_new_object();
  json_object_object_add(proof, "delta", reward_delta_json(&delta));
  json_object_object_add(proof, "before", before);
  json_object_object_add(proof, "after", reward_snapshot_json(&snapshot));
  if (!reward_proof_valid(&e, proof, 1))
  {
    json_object_put(proof);
    return reward_abort(db, CLAN_REPO_INVALID, root);
  }
  hex(encoded, json_object_to_json_string_ext(proof, JSON_C_TO_STRING_PLAIN));
  json_object_put(proof);
  if (!query(db,
             "UPDATE clan_reward_receipts SET receipt_status=1,committed_at_utc=%" PRId64
             ",decision=_utf8mb4 X'%s' WHERE operation_uid='%s' AND receipt_status=0",
             now,
             encoded,
             uid))
    return reward_abort(db, failure(db), root);
  if (mysql_affected_rows(db) != 1)
    return reward_abort(db, CLAN_REPO_CONFLICT, root);
  if (reward_stop(6) || reward_stop(7))
    return reward_abort(db, CLAN_REPO_UNAVAILABLE, root);
  json_object_put(root);
  root = NULL;
  status = reward_read_where(db, where, &root);
  if (status != CLAN_REPO_OK)
    return reward_abort(db, status, NULL);
  return reward_finish(db, root, out, 8);
}
enum clan_repository_result
clan_reward_cancel(MYSQL* db, const char* uid, int reason, int64_t now, struct json_object** out)
{
  enum clan_repository_result status;
  struct json_object* root = NULL;
  char where[128];
  int64_t state;
  if (!out || !clan_progression_uid_valid(uid) || reason <= 0 || now < 0)
    return CLAN_REPO_INVALID;
  status = reward_begin(db);
  if (status != CLAN_REPO_OK)
    return status;
  snprintf(where, sizeof(where), "operation_uid='%s' FOR UPDATE", uid);
  status = reward_read_where(db, where, &root);
  if (status != CLAN_REPO_OK)
    return reward_abort(db, status, NULL);
  number(root, "status", &state);
  if (state == 2)
  {
    if (json_object_get_int(reward_get(reward_get(root, "decision"), "reason")) != reason)
      return reward_abort(db, CLAN_REPO_CONFLICT, root);
    query(db, "ROLLBACK");
    *out = root;
    return CLAN_REPO_OK;
  }
  if (state != 0 || now < json_object_get_int64(reward_get(root, "accepted_at")))
    return reward_abort(db, CLAN_REPO_CONFLICT, root);
  if (!query(db,
             "UPDATE clan_reward_receipts SET receipt_status=2,committed_at_utc=%" PRId64
             ",decision=JSON_OBJECT('reason',%d) WHERE operation_uid='%s' AND receipt_status=0",
             now,
             reason,
             uid))
    return reward_abort(db, failure(db), root);
  if (mysql_affected_rows(db) != 1)
    return reward_abort(db, CLAN_REPO_CONFLICT, root);
  json_object_put(root);
  root = NULL;
  status = reward_read_where(db, where, &root);
  if (status != CLAN_REPO_OK)
    return reward_abort(db, status, NULL);
  return reward_finish(db, root, out, 8);
}
enum clan_repository_result
clan_reward_pending(MYSQL* db,
                    int64_t player,
                    const char* cursor,
                    size_t limit,
                    struct json_object** out)
{
  enum clan_repository_result status;
  MYSQL_RES* res;
  MYSQL_ROW row;
  struct json_object* array;
  if (!out || player < 0 || !limit || limit > 1000 ||
      (cursor && !clan_progression_uid_valid(cursor)))
    return CLAN_REPO_INVALID;
  status = clan_repository_schema_check(db);
  if (status != CLAN_REPO_OK)
    return status;
  if (!query(db,
             "SELECT operation_uid FROM clan_reward_receipts WHERE receipt_status=0 AND (%" PRId64
             "=0 OR player_id=%" PRId64 ") AND operation_uid>'%s' ORDER BY operation_uid LIMIT %lu",
             player,
             player,
             cursor ? cursor : "",
             (unsigned long)limit))
    return failure(db);
  res = mysql_store_result(db);
  if (!res)
    return failure(db);
  array = json_object_new_array();
  while ((row = mysql_fetch_row(res)))
  {
    if (!row[0] || !clan_progression_uid_valid(row[0]))
    {
      mysql_free_result(res);
      json_object_put(array);
      return CLAN_REPO_SCHEMA;
    }
    json_object_array_add(array, json_object_new_string(row[0]));
  }
  mysql_free_result(res);
  *out = array;
  return CLAN_REPO_OK;
}


static bool uid_field_is(struct json_object *o, const char *key, const char *uid)
{
  struct json_object *value = reward_get(o, key);
  const char *text = json_object_get_string(value);
  return text && !strcmp(text, uid);
}
bool clan_repository_snapshot_disband(struct json_object *root, const char *uid)
{
  struct json_object *clans, *members, *claims;
  size_t i, j;
  bool found = false;
  if (!clan_progression_uid_valid(uid) || !clan_repository_snapshot_valid(root)) return false;
  clans = reward_get(root, "clans");members = reward_get(root, "members");claims = reward_get(root, "claims");
  for (i = json_object_array_length(clans); i > 0; i--) {
    struct json_object *entry = json_object_array_get_idx(clans, i-1);
    if (uid_field_is(reward_get(entry, "core"), "uid", uid)) {
      json_object_array_del_idx(clans, i-1, 1);found = true;
    } else {
      struct json_object *edges = reward_get(reward_get(entry, "legacy"), "relationships");
      for (j = json_object_array_length(edges); j > 0; j--)
        if (uid_field_is(json_object_array_get_idx(edges, j-1), "peer_uid", uid))
          json_object_array_del_idx(edges, j-1, 1);
    }
  }
  if (!found) return false;
  for (i = json_object_array_length(members); i > 0; i--)
    if (uid_field_is(json_object_array_get_idx(members, i-1), "clan_uid", uid))
      json_object_array_del_idx(members, i-1, 1);
  for (i = 0; i < json_object_array_length(claims); i++) {
    struct json_object *claim = json_object_array_get_idx(claims, i);
    struct json_object *popularity = reward_get(claim, "popularity");
    if (uid_field_is(claim, "controller_uid", uid)) {
      json_object_object_add(claim, "controller_uid", NULL);
      json_object_object_add(claim, "claimant_id", json_object_new_int64(0));
    }
    for (j = json_object_array_length(popularity); j > 0; j--)
      if (uid_field_is(json_object_array_get_idx(popularity, j-1), "clan_uid", uid))
        json_object_array_del_idx(popularity, j-1, 1);
  }
  return clan_repository_snapshot_canonicalize(root) && clan_repository_snapshot_valid(root);
}
enum clan_repository_result clan_repository_disband(MYSQL *db, const char *uid,
    int64_t revision, int64_t now)
{
  struct json_object *root = NULL;
  struct clan_repository_record r;
  enum clan_repository_result status;
  if (!clan_progression_uid_valid(uid) || revision < 0 || now < 0) return CLAN_REPO_INVALID;
  status = clan_repository_load(db, uid, &r);
  if (status != CLAN_REPO_OK) return status;
  if (r.status != CLAN_RECORD_ACTIVE) return CLAN_REPO_DELETED;
  status = clan_repository_snapshot_read(db, &root);
  if (status != CLAN_REPO_OK) return status;
  if (json_object_get_int64(reward_get(root, "store_revision")) != revision)
    status = CLAN_REPO_CONFLICT;
  else if (!clan_repository_snapshot_disband(root, uid)) status = CLAN_REPO_CONFLICT;
  else {
    json_object_object_add(root, "written_at", json_object_new_int64(now));
    status = clan_repository_snapshot_write(db, root, revision, NULL);
  }
  json_object_put(root);
  return status;
}
enum clan_repository_result clan_repository_member_access(MYSQL *db, int64_t player,
    const char *uid, enum clan_member_access *out)
{
  struct json_object *root = NULL, *clans, *members;
  enum clan_repository_result status;
  enum clan_member_access access = CLAN_ACCESS_FORMER;
  struct clan_repository_record record;
  size_t i;
  bool found = false;
  if (!out || player <= 0 || !clan_progression_uid_valid(uid)) return CLAN_REPO_INVALID;
  status = clan_repository_snapshot_read(db, &root);
  if (status != CLAN_REPO_OK) return status;
  clans = reward_get(root, "clans");members = reward_get(root, "members");
  for (i = 0; i < json_object_array_length(clans); i++)
    if (uid_field_is(reward_get(json_object_array_get_idx(clans, i), "core"), "uid", uid)) found = true;
  if (found) {
    for (i = 0; i < json_object_array_length(members); i++) {
      struct json_object *m = json_object_array_get_idx(members, i);
      if (json_object_get_int64(reward_get(m, "player_id")) == player && uid_field_is(m, "clan_uid", uid))
        access = json_object_get_int(reward_get(m, "rank_id")) > 0 ? CLAN_ACCESS_ACCEPTED : CLAN_ACCESS_APPLICANT;
    }
  } else {
    status = clan_repository_load(db, uid, &record);
    if (status == CLAN_REPO_OK) {
      if (record.status != CLAN_RECORD_DELETED) status = CLAN_REPO_CONFLICT;
      else access = CLAN_ACCESS_DELETED;
    }
  }
  json_object_put(root);
  if (status == CLAN_REPO_OK) *out = access;
  return status;
}
