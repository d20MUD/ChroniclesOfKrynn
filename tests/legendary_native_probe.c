/* Native stationary-target gate probe. Includes ordinary misses, crits,
 * mitigation, weapon procs and death/replacement. Not the seven-role release suite. */
#define __wrap_main native_capture_unused
#include "legendary_native_capture.c"
#undef __wrap_main
#include "legendary_effects.h"
#include "mud_event.h"
#include "actions.h"
#include "estate.h"
#include "class.h"
#include "race.h"
#include "feats.h"
#include "legendary_techniques.h"
#include "actionqueues.h"
#include "treasure.h"
#include "vnums.h"
static struct char_data *probe_actor;
static long long dealt;
static int packets;
static time_t probe_time=1792922400;
time_t __wrap_time(time_t *out){if(out)*out=probe_time;return probe_time;}
void legendary_native_observe(struct char_data *a,struct char_data *v,int dam)
{
  if(a==probe_actor&&IS_NPC(v)&&GET_MOB_VNUM(v)==626){
    dealt+=MAX(0,MIN(dam,GET_HIT(v)));packets++;
  }
}
static int equip_metadata(struct char_data *ch)
{
  int positions[]={WEAR_WIELD_1,WEAR_WIELD_OFFHAND,WEAR_ARMS,WEAR_LEGS,WEAR_HEAD,WEAR_BODY,WEAR_ABOUT,WEAR_FEET,WEAR_HANDS};
  static const int passive[]={12,12,11,3,6,1,7,5,9};
  int i,count=0,foundation[]={1,2,2,3};char priorities[1024]={0};
  if(GET_EQ(ch,WEAR_WIELD_2H))positions[0]=WEAR_WIELD_2H;
  for(i=0;i<9;i++){
    struct obj_data *o=GET_EQ(ch,positions[i]);struct legendary_item_record r;
    if(!o||!add_foundation(ch,o,count<4?foundation[count]:16))continue;
    if(strlen(priorities)+36>=sizeof(priorities))return -1;
    if(*priorities)strcat(priorities," ");strcat(priorities,o->legendary_uuid);strcat(priorities,"/0");
    count++;
    if(legendary_store_load(o->legendary_uuid,&r)!=LEG_LOAD_OK)return -1;
    r.channels[0].item_passive=passive[i];
    if(!legendary_store_save_channel(&r,0,r.channels[0].version))return -1;
    if(i==0&&GET_EQ(ch,WEAR_WIELD_2H)==o&&legendary_equipment_eligible(ch,o,LEG_SLOT_WEAPON,1)){
      r.channels[1]=r.channels[0];r.channels[1].item_passive=0;r.channels[1].family=LEG_FAMILY_ASSAULT;r.channels[1].foundation=foundation[count<4?count:3];r.channels[1].version=0;
      if(!legendary_store_save_channel(&r,1,-1))return -1;
      strcat(priorities," ");strcat(priorities,o->legendary_uuid);strcat(priorities,"/1");count++;
    }
    legendary_object_unload(o);o->legendary_quarantined=false;
    if(!legendary_object_verify(o))return -1;
  }
  if(!estate_query("INSERT INTO legendary_characters(player_id,passive_priorities) VALUES(%lld,'%s') ON DUPLICATE KEY UPDATE passive_priorities=VALUES(passive_priorities),version=version+1",(long long)GET_IDNUM(ch),priorities))return -1;
  legendary_loadout_refresh(ch);affect_total(ch);
  if(legendary_foundation_value(ch,1,0)!=1||legendary_foundation_value(ch,2,0)!=2||legendary_foundation_value(ch,3,0)!=1)return -1;
  return count;
}
int __wrap_main(int argc,char **argv)
{
  FILE *manifest,*out;char label[64],name[64];struct char_data *ch,*vict;
  const char *mode=getenv("LEGENDARY_PROBE_MODE"),*role=getenv("LEGENDARY_PROBE_ROLE");bool ranged=role&&!strcmp(role,"ranged_attacker");int rnum,i,channels=0,phase,seconds;
  long target_id;int equipped=0,ab,damage,offhand;
  if(argc!=3||!mode)return 2;
  seconds=atoi(getenv("LEGENDARY_PROBE_SECONDS")?getenv("LEGENDARY_PROBE_SECONDS"):"600");
  manifest=fopen(argv[1],"r");out=fopen(argv[2],"w");if(!manifest||!out)return 3;
  if(fscanf(manifest,"%63s %63s",label,name)!=2)return 4;
  logfile=stderr;CONFIG_CONFFILE=strdup("etc/config");load_config();auto_pwipe=0;no_rent_check=1;
  circle_srandom(140026);event_init();init_lookup_table();boot_db();
  ch=new_char();if(load_char(name,ch)<0)return 5;
  char_to_room(ch,real_room(2200));Crash_load(ch);GET_POS(ch)=POS_STANDING;affect_total(ch);
  /* Register exactly as an entering PC would, without a network descriptor. */
  GET_QUEUE(ch)=create_action_queue();GET_ATTACK_QUEUE(ch)=create_attack_queue();init_condensed_combat_data(ch);
  GET_HIT(ch)=GET_MAX_HIT(ch);
  GET_ID(ch)=GET_IDNUM(ch);add_to_lookup_table(GET_ID(ch),ch);ch->next=character_list;character_list=ch;
  probe_actor=ch;legendary_loadout_refresh(ch);affect_total(ch);
  if(GET_LEVEL(ch)!=30)return 6;
  if(ranged){
    struct obj_data *bow=is_using_ranged_weapon(ch,true),*pouch=GET_EQ(ch,WEAR_AMMO_POUCH);
    if(getenv("LEGENDARY_PROBE_GEARED")&&atoi(getenv("LEGENDARY_PROBE_GEARED"))){
      static const int wanted[]={WEAPON_TYPE_SHORT_BOW,WEAPON_TYPE_LONG_BOW,WEAPON_TYPE_LIGHT_CROSSBOW};
      static const int hand[]={WEAR_WIELD_1,WEAR_WIELD_OFFHAND,WEAR_WIELD_2H,WEAR_SHIELD,WEAR_HOLD_1,WEAR_HOLD_2,WEAR_HOLD_2H};
      int kind,k;
      bow=NULL;
      for(kind=0;kind<3&&!bow;kind++)if(is_proficient_with_weapon(ch,wanted[kind])){
        struct obj_data *fresh=read_object(WEAPON_PROTO,VIRTUAL);
        if(!fresh)return 14;
        set_weapon_object(fresh,wanted[kind]);
        circle_srandom(150026);cp_modify_object_applies(ch,fresh,6,CP_TYPE_WEAPON,RARE_GRADE_NORMAL,1,30);
        for(k=0;k<7;k++)if(GET_EQ(ch,hand[k]))obj_to_char(unequip_char(ch,hand[k]),ch);
        perform_wear(ch,fresh,WEAR_WIELD_2H);bow=is_using_ranged_weapon(ch,true);
      }
    }
    if(!bow&&HAS_FEAT(ch,FEAT_POINT_BLANK_SHOT)){
      static const int wanted[]={WEAPON_TYPE_SHORT_BOW,WEAPON_TYPE_LONG_BOW,WEAPON_TYPE_LIGHT_CROSSBOW};
      int kind,k;struct obj_data *fresh=NULL;
      for(kind=0;kind<3&&!fresh;kind++)if(is_proficient_with_weapon(ch,wanted[kind]))
        for(k=0;k<=top_of_objt&&!fresh;k++){
          struct obj_data *o=&obj_proto[k];
          if((GET_OBJ_TYPE(o)==ITEM_WEAPON||GET_OBJ_TYPE(o)==ITEM_FIREWEAPON)&&GET_WEAPON_TYPE(o)==wanted[kind]&&GET_OBJ_LEVEL(o)>=20&&GET_OBJ_LEVEL(o)<=30&&
             !invalid_align(ch,o)&&!invalid_race(ch,o)&&!invalid_class(ch,o)&&is_class_req_object(ch,o,false)&&!is_class_anti_object(ch,o,false))fresh=read_object(k,REAL);
        }
      if(fresh){
        static const int hand[]={WEAR_WIELD_1,WEAR_WIELD_OFFHAND,WEAR_WIELD_2H,WEAR_SHIELD,WEAR_HOLD_1,WEAR_HOLD_2,WEAR_HOLD_2H};
        for(k=0;k<7;k++)if(GET_EQ(ch,hand[k]))obj_to_char(unequip_char(ch,hand[k]),ch);
        obj_to_char(fresh,ch);perform_wear(ch,fresh,WEAR_WIELD_2H);bow=is_using_ranged_weapon(ch,true);
      }
    }
    if(!bow||!HAS_FEAT(ch,FEAT_POINT_BLANK_SHOT)){fprintf(stderr,"PROBE: ranged prerequisites: bow=%d PBS=%d class=%d\n",bow!=NULL,HAS_FEAT(ch,FEAT_POINT_BLANK_SHOT),GET_CLASS(ch));return 11;}
    if(!pouch){
      for(i=0;i<=top_of_objt;i++)if(CAN_WEAR(&obj_proto[i],ITEM_WEAR_AMMO_POUCH)){
        pouch=read_object(i,REAL);obj_to_char(pouch,ch);perform_wear(ch,pouch,WEAR_AMMO_POUCH);
        if(GET_EQ(ch,WEAR_AMMO_POUCH)==pouch)break;
      }
      pouch=GET_EQ(ch,WEAR_AMMO_POUCH);if(!pouch)return 12;
    }
    while(pouch->contains){struct obj_data *old=pouch->contains;obj_from_obj(old);obj_to_char(old,ch);}
    /* Plain native ammo has zero weight, ordinary type and no magic bonuses.
     * Stock the same finite supply for both variants; supply cost is excluded. */
    for(i=0;i<1000;i++){
      struct obj_data *extra=read_object(AMMO_PROTO,VIRTUAL);
      if(!extra)return 12;
      {
        int ammo_type=AMMO_TYPE_ARROW;
        switch(GET_WEAPON_TYPE(bow)){
          case WEAPON_TYPE_HAND_CROSSBOW:case WEAPON_TYPE_HEAVY_REP_XBOW:case WEAPON_TYPE_LIGHT_REP_XBOW:case WEAPON_TYPE_HEAVY_CROSSBOW:case WEAPON_TYPE_LIGHT_CROSSBOW:ammo_type=AMMO_TYPE_BOLT;break;
          case WEAPON_TYPE_SLING:ammo_type=AMMO_TYPE_STONE;break;
          case WEAPON_TYPE_DART:ammo_type=AMMO_TYPE_DART;break;
        }
        set_ammo_object(extra,ammo_type);obj_to_obj(extra,pouch);
      }
    }
    if(!can_fire_ammo(ch,true))return 13;
    FIRING(ch)=true;
  }else{
    if((GET_CLASS(ch)!=CLASS_WARRIOR&&GET_CLASS(ch)!=CLASS_ASSASSIN)||!GET_EQ(ch,WEAR_WIELD_1)||!GET_EQ(ch,WEAR_WIELD_OFFHAND)||GET_EQ(ch,WEAR_SHIELD)||GET_EQ(ch,WEAR_WIELD_2H)||GET_EQ(ch,WEAR_HOLD_1)||GET_EQ(ch,WEAR_HOLD_2)||GET_EQ(ch,WEAR_HOLD_2H))return 6;
    for(i=0;i<2;i++){
      struct obj_data *w=GET_EQ(ch,i?WEAR_WIELD_OFFHAND:WEAR_WIELD_1);
      if(invalid_align(ch,w)||invalid_race(ch,w)||invalid_class(ch,w)||!is_class_req_object(ch,w,false)||is_class_anti_object(ch,w,false)||hands_needed_full(ch,w,true)!=1||!is_proficient_with_weapon(ch,GET_WEAPON_TYPE(w)))return 10;
    }
  }
  for(i=0;i<NUM_WEARS;i++)if(GET_EQ(ch,i))equipped++;
  if(!strcmp(mode,"legendary")){channels=equip_metadata(ch);if(channels<4)return 7;}
  rnum=real_mobile(626);if(rnum==NOBODY)return 8;
  vict=read_mobile(rnum,REAL);char_to_room(vict,IN_ROOM(ch));target_id=GET_ID(vict);
  ab=compute_attack_bonus_full(ch,vict,ranged?ATTACK_TYPE_RANGED:ATTACK_TYPE_PRIMARY,false);
  circle_srandom(140026);damage=compute_hit_damage(ch,vict,TYPE_HIT,10,ranged?MODE_DISPLAY_RANGED:MODE_DISPLAY_PRIMARY,false,ranged?ATTACK_TYPE_RANGED:ATTACK_TYPE_PRIMARY,DAM_SLICE);
  offhand=compute_hit_damage(ch,vict,TYPE_HIT,10,MODE_DISPLAY_OFFHAND,false,ATTACK_TYPE_OFFHAND,DAM_SLICE);
  circle_srandom(140026);
  for(i=0;i<seconds;i+=2){
    int tick;
    vict=find_char(target_id);
    if(!vict||GET_POS(vict)==POS_DEAD){vict=read_mobile(rnum,REAL);char_to_room(vict,IN_ROOM(ch));target_id=GET_ID(vict);}
    if(!FIGHTING(ch))set_fighting(ch,vict);
    if(!FIGHTING(vict))set_fighting(vict,ch);
    REMOVE_BIT_AR(AFF_FLAGS(vict),AFF_FLAT_FOOTED);
    phase=(i/2)%3+1;
    perform_violence(ch,phase);
    event_cancel_specific(ch,eCOMBAT_ROUND);
    vict=find_char(target_id);if(vict)event_cancel_specific(vict,eCOMBAT_ROUND);
    /* Native timers advance. Only automatic combat is suppressed: this probe
     * explicitly schedules the actor, and the stationary target never swings. */
    for(tick=0;tick<2*PASSES_PER_SEC;tick++){
      pulse++;legendary_affect_schedule(PULSE_VIOLENCE-pulse%PULSE_VIOLENCE);probe_time=1792922400+pulse/PASSES_PER_SEC;event_process();
      if(!(pulse%PASSES_PER_SEC))legendary_effects_pulse();
      if(!(pulse%PULSE_VIOLENCE))affect_update();
    }
    if(GET_POS(ch)==POS_DEAD||GET_HIT(ch)<=0)return 9;
  }
  fprintf(out,"{\"role\":\"%s\",\"candidate\":\"%s\",\"mode\":\"%s\",\"seconds\":%d,\"class\":%d,\"level\":%d,\"equipped_objects\":%d,\"legendary_channels\":%d,\"attack_bonus\":%d,\"primary_display_damage\":%d,\"offhand_display_damage\":%d,\"actual_hp_damage\":%lld,\"damage_packets\":%d}\n",ranged?"ranged_attacker":"dual_wielder",label,mode,seconds,GET_CLASS(ch),GET_LEVEL(ch),equipped,channels,ab,damage,offhand,dealt,packets);
  fclose(out);fclose(manifest);return 0;
}
