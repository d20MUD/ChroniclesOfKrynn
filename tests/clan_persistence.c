#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "interpreter.h"
#include "clan.h"
#include "clan_repository.h"
#include "clan_persistence.h"
#include "clan_awards.h"
#include "clan_legacy_fields.h"
#include "mysql.h"
#include <json-c/json.h>
#include <assert.h>
struct clan_data *clan_list;int num_of_clans=2;
static struct claim_data claim,claim2;struct claim_data *claim_list=&claim;
struct player_special_data dummy_mob;
static bool remove_sql_clan(clan_vnum);
clan_rnum real_clan(clan_vnum v) {int i;for(i=0;i<num_of_clans;i++)if(clan_list[i].vnum==v)return i;return NO_CLAN;}
void remove_clan_from_hash(clan_vnum v){(void)v;}
void add_clan_to_hash(clan_vnum v,clan_rnum r){(void)v;(void)r;}
void rebuild_clan_hash(void){}
void free_clan_hash(void){}
void log_clan_error(const char *f,const char *format,...){(void)f;(void)format;}
void save_player_index(void){}

static struct player_index_element index_rows[2];struct player_index_element *player_table=index_rows;int top_of_p_table=1;
struct char_data *character_list;
bool mysql_available=true;static MYSQL_POOL_CONN pool;
MYSQL_POOL_CONN *mysql_pool_acquire(void){return mysql_available?&pool:NULL;}
void mysql_pool_release(MYSQL_POOL_CONN *connection){(void)connection;}
void basic_mud_log(const char *format,...){(void)format;}
void mudlog(int a,int b,int c,const char *format,...){(void)a;(void)b;(void)c;(void)format;}
size_t send_to_char(struct char_data *ch,const char *format,...){(void)ch;(void)format;return 0;}
size_t strlcpy(char *dst,const char *src,size_t n){size_t size=strlen(src);if(n)snprintf(dst,n,"%s",src);return size;}
char *strip_cr(char *s){char *p=s,*q=s;while(*p){if(*p!='\r')*q++=*p;p++;}*q=0;return s;}
int get_line(FILE *fl,char *buf){return fgets(buf,MAX_INPUT_LENGTH,fl)?1:0;}
static struct json_object *get(struct json_object *o,const char *k){struct json_object *v=NULL;json_object_object_get_ex(o,k,&v);return v;}
static void hydrate_fixture(struct json_object *root)
{
 size_t i,j;struct json_object *clans=get(root,"clans");
 for(i=0;i<2;i++){
  struct json_object *entry=json_object_array_get_idx(clans,i),*legacy=get(entry,"legacy");struct clan_repository_record r;struct clan_data *c=&clan_list[i];assert(clan_repository_record_parse(get(entry,"core"),&r));
  c->vnum=r.vnum;c->clan_name=strdup(r.name);c->description=strdup(r.description);c->abrev=strdup(r.abbreviation);c->leader=r.leader_id;c->treasure=r.treasury_gold;c->date_founded=r.founded_at;c->last_activity=r.last_activity_at;
  #define FILL(name,member,tag) c->member=json_object_get_int64(get(legacy,#name));
  CLAN_LEGACY_FIELDS(FILL)
  #undef FILL
  c->ranks=(ubyte)json_object_array_length(get(legacy,"ranks"));
  for(j=0;j<c->ranks;j++)c->rank_name[j]=strdup(json_object_get_string(json_object_array_get_idx(get(legacy,"ranks"),j)));
  for(j=0;j<21;j++)c->privilege[j]=(ubyte)json_object_get_int(get(json_object_array_get_idx(get(legacy,"privileges"),j),"rank"));
  c->allies[1-i]=1;
 }
 claim.zn=100;claim.clan=7;claim.claimant=5000000007;claim.popularity[0]=17.25;claim.popularity[1]=82.75;
 claim2.zn=101;claim2.clan=9;claim2.next=&claim;claim_list=&claim2;
}
int main(int argc,char **argv)
{
 struct json_object *root=NULL;struct char_data player={0};struct player_special_data saved={0};FILE *f;char before[20000],after[20000];size_t n;int i,j;
 assert(argc==2);clan_list=calloc(2,sizeof(*clan_list));pool.conn=mysql_init(NULL);assert(mysql_real_connect(pool.conn,NULL,"root","","clan_test",0,argv[1],0));assert(!mysql_query(pool.conn,"SET SESSION sql_mode='STRICT_ALL_TABLES,NO_ENGINE_SUBSTITUTION'"));
 assert(clan_repository_snapshot_read(pool.conn,&root)==CLAN_REPO_OK);hydrate_fixture(root);json_object_put(root);
 assert(clan_persistence_ready()&&!clan_persistence_sql_authority());assert(save_clans()&&save_claims());
 f=fopen(CLAN_FILE,"r");assert(f);n=fread(before,1,sizeof(before),f);fclose(f);
 mysql_available=false;assert(!clan_persistence_configure(CLAN_STORE_MYSQL));assert(clan_persistence_sql_authority()&&!clan_persistence_ready());assert(!save_clans());assert(!clan_persistence_configure(CLAN_STORE_LEGACY));mysql_available=true;
 assert(clan_persistence_configure(CLAN_STORE_MYSQL));assert(clan_persistence_sql_authority());
 clan_list[0].treasure=123456;clan_list[0].modified=true;assert(save_single_clan(0));
 assert(clan_repository_snapshot_read(pool.conn,&root)==CLAN_REPO_OK);assert(json_object_get_int64(get(get(json_object_array_get_idx(get(root,"clans"),0),"core"),"treasury_gold"))==123456);json_object_put(root);
 /* Real receipt award, refresh, then a legacy metadata save must retain XP. */
 {
  struct clan_reward_event e={0};struct json_object *receipt=NULL;
  struct clan_progression_config config=clan_progression_v1;
  struct clan_progression_readiness readiness={true,false,false,false,CLAN_PROGRESSION_SCHEMA_VERSION};
  int64_t now=(int64_t)time(NULL);
  strcpy(e.operation_uid,"11111111111111111111111111111111");
  strcpy(e.event_uid,"22222222222222222222222222222222");
  strcpy(e.clan_uid,"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
  e.source=CLAN_SOURCE_BOUNTY_SHARED;e.rules_version=1;e.occurred_at=now;
  e.requested_xp_eighths=1600;e.requested_influence_eighths=800;
  config.controls.store_mode=CLAN_STORE_MYSQL;config.controls.mode=CLAN_PROGRESSION_LIVE;config.controls.public_release=true;
  assert(clan_reward_prepare(pool.conn,&e,now,&receipt)==CLAN_REPO_OK);json_object_put(receipt);
  assert(clan_award_complete_shared(pool.conn,e.operation_uid,now,&config,&readiness,&receipt)==CLAN_REPO_OK);json_object_put(receipt);
  assert(clan_persistence_refresh_progression());
  clan_list[0].modified=true;assert(save_single_clan(0));
  assert(clan_repository_snapshot_read(pool.conn,&root)==CLAN_REPO_OK);
  assert(json_object_get_int64(get(get(json_object_array_get_idx(get(root,"clans"),0),"core"),"xp_eighths"))==1600);
  assert(json_object_get_int64(get(get(json_object_array_get_idx(get(root,"clans"),0),"core"),"influence_eighths"))==800);
  json_object_put(root);
  /* A second award is reconciled automatically even without explicit refresh. */
  e.operation_uid[0]='3';e.event_uid[0]='4';now=(int64_t)time(NULL);
  assert(clan_reward_prepare(pool.conn,&e,now,&receipt)==CLAN_REPO_OK);json_object_put(receipt);
  assert(clan_award_complete_shared(pool.conn,e.operation_uid,now,&config,&readiness,&receipt)==CLAN_REPO_OK);json_object_put(receipt);
  clan_list[0].modified=true;assert(save_single_clan(0));
  assert(clan_repository_snapshot_read(pool.conn,&root)==CLAN_REPO_OK);
  assert(json_object_get_int64(get(get(json_object_array_get_idx(get(root,"clans"),0),"core"),"xp_eighths"))==3200);
  json_object_put(root);
 }
 player.player_specials=&saved;GET_IDNUM(&player)=5000000009;GET_PFILEPOS(&player)=1;GET_CLAN(&player)=9;GET_CLANRANK(&player)=1;GET_CLANPOINTS(&player)=777;
 character_list=&player;GET_CLANRANK(&player)=2;clan_list[1].total_members_joined++;clan_list[1].modified=true;
 assert(save_single_clan(1));
 assert(clan_repository_snapshot_read(pool.conn,&root)==CLAN_REPO_OK);
 assert(json_object_get_int(get(json_object_array_get_idx(get(root,"members"),1),"rank_id"))==2);
 json_object_put(root);GET_CLANRANK(&player)=1;
 assert(clan_persistence_before_player_save(&player));GET_CLAN(&player)=65535;GET_CLANRANK(&player)=0;clan_persistence_reconcile_player(&player);assert(GET_CLAN(&player)==9&&GET_CLANRANK(&player)==1&&GET_CLANPOINTS(&player)==777);
 assert(!clan_persistence_economy_available(NULL));
 /* SQL disband commits first, revokes mirrors, then compacts runtime indices. */
 {
  struct char_data member={0};struct player_special_data member_saved={0};
  member.player_specials=&member_saved;GET_IDNUM(&member)=5000000007;GET_PFILEPOS(&member)=0;
  GET_CLAN(&member)=7;GET_CLANRANK(&member)=1;GET_CLANPOINTS(&member)=123;
  index_rows[0].clan=7;member.next=character_list;character_list=&member;
  assert(clan_persistence_progression_access(&member,"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));
  assert(clan_persistence_member_update(GET_IDNUM(&member),0,0));
  assert(!clan_persistence_progression_access(&member,"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));
  /* Clear stale local mirrors before the next authoritative membership write. */
  clan_persistence_reconcile_player(&member);
  assert(clan_persistence_member_update(GET_IDNUM(&member),7,0));
  clan_persistence_reconcile_player(&member);
  assert(!clan_persistence_progression_access(&member,"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));
  assert(clan_persistence_member_update(GET_IDNUM(&member),7,1));
  clan_persistence_reconcile_player(&member);
  assert(clan_persistence_progression_access(&member,"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));
  assert(remove_sql_clan(7));
  assert(num_of_clans==1&&clan_list[0].vnum==9);
  assert(!clan_list[0].allies[0]&&!clan_list[0].at_war[0]);
  assert(claim.clan==NO_CLAN&&claim.claimant==0&&claim.popularity[0]==82.75f);
  assert(GET_CLAN(&member)==NO_CLAN&&GET_CLANRANK(&member)==0&&GET_CLANPOINTS(&member)==123);
  assert(index_rows[0].clan==NO_CLAN);
  assert(!clan_persistence_progression_access(&member,"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));
  assert(clan_repository_snapshot_read(pool.conn,&root)==CLAN_REPO_OK);
  assert(json_object_array_length(get(root,"clans"))==1);json_object_put(root);
  /* Atomic SQL creation can safely reuse its numeric vnum with a new UID. */
  {
    struct clan_data draft={0};struct clan_favor_state retained={0};
    const char *new_uid;
    retained.character_id=GET_IDNUM(&member);strcpy(retained.clan_uid,"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
    retained.favor_eighths=777;
    assert(clan_repository_favor_save(pool.conn,&retained,-1,(int64_t)time(NULL))==CLAN_REPO_DELETED);
    draft.vnum=7;draft.clan_name=strdup("Replacement");draft.leader=GET_IDNUM(&member);
    draft.ranks=2;draft.rank_name[0]=strdup("Leader");draft.rank_name[1]=strdup("Member");
    draft.date_founded=draft.last_activity=time(NULL);
    for(j=0;j<NUM_CLAN_PRIVS;j++)draft.privilege[j]=1;
    assert(!mysql_query(pool.conn,"CREATE TRIGGER fail_create BEFORE INSERT ON clan_memberships FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='injected creation failure'"));
    assert(!clan_persistence_create(&draft,&member)&&num_of_clans==1);
    assert(clan_repository_snapshot_read(pool.conn,&root)==CLAN_REPO_OK);
    assert(json_object_array_length(get(root,"clans"))==1);json_object_put(root);
    assert(!mysql_query(pool.conn,"DROP TRIGGER fail_create"));
    assert(clan_persistence_refresh_progression());
    assert(clan_persistence_create(&draft,&member));
    assert(add_clan(&draft));clan_persistence_reconcile_player(&member);
    assert(num_of_clans==2&&GET_CLAN(&member)==7&&GET_CLANRANK(&member)==1);
    assert(clan_repository_snapshot_read(pool.conn,&root)==CLAN_REPO_OK);
    new_uid=json_object_get_string(get(get(json_object_array_get_idx(get(root,"clans"),0),"core"),"uid"));
    assert(strcmp(new_uid,"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));
    assert(clan_persistence_progression_access(&member,new_uid));
    assert(!clan_persistence_progression_access(&member,"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));
    json_object_put(root);
    assert(save_single_clan(0)); /* Compacted runtime order differs from SQL sort. */
  }
  character_list=member.next;
 }
 mysql_available=false;clan_list[0].modified=true;assert(!save_single_clan(0));assert(clan_list[0].modified&&!clan_persistence_ready());assert(!clan_persistence_configure(CLAN_STORE_LEGACY));
 GET_CLANRANK(&player)=2;assert(!clan_persistence_before_player_save(&player));assert(GET_CLAN(&player)==NO_CLAN&&GET_CLANRANK(&player)==0);assert(clan_persistence_before_player_save(&player));
 f=fopen(CLAN_FILE,"r");assert(f);assert(fread(after,1,sizeof(after),f)==n&&!memcmp(before,after,n));fclose(f);
 /* Progression refresh cannot silently merge an unrelated remote clan edit. */
 mysql_available=true;
 assert(!mysql_query(pool.conn,"UPDATE clan_progression_clans SET name='Remote edit',revision=revision+1 WHERE clan_vnum=9"));
 assert(!clan_persistence_refresh_progression()&&!clan_persistence_ready());
 assert(clan_persistence_sql_authority()&&!clan_persistence_configure(CLAN_STORE_LEGACY));
 for(i=0;i<num_of_clans;i++){free(clan_list[i].clan_name);free(clan_list[i].description);free(clan_list[i].abrev);for(j=0;j<clan_list[i].ranks;j++)free(clan_list[i].rank_name[j]);}
 free(clan_list);mysql_close(pool.conn);puts("Gated clan writers: default legacy files, checked MySQL authority, membership mirrors without changing personal points, failed writes/dirty flags, unavailable eligibility and no stale-file fallback passed.");return 0;
}
