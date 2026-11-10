#!/usr/bin/env python3
"""Phase 10: production policy/receipt composition in disposable MariaDB."""
from clan_database import ROOT, database
from clan_snapshot_fixture import snapshot, manifest, A, B
import subprocess, shlex, json, sys
sys.path.insert(0, str(ROOT/'scripts'))
from build_clan_repository_tool import build
migrations=tuple(f'20261110_clan_progression_v{i:03d}.sql' for i in range(1,6))
with database(migrations) as (tmp, socket, sql, client):
    tool=tmp/'tool';build(tool,sanitize=True)
    subprocess.run([str(tool),'import','clan_test','root','',str(socket)],input=json.dumps(dict(snapshot=snapshot(),expected_revision=0,manifest=manifest())),text=True,check=True,stdout=subprocess.DEVNULL)
    sql('UPDATE clan_store_control SET store_mode=2; UPDATE clan_import_manifests SET import_status=2;')
    binary=tmp/'personal'
    flags=shlex.split(subprocess.check_output(['pkg-config','--cflags','--libs','mariadb','openssl','json-c'],text=True))
    subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-pedantic','-g','-DCLAN_REWARD_TEST','-fsanitize=address,undefined','-fno-pie','-no-pie','-I'+str(ROOT/'src'),str(ROOT/'tests/clan_rewards.c'),str(ROOT/'src/clan_repository.c'),str(ROOT/'src/clan_awards.c'),str(ROOT/'src/clan_progression.c'),*flags,'-o',str(binary)],check=True)
    seq=0
    def event(**changes):
        global seq
        seq+=1
        return dict(operation_uid=f'{seq:032x}',event_uid=f'{seq+10000:032x}',clan_uid=A,account_id=77,player_id=5000000007,source=2,rules_version=1,occurred_at=2200000001,now=2200000002,requested_xp_eighths=800,requested_influence_eighths=400,requested_favor_eighths=400)|changes
    def run(op, body, expected='ok'):
        p=subprocess.run([str(binary),str(socket),op],input=json.dumps(body),text=True,capture_output=True)
        assert p.returncode==0,(p.returncode,p.stdout,p.stderr)
        result=json.loads(p.stdout)
        assert result['result']==expected,(op,result,p.stderr)
        return result['receipt']
    def complete(e, **changes):return run('personal',e|changes)
    def balances():return sql('SELECT * FROM clan_progression_state; SELECT * FROM clan_account_windows; SELECT * FROM clan_personal_favor; SELECT * FROM clan_weekly_windows;')
    # First successful personal reward creates all rows in the same transaction.
    first=event();run('prepare',first)
    run('personal',first|dict(disabled=True),'invalid')
    run('personal',first|dict(unready=True),'invalid')
    before=balances()
    for stage in range(2,8):
        run('personal',first|dict(fault=stage),'unavailable');assert balances()==before
    run('personal',first|dict(fault=8),'commit-unknown')
    receipt=run('load',first);assert receipt['status']==1
    assert complete(first)==receipt
    d=receipt['decision']['delta']
    assert (d['xp_eighths'],d['influence_eighths'],d['favor_eighths'],d['account_claims'])==(800,400,400,1)
    assert complete(first,disabled=True)==receipt
    # A second character in a different clan shares account bands and source quota.
    sql('UPDATE clan_account_windows SET base_xp_eighths=4400,revision=revision+1 WHERE account_id=77')
    alt=event(player_id=5000000009,clan_uid=B);run('prepare',alt)
    receipt=complete(alt);d=receipt['decision']['delta']
    assert (d['xp_eighths'],d['influence_eighths'],d['favor_eighths'])==(600,300,300)
    assert d['cap_flags']==16 and receipt['decision']['after']['account_base_xp']==5200
    # Account limits survive a fresh process, changed clan and a delayed retry.
    third=event();run('prepare',third);d=complete(third,now=2200604802)['decision']['delta']
    assert (d['xp_eighths'],d['favor_eighths'])==(400,200)
    fourth=event(player_id=5000000009,clan_uid=B);run('prepare',fourth)
    d=complete(fourth,now=2200604802)['decision']['delta']
    assert d['cap_flags']==32 and not any(d[k] for k in ['xp_eighths','favor_eighths','account_claims','account_base_xp_eighths'])
    assert sql('SELECT supply_order_claims FROM clan_account_windows WHERE account_id=77')=='3'
    assert sql('SELECT COUNT(*) FROM clan_account_windows WHERE account_id=77')=='1'
    # Exhausted account XP does not block expedition Favor; Favor itself is capped.
    sql('UPDATE clan_account_windows SET base_xp_eighths=9600,favor_eighths=4799,revision=revision+1 WHERE account_id=77')
    expedition=event(source=6,requested_xp_eighths=0,requested_influence_eighths=0,requested_favor_eighths=800)
    run('prepare',expedition);d=complete(expedition,now=2200604802)['decision']['delta']
    assert d['favor_eighths']==1 and d['account_claims']==1 and d['cap_flags']==64
    capped=event(source=6,requested_xp_eighths=0,requested_influence_eighths=0,requested_favor_eighths=800)
    run('prepare',capped);d=complete(capped,now=2200604802)['decision']['delta'];assert d['favor_eighths']==0 and d['account_claims']==1
    capped_again=event(source=6,requested_xp_eighths=0,requested_influence_eighths=0,requested_favor_eighths=800)
    run('prepare',capped_again);d=complete(capped_again,now=2200604802)['decision']['delta'];assert d['cap_flags']==32 and not d['account_claims']
    # At clan level/storage caps, another account can still accumulate Favor.
    sql(f"UPDATE clan_progression_state SET xp_eighths=304000,influence_eighths=160000,revision=revision+1 WHERE clan_uid='{B}'")
    favor=event(account_id=88,player_id=5000000010,clan_uid=B);run('prepare',favor)
    d=complete(favor,now=2200604802)['decision']['delta'];assert not d['xp_eighths'] and not d['influence_eighths'] and d['favor_eighths']==400
    # A prototype-reduced kill composes with account and clan bands.
    reduced=event(account_id=99,player_id=5000000011,source=1,requested_xp_eighths=8,requested_influence_eighths=4,requested_favor_eighths=4)
    run('prepare',reduced);d=complete(reduced,now=2200604802)['decision']['delta'];assert d['xp_eighths']==8 and d['favor_eighths']==4 and d['cap_flags']==128
    sql('UPDATE clan_account_windows SET base_xp_eighths=4800,kill_claims=199,revision=revision+1 WHERE account_id=99')
    # Concurrent alt claims share the last eligible encounter; only one is paid.
    races=[event(account_id=99,player_id=5000000011+i,source=1,requested_xp_eighths=16,requested_influence_eighths=8,requested_favor_eighths=8) for i in range(2)]
    processes=[]
    for e in races:
        run('prepare',e)
        p=subprocess.Popen([str(binary),str(socket),'personal'],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True)
        p.stdin.write(json.dumps(e|dict(now=2200604802)));p.stdin.close();processes.append(p)
    outcomes=[]
    for p in processes:
        out=p.stdout.read();err=p.stderr.read();assert p.wait()==0,(out,err)
        parsed=json.loads(out);assert parsed['result']=='ok',(parsed,err);outcomes.append(parsed['receipt']['decision']['delta'])
    assert sorted(d['favor_eighths'] for d in outcomes)==[0,4]
    assert sql('SELECT kill_claims FROM clan_account_windows WHERE account_id=99')=='200'
    # Unsupported amounts/rules cannot mint Favor; cumulative overflow rolls back.
    bad=event(account_id=101,player_id=5000000013,requested_favor_eighths=401)
    run('prepare',bad);before=balances();run('personal',bad|dict(now=2200604802),'invalid');assert balances()==before
    overflow=event(account_id=88,player_id=5000000010,clan_uid=B)
    run('prepare',overflow)
    sql(f"UPDATE clan_personal_favor SET favor_eighths=9223372036854775807,revision=revision+1 WHERE player_id=5000000010 AND clan_uid='{B}'")
    before=balances();run('personal',overflow|dict(now=2200604802),'invalid');assert balances()==before
    # Accepted work keeps its original week after a Monday reset.
    assert sql('SELECT COUNT(*) FROM clan_account_windows WHERE account_id=99')=='1'
    print('Phase 10 SQL: atomic Favor/account awards, shared alt limits, exact bands, zero/capped receipts, replay, reset recovery, rollback, concurrent last claim, tuple rejection and overflow passed.')
