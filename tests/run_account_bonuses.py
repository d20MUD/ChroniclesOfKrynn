#!/usr/bin/env python3
"""Exercise account reward purchases and persistence against disposable MariaDB."""
from pathlib import Path
import os
import shlex
import subprocess
import tempfile
import time

root=Path(__file__).resolve().parents[1]
source=(root/'src/account.c').read_text()
def function(text, signature):
    start=text.index(signature+'\n{')
    return text[start:text.index('\n}',start)+2]+'\n'
a=source.index('static const char *account_bonus_names')
b=source.index('int get_account_reward_bonus(',a)
headers='\n'.join('#include "'+name+'"' for name in
                  ['conf.h','sysdep.h','structs.h','utils.h','mysql.h','db.h','comm.h','account.h','treasure.h'])
stubs=r'''
#include <assert.h>
#include <limits.h>
MYSQL *conn;
bool mysql_available=true;
struct descriptor_data *descriptor_list;
static char output[64000];
struct happyhour happy_data;
static int treasure_roll, magic_items;
static void award_random_crafting_treasure(struct char_data *ch,struct char_data *mob) {}
#undef IS_STAFF_EVENT
#define IS_STAFF_EVENT 0
int dice(int number,int size) { return size==100 ? treasure_roll : size==3 ? 1 : 10; }
void check_event_drops(struct char_data *ch,struct char_data *mob) {}
int quick_grade_check(int level) { return GRADE_MUNDANE; }
void award_magic_item(int number,struct char_data *ch,int grade,int level) { magic_items+=number; }
const char *act(const char *str,int hide,struct char_data *ch,struct obj_data *obj,void *vict,int type) { return str; }
int MIN(int a,int b) { return a<b?a:b; }
int MAX(int a,int b) { return a>b?a:b; }
size_t send_to_char(struct char_data *ch,const char *fmt,...) {
  va_list ap; va_start(ap,fmt); size_t used=strlen(output); int n=vsnprintf(output+used,sizeof(output)-used,fmt,ap); va_end(ap); return n;
}
void basic_mud_log(const char *fmt,...) {}
int is_abbrev(const char *a,const char *b) { return *a && !strncasecmp(a,b,strlen(a)); }
bool ensure_mysql_connection(MYSQL *c,const char *caller) { return !mysql_ping(c); }
int mysql_query_safe(MYSQL *c,const char *q) { return mysql_query(c,q); }
MYSQL_RES *mysql_store_result_safe(MYSQL *c) { return mysql_store_result(c); }
void load_account_characters(struct account_data *a) {}
void load_account_unlocks(struct account_data *a) {}
bool table_exists(const char *table) { return true; }
static void sql(const char *q) {
  if(mysql_query(conn,q)) { fprintf(stderr,"%s: %s\n",q,mysql_error(conn)); abort(); }
}
'''
code=source[a:b]+''.join(function(source,s) for s in [
    'int get_account_reward_bonus(struct char_data *ch, int bonus)',
    'int apply_account_reward_bonus(struct char_data *ch, int amount, int bonus)',
    'static void show_account_reward_bonuses(struct char_data *ch, struct account_data *account)',
    'static void purchase_account_reward_bonus(struct char_data *ch, const char *name)',
    'int load_account(char *name, struct account_data *account)',
    'void save_account(struct account_data *account)'])
code+=function((root/'src/db_init_data.c').read_text(),'void ensure_account_reward_bonus_columns(void)')
code+=function((root/'src/treasure.c').read_text(),'void determine_treasure(struct char_data *ch, struct char_data *mob)')

