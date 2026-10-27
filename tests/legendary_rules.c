/* Phase-2 tests link the actual production rules, with no game/database stubs. */
#include "legendary.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

static void assert_cost(struct legendary_cost c, int64_t xp, int64_t gold,
                        int64_t fragments, int64_t essences, int64_t catalysts)
{
  assert(c.xp == xp && c.gold == gold && c.fragments == fragments &&
         c.essences == essences && c.catalysts == catalysts);
}

static void costs_and_parity(void)
{
  static const int64_t cumulative[6][5] = {
    {0,0,0,0,0}, {2000000,50000,500,0,0}, {7000000,150000,1500,200,0},
    {17000000,350000,3500,600,20}, {37000000,750000,7500,1400,60}, {77000000,1550000,14000,2800,120}
  };
  struct legendary_cost c, pair, unchanged = {7,7,7,7,7};
  int a, b, family;
  for (a = 0; a <= 5; a++)
  {
    assert(legendary_cost_between(0,a,&c));
    assert_cost(c,cumulative[a][0],cumulative[a][1],cumulative[a][2],cumulative[a][3],cumulative[a][4]);
    for (b = 0; b <= 5; b++)
    {
      for (family = LEG_FAMILY_ASSAULT; family <= LEG_FAMILY_FOCUS; family++)
      {
        assert(legendary_family_slot(family) >= 0);
        assert(legendary_pair_cost(a,b,&pair));
        assert_cost(pair,cumulative[a][0]+cumulative[b][0],cumulative[a][1]+cumulative[b][1],
                    cumulative[a][2]+cumulative[b][2],cumulative[a][3]+cumulative[b][3],
                    cumulative[a][4]+cumulative[b][4]);
      }
      if (a <= b)
      {
        assert(legendary_cost_between(a,b,&c));
        assert_cost(c,cumulative[b][0]-cumulative[a][0],cumulative[b][1]-cumulative[a][1],
                    cumulative[b][2]-cumulative[a][2],cumulative[b][3]-cumulative[a][3],
                    cumulative[b][4]-cumulative[a][4]);
      }
    }
  }
  assert(!legendary_cost_between(4,3,&unchanged)); assert_cost(unchanged,7,7,7,7,7);
  assert(!legendary_pair_cost(-1,0,&unchanged)); assert_cost(unchanged,7,7,7,7,7);
  assert(!legendary_pair_cost(0,6,&unchanged));
  assert(!legendary_cost_between(0,1,NULL));
  assert(legendary_reshape_costs[LEG_RESHAPE_PATH_FAMILY].essences == 200);
  assert(legendary_reshape_costs[LEG_RESHAPE_SIGNATURE].gold == 50000);
  assert(legendary_rules.daily_fragments == 200 && legendary_rules.daily_essences == 40);
  assert(legendary_rules.daily_catalysts == 2 && legendary_rules.boss_cooldown_seconds == 72000);
  assert(legendary_boss_rewards[2].fragments == 30 && legendary_boss_rewards[2].catalysts == 1);
}

static void arithmetic(void)
{
  int64_t n=123;
  int small=123;
  assert(legendary_add(INT64_MAX-1,1,&n) && n==INT64_MAX);
  assert(!legendary_add(INT64_MAX,1,&n) && n==INT64_MAX);
  assert(!legendary_add(-1,1,&n)); assert(!legendary_add(1,-1,&n));
  assert(!legendary_add(0,0,NULL));
  assert(legendary_subtract(INT64_MAX,INT64_MAX,&n) && n==0);
  assert(!legendary_subtract(0,1,&n) && n==0);
  assert(!legendary_subtract(-1,0,&n)); assert(!legendary_subtract(1,-1,&n));
  assert(legendary_to_int(INT_MAX,&small) && small==INT_MAX);
  assert(!legendary_to_int((int64_t)INT_MAX+1,&small) && small==INT_MAX);
  assert(!legendary_to_int(-1,&small));
}

