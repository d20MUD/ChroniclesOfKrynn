/* Persistent extra item passives: real SQL, command quote/confirm and adapters. */
static void item_passive_set(struct char_data *ch,struct obj_data *o,int id,int rank)
{
  struct legendary_item_record r=saved(o);
  r.channels[0].item_passive=id;r.channels[0].rank=rank;r.channels[0].foundation=0;
  if(rank<3)r.channels[0].signature=0;
  if(rank<2)r.channels[0].technique=0;
  assert(legendary_store_save_channel(&r,0,r.channels[0].version));
  legendary_object_unload(o);o->legendary_quarantined=false;assert(legendary_object_verify(o));
  legendary_loadout_invalidate(ch);affect_total(ch);
}
static void item_passive_cases(struct char_data *ch)
{
  static const int slots[]={4,3,2,4,8,3,5,3,9,3,9,0,8,4,8,3};
  static const int wear[]={WEAR_BODY,WEAR_HEAD,WEAR_LEGS,WEAR_BODY,WEAR_FEET,WEAR_HEAD,WEAR_ABOUT,WEAR_HEAD,WEAR_HANDS,WEAR_HEAD,WEAR_HANDS,WEAR_WIELD_1,WEAR_FEET,WEAR_BODY,WEAR_FEET,WEAR_HEAD};
  static const int steps[]={5,4,8,1,2,1,1,1,1,1,1,1,1,0,0,0};
  static const int skills[]={ABILITY_PERCEPTION,ABILITY_STEALTH,ABILITY_CONCENTRATION,ABILITY_HEAL,ABILITY_SPELLCRAFT,ABILITY_USE_MAGIC_DEVICE};
  struct obj_data *o,*other;struct legendary_item_record r;struct char_data enemy={0};
  int id,rank,i,current;long q;char packed_id[33];
  for(id=1;id<=16;id++){
    foundation_reset(ch);CLASS_LEVEL(ch,CLASS_WIZARD)=30;
    for(i=0;i<6;i++)GET_ABILITY(ch,skills[i])=5;
    o=foundation_item(ch,16,slots[id-1],wear[id-1]);
    for(rank=1;rank<=5;rank++){
      int expected=steps[id-1]?steps[id-1]*rank:(rank+1)/2;
      item_passive_set(ch,o,id,rank);q=queries();
      assert(legendary_item_passive_value(ch,id)==expected);assert(queries()-q==1);
      assert(saved(o).channels[0].item_passive==id);
      if(id==1)assert(GET_MAX_HIT(ch)==100+expected&&GET_HIT(ch)==70);
      if(id==2)assert(GET_MAX_PSP(ch)==100+expected&&GET_PSP(ch)==40);
      if(id==3)assert(GET_MAX_MOVE(ch)==100+expected&&GET_MOVE(ch)==30);
      if(id==4){assert(legendary_foundation_regen(ch)==expected);FIGHTING(ch)=&enemy;assert(!legendary_foundation_regen(ch));FIGHTING(ch)=NULL;}
      if(id==5){assert(legendary_foundation_walk(ch,100,false)==100-expected);assert(legendary_foundation_walk(ch,100,true)==100);}
      if(id>=6&&id<=11){assert(legendary_foundation_skill(ch,skills[id-6])==expected);GET_ABILITY(ch,skills[id-6])=0;assert(!legendary_foundation_skill(ch,skills[id-6]));GET_ABILITY(ch,skills[id-6])=5;}
      if(id==12)assert(legendary_foundation_maneuver(ch,COMBAT_MANEUVER_TYPE_DISARM)==expected);
      if(id==13)assert(legendary_foundation_maneuver(ch,COMBAT_MANEUVER_TYPE_FORCED_MOVEMENT)==expected);
      if(id>=14)assert(GET_SAVE(ch,id==14?SAVING_FORT:id==15?SAVING_REFL:SAVING_WILL)==expected);
    }
    current=GET_HIT(ch);test_remove(ch,wear[id-1]);assert(!legendary_item_passive_value(ch,id));assert(GET_HIT(ch)<=current);
  }
  /* No resource class gains a pool; a foundation remains independently active. */
  foundation_reset(ch);o=foundation_item(ch,16,3,WEAR_HEAD);item_passive_set(ch,o,2,5);assert(!legendary_item_passive_value(ch,2));assert(GET_MAX_PSP(ch)==100);
  foundation_set(ch,o,6,0,5);assert(legendary_foundation_value(ch,6,0)==1);
  /* Cap repeated outfit contributions and apply ordinary native save caps. */
  foundation_reset(ch);o=foundation_item(ch,16,4,WEAR_BODY);other=foundation_item(ch,16,6,WEAR_SHIELD);
  item_passive_set(ch,o,14,5);item_passive_set(ch,other,14,5);assert(legendary_item_passive_value(ch,14)==3&&GET_SAVE(ch,SAVING_FORT)==3);
  test_save_cap=1;affect_total(ch);assert(GET_SAVE(ch,SAVING_FORT)==1);
  /* Paid two-hand tracks inherit one selection, with independently scaled ranks. */
  foundation_reset(ch);o=legendary_test_item(ch,"passiveblade",0,1,true);test_wear(ch,o,WEAR_WIELD_2H);
  r=saved(o);r.channels[0].foundation=0;r.channels[0].item_passive=8;r.channels[0].rank=5;
  assert(legendary_store_save_channel(&r,0,r.channels[0].version));r=saved(o);
  r.channels[1].foundation=0;r.channels[1].item_passive=0;r.channels[1].family=LEG_FAMILY_GUARD;r.channels[1].rank=1;r.channels[1].technique=r.channels[1].signature=r.channels[1].modifier=r.channels[1].modifier_target=0;
  assert(legendary_store_save_channel(&r,1,r.channels[1].version));legendary_object_unload(o);o->legendary_quarantined=false;assert(legendary_object_verify(o));legendary_loadout_refresh(ch);
  assert(legendary_item_passive_value(ch,8)==6);
  r=saved(o);r.channels[1].rank=5;assert(legendary_store_save_channel(&r,1,r.channels[1].version));legendary_object_unload(o);o->legendary_quarantined=false;assert(legendary_object_verify(o));legendary_loadout_refresh(ch);assert(legendary_item_passive_value(ch,8)==10);
  other=foundation_item(ch,16,6,WEAR_SHIELD);assert(!legendary_item_passive_value(ch,8));test_remove(ch,WEAR_SHIELD);assert(legendary_item_passive_value(ch,8)==10);
  r=saved(o);r.channels[1].item_passive=12;assert(!legendary_record_valid(&r));r=saved(o);r.channels[0].item_passive=7;assert(!legendary_record_valid(&r));
  /* First selection is free; replacements use existing safe reshape payments. */
  foundation_reset(ch);o=foundation_item(ch,16,4,WEAR_BODY);
  GET_GOLD(ch)=200000;assert(estate_query("UPDATE legendary_characters SET fragments=100,version=version+1 WHERE player_id=1"));
  command(ch,"select foundation primary passive P01");assert(strstr(output,"infuse confirm "));confirm(ch);assert(saved(o).channels[0].item_passive==1&&GET_GOLD(ch)==200000);
  command(ch,"select foundation primary passive P03");assert(strstr(output,"infuse confirm "));assert(sscanf(strstr(output,"infuse confirm "),"infuse confirm %32s",packed_id)==1);confirm(ch);assert(saved(o).channels[0].item_passive==3&&GET_GOLD(ch)==180000);
  command(ch,"select foundation primary passive P03");assert(strstr(output,"Already selected"));
  command(ch,"select foundation supporting passive P01");assert(!strstr(output,"infuse confirm "));
  command(ch,"select foundation primary passive P06");assert(strstr(output,"unavailable"));
  foundation_reset(ch);puts("legendary item passives: all 16 I-V/native adapters, slot restrictions, no resource grants, no attack-time SQL, caps, two-hand parity and command payments PASS");
}
