#!/usr/bin/env python3
"""Apply actual clan migration to a private, socket-only disposable MariaDB."""
from pathlib import Path
import getpass
import re
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]
MIGRATION = ROOT / 'sql/20261110_clan_progression_v001.sql'
UID_A, UID_B, UID_C = 'a' * 32, 'b' * 32, 'c' * 32
MAX_I64 = 9223372036854775807

with tempfile.TemporaryDirectory(prefix='clan-progression-schema-') as directory:
    temp = Path(directory)
    data = temp / 'data'
    socket = temp / 'mysql.sock'
    log = temp / 'mysql.log'
    subprocess.run(['mariadb-install-db', '--no-defaults', '--datadir=' + str(data),
                    '--auth-root-authentication-method=normal', '--skip-test-db'],
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=True)
    server = subprocess.Popen(['mariadbd', '--no-defaults', '--datadir=' + str(data),
                               '--socket=' + str(socket), '--pid-file=' + str(temp / 'pid'),
                               '--skip-networking', '--log-error=' + str(log),
                               '--user=' + getpass.getuser()],
                              stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    client = ['mariadb', '--no-defaults', '--socket=' + str(socket), '-uroot',
              '--batch', '--skip-column-names']
    try:
        for _ in range(100):
            probe = subprocess.run([*client, '-e', 'SELECT 1'], capture_output=True)
            if probe.returncode == 0:
                break
            if server.poll() is not None:
                raise RuntimeError(log.read_text())
            time.sleep(.1)
        else:
            raise RuntimeError('Disposable MariaDB startup timed out')
        subprocess.run([*client, '-e', 'CREATE DATABASE clan_schema_test'], check=True)
        client.append('clan_schema_test')

        def sql(statement, *, ok=True, error=None, strict=True):
            prefix = "SET SESSION sql_mode='STRICT_ALL_TABLES,NO_ENGINE_SUBSTITUTION';\n" if strict else "SET SESSION sql_mode='';\n"
            result = subprocess.run(client, input=prefix + statement, text=True, capture_output=True)
            if ok:
                assert result.returncode == 0, result.stderr
            else:
                assert result.returncode != 0, 'Invalid SQL succeeded: ' + statement
                assert 'ERROR' in result.stderr, result.stderr
                if error:
                    assert error in result.stderr, result.stderr
            return result.stdout.strip()

        def migrate(ok=True):
            result = subprocess.run(client, input=MIGRATION.read_text(), text=True, capture_output=True)
            assert (result.returncode == 0) == ok, result.stderr
            return result

        def clan(uid, vnum, **values):
            row = dict(clan_uid=uid, clan_vnum=vnum, name='The Clan', description='A clan',
                       created_at_utc=2200000000, updated_at_utc=2200000000)
            row.update(values)
            literals = ['NULL' if v is None else str(v) if isinstance(v, int) else "'" + v.replace("'", "''") + "'" for v in row.values()]
            return 'INSERT INTO clan_progression_clans (' + ','.join(row) + ') VALUES (' + ','.join(literals) + ');'

        def state(uid, **values):
            row = dict(clan_uid=uid)
            row.update(values)
            literals = ["'" + v + "'" if isinstance(v, str) else str(v) for v in row.values()]
            return 'INSERT INTO clan_progression_state (' + ','.join(row) + ') VALUES (' + ','.join(literals) + ');'

        # A DDL interruption before the final receipt is recoverable by rerun.
        sql('CREATE DATABASE clan_schema_interrupted')
        client[-1] = 'clan_schema_interrupted'
        sql(MIGRATION.read_text().split('CREATE TABLE IF NOT EXISTS clan_progression_state', 1)[0])
        assert sql('SELECT COUNT(*) FROM clan_progression_schema') == '0'
        migrate()
        assert sql('SELECT version FROM clan_progression_schema') == '1'
        client[-1] = 'clan_schema_test'

        # Empty installation, repeated installation, matching schema contract.
        migrate()
        migrate()
        version = re.search(r'^#define CLAN_PROGRESSION_SCHEMA_VERSION (\d+)$', (ROOT / 'src/clan_progression.h').read_text(), re.M).group(1)
        assert sql('SELECT singleton,version FROM clan_progression_schema') == '1\t1'
        assert int(version) >= 1
        assert sql("SELECT COUNT(*) FROM information_schema.TABLES WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME LIKE 'clan_progression_%' AND ENGINE='InnoDB'") == '3'
        assert sql('SELECT COUNT(*) FROM clan_progression_clans') == '0'
        assert sql('SELECT COUNT(*) FROM clan_progression_state') == '0'
        assert sql("SELECT COUNT(*) FROM information_schema.TRIGGERS WHERE TRIGGER_SCHEMA=DATABASE()") == '4'
        # Malformed identity, reserved vnums, impossible tombstones, nulls/ranges.
        for bad in ['', 'a' * 31, 'a' * 33, 'A' * 32, 'g' * 32, '0' * 32, 'a' * 31 + ' ']:
            sql(clan(bad, 7), ok=False)
        for bad in [0, -1, 65535, 65536]:
            sql(clan(UID_A, bad), ok=False)
        for field, value in [('record_status', 0), ('record_status', 3), ('record_status', 2),
                             ('deleted_at_utc', 2200000000), ('created_at_utc', -1),
                             ('updated_at_utc', 2199999999), ('revision', -1),
                             ('treasury_gold', -1), ('treasury_gold', MAX_I64 + 1),
                             ('leader_player_id', -1), ('founded_at_utc', -1),
                             ('last_activity_at_utc', -1), ('name', None)]:
            sql(clan(UID_A, 7, **{field: value}), ok=False)
        sql(clan(UID_A, 7, treasury_gold=MAX_I64, leader_player_id=5000000000,
                 founded_at_utc=5000000000, last_activity_at_utc=5000000001))
        sql(clan(UID_A, 8), ok=False, error='1062')
        sql(clan(UID_B, 7), ok=False, error='1062')
        sql(state(UID_B), ok=False, error='1452')
        for field in ['xp_eighths', 'influence_eighths', 'revision', 'updated_at_utc']:
            sql(state(UID_A, **{field: -1}), ok=False)
        sql(state(UID_A, xp_eighths=MAX_I64, influence_eighths=MAX_I64,
                  revision=5000000000, updated_at_utc=5000000000))
        sql(state(UID_A), ok=False, error='1062')
        for field in ['treasury_gold', 'revision']:
            sql(f'UPDATE clan_progression_clans SET {field}=-1', ok=False)
        for field in ['xp_eighths', 'influence_eighths', 'revision', 'updated_at_utc']:
            sql(f'UPDATE clan_progression_state SET {field}=-1', ok=False)
            sql(f'UPDATE clan_progression_state SET {field}={MAX_I64 + 1}', ok=False)
        # Signed columns plus CHECKs reject negatives even without strict mode.
        sql('UPDATE clan_progression_state SET influence_eighths=-1', ok=False, strict=False)
        sql('UPDATE clan_progression_clans SET treasury_gold=-1', ok=False, strict=False)
        for assignment in [f"clan_uid='{UID_B}'", 'clan_vnum=8', 'created_at_utc=2200000001']:
            sql('UPDATE clan_progression_clans SET ' + assignment, ok=False, error='1644')
        sql('DELETE FROM clan_progression_clans', ok=False, error='1644')
        sql('DELETE FROM clan_progression_state', ok=False, error='1644')
        # SQL transactions roll back shared treasury/progression together.
        before = sql('SELECT treasury_gold FROM clan_progression_clans; SELECT xp_eighths,influence_eighths,revision,updated_at_utc FROM clan_progression_state')
        sql('START TRANSACTION; UPDATE clan_progression_clans SET treasury_gold=123; UPDATE clan_progression_state SET xp_eighths=321,influence_eighths=456; ROLLBACK;')
        assert before == sql('SELECT treasury_gold FROM clan_progression_clans; SELECT xp_eighths,influence_eighths,revision,updated_at_utc FROM clan_progression_state')
        # Reapply over data, preserving balances, identity, revisions and receipts.
        def snapshot():
            return sql('SELECT * FROM clan_progression_schema; SELECT * FROM clan_progression_clans ORDER BY clan_uid; SELECT * FROM clan_progression_state ORDER BY clan_uid')
        before = snapshot()
        migrate()
        assert snapshot() == before
        # Tombstone frees only the numeric vnum, never the immutable UID/history.
        sql("UPDATE clan_progression_clans SET record_status=2,deleted_at_utc=2200000001,updated_at_utc=2200000001")
        sql("UPDATE clan_progression_clans SET record_status=1,deleted_at_utc=NULL", ok=False, error='1644')
        sql("UPDATE clan_progression_clans SET deleted_at_utc=2200000002,updated_at_utc=2200000002", ok=False, error='1644')
        sql(clan(UID_B, 7))
        sql(state(UID_B))
        assert sql(f"SELECT xp_eighths,influence_eighths FROM clan_progression_state WHERE clan_uid='{UID_B}'") == '0\t0'
        assert sql(f"SELECT xp_eighths FROM clan_progression_state WHERE clan_uid='{UID_A}'") == str(MAX_I64)
        sql(f"UPDATE clan_progression_state SET clan_uid='{UID_C}' WHERE clan_uid='{UID_A}'", ok=False, error='1644')
        sql(clan(UID_C, 7), ok=False, error='1062')
        # A second retained tombstone and third incarnation can share this vnum.
        sql(f"UPDATE clan_progression_clans SET record_status=2,deleted_at_utc=2200000001,updated_at_utc=2200000001 WHERE clan_uid='{UID_B}'")
        sql(clan(UID_C, 7))
        assert sql('SELECT COUNT(*) FROM clan_progression_clans WHERE clan_vnum=7') == '3'
        before = snapshot()
        migrate()
        assert snapshot() == before
        # Refuse future-version reruns instead of downgrading or resetting state.
        sql('UPDATE clan_progression_schema SET version=2')
        before = snapshot()
        failure = migrate(ok=False)
        assert 'cannot replace a different schema version' in failure.stderr
        assert snapshot() == before
        print('Clan schema v1 passed: repeated empty/populated migration; InnoDB rollback; immutable identities; tombstones/vnum reuse; CHECK/FK/unique constraints; negative/overflow balances; 64-bit values; future-version refusal.')
    finally:
        server.terminate()
        try:
            server.wait(timeout=15)
        except subprocess.TimeoutExpired:
            server.kill()
            server.wait(timeout=5)
