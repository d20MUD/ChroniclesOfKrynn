#!/usr/bin/env python3
"""Approved rules-2 production policy, atomic receipts and fail-closed cutover."""
from clan_database import ROOT, database
from clan_snapshot_fixture import snapshot, manifest, A
import json, shlex, subprocess, sys
sys.path.insert(0,str(ROOT/'scripts'))
from build_clan_repository_tool import build
flags=shlex.split(subprocess.check_output(['pkg-config','--cflags','--libs','mariadb','openssl','json-c'],text=True))
core=tuple(f'20261110_clan_progression_v{i:03d}.sql' for i in range(1,6))
with database(core) as (tmp,socket,sql,client):
    def compile(path,driver,*extra):
        subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-pedantic',
            '-fsanitize=address,undefined','-fno-pie','-no-pie','-I'+str(ROOT/'src'),
            *extra,str(driver),*[str(ROOT/'src'/n) for n in ['clan_progression.c','clan_awards.c','clan_repository.c']],*flags,'-o',str(path)],check=True)
    for percent in (100,50):
        binary=tmp/'pure'
        compile(binary,ROOT/'tests/clan_pacing.c',f'-DCLAN_FAVOR_PACE_PERCENT={percent}',f'-DCLAN_INFLUENCE_PACE_PERCENT={percent}')
        subprocess.run([str(binary)],check=True)
    tool=tmp/'tool';build(tool,sanitize=True)
    subprocess.run([str(tool),'import','clan_test','root','',str(socket)],input=json.dumps(dict(snapshot=snapshot(),expected_revision=0,manifest=manifest())),text=True,check=True,stdout=subprocess.DEVNULL)
    sql('UPDATE clan_store_control SET store_mode=2;UPDATE clan_import_manifests SET import_status=2')
    driver=tmp/'driver.c'
    driver.write_text((ROOT/'tests/clan_rewards.c').read_text().replace('clan_progression_v1','clan_progression_defaults'))
    binary=tmp/'rewards';compile(binary,driver,'-DCLAN_REWARD_TEST')
    event=dict(operation_uid='1'*32,event_uid='2'*32,clan_uid=A,account_id=77,player_id=5000000007,source=1,rules_version=2,occurred_at=2200000001,now=2200000002,requested_xp_eighths=16,requested_influence_eighths=8,requested_favor_eighths=8)
    def call(op,expected='ok',**changes):
        p=subprocess.run([str(binary),str(socket),op],input=json.dumps(event|changes),text=True,capture_output=True,check=True)
        out=json.loads(p.stdout);assert out['result']==expected,(op,out,p.stderr)
        return out['receipt']
    call('prepare')
    def state():return sql('SELECT * FROM clan_progression_state;SELECT * FROM clan_personal_favor;SELECT * FROM clan_account_windows;SELECT * FROM clan_weekly_windows')
    before=state()
    for stage in range(2,8):
        call('personal','unavailable',fault=stage);assert state()==before
    receipt=call('personal');delta=receipt['decision']['delta']
    assert (delta['xp_eighths'],delta['influence_eighths'],delta['favor_eighths'])==(1,1,2)
    after=state();assert call('personal',disabled=True)==receipt and state()==after
    sql(f"UPDATE clan_progression_state SET influence_eighths=0,revision=revision+1 WHERE clan_uid='{A}'")
    assert sql(f"SELECT xp_eighths FROM clan_progression_state WHERE clan_uid='{A}'")=='1'
    assert call('load')==receipt # paid receipt remains immutable after spending
    # Re-running v6 is harmless, but attempting the cutover on a used v5 economy fails.
    migration=(ROOT/'sql/20261113_clan_progression_v006.sql').read_text()
    sql(migration)
    sql('UPDATE clan_progression_schema SET version=5 WHERE singleton=1')
    rejected=subprocess.run(client,input=migration,text=True,capture_output=True)
    assert rejected.returncode and 'unused economy' in rejected.stderr
    assert sql('SELECT version FROM clan_progression_schema')=='5'
print('Rules 2 SQL: every partial-write rollback, exact fractional credit, replay, spending and unsafe migration refusal passed.')
