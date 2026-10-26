#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "spells.h"
#include "class.h"
#include "race.h"
#include "assign_wpn_armor.h"
#include "estate.h"
#include "legendary_identity.h"
#include "legendary_store.h"
#include "legendary_transaction.h"
#include "legendary_equipment.h"
#include "legendary_foundations.h"

#define PRIORITIES 64
struct ref { char uuid[33]; int channel, kind; };
struct source { char uuid[33]; int channel,power; int64_t version;
  void (*revoke)(struct char_data *,void *);void *context;struct source *next; };
struct legendary_equipment_cache {
  struct legendary_loadout view;
  struct ref priority[PRIORITIES],major[3];int priorities,majors;
  bool dirty,loaded,valid,reconciling,combat_locked,building;
  struct legendary_equipped_channel combat[LEGENDARY_MAX_CHANNELS];
  int combat_count;
  unsigned long long fingerprint;
  struct source *sources;
};
static const int resistance_types[]={DAM_ACID,DAM_COLD,DAM_ELECTRIC,DAM_FIRE,DAM_SOUND};
static const int flags[]={ITEM_WEAR_WIELD,ITEM_WEAR_ARMS,ITEM_WEAR_LEGS,ITEM_WEAR_HEAD,
  ITEM_WEAR_BODY,ITEM_WEAR_ABOUT,ITEM_WEAR_SHIELD,ITEM_WEAR_HOLD,ITEM_WEAR_FEET,ITEM_WEAR_HANDS};
static const int wear[]={-1,WEAR_ARMS,WEAR_LEGS,WEAR_HEAD,WEAR_BODY,WEAR_ABOUT,WEAR_SHIELD,-1,WEAR_FEET,WEAR_HANDS};

