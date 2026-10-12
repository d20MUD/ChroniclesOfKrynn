#!/usr/bin/env python3
"""Verify fixed golem descriptions leave all mechanical data unchanged."""
from pathlib import Path
import os
import subprocess
import tempfile

root=Path(__file__).resolve().parents[1]
source=(root/'src/crafting_new.c').read_text()
start=source.index('/* These IDs are saved in player files;')
end=source.index('static void battlefield_retrieve_active_golem',start)
code=source[start:end]
headers='\n'.join('#include "'+h+'"' for h in ['conf.h','sysdep.h','structs.h','utils.h','db.h','comm.h','crafting_new.h'])
stubs=r'''
#include <assert.h>
struct char_data *mob_proto;
struct index_data *mob_index;
mob_rnum top_of_mobt;
static int saves, pet_saves, size=1;
static char output[16384];
int get_golem_size_from_vnum(int vnum) { return size; }
void save_char(struct char_data *ch,int mode) { saves++; }
void save_char_pets(struct char_data *ch) { pet_saves++; }
size_t send_to_char(struct char_data *ch,const char *fmt,...) {
  va_list ap; va_start(ap,fmt); int n=vsnprintf(output+strlen(output),sizeof(output)-strlen(output),fmt,ap); va_end(ap); return n;
}
'''
checks=r'''
int main(void) {
  struct char_data owner={0}, golem={0}, prototype={0}, expected;
  struct index_data index={0}; struct follow_type follower={0};
  mob_proto=&prototype; mob_index=&index; index.vnum=500;
  prototype.player.short_descr="a stone golem"; prototype.player.long_descr="A stone golem waits here.\r\n";
  golem.player.name="stone golem construct";
  golem.player.short_descr=prototype.player.short_descr; golem.player.long_descr=prototype.player.long_descr;
  SET_BIT_AR(MOB_FLAGS(&golem),MOB_ISNPC); SET_BIT_AR(MOB_FLAGS(&golem),MOB_GOLEM);
  SET_BIT_AR(AFF_FLAGS(&golem),AFF_CHARM); golem.master=&owner;
  GET_LEVEL(&golem)=12; GET_REAL_MAX_HIT(&golem)=250; GET_HIT(&golem)=73;
  GET_REAL_AC(&golem)=240; GET_REAL_HITROLL(&golem)=8; GET_REAL_DAMROLL(&golem)=6;
  golem.mob_specials.pet_training_rank=7; golem.mob_specials.pet_training_skills[0]=3;
  expected=golem; follower.follower=&golem; owner.followers=&follower;
  for (int i=1;i<NUM_GOLEM_APPEARANCES;i++) {
    set_golem_appearance(&owner,golem_appearance_materials[i]);
    assert(strstr(golem.player.short_descr,golem_appearance_materials[i]));
    assert(strstr(golem.player.long_descr,"stands here.\r\n"));
    expected.player.short_descr=golem.player.short_descr; expected.player.long_descr=golem.player.long_descr;
    assert(!memcmp(&expected,&golem,sizeof(golem)));
    assert(owner.char_specials.saved.golem_appearance==i);
  }
  int old_saves=saves, old_pet_saves=pet_saves;
  set_golem_appearance(&owner,"custom rainbow unicorn");
  assert(saves==old_saves && pet_saves==old_pet_saves);
  set_golem_appearance(&owner,"list"); assert(strstr(output,"dragonmetal") && saves==old_saves);
  set_golem_appearance(&owner,"default");
  assert(!strcmp(golem.player.short_descr,prototype.player.short_descr));
  assert(!strcmp(golem.player.long_descr,prototype.player.long_descr));
  assert(!strcmp(prototype.player.short_descr,"a stone golem"));
  size=-1; set_golem_appearance(&owner,"IRON");
  assert(!strcmp(golem.player.short_descr,"an iron golem"));
  owner.followers=NULL; owner.char_specials.saved.golem_stored_type=GOLEM_TYPE_STONE;
  set_golem_appearance(&owner,"gold"); assert(owner.char_specials.saved.golem_appearance==8);
  owner.char_specials.saved.golem_stored_type=GOLEM_TYPE_NONE; old_saves=saves;
  set_golem_appearance(&owner,"clay"); assert(saves==old_saves);
  free(golem.player.short_descr); free(golem.player.long_descr); return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='golem-appearance-') as tmp:
    path=Path(tmp)/'check.c'; path.write_text(headers+stubs+code+checks)
    binary=Path(tmp)/'check'
    subprocess.run([os.environ.get('CC','cc'),'-std=gnu99','-Wall','-Wextra','-Wno-unused-parameter',
                    '-I'+str(root/'src'),str(path),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
print('Golem appearance: all fixed materials, unchanged mechanical data/keywords, default reset, stored choices, and invalid input passed.')
