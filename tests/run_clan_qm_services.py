#!/usr/bin/env python3
"""Clan builder access and replacement quartermaster economy/recovery."""
from clan_database import ROOT,database
from clan_snapshot_fixture import snapshot,manifest,A
import subprocess,json,shlex,sys
sys.path.insert(0,str(ROOT/'scripts'))
from build_clan_repository_tool import build
migrations=tuple(f'20261110_clan_progression_v{i:03d}.sql' for i in range(1,6))+('20261113_clan_quartermaster_services_v001.sql',)
with database(migrations) as (tmp,socket,sql,client):
 tool=tmp/'tool';build(tool,sanitize=True)
 snap=snapshot();snap['members'][0]['player_id']=1007;snap['clans'][0]['core']['leader_id']=1007
 subprocess.run([str(tool),'import','clan_test','root','',str(socket)],input=json.dumps(dict(snapshot=snap,expected_revision=0,manifest=manifest())),text=True,check=True,stdout=subprocess.DEVNULL)
 sql(f"UPDATE clan_store_control SET store_mode=2;UPDATE clan_import_manifests SET import_status=2;INSERT INTO clan_personal_favor VALUES(1007,'{A}',1832,0,2200000000);INSERT INTO clan_memberships VALUES(1010,'{A}',2,0);INSERT INTO clan_personal_favor VALUES(1010,'{A}',1832,0,2200000000)")
 flags=shlex.split(subprocess.check_output(['pkg-config','--cflags','--libs','mariadb','json-c','openssl'],text=True));native=tmp/'service'
 subprocess.run(['cc','-std=gnu99','-Wall','-Wextra','-Wno-unused-parameter','-ffunction-sections','-fdata-sections','-Wl,--gc-sections','-g','-fsanitize=address,undefined','-fno-pie','-no-pie','-Isrc',*[str(ROOT/p) for p in ['tests/clan_qm_services_runtime.c','src/clan_access.c','src/clan_enchant.c','src/clan_repository.c','src/clan_awards.c','src/clan_progression.c']],*flags,'-o',str(native)],cwd=ROOT,check=True)
 def run(path,mode=0,command='list',crash=0,fail=0,now=2200010001,player=1007,expected=0):
  path.mkdir(exist_ok=True)
  p=subprocess.run([str(native),str(socket),str(path),str(mode),command,str(crash),str(fail),str(now),str(player)],cwd=ROOT/'lib',capture_output=True,text=True)
  assert p.returncode==expected,(p.returncode,p.stdout,p.stderr)
  return json.loads(p.stdout) if not expected else None
 path=tmp/'character';run(path,3);run(tmp/'other-rank',3,player=1010);run(path,4)
 x=run(path,1,'motes fire 2');assert x['motes']==2 and x['gold']==9995000 and not x['locked'],x
 assert run(path,2)['motes']==2
 assert 'allowance' in run(path,0,'motes fire 1')['messages']
 assert 'allowance' in run(path,0,'motes ice 1')['messages']
 other=run(tmp/'other',1,'motes fire 2',player=1010);assert other['motes']==2 # same account, separate character
 x=run(path,1,'quest 10');assert x['qp']==10 and x['gold']==9985000,x
 x=run(path,1,'enhance sword');assert x['effect'] and x['gold']==9735000,x
 frozen=sql('SELECT effect,command_word FROM clan_quartermaster_services WHERE kind=3')
 run(path,5)
 x=run(path,1,'reroll sword');assert x['effect'] and x['gold']==9235000 and x['week']>0,x
 # A second reroll cannot even produce a confirmation quote.
 x=run(path,0,'reroll sword');assert 'eligible' in x['messages'] and x['gold']==9235000,x
 # The weekly lock follows the weapon across a character transfer.
 transferred=tmp/'transferred';transferred.mkdir()
 (transferred/'inventory.json').write_text((path/'inventory.json').read_text())
 assert 'eligible' in run(transferred,0,'reroll sword',player=1010)['messages']
 # Even a stale object timestamp cannot bypass the SQL unique weapon/week key.
 stale=json.loads((transferred/'inventory.json').read_text());stale['week']=0
 (transferred/'inventory.json').write_text(json.dumps(stale))
 rejected=run(transferred,1,'reroll sword',player=1010)
 assert rejected['gold']==10000000 and 'could not be accepted' in rejected['messages'],rejected
 # Same receipt may be read/recovered repeatedly without charging again.
 assert run(path,2)['gold']==9235000
 sql('TRUNCATE TABLE clan_quartermaster_services')
 for kind,command,cost,key,quantity in [('motes','motes fire 1',2500,'motes',1),('quest','quest 1',1000,'qp',1),('weapon','enhance sword',250000,'effect',None)]:
  for stop in [1,2,3]:
   sql('TRUNCATE TABLE clan_quartermaster_services')
   path=tmp/f'{kind}-crash-{stop}'
   # Weapon preview first durably saves its identity, one extra checkpoint.
   checkpoint=stop+(kind=='weapon')
   run(path,1,command,crash=checkpoint,expected=88)
   x=run(path,2);assert x['gold']==10000000-cost and not x['locked'] and (x[key]==quantity if quantity else x[key]>0),(kind,stop,x)
   assert run(path,2)['gold']==x['gold']
  for failure in [1,2,3,-1,-2,-3]:
   sql('TRUNCATE TABLE clan_quartermaster_services');path=tmp/f'{kind}-failure-{failure}'
   # For failure tests supply a saved weapon identity before injecting saves.
   if kind=='weapon':run(path,0,'enhance sword')
   x=run(path,1,command,fail=failure)
   x=run(path,2);assert x['gold']==10000000-cost and not x['locked'] and (x[key]==quantity if quantity else x[key]>0),(kind,failure,x)
 # An unavailable acknowledgement leaves a retryable delivery, never a second debit.
 sql('TRUNCATE TABLE clan_quartermaster_services');path=tmp/'ack-failure'
 sql("CREATE TRIGGER cqs_test_ack BEFORE UPDATE ON clan_quartermaster_services FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='injected ack failure'")
 pending=run(path,1,'quest 1');assert pending['locked'] and pending['qp']==1 and pending['gold']==9999000,pending
 sql('DROP TRIGGER cqs_test_ack');complete=run(path,2)
 assert not complete['locked'] and complete['qp']==1 and complete['gold']==9999000,complete
 # Retained receipt facts cannot be edited or deleted to reset weekly usage.
 for query in ['DELETE FROM clan_quartermaster_services', 'UPDATE clan_quartermaster_services SET price=1', 'UPDATE clan_quartermaster_services SET state=0']:
  assert subprocess.run(client,input=query,text=True,capture_output=True).returncode!=0,query
 # Recovery remains valid after leaving the clan.
 sql('TRUNCATE TABLE clan_quartermaster_services');path=tmp/'departure'
 run(path,1,'quest 1',crash=1,expected=88);sql('DELETE FROM clan_memberships WHERE player_id=1007')
 x=run(path,2);assert x['qp']==1 and x['gold']==9999000,x
 assert 'membership' in run(path,0,'quest 1')['messages']
 print('Clan access, unlocked-only weighted rolls, per-character quotas, weapon rerolls, activation, departure, and independent checkpoint crash/failure recovery passed.')
