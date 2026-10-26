#define main estate_baseline_main
#include "estate_integration.c"
#undef main
#include "legendary_earnings.h"
#include "legendary_transaction.h"
#include "legendary_identity.h"
#include "traps.h"
#include "spells.h"
#undef assert
#define assert(t) do {if(!(t)){fprintf(stderr,"earnings assertion %d: %s\n",__LINE__,#t);abort();}} while(0)
int MIN(int a,int b){return a<b?a:b;}
struct index_data *mob_index;
mob_rnum top_of_mobt;
#include "mudlim.h"
#include "account.h"
#include <sys/wait.h>
#include "perks.h"
struct config_data config_info;
struct happyhour happy_data;
static bool advance_during_award;
int leadership_exp_multiplier(struct char_data *ch){(void)ch;return 200;}
int get_party_size_same_room(struct char_data *ch){(void)ch;return 0;}
int get_feat_value(struct char_data *ch,int feat){(void)ch;(void)feat;return 0;}
long int level_exp(struct char_data *ch,int level){(void)ch;return (long)level*185000;}
int rand_number(int from,int to){(void)to;return from;}
int change_account_xp(struct char_data *ch,int amount){(void)ch;return amount;}
int apply_account_reward_bonus(struct char_data *ch,int amount,int bonus){(void)ch;(void)bonus;return amount*3/2;}
bool check_stage_advancement(struct char_data *ch,int *points){*points=0;if(advance_during_award)GET_LEVEL(ch)=30;return false;}
long int get_minimum_experience_floor(struct char_data *ch){(void)ch;return 0;}
void run_autowiz(void){}
int vampire_last_feeding_adjustment(struct char_data *ch){(void)ch;return 0;}
void send_combat_roll_info(struct char_data *ch,const char *format,...){(void)ch;(void)format;}
void update_pos(struct char_data *ch){GET_POS(ch)=GET_HIT(ch)>0?POS_FIGHTING:POS_DEAD;}

