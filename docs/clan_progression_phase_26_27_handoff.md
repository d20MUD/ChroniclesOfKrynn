# Clan progression Phases 26–27

Implemented behind the existing live earnings, authoritative SQL, campaign and
pilot/public gates. Defaults remain OFF. No live migration, award, process
restart or enablement was performed.

## Phase 26: ordinary kill contributions

`clan_sources.c` collects bounded, short-lived combat facts independently of
character XP, level caps, Scholar, happy hour and legendary boss eligibility.
Actual post-defense damage is captured before HP subtraction. Effective healing,
interception/mitigation, newly applied debuffs/defenses and hostile damaging
attacks use the real production hooks. Bonus damage packets cannot fabricate an
additional received attack. Controlled actors resolve to their player owner;
an owner outside the room can raise encounter difficulty but cannot claim that
out-of-room action as participation. Engaged outsiders retain their level in the
encounter even if they leave, die or disconnect before the kill.

The normal combat death path snapshots current authenticated account and accepted
SQL membership, then runs the Phase 25 query. It does not require a killing blow,
legendary boss or an uncapped character. Summoned/owned/charmed/test instances,
trivial fights, staff involvement, inactive recipients and insufficient actual
participation fail closed. Overkill is excluded. Healing and mitigation use each
supported player's non-clan maximum HP. Support/defense requires two useful
applications in different six-second combat rounds. Invalid facts or collector
capacity overflow reject the entire affected encounter. Limits: 128 concurrent
encounters, 128 owners and 1024 recent actions per encounter (old actions are pruned while retaining the full encounter level ceiling), ten-minute inactivity
expiry only after full healing and disengagement, immediate victim cleanup on free. Injured abandoned encounters retain their original difficulty ceiling. No actor pointers are retained.

SQL runs at death/acceptance and recovery, never during action collection. A
transient SQL outage does not erase participation facts or high-level outsiders;
if acceptance is unavailable when the mob dies, those unaccepted automatic facts
are discarded rather than reconstructed later. A fresh encounter UUID identifies
the occurrence; each recipient receives one durable pending/terminal receipt.

Apply **sql/20261111_clan_kill_sources_v001.sql** explicitly, offline, after core
v5 and before enabling kill earnings. This optional InnoDB source extension is
not installed implicitly. It retains account/prototype/UTC-day claims and has an
account/event unique key. Acceptance holds the existing core control lock, freezes
20 full tuples (2 XP/1 Influence/1 Favor), then 20 half tuples, then zero, and
commits the source claim and pending receipt together. Account limits span clans.
Retries retain the original frozen tuple, including when the caller uses another
operation nonce. A lost acceptance acknowledgement is recovered through the
existing SQL pending receipts. The regular personal award service still applies
weekly source/account/Favor/clan limits atomically.

## Phase 27: completed supply orders

The only gameplay hook is at the end of `complete_supply_order`, after the existing
quartermaster, active-order and quantity checks, normal gold/crafting XP/Artisan
rewards, cooldown accounting and order clearing. Producing individual items does
not award clan credit. Repeating completion after the order is cleared does
nothing. Existing rewards and cooldown values are unchanged.

Accepted membership is captured just before turn-in mutations. The successful
completion gets a fresh immutable event ID, occurrence time, account and clan
UID. Player tags `ClSE/ClSU/ClSA/ClST` checkpoint these facts together with the
cleared order and normal rewards. SQL acceptance is attempted only after
`save_char_checked` succeeds. Once SQL has accepted the receipt, clearing the
player marker is safe: an interrupted/lost acknowledgement replays the same ID.
A failed save awards no clan credit. An unaccepted marker for a provably tombstoned clan is cleared without awarding or redirecting it. Departure after the saved occurrence does
not redirect or erase it. One unresolved completion marker per character bounds
the file-to-SQL handoff; later normal turn-ins continue without overwriting that
marker or manufacturing a backlog. The fixed tuple is 100 XP/50 Influence/50
Favor before the existing account, weekly (three supply orders), Favor and clan
caps. Character reward bonuses never multiply this tuple.

Every 30 seconds the heartbeat retries saved completion markers for connected
players and a rotating page of up to 100 already accepted SQL receipts. Offline
accepted receipts are included. Saved player markers resume when the character
returns. Disabling live earnings pauses new acceptance/decisions; facts already
accepted remain durable. Failed/ambiguous commits retain their original identity.

## Validation

- `tests/run_clan_kill_sources.py`: disposable MariaDB plus ASan/UBSan production
  repository/personal-policy/live-adapter integration; full/half/zero boundaries,
  UTC/prototype reset, cross-clan account quota, concurrent boundary acceptance,
  alt/event dedup, replay/new nonce and lost acknowledgement; capped players,
  support/heal/tank/pet paths, exclusions, duplicate death, supply save failure
  and saved-marker replay.
- `tests/run_supply_order_project.py`: actual crafting/turn-in functions across
  four item types and seven skills, no early credit, wrong room, unfinished
  quantity, successful normal rewards/cooldown/clear and duplicate completion.
- Shared combat and personal award regressions; estate/legendary effects and
  source regressions; Make and CMake game builds.

Next: Phase 28 delivery contract definitions. Do not enable contracts or later
systems as part of this handoff.
