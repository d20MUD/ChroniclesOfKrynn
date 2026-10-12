#!/usr/bin/env python3
"""Exercise purchased pet XP and actual pet saves against disposable MariaDB."""
from pathlib import Path
import os
import shlex
import subprocess
import tempfile
import time

root = Path(__file__).resolve().parents[1]
def function(source, signature):
    start = source.index(signature + '\n{')
    return source[start:source.index('\n}', start) + 2] + '\n'
players = (root/'src/players.c').read_text()
mobile = (root/'src/mob_utils.c').read_text()
schema = (root/'src/db_init.c').read_text()
start = schema.index('  /* pet_data - Saved companion information */')
end = schema.index('  /* player_data - Core player character information */', start)
code = 'static void upgrade_pet_schema(void) {\n' + schema[start:end] + '\n}\n'
code += function(players, 'static bool persist_char_pets(struct char_data *ch)')
code += function(players, 'void save_char_pets(struct char_data *ch)')
code += function(players, 'bool archive_fallen_pet(struct char_data *pet)')
code += function(players, 'static struct char_data *load_char_pets_filtered(struct char_data *ch, int revive_id)')
code += function(players, 'void load_char_pets(struct char_data *ch)')
code += function(players, 'void purchased_pet_revival(struct char_data *ch, const char *id_arg)')
code += function(players, 'static void destroy_pet_holder(struct char_data *pet)')
code += function(players, 'void discard_fallen_pet(struct char_data *ch, const char *id_arg)')
code += function(mobile, 'int purchased_pet_level_cost(struct char_data *pet)')
code += function(mobile, 'void award_purchased_pet_exp(struct char_data *owner, struct char_data *victim, int reward)')
for signature in [
    'int companion_training_kind(struct char_data *pet)',
    'void restore_companion_kind(struct char_data *pet, int kind)',
    'bool is_persistent_pet(struct char_data *pet)',
    'static int companion_unspent_points(struct char_data *pet)',
    'void award_companion_training_exp(struct char_data *owner, struct char_data *victim, int reward)',
    'void show_companion_training(struct char_data *owner, struct char_data *pet)',
    'void train_companion_skill(struct char_data *owner, struct char_data *pet, const char *skill)']:
    code += function(mobile, signature)
objects = (root/'src/objsave.c').read_text()
gear_code = '#define MAX_BAG_ROWS 64\n#define LOC_INVENTORY 0\n'
gear_code += function(objects, 'static int Crash_save_pet(struct obj_data *obj, struct char_data *ch, struct char_data *owner,\n                          long int pet_idnum, int location)')
gear_code += function(objects, 'bool pet_save_objs(struct char_data *ch, struct char_data *owner, long int pet_idnum)').replace('bool pet_save_objs(', 'static bool production_pet_save_objs(', 1)
gear_code += function(objects, 'static int handle_obj(struct obj_data *temp, struct char_data *ch, int locate,\n                      struct obj_data **cont_row)')
gear_code += function(objects, 'bool pet_load_objs(struct char_data *ch, struct char_data *owner, long int pet_idnum)').replace('bool pet_load_objs(', 'static bool production_pet_load_objs(', 1)
headers = '\n'.join('#include "'+h+'"' for h in
    ['conf.h','sysdep.h','structs.h','utils.h','db.h','comm.h','mysql.h','spells.h',
     'handler.h','mudlim.h','oasis.h','mud_event.h','dg_event.h','evolutions.h','crafting_new.h','mob_utils.h'])
