#!/usr/bin/env python3
"""Exercise production buildwalk map search, connection, and creation dispatch."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'src/oasis_copy.c').read_text()
start = source.index('static int buildwalk_find_overlap(')
end = source.index('\nACMDU(do_buildwalk)', start)
production = source[start:end]
harness = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef int room_rnum;
typedef int room_vnum;
typedef int zone_rnum;
enum { NORTH, EAST, SOUTH, WEST, UP, DOWN, NORTHWEST, NORTHEAST, SOUTHEAST, SOUTHWEST, NUM_OF_DIRS };
#define NOWHERE -1
#define NOTHING -1
#define TRUE 1
#define FALSE 0
#define MAX_INPUT_LENGTH 1024
#define LVL_BUILDER 31
#define LVL_IMMORT 31
#define PRF_BUILDWALK 1
#define ZONE_WILDERNESS 1
#define BRF 0
#define SL_WLD 0
#define CLEANUP_STRUCTS 0
struct room_direction_data { int to_room, key, exit_info; };
struct room_data { int number, zone, coords[2], sector_type, room_flags[4]; char *name, *description;
  struct room_direction_data *dir_option[NUM_OF_DIRS]; };
struct oasis_olc_data { int znum, num, val; struct room_data *room; };
struct descriptor_data { struct oasis_olc_data *olc; };
struct special_data { int buildwalk_flags[4]; };
struct char_data { int room, npc, enabled, level; struct descriptor_data *desc; struct special_data *player_specials; };
static struct room_data world[64];
static struct { int number, wilderness; } zone_table[3];
static int top_of_world, save_calls, find_calls, created, permitted = 1;
static const int rev_dir[] = {SOUTH, WEST, NORTH, EAST, DOWN, UP, SOUTHEAST, SOUTHWEST, NORTHWEST, NORTHEAST};
static const char *dirs[] = {"north", "east", "south", "west", "up", "down", "northwest", "northeast", "southeast", "southwest"};
static char *yel = "", *nrm = "";
static char messages[2048];
static char *default_name, *default_desc;
#define IN_ROOM(c) ((c)->room)
#define IS_NPC(c) ((c)->npc)
#define PRF_FLAGGED(c,f) ((c)->enabled)
#define GET_LEVEL(c) ((c)->level)
#define GET_ROOM_ZONE(r) (world[r].zone)
#define GET_ROOM_VNUM(r) (world[r].number)
#define W_EXIT(r,d) (world[r].dir_option[d])
#define EXIT(c,d) W_EXIT(IN_ROOM(c),d)
#define VALID_ROOM_RNUM(r) ((r) >= 0 && (r) <= top_of_world)
#define ZONE_FLAGGED(z,f) (zone_table[z].wilderness)
#define X_LOC(c) world[IN_ROOM(c)].coords[0]
#define Y_LOC(c) world[IN_ROOM(c)].coords[1]
#define GET_BUILDWALK_NAME(c) default_name
#define GET_BUILDWALK_DESC(c) default_desc
#define GET_BUILDWALK_SECTOR(c) 0
#define GET_NAME(c) "Builder"
#define OLC_ZNUM(d) ((d)->olc->znum)
#define OLC_NUM(d) ((d)->olc->num)
#define OLC_ROOM(d) ((d)->olc->room)
#define OLC_VAL(d) ((d)->olc->val)
#define CREATE(p,t,n) do { (p) = calloc((n),sizeof(t)); assert(p); } while (0)
static void get_char_colors(struct char_data *ch) { (void)ch; }
static int can_edit_zone(struct char_data *ch, int zone) { (void)ch; (void)zone; return permitted; }
static void send_to_char(struct char_data *ch, const char *fmt, ...) {
  va_list ap; (void)ch; va_start(ap,fmt); vsnprintf(messages,sizeof(messages),fmt,ap); va_end(ap);
}
static void add_to_save_list(int zone, int type) { (void)zone; (void)type; save_calls++; }
static void mudlog(int a,int b,int c,const char *s) { (void)a; (void)b; (void)c; (void)s; }
static int redit_find_new_vnum(int zone) { (void)zone; find_calls++; return 9000; }
static int real_room(int vnum) {
  for (int i=0;i<=top_of_world;i++) if (world[i].number==vnum) return i;
  return NOWHERE;
}
static void redit_save_internally(struct descriptor_data *d) {
  int r=++top_of_world; world[r]=*OLC_ROOM(d); world[r].number=OLC_NUM(d); created++;
}
static void cleanup_olc(struct descriptor_data *d, int mode) {
  (void)mode; free(OLC_ROOM(d)); free(d->olc); d->olc=NULL;
}
''' + production + r'''
static struct descriptor_data descriptor;
static struct special_data specials;
static struct char_data builder = {0,0,1,31,&descriptor,&specials};
static void reset(int n) {
  for (int r=0;r<=top_of_world;r++) {
    for(int d=0;d<NUM_OF_DIRS;d++) free(W_EXIT(r,d));
    if(world[r].number==9000) {free(world[r].name);free(world[r].description);}
  }
  memset(world,0,sizeof(world)); top_of_world=n-1;
  for(int r=0;r<n;r++) {world[r].number=100+r;world[r].name="Test room";}
  zone_table[0].number=1; zone_table[1].number=2;
  save_calls=find_calls=created=0; permitted=1; messages[0]=0;
  builder.room=0; builder.enabled=1;
}
static void edge(int a,int dir,int b) {
  assert(!W_EXIT(a,dir)); CREATE(W_EXIT(a,dir),struct room_direction_data,1);
  W_EXIT(a,dir)->to_room=b; W_EXIT(a,dir)->key=NOTHING;
}
static void link(int a,int dir,int b) {edge(a,dir,b);edge(b,rev_dir[dir],a);}
static void square(void) {reset(4);link(0,NORTH,1);link(1,EAST,2);link(2,SOUTH,3);}
int main(void) {
  room_rnum target;
  /* Close a square: connect, move back, and do not allocate a room/vnum. */
  square(); builder.room=3;
  assert(buildwalk(&builder,WEST)==1);
  assert(W_EXIT(3,WEST)->to_room==0 && W_EXIT(0,EAST)->to_room==3);
  assert(created==0 && find_calls==0 && save_calls==2 && top_of_world==3);
  assert(W_EXIT(3,WEST)->key==NOTHING);
  assert(strstr(messages,"Connected to existing room #100"));
  /* An empty destination still uses production room creation. */
  reset(1);assert(buildwalk(&builder,NORTH)==1);
  assert(created==1 && find_calls==1 && top_of_world==1);
  assert(W_EXIT(0,NORTH)->to_room==1 && W_EXIT(1,SOUTH)->to_room==0);
  /* A reverse exit already pointing here is preserved, including door metadata. */
  square();edge(0,EAST,3);W_EXIT(0,EAST)->key=42;W_EXIT(0,EAST)->exit_info=123;
  builder.room=3;assert(buildwalk(&builder,WEST)==1);
  assert(W_EXIT(0,EAST)->key==42 && W_EXIT(0,EAST)->exit_info==123 && created==0);
  /* Never overwrite an occupied reverse exit or create an overlapping fallback. */
  square();edge(0,EAST,2);builder.room=3;
  assert(buildwalk(&builder,WEST)==0 && !W_EXIT(3,WEST));
  assert(W_EXIT(0,EAST)->to_room==2 && created==0 && find_calls==0 && save_calls==0);
  /* Diagonals resolve the same tiles as two cardinal steps. */
  reset(3);link(0,NORTH,1);link(1,EAST,2);
  assert(buildwalk(&builder,NORTHEAST)==1 && W_EXIT(0,NORTHEAST)->to_room==2);
  assert(W_EXIT(2,SOUTHWEST)->to_room==0 && created==0);
  /* Do not search through another floor, even if a route returns downstairs. */
  reset(4);link(0,UP,1);link(1,EAST,2);link(2,DOWN,3);
  assert(buildwalk(&builder,EAST)==1 && W_EXIT(0,EAST)->to_room==4 && created==1);
  reset(3);link(0,UP,1);link(1,EAST,2);
  assert(buildwalk_find_overlap(&builder,EAST,&target) && target==NOWHERE);
  /* Up/down always retain ordinary room creation, never automatic linking. */
  reset(4);link(0,NORTH,1);link(1,UP,2);link(2,SOUTH,3);
  assert(buildwalk(&builder,UP)==1 && W_EXIT(0,UP)->to_room==4 && created==1);
  reset(4);link(0,NORTH,1);link(1,DOWN,2);link(2,SOUTH,3);
  assert(buildwalk(&builder,DOWN)==1 && W_EXIT(0,DOWN)->to_room==4 && created==1);
  /* Multiple distinct rooms at the target position must block creation. */
  reset(5);link(0,NORTH,1);link(1,EAST,2);link(2,SOUTH,3);edge(1,SOUTHEAST,4);
  assert(buildwalk(&builder,EAST)==0 && created==0 && !W_EXIT(0,EAST));
  assert(strstr(messages,"Multiple rooms overlap"));
  /* Do not borrow coordinates from unrelated zones or follow invalid exits. */
  reset(3);link(0,NORTH,1);link(1,SOUTHEAST,2);world[2].zone=1;
  edge(0,WEST,NOWHERE);edge(0,SOUTH,999);
  assert(buildwalk_find_overlap(&builder,EAST,&target) && target==NOWHERE);
  /* Permissions, direction validation and an existing source exit are safe. */
  square();permitted=0;builder.room=3;
  assert(buildwalk(&builder,WEST)==0 && !W_EXIT(3,WEST) && find_calls==0);
  permitted=1;assert(buildwalk(&builder,-1)==0 && buildwalk(&builder,NUM_OF_DIRS)==0);
  assert(buildwalk(NULL,EAST)==0);
  builder.room=0;assert(buildwalk(&builder,NORTH)==0 && W_EXIT(0,NORTH)->to_room==1);
  builder.enabled=0;assert(buildwalk(&builder,EAST)==0 && created==0);
  reset(0);
  puts("Buildwalk overlap regression checks passed.");
}
'''
with tempfile.TemporaryDirectory() as tmp:
    c = Path(tmp) / 'buildwalk.c'
    c.write_text(harness)
    exe = Path(tmp) / 'buildwalk'
    subprocess.run(['cc', '-std=gnu99', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', '-fno-omit-frame-pointer',
                    str(c), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
