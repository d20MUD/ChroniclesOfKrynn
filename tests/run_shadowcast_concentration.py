#!/usr/bin/env python3
"""Verify exempt casting classes never invoke the Concentration skill roll."""
from pathlib import Path
import os
import subprocess
import tempfile

root=Path(__file__).resolve().parents[1]
source=(root/'src/spell_parser.c').read_text()
a=source.index('  /* Shadowcast uses limited-use abilities;')
b=source.index('\n}',a)
branch=source[a:b]
headers='\n'.join('#include "'+name+'"' for name in ['conf.h','sysdep.h','structs.h','utils.h','comm.h','spells.h'])
stubs=r'''
#include <assert.h>
static int checks, resets;
static bool passes;
int skill_check(struct char_data *ch,int skill,int dc) {
  assert(skill==ABILITY_CONCENTRATION); checks++; return passes;
}
bool has_warlock_witch_of_the_multiverse(struct char_data *ch) { return false; }
size_t send_to_char(struct char_data *ch,const char *fmt,...) { return 0; }
const char *act(const char *text,int hide,struct char_data *ch,struct obj_data *obj,void *vict,int type) { return text; }
void resetCastingData(struct char_data *ch) { resets++; }
'''
code='\nstatic bool check_concentration_gate(struct char_data *ch,int concentration_dc) {\n'+branch+'\n}\n'
checks=r'''
int main(void) {
  struct char_data ch={0}; struct player_special_data special={0};
  ch.player_specials=&special; FIGHTING(&ch)=&ch;
  passes=false;
  CASTING_CLASS(&ch)=CLASS_SHADOWDANCER;
  assert(check_concentration_gate(&ch,1000));
  assert(check_concentration_gate(&ch,1000));
  assert(checks==0 && resets==0);
  CASTING_CLASS(&ch)=CLASS_ALCHEMIST;
  assert(check_concentration_gate(&ch,1000));
  assert(checks==0 && resets==0);
  CASTING_CLASS(&ch)=CLASS_WIZARD;
  assert(!check_concentration_gate(&ch,1000));
  assert(checks==1 && resets==1);
  passes=true;
  assert(check_concentration_gate(&ch,1000));
  assert(checks==2 && resets==1);
  /* Having shadowdancer levels does not exempt ordinary multiclass casting. */
  CLASS_LEVEL((&ch),CLASS_SHADOWDANCER)=10;
  passes=false;
  assert(!check_concentration_gate(&ch,1000));
  assert(checks==3 && resets==2);
  FIGHTING(&ch)=NULL;
  assert(check_concentration_gate(&ch,1000));
  assert(checks==3 && resets==2);
  return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='shadowcast-concentration-') as tmp:
    path=Path(tmp)/'check.c'; path.write_text(headers+stubs+code+checks)
    binary=Path(tmp)/'check'
    subprocess.run([os.environ.get('CC','cc'),'-std=gnu99','-Wall','-Wextra','-Wno-unused-parameter',
                    '-I'+str(root/'src'),str(path),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
print('Shadowcast skips Concentration rolls; normal casting and multiclass checks remain intact.')
