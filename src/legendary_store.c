/* Explicitly migrated storage only. Economic transactions belong to Phase 4. */
#include "conf.h"
#include "sysdep.h"
#include "legendary_equipment.h"
#include "structs.h"
#include "utils.h"
#include "estate.h"
#include "legendary_store.h"
#include "legendary_identity.h"
#include <errno.h>
#include <limits.h>

static unsigned long long dragonlance_registry_epoch;
static bool feature_values[LEG_FEATURE_COUNT];
static time_t feature_checked_until;
unsigned long long legendary_dragonlance_registry_epoch(void) { return dragonlance_registry_epoch; }
static const char *const states[] = {"pending", "active", "deleted"};
static bool number(const char *s, int64_t *out)
{
  unsigned long long n;
  const char *p;
  if (!s || !*s) return false;
  for (p=s; *p; p++) if (*p<'0' || *p>'9') return false;
  errno=0; n=strtoull(s,NULL,10);
  if (errno || n>INT64_MAX) return false;
  *out=(int64_t)n; return true;
}

bool legendary_schema_ready(void)
{
  MYSQL_RES *res;
  MYSQL_ROW row;
  bool ok;
  int64_t version;
  res=estate_select("SELECT version FROM legendary_schema WHERE singleton=1");
  if (!res) return false;
  row=mysql_fetch_row(res);
  ok=row && number(row[0],&version) && version==LEGENDARY_SCHEMA_VERSION && mysql_num_rows(res)==1;
  mysql_free_result(res);
  if (!ok) return false;
  res=estate_select("SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() "
    "AND table_name IN ('legendary_schema','legendary_characters','legendary_items',"
    "'legendary_channels','legendary_cooldowns','legendary_budgets','legendary_boss_claims',"
    "'legendary_quest_xp_claims','legendary_operations','legendary_dragonlances','legendary_awards','legendary_activations','legendary_features') "
    "AND engine='InnoDB'");
  if (!res) return false;
  row=mysql_fetch_row(res); ok=row && row[0] && !strcmp(row[0],"13");
  mysql_free_result(res); return ok;
}

bool legendary_features_refresh(void)
{
  MYSQL_RES *res; MYSQL_ROW row; bool values[LEG_FEATURE_COUNT]={false};
  int seen=0, feature; int64_t enabled;
#if defined(LEGENDARY_PERSISTENCE_TEST) || defined(LEGENDARY_TRANSACTION_TEST) || \
    defined(LEGENDARY_EARNINGS_TEST) || defined(LEGENDARY_EFFECTS_TEST)
  for(feature=0;feature<LEG_FEATURE_COUNT;feature++) feature_values[feature]=true;
  feature_checked_until=time(NULL)+1;
  return true;
#endif
  if(!legendary_schema_ready()) return false;
  res=estate_select("SELECT feature_name,enabled FROM legendary_features");
  if(!res) return false;
  while((row=mysql_fetch_row(res))) {
    if(!row[0] || !number(row[1],&enabled) || (enabled!=0 && enabled!=1)) { mysql_free_result(res); return false; }
    if(!strcmp(row[0],"earnings")) feature=LEG_FEATURE_EARNINGS;
    else if(!strcmp(row[0],"purchases")) feature=LEG_FEATURE_PURCHASES;
    else if(!strcmp(row[0],"major_effects")) feature=LEG_FEATURE_MAJOR_EFFECTS;
    else { mysql_free_result(res); return false; }
    if(values[feature]) { mysql_free_result(res); return false; }
    values[feature]=true; feature_values[feature]=enabled; seen++;
  }
  mysql_free_result(res);
  if(seen!=LEG_FEATURE_COUNT) return false;
  feature_checked_until=time(NULL)+5;
  return true;
}

bool legendary_feature_enabled(enum legendary_feature feature)
{
  time_t now=time(NULL);
  if(feature<0 || feature>=LEG_FEATURE_COUNT) return false;
  if(now>=feature_checked_until && !legendary_features_refresh()) return false;
  return feature_values[feature];
}

