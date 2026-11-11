#!/usr/bin/env python3
"""Production private-house geometry and sector flags; no game database."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1];s=(root/'src/housing.c').read_text()
def function(signature):
 a=s.index(signature);return s[a:s.index('\n}',a)+2]
headers='\n'.join('#include "'+h+'"' for h in ['conf.h','sysdep.h','structs.h','utils.h','db.h'])
code=r'''
#include <assert.h>
struct room_data *world;
room_rnum top_of_world=7;
struct home_room {long id,house;room_vnum vnum;struct home_room *next;};
static struct home_room *home_rooms;
room_rnum real_room(room_vnum v){return v<=top_of_world?v:NOWHERE;}
static void edge(int from,int dir,int to){
 if(!world[from].dir_option[dir])world[from].dir_option[dir]=calloc(1,sizeof(struct room_direction_data));
 world[from].dir_option[dir]->to_room=to;
}
'''
main=r'''
int main(void){
 struct home_room records[8]={0},*target;
 world=calloc(8,sizeof(*world));
 for(int i=0;i<8;i++){records[i].id=i+1;records[i].house=i==7?2:1;records[i].vnum=world[i].number=i;records[i].next=i<7?&records[i+1]:NULL;}home_rooms=records;
 edge(0,NORTH,1);edge(1,SOUTH,0);edge(1,EAST,2);edge(2,WEST,1);edge(0,EAST,3);edge(3,WEST,0);
 assert(house_overlap(&records[3],NORTH,&target)&&target==&records[2]);
 assert(house_overlap(&records[0],NORTHEAST,&target)&&target==&records[2]);
 edge(0,UP,4);edge(4,DOWN,0);edge(4,EAST,5);edge(5,WEST,4);
 assert(house_overlap(&records[5],DOWN,&target)&&target==&records[3]);
 assert(house_overlap(&records[5],WEST,&target)&&target==&records[4]);
 // A different house at the desired location never becomes a target.
 edge(0,WEST,7);assert(house_overlap(&records[0],WEST,&target)&&!target);
 // Ambiguous overlaps refuse construction; inconsistent cycles also refuse it.
 edge(0,NORTHEAST,6);assert(!house_overlap(&records[0],NORTHEAST,&target));
 free(world[0].dir_option[NORTHEAST]);world[0].dir_option[NORTHEAST]=NULL;
 edge(2,SOUTH,0);assert(!house_overlap(&records[0],WEST,&target));
 free(world[2].dir_option[SOUTH]);world[2].dir_option[SOUTH]=NULL;
 int sectors[]={SECT_INSIDE,SECT_CITY,SECT_FIELD,SECT_FOREST,SECT_WATER_SWIM,SECT_UNDERWATER,SECT_CAVE};
 const char *names[]={"Inside","City","Field","Forest","Water (Swim)","Underwater","Cave"};
 SET_BIT_AR(world[0].room_flags,ROOM_HOUSE);
 for(int i=0;i<7;i++){assert(house_sector(names[i])==sectors[i]);apply_house_sector(0,sectors[i]);assert(world[0].sector_type==sectors[i]);assert(!!ROOM_FLAGGED(0,ROOM_INDOORS)==(i==0||i>=5));assert(ROOM_FLAGGED(0,ROOM_HOUSE));}
 assert(house_sector("lava")==-1);apply_house_sector(0,999);assert(world[0].sector_type==SECT_INSIDE&&ROOM_FLAGGED(0,ROOM_INDOORS));
 for(int i=0;i<8;i++)for(int dir=0;dir<NUM_OF_DIRS;dir++)free(world[i].dir_option[dir]);free(world);
 puts("House geometry: cardinal/diagonal/floor reuse, ambiguity, cycles, house isolation and sector flags passed");
}
'''
with tempfile.TemporaryDirectory(prefix='house-geometry-') as tmp:
 p=Path(tmp);(p/'test.c').write_text(headers+code+'\n'.join(function(n) for n in ['static struct home_room *room_record(', 'static int house_sector(', 'static void apply_house_sector(', 'static bool house_overlap('])+main)
 subprocess.run(['cc','-std=gnu99','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-fsanitize=address,undefined','-fno-pie','-no-pie','-I'+str(root/'src'),str(p/'test.c'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
