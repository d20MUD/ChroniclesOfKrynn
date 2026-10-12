#!/usr/bin/env python3
"""Exercise production climate nodes, equivalent materials, persistence and exchange."""
from pathlib import Path
import os
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
craft = (root / 'src/crafting_new.c').read_text()
constants = (root / 'src/constants.c').read_text()
players = (root / 'src/players.c').read_text()

def function(source, name):
    start = re.search(r'^(?:static )?(?:void|int|bool|const char \*)\s*' + name + r'\([^;]*?\)\n\{', source, re.M).start()
    brace = source.index('\n{', start)
    return source[start:source.index('\n}', brace) + 2] + '\n'

code = ''
for name in ['material_name', 'crafting_materials', 'crafting_material_descriptions',
             'crafting_material_nodes', 'harvesting_messages']:
    start = constants.index('const char *' + name + '[]')
    code += constants[start:constants.index('CHECK_TABLE_SIZE', start)]
for name in ['canonical_craft_material', 'material_grade', 'craft_group_by_material',
             'craft_material_level_adjustment', 'harvesting_skill_by_material',
             'craft_material_to_obj_material', 'obj_material_to_craft_material',
             'material_type_to_crafting_skill', 'climate_material_variety',
             'harvest_node_name', 'harvest_activity_name',
             'determine_harvest_material_for_room', 'determine_material_type_by_group_and_grade',
             'determine_random_material_group_by_sector_type', 'determine_grade_by_zone_level',
             'determine_random_grade', 'exchangeable_material', 'exchange_material_by_name',
             'exchange_material_varieties']:
    code += function(craft, name)
code += function(players, 'load_craft_mats_onhand')
start = players.index('  // Save Craft mats onhand')
end = players.index('  // Save Craft motes onhand', start)
code += ('static void write_balances(FILE *fp, struct char_data *ch) {\nint i;\n'
         '#define BUFFER_WRITE(...) fprintf(fp, __VA_ARGS__)\n' + players[start:end]
         + '\n#undef BUFFER_WRITE\n}\n')
start = craft.index('SPECIAL(materials_exchanger)\n{')
code += craft[start:craft.index('\n}', start) + 2]

headers = '\n'.join('#include "' + name + '"' for name in
                    ['conf.h', 'sysdep.h', 'structs.h', 'utils.h', 'comm.h', 'db.h',
                     'constants.h', 'spells.h', 'interpreter.h', 'crafting_new.h', 'craft.h'])
