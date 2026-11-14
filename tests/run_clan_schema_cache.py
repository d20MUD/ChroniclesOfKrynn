#!/usr/bin/env python3
"""Exercise the runtime schema cache against an isolated MariaDB instance."""
from clan_database import ROOT, database
import shlex
import subprocess

migrations = tuple(f'20261110_clan_progression_v00{i}.sql' for i in range(1, 6))
with database(migrations) as (tmp, socket, sql, client):
    flags = shlex.split(subprocess.check_output(
        ['pkg-config', '--cflags', '--libs', 'mariadb', 'openssl', 'json-c'], text=True))
    binary = tmp / 'schema-cache'
    subprocess.run([
        'cc', '-std=c99', '-Wall', '-Wextra', '-Werror', '-pedantic', '-g',
        '-fsanitize=address,undefined', '-fno-pie', '-no-pie', '-pthread',
        '-DCLAN_SCHEMA_CACHE_TTL=1', '-I' + str(ROOT / 'src'),
        str(ROOT / 'src/clan_repository.c'), str(ROOT / 'src/clan_progression.c'),
        str(ROOT / 'tests/clan_schema_cache.c'), *flags, '-o', str(binary)
    ], check=True)
    subprocess.run([str(binary), str(socket)], check=True)