bool legendary_record_valid(const struct legendary_item_record *item)
{
  int i, slot, power;
  const struct legendary_channel_record *c;
  struct legendary_resolved_power resolved;
  if (!item || !legendary_uuid_valid(item->uuid) || item->owner<=0 || item->version<0 ||
      item->prototype<0 || !legendary_slot(item->slot) || item->state<0 || item->state>2 ||
      (item->state==LEG_RECORD_ACTIVE && !item->channels[0].present)) return false;
  for (i=0;i<2;i++)
  {
    c=&item->channels[i];
    if (!c->present) continue;
    if (c->rank<1 || c->rank>5 || !legendary_path(c->path) || c->version<0 ||
        c->first_choices>15 || c->item_passive<0 ||
        (c->item_passive && (i || !legendary_item_passive_available(item->slot,c->item_passive))) || c->subtype<0 || c->paid.xp<0 || c->paid.gold<0 ||
        c->paid.fragments<0 || c->paid.essences<0 || c->paid.catalysts<0) return false;
    slot=item->slot;
    if (i==0 && c->family!=LEG_FAMILY_NONE) return false;
    if (i==1)
    {
      if (item->slot!=LEG_SLOT_WEAPON || !item->channels[0].present) return false;
      slot=legendary_family_slot(c->family);
      if (slot<0) return false;
    }
    if (c->foundation && !legendary_foundation(c->foundation)) return false;
    if ((c->foundation==LEG_F07 && c->subtype>5) ||
        (c->foundation==LEG_F08 && !c->subtype) ||
        (c->foundation==LEG_F12 && c->subtype>4)) return false;
    if (c->foundation!=LEG_F07 && c->foundation!=LEG_F08 && c->foundation!=LEG_F12 && c->subtype) return false;
    if (c->technique && (c->rank<2 || c->technique<LEG_T01 || c->technique>LEG_T18 ||
        !legendary_power_available(slot,c->path,c->technique,true))) return false;
    if (c->signature && (c->rank<3 || c->signature<LEG_S01 || c->signature>LEG_S13 ||
        !legendary_power_available(slot,c->path,c->signature,true))) return false;
    if (!c->modifier) { if (c->modifier_target) return false; }
    else
    {
      if (c->rank<4 || c->modifier_target<1 || c->modifier_target>2) return false;
      power=c->modifier_target==1?c->technique:c->signature;
      if (!legendary_resolve_power(power,c->rank,c->modifier,&resolved)) return false;
    }
  }
  return true;
}

int legendary_store_load(const char *uuid, struct legendary_item_record *out)
{
  struct legendary_item_record item;
  struct legendary_channel_record *c;
  MYSQL_RES *res;
  MYSQL_ROW row;
  int64_t n[19];
  int i,id;
  if (!out || !legendary_uuid_valid(uuid) || !legendary_schema_ready()) return LEG_LOAD_ERROR;
  memset(&item,0,sizeof(item)); memcpy(item.uuid,uuid,33);
  res=estate_select("SELECT owner_id,prototype_vnum,slot_id,state,version FROM legendary_items WHERE uuid='%s'",uuid);
  if (!res) return LEG_LOAD_ERROR;
  row=mysql_fetch_row(res);
  if (!row) {mysql_free_result(res);return LEG_LOAD_MISSING;}
  if (!number(row[0],&item.owner) || !number(row[1],&n[0]) || n[0]>INT_MAX ||
      !number(row[2],&n[1]) || n[1]>=10 || !number(row[4],&item.version))
  {mysql_free_result(res);return LEG_LOAD_ERROR;}
  item.prototype=(int)n[0]; item.slot=(int)n[1];
  for (i=0;i<3;i++) if (row[3] && !strcmp(row[3],states[i])) break;
  item.state=i; mysql_free_result(res);
  res=estate_select("SELECT channel_id,rank_id,path_id,family_id,foundation_id,subtype_id,technique_id,"
    "signature_id,modifier_id,modifier_target,first_choices,paid_xp,paid_gold,paid_fragments,"
    "paid_essences,paid_catalysts,version,item_passive_id FROM legendary_channels WHERE uuid='%s' ORDER BY channel_id",uuid);
  if (!res) return LEG_LOAD_ERROR;
  while ((row=mysql_fetch_row(res)))
  {
    for (i=0;i<18;i++)
      if (!number(row[i],&n[i]) || ((i<=10 || i==17) && n[i]>INT_MAX)) break;
    if (i!=18 || n[0]>1) {mysql_free_result(res);return LEG_LOAD_ERROR;}
    id=(int)n[0]; c=&item.channels[id];
    if (c->present) {mysql_free_result(res);return LEG_LOAD_ERROR;}
    c->present=true;c->rank=n[1];c->path=n[2];c->family=n[3];c->foundation=n[4];
    c->subtype=n[5];c->technique=n[6];c->signature=n[7];c->modifier=n[8];
    c->modifier_target=n[9];c->first_choices=(unsigned int)n[10];
    c->paid.xp=n[11];c->paid.gold=n[12];c->paid.fragments=n[13];c->paid.essences=n[14];
    c->paid.catalysts=n[15];c->version=n[16];c->item_passive=n[17];
  }
  mysql_free_result(res);
  if (!legendary_record_valid(&item)) return LEG_LOAD_ERROR;
  *out=item;return LEG_LOAD_OK;
}

