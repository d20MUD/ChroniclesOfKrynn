/* Player configuration front end. All writes participate in the Phase 4 journal. */
#include "conf.h"
#include "sysdep.h"
#include "legendary_foundations.h"
#include "legendary_effects.h"
#include "legendary_techniques.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "spells.h"
#include "class.h"
#include "race.h"
#include "assign_wpn_armor.h"
#include "constants.h"
#include "interpreter.h"
#include "act.h"
#include "estate.h"
#include "legendary_commands.h"
#include "legendary_equipment.h"
#include "legendary_identity.h"
#include "legendary_transaction.h"
#include "legendary_earnings.h"
#include <inttypes.h>
#include <errno.h>

#define WORDS 14
#define WORDLEN 256
#define REFS 64
struct reference { char uuid[33]; int channel, kind; };
struct arrangement { struct reference priority[REFS], major[3]; int priorities, majors; };
struct pending {
  long owner;
  long smith;
  room_rnum room;
  time_t expires;
  unsigned long long inventory, facts;
  struct obj_data *object;
  int channel;
  struct legendary_item_record item;
  struct legendary_wallet delta;
  struct legendary_cost cost;
  bool matching, purchase;
  struct legendary_quote *quote;
  struct legendary_exchange *exchange;
  struct arrangement layout;
  bool save_layout;
  struct pending *next;
};
static struct pending *pending;
static const char *const channels[]={"primary","supporting"};
static const char *const abilities[]={"str","dex","con","int","wis","cha"};
static const char *const elements[]={"acid","cold","electricity","fire","sonic"};


