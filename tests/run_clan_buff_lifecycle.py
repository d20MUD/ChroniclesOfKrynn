#!/usr/bin/env python3
"""Phases 16–17: actual SQL lifecycle and native HP/runtime, private DB only."""
from clan_database import ROOT,database
from clan_snapshot_fixture import snapshot,manifest,A
import subprocess,shlex,json,sys
sys.path.insert(0,str(ROOT/'scripts'))
from build_clan_repository_tool import build
migrations=tuple(f'20261110_clan_progression_v{i:03d}.sql' for i in range(1,6))+('20261111_clan_buff_activations_v001.sql','20261111_clan_buff_lifecycle_v001.sql')
with database(migrations) as (tmp,socket,sql,client):
    sql((ROOT/'sql'/migrations[-1]).read_text())
    tool=tmp/'tool';build(tool,sanitize=True)
    p=subprocess.run([str(tool),'import','clan_test','root','',str(socket)],input=json.dumps(dict(snapshot=snapshot(),expected_revision=0,manifest=manifest())),text=True,capture_output=True,check=True)
    assert json.loads(p.stdout)['result']=='ok',p.stdout
    sql("UPDATE clan_store_control SET store_mode=2;UPDATE clan_import_manifests SET import_status=2;")
    sql(f"UPDATE clan_progression_state SET influence_eighths=40000 WHERE clan_uid='{A}'")
    source=(ROOT/'src/legendary_foundations.c').read_text();start=source.index('void legendary_foundation_hp(')
    function=source[start:source.index('\n}',start)+2]
    driver=tmp/'driver.c';driver.write_text((ROOT/'tests/clan_buff_lifecycle.c').read_text()+'\n#define time clan_test_time\n'+function+'\n#undef time\n')
    flags=shlex.split(subprocess.check_output(['pkg-config','--cflags','--libs','mariadb','openssl','json-c'],text=True))
    binary=tmp/'lifecycle'
    subprocess.run(['cc','-std=gnu99','-Wall','-Wextra','-Werror','-g','-fsanitize=address,undefined','-fno-pie','-no-pie','-DCLAN_BUFF_LIFECYCLE_TEST','-I'+str(ROOT/'src'),str(driver),*[str(ROOT/'src'/f) for f in ['clan_repository.c','clan_progression.c','clan_awards.c','clan_buff_queries.c','clan_buff_activation.c','clan_buff_commit.c','clan_buff_lifecycle.c','clan_buff_runtime.c','clan_status.c','clan_buff_commands.c']],*flags,'-o',str(binary)],check=True)
    def proc(request):return subprocess.run([str(binary),str(socket)],input=json.dumps(request),text=True,capture_output=True)
    def run(mode='reconcile',expected='ok',**kw):
        r=dict(mode=mode,now=2200000000)|kw;p=proc(r);assert p.returncode==0,(p.stdout,p.stderr)
        result=json.loads(p.stdout);assert result['result']==expected,(r,result,p.stderr);return result
    def state():return '\n'.join(sql(f'SELECT * FROM {t} ORDER BY 1,2') for t in ['clan_progression_state','clan_buff_families','clan_buff_activations','clan_buff_lifecycle_events'])
    activate=run('activate',operation_uid='1'*32);assert activate['notifications']==1
    before=state()
    for extra,reason in [({'actor':88},8),({'disabled':True},2),({'activation_uid':'9'*32},1024)]:
        args=dict(operation_uid='2'*32,activation_uid='1'*32)|extra
        assert run('cancel',expected='ineligible',**args)['reasons']&reason
        assert state()==before
    for fault in range(1,4):
        run('cancel',expected='unavailable',operation_uid='2'*32,activation_uid='1'*32,fault=fault);assert state()==before
    for crash in range(1,4):
        p=proc(dict(mode='cancel',now=2200000010,operation_uid='2'*32,activation_uid='1'*32,crash=crash));assert p.returncode==110+crash,(p.returncode,p.stdout,p.stderr);assert state()==before
    cancel=run('cancel',operation_uid='2'*32,activation_uid='1'*32,now=2200000010)['receipt']
    after=state();assert cancel['next_available_at']==2200129600
    assert run('cancel',operation_uid='2'*32,activation_uid='1'*32,now=2200001000,disabled=True)['receipt']==cancel and state()==after
    run('cancel',expected='conflict',operation_uid='2'*32,activation_uid='1'*32,actor=99)
    assert run('activate',expected='ineligible',operation_uid='3'*32,now=2200000100)['result']=='ineligible'
    run(now=2200200000);assert state()==after # already-canceled buff never expires twice
    # Restart/offline timestamps: new activation is eligible exactly at next use.
    run('activate',operation_uid='3'*32,now=2200129600);before=state()
    assert run('cancel',operation_uid='2'*32,activation_uid='1'*32,now=2200129601)['receipt']==cancel and state()==before
    run(now=2200215999);assert state()==before
    for fault in range(1,4):run(expected='unavailable',now=2200216000,fault=fault);assert state()==before
    for crash in range(1,4):
        p=proc(dict(mode='reconcile',now=2200216000,crash=crash));assert p.returncode==110+crash,(p.returncode,p.stdout,p.stderr);assert state()==before
    # Concurrent sweeps (including a lost acknowledgement) cannot repeat expiry.
    requests=[dict(mode='reconcile',now=2200216000,**({'fault':4} if i==0 else {})) for i in range(3)]
    children=[subprocess.Popen([str(binary),str(socket)],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True) for _ in requests]
    for child,request in zip(children,requests):child.stdin.write(json.dumps(request));child.stdin.close();child.stdin=None
    for child in children:
        stdout,stderr=child.communicate();assert child.returncode==0,stderr
        assert json.loads(stdout)['result'] in ['ok','commit-unknown'],stdout
    after=state();run(now=2200216001);assert state()==after
    assert sql('SELECT COUNT(*) FROM clan_buff_lifecycle_events WHERE event_kind=2')=='1'
    assert sql("SELECT occurred_at FROM clan_buff_lifecycle_events WHERE event_kind=2")=='2200216000'
    # Lost acknowledgement after cancellation COMMIT.
    run('activate',operation_uid='4'*32,now=2200259200)
    run('cancel',expected='commit-unknown',operation_uid='5'*32,activation_uid='4'*32,now=2200259201,fault=4)
    recovered=run('cancel',operation_uid='5'*32,activation_uid='4'*32,now=2200259202)['receipt']
    after=state();assert run('cancel',operation_uid='5'*32,activation_uid='4'*32,now=2200259203)['receipt']==recovered and state()==after
    # Real runtime and HP function, including absolute expiry before SQL sweep.
    run('activate',operation_uid='6'*32,now=2200388800)
    p=proc(dict(mode='runtime',now=2200388800));assert p.returncode==0,(p.stdout,p.stderr);print(p.stdout.strip())
    assert sql('SELECT COUNT(*) FROM clan_buff_lifecycle_events WHERE event_kind=2')=='2'
    # A killed canceling process after COMMIT also recovers its exact old proof.
    run('activate',operation_uid='7'*32,now=2200518400)
    request=dict(mode='cancel',operation_uid='8'*32,activation_uid='7'*32,now=2200518401,crash=4)
    p=proc(request);assert p.returncode==114,p.stderr
    proof=run('cancel',operation_uid='8'*32,activation_uid='7'*32,now=2200518402)['receipt']
    before=state();assert run('cancel',operation_uid='8'*32,activation_uid='7'*32,now=2200518403)['receipt']==proof and state()==before
    run('activate',operation_uid='9'*32,now=2200648000,family=2)
    p=proc(dict(mode='arcane-runtime',now=2200648000));assert p.returncode==0,(p.stdout,p.stderr);print(p.stdout.strip())
    run('activate',operation_uid='b'*32,now=2200777600,family=3)
    p=proc(dict(mode='psionic-runtime',now=2200777600));assert p.returncode==0,(p.stdout,p.stderr);print(p.stdout.strip())
    run('activate',operation_uid='d'*32,now=2201036800,family=1)
    p=proc(dict(mode='scholar-runtime',now=2201036800));assert p.returncode==0,(p.stdout,p.stderr);print(p.stdout.strip())
    sql(f"UPDATE clan_progression_state SET xp_eighths=4800,revision=revision+1 WHERE clan_uid='{A}'")
    run('activate',operation_uid='e'*32,now=2201166400,family=5)
    run('activate',operation_uid='f'*32,now=2201166400,family=1,slot=2)
    p=proc(dict(mode='artisan-runtime',now=2201166400));assert p.returncode==0,(p.stdout,p.stderr);print(p.stdout.strip())
    # Actual Phase 24 command adapter, injected live config on this private DB.
    def command(args,**extra):
        p=proc(dict(mode='buff-command',now=2201296000,args=args)|extra)
        assert p.returncode==0,(args,p.stdout,p.stderr)
        return json.loads(p.stdout)['text']
    text=command('list');assert "Artisan's Guidance" in text and 'Tier 1' in text and 'activation UID' not in text,text
    before=state();assert 'Usage' not in command('activate 2 1 1',disabled=True) and state()==before
    text=command('activate 2 1 1',disabled=True);assert 'disabled' in text,text
    for args in ['activate 2 -1 1','activate 2 1 99999999999',"activate 2 1 1 bad uid",'permission -1','list extra','replace 3 1 1 bad']:
        assert 'clan buffs list' in command(args) and state()==before
    assert 'authenticated' in command('list',account=0)
    assert 'ineligible' in command('permission 2',actor=88) or 'membership' in command('permission 2',actor=88)
    assert 'invalid' in command('permission 3')
    assert 'ok' in command('permission 2')
    assert sql(f"SELECT max_rank FROM clan_buff_management WHERE clan_uid='{A}'")=='2'
    sql(f"INSERT INTO clan_memberships(player_id,clan_uid,rank_id,legacy_points) VALUES(88,'{A}',2,0)")
    text=command(f'activate 2 1 1 {"1a"*16} {A}',actor=88);assert 'Your clan buff is active.' in text,text
    after=state();assert 'Your clan buff is active.' in command(f'activate 2 1 1 {"1a"*16} {A}',actor=88) and state()==after
    assert 'could not be changed' in command(f'activate 3 1 1 {"1a"*16} {A}',actor=88)
    # Occupied slots require the exact active UID before replacement.
    after=state();text=command(f'replace 3 1 1 {"8a"*16}',actor=88)
    assert 'changed' in text and state()==after,text
    after=state();text=command(f'activate 3 1 1 {"4a"*16} {A}',actor=88)
    assert 'explicit replacement' in text and state()==after,text
    text=command(f'activate 3 1 1 {"4a"*16} {A} {"8a"*16}',actor=88)
    assert 'changed' in text and state()==after,text
    text=command(f'activate 3 1 1 {"5a"*16} {A} {"1a"*16}',actor=88)
    assert 'Your clan buff is active.' in text,text
    text=command(f'replace 4 1 1 {"5a"*16}',actor=88);assert 'Your clan buff is active.' in text,text
    text=command('list');assert 'Ready again' in text and 'UTC' in text
    # Revoke management: no refresh/cancel bypass; leader can still cancel.
    assert 'ok' in command('permission 0')
    assert 'permission' in command(f'cancel 0123456789abcdef0123456789abcdef {"2a"*16} {A}',actor=88)
    assert 'has been cancelled' in command(f'cancel 0123456789abcdef0123456789abcdef {"3a"*16} {A}')
    sql('DELETE FROM clan_memberships WHERE player_id=88')
    after=state();assert 'Your clan buff is active.' in command(f'activate 2 1 1 {"1a"*16} {A}',actor=88,disabled=True) and state()==after
    assert 'ok' in command('permission 2')
    named=command("activate Scholar's Encouragement 1 2",now=2202000000,generated_uid="a1"*16);assert 'Your clan buff is active.' in named,named
    assert 'Exact retry' not in named and A not in named
    assert 'Your clan buff is active.' in command('replace Arcane Readiness 1 2',now=2202000000,generated_uid='a2'*16)
    assert 'has been cancelled' in command('cancel Arcane Readiness',now=2202000000,generated_uid='a3'*16)
    sql(f"UPDATE clan_progression_legacy SET details=JSON_SET(details,'$.ranks',JSON_ARRAY('Leader')) WHERE clan_uid='{A}'")
    assert 'ok' in command('permission 0')
    sql(f"UPDATE clan_progression_legacy SET details=JSON_SET(details,'$.ranks',JSON_ARRAY('Leader','Member')) WHERE clan_uid='{A}'")
    print('Phase 24 command parsing, listing, delegation/revocation, activation/cancel, exact retry/conflict and departure replay passed')
    sql('DROP TRIGGER cble_retained');before=state()
    run(expected='schema-mismatch',now=2200500000);assert state()==before
    assert 'lifecycle storage is not ready' in command('activate 4 1 1') and state()==before
    print('Phases 16–17: exact-once cancellation/expiry, retained cooldowns, restart/offline reconciliation, faults/crashes/retries, invalidation and Company Vitality HP invariants passed.')
