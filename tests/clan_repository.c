#include "clan_repository.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define A "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
#define B "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
static void sql(MYSQL *db,const char *text){if(mysql_query(db,text)){fprintf(stderr,"%s\n",mysql_error(db));abort();}}
int main(int argc,char **argv)
{
 MYSQL *db=mysql_init(NULL);struct clan_repository_record r={0},out,before;char uid[33];int i;
 assert(argc==2);assert(mysql_real_connect(db,NULL,"root","","clan_test",0,argv[1],0));
 sql(db,"SET SESSION sql_mode='STRICT_ALL_TABLES,NO_ENGINE_SUBSTITUTION'");
 assert(clan_repository_schema_check(db)==CLAN_REPO_OK);
 assert(clan_repository_load(db,A,&out)==CLAN_REPO_NOT_FOUND);
 for(i=0;i<20;i++)assert(clan_repository_uid_generate(uid)&&clan_progression_uid_valid(uid));
 strcpy(r.uid,A);strcpy(r.name,"Clan 'quoted' ");strcpy(r.description,"colors \tR and \\ and ' and %\nparagraph");strcpy(r.abbreviation,"TEST");
 r.vnum=7;r.status=CLAN_RECORD_ACTIVE;r.created_at=r.updated_at=5000000000;r.founded_at=4000000000;r.leader_id=6000000000;r.treasury_gold=INT64_MAX;r.state_updated_at=5000000000;
 assert(clan_repository_create(db,&r)==CLAN_REPO_OK);
 assert(clan_repository_load(db,A,&out)==CLAN_REPO_OK);assert(!memcmp(&r,&out,sizeof(r)));
 assert(clan_repository_load_vnum(db,7,&out)==CLAN_REPO_OK);
 assert(clan_repository_create(db,&r)==CLAN_REPO_CONFLICT);
 memset(r.description,'D',49100);r.description[49100]=0;
 r.xp_eighths=INT64_MAX;r.influence_eighths=INT64_MAX;r.treasury_gold=123;r.updated_at++;
 assert(clan_repository_save(db,&r,0,0)==CLAN_REPO_OK);
 assert(clan_repository_load(db,A,&out)==CLAN_REPO_OK);assert(out.revision==1&&out.state_revision==1&&out.xp_eighths==INT64_MAX&&out.influence_eighths==INT64_MAX&&out.treasury_gold==123);
 assert(strlen(out.description)==49100);
 before=out;assert(clan_repository_save(db,&r,0,0)==CLAN_REPO_CONFLICT);
 r.treasury_gold=-1;assert(clan_repository_save(db,&r,1,1)==CLAN_REPO_INVALID);r.treasury_gold=123;
 assert(clan_repository_save(db,&r,INT64_MAX,1)==CLAN_REPO_INVALID);
 sql(db,"CREATE TRIGGER fail_state BEFORE UPDATE ON clan_progression_state FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='test interruption'");
 r.treasury_gold=456;assert(clan_repository_save(db,&r,1,1)==CLAN_REPO_INVALID);
 assert(clan_repository_load(db,A,&out)==CLAN_REPO_OK&&!memcmp(&out,&before,sizeof(out)));sql(db,"DROP TRIGGER fail_state");
 r.status=CLAN_RECORD_DELETED;r.deleted_at=r.updated_at;assert(clan_repository_save(db,&r,1,1)==CLAN_REPO_OK);
 assert(clan_repository_save(db,&r,2,2)==CLAN_REPO_DELETED);
 assert(clan_repository_load_vnum(db,7,&out)==CLAN_REPO_NOT_FOUND);
 strcpy(r.uid,B);r.status=CLAN_RECORD_ACTIVE;r.deleted_at=0;r.revision=r.state_revision=r.xp_eighths=r.influence_eighths=0;
 assert(clan_repository_create(db,&r)==CLAN_REPO_OK);
 assert(clan_repository_load_vnum(db,7,&out)==CLAN_REPO_OK&&!strcmp(out.uid,B)&&out.xp_eighths==0);
 sql(db,"DROP TRIGGER cp_state_history_no_delete");assert(clan_repository_schema_check(db)==CLAN_REPO_SCHEMA);
 assert(clan_repository_load(db,A,&out)==CLAN_REPO_SCHEMA);
 assert(clan_repository_schema_check(NULL)==CLAN_REPO_UNAVAILABLE);
 mysql_close(db);puts("Clan repository: core/state round trips, strict int64 boundaries, quoted text, create/save conflicts, rollback, tombstones/vnum reuse and missing-trigger failure passed.");return 0;
}
