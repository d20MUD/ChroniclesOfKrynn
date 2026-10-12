#!/usr/bin/env python3
"""Exercise the production device-work command gate and its command list."""
from pathlib import Path
import os
import re
import subprocess
import tempfile

root=Path(__file__).resolve().parents[1]
source=(root/'src/interpreter.c').read_text()
a=source.index('static const char *const device_work_commands[]')
b=source.index('\n}',source.index('static bool is_device_work_command(',a))+2
helper=source[a:b]
a=source.index('  else if ((char_has_mud_event(ch, eDEVICE_CREATION)')
b=source.index('\n  {',a)
device_condition=source[a:b].removeprefix('  else if ')
a=source.index('  else if ((char_has_mud_event(ch, eCRAFTING)')
b=source.index('\n  {',a)
condition=source[a:b].removeprefix('  else if ')
# Every newly allowed name must resolve to a real command with no action cost.
for name in re.findall(r'"([^"]+)"',helper):
    if name in ('class','race'): continue
    match=re.search(r'\{"'+re.escape(name)+r'"[^\n]+',source)
    assert match and 'ACTION_NONE' in match.group(), name
harness=r'''
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#define TRUE true
#define FALSE false
#define eCRAFTING 0
#define eDEVICE_CREATION 1
#define eDEVICE_REPAIR 2
#define eBREWING 3
struct char_data { int progress; };
struct command { const char *command; };
static struct command complete_cmd_info[1];
static bool events[4];
static bool char_has_mud_event(struct char_data *ch,int event) { return events[event]; }
static bool is_abbrev(const char *a,const char *b) { return *a && !strncmp(a,b,strlen(a)); }
'''
code=helper+'\nstatic bool blocked(struct char_data *ch,const char *command) {\n  int cmd=0; complete_cmd_info[cmd].command=command; return '+device_condition+' || '+condition+';\n}\n'
checks=r'''
int main(void) {
  struct char_data ch={.progress=500};
  const char *unsafe[]={"north","south","kill","cast","device","repairdevice","craft","get","wear","quit",NULL};
  for(int event=eDEVICE_CREATION;event<=eDEVICE_REPAIR;event++) {
    memset(events,0,sizeof(events)); events[event]=true;
    for(int i=0;device_work_commands[i];i++) assert(!blocked(&ch,device_work_commands[i]));
    assert(!blocked(&ch,"look") && !blocked(&ch,"tell") && !blocked(&ch,"help"));
    for(int i=0;unsafe[i];i++) assert(blocked(&ch,unsafe[i]));
    assert(events[event] && ch.progress==500);
  }
  /* The added exceptions apply only to artificer work. */
  memset(events,0,sizeof(events)); events[eCRAFTING]=true;
  assert(blocked(&ch,"news") && blocked(&ch,"motd") && blocked(&ch,"who"));
  assert(!blocked(&ch,"look"));
  memset(events,0,sizeof(events)); events[eBREWING]=true;
  assert(blocked(&ch,"news") && blocked(&ch,"motd") && blocked(&ch,"who"));
  memset(events,0,sizeof(events)); assert(!blocked(&ch,"north"));
  assert(!is_device_work_command("newcraft") && !is_device_work_command("whoever"));
  return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='device-work-commands-') as tmp:
    path=Path(tmp)/'check.c'; path.write_text(harness+code+checks)
    binary=Path(tmp)/'check'
    subprocess.run([os.environ.get('CC','cc'),'-std=c99','-Wall','-Wextra','-Wno-unused-parameter',
                    str(path),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
print('Device creation/repair informational commands, existing chat, blocked actions, and crafting/brewing isolation passed.')
