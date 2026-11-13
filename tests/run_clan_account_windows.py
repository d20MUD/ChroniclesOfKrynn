#!/usr/bin/env python3
"""Phase 7: actual checked C repository, UTC oracle, disposable SQL only."""
from clan_database import ROOT,database
import subprocess,shlex,datetime,os,random
migrations=tuple(f'20261110_clan_progression_v{i:03d}.sql' for i in range(1,6))
with database(migrations) as (tmp,socket,sql,client):
 binary=tmp/'account_windows'
 flags=shlex.split(subprocess.check_output(['pkg-config','--cflags','--libs','mariadb','openssl','json-c'],text=True))
 subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-pedantic','-g','-fsanitize=address,undefined','-fno-pie','-no-pie','-I'+str(ROOT/'src'),str(ROOT/'tests/clan_account_windows.c'),str(ROOT/'src/clan_repository.c'),str(ROOT/'src/clan_progression.c'),*flags,'-o',str(binary)],check=True)
 rng=random.Random(7)
 times={0,1,345599,345600,345601,604799,604800,1704067199,1704067200,1704067201}
 for day in ['2024-02-29','2024-03-10','2024-11-03','2026-11-09','2038-01-19','2100-03-01']:
  t=int(datetime.datetime.fromisoformat(day).replace(tzinfo=datetime.timezone.utc).timestamp());times.update([t-1,t,t+1])
 times.update(rng.randrange(0,4102444800) for _ in range(1000))
 lines=[]
 for t in sorted(times):
  dt=datetime.datetime.fromtimestamp(t,datetime.timezone.utc)
  midnight=dt.replace(hour=0,minute=0,second=0,microsecond=0)
  monday=midnight-datetime.timedelta(days=dt.weekday())
  lines.append(f'{t} {int(monday.timestamp())} {int(midnight.timestamp())}')
 # Include representable int64 periods without relying on datetime/time_t limits.
 last_week=((2**63-1-604800+259200)//604800)*604800-259200
 for t in [last_week,last_week+1,last_week+604799]:lines.append(f'{t} {last_week} {t//86400*86400}')
 for tz in ['UTC','America/Halifax','Pacific/Auckland']:
  subprocess.run([str(binary),'periods'],input='\n'.join(lines)+'\n',text=True,check=True,env=dict(os.environ,TZ=tz))
 subprocess.run([str(binary),str(socket)],check=True)
 before=sql('SELECT * FROM clan_account_windows ORDER BY account_id,starts_at_utc')
 subprocess.run(client,input=(ROOT/'sql'/'20261113_clan_progression_v006.sql').read_text(),text=True,check=True)
 assert sql('SELECT * FROM clan_account_windows ORDER BY account_id,starts_at_utc')==before
 # Actual SQL constraints enforce aligned UTC weeks and bounded values.
 for mutation in ["INSERT INTO clan_account_windows(account_id,starts_at_utc,updated_at_utc) VALUES(0,1704067200,1704067200)",
                  "INSERT INTO clan_account_windows(account_id,starts_at_utc,updated_at_utc) VALUES(10,1704067201,1704067201)",
                  "INSERT INTO clan_account_windows(account_id,starts_at_utc,base_xp_eighths,updated_at_utc) VALUES(10,1704067200,-1,1704067200)",
                  "INSERT INTO clan_account_windows(account_id,starts_at_utc,kill_claims,updated_at_utc) VALUES(10,1704067200,9223372036854775808,1704067200)"]:
  assert subprocess.run(client,input=mutation,text=True,capture_output=True).returncode!=0,mutation
 # Schema receipt alone cannot mask a missing retention guard.
 sql('DROP TRIGGER cp_account_window_retained')
 # Test via the bridge's existing checked read, avoiding a duplicate schema checker.
 import sys,json
 sys.path.insert(0,str(ROOT/'scripts'))
 from build_clan_repository_tool import build
 tool=tmp/'tool';build(tool,sanitize=True)
 out=subprocess.run([str(tool),'read','clan_test','root','',str(socket)],input='{}',text=True,capture_output=True)
 assert json.loads(out.stdout)['result']=='schema-mismatch',out
 print('Phase 7: deterministic UTC resets, shared account counters, SQL history/CAS protections, int64, migration rerun and schema drift passed')
