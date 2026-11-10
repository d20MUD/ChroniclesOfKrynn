/* Real Phase 7 module/repository, exercised by private SQL fixture. */
#include "clan_repository.h"
#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static MYSQL *connect_db(const char *socket)
{
  MYSQL *db=mysql_init(NULL);
  assert(db && mysql_real_connect(db,NULL,"root","","clan_test",0,socket,0));
  assert(!mysql_query(db,"SET SESSION sql_mode='STRICT_ALL_TABLES,NO_ENGINE_SUBSTITUTION'"));
  return db;
}
static void sql(MYSQL *db,const char *query){assert(!mysql_query(db,query));}
static void same(const struct clan_account_window *a,const struct clan_account_window *b)
{
  assert(a->account_id==b->account_id && a->starts_at==b->starts_at &&
    a->base_xp_eighths==b->base_xp_eighths && a->favor_eighths==b->favor_eighths &&
    a->revision==b->revision && !memcmp(a->claims,b->claims,sizeof(a->claims)));
}
static void periods(void)
{
  int64_t stamp,week,day;
  while(scanf("%" SCNd64 " %" SCNd64 " %" SCNd64,&stamp,&week,&day)==3) {
    struct clan_reset_period w,d,before={123,456};
    assert(clan_progression_week_at(stamp,&w));assert(clan_progression_day_at(stamp,&d));
    assert(w.starts_at==week && w.ends_at==week+604800 && stamp>=w.starts_at && stamp<w.ends_at);
    assert(d.starts_at==day && d.ends_at==day+86400 && stamp>=d.starts_at && stamp<d.ends_at);
    assert(!clan_progression_week_at(-1,&before) && before.starts_at==123 && before.ends_at==456);
    assert(!clan_progression_day_at(INT64_MAX,&before) && before.starts_at==123);
    assert(!clan_progression_week_at(INT64_MAX,&before) && before.starts_at==123);
  }
}
int main(int argc,char **argv)
{
  MYSQL *db,*alt;
  struct clan_account_window a,b,loaded,old,fresh,untouched={0};
  struct clan_reset_period period;
  int64_t t=INT64_C(1704067200); /* Monday 2024-01-01 00:00 UTC */
  size_t i;
  if(argc==2 && !strcmp(argv[1],"periods")){periods();puts("UTC weekly/daily boundaries passed");return 0;}
  assert(argc==2);db=connect_db(argv[1]);alt=connect_db(argv[1]);
  assert(clan_progression_account_window_init(INT64_C(5000000001),t,&a));
  assert(clan_progression_account_window_valid(&a));
  untouched.account_id=123;
  assert(clan_repository_account_window_load(db,a.account_id,t,&untouched)==CLAN_REPO_NOT_FOUND && untouched.account_id==123);
  assert(clan_repository_account_window_save(db,&a,-1,t)==CLAN_REPO_CONFLICT); /* legacy authority */
  sql(db,"UPDATE clan_store_control SET store_mode=2");
  a.base_xp_eighths=4401;a.favor_eighths=2201;
  for(i=1;i<=6;i++)a.claims[i]=(int64_t)i;
  assert(clan_repository_account_window_save(db,&a,-1,t)==CLAN_REPO_OK);
  assert(clan_repository_account_window_save(alt,&a,-1,t)==CLAN_REPO_CONFLICT);
  assert(clan_repository_account_window_load(alt,a.account_id,t+1234,&b)==CLAN_REPO_OK);same(&a,&b);
  /* Two characters/clans use the exact same account key, not a new budget. */
  b.base_xp_eighths+=799;b.favor_eighths+=399;b.claims[CLAN_SOURCE_SUPPLY_ORDER]++;
  assert(clan_repository_account_window_save(alt,&b,0,t+1)==CLAN_REPO_OK);
  a.base_xp_eighths++;assert(clan_repository_account_window_save(db,&a,0,t+1)==CLAN_REPO_CONFLICT);
  assert(clan_repository_account_window_load(db,a.account_id,t,&loaded)==CLAN_REPO_OK);
  assert(loaded.base_xp_eighths==5200 && loaded.favor_eighths==2600 && loaded.revision==1);
  old=loaded;
  /* Counters and balances may never go backward inside a retained window. */
  loaded.base_xp_eighths--;assert(clan_repository_account_window_save(db,&loaded,1,t+2)==CLAN_REPO_CONFLICT);loaded=old;
  loaded.favor_eighths--;{
    enum clan_repository_result actual=clan_repository_account_window_save(db,&loaded,1,t+2);
    if(actual!=CLAN_REPO_CONFLICT)fprintf(stderr,"Favor decrease result %s, mysql error %u %s, server status %u\n",clan_repository_result_name(actual),mysql_errno(db),mysql_error(db),db->server_status);
    assert(actual==CLAN_REPO_CONFLICT);
  }loaded=old;
  for(i=1;i<=6;i++){loaded.claims[i]--;assert(clan_repository_account_window_save(db,&loaded,1,t+2)==CLAN_REPO_CONFLICT);loaded=old;}
  assert(clan_repository_account_window_save(db,&loaded,1,t)==CLAN_REPO_CONFLICT);
  sql(db,"CREATE TRIGGER fail_account_write BEFORE UPDATE ON clan_account_windows FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='injected failure'");
  loaded.base_xp_eighths++;assert(clan_repository_account_window_save(db,&loaded,1,t+2)==CLAN_REPO_INVALID);
  sql(db,"DROP TRIGGER fail_account_write");
  assert(clan_repository_account_window_load(db,a.account_id,t,&loaded)==CLAN_REPO_OK);same(&old,&loaded);
  /* Reconnect/restart preserves account usage; no login writer/reset exists. */
  mysql_close(db);db=connect_db(argv[1]);
  assert(clan_repository_account_window_load(db,a.account_id,t+604799,&loaded)==CLAN_REPO_OK);same(&old,&loaded);
  assert(clan_repository_account_window_load(db,a.account_id,t+604800,&untouched)==CLAN_REPO_NOT_FOUND && untouched.account_id==123);
  assert(clan_progression_account_window_init(a.account_id,t+604800,&fresh));
  assert(fresh.starts_at==t+604800 && fresh.base_xp_eighths==0 && fresh.favor_eighths==0);
  assert(clan_repository_account_window_save(db,&fresh,-1,t+604800)==CLAN_REPO_OK);
  assert(clan_repository_account_window_load(db,a.account_id,t,&loaded)==CLAN_REPO_OK);same(&old,&loaded);
  /* A delayed operation keeps its occurrence week even when written next week. */
  loaded.base_xp_eighths++;loaded.claims[CLAN_SOURCE_KILL]++;
  assert(clan_repository_account_window_save(db,&loaded,1,t+604801)==CLAN_REPO_OK);
  assert(clan_repository_account_window_load(db,a.account_id,t+604800,&fresh)==CLAN_REPO_OK && fresh.base_xp_eighths==0);
  assert(clan_repository_account_window_load(db,a.account_id,t,&old)==CLAN_REPO_OK && old.base_xp_eighths==5201);
  /* Another account, even for the same week, is independent. */
  assert(clan_progression_account_window_init(INT64_MAX,t,&b));b.base_xp_eighths=INT64_MAX;b.favor_eighths=INT64_MAX;
  for(i=1;i<=6;i++)b.claims[i]=INT64_MAX;
  assert(clan_repository_account_window_save(db,&b,-1,t)==CLAN_REPO_OK);
  assert(clan_repository_account_window_load(alt,INT64_MAX,t,&loaded)==CLAN_REPO_OK);same(&b,&loaded);
  /* Invalid identity/time/range data never touches output or storage. */
  assert(!clan_progression_account_window_init(0,t,&untouched) && untouched.account_id==123);
  assert(clan_repository_account_window_load(db,0,t,&untouched)==CLAN_REPO_INVALID && untouched.account_id==123);
  b=old;b.starts_at++;assert(clan_repository_account_window_save(db,&b,b.revision,t+604802)==CLAN_REPO_INVALID);
  b=old;b.claims[0]=1;assert(clan_repository_account_window_save(db,&b,b.revision,t+604802)==CLAN_REPO_INVALID);
  b=old;b.claims[1]=-1;assert(clan_repository_account_window_save(db,&b,b.revision,t+604802)==CLAN_REPO_INVALID);
  b=old;b.revision=INT64_MAX;assert(clan_repository_account_window_save(db,&b,INT64_MAX,t+604802)==CLAN_REPO_INVALID);
  assert(clan_repository_account_window_load(NULL,a.account_id,t,&untouched)==CLAN_REPO_UNAVAILABLE && untouched.account_id==123);
  /* An owned save cannot accidentally commit an unrelated caller transaction. */
  sql(db,"START TRANSACTION");assert(clan_repository_account_window_save(db,&old,old.revision,t+604802)==CLAN_REPO_INVALID);sql(db,"ROLLBACK");
  /* Detect corrupt stored values even if an external SQL client bypassed a
   * CHECK. Keep the deliberate corruption inside a rolled-back fixture. */
  sql(db,"SET SESSION check_constraint_checks=OFF");sql(db,"START TRANSACTION");
  sql(db,"INSERT INTO clan_account_windows(account_id,starts_at_utc,base_xp_eighths,updated_at_utc) VALUES(42,1704067200,-1,1704067200)");
  assert(clan_repository_account_window_load(db,42,t,&untouched)==CLAN_REPO_SCHEMA && untouched.account_id==123);
  sql(db,"ROLLBACK");sql(db,"SET SESSION check_constraint_checks=ON");
  assert(mysql_query(db,"DELETE FROM clan_account_windows")!=0);
  assert(mysql_query(db,"UPDATE clan_account_windows SET starts_at_utc=starts_at_utc+604800,revision=revision+1 WHERE account_id=5000000001 AND starts_at_utc=1704067200")!=0);
  assert(mysql_query(db,"UPDATE clan_account_windows SET account_id=123,revision=revision+1 WHERE account_id=5000000001")!=0);
  /* Epoch's initial partial week has a mathematically correct negative start. */
  assert(clan_progression_week_at(0,&period) && period.starts_at==-259200 && period.ends_at==345600);
  assert(clan_progression_account_window_init(99,0,&b));assert(clan_repository_account_window_save(db,&b,-1,0)==CLAN_REPO_OK);
  mysql_close(alt);mysql_close(db);
  puts("Account windows: shared alt budget, independent accounts/weeks, monotonic CAS, retries, rollback, reconnect, delayed occurrence, int64 and retained history passed");
  return 0;
}
