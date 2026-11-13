# Clan progression Phases 48–49

These phases add integrated acceptance coverage and run the existing production
service and game-adapter regressions. They make no balance, schema, runtime
policy or feature-default changes. All databases used are private temporary
MariaDB instances with networking disabled. No game database, running MUD,
gateway or browser deployment was changed.

## Phase 48: buff lifecycle acceptance

`tests/run_clan_integrated_acceptance.py` builds sanitizer-enabled native drivers
and carries the same SQL state through these transitions:

- Activate Company Vitality and replay its operation without another charge.
  Fresh-process login/copyover hydration preserves its absolute expiry and HP.
- Reject refreshes through another tier, slot or delegated manager without
  spending Influence or changing the retained family deadline.
- Replace Vitality with Scholar using the exact occupied activation UID. An
  identical retry is free; a stale replacement expectation cannot spend.
  Removing the HP bonus clamps current HP but does not compound stats.
- Depart and rejoin the original clan. The effect disappears on departure and
  returns only with current eligibility, without resetting timers or healing.
- Expire at T+24 hours. At T+30 hours, SQL still records six hours of cooldown.
  Reject at T+36 hours minus one second, then allow tier two at the exact edge.
- Cancel tier two without a refund or shortened lock, then activate tier three
  only at its original next-use boundary. Feature disable clamps excess HP;
  restoring eligibility never heals that lost HP.
- Assert the final Influence balance equals exactly four successful purchases:
  100 + 100 + 150 + 225 points. Rejected attempts and retries charge nothing.

The probe uses the production buff runtime, shared login restore hooks and the
actual `legendary_foundation_hp` function with isolated world/HP adapters. Each
invocation is a fresh process. This models durable restart/copyover state;
it is not a live process copyover or a browser-driven playtest.

Prepared/spontaneous saved-cycle, Psionic fractional recovery and Artisan
production-check regressions also pass.

`tests/run_clan_buff_lifecycle.py` also passes the existing concurrent expiry,
SQL fault/process-exit, cancellation, offline recovery, command delegation and
all-five-family runtime checks. Its HP checks cover serialization, equipment
restore, integer bounds, negative HP, outages and no repeated healing.

## Phase 49: progression acceptance

The integrated driver then initializes the progression scenario's level-one
clan balances in the same isolated database. Test-only boundary setup is
explicit; it is not presented as an earned event. The production services verify:

- Two accounts, a level-five member, a same-account alt, a max-level contributor
  and a second account's low-level healer use the same account-wide limits.
  Personal Favor remains separate for each character/clan incarnation.
- Duplicate kill receipt/event submission does not change balances. A max-level
  character can still contribute. Supply awards cross the full/half XP boundary
  precisely; the shared Favor cap and three-order source limit hold across alts.
- Large carried gold and legacy membership points cannot bypass insufficient
  earned standing. An eligible fixed-price purchase and its retry/acknowledgement
  do not alter Favor, earned currencies or contribution counters.
- Two distinct accounts in the real level-five Stronghold catalog choice finish
  with damage and effective-healing participation. A high-level outsider cannot
  bypass the outing band; a same-account alt cannot fill the second account slot.
- Reconciliation retains outing IDs/deadlines. Duplicate encounters advance once.
  The shared completion pays 600 Clan XP and 300 Influence once, independently
  of personal claims. This small clan progresses beyond level one.
- The Favor-capped participant's zero-credit personal claim still has a durable
  identity and repeat limit. The other participant earns 100 Favor. Process exits
  before and after the claim COMMIT respectively roll back or recover exactly;
  neither recovery repeats currency.
- Private web read-model balances agree exactly with SQL Favor and Clan XP for
  both accounts and the alt.

The existing expedition progress/recovery, delivery, source/kill, gathering and
bounty, Scholar XP and quartermaster coordinator suites provide the complementary
actual game-adapter, independent-file recovery and cap/reset checks. No client
completion event or reward amount is trusted.

## Test adapter correction

The expedition native driver previously initialized a requested victim level,
then overwrote it with level 27. It now preserves the requested level except
for the explicit trivial-target test. That makes the new low-level party test
exercise level-five eligibility instead of a substituted high-level victim.
The driver also exposes the existing reward process-exit injector. The buff
driver gains general runtime probes and tier/replacement selectors. These are
test adapters; no production reward or buff fix was needed for these scenarios.

## Reproduction

Run from the MUD checkout:

```sh
python3 tests/run_clan_integrated_acceptance.py
python3 tests/run_clan_buff_lifecycle.py
python3 tests/run_clan_expedition_progress.py
python3 tests/run_clan_expedition_recovery.py
python3 tests/run_clan_quartermaster.py
python3 tests/run_clan_kill_sources.py
python3 tests/run_clan_deliveries.py
python3 tests/run_clan_objective_runtime.py
python3 tests/run_clan_scholar.py
python3 tests/run_clan_preparation.py
python3 tests/run_clan_psionic.py
python3 tests/run_clan_artisan.py
```

The integrated run compiles production SQL services and runtime/HP code with
AddressSanitizer and UndefinedBehaviorSanitizer, asserts exact totals and
identities, and prints a separate success line for each phase. Each SQL/native
suite creates and removes its own temporary database and binaries.

Next is Phase 50: staging migration/rollback rehearsal, unavailable-database and
feature-control scenarios, followed by the relevant full build/test suites.
Real party timing/balance observations and controlled live copyover remain
staging/pilot work. Pilot enablement and deployment remain later phases.
