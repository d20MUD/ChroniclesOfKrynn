#include "clan_repository.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void sql(MYSQL *db, const char *text)
{
  if (mysql_query(db, text)) {
    fprintf(stderr, "%s\n", mysql_error(db));
    abort();
  }
}
static unsigned long long questions(MYSQL *db)
{
  MYSQL_RES *res;
  MYSQL_ROW row;
  unsigned long long value;
  sql(db, "SHOW SESSION STATUS LIKE 'Questions'");
  res = mysql_store_result(db); assert(res);
  row = mysql_fetch_row(res); assert(row && row[1]);
  value = strtoull(row[1], NULL, 10);
  mysql_free_result(res);
  return value;
}
static void connect_db(MYSQL *storage, const char *socket)
{
  assert(mysql_init(storage) == storage);
  assert(mysql_real_connect(storage, NULL, "root", "", "clan_test", 0, socket, 0));
  sql(storage, "SET SESSION sql_mode='STRICT_ALL_TABLES,NO_ENGINE_SUBSTITUTION'");
}
int main(int argc, char **argv)
{
  MYSQL storage, *db = &storage, *admin;
  MYSQL_RES *res;
  MYSQL_ROW row;
  char *trigger;
  unsigned long previous_id;
  unsigned long long before, full_queries, cached_queries;
  int i;
  assert(argc == 2);
  connect_db(db, argv[1]);
  sql(db, "SHOW CREATE TRIGGER cp_state_history_no_delete");
  res = mysql_store_result(db); assert(res);
  row = mysql_fetch_row(res); assert(row && row[2]);
  trigger = malloc(strlen(row[2]) + 1); assert(trigger); strcpy(trigger, row[2]);
  mysql_free_result(res);

  /* Offline/default mode always validates, including after DDL. */
  before = questions(db);
  for (i = 0; i < 3; ++i) assert(clan_repository_schema_check(db) == CLAN_REPO_OK);
  full_queries = questions(db) - before;
  clan_repository_schema_cache_enable(true);
  assert(clan_repository_schema_check(db) == CLAN_REPO_OK);
  before = questions(db);
  for (i = 0; i < 3; ++i) assert(clan_repository_schema_check(db) == CLAN_REPO_OK);
  cached_queries = questions(db) - before;
  assert(cached_queries < 10);
  assert(full_queries > cached_queries * 50);
  printf("Core checks: %llu full vs %llu cached SQL queries for three validations.\n",
         full_queries, cached_queries);

  /* Session checks must never be bypassed by a cached metadata proof. */
  sql(db, "SET SESSION sql_mode=''");
  assert(clan_repository_schema_check(db) == CLAN_REPO_SCHEMA);
  sql(db, "SET SESSION sql_mode='STRICT_ALL_TABLES,NO_ENGINE_SUBSTITUTION'");
  assert(clan_repository_schema_check(db) == CLAN_REPO_OK);
  sql(db, "SET autocommit=0");
  assert(clan_repository_schema_check(db) == CLAN_REPO_SCHEMA);
  sql(db, "SET autocommit=1");
  assert(clan_repository_schema_check(db) == CLAN_REPO_OK);
  sql(db, "UPDATE clan_progression_schema SET version=7 WHERE singleton=1");
  assert(clan_repository_schema_check(db) == CLAN_REPO_SCHEMA);
  sql(db, "UPDATE clan_progression_schema SET version=6 WHERE singleton=1");
  assert(clan_repository_schema_check(db) == CLAN_REPO_OK);

  /* A different database with the same version still needs its own proof. */
  sql(db, "CREATE DATABASE cache_other");
  sql(db, "CREATE TABLE cache_other.clan_progression_schema (singleton INT, version INT)");
  sql(db, "INSERT INTO cache_other.clan_progression_schema VALUES (1,6)");
  assert(!mysql_select_db(db, "cache_other"));
  assert(clan_repository_schema_check(db) == CLAN_REPO_SCHEMA);
  assert(!mysql_select_db(db, "clan_test"));
  assert(clan_repository_schema_check(db) == CLAN_REPO_OK);

  /* Expiry detects metadata changes; failed proofs must not be cached. */
  sql(db, "DROP TRIGGER cp_state_history_no_delete");
  sleep(2); /* Test build uses a one-second cache TTL. */
  assert(clan_repository_schema_check(db) == CLAN_REPO_SCHEMA);
  assert(clan_repository_schema_check(db) == CLAN_REPO_SCHEMA);
  sql(db, trigger);
  assert(clan_repository_schema_check(db) == CLAN_REPO_OK);

  /* Explicit migration/pool invalidation forces immediate revalidation. */
  sql(db, "DROP TRIGGER cp_state_history_no_delete");
  clan_repository_schema_cache_invalidate(db);
  assert(clan_repository_schema_check(db) == CLAN_REPO_SCHEMA);
  sql(db, trigger);
  assert(clan_repository_schema_check(db) == CLAN_REPO_OK);

  /* Reusing the exact MYSQL storage must not reuse a prior connection's proof. */
  sql(db, "DROP TRIGGER cp_state_history_no_delete");
  previous_id = mysql_thread_id(db);
  mysql_close(db);
  connect_db(db, argv[1]);
  assert(mysql_thread_id(db) != previous_id);
  assert(clan_repository_schema_check(db) == CLAN_REPO_SCHEMA);
  sql(db, trigger);
  assert(clan_repository_schema_check(db) == CLAN_REPO_OK);

  /* Connection loss is reported rather than silently reconnecting. */
  admin = mysql_init(NULL); assert(admin);
  assert(mysql_real_connect(admin, NULL, "root", "", "clan_test", 0, argv[1], 0));
  assert(!mysql_kill(admin, mysql_thread_id(db)));
  assert(clan_repository_schema_check(db) == CLAN_REPO_UNAVAILABLE);
  mysql_close(admin); mysql_close(db);
  connect_db(db, argv[1]);
  assert(clan_repository_schema_check(db) == CLAN_REPO_OK);

  /* Disabling caching restores full offline validation behavior. */
  clan_repository_schema_cache_enable(false);
  sql(db, "DROP TRIGGER cp_state_history_no_delete");
  assert(clan_repository_schema_check(db) == CLAN_REPO_SCHEMA);
  sql(db, trigger); free(trigger);
  clan_repository_schema_cache_invalidate(NULL);
  mysql_close(db);
  puts("Schema cache: session, database, version, expiry, DDL, failure and reconnect checks passed.");
  return 0;
}