static void essence_payments(void)
{
  int64_t balances[6]={10,10,0,0,0,0}, saved[6], payment[6]={0}, total;
  struct legendary_wallet wallet={20000000,4000000,2000,{0,100,100,0,0,0},20};
  unsigned int random=721;
  int trial,i,type,amount;
  memcpy(saved,balances,sizeof(saved));
  assert(legendary_plan_essences(balances,1,10,payment));
  assert(payment[1]==5 && payment[0]==5); /* Tie after matching debit favors type 0. */
  assert(memcmp(saved,balances,sizeof(saved))==0);
  balances[1]=4; memset(payment,7,sizeof(payment)); memcpy(saved,payment,sizeof(saved));
  assert(!legendary_plan_essences(balances,1,10,payment));
  assert(memcmp(saved,payment,sizeof(saved))==0);
  balances[1]=100;
  assert(legendary_plan_essences(balances,1,10,payment) && payment[1]==10);
  assert(!legendary_plan_essences(balances,1,9,payment));
  assert(!legendary_plan_essences(balances,6,10,payment));
  assert(!legendary_allocate_essences(balances,1,10,11,payment));
  balances[0]=-1; assert(!legendary_plan_essences(balances,1,0,payment));
  balances[0]=LEGENDARY_BALANCE_MAX+1;
  assert(!legendary_plan_essences(balances,1,10,payment));
  assert(legendary_can_afford(&wallet,&legendary_rank_costs[2],LEG_PATH_GUARDIAN,false,payment));
  assert(!legendary_can_afford(&wallet,&legendary_reshape_costs[4],LEG_PATH_GUARDIAN,true,payment));
  wallet.essences[1]=200;
  assert(legendary_can_afford(&wallet,&legendary_reshape_costs[4],LEG_PATH_GUARDIAN,true,payment));
  assert(payment[1]==200 && payment[2]==0); /* Path reshape: all 200 must match. */
  wallet.gold=99999;
  assert(!legendary_can_afford(&wallet,&legendary_rank_costs[2],LEG_PATH_GUARDIAN,false,payment));
  wallet.gold=-1;
  assert(!legendary_can_afford(&wallet,&legendary_rank_costs[1],LEG_PATH_GUARDIAN,false,payment));
  /* Randomized feasibility and conservation against an independent oracle. */
  for(trial=0;trial<10000;trial++)
  {
    total=0;
    for(i=0;i<6;i++)
    {
      random=random*1664525U+1013904223U;
      balances[i]=(random>>16)%31; total+=balances[i];
    }
    type=trial%6; amount=2*(trial%80);
    if(balances[type]>=amount/2 && total>=amount)
    {
      assert(legendary_plan_essences(balances,type,amount,payment)); total=0;
      for(i=0;i<6;i++) {assert(payment[i]>=0 && payment[i]<=balances[i]);total+=payment[i];}
      assert(total==amount && payment[type]>=amount/2);
    }
    else assert(!legendary_plan_essences(balances,type,amount,payment));
  }
}

static void failure_outputs(void)
{
  struct legendary_wallet w={5000000,100000,1000,{0,100,100,0,0,0},0};
  struct legendary_cost c=legendary_rank_costs[2];
  int64_t payment[6],before[6];
  memset(payment,9,sizeof(payment));memcpy(before,payment,sizeof(before));
  w.xp--;assert(!legendary_can_afford(&w,&c,LEG_PATH_GUARDIAN,false,payment));w.xp++;
  w.gold--;assert(!legendary_can_afford(&w,&c,LEG_PATH_GUARDIAN,false,payment));w.gold++;
  w.fragments--;assert(!legendary_can_afford(&w,&c,LEG_PATH_GUARDIAN,false,payment));w.fragments++;
  c.catalysts=1;assert(!legendary_can_afford(&w,&c,LEG_PATH_GUARDIAN,false,payment));c.catalysts=0;
  c.xp=-1;assert(!legendary_can_afford(&w,&c,LEG_PATH_GUARDIAN,false,payment));c.xp=5000000;
  w.xp=LEGENDARY_BALANCE_MAX+1;
  assert(!legendary_can_afford(&w,&c,LEG_PATH_GUARDIAN,false,payment));
  assert(memcmp(payment,before,sizeof(payment))==0);
}

