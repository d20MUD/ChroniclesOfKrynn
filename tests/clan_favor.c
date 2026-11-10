#include "clan_repository.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#define A "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
#define B "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
static void sql(MYSQL *db,const char *s){assert(!mysql_query(db,s));}
int main(int argc,char **argv)
{
 MYSQL *db=mysql_init(NULL);struct clan_favor_state a={0},b={0},out={0};struct clan_buff_state buffs[125],original;size_t count=99;
 assert(argc==2);assert(mysql_real_connect(db,NULL,"root","","clan_test",0,argv[1],0));
 sql(db,"SET SESSION sql_mode='STRICT_ALL_TABLES,NO_ENGINE_SUBSTITUTION'");
 a.character_id=INT64_C(5000000007);strcpy(a.clan_uid,A);a.favor_eighths=801;
 out.character_id=123;
 assert(clan_repository_favor_load(db,a.character_id,A,&out)==CLAN_REPO_NOT_FOUND && out.character_id==123);
 assert(clan_repository_favor_save(db,&a,-1,100)==CLAN_REPO_OK);
 assert(clan_repository_favor_save(db,&a,-1,100)==CLAN_REPO_CONFLICT);
 assert(clan_repository_favor_load(db,a.character_id,A,&out)==CLAN_REPO_OK && out.favor_eighths==801 && out.revision==0);
 a=out;a.favor_eighths=800;assert(clan_repository_favor_save(db,&a,0,101)==CLAN_REPO_CONFLICT);
 a.favor_eighths=801;assert(clan_repository_favor_save(db,&a,0,99)==CLAN_REPO_CONFLICT);
 /* Purchase-related saves cannot reduce cumulative Favor. Equal is allowed. */
 assert(clan_repository_favor_save(db,&a,0,101)==CLAN_REPO_OK);
 assert(clan_repository_favor_save(db,&a,0,102)==CLAN_REPO_CONFLICT);
 b=a;strcpy(b.clan_uid,B);b.favor_eighths=INT64_MAX;b.revision=0;
 assert(clan_repository_favor_save(db,&b,-1,101)==CLAN_REPO_OK);
 sql(db,"DELETE FROM clan_memberships WHERE player_id=5000000007");
 assert(clan_repository_favor_load(db,a.character_id,A,&out)==CLAN_REPO_OK && out.favor_eighths==801 && out.revision==1);
 assert(clan_repository_favor_load(db,a.character_id,B,&out)==CLAN_REPO_OK && out.favor_eighths==INT64_MAX);
 /* New connection models logout/restart; identity doesn't depend on membership. */
 mysql_close(db);db=mysql_init(NULL);assert(mysql_real_connect(db,NULL,"root","","clan_test",0,argv[1],0));sql(db,"SET SESSION sql_mode='STRICT_ALL_TABLES,NO_ENGINE_SUBSTITUTION'");
 assert(clan_repository_favor_load(db,a.character_id,A,&out)==CLAN_REPO_OK && out.favor_eighths==801);
 assert(mysql_query(db,"DELETE FROM clan_personal_favor WHERE player_id=5000000007")!=0);
 assert(mysql_query(db,"UPDATE clan_personal_favor SET favor_eighths=0,revision=revision+1 WHERE clan_uid='" A "'")!=0);
 sql(db,"UPDATE clan_progression_clans SET record_status=2,deleted_at_utc=2200000001,updated_at_utc=2200000001 WHERE clan_uid='" A "'");
 assert(clan_repository_favor_load(db,a.character_id,A,&out)==CLAN_REPO_OK && out.favor_eighths==801);
 a.revision=1;assert(clan_repository_favor_save(db,&a,1,102)==CLAN_REPO_DELETED);
 {
  struct clan_repository_record replacement;
  assert(clan_repository_load(db,B,&replacement)==CLAN_REPO_OK);
  strcpy(replacement.uid,"cccccccccccccccccccccccccccccccc");replacement.vnum=7;
  replacement.revision=0;replacement.state_revision=0;replacement.xp_eighths=0;replacement.influence_eighths=0;
  assert(clan_repository_create(db,&replacement)==CLAN_REPO_OK);
  assert(clan_repository_favor_load(db,a.character_id,replacement.uid,&out)==CLAN_REPO_NOT_FOUND);
  assert(clan_repository_favor_load(db,a.character_id,A,&out)==CLAN_REPO_OK && out.favor_eighths==801);
 }
 assert(clan_repository_buffs_read(db,buffs,125,&count)==CLAN_REPO_OK && count==2);
 original=buffs[0];
 assert(!clan_repository_buff_active(&original,199));assert(clan_repository_buff_active(&original,200));
 assert(clan_repository_buff_active(&original,299));assert(!clan_repository_buff_active(&original,300));
 assert(original.next_available_at==500);original.canceled=true;assert(!clan_repository_buff_active(&original,250));
 assert(clan_repository_buffs_read(db,buffs,125,&count)==CLAN_REPO_OK && buffs[0].next_available_at==500);
 sql(db,"UPDATE clan_buff_families SET rules_version=2 WHERE clan_uid='" B "' AND family_id=1");count=99;buffs[0]=original;
 assert(clan_repository_buffs_read(db,buffs,125,&count)==CLAN_REPO_SCHEMA && count==99 && buffs[0].canceled);
 mysql_close(db);puts("Favor: independent former-clan standings, restart, monotonic CAS, int64, retained tombstones; buff expiry retains cooldown history passed");return 0;
}
