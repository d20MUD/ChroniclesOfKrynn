/* Offline operator bridge to the same checked repository used by the server. */
#include "clan_repository.h"
#include <json-c/json.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static struct json_object*
get(struct json_object* o, const char* name)
{
  struct json_object* v = NULL;
  json_object_object_get_ex(o, name, &v);
  return v;
}
int
main(int argc, char** argv)
{
  MYSQL* db = NULL;
  struct json_object *request = NULL, *out = NULL;
  enum clan_repository_result status = CLAN_REPO_INVALID;
  char* input = malloc(2097153);
  size_t n;
  if (!input)
    return 2;
  n = fread(input, 1, 2097153, stdin);
  if (n > 2097152 || ferror(stdin))
  {
    free(input);
    return 2;
  }
  input[n] = 0;
  {
    struct json_tokener* tok = json_tokener_new();
    json_tokener_set_flags(tok, JSON_TOKENER_STRICT | JSON_TOKENER_VALIDATE_UTF8);
    request = json_tokener_parse_ex(tok, input, (int)n);
    if (json_tokener_get_error(tok) != json_tokener_success)
    {
      if (request)
        json_object_put(request);
      request = NULL;
    }
    json_tokener_free(tok);
  }
  free(input);
  if (!request || argc < 2)
    goto done;
  if (!strcmp(argv[1], "validate"))
  {
    status = clan_repository_snapshot_valid(request) ? CLAN_REPO_OK : CLAN_REPO_INVALID;
    goto done;
  }
  /* operation, database, user, host, socket. Password is environment-only. */
  if (argc != 6)
    goto done;
  db = mysql_init(NULL);
  if (!db)
  {
    status = CLAN_REPO_UNAVAILABLE;
    goto done;
  }
  {
    unsigned int timeout = 5;
    mysql_options(db, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);
  }
  if (!mysql_real_connect(db,
                          *argv[4] ? argv[4] : NULL,
                          argv[3],
                          getenv("MYSQL_PWD"),
                          argv[2],
                          0,
                          *argv[5] ? argv[5] : NULL,
                          0))
  {
    status = CLAN_REPO_UNAVAILABLE;
    goto done;
  }
  if (mysql_set_character_set(db, "utf8mb4") ||
      mysql_query(db, "SET SESSION sql_mode='STRICT_ALL_TABLES,NO_ENGINE_SUBSTITUTION'"))
  {
    status = CLAN_REPO_UNAVAILABLE;
    goto done;
  }
  if (!strcmp(argv[1], "read"))
    status = clan_repository_snapshot_read(db, &out);
  else if (!strcmp(argv[1], "save") || !strcmp(argv[1], "import"))
  {
    struct json_object *snapshot = get(request, "snapshot"),
                       *revision = get(request, "expected_revision");
    if (snapshot && revision && json_object_is_type(revision, json_type_int))
      status = clan_repository_snapshot_write(db,
                                              snapshot,
                                              json_object_get_int64(revision),
                                              !strcmp(argv[1], "import") ? get(request, "manifest")
                                                                         : NULL);
  }
  else if (!strcmp(argv[1], "manifest"))
  {
    struct json_object* digest = get(request, "source_digest");
    const char* s = digest ? json_object_get_string(digest) : NULL;
    char query[256];
    MYSQL_RES* result;
    MYSQL_ROW row;
    size_t i;
    if (!s || strlen(s) != 64)
      goto done;
    for (i = 0; i < 64; i++)
      if (!((s[i] >= '0' && s[i] <= '9') || (s[i] >= 'a' && s[i] <= 'f')))
        goto done;
    status = clan_repository_schema_check(db);
    if (status != CLAN_REPO_OK)
      goto done;
    snprintf(query,
             sizeof(query),
             "SELECT import_uid,import_status,source_manifest,imported_at_utc FROM "
             "clan_import_manifests WHERE source_digest='%s'",
             s);
    if (mysql_query(db, query))
    {
      status = CLAN_REPO_UNAVAILABLE;
      goto done;
    }
    result = mysql_store_result(db);
    if (!result)
    {
      status = CLAN_REPO_UNAVAILABLE;
      goto done;
    }
    row = mysql_fetch_row(result);
    if (row)
    {
      out = json_object_new_object();
      json_object_object_add(out, "import_uid", json_object_new_string(row[0]));
      json_object_object_add(out, "import_status", json_object_new_int(atoi(row[1])));
      json_object_object_add(out, "source_manifest", json_tokener_parse(row[2]));
      json_object_object_add(out, "imported_at", json_object_new_int64(strtoll(row[3], NULL, 10)));
      json_object_object_add(out, "source_digest", json_object_new_string(s));
      status = CLAN_REPO_OK;
    }
    else
      status = CLAN_REPO_NOT_FOUND;
    mysql_free_result(result);
  }
done:
  if (!out)
    out = json_object_new_object();
  json_object_object_add(
    out, "result", json_object_new_string(clan_repository_result_name(status)));
  puts(json_object_to_json_string_ext(out, JSON_C_TO_STRING_PLAIN));
  json_object_put(out);
  if (request)
    json_object_put(request);
  if (db)
    mysql_close(db);
  return status == CLAN_REPO_OK ? 0 : status == CLAN_REPO_NOT_FOUND ? 3 : 2;
}
