#include "legendary_effects.h"
static struct legendary_activation last_activation;
static struct legendary_effect_spec next_effect;
static bool prerequisites=true;
static int activations;
static bool synthetic_check(const struct legendary_activation *a)
{(void)a;return prerequisites;}
static void synthetic_apply(const struct legendary_activation *a)
{
  last_activation=*a;activations++;
  /* These assertions execute inside apply: durable state must already exist. */
  assert(scalar("SELECT COUNT(*) FROM legendary_activations")>0);
  assert(scalar("SELECT COUNT(*) FROM legendary_cooldowns")>0);
  assert(GET_WAIT_STATE(a->source)>=6*PASSES_PER_SEC);
  if(next_effect.key)assert(legendary_effect_prepare(a,&next_effect));
}
static void effect_spec(int key,int kind,int magnitude,int percent,int maximum,int duration)
{memset(&next_effect,0,sizeof(next_effect));next_effect.key=key;next_effect.kind=kind;
 next_effect.magnitude=magnitude;next_effect.percent=percent;next_effect.maximum=maximum;
 next_effect.duration=duration;next_effect.charges=1;}
static void reset_activation(struct char_data *ch)
{GET_WAIT_STATE(ch)=0;legendary_effects_test_time+=1000;}
static void effect_cases(struct char_data *ch,const char *socket)
{
  struct obj_data *blade,*shield;struct char_data ally,enemy,reloaded,petmaster={0};struct group_data group={0};
  struct legendary_activation other;int hp,i;long n;
  legendary_effects_test_time=1800000000;foundation_reset(ch);
  assert(estate_query("DELETE FROM legendary_cooldowns"));assert(estate_query("DELETE FROM legendary_budgets"));
  assert(estate_query("DELETE FROM legendary_activations"));
  blade=legendary_test_item(ch,"engineblade",0,1,false);shield=legendary_test_item(ch,"engineshield",6,3,false);
  test_wear(ch,blade,WEAR_WIELD_1);test_wear(ch,shield,WEAR_SHIELD);
  test_layout(ch,blade,0,1,shield,0,1,shield,0,2);
  init_player(&ally,"engineally",2);GET_LEVEL(&ally)=30;GET_POS(&ally)=POS_STANDING;
  init_player(&enemy,"engineenemy",3);GET_LEVEL(&enemy)=20;GET_POS(&enemy)=POS_STANDING;
  assert(legendary_activation_register(LEG_T01,LEG_TARGET_SELF,synthetic_check,synthetic_apply));
  assert(legendary_activation_register(LEG_T05,LEG_TARGET_SELF,synthetic_check,synthetic_apply));
  assert(legendary_activation_register(LEG_S01,LEG_TARGET_ALLY,synthetic_check,synthetic_apply));
  assert(!legendary_activation_register(LEG_T01,LEG_TARGET_SELF,synthetic_check,synthetic_apply));
  /* Validation is free, including valid catalog powers without a handler. */
  assert(!legendary_activate(ch,LEG_T02,ch));assert(!legendary_activate(ch,LEG_T01,&ally));
  prerequisites=false;assert(!legendary_activate(ch,LEG_T01,ch));prerequisites=true;
  GET_WAIT_STATE(ch)=100;assert(!legendary_activate(ch,LEG_T01,ch)&&GET_WAIT_STATE(ch)==100);GET_WAIT_STATE(ch)=0;
  SET_BIT_AR(AFF_FLAGS(ch),AFF_STUN);assert(!legendary_activate(ch,LEG_T01,ch));REMOVE_BIT_AR(AFF_FLAGS(ch),AFF_STUN);
  IS_CASTING(ch)=true;assert(!legendary_activate(ch,LEG_T01,ch));IS_CASTING(ch)=false;
  assert(!scalar("SELECT COUNT(*) FROM legendary_cooldowns")&&!activations);
  /* Failure after cooldown inserts rolls the entire activation back. */
  assert(estate_query("CREATE TRIGGER reject_activation BEFORE INSERT ON legendary_activations FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='fixture rejection'"));
  assert(!legendary_activate(ch,LEG_T01,ch));assert(!scalar("SELECT COUNT(*) FROM legendary_cooldowns"));
  assert(estate_query("DROP TRIGGER reject_activation"));
  effect_spec(1,LEG_EFFECT_PREPARED,2,0,0,12);
  command(ch,"use T01");assert(strstr(output,"activate")&&activations==1&&legendary_effect_value(ch,1)==2);
  GET_WAIT_STATE(ch)=0;assert(!legendary_activate(ch,LEG_T05,ch));
  legendary_effects_test_time+=5;assert(!legendary_activate(ch,LEG_T05,ch));
  legendary_effects_test_time++;effect_spec(5,LEG_EFFECT_REDUCTION,0,20,30,18);next_effect.flags=LEG_EFFECT_PHYSICAL;
  assert(legendary_activate(ch,LEG_T05,ch));GET_WAIT_STATE(ch)=0;
  legendary_effects_test_time+=6;assert(!legendary_activate(ch,LEG_S01,ch)); /* family still locked */
  /* floor(4*20%)=0 does not consume; wrong packet type cannot consume. */
  assert(legendary_effects_damage(&enemy,ch,4,DAM_BLUDGEON,false,0)==4);
  assert(legendary_effects_damage(&enemy,ch,50,DAM_FIRE,false,0)==50);
  assert(legendary_effects_damage(&enemy,ch,49,DAM_BLUDGEON,false,0)==40);
  assert(legendary_effects_damage(&enemy,ch,49,DAM_BLUDGEON,false,0)==49);
  assert(legendary_budget_flush());
  assert(scalar("SELECT SUM(amount) FROM legendary_budgets WHERE player_id=1 AND budget_kind=1")==9);
  /* Individual cooldown survives ephemeral cleanup and connection replacement. */
  legendary_effects_free(ch);legendary_loadout_free(ch);legendary_loadout_refresh(ch);
  mysql_close(conn);conn=mysql_init(NULL);assert(mysql_real_connect(conn,NULL,"root",NULL,"estate_test",0,socket,0));
  assert(!legendary_activate(ch,LEG_T01,ch));assert(!legendary_effect_value(ch,1));
  assert(legendary_budget_debit(ch,LEG_BUDGET_PREVENTION,100)==91);
  assert(!legendary_budget_debit(ch,LEG_BUDGET_PREVENTION,1));
  /* Exact rolling boundaries and wall-clock rollback cannot refill allowance. */
  legendary_effects_test_time-=20;assert(!legendary_budget_debit(ch,LEG_BUDGET_PREVENTION,1));
  legendary_effects_test_time+=20+59;assert(!legendary_budget_debit(ch,LEG_BUDGET_PREVENTION,1));
  legendary_effects_test_time++;assert(legendary_budget_debit(ch,LEG_BUDGET_PREVENTION,100)==100);
  assert(legendary_budget_debit(ch,LEG_BUDGET_OFFENSE,999)==60);assert(!legendary_budget_debit(ch,LEG_BUDGET_OFFENSE,1));
  /* Ordinary packets never touch offensive allowance, bonus packets do, and
   * neither redirected nor bonus origins recursively trigger a prepared ward. */
  reset_activation(ch);effect_spec(6,LEG_EFFECT_REDUCTION,0,50,40,18);assert(legendary_activate(ch,LEG_T05,ch));
  assert(legendary_effects_damage(&enemy,ch,20,DAM_BLUDGEON,false,LEG_ORIGIN_REDIRECT)==20);
  GET_HIT(&enemy)=5;assert(legendary_effects_damage(ch,&enemy,50,DAM_FIRE,false,LEG_ORIGIN_BONUS)==5);
  assert(legendary_effects_damage(ch,&enemy,0,DAM_FIRE,false,LEG_ORIGIN_BONUS)==0);
  GET_HIT(&enemy)=100;assert(legendary_effects_damage(ch,&enemy,100,DAM_FIRE,false,LEG_ORIGIN_BONUS)==55);
  assert(!legendary_effects_damage(ch,&enemy,100,DAM_FIRE,false,LEG_ORIGIN_BONUS));
  assert(legendary_effects_damage(&enemy,ch,20,DAM_BLUDGEON,false,0)==10);
  /* Continuous wards consume only their actual remaining pool. */
  reset_activation(ch);effect_spec(60,LEG_EFFECT_REDUCTION,0,50,100,18);
  next_effect.charges=0;next_effect.pool=15;assert(legendary_activate(ch,LEG_T05,ch));
  assert(legendary_effects_damage(&enemy,ch,20,DAM_FIRE,false,0)==10);
  assert(legendary_effects_damage(&enemy,ch,20,DAM_FIRE,false,0)==15);
  assert(legendary_effects_damage(&enemy,ch,20,DAM_FIRE,false,0)==20);
  /* Strongest scalar value and longest equal-value expiry; expiry is exact. */
  reset_activation(ch);effect_spec(14,LEG_EFFECT_PREPARED,3,0,0,12);assert(legendary_activate(ch,LEG_T01,ch));
  other=last_activation;next_effect.magnitude=2;next_effect.duration=18;
  assert(legendary_effect_prepare(&other,&next_effect));
  assert(legendary_effect_value(ch,14)==3); /* strongest value, longest duration */
  assert(legendary_effect_consume(ch,14));assert(!legendary_effect_value(ch,14));
  next_effect.magnitude=3;next_effect.duration=12;assert(legendary_effect_prepare(&other,&next_effect));
  legendary_effects_test_time+=11;assert(legendary_effect_value(ch,14)==3);
  legendary_effects_test_time++;assert(!legendary_effect_value(ch,14));
  /* Delayed healing grants only effective HP, at the scheduled ticks, including
   * a final tick exactly at expiry; reconnect never recreates the timer. */
  reset_activation(ch);GROUP(ch)=GROUP((&ally))=&group;
  effect_spec(50,LEG_EFFECT_HEAL_TICKS,10,0,0,12);next_effect.interval=6;next_effect.ticks=2;next_effect.flags=LEG_EFFECT_ROOM_BOUND;
  GET_HIT(&ally)=75;assert(legendary_activate(ch,LEG_S01,&ally));
  legendary_effects_pulse();assert(GET_HIT(&ally)==75);
  legendary_effects_test_time+=5;legendary_effects_pulse();assert(GET_HIT(&ally)==75);
  legendary_effects_test_time++;legendary_effects_pulse();assert(GET_HIT(&ally)==85);
  legendary_effects_test_time+=6;legendary_effects_pulse();assert(GET_HIT(&ally)==95);
  legendary_effects_pulse();assert(GET_HIT(&ally)==95);
  assert(legendary_heal(ch,&ally,100)==5);assert(!legendary_heal(ch,&ally,100));
  GET_HIT(&ally)=1;assert(legendary_heal(ch,&ally,100)==75);assert(!legendary_heal(ch,&ally,100));
  /* Source departure, recipient departure and source gear loss cancel leases. */
  reset_activation(ch);GET_HIT(&ally)=50;assert(legendary_activate(ch,LEG_S01,&ally));
  legendary_effects_depart(&ally);legendary_effects_test_time+=6;legendary_effects_pulse();assert(GET_HIT(&ally)==50);
  reset_activation(ch);assert(legendary_activate(ch,LEG_S01,&ally));test_remove(ch,WEAR_SHIELD);
  legendary_effects_test_time+=6;legendary_effects_pulse();assert(GET_HIT(&ally)==50);test_wear(ch,shield,WEAR_SHIELD);
  /* Redirect counts only safely transferred points; excess stays on recipient. */
  reset_activation(ch);effect_spec(7,LEG_EFFECT_REDIRECT,0,20,30,12);next_effect.flags=LEG_EFFECT_PHYSICAL|LEG_EFFECT_ROOM_BOUND;
  GET_HIT(ch)=4;GET_HIT(&ally)=100;assert(legendary_activate(ch,LEG_S01,&ally));
  assert(legendary_effects_damage(&enemy,&ally,100,DAM_BLUDGEON,false,0)==97&&GET_HIT(ch)==1);
  assert(legendary_effects_damage(&enemy,&ally,100,DAM_BLUDGEON,false,0)==100);
  /* No resource grants for resource-less characters; payment and refund share
   * a real 180-second budget and cannot fill beyond the actual maximum. */
  assert(!legendary_resource_restore(ch,20));CLASS_LEVEL(ch,CLASS_WIZARD)=1;GET_PSP(ch)=100;GET_MAX_PSP(ch)=100;
  assert(!legendary_resource_restore(ch,20));GET_PSP(ch)=40;
  assert(legendary_resource_pay(ch,30,8)==22&&GET_PSP(ch)==18);
  assert(legendary_resource_restore(ch,20)==12&&GET_PSP(ch)==30);
  assert(!legendary_resource_restore(ch,20));hp=GET_PSP(ch);assert(legendary_resource_pay(ch,40,8)==-1&&GET_PSP(ch)==hp);
  legendary_effects_test_time+=179;assert(!legendary_resource_restore(ch,20));legendary_effects_test_time++;assert(legendary_resource_restore(ch,20)==20);
  GET_PSP(ch)=23;assert(legendary_resource_pay(ch,30,8)==-1&&GET_PSP(ch)==23);
  legendary_effects_test_time+=180;
  assert(legendary_resource_pay(ch,30,8)==22&&GET_PSP(ch)==1);
  /* Cross-caster recipient budgets are keyed by player ID, not address. */
  init_player(&reloaded,"reload",2);GET_HIT(&reloaded)=1;GET_POS(&reloaded)=POS_STANDING;
  assert(legendary_budget_flush());
  n=scalar("SELECT COUNT(*) FROM legendary_budgets WHERE player_id=2");assert(n>0);
  assert(legendary_budget_debit(&reloaded,LEG_BUDGET_HEALING,999)==100);
  assert(!legendary_budget_debit(&ally,LEG_BUDGET_HEALING,1));
  /* NPC recipients have the same bounded window, but are forgotten on extraction. */
  SET_BIT_AR(MOB_FLAGS(&enemy),MOB_ISNPC);
  assert(legendary_budget_debit(&enemy,LEG_BUDGET_PREVENTION,200)==100);assert(!legendary_budget_debit(&enemy,LEG_BUDGET_PREVENTION,1));
  legendary_effects_free(&enemy);assert(legendary_budget_debit(&enemy,LEG_BUDGET_PREVENTION,200)==100);
  /* Ordinary target policy: visibility, group, peaceful, staff, no-PK and pets. */
  assert(legendary_target_legal(ch,&ally,LEG_TARGET_ALLY));GROUP((&ally))=NULL;assert(!legendary_target_legal(ch,&ally,LEG_TARGET_ALLY));GROUP((&ally))=&group;
  assert(legendary_target_legal(ch,&enemy,LEG_TARGET_ENEMY));SET_BIT_AR(ROOM_FLAGS(IN_ROOM(ch)),ROOM_PEACEFUL);assert(!legendary_target_legal(ch,&enemy,LEG_TARGET_ENEMY));REMOVE_BIT_AR(ROOM_FLAGS(IN_ROOM(ch)),ROOM_PEACEFUL);
  SET_BIT_AR(MOB_FLAGS(&enemy),MOB_NOKILL);assert(!legendary_target_legal(ch,&enemy,LEG_TARGET_ENEMY));REMOVE_BIT_AR(MOB_FLAGS(&enemy),MOB_NOKILL);
  enemy.master=&ally;assert(!legendary_target_legal(ch,&enemy,LEG_TARGET_ENEMY));enemy.master=NULL;
  SET_BIT_AR(MOB_FLAGS((&petmaster)),MOB_ISNPC);petmaster.master=&ally;enemy.master=&petmaster;
  assert(!legendary_target_legal(ch,&enemy,LEG_TARGET_ENEMY));
  config_info.play.pk_allowed=true;enemy.master=ch;assert(!legendary_target_legal(ch,&enemy,LEG_TARGET_ENEMY));
  config_info.play.pk_allowed=false;enemy.master=NULL;
  SET_BIT_AR(AFF_FLAGS(&enemy),AFF_INVISIBLE);assert(!legendary_target_legal(ch,&enemy,LEG_TARGET_ENEMY));REMOVE_BIT_AR(AFF_FLAGS(&enemy),AFF_INVISIBLE);
  GET_LEVEL(&enemy)=LVL_IMMORT;assert(legendary_target_legal(ch,&enemy,LEG_TARGET_ENEMY));
  REMOVE_BIT_AR(MOB_FLAGS(&enemy),MOB_ISNPC);assert(!legendary_target_legal(ch,&enemy,LEG_TARGET_ENEMY));
  SET_BIT_AR(MOB_FLAGS(&enemy),MOB_ISNPC);GET_LEVEL(&enemy)=20;
  /* Hostile powers default to the current legal opponent. */
  assert(legendary_activation_register(LEG_S09,LEG_TARGET_ENEMY,synthetic_check,synthetic_apply));
  test_layout(ch,blade,0,2,shield,0,1,shield,0,2);reset_activation(ch);
  next_effect.key=0;FIGHTING(ch)=&enemy;hp=activations;command(ch,"use S09");
  assert(activations==hp+1&&last_activation.target==&enemy);FIGHTING(ch)=NULL;
  /* Cancellation does not delete any persistent cooldown/budget entry. */
  n=scalar("SELECT COUNT(*) FROM legendary_cooldowns");legendary_effects_cancel(ch);
  assert(scalar("SELECT COUNT(*) FROM legendary_cooldowns")==n);
  for(i=0;i<4;i++)assert(!legendary_budget_debit(NULL,i,1));
  legendary_effects_free(ch);legendary_effects_free(&ally);legendary_effects_free(&enemy);legendary_effects_free(&reloaded);
  GROUP(ch)=GROUP((&ally))=NULL;char_from_room(&ally);char_from_room(&enemy);char_from_room(&reloaded);
  puts("legendary Phase 9: activation/durable cooldowns/source timers/rolling budgets/post-defense accounting PASS");
}
