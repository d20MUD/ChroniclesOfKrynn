#include "clan_policy.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static const char live[] = "{\"version\":1,\"mode\":\"live\",\"buffs\":true,\"contracts\":true,\"expeditions\":true,\"quartermaster\":true,\"web\":true,\"public_release\":true,\"pilot_clans\":[]}";
static void invalid(const char *text)
{
  struct clan_progression_config c, saved;
  char reason[256];
  memset(&c,0x5a,sizeof(c));saved=c;
  assert(!clan_policy_parse(text,strlen(text),true,&c,reason,sizeof(reason)));
  assert(*reason && !memcmp(&c,&saved,sizeof(c)));
}
int main(int argc,char **argv)
{
  struct clan_progression_config c, saved;
  struct clan_progression_readiness ready={true,true,true,true,CLAN_PROGRESSION_SCHEMA_VERSION};
  char reason[256],path[1024],oversized[CLAN_POLICY_MAX_BYTES+2];
  bool present=true;
  FILE *f;
  assert(argc==2);
  assert(clan_policy_parse(live,strlen(live),true,&c,reason,sizeof(reason)));
  assert(c.controls.mode==CLAN_PROGRESSION_LIVE && c.controls.store_mode==CLAN_STORE_MYSQL);
  assert(c.controls.buffs && c.controls.contracts && c.controls.expeditions && c.controls.web && c.controls.quartermaster);
  assert(clan_progression_feature_available(&c,CLAN_FEATURE_EARNINGS,&ready,"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));
  saved=c;
  assert(!clan_policy_parse(live,strlen(live),false,&c,reason,sizeof(reason)) && !memcmp(&c,&saved,sizeof(c)));
  invalid("{}");invalid("[]");invalid("null");invalid("{broken");
  invalid("{\"version\":1,\"mode\":\"live\",\"buffs\":1,\"contracts\":true,\"expeditions\":true,\"quartermaster\":true,\"web\":true,\"public_release\":true,\"pilot_clans\":[]}");
  invalid("{\"version\":1,\"mode\":\"off\",\"buffs\":true,\"contracts\":true,\"expeditions\":true,\"quartermaster\":true,\"web\":true,\"public_release\":true,\"pilot_clans\":[]}");
  invalid("{\"version\":1,\"mode\":\"live\",\"buffs\":true,\"contracts\":false,\"expeditions\":true,\"quartermaster\":true,\"web\":true,\"public_release\":true,\"pilot_clans\":[]}");
  invalid("{\"version\":1,\"mode\":\"live\",\"buffs\":true,\"contracts\":true,\"expeditions\":true,\"quartermaster\":true,\"web\":true,\"public_release\":true,\"pilot_clans\":[\"not-a-clan\"]}");
  invalid("{\"version\":1,\"mode\":\"live\",\"buffs\":true,\"contracts\":true,\"expeditions\":true,\"quartermaster\":true,\"web\":true,\"public_release\":false,\"pilot_clans\":[\"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\",\"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\"]}");
  snprintf(path,sizeof(path),"%s/missing.json",argv[1]);
  assert(clan_policy_load(path,true,&c,&present,reason,sizeof(reason)) && !present);
  assert(c.controls.mode==CLAN_PROGRESSION_OFF && !c.controls.buffs && !c.controls.public_release);
  snprintf(path,sizeof(path),"%s/policy.json",argv[1]);
  f=fopen(path,"wb");assert(f);assert(fwrite(live,1,strlen(live),f)==strlen(live));assert(!fclose(f));
  assert(clan_policy_load(path,true,&c,&present,reason,sizeof(reason)) && present && c.controls.buffs);
  memset(oversized,' ',sizeof(oversized));f=fopen(path,"wb");assert(f);
  assert(fwrite(oversized,1,sizeof(oversized),f)==sizeof(oversized));assert(!fclose(f));saved=c;
  assert(!clan_policy_load(path,true,&c,&present,reason,sizeof(reason)) && !memcmp(&c,&saved,sizeof(c)));
  invalid("{\"version\":1,\"mode\":\"live\",\"buffs\":true,\"contracts\":true,\"expeditions\":true,\"quartermaster\":true,\"web\":true,\"public_release\":true,\"pilot_clans\":[]} trailing");
  puts("Clan startup policy: explicit enablement, SQL prerequisite, disabled missing defaults, strict types/keys, dependencies, identity scope, bounded files and failure atomicity passed.");
  return 0;
}
