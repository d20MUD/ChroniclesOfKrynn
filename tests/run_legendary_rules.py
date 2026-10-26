#!/usr/bin/env python3
"""Compile production legendary rules directly; no live game/database access."""
from pathlib import Path
import os
import shlex
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='krynn-legendary-rules-') as directory:
    executable = Path(directory) / 'legendary-rules'
    subprocess.run([os.environ.get('CC', 'cc'), '-std=gnu90', '-Wall', '-Wextra', '-Werror',
                    *shlex.split(os.environ.get('LEGENDARY_TEST_CFLAGS', '')),
                    '-I'+str(root/'src'), str(root/'src/legendary.c'),
                    str(root/'tests/legendary_rules.c'), '-o', str(executable)], check=True)
    subprocess.run([str(executable)], check=True)