static void eligibility(void)
{
  struct legendary_item_facts f={0};
  int slot;
  f.character_level=30;f.item_level=20;f.character_id=99;f.bound_owner=-1;
  f.permanent=true;f.ordinary_allowed=true;
  for(slot=0;slot<10;slot++)
  {
    f.kind=slot==LEG_SLOT_WEAPON?LEG_ITEM_WEAPON:slot==LEG_SLOT_SHIELD?LEG_ITEM_SHIELD:
      slot==LEG_SLOT_HELD?LEG_ITEM_FOCUS:LEG_ITEM_ARMOR;
    f.wearable_slots=LEGENDARY_FLAG(slot);
    assert(legendary_item_eligible(&f,slot));
    f.item_level=19;assert(!legendary_item_eligible(&f,slot));
    f.item_level=31;assert(!legendary_item_eligible(&f,slot));f.item_level=30;
    f.npc=true;assert(!legendary_item_eligible(&f,slot));f.npc=false;
    f.bound_owner=100;assert(!legendary_item_eligible(&f,slot));f.bound_owner=99;
    f.temporary=true;assert(!legendary_item_eligible(&f,slot));f.temporary=false;
    f.expiring=true;assert(!legendary_item_eligible(&f,slot));f.expiring=false;
    f.transformation=true;assert(!legendary_item_eligible(&f,slot));f.transformation=false;
    f.ordinary_allowed=false;assert(!legendary_item_eligible(&f,slot));f.ordinary_allowed=true;
    f.character_level=29;assert(!legendary_item_eligible(&f,slot));f.character_level=31;
    assert(!legendary_item_eligible(&f,slot));f.character_level=30;
  }
  f.kind=LEG_ITEM_WEAPON;f.wearable_slots=LEGENDARY_FLAG(LEG_SLOT_WEAPON);
  f.actually_wielded=true;assert(!legendary_supporting_active(&f));
  f.two_hand_slot=true;assert(legendary_supporting_active(&f));
  f.offhand_occupied=true;assert(!legendary_supporting_active(&f));f.offhand_occupied=false;
  f.actually_wielded=false;assert(!legendary_supporting_active(&f));f.actually_wielded=true;
  f.two_hand_slot=false;f.mandatory_two_hand_ranged=true;assert(legendary_supporting_active(&f));
  f.kind=LEG_ITEM_FOCUS;assert(!legendary_supporting_active(&f));
  f.kind=LEG_ITEM_OTHER;f.wearable_slots=LEGENDARY_FLAG(LEG_SLOT_HELD);
  assert(!legendary_item_eligible(&f,LEG_SLOT_HELD));
  assert(!legendary_item_eligible(NULL,0)); assert(!legendary_item_eligible(&f,10));
  assert(legendary_family_slot(LEG_FAMILY_ASSAULT)==LEG_SLOT_WEAPON);
  assert(legendary_family_slot(LEG_FAMILY_GUARD)==LEG_SLOT_SHIELD);
  assert(legendary_family_slot(LEG_FAMILY_FOCUS)==LEG_SLOT_HELD);
  assert(legendary_family_slot(LEG_FAMILY_NONE)==-1);
}

