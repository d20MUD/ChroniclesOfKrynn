#!/usr/bin/env python3
"""Phase 34 durable enrollment, opt-in rosters, membership invalidation and gates."""
from clan_database import ROOT,database
from clan_snapshot_fixture import snapshot,manifest,A,B
import json,subprocess,shlex,sys
sys.path.insert(0,str(ROOT/'scripts'))
from build_clan_repository_tool import build
migrations=tuple(f'20261110_clan_progression_v{i:03d}.sql' for i in range(1,6))+('20261112_clan_expeditions_v001.sql',)
with database(migrations) as (tmp,socket,sql,client):
    tool=tmp/'tool';build(tool,sanitize=True)
    subprocess.run([str(tool),'import','clan_test','root','',str(socket)],input=json.dumps(dict(snapshot=snapshot(),expected_revision=0,manifest=manifest())),text=True,check=True,stdout=subprocess.DEVNULL)
    sql('UPDATE clan_store_control SET store_mode=2;UPDATE clan_import_manifests SET import_status=2')
    flags=shlex.split(subprocess.check_output(['pkg-config','--cflags','--libs','mariadb','json-c','openssl'],text=True));binary=tmp/'enrollment'
    subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-pedantic','-g','-fsanitize=address,undefined','-fno-pie','-no-pie','-Isrc',*[str(ROOT/p) for p in ['tests/clan_expedition_enrollment.c','src/clan_expedition_enrollment.c','src/clan_expeditions.c','src/clan_repository.c','src/clan_awards.c','src/clan_progression.c','src/clan_combat.c']],*flags,'-o',str(binary)],cwd=ROOT,check=True)
    catalog=json.loads((ROOT/'lib/clan/expeditions.json').read_text())
    now=2200010001;seq=9000
    def uid():
        global seq;seq+=1;return f'{seq:032x}'
    leader=dict(player=5000000007,account=77,level=27)
    members=[leader]+[dict(player=5000000010+i,account=80+i,level=27) for i in range(7)]
    for m in members[1:]:sql(f"INSERT INTO clan_memberships VALUES({m['player']},'{A}',2,0)")
    base=dict(clan=A,now=now,catalog=catalog,definition='icewall-assault-v1',outing=uid())
    def run(op,body=None,expected='ok'):
        p=subprocess.run([str(binary),str(socket),op],input=json.dumps(base|leader|(body or {})),text=True,capture_output=True)
        assert p.returncode==0,(op,p.stdout,p.stderr)
        result=json.loads(p.stdout);assert result['result']==expected,(op,result,p.stderr);return result['data']
    run('create',dict(disabled=1),'ineligible');run('create',dict(bad_world=1),'invalid')
    run('create',dict(player=5000000009,clan=B),'ineligible');run('create',dict(level=2),'ineligible')
    sql("CREATE TRIGGER ce_test_failure BEFORE INSERT ON clan_expedition_roster FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='injected roster failure'")
    run('create',expected='unavailable')
    assert sql('SELECT COUNT(*) FROM clan_expedition_outings')=='0'
    sql('DROP TRIGGER ce_test_failure')
    run('create');run('create') # lost acknowledgement, stable identity
    run('create',dict(outing=uid()),'conflict')
    out=run('read');assert out['outing_uid']==base['outing'] and len(out['roster'])==1
    assert out['expires_at']==2200608000 and out['started_at']==0 and out['state']==0 and not out['roster_locked']
    # Ordinary repository saves must not fire departure invalidation for unchanged members.
    def toolrun(op,body=None):
        p=subprocess.run([str(tool),op,'clan_test','root','',str(socket)],input=json.dumps(body or {}),text=True,capture_output=True)
        assert p.returncode==0,(op,p.stdout,p.stderr)
        return json.loads(p.stdout)
    saved=toolrun('read');saved['written_at']=now
    saved['clans'][0]['core']['updated_at']=now
    saved['clans'][0]['core']['treasury_gold']-=1
    toolrun('save',dict(snapshot=saved,expected_revision=saved['store_revision']))
    assert not run('read')['roster'][0]['inactive']
    # A failed snapshot must roll back real departures and their trigger effects.
    failed=toolrun('read');failed['written_at']=now+1
    failed['members']=[m for m in failed['members'] if m['player_id']!=leader['player']]
    sql("CREATE TRIGGER ce_test_snapshot BEFORE INSERT ON clan_zone_claims FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='injected snapshot failure'")
    p=subprocess.run([str(tool),'save','clan_test','root','',str(socket)],input=json.dumps(dict(snapshot=failed,expected_revision=failed['store_revision'])),text=True,capture_output=True)
    assert p.returncode!=0
    assert not run('read')['roster'][0]['inactive']
    sql('DROP TRIGGER ce_test_snapshot')

    run('start',dict(members=[leader]),'ineligible')
    run('join',members[1]|dict(account=77),'conflict') # same account alt
    run('join',members[1]);run('join',members[1])
    run('start',dict(members=[leader,members[1]],player=members[1]['player']),'ineligible')
    run('start',dict(members=[leader,members[1]|dict(account=99)]),'ineligible')
    run('start',dict(members=[leader,members[1]|dict(level=1)]),'ineligible')
    run('start',dict(members=[leader,members[1]]));run('start',dict(members=[leader,members[1]]))
    assert run('read')['state']==1 and run('read')['expires_at']==now+7200
    run('start',dict(members=[leader,members[1]],now=now+60))
    assert run('read')['expires_at']==now+7200 and run('read')['started_at']==now
    # Substitutions are permitted only before the first objective seals the roster.
    run('leave',members[1]);run('join',members[1],'conflict')
    run('join',members[2]);run('seal',dict(members=[leader,members[2]]))
    run('seal',dict(members=[leader,members[2]]))
    run('join',members[3],'conflict');run('leave',members[2],'conflict')
    # A sealed roster is also protected at the SQL boundary.
    rejected=subprocess.run(client,input=f"INSERT INTO clan_expedition_roster VALUES('{base['outing']}',{members[3]['player']},{members[3]['account']},{now},0)",text=True,capture_output=True)
    assert rejected.returncode!=0 and 'sealed expedition roster' in rejected.stderr
    assert run('read')['roster_locked']==1
    # A clan change and immediate return cannot revive the original enrollment.
    sql(f"UPDATE clan_memberships SET clan_uid='{B}',rank_id=1 WHERE player_id={members[2]['player']}")
    sql(f"UPDATE clan_memberships SET clan_uid='{A}',rank_id=2 WHERE player_id={members[2]['player']}")
    run('seal',dict(members=[leader,members[2]]),'ineligible');run('join',members[2],'conflict')
    run('abandon');run('read',expected='not-found')
    assert sql('SELECT COUNT(*) FROM clan_expedition_outings')=='1'
    # New outing has a new immutable identity and no progress transfer.
    base['outing']=uid();run('create')
    for m in members[1:6]:run('join',m)
    run('join',members[6],'capacity')
    run('start',dict(members=members[:6]));assert len(run('read')['roster'])==6
    # Departure/rejoin through DELETE also invalidates; rank/points changes do not.
    sql(f"UPDATE clan_memberships SET legacy_points=123 WHERE player_id={members[1]['player']}")
    assert all(not x['inactive'] for x in run('read')['roster'])
    sql(f"DELETE FROM clan_memberships WHERE player_id={members[1]['player']};INSERT INTO clan_memberships VALUES({members[1]['player']},'{A}',2,0)")
    assert next(x for x in run('read')['roster'] if x['player']==members[1]['player'])['inactive']==1
    run('seal',dict(members=members[:6]),'ineligible')
    expiry=run('read')['expires_at'];run('join',members[6]|dict(now=expiry),'ineligible')
    base['now']=expiry;base['outing']=uid();run('create')
    assert run('read')['outing_uid']==base['outing']
    assert sql('SELECT COUNT(*) FROM clan_expedition_outings WHERE state=4')=='1'
    # UTC week is a hard earlier deadline; creating never changes source counters.
    run('abandon');base['now']=2200003200+604800-30;base['outing']=uid();run('create')
    assert run('read')['expires_at']==base['now']+30
    run('join',members[1]);run('start',dict(members=[leader,members[1]]))
    assert run('read')['expires_at']==base['now']+30
    assert sql('SELECT COUNT(*) FROM clan_reward_receipts')=='0'
    assert sql('SELECT COUNT(*) FROM clan_account_windows')=='0'
    # Reapplying the extension preserves retained history and membership hooks.
    before=sql('SELECT * FROM clan_expedition_outings;SELECT * FROM clan_expedition_roster')
    sql((ROOT/'sql/20261112_clan_expeditions_v001.sql').read_text())
    assert sql('SELECT * FROM clan_expedition_outings;SELECT * FROM clan_expedition_roster')==before
    # Concurrent leaders cannot allocate two recruiting slots for one clan.
    run('abandon')
    requests=[base|leader|dict(outing=uid()),base|members[1]|dict(outing=uid())]
    processes=[]
    for request in requests:
        p=subprocess.Popen([str(binary),str(socket),'create'],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True)
        p.stdin.write(json.dumps(request));p.stdin.close();processes.append(p)
    outcomes=[]
    for p in processes:
        output=p.stdout.read();error=p.stderr.read();assert p.wait()==0,(output,error)
        outcomes.append(json.loads(output)['result'])
    assert sorted(outcomes)==['conflict','ok'],outcomes
    live=run('read');base['outing']=live['outing_uid']
    winner=next(m for m in (leader,members[1]) if m['player']==live['leader_id'])
    run('abandon',winner)
    # Current command coordinator requires the optional progress/claim extension.
    sql((ROOT/'sql/20261112_clan_expeditions_v002.sql').read_text())
    runtime=tmp/'runtime'
    subprocess.run(['cc','-std=gnu99','-g','-Wall','-Wextra','-Wno-unused-parameter','-Werror=implicit-function-declaration','-ffunction-sections','-fdata-sections','-Wl,--gc-sections','-Dtime=expedition_test_time','-fsanitize=address,undefined','-fno-pie','-no-pie','-Isrc',*[str(ROOT/p) for p in ['tests/clan_expedition_runtime.c','src/clan_expedition_runtime.c','src/clan_expedition_enrollment.c','src/clan_expeditions.c','src/clan_repository.c','src/clan_awards.c','src/clan_progression.c','src/clan_combat.c']],*flags,'-o',str(runtime)],cwd=ROOT,check=True)
    subprocess.run([str(runtime),str(socket)],cwd=ROOT/'lib',check=True)
    sql('DROP TRIGGER ce_membership_delete');run('read',expected='schema-mismatch')
    print('Phase 34 SQL enrollment: opt-in, two distinct accounts, clan/level/gate eligibility, immutable outing/expiry, pre-objective substitution, sealed roster, membership-change/rejoin invalidation, retained history and schema checks passed.')
