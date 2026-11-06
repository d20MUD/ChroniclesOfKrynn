#!/usr/bin/env python3
"""Exercise terrain defaults, editor validation, persistence, and room copying."""
from pathlib import Path
import os
import subprocess
import tempfile
root=Path(__file__).resolve().parents[1]
utils=(root/'src/utils.c').read_text()
redit=(root/'src/redit.c').read_text()
gen=(root/'src/genwld.c').read_text()
db=(root/'src/db.c').read_text()
def function(text,name):
    a=text.index(name)
    return text[a:text.index('\n}',a)+2]
a=gen.index('      /* Automatic sizes need no record,')
b=gen.index('      /* Now you write out the exits',a)
writer=gen[a:b]
a=db.index("    case 'R': /* Optional maximum distance override")
b=db.index("    case 'C': /* Coordinates. */",a)
reader=db[a:b]
a=redit.index('  case REDIT_ROOM_SIZE:')
b=redit.index('  case REDIT_SECTOR:',a)
editor=redit[a:b]
# The complete load path must reset the override before reading optional records.
assert 'world[room_nr].room_size = 0;' in function(db,'void parse_room(')
assert 'world[IN_ROOM(ch)].room_size = world[i].room_size;' in (root/'src/act.wizard.c').read_text()
headers='\n'.join('#include "'+h+'"' for h in
                  ['conf.h','sysdep.h','structs.h','utils.h','oasis.h'])
code=headers+r'''
#include <assert.h>
struct config_data config_info;
struct room_data *world;
room_rnum top_of_world;
static int errors, menus;
#undef log
#define log(...) ((void)0)
int get_line(FILE *file,char *line) { return fgets(line,128,file)!=NULL; }
static void write_to_output(struct descriptor_data *d,const char *fmt,...) { errors++; }
static void redit_disp_menu(struct descriptor_data *d) { menus++; }
static void free_room_strings(struct room_data *room) {}
static void copy_room_strings(struct room_data *to,struct room_data *from) {}
static void free_trail_data_list(struct trail_data_list *trail) {}
static struct trap_data *copy_trap_list(struct trap_data *trap) { return NULL; }
'''
code+='\n'.join(function(utils,n) for n in ['int default_room_size(','int get_room_size(','bool parse_room_size('])
code+='\n'+function(gen,'int copy_room(')
code+='\nstatic void save_size(FILE *sf,struct room_data *room) {\n'+writer+'}\n'
code+='\nstatic void load_size(FILE *fl) { int room_nr=0,virtual_nr=1; char line[128]; world[0].room_size=0; while(get_line(fl,line)) { switch(*line) {\n'+reader+'default: break;\n} } }\n'
code+='\nstatic void edit_size(struct descriptor_data *d,char *arg) { switch(OLC_MODE(d)) {\n'+editor+'default: assert(0); } OLC_VAL(d)=1; redit_disp_menu(d); }\n'
code+=r'''
int main(void) {
  int expected[]={10,10,50,20,30,20,10,10,40,30,10,30,30,30,50,50,20,10,30,
                  20,10,10,10,10,10,20,30,30,30,10,10,50,20,50,20,10,10};
  struct room_data rooms[2]={{0}}, source={0}, destination={0};
  struct descriptor_data descriptor={0};struct oasis_olc_data olc={0};
  world=rooms;top_of_world=1;
  descriptor.olc=&olc;olc.room=&rooms[0];olc.mode=REDIT_ROOM_SIZE;
  assert(sizeof(expected)/sizeof(expected[0])==NUM_ROOM_SECTORS);
  for(int sector=0;sector<NUM_ROOM_SECTORS;sector++) {
    rooms[0].sector_type=sector;
    assert(default_room_size(sector)==expected[sector]);
    assert(ROOM_SIZE(0)==expected[sector]);
    rooms[0].room_size=73;assert(ROOM_SIZE(0)==73);
    rooms[0].room_size=0;
  }
  assert(default_room_size(SECT_OUTTER_PLANES)==30);
  assert(default_room_size(SECT_UD_WATER_NOSWIM)==10);
  assert(default_room_size(-1)==10 && default_room_size(NUM_ROOM_SECTORS)==10);
  assert(get_room_size(NULL)==10 && ROOM_SIZE(NOWHERE)==10 && ROOM_SIZE(2)==10);
  const char *bad[]={""," ","-1","+2","2.5","abc","10junk","1 2","2147483648","999999999999999999999999999999"};
  for(unsigned i=0;i<sizeof(bad)/sizeof(bad[0]);i++) {
    int result=91;assert(!parse_room_size(bad[i],&result) && result==91);
    olc.value=0;rooms[0].room_size=91;int previous=menus;
    edit_size(&descriptor,(char *)bad[i]);
    assert(rooms[0].room_size==91 && !olc.value && menus==previous);
  }
  assert(!parse_room_size(NULL,&rooms[0].room_size));
  assert(!parse_room_size("10",NULL));
  edit_size(&descriptor," 123 \r\n");assert(olc.value && rooms[0].room_size==123);
  edit_size(&descriptor,"2147483647");assert(rooms[0].room_size==INT_MAX);
  edit_size(&descriptor,"0");assert(rooms[0].room_size==0);
  rooms[0].sector_type=SECT_FIELD;assert(ROOM_SIZE(0)==50);
  rooms[0].sector_type=SECT_INSIDE;assert(ROOM_SIZE(0)==10);
  for(int size=0;size<2;size++) {
    rooms[1].room_size=size ? 123 : 0;
    FILE *file=tmpfile();assert(file);save_size(file,&rooms[1]);
    if(!size) assert(ftell(file)==0);
    rewind(file);rooms[0].room_size=999;load_size(file);fclose(file);
    assert(rooms[0].room_size==rooms[1].room_size);
  }
  source.sector_type=SECT_FOREST;source.room_size=88;
  assert(copy_room(&destination,&source));
  assert(destination.room_size==88 && get_room_size(&destination)==88);
  source.room_size=0;assert(copy_room(&destination,&source));
  assert(!destination.room_size && get_room_size(&destination)==20);
  puts("PASS: all 37 terrain defaults, aliases, custom/reset sizes, validation, redit edits, world save/load and room copy.");
}
'''
with tempfile.TemporaryDirectory(prefix='room-size-') as directory:
    cfile=Path(directory)/'room_size.c';binary=Path(directory)/'room_size'
    cfile.write_text(code)
    subprocess.run([os.environ.get('CC','cc'),'-std=c11','-g','-fsanitize=address,undefined',
                    '-fno-omit-frame-pointer','-I'+str(root/'src'),str(cfile),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
