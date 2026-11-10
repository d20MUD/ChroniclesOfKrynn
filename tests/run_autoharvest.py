#!/usr/bin/env python3
"""Exercise automatic harvest scheduling through the production timer functions."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1]
craft=(root/'src/crafting_new.c').read_text()
def function(signature):
 a=craft.index(signature);return craft[a:craft.index('\n}',a)+2]
headers=['conf.h','sysdep.h','structs.h','utils.h','spells.h','comm.h','db.h','act.h','crafting_new.h']
code='\n'.join('#include "'+h+'"' for h in headers)+r'''
#include <assert.h>
#define HARVEST_BASE_TIME 20
#define HARVEST_BASE_DC 5
#define act(...) ((void)0)
#define send_to_char(...) ((void)0)
struct player_special_data dummy_mob;
struct room_data rooms[2],*world=rooms;
static int skill_value=30,completed,mode;
static bool tool=true;
void basic_mud_log(const char *format,...) {assert(0);}
int MAX(int a,int b) {return a>b?a:b;}
int harvesting_skill_by_material(int material) {return material?1:0;}
int material_grade(int material) {return 1;}
int get_craft_skill_value(struct char_data *ch,int skill) {return skill_value;}
int get_elbow_grease_bonus(struct char_data *ch) {return 0;}
int get_jack_of_all_trades_bonus(struct char_data *ch) {return 0;}
int get_proficient_talent_bonus(struct char_data *ch,int skill) {return 0;}
int get_rapid_talent_bonus(struct char_data *ch,int skill) {return 3;}
bool has_proper_harvesting_tool_equipped(struct char_data *ch) {return tool;}
void show_harvesting_tool_needed(struct char_data *ch) {}
const char *harvest_activity_name(room_rnum room) {return "harvesting";}
void harvest_complete(struct char_data *ch) {
 completed++;
 if(mode==2)return; /* Impossible skill check returns early in production. */
 if(mode!=1 && rooms[0].harvest_material_amount>0)rooms[0].harvest_material_amount--;
 if(mode==3)IN_ROOM(ch)=1;
 if(mode==4)tool=false;
 if(mode==5)rooms[0].harvest_material=CRAFT_MAT_MAPLE_WOOD;
 if(mode==6)rooms[0].harvest_material_amount=0;
 GET_CRAFT(ch).crafting_method=0;GET_CRAFT(ch).craft_duration=0;
}
'''+function('void newcraft_harvest(')+function('static void complete_harvest_cycle(')+r'''
static struct char_data ch,opponent;
static struct player_special_data player;
static void reset(void) {
 memset(&ch,0,sizeof(ch));memset(&player,0,sizeof(player));memset(rooms,0,sizeof(rooms));
 ch.player_specials=&player;IN_ROOM(&ch)=0;GET_POS(&ch)=POS_STANDING;GET_LEVEL(&ch)=1;
 GET_SURVEY_ROOMS(&ch)=10;rooms[0].harvest_material=CRAFT_MAT_ASH_WOOD;rooms[0].harvest_material_amount=3;
 SET_BIT_AR(PRF_FLAGS(&ch),PRF_AUTOHARVEST);mode=0;completed=0;skill_value=30;tool=true;
}
int main(void) {
 reset();newcraft_harvest(&ch,"");
 assert(GET_CRAFT((&ch)).craft_duration==17);
 for(int cycle=0;cycle<3;cycle++) {
  assert(GET_CRAFT((&ch)).crafting_method==SCMD_NEWCRAFT_HARVEST);
  complete_harvest_cycle(&ch);
  assert(rooms[0].harvest_material_amount==2-cycle);
  assert(GET_CRAFT((&ch)).craft_duration==(cycle<2?17:0));
 }
 assert(completed==3 && !GET_CRAFT((&ch)).crafting_method);
 reset();REMOVE_BIT_AR(PRF_FLAGS(&ch),PRF_AUTOHARVEST);complete_harvest_cycle(&ch);
 assert(completed==1 && !GET_CRAFT((&ch)).craft_duration && rooms[0].harvest_material_amount==2);
 reset();mode=1;complete_harvest_cycle(&ch); /* An ordinary failed roll repeats. */
 assert(GET_CRAFT((&ch)).craft_duration==17 && rooms[0].harvest_material_amount==3);
 reset();mode=2;skill_value=-20;GET_CRAFT((&ch)).crafting_method=SCMD_NEWCRAFT_HARVEST;complete_harvest_cycle(&ch);
 assert(!GET_CRAFT((&ch)).craft_duration && !GET_CRAFT((&ch)).crafting_method);
 for(int scenario=3;scenario<=6;scenario++) {reset();mode=scenario;complete_harvest_cycle(&ch);assert(!GET_CRAFT((&ch)).craft_duration);}
 reset();GET_SURVEY_ROOMS(&ch)=0;complete_harvest_cycle(&ch);assert(!GET_CRAFT((&ch)).craft_duration);
 reset();GET_POS(&ch)=POS_SLEEPING;complete_harvest_cycle(&ch);assert(!GET_CRAFT((&ch)).craft_duration);
 reset();FIGHTING(&ch)=&opponent;complete_harvest_cycle(&ch);assert(!GET_CRAFT((&ch)).craft_duration);
 reset();SET_BIT_AR(PRF_FLAGS(&ch),PRF_NO_CRAFT_PROGRESS);complete_harvest_cycle(&ch);assert(GET_CRAFT((&ch)).craft_duration==17);
 reset();GET_LEVEL(&ch)=LVL_IMMORT;complete_harvest_cycle(&ch);assert(GET_CRAFT((&ch)).craft_duration==1);
 puts("Autoharvest: repeat to depletion, failed rolls, normal timing, toggle off, impossible skill, changed/depleted node, movement, missing tool/survey and combat/rest stops passed.");
}
'''
# The timer invokes the repeat path only when an active task completes.
update=function('void craft_update(')
assert update.count('complete_harvest_cycle(ch);')==1
assert update.index('if (GET_CRAFT(ch).craft_duration == 0)')<update.index('complete_harvest_cycle(ch);')
assert 'SCMD_AUTOHARVEST' in (root/'src/interpreter.c').read_text()
prefs=(root/'src/prefedit.c').read_text()
assert 'ONOFF(PREFEDIT_FLAGGED(PRF_AUTOHARVEST))' in prefs
assert 'TOGGLE_BIT_AR(PREFEDIT_GET_FLAGS, PRF_AUTOHARVEST)' in prefs
with tempfile.TemporaryDirectory(prefix='autoharvest-') as directory:
 p=Path(directory);(p/'test.c').write_text(code)
 subprocess.run(['cc','-std=gnu11','-g','-fsanitize=address,undefined','-fno-pie','-no-pie','-I'+str(root/'src'),str(p/'test.c'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
