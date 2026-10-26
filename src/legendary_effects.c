/* Shared activation, ephemeral source leases and durable effective-event debits.
 * Combat effects are intentionally not part of the economic replay journal. */
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "fight.h"
#include "spells.h"
#include "missions.h"
#include "shop.h"
#include "estate.h"
#include "legendary_effects.h"
#include "legendary_equipment.h"
#include "legendary_identity.h"
#include "legendary_store.h"
#include "legendary_earnings.h"
#include "legendary_techniques.h"
#include "dg_event.h"
#include "actions.h"
#include <errno.h>
#include <limits.h>
#include "durable_file.h"
#include <sys/file.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <openssl/sha.h>

#ifdef LEGENDARY_EFFECTS_TEST
int64_t legendary_effects_test_time;
#endif
static int64_t clock_now(void)
{
#ifdef LEGENDARY_EFFECTS_TEST
  if(legendary_effects_test_time)return legendary_effects_test_time;
#endif
  return (int64_t)time(NULL);
}
int64_t legendary_effects_now(void){return clock_now();}
static bool number(const char *s,int64_t *v)
{
  char *end;unsigned long long n;
  if(!s||!*s||*s<'0'||*s>'9')return false;
  errno=0;n=strtoull(s,&end,10);
  if(errno||*end||n>INT64_MAX)return false;
  *v=n;return true;
}
static bool scalar(const char *query,int64_t *n)
{
  MYSQL_RES *r=estate_select("%s",query);MYSQL_ROW row;bool ok;
  if(!r)return false;
  row=mysql_fetch_row(r);ok=row&&number(row[0],n);mysql_free_result(r);return ok;
}
static bool lock_player(struct char_data *ch)
{
  MYSQL_RES *r;MYSQL_ROW row;bool ok;
  if(!estate_query("INSERT IGNORE INTO legendary_characters(player_id) VALUES(%ld)",GET_IDNUM(ch)))return false;
  r=estate_select("SELECT player_id FROM legendary_characters WHERE player_id=%ld FOR UPDATE",GET_IDNUM(ch));
  if(!r)return false;
  row=mysql_fetch_row(r);ok=row!=NULL;mysql_free_result(r);return ok;
}
static int budget_limit(int kind)
{
  switch(kind){case LEG_BUDGET_OFFENSE:return legendary_rules.offense_limit;
    case LEG_BUDGET_PREVENTION:return legendary_rules.prevention_limit;
    case LEG_BUDGET_HEALING:return legendary_rules.healing_limit;
    case LEG_BUDGET_RESOURCE:return legendary_rules.resource_limit;default:return 0;}
}
struct npc_debit {struct npc_debit *next;struct char_data *ch;int kind,amount;int64_t at;};
static struct npc_debit *npc_debits;
static void prune_npc(void)
{
  struct npc_debit **at=&npc_debits,*p;int window;
  while((p=*at)){
    window=p->kind==LEG_BUDGET_RESOURCE?legendary_rules.resource_seconds:legendary_rules.rolling_seconds;
    if(p->at<=clock_now()-window){*at=p->next;free(p);}else at=&p->next;
  }
}
/* The local journal is authoritative until SQL acknowledges its stable IDs.
 * The game has one writer per player, enforced across processes by flock.
 * Every benefit is preceded by an atomic fsync checkpoint; combat never opens
 * a database transaction. SQL is a batched mirror maintained by heartbeat. */
#define BUDGET_ROWS 280
struct budget_row {char id[33];int kind,amount;int64_t at;bool synced;};
struct budget_cache {struct budget_cache *next;struct char_data *holder;long player;int fd,count;bool blocked;
  struct budget_row rows[BUDGET_ROWS];};
