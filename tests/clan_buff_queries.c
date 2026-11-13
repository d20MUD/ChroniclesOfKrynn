#include "clan_buff_queries.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static const char *A="aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",*B="bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb";
static struct clan_buff_state row(int family,int slot)
{
  struct clan_buff_state b;
  memset(&b,0,sizeof(b));strcpy(b.clan_uid,A);
  memset(b.activation_uid,'0'+family,32);b.activation_uid[32]=0;
  b.family=(enum clan_buff_family)family;b.tier=1;b.slot=slot;b.rules_version=1;
  b.started_at=100;b.expires_at=200;b.next_available_at=300;b.paid_influence_eighths=800;
  return b;
}
static struct clan_buff_family_view *family(struct clan_buff_view *v,enum clan_buff_family id)
{int i;for(i=0;i<5;i++)if(v->families[i].family==id)return &v->families[i];assert(0);return NULL;}
int main(void)
{
  struct clan_progression_config c=clan_progression_v1;
  struct clan_buff_view v,sentinel,first;
  struct clan_buff_state rows[5],saved[5];
  int level,tier;
  for(level=1;level<=20;level++) {
    assert(clan_buff_query(&c,A,CLAN_RECORD_ACTIVE,level,NULL,0,150,&v));
    assert(v.slots_total==(level>=10?3:2)&&!v.slots_used);
    tier=level>=20?3:level>=5?2:1;
    assert(family(&v,CLAN_BUFF_SCHOLAR)->unlocked_tier==tier);
    assert(family(&v,CLAN_BUFF_ARTISAN)->unlocked_tier==(level<3?0:tier));
  }
  rows[0]=row(1,1);rows[1]=row(4,2);memcpy(saved,rows,sizeof(rows));
  assert(clan_buff_query(&c,A,CLAN_RECORD_ACTIVE,10,rows,2,150,&v));
  assert(v.slots_used==2&&v.combat_used==1);
  assert(!family(&v,CLAN_BUFF_VITALITY)->category_available);
  assert(family(&v,CLAN_BUFF_ARCANE)->available_slots==4);
  assert(!family(&v,CLAN_BUFF_SCHOLAR)->available_slots);
  first=v;assert(clan_buff_query(&c,A,CLAN_RECORD_ACTIVE,10,rows,2,150,&v));
  assert(!memcmp(&v,&first,sizeof(v))&&!memcmp(rows,saved,sizeof(rows)));
  assert(clan_buff_query(&c,A,CLAN_RECORD_ACTIVE,10,rows,2,200,&v));
  assert(!v.slots_used&&family(&v,CLAN_BUFF_SCHOLAR)->cooling_down);
  assert(!family(&v,CLAN_BUFF_SCHOLAR)->available_slots);
  assert(family(&v,CLAN_BUFF_ARCANE)->available_slots==7);
  assert(clan_buff_query(&c,A,CLAN_RECORD_ACTIVE,10,rows,2,300,&v));
  assert(!family(&v,CLAN_BUFF_SCHOLAR)->cooling_down&&family(&v,CLAN_BUFF_SCHOLAR)->available_slots==7);
  rows[0].canceled=true;rows[0].tier=2;rows[0].slot=3;
  assert(clan_buff_query(&c,A,CLAN_RECORD_ACTIVE,20,rows,1,150,&v));
  assert(!v.slots_used&&family(&v,CLAN_BUFF_SCHOLAR)->next_available_at==300);
  assert(!family(&v,CLAN_BUFF_SCHOLAR)->available_slots);
  assert(clan_buff_query(&c,B,CLAN_RECORD_ACTIVE,1,rows,1,150,&v));
  assert(!v.slots_used&&!family(&v,CLAN_BUFF_SCHOLAR)->cooling_down&&family(&v,CLAN_BUFF_SCHOLAR)->available_slots==3);
  assert(clan_buff_query(&c,A,CLAN_RECORD_DELETED,20,rows,1,150,&v));
  assert(!v.slots_total&&!v.slots_used&&!family(&v,CLAN_BUFF_SCHOLAR)->unlocked_tier);
  assert(family(&v,CLAN_BUFF_SCHOLAR)->next_available_at==300);
  memset(&sentinel,0x5a,sizeof(sentinel));v=sentinel;
  rows[0]=row(1,1);rows[1]=row(2,1);
  assert(!clan_buff_query(&c,A,CLAN_RECORD_ACTIVE,10,rows,2,150,&v)&&!memcmp(&v,&sentinel,sizeof(v)));
  rows[1]=row(1,2);assert(!clan_buff_query(&c,A,CLAN_RECORD_ACTIVE,10,rows,2,150,&v));
  rows[1]=row(2,2);strcpy(rows[1].activation_uid,rows[0].activation_uid);
  assert(!clan_buff_query(&c,A,CLAN_RECORD_ACTIVE,10,rows,2,150,&v));
  rows[1]=row(2,2);rows[2]=row(3,3);
  assert(!clan_buff_query(&c,A,CLAN_RECORD_ACTIVE,9,rows,3,150,&v));
  assert(clan_buff_query(&c,A,CLAN_RECORD_ACTIVE,10,rows,3,150,&v)&&v.slots_used==3);
  assert(!family(&v,CLAN_BUFF_VITALITY)->available_slots);
  rows[0].tier=3;assert(!clan_buff_query(&c,A,CLAN_RECORD_ACTIVE,10,rows,1,150,&v));
  rows[0]=row(1,1);rows[0].rules_version=2;assert(!clan_buff_query(&c,A,CLAN_RECORD_ACTIVE,10,rows,1,150,&v));
  rows[0]=row(1,1);assert(!clan_buff_query(&c,A,CLAN_RECORD_ACTIVE,10,rows,1,99,&v));
  rows[0].started_at=INT64_MAX-5;rows[0].expires_at=INT64_MAX-1;rows[0].next_available_at=INT64_MAX;
  assert(clan_buff_query(&c,A,CLAN_RECORD_ACTIVE,10,rows,1,INT64_MAX-2,&v)&&family(&v,CLAN_BUFF_SCHOLAR)->active);
  assert(clan_buff_query(&c,A,CLAN_RECORD_ACTIVE,10,rows,1,INT64_MAX,&v)&&!family(&v,CLAN_BUFF_SCHOLAR)->cooling_down);
  c.buffs[0].duration_seconds=1;c.buffs[0].downtime_seconds=1;rows[0]=row(1,1);
  assert(clan_buff_query(&c,A,CLAN_RECORD_ACTIVE,10,rows,1,200,&v)&&family(&v,CLAN_BUFF_SCHOLAR)->next_available_at==300);
  {struct clan_buff_definition swap=c.buffs[0];c.buffs[0]=c.buffs[4];c.buffs[4]=swap;}
  assert(clan_buff_query(&c,A,CLAN_RECORD_ACTIVE,10,rows,1,150,&v)&&family(&v,CLAN_BUFF_SCHOLAR)->active);
  assert(!clan_buff_query(&c,A,CLAN_RECORD_ACTIVE,10,NULL,1,150,&v));
  assert(!clan_buff_query(&c,A,CLAN_RECORD_ACTIVE,0,NULL,0,150,&v));
  /* Phase 23: every family's duplicate tiers/copies fail, while distinct
   * approved categories coexist within the validated three-slot limit. */
  for(int id=1;id<=5;id++)for(int tier=1;tier<=3;tier++) {
    rows[0]=row(id,1);rows[1]=row(id,2);rows[1].tier=tier;
    memset(rows[1].activation_uid,'e',32);
    assert(!clan_buff_query(&c,A,CLAN_RECORD_ACTIVE,20,rows,2,150,&v));
  }
  for(int first=1;first<=3;first++)for(int second=first+1;second<=4;second++)for(int third=second+1;third<=5;third++) {
    rows[0]=row(first,1);rows[1]=row(second,2);rows[2]=row(third,3);
    assert(clan_buff_query(&c,A,CLAN_RECORD_ACTIVE,20,rows,3,150,&v));
    assert(v.slots_used==3);
  }
  puts("Phase 13: level/tier/slot unlocks, active/category limits, family cooldown boundaries, cancellation, rotation, incarnation isolation, corrupt-state rejection and pure queries passed.");
  return 0;
}
