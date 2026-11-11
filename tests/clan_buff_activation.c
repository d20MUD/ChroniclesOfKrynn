#include "clan_buff_activation.h"
#include "clan_awards.h"
#include <json-c/json.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void pure(void)
{
  struct clan_progression_config c=clan_progression_defaults;
  struct clan_progression_readiness ready={true,false,false,false,CLAN_PROGRESSION_SCHEMA_VERSION};
  struct clan_buff_request r={"11111111111111111111111111111111","aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa","",77,CLAN_BUFF_SCHOLAR,1,1,1};
  struct clan_buff_activation_snapshot s,saved;
  struct clan_buff_decision d;
  int64_t now=2200000000;
  memset(&s,0,sizeof(s));strcpy(s.clan.uid,r.clan_uid);strcpy(s.clan.name,"Clan");s.clan.vnum=7;
  s.clan.status=CLAN_RECORD_ACTIVE;s.clan.created_at=now;s.clan.updated_at=now;s.clan.state_updated_at=now;
  s.clan.influence_eighths=8000;s.accepted_member=true;s.may_manage=true;
  c.controls.store_mode=CLAN_STORE_MYSQL;c.controls.mode=CLAN_PROGRESSION_LIVE;c.controls.buffs=true;c.controls.public_release=true;
  assert(clan_repository_record_valid(&s.clan));saved=s;
  clan_buff_validate(&c,&ready,&r,&s,now,&d);assert(!d.reasons);
  assert(d.activation.paid_influence_eighths==800 && d.activation.expires_at==now+86400 && d.activation.next_available_at==now+129600);
  assert(!memcmp(&s,&saved,sizeof(s)));
  s.accepted_member=false;s.may_manage=false;s.clan.influence_eighths=0;r.tier=3;r.slot=3;
  clan_buff_validate(&c,&ready,&r,&s,now,&d);
  assert((d.reasons&(CLAN_BUFF_NOT_MEMBER|CLAN_BUFF_PERMISSION|CLAN_BUFF_LOCKED|CLAN_BUFF_BAD_SLOT|CLAN_BUFF_UNAFFORDABLE))==(CLAN_BUFF_NOT_MEMBER|CLAN_BUFF_PERMISSION|CLAN_BUFF_LOCKED|CLAN_BUFF_BAD_SLOT|CLAN_BUFF_UNAFFORDABLE));
  s=saved;r.tier=1;r.slot=1;clan_buff_validate(&c,&ready,&r,&s,now,&d);s.rows[0]=d.activation;s.count=1;
  clan_buff_validate(&c,&ready,&r,&s,now+1,&d);assert(d.reasons&CLAN_BUFF_ALREADY_ACTIVE);assert(d.reasons&CLAN_BUFF_COOLDOWN);
  r.actor_id=88;r.tier=2;r.slot=2;clan_buff_validate(&c,&ready,&r,&s,now+86400,&d);assert(d.reasons&CLAN_BUFF_COOLDOWN);
  r.tier=1;clan_buff_validate(&c,&ready,&r,&s,now+129600,&d);assert(!d.reasons);
  s.rows[0].canceled=true;clan_buff_validate(&c,&ready,&r,&s,now+1,&d);assert(d.reasons&CLAN_BUFF_COOLDOWN);
  s.rows[0].canceled=false;r.family=CLAN_BUFF_ARCANE;r.slot=1;
  clan_buff_validate(&c,&ready,&r,&s,now+1,&d);assert(d.reasons&CLAN_BUFF_SLOT_OCCUPIED);
  strcpy(r.expected_activation_uid,s.rows[0].activation_uid);
  clan_buff_validate(&c,&ready,&r,&s,now+1,&d);assert(!d.reasons && d.replaced_family==CLAN_BUFF_SCHOLAR);
  r.expected_activation_uid[0]='2';clan_buff_validate(&c,&ready,&r,&s,now+1,&d);assert(d.reasons&CLAN_BUFF_STALE);
  s=saved;r.expected_activation_uid[0]=0;r.slot=1;
  clan_buff_validate(&clan_progression_defaults,&ready,&r,&s,now,&d);assert(d.reasons&CLAN_BUFF_DISABLED);
  clan_buff_validate(&c,&ready,&r,&s,INT64_MAX,&d);assert(d.reasons&CLAN_BUFF_CLOCK);
  s=saved;r.family=CLAN_BUFF_SCHOLAR;r.slot=1;r.tier=2;
  s.clan.xp_eighths=c.xp_points[4]*8;
  clan_buff_validate(&c,&ready,&r,&s,now,&d);assert(!d.reasons && d.activation.paid_influence_eighths==1200);
  s.clan.xp_eighths=c.xp_points[9]*8;r.slot=3;
  clan_buff_validate(&c,&ready,&r,&s,now,&d);assert(!d.reasons);
  s.clan.xp_eighths=c.xp_points[19]*8;r.tier=3;
  clan_buff_validate(&c,&ready,&r,&s,now,&d);assert(!d.reasons && d.activation.paid_influence_eighths==1800);
  s.rows[0]=d.activation;s.rows[1]=d.activation;s.count=2;
  clan_buff_validate(&c,&ready,&r,&s,now,&d);assert(d.reasons&CLAN_BUFF_CORRUPT);
  s=saved;s.clan.status=CLAN_RECORD_DELETED;s.clan.deleted_at=now;
  clan_buff_validate(&c,&ready,&r,&s,now,&d);assert(d.reasons&CLAN_BUFF_DELETED);
  s=saved;r.family=CLAN_BUFF_VITALITY;r.tier=1;r.slot=1;
  clan_buff_validate(&c,&ready,&r,&s,now,&d);assert(!d.reasons);s.rows[0]=d.activation;s.count=1;r.slot=2;
  clan_buff_validate(&c,&ready,&r,&s,now,&d);assert(d.reasons&CLAN_BUFF_COMBAT_LIMIT);
  for(unsigned int i=1;i<=CLAN_BUFF_CLOCK;i*=2)assert(strncmp(clan_buff_rejection_text(i),"Unknown",7));
  puts("Pure activation validation: gates, permissions, cumulative reasons, slots/tiers, cooldown boundaries, replacement, overflow and no mutation passed.");
}
int main(int argc,char **argv)
{
  MYSQL *db;struct clan_progression_config c=clan_progression_defaults;
  struct clan_buff_request r;struct clan_buff_decision d;
  struct json_object *out=NULL,*root,*v,*answer;
  enum clan_repository_result result;char input[4096];int64_t now;
  if(argc==1){pure();return 0;}
  answer=json_object_new_object();
  assert(fgets(input,sizeof(input),stdin));root=json_tokener_parse(input);assert(root);
  memset(&r,0,sizeof(r));
#define S(key,field) assert(json_object_object_get_ex(root,key,&v));assert(strlen(json_object_get_string(v))<sizeof(r.field));strcpy(r.field,json_object_get_string(v))
#define N(key,field) assert(json_object_object_get_ex(root,key,&v));r.field=json_object_get_int64(v)
  S("operation_uid",operation_uid);S("clan_uid",clan_uid);S("expected_activation_uid",expected_activation_uid);
  N("actor_id",actor_id);N("family",family);N("tier",tier);N("slot",slot);r.rules_version=1;
#undef S
#undef N
  assert(json_object_object_get_ex(root,"now",&v));now=json_object_get_int64(v);
  c.controls.store_mode=CLAN_STORE_MYSQL;c.controls.mode=CLAN_PROGRESSION_LIVE;c.controls.buffs=true;c.controls.public_release=true;
  if(json_object_object_get_ex(root,"disabled",&v)&&json_object_get_boolean(v))c.controls.buffs=false;
  if(json_object_object_get_ex(root,"fault",&v))clan_buff_activation_fault=json_object_get_int(v);
  if(json_object_object_get_ex(root,"crash",&v))clan_buff_activation_crash=json_object_get_int(v);
  db=mysql_init(NULL);assert(mysql_real_connect(db,NULL,"root",NULL,"clan_test",0,argv[1],0));
  memset(&d,0x5a,sizeof(d));
  if(json_object_object_get_ex(root,"nested",&v)&&json_object_get_boolean(v))assert(!mysql_query(db,"START TRANSACTION"));
  result=clan_buff_activation_commit(db,&c,&r,now,&d,&out);
  json_object_object_add(answer,"result",json_object_new_string(clan_repository_result_name(result)));
  if(result==CLAN_REPO_INELIGIBLE)json_object_object_add(answer,"reasons",json_object_new_int64(d.reasons));
  else if(result!=CLAN_REPO_OK){struct clan_buff_decision sentinel;memset(&sentinel,0x5a,sizeof(sentinel));assert(!memcmp(&sentinel,&d,sizeof(d)));assert(!out);}
  if(out)json_object_object_add(answer,"receipt",out);
  puts(json_object_to_json_string_ext(answer,JSON_C_TO_STRING_PLAIN));
  json_object_put(root);json_object_put(answer);mysql_close(db);return 0;
}
