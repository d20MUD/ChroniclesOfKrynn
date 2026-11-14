#!/usr/bin/env python3
"""Exercise the actual directional guard loop, including clan-only bypass rules."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1]
s=(root/'src/movement.c').read_text();a=s.index('  for (mob = world[IN_ROOM(ch)].people;');b=s.index('\n  if (block && !IS_NPC(ch)',a)
code=r'''
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "clan_access.h"
#include <assert.h>
struct room_data room;
struct room_data *world=&room;
struct player_special_data dummy_mob;
#define act(...) ((void)0)
static int clan=7,rank=2,favor=3;
struct sivak_form *active_sivak_form(struct char_data *ch){return NULL;}
bool has_ghost(struct char_data *ch){return true;}
bool clan_access_check(struct char_data *ch,const struct clan_requirement *r,bool explain){
 return (!r->clan || r->clan==clan) && (!r->rank || rank<=r->rank) && (!r->favor || favor>=r->favor);
}
static bool blocked(struct char_data *ch,int dir){
 struct char_data *mob;bool block=false;
'''+s[a:b]+r'''
 return block;
}
int main(void){
 struct char_data player={0},guard={0},second={0};struct player_special_data ps={0};
 player.player_specials=&ps;IN_ROOM(&player)=0;GET_LEVEL(&player)=50;
 SET_BIT_AR(MOB_FLAGS(&guard),MOB_ISNPC);SET_BIT_AR(MOB_FLAGS(&second),MOB_ISNPC);
 SET_BIT_AR(MOB_FLAGS(&guard),MOB_BLOCK_N);SET_BIT_AR(MOB_FLAGS(&guard),MOB_BLOCK_CLAN);
 guard.clan_requirement=(struct clan_requirement){7,2,3};room.people=&guard;
 assert(!blocked(&player,NORTH));clan=8;assert(blocked(&player,NORTH));assert(!blocked(&player,EAST));
 clan=7;rank=3;assert(blocked(&player,NORTH));guard.clan_requirement.rank=0;assert(!blocked(&player,NORTH));
 favor=2;assert(blocked(&player,NORTH));guard.clan_requirement.favor=0;assert(!blocked(&player,NORTH));
 clan=8;guard.clan_requirement.clan=0;assert(!blocked(&player,NORTH));
 guard.clan_requirement=(struct clan_requirement){7,2,3};clan=7;rank=2;favor=3;
 /* Passing one guard must still evaluate the next guard independently. */
 SET_BIT_AR(MOB_FLAGS(&second),MOB_BLOCK_N);SET_BIT_AR(MOB_FLAGS(&second),MOB_BLOCK_CLAN);
 second.clan_requirement=(struct clan_requirement){8,0,0};guard.next_in_room=&second;
 assert(blocked(&player,NORTH));second.clan_requirement.clan=7;assert(!blocked(&player,NORTH));
 /* Clan requirements cannot be bypassed by a matching race or Ghost. */
 SET_BIT_AR(MOB_FLAGS(&guard),MOB_BLOCK_RACE);clan=8;assert(blocked(&player,NORTH));
 puts("Clan directional guards: membership/rank/Favor, zero fields, multiple guards, and bypass prevention passed.");
}
'''
with tempfile.TemporaryDirectory(prefix='clan-guard-') as d:
 p=Path(d);(p/'test.c').write_text(code)
 subprocess.run(['cc','-std=gnu99','-Wno-unused-parameter','-fsanitize=address,undefined','-fno-pie','-no-pie','-Isrc',str(p/'test.c'),'-o',str(p/'test')],cwd=root,check=True)
 subprocess.run([str(p/'test')],check=True)
