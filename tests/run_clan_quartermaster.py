#!/usr/bin/env python3
"""Phases 39-41: strict catalog, atomic acquisition, replay and departure proof."""
from clan_database import ROOT,database
from clan_snapshot_fixture import snapshot,manifest,A,B
import json,subprocess,shlex,sys,copy,re,shutil
sys.path.insert(0,str(ROOT/'scripts'))
from build_clan_repository_tool import build
catalog=json.loads((ROOT/'lib/clan/quartermaster.json').read_text())
# Audit the actual indexed prototypes. Ordinary gear only: no stats, scripts,
# spells or abilities may be smuggled into an approved zero-utility budget.
indexed=set((ROOT/'lib/world/obj/index').read_text().splitlines())
for d in catalog['items']:
    found=[]
    for file in indexed:
        p=ROOT/'lib/world/obj'/file
        if not p.is_file():continue
        m=re.search(r'^#'+str(d['prototype'])+r'\n(.*?)(?=^#|\Z)',p.read_text(errors='replace'),re.M|re.S)
        if m:found.append(m[1])
    assert len(found)==1,(d['id'],'missing/duplicate indexed prototype')
    tail=found[0].split('~',4)[4];lines=tail.strip().splitlines();flags=lines[0].split();vals=list(map(int,lines[1].split()));base=list(map(int,lines[2].split()))
    assert int(flags[0])==d['type'] and vals==d['values'] and base[0]==d['weight'] and base[1]==d['value'] and base[3]==d['level'],d
    assert not re.search(r'^[ATV]$',tail,re.M),d['id']
    assert d['price']==d['kind']*d['value'] and d['values'][4]==0
