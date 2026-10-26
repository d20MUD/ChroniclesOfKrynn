# Dragonmetal Awakening — Phase 5 earnings and participation

Status: implemented and tested. Earning hooks become active with schema v3.
No live migration, boss registration, world edits or server restart performed.
Player commands, smith interaction, equipment benefits and active powers remain
in their later phases.

Related: [implementation plan](LEGENDARY_OBJECT_IMPLEMENTATION_PLAN.md),
[design](LEGENDARY_OBJECT_SYSTEM_PROPOSAL.md),
[transaction/recovery contract](LEGENDARY_OBJECT_PHASE_4_TRANSACTIONS.md).

## Files and migration

[API](../../src/legendary_earnings.h),
[implementation](../../src/legendary_earnings.c),
[regression tests](../../tests/legendary_earnings.c),
[test runner](../../tests/run_legendary_earnings.py).

Apply these explicit migrations in order through the normal offline deployment:

1. [Base schema v1](../../sql/20261023_legendary_objects.sql).
2. [Transaction schema v2](../../sql/20261024_legendary_transactions.sql).
3. [Earnings schema v3](../../sql/20261024_legendary_earnings.sql).

The new InnoDB `legendary_awards` table records accepted XP/boss source events,
requested and received amounts, source prototype/quest, UTC occurrence time,
essence type, linked operation, and pending/credited/notified state. Its primary
key is **character ID + source kind + source UUID**. Runtime requires schema v3
and all 12 legendary tables. All three scripts are idempotent on MariaDB.

There is no retroactive XP conversion. A personal zero-balance ledger is created
on first use, not from account XP or historical character XP. **As superseded by
schema v8**, boss materials require the native `MOB_LEGENDARY_BOSS` mobile flag,
not a database registry entry. This phase does not invent or seed a boss list;
content flagging remains a later milestone.

## Infusion XP

`gain_exp()` remains the ordinary context-free API and earns no infusion XP.
`gain_exp_with_source()` accepts a server-generated context and requires a
matching award mode. The credit hook runs **after all ordinary modifiers and
per-award caps and the ordinary XP addition, before stage advancement**. It
credits the final positive amount while the character is exactly level 30,
independently of equipment count. Normal XP, levels, losses and return values
are unchanged.

| Route | Infusion XP policy |
| --- | --- |
| Solo/group PvE kill | Final award once per character and victim instance. Controlled pet kills use the existing owner/group XP routing. |
| Automatic quest completion | Final award once per quest vnum, character and UTC day, including repeats. Claims use the original completion day even after recovery. |
| Successful trap disarm | Final award once per trap-disarm instance. Detection/search/automatic-search XP remains excluded. Trap copies receive fresh runtime source identity. |
| Scripts, missions using generic QUEST mode, crafting/reforging, damage XP, energy drain, dump rewards, default/generic calls, staff adjustments, refunds, transfers and PvP | Excluded. Recognized automatic quest completion remains eligible when initiated by a script. |

Summoned, charmed, player-owned, staff-loaded and explicitly marked test victims
are excluded. An NPC that follows a master or receives charm is marked owned
for that instance, so dismissing or uncharming it does not make it farmable.
Fresh world spawns get fresh runtime identity; prototype copying cannot duplicate
a previous instance's claims or participation.

The saved infusion-XP ceiling is **2,000,000,000**. Excess is discarded and
reported, without altering ordinary XP. Death never subtracts infusion XP.
A full balance still consumes the source receipt/quest claim, so spending later
cannot replay an earlier overflowed award.

## Materials and participation

Rewards per eligible character are deterministic:

| Native level tier | Fragments | Rolled essence type | Catalysts |
| --- | ---: | ---: | ---: |
| I | 10 | 2 | 0 |
| II | 20 | 4 | 0 |
| III | 30 | 6 | 1 |

At death, recipients must be level 30, alive with positive HP, connected and
playing, in the boss's room, and either the credited solo killer or in the
credited killer's same-room group. A controlled pet credits its player owner;
pets do not receive balances. No absent, dead or linkdead recipient is queued.
Forced staff kills and test/owned instances do not qualify.

Participation is tied to a runtime **instance/encounter UUID**, never just a
prototype or room. Each recipient needs at least **three effective actions in
the final 30 seconds**. Every real qualifying packet counts, so ordinary
multi-attack combat does not require artificial waiting; one incidental action
still cannot qualify. A participant's stale activity resets after 30 seconds;
entire inactive encounters expire after 180 seconds and are swept by the mobile
heartbeat.

Connected gameplay hooks record:

- Effective HP damage dealt to the boss, including controlled-pet damage.
- Actual HP restored to an engaged participating ally, capped to missing normal
  HP. Fully overhealed or resource-only healing does not count.
- Actual damage interception through Divine Sacrifice, Life Link and Fortress
  Engine, credited to the interceptor or construct owner.
- Successfully applied new hostile spell/power debuffs or controls through the
  common affect path. Resisted/failed applications do not count.
