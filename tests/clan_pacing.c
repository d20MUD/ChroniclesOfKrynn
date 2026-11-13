/* Rules 2 invariants: approved ladders, fractional credit and spending safety. */
#include "clan_awards.h"
#include "clan_pacing.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void)
{
  const struct clan_progression_config *c = &clan_progression_defaults;
  const int favor[] = {0,229,876,1596,2316,3036};
  const int ranks[] = {0,648,1358,1716,2972,3870,5667,6566,8365,9265,
                      11064,11964,13764,14664,16464,17364,19164,20064,21864,22764};
  struct clan_reward_event e = {0};
  struct clan_reward_snapshot s = {0};
  struct clan_reward_delta d;
  struct clan_award_projection p;
  enum clan_favor_standing standing;
  int i, rank;
  assert(c->rules_version == 2 && c->schema_version == 6);
  assert(clan_progression_config_validate(c, NULL) == CLAN_CONFIG_OK);
  for (i=0; i<20; ++i) {
    assert(c->xp_points[i] == CLAN_INFLUENCE_THRESHOLD(ranks[i]));
    assert(clan_award_level(c, c->xp_points[i]*8, &rank) && rank == i+1);
    if (i) assert(clan_award_level(c,c->xp_points[i]*8-1,&rank) && rank==i);
  }
  assert(clan_award_level(c, INT64_MAX, &rank) && rank==20);
  for (i=0; i<6; ++i) {
    assert(c->standings[i].favor_points == CLAN_FAVOR_THRESHOLD(favor[i]));
    assert(clan_award_standing(c->standings[i].favor_points*8,&standing) && (int)standing==i);
  }
  strcpy(e.operation_uid,"11111111111111111111111111111111");
  strcpy(e.event_uid,"22222222222222222222222222222222");
  strcpy(e.clan_uid,"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
  e.rules_version=2; e.source=CLAN_SOURCE_KILL; e.account_id=77; e.player_id=99;
  e.occurred_at=2200000001; e.requested_xp_eighths=16;
  e.requested_influence_eighths=e.requested_favor_eighths=8;
  s.clan.status=CLAN_RECORD_ACTIVE; strcpy(s.clan.uid,e.clan_uid);
  assert(clan_progression_account_window_init(e.account_id,e.occurred_at,&s.account));
  s.weekly.starts_at=s.account.starts_at; strcpy(s.weekly.clan_uid,e.clan_uid);
  s.favor.character_id=e.player_id; strcpy(s.favor.clan_uid,e.clan_uid);
  assert(clan_award_personal_project(&e,&s,&d));
  assert(d.xp_eighths==1 && d.influence_eighths==1 && d.favor_eighths==2);
  e.requested_xp_eighths=8; e.requested_influence_eighths=e.requested_favor_eighths=4;
  assert(clan_award_personal_project(&e,&s,&d));
  assert(!d.xp_eighths && !d.influence_eighths && d.favor_eighths==1);
  /* Spending the entire balance leaves the lifetime rank unchanged. */
  s.clan.xp_eighths=c->xp_points[10]*8;
  assert(clan_award_project(c,s.clan.xp_eighths,0,0,0,800,400,&p));
  assert(p.level_before==11 && p.xp_eighths==50 && p.influence_eighths==50);
  assert(p.xp_after==s.clan.xp_eighths+50);
  /* Storage and weekly limits do not create unspendable phantom rank credit. */
  assert(clan_award_project(c,s.clan.xp_eighths,20000,0,0,16,8,&p));
  assert(!p.xp_eighths && !p.influence_eighths && p.level_before==p.level_after);
  assert(clan_award_project(c,INT64_MAX,0,0,0,16,8,&p)==false);
  puts("Rules 2: every approved threshold, reduced fractional rewards, capped credit, spending independence and lifetime overflow passed.");
  return 0;
}
