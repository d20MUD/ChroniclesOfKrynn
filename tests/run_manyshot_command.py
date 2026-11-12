#!/usr/bin/env python3
"""Run native Manyshot command eligibility and feat/perk dispatch."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1]
source=(root/'src/act.offensive.c').read_text();a=source.index('ACMDCHECK(can_manyshot)');b=source.index('/* Callback for arrow swarm',a)
start=source.index('static bool try_opening_manyshot(');opening=source[start:source.index('\n}',start)+2]
code=r'''
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "interpreter.h"
#include "act.h"
#include "fight.h"
#include "db.h"
#include "mud_event.h"
#include "actions.h"
#include "spells.h"
#include "handler.h"
#include "perks.h"
#include <assert.h>
#undef ACMD
#define ACMD(name) ACMD_DECL(name)
#undef PREREQ_CAN_FIGHT
#undef PREREQ_NOT_PEACEFUL_ROOM
#undef PREREQ_CHECK
#undef USE_STANDARD_ACTION
#undef USE_FULL_ROUND_ACTION
#define PREREQ_CAN_FIGHT() ((void)0)
#define PREREQ_NOT_PEACEFUL_ROOM() ((void)0)
#define PREREQ_CHECK(f) if(f(ch,true)!=CAN_CMD)return
#define USE_STANDARD_ACTION(ch) (standards++)
#define USE_FULL_ROUND_ACTION(ch) (fulls++)
#define act(...) ((void)0)
#define send_to_char(...) ((void)0)
#define char_has_mud_event(ch,e) (cooldown)
#define attach_mud_event(e,t) (duration=(t))
#define new_mud_event(...) NULL
static bool perk,improved,feat,ammo_ready=true,ready_two=true,move=true,standard=true,cooldown,dead,moved,kill;
static int shots,fulls,standards,routines,duration;
static struct char_data target;
struct room_data *world;
static struct obj_data weapon;
int get_feat_value(struct char_data *ch,int id){return feat;}
bool has_perk(struct char_data *ch,int id){return id==PERK_RANGER_MANYSHOT?perk:improved;}
bool can_fire_ammo(struct char_data *ch,bool silent){return ammo_ready;}
bool manyshot_ready(struct char_data *ch,struct obj_data *obj){return ready_two;}
bool is_action_available(struct char_data *ch,action_type type,bool show){return type==atMOVE?move:standard;}
struct obj_data *get_wielded(struct char_data *ch,int type){return &weapon;}
const char *one_argument(const char *arg,char *out,size_t size){snprintf(out,size,"%s",arg);return NULL;}
struct char_data *get_char_vis(struct char_data *ch,char *arg,int *number,int where){return *arg?&target:NULL;}
struct char_data *find_char(long id){return dead||id>=100000000?NULL:&target;}
bool set_fighting(struct char_data *ch,struct char_data *vict){FIGHTING(ch)=vict;return true;}
int perform_attacks(struct char_data *ch,int mode,int phase){assert(!mode && !phase && FIRING(ch) && FIGHTING(ch)==&target);routines++;return 0;}
int hit(struct char_data *ch,struct char_data *vict,int type,int dam,int penalty,int attack){
 assert(attack==ATTACK_TYPE_RANGED);shots++;
 if(kill){dead=true;GET_POS(vict)=POS_DEAD;}
 if(moved)IN_ROOM(vict)=2;
 return 0;
}
'''+source[a:b]+opening+r'''
int main(void){
 struct room_data rooms[3]={{0}};world=rooms;world[1].people=&target;
 struct char_data ch={0};struct obj_data pouch={0},ammo={0};struct player_special_data ps={0};
 ch.player_specials=&ps;GET_EQ(&ch,WEAR_AMMO_POUCH)=&pouch;pouch.contains=&ammo;GET_OBJ_VAL(&ammo,0)=AMMO_TYPE_ARROW;
 IN_ROOM(&ch)=IN_ROOM(&target)=1;GET_POS(&target)=POS_STANDING;
 assert(can_manyshot(&ch,false)==CANT_CMD_PERM);
 feat=true;assert(can_manyshot(&ch,false)==CAN_CMD);
 do_manyshot(&ch,"target",0,0);assert(routines==1 && fulls==1 && !shots && !duration);
 move=false;do_manyshot(&ch,"target",0,0);assert(routines==1);move=true;
 ready_two=false;do_manyshot(&ch,"target",0,0);assert(routines==1);ready_two=true;
 SET_BIT_AR(AFF_FLAGS(&ch),AFF_STAGGERED);do_manyshot(&ch,"target",0,0);assert(routines==1);REMOVE_BIT_AR(AFF_FLAGS(&ch),AFF_STAGGERED);
 feat=false;perk=true;do_manyshot(&ch,"target",0,0);assert(shots==3 && standards==1 && duration==120*PASSES_PER_SEC);
 improved=true;do_manyshot(&ch,"target",0,0);assert(shots==8 && standards==2 && duration==60*PASSES_PER_SEC);
 cooldown=true;assert(can_manyshot(&ch,false)==CANT_CMD_TEMP);cooldown=false;
 ammo_ready=false;do_manyshot(&ch,"target",0,0);assert(shots==8);ammo_ready=true;
 GET_OBJ_VAL(&ammo,0)=AMMO_TYPE_BOLT;assert(can_manyshot(&ch,false)==CANT_CMD_TEMP);GET_OBJ_VAL(&ammo,0)=AMMO_TYPE_ARROW;
 // A kill/move in the first shot must stop a volley before another victim access.
 moved=true;do_manyshot(&ch,"target",0,0);assert(shots==9);moved=false;IN_ROOM(&target)=1;
 kill=true;do_manyshot(&ch,"target",0,0);assert(shots==10);kill=dead=false;GET_POS(&target)=POS_STANDING;
 GET_POS(&target)=POS_DEAD;do_manyshot(&ch,"target",0,0);assert(shots==10);GET_POS(&target)=POS_STANDING;
 // Both owned: command remains the perk volley; passive feat runs on normal full attacks.
 feat=true;do_manyshot(&ch,"target",0,0);assert(shots==15 && routines==1);
 // Fire dispatches the same real helper automatically, with safe single-shot fallbacks.
 perk=false;feat=true;FIGHTING(&ch)=NULL;shots=routines=fulls=0;
 assert(try_opening_manyshot(&ch,&target) && routines==1 && fulls==1);
 move=false;assert(!try_opening_manyshot(&ch,&target));move=true;
 ready_two=false;assert(!try_opening_manyshot(&ch,&target));ready_two=true;
 perk=true;shots=0;
 // DG script lookups reject high IDs; a valid room target must still fire.
 GET_ID(&target)=100000001;assert(!find_char(GET_ID(&target)));
 FIGHTING(&ch)=NULL;GET_ATTACKS_THIS_ROUND(&ch)=7;
 assert(try_opening_manyshot(&ch,&target) && shots==5);
 assert(!GET_ATTACKS_THIS_ROUND(&ch));
 world[1].people=NULL;int prior_duration=duration,prior_standards=standards;
 assert(!try_opening_manyshot(&ch,&target));
 assert(duration==prior_duration&&standards==prior_standards);
 world[1].people=&target;
 cooldown=true;assert(!try_opening_manyshot(&ch,&target));cooldown=false;
 ammo_ready=false;assert(!try_opening_manyshot(&ch,&target));ammo_ready=true;
 SET_BIT_AR(AFF_FLAGS(&ch),AFF_STAGGERED);assert(!try_opening_manyshot(&ch,&target));
 return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='manyshot-command-') as tmp:
 p=Path(tmp)/'test.c';binary=Path(tmp)/'test';p.write_text(code)
 subprocess.run(['cc','-fsanitize=address','-g','-std=gnu99','-I'+str(root/'src'),str(p),'-o',str(binary)],check=True)
 subprocess.run([str(binary)],check=True)
print('Manyshot command: feat full attacks, perk 3/5-shot volleys, actions, ammo, cooldown, target movement/death and combined ownership passed.')