static struct char_data pc[4],boss,pet;
static struct descriptor_data descriptors[4];
static struct group_data party,other_party;
static int64_t start=INT64_C(20000)*86400+3600;
static void spawn(int index)
{
  if(IN_ROOM(&boss)!=NOWHERE)char_from_room(&boss);
  memset(&boss,0,sizeof(boss));IN_ROOM(&boss)=NOWHERE;
  SET_BIT_AR(MOB_FLAGS(&boss),MOB_ISNPC);SET_BIT_AR(MOB_FLAGS(&boss),MOB_LEGENDARY_BOSS);GET_MOB_RNUM(&boss)=index;GET_LEVEL(&boss)=30;
  GET_HIT(&boss)=GET_MAX_HIT(&boss)=100000;GET_POS(&boss)=POS_FIGHTING;
  char_to_room(&boss,1);
}
static void reset(void)
{
  int i;
  assert(estate_query("DELETE FROM legendary_awards"));assert(estate_query("DELETE FROM legendary_operations"));
  assert(estate_query("DELETE FROM legendary_boss_claims"));assert(estate_query("DELETE FROM legendary_quest_xp_claims"));
  assert(estate_query("DELETE FROM legendary_characters"));assert(estate_query("DELETE FROM estate_deliveries"));
  legendary_transaction_fault=0;legendary_transaction_clock_offset=0;
  legendary_earnings_test_time+=181;legendary_activity_expire();legendary_earnings_test_time=start;
  for(i=0;i<4;i++) {
    if(IN_ROOM(&pc[i])!=1){char_from_room(&pc[i]);char_to_room(&pc[i],1);}
    GET_LEVEL(&pc[i])=30;GET_HIT(&pc[i])=GET_MAX_HIT(&pc[i])=100;GET_POS(&pc[i])=POS_FIGHTING;
    pc[i].group=&party;FIGHTING(&pc[i])=&boss;pc[i].desc=&descriptors[i];
    descriptors[i].connected=CON_PLAYING;pc[i].legendary_awards_pending=false;pc[i].player_specials->legendary_operation[0]=0;
    assert(save_char_checked(&pc[i],0));
  }
  spawn(0);pet.master=&pc[3];IN_ROOM(&pet)=1;
  legendary_earnings_maintenance();
}
static void actions(struct char_data *actor,enum legendary_action kind,struct char_data *ally)
{
  /* Three immediate real packets model ordinary multi-attack combat. */
  int i;for(i=0;i<3;i++)legendary_record_action(actor,&boss,kind,ally,1);
}
static long balance(long id,const char *field)
{
  char sql[256];snprintf(sql,sizeof(sql),"SELECT %s FROM legendary_characters WHERE player_id=%ld",field,id);return scalar(sql);
}
static void death(void){legendary_boss_death(&boss,&pc[0]);}
static void restart_receipt(struct char_data *ch)
{
  char path[128],line[256];FILE *f;
  get_filename(path,sizeof(path),PLR_FILE,GET_NAME(ch));f=fopen(path,"r");assert(f);
  ch->player_specials->legendary_operation[0]=0;
  ch->legendary_awards_pending=false;
  while(fgets(line,sizeof(line),f)) {
    if(!strncmp(line,"LgTx:",5))sscanf(line+5,"%32s",ch->player_specials->legendary_operation);
    if(!strncmp(line,"Gold:",5))GET_GOLD(ch)=atoi(line+5);
    if(!strncmp(line,"EDbt:",5))ch->player_specials->estate_debit_receipt=strtoull(line+5,NULL,10);
  }
  fclose(f);
}
int main(int argc,char **argv)
{
  struct legendary_xp_context source,other;
  struct legendary_balance b;
  struct legendary_exchange *x;
  struct trap_data trap={0},*copy;
  int i;
  assert(argc==2);conn=mysql_init(NULL);assert(mysql_real_connect(conn,NULL,"root",NULL,"estate_test",0,argv[1],0));
  assert(estate_init() && legendary_schema_ready());
  assert(estate_query("CREATE TABLE player_save_objs(idnum INT PRIMARY KEY AUTO_INCREMENT,name VARCHAR(64),serialized_obj LONGTEXT,creation_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP)"));
  assert(estate_query("CREATE TABLE player_data(name VARCHAR(64) PRIMARY KEY,obj_save_header TEXT)"));
  world=calloc(3,sizeof(*world));top_of_world=2;world[1].number=2200;world[2].number=6530;
  mob_index=calloc(3,sizeof(*mob_index));top_of_mobt=2;
  for(i=0;i<3;i++)mob_index[i].vnum=900+i;
  IN_ROOM(&boss)=NOWHERE;SET_BIT_AR(MOB_FLAGS(&pet),MOB_ISNPC);SET_BIT_AR(AFF_FLAGS(&pet),AFF_CHARM);
  for(i=0;i<4;i++){char name[32];snprintf(name,sizeof(name),"player%d",i);init_player(&pc[i],name,i+1);pc[i].desc=&descriptors[i];descriptors[i].character=&pc[i];}
  reset();
  legendary_xp_kill_context(&source,&boss);assert(source.eligible);
  legendary_credit_xp(&pc[0],123,&source);legendary_credit_xp(&pc[0],123,&source);
  assert(balance(1,"infusion_xp")==123);
  other=source;other.origin=LEG_XP_NONE;legendary_credit_xp(&pc[0],900,&other);assert(balance(1,"infusion_xp")==123);
  GET_LEVEL(&pc[0])=29;legendary_xp_quest_context(&other,100);legendary_credit_xp(&pc[0],900,&other);GET_LEVEL(&pc[0])=30;
  assert(balance(1,"infusion_xp")==123);
  legendary_xp_quest_context(&source,100);legendary_credit_xp(&pc[0],200,&source);
  legendary_xp_quest_context(&other,100);legendary_credit_xp(&pc[0],300,&other);assert(balance(1,"infusion_xp")==323);
  legendary_earnings_test_time+=86400;legendary_xp_quest_context(&other,100);legendary_credit_xp(&pc[0],300,&other);
  assert(balance(1,"infusion_xp")==623);assert(scalar("SELECT COUNT(*) FROM legendary_quest_xp_claims")==2);
  legendary_xp_trap_context(&source,&trap);assert(!source.eligible);SET_BIT(trap.flags,TRAP_FLAG_DISARMED);
  legendary_xp_trap_context(&source,&trap);assert(source.eligible);legendary_credit_xp(&pc[0],77,&source);
  legendary_credit_xp(&pc[0],77,&source);assert(balance(1,"infusion_xp")==700);
  copy=copy_trap(&trap);assert(copy && !*copy->legendary_disarm_id);free_trap(copy);
  boss.master=&pc[0];legendary_xp_kill_context(&other,&boss);assert(!other.eligible);boss.master=NULL;
  boss.legendary_owned_instance=true;legendary_xp_kill_context(&other,&boss);assert(!other.eligible);boss.legendary_owned_instance=false;
  boss.legendary_test_encounter=true;legendary_xp_kill_context(&other,&boss);assert(!other.eligible);boss.legendary_test_encounter=false;
  assert(estate_query("UPDATE legendary_characters SET infusion_xp=1999999995,version=version+1 WHERE player_id=1"));
  legendary_xp_quest_context(&source,101);legendary_credit_xp(&pc[0],50,&source);assert(balance(1,"infusion_xp")==2000000000);
  assert(estate_query("UPDATE legendary_characters SET infusion_xp=1999999990,version=version+1 WHERE player_id=1"));
  legendary_credit_xp(&pc[0],50,&source);assert(balance(1,"infusion_xp")==1999999990);
  reset();
  actions(&pc[0],LEG_ACTION_DAMAGE,NULL);actions(&pc[1],LEG_ACTION_HEAL,&pc[0]);
  actions(&pc[2],LEG_ACTION_INTERCEPT,&pc[0]);actions(&pet,LEG_ACTION_DAMAGE,NULL);death();
  for(i=1;i<=4;i++)assert(balance(i,"fragments")==30 && balance(i,"draconic")==6 && balance(i,"catalysts")==1);
  assert(scalar("SELECT COUNT(*) FROM legendary_boss_claims")==4);death();assert(balance(1,"fragments")==30);
  spawn(0);actions(&pc[0],LEG_ACTION_DAMAGE,NULL);death();assert(balance(1,"fragments")==30); /* clone cooldown */
  spawn(1);actions(&pc[0],LEG_ACTION_DAMAGE,NULL);death();assert(balance(1,"fragments")==60); /* independent level-30 prototype */
  legendary_earnings_test_time+=72001;spawn(0);actions(&pc[0],LEG_ACTION_DAMAGE,NULL);death();assert(balance(1,"fragments")==90);
  /* Rapid real packets satisfy the three-action participation threshold. */
  reset();for(i=0;i<50;i++)legendary_record_action(&pc[0],&boss,LEG_ACTION_DAMAGE,NULL,1);death();
  assert(scalar("SELECT COUNT(*) FROM legendary_awards")==1);
  reset();actions(&pc[0],LEG_ACTION_DAMAGE,NULL);legendary_earnings_test_time+=121;death();assert(scalar("SELECT COUNT(*) FROM legendary_awards")==0);
  legendary_earnings_test_time+=181;legendary_activity_expire();legendary_record_action(&pc[0],&boss,LEG_ACTION_DAMAGE,NULL,1);death();
  assert(scalar("SELECT COUNT(*) FROM legendary_awards")==0);
  reset();actions(&pc[0],LEG_ACTION_DAMAGE,NULL);actions(&pc[1],LEG_ACTION_DAMAGE,NULL);actions(&pc[2],LEG_ACTION_DAMAGE,NULL);actions(&pc[3],LEG_ACTION_DAMAGE,NULL);
  pc[1].desc=NULL;GET_HIT(&pc[2])=0;pc[3].group=&other_party;death();
  assert(balance(1,"fragments")==30);assert(scalar("SELECT COUNT(*) FROM legendary_awards")==1);
  reset();actions(&pc[0],LEG_ACTION_DAMAGE,NULL);char_from_room(&pc[0]);char_to_room(&pc[0],2);death();assert(scalar("SELECT COUNT(*) FROM legendary_awards")==0);
  reset();FIGHTING(&pc[0])=NULL;actions(&pc[1],LEG_ACTION_HEAL,&pc[0]);death();assert(scalar("SELECT COUNT(*) FROM legendary_awards")==0);
  reset();actions(&pc[0],LEG_ACTION_DAMAGE,NULL);boss.legendary_test_encounter=true;death();assert(scalar("SELECT COUNT(*) FROM legendary_awards")==0);
  reset();assert(legendary_balance_load(&pc[0],&b));
  assert(estate_query("UPDATE legendary_characters SET fragments=1999999998,draconic=1999999999,allowance_date=DATE_ADD('1970-01-01',INTERVAL 20000 DAY),fragments_today=95,essences_today=19,catalysts_today=1,version=version+1 WHERE player_id=1"));
  actions(&pc[0],LEG_ACTION_DAMAGE,NULL);death();assert(balance(1,"fragments")==2000000000 && balance(1,"draconic")==2000000000);
  assert(balance(1,"fragments_today")==97 && balance(1,"essences_today")==20 && balance(1,"catalysts")==0);
  assert(estate_query("UPDATE legendary_characters SET fragments=0,draconic=0,version=version+1 WHERE player_id=1"));
  spawn(1);actions(&pc[0],LEG_ACTION_DAMAGE,NULL);death();assert(balance(1,"fragments")==3 && balance(1,"fragments_today")==100);
  spawn(2);actions(&pc[0],LEG_ACTION_DAMAGE,NULL);death();assert(balance(1,"fragments")==3);
  legendary_earnings_test_time+=86400;spawn(2);actions(&pc[0],LEG_ACTION_DAMAGE,NULL);death();assert(balance(1,"fragments")==33); /* zero award did not start cooldown */
  reset();assert(legendary_balance_load(&pc[0],&b));
  assert(estate_query("UPDATE legendary_characters SET fragments=1000,draconic=60,allowance_date=DATE_ADD('1970-01-01',INTERVAL 20000 DAY),essences_today=19,version=version+1 WHERE player_id=1"));
  assert(!legendary_exchange_quote(&pc[0],0,1,2)); /* combat */
  FIGHTING(&pc[0])=NULL;assert(!legendary_exchange_quote(&pc[0],0,1,2));assert(!legendary_exchange_quote(&pc[0],0,0,1));
  x=legendary_exchange_quote(&pc[0],0,1,1);assert(x && legendary_exchange_delta(x)->essences[0]==-3);
  assert(legendary_exchange_confirm(&pc[0],x));assert(!legendary_exchange_confirm(&pc[0],x));legendary_exchange_discard(x);
  assert(balance(1,"draconic")==57 && balance(1,"martial")==1 && balance(1,"essences_today")==20);
  x=legendary_exchange_quote(&pc[0],0,-1,1);assert(x && legendary_exchange_confirm(&pc[0],x));legendary_exchange_discard(x);
  assert(balance(1,"draconic")==37 && balance(1,"fragments")==900 && balance(1,"catalysts_today")==1);
  assert(!legendary_exchange_quote(&pc[0],0,-1,1));assert(!legendary_exchange_quote(&pc[0],0,1,INT_MAX));
  legendary_earnings_test_time+=86400;assert(legendary_balance_load(&pc[0],&b) && !b.essences_today && !b.catalysts_today);
  assert(estate_query("UPDATE legendary_characters SET draconic=10,martial=10,catalysts=0,version=version+1 WHERE player_id=1"));
  x=legendary_exchange_quote(&pc[0],0,-1,1);assert(x && legendary_exchange_delta(x)->essences[0]==-10 && legendary_exchange_delta(x)->essences[1]==-10);
  assert(legendary_exchange_confirm(&pc[0],x));legendary_exchange_discard(x);
  assert(estate_query("UPDATE legendary_characters SET draconic=60,catalysts=2000000000,version=version+1 WHERE player_id=1"));
  assert(!legendary_exchange_quote(&pc[0],0,-1,1));
  x=legendary_exchange_quote(&pc[0],0,1,1);assert(x);legendary_earnings_test_time+=86400;
  assert(!legendary_exchange_confirm(&pc[0],x));legendary_exchange_discard(x);assert(legendary_earnings_recover(&pc[0]));assert(balance(1,"draconic")==60);
  reset();legendary_xp_quest_context(&source,500);legendary_transaction_fault=6;legendary_credit_xp(&pc[0],100,&source);
  assert(balance(1,"infusion_xp")==0 && scalar("SELECT COUNT(*) FROM legendary_awards WHERE state='pending'")==1);
  legendary_transaction_fault=0;assert(legendary_earnings_recover(&pc[0]));assert(legendary_earnings_recover(&pc[0]));assert(balance(1,"infusion_xp")==100);
  legendary_xp_quest_context(&source,501);legendary_transaction_fault=11;legendary_credit_xp(&pc[0],50,&source);
  assert(balance(1,"infusion_xp")==150 && scalar("SELECT COUNT(*) FROM legendary_awards WHERE state='credited'")==1);
  legendary_transaction_fault=0;restart_receipt(&pc[0]);assert(legendary_earnings_recover(&pc[0]));legendary_credit_xp(&pc[0],50,&source);assert(balance(1,"infusion_xp")==150);
  legendary_credit_xp(&pc[1],50,&source);assert(balance(2,"infusion_xp")==50); /* character, never account */
  /* Concurrent accepted kills share the final daily allowance atomically. */
  reset();assert(legendary_balance_load(&pc[0],&b));
  assert(estate_query("UPDATE legendary_characters SET allowance_date=DATE_ADD('1970-01-01',INTERVAL 20000 DAY),fragments_today=85,essences_today=20,catalysts_today=1,version=version+1 WHERE player_id=1"));
  actions(&pc[0],LEG_ACTION_DAMAGE,NULL);legendary_transaction_fault=6;death();
  spawn(1);actions(&pc[0],LEG_ACTION_DAMAGE,NULL);death();legendary_transaction_fault=0;
  restart_receipt(&pc[0]);assert(legendary_earnings_recover(&pc[0]));
  assert(balance(1,"fragments")==15 && balance(1,"fragments_today")==100);
  assert(scalar("SELECT SUM(fragments) FROM legendary_boss_claims")==15);
  /* An uncommitted material source cannot spend tomorrow's allowance. */
  reset();actions(&pc[0],LEG_ACTION_DAMAGE,NULL);legendary_transaction_fault=6;death();
  legendary_transaction_fault=0;legendary_earnings_test_time+=86400;
  restart_receipt(&pc[0]);assert(legendary_earnings_recover(&pc[0]));assert(balance(1,"fragments")==0);
  assert(scalar("SELECT COUNT(*) FROM legendary_boss_claims WHERE fragments>0 OR essences>0 OR catalysts>0")==0);
  /* Zero-effect support and a lone tag must not become participation. */
  reset();for(i=0;i<3;i++){legendary_record_action(&pc[1],&boss,LEG_ACTION_HEAL,&pc[0],0);legendary_earnings_test_time+=6;}
  death();assert(scalar("SELECT COUNT(*) FROM legendary_awards")==0);
  /* The mobile flag is the entire boss-source configuration. */
  reset();REMOVE_BIT_AR(MOB_FLAGS(&boss),MOB_LEGENDARY_BOSS);
  actions(&pc[0],LEG_ACTION_DAMAGE,NULL);death();assert(scalar("SELECT COUNT(*) FROM legendary_awards")==0);
  SET_BIT_AR(MOB_FLAGS(&boss),MOB_LEGENDARY_BOSS);
  /* Competing exchange quotes cannot overspend one character's allowance. */
  reset();FIGHTING(&pc[0])=NULL;assert(legendary_balance_load(&pc[0],&b));
  assert(estate_query("UPDATE legendary_characters SET draconic=120,version=version+1 WHERE player_id=1"));
  {struct legendary_exchange *second;
    x=legendary_exchange_quote(&pc[0],0,1,15);second=legendary_exchange_quote(&pc[0],0,2,15);assert(x&&second);
    assert(legendary_exchange_confirm(&pc[0],x));assert(!legendary_exchange_confirm(&pc[0],second));
    legendary_exchange_discard(x);legendary_exchange_discard(second);assert(legendary_earnings_recover(&pc[0]));
    assert(balance(1,"essences_today")==15 && balance(1,"draconic")==75);
  }
  /* Actual HP healing qualifies; overhealing and resource-only healing do not. */
  reset();legendary_record_action(&pc[0],&boss,LEG_ACTION_DAMAGE,NULL,1);
  for(i=0;i<3;i++) {
    assert(!process_healing(&pc[1],&pc[0],SPELL_CURE_LIGHT,10,0,0));legendary_earnings_test_time+=6;
  }
  death();assert(scalar("SELECT COUNT(*) FROM legendary_awards")==0);
  for(i=0;i<3;i++) {
    GET_HIT(&pc[0])=90;assert(process_healing(&pc[1],&pc[0],SPELL_CURE_LIGHT,10,0,0));
    legendary_earnings_test_time+=6;
  }
  death();assert(balance(2,"fragments")==30 && scalar("SELECT COUNT(*) FROM legendary_awards")==1);
  reset();GET_MAX_MOVE(&pc[0])=100;GET_MOVE(&pc[0])=0;
  for(i=0;i<3;i++) {assert(process_healing(&pc[1],&pc[0],SPELL_CURE_LIGHT,0,10,0));legendary_earnings_test_time+=6;}
  death();assert(scalar("SELECT COUNT(*) FROM legendary_awards")==0);
  /* Execute the actual gain_exp pipeline, not a duplicated XP calculation. */
  reset();CONFIG_MAX_EXP_GAIN=2500;CONFIG_MAX_EXP_LOSS=1000;
  happy_data.exp_rate=100;happy_data.ticks_left=10;GET_EXP(&pc[0])=100000000;
  legendary_xp_kill_context(&source,&boss);
  assert(gain_exp_with_source(&pc[0],1000,GAIN_EXP_MODE_SOLO,&source)==2500);
  assert(GET_EXP(&pc[0])==100002500 && balance(1,"infusion_xp")==2500);
  assert(gain_exp(&pc[0],1000,GAIN_EXP_MODE_SCRIPT)==2500);assert(balance(1,"infusion_xp")==2500);
  assert(gain_exp_with_source(&pc[0],1000,GAIN_EXP_MODE_CRAFT,&source)==750);assert(balance(1,"infusion_xp")==2500);
  assert(gain_exp_with_source(&pc[0],1000,GAIN_EXP_MODE_DAMAGE,&source)==750);assert(balance(1,"infusion_xp")==2500);
  {int modes[]={GAIN_EXP_MODE_DEFAULT,GAIN_EXP_MODE_SCRIPT,GAIN_EXP_MODE_EDRAIN,GAIN_EXP_MODE_DUMP};
    for(i=0;i<4;i++){gain_exp_with_source(&pc[0],1000,modes[i],&source);assert(balance(1,"infusion_xp")==2500);}
    gain_exp(&pc[0],1000,GAIN_EXP_MODE_TRAP);gain_exp(&pc[0],1000,GAIN_EXP_MODE_QUEST);
    assert(balance(1,"infusion_xp")==2500);
    legendary_xp_kill_context(&other,&pc[1]);assert(!other.eligible);
    gain_exp_with_source(&pc[0],1000,GAIN_EXP_MODE_SOLO,&other);assert(balance(1,"infusion_xp")==2500);
  }
  gain_exp(&pc[0],-100,GAIN_EXP_MODE_DEATH);assert(balance(1,"infusion_xp")==2500);
  GET_LEVEL(&pc[0])=29;GET_EXP(&pc[0])=0;advance_during_award=true;
  legendary_xp_quest_context(&source,999);gain_exp_with_source(&pc[0],1000,GAIN_EXP_MODE_QUEST,&source);
  assert(GET_LEVEL(&pc[0])==30 && balance(1,"infusion_xp")==2500);advance_during_award=false;
  /* Accepted source awards survive real process loss before and after ledger commit. */
  {int point,status;pid_t child;char path[512],line[256];FILE *file;
   for(point=6;point<=11;point+=5){
     reset();legendary_xp_quest_context(&source,777);
     child=fork();assert(child>=0);
     if(!child){close(mysql_get_socket(conn));conn=mysql_init(NULL);
       if(!mysql_real_connect(conn,NULL,"root",NULL,"estate_test",0,argv[1],0))_exit(70);
       legendary_transaction_crash=point;legendary_credit_xp(&pc[0],500,&source);_exit(71);}
     assert(waitpid(child,&status,0)==child&&WIFEXITED(status)&&WEXITSTATUS(status)==90+point);
     pc[0].player_specials->legendary_operation[0]=0;
     assert(get_filename(path,sizeof(path),PLR_FILE,GET_NAME(&pc[0])));file=fopen(path,"r");assert(file);
     while(fgets(line,sizeof(line),file)){
       if(!strncmp(line,"LgTx:",5))sscanf(line+5,"%32s",pc[0].player_specials->legendary_operation);
       if(!strncmp(line,"Gold:",5))GET_GOLD(&pc[0])=atoi(line+5);
     }
     fclose(file);assert(legendary_earnings_recover(&pc[0]));assert(legendary_earnings_recover(&pc[0]));
     legendary_credit_xp(&pc[0],500,&source);
     assert(balance(1,"infusion_xp")==500&&scalar("SELECT COUNT(*) FROM legendary_quest_xp_claims")==1);
   }
   puts("PASS: accepted XP/quest claim recovery after actual process interruption before/after commit");}
  puts("PASS: XP provenance/quest days/replay/overflow, healer/tank/pet participation, clones/cooldowns, stale/absent/dead/linkdead/group exclusion, partial caps, UTC reset, exchanges and interrupted grant recovery");
  mysql_close(conn);return 0;
}
