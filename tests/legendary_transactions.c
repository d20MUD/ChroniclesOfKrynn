#define main estate_baseline_main
#include "estate_integration.c"
#undef main
#include "legendary_transaction.h"
#include "legendary_identity.h"
#include "durable_file.h"
#undef assert
#define assert(test) do { if(!(test)){fprintf(stderr,"transaction assertion line %d: %s\n",__LINE__,#test);abort();} } while(0)
int MIN(int a,int b){return a<b?a:b;}
extern struct index_data *obj_index;
extern obj_rnum top_of_objt;
obj_save_data *objsave_parse_objects_db(char *,room_vnum);
static void reset(struct char_data *ch)
{
  while(ch->carrying)extract_obj(ch->carrying);
  assert(estate_query("DELETE FROM legendary_operations"));
  assert(estate_query("DELETE FROM legendary_channels"));
  assert(estate_query("DELETE FROM legendary_items"));
  assert(estate_query("DELETE FROM legendary_characters"));
  assert(estate_query("DELETE FROM estate_deliveries"));
  assert(estate_query("DELETE FROM player_save_objs"));
  assert(estate_query("INSERT INTO legendary_characters(player_id,infusion_xp,fragments) VALUES(1,1000000,1000)"));
  ch->player_specials->legendary_operation[0]=0;
  ch->player_specials->estate_debit_receipt=0;
  GET_GOLD(ch)=500000;
  legendary_transaction_fault=durable_file_fault=0;
  assert(save_char_checked(ch,0));
}
static bool new_purchase_mode;
static struct legendary_quote *purchase(struct char_data *ch)
{
  struct legendary_item_record r;
  struct legendary_transaction_plan p;
  struct obj_data *obj=item("transaction blade");
  memset(&r,0,sizeof(r));memset(&p,0,sizeof(p));
  r.owner=1;r.prototype=500;r.slot=0;
  if(!new_purchase_mode){assert(legendary_uuid_generate(r.uuid));assert(legendary_store_insert(&r));}
  obj->item_number=0;GET_OBJ_BOUND_ID(obj)=new_purchase_mode?0:1;
  memcpy(obj->legendary_uuid,r.uuid,33);obj_to_char(obj,ch);
  assert(Crash_crashsave_checked(ch));
  r.channels[0].present=true;r.channels[0].rank=1;r.channels[0].path=1;r.channels[0].foundation=1;
  assert(legendary_cost_between(0,1,&r.channels[0].paid));
  p.kind=LEG_OP_PURCHASE;p.item=&r;p.channel=0;p.object=obj;
  p.delta.xp=-100000;p.delta.gold=-25000;p.delta.fragments=-25;
  return legendary_transaction_quote(ch,&p);
}
/* Reload just the persistent fields, as a fresh process would. In particular,
 * do not preserve an uncheckpointed in-memory receipt. */