bool legendary_store_insert(const struct legendary_item_record *record)
{
  /* Channel insert and activation occur separately inside the caller's transaction. */
  if (!legendary_record_valid(record) || record->state!=LEG_RECORD_PENDING || record->version ||
      record->channels[0].present || record->channels[1].present || !legendary_schema_ready()) return false;
  return estate_query("INSERT INTO legendary_items(uuid,owner_id,prototype_vnum,slot_id,state) "
    "VALUES('%s',%lld,%d,%d,'pending')",record->uuid,(long long)record->owner,record->prototype,record->slot);
}

bool legendary_store_save_channel(const struct legendary_item_record *item, int channel, int64_t expected)
{
  const struct legendary_channel_record *c;
  bool ok;
  if (!legendary_record_valid(item) || channel<0 || channel>1 || expected < -1 || expected==INT64_MAX ||
      item->state==LEG_RECORD_DELETED || !item->channels[channel].present || !legendary_schema_ready()) return false;
  c=&item->channels[channel];
  if ((expected==-1 && c->version!=0) || (expected>=0 && c->version!=expected)) return false;
  if (expected==-1)
    ok=estate_query("INSERT INTO legendary_channels(uuid,channel_id,rank_id,path_id,family_id,foundation_id,"
      "subtype_id,technique_id,signature_id,modifier_id,modifier_target,first_choices,paid_xp,paid_gold,"
      "paid_fragments,paid_essences,paid_catalysts,item_passive_id) SELECT uuid,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%u,"
      "%lld,%lld,%lld,%lld,%lld,%d FROM legendary_items WHERE uuid='%s' AND owner_id=%lld AND version=%lld "
      "AND state<>'deleted' AND (%d=0 OR EXISTS(SELECT 1 FROM legendary_channels p WHERE p.uuid='%s' AND p.channel_id=0))",
      channel,c->rank,c->path,c->family,c->foundation,c->subtype,c->technique,c->signature,c->modifier,
      c->modifier_target,c->first_choices,(long long)c->paid.xp,(long long)c->paid.gold,(long long)c->paid.fragments,
      (long long)c->paid.essences,(long long)c->paid.catalysts,c->item_passive,item->uuid,(long long)item->owner,
      (long long)item->version,channel,item->uuid);
  else
    ok=estate_query("UPDATE legendary_channels c JOIN legendary_items i ON i.uuid=c.uuid SET "
      "c.rank_id=%d,c.path_id=%d,c.family_id=%d,c.foundation_id=%d,c.subtype_id=%d,c.technique_id=%d,"
      "c.signature_id=%d,c.modifier_id=%d,c.modifier_target=%d,c.first_choices=%u,c.paid_xp=%lld,"
      "c.paid_gold=%lld,c.paid_fragments=%lld,c.paid_essences=%lld,c.paid_catalysts=%lld,c.item_passive_id=%d,c.version=c.version+1 "
      "WHERE c.uuid='%s' AND c.channel_id=%d AND c.version=%lld AND i.owner_id=%lld AND i.version=%lld AND i.state<>'deleted'",
      c->rank,c->path,c->family,c->foundation,c->subtype,c->technique,c->signature,c->modifier,c->modifier_target,
      c->first_choices,(long long)c->paid.xp,(long long)c->paid.gold,(long long)c->paid.fragments,
      (long long)c->paid.essences,(long long)c->paid.catalysts,c->item_passive,item->uuid,channel,(long long)expected,
      (long long)item->owner,(long long)item->version);
  ok = ok && mysql_affected_rows(conn)==1;
  if(ok)legendary_object_invalidate(item->uuid,-1);
  return ok;
}

