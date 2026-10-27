/* Reuse the estate test's small world adapter; every rule/codec/SQL service is production code. */
#define main estate_baseline_main
#include "estate_integration.c"
#undef main
#include "legendary_store.h"
#include "genobj.h"
struct obj_data *object_list;
int test_update_all_objects(struct obj_data *);
#include "legendary_identity.h"
#undef assert
#define assert(test) do { if (!(test)) { fprintf(stderr,"Legendary assertion at line %d: %s\n",__LINE__,#test); abort(); } } while (0)
extern struct index_data *obj_index;
extern obj_rnum top_of_objt;
extern struct obj_data *obj_proto;
int estate_write_record(struct obj_data *,FILE *);
obj_save_data *objsave_parse_objects(FILE *);
obj_save_data *objsave_parse_objects_db(char *,room_vnum);
obj_save_data *objsave_parse_objects_db_pet(char *,long);
obj_save_data *objsave_parse_objects_db_sheath(char *,long,int,struct obj_data *);
int objsave_save_obj_record_db_pet(struct obj_data *,struct char_data *,struct char_data *,long,int);
int objsave_save_obj_record_db_sheath(struct obj_data *,struct char_data *,long,int);

static struct obj_data *weapon(const char *uuid)
{
  struct obj_data *obj=item("legendary test weapon");
  obj->item_number=0;GET_OBJ_BOUND_ID(obj)=1;
  if(uuid)memcpy(obj->legendary_uuid,uuid,33);
  return obj;
}
static void dispose_list(obj_save_data *list,const char *uuid)
{
  obj_save_data *next;
  assert(list && !list->next);
  assert(!strcmp(list->obj->legendary_uuid,uuid));
  for(;list;list=next){next=list->next;extract_obj(list->obj);free(list);}
}
static void store_fixtures(char uuid[33],struct legendary_item_record *r)
{
  struct legendary_item_record loaded;
  struct legendary_registry_record registry={500,0,0,true,false,0};
  registry.version=0;registry.enabled=true;assert(legendary_registry_save(&registry,true,-1));
  assert(legendary_registry_load(500,true,&registry)==LEG_LOAD_OK && registry.enabled);
  assert(legendary_uuid_generate(uuid));memset(r,0,sizeof(*r));memcpy(r->uuid,uuid,33);
  r->owner=1;r->prototype=500;r->slot=LEG_SLOT_WEAPON;r->state=LEG_RECORD_PENDING;
  assert(legendary_store_insert(r));assert(!legendary_store_insert(r));
  assert(!legendary_store_set_state(uuid,1,0,LEG_RECORD_ACTIVE)); /* No primary yet. */
  r->channels[0].present=true;r->channels[0].rank=5;r->channels[0].path=LEG_PATH_SLAYER;
  r->channels[0].foundation=LEG_F01;r->channels[0].technique=LEG_T01;r->channels[0].signature=LEG_S09;
  r->channels[0].modifier=LEG_MOD_READY;r->channels[0].modifier_target=1;
  assert(legendary_cost_between(0,5,&r->channels[0].paid));
  assert(legendary_store_save_channel(r,0,-1));
  r->channels[1]=r->channels[0];r->channels[1].family=LEG_FAMILY_GUARD;
  r->channels[1].path=LEG_PATH_GUARDIAN;r->channels[1].foundation=LEG_F03;
  r->channels[1].technique=LEG_T07;r->channels[1].signature=LEG_S01;
  assert(legendary_store_save_channel(r,1,-1));
  assert(legendary_store_set_state(uuid,1,0,LEG_RECORD_ACTIVE));
  assert(legendary_store_load(uuid,&loaded)==LEG_LOAD_OK);
  assert(loaded.owner==1 && loaded.version==1 && loaded.channels[1].paid.xp==77000000);
  assert(loaded.channels[0].signature==LEG_S09 && loaded.channels[1].signature==LEG_S01);
  assert(legendary_store_save_channel(&loaded,0,0));
  assert(!legendary_store_save_channel(&loaded,0,0));
  loaded.owner=2;assert(!legendary_store_save_channel(&loaded,1,0));
  assert(legendary_store_load(uuid,r)==LEG_LOAD_OK && r->channels[0].version==1);
}
int main(int argc,char **argv)
{
  char uuid[33],other[33],line[64],*packed;
  struct legendary_item_record record,unchanged;
  struct obj_data *obj,*copy,*bag,*sheathed,*preview;
  struct char_data owner;
  obj_save_data *list;
  FILE *fp;
  int i;
  assert(argc==2);
  conn=mysql_init(NULL);assert(mysql_real_connect(conn,NULL,"root",NULL,"estate_test",0,argv[1],0));
  assert(estate_init() && legendary_schema_ready());
  assert(estate_query("CREATE TABLE player_save_objs(idnum INT PRIMARY KEY AUTO_INCREMENT,name VARCHAR(64),serialized_obj LONGTEXT,creation_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP)"));
  assert(estate_query("CREATE TABLE house_data(idnum INT PRIMARY KEY AUTO_INCREMENT,vnum INT,serialized_obj LONGTEXT,creation_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP)"));
  assert(estate_query("CREATE TABLE pet_save_objs(id INT PRIMARY KEY AUTO_INCREMENT,pet_idnum BIGINT,owner_name VARCHAR(64),serialized_obj LONGTEXT,creation_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP)"));
  assert(estate_query("CREATE TABLE player_save_objs_sheathed(id INT PRIMARY KEY AUTO_INCREMENT,sheath_obj_id BIGINT,sheathed_position INT,owner_name VARCHAR(64),serialized_obj LONGTEXT,creation_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP)"));
  obj_proto=calloc(1,sizeof(*obj_proto));
  obj_index=calloc(1,sizeof(*obj_index));obj_index[0].vnum=500;top_of_objt=0;
  top_of_world=1;world=calloc(2,sizeof(*world));world[1].number=2200;
  init_player(&owner,"owner",1);
  store_fixtures(uuid,&record);
  for(i=0;i<100;i++){assert(legendary_uuid_generate(other)&&legendary_uuid_valid(other)&&strcmp(uuid,other));}
  assert(!legendary_uuid_valid("00000000000000000000000000000000"));
  assert(!legendary_uuid_valid("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"));
  obj=weapon(uuid);fp=tmpfile();assert(estate_write_record(obj,fp));rewind(fp);
  list=objsave_parse_objects(fp);fclose(fp);dispose_list(list,uuid);
  assert(legendary_object_verify(obj));
  /* Both clone entry points clear identity; live prototype refresh retains it. */
  copy=weapon(NULL);assert(copy_object(copy,obj));
  assert(!copy->legendary_uuid[0] && !copy->legendary_record && obj->legendary_record);extract_obj(copy);
  assert(copy_object_preserve(&obj_proto[0],obj));assert(!obj_proto[0].legendary_uuid[0]);
  object_list=obj;assert(test_update_all_objects(&obj_proto[0])==1);
  assert(!strcmp(obj->legendary_uuid,uuid) && GET_OBJ_BOUND_ID(obj)==1);object_list=NULL;
  fp=tmpfile();fputs("#500\nName: old sword\n\n",fp);rewind(fp);
  dispose_list(objsave_parse_objects(fp),"");fclose(fp);
  /* Main player SQL and house SQL, including tagged full-record parser. */
  fp=tmpfile();assert(objsave_save_obj_record_db(obj,&owner,NOWHERE,fp,0));fclose(fp);
  dispose_list(objsave_parse_objects_db("owner",NOWHERE),uuid);
  assert(estate_query("UPDATE player_save_objs SET serialized_obj=REPLACE(serialized_obj,'SpCl: 1\\n','')"));
  dispose_list(objsave_parse_objects_db("owner",NOWHERE),uuid);
  fp=tmpfile();assert(objsave_save_obj_record_db(obj,NULL,77,fp,0));fclose(fp);
  dispose_list(objsave_parse_objects_db(NULL,77),uuid);
  /* Pet and sheath use their separate production writers AND parsers. */
  assert(objsave_save_obj_record_db_pet(obj,&owner,&owner,42,0));
  dispose_list(objsave_parse_objects_db_pet("owner",42),uuid);
  assert(objsave_save_obj_record_db_sheath(obj,&owner,99,1));
  bag=item("sheath");
  dispose_list(objsave_parse_objects_db_sheath("owner",99,1,bag),uuid);
  /* Estate framing used by housing chest, auction, stalls and delivery. */
  copy=weapon(NULL);packed=estate_pack(copy);preview=estate_unpack(packed);free(packed);
  assert(preview && !preview->legendary_uuid[0]);extract_obj(preview);extract_obj(copy);
  bag->contains=obj;obj->in_obj=bag;
  sheathed=weapon(uuid);bag->sheath_primary=sheathed;
  packed=estate_pack(bag);preview=estate_unpack(packed);free(packed);
  assert(preview && !strcmp(preview->contains->legendary_uuid,uuid));
  assert(preview->sheath_primary && !strcmp(preview->sheath_primary->legendary_uuid,uuid));
  /* Detached previews do not create live collisions with finalized originals. */
  assert(legendary_object_verify(obj));legendary_object_live(preview);
  assert(!obj->legendary_quarantined && !preview->contains->legendary_quarantined);
  extract_obj(preview);bag->sheath_primary=NULL;extract_obj(sheathed);
  /* Register a second finalized copy: both are quarantined, ordinary items retained. */
  copy=weapon(uuid);assert(!legendary_object_verify(copy));
  assert(obj->legendary_quarantined && copy->legendary_quarantined);
  assert(!obj->legendary_record && !copy->legendary_record);extract_obj(copy);extract_obj(bag);
  obj=weapon(uuid);assert(legendary_object_verify(obj)&&obj->legendary_record->channels[1].present);
  legendary_object_unload(obj);assert(legendary_store_load(uuid,&record)==LEG_LOAD_OK); /* Unload is not destruction. */
  assert(legendary_object_verify(obj));
  assert(legendary_store_tombstone(uuid,1,record.version));
  assert(obj->legendary_quarantined && !obj->legendary_record);
  assert(!legendary_store_set_state(uuid,1,record.version+1,LEG_RECORD_ACTIVE));
  assert(!legendary_object_verify(obj)&&obj->legendary_quarantined);extract_obj(obj);
  assert(legendary_uuid_generate(other));obj=weapon(other);
  assert(!legendary_object_verify(obj)&&obj->legendary_quarantined);extract_obj(obj);
  memset(&unchanged,7,sizeof(unchanged));record=unchanged;
  assert(legendary_store_load(other,&record)==LEG_LOAD_MISSING && !memcmp(&unchanged,&record,sizeof(record)));
  /* Unknown version/malformed UUID survives save/load as quarantined, not ordinary. */
  obj=weapon(NULL);legendary_identity_restore(obj,"2 abc");assert(obj->legendary_quarantined);
  assert(legendary_identity_line(obj,line,sizeof(line))&&strstr(line,"LgId: 1 "));
  copy=weapon(NULL);legendary_identity_restore(copy,line+6);assert(copy->legendary_quarantined);
  extract_obj(copy);extract_obj(obj);
  assert(estate_query("UPDATE legendary_schema SET version=99 WHERE singleton=1"));
  assert(!legendary_schema_ready());assert(legendary_store_load(uuid,&record)==LEG_LOAD_ERROR);
  assert(estate_query("UPDATE legendary_schema SET version=10 WHERE singleton=1"));
  /* FK/uniqueness/check constraints and rollback are real database behavior. */
  assert(!estate_query("INSERT INTO legendary_channels(uuid,channel_id,rank_id,path_id) VALUES('11111111111111111111111111111111',0,1,1)"));
  assert(!estate_query("INSERT INTO legendary_characters(player_id,fragments_today) VALUES(1,201)"));
  assert(estate_query("START TRANSACTION"));
  assert(legendary_uuid_generate(other));memset(&record,0,sizeof(record));memcpy(record.uuid,other,33);
  record.owner=1;record.prototype=500;record.slot=0;assert(legendary_store_insert(&record));
  assert(estate_query("ROLLBACK"));assert(legendary_store_load(other,&record)==LEG_LOAD_MISSING);
  free(obj_index);free(owner.player.name);free(owner.player_specials);mysql_close(conn);
  puts("PASS: legendary schema/version/CAS, two-channel persistence, file/player/house/pet/sheath codecs,");
  puts("nested estate payloads, previews, duplicates, missing/tombstoned UUIDs, malformed tags and rollback");
  return 0;
}
