#!/usr/bin/env python3
"""Phases 5a, 5b and 6 with real checked C code and disposable SQL only."""
from clan_database import ROOT,database
from clan_snapshot_fixture import snapshot,manifest,A,B
import subprocess,sys,shlex,json,copy,time,os
sys.path.insert(0,str(ROOT/'scripts'))
from build_clan_repository_tool import build
migrations=('20261110_clan_progression_v001.sql','20261110_clan_progression_v002.sql','20261110_clan_progression_v003.sql','20261110_clan_progression_v004.sql','20261110_clan_progression_v005.sql')
with database(migrations) as (tmp,socket,sql,client):
 tool=tmp/'tool';build(tool,sanitize=True)
 def bridge(op,payload=None,result='ok'):
  p=subprocess.run([str(tool),op,'clan_test','root','',str(socket)],input=json.dumps(payload or {}),text=True,capture_output=True)
  assert p.stdout,(p.returncode,p.stderr)
  out=json.loads(p.stdout);assert out['result']==result,(op,out,p.stderr)
  return out
 flags=shlex.split(subprocess.check_output(['pkg-config','--cflags','--libs','mariadb','openssl','json-c'],text=True))
 def compile(name,extra=()):
  binary=tmp/name
  subprocess.run(['cc','-std=gnu99','-Wall','-Wextra','-Werror','-g','-fsanitize=address,undefined','-fno-pie','-no-pie','-I'+str(ROOT/'src'),str(ROOT/f'tests/{name}.c'),*[str(ROOT/'src'/f) for f in extra],str(ROOT/'src/clan_repository.c'),str(ROOT/'src/clan_progression.c'),*flags,'-o',str(binary)],check=True)
  return binary
 boot=compile('clan_boot',('clan_boot.c','clan_persistence.c','dotenv.c'));favor=compile('clan_favor')
 def run(scenario):subprocess.run([str(boot),str(socket),scenario],check=True,cwd=tmp,env={k:v for k,v in os.environ.items() if k!='CLAN_STORE_MODE'})
 run('legacy');run('shadow');run('invalid');run('unavailable');run('rejected')
 bridge('cutover',{'expected_revision':0},'invalid')
 bridge('cutover',{'expected_revision':1,'fresh':True},'conflict')
 bridge('cutover',{'expected_revision':0,'fresh':True});run('fresh')
 # Restore an empty LEGACY staging fixture, never a live rollback.
 sql('UPDATE clan_store_control SET store_mode=0,revision=0')
 initial=snapshot();initial['clans'][0]['core']['treasury_gold']=5000000000
 for entry in initial['clans']:
  for key in ('created_at','updated_at','state_updated_at'):entry['core'][key]=int(time.time())-100
 initial['written_at']=int(time.time())-100
 bridge('import',dict(snapshot=initial,expected_revision=0,manifest=manifest()))
 before=bridge('read');run('rejected')
 bridge('cutover',{'expected_revision':1,'fresh':True},'conflict')
 bridge('cutover',{'expected_revision':1,'source_digest':'f'*64},'conflict')
 assert bridge('read')==before
 # Inject a failure after receipt update, prove control and receipt roll back.
 sql("CREATE TRIGGER fail_cutover BEFORE UPDATE ON clan_store_control FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='injected cutover failure'")
 bridge('cutover',{'expected_revision':1,'source_digest':'d'*64},'invalid')
 assert sql('SELECT import_status FROM clan_import_manifests')=='1' and bridge('read')==before
 sql('DROP TRIGGER fail_cutover')
 bridge('cutover',{'expected_revision':1,'source_digest':'d'*64})
 after=bridge('read');assert after['store_mode']==2 and after['store_revision']==2
 for key in ['clans','claims','members']:assert before[key]==after[key]
 bridge('cutover',{'expected_revision':1,'source_digest':'d'*64},'conflict')
 # Expiry/cancellation keeps family rows and cooldowns; no recomputation at boot.
 for family,canceled in [(1,0),(2,1)]:
  sql(f"INSERT INTO clan_buff_families VALUES('{B}',{family},'{str(family)*32}',1,{family},1,200,300,500,800,{canceled})")
 run('migrated');run('migrated');run('missing-player')
 (tmp/'.env').write_text('CLAN_STORE_MODE=mysql\n');run('file-mysql');run('env-legacy');(tmp/'.env').unlink()
 assert sql('SELECT next_available_at FROM clan_buff_families ORDER BY family_id')=='500\n500'
 sql(f"UPDATE clan_buff_families SET rules_version=2 WHERE clan_uid='{B}' AND family_id=1")
 run('rejected');sql('UPDATE clan_buff_families SET rules_version=1')
 sql('UPDATE clan_buff_families SET canceled=0,slot_id=1 WHERE family_id=2');run('rejected')
 sql('UPDATE clan_buff_families SET canceled=1,slot_id=2 WHERE family_id=2')
 # Post-cutover write cannot be rolled back by reimporting a stale snapshot.
 changed=copy.deepcopy(after);changed['written_at']=2200000001;changed['clans'][0]['core']['treasury_gold']=5000000123;changed['clans'][0]['core']['updated_at']=2200000001
 bridge('save',dict(snapshot=changed,expected_revision=2));committed=bridge('read')
 bridge('import',dict(snapshot=initial,expected_revision=3,manifest=manifest()),'invalid');assert bridge('read')==committed
 sql(f"INSERT INTO clan_personal_favor VALUES(5000000010,'{B}',777,0,101)")
 # Rehearse SQL-only recovery into a different private database from a full dump.
 dump=subprocess.check_output(['mariadb-dump','--no-defaults','--socket='+str(socket),'--user=root','--single-transaction','--triggers','clan_test'])
 sql('CREATE DATABASE clan_recovered');subprocess.run([*client[:-1],'clan_recovered'],input=dump,check=True,stdout=subprocess.DEVNULL)
 recovered=subprocess.run([str(tool),'read','clan_recovered','root','',str(socket)],input='{}',text=True,capture_output=True,check=True)
 assert json.loads(recovered.stdout)==committed
 assert sql('SELECT favor_eighths FROM clan_recovered.clan_personal_favor')=='777'
 assert sql('SELECT next_available_at FROM clan_recovered.clan_buff_families ORDER BY family_id')=='500\n500'
 subprocess.run([str(favor),str(socket)],check=True)
 # Current migration rerun preserves Favor and cooldown history.
 source=(ROOT/'sql'/migrations[-1]).read_text();subprocess.run(client,input=source,text=True,check=True,stdout=subprocess.DEVNULL)
 assert sql('SELECT COUNT(*) FROM clan_personal_favor')=='3'
 print('Phases 5a/5b/6: boot defaults/authority, import/cutover atomic retries, SQL recovery with post-cutover writes, retained timers and cumulative Favor passed')
