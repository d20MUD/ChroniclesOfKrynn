/* Checked clan persistence. No startup/import/membership/gameplay hooks. */
#include "clan_repository.h"
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
                                 "deleted",     "commit-unknown" };
  return r >= CLAN_REPO_OK && r <= CLAN_REPO_COMMIT_UNKNOWN ? names[r] : "invalid-result";
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
enum clan_repository_result
clan_repository_schema_check(MYSQL* db)
{
  MYSQL_RES* result;
  MYSQL_ROW row;
  size_t i;
  char actual[32768], expected[8192];
  if (!db || mysql_ping(db))
    return CLAN_REPO_UNAVAILABLE;
  if (!strict_session(db))
    return CLAN_REPO_SCHEMA;
  if (!query(db, "SELECT version FROM clan_progression_schema WHERE singleton=1"))
    return CLAN_REPO_SCHEMA;
  result = mysql_store_result(db);
  if (!result)
    return CLAN_REPO_SCHEMA;
  row = mysql_fetch_row(result);
  if (!row || !row[0] || atoi(row[0]) != CLAN_PROGRESSION_SCHEMA_VERSION)
  {
    mysql_free_result(result);
    return CLAN_REPO_SCHEMA;
  }
  mysql_free_result(result);
  for (i = 0; i < sizeof(clan_schema_fragments) / sizeof(*clan_schema_fragments); i++)
  {
    if (!query(db, "SHOW CREATE TABLE %s", clan_schema_fragments[i].table))
      return CLAN_REPO_SCHEMA;
    result = mysql_store_result(db);
    if (!result)
      return CLAN_REPO_SCHEMA;
    row = mysql_fetch_row(result);
    if (!row || !row[1])
    {
      mysql_free_result(result);
      return CLAN_REPO_SCHEMA;
    }
    canonical(actual, sizeof(actual), row[1]);
    canonical(expected, sizeof(expected), clan_schema_fragments[i].fragment);
    if (!strstr(actual, "engine=innodb") || !strstr(actual, expected))
    {
      mysql_free_result(result);
      return CLAN_REPO_SCHEMA;
    }
    mysql_free_result(result);
  }
  for (i = 0; i < sizeof(clan_schema_triggers) / sizeof(*clan_schema_triggers); i++)
  {
    if (!query(db,
               "SELECT ACTION_STATEMENT,EVENT_MANIPULATION,EVENT_OBJECT_TABLE,ACTION_TIMING FROM "
               "information_schema.TRIGGERS WHERE TRIGGER_SCHEMA=DATABASE() AND TRIGGER_NAME='%s'",
               clan_schema_triggers[i].name))
      return CLAN_REPO_SCHEMA;
    result = mysql_store_result(db);
    if (!result)
      return CLAN_REPO_SCHEMA;
    row = mysql_fetch_row(result);
    if (!row || !row[0] || !row[1] || !row[2] || !row[3])
    {
      mysql_free_result(result);
      return CLAN_REPO_SCHEMA;
    }
    canonical(actual, sizeof(actual), row[0]);
    canonical(expected, sizeof(expected), clan_schema_triggers[i].body);
    if (strcmp(actual, expected) || strcmp(row[1], clan_schema_triggers[i].event) ||
        strcmp(row[2], clan_schema_triggers[i].table) || strcmp(row[3], "BEFORE"))
    {
      mysql_free_result(result);
      return CLAN_REPO_SCHEMA;
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
      return CLAN_REPO_SCHEMA;
    result = mysql_store_result(db);
    if (!result)
      return CLAN_REPO_SCHEMA;
    row = mysql_fetch_row(result);
    if (!row || strcmp(row[0], clan_schema_columns[i].type) ||
        strcmp(row[2], clan_schema_columns[i].nullable) || strstr(row[3], "unsigned") ||
        (clan_schema_columns[i].length && atoi(row[1]) != clan_schema_columns[i].length) ||
        ((strstr(clan_schema_columns[i].name, "_uid") ||
          !strcmp(clan_schema_columns[i].name, "source_digest")) &&
         strcmp(row[4], "ascii_bin")))
    {
      mysql_free_result(result);
      return CLAN_REPO_SCHEMA;
    }
    if (clan_schema_columns[i].default_value)
    {
      size_t length;
      if (!row[5])
      {
        mysql_free_result(result);
        return CLAN_REPO_SCHEMA;
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
        return CLAN_REPO_SCHEMA;
      }
    }
    else if (row[5] && strcmp(row[5], "NULL"))
    {
      mysql_free_result(result);
      return CLAN_REPO_SCHEMA;
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
      return CLAN_REPO_SCHEMA;
    result = mysql_store_result(db);
    if (!result)
      return CLAN_REPO_SCHEMA;
    row = mysql_fetch_row(result);
    if (!row || strcmp(row[0], "1"))
    {
      mysql_free_result(result);
      return CLAN_REPO_SCHEMA;
    }
    mysql_free_result(result);
  }
  if (!query(db,
             "SELECT GENERATION_EXPRESSION FROM information_schema.COLUMNS WHERE "
             "TABLE_SCHEMA=DATABASE() AND TABLE_NAME='clan_progression_clans' AND "
             "COLUMN_NAME='active_vnum'"))
    return CLAN_REPO_SCHEMA;
  result = mysql_store_result(db);
  if (!result)
    return CLAN_REPO_SCHEMA;
  row = mysql_fetch_row(result);
  if (!row || !row[0])
  {
    mysql_free_result(result);
    return CLAN_REPO_SCHEMA;
  }
  canonical(actual, sizeof(actual), row[0]);
  mysql_free_result(result);
  if (strcmp(actual, "casewhenrecord_status=1thenclan_vnumelsenullend"))
    return CLAN_REPO_SCHEMA;
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