- Restoring a missing defensive affect on an engaged ally: armor/shield/faith,
  stoneskin/ironskin/barkskin, epic armor/warding, sanctuary, blur/mirror image/
  displacement, mind blank, protections from alignment/energy/arrows/poison,
  elemental/spell resistance, resistance, death ward and shield of fortification.
  Group/communal spells use the existing per-recipient spell path. Already active
  duplicates do not count.

Support requires the ally to be fighting the boss or to have recent qualifying
activity against that same encounter. It does not inherit activity from a
boss-owned add. There is no group damage-percentage threshold. Additional direct
ability routes can use the same actual-effect adapters; source callers must not
report attempts as successes.

Boss eligibility is the authoritative native `MOB_LEGENDARY_BOSS` flag. Tier is
derived from the flagged mobile's level, and the award rolls one essence type
once before the durable award record is written. No boss classification is
inferred from name, race, level or XP value alone.

## Caps, dates, cooldowns and exchanges

Daily acquisition caps per **character**, not account:

- 100 fragments.
- 20 essences across all six types combined.
- 1 catalyst.

Dates use integer UTC days and SQL date arithmetic independent of the database
session timezone. At the first acquisition after midnight, counters reset lazily;
read-only balance inspection shows the reset without needing a write. A future
saved allowance date fails closed on clock rollback. Balances carry over, each
up to **2,000,000,000**. Spending, death, reconnect and reboot never refund
acquisition counters.

One central output-clamp routine and transaction participant handle boss awards
and exchange outputs. Each material is limited independently by remaining daily
allowance and its own balance capacity. Counters increase only by what was
actually received. Reports distinguish full, partial and zero material awards,
include the essence type, and are sent after durable completion.

Any positive material payout starts a **rolling 20-hour per-prototype cooldown**.
Clones share it; different prototypes do not. A zero payout records the unique
death claim but starts no cooldown. Reprocessing that same zero-paid death later
cannot earn anything. Uncommitted material source events from a closed UTC day
are resolved with zero materials instead of using the next day's allowance.

The backend exchange API returns expiring opaque quotes, their operation ID and
exact delta, for the Phase 6 frontend:

- 3 essences of one type for 1 of a different type.
- 100 fragments and 20 mixed essences for 1 catalyst.

Quantities must fit the entire output allowance/capacity and all inputs. No
partial exchanges, reverse conversion, gold purchase or XP purchase are allowed.
The default mixed payment spends the most abundant type first, fixed essence ID
breaking ties. Output consumes allowance; input spending never refunds allowance.
Combat, stale versions, repeat confirmations, expired quotes and a changed UTC
day reject confirmation without consuming inputs. Smith-presence checks and the
player command/menu remain Phase 6 work.

## Persistence and recovery

An eligible event is first inserted idempotently into `legendary_awards`.
Subsequent processing uses the Phase 4 serialized transaction path. One SQL
transaction updates balances, UTC counters, quest/death claims and the event's
credited receipt. The participant rechecks the wallet snapshot version and UTC
day under the character lock. Concurrent or interrupted grants cannot overspend
allowance or claim a source twice.

A failure after source acceptance leaves the event durable. Prepared operations
cancel without deductions; source recovery can then make a fresh attempt.
Committed operations reconcile before notification, without replaying their
participant. Login scans durable events; during play an ephemeral pending flag
avoids polling SQL on every ordinary command. Pending recovery blocks the next
command. Successful notifications are acknowledged separately after transaction
completion; source rows remain audit history.

Missing/unmigrated schema leaves ordinary play intact. If initial source
insertion itself fails, no legendary credit is applied or announced; write errors
identify the source. An event is only accepted once that initial SQL write is
durable. Accepted XP is recoverable; capped overflow and closed-day materials
are intentionally discarded. No gold journal or competing absolute-gold replay
was added.

## Validation

```sh
python3 tests/run_legendary_earnings.py
ASAN_OPTIONS=detect_leaks=0 \
ESTATE_TEST_CFLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer -g' \
python3 tests/run_legendary_earnings.py
```

The runner uses disposable MariaDB only and reruns housing, auction, stalls,
legendary persistence and Phase 4 transaction regressions. It compiles the real
`gain_exp` pipeline, effective-healing function and production earning/transaction/codec/save modules into
the fixture. Controlled stubs supply surrounding XP bonuses and world state;
the XP calculation and earning logic are not duplicated in the tests.

Coverage includes final XP bonuses/caps, excluded modes and PvP, level-29 stage
advancement, daily quest claims, disarm replay, XP overflow, healer/interceptor/
pet-owner credit, actual healing versus overheal/resource-only effects, tag spam, stale/expired encounters, absent/dead/linkdead/wrong-
group recipients, clone cooldowns, distinct prototypes, partial caps, zero-paid
cooldowns, concurrent accepted kills, UTC reset, closed-day recovery, exact
exchanges, competing quotes, overflow, configuration changes, saved receipt
reloads and interrupted prepared/committed grant recovery. Characters' claims
and allowances remain independent.

AddressSanitizer and UndefinedBehaviorSanitizer pass. Leak detection is disabled
for the shared fixture's retained mock world, as in earlier phases. Full Make
and CMake builds also pass. No live data was migrated or modified.
