#!/usr/bin/env python3
"""Phases 37-38 outing reconciliation and restart/expiry correctness."""
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
    now=2200610081;seq=60000
    def uid():
        global seq;seq+=1;return f'{seq:032x}'
    leader=dict(player=5000000007,account=77,level=27);ally=dict(player=5000000010,account=80,level=27)
    sql(f"INSERT INTO clan_memberships VALUES({ally['player']},'{A}',2,0);UPDATE clan_memberships SET rank_id=1 WHERE clan_uid='{B}'")
    base=dict(clan=A,now=now,catalog=catalog,definition='icewall-assault-v1',outing=uid())
    def run(op,body=None,expected='ok',crash=None):
        p=subprocess.run([str(binary),str(socket),op],input=json.dumps(base|leader|(body or {})),text=True,capture_output=True)
        if crash is not None:assert p.returncode==crash,(op,p.returncode,p.stdout,p.stderr);return
        assert p.returncode==0,(op,p.stdout,p.stderr)
        result=json.loads(p.stdout);assert result['result']==expected,(op,result,p.stderr);return result['data']
    def create():
        base['outing']=uid();run('create');run('join',ally);run('start',dict(members=[leader,ally]));return base['outing']
    def event(target=14100):
        base['now']+=1
        return dict(event=uid(),members=[leader,ally],target=target,area=141,now=base['now'])
    first=create()
    for _ in range(5):run('combat',event())
    before=run('read');run('reconcile');assert run('read')==before and run('progress')==[5,0,0]
    runtime=tmp/'runtime'
    subprocess.run(['cc','-std=gnu99','-g','-Wall','-Wextra','-Wno-unused-parameter','-ffunction-sections','-fdata-sections','-Wl,--gc-sections','-Dtime=expedition_test_time','-fsanitize=address,undefined','-fno-pie','-no-pie','-Isrc',*[str(ROOT/p) for p in ['tests/clan_expedition_runtime.c','src/clan_expedition_runtime.c','src/clan_expedition_enrollment.c','src/clan_expeditions.c','src/clan_repository.c','src/clan_awards.c','src/clan_progression.c','src/clan_combat.c']],*flags,'-o',str(runtime)],cwd=ROOT,check=True)
    for _ in range(2):subprocess.run([str(runtime),str(socket),'resume',first],cwd=ROOT/'lib',check=True)
    assert run('read')==before
    # Independent per-outing commits and cursor pages cannot skip another clan.
    other=base|dict(clan=B,outing=uid(),player=5000000009,account=90)
    run('create',other)
    page=run('batch',dict(limit=1));assert len(page)==1 and page[0]['result']=='ok'
    page2=run('batch',dict(limit=1,cursor=page[0]['outing_uid']));assert len(page2)==1
    assert run('batch',dict(limit=1,cursor=page2[0]['outing_uid']))==[]
    run('batch',dict(limit=33),'invalid')
    expiry=before['expires_at'];run('reconcile',dict(now=expiry-1));assert run('read')['state']==1
    run('reconcile',dict(now=expiry,expedition_crash=3),crash=95);assert run('read')['state']==1
    run('reconcile',dict(now=expiry,expedition_crash=4),crash=96);run('read',expected='not-found')
    assert run('progress')==[5,0,0]
    run('reconcile',dict(now=expiry));run('claim',dict(now=expiry),'not-found')
    # Startup upkeep closes expired slots even with default-off earnings flags.
    base['now']=expiry+1;second=create();deadline=run('read')['expires_at']
    subprocess.run([str(runtime),str(socket),'restore',second,str(deadline)],cwd=ROOT/'lib',check=True)
    run('read',expected='not-found')
    # Abandonment retains source history but cannot create an entitlement.
    base['now']=deadline+1;abandoned=create();run('combat',event());run('abandon');run('reconcile')
    assert run('progress')==[1,0,0];run('claim',expected='not-found')
    # Retained corruption stops reconciliation and new progress without rewriting facts.
    bad=create();run('combat',event());e=event();run('combat',e)
    sql('DROP TRIGGER ce_event_immutable')
    sql(f"UPDATE clan_expedition_events SET target=999999 WHERE outing_uid='{bad}' AND event_uid='{e['event']}'")
    sql((ROOT/'sql/20261112_clan_expeditions_v002.sql').read_text())
    facts=sql(f"SELECT * FROM clan_expedition_events WHERE outing_uid='{bad}'")
    run('reconcile',expected='conflict');run('combat',event(),'conflict')
    assert sql(f"SELECT * FROM clan_expedition_events WHERE outing_uid='{bad}'")==facts
    batch=run('batch',dict(limit=32));assert any(v['outing_uid']==bad and v['result']=='conflict' for v in batch)
    sql('DROP TRIGGER ce_event_immutable')
    sql(f"UPDATE clan_expedition_events SET target=14100 WHERE outing_uid='{bad}' AND event_uid='{e['event']}'")
    sql((ROOT/'sql/20261112_clan_expeditions_v002.sql').read_text());run('reconcile');run('abandon')
    # Recover current membership from SQL; offline presence alone never removes a roster member.
    fresh=create();run('combat',event());before=run('read');run('reconcile');assert run('read')==before
    sql('DROP TRIGGER ce_membership_update')
    sql(f"UPDATE clan_memberships SET clan_uid='{B}',rank_id=1 WHERE player_id={ally['player']}")
    sql((ROOT/'sql/20261112_clan_expeditions_v002.sql').read_text());run('reconcile')
    assert next(m for m in run('read')['roster'] if m['player']==ally['player'])['inactive']==1
    sql(f"UPDATE clan_memberships SET clan_uid='{A}',rank_id=2 WHERE player_id={ally['player']}")
    run('join',ally,'conflict');run('abandon')
    # A prepared receipt remains authoritative through a later stale/partial player snapshot.
    completed=create()
    for _ in range(20):run('combat',event())
    run('combat',event(14001));run('combat',event(14000))
    prepared=run('claim');operation=prepared['event']['operation_uid']
    retained=sql('SELECT * FROM clan_reward_receipts;SELECT * FROM clan_expedition_personal_claims')
    run('reconcile');run('batch',dict(limit=32))
    assert sql('SELECT * FROM clan_reward_receipts;SELECT * FROM clan_expedition_personal_claims')==retained
    # A partial SQL entitlement/receipt disagreement is detected, never reminted.
    sql(f"UPDATE clan_expedition_entitlements SET state=1 WHERE outing_uid='{completed}' AND player_id={ally['player']}")
    run('claim',ally,'conflict')
    assert sql('SELECT * FROM clan_reward_receipts;SELECT * FROM clan_expedition_personal_claims')==retained
    sql('DROP TRIGGER ce_entitlement_terms');sql(f"UPDATE clan_expedition_entitlements SET state=0 WHERE outing_uid='{completed}' AND player_id={ally['player']}")
    sql((ROOT/'sql/20261112_clan_expeditions_v002.sql').read_text())
    sql(f"DELETE FROM clan_memberships WHERE player_id={leader['player']}")
    reset=2201212800
    paid=run('complete',dict(operation=operation,now=reset));assert paid['decision']['delta']['favor_eighths']==200
    assert run('complete',dict(operation=operation,now=reset+1))==paid
    run('claim',ally|dict(now=reset),'ineligible')
    assert run('progress')==[20,1,1]
    print('Phase 37: fresh-process resume, stable IDs/counters/deadlines, paged upkeep, expiry crashes, feature-off maintenance, abandon/no-claim, corruption/partial-proof refusal, membership reconciliation and retained reward recovery passed.')
