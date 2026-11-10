#!/usr/bin/env python3
"""Build the offline bridge without depending on a running MUD."""
from pathlib import Path
import os,shlex,subprocess,sys
ROOT=Path(__file__).resolve().parents[1]
def build(output, sanitize=False):
 flags=shlex.split(subprocess.check_output(['pkg-config','--cflags','--libs','mariadb','openssl','json-c'],text=True))
 subprocess.run([os.environ.get('CC','cc'),'-std=c99','-Wall','-Wextra','-Werror','-pedantic','-g',*(['-fsanitize=address,undefined','-fno-pie','-no-pie'] if sanitize else []),'-I'+str(ROOT/'src'),str(ROOT/'src/clan_repository.c'),str(ROOT/'src/clan_progression.c'),str(ROOT/'tools/clan_repository_tool.c'),*flags,'-o',str(output)],check=True)
if __name__=='__main__':
 if len(sys.argv)!=2:raise SystemExit('Usage: build_clan_repository_tool.py OUTPUT')
 build(Path(sys.argv[1]))
