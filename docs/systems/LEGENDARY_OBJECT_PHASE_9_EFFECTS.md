# Phase 9: shared activation and effect engine

Implemented 2026-10-24. This stage supplies the engine used by phases 10–12;
it does not enable the individual catalog techniques or signatures.

## Activation

`infuse use <power-ID|quoted name> [target]` dispatches by the catalog's stable
ID. Friendly powers default to self and hostile powers to the current opponent.
Future catalog adapters register a target policy, a read-only prerequisite check,
and an application callback. Unregistered powers explain that their effect is
not implemented and spend nothing.

Activation requires an eligible, equipped, attuned major power from the existing
loadout planner. It uses that channel's rank and resolved modifier definition.
Recovery, ongoing casting, stun, paralysis, daze and incapacitation prevent use.
A longer existing recovery is never shortened. Normal autoattacks are unchanged.

Target checks require visibility and the same room. Friendly targets must be
self or a group member who is not fighting the source. Hostile targets obey
peaceful rooms, protected mobiles, mission ownership, staff protection and PvP
policy, including the controlling owner of a pet. High-level NPC bosses are not
mistaken for staff. Distant Aid resolves its allied targeting before validation.
Catalog-specific checks and control immunity/fallbacks belong to their adapters.

Before invoking an application callback, a single SQL transaction locks the
player ledger and records all of:

- Six seconds of shared legendary cooldown, key `1`.
- Individual cooldown under the stable catalog power ID, shared by copies.
- Family cooldown under `1000 + family`: protection/restorative 60 seconds,
  control 30 seconds, other families as defined by the pure resolver.
- An activation receipt containing source UUID/channel, power, recipient player
  ID where applicable, and UTC timestamp.

Only after commit does the source receive six seconds of command recovery and
the callback run. Invalid targets/prerequisites cost nothing. A valid activation
that is resisted spends its cooldown normally. A database failure rolls back
an uncommitted activation; an uncertain commit never causes effect replay.
Receipts are audit/source records, not combat recovery instructions. Old receipts
are pruned per player after one day during subsequent activation.

## Source-bound effects and timers

`legendary_effect_prepare` copies its activation/specification and attaches a
phase 7 source lease. It supports scalar prepared effects, percentage reductions,
redirection and delayed healing. Timers use UTC seconds and the heartbeat checks
them once per second. Pool-based wards and one-packet charges are separate:
charges of zero allow a ward to persist until expiry or exhaustion of its pool.
Zero rounded reduction consumes neither a charge nor a budget debit.

Queries choose the strongest matching effect per key and kind; distinct copies
are not added on a packet. Packet eligibility and remaining pools participate in
ward selection. Equal strengths prefer the latest expiry. Refreshing the same
source's scalar preparation keeps its strongest value and longest duration.
Different ward keys resolve in ascending key order, with the recipient ceiling
still applying across all sources. Effect records are limited to 64 per recipient.

A delayed heal may have its final tick exactly at expiry. Missed ticks are not
replayed in a burst following a stalled heartbeat. No temporary effects are
loaded from SQL on reconnect or restart.

Effects are removed or revoked on source invalidation, death, disconnect and
character extraction. Explicit flags end group wards on either participant's
room departure and pursuit-like preparations when their source moves. Friendly
ally effects also end when group membership is lost. Character cleanup removes
both incoming and outgoing references before memory is freed. Object extraction,
unequipping, grip/capability loss and unattunement use the existing source lease
invalidation path. Cancelling a lease never deletes cooldowns or spent budgets.

## Effective-event accounting

Player budget entries commit before the corresponding health/resource mutation. A crash between debit commit
and that mutation can lose the benefit while retaining its spent allowance;
combat is never replayed to compensate, preventing duplicate damage/healing.
They share the existing `legendary_budgets` table:

| Kind | Owner of allowance | Ceiling | Rolling window |
| --- | --- | ---: | ---: |
| Offensive bonus | Source player | 60 | 60 seconds |
| Legendary prevention | Recipient | 100 | 60 seconds |
| Direct healing / temporary-HP grants | Recipient | 100 | 60 seconds |
| Resource savings / restoration | Resource user | 20 | 180 seconds |