static void powers_and_modifiers(void)
{
  struct legendary_resolved_power r, before;
  char error[160];
  int i,rank;
  assert(legendary_validate_definitions(error,sizeof(error)) && !*error);
  for(i=0;i<LEGENDARY_POWER_COUNT;i++)
  {
    const struct legendary_power_def *p=&legendary_powers[i];
    assert(legendary_parse_code(p->code)==p->id);
    assert(!legendary_resolve_power(p->id,p->minimum_rank-1,LEG_MOD_NONE,&r));
    assert(!legendary_resolve_power(p->id,3,LEG_MOD_READY,&r));
    for(rank=4;rank<=5;rank++)
    {
      assert(legendary_resolve_power(p->id,rank,LEG_MOD_READY,&r));
      assert(r.cooldown==(p->cooldown*85+99)/100);
      assert(r.family_cooldown==(p->family==LEG_CD_PROTECTION||p->family==LEG_CD_RESTORATIVE?60:
                                p->family==LEG_CD_CONTROL?30:0));
    }
  }
  assert(legendary_parse_code("f01")==LEG_F01 && legendary_parse_code("s13")==LEG_S13);
  assert(!legendary_parse_code("T00") && !legendary_parse_code("S14"));
  assert(!legendary_parse_code("F17") && !legendary_parse_code("T01junk"));
  assert(!legendary_parse_code(NULL) && !legendary_parse_code(" T01"));
  assert(legendary_power_available(LEG_SLOT_BODY,LEG_PATH_SLAYER,LEG_T09,false));
  assert(legendary_power_available(LEG_SLOT_HEAD,LEG_PATH_GUARDIAN,LEG_T07,false));
  assert(!legendary_power_available(LEG_SLOT_HEAD,LEG_PATH_GUARDIAN,LEG_S11,true));
  assert(!legendary_power_available(LEG_SLOT_WEAPON,LEG_PATH_DRAGONBANE,LEG_S11,false));
  assert(legendary_power_available(LEG_SLOT_WEAPON,LEG_PATH_DRAGONBANE,LEG_S12,true));
  assert(!legendary_power_available(-1,LEG_PATH_DRAGONBANE,LEG_T17,false));
  assert(!legendary_power_available(0,0,LEG_T01,false));
  assert(legendary_resolve_power(LEG_T01,2,0,&r));assert(r.values.attack==2&&r.values.damage==10);
  assert(legendary_resolve_power(LEG_T01,5,0,&r));assert(r.values.attack==3&&r.values.damage==15);
  assert(legendary_resolve_power(LEG_T05,5,0,&r));assert(r.values.percent==20&&r.values.pool==40);
  assert(legendary_resolve_power(LEG_T15,5,0,&r));assert(r.values.refund_percent==20&&r.values.refund_cap==15);
  assert(legendary_resolve_power(LEG_S05,5,0,&r));
  assert(r.values.heal==30&&r.values.ticks==3&&r.values.tick_heal==10&&r.values.interval==6);
  assert(legendary_resolve_power(LEG_S07,5,0,&r));assert(r.values.charges==2&&r.values.resource_discount==8);
  assert(legendary_resolve_power(LEG_S11,5,LEG_MOD_ENDURING,&r));
  assert(r.values.window==18&&r.values.control_seconds==6&&r.values.save_dc==25&&r.values.damage==25);
  assert(legendary_resolve_power(LEG_S12,5,0,&r));assert(r.values.pool==40&&r.values.targets==2);
  assert(!legendary_resolve_power(LEG_S05,5,LEG_MOD_ENDURING,&r));
  assert(!legendary_resolve_power(LEG_T04,5,LEG_MOD_ENDURING,&r));
  assert(!legendary_resolve_power(LEG_T08,4,LEG_MOD_ENDURING,&r));
  assert(legendary_resolve_power(LEG_T08,5,LEG_MOD_ENDURING,&r)&&r.values.duration==18);
  assert(legendary_resolve_power(LEG_S06,4,LEG_MOD_DISTANT_AID,&r)&&r.target_ally_instead);
  assert(!legendary_resolve_power(LEG_S11,5,LEG_MOD_DISTANT_AID,&r));
  before=r;assert(!legendary_resolve_power(999,5,0,&r));assert(memcmp(&before,&r,sizeof(r))==0);
}

/* Independent specification fixtures for every rank-V change. Offsets select
 * typed value fields; numbers are the approved catalog expectations. */
