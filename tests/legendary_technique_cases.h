#include "legendary_techniques.h"
void test_real_load_affects(FILE *,struct char_data *);
bool test_real_save_affects(FILE *,struct char_data *);
static const int technique_slots[]={0,9,0,3,6,5,6,2,4,3,4,2,8,3,7,7,6,5};
static const int technique_wears[]={WEAR_WIELD_1,WEAR_HANDS,WEAR_WIELD_1,WEAR_HEAD,WEAR_SHIELD,WEAR_ABOUT,WEAR_SHIELD,WEAR_LEGS,WEAR_BODY,WEAR_HEAD,WEAR_BODY,WEAR_LEGS,WEAR_FEET,WEAR_HEAD,WEAR_HOLD_1,WEAR_HOLD_1,WEAR_SHIELD,WEAR_ABOUT};
static void technique_clean(struct char_data *ch)
{
  legendary_effects_cancel(ch);legendary_cast_clear(ch);
  while(ch->affected)affect_remove(ch,ch->affected);
  memset(AFF_FLAGS(ch),0,sizeof(ch->char_specials.saved.affected_by));
  ch->legendary_knockdown=false;GET_WAIT_STATE(ch)=0;standard_available=true;
}
static struct obj_data *technique_setup(struct char_data *ch,int id,int rank,int modifier)
{
  struct obj_data *o;struct legendary_item_record r;char refs[64];int slot=technique_slots[id-101];
  technique_clean(ch);foundation_reset(ch);CLASS_LEVEL(ch,CLASS_WIZARD)=1;
  legendary_effects_test_time+=1000;
  o=legendary_test_item(ch,"technique",slot,slot==9?4:legendary_slots[slot].foundations[0],false);
  r=saved(o);r.channels[0].rank=rank;r.channels[0].technique=id;r.channels[0].signature=0;
  r.channels[0].modifier=modifier;r.channels[0].modifier_target=modifier?1:0;
  assert(legendary_store_save_channel(&r,0,r.channels[0].version));
  legendary_object_unload(o);o->legendary_quarantined=false;assert(legendary_object_verify(o));
  test_wear(ch,o,technique_wears[id-101]);
  snprintf(refs,sizeof(refs),"%s/0/1",o->legendary_uuid);
  assert(estate_query("UPDATE legendary_characters SET major_selections='%s',version=version+1 WHERE player_id=1",refs));
  legendary_loadout_refresh(ch);return o;
}
static void technique_affect(struct char_data *ch,int spell,int duration,int bit)
{struct affected_type af={0};af.spell=spell;af.duration=duration;SET_BIT_AR(af.bitvector,bit);affect_to_char(ch,&af);}
static bool technique_has(struct char_data *ch,int spell)
{struct affected_type *p;for(p=ch->affected;p;p=p->next)if(p->spell==spell)return true;return false;}
static struct char_data *technique_target(struct char_data *ch,struct char_data *ally,struct char_data *enemy,int id,int modifier)
{
  if(id==LEG_T04||id==LEG_T13)return enemy;
  if(id==LEG_T07||id==LEG_T09||id==LEG_T14||modifier==LEG_MOD_DISTANT_AID)return ally;
  return ch;
}
static void technique_prerequisites(struct char_data *target,int id)
{
  GET_HIT(target)=70;
  if(id==LEG_T08){GET_POS(target)=POS_SITTING;target->legendary_knockdown=true;}
  if(id==LEG_T10){legendary_affect_round();technique_affect(target,SPELL_FEAR,5,AFF_FEAR);}
  if(id==LEG_T11)technique_affect(target,SKILL_BLEEDING_ATTACK,5,AFF_BLEED);
}
static void technique_cases(struct char_data *ch)
{
  struct char_data ally,enemy;struct group_data group={0};struct char_data *target,*bad;
  struct obj_data *o;struct obj_data ordinary_weapon={0};struct legendary_resolved_power r;struct legendary_activation a;
  struct legendary_effect_spec spec;struct legendary_attack attack;struct affected_type *af;
  int id,rank,mod,pass,expected,oldhp,value;long before;char sql[256];
  GET_OBJ_TYPE(&ordinary_weapon)=ITEM_WEAPON;
  legendary_effects_test_time=1900000000;legendary_techniques_init();
  assert(estate_query("DELETE FROM legendary_cooldowns"));assert(estate_query("DELETE FROM legendary_budgets"));assert(estate_query("DELETE FROM legendary_activations"));
  init_player(&ally,"techally",2);GET_LEVEL(&ally)=30;GET_POS(&ally)=POS_STANDING;
  init_player(&enemy,"techenemy",3);SET_BIT_AR(MOB_FLAGS(&enemy),MOB_ISNPC);GET_LEVEL(&enemy)=20;GET_POS(&enemy)=POS_STANDING;
  GROUP(ch)=GROUP((&ally))=&group;
  /* Every adapter, both base/V values, and every eligible live modifier. */
  for(id=101;id<=118;id++)for(pass=0;pass<9;pass++){
    rank=pass==0?2:pass<5?4:5;mod=pass==0?0:(pass-1)%4;
    if(!legendary_resolve_power(id,rank,mod,&r))continue;
    o=technique_setup(ch,id,rank,mod);technique_clean(&ally);GET_POS(&ally)=POS_STANDING;GET_POS(ch)=POS_STANDING;GROUP(ch)=GROUP((&ally))=&group;
    target=technique_target(ch,&ally,&enemy,id,mod);technique_prerequisites(target,id);
    bad=(id==LEG_T04||id==LEG_T13)?&ally:target==ch?&ally:&enemy;
    before=scalar("SELECT COUNT(*) FROM legendary_activations WHERE player_id=1");
    assert(!legendary_activate(ch,id,bad));assert(scalar("SELECT COUNT(*) FROM legendary_activations WHERE player_id=1")==before);
    assert(legendary_activate(ch,id,target));assert(action_duration==6*PASSES_PER_SEC&&GET_WAIT_STATE(ch)==6*PASSES_PER_SEC);
    snprintf(sql,sizeof(sql),"SELECT expires_utc FROM legendary_cooldowns WHERE player_id=1 AND cooldown_key=%d",id);
    assert(scalar(sql)==legendary_effects_test_time+r.cooldown);
    GET_WAIT_STATE(ch)=0;assert(!legendary_activate(ch,id,target));
    switch(id){
      case LEG_T01:assert(legendary_effect_peek(ch,id,&a,&spec));assert(a.resolved.values.attack==r.values.attack&&a.resolved.values.damage==r.values.damage);break;
      case LEG_T02:assert(legendary_technique_maneuver(target,COMBAT_MANEUVER_TYPE_DISARM,false)==r.values.check_bonus);assert(!legendary_technique_maneuver(target,COMBAT_MANEUVER_TYPE_GRAPPLE,false));break;
      case LEG_T03:assert(legendary_effect_peek(ch,id,&a,&spec));assert(a.resolved.values.duration==12&&spec.duration==r.values.window);break;
      case LEG_T04:assert(strstr(output,"resistance")&&strstr(output,"Legendary-boss hard-control"));if(rank==5)assert(strstr(output,"Physical reduction"));break;
      case LEG_T05:case LEG_T07:case LEG_T17:
        assert(legendary_effects_damage(&enemy,target,1000,DAM_FIRE,false,0)==1000);
        oldhp=GET_HIT(ch);expected=MIN(r.values.pool,id==LEG_T07?oldhp-1:r.values.pool);
        assert(legendary_effects_damage(&enemy,target,1000,id==LEG_T17?DAM_FIRE:DAM_BLUDGEON,id==LEG_T17,0)==1000-expected);
        assert(legendary_effects_damage(&enemy,target,1000,DAM_BLUDGEON,true,0)==1000);
        if(id==LEG_T07)assert(GET_HIT(ch)==oldhp-expected);
        break;
      case LEG_T06:assert(legendary_technique_maneuver(target,COMBAT_MANEUVER_TYPE_FORCED_MOVEMENT,true)==r.values.check_bonus);assert(!legendary_technique_maneuver(target,COMBAT_MANEUVER_TYPE_FORCED_MOVEMENT,false));break;
      case LEG_T08:assert(GET_POS(target)==POS_STANDING&&!target->legendary_knockdown);assert(legendary_technique_maneuver(target,COMBAT_MANEUVER_TYPE_KNOCKDOWN,true)==r.values.check_bonus);break;
      case LEG_T09:assert(GET_HIT(target)==MIN(100,70+r.values.heal));break;
      case LEG_T10:assert(target->affected->legendary_expiry==legendary_effects_test_time+36-r.values.fear_seconds);break;
      case LEG_T11:assert(!technique_has(target,SKILL_BLEEDING_ATTACK));assert(GET_HIT(target)==MIN(100,70+r.values.heal));break;
      case LEG_T12:assert(legendary_technique_walk(ch,101,false)==80&&legendary_technique_walk(ch,1,false)==1);assert(legendary_technique_walk(ch,101,true)==101);break;
      case LEG_T13:assert(legendary_effect_from(ch,id,&a)&&a.target==&enemy);break;
      case LEG_T14:assert(legendary_technique_skill(target,ABILITY_PERCEPTION,true)==r.values.check_bonus);assert(!legendary_technique_skill(target,ABILITY_PERCEPTION,true));break;
      case LEG_T15:case LEG_T16:assert(legendary_effect_peek(ch,id,&a,&spec));assert(a.resolved.values.resource_min==20);break;
      case LEG_T18:assert(legendary_technique_skill(target,ABILITY_STEALTH,false)==r.values.check_bonus);assert(!AFF_FLAGGED(target,AFF_INVISIBLE));break;
    }
    if(legendary_effect_peek(target,id,&a,&spec)){
      legendary_effects_test_time+=spec.duration;assert(!legendary_effect_peek(target,id,&a,NULL));
    }
    /* Re-use after cooldown, then revoke by losing the actual equipped source. */
    legendary_effects_test_time+=1000;GET_WAIT_STATE(ch)=0;technique_clean(target);GET_POS(target)=POS_STANDING;
    technique_prerequisites(target,id);assert(legendary_activate(ch,id,target));
    test_remove(ch,technique_wears[id-101]);
    assert(!legendary_effect_peek(target,id,&a,NULL));assert(!legendary_effect_from(ch,id,&a));
    assert(saved(o).channels[0].technique==id);
  }
  /* T01 consumes an actual attempt, including a miss; damage is post-critical,
   * non-overkill, capped, immune-safe and source-invalidated. */
  o=technique_setup(ch,LEG_T01,5,0);assert(legendary_activate(ch,LEG_T01,ch));
  assert(!legendary_technique_attack(ch,&enemy,NULL,&attack)&&legendary_effect_value(ch,LEG_T01));
  assert(legendary_technique_attack(ch,&enemy,o,&attack)==3);assert(!legendary_effect_value(ch,LEG_T01));
  assert(!legendary_technique_attack(ch,&enemy,o,&attack)); /* previous miss did not preserve it */
  legendary_effects_test_time+=1000;GET_WAIT_STATE(ch)=0;assert(legendary_activate(ch,LEG_T01,ch));
  assert(legendary_technique_attack(ch,&enemy,o,&attack)==3);GET_HIT(&enemy)=1000;
  assert(legendary_technique_damage(ch,&enemy,200,DAM_BLUDGEON,&attack)==15);
  assert(!legendary_technique_damage(ch,&enemy,200,DAM_BLUDGEON,&attack));
  legendary_effects_test_time+=1000;GET_WAIT_STATE(ch)=0;assert(legendary_activate(ch,LEG_T01,ch));legendary_technique_attack(ch,&enemy,o,&attack);
  test_remove(ch,WEAR_WIELD_1);assert(!legendary_technique_damage(ch,&enemy,20,DAM_BLUDGEON,&attack));
  o=technique_setup(ch,LEG_T01,5,0);assert(legendary_activate(ch,LEG_T01,ch));legendary_technique_attack(ch,&enemy,o,&attack);
  {struct legendary_item_record changed=saved(o);changed.channels[0].rank=4;
   assert(legendary_store_save_channel(&changed,0,changed.channels[0].version));
   legendary_object_unload(o);o->legendary_quarantined=false;assert(legendary_object_verify(o));legendary_loadout_invalidate(ch);
   assert(!legendary_technique_damage(ch,&enemy,20,DAM_BLUDGEON,&attack));}
  /* Save-time stripping preserves leases; an actual removal still cancels. */
  o=technique_setup(ch,LEG_T01,5,0);assert(legendary_activate(ch,LEG_T01,ch));
  ch->legendary_serializing=true;test_remove(ch,WEAR_WIELD_1);test_wear(ch,o,WEAR_WIELD_1);
  ch->legendary_serializing=false;legendary_loadout_invalidate(ch);
  assert(legendary_effect_value(ch,LEG_T01));
  test_remove(ch,WEAR_WIELD_1);assert(!legendary_effect_value(ch,LEG_T01));
  /* T03 marks only on a hit; a different group member's next attempt consumes. */
  o=technique_setup(ch,LEG_T03,5,LEG_MOD_ENDURING);assert(legendary_activate(ch,LEG_T03,ch));
  legendary_technique_attack(ch,&enemy,o,&attack);assert(!legendary_technique_damage(ch,&enemy,0,DAM_BLUDGEON,&attack));assert(legendary_effect_value(ch,LEG_T03));
  legendary_technique_attack(ch,&enemy,o,&attack);legendary_technique_damage(ch,&enemy,20,DAM_BLUDGEON,&attack);
  assert(!legendary_effect_value(ch,LEG_T03)&&legendary_effect_value(&enemy,303)==3);
  assert(!legendary_technique_attack(ch,&enemy,o,&attack));assert(legendary_effect_value(&enemy,303));
  assert(legendary_technique_attack(&ally,&enemy,o,&attack)==3);assert(!legendary_effect_value(&enemy,303));
  /* T08 cannot cure voluntary sitting, immobilization, or either actor's recovery. */
  technique_setup(ch,LEG_T08,5,0);GET_POS(ch)=POS_SITTING;assert(!legendary_activate(ch,LEG_T08,ch));ch->legendary_knockdown=true;
  standard_available=false;assert(!legendary_activate(ch,LEG_T08,ch));standard_available=true;
  SET_BIT_AR(AFF_FLAGS(ch),AFF_PINNED);assert(!legendary_activate(ch,LEG_T08,ch));REMOVE_BIT_AR(AFF_FLAGS(ch),AFF_PINNED);
  GET_WAIT_STATE(ch)=1;assert(!legendary_activate(ch,LEG_T08,ch));GET_WAIT_STATE(ch)=0;assert(legendary_activate(ch,LEG_T08,ch));
  /* Exact fear reduction/removal, oldest native record, and protected sources. */
  technique_setup(ch,LEG_T10,2,0);legendary_affect_round();technique_affect(ch,SPELL_FEAR,4,AFF_FEAR);af=ch->affected;
  technique_affect(ch,SPELL_DRAGONFEAR,6,AFF_FEAR);assert(legendary_activate(ch,LEG_T10,ch));assert(af->legendary_expiry==legendary_effects_test_time+18);
  legendary_effects_test_time+=17;legendary_condition_expire(ch);assert(technique_has(ch,SPELL_FEAR));legendary_effects_test_time++;legendary_condition_expire(ch);assert(!technique_has(ch,SPELL_FEAR)&&technique_has(ch,SPELL_DRAGONFEAR));
  technique_setup(ch,LEG_T10,5,0);legendary_affect_round();technique_affect(ch,SPELL_FEAR,2,AFF_FEAR);assert(legendary_activate(ch,LEG_T10,ch)&&!ch->affected);
  technique_setup(ch,LEG_T10,5,0);technique_affect(ch,SPELL_FEAR,-1,AFF_FEAR);assert(!legendary_activate(ch,LEG_T10,ch));ch->affected->duration=5;ch->affected->legendary_flags=LEG_CONDITION_LOCKED;assert(!legendary_activate(ch,LEG_T10,ch));
  ch->affected->legendary_flags=0;legendary_script_depth=1;legendary_condition_origin(ch->affected,ch);legendary_script_depth=0;assert(ch->affected->legendary_flags&LEG_CONDITION_LOCKED);
  technique_setup(ch,LEG_T10,2,0);legendary_affect_schedule(0);technique_affect(ch,SPELL_FEAR,2,AFF_FEAR);
  assert(legendary_activate(ch,LEG_T10,ch)&&!ch->affected); /* pending native tick: exactly 12s */
  /* T11 removes one oldest supported bleed, preserving profane/psionic effects. */
  technique_setup(ch,LEG_T11,5,0);technique_affect(ch,SKILL_BLEEDING_ATTACK,5,AFF_BLEED);
  technique_affect(ch,ABILITY_BLEEDING_CRITICAL,5,AFF_BLEED);technique_affect(ch,AFFECT_BLACKGUARD_BLEEDING,5,AFF_BLEED);
  assert(legendary_activate(ch,LEG_T11,ch));assert(!technique_has(ch,SKILL_BLEEDING_ATTACK)&&technique_has(ch,ABILITY_BLEEDING_CRITICAL)&&technique_has(ch,AFFECT_BLACKGUARD_BLEEDING));
  /* F15 + T12 choose the strongest final walking reduction, not multiplication. */
  technique_setup(ch,LEG_T12,5,0);assert(legendary_activate(ch,LEG_T12,ch));assert(legendary_foundation_walk(ch,100,false)==80);
  /* T13 adjacent/open/visible only; no movement cost or hidden-exit disclosure. */
  technique_setup(ch,LEG_T13,5,0);GET_HIT(&enemy)=100;assert(legendary_activate(ch,LEG_T13,&enemy));
  char_from_room(&enemy);char_to_room(&enemy,2);world[1].dir_option[NORTH]=calloc(1,sizeof(struct room_direction_data));world[1].dir_option[NORTH]->to_room=2;
  output[0]=0;legendary_track(ch);assert(strstr(output,"north"));value=GET_MOVE(ch);
  SET_BIT(world[1].dir_option[NORTH]->exit_info,EX_CLOSED);output[0]=0;legendary_track(ch);assert(strstr(output,"obscured")&&GET_MOVE(ch)==value);
  REMOVE_BIT(world[1].dir_option[NORTH]->exit_info,EX_CLOSED);SET_BIT(world[1].dir_option[NORTH]->exit_info,EX_HIDDEN);assert(!legendary_exit_traversable(ch,NORTH));
  REMOVE_BIT(world[1].dir_option[NORTH]->exit_info,EX_HIDDEN);SET_BIT_AR(AFF_FLAGS(&enemy),AFF_INVISIBLE);output[0]=0;legendary_track(ch);assert(strstr(output,"obscured"));REMOVE_BIT_AR(AFF_FLAGS(&enemy),AFF_INVISIBLE);
  legendary_effects_depart(ch);assert(!legendary_effect_from(ch,LEG_T13,&a));char_from_room(&enemy);char_to_room(&enemy,1);
  free(world[1].dir_option[NORTH]);world[1].dir_option[NORTH]=NULL;
  /* T14 ignores crafting and display queries, then chooses attack OR real skill. */
  o=technique_setup(ch,LEG_T14,5,0);assert(legendary_activate(ch,LEG_T14,&ally));
  assert(!legendary_technique_skill(&ally,99,true)&&legendary_effect_value(&ally,LEG_T14));assert(!legendary_technique_skill(&ally,ABILITY_PERCEPTION,false));
  assert(legendary_technique_attack(&ally,&enemy,&ordinary_weapon,&attack)==3);assert(!legendary_technique_skill(&ally,ABILITY_PERCEPTION,true));
  /* Real paid-cast receipts, no item/Vancian/cancellation/refund duplication. */
  technique_setup(ch,LEG_T15,5,0);GET_PSP(ch)=GET_MAX_PSP(ch)=100;assert(legendary_activate(ch,LEG_T15,ch));
  assert(legendary_cast_queue(ch,PSIONIC_ENERGY_RAY,100)&&legendary_cast_commit(ch,PSIONIC_ENERGY_RAY));assert(GET_PSP(ch)==0);
  legendary_cast_failed(ch,PSIONIC_ENERGY_RAY);assert(GET_PSP(ch)==15&&!legendary_effect_value(ch,LEG_T15));legendary_cast_failed(ch,PSIONIC_ENERGY_RAY);assert(GET_PSP(ch)==15);
  technique_setup(ch,LEG_T15,5,0);GET_PSP(ch)=100;assert(legendary_activate(ch,LEG_T15,ch));assert(legendary_cast_queue(ch,PSIONIC_ENERGY_RAY,50));legendary_cast_clear(ch);assert(GET_PSP(ch)==100);
  assert(legendary_cast_queue(ch,PSIONIC_ENERGY_RAY,50)&&legendary_cast_commit(ch,PSIONIC_ENERGY_RAY));legendary_cast_effect(ch,PSIONIC_ENERGY_RAY,CAST_SPELL);legendary_cast_failed(ch,PSIONIC_ENERGY_RAY);assert(GET_PSP(ch)==50&&legendary_effect_value(ch,LEG_T15));
  legendary_cast_clear(ch);assert(legendary_cast_commit(ch,SPELL_FIREBALL));legendary_cast_failed(ch,SPELL_FIREBALL);assert(GET_PSP(ch)==50); /* Vancian has no point receipt */
  technique_setup(ch,LEG_T16,2,0);GET_PSP(ch)=15;assert(legendary_activate(ch,LEG_T16,ch));
  assert(legendary_technique_affordable(ch,20));
  assert(legendary_cast_queue(ch,PSIONIC_ENERGY_RAY,20)&&legendary_cast_commit(ch,PSIONIC_ENERGY_RAY));assert(GET_PSP(ch)==0);assert(legendary_cast_commit(ch,PSIONIC_ENERGY_RAY)&&GET_PSP(ch)==0);
  assert(!legendary_effect_value(ch,LEG_T16));legendary_cast_clear(ch);
  technique_setup(ch,LEG_T16,5,0);GET_PSP(ch)=100;assert(legendary_activate(ch,LEG_T16,ch));assert(legendary_technique_pay(ch,PSIONIC_ENERGY_RAY,19)==19&&legendary_effect_value(ch,LEG_T16));
  assert(legendary_technique_pay(ch,PSIONIC_ENERGY_RAY,20)==12&&!legendary_effect_value(ch,LEG_T16));
  /* Production affect writer/reader preserve exact expiry, provenance and age. */
  {FILE *file=tmpfile();char header[32];int64_t expiry=(int64_t)time(NULL)+120;assert(file);
   technique_clean(ch);technique_affect(ch,SPELL_FEAR,5,AFF_FEAR);
   ch->affected->legendary_expiry=expiry;ch->affected->legendary_flags=LEG_CONDITION_LOCKED;
   technique_affect(ch,SPELL_DRAGONFEAR,4,AFF_FEAR);
   assert(test_real_save_affects(file,ch));rewind(file);assert(fgets(header,sizeof(header),file)&&strstr(header,"Affs:"));
   technique_clean(ch);test_real_load_affects(file,ch);assert(ch->affected->spell==SPELL_DRAGONFEAR&&ch->affected->next->spell==SPELL_FEAR);
   assert(ch->affected->next->legendary_expiry==expiry&&ch->affected->next->legendary_flags==LEG_CONDITION_LOCKED);fclose(file);
   file=tmpfile();assert(file);fprintf(file,"%d 2 0 0 0 0 0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0 0 0 0 0 0 0\n",SPELL_FEAR);rewind(file);
   technique_clean(ch);test_real_load_affects(file,ch);assert(ch->affected&&!ch->affected->legendary_flags&&!ch->affected->legendary_expiry);fclose(file);
   file=tmpfile();assert(file);ch->affected->legendary_expiry=(int64_t)time(NULL)-1;
   assert(test_real_save_affects(file,ch));rewind(file);assert(fgets(header,sizeof(header),file));technique_clean(ch);test_real_load_affects(file,ch);assert(!ch->affected);fclose(file);
  }
  technique_clean(ch);technique_clean(&ally);GROUP(ch)=GROUP((&ally))=NULL;char_from_room(&ally);char_from_room(&enemy);
  puts("legendary Phase 10: T01-T18 base/V, all allowed modifiers, targets, recovery, source expiry, real-event adapters PASS");
}
