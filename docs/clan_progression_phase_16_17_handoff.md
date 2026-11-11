# Clan progression: Phases 16–17 handoff

Status: complete behind disabled defaults. Phase 16 is split into durable
cancel/expiry transitions, coherent runtime hydration, and invalidation; Phase 17
integrates Company Vitality at final effective maximum-HP calculation. Phase 18
Arcane Readiness preparation timing is next. No live migration, activation,
deployment, copyover or restart was performed.

## Persistence and lifecycle

Apply `sql/20261111_clan_buff_lifecycle_v001.sql` only through the explicit offline
migration procedure with writers quiesced. It requires core schema v5 and the
Phase 15 activation extension v1. Neither existing version changes. The new
lifecycle extension has its own version-1 receipt, immutable event triggers and
unique activation transition key. It does not enable gameplay or grant resources.

`clan_buff_cancel()` checks current SQL accepted membership, management privilege,
feature gates, exact activation identity and current activity under the same global
store-control lock as activation and membership writers. One transaction marks
the family canceled and writes a durable cancellation receipt. Influence, original
expiration, paid price and next-available time are unchanged. An exact operation
retry returns the original proof, even after disablement or subsequent reactivation;
a different actor/activation cannot reuse that operation. There are no refunds.

`clan_buff_lifecycle_reconcile()` expires up to 125 due, uncanceled family rows in
one transaction. Each activation gets at most one cancel/expiry transition;
replacement already has its Phase 15 receipt and skips this worker. Expiration
records use the saved expiration as occurrence time and retain the later processing
time separately. A restart/offline interval never starts another duration or
cooldown. Bounded historical backlogs drain on subsequent pulses; queries already
exclude expired effects by absolute time even before maintenance commits.

The transaction also returns a coherent current core/membership/family snapshot.
`clan_repository_snapshot_read_locked()` is internal composition infrastructure:
its caller must already own the transaction and global control lock. These reads
never create a nested transaction. Errors publish no partial snapshot. Lost commit
acknowledgements require replay; uncommitted transitions roll back. No clock reset,
expiry refund or resource award occurs during reconciliation.

## Runtime, login and membership

`clan_buff_runtime_tick()` runs from the once-per-second heartbeat. Its first
active pass reconstructs state from authoritative SQL; it does not trust a
playerfile buff duration, legacy numeric membership, or the boot timer snapshot.
New login and copyover-loaded characters enter the same reconstruction path.
Offline time is accounted for by original absolute timestamps. No relative timers
or saved player-stat bonuses are introduced.

While a live clan-buff configuration is selected, `load_char()` begins a transient
HP-clamp deferral before stat reconstruction. The common `enter_player_game()`
path finishes it after equipment and legendary loading, forcing an authoritative
clan refresh and one final clamp before login proceeds. Normal login and copyover
share that path. This preserves legitimate saved buffed HP without restoring a
saved HP amount or granting healing, and clamps offline-expired bonuses against
the fully reconstructed maximum. The pending flag is never serialized. Default
OFF leaves the prior loading path unchanged.

The runtime configuration defaults to `clan_progression_defaults` (OFF). The
trusted `clan_buff_runtime_configure()` API exists for future configuration
integration and isolated fixtures; it is not a player command or implicit release
switch. Campaign, authority, readiness, release/pilot and buff gates are checked.
Default OFF does not query the lifecycle tables. Storage failure, incompatible
state, clock regression, invalid configuration or disabled authority clears
runtime bonuses and clamps affected members. Recovery reconstructs the checked
view without healing. Runtime configuration changes clear effects immediately.

Activation/cancellation/expiration commits emit invalidation callbacks that queue
refresh; callbacks perform no SQL on the still-owned pool connection. The next
heartbeat revalidates all online characters and recalculates changed maxima.
The normal resolution for committed changes is the next one-second pulse. Future
Phase 24 command adapters can call the runtime tick after releasing the connection
for an immediate command result. Absolute expiration additionally gates every HP
rebuild directly, without waiting for the worker.

An observer on `clan_persistence_reconcile_player()` clears an existing cache
immediately when the authoritative accepted clan UID changes or disappears.
Ordinary saves and rank changes inside the same accepted incarnation do not strip
valid HP. Leaving, expulsion, applicant state, disbanding and vnum reuse cannot
carry another incarnation's cached bonus. Joining/rejoining gets a fresh SQL view.

## Company Vitality

The existing `calculate_max_hp()` computes ordinary HP and stores only that value
in `GET_REAL_MAX_HIT`. Its final `legendary_foundation_hp()` step now calculates:

`B = ordinary maximum + effective legendary flat HP`

`effective maximum = B + floor(B * percentage / 100)`

The percentage comes only from the checked active Company Vitality tier (5/10/15
in the pilot). Calculations use 64-bit intermediates and saturate at `INT_MAX`.
They always start from the non-clan base, never the previous effective maximum.
The HP breakdown displays Company Vitality separately.

The final clamp happens after both legendary and clan adjustments. This avoids
an intermediate legendary clamp stripping valid buffed HP on every rebuild.
Activation or a larger maximum never increases current HP. Loss/replacement,
expiry, departure, disablement or a lower base clamps current HP with `min`;
negative HP remains negative and no damage/death/healing function is called.
Even a buff whose rounded bonus is zero records runtime participation so later
base reductions still clamp correctly.

Save-time `legendary_serializing` suppression omits temporary clan HP without
clamping current HP or erasing runtime participation. Restoring the effective
view reapplies from the ordinary base. The new character fields are runtime
caches only: no playerfile/object serializer writes them. No regeneration,
healing, other clan buff effects or territory helpers are enabled by this phase.

## Verification

`python3 tests/run_clan_buff_lifecycle.py` compiles actual lifecycle/runtime code
and the production final HP function under ASan/UBSan, using disposable MariaDB:

- transaction faults and process death before/after COMMIT; exact cancellation
  replay after later activation; unchanged price and cooldown;
- exact expiry boundary, offline reconstruction, concurrent sweeps and lost
  acknowledgement, with one durable expiration event;
- default OFF, SQL hydration, active/expired login-clamp deferral, membership departure/rejoin, outage recovery,
  configuration disablement, and member invalidation;
- all Vitality percentages, repeated rebuilds, no healing, ordinary/legendary
  separation, serialization suppression, integer saturation, negative HP,
  zero-rounded bonuses and absolute expiry before the SQL worker;
- schema/trigger failure and unchanged persistent state on rejected operations.

Phase 14–15 activation and checked clan-persistence regressions pass, as does the
existing legendary foundations/equipment/commands suite. CMake and Make server
builds pass. An initial activation regression under parallel build/test load
reported a transient core schema mismatch; diagnostic and unmodified reruns
passed, with no persistent reproduction or changed data from that rejection.
All SQL checks used disposable databases, never the game database.

Next: Phase 18 prepared-caster timing and queue semantics. Keep lifecycle receipts,
family cooldowns, HP persistence separation and the no-healing invariant intact.
Player-facing lifecycle commands remain Phase 24; no new player command is exposed.
