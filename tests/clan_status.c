#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "clan.h"
#include "mysql.h"
#include "clan_commands.h"
#include "clan_status.h"
#include <json-c/json.h>
#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include "perfmon.h"
void PERF_prof_sect_init(struct PERF_prof_sect **p,const char *id){(void)id;*p=NULL;}
void PERF_prof_sect_enter(struct PERF_prof_sect *p){(void)p;}
void PERF_prof_sect_exit(struct PERF_prof_sect *p){(void)p;}
static MYSQL_POOL_CONN pool;
bool mysql_available=true;
static bool authority=true,ready=true;
static int acquires;
static char output[32768];
struct player_special_data dummy_mob;
bool clan_persistence_sql_authority(void){return authority;}
bool clan_persistence_ready(void){return ready;}
MYSQL_POOL_CONN *mysql_pool_acquire(void){acquires++;return &pool;}
void mysql_pool_release(MYSQL_POOL_CONN *p){assert(p==&pool);}
size_t send_to_char(struct char_data *ch,const char *format,...)
{
  size_t used=strlen(output);int n;va_list args;(void)ch;
  va_start(args,format);n=vsnprintf(output+used,sizeof(output)-used,format,args);va_end(args);
  assert(n>=0&&(size_t)n<sizeof(output)-used);return (size_t)n;
}
void basic_mud_log(const char *format,...){(void)format;assert(0);}
static struct json_object *get(struct json_object *o,const char *key)
{struct json_object *v=NULL;json_object_object_get_ex(o,key,&v);return v;}
static int64_t number(struct json_object *o,const char *key){return json_object_get_int64(get(o,key));}
int main(int argc,char **argv)
{
  char input[8192],rendered[16384]="UNCHANGED";
  size_t n;struct json_object *request,*out=json_object_new_object();
  enum clan_repository_result status=CLAN_REPO_OK;
  const char *uid,*args,*mode;
  assert(argc==2);n=fread(input,1,sizeof(input)-1,stdin);input[n]=0;request=json_tokener_parse(input);assert(request);
  pool.conn=mysql_init(NULL);assert(mysql_real_connect(pool.conn,NULL,"root","","clan_test",0,argv[1],0));
  assert(!mysql_query(pool.conn,"SET SESSION sql_mode='STRICT_ALL_TABLES,NO_ENGINE_SUBSTITUTION'"));
  uid=json_object_get_string(get(request,"uid"));mode=json_object_get_string(get(request,"mode"));
  args=json_object_get_string(get(request,"args"));if(!args)args="";
  if (!strcmp(mode,"command")||!strcmp(mode,"favor-command")) {
    struct char_data ch={0};struct descriptor_data desc={0};struct account_data account={0};
    GET_IDNUM(&ch)=number(request,"player");GET_LEVEL(&ch)=(int)number(request,"level");
    ch.desc=&desc;desc.character=&ch;desc.account=&account;account.id=(int)number(request,"account");
    authority=!json_object_get_boolean(get(request,"legacy"));ready=!json_object_get_boolean(get(request,"unready"));
    mysql_available=!json_object_get_boolean(get(request,"outage"));
    if (!strcmp(mode,"command"))clan_status_command(&ch,args,json_object_get_boolean(get(request,"inspect")),number(request,"now"));
    else do_clanfavor(&ch,args,0,0);
    json_object_object_add(out,"text",json_object_new_string(output));
    json_object_object_add(out,"acquires",json_object_new_int(acquires));
  } else if (!strcmp(mode,"history")) {
    struct json_object *history=NULL;
    status=clan_repository_favor_history(pool.conn,number(request,"player"),uid,&history);
    if(status==CLAN_REPO_OK)json_object_object_add(out,"history",history);
  } else {
    struct clan_status_report *report=malloc(sizeof(*report)),*old=malloc(sizeof(*old));
    bool rendered_ok=false;memset(report,0x5a,sizeof(*report));memcpy(old,report,sizeof(*old));
    status=clan_repository_status_read(pool.conn,number(request,"player"),number(request,"account"),uid,
        json_object_get_boolean(get(request,"inspect")),number(request,"now"),report);
    if(status==CLAN_REPO_OK) {
      size_t capacity=get(request,"capacity")?(size_t)number(request,"capacity"):sizeof(rendered);
      assert(capacity<=sizeof(rendered));rendered_ok=clan_status_render(&clan_progression_defaults,report,rendered,capacity);
      json_object_object_add(out,"full",json_object_new_boolean(report->full_clan_access));
      json_object_object_add(out,"account_present",json_object_new_boolean(report->account_present));
    } else assert(!memcmp(old,report,sizeof(*report)));
    json_object_object_add(out,"render_ok",json_object_new_boolean(rendered_ok));
    json_object_object_add(out,"text",json_object_new_string(rendered));free(report);free(old);
  }
  json_object_object_add(out,"result",json_object_new_string(clan_repository_result_name(status)));
  puts(json_object_to_json_string_ext(out,JSON_C_TO_STRING_PLAIN));
  json_object_put(out);json_object_put(request);mysql_close(pool.conn);return 0;
}
