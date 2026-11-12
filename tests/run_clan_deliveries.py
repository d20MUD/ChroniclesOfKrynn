#!/usr/bin/env python3
"""Phases 28–29, strict definitions and transactional delivery ledger in private SQL."""
from clan_database import ROOT, database
from clan_snapshot_fixture import snapshot, manifest, A, B
import subprocess, shlex, json, sys, copy
sys.path.insert(0,str(ROOT/'scripts'))
from build_clan_repository_tool import build
migrations=tuple(f'20261110_clan_progression_v{i:03d}.sql' for i in range(1,6))+('20261111_clan_delivery_v001.sql',)
with database(migrations) as (tmp,socket,sql,client):
    tool=tmp/'tool';build(tool,sanitize=True)
    subprocess.run([str(tool),'import','clan_test','root','',str(socket)],input=json.dumps(dict(snapshot=snapshot(),expected_revision=0,manifest=manifest())),text=True,check=True,stdout=subprocess.DEVNULL)
    sql('UPDATE clan_store_control SET store_mode=2; UPDATE clan_import_manifests SET import_status=2;')
    binary=tmp/'delivery'
    flags=shlex.split(subprocess.check_output(['pkg-config','--cflags','--libs','mariadb','openssl','json-c'],text=True))
    subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-pedantic','-g','-DCLAN_REWARD_TEST','-fsanitize=address,undefined','-fno-pie','-no-pie','-I'+str(ROOT/'src'),*[str(ROOT/p) for p in ['tests/clan_delivery.c','src/clan_delivery.c','src/clan_repository.c','src/clan_awards.c','src/clan_progression.c']],*flags,'-o',str(binary)],check=True)
    def run(op,body,expected='ok',crash=False):
        p=subprocess.run([str(binary),str(socket),op],input=json.dumps(body),text=True,capture_output=True)
        if crash:
            assert p.returncode!=0,(op,p.stdout);return
        assert p.returncode==0,(p.returncode,p.stdout,p.stderr)
        result=json.loads(p.stdout);assert result['result']==expected,(op,result,p.stderr)
        return result['data']
    cat=json.loads((ROOT/'lib/clan/deliveries.json').read_text());run('catalog',cat)
    for change in [dict(quantity=0),dict(quantity=500000),dict(kind=3),dict(resource=999),dict(grade=99),dict(id="x' OR 1=1"),dict(min_level=21),dict(quantity='50')]:
        bad=copy.deepcopy(cat);bad['deliveries'][0].update(change);run('catalog',bad,'invalid')
    bad=copy.deepcopy(cat);bad['deliveries'].append(bad['deliveries'][0]);run('catalog',bad,'invalid')
    who=dict(player_id=5000000007,account_id=77,clan_uid=A,now=2200000001)
    seq=1000
    def accept(definition=0,identity=None,expected='ok'):
        global seq;seq+=1
        e=(identity or who)|dict(accepted_uid=f'{seq:032x}',definition=cat['deliveries'][definition]);run('accept',e,expected);return e
    def snapshot_balances():return sql('SELECT * FROM clan_progression_state;SELECT * FROM clan_account_windows;SELECT * FROM clan_personal_favor;SELECT * FROM clan_weekly_windows;')
    # Applicant and locked clan-level definitions cannot be accepted.
    accept(identity=who|dict(player_id=5000000009,clan_uid=B),expected='ineligible')
    accept(definition=4,expected='ineligible')
    first=accept();accept(expected='conflict')
    active=run('active',who);assert active['definition']==cat['deliveries'][0]
    quote=run('quote',who);assert quote['delta']['xp_eighths']==800
    # Expired confirmation and tampered frozen requirements leave no receipt.
    run('prepare',dict(quote=quote,now=quote['confirm_before']),'ineligible')
    tampered=copy.deepcopy(quote);tampered['definition']['resource']=22
    run('prepare',dict(quote=tampered,now=who['now']),'ineligible')
    assert sql('SELECT COUNT(*) FROM clan_reward_receipts')=='0'
    # Prepare and participant insert roll back together; ambiguous commit replays.
    run('prepare',dict(quote=quote,now=who['now'],fault=1),'commit-unknown')
    run('prepare',dict(quote=quote,now=who['now']))
    run('abandon',who,'conflict')
    run('generic',dict(operation_uid=quote['operation_uid'],now=who['now']),'ineligible')
    before=snapshot_balances()
    for fault in range(2,8):
        run('complete',dict(quote=quote,now=who['now'],fault=fault),'unavailable')
        assert snapshot_balances()==before
        assert sql('SELECT COUNT(*) FROM clan_delivery_claims')=='0'
        assert sql('SELECT state FROM clan_delivery_operations')=='0'
    run('complete',dict(quote=quote,now=who['now'],fault=8),'commit-unknown')
    receipt=run('complete',dict(quote=quote,now=who['now']))
    assert receipt['status']==1 and receipt['decision']['delta']['favor_eighths']==400
    assert run('complete',dict(quote=quote,now=who['now']+1))==receipt
    run('ack',dict(operation_uid=quote['operation_uid']))
    assert sql('SELECT state FROM clan_delivery_operations')=='2'
    accept(expected='ineligible') # Same definition cannot be farmed.
    second=accept(1);q2=run('quote',who);run('prepare',dict(quote=q2,now=who['now']))
    # A changed cap cannot silently replace the exact confirmed payout.
    sql('UPDATE clan_account_windows SET favor_eighths=4799,revision=revision+1 WHERE account_id=77')
    run('complete',dict(quote=q2,now=who['now']),'ineligible')
    run('cancel',dict(operation_uid=q2['operation_uid'],now=who['now']))
    run('cancel',dict(operation_uid=q2['operation_uid'],now=who['now']))
    assert sql('SELECT COUNT(*) FROM clan_delivery_claims')=='1'
    accept(1);q3=run('quote',who);assert q3['delta']['favor_eighths']==1
    run('prepare',dict(quote=q3,now=who['now']));run('complete',dict(quote=q3,now=who['now']));run('ack',dict(operation_uid=q3['operation_uid']))
    accept(2,expected='ineligible') # Shared two-definition weekly limit.
    # New week: expired active contracts are closed, independent week counters.
    who['now']+=604800
    accept(2);run('abandon',who);accept(2)
    q4=run('quote',who);run('prepare',dict(quote=q4,now=who['now']))
    # Membership can change after durable economic acceptance; beneficiary stays frozen.
    sql(f"UPDATE clan_memberships SET clan_uid='{B}',rank_id=1 WHERE player_id=5000000007")
    run('complete',dict(quote=q4,now=who['now']));run('ack',dict(operation_uid=q4['operation_uid']))
    assert sql(f"SELECT clan_uid FROM clan_reward_receipts WHERE operation_uid='{q4['operation_uid']}'")==A
    who['clan_uid']=B
    accept(2,expected='ineligible') # Completed definition follows account into new clan.
    accept(1)
    sql('UPDATE clan_account_windows SET base_xp_eighths=9600,favor_eighths=4800,revision=revision+1 WHERE account_id=77')
    run('quote',who,'capacity') # No all-zero donations.
    run('abandon',who)
    # Exact-boundary reset is idempotent, keeps prepared goods recovery alive.
    resetter=who|dict(account_id=123,now=who['now']+604800)
    partial=accept(0,identity=resetter)
    expires=run('active',resetter)['expires_at']
    run('expire',resetter|dict(now=expires-1));assert run('active',resetter)
    run('expire',resetter|dict(now=expires));run('expire',resetter|dict(now=expires))
    run('active',resetter,'not-found')
    resetter['now']=expires
    accept(0,identity=resetter);held=run('quote',resetter)
    run('prepare',dict(quote=held,now=expires))
    later=run('active',resetter)['expires_at']
    run('expire',resetter|dict(now=later))
    assert run('active',resetter)['state']==0
    recovered=run('complete',dict(quote=held,now=later))
    assert recovered['event']['occurred_at']==expires
    assert run('complete',dict(quote=held,now=later+604800))==recovered
    run('ack',dict(operation_uid=held['operation_uid']))
    sql('DROP TRIGGER cp_delivery_claim_immutable')
    run('active',who,'schema-mismatch')
    print('Phases 28–29 SQL: bounded definitions, accepted membership/level, frozen terms, exact previews, atomic receipt/claim, cap changes, cancel/replay, weekly limits and original-clan recovery passed.')
