#include "legendary_equipment.h"
#include "quest.h"
/* Production give/drop functions are extracted verbatim by the runner. */
int test_real_force_finish(int mode,int dam,int foundation_damage);
bool test_real_give(struct char_data *,struct char_data *,struct obj_data *);
int test_real_drop(struct char_data *,struct obj_data *,byte,const char *,room_rnum);
int give_otrigger(struct obj_data *o,struct char_data *a,struct char_data *v){return 1;}
int receive_mtrigger(struct char_data *v,struct char_data *a,struct obj_data *o){return 1;}
int drop_otrigger(struct obj_data *o,struct char_data *a){return 1;}
int drop_wtrigger(struct obj_data *o,struct char_data *a){return 1;}
void autoquest_trigger_check(struct char_data *ch,struct char_data *v,struct obj_data *o,int x,int y){}
static long queries(void)
{
  MYSQL_RES *r=estate_select("SHOW SESSION STATUS LIKE 'Questions'");MYSQL_ROW row;long n;
  assert(r);row=mysql_fetch_row(r);assert(row);n=atol(row[1]);mysql_free_result(r);return n;
}
static struct obj_data *legendary_test_item(struct char_data *ch,const char *name,int slot,int foundation,bool support)
{
  struct legendary_item_record r;struct obj_data *o=item(name);int i;
  memset(&r,0,sizeof(r));assert(legendary_uuid_generate(r.uuid));r.owner=GET_IDNUM(ch);r.prototype=500;r.slot=slot;
  assert(legendary_store_insert(&r));
  for(i=0;i<(support?2:1);i++){
    struct legendary_channel_record *c=&r.channels[i];c->present=true;c->rank=5;c->path=slot==6?2:slot==7?3:11;
    c->family=i?LEG_FAMILY_GUARD:0;c->foundation=foundation;c->technique=(slot==6||i)?105:slot==7?115:101;
    c->signature=(slot==6||i)?201:slot==7?207:209;c->first_choices=7;c->version=0;assert(legendary_cost_between(0,5,&c->paid));
    assert(legendary_store_save_channel(&r,i,-1));
  }
  assert(legendary_store_set_state(r.uuid,r.owner,0,LEG_RECORD_ACTIVE));
  o->item_number=0;GET_ID(o)=5000+scalar("SELECT COUNT(*) FROM legendary_items");GET_OBJ_LEVEL(o)=25;GET_OBJ_BOUND_ID(o)=GET_IDNUM(ch);
  GET_OBJ_TYPE(o)=slot==0?ITEM_WEAPON:ITEM_ARMOR;
  {static const int f[]={ITEM_WEAR_WIELD,ITEM_WEAR_ARMS,ITEM_WEAR_LEGS,ITEM_WEAR_HEAD,ITEM_WEAR_BODY,ITEM_WEAR_ABOUT,ITEM_WEAR_SHIELD,ITEM_WEAR_HOLD,ITEM_WEAR_FEET,ITEM_WEAR_HANDS};SET_BIT_AR(GET_OBJ_WEAR(o),f[slot]);}
  memcpy(o->legendary_uuid,r.uuid,33);obj_to_char(o,ch);return o;
}
static void test_wear(struct char_data *ch,struct obj_data *o,int position)
{
  assert(!GET_EQ(ch,position));if(o->carried_by)obj_from_char(o);
  GET_EQ(ch,position)=o;o->worn_by=ch;o->worn_on=position;legendary_object_live(o);legendary_loadout_invalidate(ch);
}
static void test_remove(struct char_data *ch,int position)
{struct obj_data *o=unequip_char(ch,position);legendary_loadout_invalidate(ch);obj_to_char(o,ch);}
static const struct legendary_equipped_channel *view_channel(const struct legendary_loadout *v,struct obj_data *o,int channel)
{int i;for(i=0;i<v->count;i++)if(v->channels[i].channel==channel&&!strcmp(v->channels[i].uuid,o->legendary_uuid))return &v->channels[i];return NULL;}
static int active_majors(const struct legendary_loadout *v)
{int i,n=0;for(i=0;i<v->major_count;i++)if(v->majors[i].reason==LEG_EQUIPPED)n++;return n;}
static void test_layout(struct char_data *ch,struct obj_data *a,int ac,int ak,struct obj_data *b,int bc,int bk,struct obj_data *c,int cc,int ck)
{
  char refs[160];snprintf(refs,sizeof(refs),"%s/%d/%d %s/%d/%d %s/%d/%d",a->legendary_uuid,ac,ak,b->legendary_uuid,bc,bk,c->legendary_uuid,cc,ck);
  assert(estate_query("UPDATE legendary_characters SET major_selections='%s',version=version+1 WHERE player_id=%ld",refs,GET_IDNUM(ch)));
  legendary_loadout_refresh(ch);
}
static void revoke_test_ward(struct char_data *ch,void *context)
{int *ward=context;assert(*ward>0);(*ward)--;affect_total(ch);legendary_loadout_invalidate(ch); /* exercise callback reentrancy */}
static void equipment_cases(struct char_data *ch)
{
  struct obj_data *blade,*shield,*off,*two,*bag,*clone,*focus,*armor[7],*restored;
  struct char_data other;const struct legendary_loadout *v;struct legendary_item_record r;struct legendary_cost pair;
  int i,value,ward=0;long q,n,gold;unsigned long long escrow;char uuid[33];
  static const int positions[]={WEAR_ARMS,WEAR_LEGS,WEAR_HEAD,WEAR_BODY,WEAR_ABOUT,WEAR_FEET,WEAR_HANDS};
  static const int slots[]={1,2,3,4,5,8,9};
  static const int foundations[]={1,3,4,5,6,9,10};
  legendary_infuse_cleanup(ch);legendary_loadout_free(ch);
  for(i=0;i<NUM_WEARS;i++)if(GET_EQ(ch,i))obj_to_char(unequip_char(ch,i),ch);
  while(ch->carrying)extract_obj(ch->carrying);
  assert(estate_query("DELETE FROM legendary_channels"));assert(estate_query("DELETE FROM legendary_items"));
  assert(estate_query("UPDATE legendary_characters SET passive_priorities=NULL,major_selections=NULL WHERE player_id=1"));
  GET_LEVEL(ch)=30;GET_HIT(ch)=GET_MAX_HIT(ch)=100;GET_POS(ch)=POS_STANDING;FIGHTING(ch)=NULL;
  init_player(&other,"other",2);GET_LEVEL(&other)=30;GET_HIT(&other)=100;GET_POS(&other)=POS_STANDING;
  blade=legendary_test_item(ch,"ownerblade",0,2,false);shield=legendary_test_item(ch,"ownershield",6,2,false);
  off=legendary_test_item(ch,"offblade",0,2,false);two=legendary_test_item(ch,"twoblade",0,2,true);
  focus=legendary_test_item(ch,"focus",7,11,false);
  test_wear(ch,blade,WEAR_WIELD_1);test_wear(ch,shield,WEAR_SHIELD);
  for(i=0;i<7;i++){armor[i]=legendary_test_item(ch,"outfit",slots[i],foundations[i],false);test_wear(ch,armor[i],positions[i]);}
  test_layout(ch,blade,0,1,shield,0,1,shield,0,2);
  v=legendary_loadout_get(ch);assert(v->ready&&v->count==9&&v->points<=12&&active_majors(v)==3);
  value=0;n=0;for(i=0;i<v->count;i++){if(v->channels[i].foundation==2)value+=v->channels[i].passive.value;if(v->channels[i].passive.reason==LEG_PASSIVE_BUDGET)n++;}
  assert(value<=2&&n>0);
  /* A saved foundation priority, not wear order, controls the scarce budget. */
  assert(estate_query("UPDATE legendary_characters SET passive_priorities='%s/0',version=version+1 WHERE player_id=1",armor[4]->legendary_uuid));
  legendary_loadout_refresh(ch);v=legendary_loadout_get(ch);assert(!strcmp(v->channels[0].uuid,armor[4]->legendary_uuid));
  {
    int dormant=-1,remove=-1;
    for(i=0;i<v->count;i++) {
      if(dormant<0&&v->channels[i].passive.reason==LEG_PASSIVE_BUDGET)dormant=v->channels[i].wear;
      if(remove<0&&v->channels[i].passive.points==3)remove=v->channels[i].wear;
    }
    assert(dormant>=0&&remove>=0);
    legendary_loadout_combat_start(ch);FIGHTING(ch)=&other;
    {struct obj_data *removed=GET_EQ(ch,remove),*waiting=GET_EQ(ch,dormant);
      test_remove(ch,remove);v=legendary_loadout_get(ch);
      assert(view_channel(v,waiting,0)->passive.reason==LEG_PASSIVE_COMBAT);
      FIGHTING(ch)=NULL;legendary_loadout_invalidate(ch);v=legendary_loadout_get(ch);
      assert(view_channel(v,waiting,0)->passive.reason==LEG_PASSIVE_ACTIVE);
      test_wear(ch,removed,remove);v=legendary_loadout_get(ch);
    }
  }
  q=queries();for(i=0;i<1000;i++)assert(legendary_loadout_get(ch)->ready);assert(queries()-q==1);
  assert(estate_query("INSERT INTO legendary_cooldowns VALUES(1,101,9999999999)"));
  assert(estate_query("INSERT INTO legendary_budgets(player_id,budget_kind,amount,occurred_utc,operation_id) VALUES(1,0,10,1,'11111111111111111111111111111111')"));
  ward=1;assert(legendary_source_attach(ch,shield->legendary_uuid,0,201,revoke_test_ward,&ward));
  test_remove(ch,WEAR_SHIELD);assert(ward==0);v=legendary_loadout_get(ch);assert(active_majors(v)==1);
  /* No automatic replacement by the otherwise available fourth power. */
  assert(v->major_count==3);test_wear(ch,off,WEAR_WIELD_OFFHAND);v=legendary_loadout_get(ch);
  assert(view_channel(v,off,0)->reason==LEG_EQUIPPED);assert(active_majors(v)==1);
  test_remove(ch,WEAR_WIELD_1);test_remove(ch,WEAR_WIELD_OFFHAND);test_wear(ch,two,WEAR_WIELD_2H);
  test_layout(ch,two,0,1,two,0,2,two,1,2);v=legendary_loadout_get(ch);
  assert(view_channel(v,two,0)->reason==LEG_EQUIPPED&&view_channel(v,two,1)->reason==LEG_EQUIPPED&&active_majors(v)==3);
  assert(legendary_pair_cost(5,5,&pair)&&pair.xp==2*77000000); /* full two-track cost parity */
  ward=1;assert(legendary_source_attach(ch,two->legendary_uuid,1,201,revoke_test_ward,&ward));
  test_wear(ch,shield,WEAR_SHIELD);assert(ward==0);v=legendary_loadout_get(ch);
  assert(view_channel(v,two,1)->reason==LEG_BAD_GRIP&&active_majors(v)==0);
  test_remove(ch,WEAR_SHIELD);test_remove(ch,WEAR_WIELD_2H);test_wear(ch,two,WEAR_WIELD_1);
  v=legendary_loadout_get(ch);assert(view_channel(v,two,1)->reason==LEG_BAD_GRIP); /* size/empty offhand proves nothing */
  GET_OBJ_TYPE(two)=ITEM_FIREWEAPON;legendary_loadout_invalidate(ch);v=legendary_loadout_get(ch);assert(view_channel(v,two,1)->reason==LEG_EQUIPPED);
  test_remove(ch,WEAR_WIELD_1);test_wear(ch,two,WEAR_ON_BACK);v=legendary_loadout_get(ch);assert(active_majors(v)==0);
  test_remove(ch,WEAR_ON_BACK);GET_OBJ_TYPE(two)=ITEM_WEAPON;test_wear(ch,two,WEAR_WIELD_2H);
  ward=1;assert(legendary_source_attach(ch,two->legendary_uuid,0,101,revoke_test_ward,&ward));
  legendary_test_wear_allowed=false;legendary_loadout_invalidate(ch);assert(!ward);assert(active_majors(legendary_loadout_get(ch))==0);legendary_test_wear_allowed=true;
  ward=1;assert(legendary_source_attach(ch,two->legendary_uuid,0,101,revoke_test_ward,&ward));
  GET_HIT(ch)=-11;legendary_loadout_invalidate(ch);assert(!ward);assert(active_majors(legendary_loadout_get(ch))==0);GET_HIT(ch)=100;
  GET_DISGUISE_RACE(ch)=1;SET_BIT_AR(AFF_FLAGS(ch),AFF_WILD_SHAPE);legendary_loadout_invalidate(ch);assert(active_majors(legendary_loadout_get(ch))==0);
  REMOVE_BIT_AR(AFF_FLAGS(ch),AFF_WILD_SHAPE);GET_DISGUISE_RACE(ch)=0;
  ward=1;assert(legendary_source_attach(ch,two->legendary_uuid,0,101,revoke_test_ward,&ward));
  test_layout(ch,two,0,2,two,1,1,two,1,2);assert(!ward); /* unattune removes only source effects */
  assert(scalar("SELECT expires_utc FROM legendary_cooldowns WHERE player_id=1 AND cooldown_key=101")==9999999999L);
  assert(scalar("SELECT COUNT(*) FROM legendary_budgets WHERE player_id=1")==1);
  ward=1;assert(legendary_source_attach(ch,two->legendary_uuid,1,201,revoke_test_ward,&ward));
  GET_HIT(ch)=-11;legendary_loadout_free(ch);assert(!ward&&!ch->legendary_equipment);GET_HIT(ch)=100;
  /* Resource and trained-skill eligibility is reevaluated without SQL. */
  test_remove(ch,WEAR_WIELD_2H);test_wear(ch,focus,WEAR_HOLD_1);v=legendary_loadout_get(ch);assert(view_channel(v,focus,0)->passive.reason==LEG_PASSIVE_INELIGIBLE);
  test_layout(ch,focus,0,1,focus,0,2,two,0,2);v=legendary_loadout_get(ch);assert(active_majors(v)==0);
  CLASS_LEVEL(ch,CLASS_WIZARD)=1;v=legendary_loadout_get(ch);assert(active_majors(v)==2);assert(view_channel(v,focus,0)->passive.reason!=LEG_PASSIVE_INELIGIBLE);
  ward=1;assert(legendary_source_attach(ch,focus->legendary_uuid,0,207,revoke_test_ward,&ward));
  CLASS_LEVEL(ch,CLASS_WIZARD)=0;legendary_loadout_invalidate(ch);assert(!ward&&active_majors(legendary_loadout_get(ch))==0);
  test_remove(ch,WEAR_HOLD_1);
  /* Recursive barriers apply despite ordinary binding damage or staff status. */
  bag=item("ownerbag");GET_ID(bag)=9900;GET_OBJ_TYPE(bag)=ITEM_CONTAINER;obj_to_char(bag,ch);
  obj_from_char(blade);obj_to_obj(blade,bag);obj_from_char(off);bag->sheath_primary=off;
  assert(legendary_tree_present(bag)&&legendary_tree_allowed(ch,bag,LEG_ROUTE_OWNER));
  assert(!legendary_tree_allowed(&other,bag,LEG_ROUTE_OWNER));assert(!legendary_tree_allowed(ch,bag,LEG_ROUTE_TRANSFER));
  assert(!test_real_give(ch,&other,bag));gold=GET_GOLD(ch);
  assert(!test_real_drop(ch,bag,SCMD_JUNK,"junk",1)&&bag->carried_by==ch&&GET_GOLD(ch)==gold);
  assert(!legendary_tree_allowed(ch,bag,LEG_ROUTE_TRANSFORM));assert(!legendary_tree_allowed(ch,bag,LEG_ROUTE_TRANSFER));
  assert(legendary_storage_check(ch,two,bag));assert(!legendary_storage_check(&other,two,bag));
  n=scalar("SELECT COUNT(*) FROM estate_deliveries");assert(!estate_queue_delivery(&other,bag,100));assert(scalar("SELECT COUNT(*) FROM estate_deliveries")==n);
  assert(!legendary_tree_allowed(ch,shield,LEG_ROUTE_TRANSFORM));GET_OBJ_BOUND_ID(shield)=0;assert(!legendary_tree_allowed(ch,shield,LEG_ROUTE_TRANSFER));GET_OBJ_BOUND_ID(shield)=1;
  /* Real codec/escrow/delivery round trip, including nested and sheath UUIDs. */
  memcpy(uuid,blade->legendary_uuid,33);escrow=estate_prepare_escrow(ch,bag);assert(escrow);
  assert(estate_query("START TRANSACTION"));assert(estate_queue_delivery(ch,bag,0)&&estate_commit_escrow(escrow)&&estate_query("COMMIT"));
  obj_from_char(bag);extract_obj(bag);estate_recover(ch);restored=NULL;
  for(bag=ch->carrying;bag;bag=bag->next_content)if(bag->contains&&!strcmp(bag->contains->legendary_uuid,uuid)){restored=bag;break;}
  assert(restored&&restored->sheath_primary&&legendary_tree_allowed(ch,restored,LEG_ROUTE_OWNER));
  /* Duplicate identities suppress both copies in the same rebuild. */
  test_wear(ch,two,WEAR_WIELD_1);clone=item("duplicate");GET_ID(clone)=9990;clone->item_number=0;GET_OBJ_LEVEL(clone)=25;
  GET_OBJ_BOUND_ID(clone)=1;SET_BIT_AR(GET_OBJ_WEAR(clone),ITEM_WEAR_WIELD);memcpy(clone->legendary_uuid,two->legendary_uuid,33);
  GET_EQ(ch,WEAR_WIELD_OFFHAND)=clone;clone->worn_by=ch;clone->worn_on=WEAR_WIELD_OFFHAND;
  legendary_loadout_invalidate(ch);v=legendary_loadout_get(ch);assert(two->legendary_quarantined&&clone->legendary_quarantined&&active_majors(v)==0);
  assert(view_channel(v,two,0)->reason==LEG_BAD_IDENTITY);
  r=saved(two);assert(r.channels[0].rank==5&&r.channels[1].rank==5);
  char_from_room(&other);
  puts("legendary Phase 7: outfit/ownership/grip/cache/source lifetime/storage PASS");
}
