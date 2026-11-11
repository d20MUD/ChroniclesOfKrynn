#include "clan_buff_activation.h"
#include "clan_awards.h"
#include <string.h>
static bool uid(const char text[33])
{
  return memchr(text,0,33) && clan_progression_uid_valid(text);
}
bool clan_buff_request_valid(const struct clan_buff_request *r)
{
  return r && uid(r->operation_uid) && uid(r->clan_uid) && r->actor_id>0 &&
      (!r->expected_activation_uid[0] || uid(r->expected_activation_uid)) &&
      r->family>=CLAN_BUFF_SCHOLAR && r->family<=CLAN_BUFF_ARTISAN &&
      r->tier>=1 && r->tier<=CLAN_PROGRESSION_TIER_COUNT &&
      r->slot>=1 && r->slot<=CLAN_PROGRESSION_MAX_SLOTS &&
      r->rules_version==CLAN_PROGRESSION_RULES_VERSION;
}
void clan_buff_validate(const struct clan_progression_config *c,
    const struct clan_progression_readiness *ready,const struct clan_buff_request *r,
    const struct clan_buff_activation_snapshot *s,int64_t now,struct clan_buff_decision *out)
{
  struct clan_buff_decision d;
  struct clan_buff_view view;
  const struct clan_buff_definition *definition;
  const struct clan_buff_state *occupied=NULL;
  int level,combat;
  size_t i;
  memset(&d,0,sizeof(d));
  if (!out) return;
  if (!clan_buff_request_valid(r) || !s ||
      clan_progression_config_validate(c,NULL)!=CLAN_CONFIG_OK) {d.reasons=CLAN_BUFF_INVALID;*out=d;return;}
  if (!clan_progression_feature_available(c,CLAN_FEATURE_BUFFS,ready,r->clan_uid)) d.reasons|=CLAN_BUFF_DISABLED;
  if (!clan_repository_record_valid(&s->clan) || strcmp(s->clan.uid,r->clan_uid) ||
      !clan_award_level(c,s->clan.xp_eighths,&level) || s->count>CLAN_PROGRESSION_BUFF_COUNT ||
      !clan_buff_query(c,r->clan_uid,(enum clan_progression_record_status)s->clan.status,
          level,s->rows,s->count,now,&view)) {d.reasons|=CLAN_BUFF_CORRUPT;*out=d;return;}
  if (s->clan.status!=CLAN_RECORD_ACTIVE) d.reasons|=CLAN_BUFF_DELETED;
  if (!s->accepted_member) d.reasons|=CLAN_BUFF_NOT_MEMBER;
  if (!s->may_manage) d.reasons|=CLAN_BUFF_PERMISSION;
  definition=clan_progression_buff(c,r->family);
  if (level<definition->tiers[r->tier-1].unlock_level) d.reasons|=CLAN_BUFF_LOCKED;
  if (r->slot>view.slots_total) d.reasons|=CLAN_BUFF_BAD_SLOT;
  combat=view.combat_used;
  for (i=0;i<s->count;i++) {
    const struct clan_buff_state *b=&s->rows[i];
    bool active;
    if (strcmp(b->clan_uid,r->clan_uid)) continue;
    active=!b->canceled && now>=b->started_at && now<b->expires_at;
    if (b->family==r->family) {
      if (active) d.reasons|=CLAN_BUFF_ALREADY_ACTIVE;
      if (now<b->next_available_at) d.reasons|=CLAN_BUFF_COOLDOWN;
    }
    if (active && b->slot==r->slot) occupied=b;
  }
  if (occupied) {
    if (!r->expected_activation_uid[0]) d.reasons|=CLAN_BUFF_SLOT_OCCUPIED;
    else if (strcmp(r->expected_activation_uid,occupied->activation_uid)) d.reasons|=CLAN_BUFF_STALE;
    else {
      d.replaced_family=occupied->family;
      strcpy(d.replaced_activation_uid,occupied->activation_uid);
      if (clan_progression_buff(c,occupied->family)->category==CLAN_CATEGORY_COMBAT) combat--;
    }
  } else if (r->expected_activation_uid[0]) d.reasons|=CLAN_BUFF_STALE;
  if (definition->category==CLAN_CATEGORY_COMBAT && combat>=c->combat_slots) d.reasons|=CLAN_BUFF_COMBAT_LIMIT;
  clan_progression_points_to_eighths(definition->tiers[r->tier-1].influence_points,&d.activation.paid_influence_eighths);
  if (s->clan.influence_eighths<d.activation.paid_influence_eighths) d.reasons|=CLAN_BUFF_UNAFFORDABLE;
  if (now<0 || now<s->clan.state_updated_at || now>INT64_MAX-definition->duration_seconds ||
      now+definition->duration_seconds>INT64_MAX-definition->downtime_seconds) d.reasons|=CLAN_BUFF_CLOCK;
  else {
    d.activation.started_at=now;
    d.activation.expires_at=now+definition->duration_seconds;
    d.activation.next_available_at=d.activation.expires_at+definition->downtime_seconds;
  }
  strcpy(d.activation.clan_uid,r->clan_uid);strcpy(d.activation.activation_uid,r->operation_uid);
  d.activation.family=r->family;d.activation.tier=r->tier;d.activation.slot=r->slot;
  d.activation.rules_version=r->rules_version;
  *out=d;
}
const char *clan_buff_rejection_text(unsigned int r)
{
  switch(r) {
    case CLAN_BUFF_INVALID:return "Invalid activation request or configuration.";
    case CLAN_BUFF_DISABLED:return "Buff activation is disabled for this clan.";
    case CLAN_BUFF_DELETED:return "The clan has been disbanded.";
    case CLAN_BUFF_NOT_MEMBER:return "Current accepted clan membership is required.";
    case CLAN_BUFF_PERMISSION:return "Buff-management permission is required.";
    case CLAN_BUFF_LOCKED:return "The requested buff tier is locked by clan level.";
    case CLAN_BUFF_BAD_SLOT:return "The requested buff slot is not unlocked.";
    case CLAN_BUFF_ALREADY_ACTIVE:return "That buff family is already active; it cannot be refreshed.";
    case CLAN_BUFF_COOLDOWN:return "That family is still on its saved cooldown.";
    case CLAN_BUFF_UNAFFORDABLE:return "The clan has insufficient Influence.";
    case CLAN_BUFF_STALE:return "The expected active buff changed; refresh before replacing it.";
    case CLAN_BUFF_SLOT_OCCUPIED:return "The slot is occupied; explicit replacement is required.";
    case CLAN_BUFF_COMBAT_LIMIT:return "The combat-category buff limit would be exceeded.";
    case CLAN_BUFF_CORRUPT:return "Stored clan or buff state is invalid.";
    case CLAN_BUFF_CLOCK:return "The activation timestamp is invalid or would overflow.";
    default:return "Unknown activation rejection.";
  }
}

static void (*invalidation)(const char *,void *);
static void *invalidation_context;
void clan_buff_set_invalidation(void (*callback)(const char *,void *),void *context)
{invalidation=callback;invalidation_context=context;}
void clan_buff_invalidate(const char *uid)
{if(invalidation)invalidation(uid,invalidation_context);}
