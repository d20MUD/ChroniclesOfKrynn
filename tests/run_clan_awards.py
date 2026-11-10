#!/usr/bin/env python3
"""Exercise production Phase 9 policy with strict warnings and sanitizers."""
from pathlib import Path
import os, shlex, subprocess, tempfile
root = Path(__file__).resolve().parents[1]
flags = shlex.split(subprocess.check_output(
    ['pkg-config', '--cflags', '--libs', 'mariadb', 'openssl', 'json-c'], text=True))
with tempfile.TemporaryDirectory(prefix='clan-awards-') as directory:
    binary = Path(directory)/'check'
    subprocess.run([os.environ.get('CC','cc'), '-std=c99', '-Wall', '-Wextra',
        '-Werror', '-pedantic', '-fsanitize=address,undefined', '-fno-pie', '-no-pie',
        '-I'+str(root/'src'), str(root/'tests/clan_awards.c'),
        str(root/'src/clan_awards.c'), str(root/'src/clan_repository.c'),
        str(root/'src/clan_progression.c'), *flags, '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
