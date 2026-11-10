#!/usr/bin/env python3
"""Phase 11: actual SQL lifecycle, membership access, history and replay rules."""
from clan_database import ROOT, database
from clan_snapshot_fixture import snapshot, manifest, clan, A, B
import subprocess, shlex, json, sys
sys.path.insert(0,str(ROOT/'scripts'))
from build_clan_repository_tool import build
migrations=tuple(f'20261110_clan_progression_v{i:03d}.sql' for i in range(1,6))
with database(migrations) as (tmp,socket,sql,client):
    tool=tmp/'tool';build(tool,sanitize=True)
    def bridge(op,body):
        p=subprocess.run([str(tool),op,'clan_test','root','',str(socket)],input=json.dumps(body),text=True,capture_output=True,check=True)
        return json.loads(p.stdout)
    assert bridge('import',dict(snapshot=snapshot(),expected_revision=0,manifest=manifest()))['result']=='ok'
    sql('UPDATE clan_store_control SET store_mode=2; UPDATE clan_import_manifests SET import_status=2;')
    binary=tmp/'lifecycle';flags=shlex.split(subprocess.check_output(['pkg-config','--cflags','--libs','mariadb','openssl','json-c'],text=True))
    subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-pedantic','-DCLAN_REWARD_TEST','-fsanitize=address,undefined','-fno-pie','-no-pie','-g','-I'+str(ROOT/'src'),str(ROOT/'tests/clan_rewards.c'),str(ROOT/'src/clan_repository.c'),str(ROOT/'src/clan_awards.c'),str(ROOT/'src/clan_progression.c'),*flags,'-o',str(binary)],check=True)
    def run(op,body,expected='ok'):
        p=subprocess.run([str(binary),str(socket),op],input=json.dumps(body),text=True,capture_output=True)
        assert p.returncode==0,(p.stdout,p.stderr)
        out=json.loads(p.stdout);assert out['result']==expected,(op,out,p.stderr)
        return out['receipt']
    seq=0
    def event(**changes):
        global seq
        seq+=1
        return dict(operation_uid=f'{seq:032x}',event_uid=f'{seq+1000:032x}',clan_uid=A,account_id=77,player_id=5000000007,source=1,rules_version=1,occurred_at=2200000001,now=2200000002,requested_xp_eighths=16,requested_influence_eighths=8,requested_favor_eighths=8)|changes
    def access(player,uid):return run('access',dict(player_id=player,clan_uid=uid))
    assert access(5000000007,A)==0 and access(5000000009,B)==1
    run('prepare-member',event(player_id=5000000009,clan_uid=B),'ineligible')
    run('prepare-member',event(player_id=5000000007,clan_uid=B),'ineligible')
    sql('UPDATE clan_memberships SET rank_id=3 WHERE player_id=5000000009')
    run('prepare-member',event(player_id=5000000009,clan_uid=B),'ineligible')
    sql('UPDATE clan_memberships SET rank_id=1 WHERE player_id=5000000009')
    assert access(5000000009,B)==0
    # Two accepted facts before departure: automatic rewards retain occurrence
    # membership, while an uncommitted expedition claim requires current membership.
    kill=event();expedition=event(source=6,requested_xp_eighths=0,requested_influence_eighths=0,requested_favor_eighths=800)
    run('prepare-member',kill);run('prepare-member',expedition)
    sql('DELETE FROM clan_memberships WHERE player_id=5000000007')
    assert access(5000000007,A)==2
    run('prepare-member',event(),'ineligible')
    assert run('prepare-member',kill)['status']==0
    receipt=run('personal',kill)
    assert receipt['decision']['delta']['favor_eighths']==8
    run('personal',expedition,'ineligible')
    assert run('load',expedition)['status']==0
    # Joining a second incarnation does not move either Favor or eligibility.
    sql(f"INSERT INTO clan_memberships VALUES(5000000007,'{B}',1,123)")
    assert access(5000000007,A)==2 and access(5000000007,B)==0
    assert sql(f"SELECT favor_eighths FROM clan_personal_favor WHERE player_id=5000000007 AND clan_uid='{A}'")=='8'
    assert sql(f"SELECT COUNT(*) FROM clan_personal_favor WHERE player_id=5000000007 AND clan_uid='{B}'")=='0'
    run('personal',expedition,'ineligible')
    # Rejoining the same lifetime can use its standing and original pending claim.
    sql(f"UPDATE clan_memberships SET clan_uid='{A}',rank_id=1 WHERE player_id=5000000007")
    assert access(5000000007,A)==0
    expedition_receipt=run('personal',expedition)
    assert expedition_receipt['decision']['after']['favor']==808
    # Retained cooldowns and pending facts survive atomic tombstoning.
    sql(f"INSERT INTO clan_buff_families VALUES('{A}',1,'{'9'*32}',1,1,1,2200000000,2200086400,2200129600,800,0)")
    pending=event();run('prepare-member',pending)
    before=bridge('read',{})
    request=dict(clan_uid=A,revision=before['store_revision'],now=2200000010)
    run('disband',request|dict(revision=before['store_revision']-1),'conflict')
    sql("CREATE TRIGGER fail_lifecycle BEFORE DELETE ON clan_memberships FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='injected lifecycle failure'")
    run('disband',request,'invalid');assert bridge('read',{})==before
    sql('DROP TRIGGER fail_lifecycle')
    run('disband',request)
    assert access(5000000007,A)==3 and access(5000000009,B)==0
    assert sql(f"SELECT record_status FROM clan_progression_clans WHERE clan_uid='{A}'")=='2'
    assert sql(f"SELECT favor_eighths FROM clan_personal_favor WHERE player_id=5000000007 AND clan_uid='{A}'")=='808'
    assert sql(f"SELECT next_available_at FROM clan_buff_families WHERE clan_uid='{A}'")=='2200129600'
    assert run('load',pending)['status']==0
    run('personal',pending,'deleted')
    assert run('personal',kill,expected='ok')==receipt
    assert run('personal',expedition)==expedition_receipt
    run('prepare-member',event(),'deleted')
    run('disband',request,'deleted')
    after=bridge('read',{})
    assert len(after['clans'])==1 and all(m['clan_uid']!=A for m in after['members'])
    assert after['claims'][0]['controller_uid'] is None and after['claims'][0]['claimant_id']==0
    assert all(p['clan_uid']!=A for claim in after['claims'] for p in claim['popularity'])
    assert not after['clans'][0]['legacy']['relationships']
    # Reuse numeric vnum 7 with a new lifetime and zero new economy/history.
    C='c'*32;fresh=clan(C,7);fresh['legacy']['relationships']=[]
    fresh['core']['created_at']=fresh['core']['updated_at']=fresh['core']['state_updated_at']=2200000020
    after['clans'].append(fresh);after['members'].append(dict(player_id=5000000007,clan_uid=C,rank_id=1,legacy_points=0))
    after['written_at']=2200000020
    assert bridge('save',dict(snapshot=after,expected_revision=after['store_revision']))['result']=='ok'
    assert access(5000000007,C)==0 and access(5000000007,A)==3
    assert sql(f"SELECT xp_eighths,influence_eighths FROM clan_progression_state WHERE clan_uid='{C}'")=='0\t0'
    assert sql(f"SELECT COUNT(*) FROM clan_personal_favor WHERE clan_uid='{C}'")=='0'
    assert sql(f"SELECT COUNT(*) FROM clan_buff_families WHERE clan_uid='{C}'")=='0'
    run('personal',pending,'deleted')
    print('Phase 11: accepted/applicant/former/deleted access; departure/switch/rejoin; retained Favor/cooldowns/receipts; current expedition eligibility; transactional disband rollback; vnum reuse isolation passed.')
