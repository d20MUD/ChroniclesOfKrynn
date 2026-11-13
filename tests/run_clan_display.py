#!/usr/bin/env python3
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='clan-display-') as tmp:
 binary=Path(tmp)/'check'
 subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-pedantic','-fsanitize=address,undefined','-fno-pie','-no-pie','-I'+str(root/'src'),str(root/'tests/clan_display.c'),'-o',str(binary)],check=True)
 subprocess.run([str(binary)],check=True)
