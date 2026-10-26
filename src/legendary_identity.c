#include "conf.h"
#include "sysdep.h"
#include "legendary_equipment.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "legendary_identity.h"
#include "legendary_store.h"
#include <openssl/rand.h>

struct legendary_live_identity { struct obj_data *object; struct legendary_live_identity *next; };
static struct legendary_live_identity *live_identities;

bool legendary_uuid_valid(const char *uuid)
{
  int i; bool nonzero=false;
  if (!uuid || strnlen(uuid,33)!=32) return false;
  for(i=0;i<32;i++)
  {
    if (!((uuid[i]>='0'&&uuid[i]<='9')||(uuid[i]>='a'&&uuid[i]<='f'))) return false;
    if(uuid[i]!='0') nonzero=true;
  }
  return nonzero;
}
bool legendary_uuid_generate(char out[33])
{
  unsigned char bytes[16]; char result[33]; int i;
  static const char hex[]="0123456789abcdef";
  if(!out || RAND_bytes(bytes,sizeof(bytes))!=1) return false;
  bytes[6]=(bytes[6]&15)|64; bytes[8]=(bytes[8]&63)|128; /* UUID v4 variant. */
  for(i=0;i<16;i++) {result[2*i]=hex[bytes[i]>>4];result[2*i+1]=hex[bytes[i]&15];}
  result[32]='\0';memcpy(out,result,33);return true;
}
void legendary_object_unload(struct obj_data *obj)
{
  struct legendary_live_identity **p=&live_identities,*old;
  if(!obj)return;
  while(*p)
  {
    if((*p)->object==obj) {old=*p;*p=old->next;free(old);continue;}
    p=&(*p)->next;
  }
  free(obj->legendary_record);obj->legendary_record=NULL;
  if(obj->worn_by)legendary_loadout_invalidate(obj->worn_by);
}
void legendary_object_clear_identity(struct obj_data *obj)
{
  if(!obj)return;
  legendary_object_unload(obj);memset(obj->legendary_uuid,0,sizeof(obj->legendary_uuid));
  obj->legendary_quarantined=false;
}
void legendary_identity_restore(struct obj_data *obj, const char *value)
{
  char uuid[33],extra;
  if(!obj)return;
  if(obj->legendary_uuid[0] || !value || value[0]!='1' || !isspace((unsigned char)value[1]) ||
     sscanf(value+2,"%32s %c",uuid,&extra)!=1 || !legendary_uuid_valid(uuid))
  {
    legendary_object_unload(obj);
    /* A persistent invalid marker keeps this item quarantined on later saves. */
    if (!obj->legendary_uuid[0]) {
      if(value && sscanf(value,"%*s %32s",uuid)==1 && legendary_uuid_valid(uuid))
        memcpy(obj->legendary_uuid,uuid,33);
      else {
        memset(obj->legendary_uuid,'0',32);obj->legendary_uuid[32]='\0';
      }
    }
    obj->legendary_quarantined=true;
    log("SYSERR: Invalid or duplicate legendary identity tag on object runtime ID %ld",(long)GET_ID(obj));
    return;
  }
  memcpy(obj->legendary_uuid,uuid,33);
}
bool legendary_identity_line(const struct obj_data *obj,char *out,size_t size)
{
  int n;
  if(!obj||!out||!size)return false;
  if(!obj->legendary_uuid[0]) {*out='\0';return true;}
  if(strnlen(obj->legendary_uuid,33)!=32)return false;
  if(!legendary_uuid_valid(obj->legendary_uuid) &&
     strcmp(obj->legendary_uuid,"00000000000000000000000000000000"))return false;
  n=snprintf(out,size,"LgId: 1 %s\n%s",obj->legendary_uuid,
             obj->legendary_quarantined?"LgQn: 1\n":"");
  return n>=0&&(size_t)n<size;
}
static bool quarantine(struct obj_data *obj,const char *reason)
{
  obj->legendary_quarantined=true;
  if(obj->worn_by)legendary_loadout_invalidate(obj->worn_by);
  free(obj->legendary_record);obj->legendary_record=NULL;
  log("SYSERR: Legendary item %s quarantined: %s",obj->legendary_uuid,reason);
  return false;
}
void legendary_object_invalidate(const char *uuid, int prototype)
{
  struct legendary_live_identity *p;
  for(p=live_identities;p;p=p->next)
    if(uuid ? !strcmp(p->object->legendary_uuid,uuid) :
       (int64_t)GET_OBJ_VNUM(p->object)==prototype)
      quarantine(p->object,"authoritative record changed; reload after commit");
}
bool legendary_object_verify(struct obj_data *obj)
{
  struct legendary_live_identity *p,*node;
  struct legendary_item_record record;
  struct legendary_registry_record lance;
  bool duplicate=false,needs_lance=false;
  int i,result;
  if(!obj)return false;
  if(!obj->legendary_uuid[0])return true;
  if(obj->legendary_quarantined)return false;
  if(!legendary_uuid_valid(obj->legendary_uuid))return quarantine(obj,"invalid UUID");
  for(p=live_identities;p;p=p->next)
  {
    if(p->object!=obj&&!strcmp(p->object->legendary_uuid,obj->legendary_uuid))
    {quarantine(p->object,"duplicate live UUID");duplicate=true;}
  }
  if(duplicate)return quarantine(obj,"duplicate live UUID");
  if(obj->legendary_record)return true;
  node=calloc(1,sizeof(*node));
  if(!node)return quarantine(obj,"identity tracking allocation failed");
  node->object=obj;node->next=live_identities;live_identities=node;
  result=legendary_store_load(obj->legendary_uuid,&record);
  if(result!=LEG_LOAD_OK)return quarantine(obj,result==LEG_LOAD_MISSING?"missing database record":"database unavailable or invalid record");
  if(record.state!=LEG_RECORD_ACTIVE)return quarantine(obj,"pending or deleted database record");
  if((int64_t)GET_OBJ_VNUM(obj)!=record.prototype || GET_OBJ_BOUND_ID(obj)!=record.owner)
    return quarantine(obj,"prototype or binding disagrees with authoritative owner");
  for(i=0;i<2;i++)
    if(record.channels[i].present && (record.channels[i].signature==LEG_S11||record.channels[i].signature==LEG_S12)) needs_lance=true;
  if(needs_lance&&(legendary_registry_load(record.prototype,true,&lance)!=LEG_LOAD_OK||!lance.enabled))
    return quarantine(obj,"true-dragonlance registry missing or disabled");
  obj->legendary_record=malloc(sizeof(record));
  if(!obj->legendary_record)return quarantine(obj,"record allocation failed");
  *obj->legendary_record=record;return true;
}
static void live_tree(struct obj_data *obj, int depth)
{
  struct obj_data *child;
  if(!obj || depth>64)return;
  if(obj->legendary_uuid[0])legendary_object_verify(obj);
  for(child=obj->contains;child;child=child->next_content)live_tree(child,depth+1);
  live_tree(obj->sheath_primary,depth+1);
  live_tree(obj->sheath_secondary,depth+1);
}
void legendary_object_live(struct obj_data *obj)
{
  struct obj_data *root=obj;
  int depth=0;
  if(!obj)return;
  while(root->in_obj && depth++<64)root=root->in_obj;
  /* Detached parser/estate previews never claim an identity. */
  if(depth>=64 || (!root->carried_by&&!root->worn_by&&IN_ROOM(root)==NOWHERE))return;
  live_tree(obj,0);
}
