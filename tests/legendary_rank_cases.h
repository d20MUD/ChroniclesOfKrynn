/* Phase 12 crosses the real smith command, transaction, equipment and effect APIs. */
static void rank_reload(struct char_data *ch,struct obj_data *o)
{legendary_object_unload(o);o->legendary_quarantined=false;assert(legendary_object_verify(o));legendary_loadout_refresh(ch);}
static void rank_reject(struct char_data *ch,const char *text,struct obj_data *o)
{
  long gold=GET_GOLD(ch),fragments=scalar("SELECT fragments FROM legendary_characters WHERE player_id=1");
  long operations=scalar("SELECT COUNT(*) FROM legendary_operations");
  struct legendary_item_record before=saved(o),after;
  legendary_infuse_cleanup(ch);command(ch,text);assert(!strstr(output,"infuse confirm "));after=saved(o);
  assert(GET_GOLD(ch)==gold&&scalar("SELECT fragments FROM legendary_characters WHERE player_id=1")==fragments);
  assert(scalar("SELECT COUNT(*) FROM legendary_operations")==operations&&after.version==before.version);
}
static void rank_cases(struct char_data *ch)
{
  struct obj_data *o;struct legendary_item_record r,previous;const struct legendary_loadout *view;
  long gold,fragments;int points,contribution,majors;int64_t expiry,start;char query[256];
  technique_clean(ch);foundation_reset(ch);legendary_effects_test_time+=1000;
  if(!mob_index)mob_index=calloc(1,sizeof(*mob_index));
  mob_index[0].vnum=600;
  assert(estate_query("UPDATE legendary_characters SET infusion_xp=400000000,fragments=400000,catalysts=2000,draconic=20000,martial=20000,arcane=20000,spiritual=20000,primal=20000,shadow=20000,version=version+1 WHERE player_id=1"));
  GET_GOLD(ch)=20000000;assert(save_char_checked(ch,0));CLASS_LEVEL(ch,CLASS_WIZARD)=1;
  o=legendary_test_item(ch,"rankblade",0,LEG_F01,true);r=saved(o);
  r.channels[0].rank=3;r.channels[0].path=LEG_PATH_DRAGONBANE;r.channels[0].technique=LEG_T17;r.channels[0].signature=LEG_S10;
  r.channels[1].rank=4;r.channels[1].path=LEG_PATH_SLAYER;r.channels[1].technique=LEG_T05;r.channels[1].signature=LEG_S10;
  assert(legendary_store_save_channel(&r,0,r.channels[0].version));
  assert(legendary_store_save_channel(&r,1,r.channels[1].version));rank_reload(ch,o);test_wear(ch,o,WEAR_WIELD_2H);
  rank_reject(ch,"modify rankblade primary technique Ready",o);
  perform(ch,"advance rankblade primary");assert(saved(o).channels[0].rank==4&&saved(o).channels[1].rank==4);
  gold=GET_GOLD(ch);perform(ch,"modify rankblade primary technique Ready");assert(GET_GOLD(ch)==gold);
  rank_reject(ch,"modify rankblade primary technique Ready",o); /* identical selection */
  rank_reject(ch,"modify rankblade primary signature 'Distant aid'",o);
  gold=GET_GOLD(ch);fragments=scalar("SELECT fragments FROM legendary_characters WHERE player_id=1");
  command(ch,"modify rankblade primary signature Enduring");assert(strstr(output,"Enduring on signature")&&strstr(output,"S10 rank Exalted values:")&&strstr(output,"window seconds=24"));confirm(ch);
  r=saved(o);assert(r.channels[0].modifier_target==2&&r.channels[0].modifier==LEG_MOD_ENDURING);
  assert(GET_GOLD(ch)==gold-20000&&scalar("SELECT fragments FROM legendary_characters WHERE player_id=1")==fragments-100);
  perform(ch,"attune rankblade primary technique");perform(ch,"attune rankblade primary signature");perform(ch,"attune rankblade supporting technique");
  view=legendary_loadout_get(ch);points=view->points;majors=view->major_count;
  contribution=legendary_foundation_value(ch,LEG_F01,0);assert(majors==3);
  start=legendary_effects_now();assert(legendary_activate(ch,LEG_S10,ch));
  /* Modifier applies to the selected signature only; base IV values remain base. */
  expiry=scalar("SELECT expires_utc FROM legendary_cooldowns WHERE player_id=1 AND cooldown_key=210");assert(expiry==start+300);
  assert(scalar("SELECT expires_utc FROM legendary_cooldowns WHERE player_id=1 AND cooldown_key=1")==start+6);
  snprintf(query,sizeof(query),"SELECT expires_utc FROM legendary_cooldowns WHERE player_id=1 AND cooldown_key=%d",1000+LEG_CD_PROTECTION);
  assert(scalar(query)==start+60);
  legendary_effects_test_time+=6;GET_WAIT_STATE(ch)=0;
  perform(ch,"modify rankblade primary signature Ready");
  assert(scalar("SELECT expires_utc FROM legendary_cooldowns WHERE player_id=1 AND cooldown_key=210")==expiry);
  assert(scalar(query)==start+60&&!legendary_activate(ch,LEG_S10,ch));
  /* V changes values in-place, preserving both channels, passive/major budgets and timestamps. */
  previous=saved(o);GET_HIT(ch)=60;gold=GET_GOLD(ch);perform(ch,"advance rankblade primary");r=saved(o);
  assert(GET_GOLD(ch)==gold-800000&&r.channels[0].rank==5&&r.channels[1].rank==4);
  assert(r.channels[0].foundation==previous.channels[0].foundation&&r.channels[0].technique==LEG_T17&&r.channels[0].signature==LEG_S10);
  assert(r.channels[0].modifier==LEG_MOD_READY&&r.channels[0].modifier_target==2&&r.channels[0].first_choices==previous.channels[0].first_choices);
  assert(r.channels[1].version==previous.channels[1].version&&GET_HIT(ch)==60);
  view=legendary_loadout_get(ch);assert(view->major_count==majors&&view->points==points&&legendary_foundation_value(ch,LEG_F01,0)==contribution);
  assert(scalar("SELECT expires_utc FROM legendary_cooldowns WHERE player_id=1 AND cooldown_key=210")==expiry);
  rank_reject(ch,"attune rankblade supporting signature",o);rank_reject(ch,"advance rankblade primary",o);
  legendary_effects_test_time=expiry;GET_WAIT_STATE(ch)=0;assert(legendary_activate(ch,LEG_S10,ch));
  assert(scalar("SELECT expires_utc FROM legendary_cooldowns WHERE player_id=1 AND cooldown_key=210")==expiry+255);
  legendary_effects_test_time+=60;GET_WAIT_STATE(ch)=0;assert(legendary_activate(ch,LEG_T17,ch));
  assert(scalar("SELECT expires_utc FROM legendary_cooldowns WHERE player_id=1 AND cooldown_key=117")==legendary_effects_now()+120);
  /* Path invalidation includes the old modifier; each replacement is free once. */
  GET_WAIT_STATE(ch)=0;perform(ch,"modify rankblade primary technique Enduring");
  gold=GET_GOLD(ch);perform(ch,"reshape rankblade primary path Wayfarer");r=saved(o);
  assert(GET_GOLD(ch)==gold-100000&&!r.channels[0].technique&&!r.channels[0].modifier&&!r.channels[0].modifier_target);
  assert(!(r.channels[0].first_choices&2)&&!(r.channels[0].first_choices&8)&&r.channels[0].signature==LEG_S10);
  gold=GET_GOLD(ch);fragments=scalar("SELECT fragments FROM legendary_characters WHERE player_id=1");
  perform(ch,"select rankblade primary technique T01");perform(ch,"modify rankblade primary technique Ready");
  assert(GET_GOLD(ch)==gold&&scalar("SELECT fragments FROM legendary_characters WHERE player_id=1")==fragments);
  rank_reject(ch,"select rankblade primary technique T17",o);
  rank_reject(ch,"modify rankblade primary signature 'Distant aid'",o);
  gold=GET_GOLD(ch);perform(ch,"modify rankblade primary signature Ready");assert(GET_GOLD(ch)==gold-20000); /* Moving same modifier is one fee. */
  /* Supporting-family invalidation is independent from the primary channel. */
  perform(ch,"modify rankblade supporting technique Enduring");previous=saved(o);
  gold=GET_GOLD(ch);fragments=scalar("SELECT fragments FROM legendary_characters WHERE player_id=1");
  perform(ch,"reshape rankblade supporting family focus");r=saved(o);
  assert(GET_GOLD(ch)==gold-100000&&scalar("SELECT fragments FROM legendary_characters WHERE player_id=1")==fragments-500);
  assert(!r.channels[1].technique&&!r.channels[1].signature&&!r.channels[1].modifier&&r.channels[1].first_choices==1);
  assert(r.channels[1].rank==4&&r.channels[0].version==previous.channels[0].version&&r.channels[0].rank==5);
  gold=GET_GOLD(ch);fragments=scalar("SELECT fragments FROM legendary_characters WHERE player_id=1");
  perform(ch,"select rankblade supporting technique T16");perform(ch,"select rankblade supporting signature S07");perform(ch,"modify rankblade supporting signature Ready");
  assert(GET_GOLD(ch)==gold&&scalar("SELECT fragments FROM legendary_characters WHERE player_id=1")==fragments);
  rank_reject(ch,"modify rankblade supporting signature 'Distant aid'",o);
  /* The remaining families invalidate only incompatible choices; compatible ones survive. */
  gold=GET_GOLD(ch);perform(ch,"reshape rankblade supporting family assault");r=saved(o);
  assert(GET_GOLD(ch)==gold-100000&&r.channels[1].family==LEG_FAMILY_ASSAULT);
  assert(!r.channels[1].technique&&!r.channels[1].signature&&!r.channels[1].modifier&&r.channels[1].first_choices==1);
  gold=GET_GOLD(ch);perform(ch,"select rankblade supporting technique T01");perform(ch,"select rankblade supporting signature S09");perform(ch,"modify rankblade supporting technique Ready");
  assert(GET_GOLD(ch)==gold);previous=saved(o);
  perform(ch,"reshape rankblade supporting family guard");r=saved(o);
  assert(r.channels[1].technique==LEG_T01&&r.channels[1].signature==LEG_S09&&r.channels[1].modifier==LEG_MOD_READY&&r.channels[1].modifier_target==1);
  assert(r.channels[1].first_choices==previous.channels[1].first_choices&&r.channels[1].rank==4&&r.channels[0].version==previous.channels[0].version);
  technique_clean(ch);legendary_infuse_cleanup(ch);
  puts("legendary Phase 12: rank IV/V, modifier target, persistent cooldowns, budgets, path/family free replacements and pre-payment rejection PASS");
}
