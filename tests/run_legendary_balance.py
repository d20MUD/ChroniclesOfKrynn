#!/usr/bin/env python3
"""Read-only world economy and fixed-seed sensitivity analysis. No server/DB writes.

Combat input is explicitly tagged synthetic until real level-30 build captures
replace the bundled fixture. This is an additional screening tool, not a claim
that native server combat or the owner-review release gate has passed.
"""
from pathlib import Path
import argparse, csv, hashlib, json, math, random, re, statistics, subprocess, tempfile
ROOT=Path(__file__).resolve().parents[1]

def catalog():
    with tempfile.TemporaryDirectory(prefix='legendary-catalog-') as d:
        exe=Path(d)/'catalog'
        subprocess.run(['cc','-std=gnu90','-Wall','-Wextra','-Werror','-I'+str(ROOT/'src'),str(ROOT/'src/legendary.c'),str(ROOT/'tests/legendary_catalog_export.c'),'-o',str(exe)],check=True)
        return json.loads(subprocess.check_output([str(exe)]))

def files(world,kind,suffix):
    directory=world/kind
    return [directory/line.strip() for line in (directory/'index').read_text().splitlines() if line.strip().endswith(suffix)]

def records(path):
    text=path.read_text(errors='replace')
    for match in re.finditer(r'^#(\d+)\s*\n',text,re.M):
        position=match.end(); strings=[]
        for _ in range(4):
            end=text.find('~',position)
            if end<0: raise ValueError(f'{path}: unterminated record {match[1]}')
            strings.append(text[position:end]);position=end+1
        numeric=[line.strip() for line in text[position:].splitlines() if line.strip()][:3]
        yield int(match[1]),strings,numeric

