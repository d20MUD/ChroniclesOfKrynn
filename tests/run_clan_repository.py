#!/usr/bin/env python3
from clan_database import ROOT,database
import subprocess,shlex
subprocess.run(['python3',str(ROOT/'scripts/generate_clan_schema_contract.py'),'--check'],check=True)
with database(('20261110_clan_progression_v001.sql','20261110_clan_progression_v002.sql')) as (tmp,socket,sql,client):
 binary=tmp/'repo';flags=shlex.split(subprocess.check_output(['pkg-config','--cflags','--libs','mariadb','openssl','json-c'],text=True))
 subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-pedantic','-g','-fsanitize=address,undefined','-fno-pie','-no-pie','-I'+str(ROOT/'src'),str(ROOT/'src/clan_repository.c'),str(ROOT/'src/clan_progression.c'),str(ROOT/'tests/clan_repository.c'),*flags,'-o',str(binary)],check=True)
 subprocess.run([str(binary),str(socket)],check=True)