static struct budget_cache *budget_caches;
static int budget_window(int kind)
{return kind==LEG_BUDGET_RESOURCE?legendary_rules.resource_seconds:legendary_rules.rolling_seconds;}
static struct budget_cache *budget_find(long player)
{struct budget_cache *c;for(c=budget_caches;c;c=c->next)if(c->player==player)return c;return NULL;}
static void budget_prune(struct budget_cache *c)
{int i,n=0;for(i=0;i<c->count;i++)if(c->rows[i].at>clock_now()-budget_window(c->rows[i].kind))c->rows[n++]=c->rows[i];c->count=n;}
static bool budget_directory(const char *path,const char *parent)
{
  int fd;if(mkdir(path,0700)){if(errno!=EEXIST)return false;return true;}
  fd=open(parent,O_RDONLY|O_DIRECTORY);if(fd<0)return false;
  if(fsync(fd)){close(fd);return false;}close(fd);return true;
}
static void budget_hash(const void *data,size_t size,char hex[65])
{
  unsigned char digest[SHA256_DIGEST_LENGTH];int i;
  SHA256(data,size,digest);
  for(i=0;i<SHA256_DIGEST_LENGTH;i++)snprintf(hex+2*i,3,"%02x",digest[i]);
}
static bool budget_checkpoint(struct budget_cache *c)
{
  char path[128],data[32768],header[160],hash[65];int i,n=0,written,head;
  for(i=0;i<c->count;i++){
    written=snprintf(data+n,sizeof(data)-n,"%s %lld %d %d\n",c->rows[i].id,(long long)c->rows[i].at,c->rows[i].kind,c->rows[i].amount);
    if(written<0||written>=(int)sizeof(data)-n)return false;
    n+=written;
  }
  /* An empty journal has no serialized rows in data yet. */
  budget_hash(n ? data : "",n,hash);
  head=snprintf(header,sizeof(header),"LGB1 %ld %d %s\n",c->player,c->count,hash);
  if(head<=0||head>=(int)sizeof(header)||head+n>(int)sizeof(data))return false;
  memmove(data+head,data,n);memcpy(data,header,head);
  snprintf(path,sizeof(path),"etc/legendary-budgets/%ld",c->player);
  return durable_file_replace(path,data,n+head);
}
static bool budget_merge(struct budget_cache *c,const char *id,int kind,int amount,int64_t at,bool synced)
{
  struct budget_row *r;int i;
  if(!legendary_uuid_valid(id)||kind<0||kind>3||amount<=0||amount>budget_limit(kind)||at<0)return false;
  if(at<=clock_now()-budget_window(kind))return true;
  for(i=0;i<c->count;i++)if(!strcmp(c->rows[i].id,id)){
    r=&c->rows[i];if(r->kind!=kind||r->amount!=amount||r->at!=at)return false;
    r->synced|=synced;return true;
  }
  if(c->count==BUDGET_ROWS)return false;
  r=&c->rows[c->count++];memcpy(r->id,id,33);r->kind=kind;r->amount=amount;r->at=at;r->synced=synced;return true;
}
bool legendary_budget_prime(struct char_data *ch)
{
  struct budget_cache *c;MYSQL_RES *res=NULL;MYSQL_ROW row;FILE *file=NULL;
  char path[128],line[160],id[33],tokens[3][24],magic[8],expected_hash[65],actual_hash[65],raw[32768],extra;int fd=-1,i,raw_size=0,rows=0;
  int64_t expected_rows;
  int64_t parsed_player;int64_t values[3],used[4]={0};struct stat st;
  if(!ch||IS_NPC(ch)||GET_IDNUM(ch)<=0)return false;
  if((c=budget_find(GET_IDNUM(ch))))return !c->blocked;
  if(!legendary_schema_ready()||!budget_directory("etc",".")||!budget_directory("etc/legendary-budgets","etc"))return false;
  c=calloc(1,sizeof(*c));if(!c)return false;c->player=GET_IDNUM(ch);c->holder=ch;c->fd=-1;
  snprintf(path,sizeof(path),"etc/legendary-budgets/%ld.lock",c->player);
  /* Copyover preserves sockets across exec, but local writer locks must close.
   * Otherwise the fresh process image inherits a lock with no matching cache
   * and blocks its own player's next login. */
  c->fd=open(path,O_RDWR|O_CREAT|O_NOFOLLOW|O_CLOEXEC,0600);
  if(c->fd<0){log("SYSERR: Cannot open legendary budget lock for player %ld: %s",c->player,strerror(errno));goto fail;}
  if(flock(c->fd,LOCK_EX|LOCK_NB)){
    log("SYSERR: Cannot acquire legendary budget lock for player %ld: %s",c->player,strerror(errno));
    goto fail;
  }
  snprintf(path,sizeof(path),"etc/legendary-budgets/%ld",c->player);fd=open(path,O_RDONLY|O_NOFOLLOW);
  if(fd<0&&errno!=ENOENT)goto fail;
  if(fd>=0){
    if(fstat(fd,&st)||!S_ISREG(st.st_mode)||st.st_size>32768)goto fail;
    file=fdopen(fd,"r");if(!file)goto fail;fd=-1;
    if(!fgets(line,sizeof(line),file)||!strchr(line,'\n')||sscanf(line,"%7s %23s %23s %64[0-9a-f] %c",magic,tokens[0],tokens[1],expected_hash,&extra)!=4||
       strcmp(magic,"LGB1")||!number(tokens[0],&parsed_player)||parsed_player!=c->player||
       !number(tokens[1],&expected_rows)||expected_rows>BUDGET_ROWS||strlen(expected_hash)!=64)goto fail;
    while(fgets(line,sizeof(line),file)){
      size_t length=strlen(line);if(length>sizeof(raw)-raw_size||++rows>BUDGET_ROWS)goto fail;
      memcpy(raw+raw_size,line,length);raw_size+=length;
      if(!strchr(line,'\n')||sscanf(line,"%32[0-9a-f] %23s %23s %23s %c",id,tokens[0],tokens[1],tokens[2],&extra)!=4)goto fail;
      for(i=0;i<3;i++)if(!number(tokens[i],&values[i]))goto fail;
      if(values[1]>3||values[2]>INT_MAX||!budget_merge(c,id,values[1],values[2],values[0],false))goto fail;
    }
    if(ferror(file)||rows!=expected_rows)goto fail;
    budget_hash(raw,raw_size,actual_hash);if(strcmp(actual_hash,expected_hash))goto fail;
    fclose(file);file=NULL;
  }
  res=estate_select("SELECT operation_id,budget_kind,amount,occurred_utc FROM legendary_budgets WHERE player_id=%ld AND ((budget_kind=3 AND occurred_utc>%lld) OR (budget_kind<>3 AND occurred_utc>%lld)) LIMIT 281",c->player,(long long)(clock_now()-legendary_rules.resource_seconds),(long long)(clock_now()-legendary_rules.rolling_seconds));
  if(!res||mysql_num_rows(res)>BUDGET_ROWS)goto fail;
  while((row=mysql_fetch_row(res))){
    for(i=0;i<3;i++)if(!number(row[i+1],&values[i])||(values[i]>INT_MAX&&i<2))goto fail;
    if(!budget_merge(c,row[0],values[0],values[1],values[2],true))goto fail;
  }
  mysql_free_result(res);res=NULL;
  for(i=0;i<c->count;i++){used[c->rows[i].kind]+=c->rows[i].amount;if(used[c->rows[i].kind]>budget_limit(c->rows[i].kind))goto fail;}
  if(!budget_checkpoint(c))goto fail;
  c->next=budget_caches;budget_caches=c;return true;
fail:
  if(res)mysql_free_result(res);
  if(file)fclose(file);
  if(fd>=0)close(fd);
  if(c->fd>=0)close(c->fd);
  free(c);return false;
}
/* Public for login/recovery and tests. No caller on an attack path may flush. */
bool legendary_budget_flush(void)
{
  struct budget_cache *c;char query[65536];int i,n,written;bool dirty,ok=true;
  for(c=budget_caches;c;c=c->next){
    if(c->blocked){if(!budget_checkpoint(c)){ok=false;continue;}c->blocked=false;}
    budget_prune(c);dirty=false;for(i=0;i<c->count;i++)if(!c->rows[i].synced)dirty=true;
    if(!dirty)continue;
    if(!estate_query("START TRANSACTION")){ok=false;continue;}
    n=snprintf(query,sizeof(query),"INSERT INTO legendary_budgets(player_id,budget_kind,amount,occurred_utc,operation_id) VALUES");
    dirty=false;
    for(i=0;i<c->count;i++)if(!c->rows[i].synced){
      written=snprintf(query+n,sizeof(query)-n,"%s(%ld,%d,%d,%lld,'%s')",dirty?",":"",c->player,c->rows[i].kind,c->rows[i].amount,(long long)c->rows[i].at,c->rows[i].id);
      if(written<0||written>=(int)sizeof(query)-n)break;
      n+=written;dirty=true;
    }
    if(i<c->count||!estate_query("%s ON DUPLICATE KEY UPDATE operation_id=VALUES(operation_id)",query)||
       !estate_query("DELETE FROM legendary_budgets WHERE player_id=%ld AND ((budget_kind=3 AND occurred_utc<=%lld) OR (budget_kind<>3 AND occurred_utc<=%lld))",c->player,(long long)(clock_now()-legendary_rules.resource_seconds),(long long)(clock_now()-legendary_rules.rolling_seconds))||
       !estate_query("COMMIT")){estate_query("ROLLBACK");ok=false;continue;}
    for(i=0;i<c->count;i++)c->rows[i].synced=true;
  }
  return ok;
}
void legendary_budget_unload(struct char_data *ch)
{
  struct budget_cache **at=&budget_caches,*c;
  if(!ch||IS_NPC(ch))return;
  while((c=*at))if(c->player==GET_IDNUM(ch)&&c->holder==ch){*at=c->next;close(c->fd);free(c);return;}else at=&c->next;
}
static int budget_used(struct budget_cache *c,int kind)
{int i,used=0;budget_prune(c);for(i=0;i<c->count;i++)if(c->rows[i].kind==kind)used+=c->rows[i].amount;return used;}
static int budget_debit(struct char_data *ch,int kind,int requested,int minimum)
{
  int amount;int64_t used=0;struct npc_debit *p;struct budget_cache *c;
  if(!ch||requested<=0||!budget_limit(kind))return 0;
  if(IS_NPC(ch)){
    prune_npc();for(p=npc_debits;p;p=p->next)if(p->ch==ch&&p->kind==kind)used+=p->amount;
    amount=MIN(requested,MAX(0,budget_limit(kind)-(int)used));if(!amount||amount<minimum)return 0;
    p=calloc(1,sizeof(*p));if(!p)return 0;
    p->ch=ch;p->kind=kind;p->at=clock_now();p->amount=amount;p->next=npc_debits;npc_debits=p;return amount;
  }
#ifdef LEGENDARY_EFFECTS_TEST
  /* Legacy adapter fixtures enter directly; real login/activation primes first. */
  if(!budget_find(GET_IDNUM(ch))&&!legendary_budget_prime(ch))return 0;
#endif
  c=budget_find(GET_IDNUM(ch));if(!c||c->blocked)return 0;
  amount=MIN(requested,MAX(0,budget_limit(kind)-budget_used(c,kind)));
  if(!amount||amount<minimum||c->count==BUDGET_ROWS)return 0;
  {struct budget_row *r=&c->rows[c->count];memset(r,0,sizeof(*r));
   if(!legendary_uuid_generate(r->id))return 0;
   r->kind=kind;r->at=clock_now();r->amount=amount;c->count++;}
  if(!budget_checkpoint(c)){c->blocked=true;return 0;}
  return amount;
}
int legendary_budget_available(struct char_data *ch,int kind)
{
  struct budget_cache *c;if(!ch||IS_NPC(ch)||!budget_limit(kind))return 0;
  c=budget_find(GET_IDNUM(ch));return !c||c->blocked?0:MAX(0,budget_limit(kind)-budget_used(c,kind));
}
int legendary_budget_debit(struct char_data *ch,int kind,int requested)
{return budget_debit(ch,kind,requested,0);}
bool legendary_target_legal(struct char_data *ch,struct char_data *target,int kind)
{
  struct char_data *owner;int depth=0;
  if(!ch||!target||IN_ROOM(ch)==NOWHERE||IN_ROOM(ch)!=IN_ROOM(target)||
     GET_POS(ch)<=POS_STUNNED||GET_POS(target)<=POS_DEAD||GET_HIT(target)<=0||!CAN_SEE(ch,target))return false;
  if(kind==LEG_TARGET_SELF)return ch==target;
  if(kind==LEG_TARGET_ALLY)return ch==target||(GROUP(ch)&&GROUP(ch)==GROUP(target)&&
      FIGHTING(target)!=ch&&FIGHTING(ch)!=target);
  if(kind!=LEG_TARGET_ENEMY||ch==target||(GROUP(ch)&&GROUP(ch)==GROUP(target))||
     ROOM_FLAGGED(IN_ROOM(ch),ROOM_PEACEFUL)||(!IS_NPC(target)&&GET_LEVEL(target)>=LVL_IMMORT)||
     MOB_FLAGGED(target,MOB_NOKILL)||!ok_damage_shopkeeper(ch,target)||!is_mission_mob(ch,target))return false;
  /* Pets inherit their owner's PvP restrictions. Do not set killer flags just to
   * validate a target. This deliberately follows the normal no-PK policy. */
  owner=target;
  while(IS_NPC(owner)&&owner->master){
    if(++depth>32)return false; /* Malformed/cyclic follower chains fail closed. */
    owner=owner->master;
  }
  if(owner==ch)return false;
  if((GROUP(ch)&&GROUP(ch)==GROUP(owner))||(!IS_NPC(owner)&&!CONFIG_PK_ALLOWED))return false;
  if(AFF_FLAGGED(ch,AFF_CHARM)&&ch->master==target)return false;
  return true;
}
struct adapter {int target;legendary_power_check check;legendary_power_apply apply;};
static struct adapter adapters[LEGENDARY_POWER_COUNT];
static int power_index(int power)
{if(power>=LEG_T01&&power<=LEG_T18)return power-LEG_T01;if(power>=LEG_S01&&power<=LEG_S13)return 18+power-LEG_S01;return -1;}
bool legendary_activation_register(int power,int target,legendary_power_check check,legendary_power_apply apply)
{
  int i=power_index(power);
  if(i<0||target<0||target>2||!check||!apply||adapters[i].apply)return false;
  adapters[i].target=target;adapters[i].check=check;adapters[i].apply=apply;return true;
}
/* Keys are global, family, catalog ID. All equivalent copies share keys. */
static bool cooldown_commit(struct legendary_activation *a)
{
  int i,keys[3]={1,1000+legendary_power(a->power)->family,a->power};
  int seconds[3];int64_t expiry;char query[256];
  seconds[0]=legendary_rules.global_cooldown;seconds[1]=a->resolved.family_cooldown;seconds[2]=a->resolved.cooldown;
  if(!legendary_schema_ready()||!legendary_uuid_generate(a->receipt)||!estate_query("START TRANSACTION"))return false;
  if(!lock_player(a->source))goto fail;
  for(i=0;i<3;i++)if(seconds[i]){
    snprintf(query,sizeof(query),"SELECT COALESCE(MAX(expires_utc),0) FROM legendary_cooldowns WHERE player_id=%ld AND cooldown_key=%d",GET_IDNUM(a->source),keys[i]);
    if(!scalar(query,&expiry)||expiry>clock_now())goto fail;
  }
  if(!estate_query("DELETE FROM legendary_cooldowns WHERE player_id=%ld AND expires_utc<=%lld",GET_IDNUM(a->source),(long long)clock_now()))goto fail;
  for(i=0;i<3;i++)if(seconds[i]&&!estate_query("INSERT INTO legendary_cooldowns(player_id,cooldown_key,expires_utc) VALUES(%ld,%d,%lld) ON DUPLICATE KEY UPDATE expires_utc=VALUES(expires_utc)",GET_IDNUM(a->source),keys[i],(long long)(clock_now()+seconds[i])))goto fail;
  if(!estate_query("DELETE FROM legendary_activations WHERE player_id=%ld AND started_utc<=%lld",GET_IDNUM(a->source),(long long)(clock_now()-86400)))goto fail;
  if(!estate_query("INSERT INTO legendary_activations(activation_id,player_id,source_uuid,channel_id,power_id,target_player_id,started_utc) VALUES('%s',%ld,'%s',%d,%d,%ld,%lld)",a->receipt,GET_IDNUM(a->source),a->uuid,a->channel,a->power,IS_NPC(a->target)?0:GET_IDNUM(a->target),(long long)clock_now()))goto fail;
  if(!estate_query("COMMIT"))goto fail;
  return true;
fail:estate_query("ROLLBACK");return false;
}
bool legendary_activate(struct char_data *ch,int power,struct char_data *target)
{
  const struct legendary_loadout *v;struct legendary_activation a;int i,j,index=power_index(power),mod,target_kind;
#ifndef LEGENDARY_EFFECTS_TEST
  legendary_techniques_init();
#endif
  if(!ch||IS_NPC(ch)||GET_IDNUM(ch)<=0||index<0)return false;
  if(!legendary_feature_enabled(LEG_FEATURE_MAJOR_EFFECTS)){
    send_to_char(ch,"Legendary major powers are currently disabled.\r\n");return false;
  }
  if(!legendary_registry_refresh())return false;
  if(!adapters[index].apply){send_to_char(ch,"That power's effect is not implemented yet.\r\n");return false;}
  if((GET_POS(ch)<POS_FIGHTING&&!(power==LEG_T08&&ch->legendary_knockdown&&GET_POS(ch)==POS_SITTING))||GET_WAIT_STATE(ch)>0||!is_action_available(ch,atSTANDARD,false)||IS_CASTING(ch)||AFF_FLAGGED(ch,AFF_STUN)||AFF_FLAGGED(ch,AFF_PARALYZED)||AFF_FLAGGED(ch,AFF_DAZED)){
    send_to_char(ch,"You cannot activate a legendary power while incapacitated or recovering.\r\n");return false;}
  memset(&a,0,sizeof(a));a.source=ch;a.target=target;a.power=power;
  v=legendary_loadout_get(ch);if(!v->ready)return false;
  for(i=0;i<v->major_count;i++)if(v->majors[i].reason==LEG_EQUIPPED&&v->majors[i].power==power)break;
  if(i==v->major_count){send_to_char(ch,"That power is not active in your attuned loadout.\r\n");return false;}
  a.channel=v->majors[i].channel;memcpy(a.uuid,v->majors[i].uuid,33);
  for(j=0;j<v->count;j++)if(v->channels[j].channel==a.channel&&!strcmp(v->channels[j].uuid,a.uuid))break;
  if(j==v->count)return false;
  a.rank=v->channels[j].rank;a.source_version=v->channels[j].version;
  mod=v->channels[j].modifier_target==(power>=LEG_S01?2:1)?v->channels[j].modifier:0;
  if(!legendary_resolve_power(power,a.rank,mod,&a.resolved))return false;
  target_kind=a.resolved.target_ally_instead?LEG_TARGET_ALLY:adapters[index].target;
  if(!target)target=target_kind==LEG_TARGET_ENEMY?FIGHTING(ch):ch;
  a.target=target;
  if(!legendary_target_legal(ch,target,target_kind)||(a.resolved.target_ally_instead&&target==ch)){
    send_to_char(ch,"That is not a legal target for this power.\r\n");return false;}
  {struct char_data *member;
    if(!legendary_budget_prime(ch))return false;
    for(member=world[IN_ROOM(ch)].people;member;member=member->next_in_room)
      if(!IS_NPC(member)&&member!=ch&&(member==target||(GROUP(ch)&&GROUP(ch)==GROUP(member)))&&!legendary_budget_prime(member))return false;
  }
  if(!adapters[index].check(&a)){
    send_to_char(ch,"The power's prerequisites are not satisfied.\r\n");return false;}
  if(!cooldown_commit(&a)){send_to_char(ch,"Power unavailable: cooldown active or durable storage unavailable.\r\n");return false;}
  GET_WAIT_STATE(ch)=MAX(GET_WAIT_STATE(ch),legendary_rules.global_cooldown*PASSES_PER_SEC);
  start_action_cooldown(ch,atSTANDARD,legendary_rules.global_cooldown*PASSES_PER_SEC);
  adapters[index].apply(&a);send_to_char(ch,"You activate %s.\r\n",legendary_power(power)->name);return true;
}
void legendary_use(struct char_data *ch,const char *power,const char *target)
{
  int i,id=0;struct char_data *vict=NULL;char name[MAX_INPUT_LENGTH];
  for(i=0;i<LEGENDARY_POWER_COUNT;i++)if(!strcasecmp(power,legendary_powers[i].code)||!strcasecmp(power,legendary_powers[i].name)){id=legendary_powers[i].id;break;}
  if(!id){send_to_char(ch,"Specify a power code or quoted name, such as infuse use T01.\r\n");return;}
  if(target&&*target&&!strcasecmp(target,"self"))vict=ch;
  if(target&&*target&&strcasecmp(target,"self")){snprintf(name,sizeof(name),"%s",target);vict=get_char_vis(ch,name,NULL,FIND_CHAR_ROOM);}
  if(target&&*target&&!vict){send_to_char(ch,"No visible target by that name is here.\r\n");return;}
  (void)legendary_activate(ch,id,vict);
}
int legendary_heal(struct char_data *source,struct char_data *target,int amount)
{
  int actual;
  if(!legendary_target_legal(source,target,LEG_TARGET_ALLY)||amount<=0)return 0;
  if(IS_NPC(target)&&MOB_FLAGGED(target,MOB_GOLEM))amount/=2;
  if(AFF_FLAGGED(source,AFF_BLACKMANTLE)||AFF_FLAGGED(target,AFF_BLACKMANTLE))amount=(int)((int64_t)amount*8/10);
  actual=MIN(amount,MAX(0,GET_MAX_HIT(target)-GET_HIT(target)));
  actual=legendary_budget_debit(target,LEG_BUDGET_HEALING,actual);
  if(actual){GET_HIT(target)+=actual;update_pos(target);legendary_record_support(source,target,LEG_ACTION_HEAL,actual);}return actual;
}
int legendary_resource_restore(struct char_data *ch,int amount)
{
  int actual;
  if(!ch||IS_NPC(ch)||!legendary_resource_class(ch)||GET_HIT(ch)<=0||amount<=0)return 0;
  actual=MIN(amount,MAX(0,GET_MAX_PSP(ch)-GET_PSP(ch)));
  actual=legendary_budget_debit(ch,LEG_BUDGET_RESOURCE,actual);GET_PSP(ch)+=actual;return actual;
}
int legendary_resource_pay(struct char_data *ch,int cost,int discount)
{
  int saved,minimum;
  if(!ch||IS_NPC(ch)||GET_HIT(ch)<=0||!legendary_resource_class(ch)||cost<1)return -1;
  minimum=MAX(0,cost-MAX(0,GET_PSP(ch)));
  saved=budget_debit(ch,LEG_BUDGET_RESOURCE,MIN(MAX(0,discount),cost-1),minimum);
  if(saved<minimum)return -1;
  GET_PSP(ch)-=cost-saved;return cost-saved;
}
struct effect {
  struct effect *next;
  struct legendary_activation activation;
  struct legendary_effect_spec spec;
  int64_t expires,next_tick;
  room_rnum room;
  bool revoked;
};
static struct effect *effects;
static void effect_revoke(struct char_data *ch,void *context)
{struct effect *e=context;(void)ch;e->revoked=true;}
static void remove_effect(struct effect **at)
{
  struct effect *e=*at;*at=e->next;
  legendary_source_forget(e->activation.source,e);free(e);
}
static bool live(struct effect *e)
{
  if(e->revoked||(clock_now()>e->expires||(clock_now()==e->expires&&e->spec.kind!=LEG_EFFECT_HEAL_TICKS))||GET_HIT(e->activation.source)<=0||GET_HIT(e->activation.target)<=0||
     GET_POS(e->activation.source)<=POS_DEAD||GET_POS(e->activation.target)<=POS_DEAD)return false;
  if(e->activation.source!=e->activation.target&&
     !(e->spec.flags&LEG_EFFECT_MOUNT)&&
     ((e->spec.flags&LEG_EFFECT_GROUP)||adapters[power_index(e->activation.power)].target==LEG_TARGET_ALLY||e->activation.resolved.target_ally_instead)&&
     (!GROUP(e->activation.source)||GROUP(e->activation.source)!=GROUP(e->activation.target)))return false;
  if((e->spec.flags&LEG_EFFECT_ROOM_BOUND)&&
     (IN_ROOM(e->activation.source)!=e->room||IN_ROOM(e->activation.target)!=e->room))return false;
  if((e->spec.flags&LEG_EFFECT_MOUNT)&&(!RIDING(e->activation.source)||
     RIDING(e->activation.source)->master!=e->activation.source||
     !IS_NPC(RIDING(e->activation.source))||GET_REAL_RACE(RIDING(e->activation.source))!=RACE_TYPE_DRAGON||
     IN_ROOM(RIDING(e->activation.source))!=IN_ROOM(e->activation.source)||GET_HIT(RIDING(e->activation.source))<=0||
     (e->activation.target!=e->activation.source&&e->activation.target!=RIDING(e->activation.source))))return false;
  return true;
}
static void sweep(void)
{
  struct effect **at=&effects,*e;
  while((e=*at)){
    (void)legendary_loadout_get(e->activation.source);
    if(!live(e))remove_effect(at);else at=&e->next;
  }
}
bool legendary_effect_prepare(const struct legendary_activation *a,const struct legendary_effect_spec *s)
{
  struct effect *e,*p,**at;int count=0;
  if(!a||!s||!a->source||!a->target||!legendary_uuid_valid(a->uuid)||!legendary_uuid_valid(a->receipt)||
     !legendary_power(a->power)||s->key<=0||s->kind<0||s->kind>LEG_EFFECT_TEMP_HP||s->duration<=0||s->duration>86400||
     s->magnitude<0||s->percent<0||s->percent>100||s->maximum<0||s->pool<0||s->charges<0||s->ticks<0||s->ticks>100||
     (s->kind==LEG_EFFECT_HEAL_TICKS&&(s->interval<=0||s->ticks<=0||(int64_t)s->interval*s->ticks>s->duration)))return false;
  sweep();for(p=effects;p;p=p->next)if(p->activation.target==a->target)count++;
  if(count>=64)return false;
  e=calloc(1,sizeof(*e));if(!e)return false;
  e->activation=*a;e->spec=*s;e->expires=clock_now()+s->duration;e->next_tick=clock_now()+s->interval;e->room=IN_ROOM(a->source);
  if(!legendary_source_attach(a->source,a->uuid,a->channel,a->power,effect_revoke,e)){free(e);return false;}
  at=&effects;
  while((p=*at))if(p->activation.source==a->source&&p->activation.target==a->target&&
      p->spec.key==s->key&&p->spec.kind==s->kind&&s->kind==LEG_EFFECT_PREPARED){
    if(s->kind==LEG_EFFECT_PREPARED){
      e->spec.magnitude=MAX(e->spec.magnitude,p->spec.magnitude);
      e->expires=MAX(e->expires,p->expires);
    }
    remove_effect(at);
  }else at=&p->next;
  e->next=effects;effects=e;return true;
}
/* Strength is magnitude for scalar effects and the actual packet reduction for
 * wards. Selection is per key/kind, across all casters; duplicates never add. */