def mob_economy(world,config,stats_source):
    spawned=set()
    for path in files(world,'zon','.zon'):
        spawned.update(int(m[1]) for m in re.finditer(r'^M\s+\d+\s+(\d+)\s',path.read_text(errors='replace'),re.M))
    custom=int(re.search(r'#define MOB_CUSTOM_GOLD\s+(\d+)',stats_source)[1])
    excluded=[int(re.search(r'#define '+name+r'\s+(\d+)',stats_source)[1]) for name in ['MOB_NOKILL','MOB_C_ANIMAL','MOB_C_FAMILIAR','MOB_C_MOUNT','MOB_EIDOLON','MOB_GOLEM','MOB_SUMMON_SOLAR']]
    multiplier=int(config.get('experience_multiplier',100));xp_level=lambda n: ((n-1)**2*(2000+(n-2)*800)*multiplier)//100
    cap=min(int(config.get('max_exp_gain',100000)),(xp_level(31)-xp_level(30))//185)
    result=[]
    for path in files(world,'mob','.mob'):
        for vnum,strings,numeric in records(path):
            attrs=numeric[0].split();vitals=numeric[1].split();rewards=numeric[2].split()
            if not vitals or not vitals[0].isdigit() or int(vitals[0])!=30 or vnum not in spawned:continue
            flags=[int(s) for s in attrs[:4]]
            bit=lambda b:bool(flags[b//32]&(1<<(b%32)))
            if any(bit(b) for b in excluded):continue
            stored_gold,stored_xp=map(int,rewards[:2])
            gold=stored_gold if bit(custom) else 40+30*2*2
            xp=min(cap,max(1,min(int(config.get('max_exp_gain',100000)),stored_xp//3)))
            result.append({'vnum':vnum,'name':strings[1].strip(),'stored_xp':stored_xp,'solo_infusion_xp':xp,'expected_gold':gold,'custom_gold':bit(custom)})
    return result,cap

def items(world):
    result=[]
    def flag_word(value):
        if re.fullmatch(r'-?\d+',value):return int(value)&0xffffffff
        return sum(1<<(ord(c)-ord('a') if c.islower() else ord(c)-ord('A')+26) for c in value)
    blocked=(16,17,114,116) # Native shop trade_with excludes these flags.
    for path in files(world,'obj','.obj'):
        for vnum,strings,numeric in records(path):
            parts=numeric[2].split();flags=[flag_word(v) for v in numeric[0].split()[1:5]]
            if any(flags[b//32]&(1<<(b%32)) for b in blocked):continue
            if len(parts)>=4 and parts[3]=='30':
                cost=int(parts[1])
                if cost>0:result.append({'vnum':vnum,'cost':cost,'name':strings[1].strip()})
    return result

def quantile(values,q):
    ordered=sorted(values);return ordered[round((len(ordered)-1)*q)]

def combat(profiles,defs):
    f={v['id']:v for v in defs['foundations']};rows=[]
    # Isolated passive sensitivity: a legal 12-point attack/defense allocation.
    # Empty remaining outfit channels add no bonus. All nine channels still
    # have ordinary independent advancement costs. Active effects are measured
    # separately by the production integration suite; never invented here.
    points=f[1]['points']+2*f[2]['points']+f[3]['points']
    assert points<=12
    for profile in profiles['profiles']:
        for duration in (30,600):
            for group_sources in (1,3):
                rng=random.Random(profiles['seed']);base_out=extra_out=base_in=extra_in=0
                for _ in range(duration//6):
                    for _ in range(profile['attacks_per_round']):
                        roll=rng.randint(1,20);spell=profile.get('spell',False)
                        baseline=spell or roll==20 or (roll!=1 and roll+profile['attack_bonus']>=profile['enemy_ac'])
                        modified=spell or roll==20 or (roll!=1 and roll+profile['attack_bonus']+f[1]['value']>=profile['enemy_ac'])
                        base_out+=profile['damage']*baseline
                        extra_out+=(profile['damage']+(0 if spell else f[2]['ceiling']))*modified
                    for _ in range(4):
                        roll=rng.randint(1,20)
                        baseline=roll==20 or (roll!=1 and roll+profile['incoming_attack']>=profile['armor_class'])
                        modified=roll==20 or (roll!=1 and roll+profile['incoming_attack']>=profile['armor_class']+f[3]['value'])
                        base_in+=profile['incoming_damage']*baseline;extra_in+=profile['incoming_damage']*modified
                offense=100*(extra_out-base_out)/base_out if base_out else None
                defense=100*(base_in-extra_in)/base_in if base_in else None
                rows.append({'role':profile['role'],'seconds':duration,'group_sources':group_sources,'passive_points':points,'baseline_damage':base_out,'legendary_damage':extra_out,'outgoing_increase_pct':offense,'baseline_received':base_in,'legendary_received':extra_in,'received_reduction_pct':defense,'screen_pass':offense is not None and defense is not None and offense<=5 and defense<=8})
    return rows

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--world',type=Path,default=Path('/home/krynn/live/mud/lib/world'))
    parser.add_argument('--config',type=Path,default=Path('/home/krynn/live/mud/lib/etc/config'))
    parser.add_argument('--profiles',type=Path,default=ROOT/'tests/data/legendary_balance_profiles.json')
    parser.add_argument('--output',type=Path,default=ROOT/'docs/systems/legendary_balance_results')
    args=parser.parse_args();defs=catalog();config=dict(re.findall(r'^(\w+)\s*=\s*([^\n*]+)',args.config.read_text(),re.M))
    mobs,xp_cap=mob_economy(args.world,config,(ROOT/'src/structs.h').read_text());objects=items(args.world)
    if not mobs or not objects:raise ValueError('No eligible level-30 mobs/items found; refusing an invented estimate')
    gold_modifiers=[int(v.strip()) for k,v in config.items() if re.fullmatch(r'mob_26_30_\w+_gold',k)]
    if any(v!=100 for v in gold_modifiers):raise ValueError('Non-default level-30 gold modifiers require explicit category handling')
    economy=[]
    # Five total items/hour; show all random, equal mix, and all regular drops.
    for pct in (0.25,0.5,0.75):
        xp=quantile([m['solo_infusion_xp'] for m in mobs],pct)*20
        coins=quantile([m['expected_gold'] for m in mobs],pct)*20
        price=quantile([i['cost'] for i in objects],pct)
        for random_share in (0,0.5,1):
            random_price=3100+6*5 # native level-30, grade-six generated gear
            sale_each=(random_share*math.floor(random_price*.1)+(1-random_share)*math.floor(price*.1))
            hourly_gold=coins+5*sale_each
            for channels in (1,2,9):
                rank=defs['ranks'][-1];materials_days=max(math.ceil(rank[k]*channels/defs['daily_caps'][k]) for k in defs['daily_caps'])
                economy.append({'percentile':int(pct*100),'random_drop_share':random_share,'kills_per_hour':20,'items_per_hour':5,'sale_fraction':.1,'xp_per_hour':xp,'mob_gold_per_hour':coins,'regular_item_cost':price,'random_item_cost':random_price,'item_sales_per_hour':5*sale_each,'gold_per_hour':hourly_gold,'channels':channels,'xp_hours_to_v':rank['xp']*channels/xp,'gold_hours_to_v':rank['gold']*channels/hourly_gold,'material_days_min':materials_days})
    profile=json.loads(args.profiles.read_text());rows=combat(profile,defs)
    result={'model':'Owner-approved 20 level-30 kills/hour + 5 total level-30 item sales/hour at 10%; not measured routes','source_config_sha256':hashlib.sha256(args.config.read_bytes()).hexdigest(),'eligible_spawned_mob_prototypes':len(mobs),'positive_value_level30_objects':len(objects),'normal_race_solo_xp_cap':xp_cap,'combat_provenance':profile['provenance'],'combat_status':'SCREENING ONLY — additional item passives are not modeled; native representative builds, active rotations and group stacking remain required. Owner usability review is already accepted.','catalog':defs,'economy':economy,'combat_screen':rows}
    args.output.mkdir(parents=True,exist_ok=True)
    (args.output/'results.json').write_text(json.dumps(result,indent=2)+'\n')
    for name,data in [('economy',economy),('combat_screen',rows),('level30_mobs',mobs),('level30_items',objects)]:
        with (args.output/(name+'.csv')).open('w',newline='') as stream:
            writer=csv.DictWriter(stream,fieldnames=data[0]);writer.writeheader();writer.writerows(data)
    print(json.dumps({k:result[k] for k in ['eligible_spawned_mob_prototypes','positive_value_level30_objects','normal_race_solo_xp_cap','combat_status']},indent=2))
    print('Screening failures:',sum(not r['screen_pass'] for r in rows),'/',len(rows));print('Artifacts:',args.output)

if __name__=='__main__':main()
