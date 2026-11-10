#!/usr/bin/env python3
from clan_database import ROOT,database
from clan_snapshot_fixture import snapshot,manifest
import sys,json,subprocess,shlex,time
sys.path.insert(0,str(ROOT/'scripts'))
from build_clan_repository_tool import build
with database(('20261110_clan_progression_v001.sql','20261110_clan_progression_v002.sql','20261110_clan_progression_v003.sql','20261110_clan_progression_v004.sql','20261110_clan_progression_v005.sql')) as (tmp,socket,sql,client):
 tool=tmp/'tool';build(tool,sanitize=True);initial=snapshot();at=int(time.time())-100
 for entry in initial['clans']:
  for k in ['created_at','updated_at','state_updated_at']:entry['core'][k]=at
 initial['written_at']=at
 initial['claims'].append(dict(zone_vnum=101,controller_uid='b'*32,claimant_id=0,popularity=[]))
 initial['claims'][0]['popularity'].reverse()
 subprocess.run([str(tool),'import','clan_test','root','',str(socket)],input=json.dumps(dict(snapshot=initial,expected_revision=0,manifest=manifest())),text=True,check=True,stdout=subprocess.DEVNULL)
 sql('UPDATE clan_store_control SET store_mode=2; UPDATE clan_import_manifests SET import_status=2;')
 source=(ROOT/'src/clan_edit.c').read_text();extracted=''
 for signature in ['bool save_clans(void)','bool save_single_clan(clan_rnum c)','bool save_claims(void)']:
  a=source.index(signature+'\n{');extracted+=source[a:source.index('\n}',a)+2]+'\n'
 source_clan=(ROOT/'src/clan.c').read_text()
 for signature in ['bool add_clan(struct clan_data *this_clan)', 'bool remove_clan(clan_vnum c_v)', 'void copy_clan_data(struct clan_data *to_clan, struct clan_data *from_clan)', 'void free_clan_list(void)', 'static bool remove_sql_clan(clan_vnum c_v)']:
  a=source_clan.index(signature+'\n{');extracted+=source_clan[a:source_clan.index('\n}',a)+2]+'\n'
 harness=tmp/'test.c';harness.write_text((ROOT/'tests/clan_persistence.c').read_text()+'\n'+extracted)
 binary=tmp/'check';flags=shlex.split(subprocess.check_output(['pkg-config','--cflags','--libs','mariadb','openssl','json-c'],text=True))
 subprocess.run(['cc','-std=gnu99','-g','-fsanitize=address,undefined','-fno-pie','-no-pie','-I'+str(ROOT/'src'),str(harness),str(ROOT/'src/clan_persistence.c'),str(ROOT/'src/clan_repository.c'),str(ROOT/'src/clan_awards.c'),str(ROOT/'src/clan_progression.c'),*flags,'-o',str(binary)],check=True)
 (tmp/'etc').mkdir();subprocess.run([str(binary),str(socket)],cwd=tmp,check=True)