static bool eligible(struct effect *,int,bool);
static struct effect *best(struct char_data *target,int key,int kind,int packet,int type,bool breath)
{
  struct effect *e,*winner=NULL;int value,high=-1;
  for(e=effects;e;e=e->next)if(e->activation.target==target&&e->spec.key==key&&e->spec.kind==kind&&live(e)){
    if((kind==LEG_EFFECT_REDUCTION||kind==LEG_EFFECT_REDIRECT)&&!eligible(e,type,breath))continue;
    value=kind==LEG_EFFECT_REDUCTION||kind==LEG_EFFECT_REDIRECT?
      (int)MIN((int64_t)e->spec.maximum,((int64_t)packet*e->spec.percent)/100):e->spec.magnitude;
    if((kind==LEG_EFFECT_REDUCTION||kind==LEG_EFFECT_REDIRECT)&&e->spec.pool>0)value=MIN(value,e->spec.pool);
    if(kind==LEG_EFFECT_REDIRECT)value=MIN(value,MAX(0,GET_HIT(e->activation.source)-1));
    if(value>high||(value==high&&winner&&e->expires>winner->expires)){winner=e;high=value;}
  }
  return winner;
}
bool legendary_effect_peek(struct char_data *target,int key,struct legendary_activation *a,struct legendary_effect_spec *s)
{struct effect *e;sweep();e=best(target,key,LEG_EFFECT_PREPARED,0,0,false);if(!e)return false;
 if(a)*a=e->activation;
 if(s)*s=e->spec;
 return true;}
