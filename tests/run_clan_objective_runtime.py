#!/usr/bin/env python3
"""Actual game harvest handoff and combat collector, disposable SQL and saved markers."""
from clan_database import ROOT,database
from clan_snapshot_fixture import snapshot,manifest,A
import subprocess,shlex,json,sys
sys.path.insert(0,str(ROOT/'scripts'))
from build_clan_repository_tool import build
migrations=tuple(f'20261110_clan_progression_v{i:03d}.sql' for i in range(1,6))+('20261111_clan_kill_sources_v001.sql','20261111_clan_objectives_v001.sql')
with database(migrations) as (tmp,socket,sql,client):
    tool=tmp/'tool';build(tool,sanitize=True)
    subprocess.run([str(tool),'import','clan_test','root','',str(socket)],input=json.dumps(dict(snapshot=snapshot(),expected_revision=0,manifest=manifest())),text=True,check=True,stdout=subprocess.DEVNULL)
    sql(f"UPDATE clan_store_control SET store_mode=2;UPDATE clan_import_manifests SET import_status=2;INSERT INTO clan_memberships(player_id,clan_uid,rank_id,legacy_points) VALUES(5000000012,'{A}',2,0)")
    binary=tmp/'runtime';flags=shlex.split(subprocess.check_output(['pkg-config','--cflags','--libs','mariadb','openssl','json-c'],text=True))
    subprocess.run(['cc','-std=gnu99','-g','-Wall','-Wextra','-Wno-unused-parameter','-ffunction-sections','-fdata-sections','-Wl,--gc-sections','-DCLAN_REWARD_TEST','-Dtime=objective_test_time','-fsanitize=address,undefined','-fno-pie','-no-pie','-Isrc',*[str(ROOT/p) for p in ['tests/clan_objective_runtime.c','src/clan_objective_runtime.c','src/clan_objectives.c','src/clan_sources.c','src/clan_combat.c','src/clan_repository.c','src/clan_awards.c','src/clan_progression.c','src/clan_expedition_enrollment.c','src/clan_expeditions.c']],*flags,'-o',str(binary)],cwd=ROOT,check=True)
    seq=0
    def run(mode,path,crash_save=0,sql_crash=0,crash=None):
        now=2200000001+seq*604800
        p=subprocess.run([str(binary),str(socket),str(path),mode,str(now),str(crash_save),str(sql_crash)],text=True,capture_output=True,cwd=ROOT/'lib')
        if crash is not None:
            assert p.returncode==crash,(mode,p.returncode,p.stdout,p.stderr);return
        assert p.returncode==0,(mode,p.returncode,p.stdout,p.stderr)
        return json.loads(p.stdout)
    def clean_active():sql('UPDATE clan_objective_contracts SET state=3 WHERE state=0')
    prior=0
    selected=sys.argv[1:]
    modes=selected or ['commands','gather','gather-failed-save','damage','support','heal','tank','alt','owned','trivial','wrong-target','disabled']
    for mode in modes:
        seq+=1;path=tmp/mode;result=run(mode,path)
        completed=mode in ['gather','damage','support','heal','tank','alt']
        target=20 if completed else 4 if mode in ('gather-failed-save','commands') else 0
        total=int(sql('SELECT COALESCE(SUM(progress),0) FROM clan_objective_contracts'))
        assert total-prior==target,(mode,result,total,prior);prior=total
        if mode=='gather':assert result['materials']==200
        clean_active()
    for boundary in ([] if selected else ['saved-before-progress','sql-progress-committed']):
        seq+=1;path=tmp/boundary
        run('gather-once',path,crash_save=1 if boundary=='saved-before-progress' else 0,sql_crash=2 if boundary=='sql-progress-committed' else 0,crash=88 if boundary=='saved-before-progress' else 92)
        result=run('recover',path);assert result['materials']==40
        total=int(sql('SELECT COALESCE(SUM(progress),0) FROM clan_objective_contracts'));assert total-prior==4;prior=total
        assert run('recover',path)==result
        clean_active()
    if not selected:
        seq+=1;run('gather-schema',tmp/'gather-schema')
    print('Phases 30-31 game adapters: base-only harvest vs bonus yield, failed-save replay, reconnect before/after SQL commit, damage/support/heal/tank bounties, same-account alt dedup, exclusions and disabled gate passed.')
