#!/usr/bin/env python3
"""Phases 30-31 durable gathering/bounty objectives, dedup and atomic awards."""
from clan_database import ROOT,database
from clan_snapshot_fixture import snapshot,manifest,A,B
import subprocess,json,shlex,sys,copy
sys.path.insert(0,str(ROOT/'scripts'))
from build_clan_repository_tool import build
migrations=tuple(f'20261110_clan_progression_v{i:03d}.sql' for i in range(1,6))+('20261111_clan_delivery_v001.sql','20261111_clan_objectives_v001.sql')
with database(migrations) as (tmp,socket,sql,client):
    tool=tmp/'tool';build(tool,sanitize=True)
    subprocess.run([str(tool),'import','clan_test','root','',str(socket)],input=json.dumps(dict(snapshot=snapshot(),expected_revision=0,manifest=manifest())),text=True,check=True,stdout=subprocess.DEVNULL)
    sql('UPDATE clan_store_control SET store_mode=2;UPDATE clan_import_manifests SET import_status=2;')
    binary=tmp/'objectives';flags=shlex.split(subprocess.check_output(['pkg-config','--cflags','--libs','mariadb','openssl','json-c'],text=True))
    subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-pedantic','-g','-DCLAN_REWARD_TEST','-fsanitize=address,undefined','-fno-pie','-no-pie','-Isrc',*[str(ROOT/p) for p in ['tests/clan_objectives.c','src/clan_objectives.c','src/clan_repository.c','src/clan_awards.c','src/clan_progression.c']],*flags,'-o',str(binary)],cwd=ROOT,check=True)
    def run(op,body,expected='ok',crash=None):
        p=subprocess.run([str(binary),str(socket),op],input=json.dumps(body),text=True,capture_output=True)
        if crash is not None:
            assert p.returncode==crash,(op,p.returncode,p.stdout,p.stderr);return
        assert p.returncode==0,(p.returncode,p.stdout,p.stderr)
        result=json.loads(p.stdout);assert result['result']==expected,(op,result,p.stderr);return result['data']
    catalog=json.loads((ROOT/'lib/clan/objectives.json').read_text());run('catalog',catalog)
    for changes in [dict(required=0),dict(required=200),dict(family=3),dict(id="bad'id"),dict(max_player_level=31),dict(grade=0),dict(target=-1),dict(required='20')]:
        bad=copy.deepcopy(catalog);bad['objectives'][0].update(changes);run('catalog',bad,'invalid')
    bad=copy.deepcopy(catalog);bad['objectives'].append(bad['objectives'][0]);run('catalog',bad,'invalid')
    who=dict(player_id=5000000007,account_id=77,clan_uid=A,level=7,now=2200000001)
    seq=20000
    def uid():
        global seq;seq+=1;return f'{seq:032x}'
    def accept(index,identity=None,expected='ok'):
        req=(identity or who)|dict(definition=catalog['objectives'][index],contract_uid=uid());run('accept',req,expected);return req
    def event(contract,**kw):
        d=contract['definition'];return {k:contract[k] for k in ('clan_uid','player_id','account_id','contract_uid','level','now')}|dict(event_uid=uid(),occurred_at=contract['now'],room=4300,family=d['family'],target=d['target'],grade=d['grade'],base_units=1)|kw
    def finish(contract):return run('complete',contract)
    # Applicants, wrong character bands and locked clan tiers cannot accept.
    accept(0,who|dict(player_id=5000000009,clan_uid=B),expected='ineligible')
    accept(3,who|dict(level=30),expected='ineligible');accept(2,expected='ineligible')
    gathering=accept(0);bounty=accept(3);accept(1,expected='conflict');accept(4,expected='conflict')
    e=event(gathering,base_units=4);assert run('capture',e)==gathering['contract_uid']
    run('capture',e|dict(target=26),'ineligible');run('progress',e|dict(target=26),'ineligible')
    run('progress',e|dict(grade=2),'ineligible');run('progress',e|dict(player_id=5000000012),'ineligible')
    run('progress',e|dict(occurred_at=who['now']-1),'ineligible')
    # Before COMMIT the event and progress both roll back; after COMMIT replay dedups.
    run('progress',e|dict(objective_crash=1),crash=91)
    assert run('active',who|dict(family=4))['progress']==0
    run('progress',e|dict(objective_crash=2),crash=92)
    run('progress',e);assert run('active',who|dict(family=4))['progress']==4
    run('progress',e|dict(base_units=3),'conflict')
    run('progress',e|dict(clan_uid=B),'conflict')
    # Inventory changes have no API path to progress. Only explicit source events count.
    assert sql('SELECT COUNT(*) FROM clan_objective_events')=='1'
    for _ in range(3):run('progress',event(gathering,base_units=4))
    final=event(gathering,base_units=4);run('progress',final)
    assert run('active',who|dict(family=4))['progress']==20
    run('generic',gathering,'ineligible')
    before=sql('SELECT * FROM clan_progression_state;SELECT * FROM clan_account_windows;SELECT * FROM clan_personal_favor;')
    for point in range(2,8):
        run('complete',gathering|dict(fault=point),'unavailable');assert sql('SELECT COUNT(*) FROM clan_objective_claims')=='0'
        assert sql('SELECT * FROM clan_progression_state;SELECT * FROM clan_account_windows;SELECT * FROM clan_personal_favor;')==before
    run('complete',gathering|dict(fault=8),'commit-unknown');receipt=finish(gathering)
    assert receipt['decision']['delta']['xp_eighths']==800 and finish(gathering)==receipt
    run('progress',final);accept(0,expected='ineligible')
    # A bounty requires 20 unique eligible occurrences. The accepted UID remains frozen.
    for i in range(20):
        e=event(bounty);run('capture',e);run('progress',e);run('progress',e)
        assert run('active',who|dict(family=5))['progress']==i+1
    paid=finish(bounty);assert paid['decision']['delta']['favor_eighths']==400
    assert sql('SELECT COUNT(*) FROM clan_bounty_shared_claims')=='1'
    shared=sql('SELECT operation_uid FROM clan_bounty_shared_claims');sr=run('shared',dict(contract_uid=shared,now=who['now']))
    assert sr['decision']['delta']['xp_eighths']==1600
    assert run('shared',dict(contract_uid=shared,now=who['now']))==sr
    # A different account/member finishing this definition receives personal credit,
    # while the shared clan receipt remains unique.
    sql(f"INSERT INTO clan_memberships(player_id,clan_uid,rank_id,legacy_points) VALUES(5000000012,'{A}',2,0)")
    other=who|dict(player_id=5000000012,account_id=88)
    member=accept(3,other)
    for _ in range(20):run('progress',event(member))
    assert finish(member)['decision']['delta']['favor_eighths']==400
    assert sql('SELECT COUNT(*) FROM clan_bounty_shared_claims')=='1'
    # Cross-alt encounter IDs do not count twice for the same account.
    second=accept(4);alt=accept(4,other|dict(account_id=77))
    occurrence=event(second);run('progress',occurrence)
    run('progress',event(alt,event_uid=occurrence['event_uid']),'conflict')
    for _ in range(19):run('progress',event(second))
    finish(second);assert sql('SELECT COUNT(*) FROM clan_bounty_shared_claims')=='2'
    assert len(run('claims',who|dict(family=5)))==2
    accept(3,expected='ineligible')
    # Abandonment cannot reset already claimed definitions; unearned effort is lost.
    run('abandon',other|dict(family=5));run('active',other|dict(family=5),'not-found')
    # A third distinct clan bounty does not create a third shared reward slot.
    third_shared=accept(5,other|dict(level=17))
    for _ in range(20):run('progress',event(third_shared))
    assert finish(third_shared)['decision']['delta']['favor_eighths']==400
    assert sql('SELECT COUNT(*) FROM clan_bounty_shared_claims')=='2'
    # Weekly expiry rejects late events, but a saved in-week event can replay later.
    hemp=accept(1);expired=event(hemp,occurred_at=who['now']+604800,now=who['now']+604800)
    run('progress',expired,'ineligible')
    for _ in range(5):run('progress',event(hemp,base_units=4,now=who['now']+604800))
    finish(hemp|dict(now=who['now']+604800))
    assert len(run('claims',who|dict(family=4)))==2
    # A completion after switching clans is paid to its original incarnation.
    who['now']+=604800;fresh=accept(0)
    for _ in range(5):run('progress',event(fresh,base_units=4))
    sql(f"UPDATE clan_memberships SET clan_uid='{B}',rank_id=1 WHERE player_id=5000000007")
    assert finish(fresh)['event']['clan_uid']==A
    # Concurrent alts in different clans compete for one definition allowance.
    ra=accept(4,who|dict(account_id=99,clan_uid=B))
    rb=accept(4,who|dict(account_id=99,player_id=5000000012,clan_uid=A))
    for contract in (ra,rb):
        for _ in range(20):run('progress',event(contract))
    children=[]
    for contract in (ra,rb):
        p=subprocess.Popen([str(binary),str(socket),'complete'],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True)
        p.stdin.write(json.dumps(contract));p.stdin.close();children.append(p)
    for p in children:
        out=p.stdout.read();err=p.stderr.read();assert p.wait()==0,(out,err)
        assert json.loads(out)['result'] in ('ok','ineligible'),(out,err)
    results=[finish(ra),finish(rb)]
    assert sum(r['decision']['delta']['favor_eighths'] for r in results)==400
    assert sql('SELECT COUNT(*) FROM clan_objective_claims WHERE account_id=99')=='1'
    # Reset closes only unfinished slots. Pending receipts retain their old period.
    resetter=who|dict(clan_uid=B,account_id=100)
    unfinished=accept(0,resetter)
    pending=accept(3,resetter)
    for _ in range(20):run('progress',event(pending))
    expiry=int(run('active',resetter|dict(family=4))['expires_at'])
    run('expire',resetter|dict(now=expiry-1))
    assert run('active',resetter|dict(family=4))['state']==0
    claims_before=sql('SELECT * FROM clan_objective_claims ORDER BY account_id,week_start,family,definition_id')
    run('expire',resetter|dict(now=expiry));run('expire',resetter|dict(now=expiry))
    run('active',resetter|dict(family=4),'not-found')
    assert run('active',resetter|dict(family=5))['state']==1
    assert sql('SELECT * FROM clan_objective_claims ORDER BY account_id,week_start,family,definition_id')==claims_before
    recovered=finish(pending|dict(now=expiry))
    assert recovered['event']['occurred_at']==pending['now']
    assert finish(pending|dict(now=expiry+604800))==recovered
    assert run('claims',resetter|dict(family=5,now=expiry))==[]
    accept(0,resetter|dict(now=expiry))
    sql('DROP TRIGGER co_event_immutable');run('active',who|dict(family=4),'schema-mismatch')
    print('Phases 30-31 SQL: strict definitions, independent slots, frozen identity, base-unit progress, designated kills, event/account dedup, reconnect/crash recovery, atomic rewards, shared bounty dedup, weekly limits and old-clan receipts passed.')