bool legendary_store_set_state(const char *uuid, int64_t owner, int64_t expected, int state)
{
  bool ok;
  if (!legendary_uuid_valid(uuid) || owner<=0 || expected<0 || expected==INT64_MAX || state<0 || state>2 ||
      !legendary_schema_ready()) return false;
  /* Never revive tombstones. Active items require a primary channel in SQL. */
  ok = estate_query("UPDATE legendary_items SET state='%s',version=version+1,"
    "deleted_at=IF(%d=2,UTC_TIMESTAMP(),deleted_at) WHERE uuid='%s' AND owner_id=%lld "
    "AND version=%lld AND state<>'deleted' AND (%d<>1 OR EXISTS(SELECT 1 FROM legendary_channels c "
    "WHERE c.uuid='%s' AND c.channel_id=0))",states[state],state,uuid,(long long)owner,
    (long long)expected,state,uuid) && mysql_affected_rows(conn)==1;
  if(ok)legendary_object_invalidate(uuid,-1);
  return ok;
}
bool legendary_store_tombstone(const char *uuid, int64_t owner, int64_t expected)
{
  return legendary_store_set_state(uuid,owner,expected,LEG_RECORD_DELETED);
}

/* Explicit character deletion is permanent destruction, never ordinary unload. */
bool legendary_retire_owner(long player,const char *name)
{
  MYSQL_RES *res;MYSQL_ROW row;bool exists=false;char **ids=NULL;size_t count=0,i;
  if(player<=0||!name||!*name)return false;
  {char path[MAX_STRING_LENGTH],line[512],receipt[64];FILE *file;
    if(!get_filename(path,sizeof(path),PLR_FILE,name))return false;
    file=fopen(path,"r");
    if(!file&&errno!=ENOENT)return false;
    if(file){while(fgets(line,sizeof(line),file))if(!strncmp(line,"LgTx:",5)&&sscanf(line+5,"%63s",receipt)==1){fclose(file);return false;}
      if(ferror(file)){fclose(file);return false;}fclose(file);}
  }
  res=estate_select("SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='legendary_items'");
  if(!res)return false;
  row=mysql_fetch_row(res);exists=row&&!strcmp(row[0],"1");mysql_free_result(res);
  if(!exists)return true;
  if(!legendary_schema_ready()||!estate_query("START TRANSACTION"))return false;
  res=estate_select("SELECT player_id FROM legendary_characters WHERE player_id=%ld FOR UPDATE",player);
  if(!res)goto fail;
  mysql_free_result(res);
  res=estate_select("SELECT (SELECT COUNT(*) FROM legendary_operations WHERE held_player=%ld)+(SELECT COUNT(*) FROM legendary_awards WHERE player_id=%ld AND state='pending')",player,player);
  if(!res)goto fail;
  row=mysql_fetch_row(res);exists=row&&!strcmp(row[0],"0");mysql_free_result(res);if(!exists)goto fail;
  res=estate_select("SELECT uuid FROM legendary_items WHERE owner_id=%ld AND state<>'deleted' FOR UPDATE",player);
  if(!res)goto fail;
  count=mysql_num_rows(res);ids=calloc(count?count:1,sizeof(*ids));if(!ids){mysql_free_result(res);goto fail;}
  i=0;while((row=mysql_fetch_row(res))){if(!legendary_uuid_valid(row[0])||!(ids[i++]=strdup(row[0]))){mysql_free_result(res);goto fail;}}
  mysql_free_result(res);
  if(!estate_query("UPDATE legendary_items SET state='deleted',deleted_at=UTC_TIMESTAMP(),version=version+1 WHERE owner_id=%ld AND state<>'deleted'",player)||
     !estate_query("UPDATE legendary_characters SET infusion_xp=0,fragments=0,catalysts=0,draconic=0,martial=0,arcane=0,spiritual=0,primal=0,shadow=0,passive_priorities=NULL,major_selections=NULL,version=version+1 WHERE player_id=%ld",player)||!estate_query("COMMIT"))goto fail;
  for(i=0;i<count;i++){legendary_object_invalidate(ids[i],-1);free(ids[i]);}free(ids);return true;
fail:
  estate_query("ROLLBACK");if(ids){for(i=0;i<count;i++)free(ids[i]);free(ids);}return false;
}

