from pathlib import Path
import re
ROOT=Path(__file__).resolve().parents[1]
A,B='a'*32,'b'*32
FIELDS=re.findall(r'X\((\w+),(\w+),(\w+)\)',(ROOT/'src/clan_legacy_fields.h').read_text())
def clan(uid,vnum):
 core=dict(uid=uid,name="Clan '"+str(vnum),description='A colorful @[F510]clan@n\n\nSecond paragraph',abbreviation='CLAN',vnum=vnum,status=1,leader_id=5000000000+vnum,treasury_gold=9999999999,founded_at=2000000000,last_activity_at=2100000000,created_at=2200000000,updated_at=2200000000,deleted_at=0,revision=0,xp_eighths=0,influence_eighths=0,state_revision=0,state_updated_at=2200000000)
 legacy={name:i+1 for i,(name,member,tag) in enumerate(FIELDS)}
 legacy.update(ranks=['Leader','Member'],privileges=[dict(id=i,rank=[0,1,2,254,255][i%5]) for i in range(21)],relationships=[dict(peer_uid=B if uid==A else A,allied=True,at_war=False)])
 legacy['total_deposits']=5000000000
 return dict(core=core,legacy=legacy)
def snapshot():
 return dict(clans=[clan(A,7),clan(B,9)],members=[dict(player_id=5000000007,clan_uid=A,rank_id=1,legacy_points=101),dict(player_id=5000000009,clan_uid=B,rank_id=0,legacy_points=202)],claims=[dict(zone_vnum=100,controller_uid=A,claimant_id=5000000007,popularity=[dict(clan_uid=A,percentage=17.25),dict(clan_uid=B,percentage=82.75)])],written_at=2200000000)
def manifest():return dict(source_digest='d'*64,import_uid='e'*32,imported_at=2200000000,source_manifest=dict(mapping={'7':A,'9':B},files=[]))
