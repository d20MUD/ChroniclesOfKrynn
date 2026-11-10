#!/usr/bin/env python3
"""End-to-end native-repository import from representative legacy files."""
from clan_database import ROOT,database
from clan_snapshot_fixture import snapshot,FIELDS
import sys,json,subprocess,time,hashlib,copy,stat
sys.path.insert(0,str(ROOT/'scripts'))
from build_clan_repository_tool import build
with database(('20261110_clan_progression_v001.sql','20261110_clan_progression_v002.sql')) as (tmp,socket,sql,client):
 tool=tmp/'tool';build(tool,sanitize=True);source=tmp/'legacy';source.mkdir();players=source/'players';(players/'A-E').mkdir(parents=True)
 data=snapshot();data['clans'][1]['legacy']['relationships'][0]['at_war']=True
 lines=['* representative legacy clans']
 legacy_order=list(reversed(data['clans']))
 for entry in legacy_order:
  core,legacy=entry['core'],entry['legacy'];lines += ['#'+str(core['vnum']),'Name: '+core['name'],'Init: '+core['abbreviation'],'Desc:','Room text includes a fake Cln : 123 inside a description.','\nSecond paragraph~','Lder: '+str(core['leader_id']),'Bank: '+str(core['treasury_gold']),'DFnd: '+str(core['founded_at']),'LAct: '+str(core['last_activity_at'])]
  lines += [tag.ljust(4)+': '+str(legacy[name]) for name,member,tag in FIELDS]
  lines+=['Rank:',*legacy['ranks'],'~','Priv:',*[str(p['id'])+' '+str(p['rank']) for p in reversed(legacy['privileges'])],'~']
  for tag,flag in [('Ally','allied'),('War ','at_war')]:
   values=[0]*25;values[[e['core']['vnum'] for e in legacy_order].index(9 if core['vnum']==7 else 7)]=int(legacy['relationships'][0][flag]);lines.append(tag+': '+' '.join(map(str,values)))
 lines+=['$'];(source/'clans').write_text('\n'.join(lines)+'\n')
 (source/'claims').write_text('* Claims\n#100\nClan: 7\nClmt: 5000000007\nPopu:\n0 82.750000\n1 17.250000\n~\n$\n')
 entries=[(5000000007,'alice',7,1,101),(5000000009,'caide',9,1,202),(5000000010,'bob',7,0,303),(5000000011,'dena',0,0,404)]
 for id,name,clan,rank,points in entries:
  (players/'A-E'/(name+'.plr')).write_text(f'Name: {name.capitalize()}\nId  : {id}\n'+''.join(f'{tag}:\nCln : 444\nNot membership.~\n' for tag in ('Bond','BGrd','Flaw','Goal','Idel','Pers'))+f'Desc:\nCln : 444\nThis is only character description text.~\nCln : {clan}\nClrk: {rank}\nCPts: {points}\n')
 (source/'index').write_text('\n'.join(f'{id} {name} 10 0 123456 {clan}' for id,name,clan,rank,points in entries)+'\n~\n')
 backup=tmp/'backups'
 base=[sys.executable,str(ROOT/'scripts/clan_legacy_import.py'),'--clans',str(source/'clans'),'--claims',str(source/'claims'),'--index',str(source/'index'),'--players',str(players),'--tool',str(tool)]
 db=['--database','clan_test','--user','root','--socket',str(socket)]
 def run(extra=(),ok=True):
  result=subprocess.run([*base,*extra],text=True,capture_output=True)
  assert (result.returncode==0)==ok,(result.stdout,result.stderr)
  return json.loads(result.stdout if ok else result.stderr)
 before_files={str(p.relative_to(source)):p.read_bytes() for p in source.rglob('*') if p.is_file()}
 offline=run();assert offline['mode']=='dry-run' and offline['counts']==dict(clans=2,members=3,claims=1)
 run(db);assert sql('SELECT COUNT(*) FROM clan_progression_clans')=='0'
 # Corrupt/mismatched sources are reported before any write.
 index=(source/'index').read_text();(source/'index').write_text(index.replace('123456 7','123456 9',1));bad=run(db,ok=False);assert 'Membership disagreement' in bad['error'];(source/'index').write_text(index)
 (source/'clan_investments').write_text('1 100 123456 5\n$\n');bad=run(db,ok=False);assert 'no owning clan' in bad['error'];(source/'clan_investments').unlink()
 raw=(source/'clans').read_text();(source/'clans').write_text(raw.replace('Bank: 9999999999','Bank: -1',1));run(db,ok=False);(source/'clans').write_text(raw)
 assert sql('SELECT COUNT(*) FROM clan_progression_clans')=='0'
 # Complete import, durable backups, exact source preservation and zero new XP.
 applied=run([*db,'--apply','--writers-quiesced','--backup-dir',str(backup)]);assert applied['imported'] and not applied['differences']
 assert sql('SELECT COUNT(*) FROM clan_memberships')=='3';assert sql('SELECT SUM(xp_eighths),SUM(influence_eighths) FROM clan_progression_state')=='0\t0'
 assert sql('SELECT COUNT(*) FROM clan_memberships WHERE rank_id=0')=='1'
 mapping_by_uid={uid:int(vnum) for uid,vnum in (line.split('\t') for line in sql('SELECT clan_uid,clan_vnum FROM clan_progression_clans').splitlines())}
 popularity=json.loads(sql('SELECT popularity FROM clan_zone_claims WHERE zone_vnum=100'))
 assert {mapping_by_uid[p['clan_uid']]:p['percentage'] for p in popularity}=={7:17.25,9:82.75}
 assert sql('SELECT GROUP_CONCAT(legacy_points ORDER BY player_id) FROM clan_memberships')=='101,202,303'
 archived=__import__('pathlib').Path(applied['backup']);assert stat.S_IMODE(archived.stat().st_mode)==0o700
 manifest=json.loads((archived/'manifest.json').read_text());assert manifest['digest']==applied['source_digest']
 for role,digest in manifest['files'].items():assert hashlib.sha256((archived/role).read_bytes()).hexdigest()==digest
 assert before_files=={str(p.relative_to(source)):p.read_bytes() for p in source.rglob('*') if p.is_file()}
 first=sql('SELECT * FROM clan_import_manifests');revision=sql('SELECT revision FROM clan_store_control')
 again=run([*db,'--apply','--writers-quiesced','--backup-dir',str(backup)]);assert again['already_imported'] and again['import_uid']==applied['import_uid']
 assert sql('SELECT * FROM clan_import_manifests')==first and sql('SELECT revision FROM clan_store_control')==revision
 (source/'clans').write_text(raw.replace('Bank: 9999999999','Bank: 888888',1));run([*db,'--apply','--writers-quiesced','--backup-dir',str(backup)],ok=False);(source/'clans').write_text(raw)
 assert sql('SELECT COUNT(*) FROM clan_import_manifests')=='1'
 # Newer balances and a deleted incarnation must never be overwritten/recreated.
 sql("UPDATE clan_progression_clans SET treasury_gold=123456 WHERE clan_vnum=7; UPDATE clan_progression_state SET influence_eighths=999;")
 report=run(db);assert report['differences']==['clans']
 run([*db,'--apply','--writers-quiesced','--backup-dir',str(backup)],ok=False)
 assert sql('SELECT treasury_gold FROM clan_progression_clans WHERE clan_vnum=7')=='123456'
 sql('UPDATE clan_import_manifests SET import_status=2;')
 run([*db,'--apply','--writers-quiesced','--backup-dir',str(backup)],ok=False)
 now=int(time.time());sql(f'UPDATE clan_progression_clans SET record_status=2,deleted_at_utc={now},updated_at_utc={now} WHERE clan_vnum=7; DELETE FROM clan_memberships WHERE clan_uid=(SELECT clan_uid FROM clan_progression_clans WHERE clan_vnum=7); DELETE FROM clan_zone_claims;')
 # Remaining JSON relationships still refer to retained old UID; read rejects the
 # inconsistent active projection instead of silently restoring old clans.
 run([*db,'--apply','--writers-quiesced','--backup-dir',str(backup)],ok=False)
 assert sql('SELECT record_status FROM clan_progression_clans WHERE clan_vnum=7')=='2'
 print('Legacy clan importer: offline/SQL dry runs; complete fields, statistics, ranks/privileges, relationships, claims/popularity, offline members/applicants and points; disagreements/unknown investments; private verified backups; atomic native import; repeat receipts; newer balances/cutover/deletion protection passed.')
