#!/usr/bin/env python3
"""Actual game delivery coordinator, private SQL and restartable file checkpoints."""
from clan_database import ROOT,database
from clan_snapshot_fixture import snapshot,manifest
import subprocess,shlex,json,sys
sys.path.insert(0,str(ROOT/'scripts'))
from build_clan_repository_tool import build
migrations=tuple(f'20261110_clan_progression_v{i:03d}.sql' for i in range(1,6))+('20261111_clan_delivery_v001.sql',)
with database(migrations) as (tmp,socket,sql,client):
    tool=tmp/'tool';build(tool,sanitize=True)
    subprocess.run([str(tool),'import','clan_test','root','',str(socket)],input=json.dumps(dict(snapshot=snapshot(),expected_revision=0,manifest=manifest())),text=True,check=True,stdout=subprocess.DEVNULL)
    sql('UPDATE clan_store_control SET store_mode=2;UPDATE clan_import_manifests SET import_status=2;')
    native=tmp/'native';flags=shlex.split(subprocess.check_output(['pkg-config','--cflags','--libs','mariadb','openssl','json-c'],text=True))
    subprocess.run(['cc','-std=gnu99','-Wall','-Wextra','-Wno-unused-parameter','-g','-DCLAN_REWARD_TEST','-ffunction-sections','-fdata-sections','-Wl,--gc-sections','-fsanitize=address,undefined','-fno-pie','-no-pie','-I'+str(ROOT/'src'),*[str(ROOT/p) for p in ['tests/clan_delivery_runtime.c','src/clan_delivery.c','src/crafting_recipes.c','src/clan_repository.c','src/clan_awards.c','src/clan_progression.c']],*flags,'-o',str(native)],check=True)
    case=0
    def invoke(kind,mode,path,crash=0,fail=0,sqlcrash=0,expected_crash=False):
        now=2200000001+case*604800
        p=subprocess.run([str(native),str(socket),str(path),kind,mode,str(crash),str(fail),str(now),str(sqlcrash)],text=True,capture_output=True,cwd=ROOT/'lib')
        if expected_crash:
            assert p.returncode!=0,(p.stdout,p.stderr);return
        assert p.returncode==0,(p.returncode,p.stdout,p.stderr)
        return json.loads(p.stdout)
    if '--goods-only' in sys.argv:
        for kind in ['material','ammo','crafted']:
            directory=tmp/f'goods-only-{kind}';directory.mkdir();invoke(kind,'goods',directory)
        print('Goods matching passed against all 17 definitions and actual weapon/armor/shield/clothing/instrument recipes.')
        raise SystemExit(0)
    directory=tmp/'commands';directory.mkdir();invoke('material','commands',directory)
    for kind in ['material','ammo','crafted']:
        directory=tmp/f'goods-{kind}';directory.mkdir();invoke(kind,'goods',directory)
        for stop in [0,1,2,3,4,5]:
            case+=1;directory=tmp/f'{kind}-{stop}';directory.mkdir()
            if stop:invoke(kind,'begin',directory,crash=stop,expected_crash=True)
            else:assert invoke(kind,'begin',directory)['ok']
            result=invoke(kind,'recover',directory);assert result['ok'],(kind,stop,result)
            consumed=stop!=1
            assert result['materials']==(150 if consumed and kind=='material' else 200),(kind,stop,result)
            assert result['shots']==(50 if consumed and kind=='ammo' else 150),(kind,stop,result)
            if kind=='crafted':assert result['items']==(1 if consumed else 3),(kind,stop,result)
            assert invoke(kind,'recover',directory)==result
            assert sql('SELECT COUNT(*) FROM clan_delivery_operations WHERE state IN (0,1)')=='0'
            sql('UPDATE clan_delivery_contracts SET state=2 WHERE state=0')
        # Save failure before paid goods reached disk: restart cancels without loss.
        case+=1;directory=tmp/f'{kind}-failed-checkpoint';directory.mkdir()
        result=invoke(kind,'begin',directory,fail=3) # first recovery paid re-checkpoint fails
        assert not result['ok'];result=invoke(kind,'recover',directory);assert result['ok']
        assert result['materials']==(150 if kind=='material' else 200)
        assert result['shots']==(50 if kind=='ammo' else 150)
        if kind=='crafted':assert result['items']==1
        # Process dies after SQL COMMIT, before acknowledgement; resources stay spent once.
        case+=1;directory=tmp/f'{kind}-sql-commit';directory.mkdir()
        invoke(kind,'begin',directory,sqlcrash=8,expected_crash=True)
        result=invoke(kind,'recover',directory);assert result['ok']
        assert result['materials']==(150 if kind=='material' else 200)
        assert result['shots']==(50 if kind=='ammo' else 150)
        if kind=='crafted':assert result['items']==1
        assert invoke(kind,'recover',directory)==result
        # A competing account award changes the exact quote: refund, then cancel.
        case+=1;directory=tmp/f'{kind}-refund';directory.mkdir()
        invoke(kind,'begin',directory,crash=2,expected_crash=True)
        week=int(sql('SELECT week_start FROM clan_delivery_contracts WHERE state=0'))
        sql(f"INSERT INTO clan_account_windows(account_id,starts_at_utc,delivery_claims,updated_at_utc) VALUES(77,{week},2,{week})")
        # Crash after refund checkpoint; restart must not refund twice.
        invoke(kind,'recover',directory,crash=2,expected_crash=True)
        result=invoke(kind,'recover',directory);assert result['ok']
        assert result['materials']==200 and result['shots']==150,(kind,result)
        if kind=='crafted':assert result['items']==3,result
        assert invoke(kind,'recover',directory)==result
        assert sql('SELECT COUNT(*) FROM clan_delivery_operations WHERE state IN (0,1)')=='0'
    print('Delivery game recovery: wrong/insufficient/Returning/infinite goods, partial bundles, five file crash boundaries, failed checkpoint, SQL commit crash, capped refund crash, reconnect and repeated retry passed for materials, ammo and finished crafted items.')
