#!/usr/bin/env python3
"""Validate the production familiar access macro used by Study and shortcut 5."""
from pathlib import Path
import os
import subprocess
import tempfile
root=Path(__file__).resolve().parents[1]
source=(root/'src/study.c').read_text()
assert 'STUDY_OPTION("5", CAN_STUDY_FAMILIAR(ch)' in source
assert "case '5':\n      if (CAN_STUDY_FAMILIAR(ch))" in source
code=r'''
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include <assert.h>
struct player_special_data dummy_mob;
void basic_mud_log(const char *format, ...) { (void)format; }
static bool familiar_feat;
int get_feat_value(struct char_data *ch,int feat) {
  (void)ch; assert(feat==FEAT_SUMMON_FAMILIAR);return familiar_feat;
}
int main(void) {
  struct char_data pc={0};struct player_special_data player={0};
  struct char_data *ch=&pc;pc.player_specials=&player;
  assert(!CAN_STUDY_FAMILIAR((struct char_data *)NULL));
  familiar_feat=true;
  assert(!CAN_STUDY_FAMILIAR(ch));
  for(int i=0;i<NUM_CLASSES;i++) {
    player.saved.class_level[i]=1;
    bool expected=i==CLASS_WIZARD || i==CLASS_SORCERER || i==CLASS_WARLOCK;
    assert((bool)CAN_STUDY_FAMILIAR(ch)==expected);
    familiar_feat=false;assert(!CAN_STUDY_FAMILIAR(ch));familiar_feat=true;
    player.saved.class_level[i]=0;
  }
  player.saved.class_level[CLASS_WARRIOR]=5;
  player.saved.class_level[CLASS_SORCERER]=1;
  assert(CAN_STUDY_FAMILIAR(ch));
  SET_BIT_AR(MOB_FLAGS(ch),MOB_ISNPC);assert(!CAN_STUDY_FAMILIAR(ch));
  puts("PASS: familiar Study access requires Wizard, Sorcerer or Warlock ranks and the familiar feature; unrelated classes and NPCs are excluded.");
}
'''
with tempfile.TemporaryDirectory(prefix='familiar-study-') as directory:
    cfile=Path(directory)/'access.c';binary=Path(directory)/'access'
    cfile.write_text(code)
    subprocess.run([os.environ.get('CC','cc'),'-std=gnu11','-g','-fsanitize=address,undefined',
                    '-fno-omit-frame-pointer','-I'+str(root/'src'),str(cfile),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
