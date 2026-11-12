# Clan progression Phases 28–29

Implemented behind disabled defaults. No live migration, enablement or process
restart was performed. Core v5 remains required. Apply the optional
`sql/20261111_clan_delivery_v001.sql` offline before enabling contracts; publish
`sql/20261111_clan_delivery_help.sql` separately if desired. Do not drop history
or remove recovery tags when rolling back a deployment with accepted deliveries.

## Definitions and access

`lib/clan/deliveries.json` supplies version-1 definitions, with at most 32 unique
stable IDs. Never reuse an ID for a different objective; create a new versioned
ID instead. The parser rejects unsupported kinds, unbounded quantities, invalid
IDs, duplicate IDs, bad grades, unknown fields and invalid unlocks. The runtime
also verifies material IDs/grades against the authoritative crafting registry.
Malformed/missing catalogs or extension schemas make contracts unavailable;
legacy clan operation does not install or enable anything implicitly.

The initial choices are 50 ash wood, 50 bronze, 100 finite arrow shots, 100 finite
bolt shots, and (clan level 6) 100 finite bullet shots with enhancement +1 or
better. Material requirements use exact material/grade; ammunition quality is
an explicit minimum enhancement, never guessed from an item description.
Ammunition with an unknown legacy bundle size, infinite uses or Returning is
excluded. Only the equipped ammo pouch and directly carried ammo are inspected.
Existing bonuses, descriptions and ownership of surviving partial stacks remain
unchanged. Purchased/transferred goods qualify. Gathering and bounty hooks and
rotating contract pools remain later phases.

`clan deliveries` offers list, accept, active, abandon, turnin, confirm and retry.
Available definitions show requirements, clan-level unlocks, exact base credits,
account limit and the distinction between preview and confirmation. One active
delivery per player is enforced by SQL, leaving two slots for future families.
Contracts expire at the weekly boundary; a later acceptance closes an expired
slot only when no economic operation holds it. Weekly completed-definition keys
are per account, across clans/alts. Two different delivery definitions may be
completed per week. Abandonment/reacceptance cannot erase those keys.

Current authenticated account and accepted SQL membership, authoritative MYSQL,
live progression, Krynn campaign, pilot/public admission, the contracts toggle,
validated catalog and extension readiness are required. Buffs need not be on.
Acceptance and manual turn-in require a quartermaster, outside combat/timed work.
Definitions, accepted account, incarnation, occurrence week and requirements are
frozen at acceptance. Confirmation checks current membership and terms again
under the same control-row lock as the reward receipt.

## Recoverable turn-in

1. A server-held, 60-second quote shows exact XP/Influence/Favor in eighth points
   after all personal/account/clan caps. Wrong/insufficient goods consume nothing.
   All-zero credit cannot prepare a donation. Confirmation rechecks the quote's
   exact projected delta under SQL locks; changed credit requires another preview.
2. The character's `ClDv` intent lock is checkpointed before SQL prepare. The
   pending reward receipt and delivery operation/quote commit together. The
   generic reward worker cannot complete pending delivery receipts.
3. Material deduction and its paid marker share a checked atomic player-file
   save. Finite-shot deductions and per-object `CDel` amounts share a checked
   inventory checkpoint, including the inventory header's operation/stage. The
   DB and file inventory representations both preserve these fields. Zero-shot
   objects remain as refund placeholders until a terminal receipt is known.
4. Only the game coordinator with a durable paid checkpoint completes the reward.
   Receipt, counters/balances, unique weekly definition claim, completed contract
   and operation transition commit together. Completion uses the stored quote,
   not caller-supplied replacement amounts. Membership changes after durable
   economic acceptance do not redirect an already accepted delivery.
5. If caps change before the locked award, or the clan is no longer active,
   recovery restores the exact units/shots and checkpoints the refund before
   canceling the receipt/contract. Transient storage failures retain the lock.
   An ambiguous SQL commit is read back before deciding whether to refund.
6. On committed receipts, cleanup preserves surviving bundles and extracts only
   depleted placeholders. Resource cleanup is checkpointed before SQL's hold is
   acknowledged, then the player intent is cleared. A crash at any stage replays
   the same identity. Reconnect, copyover entry and command dispatch recover
   before command mutations. Pool connections are released before file saves or
   cache refreshes. Accepted economic recovery continues if new contracts are
   disabled; it never charges unpaid goods automatically after a restart.

The optional schema uses InnoDB, foreign keys, unique active slots/operation
holds/definition claims, immutable frozen facts and retained history triggers.
Disbanding cannot cascade away receipts or goods records. Capacity is bounded
by existing reward pending limits and one in-flight delivery per character.
No client/MSDP field supplies account identity, resource costs or reward amounts.
Corrupt checkpoints or reserved ammo moved/destroyed outside the command path
fail closed for staff reconciliation; recovery never invents replacement item
identities or assumes missing objects were paid.

## Validation

- `tests/run_clan_deliveries.py`: ASan/UBSan private MariaDB tests for malformed
  definitions, membership/level access, frozen requirements, expiring quotes,
  atomic claim/receipt rollback, ambiguous commit/replay, changed caps, cancel,
  two-definition limits and accepted old-incarnation recovery.
- `tests/run_clan_delivery_recovery.py`: actual game coordinator and goods logic
  with private SQL and restartable checkpoint adapters; wrong/insufficient goods,
  infinite/Returning exclusion, partial stacks, five file crash boundaries,
  failed checkpoint, SQL post-commit crash, refund crash and repeated recovery;
  actual command acceptance/confirmation, disabled gate and quartermaster checks.
- Estate real-codec regression verifies `CDel` survives both object record loaders.
- Existing personal award, kill/supply source and lifecycle regressions; Make and
  CMake game builds.

Next: Phase 30 gathering event hooks. No activation is implied by this handoff.