static void all_final_values(void)
{
  static const struct { int id; size_t field; int expected; } checks[] = {
#define V(id,field,n) {LEG_##id,offsetof(struct legendary_values,field),n}
    V(T01,attack,3),V(T01,damage,15),V(T02,check_bonus,4),V(T02,duration,24),
    V(T03,attack,3),V(T04,physical_info,1),V(T05,pool,40),V(T06,check_bonus,6),
    V(T06,duration,24),V(T07,pool,40),V(T08,check_bonus,2),V(T08,duration,12),
    V(T09,heal,40),V(T10,fear_seconds,18),V(T11,heal,20),V(T12,duration,45),
    V(T13,duration,45),V(T14,check_bonus,3),V(T15,refund_cap,15),
    V(T16,resource_discount,8),V(T17,pool,50),V(T18,check_bonus,4),V(T18,duration,24),
    V(S01,pool,80),V(S02,check_bonus,3),V(S02,duration,24),V(S03,duration,60),
    V(S04,check_bonus,3),V(S04,duration,24),V(S05,heal,30),V(S06,charges,3),
    V(S07,resource_discount,8),V(S08,check_bonus,4),V(S08,temporary_hp,30),
    V(S09,attack,3),V(S09,damage,30),V(S10,pool,80),V(S11,damage,25),
    V(S12,pool,40),V(S13,heal,40)
#undef V
  };
  struct legendary_resolved_power r;
  struct legendary_values expected;
  size_t i,j;
  int power;
  int value;
  for(i=0;i<sizeof(checks)/sizeof(checks[0]);i++)
  {
    assert(legendary_resolve_power(checks[i].id,5,0,&r));
    memcpy(&value,(const char *)&r.values+checks[i].field,sizeof(value));
    assert(value==checks[i].expected);
  }
  for(power=0;power<LEGENDARY_POWER_COUNT;power++) {
    expected=legendary_powers[power].base;
    for(j=0;j<sizeof(checks)/sizeof(checks[0]);j++)if(checks[j].id==legendary_powers[power].id)
      memcpy((char *)&expected+checks[j].field,&checks[j].expected,sizeof(int));
    assert(legendary_resolve_power(legendary_powers[power].id,5,0,&r));
    assert(!memcmp(&r.values,&expected,sizeof(expected)));
  }
}

/* Exhaustive rank/modifier eligibility, independent of the capability masks. */
static void rank_modifier_matrix(void)
{
  struct legendary_resolved_power got,expected,sentinel;
  const struct legendary_power_def *p;
  int i,rank,mod,legal,distant,enduring;
  for(i=0;i<LEGENDARY_POWER_COUNT;i++) {
    p=&legendary_powers[i];
    distant=p->id==LEG_T02||p->id==LEG_T05||p->id==LEG_T06||p->id==LEG_T08||
      p->id==LEG_T10||p->id==LEG_T11||p->id==LEG_T17||p->id==LEG_T18||p->id==LEG_S06;
    for(rank=-1;rank<=6;rank++)for(mod=-1;mod<=4;mod++) {
      enduring=p->id!=LEG_T04&&p->id!=LEG_T09&&p->id!=LEG_T10&&p->id!=LEG_T11&&p->id!=LEG_S05&&(p->id!=LEG_T08||rank==5);
      legal=rank>=p->minimum_rank&&rank<=5&&mod>=0&&mod<=3&&
        (mod==0||(rank>=4&&(mod==LEG_MOD_READY||
         (mod==LEG_MOD_ENDURING&&enduring)||(mod==LEG_MOD_DISTANT_AID&&distant))));
      memset(&sentinel,0x5a,sizeof(sentinel));got=sentinel;
      assert(legendary_resolve_power(p->id,rank,mod,&got)==(legal!=0));
      if(!legal){assert(!memcmp(&got,&sentinel,sizeof(got)));continue;}
      memset(&expected,0,sizeof(expected));expected.values=rank==5?p->final:p->base;
      expected.cooldown=p->cooldown;
      expected.family_cooldown=p->family==LEG_CD_PROTECTION||p->family==LEG_CD_RESTORATIVE?60:p->family==LEG_CD_CONTROL?30:0;
      if(mod==LEG_MOD_READY)expected.cooldown=(p->cooldown*85+99)/100;
      if(mod==LEG_MOD_DISTANT_AID)expected.target_ally_instead=true;
      if(mod==LEG_MOD_ENDURING){
        if(expected.values.window>=12)expected.values.window+=6;
        else expected.values.duration+=6;
      }
      assert(!memcmp(&got.values,&expected.values,sizeof(got.values)));
      assert(got.cooldown==expected.cooldown&&got.family_cooldown==expected.family_cooldown&&got.target_ally_instead==expected.target_ally_instead);
      if(p->id==LEG_S11)assert(got.values.control_seconds==6&&got.values.save_dc==25);
      if(p->id==LEG_S05)assert(got.values.interval==6&&got.values.ticks==3&&got.values.tick_heal==10);
    }
  }
}

