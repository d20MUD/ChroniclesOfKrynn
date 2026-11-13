#!/usr/bin/env python3
"""Phases 42/45: bounded private SQL reports and receipt-backed UI commands."""
from clan_database import ROOT,database
from clan_snapshot_fixture import snapshot,manifest,A,B
import json,subprocess,shlex,sys
sys.path.insert(0,str(ROOT/'scripts'))
from build_clan_repository_tool import build
migrations=tuple(f'20261110_clan_progression_v{i:03d}.sql' for i in range(1,6))+('20261111_clan_buff_activations_v001.sql','20261111_clan_buff_lifecycle_v001.sql')
with database(migrations) as (tmp,socket,sql,client):
    tool=tmp/'tool';build(tool,sanitize=True)
    subprocess.run([str(tool),'import','clan_test','root','',str(socket)],input=json.dumps(dict(snapshot=snapshot(),expected_revision=0,manifest=manifest())),text=True,check=True,stdout=subprocess.DEVNULL)
    sql('UPDATE clan_store_control SET store_mode=2;UPDATE clan_import_manifests SET import_status=2')
    sql(f"UPDATE clan_progression_state SET xp_eighths=44800,influence_eighths=24000,revision=revision+1 WHERE clan_uid='{A}';INSERT INTO clan_personal_favor VALUES(5000000007,'{A}',801,0,2200000000);INSERT INTO clan_memberships VALUES(5000000010,'{A}',2,0)")
    flags=shlex.split(subprocess.check_output(['pkg-config','--cflags','--libs','mariadb','json-c','openssl'],text=True));binary=tmp/'web'
    subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-pedantic','-g','-fsanitize=address,undefined','-fno-pie','-no-pie','-Isrc',*[str(ROOT/p) for p in ['tests/clan_web.c','src/clan_web.c','src/clan_web_content.c','src/clan_quartermaster.c','src/clan_delivery.c','src/clan_objectives.c','src/clan_expedition_enrollment.c','src/clan_expeditions.c','src/clan_combat.c','src/clan_buff_activation.c','src/clan_buff_commit.c','src/clan_buff_lifecycle.c','src/clan_buff_queries.c','src/clan_repository.c','src/clan_awards.c','src/clan_progression.c']],*flags,'-o',str(binary)],cwd=ROOT,check=True)
    base=dict(player=5000000007,account=77,now=2200000001,clan=A,family=1,tier=1,slot=1,operation='1'*32)
    def run(op='read',change=None,expected='ok'):
        p=subprocess.run([str(binary),str(socket),op],input=json.dumps(base|(change or {})),text=True,capture_output=True);assert p.returncode==0,(p.stdout,p.stderr)
        r=json.loads(p.stdout);assert r['result']==expected,(op,r,p.stderr);return r.get('data')
    retained=sql('SELECT * FROM clan_progression_state;SELECT * FROM clan_personal_favor;SELECT * FROM clan_buff_families;SELECT * FROM clan_buff_activations')
    view=run();assert view['status']=='ready' and view['clan']['level']==8 and view['favor']['eighths']=='801' and view['may_manage'] and view['character_id']=='5000000007',view
    assert len(json.dumps(view))<49152 and len(view['buffs'])==5 and len(view['next_unlocks'])<=6
    assert run()==view
    assert sql('SELECT * FROM clan_progression_state;SELECT * FROM clan_personal_favor;SELECT * FROM clan_buff_families;SELECT * FROM clan_buff_activations')==retained
    assert 'clan' not in run(change=dict(disabled=1))
    assert run(change=dict(player=5000000009))['status']=='non_member'
    assert run(change=dict(player=99999))['status']=='non_member'
    member=run(change=dict(player=5000000010,account=80));assert not member['may_manage'] and member['favor']['eighths']=='0'
    assert all(not s['enabled'] for b in member['buffs'] for t in b['tiers'] for s in t['slots'])
    assert all(not s['enabled'] for b in run(change=dict(buffs_disabled=1))['buffs'] for t in b['tiers'] for s in t['slots'])
    # Actual serialized server report is consumed by the gateway/browser regression.
    fixture=ROOT/'tests/fixtures/clan_progress.json';fixture.parent.mkdir(exist_ok=True);fixture.write_text(json.dumps(view,indent=2)+'\n')
    # All earning sources feed the same account window; a fresh read reflects
    # shooting/kill, crafting/supply and completion updates without special UI hooks.
    week=view['contribution']['week_ends_at']-604800
    sql(f"INSERT INTO clan_account_windows(account_id,starts_at_utc,base_xp_eighths,favor_eighths,kill_claims,supply_order_claims,delivery_claims,gathering_claims,bounty_claims,expedition_claims,revision,updated_at_utc) VALUES(77,{week},800,400,2,1,1,1,1,1,0,2200000001)")
    earned=run()['contribution'];assert earned['base_xp_eighths']=='800' and earned['favor_eighths']=='400' and [s['used'] for s in earned['sources']]==['2','1','1','1','1','1']
    assert run(change=dict(account=80))['contribution']['base_xp_eighths']=='0'
    assert run(change=dict(now=view['contribution']['week_ends_at']))['contribution']['base_xp_eighths']=='0'
    receipt=run('activate');active=run();assert active['slots_used']==1 and active['buffs'][0]['activation_uid']=='1'*32
    balance=active['clan']['influence_eighths'];assert run('activate')==receipt and run()['clan']['influence_eighths']==balance
    run('activate',dict(operation='2'*32),expected='ineligible')
    # A stale replacement expectation never spends; explicit exact replacement works once.
    run('activate',dict(family=2,operation='3'*32,expected='f'*32),expected='ineligible')
    replacement=run('activate',dict(family=2,operation='4'*32,expected='1'*32));assert run('activate',dict(family=2,operation='4'*32,expected='1'*32))==replacement
    canceled=run('cancel',dict(operation='5'*32,activation='4'*32));assert run('cancel',dict(operation='5'*32,activation='4'*32))==canceled
    after=run();assert after['slots_used']==0 and after['buffs'][1]['cooling_down']
    deadline=after['buffs'][1]['next_available_at'];expiry=after['buffs'][1]['expires_at']
    assert run(change=dict(now=expiry))['buffs'][1]['cooling_down']
    assert not run(change=dict(now=deadline))['buffs'][1]['cooling_down']
    # Current SQL state, not cache/file clan IDs or caller-selected UIDs.
    sql(f"INSERT INTO clan_buff_management VALUES('{A}',21,2)")
    assert run(change=dict(player=5000000010,account=80))['may_manage']
    sql('DELETE FROM clan_memberships WHERE player_id=5000000007')
    assert run()['status']=='non_member'
    assert run('activate')==receipt # immutable receipt remains replayable after departure
    sql('DROP TRIGGER cble_retained')
    unavailable=run(change=dict(player=5000000010,account=80));assert not unavailable['buff_state_available']
    assert all(not s['enabled'] for b in unavailable['buffs'] for t in b['tiers'] for s in t['slots'])
    # Phase 46 optional schemas: exact saved progress, own outing participation,
    # private entitlements and journal holds are read without lazy-reset writes.
    for migration in ['20261111_clan_delivery_v001.sql','20261111_clan_objectives_v001.sql','20261112_clan_expeditions_v001.sql','20261112_clan_expeditions_v002.sql','20261112_clan_quartermaster_v001.sql']:
        sql((ROOT/'sql'/migration).read_text())
    sql(f"INSERT INTO clan_memberships VALUES(1007,'{A}',1,0);INSERT INTO clan_personal_favor VALUES(1007,'{A}',12000,0,2200000000)")
    base['player']=1007
    delivery=json.loads((ROOT/'lib/clan/deliveries.json').read_text())['deliveries'][0]
    objective=json.loads((ROOT/'lib/clan/objectives.json').read_text())['objectives'][0]
    expedition=json.loads((ROOT/'lib/clan/expeditions.json').read_text())['expeditions'][0]
    def quoted(value):return json.dumps(value).replace("'", "''")
    now=base['now'];end=week+604800
    sql(f"INSERT INTO clan_delivery_contracts(accepted_uid,clan_uid,player_id,account_id,definition_id,terms,accepted_at,expires_at,week_start) VALUES('{'a'*32}','{A}',1007,77,'{delivery['id']}','{quoted(delivery)}',{now-1},{end},{week})")
    sql(f"INSERT INTO clan_objective_contracts(contract_uid,clan_uid,player_id,account_id,family,definition_id,terms,accepted_at,expires_at,week_start,progress) VALUES('{'b'*32}','{A}',1007,77,4,'{objective['id']}','{quoted(objective)}',{now-1},{end},{week},7)")
    sql(f"INSERT INTO clan_expedition_outings(outing_uid,clan_uid,leader_id,terms,created_at,expires_at,week_start) VALUES('{'c'*32}','{A}',1007,'{quoted(expedition)}',{now-1},{end},{week})")
    sql(f"INSERT INTO clan_expedition_roster VALUES('{'c'*32}',1007,77,{now-1},0),('{'c'*32}',5000000010,80,{now-1},0)")
    sql(f"INSERT INTO clan_expedition_events VALUES('{'c'*32}','{A}','{'d'*32}',{now},210,21007,0,'[1007,5000000010]','{{}}',21001)")
    before=sql('SELECT * FROM clan_delivery_contracts;SELECT * FROM clan_objective_contracts;SELECT * FROM clan_expedition_outings;SELECT * FROM clan_expedition_events;SELECT * FROM clan_expedition_roster')
    services=run()['content'];assert all(s['ready'] for s in services['contracts']) and services['expedition']['ready'] and services['quartermaster']['ready'],services
    assert services['contracts'][1]['active']['progress']==7
    assert services['expedition']['active']['personal_credit']==1 and services['expedition']['active']['counts']==[1,0,0]
    assert services['expedition']['active']['roster_count']==2 and 'roster' not in services['expedition']['active']
    assert run(change=dict(player=5000000010,account=80))['content']['contracts'][1].get('active') is None
    assert run(change=dict(account=99))['content']['contracts'][0]['ready'] is False
    assert before==sql('SELECT * FROM clan_delivery_contracts;SELECT * FROM clan_objective_contracts;SELECT * FROM clan_expedition_outings;SELECT * FROM clan_expedition_events;SELECT * FROM clan_expedition_roster')
    # Compose actual production runtime actions and consume this fixture in web tests.
    servicefile=tmp/'services.json';servicefile.write_text(json.dumps(run()))
    native=tmp/'content-runtime'
    subprocess.run(['cc','-std=gnu99','-Wall','-Wextra','-Wno-unused-parameter','-ffunction-sections','-fdata-sections','-Wl,--gc-sections','-g','-fsanitize=address,undefined','-fno-pie','-no-pie','-Isrc',*[str(ROOT/p) for p in ['tests/clan_web_content_runtime.c','src/clan_web_content_runtime.c','src/crafting_recipes.c','src/clan_delivery.c','src/clan_objectives.c','src/clan_expeditions.c','src/clan_quartermaster.c','src/clan_progression.c']],*flags,'-o',str(native)],cwd=ROOT,check=True)
    def enriched(mode):
        p=subprocess.run([str(native),str(servicefile),mode],cwd=ROOT/'lib',capture_output=True,text=True);assert p.returncode==0,p.stderr;return json.loads(p.stdout)
    live=enriched('ready');c=live['content'];assert c['context_ready'] and c['contracts'][0]['active']['progress']==50
    assert len(c['quartermaster']['choices'])==7 and c['quartermaster']['choices'][0]['actions'][0]['enabled']
    for mode in ['away','poor','restricted','disabled']:
        locked=enriched(mode)['content']['quartermaster']['choices'][0]['actions'][0];assert not locked['enabled'] and locked['reasons'],(mode,locked)
    assert c['expedition']['active']['objectives'][0]['progress']==1 and c['expedition']['active']['personal_required']==11
    assert len(json.dumps(live))<49152
    preview=enriched('preview');assert '100.000 lifetime Influence, 50.000 Influence and 50.000 Favor' in preview['content']['contracts'][0]['preview']['summary']
    assert preview['content']['quartermaster']['preview']['actions'][0]['token']=='2'*32
    (ROOT/'tests/fixtures/clan_services.json').write_text(json.dumps(preview,indent=2)+'\n')
    sql(f"UPDATE clan_expedition_outings SET state=3 WHERE outing_uid='{'c'*32}';INSERT INTO clan_expedition_completions VALUES('{'c'*32}',1,{week},{now},{end},22);INSERT INTO clan_expedition_entitlements VALUES('{'c'*32}',1007,77,'{'e'*32}',0)")
    own=run()['content']['expedition']['claims'];assert len(own)==1 and own[0]['state']=='unclaimed'
    assert run(change=dict(player=5000000010,account=80))['content']['expedition']['claims']==[]
    sql(f"UPDATE clan_expedition_entitlements SET state=1 WHERE outing_uid='{'c'*32}'")
    assert run(change=dict(now=end+604800))['content']['expedition']['claims'][0]['state']=='reward_pending'
    # Missing a required schema disables just that optional section.
    sql('DROP TABLE clan_quartermaster_schema')
    assert not run()['content']['quartermaster']['ready'] and run()['status']=='ready'
    # Help publication is repeatable and matches the checked-in player help.
    sql("CREATE TABLE help_entries(tag VARCHAR(80) PRIMARY KEY,category VARCHAR(40),entry LONGTEXT,min_level INT,max_level INT,auto_generated BOOL);CREATE TABLE help_keywords(help_tag VARCHAR(80),keyword VARCHAR(80),PRIMARY KEY(help_tag,keyword))")
    help_sql=(ROOT/'sql/20261112_clan_progression_services_help.sql').read_text()
    sql(help_sql);sql(help_sql)
    assert sql('SELECT COUNT(*) FROM help_entries')=='6'
    for tag,name in [('CLANPROGRESSION','clan_progression'),('CLANBUFFS','clan_buffs'),('CLANDELIVERIES','clan_deliveries'),('CLANCONTRACTS','clan_contracts'),('CLANEXPEDITIONS','clan_expeditions'),('CLANQUARTERMASTER','clan_quartermaster')]:
        saved=sql(f"SELECT HEX(entry) FROM help_entries WHERE tag='{tag}'")
        expected=(ROOT/f'docs/help/{name}.hlp').read_text().removesuffix('#0\n').rstrip().replace('\n','\r\n')
        assert bytes.fromhex(saved).decode()==expected,tag
    print('Clan services: saved SQL progress/privacy/no writes, production live eligibility, fixed actions and optional schema failure passed.')
    print('Clan web: private bounded consistent reads, no writes, disabled/applicant/former/delegated access, exact currencies, active/cooldown boundaries, receipt replay and stale replacement protection passed.')

    runtime=tmp/'runtime'
    subprocess.run(['cc','-std=gnu99','-Wall','-Wextra','-Werror','-g','-fsanitize=address,undefined','-fno-pie','-no-pie','-Isrc',str(ROOT/'tests/clan_web_runtime.c'),str(ROOT/'src/clan_progression.c'),*flags,'-o',str(runtime)],cwd=ROOT,check=True)
    subprocess.run([str(runtime)],check=True)
    # Export a frame made by the production MSDPSend, not a hand-built JSON bridge.
    source=(ROOT/'src/protocol.c').read_text();start=source.index('bool_t MSDPSend(');function=source[start:source.index('\n}',start)+2]
    code='''
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "protocol.h"
#include <arpa/telnet.h>
#include <json-c/json.h>
#include <assert.h>
static struct { bool_t bString; const char *pName; } VariableNameTable[eMSDP_MAX]={[eMSDP_CLAN_PROGRESS]={true,"CLAN_PROGRESS"}};
void ReportBug(const char *s){(void)s;abort();}
static bool_t Write(struct descriptor_data *d,const char *s){(void)d;return fwrite(s,1,strlen(s),stdout)==strlen(s);}
'''+function+'''
int main(int argc,char **argv){struct descriptor_data d={0};protocol_t p={0};MSDP_t v={0},*values[eMSDP_MAX]={0};struct json_object *o;assert(argc==2);o=json_object_from_file(argv[1]);assert(o);p.bMSDP=true;p.pVariables=values;values[eMSDP_CLAN_PROGRESS]=&v;v.pValueString=(char *)json_object_to_json_string_ext(o,JSON_C_TO_STRING_PLAIN);d.pProtocol=&p;assert(MSDPSend(&d,eMSDP_CLAN_PROGRESS));json_object_put(o);return 0;}
'''
    wire=tmp/'wire';(tmp/'wire.c').write_text(code)
    subprocess.run(['cc','-std=gnu99','-Isrc',str(tmp/'wire.c'),*flags,'-o',str(wire)],cwd=ROOT,check=True)
    import base64
    frame=subprocess.check_output([str(wire),str(fixture)])
    (fixture.parent/'clan_progress.msdp.b64').write_text(base64.b64encode(frame).decode()+'\n')
    assert frame.startswith(bytes([255,250,69,1])+b'CLAN_PROGRESS'+bytes([2])) and frame.endswith(bytes([255,240]))
