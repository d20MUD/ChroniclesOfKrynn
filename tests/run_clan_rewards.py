#!/usr/bin/env python3
"""Phase 8: real C/SQL transactions, process crashes, concurrent claims and bounds."""
from clan_database import ROOT,database
from clan_snapshot_fixture import snapshot,manifest,A,B
import subprocess,shlex,json,sys,copy
sys.path.insert(0,str(ROOT/'scripts'))
from build_clan_repository_tool import build
migrations=tuple(f'20261110_clan_progression_v{i:03d}.sql' for i in range(1,6))
with database(migrations) as (tmp,socket,sql,client):
 tool=tmp/'tool';build(tool,sanitize=True)
 subprocess.run([str(tool),'import','clan_test','root','',str(socket)],input=json.dumps(dict(snapshot=snapshot(),expected_revision=0,manifest=manifest())),text=True,check=True,stdout=subprocess.DEVNULL)
 sql('UPDATE clan_store_control SET store_mode=2; UPDATE clan_import_manifests SET import_status=2;')
 binary=tmp/'rewards';flags=shlex.split(subprocess.check_output(['pkg-config','--cflags','--libs','mariadb','openssl','json-c'],text=True))
 subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-pedantic','-g','-DCLAN_REWARD_TEST','-fsanitize=address,undefined','-fno-pie','-no-pie','-I'+str(ROOT/'src'),str(ROOT/'tests/clan_rewards.c'),str(ROOT/'src/clan_repository.c'),str(ROOT/'src/clan_awards.c'),str(ROOT/'src/clan_progression.c'),*flags,'-o',str(binary)],check=True)
 seq=100
 def event(**changes):
  global seq
  seq+=1
  return dict(operation_uid=f'{seq:032x}',event_uid=f'{seq+100000:032x}',clan_uid=A,account_id=77,player_id=5000000007,source=1,rules_version=1,occurred_at=2200000001,now=2200000002,requested_xp_eighths=16,requested_influence_eighths=8,requested_favor_eighths=8,**{})|changes
 delta=dict(xp_eighths=16,influence_eighths=8,favor_eighths=8,account_base_xp_eighths=16,account_claims=1,incoming_xp_eighths=16,cap_flags=0)
 def request(e,**changes):return dict(operation_uid=e['operation_uid'],now=e['now'],delta=delta)|changes
 def run(op,body,result='ok',crash=None):
  p=subprocess.run([str(binary),str(socket),op],input=json.dumps(body),text=True,capture_output=True)
  if crash is not None:
   assert p.returncode==90+crash,(op,p.returncode,p.stdout,p.stderr);return
  assert p.returncode==0,(op,p.returncode,p.stderr)
  out=json.loads(p.stdout);assert out['result']==result,(op,out,p.stderr)
  if result!='ok':assert out['receipt']=={'untouched':True},out
  return out
 def balances():return sql('SELECT * FROM clan_progression_state ORDER BY clan_uid; SELECT * FROM clan_personal_favor ORDER BY player_id,clan_uid; SELECT * FROM clan_account_windows ORDER BY account_id,starts_at_utc; SELECT * FROM clan_weekly_windows ORDER BY clan_uid,starts_at_utc; SELECT * FROM clan_reward_receipts ORDER BY operation_uid;')
 first=event();prepared=run('prepare',first);assert prepared['receipt']['status']==0
 assert run('prepare',first)['receipt']==prepared['receipt']
 alias=event(event_uid=first['event_uid']);assert run('prepare',alias)['receipt']==prepared['receipt']
 run('prepare',dict(alias,clan_uid=B),'conflict');run('prepare',dict(alias,requested_xp_eighths=17),'conflict')
 before=balances()
 for point in range(2,8):
  run('complete',request(first,fault=point),'unavailable');assert balances()==before,point
 done=run('complete',request(first));assert done['calls']==1 and done['receipt']['status']==1
 proof=done['receipt']['decision'];assert proof['after']['clan_xp']==16 and proof['after']['account_base_xp']==16 and proof['after']['favor']==8
 assert run('complete',request(first,reject_callback=True,now=first['now']+604800))['receipt']==done['receipt']
 assert run('complete',request(first,reject_callback=True))['calls']==0
 run('cancel',dict(operation_uid=first['operation_uid'],reason=1,now=first['now']),'conflict')
 run('prepare',dict(first,operation_uid='f'*32))
 # An uncertain acknowledgement reconciles the exact durable original receipt.
 unknown=event();run('prepare',dict(unknown,fault=1),'commit-unknown')
 assert run('load',request(unknown))['receipt']['status']==0
 run('complete',request(unknown,fault=8),'commit-unknown')
 prior=balances();assert run('complete',request(unknown,reject_callback=True))['calls']==0 and balances()==prior
 # Simulate actual process death, not merely a failed function return.
 for point in range(1,9):
  e=event()
  if point==1:run('prepare',dict(e,crash=1),crash=1)
  else:run('prepare',e)
  before_xp=int(sql(f"SELECT xp_eighths FROM clan_progression_state WHERE clan_uid='{A}'"))
  if point!=1:run('complete',request(e,crash=point),crash=point)
  recovered=run('complete',request(e));assert recovered['receipt']['status']==1
  assert int(sql(f"SELECT xp_eighths FROM clan_progression_state WHERE clan_uid='{A}'"))==before_xp+16
  assert run('complete',request(e,reject_callback=True))['calls']==0
 # A recovery after reset preserves the accepted occurrence week and audit.
 delayed=event();run('prepare',delayed)
 old_week=run('load',request(delayed))['receipt']['event']['starts_at']
 recovered=run('complete',request(delayed,now=delayed['now']+604800))
 assert recovered['receipt']['event']['starts_at']==old_week
 assert sql(f'SELECT COUNT(*) FROM clan_account_windows WHERE account_id=77 AND starts_at_utc={old_week+604800}')=='0'
 # Same encounter can credit a different source without duplicate ordinary kill.
 other=event(event_uid=first['event_uid'],source=2);run('prepare',other)
 run('prepare',dict(other,operation_uid=first['operation_uid']),'conflict')
 run('complete',request(other,now=delayed['now']+604800))
 shared=event(source=7,account_id=0,player_id=0,clan_uid=B,requested_favor_eighths=0);run('prepare',shared)
 shared_delta=dict(delta,favor_eighths=0,account_base_xp_eighths=0,account_claims=0)
 assert run('complete',request(shared,delta=shared_delta))['receipt']['decision']['after']['account_revision']==-1
 # Empty/capped automatic outcomes still get a terminal deduplication receipt.
 zero=event();run('prepare',zero)
 zero_delta={k:0 for k in delta};run('complete',request(zero,delta=zero_delta,now=delayed['now']+604800));assert run('complete',request(zero,reject_callback=True))['calls']==0
 # Invalid decisions and overflows roll back ALL staged counters/balances.
 bad=event();run('prepare',bad);before=balances()
 for d in [dict(delta,xp_eighths=17),dict(delta,favor_eighths=-1),dict(delta,account_claims=2),dict(delta,incoming_xp_eighths=15)]:
  run('complete',request(bad,delta=d,now=delayed['now']+604800),'invalid');assert balances()==before
 overflow=event(clan_uid=B,account_id=88,player_id=5000000009);run('prepare',overflow)
 sql(f"UPDATE clan_progression_state SET xp_eighths=9223372036854775807,revision=revision+1 WHERE clan_uid='{B}'")
 before=balances();run('complete',request(overflow),'invalid');assert balances()==before
 sql(f"UPDATE clan_progression_state SET xp_eighths=16,revision=revision+1 WHERE clan_uid='{B}'")
 # Cancellation retains facts/event key, releases pending capacity, and is final.
 canceled=run('cancel',dict(operation_uid=bad['operation_uid'],reason=7,now=delayed['now']+604800))['receipt'];assert canceled['status']==2
 assert run('cancel',dict(operation_uid=bad['operation_uid'],reason=7,now=delayed['now']+604800))['receipt']==canceled
 run('complete',request(bad),'conflict');assert run('prepare',bad)['receipt']==canceled
 # Queue discovery after restart: bounded, filterable and cursor based.
 pending=run('pending',dict(player_id=0,limit=100))['receipt'];assert overflow['operation_uid'] in pending
 assert run('pending',dict(player_id=0,limit=100,cursor=pending[-1]))['receipt']==[]
 run('pending',dict(player_id=0,limit=0),'invalid')
 # Concurrent identical claims call policy once; different claims share one cap row.
 race=event(clan_uid=B,account_id=99,player_id=5000000010);run('prepare',race)
 def concurrent_complete(events):
  processes=[]
  for e in events:
   p=subprocess.Popen([str(binary),str(socket),'complete'],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True);p.stdin.write(json.dumps(request(e)));p.stdin.close();processes.append(p)
  outputs=[]
  for p in processes:
   out=p.stdout.read();err=p.stderr.read();assert p.wait()==0,(out,err);out=json.loads(out);assert out['result']=='ok',out;outputs.append(out)
  return outputs
 outcomes=concurrent_complete([race,race]);assert sum(x['calls'] for x in outcomes)==1
 races=[event(clan_uid=B,account_id=99,player_id=5000000010) for _ in range(2)]
 for e in races:run('prepare',e)
 concurrent_complete(races);assert sql('SELECT base_xp_eighths FROM clan_account_windows WHERE account_id=99')=='48'
 # Phase 9 service uses actual locked balances, caps and immutable receipts.
 sql(f"UPDATE clan_progression_state SET xp_eighths=303992,influence_eighths=159960,revision=revision+1 WHERE clan_uid='{B}'")
 policy_event=event(clan_uid=B,source=7,account_id=0,player_id=0,requested_xp_eighths=1600,requested_influence_eighths=800,requested_favor_eighths=0)
 run('prepare',policy_event)
 run('policy',request(policy_event,disabled=True),'invalid')
 run('policy',request(policy_event,unready=True),'invalid')
 before=balances();run('policy',request(policy_event,fault=3),'unavailable');assert balances()==before
 award=run('policy',request(policy_event))['receipt'];d=award['decision']['delta']
 assert d['xp_eighths']==8 and d['influence_eighths']==40 and d['cap_flags']==10,d
 assert award['decision']['after']['clan_xp']==304000 and award['decision']['after']['clan_influence']==160000
 assert run('policy',request(policy_event,disabled=True))['receipt']==award
 personal_policy=event(clan_uid=B);run('prepare',personal_policy);run('policy',request(personal_policy),'invalid')
 wrong_tuple=event(clan_uid=B,source=7,account_id=0,player_id=0,requested_favor_eighths=0)
 run('prepare',wrong_tuple);run('policy',request(wrong_tuple),'invalid')
 # A capped clan still records a zero payout and its original incoming XP.
 capped=event(clan_uid=B,source=7,account_id=0,player_id=0,requested_xp_eighths=1600,requested_influence_eighths=800,requested_favor_eighths=0)
 run('prepare',capped);out=run('policy',request(capped))['receipt']['decision']
 assert out['delta']['xp_eighths']==out['delta']['influence_eighths']==0
 assert out['delta']['incoming_xp_eighths']==1600
 # Receipt replay survives membership changes and a retained deleted incarnation.
 tombstone=event();run('prepare',tombstone)
 sql(f"UPDATE clan_progression_clans SET record_status=2,deleted_at_utc=2200604802,updated_at_utc=2200604802 WHERE clan_uid='{A}'")
 assert run('complete',request(first,reject_callback=True))['receipt']==done['receipt']
 run('complete',request(tombstone,now=2200604802),'deleted')
 run('cancel',dict(operation_uid=tombstone['operation_uid'],reason=8,now=2200604802))
 # Private fixture seeds capacity; no production table/config is used.
 capacity_events=[event(clan_uid=B,player_id=1234,account_id=1234) for _ in range(101)]
 # Seed valid rows using SQL to avoid 100 repeated schema probes/process startups.
 def seed(events):
  values=[]
  for e in events:
   week=e['occurred_at']-((e['occurred_at']%604800+259200)%604800)
   beneficiary=str(e['player_id']) if e['player_id'] else e['clan_uid']
   values.append(f"('{e['operation_uid']}','{e['event_uid']}','{e['clan_uid']}','{beneficiary}',{e['account_id']},{e['player_id']},{e['source']},{e['occurred_at']},{week},1,16,8,{e['requested_favor_eighths']},{e['now']})")
  sql('INSERT INTO clan_reward_receipts(operation_uid,event_uid,clan_uid,beneficiary_key,account_id,player_id,source_id,occurred_at_utc,starts_at_utc,rules_version,requested_xp_eighths,requested_influence_eighths,requested_favor_eighths,accepted_at_utc) VALUES'+','.join(values))
 seed(capacity_events[:100]);run('prepare',capacity_events[-1],'capacity')
 assert len(run('pending',dict(player_id=1234,limit=1000))['receipt'])==100
 run('cancel',dict(operation_uid=capacity_events[0]['operation_uid'],reason=9,now=capacity_events[0]['now']));run('prepare',capacity_events[-1])
 count=int(sql('SELECT COUNT(*) FROM clan_reward_receipts WHERE receipt_status=0'))
 for offset in range(0,10000-count,500):
  seed([event(clan_uid=B,source=7,player_id=0,account_id=0,requested_favor_eighths=0) for _ in range(min(500,10000-count-offset))])
 assert sql('SELECT COUNT(*) FROM clan_reward_receipts WHERE receipt_status=0')=='10000'
 run('prepare',event(clan_uid=B),'capacity');assert run('prepare',capacity_events[-1])['receipt']['status']==0
 # History and terminal proof cannot be edited/deleted, including by SQL callers.
 for mutation in ["DELETE FROM clan_reward_receipts",f"UPDATE clan_reward_receipts SET decision=JSON_OBJECT('forged',1) WHERE operation_uid='{first['operation_uid']}'",f"UPDATE clan_reward_receipts SET event_uid='{'1'*32}' WHERE operation_uid='{overflow['operation_uid']}'","DELETE FROM clan_weekly_windows"]:
  assert subprocess.run(client,input=mutation,text=True,capture_output=True).returncode!=0,mutation
 before=balances();subprocess.run(client,input=(ROOT/'sql'/'20261113_clan_progression_v006.sql').read_text(),text=True,check=True);assert balances()==before
 sql('DROP TRIGGER cp_reward_immutable');run('load',request(first),'schema-mismatch')
 print('Phases 8–9: clan XP/Influence policy, gated and capped receipts, durable acceptance/bounds, SQL atomicity, eight process crash boundaries, exact replay, reset recovery, concurrent claims, canceled/deleted history, overflow and migration guards passed')
