#!/usr/bin/env python3
"""Compare import decoding to the actual native fread_string/parse_at code."""
from pathlib import Path
import subprocess,tempfile,sys
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'scripts'))
from clan_legacy_import import legacy_description,records,ImportProblem

def function(text,signature):
 a=text.index(signature);return text[a:text.index('\n}',a)+2]
code='''#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#define MAX_STRING_LENGTH 49152
#define log(...) ((void)0)
'''+function((ROOT/'src/helpers.c').read_text(),'void parse_at(')+function((ROOT/'src/db.c').read_text(),'char *fread_string(FILE *fl, const char *error)')+'''
int main(void){char *s=fread_string(stdin,"test");if(s){fwrite(s,1,strlen(s),stdout);free(s);}return 0;}
'''
with tempfile.TemporaryDirectory(prefix='clan-desc-') as directory:
 binary=Path(directory)/'native';path=Path(directory)/'test.c';path.write_text(code)
 subprocess.run(['cc','-std=gnu99','-g','-fsanitize=address,undefined','-fno-pie','-no-pie',str(path),'-o',str(binary)],check=True)
 for raw in ['~\n','text~\n','text\n~\n','@[F510]color@n\n\nSecond paragraph~\n','literal @@ marker and internal~tilde stays\nend~\n','first  \r\n\r\nlast~\r\n','x'*1600+'~\n','x'*510+'\n~\n','x'*511+'\n~\n','café and naïve~\n']:
  expected=subprocess.check_output([str(binary)],input=raw.encode()).decode()
  assert legacy_description(raw)==expected,(repr(raw),repr(legacy_description(raw)),repr(expected))
 value=records('#7\nName: \tRName with trailing space \nDesc:\n@RRed\n\nBody~\n$\n','clans')[0]
 assert value['Name']=='\tRName with trailing space ' and value['Desc']=='\tRRed\r\n\r\nBody'
 print('Legacy clan descriptions match production fread_string/parse_at: RGB/basic/escaped colors, CRLF, blank/trailing lines, spaces, internal tildes, long fgets chunks and UTF-8.')