bool legendary_effect_from(struct char_data *source,int key,struct legendary_activation *a)
{struct effect *e;sweep();for(e=effects;e;e=e->next)if(e->activation.source==source&&e->spec.key==key){if(a)*a=e->activation;return true;}return false;}
void legendary_effect_clear(struct char_data *target,int key)
{struct effect **at=&effects,*e;while((e=*at))if(e->activation.target==target&&e->spec.key==key)remove_effect(at);else at=&e->next;}
void legendary_effect_clear_source(struct char_data *source,int key)
{struct effect **at=&effects,*e;while((e=*at))if(e->activation.source==source&&e->spec.key==key)remove_effect(at);else at=&e->next;}
int legendary_effect_value(struct char_data *target,int key)
{struct effect *e;sweep();e=best(target,key,LEG_EFFECT_PREPARED,0,0,false);return e?e->spec.magnitude:0;}
bool legendary_effect_consume(struct char_data *target,int key)
{
  struct effect *e;sweep();e=best(target,key,LEG_EFFECT_PREPARED,0,0,false);if(!e)return false;
  if(e->spec.charges>1)e->spec.charges--;else e->revoked=true;sweep();return true;
}
/* Temporary HP is a separate, source-leased reserve. It never fills real HP,
 * survives ordinary healing, and cannot be serialized as permanent health. */
