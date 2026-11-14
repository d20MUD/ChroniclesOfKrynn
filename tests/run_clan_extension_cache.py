#!/usr/bin/env python3
"""Exercise real optional-store validators against disposable MariaDB only."""
from clan_database import ROOT, database
import shlex
import subprocess

migrations = tuple(f'20261110_clan_progression_v{i:03d}.sql' for i in range(1, 6)) + (
    '20261111_clan_buff_activations_v001.sql', '20261111_clan_buff_lifecycle_v001.sql',
    '20261111_clan_kill_sources_v001.sql', '20261111_clan_delivery_v001.sql',
    '20261111_clan_objectives_v001.sql', '20261112_clan_expeditions_v001.sql',
    '20261112_clan_expeditions_v002.sql', '20261112_clan_quartermaster_v001.sql')
with database(migrations) as (tmp, socket, sql, client):
    flags = shlex.split(subprocess.check_output(
        ['pkg-config', '--cflags', '--libs', 'mariadb', 'openssl', 'json-c'], text=True))
    sources = ('clan_repository.c', 'clan_progression.c', 'clan_buff_commit.c',
               'clan_buff_lifecycle.c', 'clan_delivery.c', 'clan_objectives.c',
               'clan_expedition_enrollment.c', 'clan_quartermaster.c')
    binary = tmp / 'extension-cache'
    subprocess.run([
        'cc', '-std=c99', '-Wall', '-Wextra', '-Werror', '-pedantic', '-g',
        '-fsanitize=address,undefined', '-fno-pie', '-no-pie', '-pthread',
        '-ffunction-sections', '-fdata-sections', '-Wl,--gc-sections',
        '-DCLAN_SCHEMA_CACHE_TTL=1', '-I' + str(ROOT / 'src'),
        str(ROOT / 'tests/clan_extension_cache.c'),
        *[str(ROOT / 'src' / name) for name in sources], *flags, '-o', str(binary)
    ], check=True)
    subprocess.run([str(binary), str(socket)], check=True)
