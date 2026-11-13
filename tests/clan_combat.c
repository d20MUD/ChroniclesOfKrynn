#include "clan_combat.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
static struct clan_combat_actor actors[5];
static struct clan_combat_action actions[16];
static struct clan_combat_encounter e;
static struct clan_combat_result result[5];
static void reset(void)
{
  size_t i;memset(actors,0,sizeof(actors));memset(actions,0,sizeof(actions));memset(&e,0,sizeof(e));
  e.actors=actors;e.actor_count=5;e.actions=actions;e.action_count=1;e.normal_death=e.world_npc=true;
  e.victim_level=20;e.victim_prototype=4301;strcpy(e.encounter_uid,"eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee");
  e.room=100;e.started_at=1000;e.occurred_at=1200;e.start_max_hp=1000;
  for(i=0;i<5;i++) {
    actors[i].owner=-1;actors[i].level=20;actors[i].player_id=100+i;actors[i].account_id=200+i;
    actors[i].room=100;actors[i].non_clan_max_hp=500;
    actors[i].accepted=actors[i].alive=actors[i].playing=true;
    strcpy(actors[i].clan_uid,"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
    actions[i].actor=0;actions[i].ally=-1;actions[i].application=i+1;actions[i].at=1199;
    actions[i].round=i+1;actions[i].room=100;actions[i].amount=50;actions[i].available=1000;
  }
}
static void query(void){assert(clan_combat_query(&clan_progression_v1,&e,result,5));}
int main(void)
{
  reset();query();assert(!result[0].reasons);assert(result[1].reasons&CLAN_COMBAT_NO_PARTICIPATION);
  actions[0].amount=49;query();assert(result[0].reasons&CLAN_COMBAT_NO_PARTICIPATION);
  actions[0].amount=10000;actions[0].available=49;query();assert(result[0].reasons&CLAN_COMBAT_NO_PARTICIPATION);
  // Healing and mitigation depend on effective values and an engaged ally.
  reset();actions[0].kind=CLAN_COMBAT_HEAL;actions[0].ally=1;actions[0].ally_engaged=true;query();assert(!result[0].reasons);
  actions[0].available=0;query();assert(result[0].reasons&CLAN_COMBAT_NO_PARTICIPATION);
  actions[0].available=100;actions[0].ally_engaged=false;query();assert(result[0].reasons&CLAN_COMBAT_NO_PARTICIPATION);
  actions[0].ally_engaged=true;actions[0].kind=CLAN_COMBAT_MITIGATE;actions[0].amount=25;query();assert(!result[0].reasons);
  actions[0].amount=24;query();assert(result[0].reasons&CLAN_COMBAT_NO_PARTICIPATION);
  // Distinct useful actions must change state in distinct combat rounds.
  reset();e.action_count=2;actions[0].kind=actions[1].kind=CLAN_COMBAT_SUPPORT;
  actions[0].useful=actions[1].useful=true;query();assert(!result[0].reasons);
  actions[1].round=actions[0].round;query();assert(result[0].reasons&CLAN_COMBAT_NO_PARTICIPATION);
  actions[1].round=2;actions[1].application=actions[0].application;query();assert(result[0].reasons&CLAN_COMBAT_NO_PARTICIPATION);
  actions[1].application=2;actions[1].useful=false;query();assert(result[0].reasons&CLAN_COMBAT_NO_PARTICIPATION);
  reset();e.action_count=2;actions[0].kind=actions[1].kind=CLAN_COMBAT_HOSTILE_HIT;
  actions[0].ally_engaged=actions[1].ally_engaged=true;actions[1].round=actions[0].round;query();assert(!result[0].reasons);
  // Recent/in-room only, with an inclusive 120-second boundary.
  reset();actions[0].at=1080;query();assert(!result[0].reasons);actions[0].at=1079;query();assert(result[0].reasons&CLAN_COMBAT_NO_PARTICIPATION);
  actions[0].at=1199;actions[0].room=999;query();assert(result[0].reasons&CLAN_COMBAT_NO_PARTICIPATION);
  // Outsiders and controlled owners set H, independently of recipient eligibility.
  reset();actors[1].level=24;actors[1].accepted=false;actors[1].participated=true;query();assert(result[0].reasons&CLAN_COMBAT_TRIVIAL);
  e.victim_level=21;query();assert(!result[0].reasons);actors[0].level=19;query();assert(!result[0].reasons);actors[0].level=18;query();assert(result[0].reasons&CLAN_COMBAT_LEVEL);
  reset();actors[4].owner=0;actors[4].player_id=actors[4].account_id=0;actors[4].clan_uid[0]=0;actions[0].actor=4;query();assert(!result[0].reasons);assert(result[4].reasons&CLAN_COMBAT_IDENTITY);
  actors[0].level=30;e.victim_level=27;query();assert(!result[0].reasons&&result[0].highest_level==30);
  // No extra account reward for alts; choose the eligible lowest character ID.
  reset();e.action_count=2;actions[1].actor=1;actors[1].account_id=actors[0].account_id;query();assert(!result[0].reasons);assert(result[1].reasons&CLAN_COMBAT_ACCOUNT_DUPLICATE);
  actors[0].alive=false;query();assert(!result[1].reasons);
  reset();actors[0].playing=false;query();assert(result[0].reasons&CLAN_COMBAT_INACTIVE);
  actors[0].playing=true;actors[0].account_id=0;query();assert(result[0].reasons&CLAN_COMBAT_IDENTITY);
  reset();e.manipulated=true;query();assert(result[0].reasons&CLAN_COMBAT_BAD_VICTIM);
  reset();e.summoned=true;query();assert(result[0].reasons&CLAN_COMBAT_BAD_VICTIM);
  reset();e.pet=true;query();assert(result[0].reasons&CLAN_COMBAT_BAD_VICTIM);
  reset();e.world_npc=false;query();assert(result[0].reasons&CLAN_COMBAT_BAD_VICTIM);
  reset();e.test=true;query();assert(result[0].reasons&CLAN_COMBAT_BAD_VICTIM);
  {
    bool *excluded[]={&e.owned,&e.charmed,&e.summoned,&e.pet,&e.test,&e.staff_test,&e.manipulated};
    for(size_t i=0;i<sizeof(excluded)/sizeof(*excluded);i++){reset();*excluded[i]=true;query();assert(result[0].reasons&CLAN_COMBAT_BAD_VICTIM);}
  }
  reset();e.normal_death=false;query();assert(result[0].reasons&CLAN_COMBAT_BAD_VICTIM);
  reset();e.started_at=1199;e.start_max_hp=10;actions[0].amount=1;query();assert(!result[0].reasons);
  reset();e.action_count=2;actions[0].amount=actions[1].amount=25;query();assert(!result[0].reasons);
  reset();actors[0].accepted=false;query();assert(result[0].reasons&CLAN_COMBAT_IDENTITY);
  reset();actors[0].room=999;query();assert(result[0].reasons&CLAN_COMBAT_INACTIVE);
  reset();actors[3].owner=actors[4].owner=0;e.action_count=2;actions[0].actor=3;actions[1].actor=4;
  actions[0].amount=actions[1].amount=25;query();assert(!result[0].reasons);assert(result[3].reasons&CLAN_COMBAT_IDENTITY);assert(result[4].reasons&CLAN_COMBAT_IDENTITY);
  // Invalid facts fail closed and do not partially publish outputs.
  reset();memset(result,0x55,sizeof(result));struct clan_combat_result before[5];memcpy(before,result,sizeof(result));
  actors[0].owner=4;actors[4].owner=0;assert(!clan_combat_query(&clan_progression_v1,&e,result,5));assert(!memcmp(before,result,sizeof(result)));
  reset();e.action_count=2;actions[1].application=actions[0].application;actions[0].amount=actions[1].amount=25;query();assert(result[0].reasons&CLAN_COMBAT_NO_PARTICIPATION);
  reset();e.start_max_hp=INT64_MAX;actions[0].amount=actions[0].available=INT64_MAX;query();assert(!result[0].reasons);
  puts("Phase 25: damage/heal/mitigation/support/defense, recency, difficulty, owner/account identity, exclusions, deduplication and bounds passed");
}
