#!/usr/bin/env python3
"""Actual validation/SQL commit service; only disposable MariaDB fixtures."""
from clan_database import ROOT,database
from clan_snapshot_fixture import snapshot,manifest,A,B
import subprocess,shlex,json,sys,copy
sys.path.insert(0,str(ROOT/'scripts'))
from build_clan_repository_tool import build
migrations=tuple(f'20261110_clan_progression_v{i:03d}.sql' for i in range(1,6))+('20261111_clan_buff_activations_v001.sql',)
with database(migrations) as (tmp,socket,sql,client):
    # Idempotent extension; it changes neither core version nor enablement.
    sql((ROOT/'sql'/migrations[-1]).read_text())
    tool=tmp/'tool';build(tool,sanitize=True)
    p=subprocess.run([str(tool),'import','clan_test','root','',str(socket)],input=json.dumps(dict(snapshot=snapshot(),expected_revision=0,manifest=manifest())),text=True,capture_output=True,check=True)
    assert json.loads(p.stdout)['result']=='ok',p.stdout
    sql('UPDATE clan_store_control SET store_mode=2; UPDATE clan_import_manifests SET import_status=2;')
    sql(f"UPDATE clan_progression_state SET influence_eighths=40000 WHERE clan_uid='{A}'")
    flags=shlex.split(subprocess.check_output(['pkg-config','--cflags','--libs','mariadb','openssl','json-c'],text=True))
    binary=tmp/'activation'
    subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-pedantic','-g','-fsanitize=address,undefined','-fno-pie','-no-pie','-DCLAN_BUFF_ACTIVATION_TEST','-I'+str(ROOT/'src'),str(ROOT/'tests/clan_buff_activation.c'),*[str(ROOT/'src'/f) for f in ['clan_repository.c','clan_progression.c','clan_awards.c','clan_buff_queries.c','clan_buff_activation.c','clan_buff_commit.c']],*flags,'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
    base=dict(operation_uid='1'*32,clan_uid=A,expected_activation_uid='',actor_id=5000000007,family=1,tier=1,slot=1,now=2200000000)
    def proc(request):return subprocess.run([str(binary),str(socket)],input=json.dumps(request),text=True,capture_output=True)
    def run(changes={},expected='ok'):
        request=base|changes;p=proc(request);assert p.returncode==0,(p.stdout,p.stderr)
        result=json.loads(p.stdout);assert result['result']==expected,(request,result,p.stderr);return result
    def state():return '\n'.join(sql(f'SELECT * FROM {t} ORDER BY 1,2') for t in ['clan_progression_state','clan_buff_families','clan_buff_activations','clan_store_control'])
    before=state()
    for changes,reason in [({'disabled':True},2),({'actor_id':999},8),({'tier':3},32),({'slot':3},64)]:
        assert run(changes,'ineligible')['reasons']&reason
        assert state()==before
    run({'nested':True},'invalid');assert state()==before
    # Every precommit failure rolls back; an identical request can then succeed.
    for fault in range(1,6):
        run({'fault':fault},'unavailable');assert state()==before
    for crash in range(1,6):
        p=proc(base|{'crash':crash});assert p.returncode==100+crash,(crash,p.stderr)
        assert state()==before
    first=run()['receipt'];after=state()
    assert first['influence_before']==40000 and first['influence_after']==39200
    assert first['expires_at']==2200086400 and first['next_available_at']==2200129600
    assert run({'now':2200000100})['receipt']==first and state()==after
    assert run({'disabled':True})['receipt']==first and state()==after
    run({'tier':2},'conflict');run({'actor_id':999},'conflict');run({'clan_uid':B},'conflict');assert state()==after
    assert run({'operation_uid':'2'*32,'slot':2},'ineligible')['reasons']&128
    assert run({'operation_uid':'2'*32,'now':2200086400,'slot':2},'ineligible')['reasons']&256
    # A failed replacement preserves old activity, cooldown, balances and log.
    replacement=dict(operation_uid='2'*32,family=2,expected_activation_uid='1'*32,now=2200000010)
    assert run(replacement|{'expected_activation_uid':'3'*32},'ineligible')['reasons']&1024
    for fault in range(1,6):run(replacement|{'fault':fault},'unavailable');assert state()==after
    second=run(replacement)['receipt']
    saved=state();assert run()['receipt']==first and state()==saved
    assert second['replaced_family']==1 and second['replaced_activation_uid']=='1'*32
    assert sql(f"SELECT canceled,next_available_at FROM clan_buff_families WHERE clan_uid='{A}' AND family_id=1")=='1\t2200129600'
    assert run({'operation_uid':'3'*32,'slot':2,'now':2200000020},'ineligible')['reasons']&256
    # New officer identity and higher tier cannot bypass a family's lockout.
    sql(f"INSERT INTO clan_memberships VALUES(88,'{A}',2,0)")
    assert run({'operation_uid':'3'*32,'actor_id':88,'family':3,'slot':2,'now':2200000020},'ineligible')['reasons']&16
    sql(f"INSERT INTO clan_buff_management VALUES('{A}',21,2)")
    third=run({'operation_uid':'3'*32,'actor_id':88,'family':3,'slot':2,'now':2200000020,'fault':6},'commit-unknown')
    third=run({'operation_uid':'3'*32,'actor_id':88,'family':3,'slot':2,'now':2200000021})['receipt']
    assert third['influence_after']==37600
    # Replay remains readable after departure; a new operation is rejected.
    sql('DELETE FROM clan_memberships WHERE player_id=88')
    assert run({'operation_uid':'3'*32,'actor_id':88,'family':3,'slot':2})['receipt']==third
    # Expired family row can be reused at the exact saved cooldown boundary.
    resumed=run({'operation_uid':'4'*32,'now':2200129600})['receipt'];assert resumed['influence_after']==36800
    # Simultaneous retries and competing replacement requests serialize globally.
    requests=[base|dict(operation_uid='5'*32,family=2,slot=2,now=2200259300) for _ in range(4)]
    children=[subprocess.Popen([str(binary),str(socket)],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True) for _ in requests]
    for child,request in zip(children,requests):child.stdin.write(json.dumps(request));child.stdin.close();child.stdin=None
    outputs=[]
    for child in children:
        stdout,stderr=child.communicate();assert child.returncode==0,stderr;outputs.append(json.loads(stdout))
    assert all(o['result']=='ok' for o in outputs),outputs
    assert all(o['receipt']==outputs[0]['receipt'] for o in outputs)
    assert sql('SELECT COUNT(*) FROM clan_buff_activations')=='5'
    # A killed client after COMMIT recovers by operation ID without another debit.
    request=base|dict(operation_uid='6'*32,family=3,slot=1,now=2200388900,crash=6)
    p=proc(request);assert p.returncode==106,p.stderr
    recovered=run({k:v for k,v in request.items() if k!='crash'})['receipt']
    after=state();assert run({k:v for k,v in request.items() if k!='crash'})['receipt']==recovered and state()==after
    # Distinct simultaneous requests for the same slot: one wins, one changes nothing.
    requests=[base|dict(operation_uid=str(i)*32,family=family,slot=1,now=2200518500) for i,family in [(7,3),(8,4)]]
    balance=int(sql(f"SELECT influence_eighths FROM clan_progression_state WHERE clan_uid='{A}'"))
    children=[subprocess.Popen([str(binary),str(socket)],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True) for _ in requests]
    for child,request in zip(children,requests):child.stdin.write(json.dumps(request));child.stdin.close();child.stdin=None
    outputs=[]
    for child in children:
        stdout,stderr=child.communicate();assert child.returncode==0,stderr;outputs.append(json.loads(stdout))
    assert sorted(o['result'] for o in outputs)==['ineligible','ok'],outputs
    assert int(sql(f"SELECT influence_eighths FROM clan_progression_state WHERE clan_uid='{A}'"))==balance-800
    after=state()
    # Structurally valid JSON with an invalid receipt proof never becomes a retry.
    sql(f"INSERT INTO clan_buff_activations VALUES('{'9'*32}','{A}',5000000007,1,1,1,1,'','{{}}')")
    run({'operation_uid':'9'*32},'schema-mismatch');after=state()
    # Database-level immutable history and missing-contract rejection.
    for statement in ['UPDATE clan_buff_activations SET tier=2','DELETE FROM clan_buff_activations']:
        p=subprocess.run(client,input=statement,text=True,capture_output=True);assert p.returncode
    sql('DROP TRIGGER cba_retained')
    run({'operation_uid':'7'*32},'schema-mismatch');assert state()==after
    print('Phases 14–15: pure validation, current SQL permission, atomic debit/replacement/timers/log, rollback/crash boundaries, retries/concurrency, retained cooldowns and schema failure passed.')
