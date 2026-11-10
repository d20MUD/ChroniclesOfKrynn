#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "clan.h"
#include "mysql.h"
#include "clan_repository.h"
#include "clan_persistence.h"
#include <assert.h>
struct clan_data *clan_list;int num_of_clans;
struct claim_data *claim_list;
struct player_special_data dummy_mob;
static struct player_index_element rows[3];struct player_index_element *player_table=rows;int top_of_p_table=2;
struct char_data *character_list;
bool mysql_available=true;static MYSQL_POOL_CONN pool;static int legacy_calls,hash_rebuilds;
MYSQL_POOL_CONN *mysql_pool_acquire(void){return mysql_available?&pool:NULL;}
void mysql_pool_release(MYSQL_POOL_CONN *c){(void)c;}
size_t strlcpy(char *dst,const char *src,size_t n){size_t len=strlen(src);if(n)snprintf(dst,n,"%s",src);return len;}
void basic_mud_log(const char *s,...){(void)s;}
size_t send_to_char(struct char_data *c,const char *s,...){(void)c;(void)s;return 0;}
void load_clans(void){legacy_calls++;}void load_claims(void){legacy_calls++;}
void rebuild_clan_hash(void){hash_rebuilds++;}
void free_clan_list(void){int i,j;for(i=0;i<num_of_clans;i++){free(clan_list[i].clan_name);for(j=0;j<15;j++)free(clan_list[i].rank_name[j]);}free(clan_list);clan_list=NULL;num_of_clans=0;}
void free_claim_list(void){while(claim_list){struct claim_data *next=claim_list->next;free(claim_list);claim_list=next;}}
int main(int argc,char **argv)
{
 const char *scenario;struct clan_buff_state b;struct char_data npc={0},ch={0};struct player_special_data saved={0};int i;
 SET_BIT_AR(MOB_FLAGS(&npc),MOB_ISNPC);character_list=&npc;
 assert(argc==3);scenario=argv[2];pool.conn=mysql_init(NULL);assert(mysql_real_connect(pool.conn,NULL,"root","","clan_test",0,argv[1],0));assert(!mysql_query(pool.conn,"SET SESSION sql_mode='STRICT_ALL_TABLES,NO_ENGINE_SUBSTITUTION'"));
 for(i=0;i<3;i++){rows[i].id=INT64_C(5000000007)+(i==0?0:i==1?2:3);rows[i].clan=123;rows[i].name="fixture";}
 if(!strcmp(scenario,"env-legacy")){setenv("CLAN_STORE_MODE","legacy",1);assert(clan_persistence_boot_environment());assert(legacy_calls==2);}
 else if(!strcmp(scenario,"legacy")){assert(clan_persistence_boot(NULL));assert(legacy_calls==2&&!clan_persistence_sql_authority());}
 else if(!strcmp(scenario,"shadow")){assert(clan_persistence_boot("shadow"));assert(legacy_calls==2&&!clan_persistence_sql_authority());}
 else if(!strcmp(scenario,"invalid")){assert(!clan_persistence_boot("MySQL"));assert(!legacy_calls);}
 else if(!strcmp(scenario,"unavailable")){mysql_available=false;assert(!clan_persistence_boot("mysql"));assert(clan_persistence_sql_authority()&&!clan_persistence_ready()&&!legacy_calls);assert(!clan_persistence_configure(CLAN_STORE_LEGACY));}
 else if(!strcmp(scenario,"missing-player")){rows[0].flags=PINDEX_DELETED;assert(!clan_persistence_boot("mysql"));assert(!clan_persistence_ready()&&!legacy_calls);}
 else if(!strcmp(scenario,"rejected")){assert(!clan_persistence_boot("mysql"));assert(clan_persistence_sql_authority()&&!clan_persistence_ready()&&!legacy_calls);assert(!clan_persistence_boot_buff(0,&b));}
 else {
  assert(!strcmp(scenario,"file-mysql")?clan_persistence_boot_environment():clan_persistence_boot("mysql"));assert(clan_persistence_sql_authority()&&clan_persistence_ready()&&!legacy_calls&&hash_rebuilds==1);
  if(!strcmp(scenario,"fresh")){assert(num_of_clans==0&&!claim_list&&!clan_persistence_boot_buff(0,&b));}
  else {
   assert(num_of_clans==2&&clan_list[0].vnum==7&&clan_list[1].vnum==9);assert(clan_list[0].allies[1]&&clan_list[1].allies[0]);
   assert(clan_list[0].treasure==INT64_C(5000000000));assert(claim_list&&claim_list->clan==7&&claim_list->popularity[0]==17.25f);
   assert(rows[0].clan==7&&rows[1].clan==9&&rows[2].clan==(int)NO_CLAN);
   ch.player_specials=&saved;GET_IDNUM(&ch)=rows[0].id;GET_PFILEPOS(&ch)=0;GET_CLAN(&ch)=123;GET_CLANRANK(&ch)=3;GET_CLANPOINTS(&ch)=99;
   clan_persistence_reconcile_player(&ch);assert(GET_CLAN(&ch)==7&&GET_CLANRANK(&ch)==1&&GET_CLANPOINTS(&ch)==99);
   assert(clan_persistence_boot_buff(0,&b));assert(b.expires_at==300&&b.next_available_at==500&&!clan_repository_buff_active(&b,400));
   assert(!clan_persistence_boot_buff(2,&b));
   /* No saved progression-mode flag can enable effects at boot. */
   assert(clan_progression_defaults.controls.mode==CLAN_PROGRESSION_OFF);
  }
 }
 for(i=0;i<num_of_clans;i++){free(clan_list[i].description);free(clan_list[i].abrev);}free_clan_list();free_claim_list();mysql_close(pool.conn);printf("Clan boot scenario %s passed\n",scenario);return 0;
}
