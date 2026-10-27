#!/usr/bin/env python3
"""Load COPIES of live flat snapshots through real server code, isolated SQL only.
No passwords, names, network endpoints or player file content are exported.
Native boot diagnostics stay in a disposable directory.
"""
from pathlib import Path
import argparse,json,os,re,shlex,shutil,subprocess,tempfile,time
ROOT=Path(__file__).resolve().parents[1]
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--blueprints',action='store_true');p.add_argument('--geared',action='store_true',help='Upgrade only the provisional ranged weapon; does not validate a representative endgame role build');p.add_argument('--role',choices=['dual_wielder','ranged_attacker'],default='dual_wielder');p.add_argument('--candidate',default='candidate_012',help='Anonymous snapshot ID for --probe');p.add_argument('--probe',action='store_true',help='Run native stationary-target counterexample probe, not full release suite');p.add_argument('--debug',action='store_true');p.add_argument('--lib',type=Path,default=Path('/home/krynn/live/mud/lib'));p.add_argument('--build',type=Path,default=Path('/tmp/krynn-legendary-phase8-cmake'));p.add_argument('--output',type=Path,default=None);args=p.parse_args()
    if args.output is None:args.output=ROOT/'docs/systems/legendary_balance_results'/('native_blueprints.json' if args.blueprints else 'native_gate_probe.json' if args.probe else 'native_snapshots.json')
    with tempfile.TemporaryDirectory(prefix='legendary-native-') as temp:
        temp=Path(temp);sandbox=temp/'lib'
        for directory in ['world','etc','text','misc','house','plrfiles','plrobjs']:
            shutil.copytree(args.lib/directory,sandbox/directory)
        # No live credentials are copied. Link wrappers additionally force ALL
        # native connection attempts to this disposable Unix-socket database.
        (sandbox/'mysql_config').write_text('mysql_host = localhost\nmysql_username = root\nmysql_database = estate_test\nmysql_password = none\n')
        candidates=[]
        for path in sorted((sandbox/'plrfiles').glob('*/*.plr')):
            text=path.read_text(errors='replace');level=re.search(r'^Levl:\s*(\d+)',text,re.M)
            if level and int(level[1])==30:candidates.append((f'candidate_{len(candidates)+1:03d}',path.stem))
        if args.probe:
            candidates=[v for v in candidates if v[0]==args.candidate]
            if not candidates:raise RuntimeError('Missing selected dual-wielder snapshot')
        manifest=temp/'candidates';manifest.write_text(''.join(f'{label} {name}\n' for label,name in candidates))
        # Reset only copied object timestamps, retaining the snapshot's gear.
        for path in (sandbox/'plrobjs').glob('*/*.objs'):
            text=path.read_text(errors='replace');line,separator,tail=text.partition('\n');parts=line.split()
            if len(parts)==6 and all(v.lstrip('-').isdigit() for v in parts):
                parts[1]=str(int(time.time()));path.write_text(' '.join(parts)+separator+tail)
        flags=(args.build/'CMakeFiles/circle.dir/flags.make').read_text();definitions=shlex.split(re.search(r'^C_DEFINES = (.*)',flags,re.M)[1]);includes=shlex.split(re.search(r'^C_INCLUDES = (.*)',flags,re.M)[1]);obj=temp/'capture.o';exe=temp/'capture'
        subprocess.run(['cc','-std=gnu99',*definitions,*includes,'-c',str(ROOT/('tests/legendary_native_blueprints.c' if args.blueprints else 'tests/legendary_native_probe.c' if args.probe else 'tests/legendary_native_capture.c')),'-o',str(obj)],check=True)
        command=shlex.split((args.build/'CMakeFiles/circle.dir/link.txt').read_text());position=command.index('-o');command[position+1]=str(exe);command.extend([str(obj),'-Wl,--wrap=main','-Wl,--wrap=mysql_real_connect','-Wl,--wrap=curl_easy_perform','-Wl,--wrap=socket'])
        if args.probe:
            fight=temp/'probe-fight.o'
            subprocess.run(['cc','-std=gnu99',*definitions,*includes,'-DLEGENDARY_NATIVE_PROBE','-c',str(ROOT/'src/fight.c'),'-o',str(fight)],check=True)
            command=[str(fight) if v.endswith('src/fight.c.o') else v for v in command]
            command.append('-Wl,--wrap=time')
        subprocess.run(command,cwd=args.build,check=True)
        data=temp/'data';data.mkdir();socket=temp/'mysql.sock'
        subprocess.run(['mariadb-install-db','--no-defaults','--datadir='+str(data),'--auth-root-authentication-method=normal','--skip-test-db'],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL,check=True)
        server=subprocess.Popen(['mariadbd','--no-defaults','--datadir='+str(data),'--socket='+str(socket),'--pid-file='+str(temp/'mysql.pid'),'--skip-networking','--user='+os.environ.get('USER','krynn')],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
        try:
            client=['mariadb','--no-defaults','--socket='+str(socket),'-uroot']
            for _ in range(100):
                if subprocess.run([*client,'-e','SELECT 1'],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL).returncode==0:break
                time.sleep(.1)
            else:raise RuntimeError('Sandbox SQL unavailable')
            subprocess.run([*client,'-e','CREATE DATABASE estate_test'],check=True)
            migration=''.join((ROOT/'sql'/name).read_text() for name in ['20261023_legendary_objects.sql','20261024_legendary_transactions.sql','20261024_legendary_earnings.sql','20261024_legendary_smiths.sql','20261024_legendary_effects.sql','20261025_legendary_item_passives.sql','20261025_legendary_release_controls.sql','20261026_legendary_mob_bosses.sql','20261026_legendary_smith_specproc.sql','20261026_legendary_daily_limits.sql'])
            subprocess.run([*client,'estate_test'],input=migration,text=True,check=True)
            captured=[]
            runs=[(None,None)] if not args.probe else [(seconds,mode) for seconds in (30,600) for mode in ('baseline','control','legendary')]
            if args.probe:
                for directory in ('plrfiles','plrobjs'):shutil.copytree(sandbox/directory,temp/('pristine-'+directory))
            for seconds,mode in runs:
                if args.probe:
                    for directory in ('plrfiles','plrobjs'):shutil.copytree(temp/('pristine-'+directory),sandbox/directory,dirs_exist_ok=True)
                env=dict(os.environ,LEGENDARY_SANDBOX_SOCKET=str(socket));raw=temp/'profiles.jsonl'
                if args.probe:env.update(LEGENDARY_PROBE_MODE=mode,LEGENDARY_PROBE_SECONDS=str(seconds),LEGENDARY_PROBE_ROLE=args.role,LEGENDARY_PROBE_GEARED=str(int(args.geared)))
                with (temp/'boot.log').open('w') as log:
                    run_command=[str(exe),str(manifest),str(raw)]
                    if args.debug:run_command=['gdb','--batch','-ex','run','-ex','bt','--args',*run_command]
                    result=subprocess.run(run_command,cwd=sandbox,env=env,stdout=log,stderr=log,timeout=150)
                if (not args.debug and result.returncode) or (args.debug and 'exited normally' not in (temp/'boot.log').read_text(errors='replace')):
                    lines=(temp/'boot.log').read_text(errors='replace').splitlines()
                    for _,name in candidates:lines=[re.sub(r'(?i)\b'+re.escape(name)+r'\b','<snapshot>',line) for line in lines]
                    raise RuntimeError('Native sandbox capture failed (exit '+str(result.returncode)+'): '+ '\n'.join(lines[-14:]))
                case_rows=[json.loads(line) for line in raw.read_text().splitlines()]
                captured.extend(case_rows)
                if args.probe:print(json.dumps(case_rows[0]),flush=True)
            result={'provenance':'Real native engine loading copied level-30 flat snapshots; anonymized; not live SQL character state','validation_status':'Diagnostic native snapshots only; repeated same-seed controls and Dark Revelation display/attack behavior verified; not native encounter balance validation','profiles':captured}
            if args.blueprints:
                result['provenance']='Native advancement and premade base builds with generated common grade-six gear in a copied world; these builds do not load player archives'
                result['validation_status']='Draft native level-30 role blueprints; gear generation succeeds, but feat/rotation audit and encounter validation are incomplete'
            if args.probe:
                result['validation_status']='Native stationary-target outgoing gate probe; incoming retaliation/major rotations/seven-role suite are not modeled. Native damage, crits, mitigation, procs, timers and target death are executed.'
                result['comparisons']=[]
                for seconds in (30,600):
                    base,control,aug=[next(v for v in captured if v['seconds']==seconds and v['mode']==mode) for mode in ('baseline','control','legendary')]
                    if base['actual_hp_damage']!=control['actual_hp_damage']:raise RuntimeError('Independent native controls differ')
                    pct=100*(aug['actual_hp_damage']-base['actual_hp_damage'])/base['actual_hp_damage'] if base['actual_hp_damage'] else None
                    result['comparisons'].append({'seconds':seconds,'baseline_damage':base['actual_hp_damage'],'legendary_damage':aug['actual_hp_damage'],'outgoing_increase_percent':pct,'within_five_percent':pct is not None and pct<=5})
            args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(result,indent=2)+'\n')
            print('Captured native anonymous cases:',len(captured));print(args.output)
            if args.probe:print(json.dumps(result['comparisons'],indent=2))
        finally:server.terminate();server.wait(timeout=15)
if __name__=='__main__':main()
