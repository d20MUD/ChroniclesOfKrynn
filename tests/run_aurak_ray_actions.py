#!/usr/bin/env python3
"""Exercise production Aurak ray action selection and command recognition."""
from pathlib import Path
import subprocess
import tempfile
root = Path(__file__).resolve().parents[1]
source = (root / 'src/spell_parser.c').read_text()
a = source.index('bool is_aurak_energy_ray_cast(')
helper = source[a:source.index('\n}',a)+2]
a = source.index('  /* The racial ray always uses a swift action')
b = source.index('  else if (CONFIG_SPELLCASTING_TIME_MODE == 0)',a)
actions = source[a:b]
a = source.index('    else if (spellnum != SPELL_AURAK_ENERGY_RAY)')
b = source.index('\n    }',a)+6
instant = source[a:b]
code = r'''
#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#define TRUE true
#define FALSE false
#define MAX_INPUT_LENGTH 256
#define SPELL_AURAK_ENERGY_RAY 536
#define IS_NPC(ch) ((ch)->npc)
#define atSWIFT 1
#define USE_SWIFT_ACTION(ch) ((ch)->swift_used++, (ch)->swift_available=false)
#define USE_MOVE_ACTION(ch) ((ch)->move_used++)
struct char_data { bool npc, swift_available; int swift_used, move_used; };
static bool is_action_available(struct char_data *ch,int action,bool display) {
  assert(action==atSWIFT); return ch->swift_available;
}
static int find_skill_num(char *name) {
  /* Ordinary spell resolver accepts unambiguous prefixes. */
  if (!strncmp("aurak energy ray",name,strlen(name))) return SPELL_AURAK_ENERGY_RAY;
  return 1;
}
''' + helper + r'''
static int cast(struct char_data *ch,int mode,int spellnum) {
  int casting_time=5;
''' + actions + r'''
  else { return -1; }
  assert(casting_time==0);
  if (mode==0) { /* no extra cost in instant casting branch */ }
''' + instant + r'''
  return 1;
}
int main(void) {
  assert(is_aurak_energy_ray_cast(" 'aurak energy ray' healer"));
  assert(is_aurak_energy_ray_cast(" 'aurak energy' healer"));
  assert(is_aurak_energy_ray_cast("quicken 'aurak energy ray' healer"));
  assert(!is_aurak_energy_ray_cast(" 'aurak breath'"));
  assert(!is_aurak_energy_ray_cast("aurak energy ray"));
  assert(!is_aurak_energy_ray_cast(" 'aurak energy ray"));
  assert(!is_aurak_energy_ray_cast(" ''"));
  for(int mode=0;mode<=1;mode++) {
    struct char_data ch={.swift_available=true};
    assert(cast(&ch,mode,SPELL_AURAK_ENERGY_RAY)==1);
    assert(ch.swift_used==1 && ch.move_used==0);
    assert(cast(&ch,mode,SPELL_AURAK_ENERGY_RAY)==0);
    assert(ch.swift_used==1 && ch.move_used==0);
    ch.swift_available=true; /* next round */
    assert(cast(&ch,mode,SPELL_AURAK_ENERGY_RAY)==1);
    assert(ch.swift_used==2 && ch.move_used==0);
    assert(cast(&ch,mode,1)==-1);
  }
  puts("Aurak energy ray: swift cost, unavailable action, both casting modes, and command parsing passed.");
}
'''
interpreter = (root/'src/interpreter.c').read_text()
assert 'complete_cmd_info[cmd].subcmd == SCMD_CAST_SPELL && is_aurak_energy_ray_cast(line)' in interpreter
with tempfile.TemporaryDirectory() as tmp:
    c=Path(tmp)/'ray.c'; c.write_text(code); exe=Path(tmp)/'ray'
    subprocess.run(['cc','-std=c99','-fsanitize=address,undefined',str(c),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
