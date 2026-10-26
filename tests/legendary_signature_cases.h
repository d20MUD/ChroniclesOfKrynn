#include "lists.h"
static const int signature_slots[]={6,6,2,3,4,2,7,7,0,6,0,0,0};
static const int signature_wears[]={WEAR_SHIELD,WEAR_SHIELD,WEAR_LEGS,WEAR_HEAD,WEAR_BODY,WEAR_LEGS,WEAR_HOLD_1,WEAR_HOLD_1,WEAR_WIELD_1,WEAR_SHIELD,WEAR_WIELD_1,WEAR_WIELD_1,WEAR_WIELD_1};
static struct obj_data *signature_setup(struct char_data *ch,int id,int rank,int modifier)
{
  int slot=signature_slots[id-201];char refs[64];struct legendary_item_record r;struct obj_data *o;
  technique_clean(ch);foundation_reset(ch);CLASS_LEVEL(ch,CLASS_WIZARD)=1;legendary_effects_test_time+=1000;
  o=legendary_test_item(ch,"signature",slot,legendary_slots[slot].foundations[0],false);
  r=saved(o);r.channels[0].rank=rank;r.channels[0].signature=id;r.channels[0].technique=0;
  r.channels[0].modifier=modifier;r.channels[0].modifier_target=modifier?2:0;
  if(id==LEG_S11||id==LEG_S12)r.channels[0].path=LEG_PATH_DRAGONBANE;
  assert(legendary_store_save_channel(&r,0,r.channels[0].version));
  legendary_object_unload(o);o->legendary_quarantined=false;assert(legendary_object_verify(o));
  test_wear(ch,o,signature_wears[id-201]);
  snprintf(refs,sizeof(refs),"%s/0/2",o->legendary_uuid);
  assert(estate_query("UPDATE legendary_characters SET major_selections='%s',version=version+1 WHERE player_id=1",refs));
  legendary_loadout_refresh(ch);return o;
}
static void signature_cases(struct char_data *ch)
{
  struct char_data ally,enemy,mount,others[3];struct group_data group={0};
  struct list_data members={0};struct item_data entries[4]={{0}};
  struct obj_data *o;struct legendary_resolved_power r;struct legendary_activation a;
  struct legendary_effect_spec s;struct legendary_attack attack;struct char_data *target;
  int id,pass,rank,mod,i,hp,before;char sql[256];
  legendary_signatures_init();init_player(&ally,"sigally",2);init_player(&enemy,"sigenemy",3);
  init_player(&mount,"sigmount",7);SET_BIT_AR(MOB_FLAGS(&mount),MOB_ISNPC);GET_REAL_RACE(&mount)=RACE_TYPE_DRAGON;
  SET_BIT_AR(MOB_FLAGS(&enemy),MOB_ISNPC);GET_REAL_RACE(&enemy)=RACE_TYPE_DRAGON;GET_LEVEL(&enemy)=20;
  mob_index=calloc(1,sizeof(*mob_index));mob_index[0].vnum=777;GET_MOB_RNUM(&enemy)=0;
  assert(estate_query("DELETE FROM legendary_dragonlances WHERE prototype_vnum=500"));
  assert(estate_query("INSERT INTO legendary_dragonlances(prototype_vnum,enabled) VALUES(500,1)"));
  GROUP(ch)=GROUP((&ally))=&group;group.members=&members;members.pFirstItem=&entries[0];members.pLastItem=&entries[3];members.iSize=4;
  entries[0].pContent=&ally;
  for(i=0;i<3;i++){init_player(&others[i],"sigother",4+i);GROUP((&others[i]))=&group;GET_POS(&others[i])=POS_STANDING;GET_HIT(&others[i])=100;entries[i+1].pContent=&others[i];entries[i].pNextItem=&entries[i+1];}
  for(id=201;id<=213;id++)for(pass=0;pass<9;pass++){
    rank=pass==0?3:pass<5?4:5;mod=pass==0?0:(pass-1)%4;
    if(!legendary_resolve_power(id,rank,mod,&r))continue;
    o=signature_setup(ch,id,rank,mod);technique_clean(&ally);technique_clean(&mount);technique_clean(&enemy);
    GROUP(ch)=GROUP((&ally))=&group;GET_HIT(&ally)=20;GET_HIT(&enemy)=1000;GET_HIT(&mount)=100;
    GET_POS(&ally)=GET_POS(&enemy)=GET_POS(&mount)=POS_STANDING;
    mount.master=ch;RIDING(ch)=&mount;RIDDEN_BY(&mount)=ch;
    target=id==LEG_S09?&enemy:id==LEG_S08||id==LEG_S13||mod==LEG_MOD_DISTANT_AID?&ally:ch;
    before=scalar("SELECT COUNT(*) FROM legendary_activations WHERE player_id=1");
    assert(!legendary_activate(ch,id,id==LEG_S09?&ally:&enemy));
    assert(scalar("SELECT COUNT(*) FROM legendary_activations WHERE player_id=1")==before);
    assert(legendary_activate(ch,id,target));
    snprintf(sql,sizeof(sql),"SELECT expires_utc FROM legendary_cooldowns WHERE player_id=1 AND cooldown_key=%d",id);
    assert(scalar(sql)==legendary_effects_test_time+r.cooldown);
    GET_WAIT_STATE(ch)=0;assert(!legendary_activate(ch,id,target));
    switch(id){
      case LEG_S01:
        assert(legendary_effects_damage(&enemy,ch,100,DAM_FIRE,false,0)==100);
        for(i=0;i<r.values.pool/20;i++)assert(legendary_effects_damage(&enemy,ch,100,DAM_SLICE,false,0)==80);
        assert(legendary_effects_damage(&enemy,ch,100,DAM_SLICE,false,0)==100);break;
      case LEG_S02:
        assert(legendary_technique_maneuver(ch,COMBAT_MANEUVER_TYPE_KNOCKDOWN,true)==r.values.check_bonus);
        assert(legendary_effect_value(&ally,id)==r.values.check_bonus);
        assert(legendary_effect_value(&others[0],id)==r.values.check_bonus);
        assert(!legendary_effect_value(&others[1],id)&&!legendary_effect_value(&others[2],id));break;
      case LEG_S03:
        assert(legendary_technique_walk(ch,101,false)==80&&legendary_technique_walk(ch,101,true)==101);
        assert(legendary_technique_walk(&ally,101,false)==80);
        legendary_effects_depart(&ally);char_from_room(&ally);char_to_room(&ally,2);assert(legendary_technique_walk(&ally,101,false)==80);
        GROUP((&ally))=NULL;assert(legendary_technique_walk(&ally,101,false)==101);
        char_from_room(&ally);char_to_room(&ally,1);GROUP((&ally))=&group;break;
      case LEG_S04:
        assert(legendary_signature_save(ch,SAVING_WILL,SPELL_CHARM)==r.values.check_bonus);
        assert(legendary_signature_save(ch,SAVING_WILL,SPELL_CONFUSION)==r.values.check_bonus);
        assert(!legendary_signature_save(ch,SAVING_FORT,SPELL_FEAR)&&!legendary_signature_save(ch,SAVING_WILL,SPELL_FIREBALL));break;
      case LEG_S05:
        assert(GET_HIT(ch)==70+r.values.heal);
        GET_HIT(ch)=10;
        for(i=0;i<3;i++){legendary_effects_test_time+=6;legendary_effects_pulse();assert(GET_HIT(ch)==20+i*10);}
        assert(GET_HIT(ch)==40);break;
      case LEG_S06:
        for(i=0;i<r.values.charges;i++){assert(legendary_signature_dodge(target,false)==2);assert(legendary_signature_dodge(target,true)==2);}
        assert(!legendary_signature_dodge(target,false));break;
      case LEG_S07:
        GET_PSP(ch)=100;
        assert(legendary_technique_pay(ch,SPELL_FIREBALL,19)==19);
        for(i=0;i<2;i++)assert(legendary_technique_pay(ch,SPELL_FIREBALL,20)==20-r.values.resource_discount);
        assert(legendary_technique_pay(ch,SPELL_FIREBALL,20)==20);break;
      case LEG_S08:
        assert(legendary_technique_skill(&ally,ABILITY_CONCENTRATION,false)==r.values.check_bonus);
        assert(legendary_temporary_remaining(&ally)==r.values.temporary_hp&&GET_HIT(&ally)==20);
        assert(legendary_temporary_absorb(&ally,7)==0&&legendary_temporary_remaining(&ally)==r.values.temporary_hp-7);
        assert(legendary_temporary_absorb(&ally,100)==100-r.values.temporary_hp+7);break;
      case LEG_S09:
        assert(legendary_technique_attack(ch,&enemy,o,&attack)==r.values.attack);
        assert(legendary_technique_damage(ch,&enemy,10,DAM_SLICE,&attack)==r.values.damage);
        assert(!legendary_technique_attack(ch,&enemy,o,&attack));break;
      case LEG_S10:
        assert(legendary_effects_damage(&enemy,ch,1000,DAM_FIRE,false,0)==1000);
        assert(legendary_effects_damage(&enemy,ch,1000,DAM_FIRE,true,0)==1000-r.values.pool);
        assert(legendary_effects_damage(&enemy,ch,1000,DAM_COLD,true,0)==1000);break;
      case LEG_S11:
        signature_roll=1;signature_save=0;
        legendary_technique_attack(ch,&enemy,o,&attack);assert(attack.grounding);
        assert(!legendary_technique_damage(ch,&enemy,10,DAM_SLICE,&attack)&&legendary_grounded(&enemy));
        legendary_script_depth++;assert(!legendary_grounded(&enemy));legendary_script_depth--;
        legendary_effects_test_time+=6;assert(!legendary_grounded(&enemy));break;
      case LEG_S12:
        assert(legendary_effects_damage(&enemy,ch,1000,DAM_SLICE,false,0)==1000-r.values.pool);
        assert(legendary_effects_damage(&enemy,&mount,1000,DAM_SLICE,false,0)==1000-r.values.pool);
        assert(legendary_effects_damage(&enemy,ch,1000,DAM_SLICE,false,0)==1000);break;
      case LEG_S13:
        hp=GET_HIT(&ally);legendary_technique_attack(ch,&enemy,o,&attack);
        assert(!legendary_technique_damage(ch,&enemy,10,DAM_SLICE,&attack));assert(GET_HIT(&ally)==hp+r.values.heal);break;
    }
    if(legendary_effect_peek(target,id,&a,&s)){legendary_effects_test_time+=s.duration;assert(!legendary_effect_peek(target,id,&a,NULL));}
    test_remove(ch,signature_wears[id-201]);legendary_effects_pulse();
    assert(!legendary_effect_from(ch,id,&a));
    RIDING(ch)=NULL;RIDDEN_BY(&mount)=NULL;
  }
  /* A landed but fully absorbed hit still delivers support/control, never bonus damage. */
  o=signature_setup(ch,LEG_S13,5,0);GET_HIT(&ally)=20;
  assert(legendary_activate(ch,LEG_S13,&ally));legendary_technique_attack(ch,&enemy,o,&attack);
  assert(!legendary_technique_damage(ch,&enemy,0,DAM_SLICE,&attack)&&GET_HIT(&ally)==60);
  o=signature_setup(ch,LEG_S11,5,0);assert(legendary_activate(ch,LEG_S11,ch));signature_roll=1;
  legendary_technique_attack(ch,&enemy,o,&attack);
  assert(!legendary_technique_damage(ch,&enemy,0,DAM_SLICE,&attack)&&legendary_grounded(&enemy));
  o=signature_setup(ch,LEG_S09,5,0);assert(legendary_activate(ch,LEG_S09,&enemy));
  legendary_technique_attack(ch,&enemy,o,&attack);assert(!legendary_technique_damage(ch,&enemy,0,DAM_SLICE,&attack));
  /* True provenance: renaming or retaining a disabled registry entry grants nothing. */
  o=signature_setup(ch,LEG_S11,5,0);
  assert(estate_query("UPDATE legendary_dragonlances SET enabled=0 WHERE prototype_vnum=500"));
  before=scalar("SELECT COUNT(*) FROM legendary_activations WHERE player_id=1");
  assert(!legendary_activate(ch,LEG_S11,ch));
  assert(scalar("SELECT COUNT(*) FROM legendary_activations WHERE player_id=1")==before);
  assert(estate_query("UPDATE legendary_dragonlances SET enabled=1 WHERE prototype_vnum=500"));
  /* Only an actual hit by the source lance triggers the save. Misses preserve it. */
  o=signature_setup(ch,LEG_S11,5,LEG_MOD_ENDURING);GET_HIT(&enemy)=1000;
  assert(legendary_activate(ch,LEG_S11,ch));
  {struct obj_data other={0};GET_OBJ_TYPE(&other)=ITEM_WEAPON;
   legendary_technique_attack(ch,&enemy,&other,&attack);assert(!attack.grounding);}
  legendary_technique_attack(ch,&enemy,o,&attack);assert(legendary_effect_value(ch,LEG_S11));
  signature_roll=20;legendary_technique_damage(ch,&enemy,10,DAM_SLICE,&attack);
  assert(!legendary_grounded(&enemy)&&!legendary_effect_value(ch,LEG_S11));
  /* Fixed DC25 boundary and six-second control, unaffected by Enduring. */
  for(i=0;i<2;i++){
    o=signature_setup(ch,LEG_S11,5,LEG_MOD_ENDURING);assert(legendary_activate(ch,LEG_S11,ch));
    signature_roll=10;signature_save=14+i;
    legendary_technique_attack(ch,&enemy,o,&attack);legendary_technique_damage(ch,&enemy,10,DAM_SLICE,&attack);
    assert(legendary_grounded(&enemy)==(i==0));
    legendary_effects_test_time+=6;assert(!legendary_grounded(&enemy));
  }
  SET_BIT_AR(MOB_FLAGS(&enemy),MOB_LEGENDARY_BOSS);
  o=signature_setup(ch,LEG_S11,5,0);assert(legendary_activate(ch,LEG_S11,ch));
  legendary_technique_attack(ch,&enemy,o,&attack);assert(legendary_technique_damage(ch,&enemy,10,DAM_SLICE,&attack)==25);
  assert(!legendary_grounded(&enemy));
  o=signature_setup(ch,LEG_S11,5,0);assert(legendary_activate(ch,LEG_S11,ch));
  GET_REAL_RESISTANCES(&enemy,DAM_SLICE)=100;affect_total(&enemy);
  legendary_technique_attack(ch,&enemy,o,&attack);assert(!legendary_technique_damage(ch,&enemy,10,DAM_SLICE,&attack));
  GET_REAL_RESISTANCES(&enemy,DAM_SLICE)=0;affect_total(&enemy);
  REMOVE_BIT_AR(MOB_FLAGS(&enemy),MOB_LEGENDARY_BOSS);
  /* Mount ownership/identity is required and dismounting revokes both reserves. */
  o=signature_setup(ch,LEG_S12,5,0);RIDING(ch)=&mount;mount.master=NULL;
  assert(!legendary_activate(ch,LEG_S12,ch));mount.master=ch;
  GET_REAL_RACE(&mount)=RACE_TYPE_ANIMAL;assert(!legendary_activate(ch,LEG_S12,ch));
  GET_REAL_RACE(&mount)=RACE_TYPE_DRAGON;assert(legendary_activate(ch,LEG_S12,ch));RIDING(ch)=NULL;
  assert(legendary_effects_damage(&enemy,ch,100,DAM_SLICE,false,0)==100);
  assert(legendary_effects_damage(&enemy,&mount,100,DAM_SLICE,false,0)==100);
  /* Registry edits through the staff/store API invalidate already active leases. */
  o=signature_setup(ch,LEG_S12,5,0);RIDING(ch)=&mount;mount.master=ch;
  assert(legendary_activate(ch,LEG_S12,ch));
  {struct legendary_registry_record registry;
   assert(legendary_registry_load(500,true,&registry)==LEG_LOAD_OK);registry.enabled=false;
   assert(legendary_registry_save(&registry,true,registry.version));
   assert(legendary_effects_damage(&enemy,ch,100,DAM_SLICE,false,0)==100);
   assert(legendary_effects_damage(&enemy,&mount,100,DAM_SLICE,false,0)==100);
   assert(legendary_registry_load(500,true,&registry)==LEG_LOAD_OK);registry.enabled=true;
   assert(legendary_registry_save(&registry,true,registry.version));}
  RIDING(ch)=NULL;
  /* Hunter designation survives an attempt against someone else, but not a miss. */
  o=signature_setup(ch,LEG_S09,5,0);assert(legendary_activate(ch,LEG_S09,&enemy));mount.master=NULL;
  assert(!legendary_technique_attack(ch,&mount,o,&attack));
  assert(legendary_technique_attack(ch,&enemy,o,&attack)==3);
  assert(!legendary_technique_attack(ch,&enemy,o,&attack));
  /* All flat offense shares one allowance; source removal suppresses delivery. */
  o=signature_setup(ch,LEG_S09,5,0);assert(legendary_budget_debit(ch,LEG_BUDGET_OFFENSE,40)==40);
  assert(legendary_activate(ch,LEG_S09,&enemy));legendary_technique_attack(ch,&enemy,o,&attack);
  assert(legendary_technique_damage(ch,&enemy,10,DAM_SLICE,&attack)==20);
  o=signature_setup(ch,LEG_S09,5,0);assert(legendary_activate(ch,LEG_S09,&enemy));legendary_technique_attack(ch,&enemy,o,&attack);
  test_remove(ch,WEAR_WIELD_1);assert(!legendary_technique_damage(ch,&enemy,10,DAM_SLICE,&attack));
  /* Rally consumes on a miss; delivery cannot heal a departed, hidden or ungrouped ally. */
  for(i=0;i<3;i++){
    o=signature_setup(ch,LEG_S13,5,0);GET_HIT(&ally)=20;GROUP((&ally))=&group;
    assert(legendary_activate(ch,LEG_S13,&ally));legendary_technique_attack(ch,&enemy,o,&attack);
    if(i==0){legendary_technique_attack(ch,&enemy,o,&attack);assert(!attack.rallying);}
    if(i==1)GROUP((&ally))=NULL;
    if(i==2)SET_BIT_AR(AFF_FLAGS(&ally),AFF_HIDE);
    legendary_technique_damage(ch,&enemy,10,DAM_SLICE,&attack);assert(GET_HIT(&ally)==20);
    GROUP((&ally))=&group;REMOVE_BIT_AR(AFF_FLAGS(&ally),AFF_HIDE);
  }
  /* Delayed healing cancels on room departure, even if the recipient returns. */
  signature_setup(ch,LEG_S05,3,0);assert(legendary_activate(ch,LEG_S05,&ally));hp=GET_HIT(&ally);
  legendary_effects_depart(&ally);char_from_room(&ally);char_to_room(&ally,2);char_from_room(&ally);char_to_room(&ally,1);
  legendary_effects_test_time+=6;legendary_effects_pulse();assert(GET_HIT(&ally)==hp);
  /* Temporary HP spends the healing allowance once and never refills on removal. */
  signature_setup(ch,LEG_S08,5,0);GET_HIT(&ally)=20;assert(legendary_activate(ch,LEG_S08,&ally));
  assert(legendary_temporary_remaining(&ally)==30);
  assert(legendary_heal(ch,&ally,90)==70&&GET_HIT(&ally)==90);
  assert(legendary_temporary_absorb(&ally,5)==0);assert(legendary_temporary_remaining(&ally)==25);
  assert(legendary_effect_peek(&ally,LEG_S08,&a,NULL));
  assert(!legendary_temporary_hp(&a,30,18)&&legendary_temporary_remaining(&ally)==25);
  legendary_effects_cancel(ch);assert(!legendary_temporary_remaining(&ally)&&GET_HIT(&ally)==90);
  /* Protection clocks and recipient prevention limits are shared with techniques. */
  o=signature_setup(ch,LEG_S01,5,0);
  {struct legendary_item_record changed=saved(o);changed.channels[0].technique=LEG_T05;
   assert(legendary_store_save_channel(&changed,0,changed.channels[0].version));
   legendary_object_unload(o);o->legendary_quarantined=false;assert(legendary_object_verify(o));
   snprintf(sql,sizeof(sql),"UPDATE legendary_characters SET major_selections='%s/0/2 %s/0/1',version=version+1 WHERE player_id=1",o->legendary_uuid,o->legendary_uuid);
   assert(estate_query("%s",sql));legendary_loadout_refresh(ch);}
  assert(legendary_activate(ch,LEG_S01,ch));legendary_effects_test_time+=6;GET_WAIT_STATE(ch)=0;
  assert(!legendary_activate(ch,LEG_T05,ch));
  assert(legendary_budget_debit(ch,LEG_BUDGET_PREVENTION,70)==70);
  assert(legendary_effects_damage(&enemy,ch,1000,DAM_SLICE,false,0)==970);
  /* T16/S07 use the strongest discount, not their sum, and share 20/180s. */
  o=signature_setup(ch,LEG_S07,5,0);
  {struct legendary_item_record changed=saved(o);changed.channels[0].technique=LEG_T16;
   assert(legendary_store_save_channel(&changed,0,changed.channels[0].version));
   legendary_object_unload(o);o->legendary_quarantined=false;assert(legendary_object_verify(o));
   snprintf(sql,sizeof(sql),"UPDATE legendary_characters SET major_selections='%s/0/2 %s/0/1',version=version+1 WHERE player_id=1",o->legendary_uuid,o->legendary_uuid);
   assert(estate_query("%s",sql));legendary_loadout_refresh(ch);}
  assert(legendary_activate(ch,LEG_S07,ch));legendary_effects_test_time+=6;GET_WAIT_STATE(ch)=0;
  assert(legendary_activate(ch,LEG_T16,ch));GET_PSP(ch)=100;
  assert(legendary_technique_pay(ch,SPELL_FIREBALL,20)==12&&!legendary_effect_value(ch,LEG_T16));
  assert(legendary_technique_pay(ch,SPELL_FIREBALL,20)==12);
  assert(legendary_technique_pay(ch,SPELL_FIREBALL,20)==16&&!legendary_effect_value(ch,LEG_S07));
  assert(legendary_technique_pay(ch,SPELL_FIREBALL,20)==20);
  technique_clean(ch);technique_clean(&ally);technique_clean(&mount);technique_clean(&enemy);
  for(i=0;i<3;i++){legendary_effects_free(&others[i]);char_from_room(&others[i]);}
  GROUP(ch)=GROUP((&ally))=NULL;char_from_room(&ally);char_from_room(&enemy);char_from_room(&mount);
  legendary_effects_free(&ally);legendary_effects_free(&enemy);legendary_effects_free(&mount);free(mob_index);mob_index=NULL;
  puts("legendary Phase 11: S01-S13 base/V and permitted modifiers, target/charge/ward/healing/travel/lance adapters PASS");
}
