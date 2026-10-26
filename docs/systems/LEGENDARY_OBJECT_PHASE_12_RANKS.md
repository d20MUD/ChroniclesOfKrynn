# Phase 12: rank-IV and rank-V integration

Completed 2026-10-24. The modifier resolver and gameplay adapters introduced in
earlier phases now have exhaustive rank integration coverage. Smith confirmation
quotes show the modifier's target and the exact resolved technique/signature
values before payment. Schema remains v5; no new migration, world edits or
server restart is required by these changes.

## Verified behavior

- Enduring, Ready and Distant aid use each power's validated capabilities. A
  channel has exactly one modifier on exactly one of its selected powers.
  Unsupported modifiers, missing powers and modifiers below rank IV are rejected
  before a quote or payment. Unchanged selections spend nothing.
- Enduring extends an eligible duration/preparation by six seconds. S05's healing
  schedule cannot be extended. S11's preparation can be extended, while its
  control remains exactly six seconds and DC25. T08 permits Enduring only at V,
  when it actually gains a timed defense benefit.
- Ready uses ceiling rounding: individual cooldowns 60/90/120/180/300 become
  51/77/102/153/255 seconds. The global six-second cooldown and Protection/
  Restorative 60-second or Control 30-second family clocks are unchanged.
- Reshaping, moving a modifier between powers and advancing rank do not rewrite
  existing cooldown timestamps. A newly selected Ready modifier applies to the
  next activation after the old cooldown ends. The other selected power retains
  its ordinary cooldown and values.
- V changes only the documented fields in the existing power. It adds no
  property, passive strength, passive budget or fourth major selection. A
  two-handed item's primary/supporting ranks remain independent, with ordinary
  advancement costs. Increasing rank does not fill current HP.
- Path/family changes invalidate incompatible powers and their modifier, clearing
  only the corresponding first-choice fee flags. Their first replacements are
  free; moving an already selected modifier costs one modifier reshape fee.
  Supporting-family changes preserve the primary channel. Valid remaining
  selections are retained.

## Validation

`tests/run_legendary_ranks.py` runs the pure rules and the disposable-MariaDB
production integration fixtures. Coverage includes:

1. All 31 powers across rank inputs -1 through 6 and modifier inputs -1 through
   4, with an independent eligibility specification, exact resolved values,
   unchanged output on rejection and exact cooldown/family values.
2. Every rank-V field compared with base values plus an independent list of
   approved changes, catching undocumented extra bonuses as well as incorrect
   documented values.
3. All legal live rank-IV and rank-V modifier combinations for T01–T18 and
   S01–S13, extending the earlier base/V adapter tests to rank IV. Target,
   recovery, source removal, charge, healing, prevention and lance tests remain.
4. Real smith commands and durable transactions for rank gating, changing the
   modifier target, exact fees, path and supporting-family invalidation, free
   replacements, rejected operations without payment, IV→V advancement,
   unchanged passive/major allocation, independent channel ranks and persisted
   individual/global/family cooldowns through reshaping and advancement.
5. Existing persistence, earnings, equipment, foundation, technique and
   signature regressions through the same integration runner.

Validation commands:

```sh
ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 \
LEGENDARY_TEST_CFLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer -g' \
ESTATE_TEST_CFLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer -g' \
python3 tests/run_legendary_ranks.py
cmake --build /tmp/krynn-legendary-phase8-cmake -j4
make -j4
git diff --check
```

The fixtures execute production catalog, command, transaction, SQL, loadout and
effect code, with deterministic native world/dice/stat adapters. This does not
claim a running-server playtest or completion of phase 13's full security,
lifecycle and performance audit, phase 14's balance review, or deployment.