int legendary_temporary_remaining(struct char_data *ch)
{struct effect *e;sweep();e=best(ch,LEG_S08,LEG_EFFECT_TEMP_HP,0,0,false);return e?e->spec.magnitude:0;}
int legendary_temporary_hp(const struct legendary_activation *a,int amount,int duration)
{
  struct legendary_effect_spec s={0};struct effect *e;int old,grant;
  sweep();e=best(a->target,LEG_S08,LEG_EFFECT_TEMP_HP,0,0,false);old=e?e->spec.magnitude:0;
  if(amount<=old)return 0;
  /* Allocate/attach first; no allowance is spent for an impossible grant. */
  s.key=LEG_S08;s.kind=LEG_EFFECT_TEMP_HP;s.magnitude=0;s.duration=duration;s.flags=LEG_EFFECT_ROOM_BOUND|LEG_EFFECT_GROUP;
  if(!legendary_effect_prepare(a,&s))return 0;
  grant=legendary_budget_debit(a->target,LEG_BUDGET_HEALING,amount-old);
  if(!grant){effects->revoked=true;sweep();return 0;}
  effects->spec.magnitude=old+grant;
  if(e)e->revoked=true;
  legendary_record_support(a->source,a->target,LEG_ACTION_HEAL,grant);
  sweep();return grant;
}
int legendary_temporary_absorb(struct char_data *ch,int damage)
{
  struct effect *e;int taken;sweep();e=best(ch,LEG_S08,LEG_EFFECT_TEMP_HP,0,0,false);
  if(!e||damage<=0)return damage;
  taken=MIN(damage,e->spec.magnitude);e->spec.magnitude-=taken;
  if(!e->spec.magnitude)e->revoked=true;
  sweep();return damage-taken;
}
static bool eligible(struct effect *e,int type,bool breath)
{
  if((e->spec.flags&LEG_EFFECT_BREATH)&&!breath)return false;
  if((e->spec.flags&LEG_EFFECT_PHYSICAL)&&type!=DAM_BLUDGEON&&type!=DAM_PUNCTURE&&type!=DAM_SLICE)return false;
  return true;
}
int legendary_effects_damage(struct char_data *source,struct char_data *target,int damage,int type,bool breath,unsigned int origin)
{
  struct effect *e;int prevented,want,remaining=MAX(0,damage),keys[64],n=0,i;
  if(!target||remaining<=0)return remaining;
  if(origin&LEG_ORIGIN_REDIRECT)return remaining;
  if(origin&LEG_ORIGIN_BONUS){
    /* Post-defense bonus packets cannot turn an immune packet into damage.
     * Overkill does not spend offensive allowance. No secondary triggers. */
    return legendary_budget_debit(source,LEG_BUDGET_OFFENSE,MIN(remaining,MAX(0,GET_HIT(target))));
  }
  if(origin&LEG_ORIGIN_NO_TRIGGER)return remaining;
  sweep();
  for(e=effects;e;e=e->next)if(e->activation.target==target&&(e->spec.kind==LEG_EFFECT_REDUCTION||e->spec.kind==LEG_EFFECT_REDIRECT)&&eligible(e,type,breath)){
    for(i=0;i<n;i++)if(keys[i]==e->spec.key)break;
    if(i==n&&n<(int)(sizeof(keys)/sizeof(keys[0])))keys[n++]=e->spec.key;
  }
  /* Stable key order makes multiple distinct wards independent of allocation. */
  for(i=1;i<n;i++){int k=keys[i],j=i;while(j&&keys[j-1]>k){keys[j]=keys[j-1];j--;}keys[j]=k;}
  for(i=0;i<n&&remaining>0;i++){
    int kind;
    for(kind=LEG_EFFECT_REDUCTION;kind<=LEG_EFFECT_REDIRECT&&remaining>0;kind++){
      e=best(target,keys[i],kind,remaining,type,breath);if(!e||!eligible(e,type,breath))continue;
      want=(int)MIN((int64_t)e->spec.maximum,((int64_t)remaining*e->spec.percent)/100);
      if(kind==LEG_EFFECT_REDIRECT){
        if(e->activation.source==target||IN_ROOM(e->activation.source)!=IN_ROOM(target))continue;
        want=MIN(want,MAX(0,GET_HIT(e->activation.source)-1));
      }
      if(e->spec.pool>0)want=MIN(want,e->spec.pool);
      prevented=legendary_budget_debit(target,LEG_BUDGET_PREVENTION,want);if(!prevented)continue;
      remaining-=prevented;
      if(kind==LEG_EFFECT_REDIRECT){
        /* A terminal, tagged transfer: no defenses, recursion, lifesteal or new
         * offensive participation; the actual interceptor gets one receipt. */
        GET_HIT(e->activation.source)-=prevented;update_pos(e->activation.source);
        legendary_record_action(e->activation.source,source,LEG_ACTION_INTERCEPT,target,prevented);
      }else legendary_record_support(e->activation.source,target,LEG_ACTION_DEFENSE,prevented);
      if(e->spec.pool>0){e->spec.pool-=prevented;if(!e->spec.pool)e->revoked=true;}
      if(e->spec.charges>0&&!--e->spec.charges)e->revoked=true;
    }
  }
  sweep();return remaining;
}
void legendary_effects_pulse(void)
{
  struct effect *e;(void)legendary_budget_flush();sweep();prune_npc();
  for(e=effects;e;e=e->next)if(e->spec.kind==LEG_EFFECT_HEAL_TICKS&&e->next_tick<=clock_now()){
    (void)legendary_heal(e->activation.source,e->activation.target,e->spec.magnitude);
    /* One tick per pulse; never replay missed ticks after a stall or restart. */
    e->next_tick=clock_now()+e->spec.interval;if(--e->spec.ticks<=0)e->revoked=true;
  }
  sweep();
}
void legendary_effects_depart(struct char_data *ch)
{
  struct effect *e;
  for(e=effects;e;e=e->next)if(((e->spec.flags&LEG_EFFECT_ROOM_BOUND)&&
      (e->activation.source==ch||e->activation.target==ch))||
      ((e->spec.flags&LEG_EFFECT_SOURCE_STATIONARY)&&e->activation.source==ch))e->revoked=true;
  sweep();
}
void legendary_effects_cancel(struct char_data *ch)
{
  struct effect **at=&effects,*e;
  while((e=*at))if(e->activation.source==ch||e->activation.target==ch)remove_effect(at);else at=&e->next;
}
void legendary_effects_free(struct char_data *ch)
{
  struct npc_debit **at=&npc_debits,*p;legendary_effects_cancel(ch);legendary_cast_clear(ch);legendary_budget_unload(ch);
  while((p=*at))if(p->ch==ch){*at=p->next;free(p);}else at=&p->next;
}
