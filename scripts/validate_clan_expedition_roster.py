#!/usr/bin/env python3
"""Audit staged expeditions against indexed world spawns, levels and walking routes.
No database, instance, quest, inventory or generated world content is consulted.
"""
from pathlib import Path
from collections import defaultdict,deque
import argparse,json,re
ROOT=Path(__file__).resolve().parents[1]
def flags(text):
    if re.fullmatch(r'-?\d+',text):return int(text)&0xffffffff
    return sum(1<<(ord(c)-ord('a') if c.islower() else ord(c)-ord('A')+26) for c in text if c.isalpha())
def indexed(kind):
    folder=ROOT/'lib/world'/kind
    return [folder/line.strip() for line in (folder/'index').read_text().splitlines() if line.strip() and not line.lstrip().startswith(('$','*'))]
def world():
    mobs={};rooms={};areas={};spawns=defaultdict(list);locked=set()
    for p in indexed('mob'):
        for m in re.finditer(r'^#(\d+)\n(.*?)(?=^#|\Z)',p.read_text(errors='replace'),re.M|re.S):
            st=re.search(r'^([^\n]+) [ES]\s*\n(\d+) ',m[2],re.M)
            if not st:continue
            fields=st[1].split()
            if len(fields)<8:continue
            mobs[int(m[1])]=dict(level=int(st[2]),name=m[2].splitlines()[1].rstrip('~'),flags=flags(fields[0]),affects=flags(fields[4]))
    for p in indexed('wld'):
        for m in re.finditer(r'^#(\d+)\n(.*?)(?=^#|\Z)',p.read_text(errors='replace'),re.M|re.S):
            parts=m[2].split('~',2)
            if len(parts)<3:continue
            lines=parts[2].strip().splitlines()
            if not lines:continue
            fields=lines[0].split()
            if len(fields)<2:continue
            exits={int(d):int(dest) for d,door,key,dest in re.findall(r'^D(\d+)\n.*?~\n.*?~\n(-?\d+)\s+(-?\d+)\s+(-?\d+)',parts[2],re.M|re.S)}
            rooms[int(m[1])]=dict(flags=flags(fields[1]),exits=exits)
    for p in indexed('zon'):
        text=p.read_text(errors='replace');parts=text.split('~',2)
        header=parts[2].strip().splitlines()[0].split();a=int(text.splitlines()[0][1:])
        if len(header)<4:continue
        lo,hi,lifespan,mode=map(int,header[:4]);closed=len(header)>4 and flags(header[4])&1
        areas[a]=dict(name=parts[1].strip(),lo=lo,hi=hi,lifespan=lifespan,mode=mode,closed=bool(closed))
        for ifflag,mob,maximum,room,chance in re.findall(r'^M\s+(\d+)\s+(\d+)\s+(-?\d+)\s+(\d+)\s+(\d+)',text,re.M):
            if int(ifflag)==0 and int(maximum)!=0 and int(chance)>0 and lo<=int(room)<=hi:
                spawns[a,int(mob)].append(dict(room=int(room),chance=int(chance),maximum=int(maximum)))
        for room,direction,state in re.findall(r'^D\s+\d+\s+(\d+)\s+(\d+)\s+(\d+)',text,re.M):
            if int(state)==2:locked.add((int(room),int(direction)))
    def accessible(room):
        return room in rooms and not rooms[room]['flags']&(1<<1) and not any(a['closed'] and a['lo']<=room<=a['hi'] for a in areas.values())
    graph={r:[dest for d,dest in data['exits'].items() if (r,d) not in locked and accessible(dest)] for r,data in rooms.items() if accessible(r)}
    return mobs,rooms,areas,spawns,graph

