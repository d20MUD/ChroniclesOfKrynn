#include <sys/wait.h>
#include "durable_file.h"
#include <time.h>
#include "account.h"
void test_real_corpse_get(struct char_data *,struct obj_data *,struct obj_data *,int);
int get_otrigger(struct obj_data *o,struct char_data *ch){return 1;}
char *get_name_by_id(long id){return NULL;}
char *CAP(char *text){return text;}
int increase_gold(struct char_data *ch,int amount){GET_GOLD(ch)+=amount;return amount;}
void do_clan_tax_losses(struct char_data *ch,int amount){}
void auto_store_obj(struct char_data *ch,struct obj_data *o){}
void auto_sort_obj(struct char_data *ch,struct obj_data *o){}
void get_check_craft_material(struct char_data *ch,struct obj_data *o){}
void check_room_lighting_special(room_rnum room,struct char_data *ch,struct obj_data *o,bool add){}
static void security_cases(struct char_data *ch,const char *socket)
{
  struct char_data ledger,enemy,staff;struct obj_data *o,*chest,*restored;struct legendary_attack attack;
  struct account_data account={0};struct legendary_item_record record;
  struct legendary_transaction_plan plan={0};struct legendary_quote *quote;
  char input[4096],uuid[33],journal[32768];const char *bad[]={"-1","+1","2147483648","9223372036854775808","1junk","NaN","0x10","999999999999999999999999999999999999999","1;DROP TABLE legendary_items"};
  long count,gold,fragments,q,entry_count;size_t bytes;int i,j,status,extra,actual;pid_t child;FILE *file;
  unsigned seed=1701;struct timespec begin,end;double elapsed;
  /* Malformed command choices and administrative verbs cannot mint grants. */
  o=signature_setup(ch,LEG_S09,5,0);free(o->name);o->name=strdup("phaseblade");
  count=scalar("SELECT COUNT(*) FROM legendary_operations");gold=GET_GOLD(ch);fragments=scalar("SELECT fragments FROM legendary_characters WHERE player_id=1");
  for(i=0;i<(int)(sizeof(bad)/sizeof(bad[0]));i++){
    snprintf(input,sizeof(input),"prioritize phaseblade primary %s",bad[i]);command(ch,input);assert(!strstr(output,"infuse confirm "));
    snprintf(input,sizeof(input),"select phaseblade primary technique %s",bad[i]);command(ch,input);assert(!strstr(output,"infuse confirm "));
    snprintf(input,sizeof(input),"confirm %s",bad[i]);command(ch,input);assert(!strstr(output,"confirmed and saved"));
  }
  for(i=0;i<512;i++){
    strcpy(input,"modify phaseblade primary ");
    for(j=strlen(input);j<40+i%1000;j++){seed=seed*1664525u+1013904223u;input[j]=(char)(1+(seed>>16)%255);}
    input[j]=0;command(ch,input);assert(!strstr(output,"infuse confirm "));legendary_infuse_cleanup(ch);
  }
  memset(input,'x',sizeof(input)-1);input[sizeof(input)-1]=0;command(ch,input);assert(!strstr(output,"infuse confirm "));
  {const char *verbs[]={"grant fragments 999","admin grant essence 999","registry add 500","sql DROP TABLE legendary_items"};
   for(i=0;i<4;i++){command(ch,verbs[i]);assert(!strstr(output,"infuse confirm "));}}
  assert(scalar("SELECT COUNT(*) FROM legendary_operations")==count&&GET_GOLD(ch)==gold&&scalar("SELECT fragments FROM legendary_characters WHERE player_id=1")==fragments);
  /* Measure the actual native post-defense combat finalizer, not a copied adapter. */
  init_player(&enemy,"securityenemy",902);SET_BIT_AR(MOB_FLAGS(&enemy),MOB_ISNPC);GET_POS(&enemy)=POS_STANDING;GET_LEVEL(&enemy)=20;GET_HIT(&enemy)=10000;
  assert(legendary_activate(ch,LEG_S09,&enemy));legendary_earnings_maintenance();
  q=queries();clock_gettime(CLOCK_MONOTONIC,&begin);
  for(i=0;i<1000;i++){
    legendary_technique_attack(ch,&enemy,o,&attack);
    actual=legendary_combat_packet(ch,&enemy,10,TYPE_HIT,DAM_SLICE,0,&attack,&extra);assert(actual>=10&&actual<=40);
    (void)legendary_budget_available(ch,LEG_BUDGET_OFFENSE);
    legendary_record_action(ch,&enemy,LEG_ACTION_DAMAGE,NULL,actual);
  }
  clock_gettime(CLOCK_MONOTONIC,&end);assert(queries()-q==1);
  elapsed=(end.tv_sec-begin.tv_sec)+(end.tv_nsec-begin.tv_nsec)/1e9;
  printf("PERF: 1000 native legendary combat packets: 0 SQL calls, %.3f ms total (includes durable actual-benefit journal writes)\n",elapsed*1000);
  /* A copyover exec must release the old image's local budget writer lock. */
  child=fork();assert(child>=0);
  if(!child){
    close(mysql_get_socket(conn));conn=mysql_init(NULL);
    if(!mysql_real_connect(conn,NULL,"root",NULL,"estate_test",0,socket,0))_exit(70);
    init_player(&ledger,"copyoverledger",901);
    if(!legendary_budget_prime(&ledger))_exit(71);
    execl("/proc/self/exe","legendary-commands-test","--budget-copyover",socket,(char *)NULL);
    _exit(72);
  }
  assert(waitpid(child,&status,0)==child&&WIFEXITED(status)&&WEXITSTATUS(status)==0);
  /* Cold login imports SQL+journal once; hot lookups are query-free. */
  init_player(&ledger,"ledger",900);GET_POS(&ledger)=POS_STANDING;legendary_budget_unload(&ledger);
  q=queries();assert(legendary_budget_prime(&ledger));printf("PERF: cold budget login: %ld SQL calls\n",queries()-q-1);
  assert(legendary_budget_debit(&ledger,LEG_BUDGET_OFFENSE,10)==10);legendary_budget_unload(&ledger);
  child=fork();assert(child>=0);
  if(!child){close(mysql_get_socket(conn));conn=mysql_init(NULL);if(!mysql_real_connect(conn,NULL,"root",NULL,"estate_test",0,socket,0))_exit(70);
    if(!legendary_budget_prime(&ledger)||legendary_budget_debit(&ledger,LEG_BUDGET_OFFENSE,10)!=10)_exit(71);
    _exit(88);}
  assert(waitpid(child,&status,0)==child&&WIFEXITED(status)&&WEXITSTATUS(status)==88);
  assert(legendary_budget_prime(&ledger)&&legendary_budget_available(&ledger,LEG_BUDGET_OFFENSE)==40);
  /* Another process cannot concurrently use the same player's local authority. */
  child=fork();assert(child>=0);
  if(!child){legendary_budget_unload(&ledger);close(mysql_get_socket(conn));conn=mysql_init(NULL);if(!mysql_real_connect(conn,NULL,"root",NULL,"estate_test",0,socket,0))_exit(70);
    _exit(legendary_budget_prime(&ledger)?72:0);}
  assert(waitpid(child,&status,0)==child&&WIFEXITED(status)&&WEXITSTATUS(status)==0);
  assert(estate_query("CREATE TRIGGER reject_budget BEFORE INSERT ON legendary_budgets FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='budget SQL unavailable'"));
  assert(!legendary_budget_flush());legendary_budget_unload(&ledger);assert(legendary_budget_prime(&ledger));
  assert(legendary_budget_available(&ledger,LEG_BUDGET_OFFENSE)==40);
  mysql_available=false;q=0;assert(legendary_budget_debit(&ledger,LEG_BUDGET_OFFENSE,10)==10);mysql_available=true;
  assert(estate_query("DROP TRIGGER reject_budget"));assert(legendary_budget_flush());
  entry_count=scalar("SELECT COUNT(*) FROM legendary_budgets WHERE player_id=900");assert(entry_count==3);
  assert(legendary_budget_flush()&&scalar("SELECT COUNT(*) FROM legendary_budgets WHERE player_id=900")==entry_count);
  for(i=1;i<=5;i++){
    legendary_effects_test_time+=1000;durable_file_fault=i;
    assert(!legendary_budget_debit(&ledger,LEG_BUDGET_PREVENTION,10));durable_file_fault=0;
    assert(legendary_budget_flush());legendary_budget_unload(&ledger);assert(legendary_budget_prime(&ledger));
    assert(legendary_budget_available(&ledger,LEG_BUDGET_PREVENTION)==90);
  }
  /* Malformed/truncated disk state fails closed instead of resetting allowance. */
  legendary_budget_unload(&ledger);file=fopen("etc/legendary-budgets/900","r");assert(file);bytes=fread(journal,1,sizeof(journal),file);assert(!ferror(file));fclose(file);
  assert(durable_file_replace("etc/legendary-budgets/900","LGB1 900\nbad\n",13));assert(!legendary_budget_prime(&ledger));
  i=(int)bytes-1;while(i>0&&journal[i-1]!='\n')i--;
  assert(durable_file_replace("etc/legendary-budgets/900",journal,i));assert(!legendary_budget_prime(&ledger));
  assert(durable_file_replace("etc/legendary-budgets/900",journal,bytes));assert(legendary_budget_prime(&ledger));
  legendary_effects_test_time+=1000;
  q=queries();clock_gettime(CLOCK_MONOTONIC,&begin);
  for(i=0;i<4;i++)for(j=0;j<(i==0?60:i==3?20:100);j++)assert(legendary_budget_debit(&ledger,i,1)==1);
  clock_gettime(CLOCK_MONOTONIC,&end);assert(queries()-q==1);
  elapsed=(end.tv_sec-begin.tv_sec)+(end.tv_nsec-begin.tv_nsec)/1e9;
  printf("PERF: 280 durable one-point budget debits: 0 SQL calls, %.3f ms total\n",elapsed*1000);
  assert(!legendary_budget_debit(&ledger,0,1));legendary_budget_unload(&ledger);assert(legendary_budget_prime(&ledger));
  assert(!legendary_budget_available(&ledger,0)&&!legendary_budget_available(&ledger,1)&&!legendary_budget_available(&ledger,2)&&!legendary_budget_available(&ledger,3));
  /* Real house/chest persistence retains identity and permits only the owner. */
  technique_clean(ch);test_remove(ch,WEAR_WIELD_1);memcpy(uuid,o->legendary_uuid,33);
  init_player(&staff,"securitystaff",904);GET_LEVEL(&staff)=LVL_GRSTAFF;
  assert(housing_admin(&staff,"neighborhood add 2200 Security Neighborhood"));house(ch,"buy Security Home");house(ch,"enter 1");house(ch,"chest buy");
  chest=world[IN_ROOM(ch)].contents;assert(chest&&housing_put(ch,o,chest));assert(scalar("SELECT COUNT(*) FROM housing_chest_items")==1);
  assert(housing_get(ch,chest,"phaseblade",1));restored=NULL;
  for(o=ch->carrying;o;o=o->next_content)if(!strcmp(o->legendary_uuid,uuid))restored=o;
  assert(restored&&legendary_object_verify(restored));house(ch,"leave");
  /* Actual corpse retrieval checks immutable ownership before ordinary corpse rules. */
  {struct obj_data *corpse=item("corpse owner");GET_OBJ_TYPE(corpse)=ITEM_CONTAINER;
   obj_from_char(restored);obj_to_obj(restored,corpse);GET_OBJ_BOUND_ID(corpse)=NOBODY;
   test_real_corpse_get(&staff,restored,corpse,FIND_OBJ_INV);assert(restored->in_obj==corpse);
   test_real_corpse_get(ch,restored,corpse,FIND_OBJ_INV);assert(restored->carried_by==ch&&!corpse->contains);
   extract_obj(corpse);}
  /* Auction and stall entry points refuse awakened items even if bound bits are damaged. */
  GET_OBJ_BOUND_ID(restored)=0;SET_BIT_AR(GET_OBJ_EXTRA(restored),ITEM_IDENTIFIED);
  count=scalar("SELECT COUNT(*) FROM auction_listings");auction(ch,"sell phaseblade 100");assert(scalar("SELECT COUNT(*) FROM auction_listings")==count);
  account.id=1;ch->desc->account=&account;stall(&staff,"admin add 2200 1");stall(ch,"rent 1");
  count=scalar("SELECT COUNT(*) FROM player_shop_listings");stall(ch,"sell phaseblade 100");assert(scalar("SELECT COUNT(*) FROM player_shop_listings")==count);
  ch->desc->account=NULL;GET_OBJ_BOUND_ID(restored)=GET_IDNUM(ch);
  /* Deletion refuses unsettled economics, then irreversibly tombstones gear and wallet. */
  plan.kind=LEG_OP_GRANT;plan.delta.fragments=1;quote=legendary_transaction_quote(ch,&plan);assert(quote);
  legendary_transaction_fault=3;assert(!legendary_transaction_confirm(ch,quote));legendary_transaction_fault=0;
  assert(!legendary_retire_owner(GET_IDNUM(ch),GET_NAME(ch)));assert(legendary_transaction_recover(ch));legendary_transaction_discard(quote);
  assert(legendary_retire_owner(GET_IDNUM(ch),GET_NAME(ch))&&legendary_retire_owner(GET_IDNUM(ch),GET_NAME(ch)));
  assert(legendary_store_load(uuid,&record)==LEG_LOAD_OK&&record.state==LEG_RECORD_DELETED);
  assert(!legendary_object_verify(restored)&&scalar("SELECT infusion_xp+fragments+catalysts+draconic+martial+arcane+spiritual+primal+shadow FROM legendary_characters WHERE player_id=1")==0);
  legendary_effects_free(&ledger);char_from_room(&ledger);legendary_effects_free(&enemy);char_from_room(&enemy);char_from_room(&staff);
  puts("legendary Phase 13: input fuzz, native combat SQL metrics, process journal restart/locking/faults, full budget bounds, chest/commerce and deletion PASS");
}