static bool present_tree(const struct obj_data *obj,int depth)
{
  const struct obj_data *o;
  if(!obj)return false;
  if(depth>64)return true; /* Invalid containment is never transferable. */
  if(obj->legendary_uuid[0]||obj->legendary_quarantined)return true;
  for(o=obj->contains;o;o=o->next_content)if(present_tree(o,depth+1))return true;
  return present_tree(obj->sheath_primary,depth+1)||present_tree(obj->sheath_secondary,depth+1);
}
bool legendary_tree_present(const struct obj_data *obj){return present_tree(obj,0);}
static bool pending_owner(struct char_data *ch,const struct legendary_item_record *r)
{
  MYSQL_RES *res;MYSQL_ROW row;bool ok=false;
  if(r->state!=LEG_RECORD_PENDING||!legendary_transaction_locked(ch)||!legendary_uuid_valid(ch->player_specials->legendary_operation))return false;
  res=estate_select("SELECT COUNT(*) FROM legendary_operations WHERE operation_id='%s' AND player_id=%ld AND uuid='%s' AND state IN ('prepared','committed','applied')",
    ch->player_specials->legendary_operation,GET_IDNUM(ch),r->uuid);
  if(res){row=mysql_fetch_row(res);ok=row&&!strcmp(row[0],"1");mysql_free_result(res);}
  return ok;
}
static bool owner_tree(struct char_data *ch,struct obj_data *obj,int depth)
{
  struct legendary_item_record r;struct obj_data *o;
  if(!obj)return true;
  if(depth>64)return false;
  if(obj->legendary_quarantined)return false;
  if(obj->legendary_uuid[0]) {
    /* Detached storage previews do not claim live identities. */
    if(!ch||IS_NPC(ch)||legendary_store_load(obj->legendary_uuid,&r)!=LEG_LOAD_OK||
       (r.state!=LEG_RECORD_ACTIVE&&!pending_owner(ch,&r))||r.owner!=GET_IDNUM(ch)||r.prototype!=(int)GET_OBJ_VNUM(obj)||
       GET_OBJ_BOUND_ID(obj)!=r.owner)return false;
    if(r.state==LEG_RECORD_ACTIVE) {
      legendary_object_live(obj);
      if(obj->legendary_quarantined)return false;
    }
  }
  for(o=obj->contains;o;o=o->next_content)if(!owner_tree(ch,o,depth+1))return false;
  return owner_tree(ch,obj->sheath_primary,depth+1)&&owner_tree(ch,obj->sheath_secondary,depth+1);
}
bool legendary_tree_allowed(struct char_data *ch,struct obj_data *obj,enum legendary_route route)
{
  if(!legendary_tree_present(obj))return true;
  return route==LEG_ROUTE_OWNER&&owner_tree(ch,obj,0);
}
bool legendary_tree_check(struct char_data *ch,struct obj_data *obj,enum legendary_route route)
{
  if(legendary_tree_allowed(ch,obj,route))return true;
  if(ch)send_to_char(ch,"Awakened equipment is permanently owned: that transfer, conversion, or access is not allowed.\r\n");
  return false;
}
static void quarantine_tree(struct obj_data *obj,int depth)
{
  struct obj_data *o;if(!obj||depth>64)return;
  if(obj->legendary_uuid[0]) {
    obj->legendary_quarantined=true;legendary_object_unload(obj);
    log("SYSERR: Legendary owner transition bypass quarantined UUID %s",obj->legendary_uuid);
  }
  for(o=obj->contains;o;o=o->next_content)quarantine_tree(o,depth+1);
  quarantine_tree(obj->sheath_primary,depth+1);quarantine_tree(obj->sheath_secondary,depth+1);
}
void legendary_holder_check(struct char_data *ch,struct obj_data *obj)
{
  if(!legendary_tree_allowed(ch,obj,LEG_ROUTE_OWNER))quarantine_tree(obj,0);
}
bool legendary_storage_check(struct char_data *ch,struct obj_data *obj,struct obj_data *container)
{
  struct obj_data *root=container;int depth=0;
  if(!legendary_tree_present(obj))return true;
  while(root&&root->in_obj&&depth++<64)root=root->in_obj;
  if(root&&depth<64&&(root->carried_by==ch||root->worn_by==ch)&&
     legendary_tree_allowed(ch,container,LEG_ROUTE_OWNER)&&legendary_tree_allowed(ch,obj,LEG_ROUTE_OWNER))return true;
  send_to_char(ch,"Awakened equipment may only be put in your carried containers or your personal housing storage.\r\n");return false;
}
int legendary_equipment_slot(struct obj_data *o)
{
  int i;
  if(GET_OBJ_TYPE(o)==ITEM_WEAPON||GET_OBJ_TYPE(o)==ITEM_FIREWEAPON)return CAN_WEAR(o,ITEM_WEAR_WIELD)?0:-1;
  for(i=1;i<10;i++)if(CAN_WEAR(o,flags[i]))return i;
  return -1;
}
bool legendary_resource_class(struct char_data *ch)
{
  int i;if(IS_NPC(ch))return false;
  for(i=0;i<NUM_CLASSES;i++)if(CLASS_LEVEL(ch,i)>0&&(is_caster_class(i)||i==CLASS_PSIONICIST||i==CLASS_ARTIFICER))return true;
  return false;
}
bool legendary_supported_skill(int id)
{
  /* These all have real checks in compute_ability/skill_check. In particular,
   * Religion's old UNUSED comment and Total Defense's old label are stale. */
  static const int allowed[]={1,2,3,4,5,6,7,8,9,10,11,12,13,14,17,18,20,22,23,24,26,28,29,31,33};int i;
  for(i=0;i<(int)(sizeof(allowed)/sizeof(*allowed));i++)if(id==allowed[i])return true;
  return false;
}
bool legendary_trained_skill(struct char_data *ch,int id)
{return ch&&!IS_NPC(ch)&&legendary_supported_skill(id)&&GET_ABILITY(ch,id)>0;}
bool legendary_equipment_eligible(struct char_data *ch,struct obj_data *o,int slot,int channel)
{
  struct legendary_item_facts f;int i;memset(&f,0,sizeof(f));
  if(!ch||!o||IS_NPC(ch)||slot<0||slot>=10||o->legendary_quarantined)return false;
  if(ch->char_specials.crafting_object==o||
     (GET_CRAFT(ch).craft_duration>0&&(obj_rnum)GET_CRAFT(ch).craft_obj_rnum==GET_OBJ_RNUM(o)))return false;
  f.character_level=GET_LEVEL(ch);f.item_level=GET_OBJ_LEVEL(o);f.character_id=GET_IDNUM(ch);f.bound_owner=GET_OBJ_BOUND_ID(o);
  f.kind=(GET_OBJ_TYPE(o)==ITEM_WEAPON||GET_OBJ_TYPE(o)==ITEM_FIREWEAPON)?LEG_ITEM_WEAPON:
    (GET_OBJ_TYPE(o)==ITEM_ARMOR||GET_OBJ_TYPE(o)==ITEM_WORN||(slot==7&&GET_OBJ_TYPE(o)==ITEM_TREASURE))?
      (slot==6?LEG_ITEM_SHIELD:slot==7?LEG_ITEM_FOCUS:LEG_ITEM_ARMOR):LEG_ITEM_OTHER;
  f.permanent=!OBJ_FLAGGED(o,ITEM_NORENT)&&!OBJ_FLAGGED(o,ITEM_EXTRACT_AFTER_USE);
  f.expiring=GET_OBJ_TIMER(o)>0;f.transformation=OBJ_FLAGGED(o,ITEM_MOLD)||IS_WILDSHAPED(ch);
  f.ordinary_allowed=!invalid_align(ch,o)&&!invalid_race(ch,o)&&!invalid_class(ch,o)&&
    is_class_req_object(ch,o,false)&&!is_class_anti_object(ch,o,false)&&hands_needed_full(ch,o,true)!=-1;
  for(i=0;i<10;i++)if(CAN_WEAR(o,flags[i]))f.wearable_slots|=LEGENDARY_FLAG(i);
  f.two_hand_slot=GET_EQ(ch,WEAR_WIELD_2H)==o;
  f.actually_wielded=f.two_hand_slot||GET_EQ(ch,WEAR_WIELD_1)==o;
  f.mandatory_two_hand_ranged=is_two_handed_ranged_weapon(o);
  f.offhand_occupied=GET_EQ(ch,WEAR_WIELD_OFFHAND)||GET_EQ(ch,WEAR_SHIELD)||
    GET_EQ(ch,WEAR_HOLD_1)||GET_EQ(ch,WEAR_HOLD_2)||GET_EQ(ch,WEAR_HOLD_2H)||
    (f.two_hand_slot&&GET_EQ(ch,WEAR_WIELD_1));
  return legendary_item_eligible(&f,slot)&&(!channel||legendary_supporting_active(&f));
}
static bool actual_slot(int slot,int position)
{
  if(slot==0)return position==WEAR_WIELD_1||position==WEAR_WIELD_OFFHAND||position==WEAR_WIELD_2H;
  if(slot==7)return position==WEAR_HOLD_1||position==WEAR_HOLD_2||position==WEAR_HOLD_2H;
  return slot>0&&slot<10&&wear[slot]==position;
}
static int hands(struct char_data *ch)
{
  int count=0,i;static const int one[]={WEAR_WIELD_1,WEAR_WIELD_OFFHAND,WEAR_SHIELD,WEAR_HOLD_1,WEAR_HOLD_2};
  for(i=0;i<5;i++)if(GET_EQ(ch,one[i]))count+=one[i]==WEAR_WIELD_1&&is_two_handed_ranged_weapon(GET_EQ(ch,one[i]))?2:1;
  if(GET_EQ(ch,WEAR_WIELD_2H))count+=2;
  if(GET_EQ(ch,WEAR_HOLD_2H))count+=2;
  return count;
}
static int channel_reason(struct char_data *ch,struct obj_data *obj,int pos,int channel)
{
  struct legendary_item_record *r=obj->legendary_record;
  if(obj->legendary_quarantined||!r||r->state!=LEG_RECORD_ACTIVE||strcmp(obj->legendary_uuid,r->uuid)||
     (int)GET_OBJ_VNUM(obj)!=r->prototype)return LEG_BAD_IDENTITY;
  if(IS_NPC(ch)||r->owner!=GET_IDNUM(ch)||GET_OBJ_BOUND_ID(obj)!=r->owner)return LEG_BAD_OWNER;
  if(!actual_slot(r->slot,pos)||obj->worn_by!=ch||obj->worn_on!=pos)return LEG_NOT_EQUIPPED;
  if((r->slot==0||r->slot==6||r->slot==7)&&hands(ch)>2)return LEG_BAD_GRIP;
  if(channel&&!legendary_equipment_eligible(ch,obj,r->slot,1))return LEG_BAD_GRIP;
  if(GET_HIT(ch)<=0||GET_POS(ch)==POS_DEAD||!legendary_equipment_eligible(ch,obj,r->slot,0))return LEG_BAD_EQUIPMENT;
  return LEG_EQUIPPED;
}
const char *legendary_dormant_reason(int reason)
{
  static const char *const names[]={"active","invalid/quarantined identity","wrong owner","ineligible equipment or character",
    "invalid two-hand grip/hand occupancy","not in its actual wear slot","saved configuration unavailable",
    "no selected power","missing skill/resource capability","duplicate selected power"};
  return reason>=0&&reason<(int)(sizeof(names)/sizeof(*names))?names[reason]:"unknown";
}
static bool parse_refs(const char *text,struct ref *out,int *count,int maximum,bool major)
{
  char buffer[4096],*save,*word,tail;int i,n;*count=0;
  if(!text||!*text)return true;
  if(strlen(text)>=sizeof(buffer))return false;
  strcpy(buffer,text);
  for(word=strtok_r(buffer," ",&save);word;word=strtok_r(NULL," ",&save)) {
    struct ref *r=&out[*count];
    if(*count==maximum)return false;
    memset(r,0,sizeof(*r));n=major?sscanf(word,"%32[0-9a-f]/%d/%d%c",r->uuid,&r->channel,&r->kind,&tail):
      sscanf(word,"%32[0-9a-f]/%d%c",r->uuid,&r->channel,&tail);
    if(n!=(major?3:2)||!legendary_uuid_valid(r->uuid)||r->channel<0||r->channel>1||(major&&(r->kind<1||r->kind>2)))return false;
    for(i=0;i<*count;i++)if(!strcmp(out[i].uuid,r->uuid)&&out[i].channel==r->channel&&(!major||out[i].kind==r->kind))return false;
    (*count)++;
  }
  return true;
}
static void config_load(struct char_data *ch,struct legendary_equipment_cache *c)
{
  MYSQL_RES *r;MYSQL_ROW row;c->loaded=true;c->valid=false;c->priorities=c->majors=0;
  if(!legendary_schema_ready())return;
  r=estate_select("SELECT passive_priorities,major_selections FROM legendary_characters WHERE player_id=%ld",GET_IDNUM(ch));
  if(!r)return;
  row=mysql_fetch_row(r);
  c->valid=!row||(parse_refs(row[0],c->priority,&c->priorities,PRIORITIES,false)&&parse_refs(row[1],c->major,&c->majors,3,true));
  mysql_free_result(r);
}
static int priority(struct legendary_equipment_cache *c,const char *uuid,int channel)
{
  int i;for(i=0;i<c->priorities;i++)if(c->priority[i].channel==channel&&!strcmp(c->priority[i].uuid,uuid))return i;
  return PRIORITIES;
}
static bool weapon_ready(struct char_data *ch);
static unsigned long long fingerprint(struct char_data *ch)
{
  unsigned long long h=1469598103934665603ULL;int i,j;struct obj_data *o;
#define HASH(v) h=(h^(unsigned long long)(v))*1099511628211ULL
  HASH(legendary_dragonlance_registry_epoch());
  HASH(GET_HIT(ch)>0);HASH(GET_POS(ch)==POS_DEAD);HASH(GET_LEVEL(ch));HASH(GET_ALIGNMENT(ch));HASH(GET_DISGUISE_RACE(ch));
  HASH(legendary_resource_class(ch));HASH(weapon_ready(ch));
  HASH(RIDING(ch)?GET_ID(RIDING(ch)):0);
  if(RIDING(ch)){HASH(GET_HIT(RIDING(ch))>0);HASH(RIDING(ch)->master==ch);HASH(is_riding_dragon_mount(ch));HASH(GET_REAL_RACE(RIDING(ch)));}
  for(i=0;i<NUM_WEARS;i++) {
    o=GET_EQ(ch,i);HASH(o?GET_ID(o):0);
    if(!o)continue;
    HASH(GET_OBJ_BOUND_ID(o));HASH(GET_OBJ_LEVEL(o));HASH(GET_OBJ_TYPE(o));HASH(GET_OBJ_TIMER(o));HASH(GET_OBJ_SIZE(o));
    for(j=0;j<33;j++)HASH((unsigned char)o->legendary_uuid[j]);
    HASH(o->legendary_quarantined);
    if(o->legendary_record) {
      HASH(o->legendary_record->version);
      HASH(channel_reason(ch,o,i,0));HASH(channel_reason(ch,o,i,1));
      for(j=0;j<2;j++) {
        struct legendary_channel_record *r=&o->legendary_record->channels[j];HASH(r->version);
        if(r->foundation==LEG_F08)HASH(legendary_trained_skill(ch,r->subtype));
      }
    }
  }
  for(i=0;i<5;i++)HASH(legendary_foundation_resistance_base(ch,resistance_types[i]));
#undef HASH
  return h;
}
static void combat_capture(struct legendary_equipment_cache *c)
{
  int i;c->combat_count=0;
  for(i=0;i<c->view.count;i++)if(c->view.channels[i].passive.reason==LEG_PASSIVE_ACTIVE&&c->view.channels[i].passive.points)
    c->combat[c->combat_count++]=c->view.channels[i];
  c->combat_locked=true;c->dirty=true;
}
static bool combat_allows(struct legendary_equipment_cache *c,const struct legendary_equipped_channel *e)
{
  int i;if(!c->combat_locked)return true;
  for(i=0;i<c->combat_count;i++)if(c->combat[i].channel==e->channel&&c->combat[i].version==e->version&&
    c->combat[i].foundation==e->foundation&&c->combat[i].subtype==e->subtype&&!strcmp(c->combat[i].uuid,e->uuid))return true;
  return false;
}
static bool weapon_ready(struct char_data *ch)
{
  static const int positions[]={WEAR_WIELD_1,WEAR_WIELD_OFFHAND,WEAR_WIELD_2H};int i;
  if(hands(ch)>2)return false;
  for(i=0;i<3;i++) {
    struct obj_data *o=GET_EQ(ch,positions[i]);
    if(o&&(GET_OBJ_TYPE(o)==ITEM_WEAPON||GET_OBJ_TYPE(o)==ITEM_FIREWEAPON)&&!o->legendary_quarantined&&
       (GET_OBJ_BOUND_ID(o)<=0||GET_OBJ_BOUND_ID(o)==GET_IDNUM(ch))&&
       !invalid_align(ch,o)&&!invalid_race(ch,o)&&!invalid_class(ch,o)&&
       is_class_req_object(ch,o,false)&&!is_class_anti_object(ch,o,false)&&hands_needed_full(ch,o,true)!=-1)return true;
  }
  return false;
}
static bool power_capable(struct char_data *ch,int id)
{
  const struct legendary_power_def *p=legendary_power(id);
  if(!p)return false;
  if(p->base.resource_min&&!legendary_resource_class(ch))return false;
  if((id==LEG_T01||id==LEG_T03||id==LEG_S09||id==LEG_S11||id==LEG_S13)&&!weapon_ready(ch))return false;
  if(id==LEG_S12&&(!RIDING(ch)||RIDING(ch)->master!=ch||GET_HIT(RIDING(ch))<=0||!is_riding_dragon_mount(ch)||!IS_NPC(RIDING(ch))||GET_REAL_RACE(RIDING(ch))!=RACE_TYPE_DRAGON))return false;
  return true;
}
static void rebuild(struct char_data *ch,struct legendary_equipment_cache *cache)
{
  struct legendary_loadout *v=&cache->view;struct obj_data *o;
  struct legendary_passive_request req[LEGENDARY_MAX_CHANNELS];struct legendary_passive_result result[LEGENDARY_MAX_CHANNELS];
  bool blocked[LEGENDARY_MAX_CHANNELS]={false};
  int i,j,k,n=0;cache->building=true;memset(v,0,sizeof(*v));v->ready=cache->valid;
  /* Claim every identity before calculating any channel: a duplicate discovered
   * later in the outfit must suppress the earlier copy in this same rebuild. */
  for(i=0;i<NUM_WEARS;i++) {
    o=GET_EQ(ch,i);
    if(o&&*o->legendary_uuid&&!o->legendary_quarantined&&!o->legendary_record)legendary_object_verify(o);
  }
  for(i=0;i<NUM_WEARS;i++) {
    o=GET_EQ(ch,i);if(!o||!*o->legendary_uuid)continue;
    if(!o->legendary_record&&!o->legendary_quarantined)legendary_object_verify(o);
    for(j=0;j<2&&n<LEGENDARY_MAX_CHANNELS;j++) {
      struct legendary_equipped_channel *e=&v->channels[n];struct legendary_item_record *r=o->legendary_record;
      if(j&&(!r||!r->channels[j].present))continue;
      memcpy(e->uuid,o->legendary_uuid,33);e->channel=j;e->wear=i;
      e->reason=cache->valid?channel_reason(ch,o,i,j):LEG_BAD_CONFIG;
      if(r&&r->channels[j].present){struct legendary_channel_record *c=&r->channels[j];
        e->item_passive=r->channels[0].item_passive;e->rank=c->rank;e->path=c->path;e->family=c->family;e->foundation=c->foundation;e->subtype=c->subtype;
        e->technique=c->technique;e->signature=c->signature;e->modifier=c->modifier;e->modifier_target=c->modifier_target;e->version=c->version;}
      n++;
    }
  }
  v->count=n;
  for(i=1;i<n;i++){struct legendary_equipped_channel e=v->channels[i];j=i;
    while(j>0&&priority(cache,v->channels[j-1].uuid,v->channels[j-1].channel)>priority(cache,e.uuid,e.channel)){
      v->channels[j]=v->channels[j-1];j--;}
    v->channels[j]=e;}
  memset(req,0,sizeof(req));
  for(i=0;i<n;i++) {
    struct legendary_equipped_channel *e=&v->channels[i];req[i].id=e->foundation;req[i].subtype=e->subtype;
    req[i].eligible=e->reason==LEG_EQUIPPED&&
      (e->foundation!=LEG_F08||legendary_trained_skill(ch,e->subtype))&&
      (e->foundation!=LEG_F11||legendary_resource_class(ch));
    if(req[i].eligible&&!combat_allows(cache,e)){req[i].eligible=false;blocked[i]=true;}
    /* Phase 8 integrates typed nonlegendary resistance and ability/stat caps. */
    if(e->foundation==LEG_F12&&e->subtype>=0&&e->subtype<5)req[i].current_resistance=legendary_foundation_resistance_base(ch,resistance_types[e->subtype]);
  }
  if(legendary_plan_passives(req,n,result,&v->points))for(i=0;i<n;i++){v->channels[i].passive=result[i];if(blocked[i])v->channels[i].passive.reason=LEG_PASSIVE_COMBAT;}
  v->major_count=cache->majors;
  for(i=0;i<cache->majors;i++) {
    struct legendary_equipped_major *m=&v->majors[i];struct ref *r=&cache->major[i];
    memcpy(m->uuid,r->uuid,33);m->channel=r->channel;m->kind=r->kind;m->reason=cache->valid?LEG_NOT_EQUIPPED:LEG_BAD_CONFIG;
    for(j=0;j<n;j++)if(v->channels[j].channel==r->channel&&!strcmp(v->channels[j].uuid,r->uuid)){
      m->power=r->kind==1?v->channels[j].technique:v->channels[j].signature;
      m->reason=v->channels[j].reason==LEG_EQUIPPED&&!m->power?LEG_NO_SELECTION:v->channels[j].reason;break;}
    if(m->reason==LEG_EQUIPPED&&!power_capable(ch,m->power))m->reason=LEG_BAD_CAPABILITY;
    if(m->reason==LEG_EQUIPPED)for(k=0;k<i;k++)if(v->majors[k].reason==LEG_EQUIPPED&&v->majors[k].power==m->power)m->reason=LEG_DUPLICATE_POWER;
  }
  cache->dirty=false;cache->fingerprint=fingerprint(ch);cache->building=false;
}
static bool source_valid(struct char_data *ch,struct source *s)
{
  struct legendary_equipment_cache *c=ch->legendary_equipment;int i,j;
  if(!power_capable(ch,s->power))return false;
  for(i=0;i<c->view.count;i++){
    struct legendary_equipped_channel *e=&c->view.channels[i];struct obj_data *o=GET_EQ(ch,e->wear);
    if(e->channel!=s->channel||strcmp(e->uuid,s->uuid)||e->version!=s->version||!o||
       strcmp(o->legendary_uuid,s->uuid)||channel_reason(ch,o,e->wear,s->channel)!=LEG_EQUIPPED)continue;
    if(legendary_power(s->power)->requires_dragonlance){struct legendary_registry_record r;
      if(!legendary_registry_cached(GET_OBJ_VNUM(o),true,&r)||!r.enabled)return false;}
    for(j=0;j<c->view.major_count;j++)if(c->view.majors[j].reason==LEG_EQUIPPED&&c->view.majors[j].channel==s->channel&&
      c->view.majors[j].power==s->power&&!strcmp(c->view.majors[j].uuid,s->uuid))return true;
  }
  return false;
}
static void reconcile(struct char_data *ch)
{
  struct legendary_equipment_cache *c=ch->legendary_equipment;struct source **at,*s;
  if(!c||c->reconciling||ch->legendary_serializing)return;
  c->reconciling=true;
  at=&c->sources;
  while((s=*at)) {if(!source_valid(ch,s)){*at=s->next;s->revoke(ch,s->context);free(s);}else at=&s->next;}
  c->reconciling=false;
}
void legendary_loadout_invalidate(struct char_data *ch)
{ if(ch&&ch->legendary_equipment){ch->legendary_equipment->dirty=true;reconcile(ch);} }
const struct legendary_loadout *legendary_loadout_get(struct char_data *ch)
{
  static const struct legendary_loadout empty={.ready=true};
  struct legendary_equipment_cache *c;int i;bool any=false;
  if(!ch||IS_NPC(ch))return &empty;
  for(i=0;i<NUM_WEARS;i++)if(GET_EQ(ch,i)&&*GET_EQ(ch,i)->legendary_uuid)any=true;
  c=ch->legendary_equipment;if(!c&&!any)return &empty;
  if(!c){c=calloc(1,sizeof(*c));if(!c)return &empty;ch->legendary_equipment=c;c->dirty=true;}
  if(c->reconciling||c->building)return &c->view;
  if(!FIGHTING(ch)&&c->combat_locked){c->combat_locked=false;c->dirty=true;}
  else if(FIGHTING(ch)&&!c->combat_locked)combat_capture(c);
  if(!c->loaded){config_load(ch,c);c->dirty=true;}
  if(c->dirty||c->fingerprint!=fingerprint(ch)){rebuild(ch,c);reconcile(ch);}
  legendary_foundations_sync(ch,&c->view);
  return &c->view;
}
void legendary_loadout_combat_start(struct char_data *ch)
{
  if(!ch||IS_NPC(ch))return;
  (void)legendary_loadout_get(ch);
  if(!ch->legendary_equipment)ch->legendary_equipment=calloc(1,sizeof(*ch->legendary_equipment));
  if(ch->legendary_equipment)combat_capture(ch->legendary_equipment);
}
void legendary_loadout_refresh(struct char_data *ch)
{
  if(!ch||IS_NPC(ch))return;
  if(!ch->legendary_equipment)ch->legendary_equipment=calloc(1,sizeof(*ch->legendary_equipment));
  if(ch->legendary_equipment){ch->legendary_equipment->loaded=false;(void)legendary_loadout_get(ch);}
}
bool legendary_source_attach(struct char_data *ch,const char *uuid,int channel,int power,
    void (*revoke)(struct char_data *,void *),void *context)
{
  const struct legendary_loadout *v=legendary_loadout_get(ch);struct source *s;int i;
  if(!ch||!legendary_uuid_valid(uuid)||!revoke||!ch->legendary_equipment||!v->ready)return false;
  s=calloc(1,sizeof(*s));if(!s)return false;memcpy(s->uuid,uuid,33);s->channel=channel;s->power=power;
  for(i=0;i<v->count;i++)if(v->channels[i].channel==channel&&!strcmp(v->channels[i].uuid,uuid))s->version=v->channels[i].version;
  if(!source_valid(ch,s)){free(s);return false;}
  s->revoke=revoke;s->context=context;s->next=ch->legendary_equipment->sources;ch->legendary_equipment->sources=s;return true;
}
void legendary_source_forget(struct char_data *ch,void *context)
{
  struct source **at,*s;if(!ch||!ch->legendary_equipment)return;at=&ch->legendary_equipment->sources;
  while((s=*at)){if(s->context==context){*at=s->next;free(s);}else at=&s->next;}
}
void legendary_loadout_free(struct char_data *ch)
{
  struct source *s,*next;struct legendary_equipment_cache *cache;
  if(!ch||!ch->legendary_equipment)return;
  cache=ch->legendary_equipment;s=cache->sources;cache->sources=NULL;cache->reconciling=true;
  for(;s;s=next){next=s->next;s->revoke(ch,s->context);free(s);}
  free(cache);ch->legendary_equipment=NULL;
}

int legendary_item_passive_value(struct char_data *ch,int id)
{
  const struct legendary_loadout *v;const struct legendary_item_passive_def *p;
  int i,total=0;
  if(!ch||IS_NPC(ch)||!(p=legendary_item_passive(id)))return 0;
  if(id==2&&!legendary_resource_class(ch))return 0;
  v=legendary_loadout_get(ch);if(!v->ready)return 0;
  for(i=0;i<v->count;i++)if(v->channels[i].reason==LEG_EQUIPPED&&v->channels[i].item_passive==id)
    total+=legendary_item_passive_amount(id,v->channels[i].rank);
  return MIN(total,p->ceiling);
}
