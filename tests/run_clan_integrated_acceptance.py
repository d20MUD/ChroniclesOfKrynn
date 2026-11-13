#!/usr/bin/env python3
"""Phases 48–49: one private database, production services and restartable adapters.

Fixtures enable only this temporary database. No game configuration is changed.
"""
from clan_database import ROOT, database
from clan_snapshot_fixture import snapshot, manifest, A
import json
import shlex
import subprocess
import sys
sys.path.insert(0, str(ROOT / 'scripts'))
from build_clan_repository_tool import build

migrations = tuple(f'20261110_clan_progression_v{i:03d}.sql' for i in range(1, 6)) + (
    '20261111_clan_buff_activations_v001.sql', '20261111_clan_buff_lifecycle_v001.sql',
    '20261111_clan_kill_sources_v001.sql', '20261111_clan_delivery_v001.sql',
    '20261111_clan_objectives_v001.sql', '20261112_clan_expeditions_v001.sql',
    '20261112_clan_expeditions_v002.sql', '20261112_clan_quartermaster_v001.sql')
def run(after=None):
    with database(migrations) as (tmp, socket, sql, client):
        tool = tmp / 'repository'
        build(tool, sanitize=True)
        subprocess.run([str(tool), 'import', 'clan_test', 'root', '', str(socket)],
                       input=json.dumps(dict(snapshot=snapshot(), expected_revision=0, manifest=manifest())),
                       text=True, check=True, stdout=subprocess.DEVNULL)
        sql('UPDATE clan_store_control SET store_mode=2;UPDATE clan_import_manifests SET import_status=2')
        sql(f"UPDATE clan_progression_state SET xp_eighths=304000,influence_eighths=20000 WHERE clan_uid='{A}'")
        flags = shlex.split(subprocess.check_output(['pkg-config', '--cflags', '--libs', 'mariadb', 'openssl', 'json-c'], text=True))
        common = ['clan_repository.c', 'clan_progression.c', 'clan_awards.c']
        def compile_native(name, driver, sources, defines=()):
            path = tmp / name
            if True:  # Exercise current rules in this integrated acceptance.
                current = tmp / (name + '.c')
                current.write_text(driver.read_text().replace('clan_progression_v1', 'clan_progression_defaults'))
                driver = current
            subprocess.run(['cc', '-std=gnu99', '-Wall', '-Wextra', '-Werror', '-g',
                            '-fsanitize=address,undefined', '-fno-pie', '-no-pie', '-Isrc',
                            *['-D' + d for d in defines], str(driver),
                            *[str(ROOT / 'src' / s) for s in sources], *flags, '-o', str(path)], cwd=ROOT, check=True)
            return path
        # Use the production final HP function with the existing world's isolated adapters.
        hp_source = (ROOT / 'src/legendary_foundations.c').read_text()
        start = hp_source.index('void legendary_foundation_hp(')
        hp_function = hp_source[start:hp_source.index('\n}', start) + 2]
        driver = tmp / 'buff.c'
        driver.write_text((ROOT / 'tests/clan_buff_lifecycle.c').read_text() + '\n#define time clan_test_time\n' + hp_function + '\n#undef time\n')
        buffs = compile_native('buffs', driver, common + ['clan_buff_queries.c', 'clan_buff_activation.c',
            'clan_buff_commit.c', 'clan_buff_lifecycle.c', 'clan_buff_runtime.c', 'clan_status.c', 'clan_buff_commands.c'], ['CLAN_BUFF_LIFECYCLE_TEST'])
        def call(binary, operation, body, expected='ok', key='data', crash=None):
            args = [str(binary), str(socket)] + ([] if binary == buffs else [operation])
            p = subprocess.run(args, input=json.dumps(body | ({'mode': operation} if binary == buffs else {})), text=True, capture_output=True)
            if crash is not None:
                assert p.returncode == crash, (operation, p.returncode, p.stdout, p.stderr)
                return
            assert p.returncode == 0, (operation, p.stdout, p.stderr)
            out = json.loads(p.stdout)
            if operation == 'acceptance-probe':
                return out
            assert out['result'] == expected, (operation, body, out, p.stderr)
            return out.get(key)
        clock = 2200000001
        serial = 10000
        def uid():
            nonlocal serial
            serial += 1
            return f'{serial:032x}'
        def buff(operation, **kw):
            expected = kw.pop('result', 'ok')
            return call(buffs, operation, dict(now=clock) | kw, expected, 'receipt')
        def probe(hp=80, **kw):
            return call(buffs, 'acceptance-probe', dict(now=clock, hp=hp) | kw)
        def state():
            return sql('SELECT * FROM clan_progression_state ORDER BY clan_uid;SELECT * FROM clan_buff_families ORDER BY clan_uid,family_id;SELECT * FROM clan_buff_activations ORDER BY operation_uid')
        original_start = clock
        first = uid()
        buff('activate', operation_uid=first, family=4)
        paid = state()
        assert int(sql(f"SELECT next_available_at FROM clan_buff_families WHERE clan_uid='{A}' AND family_id=4")) == original_start + 129600
        buff('activate', operation_uid=first, family=4)
        assert state() == paid
        assert probe() == dict(hp=80, maximum=136, vitality=5, scholar=0, expires_at=clock+86400)
        # Repeated fresh processes model reconnect/copyover hydration of saved HP/timers.
        for _ in range(3):
            assert probe(136)['hp'] == 136 and state() == paid
        sql(f"INSERT INTO clan_memberships VALUES(5000000010,'{A}',2,0);INSERT INTO clan_buff_management VALUES('{A}',21,2)")
        buff('activate', operation_uid=uid(), family=4, actor=5000000010, slot=2, result='ineligible')
        assert state() == paid
        for change in [dict(tier=2), dict(slot=2), dict(tier=3, slot=3)]:
            buff('activate', operation_uid=uid(), family=4, result='ineligible', **change)
            assert state() == paid
        replacement = uid()
        buff('activate', operation_uid=replacement, family=1, expected_activation_uid=first)
        replaced = state()
        assert probe(136)['maximum'] == 130 and probe(136)['hp'] == 130
        assert probe(80)['scholar'] == 5
        buff('activate', operation_uid=replacement, family=1, expected_activation_uid=first)
        assert state() == replaced
        buff('activate', operation_uid=uid(), family=2, expected_activation_uid=first, result='ineligible')
        assert state() == replaced
        # Leaving loses the effect; rejoining restores eligibility but never heals or resets time.
        sql('DELETE FROM clan_memberships WHERE player_id=5000000007')
        assert probe()['scholar'] == 0
        sql(f"INSERT INTO clan_memberships VALUES(5000000007,'{A}',1,101)")
        assert probe()['scholar'] == 5
        clock += 86400
        assert probe(136)['maximum'] == 130 and probe(136)['scholar'] == 0
        expired = state()
        buff('activate', operation_uid=uid(), family=4, tier=2, result='ineligible')
        assert state() == expired
        clock += 21600  # Restart at T+30h: six hours remain, no new duration.
        assert probe()['vitality'] == 0
        assert int(sql(f"SELECT next_available_at FROM clan_buff_families WHERE clan_uid='{A}' AND family_id=4")) - clock == 21600
        buff('activate', operation_uid=uid(), family=4, tier=3, result='ineligible')
        clock += 21599
        buff('activate', operation_uid=uid(), family=4, tier=2, result='ineligible')
        clock += 1
        tier2 = uid()
        buff('activate', operation_uid=tier2, family=4, tier=2)
        assert probe()['maximum'] == 143 and probe()['hp'] == 80
        cancel = uid()
        buff('cancel', operation_uid=cancel, activation_uid=tier2)
        canceled = state()
        buff('cancel', operation_uid=cancel, activation_uid=tier2)
        assert state() == canceled and probe(143)['hp'] == 130
        buff('activate', operation_uid=uid(), family=4, tier=3, result='ineligible')
        clock += 129600
        tier3 = uid()
        buff('activate', operation_uid=tier3, family=4, tier=3)
        assert probe()['maximum'] == 149 and probe()['hp'] == 80
        assert probe(149, disabled=True)['hp'] == 130
        # Re-enabling/reconnecting cannot heal the HP removed by disabling.
        assert probe(130)['maximum'] == 149 and probe(130)['hp'] == 130
        influence = int(sql(f"SELECT influence_eighths FROM clan_progression_state WHERE clan_uid='{A}'"))
        assert influence == 20000 - (13 + 13 + 19 + 29) * 8, influence
        print('Phase 48: integrated activation/replacement/expiry/tier locks/departure/restart/HP and exact charges passed.', flush=True)

        # Phase 49 uses the same database, immutable IDs and read model, with a small
        # clan starting at level one. Player levels are independent of clan level.
        rewards = compile_native('rewards', ROOT/'tests/clan_rewards.c', common, ['CLAN_REWARD_TEST'])
        expeditions = compile_native('expeditions', ROOT/'tests/clan_expedition_enrollment.c',
            common + ['clan_combat.c', 'clan_expeditions.c', 'clan_expedition_enrollment.c'], ['CLAN_REWARD_TEST'])
        quartermaster = compile_native('quartermaster', ROOT/'tests/clan_quartermaster.c', common + ['clan_quartermaster.c'])
        web = compile_native('web', ROOT/'tests/clan_web.c', common + ['clan_web.c', 'clan_web_content.c',
            'clan_quartermaster.c', 'clan_delivery.c', 'clan_objectives.c', 'clan_expedition_enrollment.c',
            'clan_expeditions.c', 'clan_combat.c', 'clan_buff_queries.c', 'clan_buff_activation.c',
            'clan_buff_commit.c', 'clan_buff_lifecycle.c'])
        clock += 604800
        sql(f"UPDATE clan_progression_state SET xp_eighths=0,influence_eighths=0,revision=revision+1 WHERE clan_uid='{A}'")
        low = dict(player=1007, account=77, level=5, kind=0)
        high = dict(player=1010, account=88, level=30, kind=3)
        alt = dict(player=1012, account=77, level=5, kind=0)
        healer = dict(player=1011, account=88, level=5, kind=1)
        sql(f"INSERT INTO clan_memberships VALUES(1007,'{A}',2,0),(1010,'{A}',2,0),(1012,'{A}',2,0),(1011,'{A}',2,0)")
        def read(m=low):
            return call(web, 'read', dict(clan=A, now=clock) | m)
        initial = read()
        assert initial['clan']['level'] == 1
        week = initial['contribution']['week_ends_at'] - 604800
        def earned_state():
            return sql('SELECT * FROM clan_progression_state ORDER BY clan_uid;SELECT * FROM clan_personal_favor ORDER BY player_id,clan_uid;SELECT * FROM clan_account_windows ORDER BY account_id,starts_at_utc;SELECT * FROM clan_weekly_windows ORDER BY clan_uid,starts_at_utc')
        def earning(m, source=1, **changes):
            amount = (16, 8, 8) if source == 1 else (800, 400, 400)
            return dict(operation_uid=uid(), event_uid=uid(), clan_uid=A, player_id=m['player'],
                account_id=m['account'], source=source, rules_version=2, occurred_at=clock, now=clock,
                prototype=4301, requested_xp_eighths=amount[0], requested_influence_eighths=amount[1],
                requested_favor_eighths=amount[2]) | changes
        def award(e):
            prepared = call(rewards, 'prepare-kill' if e['source'] == 1 else 'prepare-member', e, key='receipt')
            return call(rewards, 'personal', e | dict(operation_uid=prepared['event']['operation_uid']), key='receipt')
        event = earning(low)
        first_award = award(event)
        after_first = earned_state()
        assert award(event) == first_award and earned_state() == after_first
        assert read()['clan']['xp_eighths'] == '1' and read()['favor']['eighths'] == '2'
        # A max-level character still contributes without a character-XP grant.
        high_award = award(earning(high))
        assert high_award['decision']['delta']['xp_eighths'] == 1
        # An alt uses the same account window and prototype/day quota.
        award(earning(alt))
        assert read(alt)['contribution'] == read(low)['contribution']
        assert read(alt)['favor']['eighths'] == '2'
        # Bring the account just below its full-XP/Favor edges, then cross with one
        # real supply-order tuple. The seed is cap setup, not an earned achievement.
        sql(f"UPDATE clan_account_windows SET base_xp_eighths=4792,favor_eighths=1192,revision=revision+1 WHERE account_id=77 AND starts_at_utc={week}")
        edge = award(earning(low, 2))
        delta = edge['decision']['delta']
        assert delta['xp_eighths'] == 25 and delta['favor_eighths'] == 8, delta
        alt_award = award(earning(alt, 2))
        assert alt_award['decision']['delta']['xp_eighths'] == 25
        assert alt_award['decision']['delta']['favor_eighths'] == 0
        assert read()['contribution']['favor_eighths'] == '1200'
        assert read(alt)['contribution'] == read()['contribution']
        # Shared source quota remains account-wide, despite a different character.
        award(earning(low, 2))
        capped = award(earning(alt, 2))
        assert capped['decision']['delta']['xp_eighths'] == 0 and capped['decision']['delta']['favor_eighths'] == 0
        # Expensive purchases and legacy points cannot manufacture earned standing.
        qm_catalog = json.loads((ROOT/'lib/clan/quartermaster.json').read_text())
        request = dict(catalog=qm_catalog, index=0, uid=uid(), clan=A, player=alt['player'], account=77,
            level=30, gold=1000000000, payload='isolated bound payload', now=clock)
        before = earned_state()
        call(quartermaster, 'prepare', request, expected='ineligible')
        assert earned_state() == before
        # A two-account level-five party can complete the actual level-five catalog
        # choice with damage and effective healing; a level-thirty member cannot
        # bypass this outing's band. Neither can a same-account alt fill a slot.
        catalog = json.loads((ROOT/'lib/clan/expeditions.json').read_text())
        definition = next(d for d in catalog['expeditions'] if d['family'] == 1 and d['min_player_level'] == 5)
        outing = uid()
        exp_base = dict(catalog=catalog, definition=definition['id'], outing=outing, clan=A, now=clock)
        def exp(op, m=low, expected='ok', **changes):
            return call(expeditions, op, exp_base | m | dict(now=clock) | changes, expected)
        exp('create')
        exp('join', high, expected='ineligible')
        exp('join', alt, expected='conflict')
        exp('join', healer)
        exp('start', members=[low, healer])
        opening = exp('read')
        deadline = opening['expires_at']
        for _ in range(2):
            exp('reconcile')
            assert exp('read')['expires_at'] == deadline
        def encounter(objective):
            nonlocal clock
            clock += 1
            target = definition['objectives'][objective]['targets'][0]
            return dict(event=uid(), target=target['mob'], area=target['area'], victim_level=5,
                        members=[low, healer], now=clock)
        first_event = encounter(0)
        exp('combat', **first_event)
        exp('combat', **first_event)
        assert exp('progress') == [1, 0, 0]
        for objective, quota in enumerate([19, 1, 1]):
            for _ in range(quota):
                event = encounter(objective)
                exp('combat', **event)
        assert exp('progress') == [20, 1, 1]
        shared_uid = sql(f"SELECT operation_uid FROM clan_expedition_shared_claims WHERE clan_uid='{A}' AND week_start={week}")
        shared = exp('complete', operation=shared_uid)
        assert shared['decision']['delta']['xp_eighths'] == 300
        assert shared['decision']['delta']['influence_eighths'] == 300
        shared_after = earned_state()
        assert exp('complete', operation=shared_uid) == shared and earned_state() == shared_after
        # The capped account earns a durable zero-Favor claim; the second account
        # earns its full personal reward. Duplicate claims cannot repeat either.
        low_claim = exp('claim')
        low_op = low_claim['event']['operation_uid']
        low_paid = exp('complete', operation=low_op)
        assert low_paid['decision']['delta']['favor_eighths'] == 0
        assert exp('claim') == low_paid
        prepared = exp('claim', healer)
        personal_uid = prepared['event']['operation_uid']
        # A process exit before COMMIT must leave the claim pending with no currency.
        before_commit = earned_state()
        call(expeditions, 'complete', exp_base | healer | dict(operation=personal_uid, now=clock, crash=7), crash=97)
        assert earned_state() == before_commit
        assert sql(f"SELECT state FROM clan_expedition_entitlements WHERE operation_uid='{personal_uid}'") == '1'
        # Kill the claiming process immediately after the durable reward COMMIT.
        # A fresh process must recover the exact receipt without another award.
        body = exp_base | healer | dict(operation=personal_uid, now=clock, crash=8)
        call(expeditions, 'complete', body, crash=98)
        after_commit = earned_state()
        recovered = exp('complete', healer, operation=personal_uid)
        assert recovered['decision']['delta']['favor_eighths'] == 200
        assert exp('complete', healer, operation=personal_uid) == recovered
        assert earned_state() == after_commit
        status = read(healer)
        assert int(status['favor']['eighths']) == 200
        assert status['contribution']['favor_eighths'] == '202'
        assert status['clan']['level'] == 1
        # A few kills and one expedition cannot shortcut the long rank ladder.
        # Seed the independent purchase gates to exercise the unlocked transaction.
        sql(f"UPDATE clan_progression_state SET xp_eighths=5184,revision=revision+1 WHERE clan_uid='{A}'")
        sql(f"UPDATE clan_personal_favor SET favor_eighths=1832,revision=revision+1 WHERE player_id={healer['player']} AND clan_uid='{A}'")
        # With the clan unlock now satisfied, gold and legacy points still cannot
        # bypass this alt's insufficient personal Favor standing.
        sql(f"UPDATE clan_memberships SET legacy_points=1000000000 WHERE player_id={alt['player']}")
        before = earned_state()
        call(quartermaster, 'prepare', request, expected='ineligible')
        assert earned_state() == before
        # An eligible fixed-price purchase records no Favor/account/currency award.
        bought = request | dict(player=healer['player'], account=healer['account'], uid=uid())
        before_purchase = earned_state()
        call(quartermaster, 'prepare', bought)
        call(quartermaster, 'prepare', bought)
        call(quartermaster, 'ack', bought)
        assert earned_state() == before_purchase
        # Private reports reflect the exact SQL results rather than client totals.
        for m in [low, alt, healer]:
            current = read(m)
            favor = sql(f"SELECT favor_eighths FROM clan_personal_favor WHERE player_id={m['player']} AND clan_uid='{A}'")
            assert current['favor']['eighths'] == favor
            assert current['clan']['xp_eighths'] == sql(f"SELECT xp_eighths FROM clan_progression_state WHERE clan_uid='{A}'")
        print('Phase 49: integrated two accounts/alt/level bands/caps/dedup/mixed-role expedition/interrupted claims/standing purchases/readouts passed.', flush=True)

        if after is not None:
            after(tmp, socket, sql, client, tool)


if __name__ == "__main__":
    run()
