#!/usr/bin/env python3
"""Read a dev clan SQL snapshot; migrate only a separate disposable database.

No DDL/DML is issued to the source. Credentials/raw dumps never enter the report.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'tests'))
from clan_database import database
from run_clan_integrated_acceptance import migrations


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--dev-lib', type=Path, required=True)
    args = parser.parse_args()
    cfg = {}
    for line in (args.dev_lib/'mysql_config').read_text().splitlines():
        if '=' in line and not line.lstrip().startswith('#'):
            k, v = line.split('=', 1)
            cfg[k.strip()] = v.strip().strip('\"\'')
    env = dict(os.environ, MYSQL_PWD=cfg['mysql_password'])
    options = ['--no-defaults', '--host='+cfg['mysql_host'], '--user='+cfg['mysql_username']]
    name = cfg['mysql_database']
    client = ['mariadb', *options, '--batch', '--skip-column-names', name]

    def source(query):
        p = subprocess.run(client, input=query, text=True, capture_output=True, env=env)
        if p.returncode:
            raise RuntimeError('Development read-only SQL query failed')
        return p.stdout.strip()

    tables = source("SELECT table_name FROM information_schema.tables WHERE table_schema=DATABASE() AND table_type='BASE TABLE' AND table_name LIKE 'clan\\_%' ORDER BY table_name").splitlines()
    if not tables or not all(re.fullmatch('clan_[a-z_]+', t) for t in tables):
        parser.error('Expected existing development clan tables')
    p = subprocess.run(['mariadb-dump', *options, '--single-transaction', '--triggers', '--hex-blob', name, *tables],
                       capture_output=True, env=env)
    if p.returncode:
        raise RuntimeError('Development read-only clan dump failed')
    with database(()) as (tmp, socket, sql, private_client):
        subprocess.run(private_client, input=p.stdout, check=True, stdout=subprocess.DEVNULL)
        old_version = int(sql('SELECT version FROM clan_progression_schema WHERE singleton=1'))
        assert 1 <= old_version <= 6
        def state():
            return {t: hashlib.sha256('\n'.join(sorted(sql(f'SELECT * FROM {t}').splitlines())).encode()).hexdigest()
                    for t in tables if t != 'clan_progression_schema'}
        before = state()
        for migration in migrations[min(old_version, 5):]:
            sql((ROOT/'sql'/migration).read_text())
        sql((ROOT/'sql/20261113_clan_progression_v006.sql').read_text())
        assert sql('SELECT version FROM clan_progression_schema WHERE singleton=1') == '6'
        assert state() == before
        current = sql("SELECT COUNT(*) FROM information_schema.tables WHERE table_schema='clan_test' AND table_type='BASE TABLE' AND table_name LIKE 'clan\\_%'")
        print(json.dumps(dict(source='development read-only snapshot', old_schema=old_version,
            rehearsed_schema=6, source_tables=len(tables), rehearsed_tables=int(current),
            existing_data_preserved=True, source_mutations=False, features_enabled=False)))


if __name__ == '__main__':
    main()