checks=r'''
static void free_account(struct account_data *account) {
  free(account->name); free(account->email);
}
int main(int argc,char **argv) {
  struct account_data account={0}, alt_account={0}, other_account={0}, reloaded={0};
  struct descriptor_data desc={0}, alt_desc={0}, other_desc={0};
  struct char_data ch={0}, alt={0}, other={0}, offline={0};
  int balance, rank;
  assert(argc==2);
  conn=mysql_init(NULL);
  assert(mysql_real_connect(conn,NULL,"root",NULL,"account_bonus_test",0,argv[1],0));
  sql("CREATE TABLE account_data (id INT PRIMARY KEY AUTO_INCREMENT, name VARCHAR(50) UNIQUE, password VARCHAR(64), experience INT DEFAULT 0, email VARCHAR(255), quit_survey_completed TINYINT DEFAULT NULL)");
  sql("CREATE TABLE unlocked_classes (account_id INT, class_id INT, UNIQUE(account_id,class_id))");
  sql("CREATE TABLE unlocked_races (account_id INT, race_id INT, UNIQUE(account_id,race_id))");
  ensure_account_reward_bonus_columns(); ensure_account_reward_bonus_columns();
  sql("INSERT INTO account_data (name,password,experience) VALUES ('bonustest','test',100000),('other','test',4000)");
  assert(!load_account("bonustest",&account)); assert(!load_account("bonustest",&alt_account));
  assert(!load_account("other",&other_account));
  desc.account=&account; desc.character=&ch; ch.desc=&desc; desc.next=&alt_desc;
  alt_desc.account=&alt_account; alt_desc.character=&alt; alt.desc=&alt_desc; alt_desc.next=&other_desc;
  other_desc.account=&other_account; other_desc.character=&other; other.desc=&other_desc;
  descriptor_list=&desc;
  assert(!get_account_reward_bonus(NULL,0)); assert(!get_account_reward_bonus(&offline,0));
  assert(!get_account_reward_bonus(&ch,-1)); assert(!get_account_reward_bonus(&ch,4));
  purchase_account_reward_bonus(&ch,""); assert(strstr(output,"2000") && strstr(output,"10%"));
  purchase_account_reward_bonus(&ch,"unknown"); assert(account.experience==100000);
  for(int type=0;type<NUM_ACCOUNT_REWARD_BONUSES;type++) {
    for(rank=1;rank<=ACCOUNT_REWARD_BONUS_CAP;rank++) {
      balance=account.experience;
      purchase_account_reward_bonus(rank%2 ? &ch : &alt,account_bonus_names[type]);
      assert(account.experience==balance-2000 && alt_account.experience==account.experience);
      assert(account.reward_bonus_pct[type]==rank && alt_account.reward_bonus_pct[type]==rank);
      assert(other_account.experience==4000 && !other_account.reward_bonus_pct[type]);
      assert(apply_account_reward_bonus(&alt,1000,type)==1000+rank*10);
      assert(strstr(output,"Purchased +1%"));
    }
    balance=account.experience;
    purchase_account_reward_bonus(&ch,account_bonus_names[type]);
    assert(account.experience==balance && account.reward_bonus_pct[type]==10);
  }
  assert(account.experience==20000);
  /* The actual treasure reward path uses percentage points and boosted coins. */
  {
    struct char_data mob={0};
    SET_BIT_AR(MOB_FLAGS(&mob),MOB_ISNPC); GET_LEVEL(&mob)=20;
#if defined(CAMPAIGN_DL)
    int coin_reward=550;
#else
    int coin_reward=110;
#endif
    HAPPY_TREASURE=0; treasure_roll=15; magic_items=0;
    determine_treasure(&ch,&mob);
    assert(GET_GOLD(&ch)==coin_reward && magic_items==1); /* base 5% + account 10% */
    treasure_roll=16; determine_treasure(&ch,&mob); assert(GET_GOLD(&ch)==coin_reward);
    treasure_roll=6; determine_treasure(&other,&mob); assert(GET_GOLD(&other)==0);
    HAPPY_TREASURE=25; treasure_roll=35;
    determine_treasure(&ch,&mob); assert(GET_GOLD(&ch)==coin_reward*2);
    treasure_roll=36; determine_treasure(&ch,&mob); assert(GET_GOLD(&ch)==coin_reward*2);
    HAPPY_TREASURE=100; treasure_roll=100;
    determine_treasure(&ch,&mob); assert(GET_GOLD(&ch)==coin_reward*3);
  }

  assert(apply_account_reward_bonus(&ch,INT_MAX,ACCOUNT_BONUS_EXPERIENCE)==INT_MAX);
  assert(apply_account_reward_bonus(&ch,0,ACCOUNT_BONUS_GOLD)==0);
  assert(apply_account_reward_bonus(&ch,-100,ACCOUNT_BONUS_GOLD)==-100);
  assert(apply_account_reward_bonus(&ch,19,ACCOUNT_BONUS_GOLD)==20);
  assert(apply_account_reward_bonus(&ch,1500,ACCOUNT_BONUS_EXPERIENCE)==1650); /* after +50% happyhour */
  SET_BIT_AR(MOB_FLAGS(&ch),MOB_ISNPC);
  assert(!get_account_reward_bonus(&ch,ACCOUNT_BONUS_GOLD));
  assert(apply_account_reward_bonus(&ch,1000,ACCOUNT_BONUS_GOLD)==1000);
  REMOVE_BIT_AR(MOB_FLAGS(&ch),MOB_ISNPC);
  save_account(&account);
  assert(!load_account("bonustest",&reloaded));
  assert(reloaded.experience==20000);
  for(int i=0;i<4;i++) assert(reloaded.reward_bonus_pct[i]==10);
  purchase_account_reward_bonus(&other,"qp"); assert(other_account.experience==2000);
  purchase_account_reward_bonus(&other,"xp"); assert(other_account.experience==0);
  purchase_account_reward_bonus(&other,"gold"); assert(other_account.reward_bonus_pct[2]==0);
  /* Database failure cannot charge the account or grant a cached upgrade. */
  sql("UPDATE account_data SET experience=4000 WHERE name='other'");
  other_account.experience=4000;
  sql("CREATE TRIGGER reject_bonus BEFORE UPDATE ON account_data FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='test rejection'");
  purchase_account_reward_bonus(&other,"gold");
  assert(other_account.experience==4000 && other_account.reward_bonus_pct[2]==0);
  sql("DROP TRIGGER reject_bonus");
  mysql_available=false;
  purchase_account_reward_bonus(&other,"gold"); assert(other_account.experience==4000);
  mysql_available=true;
  /* A stale cached balance cannot bypass the SQL affordability condition. */
  sql("UPDATE account_data SET experience=0 WHERE name='other'");
  purchase_account_reward_bonus(&other,"gold"); assert(other_account.reward_bonus_pct[2]==0);
  free_account(&account); free_account(&alt_account); free_account(&other_account); free_account(&reloaded);
  mysql_close(conn);
  puts("Account rewards: atomic purchases, all four caps, persistence, alt synchronization, rounding, saturation, and failure paths passed.");
  return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='account-bonuses-') as tmp:
    temp=Path(tmp); data=temp/'data'; data.mkdir(); socket=temp/'mysql.sock'; log=temp/'mysql.log'
    path=temp/'check.c'; path.write_text(headers+stubs+code+checks)
    flags=shlex.split(subprocess.check_output(['pkg-config','--cflags','--libs','mariadb'],text=True))
    binary=temp/'check'
    subprocess.run([os.environ.get('CC','cc'),'-std=gnu99','-Wall','-Wextra','-Wno-unused-parameter',
                    '-I'+str(root/'src'),str(path),*flags,'-o',str(binary)],check=True)
    subprocess.run(['mariadb-install-db','--no-defaults','--datadir='+str(data),
                    '--auth-root-authentication-method=normal','--skip-test-db'],
                   stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL,check=True)
    server=subprocess.Popen(['mariadbd','--no-defaults','--datadir='+str(data),'--socket='+str(socket),
                             '--pid-file='+str(temp/'pid'),'--skip-networking','--log-error='+str(log),
                             '--user='+os.environ.get('USER','krynn')],
                            stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
    try:
        client=['mariadb','--no-defaults','--socket='+str(socket),'-uroot']
        for attempt in range(100):
            ready=subprocess.run([*client,'-e','SELECT 1'],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
            if ready.returncode==0: break
            if server.poll() is not None: raise RuntimeError(log.read_text())
            time.sleep(.1)
        else: raise RuntimeError('Temporary MariaDB did not become ready')
        subprocess.run([*client,'-e','CREATE DATABASE account_bonus_test'],check=True)
        subprocess.run([str(binary),str(socket)],check=True)
    finally:
        server.terminate(); server.wait(timeout=15)
