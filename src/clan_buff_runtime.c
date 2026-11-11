#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "mysql.h"
#include "clan.h"
#include "clan_persistence.h"
#include "clan_buff_runtime.h"
#include "clan_buff_lifecycle.h"
#include "clan_awards.h"
#include <json-c/json.h>
static struct clan_progression_config configured;
static const struct clan_progression_config *config=&clan_progression_defaults;
static bool dirty=true,registered;
static int64_t last_tick=-1;
static void clear(struct char_data *ch)
{
  int old=ch->clan_vitality_percent;
  ch->clan_artisan_bonus=0;ch->clan_artisan_expires_at=0;
  ch->clan_scholar_percent=0;ch->clan_scholar_expires_at=0;
  ch->clan_psionic_percent=0;ch->clan_psionic_expires_at=0;
  ch->clan_psionic_remainder=0;ch->clan_psionic_activation_uid[0]=0;
  ch->clan_arcane_percent=0;ch->clan_arcane_expires_at=0;
  ch->clan_vitality_percent=0;ch->clan_vitality_expires_at=0;
  ch->clan_buff_clan_uid[0]=0;ch->clan_buff_vnum=NO_CLAN;
  if(!IS_NPC(ch)&&(old||ch->clan_vitality_applied||ch->clan_vitality_was_active))calculate_max_hp(ch,false);
}
static void clear_all(void)
{
  struct char_data *ch;
  for(ch=character_list;ch;ch=ch->next)if(!IS_NPC(ch))clear(ch);
}
static void invalidate(const char *uid,void *context)
{
  (void)uid;(void)context;dirty=true; /* No SQL or guessed state during commit. */
}
static void member_changed(struct char_data *ch,const char *uid)
{
  if(ch->clan_buff_clan_uid[0]&&(!uid||strcmp(ch->clan_buff_clan_uid,uid)))clear(ch);
  dirty=true;
}
bool clan_buff_runtime_configure(const struct clan_progression_config *c)
{
  bool valid=clan_progression_config_validate(c,NULL)==CLAN_CONFIG_OK;
  clear_all();
  if(valid){configured=*c;config=&configured;}else config=&clan_progression_defaults;
  dirty=true;last_tick=-1;return valid;
}
static struct json_object *get(struct json_object *o,const char *key)
{
  struct json_object *v=NULL;json_object_object_get_ex(o,key,&v);return v;
}
static const char *membership(struct json_object *root,int64_t player)
{
  struct json_object *members=get(root,"members");size_t i;
  for(i=0;i<json_object_array_length(members);i++) {
    struct json_object *m=json_object_array_get_idx(members,i);
    if(json_object_get_int64(get(m,"player_id"))==player && json_object_get_int64(get(m,"rank_id"))>0)
      return json_object_get_string(get(m,"clan_uid"));
  }
  return NULL;
}
void clan_buff_runtime_tick(int64_t now)
{
  MYSQL_POOL_CONN *connection;
  struct json_object *root=NULL,*clans;
  struct clan_buff_state rows[CLAN_PROGRESSION_MAX_PILOT_CLANS*CLAN_PROGRESSION_BUFF_COUNT];
  struct clan_buff_view views[CLAN_PROGRESSION_MAX_PILOT_CLANS];
  struct clan_progression_readiness ready={true,false,false,false,CLAN_PROGRESSION_SCHEMA_VERSION};
  size_t count=0,i,j,n;struct char_data *ch;
  if(!registered){clan_buff_set_invalidation(invalidate,NULL);clan_persistence_set_member_invalidation(member_changed);registered=true;}
  if(!clan_progression_campaign_supported()||config->controls.store_mode!=CLAN_STORE_MYSQL||config->controls.mode!=CLAN_PROGRESSION_LIVE||!config->controls.buffs||(!config->controls.public_release&&!config->controls.pilot_count)||!clan_persistence_sql_authority()||!clan_persistence_ready()) {clear_all();return;}
  if(now<0||now<last_tick){clear_all();dirty=true;return;}
  if(now==last_tick&&!dirty)return;
  last_tick=now;dirty=false;
  if(!mysql_available||!(connection=mysql_pool_acquire())){clear_all();dirty=true;return;}
  enum clan_repository_result status=clan_buff_lifecycle_reconcile(connection->conn,now,&root,rows,sizeof(rows)/sizeof(*rows),&count);
  mysql_pool_release(connection);
  if(status!=CLAN_REPO_OK||!clan_repository_snapshot_valid(root)){if(root)json_object_put(root);clear_all();dirty=true;return;}
  clans=get(root,"clans");n=json_object_array_length(clans);
  if(n>CLAN_PROGRESSION_MAX_PILOT_CLANS)goto invalid;
  for(i=0;i<n;i++) {
    struct clan_repository_record record;
    int level;
    if(!clan_repository_record_parse(get(json_object_array_get_idx(clans,i),"core"),&record)||
        !clan_award_level(config,record.xp_eighths,&level)||
        !clan_buff_query(config,record.uid,(enum clan_progression_record_status)record.status,level,rows,count,now,&views[i]))goto invalid;
  }
  for(ch=character_list;ch;ch=ch->next)if(!IS_NPC(ch)) {
    const char *uid=membership(root,GET_IDNUM(ch));
    int percent=0,arcane=0,psionic=0,scholar=0,artisan=0,vnum=NO_CLAN;int64_t expiry=0,arcane_expiry=0,psionic_expiry=0,scholar_expiry=0,artisan_expiry=0;
    const char *psionic_uid="";
    if(uid&&clan_progression_feature_available(config,CLAN_FEATURE_BUFFS,&ready,uid))
      for(i=0;i<n;i++) {
        struct json_object *core=get(json_object_array_get_idx(clans,i),"core");
        if(strcmp(json_object_get_string(get(core,"uid")),uid))continue;
        vnum=json_object_get_int(get(core,"vnum"));
        for(j=0;j<CLAN_PROGRESSION_BUFF_COUNT;j++)if(views[i].families[j].active) {
          if(views[i].families[j].family==CLAN_BUFF_ARTISAN) {
            artisan=clan_progression_buff(config,CLAN_BUFF_ARTISAN)->tiers[views[i].families[j].active_tier-1].amount;
            artisan_expiry=views[i].families[j].expires_at;
          }
          if(views[i].families[j].family==CLAN_BUFF_SCHOLAR) {
            scholar=clan_progression_buff(config,CLAN_BUFF_SCHOLAR)->tiers[views[i].families[j].active_tier-1].amount;
            scholar_expiry=views[i].families[j].expires_at;
          }
          if(views[i].families[j].family==CLAN_BUFF_PSIONIC) {
            psionic=clan_progression_buff(config,CLAN_BUFF_PSIONIC)->tiers[views[i].families[j].active_tier-1].amount;
            psionic_expiry=views[i].families[j].expires_at;
            psionic_uid=views[i].families[j].activation_uid;
          }
          if(views[i].families[j].family==CLAN_BUFF_ARCANE) {
            arcane=clan_progression_buff(config,CLAN_BUFF_ARCANE)->tiers[views[i].families[j].active_tier-1].amount;
            arcane_expiry=views[i].families[j].expires_at;
          }
          if(views[i].families[j].family!=CLAN_BUFF_VITALITY)continue;
          percent=clan_progression_buff(config,CLAN_BUFF_VITALITY)->tiers[views[i].families[j].active_tier-1].amount;
          expiry=views[i].families[j].expires_at;
        }
      }
    bool changed=ch->clan_vitality_percent!=percent||ch->clan_vitality_expires_at!=expiry||
        (uid?strcmp(ch->clan_buff_clan_uid,uid)!=0:ch->clan_buff_clan_uid[0]!=0);
    if(strcmp(ch->clan_psionic_activation_uid,psionic_uid)||
        (uid?strcmp(ch->clan_buff_clan_uid,uid)!=0:ch->clan_buff_clan_uid[0]!=0)||
        ch->clan_psionic_percent!=psionic||ch->clan_psionic_expires_at!=psionic_expiry)
      ch->clan_psionic_remainder=0;
    ch->clan_psionic_percent=psionic;ch->clan_psionic_expires_at=psionic_expiry;
    strcpy(ch->clan_psionic_activation_uid,psionic_uid);
    ch->clan_artisan_bonus=artisan;ch->clan_artisan_expires_at=artisan_expiry;
    ch->clan_scholar_percent=scholar;ch->clan_scholar_expires_at=scholar_expiry;
    ch->clan_arcane_percent=arcane;ch->clan_arcane_expires_at=arcane_expiry;
    ch->clan_vitality_percent=percent;ch->clan_vitality_expires_at=expiry;ch->clan_buff_vnum=vnum;
    if(uid)strcpy(ch->clan_buff_clan_uid,uid);else ch->clan_buff_clan_uid[0]=0;
    if(changed)calculate_max_hp(ch,false);
  }
  json_object_put(root);return;
invalid:
  json_object_put(root);clear_all();dirty=true;
}

void clan_buff_runtime_login_begin(struct char_data *ch)
{
  if(ch&&!IS_NPC(ch)&&clan_progression_campaign_supported()&&config->controls.store_mode==CLAN_STORE_MYSQL&&
      config->controls.mode==CLAN_PROGRESSION_LIVE&&config->controls.buffs&&
      (config->controls.public_release||config->controls.pilot_count)&&clan_persistence_sql_authority())
    ch->clan_buff_login_pending=true;
}
void clan_buff_runtime_login_end(struct char_data *ch,int64_t now)
{
  if(!ch||IS_NPC(ch)||!ch->clan_buff_login_pending)return;
  dirty=true;clan_buff_runtime_tick(now);
  ch->clan_buff_login_pending=false;
  calculate_max_hp(ch,false);
  GET_HIT(ch)=MIN(GET_HIT(ch),GET_MAX_HIT(ch));
}

const struct clan_progression_config *clan_buff_runtime_policy(void) { return config; }