static void passives(void)
{
  struct legendary_passive_request requests[16];
  struct legendary_passive_result result[16];
  int points,i;
  memset(requests,0,sizeof(requests));
  for(i=0;i<16;i++) {requests[i].id=LEG_F02;requests[i].eligible=true;}
  assert(legendary_plan_passives(requests,9,result,&points)&&points==6);
  assert(result[0].value==1&&result[1].value==1);
  for(i=2;i<9;i++)assert(result[i].reason==LEG_PASSIVE_CAPPED&&result[i].value==0);
  requests[0].id=LEG_F01;requests[1].id=LEG_F02;requests[2].id=LEG_F02;requests[3].id=LEG_F03;
  requests[4].id=LEG_F10;
  assert(legendary_plan_passives(requests,5,result,&points)&&points==12);
  assert(result[4].reason==LEG_PASSIVE_BUDGET&&result[4].value==0);
  requests[0].eligible=false;
  assert(legendary_plan_passives(requests,5,result,&points)&&points==10);
  assert(result[0].reason==LEG_PASSIVE_INELIGIBLE&&result[4].reason==LEG_PASSIVE_ACTIVE);
  for(i=0;i<4;i++) {requests[i].id=LEG_F07;requests[i].subtype=i;requests[i].eligible=true;}
  assert(legendary_plan_passives(requests,4,result,&points)&&points==6);
  assert(result[2].reason==LEG_PASSIVE_CAPPED&&result[3].reason==LEG_PASSIVE_CAPPED);
  for(i=0;i<5;i++) {requests[i].id=LEG_F08;requests[i].subtype=i+1;requests[i].eligible=true;}
  assert(legendary_plan_passives(requests,5,result,&points)&&points==6);
  assert(result[3].reason==LEG_PASSIVE_CAPPED);
  requests[0].id=LEG_F12;requests[0].subtype=0;requests[0].current_resistance=74;
  requests[1]=requests[0];requests[2]=requests[0];requests[2].subtype=1;requests[2].current_resistance=75;
  requests[3]=requests[0];requests[3].subtype=2;requests[3].current_resistance=-100;
  requests[4]=requests[0];requests[4].subtype=3;requests[4].current_resistance=0;
  assert(legendary_plan_passives(requests,5,result,&points)&&points==4);
  assert(result[0].value==1&&result[1].reason==LEG_PASSIVE_CAPPED);
  assert(result[2].reason==LEG_PASSIVE_CAPPED&&result[3].value==3);
  assert(result[4].reason==LEG_PASSIVE_CAPPED);
  requests[0].id=999;
  assert(legendary_plan_passives(requests,1,result,&points)&&points==0&&result[0].reason==LEG_PASSIVE_INVALID);
  assert(legendary_plan_passives(NULL,0,NULL,&points)&&points==0);
  assert(!legendary_plan_passives(requests,17,result,&points));
}

int main(void)
{
  costs_and_parity();arithmetic();essence_payments();failure_outputs();eligibility();powers_and_modifiers();all_final_values();rank_modifier_matrix();passives();
  puts("Legendary Phase 2: catalog, 36 rank pairs x 3 families, costs, overflow, eligibility,");
  puts("10,000 essence allocations, rank modifiers and passive budgets/caps passed.");
  return 0;
}
