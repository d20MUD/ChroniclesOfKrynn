#!/usr/bin/env python3
"""Phase 12: actual SQL read models, text commands/permissions and no-write proof."""
from clan_database import ROOT,database
from clan_snapshot_fixture import snapshot,manifest,clan,A,B
import subprocess,shlex,json,sys,copy,os
sys.path.insert(0,str(ROOT/'scripts'))
from build_clan_repository_tool import build
migrations=tuple(f'20261110_clan_progression_v{i:03d}.sql' for i in range(1,6))
with database(migrations) as (tmp,socket,sql,client):
    tool=tmp/'tool';build(tool,sanitize=True)
    def bridge(op,payload):
        p=subprocess.run([str(tool),op,'clan_test','root','',str(socket)],input=json.dumps(payload),text=True,capture_output=True,check=True)
        out=json.loads(p.stdout);assert out['result']=='ok',(op,out,p.stderr);return out
    bridge('import',dict(snapshot=snapshot(),expected_revision=0,manifest=manifest()))
    sql('UPDATE clan_store_control SET store_mode=2; UPDATE clan_import_manifests SET import_status=2;')
    flags=shlex.split(subprocess.check_output(['pkg-config','--cflags','--libs','mariadb','openssl','json-c'],text=True))
    binary=tmp/'status'
    subprocess.run(['cc','-std=gnu99','-Wall','-Wextra','-Werror','-g','-fsanitize=address,undefined','-fno-pie','-no-pie','-I'+str(ROOT/'src'),str(ROOT/'tests/clan_status.c'),*[str(ROOT/'src'/f) for f in ['clan_repository.c','clan_progression.c','clan_awards.c','clan_buff_queries.c','clan_status.c','clan_commands.c']],*flags,'-o',str(binary)],check=True)
    reward=tmp/'reward'
    current=tmp/'rewards.c';current.write_text((ROOT/'tests/clan_rewards.c').read_text().replace('clan_progression_v1','clan_progression_defaults'))
    subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-pedantic','-DCLAN_REWARD_TEST','-fsanitize=address,undefined','-fno-pie','-no-pie','-I'+str(ROOT/'src'),str(current),*[str(ROOT/'src'/f) for f in ['clan_repository.c','clan_progression.c','clan_awards.c']],*flags,'-o',str(reward)],check=True)
    def award(op,body):
        p=subprocess.run([str(reward),str(socket),op],input=json.dumps(body),text=True,capture_output=True,check=True)
        out=json.loads(p.stdout);assert out['result']=='ok',(out,p.stderr);return out
    e=dict(operation_uid='1'*32,event_uid='2'*32,clan_uid=A,account_id=77,player_id=5000000007,source=2,rules_version=2,occurred_at=2200000001,now=2200000002,requested_xp_eighths=800,requested_influence_eighths=400,requested_favor_eighths=400)
    award('prepare-member',e);award('personal',e)
    sql('UPDATE clan_account_windows SET base_xp_eighths=4400,revision=revision+1 WHERE account_id=77')
    e=e|dict(operation_uid='3'*32,event_uid='4'*32)
    award('prepare-member',e);award('personal',e)
    sql(f"INSERT INTO clan_personal_favor VALUES(5000000007,'{B}',160,0,2200000002)")
    sql(f"INSERT INTO clan_buff_families VALUES('{A}',1,'{'5'*32}',1,1,1,2200000000,2200000100,2200000200,800,0),('{A}',2,'{'6'*32}',1,2,1,2200000000,2200000100,2200000300,800,1)")
    def run(body,expected='ok',zone='UTC'):
        body=dict(mode='status',player=5000000007,account=77,now=2200000002)|body
        p=subprocess.run([str(binary),str(socket)],input=json.dumps(body),text=True,capture_output=True,env=os.environ|dict(TZ=zone))
        assert p.returncode==0,(body,p.stdout,p.stderr)
        out=json.loads(p.stdout);assert out['result']==expected,(body,out,p.stderr);return out
    def state():
        return '\n'.join(sql('SELECT * FROM '+table+' ORDER BY 1,2') for table in ['clan_progression_clans','clan_progression_state','clan_memberships','clan_personal_favor','clan_account_windows','clan_weekly_windows','clan_buff_families','clan_reward_receipts','clan_store_control'])
    before=state()
    report=run({});text=report['text']
    assert report['full'] and report['render_ok']
    assert 'Level         1' in text and 'Lifetime      10.875 Influence' in text and 'Favor         21.875' in text
    assert A not in text and '#5000000007' not in text and 'Account #77' not in text
    assert 'Contribution 650' in text and 'Supply orders 2 / 3' in text
    assert '+9.375 Favor   +4.625 Influence' in text
    assert 'Account weekly contribution reduced or exhausted.' in text
    assert '1 / 2 slots used' in text and 'ready ' in text and 'Buff activation is disabled.' in text
    assert run({},zone='America/Halifax')['text']==text and run({},zone='Pacific/Auckland')['text']==text
    small=run(dict(capacity=8));assert not small['render_ok'] and small['text']=='UNCHANGED'
    former=run(dict(uid=B));assert not former['full'] and 'Clan balances and buffs require' in former['text'] and 'Clan XP:' not in former['text'] and 'Favor         20' in former['text']
    run(dict(uid='c'*32),'not-found')
    run(dict(player=5000000010,account=99,uid=A),'ineligible')
    # Native command handlers enforce privacy even if called outside dispatcher.
    denied=run(dict(mode='command',inspect=True,args=f'5000000009 88 {B}',level=30));assert 'Staff permission' in denied['text'] and not denied['acquires']
    own=run(dict(mode='command',args='',level=10));assert own['text']==text
    for args in ['5000000009',f'{A} extra',"' OR 1=1 --"]:
        invalid=run(dict(mode='command',args=args,level=10));assert 'No saved standing' in invalid['text'] and invalid['acquires']==1
    privileged=run(dict(mode='command',inspect=True,args=f'5000000009 88 {B}',level=50));assert 'Staff inspection:' in privileged['text'] and privileged['acquires']==1
    noaccount=run(dict(mode='command',account=0));assert 'account identity is unavailable' in noaccount['text'] and not noaccount['acquires']
    outage=run(dict(mode='command',outage=True));assert 'unavailable' in outage['text'] and not outage['acquires']
    legacy=run(dict(mode='command',legacy=True));assert 'unavailable' in legacy['text'] and not legacy['acquires']
    history=run(dict(mode='favor-command'));assert A not in history['text'] and B not in history['text'] and 'Your Clan Favor' in history['text']
    named=run(dict(mode='command',args="Clan '7"));assert named['text']==text
    former_named=run(dict(mode='command',args="Clan '9"));assert 'Favor         20' in former_named['text']
    assert 'More than one' in run(dict(mode='command',args='Clan'))['text']
    week=int(sql('SELECT starts_at_utc FROM clan_account_windows WHERE account_id=77'))
    reset=run(dict(now=week+604800));assert not reset['account_present'] and 'Contribution 0' in reset['text']
    expired=run(dict(now=2200000100));assert '0 / 2 slots used' in expired['text'] and 'ready ' in expired['text']
    assert state()==before,'readout changed authoritative data'
    # Tombstone history remains self-readable but grants no current clan access.
    current=bridge('read',{})
    p=subprocess.run([str(reward),str(socket),'disband'],input=json.dumps(dict(clan_uid=A,revision=current['store_revision'],now=2200000010)),text=True,capture_output=True,check=True)
    assert json.loads(p.stdout)['result']=='ok',p.stdout
    deleted=run(dict(uid=A,now=2200000010));assert not deleted['full'] and 'deleted clan' in deleted['text']
    assert '[disbanded]' in run(dict(mode='favor-command'))['text']
    # Bounded, stable per-character history pagination, without leaking others.
    current=bridge('read',{});current['written_at']=2200000020
    for i in range(24):
        uid=f'{100+i:032x}';entry=clan(uid,100+i);entry['legacy']['relationships']=[]
        current['clans'].append(entry)
    bridge('save',dict(snapshot=current,expected_revision=current['store_revision']))
    for i in range(24):sql(f"INSERT INTO clan_personal_favor VALUES(5000000007,'{100+i:032x}',{i},0,2200000020)")
    sql(f"INSERT INTO clan_personal_favor VALUES(5000000009,'{B}',9999,0,2200000020)")
    page=run(dict(mode='history'))['history'];assert len(page['entries'])==25 and page['next_uid']
    page2=run(dict(mode='history',uid=page['next_uid']))['history'];assert len(page2['entries'])==1 and not page2['next_uid']
    assert len({e['uid'] for e in page['entries']+page2['entries']})==26
    history_page2=run(dict(mode='favor-command',args='2'))['text']
    assert 'Your Clan Favor' in history_page2 and 'clan progress <clan name>' in history_page2
    assert all(e['uid'] not in history_page2 for e in page2['entries'])
    assert '9999' not in run(dict(mode='favor-command'))['text']
    # Schema drift never masquerades as an empty/zero report.
    sql('DROP TRIGGER cp_favor_retained');bad=run(dict(uid=B),'schema-mismatch');assert bad['text']=='UNCHANGED'
    print('Phase 12: coherent SQL status, exact fractional rewards/budgets, ownership/staff command permissions, account identity, retained history/pagination, UTC resets, pure buff readouts, no writes and fail-closed errors passed.')
