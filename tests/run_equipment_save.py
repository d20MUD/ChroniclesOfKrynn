#!/usr/bin/env python3
"""Exercise production logout checkpoints against disposable MariaDB and files."""
from pathlib import Path
import os
import re
import shlex
import subprocess
import tempfile
import time

root = Path(__file__).resolve().parents[1]
source = (root / 'src/objsave.c').read_text()
def function(name, text=source):
    match = re.search(r'^(?:static )?(?:void|int|bool) ' + name + r'\([^;]*?\)\n\{', text, re.M)
    return text[match.start():text.index('\n}', match.start()) + 2] + '\n'

headers = ''.join(f'#include "{h}"\n' for h in ['conf.h', 'sysdep.h', 'structs.h',
    'utils.h', 'comm.h', 'handler.h', 'db.h', 'mysql.h', 'estate.h', 'durable_file.h'])
stubs = r'''
#include <assert.h>
#define OBJSAVE_DB 1
struct player_special_data dummy_mob;
MYSQL *conn;
bool mysql_available = TRUE;
extern int durable_file_fault;
static int fail_value, extracted, reject_extract, fail_commit, fail_begin;
static char messages[2048];
void basic_mud_log(const char *fmt, ...) {}
int MIN(int a, int b) { return a < b ? a : b; }
int get_filename(char *path, size_t size, int mode, const char *name) {
  snprintf(path, size, "player.objs"); return TRUE;
}
int estate_query(const char *fmt, ...) {
  char sql[4096]; va_list ap; va_start(ap,fmt); vsnprintf(sql,sizeof(sql),fmt,ap); va_end(ap);
  return !mysql_query(conn,sql);
}
char *estate_quote(const char *s) {
  char *result=malloc(strlen(s)*2+1); mysql_real_escape_string(conn,result,s,strlen(s)); return result;
}
size_t send_to_char(struct char_data *ch,const char *fmt,...) {
  va_list ap; va_start(ap,fmt); int n=vsnprintf(messages,sizeof(messages),fmt,ap); va_end(ap); return n;
}
/* The production tree traversal and checkpoint are under test. The small record
 * codec gives each object a stable ID and exposes weights/locations for assertions. */
int objsave_save_obj_record(struct obj_data *obj,struct char_data *ch,FILE *fp,int loc) {
  assert(!extracted);
  if(GET_OBJ_VAL(obj,0)==fail_value) return FALSE;
  fprintf(fp,"#%d\nLoc : %d\nWght: %d\n",GET_OBJ_VAL(obj,0),loc,GET_OBJ_WEIGHT(obj));
  return estate_query("INSERT INTO player_save_objs(name,value,loc,weight) VALUES('tester',%d,%d,%d)",
                      GET_OBJ_VAL(obj,0),loc,GET_OBJ_WEIGHT(obj));
}
void extract_obj(struct obj_data *obj) {
  assert(!reject_extract);
  if(obj->carried_by) {
    struct obj_data **p=&obj->carried_by->carrying;
    while(*p && *p!=obj)p=&(*p)->next_content;
    assert(*p==obj); *p=obj->next_content; obj->carried_by=NULL;
  }
  if(obj->worn_by) { GET_EQ(obj->worn_by,obj->worn_on)=NULL; obj->worn_by=NULL; }
  if(obj->in_obj) {
    struct obj_data **p=&obj->in_obj->contains;
    while(*p && *p!=obj)p=&(*p)->next_content;
    assert(*p==obj); *p=obj->next_content; obj->in_obj=NULL;
  }
  extracted++;
}
'''
# Intercept only transaction-control failures; record writes and rollbacks use MariaDB.
stubs += r'''
static int test_mysql_query(MYSQL *db, const char *query) {
  if (fail_commit && !strcasecmp(query,"COMMIT")) return 1;
  if (fail_begin && !strcasecmp(query,"START TRANSACTION")) return 1;
  return mysql_query(db,query);
}
#define mysql_query test_mysql_query
'''
code = ''.join(function(name) for name in ['Crash_is_unrentable', 'Crash_save',
    'Crash_save_rent', 'Crash_restore_weight', 'Crash_extract_objs', 'objsave_write_rentcode',
    'Crash_checkpoint', 'Crash_crashsave_checked', 'Crash_rentsave'])
