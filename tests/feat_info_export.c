/* Run only against the disposable database created by run_feat_info_export.py. */
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "feats.h"
#include "mysql.h"
#include <assert.h>

bool mysql_available = TRUE;
static MYSQL_POOL_CONN test_connection;
static int acquired;

MYSQL_POOL_CONN *mysql_pool_acquire(void)
{
  assert(!acquired);
  acquired = 1;
  return &test_connection;
}
void mysql_pool_release(MYSQL_POOL_CONN *pc)
{
  assert(pc == &test_connection && acquired);
  acquired = 0;
}
void basic_mud_log(const char *format, ...)
{
  va_list ap;
  va_start(ap, format);
  vfprintf(stderr, format, ap);
  fputc('\n', stderr);
  va_end(ap);
}
static void query(const char *sql)
{
  if (mysql_query(test_connection.conn, sql))
  {
    fprintf(stderr, "%s\n", mysql_error(test_connection.conn));
    abort();
  }
}
static void same_text(const char *actual, const char *expected)
{
  assert((actual == NULL) == (expected == NULL));
  if (expected)
    assert(strcmp(actual, expected) == 0);
}
static void verify_snapshot(void)
{
  MYSQL_RES *result, *prerequisites;
  MYSQL_ROW row, pr;
  int i, order;
  const struct feat_prerequisite *p;
  query("SELECT feat_id,name,in_game,can_learn,can_stack,feat_type,short_description,"
        "description,epic,combat_feat,event,teamwork_feat FROM feat_info ORDER BY feat_id");
  result = mysql_store_result(test_connection.conn);
  assert(result && mysql_num_rows(result) == NUM_FEATS);
  query("SELECT feat_id,prerequisite_order,prerequisite_type,description,value_0,value_1,value_2 "
        "FROM feat_info_prerequisites ORDER BY feat_id,prerequisite_order");
  prerequisites = mysql_store_result(test_connection.conn);
  assert(prerequisites);
  for (i = 0; i < NUM_FEATS; i++)
  {
    const struct feat_info *f = &feat_list[i];
    row = mysql_fetch_row(result);
    assert(row && atoi(row[0]) == i);
    same_text(row[1], f->name);
    assert(atoi(row[2]) == f->in_game);
    assert(atoi(row[3]) == f->can_learn);
    assert(atoi(row[4]) == f->can_stack);
    assert(atoi(row[5]) == f->feat_type);
    same_text(row[6], f->short_description);
    same_text(row[7], f->description);
    assert(atoi(row[8]) == f->epic);
    assert(atoi(row[9]) == f->combat_feat);
    assert(atoi(row[10]) == f->event);
    assert(atoi(row[11]) == f->teamwork_feat);
    for (p = f->prerequisite_list, order = 0; p; p = p->next, order++)
    {
      pr = mysql_fetch_row(prerequisites);
      assert(pr && atoi(pr[0]) == i && atoi(pr[1]) == order);
      assert(atoi(pr[2]) == p->prerequisite_type);
      same_text(pr[3], p->description);
      assert(atoi(pr[4]) == p->values[0]);
      assert(atoi(pr[5]) == p->values[1]);
      assert(atoi(pr[6]) == p->values[2]);
    }
  }
  assert(!mysql_fetch_row(prerequisites));
  mysql_free_result(result);
  mysql_free_result(prerequisites);
}
int main(int argc, char **argv)
{
  char *long_description;
  const char *original_name;
  struct feat_prerequisite second = {7, NULL, {-1, 2, 3}, NULL};
  struct feat_prerequisite first = {3, "O'Brien \\ test\r\n", {4, -5, 6}, &second};
  assert(argc == 2);
  test_connection.conn = mysql_init(NULL);
  assert(mysql_real_connect(test_connection.conn, "localhost", "root", "", "feat_export_test",
                            0, argv[1], 0));
  assert(!mysql_set_character_set(test_connection.conn, "utf8mb4"));
  assign_feats();
  assert(sync_feat_info());
  verify_snapshot();
  /* Full text, SQL-sensitive characters, UTF-8, NULLs and prerequisite order. */
  long_description = malloc(100001);
  assert(long_description);
  memset(long_description, 'x', 100000);
  long_description[100000] = '\0';
  feat_list[0].name = "O'Brien \\ magic \tR\r\n\xc3\xa9";
  feat_list[0].short_description = NULL;
  feat_list[0].description = long_description;
  feat_list[0].teamwork_feat = TRUE;
  feat_list[0].prerequisite_list = &first;
  assert(sync_feat_info());
  verify_snapshot();
  query("INSERT INTO feat_info SELECT 999999,name,in_game,can_learn,can_stack,feat_type,"
        "short_description,description,epic,combat_feat,event,teamwork_feat FROM feat_info WHERE feat_id=0");
  query("INSERT INTO feat_info_prerequisites VALUES (999999,0,1,'stale',1,2,3)");
  assert(sync_feat_info());
  verify_snapshot();
  /* Fail after partial insertion; both tables must retain their previous contents. */
  query("CREATE TRIGGER fail_feat_export BEFORE INSERT ON feat_info FOR EACH ROW "
        "BEGIN IF NEW.feat_id=10 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='test failure'; END IF; END");
  original_name = feat_list[0].name;
  feat_list[0].name = "must roll back";
  assert(!sync_feat_info());
  feat_list[0].name = original_name;
  verify_snapshot();
  query("DROP TRIGGER fail_feat_export");
  assert(sync_feat_info());
  verify_snapshot();
  mysql_available = FALSE;
  assert(!sync_feat_info() && !acquired);
  mysql_close(test_connection.conn);
  free(long_description);
  puts("Feat info export: all fields, prerequisites, refresh, rollback and offline checks passed.");
  return 0;
}
