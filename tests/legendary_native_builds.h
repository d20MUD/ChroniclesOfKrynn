/* Deterministic, ordinary level-30 geared role blueprints. No legendary bonuses
 * are installed here. Advancement and common grade-six gear use native code. */
#include "premadebuilds.h"
#include "spell_prep.h"
extern void init_start_char(struct char_data *);
extern void levelup_wizard(struct char_data *,int,bool);
extern void levelup_cleric(struct char_data *,int,bool);
extern void levelup_psionicist(struct char_data *,int,bool);
static const char *build_roles[]={"two_handed_martial","sword_and_shield","dual_wielder","ranged_attacker","offensive_caster","psionic_character","healer_support"};
static struct obj_data *build_weapon(struct char_data *ch,int type,int wear)
{
  struct obj_data *o=read_object(WEAPON_PROTO,VIRTUAL);
  if(!o)return NULL;set_weapon_object(o,type);
  cp_modify_object_applies(ch,o,6,CP_TYPE_WEAPON,RARE_GRADE_NORMAL,1,30);
  if(!o->carried_by)obj_to_char(o,ch);
  perform_wear(ch,o,wear);if(GET_EQ(ch,wear)!=o)fprintf(stderr,"BLUEPRINT weapon failed: type=%d wear=%d level=%d class=%d str=%d flags=%d\n",type,wear,GET_LEVEL(ch),GET_CLASS(ch),GET_STR(ch),CAN_WEAR(o,ITEM_WEAR_WIELD));if(GET_EQ(ch,wear)!=o)fprintf(stderr,"BLUEPRINT preconditions: owner=%ld actor=%ld mold=%d needed=%d freehands=%d race=%d animal=%d req=%d anti=%d carry=%d\n",(long)GET_OBJ_BOUND_ID(o),(long)GET_IDNUM(ch),!!OBJ_FLAGGED(o,ITEM_MOLD),hands_needed_full(ch,o,true),hands_available(ch),GET_RACE(ch),!!IS_ANIMAL(ch),is_class_req_object(ch,o,false),is_class_anti_object(ch,o,false),o->carried_by==ch);return GET_EQ(ch,wear)==o?o:NULL;
}
static bool build_armor(struct char_data *ch,int wear,int flag,int kind)
{
  struct obj_data *o;int k,selected=-1;
  for(k=1;k<NUM_SPEC_ARMOR_TYPES;k++)if(armor_list[k].wear==flag&&armor_list[k].armorType==kind){selected=k;break;}
  if(selected<0){fprintf(stderr,"BLUEPRINT armor absent: wear=%d flag=%d kind=%d\n",wear,flag,kind);return false;}
  o=read_object(ARMOR_PROTO,VIRTUAL);if(!o)return false;set_armor_object(o,selected);
  cp_modify_object_applies(ch,o,6,CP_TYPE_ARMOR,RARE_GRADE_NORMAL,1,30);
  if(!o->carried_by)obj_to_char(o,ch);
  perform_wear(ch,o,wear);if(GET_EQ(ch,wear)!=o)fprintf(stderr,"BLUEPRINT armor/misc wear failed: wear=%d type=%d\n",wear,GET_OBJ_TYPE(o));return GET_EQ(ch,wear)==o;
}
static bool build_misc(struct char_data *ch,int proto,int wear)
{
  struct obj_data *o=read_object(proto,VIRTUAL);if(!o)return false;
  cp_modify_object_applies(ch,o,6,CP_TYPE_MISC,RARE_GRADE_NORMAL,1,30);
  if(!o->carried_by)obj_to_char(o,ch);
  perform_wear(ch,o,wear);if(GET_EQ(ch,wear)!=o)fprintf(stderr,"BLUEPRINT armor/misc wear failed: wear=%d type=%d\n",wear,GET_OBJ_TYPE(o));return GET_EQ(ch,wear)==o;
}
static struct char_data *build_role(int role)
{
  struct char_data *ch=new_char();int level,k,class_id=role==2?CLASS_ROGUE:role==4?CLASS_WIZARD:role==5?CLASS_PSIONICIST:role==6?CLASS_CLERIC:CLASS_WARRIOR;
  void (*levelup)(struct char_data *,int,bool)=class_id==CLASS_ROGUE?levelup_rogue:class_id==CLASS_WIZARD?levelup_wizard:class_id==CLASS_PSIONICIST?levelup_psionicist:class_id==CLASS_CLERIC?levelup_cleric:levelup_warrior;
  char name[32];snprintf(name,sizeof(name),"Legendbench%c",'a'+role);
  ch->player.name=strdup(name);GET_CLASS(ch)=class_id;GET_REAL_RACE(ch)=
#ifdef CAMPAIGN_DL
    DL_RACE_HUMAN;
#else
    RACE_HUMAN;
#endif
  GET_LEVEL(ch)=0;GET_ALIGNMENT(ch)=500;
  create_entry(name);init_char(ch);GET_PREMADE_BUILD_CLASS(ch)=CLASS_UNDEFINED;
  char_to_room(ch,real_room(2200));init_start_char(ch);
  for(level=1;level<=30;level++){
    GET_LEVEL(ch)=level;CLASS_LEVEL(ch,class_id)=level;advance_level(ch,class_id);
    if(level<=20){setup_premade_levelup(ch,class_id);levelup(ch,level,false);}
    else {
      increase_skills(ch,class_id,false,level);
      if(level%4==0&&GET_BOOSTS(ch)>0){
        if(class_id==CLASS_WIZARD||class_id==CLASS_PSIONICIST)GET_REAL_INT(ch)++;
        else if(class_id==CLASS_CLERIC)GET_REAL_WIS(ch)++;else if(class_id==CLASS_ROGUE)GET_REAL_DEX(ch)++;else GET_REAL_STR(ch)++;
        GET_BOOSTS(ch)--;
      }
    }
  }
  if(role==3){
    int str=GET_REAL_STR(ch),cfeat;
    GET_REAL_STR(ch)=GET_REAL_DEX(ch);GET_REAL_DEX(ch)=str;
    for(cfeat=0;cfeat<NUM_CFEATS;cfeat++)if(HAS_COMBAT_FEAT(ch,cfeat,WEAPON_FAMILY_HEAVY_BLADE)){
      REMOVE_BIT_AR(ch->char_specials.saved.combat_feats[cfeat],WEAPON_FAMILY_HEAVY_BLADE);SET_COMBAT_FEAT(ch,cfeat,WEAPON_FAMILY_RANGED);
    }
    /* Same three elective feat slots, choosing the ranged counterparts. */
    SET_FEAT(ch,FEAT_LUCK_OF_HEROES,0);SET_FEAT(ch,FEAT_GREAT_FORTITUDE,0);SET_FEAT(ch,FEAT_POWER_ATTACK,0);
    SET_FEAT(ch,FEAT_POINT_BLANK_SHOT,1);SET_FEAT(ch,FEAT_PRECISE_SHOT,1);SET_FEAT(ch,FEAT_RAPID_SHOT,1);
    SET_BIT_AR(AFF_FLAGS(ch),AFF_RAPID_SHOT);
  }
  GET_ID(ch)=GET_IDNUM(ch);add_to_lookup_table(GET_ID(ch),ch);ch->next=character_list;character_list=ch;init_condensed_combat_data(ch);
  if(role==0){if(!build_weapon(ch,WEAPON_TYPE_GREAT_SWORD,WEAR_WIELD_2H))return NULL;}
  else if(role==1){if(!build_weapon(ch,WEAPON_TYPE_LONG_SWORD,WEAR_WIELD_1)||!build_armor(ch,WEAR_SHIELD,ITEM_WEAR_SHIELD,ARMOR_TYPE_SHIELD))return NULL;}
  else if(role==2){if(!build_weapon(ch,WEAPON_TYPE_DAGGER,WEAR_WIELD_1)||!build_weapon(ch,WEAPON_TYPE_DAGGER,WEAR_WIELD_OFFHAND))return NULL;}
  else if(role==3){if(!build_weapon(ch,WEAPON_TYPE_LONG_BOW,WEAR_WIELD_2H))return NULL;}
  else {if(!build_weapon(ch,WEAPON_TYPE_DAGGER,WEAR_WIELD_1)||!build_misc(ch,HELD_MOLD,WEAR_HOLD_1))return NULL;}
  {
    static const int wears[]={WEAR_ARMS,WEAR_LEGS,WEAR_HEAD,WEAR_BODY};
    static const int flags[]={ITEM_WEAR_ARMS,ITEM_WEAR_LEGS,ITEM_WEAR_HEAD,ITEM_WEAR_BODY};
    for(k=0;k<4;k++)if(!build_armor(ch,wears[k],flags[k],role>=4?ARMOR_TYPE_NONE:role==2?ARMOR_TYPE_LIGHT:ARMOR_TYPE_HEAVY))return NULL;
  }
  if(!build_misc(ch,CLOAK_MOLD,WEAR_ABOUT)||!build_misc(ch,BOOTS_MOLD,WEAR_FEET)||!build_misc(ch,GLOVES_MOLD,WEAR_HANDS))return NULL;
  build_misc(ch,RING_MOLD,WEAR_FINGER_R);build_misc(ch,RING_MOLD,WEAR_FINGER_L);
  build_misc(ch,NECKLACE_MOLD,WEAR_NECK_1);build_misc(ch,NECKLACE_MOLD,WEAR_NECK_2);
  build_misc(ch,WRIST_MOLD,WEAR_WRIST_R);build_misc(ch,WRIST_MOLD,WEAR_WRIST_L);build_misc(ch,BELT_MOLD,WEAR_WAIST);
  affect_total(ch);GET_HIT(ch)=GET_MAX_HIT(ch);GET_MOVE(ch)=GET_MAX_MOVE(ch);GET_PSP(ch)=GET_MAX_PSP(ch);GET_POS(ch)=POS_STANDING;
  return ch;
}