stubs = r'''
#include <assert.h>
#include <limits.h>
MYSQL *conn;
bool mysql_available = TRUE;
struct room_data *world;
room_rnum top_of_world;
struct zone_data *zone_table;
struct index_data *mob_index;
mob_rnum top_of_mobt;
struct config_data config_info;
struct char_data loaded;
static int save_objects, loaded_objects, saved_players, deny_follower, deny_mobile, fail_gear, inventory_saves;
static struct char_data *temporary_pet;
static struct mud_event_data temporary_event;
static char messages[32768];
bool can_add_follower(struct char_data *ch,int vnum) { return !deny_follower; }
int decrease_gold(struct char_data *ch,int amount) { GET_GOLD(ch)-=amount; return amount; }
int increase_gold(struct char_data *ch,int amount) { GET_GOLD(ch)+=amount; return amount; }
void save_char(struct char_data *ch,int mode) { saved_players++; }
void extract_char(struct char_data *ch) {}
int MIN(int a,int b) { return a < b ? a : b; }
int MAX(int a,int b) { return a > b ? a : b; }
int LIMIT(int val,int low,int high) { return MAX(low,MIN(val,high)); }
size_t send_to_char(struct char_data *ch,const char *fmt,...) {
  va_list ap; va_start(ap,fmt); size_t used=strlen(messages);
  int n=vsnprintf(messages+used,sizeof(messages)-used,fmt,ap); va_end(ap); return n;
}
void basic_mud_log(const char *fmt,...) {
  va_list ap; va_start(ap,fmt); vfprintf(stderr,fmt,ap); va_end(ap); fputc('\n',stderr);
}
bool ensure_mysql_connection(MYSQL *c,const char *caller) { return !mysql_ping(c); }
int mysql_query_safe(MYSQL *c,const char *q) { return mysql_query(c,q); }
char *mysql_escape_string_alloc(MYSQL *c,const char *s) {
  char *out=malloc(strlen(s)*2+1); mysql_real_escape_string(c,out,s,strlen(s)); return out;
}
bool valid_pet_name(char *s) { return s && *s; }
struct mud_event_data *char_has_mud_event(struct char_data *ch,event_id id) { return ch==temporary_pet && id==ePURGEMOB ? &temporary_event : NULL; }
long event_time(struct event *e) { return 0; }
struct mud_event_data *new_mud_event(event_id id,void *p,const char *s) { return NULL; }
void attach_mud_event(struct mud_event_data *e,long time) {}
bool pet_save_objs(struct char_data *pet,struct char_data *owner,long id);
bool pet_load_objs(struct char_data *pet,struct char_data *owner,long id);
bool isSummonMob(int vnum) { return FALSE; }
void restore_crafted_golem_identity(struct char_data *ch) {}
void set_eidolon_descs(struct char_data *ch) {}
void assign_eidolon_evolutions(struct char_data *ch,struct char_data *pet,bool from_db) {}
void load_mtrigger(struct char_data *ch) {}
void join_group(struct char_data *ch,struct group_data *g) {}
const char *act(const char *s,int hide,struct char_data *ch,struct obj_data *obj,void *vict,int type) { return s; }
void char_to_room(struct char_data *ch,room_rnum room) { IN_ROOM(ch)=room; }
void add_follower(struct char_data *pet,struct char_data *owner) {
  struct follow_type *f=calloc(1,sizeof(*f)); f->follower=pet; f->next=owner->followers;
  owner->followers=f; pet->master=owner;
}
void affect_total(struct char_data *ch) {
  GET_MAX_HIT(ch)=GET_REAL_MAX_HIT(ch)+10; /* Emulate a persistent max-HP bonus. */
  ch->points.armor=GET_REAL_AC(ch);
  GET_HITROLL(ch)=GET_REAL_HITROLL(ch); GET_DAMROLL(ch)=GET_REAL_DAMROLL(ch);
}
void autoroll_mob(struct char_data *ch,bool real,bool summon) {
  GET_REAL_HITROLL(ch)=99; GET_REAL_DAMROLL(ch)=98;
  ch->mob_specials.damnodice=7; ch->mob_specials.damsizedice=30;
  GET_EXP(ch)=90000;
}
struct char_data *read_mobile(mob_vnum vnum,int type) {
  if (deny_mobile) return NULL;
  memset(&loaded,0,sizeof(loaded)); SET_BIT_AR(MOB_FLAGS(&loaded),MOB_ISNPC);
  loaded.player.name="crocodile"; loaded.player.short_descr="a crocodile";
  loaded.player.long_descr="A crocodile is here."; loaded.player.description="A crocodile.";
  GET_MOB_RNUM(&loaded)=0; return &loaded;
}
void save_char_pets(struct char_data *ch);
void load_char_pets(struct char_data *ch);
static void sql(const char *q) {
  if(mysql_query(conn,q)) { fprintf(stderr,"%s: %s\n",q,mysql_error(conn)); abort(); }
}
void Crash_crashsave(struct char_data *ch) { inventory_saves++; }
int can_carry_weight_limit(struct char_data *ch) { return 10000; }
void obj_from_char(struct obj_data *obj) {
  struct char_data *ch=obj->carried_by;
  if (!ch) return;
  struct obj_data **p=&ch->carrying;
  while (*p && *p!=obj) p=&(*p)->next_content;
  if (*p) *p=obj->next_content;
  IS_CARRYING_W(ch)-=GET_OBJ_WEIGHT(obj); IS_CARRYING_N(ch)--;
  obj->carried_by=NULL; obj->next_content=NULL;
}
void obj_to_char(struct obj_data *obj,struct char_data *ch) {
  obj->carried_by=ch; obj->in_obj=NULL; obj->next_content=ch->carrying; ch->carrying=obj;
  IS_CARRYING_W(ch)+=GET_OBJ_WEIGHT(obj); IS_CARRYING_N(ch)++;
}
struct obj_data *unequip_char(struct char_data *ch,int slot) {
  struct obj_data *obj=GET_EQ(ch,slot); GET_EQ(ch,slot)=NULL;
  if (obj) { obj->worn_by=NULL; obj->worn_on=-1; }
  return obj;
}
void equip_char(struct char_data *ch,struct obj_data *obj,int slot) {
  GET_EQ(ch,slot)=obj; obj->worn_by=ch; obj->worn_on=slot;
}
void obj_to_obj(struct obj_data *obj,struct obj_data *container) {
  obj->in_obj=container; obj->carried_by=NULL; obj->next_content=container->contains;
  container->contains=obj;
}
void extract_obj(struct obj_data *obj) {
  if (obj->carried_by) obj_from_char(obj);
  if (obj->worn_by) unequip_char(obj->worn_by,obj->worn_on);
  while (obj->contains) { struct obj_data *child=obj->contains; obj->contains=child->next_content; extract_obj(child); }
  free(obj);
}
static void auto_equip(struct char_data *ch,struct obj_data *obj,int locate) {
  if (locate>0 && locate<=NUM_WEARS && !GET_EQ(ch,locate-1)) equip_char(ch,obj,locate-1);
  else obj_to_char(obj,ch);
}
/* Keep object payload serialization small; production traversal and restoration are exercised. */
int objsave_save_obj_record_db_pet(struct obj_data *obj,struct char_data *pet,struct char_data *owner,long id,int locate) {
  char query[1024];
  char *escaped=mysql_escape_string_alloc(conn,GET_NAME(owner));
  snprintf(query,sizeof(query),"INSERT INTO pet_save_objs(pet_idnum,owner_name,serialized_obj) VALUES(%ld,'%s','%d %d %d %d')",
           id,escaped,locate,GET_OBJ_TYPE(obj),GET_OBJ_WEIGHT(obj),GET_OBJ_VAL(obj,0)); free(escaped);
  return !mysql_query(conn,query);
}
obj_save_data *objsave_parse_objects_db_pet(char *name,long id) {
  char query[256]; obj_save_data *head=NULL,**tail=&head;
  if (fail_gear) return NULL;
  snprintf(query,sizeof(query),"SELECT serialized_obj FROM pet_save_objs WHERE pet_idnum=%ld ORDER BY row_id",id);
  sql(query); MYSQL_RES *r=mysql_store_result(conn); MYSQL_ROW row;
  while ((row=mysql_fetch_row(r))) {
    int locate,type,weight,value; assert(sscanf(row[0],"%d %d %d %d",&locate,&type,&weight,&value)==4);
    obj_save_data *item=calloc(1,sizeof(*item)); item->obj=calloc(1,sizeof(*item->obj)); item->locate=locate;
    GET_OBJ_TYPE(item->obj)=type; GET_OBJ_WEIGHT(item->obj)=weight; GET_OBJ_VAL(item->obj,0)=value;
    *tail=item; tail=&item->next;
  }
  mysql_free_result(r); return head;
}

'''
checks = r'''
int main(int argc,char **argv) {
  struct char_data owner={0},pet={0},victim={0},summon={0};
  struct descriptor_data desc={0}; struct follow_type follower={0},second={0};
  struct room_data rooms[2]={0}; struct zone_data zones[1]={0}; struct index_data indices[1]={0};
  assert(argc==2); conn=mysql_init(NULL);
  assert(mysql_real_connect(conn,NULL,"root",NULL,"pet_progression_test",0,argv[1],0));
  /* Old schema upgrades with default-zero fields, then upgrades idempotently. */
  sql("CREATE TABLE pet_data (pet_data_id INT AUTO_INCREMENT PRIMARY KEY, owner_name VARCHAR(50), "
      "pet_name VARCHAR(50), pet_sdesc VARCHAR(255), pet_ldesc TEXT, pet_ddesc TEXT, "
      "vnum INT, level INT, hp INT, max_hp INT, str INT, con INT, dex INT, ac INT, "
      "intel INT DEFAULT 0, wis INT, cha INT, purge_mob_timer INT DEFAULT 0)");
  sql("CREATE TABLE pet_save_objs (row_id INT AUTO_INCREMENT PRIMARY KEY, owner_name VARCHAR(50), pet_idnum INT, serialized_obj TEXT)");
  sql("INSERT INTO pet_data(owner_name) VALUES('old')");
  upgrade_pet_schema(); upgrade_pet_schema();
  sql("SELECT purchased_pet,pet_experience FROM pet_data WHERE owner_name='old'");
  MYSQL_RES *result=mysql_store_result(conn); MYSQL_ROW row=mysql_fetch_row(result);
  assert(atoi(row[0])==0 && atoi(row[1])==0); mysql_free_result(result);
  world=rooms; zone_table=zones; mob_index=indices; top_of_world=1; indices[0].vnum=500;
  owner.player.name="Trainer"; owner.desc=&desc; GET_EXP(&owner)=12345; GET_LEVEL(&owner)=5;
  IN_ROOM(&owner)=IN_ROOM(&pet)=IN_ROOM(&victim)=IN_ROOM(&summon)=0;
  SET_BIT_AR(MOB_FLAGS(&pet),MOB_ISNPC); SET_BIT_AR(AFF_FLAGS(&pet),AFF_CHARM);
  SET_BIT_AR(MOB_FLAGS(&victim),MOB_ISNPC);
  pet.player.name="crocodile"; pet.player.short_descr="a crocodile";
  pet.player.long_descr="A crocodile is here."; pet.player.description="A crocodile.";
  pet.master=&owner; GET_LEVEL(&pet)=2; GET_POS(&pet)=POS_FIGHTING; FIGHTING(&pet)=&victim;
  GET_REAL_MAX_HIT(&pet)=GET_MAX_HIT(&pet)=60; GET_HIT(&pet)=20; GET_REAL_AC(&pet)=180;
  GET_REAL_STR(&pet)=14; GET_REAL_CON(&pet)=13; GET_REAL_DEX(&pet)=12;
  GET_REAL_INT(&pet)=2; GET_REAL_WIS(&pet)=10; GET_REAL_CHA(&pet)=6;
  GET_REAL_HITROLL(&pet)=3; GET_REAL_DAMROLL(&pet)=4;
  pet.mob_specials.damnodice=2; pet.mob_specials.damsizedice=6;
  pet.mob_specials.purchased_pet=TRUE;
  follower.follower=&pet; follower.next=&second; second.follower=&summon; owner.followers=&follower;
  summon.master=&owner; SET_BIT_AR(MOB_FLAGS(&summon),MOB_ISNPC);
  SET_BIT_AR(AFF_FLAGS(&summon),AFF_CHARM); GET_LEVEL(&summon)=1;
  GET_POS(&summon)=POS_FIGHTING; FIGHTING(&summon)=&victim;
  summon.player.name="summon"; summon.player.short_descr="summon";
  summon.player.long_descr="summon"; summon.player.description="summon";
  award_purchased_pet_exp(&owner,&victim,5999);
  assert(pet.mob_specials.pet_experience==5999 && GET_LEVEL(&pet)==2);
  award_purchased_pet_exp(&owner,&victim,1001);
  assert(GET_LEVEL(&pet)==3 && pet.mob_specials.pet_experience==1000);
  assert(GET_REAL_MAX_HIT(&pet)==70 && GET_HIT(&pet)==20 && GET_REAL_AC(&pet)==190);
  assert(GET_EXP(&pet)==0 && GET_LEVEL(&summon)==1 && summon.mob_specials.pet_experience==0);
  award_purchased_pet_exp(&owner,&victim,8000);
  assert(GET_LEVEL(&pet)==4 && pet.mob_specials.pet_experience==1000);
  assert(GET_REAL_HITROLL(&pet)==4 && GET_REAL_DAMROLL(&pet)==5);
  assert(GET_REAL_SAVE(&pet,SAVING_FORT)==1);
  /* Rejections: parked/outside, no contribution, dying, no meaningful XP, charmed victim, PvP. */
  IN_ROOM(&pet)=1; award_purchased_pet_exp(&owner,&victim,1000); IN_ROOM(&pet)=0;
  FIGHTING(&pet)=NULL; award_purchased_pet_exp(&owner,&victim,1000); FIGHTING(&pet)=&victim;
  GET_POS(&pet)=POS_DEAD; award_purchased_pet_exp(&owner,&victim,1000); GET_POS(&pet)=POS_FIGHTING;
  award_purchased_pet_exp(&owner,&victim,1);
  SET_BIT_AR(AFF_FLAGS(&victim),AFF_CHARM); award_purchased_pet_exp(&owner,&victim,1000);
  REMOVE_BIT_AR(AFF_FLAGS(&victim),AFF_CHARM);
  REMOVE_BIT_AR(MOB_FLAGS(&victim),MOB_ISNPC); award_purchased_pet_exp(&owner,&victim,1000);
  SET_BIT_AR(MOB_FLAGS(&victim),MOB_ISNPC);
  victim.mob_specials.purchased_pet=TRUE; award_purchased_pet_exp(&owner,&victim,1000);
  victim.mob_specials.purchased_pet=FALSE;
  assert(pet.mob_specials.pet_experience==1000 && GET_EXP(&owner)==12345);
  /* Saves keep base attack values, rather than persisting temporary buffs. */
  GET_HITROLL(&pet)=100; GET_DAMROLL(&pet)=100;
  follower.next=NULL; save_char_pets(&owner); assert(save_objects>0);
  owner.followers=NULL; load_char_pets(&owner);
  assert(owner.followers && owner.followers->follower==&loaded);
  assert(loaded.mob_specials.purchased_pet && loaded.mob_specials.pet_experience==1000);
  assert(GET_LEVEL(&loaded)==4 && GET_HIT(&loaded)==20 && GET_REAL_MAX_HIT(&loaded)==80);
  assert(GET_REAL_AC(&loaded)==200 && GET_REAL_INT(&loaded)==2);
  assert(GET_HITROLL(&loaded)==4 && GET_DAMROLL(&loaded)==5);
  assert(loaded.mob_specials.damnodice==2 && loaded.mob_specials.damsizedice==6 && GET_EXP(&loaded)==0);
  GET_POS(&loaded)=POS_FIGHTING; FIGHTING(&loaded)=&victim;
  award_purchased_pet_exp(&owner,&victim,INT_MAX);
  assert(GET_LEVEL(&loaded)==5 && loaded.mob_specials.pet_experience==0);
  award_purchased_pet_exp(&owner,&victim,1000); assert(loaded.mob_specials.pet_experience==0);
  GET_LEVEL(&owner)=40; GET_LEVEL(&loaded)=29;
  award_purchased_pet_exp(&owner,&victim,INT_MAX);
  assert(GET_LEVEL(&loaded)==30 && loaded.mob_specials.pet_experience==0);
  /* Worn gear and nested inventory survive death without a corpse. */
  struct obj_data *armor=calloc(1,sizeof(*armor)), *bag=calloc(1,sizeof(*bag)), *gem=calloc(1,sizeof(*gem));
  GET_GOLD(&loaded)=77;
  GET_OBJ_TYPE(armor)=ITEM_ARMOR; GET_OBJ_WEIGHT(armor)=4; GET_OBJ_VAL(armor,0)=101;
  GET_OBJ_TYPE(bag)=ITEM_CONTAINER; GET_OBJ_WEIGHT(bag)=5; GET_OBJ_VAL(bag,0)=202;
  GET_OBJ_TYPE(gem)=ITEM_TREASURE; GET_OBJ_WEIGHT(gem)=1; GET_OBJ_VAL(gem,0)=303;
  equip_char(&loaded,armor,WEAR_BODY); obj_to_obj(gem,bag); obj_to_char(bag,&loaded);
  sql("CREATE TRIGGER reject_nested_gear BEFORE INSERT ON pet_save_objs FOR EACH ROW BEGIN IF NEW.serialized_obj LIKE '%303' THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='test nested gear save rejection'; END IF; END");
  assert(!archive_fallen_pet(&loaded));
  assert(GET_EQ(&loaded,WEAR_BODY)==armor && loaded.carrying==bag && bag->contains==gem);
  sql("DROP TRIGGER reject_nested_gear");
  /* Death archives before unlinking; subsequent saves and logins retain the fallen record. */
  GET_POS(&loaded)=POS_DEAD; GET_HIT(&loaded)=-10; loaded.mob_specials.pet_experience=1234;
  assert(archive_fallen_pet(&loaded)); owner.followers=NULL; destroy_pet_holder(&loaded); save_char_pets(&owner);
  sql("SELECT pet_data_id FROM pet_data WHERE owner_name='Trainer' AND pet_dead=1");
  result=mysql_store_result(conn); row=mysql_fetch_row(result); assert(row);
  char id[32]; snprintf(id,sizeof(id),"%s",row[0]); mysql_free_result(result);
  load_char_pets(&owner); assert(!owner.followers);
  purchased_pet_revival(&owner,NULL); assert(strstr(messages,"15000 gold"));
  GET_GOLD(&owner)=15000;
  owner.player.name="Other"; purchased_pet_revival(&owner,id); assert(GET_GOLD(&owner)==15000 && !owner.followers);
  owner.player.name="Trainer";
  purchased_pet_revival(&owner,"0"); purchased_pet_revival(&owner,"1x");
  purchased_pet_revival(&owner,"999999999999999999999999");
  FIGHTING(&owner)=&victim; purchased_pet_revival(&owner,id); FIGHTING(&owner)=NULL;
  deny_follower=1; purchased_pet_revival(&owner,id); deny_follower=0;
  GET_GOLD(&owner)=14999; purchased_pet_revival(&owner,id);
  assert(!owner.followers && GET_GOLD(&owner)==14999 && !saved_players);
  GET_GOLD(&owner)=15000; deny_mobile=1; purchased_pet_revival(&owner,id); deny_mobile=0;
  assert(!owner.followers && GET_GOLD(&owner)==15000 && !saved_players);
  sql("CREATE TRIGGER reject_pet_revival BEFORE UPDATE ON pet_data FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='test revival rejection'");
  purchased_pet_revival(&owner,id);
  assert(!owner.followers && GET_GOLD(&owner)==15000 && !saved_players);
  sql("DROP TRIGGER reject_pet_revival");
  fail_gear=1; purchased_pet_revival(&owner,id); fail_gear=0;
  assert(!owner.followers && GET_GOLD(&owner)==15000 && !saved_players);
  purchased_pet_revival(&owner,id);
  assert(owner.followers && GET_GOLD(&owner)==0 && saved_players==1);
  assert(GET_LEVEL(&loaded)==30 && loaded.mob_specials.pet_experience==1234);
  assert(GET_HIT(&loaded)==GET_MAX_HIT(&loaded) && GET_HIT(&loaded)>0 && GET_POS(&loaded)==POS_STANDING);
  assert(loaded_objects>=2);
  assert(GET_GOLD(&loaded)==77);
  assert(GET_EQ(&loaded,WEAR_BODY) && GET_OBJ_VAL(GET_EQ(&loaded,WEAR_BODY),0)==101);
  assert(loaded.carrying && GET_OBJ_VAL(loaded.carrying,0)==202 && loaded.carrying->contains);
  assert(GET_OBJ_VAL(loaded.carrying->contains,0)==303);
  assert(GET_REAL_AC(&loaded)==220 && GET_HITROLL(&loaded)==5 && GET_DAMROLL(&loaded)==6);
  assert(loaded.mob_specials.damnodice==2 && loaded.mob_specials.damsizedice==6);
  GET_GOLD(&owner)=15000; purchased_pet_revival(&owner,id);
  assert(GET_GOLD(&owner)==15000 && saved_players==1);
  sql("SELECT COUNT(*) FROM pet_data WHERE owner_name='Trainer' AND pet_dead=1");
  result=mysql_store_result(conn); row=mysql_fetch_row(result); assert(atoi(row[0])==0); mysql_free_result(result);
  /* Recovering gear is owner-only, capacity checked, rollback safe, and consumable once. */
  assert(archive_fallen_pet(&loaded)); owner.followers=NULL; destroy_pet_holder(&loaded); save_char_pets(&owner);
  sql("SELECT pet_data_id FROM pet_data WHERE owner_name='Trainer' AND pet_dead=1");
  result=mysql_store_result(conn); row=mysql_fetch_row(result); assert(row);
  snprintf(id,sizeof(id),"%s",row[0]); mysql_free_result(result);
  owner.player.name="Other"; discard_fallen_pet(&owner,id); owner.player.name="Trainer"; assert(!owner.carrying);
  IS_CARRYING_W(&owner)=10000; discard_fallen_pet(&owner,id); IS_CARRYING_W(&owner)=0; assert(!owner.carrying);
  sql("CREATE TRIGGER reject_discard BEFORE DELETE ON pet_data FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='test discard rejection'");
  discard_fallen_pet(&owner,id); assert(!owner.carrying); sql("DROP TRIGGER reject_discard");
  sql("SELECT COUNT(*) FROM pet_save_objs"); result=mysql_store_result(conn); row=mysql_fetch_row(result);
  assert(atoi(row[0])==3); mysql_free_result(result);
  GET_GOLD(&owner)=MAX_GOLD; discard_fallen_pet(&owner,id); assert(!owner.carrying);
  GET_GOLD(&owner)=0;
  discard_fallen_pet(&owner,id); assert(inventory_saves==1 && !owner.followers && GET_GOLD(&owner)==77);
  int roots=0, contents=0;
  for (struct obj_data *o=owner.carrying;o;o=o->next_content) {
    roots++;
    if (GET_OBJ_VAL(o,0)==202) { assert(o->contains && GET_OBJ_VAL(o->contains,0)==303); contents++; }
    else assert(GET_OBJ_VAL(o,0)==101);
  }
  assert(roots==2 && contents==1);
  discard_fallen_pet(&owner,id); assert(inventory_saves==1);
  sql("SELECT COUNT(*) FROM pet_data WHERE owner_name='Trainer'");
  result=mysql_store_result(conn); row=mysql_fetch_row(result); assert(atoi(row[0])==0); mysql_free_result(result);
  while (owner.carrying) extract_obj(owner.carrying);
  /* Companion ranks award points without raising the creature's level. */
  follower.follower=&pet; follower.next=NULL; owner.followers=&follower;
  pet.master=&owner; pet.mob_specials.purchased_pet=FALSE; SET_BIT_AR(MOB_FLAGS(&pet),MOB_EIDOLON);
  GET_LEVEL(&pet)=12; GET_REAL_MAX_HIT(&pet)=100; GET_HIT(&pet)=20;
  GET_POS(&pet)=POS_FIGHTING; FIGHTING(&pet)=&victim; IN_ROOM(&pet)=IN_ROOM(&owner)=0;
  award_companion_training_exp(&owner,&victim,1999); assert(pet.mob_specials.pet_training_rank==0);
  award_companion_training_exp(&owner,&victim,2);
  assert(GET_LEVEL(&pet)==12 && pet.mob_specials.pet_training_rank==1 && pet.mob_specials.pet_training_experience==1);
  FIGHTING(&pet)=NULL; train_companion_skill(&owner,&pet,"health");
  assert(GET_REAL_MAX_HIT(&pet)==120 && GET_HIT(&pet)==20 && pet.mob_specials.pet_training_skills[0]==1);
  train_companion_skill(&owner,&pet,"health"); assert(GET_REAL_MAX_HIT(&pet)==120);
  FIGHTING(&pet)=&victim; award_companion_training_exp(&owner,&victim,INT_MAX); FIGHTING(&pet)=NULL;
  assert(GET_LEVEL(&pet)==12 && pet.mob_specials.pet_training_rank==10 && !pet.mob_specials.pet_training_experience);
  for (int n=0;n<4;n++) train_companion_skill(&owner,&pet,"health");
  train_companion_skill(&owner,&pet,"health"); assert(GET_REAL_MAX_HIT(&pet)==200 && GET_HIT(&pet)==20);
  train_companion_skill(&owner,&pet,"attack"); train_companion_skill(&owner,&pet,"damage");
  train_companion_skill(&owner,&pet,"armor"); train_companion_skill(&owner,&pet,"saves"); train_companion_skill(&owner,&pet,"saves");
  assert(pet.mob_specials.pet_training_skills[4]==2);
  train_companion_skill(&owner,&pet,"attack"); assert(pet.mob_specials.pet_training_skills[1]==1);
  show_companion_training(&owner,&pet);
  save_char_pets(&owner); owner.followers=NULL; load_char_pets(&owner);
  assert(GET_LEVEL(&loaded)==12 && companion_training_kind(&loaded)==4 && loaded.mob_specials.pet_training_rank==10);
  assert(loaded.mob_specials.pet_training_skills[0]==5 && loaded.mob_specials.pet_training_skills[4]==2);
  assert(GET_REAL_MAX_HIT(&loaded)==200 && GET_REAL_SAVE(&loaded,SAVING_FORT)==3);
  /* Class companion ranks, skill investments and gear survive paid revival too. */
  assert(archive_fallen_pet(&loaded)); owner.followers=NULL; destroy_pet_holder(&loaded); save_char_pets(&owner);
  sql("SELECT pet_data_id FROM pet_data WHERE owner_name='Trainer' AND pet_dead=1");
  result=mysql_store_result(conn); row=mysql_fetch_row(result); assert(row);
  snprintf(id,sizeof(id),"%s",row[0]); mysql_free_result(result);
  GET_GOLD(&owner)=6000; purchased_pet_revival(&owner,id);
  assert(GET_GOLD(&owner)==0 && GET_LEVEL(&loaded)==12 && companion_training_kind(&loaded)==4);
  assert(loaded.mob_specials.pet_training_rank==10 && loaded.mob_specials.pet_training_skills[0]==5);
  assert(GET_REAL_MAX_HIT(&loaded)==200 && GET_REAL_SAVE(&loaded,SAVING_FORT)==3);
  /* Familiar support and ridden mounts can train; temporary reflections cannot. */
  owner.followers=&follower; pet.mob_specials.pet_training_rank=0;
  pet.mob_specials.pet_training_experience=0;
  memset(pet.mob_specials.pet_training_skills,0,sizeof(pet.mob_specials.pet_training_skills));
  REMOVE_BIT_AR(MOB_FLAGS(&pet),MOB_EIDOLON); SET_BIT_AR(MOB_FLAGS(&pet),MOB_C_FAMILIAR);
  FIGHTING(&pet)=NULL; award_companion_training_exp(&owner,&victim,2000); assert(pet.mob_specials.pet_training_rank==1);
  pet.mob_specials.pet_training_rank=0; REMOVE_BIT_AR(MOB_FLAGS(&pet),MOB_C_FAMILIAR);
  SET_BIT_AR(MOB_FLAGS(&pet),MOB_C_MOUNT); RIDDEN_BY(&pet)=&owner;
  award_companion_training_exp(&owner,&victim,2000); assert(pet.mob_specials.pet_training_rank==1);
  pet.mob_specials.pet_training_rank=0; temporary_pet=&pet;
  award_companion_training_exp(&owner,&victim,2000); assert(pet.mob_specials.pet_training_rank==0 && !is_persistent_pet(&pet));
  temporary_pet=NULL;
  sql("DROP TABLE pet_data"); upgrade_pet_schema(); upgrade_pet_schema();
  mysql_close(conn); return 0;
}
'''
fight = (root/'src/fight.c').read_text()
death = function(fight, 'void raw_kill(struct char_data *ch, struct char_data *killer)')
archived = death.index('Its equipment and inventory are safely stored; no corpse or loot.')
assert archived < death.index('determine_treasure(') < death.index('make_corpse(')
assert 'extract_obj(unequip_char(ch, slot))' in death[:archived]
assert 'while (ch->carrying)' in death[:archived]