static int words(const char *s,char out[WORDS][WORDLEN])
{
  int n=0,k; char quote;
  while(*s) {
    while(isspace((unsigned char)*s))s++;
    if(!*s)break;
    if(n==WORDS)return -1;
    quote=(*s=='\''||*s=='"')?*s++:0;k=0;
    while(*s && (quote?*s!=quote:!isspace((unsigned char)*s))) {
      if(k==WORDLEN-1)return -1;
      out[n][k++]=*s++;
    }
    if(quote && *s++!=quote)return -1;
    out[n++][k]=0;
  }
  return n;
}
static int named(const char *s,const char *const *list,int count)
{ int i;for(i=0;i<count;i++)if(!strcasecmp(s,list[i]))return i;return -1; }
static int path_id(const char *s)
{ int i;for(i=0;i<12;i++)if(!strcasecmp(s,legendary_paths[i].name))return legendary_paths[i].id;return 0; }
static int integer(const char *s)
{
  char *end;long n;
  if(!s||!*s||!isdigit((unsigned char)*s))return -1;
  errno=0;n=strtol(s,&end,10);return errno||*end||n>INT_MAX?-1:(int)n;
}
static struct obj_data *find_item(struct char_data *ch,const char *name)
{
  struct obj_data *obj,*found=NULL;int i,count=0,ordinal=0;const char *dot=strchr(name,'.');
  if(dot) {char number[16];size_t len=dot-name;if(!len||len>=sizeof(number))return NULL;
    memcpy(number,name,len);number[len]=0;ordinal=integer(number);if(ordinal<1)return NULL;name=dot+1;}
  for(obj=ch->carrying;obj;obj=obj->next_content)if(isname(name,obj->name)) {
    count++;if(!ordinal||count==ordinal)found=obj;
  }
  for(i=0;i<NUM_WEARS;i++)if((obj=GET_EQ(ch,i)) && isname(name,obj->name)) {
    count++;if(!ordinal||count==ordinal)found=obj;
  }
  if(!found || (!ordinal && count>1)) {
    send_to_char(ch,"%s Use a unique keyword, a quoted name, or 2.name.\r\n",count>1?"Ambiguous item name.":"No matching carried or equipped item.");return NULL;
  }
  return found;
}
static unsigned long long inventory_hash(struct char_data *ch)
{
  struct obj_data *o;int i;unsigned long long h=1469598103934665603ULL;
  for(o=ch->carrying;o;o=o->next_content)h=(h^(unsigned long long)GET_ID(o))*1099511628211ULL;
  for(i=0;i<NUM_WEARS;i++){o=GET_EQ(ch,i);h=(h^(o?(unsigned long long)GET_ID(o):0))*1099511628211ULL;}
  return h;
}
/* Excludes UUID/binding: first confirmation assigns these before its checkpoint. */
static unsigned long long item_fingerprint(struct obj_data *o)
{
  unsigned long long h=1469598103934665603ULL;int i;
#define MIX_FACT(v) h=(h^(unsigned long long)(v))*1099511628211ULL
  MIX_FACT(GET_OBJ_VNUM(o));MIX_FACT(GET_OBJ_LEVEL(o));MIX_FACT(GET_OBJ_TYPE(o));
  MIX_FACT(GET_OBJ_TIMER(o));MIX_FACT(GET_OBJ_SIZE(o));
  for(i=0;i<TW_ARRAY_MAX;i++){MIX_FACT(o->obj_flags.wear_flags[i]);}
  for(i=0;i<EF_ARRAY_MAX;i++){MIX_FACT(o->obj_flags.extra_flags[i]);}
#undef MIX_FACT
  return h;
}
static long smith(struct char_data *ch)
{
  struct char_data *mob;MYSQL_RES *r;MYSQL_ROW row;char sql[180];bool yes;
  if(IN_ROOM(ch)==NOWHERE)return 0;
  for(mob=world[IN_ROOM(ch)].people;mob;mob=mob->next_in_room) {
    if(!IS_NPC(mob)||mob->master||mob->legendary_owned_instance||mob->legendary_test_encounter||
       AFF_FLAGGED(mob,AFF_CHARM)||GET_POS(mob)<=POS_SLEEPING||GET_MOB_RNUM(mob)==NOBODY)continue;
    snprintf(sql,sizeof(sql),"SELECT enabled FROM legendary_smiths WHERE mob_vnum=%d",GET_MOB_VNUM(mob));
    r=estate_select("%s",sql);if(!r)return 0;row=mysql_fetch_row(r);yes=row&&row[0]&&!strcmp(row[0],"1");mysql_free_result(r);
    if(yes)return GET_ID(mob);
  }
  return 0;
}
static bool service(struct char_data *ch)
{
  if(GET_LEVEL(ch)!=30||GET_HIT(ch)<=0||!ch->desc||STATE(ch->desc)!=CON_PLAYING||FIGHTING(ch)||GET_POS(ch)<=POS_SLEEPING) {
    send_to_char(ch,"Infusion requires a conscious level 30 character out of combat.\r\n");return false;
  }
  if(!smith(ch)){send_to_char(ch,"Visit a registered dragonmetal smith for changes.\r\n");return false;}
  if(!legendary_feature_enabled(LEG_FEATURE_PURCHASES)){
    send_to_char(ch,"Legendary purchases and reshapes are currently disabled.\r\n");return false;
  }
  return true;
}
static bool lance(struct obj_data *obj)
{ struct legendary_registry_record r;return legendary_registry_load(GET_OBJ_VNUM(obj),true,&r)==LEG_LOAD_OK&&r.enabled; }
static bool record(struct char_data *ch,struct obj_data *obj,struct legendary_item_record *r)
{
  if(!*obj->legendary_uuid){memset(r,0,sizeof(*r));r->slot=legendary_equipment_slot(obj);r->prototype=GET_OBJ_VNUM(obj);r->owner=GET_IDNUM(ch);return true;}
  if(obj->legendary_quarantined||legendary_store_load(obj->legendary_uuid,r)!=LEG_LOAD_OK||
    r->owner!=GET_IDNUM(ch)||r->state!=LEG_RECORD_ACTIVE) {
    send_to_char(ch,"That legendary identity is unavailable, quarantined, or belongs to someone else.\r\n");return false;
  }
  return true;
}
static void free_pending(struct pending *p)
{ if(p->quote)legendary_transaction_discard(p->quote);if(p->exchange)legendary_exchange_discard(p->exchange);free(p); }
void legendary_infuse_cleanup(struct char_data *ch)
{
  struct pending **at=&pending,*p;
  while((p=*at)) {if(p->owner==GET_IDNUM(ch)){*at=p->next;free_pending(p);}else at=&p->next;}
}
static struct pending *get_pending(struct char_data *ch)
{ struct pending *p;for(p=pending;p;p=p->next)if(p->owner==GET_IDNUM(ch))return p;return NULL; }
static void expire_quotes(void)
{
  struct pending **at=&pending,*p;time_t now=time(NULL);
  while((p=*at)){if(p->expires<now){*at=p->next;free_pending(p);}else at=&p->next;}
}
static struct pending *new_pending(struct char_data *ch)
{
  struct pending *p=calloc(1,sizeof(*p));if(!p)return NULL;
  legendary_infuse_cleanup(ch);p->owner=GET_IDNUM(ch);p->smith=smith(ch);p->room=IN_ROOM(ch);
  p->inventory=inventory_hash(ch);p->expires=time(NULL)+60;p->next=pending;pending=p;return p;
}
static bool same_context(struct char_data *ch,struct pending *p)
{
  if(!(GET_LEVEL(ch)==30&&GET_HIT(ch)>0&&!FIGHTING(ch)&&GET_POS(ch)>POS_SLEEPING&&IN_ROOM(ch)==p->room&&
    smith(ch)==p->smith&&p->smith&&inventory_hash(ch)==p->inventory&&time(NULL)<=p->expires&&
    (!p->object||(item_fingerprint(p->object)==p->facts&&legendary_equipment_eligible(ch,p->object,p->item.slot,p->channel)))))return false;
  if(p->object&&!p->save_layout) {
    const struct legendary_channel_record *c=&p->item.channels[p->channel];
    const struct legendary_power_def *t=legendary_power(c->technique),*s=legendary_power(c->signature);
    if((c->foundation==LEG_F11&&!legendary_resource_class(ch))||
       (c->foundation==LEG_F08&&(!legendary_trained_skill(ch,c->subtype)))||
       (((t&&t->requires_dragonlance)||(s&&s->requires_dragonlance))&&!lance(p->object)))return false;
  }
  return true;
}
static bool reference_parse(const char *text,struct reference *out,bool major)
{
  char uuid[33],tail;int channel,kind=0,n;
  n=major?sscanf(text,"%32[0-9a-f]/%d/%d%c",uuid,&channel,&kind,&tail):sscanf(text,"%32[0-9a-f]/%d%c",uuid,&channel,&tail);
  if(n!=(major?3:2)||!legendary_uuid_valid(uuid)||channel<0||channel>1||(major&&(kind<1||kind>2)))return false;
  memcpy(out->uuid,uuid,33);out->channel=channel;out->kind=kind;return true;
}
static bool parse_refs(const char *text,struct reference *out,int *count,int max,bool major)
{
  char buffer[4096],*save,*s;*count=0;
  if(!text||!*text)return true;
  if(strlen(text)>=sizeof(buffer))return false;
  strcpy(buffer,text);
  for(s=strtok_r(buffer," ",&save);s;s=strtok_r(NULL," ",&save)) {
    if(*count==max||!reference_parse(s,&out[*count],major))return false;
    (*count)++;
  }
  return true;
}
static bool layout_load(struct char_data *ch,struct arrangement *a)
{
  MYSQL_RES *r;MYSQL_ROW row;char sql[200];bool ok;
  memset(a,0,sizeof(*a));snprintf(sql,sizeof(sql),"SELECT passive_priorities,major_selections FROM legendary_characters WHERE player_id=%ld",GET_IDNUM(ch));
  r=estate_select("%s",sql);if(!r)return false;row=mysql_fetch_row(r);
  ok=row&&parse_refs(row[0],a->priority,&a->priorities,REFS,false)&&parse_refs(row[1],a->major,&a->majors,3,true);
  mysql_free_result(r);return ok;
}
static void serialize(const struct reference *r,int count,bool major,char *out,size_t size)
{
  int i;size_t used=0;out[0]=0;
  for(i=0;i<count;i++)used+=snprintf(out+used,size-used,major?"%s%s/%d/%d":"%s%s/%d",i?" ":"",r[i].uuid,r[i].channel,r[i].kind);
}
static bool participant(struct char_data *ch,const char *id,void *context)
{
  struct pending *p=context;char priorities[4096],majors[160],sql[4600];(void)id;
  if(!same_context(ch,p))return false;
  if(!p->save_layout)return true;
  {
    struct legendary_item_record current;
    MYSQL_RES *locked=estate_select("SELECT uuid FROM legendary_items WHERE uuid='%s' FOR UPDATE",p->item.uuid);
    if(!locked)return false;
    mysql_free_result(locked);
    if(legendary_store_load(p->item.uuid,&current)!=LEG_LOAD_OK||current.owner!=GET_IDNUM(ch)||
       current.state!=LEG_RECORD_ACTIVE||current.version!=p->item.version)return false;
  }
  serialize(p->layout.priority,p->layout.priorities,false,priorities,sizeof(priorities));
  serialize(p->layout.major,p->layout.majors,true,majors,sizeof(majors));
  snprintf(sql,sizeof(sql),"UPDATE legendary_characters SET passive_priorities='%s',major_selections='%s' WHERE player_id=%ld",priorities,majors,GET_IDNUM(ch));
  return estate_query("%s",sql);
}
static void quote_output(struct char_data *ch,struct pending *p,const struct legendary_wallet *d)
{
  int i;const char *id=p->exchange?legendary_exchange_id(p->exchange):legendary_transaction_id(p->quote);
  send_to_char(ch,"Exact changes: infusion XP %"PRId64", gold %"PRId64", fragments %"PRId64", catalysts %"PRId64".\r\n",d->xp,d->gold,d->fragments,d->catalysts);
  for(i=0;i<6;i++)if(d->essences[i])send_to_char(ch,"  %s essence: %"PRId64"\r\n",legendary_essence_names[i],d->essences[i]);
  if(p->object)send_to_char(ch,"Channel: %s. The other channel will not advance.\r\n",channels[p->channel]);
  send_to_char(ch,"Valid for 60 seconds: infuse confirm %s (or infuse cancel).\r\n",id);
}
static void power_values(struct char_data *,int,int,int);
static bool quote_item(struct char_data *ch,struct obj_data *obj,struct legendary_item_record *r,int channel,
                       const struct legendary_cost *cost,bool matching,bool purchase,struct arrangement *layout)
{
  struct legendary_balance b;struct legendary_wallet d;int64_t payment[6];int i;
  struct legendary_transaction_plan plan;struct pending *p;
  if(!legendary_balance_load(ch,&b))return false;
  b.wallet.gold=GET_GOLD(ch);
  if(!legendary_can_afford(&b.wallet,cost,r?r->channels[channel].path:1,matching,payment)) {
    send_to_char(ch,"Insufficient infusion XP, gold, fragments, matching essences, or catalysts. No changes made.\r\n");return false;
  }
  memset(&d,0,sizeof(d));d.xp=-cost->xp;d.gold=-cost->gold;d.fragments=-cost->fragments;d.catalysts=-cost->catalysts;
  for(i=0;i<6;i++)d.essences[i]=-payment[i];
  p=new_pending(ch);if(!p)return false;p->object=obj;p->channel=channel;if(obj)p->facts=item_fingerprint(obj);
  if(r)p->item=*r;
  if(layout){p->save_layout=true;p->layout=*layout;}
  memset(&plan,0,sizeof(plan));plan.kind=purchase?LEG_OP_PURCHASE:LEG_OP_RESHAPE;
  p->delta=d;p->cost=*cost;p->matching=matching;p->purchase=purchase;
  plan.delta=d;plan.item=r&&!layout?&p->item:NULL;plan.channel=channel;plan.object=obj;plan.context=p;plan.participate=participant;
  p->quote=legendary_transaction_quote(ch,&plan);
  if(!p->quote){legendary_infuse_cleanup(ch);send_to_char(ch,"Unable to quote safely; no changes made.\r\n");return false;}
  if(r&&!layout) {
    const struct legendary_channel_record *c=&r->channels[channel];
    send_to_char(ch,"Proposed: %s %s, rank %s, %s path, %s family; F%02d subtype %d, T%02d, S%02d, modifier %s on %s.\r\n",
      obj->short_description,channels[channel],legendary_rank_names[c->rank],legendary_path(c->path)->name,
      legendary_family_names[c->family],c->foundation,c->subtype,c->technique?c->technique-100:0,
      c->signature?c->signature-200:0,legendary_modifier_names[c->modifier],
      c->modifier_target==1?"technique":c->modifier_target==2?"signature":"none");
    send_to_char(ch,"Item passive: P%02d (one selection for this whole item).\r\n",r->channels[0].item_passive);
    power_values(ch,c->technique,c->rank,c->modifier_target==1?c->modifier:LEG_MOD_NONE);
    power_values(ch,c->signature,c->rank,c->modifier_target==2?c->modifier:LEG_MOD_NONE);
  }
  quote_output(ch,p,&d);return true;
}
static void infuse_help(struct char_data *ch)
{
  send_to_char(ch,
    "Dragonmetal Awakening: infuse next <item> [primary|supporting] | inspect <item> [channel]\r\n"
    "infuse materials | paths <item> [channel] | choices <item> [channel] | loadout\r\n"
    "At a registered smith, out of combat, at level 30:\r\n"
    "infuse awaken <item> [primary] <path>\r\n"
    "infuse awaken <item> supporting <assault|guard|focus> <path>\r\n"
    "infuse advance <item> [channel]\r\n"
    "infuse select <item> <channel> foundation <Fxx> [subtype]\r\n"
    "infuse select <item> primary passive <Pxx> (one per item; grows with rank)\r\n"
    "infuse select <item> <channel> technique|signature <Txx|Sxx>\r\n"
    "infuse modify <item> <channel> technique|signature <Enduring|Ready|\"Distant aid\">\r\n"
    "infuse reshape <item> <channel> path|family <choice>\r\n"
    "infuse prioritize <item> <channel> <position>\r\n"
    "infuse attune|unattune <item> <channel> technique|signature\r\n"
    "infuse exchange essence <from> <to> <quantity> | exchange catalyst <quantity>\r\n"
    "infuse mix <draconic> <martial> <arcane> <spiritual> <primal> <shadow> (pending purchase payment)\r\n"
    "infuse confirm <quote-id> | cancel. Quote item names containing spaces.\r\n"
    "Binding is permanent. First choices are free, replacements cost materials/gold.\r\n"
    "infuse use <power-ID|quoted name> [target] | infuse track.\r\n"
    "Foundations, T01-T18 techniques and S01-S13 signatures are active.\r\n");
}
static void materials(struct char_data *ch)
{
  struct legendary_balance b;int i;time_t reset;struct tm tm;char date[64];
  if(!legendary_balance_load(ch,&b)){send_to_char(ch,"Legendary balances unavailable.\r\n");return;}
  reset=(time_t)((b.utc_day+1)*86400);gmtime_r(&reset,&tm);strftime(date,sizeof(date),"%Y-%m-%d %H:%M UTC",&tm);
  send_to_char(ch,"Infusion XP: %"PRId64"; gold: %d; fragments: %"PRId64"; catalysts: %"PRId64".\r\n",b.wallet.xp,GET_GOLD(ch),b.wallet.fragments,b.wallet.catalysts);
  for(i=0;i<6;i++)send_to_char(ch,"%s essences: %"PRId64"\r\n",legendary_essence_names[i],b.wallet.essences[i]);
  send_to_char(ch,"Today's remaining acquisitions: %d fragments, %d essences, %d catalysts. Reset %s.\r\n",
    legendary_rules.daily_fragments-b.fragments_today,legendary_rules.daily_essences-b.essences_today,
    legendary_rules.daily_catalysts-b.catalysts_today,date);
}
static int effective_slot(const struct legendary_item_record *r,int channel)
{ return channel?legendary_family_slot(r->channels[1].family):r->slot; }
static void power_values(struct char_data *ch,int power,int rank,int modifier)
{
  struct legendary_resolved_power resolved;struct legendary_values *v=&resolved.values;
  const struct legendary_power_def *p=legendary_power(power);
  if(!p||!legendary_resolve_power(power,rank,modifier,&resolved))return;
  send_to_char(ch,"  %s rank %s values:",p->code,legendary_rank_names[rank]);
#define VALUE(field,label) if(v->field)send_to_char(ch," %s=%d;",label,v->field)
  VALUE(attack,"attack");VALUE(damage,"damage");VALUE(check_bonus,"check");VALUE(ac,"AC");
  VALUE(percent,"percent");VALUE(pool,"pool");VALUE(heal,"healing");VALUE(temporary_hp,"temporary HP");
  VALUE(duration,"duration seconds");VALUE(window,"window seconds");VALUE(interval,"interval seconds");
  VALUE(ticks,"ticks");VALUE(tick_heal,"healing per tick");VALUE(charges,"charges");VALUE(targets,"targets");
  VALUE(resource_min,"minimum resource cost");VALUE(resource_discount,"resource discount");
  VALUE(refund_percent,"refund percent");VALUE(refund_cap,"refund cap");VALUE(fear_seconds,"fear seconds");
  VALUE(save_dc,"save DC");VALUE(control_seconds,"control seconds");VALUE(physical_info,"physical defense information");
#undef VALUE
  send_to_char(ch," cooldown=%ds; family cooldown=%ds.%s\r\n",resolved.cooldown,resolved.family_cooldown,
    resolved.target_ally_instead?" Targets an ally instead of self.":"");
}
static void next_step(struct char_data *ch,struct obj_data *obj,const struct legendary_item_record *r,int channel)
{
  const struct legendary_channel_record *c=&r->channels[channel];struct legendary_cost cost;
  const struct legendary_loadout *view;int i;bool attuned=false;
  const char *side=channels[channel];
  send_to_char(ch,"Next step for %s (%s):\r\n",obj->short_description,side);
  if(!c->present){
    cost=legendary_rank_costs[1];
    send_to_char(ch,"Choose a path with infuse paths <item> %s, then ask the registered smith to infuse awaken <item> %s %s<path>.\r\n",side,side,channel?"<assault|guard|focus> ":"");
    send_to_char(ch,"Awakening is permanent and cannot transfer or refund its investment.\r\n");
  } else if(!c->foundation){
    send_to_char(ch,"Choose your free foundation: infuse choices <item> %s, then infuse select <item> %s foundation <Fxx>. Foundations share 12 points across your equipment.\r\n",side,side);return;
  } else if(!r->channels[0].item_passive){
    send_to_char(ch,"Choose your free additional item passive: infuse choices <item> primary, then infuse select <item> primary passive <Pxx>. One selection per item grows with rank, separately from its foundation.\r\n");return;
  } else if(c->rank>=2&&!c->technique){
    send_to_char(ch,"Choose your free technique: infuse choices <item> %s, then infuse select <item> %s technique <Txx>.\r\n",side,side);return;
  } else if(c->technique){
    view=legendary_loadout_get(ch);
    for(i=0;i<view->major_count;i++)if(view->majors[i].channel==channel&&view->majors[i].kind==1&&!strcmp(view->majors[i].uuid,r->uuid))attuned=true;
    if(!attuned){send_to_char(ch,"At the smith: infuse attune <item> %s technique. You may attune three powers across all equipment.\r\n",side);return;}
    send_to_char(ch,"Equip the item, then infuse use %s [target]. Using it takes a standard action and six seconds of recovery. Inspect shows shared cooldowns.\r\n",legendary_power(c->technique)->code);
    if(c->rank>=3&&!c->signature){send_to_char(ch,"Your free signature is ready: infuse choices <item> %s, then infuse select <item> %s signature <Sxx>.\r\n",side,side);return;}
    if(c->rank>=4&&!c->modifier){send_to_char(ch,"Choose one free compatible modifier with infuse choices <item> %s, then infuse modify <item> %s technique|signature <modifier>.\r\n",side,side);return;}
    if(c->rank==5){send_to_char(ch,"This channel is complete. A two-handed supporting channel has its own rank and costs; it does not add a fourth attuned power.\r\n");return;}
    cost=legendary_rank_costs[c->rank+1];
  } else cost=legendary_rank_costs[c->rank+1];
  send_to_char(ch,"Next cost: %"PRId64" infusion XP, %"PRId64" gold, %"PRId64" fragments, %"PRId64" essences, %"PRId64" catalysts. Check infuse materials, then request the smith's quote.\r\n",cost.xp,cost.gold,cost.fragments,cost.essences,cost.catalysts);
  if(c->present)send_to_char(ch,"Request infuse advance <item> %s. Only this channel advances.\r\n",side);
  send_to_char(ch,"Nothing is spent until you confirm the exact quote.\r\n");
}
static void inspect(struct char_data *ch,struct obj_data *obj,struct legendary_item_record *r,int channel)
{
  const struct legendary_channel_record *c=&r->channels[channel];const struct legendary_foundation_def *f;
  const struct legendary_power_def *t,*s;struct legendary_cost cost;
  send_to_char(ch,"%s: %s channel%s\r\n",obj->short_description,channels[channel],*r->uuid?", permanently bound":" (ordinary item)");
  if(!c->present){next_step(ch,obj,r,channel);return;}
  {
    const struct legendary_item_passive_def *ip=legendary_item_passive(r->channels[0].item_passive);
    if(ip)send_to_char(ch,"Item passive: %s %s; track value %d before caps/training. Outfit ceiling: %d. %s.\r\n",ip->code,ip->name,legendary_item_passive_amount(ip->id,c->rank),ip->ceiling,ip->description);
    if(ip)send_to_char(ch,"Current equipped outfit total for %s: %d (eligibility, capability and ceilings apply).\r\n",ip->code,legendary_item_passive_value(ch,ip->id));
    else send_to_char(ch,"Additional item passive: unselected. Free first choice: infuse select <item> primary passive <Pxx>.\r\n");
  }
  f=legendary_foundation(c->foundation);t=legendary_power(c->technique);s=legendary_power(c->signature);
  send_to_char(ch,"Rank %s; path %s; family %s; UUID %s.\r\n",legendary_rank_names[c->rank],legendary_path(c->path)->name,legendary_family_names[c->family],r->uuid);
  send_to_char(ch,"Foundation: %s%s%s (subtype %d); technique: %s; signature: %s.\r\n",f?f->code:"unselected",f?" ":"",f?f->name:"",c->subtype,t?t->name:"unselected",s?s->name:"unselected");
  send_to_char(ch,"Modifier: %s on %s. First choices remaining: %s%s%s%s\r\n",legendary_modifier_names[c->modifier],c->modifier_target==1?"technique":c->modifier_target==2?"signature":"none",
    !(c->first_choices&1)?"foundation ":"",!(c->first_choices&2)?"technique ":"",!(c->first_choices&4)?"signature ":"",!(c->first_choices&8)?"modifier":"");
  send_to_char(ch,"Invested: %"PRId64" XP, %"PRId64" gold, %"PRId64" fragments, %"PRId64" essences, %"PRId64" catalysts.\r\n",c->paid.xp,c->paid.gold,c->paid.fragments,c->paid.essences,c->paid.catalysts);
  {
    const struct legendary_loadout *view=legendary_loadout_get(ch);
    int i,reason=LEG_NOT_EQUIPPED;
    for(i=0;i<view->count;i++)if(view->channels[i].channel==channel&&!strcmp(view->channels[i].uuid,r->uuid)){
      reason=view->channels[i].reason;break;
    }
    send_to_char(ch,"Equipped channel: %s. Foundations, techniques and signatures are implemented.\r\n",legendary_dormant_reason(reason));
  }
  power_values(ch,c->technique,c->rank,c->modifier_target==1?c->modifier:0);
  power_values(ch,c->signature,c->rank,c->modifier_target==2?c->modifier:0);
  if(c->rank<5&&legendary_cost_between(c->rank,c->rank+1,&cost))send_to_char(ch,"Next rank: %"PRId64" XP, %"PRId64" gold, %"PRId64" fragments, %"PRId64" essences (half matching), %"PRId64" catalysts.\r\n",cost.xp,cost.gold,cost.fragments,cost.essences,cost.catalysts);
}
static void choices(struct char_data *ch,struct obj_data *obj,struct legendary_item_record *r,int channel)
{
  int i,slot=effective_slot(r,channel);struct legendary_channel_record *c=&r->channels[channel];bool dragon=lance(obj);
  if(!c->present){send_to_char(ch,"Awaken this channel first; use infuse paths <item>.\r\n");return;}
  send_to_char(ch,"Additional item passive (one per item, separate from foundations; choose on primary):\r\n");
  for(i=1;i<=LEGENDARY_ITEM_PASSIVE_COUNT;i++)if(legendary_item_passive_available(r->slot,i)){
    const struct legendary_item_passive_def *ip=legendary_item_passive(i);
    send_to_char(ch,"%s %s: %s; I/II/III/IV/V: %d/%d/%d/%d/%d; outfit cap %d.\r\n",ip->code,ip->name,ip->description,
      legendary_item_passive_amount(i,1),legendary_item_passive_amount(i,2),legendary_item_passive_amount(i,3),legendary_item_passive_amount(i,4),legendary_item_passive_amount(i,5),ip->ceiling);
  }
  for(i=0;i<16;i++){const struct legendary_foundation_def *f=&legendary_foundations[i];
    send_to_char(ch,"%s %s (%d points): %s%s\r\n",f->code,f->name,f->points,f->description,f->id==11&&!legendary_resource_class(ch)?" [Unavailable: no spell/psionic class]":"");}
  send_to_char(ch,"F07 subtypes: str dex con int wis cha. F12: acid cold electricity fire sonic.\r\nF08 trained skill subtypes:");
  for(i=1;i<=END_GENERAL_ABILITIES;i++)if(legendary_trained_skill(ch,i))
    send_to_char(ch," %d=%s;",i,ability_names[i]);
  send_to_char(ch,"\r\n");
  for(i=0;i<31;i++){const struct legendary_power_def *p=&legendary_powers[i];const char *reason="available";
    if(p->requires_dragonlance&&!dragon)reason="unavailable: registered dragonlance required";
    else if(!legendary_power_available(slot,c->path,p->id,true))reason="unavailable: slot/path";
    else if(c->rank<p->minimum_rank)reason="unavailable: rank too low";
    send_to_char(ch,"%s %s [%s]: %s (cooldown %ds)\r\n",p->code,p->name,reason,p->description,p->cooldown);
  }
  send_to_char(ch,"Rank IV modifiers for selected powers:\r\n");
  for(i=1;i<=2;i++) {
    int m,power=i==1?c->technique:c->signature;
    for(m=1;m<=3;m++) {
      struct legendary_resolved_power resolved;
      const char *why=!power?"unavailable: select a power first":c->rank<4?"unavailable: requires rank IV":
        legendary_resolve_power(power,c->rank,m,&resolved)?"available":"unavailable: incompatible with this power/rank";
      send_to_char(ch,"%s %s: %s\r\n",i==1?"technique":"signature",legendary_modifier_names[m],why);
    }
  }
}
static bool same_ref(const struct reference *a,const struct reference *b,bool major)
{ return !strcmp(a->uuid,b->uuid)&&a->channel==b->channel&&(!major||a->kind==b->kind); }
static void loadout(struct char_data *ch)
{
  struct arrangement a;struct legendary_balance b;int i;struct legendary_item_record r;
  if(!legendary_balance_load(ch,&b)||!layout_load(ch,&a)){send_to_char(ch,"Saved loadout unavailable.\r\n");return;}
  send_to_char(ch,"Saved priorities (passive budget 12); up to 3 attuned major powers.\r\n");
  for(i=0;i<a.priorities;i++)send_to_char(ch,"%d: %s %s\r\n",i+1,a.priority[i].uuid,channels[a.priority[i].channel]);
  for(i=0;i<a.majors;i++) {
    const struct legendary_power_def *p=NULL;struct reference *ref=&a.major[i];
    if(legendary_store_load(ref->uuid,&r)==LEG_LOAD_OK&&r.owner==GET_IDNUM(ch)&&r.state==LEG_RECORD_ACTIVE)
      p=legendary_power(ref->kind==1?r.channels[ref->channel].technique:r.channels[ref->channel].signature);
    send_to_char(ch,"Major %d: %s %s %s (%s)\r\n",i+1,ref->uuid,channels[ref->channel],ref->kind==1?"technique":"signature",p?p->name:"unavailable");
  }
  {
    MYSQL_RES *result=estate_select("SELECT cooldown_key,expires_utc FROM legendary_cooldowns WHERE player_id=%ld AND expires_utc>%lld ORDER BY cooldown_key",GET_IDNUM(ch),(long long)time(NULL));
    MYSQL_ROW row;
    if(result){while((row=mysql_fetch_row(result)))send_to_char(ch,"Cooldown key %s: %lld seconds remaining.\r\n",row[0],atoll(row[1])-(long long)time(NULL));mysql_free_result(result);}
  }
  {
    const struct legendary_loadout *view;
    static const char *const passive_reasons[]={"active","ineligible capability/equipment","unselected/invalid","category/subtype ceiling","12-point budget","combat allocation locked"};
    legendary_loadout_refresh(ch);view=legendary_loadout_get(ch);
    send_to_char(ch,"Equipped loadout: %d/12 passive points; %s.\r\n",view->points,view->ready?"configuration ready":"configuration unavailable");
    for(i=0;i<view->count;i++) {
      const struct legendary_equipped_channel *e=&view->channels[i];
      send_to_char(ch,"%s %s: %s; F%02d %s (%d points, value %d).\r\n",e->uuid,channels[e->channel],
        legendary_dormant_reason(e->reason),e->foundation,
        e->passive.reason>=0&&e->passive.reason<6?passive_reasons[e->passive.reason]:"invalid",e->passive.points,e->passive.value);
      if(e->passive.reason==LEG_PASSIVE_ACTIVE) {
        int actual=legendary_foundation_channel_actual(ch,e->uuid,e->channel);
        send_to_char(ch,"  Foundation contribution: %d%s.\r\n",MIN(actual,e->passive.value),
          actual<e->passive.value?" (superseded or limited by ordinary bonus/stat caps)":"");
      }
    }
    for(i=0;i<view->major_count;i++) {
      const struct legendary_equipped_major *m=&view->majors[i];
      const struct legendary_power_def *power=legendary_power(m->power);
      send_to_char(ch,"Attuned %s %s %s: %s (%s).\r\n",m->uuid,channels[m->channel],m->kind==1?"technique":"signature",
        power?power->name:"unavailable",legendary_dormant_reason(m->reason));
    }
  }
  send_to_char(ch,"Shared focus temporary HP remaining: %d.\r\n",legendary_temporary_remaining(ch));
  send_to_char(ch,"Equipped foundations, T01-T18 techniques and S01-S13 signatures apply to gameplay.\r\n");
}
static int subtype(struct char_data *ch,int foundation,const char *s)
{
  int i,id;
  if(foundation==7)return s?named(s,abilities,6):-1;
  if(foundation==12)return s?named(s,elements,5):-1;
  if(foundation==8){
    if(!s)return -1;
    id=integer(s);
    for(i=1;i<=END_GENERAL_ABILITIES;i++)if(legendary_trained_skill(ch,i)&&
      (id==i||(ability_names[i]&&!strcasecmp(s,ability_names[i]))))return i;
    return -1;
  }
  return s?-1:0;
}
static void invalidate_choices(struct legendary_item_record *r,int channel,bool dragon)
{
  struct legendary_channel_record *c=&r->channels[channel];int slot=effective_slot(r,channel);struct legendary_resolved_power result;
  if(c->technique&&!legendary_power_available(slot,c->path,c->technique,dragon)){c->technique=0;c->first_choices&=~2U;}
  if(c->signature&&!legendary_power_available(slot,c->path,c->signature,dragon)){c->signature=0;c->first_choices&=~4U;}
  if(c->modifier&&!legendary_resolve_power(c->modifier_target==1?c->technique:c->signature,c->rank,c->modifier,&result)) {
    c->modifier=c->modifier_target=0;c->first_choices&=~8U;
  }
}
void legendary_infuse(struct char_data *ch,const char *argument)
{
  char w[WORDS][WORDLEN];int n,i,channel=0,index=2,value,kind,slot;bool purchase=false,matching=false;
  struct pending *p;struct obj_data *obj;struct legendary_item_record r;struct legendary_channel_record *c;
  struct legendary_cost cost={0};struct legendary_balance balance;
  expire_quotes();
  if(IS_NPC(ch)){send_to_char(ch,"Players only.\r\n");return;}
  n=words(argument,w);if(n<0){send_to_char(ch,"Malformed, overlong, or unclosed quoted input.\r\n");return;}
  if(!n){send_to_char(ch,"Dragonmetal Awakening: start with infuse next <item>.\r\nAt level 30, a registered smith can permanently awaken eligible level 20-30 equipment.\r\nUse infuse materials for your balances; infuse help for all commands.\r\nFoundations share 12 points; choose at most three active powers across all gear.\r\n");return;}
  if(n==1&&!strcasecmp(w[0],"help")){infuse_help(ch);return;}
  if(!strcasecmp(w[0],"cancel")&&n==1){legendary_infuse_cleanup(ch);send_to_char(ch,"Quote cancelled; no charges.\r\n");return;}
  if(!legendary_earnings_available()){send_to_char(ch,"Legendary service unavailable; required schema migration is not installed.\r\n");return;}
  if(!strcasecmp(w[0],"materials")&&n==1){materials(ch);return;}
  if(!strcasecmp(w[0],"loadout")&&n==1){loadout(ch);return;}
  if(!strcasecmp(w[0],"track")&&n==1){legendary_track(ch);return;}
  if(!strcasecmp(w[0],"confirm")&&n==2) {
    bool ok;const char *id;p=get_pending(ch);
    if(!p){send_to_char(ch,"No pending quote (it may have expired).\r\n");return;}
    id=p->exchange?legendary_exchange_id(p->exchange):legendary_transaction_id(p->quote);
    if(strcmp(w[1],id)){send_to_char(ch,"That quote ID does not match.\r\n");return;}
    if(!service(ch)||!same_context(ch,p)){legendary_infuse_cleanup(ch);send_to_char(ch,"Quote invalidated by changed conditions; request a new quote.\r\n");return;}
    ok=p->exchange?legendary_exchange_confirm(ch,p->exchange):legendary_transaction_confirm(ch,p->quote);
    if(ok){legendary_loadout_refresh(ch);}
    legendary_infuse_cleanup(ch);send_to_char(ch,ok?"Infusion confirmed and saved.\r\n":"Confirmation unavailable or stale; no success recorded. Any durable operation will recover before further spending.\r\n");return;
  }
  if(!strcasecmp(w[0],"mix")) {
    struct legendary_transaction_plan plan;
    struct legendary_quote *q;
    int64_t total=0,pay[6];
    p=get_pending(ch);
    if(n!=7||!p||!p->quote||p->save_layout||!p->object)goto syntax;
    if(!service(ch)||!same_context(ch,p))return;
    for(i=0;i<6;i++){value=integer(w[i+1]);if(value<0)goto syntax;pay[i]=value;total+=pay[i];}
    if(total!=p->cost.essences||pay[legendary_path(p->item.channels[p->channel].path)->essence] <
       (p->matching?p->cost.essences:(p->cost.essences+1)/2)) {
      send_to_char(ch,"Payment must equal the quoted essence total and meet its matching requirement.\r\n");return;
    }
    if(!legendary_balance_load(ch,&balance))return;
    for(i=0;i<6;i++)if(pay[i]>balance.wallet.essences[i]){send_to_char(ch,"Insufficient essences for that mix.\r\n");return;}
    memset(&plan,0,sizeof(plan));plan.kind=p->purchase?LEG_OP_PURCHASE:LEG_OP_RESHAPE;plan.item=&p->item;
    plan.channel=p->channel;plan.object=p->object;plan.delta=p->delta;
    for(i=0;i<6;i++)plan.delta.essences[i]=-pay[i];
    plan.participate=participant;plan.context=p;
    q=legendary_transaction_quote(ch,&plan);
    if(!q){send_to_char(ch,"Cannot replace this quote; request a new one.\r\n");return;}
    legendary_transaction_discard(p->quote);p->quote=q;p->delta=plan.delta;p->expires=time(NULL)+60;
    quote_output(ch,p,&p->delta);return;
  }
  if(!strcasecmp(w[0],"exchange")) {
    int from=0,to=-1,quantity;
    if(n==3&&!strcasecmp(w[1],"catalyst"))quantity=integer(w[2]);
    else if(n==5&&!strcasecmp(w[1],"essence")){from=named(w[2],legendary_essence_names,6);to=named(w[3],legendary_essence_names,6);quantity=integer(w[4]);if(from<0||to<0)goto syntax;}
    else goto syntax;
    if(quantity<1)goto syntax;
    if(!service(ch))return;
    p=new_pending(ch);if(!p)return;
    p->exchange=legendary_exchange_quote(ch,from,to,quantity);
    if(!p->exchange){legendary_infuse_cleanup(ch);send_to_char(ch,"Exchange unavailable: check balances and remaining daily allowance.\r\n");return;}
    quote_output(ch,p,legendary_exchange_delta(p->exchange));return;
  }
  if(!strcasecmp(w[0],"use")){if(n<2||n>3)goto syntax;legendary_use(ch,w[1],n==3?w[2]:NULL);return;}
  if(n<2)goto syntax;
  obj=find_item(ch,w[1]);if(!obj)return;if(!record(ch,obj,&r))return;
  if(n>2&&(value=named(w[2],channels,2))>=0){channel=value;index=3;}
  c=&r.channels[channel];
  if(!strcasecmp(w[0],"next")&&n==index){next_step(ch,obj,&r,channel);return;}
  if(!strcasecmp(w[0],"inspect")&&n==index){inspect(ch,obj,&r,channel);return;}
  if(!strcasecmp(w[0],"paths")&&n==index){
    for(i=0;i<12;i++)send_to_char(ch,"%s (%s essence)\r\n",legendary_paths[i].name,legendary_essence_names[legendary_paths[i].essence]);
    send_to_char(ch,"Supporting channels require an actually wielded two-handed weapon and assault, guard, or focus family.\r\n");return;
  }
  if(!strcasecmp(w[0],"choices")&&n==index){choices(ch,obj,&r,channel);return;}
  if(!service(ch))return;
  if(!legendary_equipment_eligible(ch,obj,r.slot,channel)){send_to_char(ch,"Ineligible item: permanent level 20-30 equipment, valid ordinary wear rules, and ownership required. Supporting requires an actually wielded two-handed weapon with no occupied offhand.\r\n");return;}
  if(!strcasecmp(w[0],"awaken")) {
    if(c->present){send_to_char(ch,"That channel is already awakened.\r\n");return;}
    if(channel){if(!r.channels[0].present||n!=index+2)goto syntax;value=named(w[index++],legendary_family_names,4);if(value<1)goto syntax;c->family=value;}
    if(n!=index+1||!(value=path_id(w[index])))goto syntax;
    c->present=true;c->rank=1;c->path=value;cost=legendary_rank_costs[1];purchase=true;
    send_to_char(ch,"WARNING: Awakening permanently binds this item to you. It cannot transfer or refund its awakening.\r\n");
  } else if(!c->present){send_to_char(ch,"Awaken this channel first.\r\n");return;}
  else if(!strcasecmp(w[0],"advance")&&n==index){
    if(c->rank==5){send_to_char(ch,"Already rank V.\r\n");return;}
    cost=legendary_rank_costs[++c->rank];purchase=true;
  } else if(!strcasecmp(w[0],"select")) {
    if(index!=3||n<index+2||n>index+3)goto syntax;
    kind=!strcasecmp(w[index],"passive")?4:!strcasecmp(w[index],"foundation")?0:!strcasecmp(w[index],"technique")?1:!strcasecmp(w[index],"signature")?2:-1;
    if(kind==4){
      const char *code=w[index+1];
      if(channel||n!=index+2||strlen(code)!=3||(code[0]!='P'&&code[0]!='p')||!isdigit((unsigned char)code[1])||!isdigit((unsigned char)code[2]))goto syntax;
      value=(code[1]-'0')*10+code[2]-'0';
      if(!legendary_item_passive_available(r.slot,value)){send_to_char(ch,"That passive is unavailable for this item's wear slot. See infuse choices.\r\n");return;}
      if(c->item_passive==value){send_to_char(ch,"Already selected.\r\n");return;}
      if(c->item_passive)cost=legendary_reshape_costs[LEG_RESHAPE_FOUNDATION];
      c->item_passive=value;
    } else {
    value=legendary_parse_code(w[index+1]);if(kind<0)goto syntax;
    if(kind==0) {
      int sub;if(!legendary_foundation(value))goto syntax;
      sub=subtype(ch,value,n==index+3?w[index+2]:NULL);
      if(sub<0||(value==11&&!legendary_resource_class(ch))){send_to_char(ch,"Invalid or untrained subtype, or no spell/psionic resource class. See infuse choices.\r\n");return;}
      if(c->foundation==value&&c->subtype==sub){send_to_char(ch,"Already selected.\r\n");return;}
      c->foundation=value;c->subtype=sub;
    } else {
      const struct legendary_power_def *power=legendary_power(value);slot=effective_slot(&r,channel);
      if(n!=index+2||!power||(kind==1?(value<101||value>118):(value<201||value>213)))goto syntax;
      if(c->rank<power->minimum_rank||!legendary_power_available(slot,c->path,value,lance(obj))){send_to_char(ch,"Unavailable: rank, slot/path, or registered dragonlance requirement. See infuse choices.\r\n");return;}
      if((kind==1?c->technique:c->signature)==value){send_to_char(ch,"Already selected.\r\n");return;}
      if(kind==1)c->technique=value;else c->signature=value;
      invalidate_choices(&r,channel,lance(obj));
    }
    if(c->first_choices&(1U<<kind))cost=legendary_reshape_costs[kind];
    c->first_choices|=1U<<kind;
    }
  } else if(!strcasecmp(w[0],"modify")) {
    struct legendary_resolved_power result;
    if(index!=3||n!=index+2)goto syntax;
    kind=!strcasecmp(w[index],"technique")?1:!strcasecmp(w[index],"signature")?2:0;
    value=named(w[index+1],legendary_modifier_names,4);
    if(!kind||value<1||!legendary_resolve_power(kind==1?c->technique:c->signature,c->rank,value,&result)) {
      send_to_char(ch,"Modifier unavailable: requires rank IV and compatible selected power (Enduring T08 requires V).\r\n");return;}
    if(c->modifier==value&&c->modifier_target==kind){send_to_char(ch,"Already selected.\r\n");return;}
    if(c->first_choices&8)cost=legendary_reshape_costs[3];
    c->modifier=value;c->modifier_target=kind;c->first_choices|=8;
  } else if(!strcasecmp(w[0],"reshape")) {
    if(index!=3||n!=index+2)goto syntax;
    if(!strcasecmp(w[index],"path")){value=path_id(w[index+1]);if(!value)goto syntax;if(c->path==value)goto unchanged;c->path=value;}
    else if(channel&&!strcasecmp(w[index],"family")){value=named(w[index+1],legendary_family_names,4);if(value<1)goto syntax;if(c->family==value)goto unchanged;c->family=value;}
    else goto syntax;
    invalidate_choices(&r,channel,lance(obj));cost=legendary_reshape_costs[4];matching=true;
    send_to_char(ch,"After reshape: foundation F%02d, technique T%02d, signature S%02d, modifier %s. Invalidated choices may be replaced free.\r\n",c->foundation,c->technique?c->technique-100:0,c->signature?c->signature-200:0,legendary_modifier_names[c->modifier]);
  } else if(!strcasecmp(w[0],"prioritize")||!strcasecmp(w[0],"attune")||!strcasecmp(w[0],"unattune")) {
    struct arrangement a;struct reference ref;bool priority=!strcasecmp(w[0],"prioritize"),remove=!strcasecmp(w[0],"unattune");
    if(index!=3||n!=index+1)goto syntax;
    if(!legendary_balance_load(ch,&balance)||!layout_load(ch,&a)){send_to_char(ch,"Saved arrangement unavailable.\r\n");return;}
    memset(&ref,0,sizeof(ref));memcpy(ref.uuid,r.uuid,33);ref.channel=channel;
    if(priority) {
      value=integer(w[index]);if(value<1||value>a.priorities+1||!c->foundation){send_to_char(ch,"Choose an existing foundation and a priority position from 1 through the end of the list.\r\n");return;}
      for(i=0;i<a.priorities;i++)if(same_ref(&ref,&a.priority[i],false))break;
      if(i<a.priorities){memmove(&a.priority[i],&a.priority[i+1],(a.priorities-i-1)*sizeof(ref));a.priorities--;}
      if(value>a.priorities+1)value=a.priorities+1;
      if(a.priorities==REFS){send_to_char(ch,"Saved priorities are full.\r\n");return;}
      memmove(&a.priority[value],&a.priority[value-1],(a.priorities-value+1)*sizeof(ref));a.priority[value-1]=ref;a.priorities++;
    } else {
      ref.kind=!strcasecmp(w[index],"technique")?1:!strcasecmp(w[index],"signature")?2:0;
      if(!ref.kind)goto syntax;
      if(!remove&&!(ref.kind==1?c->technique:c->signature)){send_to_char(ch,"Select that power first.\r\n");return;}
      for(i=0;i<a.majors;i++)if(same_ref(&ref,&a.major[i],true))break;
      if(remove){if(i==a.majors)goto unchanged;memmove(&a.major[i],&a.major[i+1],(a.majors-i-1)*sizeof(ref));a.majors--;}
      else {if(i<a.majors)goto unchanged;if(a.majors==3){send_to_char(ch,"Three major powers already attuned. Unattune one first.\r\n");return;}a.major[a.majors++]=ref;}
    }
    quote_item(ch,obj,&r,channel,&cost,false,false,&a);return;
  } else goto syntax;
  /* Track all paid costs, including reshapes, with checked bounded arithmetic. */
  if(!legendary_add(c->paid.xp,cost.xp,&c->paid.xp)||!legendary_add(c->paid.gold,cost.gold,&c->paid.gold)||
     !legendary_add(c->paid.fragments,cost.fragments,&c->paid.fragments)||!legendary_add(c->paid.essences,cost.essences,&c->paid.essences)||
     !legendary_add(c->paid.catalysts,cost.catalysts,&c->paid.catalysts)){send_to_char(ch,"Investment limit reached.\r\n");return;}
  quote_item(ch,obj,&r,channel,&cost,matching,purchase,NULL);return;
unchanged:send_to_char(ch,"Already configured that way.\r\n");return;
syntax:send_to_char(ch,"Invalid syntax. Type infuse help; quote multiword names and use explicit channel names for selections.\r\n");
}
ACMD(do_infuse)
{ legendary_infuse(ch,argument); }