Windows retain entries with `occurred_utc > now - window`; entries exactly at
the boundary expire. Future-dated entries remain spent after clock rollback.
Pruning and debit insertion run under the player's ledger row lock. Each debit
has a unique operation ID. Integer minimum-one-point debits bound the live row
count by its ceiling. A new character object with the same player ID, a new SQL
connection, or a server restart reads the same durable allowance. Transient NPC
recipients have equivalent timestamped in-memory ledgers for their lifetime;
extraction discards that NPC instance's ledger.

The production damage path calls the engine immediately before final HP loss,
after ordinary damage handling and existing class sharing/defenses. Percentage
reduction uses integer rounding down, then per-effect maximum, remaining pool
and recipient allowance. Redirection is capped by the source's HP minus one;
only the amount safely shifted is debited, and excess stays on the recipient.
Transferred HP loss bypasses additional defenses and does not recurse.

`damage_with_origin` carries explicit bonus/redirection/no-trigger packet flags.
Bonus packets are capped against their input amount, settled only after ordinary
defenses, and charged for actual non-overkill damage. They do not produce extra
participation actions, Sanguine Barrier lifesteal, Energy Retort recursion or epic
spell follow-up effects. F02 remains part of ordinary weapon damage and does not
spend this offensive allowance. Redirection packets use terminal, nonlethal HP
loss rather than re-entering damage defenses or proc chains.

`legendary_heal` clamps to missing HP before spending recipient allowance and
records only effective healing. It respects golem healing reduction and Black
Mantle penalties without adding class healing multipliers to fixed legendary
values. Resource helpers pay the real cost or restore only missing points in an
existing pool; discounts cannot reduce final cost below one. Payments require
affordability of the final cost; an insufficient remaining discount budget
rolls back without spending points or allowance. Discount and
refund adapters must use these helpers in place of their ordinary payment, not
in addition to it. Temporary-HP catalog adapters use the same healing debit kind
for their actual nonstacking grant; their grant/removal mechanics arrive with S08.
Normal healing, regeneration, resources and class defenses remain unbudgeted.
These helpers own their SQL transactions and must not be nested inside an
economic transaction or used to speculate about an event that might occur later.

## Schema and validation

Apply `sql/20261024_legendary_effects.sql` after the existing v1–v4 migrations.
It adds the activation receipt table and advances the schema to v5. Migration
reruns are idempotent. Readiness rejects missing or outdated schema. No live SQL,
world files or quest files were changed, and the server was not restarted.

Run:

```sh
python3 tests/run_legendary_effects.py
python3 tests/run_legendary_rules.py
ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 \
ESTATE_TEST_CFLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer -g' \
python3 tests/run_legendary_effects.py
```

The disposable MariaDB fixture compiles the production engine, rules, equipment
cache, source leases, persistence and command code. Synthetic registered handlers
assert that cooldowns/receipts are committed before application. Ordinary world,
visibility, target-policy dependencies and stat recalculation use the existing
fixture adapter; this is not a running-server combat simulation.

Coverage includes invalid/unimplemented uses, action/casting restrictions, SQL
failure rollback, six-second timing, individual/family sharing, reconnect,
rounding, packet eligibility, one-shot and pooled consumption, safe redirection,
nonrecursive origins, non-overkill offensive debits, exact rolling windows,
clock rollback, effective healing, final delayed ticks, departure/gear-source
cancellation, resource caps and actual payments, recipient ID sharing, NPC
lifetime cleanup and ordinary target policy. All earlier phase regressions run
with it. Make and CMake builds validate the production integration hooks.

Validation passed: the phase 9 integration suite and earlier-phase regressions,
pure rules tests, AddressSanitizer/UndefinedBehaviorSanitizer checks, both Make
and CMake server builds, and `git diff --check`. The final builds produced no
compiler warnings. Expected fault-injection SQL errors in the fixtures do not
represent test failures.
