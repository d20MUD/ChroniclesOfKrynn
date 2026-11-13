#define _POSIX_C_SOURCE 200809L
#include "clan_status.h"
#include "clan_awards.h"
#include "clan_display.h"
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
struct text_writer { char text[16384];size_t used;bool ok; };
static void append(struct text_writer *w,const char *format,...)
{
  int n;va_list args;
  if (!w->ok) return;
  va_start(args,format);n=vsnprintf(w->text+w->used,sizeof(w->text)-w->used,format,args);va_end(args);
  if (n<0 || (size_t)n>=sizeof(w->text)-w->used) w->ok=false;
  else w->used+=(size_t)n;
}
static const char *points(int64_t value,char out[48])
{
  static const char *fractions[]={"",".125",".25",".375",".5",".625",".75",".875"};
  if (value<0) return "invalid";
  snprintf(out,48,"%" PRId64 "%s",value/8,fractions[value%8]);return out;
}
bool clan_status_render(const struct clan_progression_config *c,
    const struct clan_status_report *r,char *out,size_t capacity)
{
  struct text_writer w={{0},0,true};
  struct clan_reset_period period;
  struct clan_buff_view buffs;
  struct clan_progression_readiness ready={true,false,false,false,CLAN_PROGRESSION_SCHEMA_VERSION};
  enum clan_favor_standing standing;
  int level,i;
  int64_t favor_limit,influence_limit,weekly_influence_limit;
  char a[48],b[48],at[64];
  const char *membership[]={"accepted member","applicant","former/non-member","deleted clan"};
  if (!r || !out || !capacity || clan_progression_config_validate(c,NULL)!=CLAN_CONFIG_OK ||
      !clan_repository_record_valid(&r->clan) || r->membership<CLAN_ACCESS_ACCEPTED ||
      r->membership>CLAN_ACCESS_DELETED || !clan_award_level(c,r->clan.xp_eighths,&level) ||
      !clan_award_standing(r->favor.favor_eighths,&standing) ||
      r->favor.character_id<=0 || strcmp(r->favor.clan_uid,r->clan.uid) || r->favor.revision<0 ||
      !clan_progression_account_window_valid(&r->account) ||
      !clan_progression_week_at(r->queried_at,&period) || r->account.starts_at!=period.starts_at ||
      !clan_progression_points_to_eighths(c->limits.account_favor_points,&favor_limit) ||
      r->account.favor_eighths>favor_limit ||
      !clan_progression_points_to_eighths(c->limits.stored_influence_points,&influence_limit) ||
      r->clan.influence_eighths>influence_limit ||
      strcmp(r->weekly.clan_uid,r->clan.uid) || r->weekly.starts_at!=period.starts_at ||
      r->weekly.incoming_xp_eighths<0 || r->weekly.influence_eighths<0 || r->weekly.revision<0 ||
      !clan_progression_points_to_eighths(c->limits.clan_weekly_influence_points,&weekly_influence_limit) ||
      r->weekly.influence_eighths>weekly_influence_limit ||
      r->buff_count>CLAN_PROGRESSION_BUFF_COUNT ||
      (r->receipt_present && (r->recent.xp_eighths<0 || r->recent.influence_eighths<0 ||
       r->recent.favor_eighths<0 || r->recent.cap_flags<0))) return false;
  if (r->full_clan_access && !clan_buff_query(c,r->clan.uid,(enum clan_progression_record_status)r->clan.status,
      level,r->buffs,r->buff_count,r->queried_at,&buffs)) return false;
  append(&w,"\r\n\tC%s - Clan Progress\tn\r\n",r->clan.name);
  append(&w,"\tc------------------------------------------------------------\tn\r\n");
  append(&w,"  Membership    %s\r\n",membership[r->membership]);
  if (!clan_progression_feature_available(c,CLAN_FEATURE_EARNINGS,&ready,r->clan.uid) ||
      r->clan.status!=CLAN_RECORD_ACTIVE)
    append(&w,"  Earnings      Currently paused\r\n");
  if (r->full_clan_access) {
    append(&w,"\r\n\tWClan Rank\tn\r\n  Level         %d",level);
    if (level<c->max_level) append(&w,"  (%s Influence to level %d)",points(c->xp_points[level]*8-r->clan.xp_eighths,b),level+1);
    else append(&w,"  (maximum)");
    append(&w,"\r\n  Lifetime      %s %s\r\n",points(r->clan.xp_eighths,a),c->rules_version>=2?"Influence":"Clan XP");
    append(&w,"  Available     %s Influence  /  %" PRId64 " stored maximum\r\n",
      points(r->clan.influence_eighths,a),c->limits.stored_influence_points);
    append(&w,"  This week     %s / %" PRId64 " Influence earned\r\n",
      points(r->weekly.influence_eighths,a),c->limits.clan_weekly_influence_points);
  } else append(&w,"Clan balances and buffs require current accepted membership.\r\n");
  append(&w,"\r\n\tWYour Standing\tn\r\n  Favor         %s  -  %s\r\n",
      points(r->favor.favor_eighths,a),c->standings[standing].name);
  if (standing+1<CLAN_PROGRESSION_STANDING_COUNT)
    append(&w,"  Next rank     %s Favor to %s\r\n",points(c->standings[standing+1].favor_points*8-r->favor.favor_eighths,b),c->standings[standing+1].name);
  append(&w,"  Favor is permanent and never spent.\r\n");
  append(&w,"\r\n\tWYour Week\tn  (shared across your account)\r\n");
  append(&w,"  Favor earned  %s / %" PRId64 "\r\n",points(r->account.favor_eighths,a),c->limits.account_favor_points);
  append(&w,"  Resets        %s\r\n",clan_display_time(period.ends_at,at));
  append(&w,"  Contribution %s used; full rewards through %" PRId64 ", half through %" PRId64 "\r\n",
    points(r->account.base_xp_eighths,a),c->limits.account_full_xp_points,
    c->limits.account_full_xp_points+c->limits.account_half_xp_points);
  for (i=CLAN_SOURCE_KILL;i<=CLAN_SOURCE_EXPEDITION_PERSONAL;i++) {
    static const char *names[]={"","Kills","Supply orders","Deliveries","Gathering","Bounties","Expeditions"};
    const struct clan_reward_definition *source=clan_progression_reward(c,(enum clan_reward_source)i);
    append(&w,"  %-14s%" PRId64 " / %d\r\n",names[i],r->account.claims[i],source->weekly_claims);
  }
  if (r->receipt_present) {
    append(&w,"\r\n\tWLast Reward\tn\r\n  +%s Favor   +%s Influence\r\n",
      points(r->recent.favor_eighths,a),points(r->recent.influence_eighths,b));
    for (i=1;i<=CLAN_AWARD_SOURCE_REDUCED;i*=2)
      if (r->recent.cap_flags&i) append(&w,"  %s\r\n",clan_award_explanation(i));
  }
  if (r->full_clan_access) {
    append(&w,"\r\n\tWClan Buffs\tn  -  %d / %d slots used; %d / %d combat\r\n",buffs.slots_used,buffs.slots_total,buffs.combat_used,c->combat_slots);
    for (i=0;i<CLAN_PROGRESSION_BUFF_COUNT;i++) {
      const struct clan_buff_family_view *f=&buffs.families[i];
      const struct clan_buff_definition *definition=clan_progression_buff(c,f->family);
      append(&w,"  %s: ",definition->name);
      if (r->clan.status==CLAN_RECORD_DELETED) append(&w,"clan disbanded");
      else if (!f->unlocked_tier) append(&w,"locked (first tier at level %d)",definition->tiers[0].unlock_level);
      else append(&w,"tier %d unlocked",f->unlocked_tier);
      if (f->active) append(&w,"; tier %d active in slot %d until %s",f->active_tier,f->active_slot,clan_display_time(f->expires_at,at));
      if (f->cooling_down) append(&w,"; ready %s",clan_display_time(f->next_available_at,at));
      else if (f->available_slots) append(&w,"; free slot available");
      else if (f->unlocked_tier && !f->active && r->clan.status==CLAN_RECORD_ACTIVE)
        append(&w,"; %s",f->category_available?"no free slot":"combat slot occupied");
      append(&w,".\r\n");
    }
    if (!clan_progression_feature_available(c,CLAN_FEATURE_BUFFS,&ready,r->clan.uid))
      append(&w,"Buff activation is disabled.\r\n");
  }
  if (!w.ok || w.used>=capacity) return false;
  memcpy(out,w.text,w.used+1);return true;
}
