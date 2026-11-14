#include "clan_repository.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

enum clan_repository_result clan_buff_activation_schema_check(MYSQL *);
enum clan_repository_result clan_buff_lifecycle_schema_check(MYSQL *);
bool clan_delivery_schema_ready(MYSQL *);
bool clan_objective_schema_ready(MYSQL *);
bool clan_expedition_schema_ready(MYSQL *);
bool clan_expedition_progress_ready(MYSQL *);
bool clan_qm_schema_ready(MYSQL *);
static int a_calls, b_calls;
static bool b_valid = true;
static enum clan_repository_result proof_a(MYSQL *db)
{ (void)db; ++a_calls; return CLAN_REPO_OK; }
static enum clan_repository_result proof_b(MYSQL *db)
{ (void)db; ++b_calls; return b_valid ? CLAN_REPO_OK : CLAN_REPO_SCHEMA; }
static void sql(MYSQL *db, const char *text)
{ if (mysql_query(db, text)) { fprintf(stderr, "%s\n", mysql_error(db)); abort(); } }
static unsigned long long questions(MYSQL *db)
{
  MYSQL_RES *res; MYSQL_ROW row; unsigned long long n;
  sql(db, "SHOW SESSION STATUS LIKE 'Questions'");
  res=mysql_store_result(db); assert(res); row=mysql_fetch_row(res); assert(row);
  n=strtoull(row[1],NULL,10);mysql_free_result(res);return n;
}
static void check_all(MYSQL *db)
{
  assert(clan_buff_activation_schema_check(db)==CLAN_REPO_OK);
  assert(clan_buff_lifecycle_schema_check(db)==CLAN_REPO_OK);
  assert(clan_delivery_schema_ready(db));
  assert(clan_objective_schema_ready(db));
  assert(clan_expedition_schema_ready(db));
  assert(clan_expedition_progress_ready(db));
  assert(clan_qm_schema_ready(db));
}
int main(int argc,char **argv)
{
  MYSQL *db=mysql_init(NULL);MYSQL_RES *res;MYSQL_ROW row;char *trigger;
  unsigned long long before,full,cached;
  assert(argc==2);assert(mysql_real_connect(db,NULL,"root","","clan_test",0,argv[1],0));
  sql(db,"SET SESSION sql_mode='STRICT_ALL_TABLES,NO_ENGINE_SUBSTITUTION'");
  before=questions(db);check_all(db);full=questions(db)-before;
  clan_repository_schema_cache_enable(true);check_all(db);
  before=questions(db);check_all(db);cached=questions(db)-before;
  assert(cached<15);assert(full>cached*50);
  printf("Seven optional checks: %llu full vs %llu cached SQL queries.\n",full,cached);

  /* Separate optional contracts must never borrow each other's proof. */
  assert(clan_repository_schema_extension_check(db,proof_a)==CLAN_REPO_OK);
  assert(clan_repository_schema_extension_check(db,proof_b)==CLAN_REPO_OK);
  assert(a_calls==1&&b_calls==1);
  assert(clan_repository_schema_extension_check(db,proof_a)==CLAN_REPO_OK);
  assert(clan_repository_schema_extension_check(db,proof_b)==CLAN_REPO_OK);
  assert(a_calls==1&&b_calls==1);
  clan_repository_schema_cache_invalidate(db);b_valid=false;
  assert(clan_repository_schema_extension_check(db,proof_a)==CLAN_REPO_OK);
  assert(clan_repository_schema_extension_check(db,proof_b)==CLAN_REPO_SCHEMA);
  assert(clan_repository_schema_extension_check(db,proof_b)==CLAN_REPO_SCHEMA);
  assert(a_calls==2&&b_calls==3);b_valid=true;
  assert(clan_repository_schema_extension_check(db,proof_b)==CLAN_REPO_OK);
  assert(b_calls==4);

  /* A real missing retention trigger must be detected on expiry. */
  check_all(db);
  sql(db,"SHOW CREATE TRIGGER cba_immutable");res=mysql_store_result(db);assert(res);
  row=mysql_fetch_row(res);assert(row&&row[2]);trigger=malloc(strlen(row[2])+1);assert(trigger);
  strcpy(trigger,row[2]);mysql_free_result(res);
  sql(db,"DROP TRIGGER cba_immutable");sleep(2);
  assert(clan_buff_activation_schema_check(db)==CLAN_REPO_SCHEMA);
  assert(clan_buff_lifecycle_schema_check(db)==CLAN_REPO_SCHEMA);
  sql(db,trigger);free(trigger);check_all(db);
  sql(db,"SET SESSION sql_mode=''");
  assert(clan_repository_schema_extension_check(db,proof_a)==CLAN_REPO_SCHEMA);
  assert(!clan_delivery_schema_ready(db));
  sql(db,"SET SESSION sql_mode='STRICT_ALL_TABLES,NO_ENGINE_SUBSTITUTION'");check_all(db);
  clan_repository_schema_cache_enable(false);
  sql(db,"DROP TRIGGER cba_immutable");
  assert(clan_buff_lifecycle_schema_check(db)==CLAN_REPO_SCHEMA);
  mysql_close(db);
  puts("Optional cache: independent proofs, failures, invalidation, expiry and strict-session checks passed.");
  return 0;
}
