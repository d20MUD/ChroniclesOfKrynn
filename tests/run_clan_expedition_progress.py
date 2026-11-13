#!/usr/bin/env python3
"""Phases 35-36 production combat progress and proof-backed shared/personal claims."""
from clan_database import ROOT,database
from clan_snapshot_fixture import snapshot,manifest,A,B
import json,subprocess,shlex,sys
sys.path.insert(0,str(ROOT/'scripts'))
from build_clan_repository_tool import build
migrations=tuple(f'20261110_clan_progression_v{i:03d}.sql' for i in range(1,6))+('20261112_clan_expeditions_v001.sql','20261112_clan_expeditions_v002.sql','20261111_clan_kill_sources_v001.sql','20261111_clan_objectives_v001.sql',)
with database(migrations) as (tmp,socket,sql,client):
    tool=tmp/'tool';build(tool,sanitize=True)
    subprocess.run([str(tool),'import','clan_test','root','',str(socket)],input=json.dumps(dict(snapshot=snapshot(),expected_revision=0,manifest=manifest())),text=True,check=True,stdout=subprocess.DEVNULL)
    sql('UPDATE clan_store_control SET store_mode=2;UPDATE clan_import_manifests SET import_status=2')
    flags=shlex.split(subprocess.check_output(['pkg-config','--cflags','--libs','mariadb','json-c','openssl'],text=True));binary=tmp/'enrollment'
    subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-pedantic','-g','-DCLAN_REWARD_TEST','-fsanitize=address,undefined','-fno-pie','-no-pie','-Isrc',*[str(ROOT/p) for p in ['tests/clan_expedition_enrollment.c','src/clan_expedition_enrollment.c','src/clan_expeditions.c','src/clan_repository.c','src/clan_awards.c','src/clan_progression.c','src/clan_combat.c']],*flags,'-o',str(binary)],cwd=ROOT,check=True)
    catalog=json.loads((ROOT/'lib/clan/expeditions_legacy.json').read_text())
    clock=2200010001;seq=20000
    def uid():
        global seq;seq+=1;return f'{seq:032x}'
    leader=dict(player=5000000007,account=77,level=27)
    others=[dict(player=5000000010+i,account=80+i,level=27) for i in range(5)]
    for m in others:sql(f"INSERT INTO clan_memberships VALUES({m['player']},'{A}',2,0)")
    base=dict(clan=A,now=clock,catalog=catalog,definition='icewall-assault-v1',outing=uid())
    def run(op,body=None,expected='ok',crash=None):
        p=subprocess.run([str(binary),str(socket),op],input=json.dumps(base|leader|(body or {})),text=True,capture_output=True)
        if crash is not None:
            assert p.returncode==crash,(op,p.returncode,p.stdout,p.stderr);return
        assert p.returncode==0,(op,p.stdout,p.stderr)
        r=json.loads(p.stdout);assert r['result']==expected,(op,r,p.stderr);return r['data']
    def outing(family=1,roster=None):
        base['outing']=uid();base['definition']='icewall-assault-v1' if family==1 else 'icewall-hunt-v1'
        run('create')
        for m in roster or others[:1]:run('join',m)
        run('start',dict(members=[leader]+(roster or others[:1])))
        return base['outing']
    def occurrence(target=14100,area=141,members=None,**kw):
        global clock;clock+=1
        return dict(event=uid(),target=target,area=area,members=members or [leader,others[0]],now=clock)|kw
    def finish(family=1,members=None):
        if family==1:
            for _ in range(20):run('combat',occurrence(members=members))
            run('combat',occurrence(14001,members=members));run('combat',occurrence(14000,members=members))
        else:
            for target,area in [(14012,140),(14015,140),(14106,141)]:
                for _ in range(10):run('combat',occurrence(target,area,members))
    def claim(m=leader,expected='ok'):
        return run('claim',m|dict(now=clock),expected)
    def pay(receipt,**kw):return run('complete',dict(operation=receipt['event']['operation_uid'],now=clock)|kw)
    first=outing()
    # Only actual designated encounters with two eligible enrolled accounts count.
    run('combat',occurrence(999),'not-found');run('combat',occurrence(area=140),'not-found')
    run('combat',occurrence(members=[leader]),'ineligible')
    run('combat',occurrence(members=[leader,others[0]|dict(account=77)]),'ineligible')
    for bad in [dict(owned=1),dict(test=1),dict(trivial=1)]:run('combat',occurrence(**bad),'not-found')
    run('combat',occurrence(disabled=1),'ineligible')
    assert run('progress')==[0,0,0]
    e=occurrence();run('combat',e|dict(expedition_crash=1),crash=93);assert run('progress')==[0,0,0]
    run('combat',e|dict(expedition_crash=2),crash=94);run('combat',e);assert run('progress')==[1,0,0]
    run('combat',e|dict(target=14000),'conflict')
    run('combat',e|dict(members=[leader,others[0]|dict(level=26)]),'conflict')
    run('join',others[1],'conflict');run('leave',others[0],'conflict')
    # Same participation result covers heal/mitigate/support/defense/tank roles.
    for kind in [1,2,3,4,5]:run('combat',occurrence(members=[leader,others[0]|dict(kind=kind)]))
    assert run('progress')==[6,0,0]
    for _ in range(14):run('combat',occurrence())
    run('combat',occurrence(),'not-found') # defender respawns cannot exceed quota
    run('combat',occurrence(14103));last=occurrence(14009)
    # Completion, final event, entitlements and shared receipt roll back together.
    sql("CREATE TRIGGER ce_test_finish BEFORE INSERT ON clan_expedition_entitlements FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='test failure'")
    run('combat',last,'ineligible');assert run('progress')==[20,1,0]
    assert sql('SELECT COUNT(*) FROM clan_expedition_completions')=='0'
    sql('DROP TRIGGER ce_test_finish');run('combat',last);run('combat',last)
    assert run('progress')==[20,1,1];run('read',expected='not-found')
    assert sql('SELECT COUNT(*) FROM clan_expedition_entitlements')=='2'
    assert len(run('claims'))==1
    shared=sql('SELECT operation_uid FROM clan_expedition_shared_claims')
    shared_receipt=run('complete',dict(operation=shared,now=clock))
    assert shared_receipt['decision']['delta']['xp_eighths']==300
    assert shared_receipt['decision']['delta']['influence_eighths']==300
    assert run('complete',dict(operation=shared,now=clock))==shared_receipt
    prepared=claim();op=prepared['event']['operation_uid']
    run('generic',dict(operation=op,now=clock),'ineligible')
    # Participant claims roll back with currency/counters and recover lost ACKs.
    for fault in range(2,8):run('complete',dict(operation=op,now=clock,fault=fault),'unavailable')
    assert sql(f"SELECT state FROM clan_expedition_entitlements WHERE operation_uid='{op}'")=='1'
    run('complete',dict(operation=op,now=clock,fault=8),'commit-unknown');paid=pay(prepared)
    assert paid['decision']['delta']['favor_eighths']==200 and pay(prepared)==paid
    assert pay(claim())==paid
    # A later participant still receives personal credit without another shared award.
    second=outing(roster=[others[1]])
    finish(members=[leader,others[1]])
    assert sql('SELECT COUNT(*) FROM clan_expedition_shared_claims')=='1'
    claim(expected='ineligible')
    assert pay(claim(others[1]))['decision']['delta']['favor_eighths']==200
    # A departure and immediate rejoin invalidate an unclaimed entitlement.
    sql(f"DELETE FROM clan_memberships WHERE player_id={others[0]['player']};INSERT INTO clan_memberships VALUES({others[0]['player']},'{A}',2,0)")
    run('claim',others[0]|dict(outing=first,now=clock),'ineligible')
    # Both families can earn shared/personal rewards, with separate weekly keys.
    hunt=outing(2,roster=others[1:3])
    finish(2,[leader]+others[1:3])
    assert run('progress')==[10,10,10]
    assert sql('SELECT COUNT(*) FROM clan_expedition_shared_claims')=='2'
    sql('UPDATE clan_account_windows SET favor_eighths=1199,revision=revision+1 WHERE account_id=81')
    capped=claim(others[1]);assert pay(capped)['decision']['delta']['favor_eighths']==1
    pending=claim();assert pending['event']['source']==6
    # A durably accepted claim retains identity/week through departure and reset.
    sql(f"UPDATE clan_memberships SET clan_uid='{B}',rank_id=1 WHERE player_id={leader['player']}")
    recovered=pay(pending,now=2200608000)
    assert recovered['event']['clan_uid']==A and recovered['event']['occurred_at']==clock
    assert recovered['decision']['delta']['favor_eighths']==200
    sql(f"UPDATE clan_memberships SET clan_uid='{A}',rank_id=1 WHERE player_id={leader['player']}")
    # Unclaimed entitlements cannot be revived after the original weekly deadline.
    run('claim',others[2]|dict(now=2200608000),'ineligible')
    # Roster members absent from the final kill cannot earn personal completion.
    outing(roster=others[:2])
    for _ in range(20):run('combat',occurrence(members=[leader,others[0],others[1]]))
    run('combat',occurrence(14001,members=[leader,others[0],others[1]]));run('combat',occurrence(14000))
    assert len(run('claims',others[1]|dict(now=clock)))==2 # previous earned completions only
    # Shared receipts are proof-backed, not arbitrary generic source awards.
    assert sql('SELECT COUNT(*) FROM clan_expedition_shared_claims')=='2'
    # Low attendance cannot finish; an exact half participant can finish later.
    account_a=others[0]|dict(account=200);account_b=others[1]|dict(account=200)
    threshold=outing(roster=[account_a,others[1],others[2],others[3]])
    for _ in range(10):run('combat',occurrence(members=[leader,account_a]))
    for _ in range(10):run('combat',occurrence(members=[leader,others[1]]))
    run('combat',occurrence(14001,members=[leader,others[2]]))
    run('combat',occurrence(14000,members=[leader,others[3]]),'ineligible')
    assert run('progress')==[20,1,0]
    run('combat',occurrence(14000,members=[leader,account_a]))
    assert sql(f"SELECT COUNT(*) FROM clan_expedition_entitlements WHERE outing_uid='{threshold}'")=='2'
    last_alt=outing(roster=[account_b]);finish(members=[leader,account_b])
    requests=[base|account_a|dict(outing=threshold,now=clock),base|account_b|dict(outing=last_alt,now=clock)]
    children=[]
    for request in requests:
        p=subprocess.Popen([str(binary),str(socket),'claim'],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True)
        p.stdin.write(json.dumps(request));p.stdin.close();children.append(p)
    results=[]
    for p in children:
        output=p.stdout.read();error=p.stderr.read();assert p.wait()==0,(output,error)
        results.append(json.loads(output))
    assert sorted(r['result'] for r in results)==['ineligible','ok'],results
    accepted=next(r['data'] for r in results if r['result']=='ok')
    assert pay(accepted,now=2200608000)['decision']['delta']['favor_eighths']==200
    assert sql('SELECT COUNT(*) FROM clan_expedition_personal_claims WHERE account_id=200')=='1'
    before=sql('SELECT * FROM clan_expedition_events;SELECT * FROM clan_expedition_completions;SELECT * FROM clan_expedition_entitlements')
    sql((ROOT/'sql/20261112_clan_expeditions_v001.sql').read_text())
    sql((ROOT/'sql/20261112_clan_expeditions_v002.sql').read_text())
    assert sql('SELECT * FROM clan_expedition_events;SELECT * FROM clan_expedition_completions;SELECT * FROM clan_expedition_entitlements')==before
    native=tmp/'live'
    subprocess.run(['cc','-std=gnu99','-g','-Wall','-Wextra','-Wno-unused-parameter','-ffunction-sections','-fdata-sections','-Wl,--gc-sections','-DCLAN_REWARD_TEST','-Dtime=objective_test_time','-fsanitize=address,undefined','-fno-pie','-no-pie','-Isrc',*[str(ROOT/p) for p in ['tests/clan_objective_runtime.c','src/clan_objective_runtime.c','src/clan_objectives.c','src/clan_sources.c','src/clan_combat.c','src/clan_repository.c','src/clan_awards.c','src/clan_progression.c','src/clan_expedition_enrollment.c','src/clan_expeditions.c']],*flags,'-o',str(native)],cwd=ROOT,check=True)
    # A separate clan incarnation/week isolates the live collector's history checks.
    # Keep retention intact: native harness compares its occurrence delta.
    subprocess.run([str(native),str(socket),str(tmp/'live-checkpoint'),'expedition-heal','2200610001'],cwd=ROOT/'lib',check=True)
    command_binary=tmp/'commands'
    subprocess.run(['cc','-std=gnu99','-g','-Wall','-Wextra','-Wno-unused-parameter','-ffunction-sections','-fdata-sections','-Wl,--gc-sections','-Dtime=expedition_test_time','-fsanitize=address,undefined','-fno-pie','-no-pie','-Isrc',*[str(ROOT/p) for p in ['tests/clan_expedition_runtime.c','src/clan_expedition_runtime.c','src/clan_expedition_enrollment.c','src/clan_expeditions.c','src/clan_repository.c','src/clan_awards.c','src/clan_progression.c','src/clan_combat.c']],*flags,'-o',str(command_binary)],cwd=ROOT,check=True)
    completed_uid=sql('SELECT outing_uid FROM clan_expedition_completions ORDER BY completed_at DESC LIMIT 1')
    subprocess.run([str(command_binary),str(socket),completed_uid],cwd=ROOT/'lib',check=True)
    sql('DROP TRIGGER ce_shared_immutable');run('claims',expected='schema-mismatch')
    print('Phases 35-36: designated combat, role qualification, bounded/atomic progress, event replay/conflict/crashes, sealed roster, completion participation/final objective, shared dedup, personal family/account limits, atomic capped claims, membership/reset recovery and retained SQL history passed.')
