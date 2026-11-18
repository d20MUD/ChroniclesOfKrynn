#!/usr/bin/env python3
"""Exercise real flee entry points, Nimble Escape speed and combat cleanup."""
from pathlib import Path
import os
import subprocess
import tempfile
root=Path(__file__).resolve().parents[1]
def function(file, name):
    text=(root/file).read_text();a=text.index(name+'\n{');return text[a:text.index('\n}',a)+2]
headers=['conf.h','sysdep.h','structs.h','utils.h','spells.h','fight.h','interpreter.h','mud_event.h','actions.h']
code='\n'.join(f'#include "{name}"' for name in headers)+r'''
#include <assert.h>
#undef ACMD
#define ACMD(name) void name(struct char_data *ch,const char *argument,int cmd,int subcmd)
#undef act
#define act(...) ((void)0)
#define send_to_char(...) ((void)0)
#undef GUI_CMBT_OPEN
#undef GUI_CMBT_CLOSE
#undef GUI_CMBT_NOTVICT_OPEN
#undef GUI_CMBT_NOTVICT_CLOSE
#define GUI_CMBT_OPEN(...) ((void)0)
#define GUI_CMBT_CLOSE(...) ((void)0)
#define GUI_CMBT_NOTVICT_OPEN(...) ((void)0)
#define GUI_CMBT_NOTVICT_CLOSE(...) ((void)0)
#undef CAN_GO
#define CAN_GO(ch,dir) (open_exits[dir])
#undef CAN_SEE
#define CAN_SEE(...) true
#undef USE_MOVE_ACTION
#define USE_MOVE_ACTION(ch) (moves++, move_ready=false)
#define is_action_available(ch,action,show) move_ready
#define legendary_grounded(ch) grounded
#define char_has_mud_event(ch,event) stunned
#define has_bard_agile_disengage(ch) false
#define is_affected_by_agile_disengage(ch) false
#define call_magic(...) ((void)0)
#define in_encounter_room(ch) true
#define one_argument(argument,out,size) snprintf(out,size,"%s",argument)
#define get_perk_rank(...) 0
struct player_special_data dummy_mob;
struct config_data config_info;
struct room_data *world;
struct char_data *combat_list;
static struct char_data pc,enemy,other;
static struct player_special_data player;
static struct room_data rooms[2];
static bool open_exits[NUM_OF_DIRS],move_ready,grounded,stunned,move_success;
static int moves,moved_dir,stops,movement_calls;
const char *dirs[]={"north","east","south","west","up","down"};
int get_feat_value(struct char_data *ch,int feat) {return HAS_REAL_FEAT(ch,feat);}
int get_speed(struct char_data *ch,sbyte display) {(void)display;return ch==&pc?10:100;}
int MAX(int a,int b) {return a>b?a:b;}
int rand_number(int low,int high) {(void)high;return low;}
int search_block(char *arg,const char *const *list,bool exact) {
  (void)list;(void)exact;if(!strcmp(arg,"north")||!strcmp(arg,"n"))return NORTH;
  if(!strcmp(arg,"south"))return SOUTH;return -1;
}
int do_simple_move(struct char_data *ch,int dir,int mode) {
  assert(mode==3);movement_calls++;moved_dir=dir;
  if(move_success) IN_ROOM(ch)=1;return move_success;
}
void stop_fighting(struct char_data *ch) {FIGHTING(ch)=NULL;stops++;}
'''
code+='\n'+function('src/utils.c','bool can_flee_speed(struct char_data *ch)')
code+='\n'+function('src/fight.c','void perform_flee(struct char_data *ch)')
code+='\n'+function('src/fight.c','void perform_flee_direction(struct char_data *ch, int direction)')
code+='\n'+function('src/act.offensive.c','ACMD(do_flee)')
code+=r'''
static void fixture(void) {
  memset(&pc,0,sizeof(pc));memset(&enemy,0,sizeof(enemy));memset(&other,0,sizeof(other));
  memset(&player,0,sizeof(player));memset(rooms,0,sizeof(rooms));memset(open_exits,0,sizeof(open_exits));
  pc.player_specials=&player;pc.next_in_room=&enemy;world=rooms;rooms[0].people=&pc;
  GET_POS(&pc)=POS_FIGHTING;GET_HIT(&pc)=100;
  SET_BIT_AR(MOB_FLAGS(&enemy),MOB_ISNPC);SET_BIT_AR(MOB_FLAGS(&enemy),MOB_ENCOUNTER);
  FIGHTING(&pc)=&enemy;FIGHTING(&enemy)=FIGHTING(&other)=&pc;
  combat_list=&enemy;enemy.next_fighting=&other;
  open_exits[NORTH]=open_exits[SOUTH]=true;
  move_ready=move_success=true;grounded=stunned=false;
  moves=stops=movement_calls=0;moved_dir=-1;
}
int main(void) {
  fixture();assert(!can_flee_speed(&pc));
  HAS_REAL_FEAT(&pc,FEAT_NIMBLE_ESCAPE)=1;assert(can_flee_speed(&pc));
  do_flee(&pc,"south",0,0);
  assert(moved_dir==SOUTH && IN_ROOM(&pc)==1 && moves==1 && stops==3);
  assert(!FIGHTING(&pc)&&!FIGHTING(&enemy)&&!FIGHTING(&other));
  fixture();HAS_REAL_FEAT(&pc,FEAT_NIMBLE_ESCAPE)=1;
  do_flee(&pc,"",0,0);assert(moved_dir==NORTH && stops==3 && moves==1);
  fixture();HAS_REAL_FEAT(&pc,FEAT_NIMBLE_ESCAPE)=1;move_success=false;
  do_flee(&pc,"south",0,0);assert(!stops && FIGHTING(&pc)==&enemy && moves==1);
  fixture();HAS_REAL_FEAT(&pc,FEAT_NIMBLE_ESCAPE)=1;stunned=true;
  do_flee(&pc,"south",0,0);assert(!movement_calls && !moves);
  fixture();HAS_REAL_FEAT(&pc,FEAT_NIMBLE_ESCAPE)=1;move_ready=false;
  do_flee(&pc,"south",0,0);assert(!movement_calls && !moves);
  fixture();HAS_REAL_FEAT(&pc,FEAT_NIMBLE_ESCAPE)=1;open_exits[SOUTH]=false;
  do_flee(&pc,"south",0,0);assert(!movement_calls && !moves);
  fixture();HAS_REAL_FEAT(&pc,FEAT_NIMBLE_ESCAPE)=1;grounded=true;
  do_flee(&pc,"south",0,0);assert(!movement_calls && !moves);
  puts("PASS: Nimble Escape ignores encounter speed; random/directed flee share action checks, movement and complete combat cleanup, with no cleanup on failed movement.");
}
'''
with tempfile.TemporaryDirectory(prefix='nimble-escape-') as directory:
    cfile=Path(directory)/'escape.c';binary=Path(directory)/'escape';cfile.write_text(code)
    subprocess.run([os.environ.get('CC','cc'),'-std=gnu11','-g','-fsanitize=address,undefined',
                    '-fno-omit-frame-pointer','-I'+str(root/'src'),str(cfile),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