stubs = r'''
#include <assert.h>
struct room_data *world;
struct zone_data *zone_table;
room_rnum top_of_world;
struct player_special_data dummy_mob;
static int die, saves, visible=1;
static const char *command="exchange";
static char output[32768];
void PERF_prof_sect_init(struct PERF_prof_sect **ptr,const char *id) {}
void PERF_prof_sect_enter(struct PERF_prof_sect *ptr) {}
void PERF_prof_sect_exit(struct PERF_prof_sect *ptr) {}
#undef CMD_IS
#define CMD_IS(name) (!strcmp(command,name))
#undef CAN_SEE
#define CAN_SEE(ch,mob) visible
int MAX(int a,int b) { return a>b?a:b; }
int MIN(int a,int b) { return a<b?a:b; }
int dice(int count,int sides) { return 1 + (die % sides); }
int is_abbrev(const char *arg,const char *name) { return *arg && !strncasecmp(arg,name,strlen(arg)); }
int get_line(FILE *fp,char *line) { return fgets(line,MAX_INPUT_LENGTH,fp)!=NULL; }
void save_char(struct char_data *ch,int mode) { saves++; }
size_t send_to_char(struct char_data *ch,const char *fmt,...) {
  va_list ap; va_start(ap,fmt); int n=vsnprintf(output+strlen(output),sizeof(output)-strlen(output),fmt,ap);
  va_end(ap); assert(n>=0 && n<(int)(sizeof(output)-strlen(output))); return n;
}
static void exchange(struct char_data *ch,const char *arg) {
  output[0]=0; exchange_material_varieties(ch,arg);
}
'''
checks = r'''
int main(void) {
  struct char_data ch={0}, mob={0}; struct player_special_data ps={0}, loaded={0};
  struct room_data room={0}; struct zone_data zone={0};
  world=&room; top_of_world=0; zone_table=&zone; ch.player_specials=&ps;
  zone.min_level=zone.max_level=15;
  assert(CRAFT_MAT_MAPLE_WOOD==22 && CRAFT_MAT_MAHAGONY_WOOD==23 && CRAFT_MAT_CATALYST_FRAGMENT==39);
  assert(MATERIAL_MAPLE==52 && MATERIAL_DRAGONBLOOD==59);
  assert(sizeof(crafting_materials)/sizeof(*crafting_materials)==NUM_CRAFT_MATS+1);
  assert(sizeof(material_name)/sizeof(*material_name)==NUM_MATERIALS+1);
  for(int id=CRAFT_MAT_BAMBOO;id<NUM_CRAFT_MATS;id++) {
    int base=canonical_craft_material(id), object=craft_material_to_obj_material(id);
    assert(base!=id && material_grade(id)==material_grade(base));
    assert(craft_material_level_adjustment(id)==craft_material_level_adjustment(base));
    assert(craft_group_by_material(id)==craft_group_by_material(base));
    assert(harvesting_skill_by_material(id)==harvesting_skill_by_material(base));
    assert(object>=MATERIAL_BAMBOO && object<NUM_MATERIALS);
    assert(obj_material_to_craft_material(object)==id);
    assert(material_type_to_crafting_skill(object)==(base==CRAFT_MAT_HEMP ? ABILITY_CRAFT_TAILORING : ABILITY_CRAFT_WOODWORKING));
    assert(base==CRAFT_MAT_HEMP ? IS_CLOTH(object) : IS_WOOD(object));
  }
  const int timber[]={CRAFT_MAT_ASH_WOOD,CRAFT_MAT_MAPLE_WOOD,CRAFT_MAT_MAHAGONY_WOOD,CRAFT_MAT_VALENWOOD,CRAFT_MAT_IRONWOOD};
  for(int climate=0;climate<NUM_ZONE_CLIMATES;climate++)
    for(int g=0;g<5;g++)
      for(die=0;die<12;die++) {
        int id=climate_material_variety(timber[g],climate,ZONE_TERRAIN_FOREST);
        assert(material_grade(id)==g+1 && craft_group_by_material(id)==CRAFT_GROUP_WOOD);
        assert(climate || id==timber[g]);
        if(g>=3) assert(id==timber[g]);
      }
  assert(climate_material_variety(CRAFT_MAT_MAHAGONY_WOOD,ZONE_CLIMATE_TEMPERATE,ZONE_TERRAIN_FOREST)==CRAFT_MAT_OAK);
  assert(climate_material_variety(CRAFT_MAT_MAPLE_WOOD,ZONE_CLIMATE_SUBARCTIC,ZONE_TERRAIN_FOREST)==CRAFT_MAT_BIRCH);
  die=0;
  assert(climate_material_variety(CRAFT_MAT_MAHAGONY_WOOD,ZONE_CLIMATE_TROPICAL,ZONE_TERRAIN_JUNGLE)==CRAFT_MAT_MAHAGONY_WOOD);
  assert(climate_material_variety(CRAFT_MAT_MAPLE_WOOD,ZONE_CLIMATE_TROPICAL,ZONE_TERRAIN_SWAMP)==CRAFT_MAT_CYPRESS);
  assert(climate_material_variety(CRAFT_MAT_HEMP,ZONE_CLIMATE_TROPICAL,ZONE_TERRAIN_PLAINS)==CRAFT_MAT_JUTE);
  assert(climate_material_variety(CRAFT_MAT_HEMP,ZONE_CLIMATE_ARID,ZONE_TERRAIN_DESERT)==CRAFT_MAT_AGAVE);
  for(int c=0;c<NUM_ZONE_CLIMATES;c++) {
    assert(climate_material_variety(CRAFT_MAT_IRON,c,ZONE_TERRAIN_MOUNTAINS)==CRAFT_MAT_IRON);
    assert(climate_material_variety(CRAFT_MAT_DRAGONBONE,c,ZONE_TERRAIN_UNDERGROUND)==CRAFT_MAT_DRAGONBONE);
    assert(climate_material_variety(CRAFT_MAT_OAK,c,ZONE_TERRAIN_UNDERGROUND)==CRAFT_MAT_NONE);
    assert(climate_material_variety(CRAFT_MAT_IRONWOOD,c,ZONE_TERRAIN_URBAN)==CRAFT_MAT_NONE);
  }
  assert(climate_material_variety(-1,1,1)==CRAFT_MAT_NONE);
  assert(climate_material_variety(NUM_CRAFT_MATS,1,1)==CRAFT_MAT_NONE);
  assert(climate_material_variety(CRAFT_MAT_MAPLE_WOOD,999,1)==CRAFT_MAT_MAPLE_WOOD);
  /* Full room generator, not just the mapping table. High-end grade stays bounded. */
  room.sector_type=SECT_FOREST; zone.climate=ZONE_CLIMATE_TEMPERATE; zone.terrain=ZONE_TERRAIN_FOREST;
  bool found_oak=false;
  for(die=0;die<100;die++) {
    int id=determine_harvest_material_for_room(0);
    assert(id>0 && id<NUM_CRAFT_MATS && material_grade(id)<=4);
    if(id==CRAFT_MAT_OAK) found_oak=true;
    assert(id!=CRAFT_MAT_MAHAGONY_WOOD);
  }
  assert(found_oak && determine_harvest_material_for_room(NOWHERE)==CRAFT_MAT_NONE);
  /* Display uses the material's species, and hunting/mining retain actual yields. */
  room.harvest_material=CRAFT_MAT_OAK;
  assert(strstr(harvest_node_name(0),"oak") && strstr(harvest_activity_name(0),"logging"));
  zone.climate=ZONE_CLIMATE_TROPICAL; room.harvest_material=CRAFT_MAT_HIGH_GRADE_HIDE;
  assert(strstr(harvest_node_name(0),"antelope") && strstr(harvest_node_name(0),"high grade hide"));
  zone.climate=ZONE_CLIMATE_POLAR; room.harvest_material=CRAFT_MAT_BIRCH;
  assert(strstr(harvest_node_name(0),"birch driftwood"));
  room.harvest_material=CRAFT_MAT_IRON;
  assert(strstr(harvest_node_name(0),"ice-rimed") && strstr(harvest_node_name(0),"iron"));
  zone.climate=0; assert(!strcmp(harvest_node_name(0),crafting_material_nodes[CRAFT_MAT_IRON]));
  assert(!strcmp(harvest_node_name(NOWHERE),"an unknown resource"));
  /* Balances survive actual writer/loader; old 40-entry pfiles leave new IDs zero. */
  for(int id=0;id<NUM_CRAFT_MATS;id++) GET_CRAFT_MAT((&ch),id)=id*7;
  FILE *fp=tmpfile(); assert(fp); write_balances(fp,&ch); rewind(fp);
  char line[MAX_INPUT_LENGTH]; assert(fgets(line,sizeof(line),fp) && !strcmp(line,"CfMt:\n"));
  ch.player_specials=&loaded; load_craft_mats_onhand(fp,&ch); fclose(fp);
  for(int id=0;id<NUM_CRAFT_MATS;id++) assert(GET_CRAFT_MAT((&ch),id)==id*7);
  memset(&loaded,0,sizeof(loaded)); fp=tmpfile();
  for(int id=0;id<40;id++) fprintf(fp,"%d\n",id);
  fprintf(fp,"-1\nNEXT\n"); rewind(fp);
  load_craft_mats_onhand(fp,&ch); assert(fgets(line,sizeof(line),fp) && !strcmp(line,"NEXT\n")); fclose(fp);
  assert(GET_CRAFT_MAT((&ch),39)==39);
  for(int id=40;id<NUM_CRAFT_MATS;id++) assert(!GET_CRAFT_MAT((&ch),id));
  fp=tmpfile(); for(int id=0;id<NUM_CRAFT_MATS+10;id++) fprintf(fp,"%d\n",id==2?-9:id);
  fprintf(fp,"-1\nNEXT\n"); rewind(fp); load_craft_mats_onhand(fp,&ch);
  assert(!GET_CRAFT_MAT((&ch),2) && fgets(line,sizeof(line),fp) && !strcmp(line,"NEXT\n")); fclose(fp);
  /* Trades conserve units; rejection never mutates balances or saves a player. */
  memset(&loaded,0,sizeof(loaded)); GET_CRAFT_MAT((&ch),CRAFT_MAT_MAPLE_WOOD)=25;
  exchange(&ch,"10 maple for birch"); assert(saves==1);
  assert(GET_CRAFT_MAT((&ch),CRAFT_MAT_MAPLE_WOOD)==15 && GET_CRAFT_MAT((&ch),CRAFT_MAT_BIRCH)==10);
  exchange(&ch,"  5 \"BIRCH WOOD\" FOR \"maple wood\"  "); assert(saves==2);
  assert(GET_CRAFT_MAT((&ch),CRAFT_MAT_MAPLE_WOOD)==20 && GET_CRAFT_MAT((&ch),CRAFT_MAT_BIRCH)==5);
  GET_CRAFT_MAT((&ch),CRAFT_MAT_HEMP)=10; exchange(&ch,"4 hemp for jute"); assert(saves==3);
  assert(GET_CRAFT_MAT((&ch),CRAFT_MAT_HEMP)==6 && GET_CRAFT_MAT((&ch),CRAFT_MAT_JUTE)==4);
  const char *bad[]={"0 maple for birch","-1 maple for birch","1x maple for birch","999999999999999999999 maple for birch",
    "21 maple for birch","1 maple for oak","1 maple for maple","1 maple for copper","1 mithril for adamantite",
    "1 dragonbone for ironwood","1 bamboo for jute","1 ma for birch","1 maple for mystery","1 maple birch"};
  struct player_special_data before=loaded;
  for(size_t i=0;i<sizeof(bad)/sizeof(*bad);i++) { exchange(&ch,bad[i]); assert(!memcmp(&loaded,&before,sizeof(loaded)) && saves==3); }
  GET_CRAFT_MAT((&ch),CRAFT_MAT_BIRCH)=INT_MAX; exchange(&ch,"1 maple for birch");
  assert(GET_CRAFT_MAT((&ch),CRAFT_MAT_MAPLE_WOOD)==20 && saves==3);
  FIGHTING(&ch)=&mob; exchange(&ch,"1 maple for cedar"); assert(saves==3); FIGHTING(&ch)=NULL;
  exchange(&ch,"list"); assert(strstr(output,"birch wood") && strstr(output,"Grade 2") && saves==3);
  SET_BIT_AR(MOB_FLAGS(&mob),MOB_ISNPC); GET_POS(&mob)=POS_STANDING;
  assert(materials_exchanger(&ch,&mob,1,"1 maple for cedar")==1 && saves==4);
  command="look"; assert(!materials_exchanger(&ch,&mob,1,"")); command="exchange";
  assert(!materials_exchanger(&ch,&mob,0,""));
  visible=0; assert(!materials_exchanger(&ch,&mob,1,"")); visible=1;
  GET_POS(&mob)=POS_SLEEPING; assert(!materials_exchanger(&ch,&mob,1,""));
  return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='climate-materials-') as tmp:
    source = Path(tmp) / 'check.c'
    source.write_text(headers + stubs + code + checks)
    binary = Path(tmp) / 'check'
    subprocess.run([os.environ.get('CC', 'cc'), '-std=gnu99', '-Wall', '-Wextra',
                    '-Wno-unused-parameter', '-I' + str(root / 'src'), str(source), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
assert '"Materials Exchanger", materials_exchanger' in (root / 'src/spec_assign.c').read_text()
print('Climate materials: all climates/grades, mechanics, object round trips, legacy/new balances, descriptions, trades and spec dispatch passed.')