def audit(catalog):
    mobs,rooms,areas,spawns,graph=world();report=[]
    def distance(starts,goals):
        q=deque((r,0) for r in starts if r in graph);seen={r for r,_ in q}
        while q:
            r,n=q.popleft()
            if r in goals:return n
            for nxt in graph.get(r,[]):
                if nxt not in seen:seen.add(nxt);q.append((nxt,n+1))
        return None
    for d in catalog['expeditions']:
        objective_rooms=[];choices=[]
        for index,o in enumerate(d['objectives']):
            valid_rooms=set();levels=[]
            for t in o['targets']:
                area=areas.get(t['area']);mob=mobs.get(t['mob']);spawn=spawns[t['area'],t['mob']]
                assert area and not area['closed'] and area['mode']!=0 and 0<area['lifespan']<=15,(d['id'],t,'area/reset')
                assert mob and not mob['flags']&((1<<18)|(1<<23)) and not mob['affects']&(1<<22),(d['id'],t,'not an ordinary fightable mob')
                assert mob['level']>=d['max_player_level']-3,(d['id'],t,'trivial at top of band')
                assert mob['level']<=d['min_player_level']+5,(d['id'],t,'oversized low-band encounter')
                candidate={s['room'] for s in spawn if s['room'] in graph and not rooms[s['room']]['flags']&(1<<4)}
                assert candidate,(d['id'],t,'no unconditional fightable spawn')
                valid_rooms.update(candidate);levels.append(mob['level'])
            assert len(o['targets'])>=2,(d['id'],index,'no fallback')
            population=0
            for t in o['targets']:
                ss=[s for s in spawns[t['area'],t['mob']] if s['chance']==100]
                assert ss,(d['id'],t,'no guaranteed reset')
                capacity=max(s['maximum'] if s['maximum']>0 else 1 for s in ss)
                population+=min(len(ss),capacity)
            resets=(o['quota']+population-1)//population
            minutes=max(areas[t['area']]['lifespan'] for t in o['targets'])*max(0,resets-1)
            assert minutes<=45,(d['id'],index,'too few recurring spawns for a short outing')
            choices.append(dict(objective=index+1,quota=o['quota'],alternatives=len(o['targets']),levels=levels,spawn_rooms=sorted(valid_rooms),population_per_reset=population,reset_wait_minutes=minutes))
            objective_rooms.append(valid_rooms)
        routes=[]
        for i in range(1,3):
            hops=distance(objective_rooms[i-1],objective_rooms[i]);back=distance(objective_rooms[i],objective_rooms[i-1])
            assert hops is not None and back is not None,(d['id'],'targets need a locked, scripted or missing route')
            routes.append(hops)
        # One common anchor must reach every fallback and return without a
        # locked/scripted door; membership in its own spawn set is insufficient.
        anchors=choices[0]['spawn_rooms'];anchor=None
        for start in anchors:
            reachable=True
            for o in d['objectives']:
                for t in o['targets']:
                    destinations={s['room'] for s in spawns[t['area'],t['mob']] if s['room'] in graph and not rooms[s['room']]['flags']&(1<<4)}
                    if distance({start},destinations) is None or distance(destinations,{start}) is None:reachable=False;break
                if not reachable:break
            if reachable:anchor=start;break
        assert anchor is not None,(d['id'],'fallback lacks a common unlocked route')
        report.append(dict(id=d['id'],family=d['family'],character_band=[d['min_player_level'],d['max_player_level']],clan_unlock=d['min_clan_level'],objectives=choices,route_hops=routes,anchor_room=anchor))
    for level in range(5,31):
        for family in [1,2]:assert any(d['family']==family and d['min_player_level']<=level<=d['max_player_level'] and d['min_clan_level']==1 for d in catalog['expeditions']),(level,family,'missing choice')
    return report
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--json',action='store_true');args=parser.parse_args()
    result=audit(json.loads((ROOT/'lib/clan/expeditions.json').read_text()))
    if args.json:print(json.dumps(result,indent=2))
    else:
        for d in result:print(f"{d['id']}: levels {d['character_band'][0]}-{d['character_band'][1]}, clan unlock {d['clan_unlock']}, route hops {d['route_hops']}, all alternatives valid")