code += r'''
struct config_data config_info;
static struct room_data rooms[1];
struct room_data *world=rooms;
room_rnum top_of_world=0;
static int extracted_chars;
bool return_all_eidolon_gear(struct char_data *ch) { return TRUE; }
void save_eidolon_descs(struct char_data *ch) {}
void save_char_pets(struct char_data *ch) {}
void dismiss_all_followers(struct char_data *ch) {}
void cancel_trade(struct char_data *ch,const char *reason) {}
void clear_trade_invites(struct char_data *ch,const char *reason) {}
void quest_timeout(struct char_data *ch,int index) {}
void mudlog(int type,int level,int file,const char *fmt,...) {}
size_t write_to_output(struct descriptor_data *d,const char *fmt,...) { return 0; }
const char *act(const char *s,int hide,struct char_data *ch,struct obj_data *obj,void *vict,int type) { return s; }
void extract_char(struct char_data *ch) {
  assert(!reject_extract && !GET_EQ(ch,0) && !ch->carrying); extracted_chars++;
}
'''
code += function('perform_player_quit', (root/'src/act.other.c').read_text())
checks = r'''
static long scalar(const char *sql) {
  MYSQL_RES *r; MYSQL_ROW row; long value;
  assert(!mysql_query(conn,sql)); r=mysql_store_result(conn); assert(r);
  row=mysql_fetch_row(r); assert(row && row[0]); value=atol(row[0]);mysql_free_result(r);return value;
}
static void sql(const char *s) { if(mysql_query(conn,s)) {fprintf(stderr,"%s\n",mysql_error(conn));assert(0);} }
static void old_save(void) {
  sql("DELETE FROM player_save_objs");sql("INSERT INTO player_save_objs(name,value,loc,weight) VALUES('tester',999,0,1)");
  sql("UPDATE player_data SET obj_save_header='old header'");
  FILE *fp=fopen("player.objs","w");assert(fp);fputs("old file\n",fp);assert(!fclose(fp));
}
static void old_snapshot(void) {
  assert(scalar("SELECT COUNT(*) FROM player_save_objs WHERE value=999")==1);
  assert(scalar("SELECT COUNT(*) FROM player_save_objs")==1);
  assert(scalar("SELECT COUNT(*) FROM player_data WHERE obj_save_header='old header'")==1);
}
static void old_file(void) {
  char buf[64];FILE *fp=fopen("player.objs","r");assert(fp);assert(fgets(buf,sizeof(buf),fp));fclose(fp);assert(!strcmp(buf,"old file\n"));
}
int main(int argc,char **argv) {
  struct char_data ch={0};struct player_special_data ps={0};struct bag_data bags={0};
  struct obj_data worn,loose,bagged[10],container,child,excluded,excluded_child;
  int failure, b;
  conn=mysql_init(NULL); assert(mysql_real_connect(conn,NULL,"root",NULL,"gear_test",0,argv[1],0));
  sql("CREATE TABLE player_data(name VARCHAR(20) PRIMARY KEY,obj_save_header TEXT) ENGINE=InnoDB");
  sql("INSERT INTO player_data VALUES('tester','old header')");
  sql("CREATE TABLE player_save_objs(name VARCHAR(20),value INT,loc INT,weight INT) ENGINE=InnoDB");
  ch.player_specials=&ps;ch.bags=&bags;ch.player.name="tester";
  /* Every worn, inventory, nested, and virtual-bag write can independently fail. */
  for(failure=0;failure<24;failure++) {
    memset(&worn,0,sizeof(worn));memset(&loose,0,sizeof(loose));memset(bagged,0,sizeof(bagged));
    memset(&container,0,sizeof(container));memset(&child,0,sizeof(child));
    memset(&excluded,0,sizeof(excluded));memset(&excluded_child,0,sizeof(excluded_child));
    worn.item_number=loose.item_number=NOTHING;
    worn.short_description=loose.short_description="valid custom gear";
    worn.worn_by=&ch;worn.worn_on=0;GET_EQ((&ch),0)=&worn;
    ch.carrying=&loose;loose.carried_by=&ch;loose.next_content=&container;container.carried_by=&ch;
    container.contains=&child;child.in_obj=&container;child.next_content=&excluded;
    excluded.in_obj=&container;excluded.contains=&excluded_child;excluded_child.in_obj=&excluded;
    SET_BIT_AR(GET_OBJ_EXTRA(&excluded),ITEM_NORENT);
    GET_OBJ_VAL(&worn,0)=1;GET_OBJ_VAL(&loose,0)=2;GET_OBJ_VAL(&container,0)=3;
    GET_OBJ_VAL(&child,0)=4;GET_OBJ_VAL(&excluded,0)=5;GET_OBJ_VAL(&excluded_child,0)=6;
    GET_OBJ_WEIGHT(&container)=11;GET_OBJ_WEIGHT(&child)=2;GET_OBJ_WEIGHT(&excluded)=4;GET_OBJ_WEIGHT(&excluded_child)=1;
    for(b=0;b<10;b++) {bagged[b].item_number=NOTHING;GET_OBJ_VAL(&bagged[b],0)=10+b;}
    bags.bag1=&bagged[0];bags.bag2=&bagged[1];bags.bag3=&bagged[2];bags.bag4=&bagged[3];bags.bag5=&bagged[4];
    bags.bag6=&bagged[5];bags.bag7=&bagged[6];bags.bag8=&bagged[7];bags.bag9=&bagged[8];bags.bag10=&bagged[9];
    extracted=0;fail_value=0;durable_file_fault=0;fail_commit=fail_begin=0;old_save();
    if(failure>=1 && failure<=4)fail_value=failure;
    if(failure>=5 && failure<=14)fail_value=failure+5;
    if(failure>=15 && failure<=19)durable_file_fault=failure-14;
    if(failure==20)sql("CREATE TRIGGER reject_save BEFORE INSERT ON player_save_objs FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='injected DB write failure'");
    if(failure==21)sql("CREATE TRIGGER reject_header BEFORE UPDATE ON player_data FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='injected header failure'");
    fail_commit=failure==22;fail_begin=failure==23;
    reject_extract=failure!=0;
    bool ok=Crash_rentsave(&ch,0);
    if(!failure) {
      assert(ok && extracted==6 && !GET_EQ((&ch),0) && !ch.carrying);
      assert(scalar("SELECT COUNT(*) FROM player_save_objs")==14);
      assert(scalar("SELECT COUNT(*) FROM player_save_objs WHERE value IN (1,2)")==2);
      assert(scalar("SELECT COUNT(*) FROM player_save_objs WHERE value IN (5,6)")==0);
      assert(scalar("SELECT weight FROM player_save_objs WHERE value=3")==5);
      assert(scalar("SELECT loc FROM player_save_objs WHERE value=1")==1);
      assert(scalar("SELECT loc FROM player_save_objs WHERE value=4")==-1);
    } else {
      assert(!ok && !extracted && GET_EQ((&ch),0)==&worn && ch.carrying==&loose);
      assert(container.contains==&child && child.next_content==&excluded && excluded.contains==&excluded_child);
      assert(GET_OBJ_WEIGHT(&container)==11 && GET_OBJ_WEIGHT(&excluded)==4);
      old_snapshot();if(failure!=19 && failure!=22)old_file(); /* Directory fsync can fail after a valid rename. */
      assert(strstr(messages,"Logout was cancelled"));
    }
    if(failure==20)sql("DROP TRIGGER reject_save");
    if(failure==21)sql("DROP TRIGGER reject_header");
  }
  fail_commit=fail_begin=0;fail_value=durable_file_fault=0;extracted=0;reject_extract=1;
  /* Explicit exclusions apply inside virtual bags and containers, while custom gear survives. */
  memset(&worn,0,sizeof(worn));memset(&loose,0,sizeof(loose));memset(&container,0,sizeof(container));
  worn.item_number=loose.item_number=container.item_number=NOTHING;
  GET_OBJ_VAL(&worn,0)=100;GET_OBJ_VAL(&loose,0)=101;GET_OBJ_VAL(&container,0)=102;
  GET_OBJ_TYPE(&worn)=ITEM_KEY;GET_OBJ_RENT(&loose)=-1;
  SET_BIT_AR(GET_OBJ_EXTRA(&container),ITEM_NORENT);
  memset(&bags,0,sizeof(bags));bags.bag1=&worn;bags.bag2=&loose;bags.bag3=&container;
  ch.carrying=NULL;GET_EQ((&ch),0)=NULL;
  assert(Crash_checkpoint(&ch,RENT_RENTED,0,TRUE));
  assert(scalar("SELECT COUNT(*) FROM player_save_objs")==0);
  /* Missing virtual bags are legal. Copyover/crash checkpoints never extract. */
  ch.bags=NULL;ch.carrying=NULL;GET_EQ((&ch),0)=NULL;
  fail_value=durable_file_fault=0;extracted=0;reject_extract=1;
  assert(Crash_crashsave_checked(&ch));assert(!extracted);
  /* A failed real logout must retain both the connection and the live gear. */
  struct descriptor_data descriptor={0};
  ch.desc=&descriptor;descriptor.character=&ch;STATE(&descriptor)=CON_PLAYING;
  CONFIG_FREE_RENT=1;IN_ROOM(&ch)=0;
  memset(&worn,0,sizeof(worn));worn.item_number=NOTHING;GET_OBJ_VAL(&worn,0)=1;
  GET_EQ((&ch),0)=&worn;worn.worn_by=&ch;worn.worn_on=0;
  fail_value=1;extracted=extracted_chars=0;reject_extract=1;
  perform_player_quit(&ch);
  assert(!extracted_chars && !extracted && ch.desc==&descriptor && STATE(&descriptor)==CON_PLAYING);
  assert(GET_EQ((&ch),0)==&worn);
  fail_value=0;reject_extract=0;
  perform_player_quit(&ch);
  assert(extracted_chars==1 && extracted==1);
  mysql_close(conn);
  puts("Equipment save: custom worn/inventory/bag gear, all ten bag failures, nested weights, DB/header rollback, durable-file failures, extraction ordering, and failed/successful logout dispatch passed.");
  return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='equipment-save-') as directory:
    temp=Path(directory);data=temp/'data';data.mkdir();socket=temp/'mysql.sock'
    path=temp/'check.c';path.write_text(headers+stubs+code+checks)
    flags=shlex.split(subprocess.check_output(['pkg-config','--cflags','--libs','mariadb'],text=True))
    binary=temp/'check'
    subprocess.run([os.environ.get('CC','cc'),'-std=gnu99','-Wall','-Wextra','-Wno-unused-parameter',
        '-DLEGENDARY_TRANSACTION_TEST','-I'+str(root/'src'),str(path),str(root/'src/durable_file.c'),
        *flags,'-o',str(binary)],check=True)
    subprocess.run(['mariadb-install-db','--no-defaults','--datadir='+str(data),
        '--auth-root-authentication-method=normal','--skip-test-db'],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL,check=True)
    log=temp/'mysql.log'
    server=subprocess.Popen(['mariadbd','--no-defaults','--datadir='+str(data),'--socket='+str(socket),
        '--pid-file='+str(temp/'mysql.pid'),'--skip-networking','--log-error='+str(log),
        '--user='+os.environ.get('USER','krynn')],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
    try:
        client=['mariadb','--no-defaults','--socket='+str(socket),'-uroot']
        for attempt in range(100):
            if subprocess.run([*client,'-e','SELECT 1'],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL).returncode==0:
                break
            if server.poll() is not None: raise RuntimeError(log.read_text())
            time.sleep(.1)
        else: raise RuntimeError('Temporary MariaDB did not become ready')
        subprocess.run([*client,'-e','CREATE DATABASE gear_test'],check=True)
        subprocess.run([str(binary),str(socket)],cwd=temp,check=True)
    finally:
        server.terminate();server.wait(timeout=15)
