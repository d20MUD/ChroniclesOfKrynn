/* Offline native snapshot capture. Linked with the real server objects; never
 * starts init_game/game_loop. Every MySQL connect is forced to the test socket. */
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "comm.h"
#include "handler.h"
#include "fight.h"
#include "act.h"
#include "assign_wpn_armor.h"
#include "legendary_identity.h"
#include "legendary_store.h"
#include "legendary_equipment.h"
#include "legendary_foundations.h"
#include "spells.h"
#include "dg_scripts.h"
#include "dg_event.h"
#include "mysql.h"
#include <curl/curl.h>
extern int auto_pwipe;
MYSQL *__real_mysql_real_connect(MYSQL *,const char *,const char *,const char *,const char *,unsigned int,const char *,unsigned long);
MYSQL *__wrap_mysql_real_connect(MYSQL *m,const char *h,const char *u,const char *p,const char *d,unsigned int port,const char *s,unsigned long flags)
{
  const char *socket=getenv("LEGENDARY_SANDBOX_SOCKET");
  (void)h;(void)u;(void)p;(void)d;(void)port;(void)s;(void)flags;
  if(!socket||!*socket)abort();
  return __real_mysql_real_connect(m,NULL,"root",NULL,"estate_test",0,socket,0);
}
CURLcode __wrap_curl_easy_perform(CURL *handle){(void)handle;return CURLE_COULDNT_CONNECT;}
int __real_socket(int,int,int);
int __wrap_socket(int family,int type,int protocol)
{if(family!=AF_UNIX){errno=EPERM;return -1;}return __real_socket(family,type,protocol);}
static bool add_foundation(struct char_data *ch,struct obj_data *obj,int foundation)
{
  struct legendary_item_record r;int slot=legendary_equipment_slot(obj);
  if(slot<0||!legendary_equipment_eligible(ch,obj,slot,0))return false;
  memset(&r,0,sizeof(r));if(!legendary_uuid_generate(r.uuid))return false;
  r.owner=GET_IDNUM(ch);r.prototype=GET_OBJ_VNUM(obj);r.slot=slot;
  if(!legendary_store_insert(&r))return false;
  r.channels[0].present=true;r.channels[0].rank=5;r.channels[0].path=LEG_PATH_SLAYER;
  r.channels[0].foundation=foundation;r.channels[0].first_choices=1;legendary_cost_between(0,5,&r.channels[0].paid);
  if(!legendary_store_save_channel(&r,0,-1)||!legendary_store_set_state(r.uuid,r.owner,0,LEG_RECORD_ACTIVE))return false;
  memcpy(obj->legendary_uuid,r.uuid,33);GET_OBJ_BOUND_ID(obj)=GET_IDNUM(ch);return legendary_object_verify(obj);
}
int __wrap_main(int argc,char **argv)
{
  FILE *manifest,*out;char label[64],name[64];struct char_data *ch=NULL,*vict;int rnum,attack,mode,ab,ac,dam,control,i,n,lab,lac,ldam,eac,eab,edam;int foundations[]={LEG_F01,LEG_F02,LEG_F02,LEG_F03};struct obj_data *weapon;
  if(argc!=3)return 2;
  manifest=fopen(argv[1],"r");out=fopen(argv[2],"w");if(!manifest||!out)return 3;
  logfile=stderr;CONFIG_CONFFILE=strdup("etc/config");load_config();auto_pwipe=0;no_rent_check=1;circle_srandom(140026);
  event_init();init_lookup_table();boot_db();
  rnum=real_mobile(626);if(rnum==NOBODY){fprintf(stderr,"CAPTURE: missing target prototype 626\n");return 4;}
  vict=read_mobile(rnum,REAL);char_to_room(vict,real_room(2200));GET_POS(vict)=POS_STANDING;
  while(fscanf(manifest,"%63s %63s",label,name)==2){
    ch=new_char();if(load_char(name,ch)<0)continue;
    char_to_room(ch,IN_ROOM(vict));Crash_load(ch);GET_POS(ch)=POS_STANDING;GET_HIT(ch)=GET_MAX_HIT(ch);affect_total(ch);
    attack=GET_EQ(ch,WEAR_WIELD_2H)?ATTACK_TYPE_TWOHAND:ATTACK_TYPE_PRIMARY;
    weapon=is_using_ranged_weapon(ch,true);
    if(weapon)attack=ATTACK_TYPE_RANGED;
    else weapon=GET_EQ(ch,attack==ATTACK_TYPE_TWOHAND?WEAR_WIELD_2H:WEAR_WIELD_1);
    mode=attack==ATTACK_TYPE_RANGED?MODE_DISPLAY_RANGED:MODE_DISPLAY_PRIMARY;
    legendary_loadout_refresh(ch);affect_total(ch);
    eac=compute_armor_class(ch,vict,false,MODE_ARMOR_CLASS_NORMAL);eab=compute_attack_bonus_full(vict,ch,ATTACK_TYPE_PRIMARY,false);edam=compute_hit_damage(vict,ch,TYPE_HIT,10,MODE_DISPLAY_PRIMARY,false,ATTACK_TYPE_PRIMARY,DAM_SLICE);
    circle_srandom(140026);
    ab=compute_attack_bonus_full(ch,vict,attack,false);ac=compute_armor_class(vict,ch,false,MODE_ARMOR_CLASS_NORMAL);
    {
      int saved_target=ch->char_specials.dark_revelation_mob_rnum;
      bool saved_used=ch->char_specials.dark_revelation_used;
      ch->char_specials.dark_revelation_mob_rnum=GET_MOB_RNUM(ch);
      ch->char_specials.dark_revelation_used=false;
      (void)compute_hit_damage(ch,ch,TYPE_HIT,10,mode,false,attack,DAM_SLICE);
      if(ch->char_specials.dark_revelation_used){fprintf(stderr,"CAPTURE: display consumed Dark Revelation\n");return 6;}
      ch->char_specials.dark_revelation_mob_rnum=GET_MOB_RNUM(vict);
      (void)compute_hit_damage(ch,vict,TYPE_HIT,10,mode,false,attack,DAM_SLICE);
      if(ch->char_specials.dark_revelation_used){fprintf(stderr,"CAPTURE: display consumed lored-target bonus\n");return 7;}
      ch->char_specials.dark_revelation_mob_rnum=saved_target;ch->char_specials.dark_revelation_used=saved_used;
    }
    circle_srandom(140026);dam=compute_hit_damage(ch,vict,TYPE_HIT,10,mode,false,attack,DAM_SLICE);
    circle_srandom(140026);control=compute_hit_damage(ch,vict,TYPE_HIT,10,mode,false,attack,DAM_SLICE);
    if(dam!=control){fprintf(stderr,"CAPTURE: nonrepeatable baseline for %s\n",label);return 5;}
    n=0;for(i=0;i<NUM_WEARS&&n<4;i++)if(GET_EQ(ch,i)&&add_foundation(ch,GET_EQ(ch,i),foundations[n]))n++;
    legendary_loadout_refresh(ch);circle_srandom(140026);
    lab=compute_attack_bonus_full(ch,vict,attack,false);lac=compute_armor_class(vict,ch,false,MODE_ARMOR_CLASS_NORMAL);circle_srandom(140026);ldam=compute_hit_damage(ch,vict,TYPE_HIT,10,mode,false,attack,DAM_SLICE);
    fprintf(out,"{\"candidate\":\"%s\",\"class\":%d,\"level\":%d,\"attack_type\":%d,\"offhand\":%d,\"shield\":%d,\"weapon\":%d,\"attack_bonus\":%d,\"attack_cap\":%d,\"armor_class\":%d,\"weapon_damage\":%d,\"max_hp\":%d,\"max_resource\":%d,\"eligible_foundations\":%d,\"legendary_attack_bonus\":%d,\"legendary_ac\":%d,\"legendary_weapon_damage\":%d,\"enemy_ac\":%d,\"enemy_attack\":%d,\"enemy_damage\":%d,\"target_vnum\":%d,\"target_level\":%d,\"control_weapon_damage\":%d,\"can_fire_ammo\":%d,\"point_blank_shot\":%d}\n",
      label,GET_CLASS(ch),GET_LEVEL(ch),attack,GET_EQ(ch,WEAR_WIELD_OFFHAND)!=NULL,GET_EQ(ch,WEAR_SHIELD)!=NULL,weapon!=NULL,
      ab,get_attack_bonus_cap(ch),ac,dam,GET_MAX_HIT(ch),GET_MAX_PSP(ch),n,
      lab,lac,ldam,eac,eab,edam,GET_MOB_VNUM(vict),GET_LEVEL(vict),control,can_fire_ammo(ch,true),HAS_FEAT(ch,FEAT_POINT_BLANK_SHOT));fflush(out);
    /* Keep independent loaded snapshots alive until process exit; extraction
     * must not write player files, run logout scripts, or mutate live data. */
    char_from_room(ch);
  }
  /* After all measurements, verify that real attacks still consume the
   * legitimately lored NPC bonus, while sentinel-matching PCs never do. */
  if(ch){
    char_to_room(ch,IN_ROOM(vict));
    ch->char_specials.dark_revelation_mob_rnum=GET_MOB_RNUM(ch);
    ch->char_specials.dark_revelation_used=false;
    (void)compute_damage_bonus(ch,ch,weapon,TYPE_HIT,NO_MOD,MODE_NORMAL_HIT,attack);
    if(ch->char_specials.dark_revelation_used)return 8;
    ch->char_specials.dark_revelation_mob_rnum=GET_MOB_RNUM(vict);
    (void)compute_damage_bonus(ch,vict,weapon,TYPE_HIT,NO_MOD,MODE_NORMAL_HIT,attack);
    if(!ch->char_specials.dark_revelation_used)return 9;
    char_from_room(ch);
  }
  fclose(out);fclose(manifest);return 0;
}