static void restart_player(struct char_data *ch)
{
  FILE *f=fopen("owner.plr","r");char line[256];
  GET_GOLD(ch)=0;ch->player_specials->estate_debit_receipt=0;ch->player_specials->legendary_operation[0]=0;
  assert(f);
  while(fgets(line,sizeof(line),f)) {
    if(!strncmp(line,"Gold:",5))GET_GOLD(ch)=atoi(line+5);
    if(!strncmp(line,"EDbt:",5))ch->player_specials->estate_debit_receipt=strtoull(line+5,NULL,10);
    if(!strncmp(line,"LgTx:",5))sscanf(line+5,"%32s",ch->player_specials->legendary_operation);
  }
  fclose(f);
  {
    obj_save_data *list,*next;
    while(ch->carrying)extract_obj(ch->carrying);
    list=objsave_parse_objects_db(GET_NAME(ch),NOWHERE);
    for(;list;list=next){next=list->next;obj_to_char(list->obj,ch);free(list);}
  }
}
static void outcome(struct char_data *ch,bool committed)
{
  assert(legendary_transaction_recover(ch));
  assert(legendary_transaction_recover(ch));
  assert(!legendary_transaction_locked(ch));
  assert(GET_GOLD(ch)==(committed?475000:500000));
  assert(scalar("SELECT infusion_xp FROM legendary_characters WHERE player_id=1")== (committed?900000:1000000));
  assert(scalar("SELECT fragments FROM legendary_characters WHERE player_id=1")== (committed?975:1000));
  assert(scalar("SELECT COUNT(*) FROM legendary_channels")== (committed?1:0));
  assert(scalar("SELECT COUNT(*) FROM legendary_items WHERE state='active'")== (committed?1:0));
  assert(scalar("SELECT COUNT(*) FROM estate_deliveries")== (committed?1:0));
  if(new_purchase_mode && !committed && ch->carrying) {
    assert(!*ch->carrying->legendary_uuid);assert(GET_OBJ_BOUND_ID(ch->carrying)==0);
  }
  if(committed)assert(scalar("SELECT COUNT(*) FROM estate_deliveries WHERE delivered=1")==1);
}
static bool claim_once(struct char_data *ch,const char *id,void *context)
{
  (void)ch;(void)id;(void)context;
  return estate_query("INSERT INTO test_claims(id) VALUES(1)");
}
#include <sys/wait.h>
static void crash_child(struct char_data *ch,struct legendary_quote *q,int point,const char *socket)
{
  pid_t pid=fork();int status;assert(pid>=0);
  if(!pid){
    close(mysql_get_socket(conn));conn=mysql_init(NULL);
    if(!mysql_real_connect(conn,NULL,"root",NULL,"estate_test",0,socket,0))_exit(70);
    if(point==12||point==13){legendary_transaction_fault=4;(void)legendary_transaction_confirm(ch,q);legendary_transaction_fault=0;
      legendary_transaction_crash=point;(void)legendary_transaction_recover(ch);}
    else {legendary_transaction_crash=point;(void)legendary_transaction_confirm(ch,q);}
    _exit(71);
  }
  assert(waitpid(pid,&status,0)==pid&&WIFEXITED(status)&&WEXITSTATUS(status)==90+point);
}
int main(int argc,char **argv)
{
  struct char_data ch;
  struct legendary_quote *q,*other;
  struct legendary_transaction_plan p;
  int fault,mode;
  assert(argc==2);
  conn=mysql_init(NULL);assert(mysql_real_connect(conn,NULL,"root",NULL,"estate_test",0,argv[1],0));
  assert(estate_init() && legendary_schema_ready());
  assert(estate_query("CREATE TABLE player_save_objs(idnum INT PRIMARY KEY AUTO_INCREMENT,name VARCHAR(64),serialized_obj LONGTEXT,creation_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP)"));
  assert(estate_query("CREATE TABLE player_data(name VARCHAR(64) PRIMARY KEY,obj_save_header TEXT)"));
  assert(estate_query("INSERT INTO player_data(name) VALUES('owner')"));
  world=calloc(2,sizeof(*world));top_of_world=1;world[1].number=2200;
  obj_index=calloc(1,sizeof(*obj_index));obj_index[0].vnum=500;top_of_objt=0;
  init_player(&ch,"owner",1);
  for(mode=0;mode<2;mode++) {
  new_purchase_mode=mode!=0;
  for(fault=1;fault<=20;fault++) {
    reset(&ch);q=purchase(&ch);assert(q);
    if(fault==12||fault==13){legendary_transaction_fault=4;assert(!legendary_transaction_confirm(&ch,q));
      legendary_transaction_fault=fault;assert(!legendary_transaction_recover(&ch));}
    else {legendary_transaction_fault=fault;assert(!legendary_transaction_confirm(&ch,q));}
    legendary_transaction_fault=0;
    /* Stops before preparation do not persist a marker; all others do. */
    restart_player(&ch);
    outcome(&ch,fault==11 || fault>=14);
    assert(!legendary_transaction_confirm(&ch,q));legendary_transaction_discard(q);
  }
  }
  new_purchase_mode=false;
  for(fault=1;fault<=5;fault++) {
    reset(&ch);q=purchase(&ch);assert(q);
    legendary_transaction_fault=11;assert(!legendary_transaction_confirm(&ch,q));legendary_transaction_fault=0;
    durable_file_fault=fault;assert(!legendary_transaction_recover(&ch));
    /* Test both retry within process and an actual receipt reload. */
    durable_file_fault=0;restart_player(&ch);outcome(&ch,true);legendary_transaction_discard(q);
  }
  reset(&ch);q=purchase(&ch);assert(q);
  assert(estate_query("UPDATE legendary_characters SET version=version+1 WHERE player_id=1"));
  assert(!legendary_transaction_confirm(&ch,q));outcome(&ch,false);legendary_transaction_discard(q);
  reset(&ch);q=purchase(&ch);other=purchase(&ch);assert(q&&other);
  assert(legendary_transaction_confirm(&ch,q));assert(!legendary_transaction_confirm(&ch,q));
  assert(!legendary_transaction_confirm(&ch,other));assert(legendary_transaction_recover(&ch));
  assert(GET_GOLD(&ch)==475000);assert(scalar("SELECT COUNT(*) FROM legendary_channels")==1);
  legendary_transaction_discard(q);legendary_transaction_discard(other);
  /* The generic backend supports trusted grants and exchanges, with no gold journal when zero. */
  reset(&ch);memset(&p,0,sizeof(p));p.kind=LEG_OP_GRANT;p.delta.essences[0]=3;
  q=legendary_transaction_quote(&ch,&p);assert(q&&legendary_transaction_confirm(&ch,q));legendary_transaction_discard(q);
  p.kind=LEG_OP_EXCHANGE;p.delta.essences[0]=-3;p.delta.essences[1]=1;
  q=legendary_transaction_quote(&ch,&p);assert(q&&legendary_transaction_confirm(&ch,q));legendary_transaction_discard(q);
  assert(scalar("SELECT draconic FROM legendary_characters WHERE player_id=1")==0);
  assert(scalar("SELECT martial FROM legendary_characters WHERE player_id=1")==1);
  assert(scalar("SELECT COUNT(*) FROM estate_deliveries")==0);
  p.delta.fragments=INT64_MAX;assert(!legendary_transaction_quote(&ch,&p));
  reset(&ch);q=purchase(&ch);assert(q);legendary_transaction_fault=11;
  assert(!legendary_transaction_confirm(&ch,q));legendary_transaction_fault=0;
  assert(estate_query("CREATE TRIGGER reject_legendary_ack BEFORE UPDATE ON estate_deliveries FOR EACH ROW BEGIN IF NEW.delivered=1 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='lost receipt acknowledgment'; END IF; END"));
  assert(!legendary_transaction_recover(&ch));assert(!legendary_transaction_recover(&ch));assert(GET_GOLD(&ch)==475000);
  assert(estate_query("DROP TRIGGER reject_legendary_ack"));restart_player(&ch);outcome(&ch,true);
  legendary_transaction_discard(q);
  /* Currency checkpoint failures independently of object checkpoints. */
  for(fault=1;fault<=5;fault++) {
    reset(&ch);
    assert(estate_queue_money(1,100));
    durable_file_fault=fault;estate_recover(&ch);
    assert(GET_GOLD(&ch)==499900);
    durable_file_fault=0;restart_player(&ch);estate_recover(&ch);estate_recover(&ch);
    assert(GET_GOLD(&ch)==499900);
    assert(scalar("SELECT COUNT(*) FROM estate_deliveries WHERE delivered=0")==0);
  }
  reset(&ch);q=purchase(&ch);assert(q);legendary_transaction_clock_offset=60;
  assert(!legendary_transaction_confirm(&ch,q));
  assert(scalar("SELECT COUNT(*) FROM legendary_operations")==0);
  legendary_transaction_clock_offset=0;legendary_transaction_discard(q);
  /* Fail each SQL mutation. Pre-commit errors leave every balance unchanged. */
  {
    const char *tables[]={"legendary_operations","legendary_characters","legendary_channels",
      "legendary_items","estate_deliveries","player_save_objs"};
    const char *verbs[]={"INSERT","UPDATE","INSERT","UPDATE","INSERT","INSERT"};
    int n;
    for(n=0;n<6;n++) {
      reset(&ch);q=purchase(&ch);assert(q);
      assert(estate_query("CREATE TRIGGER reject_write BEFORE %s ON %s FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='injected write failure'",verbs[n],tables[n]));
      assert(!legendary_transaction_confirm(&ch,q));
      assert(estate_query("DROP TRIGGER reject_write"));
      restart_player(&ch);outcome(&ch,false);legendary_transaction_discard(q);
    }
  }
  reset(&ch);q=purchase(&ch);assert(q);legendary_transaction_fault=11;
  assert(!legendary_transaction_confirm(&ch,q));legendary_transaction_fault=0;
  mysql_available=false;assert(!legendary_transaction_recover(&ch));assert(legendary_transaction_locked(&ch));
  mysql_available=true;restart_player(&ch);outcome(&ch,true);legendary_transaction_discard(q);
  reset(&ch);
  assert(estate_query("CREATE TABLE test_claims(id INT PRIMARY KEY) ENGINE=InnoDB"));
  memset(&p,0,sizeof(p));p.kind=LEG_OP_GRANT;p.delta.fragments=1;p.participate=claim_once;
  q=legendary_transaction_quote(&ch,&p);assert(q);
  legendary_transaction_fault=6;assert(!legendary_transaction_confirm(&ch,q));legendary_transaction_fault=0;
  assert(scalar("SELECT COUNT(*) FROM test_claims")==0);
  assert(legendary_transaction_recover(&ch));legendary_transaction_discard(q);
  q=legendary_transaction_quote(&ch,&p);assert(q&&legendary_transaction_confirm(&ch,q));legendary_transaction_discard(q);
  q=legendary_transaction_quote(&ch,&p);assert(q&&!legendary_transaction_confirm(&ch,q));
  assert(legendary_transaction_recover(&ch));legendary_transaction_discard(q);
  assert(scalar("SELECT fragments FROM legendary_characters WHERE player_id=1")==1001);
  /* Actual process exits lose every child heap/receipt cache and SQL session. */
  for(mode=0;mode<2;mode++)for(fault=1;fault<=20;fault++){
    new_purchase_mode=mode!=0;reset(&ch);q=purchase(&ch);assert(q);
    crash_child(&ch,q,fault,argv[1]);restart_player(&ch);outcome(&ch,fault==11||fault>=14);
    legendary_transaction_discard(q);
  }
  new_purchase_mode=false;
  for(fault=6;fault<=11;fault+=5){
    reset(&ch);assert(estate_query("DELETE FROM test_claims"));memset(&p,0,sizeof(p));
    p.kind=LEG_OP_GRANT;p.delta.fragments=1;p.participate=claim_once;
    q=legendary_transaction_quote(&ch,&p);assert(q);crash_child(&ch,q,fault,argv[1]);
    restart_player(&ch);assert(legendary_transaction_recover(&ch));assert(legendary_transaction_recover(&ch));
    assert(scalar("SELECT COUNT(*) FROM test_claims")==(fault==11));
    assert(scalar("SELECT fragments FROM legendary_characters WHERE player_id=1")==1000+(fault==11));
    legendary_transaction_discard(q);
  }
  puts("PASS: real process interruption at all 20 purchase checkpoints (ordinary/new UUID) and atomic grant claims");
  puts("PASS: transaction boundary faults, durable checkpoint faults, restart/replay, stale/concurrent quotes, grants/exchanges, overflow and lost gold acknowledgment");
  mysql_close(conn);return 0;
}
