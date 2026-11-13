#include "clan_awards.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define E(x) (INT64_C(x)*8)
static void personal(void)
{
  struct clan_reward_event e={0};
  struct clan_reward_snapshot s={0};
  struct clan_reward_delta d, sentinel;
  enum clan_favor_standing standing;
  struct clan_reset_period period;
  int i;
  strcpy(e.operation_uid,"11111111111111111111111111111111");
  strcpy(e.event_uid,"22222222222222222222222222222222");
  strcpy(e.clan_uid,"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
  e.account_id=77;e.player_id=99;e.occurred_at=2200000001;e.rules_version=1;
  e.source=CLAN_SOURCE_SUPPLY_ORDER;
  e.requested_xp_eighths=E(100);e.requested_influence_eighths=e.requested_favor_eighths=E(50);
  s.clan.status=CLAN_RECORD_ACTIVE;strcpy(s.clan.uid,e.clan_uid);
  assert(clan_progression_account_window_init(e.account_id,e.occurred_at,&s.account));
  assert(clan_progression_week_at(e.occurred_at,&period));
  s.weekly.starts_at=period.starts_at;strcpy(s.weekly.clan_uid,e.clan_uid);
  s.favor.character_id=e.player_id;strcpy(s.favor.clan_uid,e.clan_uid);
  assert(clan_award_personal_project(&e,&s,&d));
  assert(d.xp_eighths==E(100)&&d.favor_eighths==E(50)&&d.account_claims==1&&!d.cap_flags);
  s.account.base_xp_eighths=E(550);
  assert(clan_award_personal_project(&e,&s,&d));
  assert(d.xp_eighths==E(75)&&d.influence_eighths==300&&d.favor_eighths==300);
  assert(d.account_base_xp_eighths==E(100)&&d.cap_flags==CLAN_AWARD_ACCOUNT_BAND);
  s.account.base_xp_eighths=E(1150);
  assert(clan_award_personal_project(&e,&s,&d)&&d.xp_eighths==E(25)&&d.favor_eighths==100);
  s.account.base_xp_eighths=E(1200);
  assert(clan_award_personal_project(&e,&s,&d)&&!d.xp_eighths&&!d.favor_eighths&&d.account_claims==1&&d.account_base_xp_eighths==E(100));
  s.account.claims[e.source]=3;
  assert(clan_award_personal_project(&e,&s,&d)&&!d.account_claims&&!d.account_base_xp_eighths&&(d.cap_flags&CLAN_AWARD_SOURCE_LIMIT));
  s.account.claims[e.source]=0;s.account.base_xp_eighths=0;
  s.clan.xp_eighths=E(38000);s.clan.influence_eighths=E(20000);
  assert(clan_award_personal_project(&e,&s,&d)&&!d.xp_eighths&&!d.influence_eighths&&d.favor_eighths==E(50));
  s.account.favor_eighths=E(600)-1;
  assert(clan_award_personal_project(&e,&s,&d)&&d.favor_eighths==1&&(d.cap_flags&CLAN_AWARD_FAVOR_WEEK));
  s.clan.xp_eighths=s.clan.influence_eighths=0;s.account.favor_eighths=E(600);
  assert(clan_award_personal_project(&e,&s,&d)&&d.xp_eighths==E(100)&&!d.favor_eighths);
  s.account.favor_eighths=0;s.account.base_xp_eighths=E(1200);
  e.source=CLAN_SOURCE_EXPEDITION_PERSONAL;e.requested_xp_eighths=e.requested_influence_eighths=0;e.requested_favor_eighths=E(100);
  assert(clan_award_personal_project(&e,&s,&d)&&d.favor_eighths==E(100)&&!d.account_base_xp_eighths&&d.account_claims==1);
  s.account.claims[e.source]=2;
  assert(clan_award_personal_project(&e,&s,&d)&&!d.favor_eighths&&!d.account_claims);
  s.account.base_xp_eighths=E(600);e.source=CLAN_SOURCE_KILL;
  e.requested_xp_eighths=8;e.requested_influence_eighths=e.requested_favor_eighths=4;
  s.weekly.incoming_xp_eighths=E(6000);
  assert(clan_award_personal_project(&e,&s,&d));
  assert(d.xp_eighths==1&&d.influence_eighths==2&&d.favor_eighths==2&&d.account_base_xp_eighths==8);
  assert((d.cap_flags&(CLAN_AWARD_ACCOUNT_BAND|CLAN_AWARD_XP_WEEK|CLAN_AWARD_SOURCE_REDUCED))==145);
  e.requested_xp_eighths=e.requested_influence_eighths=e.requested_favor_eighths=0;
  assert(clan_award_personal_project(&e,&s,&d)&&!d.account_claims&&!d.favor_eighths&&d.cap_flags==CLAN_AWARD_SOURCE_REDUCED);
  memset(&sentinel,0x5a,sizeof(sentinel));d=sentinel;
  e.requested_xp_eighths=7;
  assert(!clan_award_personal_project(&e,&s,&d)&&!memcmp(&d,&sentinel,sizeof(d)));
  e.requested_xp_eighths=16;e.requested_influence_eighths=e.requested_favor_eighths=8;
  s.favor.favor_eighths=INT64_MAX;
  assert(!clan_award_personal_project(&e,&s,&d));s.favor.favor_eighths=0;
  s.account.base_xp_eighths=1;assert(!clan_award_personal_project(&e,&s,&d));s.account.base_xp_eighths=0;
  s.account.base_xp_eighths=INT64_MAX-7;assert(!clan_award_personal_project(&e,&s,&d));s.account.base_xp_eighths=0;
  s.account.account_id=88;assert(!clan_award_personal_project(&e,&s,&d));s.account.account_id=77;
  e.rules_version=3;assert(!clan_award_personal_project(&e,&s,&d));e.rules_version=1;
  for(i=CLAN_SOURCE_DELIVERY;i<=CLAN_SOURCE_BOUNTY;i++) {
    e.source=(enum clan_reward_source)i;e.requested_xp_eighths=800;
    e.requested_influence_eighths=e.requested_favor_eighths=400;
    s.account.claims[i]=2;assert(clan_award_personal_project(&e,&s,&d)&&d.cap_flags==CLAN_AWARD_SOURCE_LIMIT);
    s.account.claims[i]=0;assert(clan_award_personal_project(&e,&s,&d)&&d.account_claims==1);
  }

  for(i=0;i<6;i++) {
    int64_t threshold=clan_progression_defaults.standings[i].favor_points*8;
    assert(clan_award_standing(threshold,&standing)&&(int)standing==i);
    if(i)assert(clan_award_standing(threshold-1,&standing)&&(int)standing==i-1);
  }
  assert(clan_award_standing(INT64_MAX,&standing)&&standing==CLAN_FAVOR_EXALTED);
  assert(!clan_award_standing(-1,&standing));
  for(i=1;i<=128;i*=2)assert(clan_award_explanation(i));
  assert(!clan_award_explanation(3)&&!clan_award_explanation(256));
  puts("Personal policy: exact account bands, source/Favor caps, expedition independence, half-credit kills, standing thresholds and malformed facts passed.");
}
int main(void)
{
  const struct clan_progression_config *c=&clan_progression_v1;
  personal();
  struct clan_award_projection p, sentinel;
  struct clan_progression_config bad=*c;
  int level=-1, i;
  for(i=0;i<20;i++) {
    int64_t t=c->xp_points[i]*8;
    assert(clan_award_level(c,t,&level)&&level==i+1);
    if(i)assert(clan_award_level(c,t-1,&level)&&level==i);
  }
  level=-1;assert(!clan_award_level(c,-1,&level)&&level==-1);
  assert(!clan_award_level(c,E(38000)+1,&level)&&level==-1);
  assert(!clan_award_level(c,INT64_MAX,&level));
  assert(clan_award_project(c,0,0,0,0,E(6000),E(100),&p));
  assert(p.xp_after==E(6000)&&p.level_before==1&&p.level_after==8&&!p.cap_flags);
  assert(clan_award_project(c,0,0,E(5900),0,E(200),E(100),&p));
  assert(p.xp_eighths==E(125)&&p.influence_eighths==E(100)&&p.cap_flags==CLAN_AWARD_XP_WEEK);
  assert(clan_award_project(c,0,0,E(11900),0,E(200),0,&p)&&p.xp_eighths==E(25));
  assert(clan_award_project(c,0,0,E(12000),0,E(200),E(10),&p));
  assert(!p.xp_eighths&&p.influence_eighths==E(10));
  assert(clan_award_project(c,E(37999),0,0,0,E(100),E(50),&p));
  assert(p.xp_after==E(38000)&&p.level_after==20&&p.xp_eighths==E(1));
  assert(p.cap_flags==CLAN_AWARD_XP_LEVEL&&p.influence_eighths==E(50));
  assert(clan_award_project(c,E(38000),0,0,0,E(100),E(50),&p)&&!p.xp_eighths&&p.influence_eighths==E(50));
  assert(clan_award_project(c,0,E(19995),0,E(4990),0,E(50),&p));
  assert(p.influence_eighths==E(5)&&p.influence_after==E(20000));
  assert(p.cap_flags==(CLAN_AWARD_INFLUENCE_WEEK|CLAN_AWARD_INFLUENCE_STORED));
  assert(clan_award_project(c,0,0,0,E(4990),0,E(50),&p)&&p.influence_eighths==E(10));
  assert(clan_award_project(c,0,E(20000),0,0,E(100),E(50),&p)&&p.xp_eighths==E(100)&&!p.influence_eighths);
  /* Splitting quarter-band events never loses sub-eighth remainder. */
  {int64_t sum=0;for(i=0;i<8;i++){assert(clan_award_project(c,0,0,E(6000)+i,0,1,0,&p));sum+=p.xp_eighths;}assert(sum==2);}
  assert(clan_award_project(c,0,0,0,0,E(12000),0,&p)&&p.xp_eighths==E(7500));
  assert(clan_award_project(c,0,0,0,0,INT64_MAX,INT64_MAX,&p)&&p.xp_eighths==E(7500)&&p.influence_eighths==E(5000));
  memset(&sentinel,0x5a,sizeof(sentinel));p=sentinel;
  assert(!clan_award_project(c,0,0,1,0,INT64_MAX,0,&p)&&!memcmp(&p,&sentinel,sizeof(p)));
  assert(!clan_award_project(c,0,-1,0,0,0,0,&p));
  assert(!clan_award_project(c,0,E(20000)+1,0,0,0,0,&p));
  assert(!clan_award_project(c,0,0,0,E(5000)+1,0,0,&p));
  bad.xp_points[19]=INT64_MAX;assert(!clan_award_level(&bad,0,&level));
  assert(!clan_award_project(NULL,0,0,0,0,0,0,&p));
  puts("Clan award policy: all level thresholds, multi-level awards, cap independence, weekly bands, fractions and int64 bounds passed.");
  return 0;
}
