#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "mysql.h"
#include "clan_sources.h"
#include "clan_buff_runtime.h"
#include "clan_persistence.h"
#include "clan_repository.h"
#include <assert.h>
#include <json-c/json.h>
static struct clan_progression_config policy;
static MYSQL_POOL_CONN connection;
static bool busy,save_ok=true;
static unsigned sequence;
static int saves;
static struct crafting_data_info checkpoint;
static bool captured;
static int64_t clock_value=2200172801;
struct char_data *character_list;
struct player_special_data dummy_mob;
struct zone_data *zone_table;
struct room_data *world;
room_rnum top_of_world;
size_t strlcpy(char *dst,const char *src,size_t size){size_t n=strlen(src);if(size){size_t take=n<size-1?n:size-1;memcpy(dst,src,take);dst[take]=0;}return n;}
struct index_data *mob_index;
mob_rnum top_of_mobt;
bool mysql_available=true;
time_t clan_source_test_time(time_t *out){if(out)*out=clock_value;return clock_value;}
int MIN(int a,int b){return a<b?a:b;}
int MAX(int a,int b){return a>b?a:b;}
const struct clan_progression_config *clan_buff_runtime_policy(void){return &policy;}
bool clan_persistence_sql_authority(void){return true;}
bool clan_persistence_ready(void){return true;}
bool clan_persistence_refresh_progression(void){assert(!busy);return true;}
MYSQL_POOL_CONN *mysql_pool_acquire(void){assert(!busy);busy=true;return &connection;}
void mysql_pool_release(MYSQL_POOL_CONN *c){assert(c==&connection&&busy);busy=false;}
bool legendary_uuid_generate(char out[33]){snprintf(out,33,"%032x",++sequence);return true;}
bool save_char_checked(struct char_data *ch,int mode){
  (void)mode;assert(!busy);saves++;
  if(save_ok&&!captured&&GET_CRAFT(ch).clan_supply_event[0]){checkpoint=GET_CRAFT(ch);captured=true;}
  return save_ok;
}
void save_char(struct char_data *ch,int mode){(void)save_char_checked(ch,mode);}
size_t send_to_char(struct char_data *ch,const char *fmt,...){(void)ch;(void)fmt;return 0;}
void basic_mud_log(const char *format,...){(void)format;}
static void player(struct char_data *ch,struct player_special_data *ps,
    struct descriptor_data *d,struct account_data *account,int64_t id,int level)
{
  ch->player_specials=ps;ch->desc=d;d->character=ch;d->account=account;STATE(d)=CON_PLAYING;
  GET_IDNUM(ch)=id;GET_LEVEL(ch)=level;GET_HIT(ch)=100;GET_MAX_HIT(ch)=100;
  GET_POS(ch)=POS_STANDING;IN_ROOM(ch)=0;
}
int main(int argc,char **argv)
{
  struct char_data ch={0},alt={0},npc={0},outsider={0},pet={0};
  struct player_special_data ps={0},aps={0},ops={0};
  struct descriptor_data d={0},ad={0},od={0};struct account_data account={0},other={0};
  struct room_data rooms[1]={{0}};struct index_data mobs[1]={{0}};
  struct group_data group={0};
  char uid[33];assert(argc==3||argc==4);
  if(argc==4)clock_value=atoll(argv[3]);
  connection.conn=mysql_init(NULL);
  assert(mysql_real_connect(connection.conn,NULL,"root","","clan_test",0,argv[1],0));
  assert(!mysql_query(connection.conn,"SET SESSION sql_mode='STRICT_ALL_TABLES,NO_ENGINE_SUBSTITUTION'"));
  for(const char *p=argv[2];*p;p++)sequence=sequence*31+(unsigned char)*p;
  policy=clan_progression_defaults;policy.controls.store_mode=CLAN_STORE_MYSQL;
  policy.controls.mode=CLAN_PROGRESSION_LIVE;policy.controls.public_release=true;
  account.id=77;other.id=88;
  player(&ch,&ps,&d,&account,5000000007,30);
  player(&alt,&aps,&ad,&account,5000000012,30);
  player(&outsider,&ops,&od,&other,5000000013,40);
  GROUP((&ch))=&group;GROUP((&alt))=&group;
  world=rooms;mob_index=mobs;mob_index[0].vnum=4301;
  SET_BIT_AR(MOB_FLAGS(&npc),MOB_ISNPC);npc.nr=0;GET_LEVEL(&npc)=30;
  GET_MAX_HIT(&npc)=1000;GET_HIT(&npc)=1000;IN_ROOM(&npc)=0;
  character_list=&ch;ch.next=&alt;alt.next=&outsider;
  world[0].people=&ch;ch.next_in_room=&alt;alt.next_in_room=&npc;
  if(!strncmp(argv[2],"supply",6)) {
    clan_source_supply_identity(&ch,uid);assert(strlen(uid)==32);
    if(!strcmp(argv[2],"supply-deleted")) {
      assert(!mysql_query(connection.conn,"SELECT revision FROM clan_store_control WHERE singleton=1"));
      MYSQL_RES *res=mysql_store_result(connection.conn);MYSQL_ROW row=mysql_fetch_row(res);
      assert(row);int64_t revision=atoll(row[0]);mysql_free_result(res);
      assert(clan_repository_disband(connection.conn,uid,revision,clock_value)==CLAN_REPO_OK);
    }
    if(!strcmp(argv[2],"supply-failed-save"))save_ok=false;
    clan_source_supply_complete(&ch,uid);
    assert(saves>0);
    if(!save_ok)assert(GET_CRAFT((&ch)).clan_supply_event[0]);
    else assert(!GET_CRAFT((&ch)).clan_supply_event[0]);
    if(!strcmp(argv[2],"supply-replay")) {
      assert(captured);GET_CRAFT((&ch))=checkpoint;
    }
    /* A stale saved checkpoint must replay its original receipt. */
    clan_source_tick(clock_value+31);
  } else {
    FIGHTING(&ch)=&npc;
    if(!strcmp(argv[2],"outsider")||!strcmp(argv[2],"abandoned")||
        !strcmp(argv[2],"reset")||!strcmp(argv[2],"outroom-pet")) {
      GET_LEVEL(&ch)=20;GET_LEVEL(&alt)=20;GET_LEVEL(&npc)=20;GET_LEVEL(&outsider)=30;
    }
    if(!strcmp(argv[2],"disabled"))policy.controls.mode=CLAN_PROGRESSION_OFF;
    if(!strcmp(argv[2],"trivial"))GET_LEVEL(&npc)=20;
    if(!strcmp(argv[2],"owned"))npc.legendary_owned_instance=true;
    if(!strcmp(argv[2],"outsider")||!strcmp(argv[2],"abandoned")||!strcmp(argv[2],"reset")) {
      FIGHTING(&outsider)=&npc;outsider.next_in_room=world[0].people;world[0].people=&outsider;
    }
    if(!strcmp(argv[2],"outage"))mysql_available=false;
    clan_source_damage(&ch,&npc,50,true);
    if(!strcmp(argv[2],"outage"))mysql_available=true;
    if(!strcmp(argv[2],"pet")||!strcmp(argv[2],"confuser")) {
      clan_source_forget(&npc);pet.master=&ch;SET_BIT_AR(MOB_FLAGS(&pet),MOB_ISNPC);
      if(!strcmp(argv[2],"confuser")){pet.master=NULL;pet.confuser_idnum=GET_IDNUM(&ch);}
      IN_ROOM(&pet)=0;clan_source_damage(&pet,&npc,50,true);
    }
    if(!strcmp(argv[2],"outroom-pet")) {
      pet.master=&outsider;SET_BIT_AR(MOB_FLAGS(&pet),MOB_ISNPC);
      IN_ROOM(&pet)=0;IN_ROOM(&outsider)=1;clan_source_damage(&pet,&npc,50,true);
    }
    if(!strcmp(argv[2],"alt")){FIGHTING(&alt)=&npc;clan_source_damage(&alt,&npc,50,true);}
    if(!strcmp(argv[2],"insufficient")) {
      clan_source_forget(&npc);clan_source_damage(&ch,&npc,49,true);
    }
    if(!strcmp(argv[2],"support")) {
      clan_source_forget(&npc);FIGHTING(&alt)=&npc;
      clan_source_action(&ch,&npc,CLAN_COMBAT_DEFENSE,&alt,1);
      clock_value+=6;clan_source_action(&ch,&npc,CLAN_COMBAT_DEFENSE,&alt,1);
    }
    if(!strcmp(argv[2],"heal")) {
      clan_source_forget(&npc);FIGHTING(&alt)=&npc;
      clan_source_support(&ch,&alt,CLAN_COMBAT_HEAL,10);
    }
    if(!strcmp(argv[2],"tank")) {
      clan_source_forget(&npc);FIGHTING(&npc)=&ch;
      clan_source_damage(&npc,&ch,1,true);clan_source_damage(&npc,&ch,1,true);
    }
    if(!strcmp(argv[2],"abandoned")||!strcmp(argv[2],"reset")) {
      FIGHTING(&outsider)=NULL;IN_ROOM(&outsider)=1;
      FIGHTING(&ch)=NULL;
      GET_HIT(&npc)=!strcmp(argv[2],"reset")?1000:500;
      clock_value+=601;clan_source_tick(clock_value);
      FIGHTING(&ch)=&npc;clan_source_damage(&ch,&npc,50,true);
    }
    if(!strcmp(argv[2],"long")) {
      for(int i=0;i<1100;i++) {
        clock_value++;clan_source_damage(&ch,&npc,1,true);
      }
    }
    if(!strcmp(argv[2],"overflow")) {
      for(int i=0;i<1100;i++)clan_source_damage(&ch,&npc,1,true);
    }
    if(!strcmp(argv[2],"one-hit-bonus")) {
      clan_source_forget(&npc);FIGHTING(&npc)=&ch;
      clan_source_damage(&npc,&ch,1,true);clan_source_damage(&npc,&ch,1,false);
    }
    if(!strcmp(argv[2],"dead"))GET_HIT(&ch)=0;
    clan_source_death(&npc,&ch);
    clan_source_death(&npc,&ch); /* exact live death must not repeat */
    clan_source_forget(&npc);
  }
  mysql_close(connection.conn);puts("live source passed");return 0;
}
