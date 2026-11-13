#!/usr/bin/env python3
"""Phase 50: restore every optional clan store after real post-cutover rewards.

Only disposable databases created by clan_database.database are accessed.
The legacy-format import and unavailable-store boot matrix run in the full suite.
"""
from run_clan_integrated_acceptance import run
import hashlib
import json
import subprocess


def restore(tmp, socket, sql, client, tool):
    tables = sql("SELECT table_name FROM information_schema.tables WHERE table_schema='clan_test' AND table_type='BASE TABLE' ORDER BY table_name").splitlines()
    assert tables and all(t.startswith('clan_') for t in tables)
    assert int(sql('SELECT COUNT(*) FROM clan_reward_receipts')) > 0
    assert int(sql('SELECT COUNT(*) FROM clan_quartermaster_operations')) > 0
    assert int(sql('SELECT COUNT(*) FROM clan_expedition_completions')) > 0
    assert sql('SELECT store_mode FROM clan_store_control') == '2'

    def digest(database):
        result = {}
        for table in tables:
            # Client-side ordering is independent of physical restore/index order.
            rows = sql(f'SELECT * FROM {database}.{table}').splitlines()
            result[table] = hashlib.sha256('\n'.join(sorted(rows)).encode()).hexdigest()
        return result

    before = digest('clan_test')
    dump = subprocess.check_output(['mariadb-dump', '--no-defaults', '--socket='+str(socket),
        '--user=root', '--single-transaction', '--triggers', '--hex-blob', 'clan_test'])
    backup = tmp / 'post-cutover.sql'
    backup.write_bytes(dump)
    backup.chmod(0o600)
    sql('CREATE DATABASE clan_recovered CHARACTER SET utf8mb4')
    subprocess.run([*client[:-1], 'clan_recovered'], input=dump, check=True, stdout=subprocess.DEVNULL)
    assert digest('clan_recovered') == before
    for database in ['clan_test', 'clan_recovered']:
        p = subprocess.run([str(tool), 'read', database, 'root', '', str(socket)],
                           input='{}', text=True, capture_output=True, check=True)
        data = json.loads(p.stdout)
        assert data['result'] == 'ok' and data['store_mode'] == 2
        if database == 'clan_test':
            original = data
        else:
            assert data == original
    guards = sql("SELECT trigger_name FROM information_schema.triggers WHERE trigger_schema='clan_test' ORDER BY trigger_name")
    recovered = sql("SELECT trigger_name FROM information_schema.triggers WHERE trigger_schema='clan_recovered' ORDER BY trigger_name")
    assert guards == recovered and guards
    assert digest('clan_test') == before
    print(f'Phase 50: post-cutover dump/restore preserves {len(tables)} clan tables, SQL authority, all receipts, balances, cooldowns, purchase journals, claims and retention triggers.', flush=True)


if __name__ == '__main__':
    run(after=restore)