migrations=tuple(f'20261110_clan_progression_v{i:03d}.sql' for i in range(1,6))+('20261112_clan_quartermaster_v001.sql',)
with database(migrations) as (tmp,socket,sql,client):
    tool=tmp/'tool';build(tool,sanitize=True)
    snap=snapshot();snap['members'][0]['player_id']=1007;snap['clans'][0]['core']['leader_id']=1007
    snap['clans'][0]['core']['xp_eighths']=0
    subprocess.run([str(tool),'import','clan_test','root','',str(socket)],input=json.dumps(dict(snapshot=snap,expected_revision=0,manifest=manifest())),text=True,check=True,stdout=subprocess.DEVNULL)
    sql('UPDATE clan_store_control SET store_mode=2;UPDATE clan_import_manifests SET import_status=2')
    sql(f"UPDATE clan_progression_state SET xp_eighths=44800,revision=revision+1 WHERE clan_uid='{A}'")
    sql(f"INSERT INTO clan_personal_favor VALUES(1007,'{A}',12000,0,2200000000)")
    flags=shlex.split(subprocess.check_output(['pkg-config','--cflags','--libs','mariadb','json-c','openssl'],text=True));binary=tmp/'qm'
    subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-pedantic','-g','-fsanitize=address,undefined','-fno-pie','-no-pie','-Isrc',*[str(ROOT/p) for p in ['tests/clan_quartermaster.c','src/clan_quartermaster.c','src/clan_repository.c','src/clan_awards.c','src/clan_progression.c']],*flags,'-o',str(binary)],cwd=ROOT,check=True)
    base=dict(catalog=catalog,index=0,uid='1'*32,clan=A,player=1007,account=77,level=10,gold=10000,payload="EOBJ frozen ' item\n",now=2200010001)
    def run(op,change=None,expected='ok'):
        p=subprocess.run([str(binary),str(socket),op],input=json.dumps(base|(change or {})),capture_output=True,text=True);assert p.returncode==0,(p.stdout,p.stderr)
        result=json.loads(p.stdout);assert result['result']==expected,(op,result,p.stderr);return result.get('data')
    run('parse')
    for change in [dict(price=0),dict(price=1),dict(prototype=0),dict(level=31),dict(favor=0),dict(clan_level=1),dict(values=[0]*15),dict(budget='')]:
        bad=copy.deepcopy(catalog);bad['items'][0].update(change);run('parse',dict(catalog=bad),'invalid')
    for change in [dict(version=2),dict(items=[]),dict(items=catalog['items']*3)]:run('parse',dict(catalog=catalog|change),'invalid')
    bad=copy.deepcopy(catalog);bad['items'].append(bad['items'][0]);run('parse',dict(catalog=bad),'invalid')
    run('prepare',dict(disabled=1),'ineligible');run('prepare',dict(gold=0),'ineligible');run('prepare',dict(player=1009,clan=B),'ineligible')
    # Membership, clan level, Favor and character gates rechecked on commit.
    sql(f"UPDATE clan_memberships SET rank_id=0 WHERE player_id=1007")
    run('prepare',expected='ineligible');sql('UPDATE clan_memberships SET rank_id=1 WHERE player_id=1007')
    run('prepare',dict(index=4,level=1),'ineligible')
    sql(f"INSERT INTO clan_personal_favor VALUES(5000000009,'{B}',12000,0,2200000000);UPDATE clan_memberships SET rank_id=1 WHERE clan_uid='{B}'")
    run('prepare',dict(player=5000000009,clan=B),'ineligible') # clan below initial level-2 unlock

    sql(f"INSERT INTO clan_memberships VALUES(1010,'{A}',2,0)")
    run('prepare',dict(player=1010),'ineligible')
    sql("CREATE TRIGGER qm_injected BEFORE INSERT ON clan_quartermaster_operations FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='test rollback'")
    run('prepare',expected='unavailable');assert sql('SELECT COUNT(*) FROM clan_quartermaster_operations')=='0';sql('DROP TRIGGER qm_injected')
    retained=sql('SELECT * FROM clan_personal_favor;SELECT * FROM clan_progression_state')
    run('prepare');first=run('read');run('prepare');assert run('read')==first
    run('prepare',dict(payload='changed'),'conflict');run('prepare',dict(uid='2'*32),'conflict')
    run('read',dict(player=1010),'not-found')
    # Frozen proofs and recipient survive departure; acquisition does not.
    sql('DELETE FROM clan_memberships WHERE player_id=1007')
    assert run('read')==first
    run('prepare',dict(uid='3'*32),'ineligible')
    run('ack');paid=run('read');assert paid['state']==1
    run('ack');assert run('read')==paid
    assert sql('SELECT * FROM clan_personal_favor;SELECT * FROM clan_progression_state')==retained
    assert run('read',dict(pending=True),expected='not-found') is None
    for query in ["DELETE FROM clan_quartermaster_operations", "UPDATE clan_quartermaster_operations SET state=0", "UPDATE clan_quartermaster_operations SET payload='other'", "UPDATE clan_quartermaster_operations SET account_id=99"]:
        p=subprocess.run(client,input=query,text=True,capture_output=True);assert p.returncode!=0,query
    # Populated rerun is non-destructive. Missing immutability guards fail closed.
    sql((ROOT/'sql/20261112_clan_quartermaster_v001.sql').read_text());assert run('read')==paid
    sql('DROP TRIGGER qm_terms');run('read',expected='schema-mismatch');sql((ROOT/'sql/20261112_clan_quartermaster_v001.sql').read_text())
    # Concurrent distinct intents serialize to one outstanding purchase per recipient.
    sql(f"INSERT INTO clan_personal_favor VALUES(1010,'{A}',12000,0,2200000000)")
    contenders=[base|dict(player=1010,account=90,uid=f'{x:032x}') for x in [400,401]]
    children=[subprocess.Popen([str(binary),str(socket),'prepare'],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True) for _ in contenders]
    for child,request in zip(children,contenders):child.stdin.write(json.dumps(request));child.stdin.close();child.stdin=None
    results=[json.loads(child.communicate()[0])['result'] for child in children]
    assert sorted(results)==['conflict','ok'],results
    assert sql('SELECT COUNT(*) FROM clan_quartermaster_operations WHERE active_player=1010')=='1'
    print('Quartermaster catalog/world budgets, SQL acquisition gates, rollback/replay, frozen delivery, departure, retained Favor and schema guards passed.')

    # Actual coordinator and commands; independent stores persist across C processes.
    sql(f"INSERT INTO clan_memberships VALUES(1007,'{A}',1,0)")
    native=tmp/'runtime'
    subprocess.run(['cc','-std=gnu99','-Wall','-Wextra','-Wno-unused-parameter','-g','-ffunction-sections','-fdata-sections','-Wl,--gc-sections','-fsanitize=address,undefined','-fno-pie','-no-pie','-Isrc',*[str(ROOT/p) for p in ['tests/clan_quartermaster_runtime.c','src/clan_quartermaster.c','src/clan_repository.c','src/clan_awards.c','src/clan_progression.c']],*flags,'-o',str(native)],cwd=ROOT,check=True)
    def invoke(path,mode,uid,crash=0,fail=0,ack=0,expected_exit=0):
        p=subprocess.run([str(native),str(socket),str(path),mode,uid,str(crash),str(fail),str(ack)],capture_output=True,text=True,cwd=ROOT/'lib')
        assert p.returncode==expected_exit,(p.returncode,p.stdout,p.stderr)
        if expected_exit:return
        return json.loads(p.stdout)
    for stop in range(5):
        path=tmp/f'crash-{stop}';path.mkdir();uid=f'{100+stop:032x}'
        if stop==4:invoke(path,'begin',uid,ack=1,expected_exit=89)
        elif stop:invoke(path,'begin',uid,crash=stop,expected_exit=88)
        else:
            result=invoke(path,'begin',uid);assert result['gold']==9985 and result['items']==1 and not result['locked'],result
        if stop==1:sql('DELETE FROM clan_memberships WHERE player_id=1007')
        result=invoke(path,'recover',uid);assert result['ok'] and result['gold']==9985 and result['items']==1 and not result['locked'],(stop,result)
        assert invoke(path,'recover',uid)['items']==1
        replay=invoke(path,'command',f'confirm field-map {uid}');assert replay['gold']==9985 and replay['items']==1 and 'already' in replay['messages'],replay
        if stop==1:
            denial=invoke(path,'command','buy field-map');assert 'membership' in denial['messages'] and denial['items']==1,denial
            sql(f"INSERT INTO clan_memberships VALUES(1007,'{A}',1,0)")
    for failure in [1,2,3,-1,-2,-3]:
        path=tmp/f'failure-{failure}';path.mkdir();uid=f'{200+failure:032x}'
        result=invoke(path,'begin',uid,fail=failure)
        assert result['locked'],(failure,result)
        result=invoke(path,'recover',uid);assert result['ok'] and result['gold']==9985 and result['items']==1 and not result['locked'],(failure,result)
    path=tmp/'ack-failure';path.mkdir();uid=f'{500:032x}'
    sql("CREATE TRIGGER qm_ack_failure BEFORE UPDATE ON clan_quartermaster_operations FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='injected acknowledgement failure'")
    result=invoke(path,'begin',uid);assert result['locked'] and result['gold']==9985 and result['items']==1,result
    sql('DROP TRIGGER qm_ack_failure');result=invoke(path,'recover',uid);assert result['ok'] and result['gold']==9985 and result['items']==1,result
    path=tmp/'missing-item';path.mkdir();uid=f'{501:032x}'
    invoke(path,'begin',uid,crash=2,expected_exit=88)
    saved=json.loads((path/'inventory.json').read_text());(path/'inventory.json').write_text(json.dumps(saved|dict(items=[])))
    result=invoke(path,'recover',uid);assert not result['ok'] and result['locked'] and result['items']==0 and result['gold']==9985,result
    (path/'inventory.json').write_text(json.dumps(saved));result=invoke(path,'recover',uid);assert result['ok'] and result['items']==1 and result['gold']==9985,result
    path=tmp/'commands';path.mkdir()
    preview=invoke(path,'command','buy field-map');assert preview['gold']==10000 and preview['items']==0 and 'quartermaster confirm' in preview['messages'] and 'confirm field-map' not in preview['messages'],preview
    named_path=tmp/'named-commands';named_path.mkdir();invoke(named_path,'named','unused')
    restricted=invoke(path,'restricted','unused');assert restricted['items']==0 and restricted['gold']==10000 and 'restrictions' in restricted['messages'],restricted
    invoke(path,'world','unused')
    invoke(path,'quotes','unused')
    raw=invoke(path,'command','confirm field-map '+('f'*32));assert 'Preview' in raw['messages'] and raw['items']==0,raw
    # Phase 50: a coherent backup captures SQL plus independent player/inventory
    # files at one quiesced crash checkpoint. Restore into another private DB.
    path=tmp/'backup-checkpoint';path.mkdir();uid=f'{700:032x}'
    invoke(path,'begin',uid,crash=2,expected_exit=88)
    files=tmp/'checkpoint-files';shutil.copytree(path,files)
    dump=subprocess.check_output(['mariadb-dump','--no-defaults','--socket='+str(socket),'-uroot','--single-transaction','--triggers','clan_test'])
    sql('CREATE DATABASE clan_qm_restored CHARACTER SET utf8mb4')
    subprocess.run([*client[:-1],'clan_qm_restored'],input=dump,check=True,stdout=subprocess.DEVNULL)
    # Original continues after the backup; that later ACK must not be required
    # to recover the separate restored checkpoint.
    original=invoke(path,'recover',uid);assert original['gold']==9985 and original['items']==1
    restored=tmp/'restored-files';shutil.copytree(files,restored)
    def restored_recover():
        p=subprocess.run([str(native),str(socket),str(restored),'recover',uid,'0','0','0','clan_qm_restored'],capture_output=True,text=True,cwd=ROOT/'lib')
        assert p.returncode==0,(p.stdout,p.stderr)
        return json.loads(p.stdout)
    recovered=restored_recover()
    assert recovered['ok'] and recovered['gold']==9985 and recovered['items']==1 and not recovered['locked'],recovered
    again=restored_recover()
    assert all(again[k]==recovered[k] for k in ['ok','gold','items','locked']),again
    assert sql(f"SELECT state FROM clan_qm_restored.clan_quartermaster_operations WHERE operation_uid='{uid}'")=='1'
    print('Phase 50 coherent SQL/player/inventory checkpoint restore and exactly-once recovery passed.')
    print('Quartermaster game coordinator: independent player/inventory crash recovery, uncertain checkpoint retries, durable acknowledgement, departure delivery/use, confirmation replay, item restrictions and budget drift passed.')
