#!/usr/bin/env python3
"""Regression: line-leading color text stays in the editor; only /s saves."""
from pathlib import Path
import os
import subprocess
import tempfile

root=Path(__file__).resolve().parents[1]
source=(root/'src/improved-edit.c').read_text()
def function(signature):
    start=source.index(signature+'\n{')
    return source[start:source.index('\n}',start)+2]+'\n'
code=function('int improved_editor_execute(struct descriptor_data *d, char *str)')
code+=function('void send_editor_help(struct descriptor_data *d)')
headers='\n'.join('#include "'+h+'"' for h in ['conf.h','sysdep.h','structs.h','utils.h','comm.h','improved-edit.h'])
stubs=r'''
#include <assert.h>
static char output[8192];
static int parsed=-1;
void parse_edit_action(int action,char *text,struct descriptor_data *d) { parsed=action; }
size_t write_to_output(struct descriptor_data *d,const char *fmt,...) {
  va_list ap; va_start(ap,fmt); int n=vsnprintf(output+strlen(output),sizeof(output)-strlen(output),fmt,ap); va_end(ap); return n;
}
'''
checks=r'''
int main(void) {
  struct descriptor_data d={0}; char *buffer=NULL; d.str=&buffer;
  const char *text[]={"@rRed text", "@WWhite text", "@@literal at sign", "@", "@s", "\trRed text", "\t", "ordinary text", ""};
  for(size_t i=0;i<sizeof(text)/sizeof(*text);i++) {
    char line[MAX_INPUT_LENGTH]={0}; strcpy(line,text[i]);
    assert(improved_editor_execute(&d,line)==STRINGADD_OK);
    assert(!strcmp(line,text[i]));
  }
  char save[MAX_INPUT_LENGTH]="/s"; assert(improved_editor_execute(&d,save)==STRINGADD_SAVE);
  char old_save[MAX_INPUT_LENGTH]="/@"; assert(improved_editor_execute(&d,old_save)==STRINGADD_ACTION);
  assert(strstr(output,"Invalid option"));
  char help[MAX_INPUT_LENGTH]="/h"; assert(improved_editor_execute(&d,help)==STRINGADD_ACTION && parsed==PARSE_HELP);
  char abort[MAX_INPUT_LENGTH]="/a"; assert(improved_editor_execute(&d,abort)==STRINGADD_ABORT);
  output[0]=0; send_editor_help(&d); assert(strstr(output,"/s to save") && !strstr(output,"Type @"));
  return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='editor-save-') as tmp:
    path=Path(tmp)/'check.c'; path.write_text(headers+stubs+code+checks)
    binary=Path(tmp)/'check'
    subprocess.run([os.environ.get('CC','cc'),'-std=gnu99','-Wall','-Wextra','-Wno-unused-parameter',
        '-I'+str(root/'src'),str(path),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
print('Editor save: leading @/tab color text preserved, /s saves, /@ rejected, help and abort remain functional.')
