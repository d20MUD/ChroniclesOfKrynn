#!/usr/bin/env python3
"""Phase 32 rotation and Phase 33 definition/world validation, no live services."""
from pathlib import Path
import copy,json,re,shlex,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='clan-expeditions-') as tmp:
    binary=Path(tmp)/'validate'
    flags=shlex.split(subprocess.check_output(['pkg-config','--cflags','--libs','json-c'],text=True))
    subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-pedantic','-g','-fsanitize=address,undefined','-fno-pie','-no-pie','-Isrc','tests/clan_expeditions.c','src/clan_expeditions.c','src/clan_progression.c',*flags,'-o',str(binary)],cwd=ROOT,check=True)
    subprocess.run([str(binary),'rotation'],check=True)
    catalog=json.loads((ROOT/'lib/clan/expeditions.json').read_text())
    # Actual indexed world: prototypes alone are insufficient; require a positive
    # probability reset into a room in the declared zone's bounds.
    def indexed(kind):
        folder=ROOT/'lib/world'/kind
        return [folder/line.strip() for line in (folder/'index').read_text().splitlines() if line.strip() and not line.startswith(('$','*'))]
    mobs=set()
    for p in indexed('mob'):
        mobs.update(map(int,re.findall(r'^#(\d+)\s*$',p.read_text(errors='replace'),re.M)))
    areas=[];targets=[]
    for p in indexed('zon'):
        text=p.read_text(errors='replace');lines=text.splitlines()
        a=int(lines[0][1:]);areas.append(a)
        bounds=next(line for line in lines[1:] if re.match(r'^\d+ \d+ ',line)).split();lo,hi=map(int,bounds[:2])
        for m,room,chance in re.findall(r'^M\s+\d+\s+(\d+)\s+-?\d+\s+(\d+)\s+(\d+)',text,re.M):
            if int(m) in mobs and lo<=int(room)<=hi and int(chance)>0:targets.append(dict(area=a,mob=int(m)))
    wanted={(t['area'],t['mob']) for d in catalog['expeditions'] for o in d['objectives'] for t in o['targets']}
    targets=[t for t in targets if (t['area'],t['mob']) in wanted]
    request=dict(catalog=catalog,areas=areas,targets=targets,available=targets)
    def run(req,valid=False,contains=None):
        p=subprocess.run([str(binary)],input=json.dumps(req),text=True,capture_output=True,check=True)
        out=json.loads(p.stdout);assert out['ok']==valid,out
        if contains:assert contains in out['error'],out
    run(request,True)
    for changes in [dict(family=3),dict(min_player_level=31),dict(max_player_level=1),dict(shared_reward_id='arbitrary'),dict(personal_reward_id='arbitrary'),dict(id="bad'id"),dict(extra=1),dict(family='1')]:
        bad=copy.deepcopy(request);bad['catalog']['expeditions'][0].update(changes);run(bad)
    for field in ['id','name','objectives','shared_reward_id']:
        bad=copy.deepcopy(request);del bad['catalog']['expeditions'][0][field];run(bad)
    bad=copy.deepcopy(request);bad['catalog']['expeditions']*=2;run(bad,contains='duplicate ID')
    for objective in range(3):
        bad=copy.deepcopy(request);bad['catalog']['expeditions'][0]['objectives'][objective]['quota']+=1;run(bad,contains='quota')
    bad=copy.deepcopy(request);bad['catalog']['expeditions'][0]['objectives'][1]['targets'].pop();run(bad,contains='alternatives')
    bad=copy.deepcopy(request);bad['catalog']['expeditions'][0]['objectives'][1]['targets'][0]=bad['catalog']['expeditions'][0]['objectives'][0]['targets'][0];run(bad,contains='overlapping')
    bad=copy.deepcopy(request);bad['catalog']['expeditions'][0]['objectives'][1]['targets'][0]['area']=140;run(bad,contains='one area')
    bad=copy.deepcopy(request)
    for obj in bad['catalog']['expeditions'][1]['objectives']:
        for ref in obj['targets']:ref['area']=140
    run(bad,contains='two areas')
    bad=copy.deepcopy(request);bad['areas']=[];run(bad,contains='missing area/reset')
    bad=copy.deepcopy(request);bad['targets']=[];run(bad,contains='missing area/reset')
    bad=copy.deepcopy(request);bad['available']=[];run(bad,contains='no available alternative')
    # Missing population of one alternative is fine; missing prototype/reset is not.
    bad=copy.deepcopy(request);missing=dict(area=141,mob=14001)
    bad['available']=[t for t in targets if t!=missing];run(bad,True)
    bad['targets']=bad['available'];run(bad,contains='141/14001')
    for value in [None,{},[],dict(version=2,expeditions=[]),dict(version=1,expeditions=[None]),dict(version=1,expeditions=catalog['expeditions']*9)]:
        run(request|dict(catalog=value))
    print('Expedition definitions, indexed world resets, strict quotas/alternatives/rewards/bands, rejected malformed catalogs, and deterministic weekly rotation passed.')
