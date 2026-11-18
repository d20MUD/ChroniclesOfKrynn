#!/usr/bin/env python3
"""Check production eidolon Study access for base classes and multiclasses."""
from pathlib import Path
import os
import subprocess
import tempfile
root = Path(__file__).resolve().parents[1]
source = (root / 'src/class.c').read_text()
start = source.index('sbyte has_eidolon_choices_unchosen(struct char_data *ch)')
function = source[start:source.index('\n}', start) + 2]
headers = ['conf.h', 'sysdep.h', 'structs.h', 'utils.h', 'spells.h', 'class.h']
code = '\n'.join(f'#include "{name}"' for name in headers) + r'''
#include <assert.h>
struct player_special_data dummy_mob;
void basic_mud_log(const char *format, ...) { (void)format; }
static int calls;
static bool evolutions, aspects;
#define has_evolutions_unchosen(ch) (calls++, evolutions)
#define study_has_aspects_unchosen(d) (calls++, aspects)
'''+function+r'''
int main(void) {
  struct char_data pc={0};struct player_special_data player={0};
  pc.player_specials=&player;
  assert(!has_eidolon_choices_unchosen(NULL));
  for(int i=0;i<NUM_CLASSES;i++) {
    player.saved.class_level[i]=1;calls=0;
    bool eligible=i==CLASS_SUMMONER || i==CLASS_NECROMANCER;
    assert((bool)has_eidolon_choices_unchosen(&pc)==eligible);
    assert(!calls); /* missing base form must not bypass class eligibility */
    player.saved.class_level[i]=0;
  }
  evolutions=aspects=true;GET_EIDOLON_BASE_FORM(&pc)=1;
  assert(!has_eidolon_choices_unchosen(&pc) && !calls);
  player.saved.class_level[CLASS_WARRIOR]=5;
  player.saved.class_level[CLASS_NECROMANCER]=1;
  assert(has_eidolon_choices_unchosen(&pc));
  evolutions=aspects=false;assert(!has_eidolon_choices_unchosen(&pc));
  aspects=true;assert(has_eidolon_choices_unchosen(&pc));
  SET_BIT_AR(MOB_FLAGS(&pc),MOB_ISNPC);
  assert(!has_eidolon_choices_unchosen(&pc));
  puts("PASS: eidolon Study access requires Summoner/Necromancer ranks, including multiclasses and missing base forms.");
}
'''
with tempfile.TemporaryDirectory(prefix='eidolon-study-') as directory:
    cfile=Path(directory)/'access.c';binary=Path(directory)/'access'
    cfile.write_text(code)
    subprocess.run([os.environ.get('CC','cc'),'-std=gnu11','-g','-fsanitize=address,undefined',
                    '-fno-omit-frame-pointer','-I'+str(root/'src'),str(cfile),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
