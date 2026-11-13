#!/usr/bin/env python3
"""Reproducible, conservative pacing proposal. Does not change game balances.

Death-XP-only baseline: damage XP and optional bonuses shorten real leveling.
Points/rewards use existing eighth-point precision. Rank progress is lifetime
Influence earned, separate from Influence spent on buffs.
"""
import argparse
import csv
from fractions import Fraction
import json
from math import ceil
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]


def read_define(path, name):
    m = re.search(r'^#define\s+'+re.escape(name)+r'\s+(\d+)\b', path.read_text(), re.M)
    if not m:
        raise ValueError('Missing production constant '+name)
    return int(m[1])


def build(args):
    limits = ROOT/'src/limits.c'
    # These are the actual normal-race production formulas, not stored mob XP.
    assert 'factor = 2000 + (level - 2) * 800;' in (ROOT/'src/class.c').read_text()
    assert 'GET_EXP(mob) = (level * level * 75);' in (ROOT/'src/medit.c').read_text()
    cap_bands = [read_define(limits, 'MIN_NUM_MOBS_TO_KILL_'+str(n)) for n in [5, 10, 15, 20, 25]]
    newbie_level = read_define(ROOT/'src/structs.h', 'NEWBIE_LEVEL')
    newbie_percent = read_define(limits, 'NEWBIE_EXP')
    max_gain = 100000
    experience_multiplier = 100
    if args.game_config:
        match = re.search(r'^max_exp_gain\s*=\s*(\d+)', args.game_config.read_text(), re.M)
        if not match:
            raise ValueError('Missing max_exp_gain in game config')
        max_gain = int(match[1])
        multiplier = re.search(r'^experience_multiplier\s*=\s*(\d+)', args.game_config.read_text(), re.M)
        if multiplier:
            experience_multiplier = int(multiplier[1])
    afk = Fraction(args.afk_share)
    if not 0 <= afk < 1 or args.fight_seconds <= 0 or args.fights_per_rest <= 0 or args.rest_seconds < 0:
        raise ValueError('Invalid combat/rest/AFK assumptions')
    if min(args.favor_eighths, args.influence_eighths, args.reference_accounts) <= 0:
        raise ValueError('Point rates and reference account count must be positive')
    if min(Fraction(args.favor_time), Fraction(args.clan_time)) <= 0:
        raise ValueError('Pacing multipliers must be positive')
    fights_hour = Fraction(3600*args.fights_per_rest, args.fight_seconds*args.fights_per_rest+args.rest_seconds)*(1-afk)
    favor_rate, influence_rate = Fraction(args.favor_eighths, 8), Fraction(args.influence_eighths, 8)
    favor_hour, influence_hour = fights_hour*favor_rate, fights_hour*influence_rate
    def xp(level):
        return (level-1)**2*(2000+(level-2)*800)*experience_multiplier//100
    rows, kills = [], Fraction(0)
    for level in range(1, 31):
        f = ceil(kills*Fraction(args.favor_time)*favor_rate)
        i = ceil(kills*Fraction(args.clan_time)*influence_rate*args.reference_accounts)
        row = dict(level=level, cumulative_character_xp=xp(level),
                   character_logged_hours=float(kills/fights_hour), equivalent_kills=float(kills),
                   favor_threshold=f, favor_logged_hours=float(Fraction(f)/favor_hour),
                   influence_threshold=i, influence_contributor_hours=float(Fraction(i)/influence_hour),
                   influence_clan_hours_at_reference=float(Fraction(i)/influence_hour/args.reference_accounts))
        if level < 30:
            delta = xp(level+1)-xp(level)
            band = 0 if level < 11 else 1 if level < 16 else 2 if level < 21 else 3 if level < 26 else 4
            raw = 25*level*level
            boosted = raw+raw*newbie_percent//100 if level <= newbie_level else raw
            award = min(boosted, delta//cap_bands[band], max_gain)
            stage_kills = Fraction(delta, award)
            row.update(xp_to_next_level=delta, same_level_mob_stored_xp=75*level*level,
                       same_level_kill_award=award, kills_to_next_level=float(stage_kills),
                       logged_hours_to_next_level=float(stage_kills/fights_hour))
            kills += stage_kills
        rows.append(row)
    # Upward-rounded rank increments also protect the requested ratio at each
    # corresponding reference-level interval, not only at the final threshold.
    def ladder(levels, rate, multiplier):
        result, total = [], 0
        previous_kills = Fraction(0)
        exact_kills = [Fraction(0)]
        for row in rows[:-1]:
            exact_kills.append(exact_kills[-1]+Fraction(row['xp_to_next_level'], row['same_level_kill_award']))
        for rank, level in enumerate(levels, 1):
            current_kills = exact_kills[level-1]
            increment = ceil((current_kills-previous_kills)*rate*multiplier)
            total += increment
            result.append(dict(rank=rank, reference_character_level=level, threshold=total,
                               increment=increment, logged_contributor_hours=float(Fraction(total)/rate/fights_hour)))
            previous_kills = current_kills
        return result
    favor_levels = [1, 6, 12, 18, 24, 30]
    influence_levels = [1+ceil(n*29/19) for n in range(20)]
    favor = ladder(favor_levels, favor_rate, Fraction(args.favor_time))
    for row, name in zip(favor, ['Unproven', 'Recognized', 'Trusted', 'Proven', 'Distinguished', 'Exalted']):
        row['name'] = name
    influence = ladder(influence_levels, influence_rate, Fraction(args.clan_time)*args.reference_accounts)
    for row in influence:
        row['clan_hours_at_reference'] = row['logged_contributor_hours']/args.reference_accounts
    return dict(assumptions=dict(fight_seconds=args.fight_seconds, fights_per_rest=args.fights_per_rest,
        rest_seconds=args.rest_seconds, additional_afk_share=float(afk), kills_per_logged_hour=float(fights_hour),
        favor_per_kill=float(favor_rate), influence_per_kill=float(influence_rate), favor_time_multiplier=float(Fraction(args.favor_time)),
        clan_time_multiplier=float(Fraction(args.clan_time)), reference_accounts=args.reference_accounts,
        character_race='ordinary race, no XP surcharge', xp_baseline='same-level ordinary auto-stat mob, death XP only',
        max_exp_gain=max_gain, experience_requirement_percent=experience_multiplier), comparison=rows, favor_ranks=favor, influence_ranks=influence)


def write(output, data):
    output.mkdir(parents=True, exist_ok=True)
    (output/'pacing.json').write_text(json.dumps(data, indent=2)+'\n')
    for name, rows in [('level_comparison', data['comparison']), ('favor_ranks', data['favor_ranks']), ('influence_ranks', data['influence_ranks'])]:
        fields = list(dict.fromkeys(k for row in rows for k in row))
        with (output/(name+'.csv')).open('w', newline='') as file:
            writer = csv.DictWriter(file, fieldnames=fields)
            writer.writeheader()
            writer.writerows(rows)


def parser():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--game-config', type=Path)
    p.add_argument('--fight-seconds', type=int, default=30)
    p.add_argument('--fights-per-rest', type=int, default=10)
    p.add_argument('--rest-seconds', type=int, default=300)
    p.add_argument('--afk-share', default='0.5')
    p.add_argument('--favor-time', default='2')
    p.add_argument('--clan-time', default='10')
    p.add_argument('--favor-eighths', type=int, default=2)
    p.add_argument('--influence-eighths', type=int, default=1)
    p.add_argument('--reference-accounts', type=int, default=3)
    return p


if __name__ == '__main__':
    args = parser().parse_args()
    data = build(args)
    write(args.output, data)
    print(json.dumps(dict(assumptions=data['assumptions'], level_30=data['comparison'][-1],
                         top_favor=data['favor_ranks'][-1], top_influence=data['influence_ranks'][-1]), indent=2))
