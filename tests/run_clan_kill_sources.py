#!/usr/bin/env python3
"""Actual receipt acceptance: prototype/day quotas, replay and crash recovery."""
from clan_database import ROOT,database
from clan_snapshot_fixture import snapshot,manifest,A,B
import subprocess,shlex,json,sys
sys.path.insert(0,str(ROOT/'scripts'))
from build_clan_repository_tool import build
migrations=tuple(f'20261110_clan_progression_v{i:03d}.sql' for i in range(1,6))+('20261111_clan_kill_sources_v001.sql',)
with database(migrations) as (tmp,socket,sql,client):
    tool=tmp/'tool';build(tool,sanitize=True)
    subprocess.run([str(tool),'import','clan_test','root','',str(socket)],input=json.dumps(dict(snapshot=snapshot(),expected_revision=0,manifest=manifest())),text=True,check=True,stdout=subprocess.DEVNULL)
    sql('UPDATE clan_store_control SET store_mode=2; UPDATE clan_import_manifests SET import_status=2;')
    binary=tmp/'source'
    flags=shlex.split(subprocess.check_output(['pkg-config','--cflags','--libs','mariadb','openssl','json-c'],text=True))
    subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-pedantic','-g','-DCLAN_REWARD_TEST','-fsanitize=address,undefined','-fno-pie','-no-pie','-I'+str(ROOT/'src'),str(ROOT/'tests/clan_rewards.c'),str(ROOT/'src/clan_repository.c'),str(ROOT/'src/clan_awards.c'),str(ROOT/'src/clan_progression.c'),*flags,'-o',str(binary)],check=True)
    def event(i,**kw):
        return dict(operation_uid=f'{i:032x}',event_uid=f'{i+10000:032x}',clan_uid=A,account_id=77,player_id=5000000007,prototype=4301,source=1,rules_version=1,occurred_at=2200000001,now=2200000002,requested_xp_eighths=16,requested_influence_eighths=8,requested_favor_eighths=8)|kw
    def run(mode,e,expected='ok'):
        p=subprocess.run([str(binary),str(socket),mode],input=json.dumps(e),text=True,capture_output=True)
        assert p.returncode==0,(p.returncode,p.stdout,p.stderr)
        r=json.loads(p.stdout);assert r['result']==expected,(mode,r,p.stderr)
        return r['receipt']
    def state():return sql('SELECT COUNT(*) FROM clan_kill_sources;SELECT COUNT(*) FROM clan_reward_receipts;SELECT * FROM clan_progression_state')
    # First twenty full, next twenty half, later zero; fresh processes each time.
    receipts=[]
    for i in range(1,43):
        e=event(i);r=run('prepare-kill',e);receipts.append(r)
        mult=2 if i<=20 else 1 if i<=40 else 0
        assert [r['event'][k] for k in ('requested_xp_eighths','requested_influence_eighths','requested_favor_eighths')]==[8*mult,4*mult,4*mult]
        paid=run('personal',e)
        assert paid['decision']['delta']['xp_eighths']==8*mult
        assert run('prepare-kill',e)['event']==r['event']
    before=state()
    # Another operation nonce returns the original frozen reduced receipt.
    assert run('prepare-kill',event(21,operation_uid='f'*32))['event']==receipts[20]['event']
    run('prepare-kill',event(21,prototype=4302),'conflict');assert state()==before
    # UTC day and prototype boundaries are independent.
    assert run('prepare-kill',event(50,prototype=4302))['event']['requested_xp_eighths']==16
    assert run('prepare-kill',event(51,occurred_at=2200086401,now=2200086402))['event']['requested_xp_eighths']==16
    # An account cannot claim the same encounter with an alt.
    sql(f"INSERT INTO clan_memberships(player_id,clan_uid,rank_id,legacy_points) VALUES(5000000012,'{A}',2,0)")
    run('prepare-kill',event(52,event_uid=event(50)['event_uid'],player_id=5000000012),'conflict')
    # Daily account limits follow membership changes, not the clan incarnation.
    sql(f"UPDATE clan_memberships SET clan_uid='{B}' WHERE player_id=5000000012")
    r=run('prepare-kill',event(53,player_id=5000000012,clan_uid=B))
    assert r['event']['requested_xp_eighths']==0
    sql(f"UPDATE clan_memberships SET clan_uid='{A}' WHERE player_id=5000000012")
    # Concurrent acceptance serializes the full-to-half boundary under control lock.
    for i in range(100,119):run('prepare-kill',event(i,prototype=4400))
    children=[]
    for i in range(120,122):
        p=subprocess.Popen([str(binary),str(socket),'prepare-kill'],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True)
        p.stdin.write(json.dumps(event(i,prototype=4400)));p.stdin.close();p.stdin=None;children.append(p)
    amounts=[]
    for p in children:
        out,err=p.communicate();assert p.returncode==0,(out,err)
        r=json.loads(out);assert r['result']=='ok',r
        amounts.append(r['receipt']['event']['requested_xp_eighths'])
    assert sorted(amounts)==[8,16]
    # Lost acknowledgement retains one source row and one receipt.
    e=event(60,prototype=4303,fault=1)
    run('prepare-kill',e,'commit-unknown')
    assert run('prepare-kill',e|dict(fault=0))['event']['requested_xp_eighths']==16
    assert sql('SELECT COUNT(*) FROM clan_kill_sources WHERE prototype=4303')=='1'
    run('personal',e|dict(fault=0))
    # Zero-source receipts remain terminal and do not consume a weekly claim.
    assert sql('SELECT kill_claims FROM clan_account_windows WHERE account_id=77')=='41'
    print('Kill source SQL: daily full/half/zero, exact replay, new nonce, account dedup, UTC/prototype reset and lost acknowledgement passed.')

    # Exercise the actual live collector and supply checkpoint adapters with SQL.
    native=tmp/'native'
    subprocess.run(['cc','-std=gnu99','-g','-fsanitize=address,undefined','-fno-pie','-no-pie',
        '-ffunction-sections','-fdata-sections','-Wl,--gc-sections','-Dtime=clan_source_test_time','-I'+str(ROOT/'src'),str(ROOT/'tests/clan_sources.c'),
        *[str(ROOT/'src'/f) for f in ['clan_objectives.c','clan_objective_runtime.c','clan_sources.c','clan_combat.c','clan_repository.c','clan_awards.c','clan_progression.c','clan_expedition_enrollment.c','clan_expeditions.c']],*flags,'-o',str(native)],check=True)
    for index,(mode,expected) in enumerate([('kill',1),('alt',1),('support',1),('heal',1),('tank',1),('pet',1),('confuser',1),('outage',1),('outroom-pet',0),('one-hit-bonus',0),
        ('disabled',0),('trivial',0),('owned',0),('outsider',0),('insufficient',0),('dead',0),('abandoned',0),('reset',1),('long',1),('overflow',0),
        ('supply',1),('supply-replay',1),('supply-failed-save',0),('supply-deleted',0)]):
        before=int(sql('SELECT COUNT(*) FROM clan_reward_receipts'))
        committed=int(sql('SELECT COUNT(*) FROM clan_reward_receipts WHERE receipt_status=1'))
        subprocess.run([str(native),str(socket),mode,str(2200172801+index*1800)],check=True)
        assert int(sql('SELECT COUNT(*) FROM clan_reward_receipts WHERE receipt_status=1'))>=committed+expected,mode
        after=int(sql('SELECT COUNT(*) FROM clan_reward_receipts'))
        assert after-before==expected,(mode,before,after)
    print('Live adapters: capped character, account alt dedup, support, healing, tanking, exclusions, repeated death, successful supply checkpoint and failed save passed.')

    before=state()
    sql('DROP TRIGGER cp_kill_immutable')
    run('prepare-kill',event(999,prototype=9900),'schema-mismatch')
    assert state()==before
    print('Missing kill-source retention guard fails closed without accepting a receipt.')
