#!/usr/bin/env python3
from clan_database import ROOT,database
from clan_snapshot_fixture import snapshot,manifest,A,B
import sys,json,subprocess,copy
sys.path.insert(0,str(ROOT/'scripts'))
from build_clan_repository_tool import build
with database(('20261110_clan_progression_v001.sql','20261110_clan_progression_v002.sql','20261110_clan_progression_v003.sql','20261110_clan_progression_v004.sql','20261110_clan_progression_v005.sql')) as (tmp,socket,sql,client):
 tool=tmp/'tool';build(tool,sanitize=True)
 def run(operation,payload=None,result='ok'):
  p=subprocess.run([str(tool),operation,'clan_test','root','',str(socket)],input=json.dumps(payload or {}),text=True,capture_output=True)
  assert p.stdout,(p.stderr,p.returncode)
  out=json.loads(p.stdout);assert out['result']==result,(operation,out,p.stderr)
  assert (p.returncode==0)==(result=='ok'),p.stderr
  return out
 initial=snapshot();run('validate',initial)
 for bad in [[],{},dict(initial,clans=[1]),dict(initial,members=[[]]),dict(initial,claims=['bad'])]:run('validate',bad,'invalid')
 run('import',dict(snapshot=initial,expected_revision=0,manifest=manifest()))
 stored=run('read');assert stored['store_revision']==1 and stored['store_mode']==0
 for name in ['clans','members','claims']:assert stored[name]==initial[name],name
 before=run('read');run('import',dict(snapshot=initial,expected_revision=1,manifest=manifest()),'conflict');assert run('read')==before
 receipt=run('manifest',dict(source_digest='d'*64));assert receipt['source_manifest']==manifest()['source_manifest']
 sql('UPDATE clan_store_control SET store_mode=2; UPDATE clan_import_manifests SET import_status=2;')
 changed=run('read');changed['written_at']=2200000001;changed['clans'][0]['core']['updated_at']=2200000001
 changed['clans'][0]['core']['xp_eighths']=9223372036854775807;changed['clans'][0]['core']['treasury_gold']=12345
 changed['members'][1]['rank_id']=2
 run('save',dict(snapshot=changed,expected_revision=1))
 stored=run('read');assert stored['clans'][0]['core']['xp_eighths']==9223372036854775807 and stored['members'][1]['rank_id']==2
 run('save',dict(snapshot=changed,expected_revision=1),'conflict')
 for mutate in [lambda x:x['clans'][0]['legacy']['privileges'].__setitem__(1,dict(id=0,rank=2)),lambda x:x['members'][0].update(rank_id=3),lambda x:x['claims'][0]['popularity'][0].update(percentage=101),lambda x:x['clans'][0]['core'].update(xp_eighths=9223372036854775808),lambda x:x['clans'][0]['legacy']['relationships'][0].update(peer_uid='f'*32)]:
  bad=copy.deepcopy(stored);bad['written_at']=2200000002;mutate(bad);run('save',dict(snapshot=bad,expected_revision=2),'invalid')
 assert run('read')==stored
 sql("CREATE TRIGGER fail_claim BEFORE INSERT ON clan_zone_claims FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='test rollback';")
 failed=copy.deepcopy(stored);failed['written_at']=2200000002;failed['clans'][0]['core']['treasury_gold']=1;failed['members']=[]
 run('save',dict(snapshot=failed,expected_revision=2),'invalid');assert run('read')==stored
 sql('DROP TRIGGER fail_claim')
 # Delete/recreate with retained old balances and no inherited clan identity.
 removed=copy.deepcopy(stored);removed['written_at']=2200000002;removed['clans']=removed['clans'][1:];removed['clans'][0]['legacy']['relationships']=[];removed['members']=removed['members'][1:];removed['claims']=[]
 run('save',dict(snapshot=removed,expected_revision=2));assert sql(f"SELECT record_status,xp_eighths FROM clan_progression_clans JOIN clan_progression_state USING(clan_uid) WHERE clan_uid='{A}'")=='2\t9223372036854775807'
 assert len(run('read')['clans'])==1
 sql('ALTER TABLE clan_progression_state ALTER COLUMN xp_eighths SET DEFAULT 1')
 run('read',result='schema-mismatch')
 sql('ALTER TABLE clan_progression_state ALTER COLUMN xp_eighths SET DEFAULT 0')
 run('read')
 sql('ALTER TABLE clan_memberships DROP CONSTRAINT cp_member_rank')
 run('read',result='schema-mismatch')
 print('Clan snapshots: complete scalar/rank/privilege/member/applicant/relationship/claim round trips, global revisions, invalid/corrupt input, int64 boundaries, atomic failure rollback, import receipts and retained deletion history passed.')