with tempfile.TemporaryDirectory(prefix='pet-progression-') as tmp:
    temp=Path(tmp); data=temp/'data'; data.mkdir(); socket=temp/'mysql.sock'; log=temp/'mysql.log'
    path=temp/'check.c'; path.write_text(headers+stubs+gear_code+r"""
bool pet_save_objs(struct char_data *pet,struct char_data *owner,long id) {
  save_objects++; return production_pet_save_objs(pet,owner,id);
}
bool pet_load_objs(struct char_data *pet,struct char_data *owner,long id) {
  loaded_objects++; return production_pet_load_objs(pet,owner,id);
}
"""+code+checks)
    flags=shlex.split(subprocess.check_output(['pkg-config','--cflags','--libs','mariadb'],text=True))
    binary=temp/'check'
    subprocess.run([os.environ.get('CC','cc'),'-std=gnu99','-Wall','-Wextra','-Wno-unused-parameter',
                    '-I'+str(root/'src'),str(path),*flags,'-o',str(binary)],check=True)
    subprocess.run(['mariadb-install-db','--no-defaults','--datadir='+str(data),
                    '--auth-root-authentication-method=normal','--skip-test-db'],
                   stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL,check=True)
    server=subprocess.Popen(['mariadbd','--no-defaults','--datadir='+str(data),'--socket='+str(socket),
                             '--pid-file='+str(temp/'pid'),'--skip-networking','--log-error='+str(log),
                             '--user='+os.environ.get('USER','krynn')],
                            stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
    try:
        client=['mariadb','--no-defaults','--socket='+str(socket),'-uroot']
        for attempt in range(100):
            ready=subprocess.run([*client,'-e','SELECT 1'],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
            if ready.returncode==0: break
            if server.poll() is not None: raise RuntimeError(log.read_text())
            time.sleep(.1)
        else: raise RuntimeError('Temporary MariaDB did not become ready')
        subprocess.run([*client,'-e','CREATE DATABASE pet_progression_test'],check=True)
        subprocess.run([str(binary),str(socket)],check=True)
    finally:
        server.terminate(); server.wait(timeout=15)
print('Persistent pets: combat XP, levels/caps, no healing, abuse exclusions, migration, real database save/load, fallen archives, paid revival, failure refunds, equipped/nested gear, safe discard, and companion training passed.')