int legendary_registry_load(int vnum, bool lance, struct legendary_registry_record *out)
{
  struct legendary_registry_record r;
  MYSQL_RES *res;
  MYSQL_ROW row;
  int64_t n[2]; int i;
  if (vnum<0 || !out || !lance || !legendary_schema_ready()) return LEG_LOAD_ERROR;
  res=estate_select("SELECT enabled,version FROM legendary_dragonlances WHERE prototype_vnum=%d",vnum);
  if (!res) return LEG_LOAD_ERROR;
  row=mysql_fetch_row(res);
  if (!row) {mysql_free_result(res);return LEG_LOAD_MISSING;}
  memset(&r,0,sizeof(r));r.vnum=vnum;
  for (i=0;i<2;i++) if (!number(row[i],&n[i])) break;
  mysql_free_result(res);
  if (i!=2 || n[0]>1) return LEG_LOAD_ERROR;
  r.enabled=n[0];r.version=n[1];
  *out=r;return LEG_LOAD_OK;
}

struct registry_cache {struct registry_cache *next;bool lance;struct legendary_registry_record record;};
static struct registry_cache *registry_cache;
static bool registry_ready;
bool legendary_registry_cached(int vnum,bool lance,struct legendary_registry_record *out)
{
  struct registry_cache *p;if(!registry_ready||!out)return false;
  for(p=registry_cache;p;p=p->next)if(p->lance==lance&&p->record.vnum==vnum){*out=p->record;return true;}
  return false;
}
bool legendary_registry_refresh(void)
{
  struct registry_cache *fresh=NULL,*p,*next;MYSQL_RES *res=NULL;MYSQL_ROW row;
  int i;int64_t n[3];
  res=estate_select("SELECT prototype_vnum,enabled,version FROM legendary_dragonlances");
  if(!res)goto fail;
  while((row=mysql_fetch_row(res))){
    for(i=0;i<3;i++)if(!number(row[i],&n[i]))goto fail;
    if(n[0]>INT_MAX||n[1]>1)goto fail;
    p=calloc(1,sizeof(*p));if(!p)goto fail;p->lance=true;p->record.vnum=n[0];
    p->record.enabled=n[1];p->record.version=n[2];p->next=fresh;fresh=p;
  }
  mysql_free_result(res);res=NULL;
  for(p=registry_cache;p;p=next){next=p->next;free(p);}registry_cache=fresh;registry_ready=true;return true;
fail:
  if(res)mysql_free_result(res);
  for(p=fresh;p;p=next){next=p->next;free(p);}
  registry_ready=false;return false;
}

bool legendary_registry_save(const struct legendary_registry_record *r, bool lance, int64_t expected)
{
  bool ok;
  if (!r || r->vnum<0 || expected < -1 || expected==INT64_MAX ||
      (expected==-1?r->version!=0:r->version!=expected) ||
      !lance || !legendary_schema_ready()) return false;
  ok=expected==-1?estate_query("INSERT INTO legendary_dragonlances(prototype_vnum,enabled) VALUES(%d,%d)",r->vnum,r->enabled):
    estate_query("UPDATE legendary_dragonlances SET enabled=%d,version=version+1 WHERE prototype_vnum=%d AND version=%lld",
                 r->enabled,r->vnum,(long long)expected);
  ok = ok && mysql_affected_rows(conn)==1;
  if(ok)legendary_object_invalidate(NULL,r->vnum);
  if(ok){dragonlance_registry_epoch++;(void)legendary_registry_refresh();}
  return ok;
}
