#!/usr/bin/env python3
"""Exercise native craft preview color conversion without mutating saved strings."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1]
def function(text,signature):
 a=text.index(signature);return text[a:text.index('\n}',a)+2]
craft=(root/'src/crafting_new.c').read_text()
code=r'''
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
struct char_data {int unused;};
static char output[8192];
size_t send_to_char(struct char_data *ch,const char *format,...) {
 va_list args;va_start(args,format);int n=vsnprintf(output,sizeof(output),format,args);va_end(args);return n;
}
'''+function((root/'src/helpers.c').read_text(),'void parse_at(')+function(craft,'static void show_craft_description(')+r'''
int main(void) {
 const char *inputs[]={"@[F510]a @[F520]br@[F530]on@[F540]ze @[F550]light mace@n",
 "\tRalready parsed\tn", "@Rred@n and @@literal", "plain 100% mace", "@[B123]background@n\r\n\r\n@Csecond paragraph@n"};
 for(unsigned i=0;i<sizeof(inputs)/sizeof(*inputs);i++) {
  char saved[1024],expected[2048],parsed[1024];
  strcpy(saved,inputs[i]);strcpy(parsed,saved);parse_at(parsed);
  snprintf(expected,sizeof(expected),"-- short desc: %s\tn\r\n",parsed);
  show_craft_description(NULL,"-- short desc: ",saved);
  assert(!strcmp(output,expected));assert(!strcmp(saved,inputs[i]));
 }
 show_craft_description(NULL,"-- extra desc: ",NULL);assert(!strcmp(output,"-- extra desc: not set\tn\r\n"));
 puts("Craft show colors: RGB, basic and existing tab codes, literal markers, percent signs, blank lines and unchanged saved descriptions passed.");
}
'''
# Every displayed description uses the rendering helper; extra text is parsed
# before wrapping so formatting can recognize its color markers.
show=function(craft,'void show_current_craft(')
assert show.count('show_craft_description(ch,')==4
assert show.index('parse_at(temp)')<show.index('strfrmt(temp')
with tempfile.TemporaryDirectory(prefix='craft-show-colors-') as directory:
 p=Path(directory);(p/'test.c').write_text(code)
 subprocess.run(['cc','-std=gnu99','-g','-fsanitize=address,undefined','-fno-pie','-no-pie',str(p/'test.c'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
