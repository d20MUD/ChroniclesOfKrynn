#include "legendary_foundations.h"
#include "fight.h"
bool test_real_push(struct char_data *,struct char_data *,bool);
static int test_push_result;
int combat_maneuver_check(struct char_data *a,struct char_data *b,int kind,int bonus)
{(void)a;(void)b;(void)bonus;assert(kind==COMBAT_MANEUVER_TYPE_FORCED_MOVEMENT);return test_push_result;}
int savingthrow(struct char_data *a,struct char_data *b,int type,int modifier,int casttype,int level,int school)
{(void)a;(void)b;(void)type;(void)modifier;(void)casttype;(void)level;(void)school;return 0;}
static void foundation_reset(struct char_data *ch)
{
  int i;legendary_infuse_cleanup(ch);legendary_loadout_free(ch);legendary_foundations_free(ch);
  for(i=0;i<NUM_WEARS;i++)if(GET_EQ(ch,i))obj_to_char(unequip_char(ch,i),ch);
  while(ch->carrying)extract_obj(ch->carrying);
  assert(estate_query("DELETE FROM legendary_channels"));assert(estate_query("DELETE FROM legendary_items"));
  assert(estate_query("UPDATE legendary_characters SET passive_priorities=NULL,major_selections=NULL WHERE player_id=1"));
  for(i=0;i<NUM_CLASSES;i++)CLASS_LEVEL(ch,i)=0;
  for(i=0;i<NUM_OF_SAVING_THROWS;i++)GET_REAL_SAVE(ch,i)=0;
  for(i=0;i<NUM_DAM_TYPES;i++)GET_REAL_RESISTANCES(ch,i)=0;
  ch->real_abils.str=ch->real_abils.con=ch->real_abils.intel=ch->real_abils.wis=ch->real_abils.cha=10;ch->real_abils.dex=18;
  GET_LEVEL(ch)=30;GET_POS(ch)=POS_STANDING;FIGHTING(ch)=NULL;GET_DISGUISE_RACE(ch)=0;REMOVE_BIT_AR(AFF_FLAGS(ch),AFF_WILD_SHAPE);
  GET_REAL_MAX_HIT(ch)=GET_REAL_MAX_MOVE(ch)=GET_REAL_MAX_PSP(ch)=100;
  GET_HIT(ch)=70;GET_MOVE(ch)=30;GET_PSP(ch)=40;
  test_pool_cap=10000;test_save_cap=15;test_ability_cap=32;legendary_test_wear_allowed=true;
  affect_total(ch);
}
static void foundation_set(struct char_data *ch,struct obj_data *o,int id,int subtype,int rank)
{
  struct legendary_item_record r=saved(o);struct legendary_channel_record *c=&r.channels[0];
  c->foundation=id;c->subtype=subtype;c->rank=rank;
  if(rank<3)c->signature=0;
  if(rank<2)c->technique=0;
  assert(legendary_store_save_channel(&r,0,c->version));
  legendary_object_unload(o);o->legendary_quarantined=false;assert(legendary_object_verify(o));
  legendary_loadout_invalidate(ch);legendary_foundations_refresh(ch);
}
static struct obj_data *foundation_item(struct char_data *ch,int id,int slot,int wear)
{
  struct obj_data *o=legendary_test_item(ch,"foundation",slot,id==8?1:id,false);
  test_wear(ch,o,wear);legendary_foundations_refresh(ch);return o;
}
static void foundation_cases(struct char_data *ch)
{
  struct obj_data *o,*a,*b,*outfit[9],*weapon;
  struct legendary_item_record r;struct affected_type penalty={0};struct char_data enemy={0};
  int i,j,save,current,rank,base[6];long q;char *packed;char uuid[33];
  static const int slots[]={0,6,1,2,3,4,5,8,9};
  static const int positions[]={WEAR_WIELD_1,WEAR_SHIELD,WEAR_ARMS,WEAR_LEGS,WEAR_HEAD,WEAR_BODY,WEAR_ABOUT,WEAR_FEET,WEAR_HANDS};
  {
    int bits[TW_ARRAY_MAX]={0};
    SET_BIT_AR(bits,31);assert(IS_SET_AR(bits,31));REMOVE_BIT_AR(bits,31);assert(!IS_SET_AR(bits,31));
    TOGGLE_BIT_AR(bits,31);assert(IS_SET_AR(bits,31));TOGGLE_BIT_AR(bits,31);assert(!IS_SET_AR(bits,31));
  }
  /* F01/F02: ordinary weapons only, global flat ceiling across nine channels. */
  foundation_reset(ch);o=foundation_item(ch,1,0,WEAR_WIELD_1);
  assert(legendary_foundation_weapon(ch,o,false)==1);assert(!legendary_foundation_weapon(ch,NULL,false));
  foundation_set(ch,o,1,0,1);assert(legendary_foundation_weapon(ch,o,false)==1);foundation_set(ch,o,1,0,5);assert(legendary_foundation_weapon(ch,o,false)==1);
  foundation_reset(ch);
  for(i=0;i<9;i++)outfit[i]=foundation_item(ch,2,slots[i],positions[i]);
  weapon=outfit[0];assert(legendary_foundation_value(ch,2,0)==2);
  assert(legendary_foundation_weapon(ch,weapon,true)==2);
  for(rank=1;rank<=5;rank++){foundation_set(ch,weapon,2,0,rank);assert(legendary_foundation_weapon(ch,weapon,true)==2);}
  GET_OBJ_TYPE(weapon)=ITEM_FIREWEAPON;assert(legendary_foundation_weapon(ch,weapon,true)==2);
  assert(!legendary_foundation_weapon(ch,NULL,true));GET_OBJ_TYPE(weapon)=ITEM_WEAPON;
  assert(test_real_force_finish(MODE_NORMAL_HIT,400,2)==402); /* already-critical ordinary damage */
  assert(test_real_force_finish(MODE_DISPLAY_PRIMARY,400,2)==400);
  /* F03: strongest positive deflection wins; penalties are never erased. */
  foundation_reset(ch);o=foundation_item(ch,3,4,WEAR_BODY);
  assert(legendary_foundation_deflection(ch,0)==1);assert(legendary_foundation_deflection(ch,5)==5);
  penalty.location=APPLY_AC_NEW;penalty.bonus_type=BONUS_TYPE_DEFLECTION;penalty.modifier=-2;ch->affected=&penalty;
  assert(legendary_foundation_deflection(ch,-2)==-1);assert(legendary_foundation_deflection(ch,3)==3);ch->affected=NULL;
  test_remove(ch,WEAR_BODY);assert(!legendary_foundation_actual(ch,3,0));
  /* F04-F06: exact +1, separate saves, existing ordinary caps. */
  for(i=4;i<=6;i++){
    static const int save_types[]={SAVING_FORT,SAVING_REFL,SAVING_WILL};
    foundation_reset(ch);save=save_types[i-4];o=foundation_item(ch,i,4,WEAR_BODY);
    assert(GET_SAVE(ch,save)==1);for(j=0;j<NUM_OF_SAVING_THROWS;j++)if(j!=save)assert(GET_SAVE(ch,j)==0);
    test_save_cap=0;affect_total(ch);assert(GET_SAVE(ch,save)==0&&legendary_foundation_actual(ch,i,0)==0);
  }
  /* F07: every subtype uses the normal cap, and real/base abilities never change. */
  foundation_reset(ch);o=foundation_item(ch,7,4,WEAR_BODY);
  for(i=0;i<6;i++) {
    base[0]=ch->real_abils.str;base[1]=ch->real_abils.dex;base[2]=ch->real_abils.con;
    base[3]=ch->real_abils.intel;base[4]=ch->real_abils.wis;base[5]=ch->real_abils.cha;
    foundation_set(ch,o,7,i,5);assert(legendary_foundation_actual(ch,7,i)==1);
    assert(GET_REAL_STR(ch)==base[0]&&GET_REAL_DEX(ch)==base[1]&&GET_REAL_CON(ch)==base[2]);
  }
  foundation_set(ch,o,7,0,5);test_ability_cap=10;affect_total(ch);assert(GET_STR(ch)==10&&legendary_foundation_actual(ch,7,0)==0);
  /* F08: real training only; no ranks, crafting progress, or wrong-skill bonus. */
  foundation_reset(ch);GET_ABILITY(ch,1)=5;o=foundation_item(ch,8,4,WEAR_BODY);foundation_set(ch,o,8,1,5);
  assert(legendary_foundation_skill(ch,1)==2&&GET_ABILITY(ch,1)==5);assert(!legendary_foundation_skill(ch,4));assert(!legendary_foundation_skill(ch,ABILITY_CRAFT_ALCHEMY));
  GET_ABILITY(ch,1)=0;assert(!legendary_foundation_skill(ch,1));
  {static const int real_checks[]={ABILITY_RELIGION,ABILITY_CONCENTRATION,ABILITY_DISCIPLINE,ABILITY_TOTAL_DEFENSE};
   for(i=0;i<4;i++){GET_ABILITY(ch,real_checks[i])=5;foundation_set(ch,o,8,real_checks[i],5);assert(legendary_foundation_skill(ch,real_checks[i])==2);}}
  GET_ABILITY(ch,ABILITY_LINGUISTICS)=5;assert(!legendary_supported_skill(ABILITY_LINGUISTICS));
  /* F09/F10/F11: increases never refill; removals clamp, re-equip cannot restore. */
  foundation_reset(ch);o=foundation_item(ch,9,4,WEAR_BODY);a=foundation_item(ch,9,1,WEAR_ARMS);
  assert(GET_MAX_HIT(ch)==140&&GET_REAL_MAX_HIT(ch)==100&&GET_HIT(ch)==70);
  for(i=0;i<20;i++){affect_total(ch);assert(GET_MAX_HIT(ch)==140&&GET_HIT(ch)==70);}
  GET_HIT(ch)=130;test_remove(ch,WEAR_ARMS);legendary_foundations_refresh(ch);assert(GET_MAX_HIT(ch)==120&&GET_HIT(ch)==120);
  test_remove(ch,WEAR_BODY);legendary_foundations_refresh(ch);assert(GET_MAX_HIT(ch)==100&&GET_HIT(ch)==100);
  test_wear(ch,o,WEAR_BODY);legendary_foundations_refresh(ch);assert(GET_MAX_HIT(ch)==120&&GET_HIT(ch)==100);
  /* Production object codec reconnect round trip: no hidden base-stat applies. */
  GET_HIT(ch)=70;memcpy(uuid,o->legendary_uuid,33);packed=estate_pack(o);assert(packed);
  test_remove(ch,WEAR_BODY);obj_from_char(o);extract_obj(o);legendary_loadout_free(ch);legendary_foundations_free(ch);affect_total(ch);
  b=estate_unpack(packed);free(packed);assert(b&&!strcmp(b->legendary_uuid,uuid));obj_to_char(b,ch);test_wear(ch,b,WEAR_BODY);legendary_foundations_refresh(ch);
  assert(GET_MAX_HIT(ch)==120&&GET_REAL_MAX_HIT(ch)==100&&GET_HIT(ch)==70);
  foundation_reset(ch);o=foundation_item(ch,10,4,WEAR_BODY);a=foundation_item(ch,10,1,WEAR_ARMS);
  assert(GET_MAX_MOVE(ch)==180&&GET_MOVE(ch)==30&&GET_REAL_MAX_MOVE(ch)==100);
  test_pool_cap=150;affect_total(ch);assert(GET_MAX_MOVE(ch)==150&&legendary_foundation_actual(ch,10,0)==50);
  assert(legendary_foundation_channel_actual(ch,o->legendary_uuid,0)+legendary_foundation_channel_actual(ch,a->legendary_uuid,0)==50);
  GET_MOVE(ch)=150;test_remove(ch,WEAR_ARMS);legendary_foundations_refresh(ch);test_remove(ch,WEAR_BODY);legendary_foundations_refresh(ch);assert(GET_MOVE(ch)==100);
  test_wear(ch,o,WEAR_BODY);test_wear(ch,a,WEAR_ARMS);legendary_foundations_refresh(ch);assert(GET_MOVE(ch)==100);
  foundation_reset(ch);o=foundation_item(ch,11,4,WEAR_BODY);assert(GET_MAX_PSP(ch)==100&&GET_PSP(ch)==40);
  CLASS_LEVEL(ch,CLASS_WIZARD)=1;legendary_foundations_refresh(ch);assert(GET_MAX_PSP(ch)==120&&GET_REAL_MAX_PSP(ch)==100&&GET_PSP(ch)==40);
  GET_PSP(ch)=110;CLASS_LEVEL(ch,CLASS_WIZARD)=0;legendary_foundations_refresh(ch);assert(GET_MAX_PSP(ch)==100&&GET_PSP(ch)==100);
  CLASS_LEVEL(ch,CLASS_PSIONICIST)=1;legendary_foundations_refresh(ch);assert(GET_MAX_PSP(ch)==120&&GET_PSP(ch)==100);
  /* F12: exact percentage points, no self-feedback, ordinary >75 stays intact. */
  foundation_reset(ch);GET_REAL_RESISTANCES(ch,DAM_ACID)=73;affect_total(ch);o=foundation_item(ch,12,4,WEAR_BODY);
  assert(GET_RESISTANCES(ch,DAM_ACID)==75&&legendary_foundation_actual(ch,12,0)==2);
  q=queries();for(i=0;i<50;i++){affect_total(ch);assert(GET_RESISTANCES(ch,DAM_ACID)==75);}assert(queries()-q==1);
  GET_REAL_RESISTANCES(ch,DAM_ACID)=85;affect_total(ch);assert(GET_RESISTANCES(ch,DAM_ACID)==85&&legendary_foundation_actual(ch,12,0)==0);
  for(i=0;i<5;i++){foundation_set(ch,o,12,i,5);assert(legendary_foundation_actual(ch,12,i)==(i?3:0));}
  /* F13/F14: defender checks only, never unopposed immunity or generic attacks. */
  foundation_reset(ch);o=foundation_item(ch,13,4,WEAR_BODY);
  assert(legendary_foundation_maneuver(ch,COMBAT_MANEUVER_TYPE_KNOCKDOWN)==2);
  assert(legendary_foundation_maneuver(ch,COMBAT_MANEUVER_TYPE_FORCED_MOVEMENT)==2);
  assert(!legendary_foundation_maneuver(ch,COMBAT_MANEUVER_TYPE_DISARM));
  SET_BIT_AR(MOB_FLAGS(&enemy),MOB_ISNPC);GET_LEVEL(&enemy)=20;
  test_push_result=-2;assert(!test_real_push(ch,&enemy,false));test_push_result=0;assert(!test_real_push(ch,&enemy,false));
  test_push_result=1;assert(test_real_push(ch,&enemy,false));
  foundation_set(ch,o,14,0,5);assert(legendary_foundation_maneuver(ch,COMBAT_MANEUVER_TYPE_DISARM)==2);
  assert(!legendary_foundation_maneuver(ch,COMBAT_MANEUVER_TYPE_KNOCKDOWN));
  /* F15/F16: final walking cost and normal regeneration cadence only. */
  foundation_reset(ch);o=foundation_item(ch,15,4,WEAR_BODY);
  assert(legendary_foundation_walk(ch,100,false)==90&&legendary_foundation_walk(ch,1,false)==1);
  assert(legendary_foundation_walk(ch,100,true)==100&&legendary_foundation_walk(ch,0,false)==0);
  foundation_set(ch,o,16,0,5);assert(legendary_foundation_regen(ch)==5);
  legendary_loadout_combat_start(ch);FIGHTING(ch)=&enemy;assert(!legendary_foundation_regen(ch));FIGHTING(ch)=NULL;
  current=GET_HIT(ch);for(i=0;i<100;i++)assert(legendary_foundation_regen(ch)==5);assert(GET_HIT(ch)==current);
  r=saved(o);assert(r.channels[0].rank==5);assert(GET_REAL_MAX_HIT(ch)==100);
  puts("legendary Phase 8: all foundations, typed/capped bonuses, nine-channel damage, no-refill/reload PASS");
}
