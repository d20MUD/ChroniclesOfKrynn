#!/usr/bin/env python3
"""Exercise specialization selection and actual player-tag save/load routing."""
from pathlib import Path
import re
import subprocess
import tempfile
root=Path(__file__).resolve().parents[1]
craft=(root/'src/crafting_new.c').read_text()
players=(root/'src/players.c').read_text()
db=(root/'src/db.c').read_text()
a=craft.index('void set_craft_specialization(')
command=craft[a:craft.index('\n}',a)+2]
a=players.index('  // craft specializations\n')
writer=players[a:players.index('\n  BUFFER_WRITE("CrOL:',a)]
# Retain the actual outer case for each tag: a handler under the wrong initial
# must fail this test even when its inner strcmp and assignment look correct.
handlers={}
for tag in ['CSp1','CSp2']:
    a=players.index('else if (!strcmp(tag, "'+tag+'"))')
    branch=list(re.finditer(r"^      case '(.)':",players[:a],re.M))[-1].group(1)
    assignment=players[players.index('\n',a)+1:players.index('\n',players.index('\n',a)+1)]
    handlers.setdefault(branch,[]).append('if (!strcmp(tag,"'+tag+'")) { '+assignment.strip()+' }')
loader='switch(tag[0]) {\n'+''.join("case '"+branch+"':\n"+'\n'.join(lines)+'\nbreak;\n' for branch,lines in handlers.items())+'}\n'
def initializers(source):
    return '\n'.join(re.findall(r'^[ \t]+GET_CRAFT\(ch\)\.craft_specialization\[[01]\] = -1;',source,re.M))
code=r'''
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "spells.h"
#include <assert.h>
#include <strings.h>
#define send_to_char(...) ((void)0)
const char *ability_names[NUM_ABILITIES+1];
static char saved[256];
static int saves;
static void new_character(struct char_data *ch) {
'''+initializers(db)+r'''
}
static void load_defaults(struct char_data *ch) {
'''+initializers(players)+r'''
}
static void write_specializations(struct char_data *ch) {
  saved[0]=0;
  #define BUFFER_WRITE(...) snprintf(saved+strlen(saved),sizeof(saved)-strlen(saved),__VA_ARGS__)
'''+writer+r'''
}
void save_char(struct char_data *ch,int mode) {saves++;write_specializations(ch);}
int is_abbrev(const char *arg,const char *name) {return *arg && !strncasecmp(arg,name,strlen(arg));}
'''+command+r'''
static void reload(struct char_data *ch) {
  char copy[256],tag[5],line[32],*record;
  strcpy(copy,saved);load_defaults(ch);
  for(record=strtok(copy,"\n");record;record=strtok(NULL,"\n")) {
    assert(sscanf(record,"%4[^:]: %31s",tag,line)==2);
'''+loader+r'''
  }
}
int main(void) {
  struct char_data ch={0};struct player_special_data special={0};ch.player_specials=&special;
  for(int i=0;i<=NUM_ABILITIES;i++)ability_names[i]="unused";
  ability_names[ABILITY_CRAFT_BOWMAKING]="bowmaking";
  ability_names[ABILITY_HARVEST_MINING]="mining";
  ability_names[ABILITY_CRAFT_ALCHEMY]="alchemy";
  new_character(&ch);
  assert(GET_CRAFT((&ch)).craft_specialization[0]==-1 && GET_CRAFT((&ch)).craft_specialization[1]==-1);
  set_craft_specialization(&ch,"");set_craft_specialization(&ch,"invalid");assert(saves==0);
  set_craft_specialization(&ch,"bowmaking");assert(saves==1);
  assert(strstr(saved,"CSp1: 39\n") && !strstr(saved,"CSp2:"));
  reload(&ch);assert(GET_CRAFT((&ch)).craft_specialization[0]==ABILITY_CRAFT_BOWMAKING);
  assert(GET_CRAFT((&ch)).craft_specialization[1]==-1);
  set_craft_specialization(&ch,"bowmaking");assert(saves==1);
  set_craft_specialization(&ch,"mining");assert(saves==2);
  reload(&ch);
  assert(GET_CRAFT((&ch)).craft_specialization[0]==ABILITY_CRAFT_BOWMAKING);
  assert(GET_CRAFT((&ch)).craft_specialization[1]==ABILITY_HARVEST_MINING);
  set_craft_specialization(&ch,"alchemy");assert(saves==2);
  reload(&ch);assert(GET_CRAFT((&ch)).craft_specialization[1]==ABILITY_HARVEST_MINING);
  // Old player files without specialization tags retain two empty slots.
  saved[0]=0;reload(&ch);
  assert(GET_CRAFT((&ch)).craft_specialization[0]==-1 && GET_CRAFT((&ch)).craft_specialization[1]==-1);
  puts("PASS: new/legacy defaults, immediate saves, both specialization tags routed through C, save/reload round trip, duplicate and third-choice rejection.");
}
'''
with tempfile.TemporaryDirectory(prefix='craft-specialization-') as directory:
    source=Path(directory)/'test.c';binary=Path(directory)/'test';source.write_text(code)
    subprocess.run(['cc','-std=gnu99','-fsanitize=address,undefined','-I'+str(root/'src'),str(source),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
