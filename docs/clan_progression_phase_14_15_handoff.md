# Clan progression: Phases 14–15 handoff

Status: complete as disabled activation services. Phase 14 is split into pure
validation/reasons and locked permission validation; Phase 15 into atomic commit,
immutable receipts/activity history, and interruption/concurrency acceptance.
Phase 16 expiration/cancellation/reconciliation and invalidation is next.
No live migration, feature enablement, activation, deployment or restart occurred.

## Pure validation

`clan_buff_validate()` accepts server configuration/readiness, an immutable request,
a checked snapshot and explicit UTC time. It returns a bitmask of applicable
reasons and a proposed activation. Only zero reasons permits a commit. Neither
input state nor resources change. Invalid configuration/request and corrupt state
fail explicitly. `clan_buff_rejection_text()` supplies explanations for each bit.

Checks include release/campaign/storage gates, accepted membership, management
permission, clan lifecycle, XP-derived level, tier/slot unlocks, existing activity,
family cooldown, Influence, replacement identity, combat capacity and timestamp
overflow/backwards clock movement. Higher tiers, another officer, or a different
slot do not bypass the saved family timestamp. Refreshing an active family is
rejected. A replacement must name the exact activation currently occupying its
slot; absence or mismatch is a rejection, never implicit replacement. Combat
capacity is calculated for the proposed result after removing that exact old
activation. Expired rows do not occupy slots, but retain their family lockout.

The snapshot's permission booleans are trusted internal facts, not client flags.
The SQL commit service derives them again under the transaction lock. There is
no normal staff bypass. Actor IDs must come from authenticated server callers.

## Permission and schema extension

The core schema contract remains version 5. The separately versioned extension
`sql/20261111_clan_buff_activations_v001.sql` requires that core version and adds
only activation history and the new management privilege. It is an explicit,
repeatable offline migration, never run automatically by startup or a command.
The service requires both contracts; the old core can keep disabled readouts and
reward services without the extension installed. The extension does not enable
any feature or grant a balance.

Management privilege ID 21 is appended logically after existing IDs 0–20;
none of those IDs or the legacy 21-entry snapshot shape is changed. Its rank
threshold is held in `clan_buff_management`, keyed by immutable clan UID.
Missing row or `max_rank=0` means accepted leader only. Explicit thresholds
allow accepted ranks 1 through `max_rank`; applicants/former members never
qualify. Current SQL membership, rank count and leader ID are authoritative.
Native command/editor integration and delegation commands remain Phase 24;
there is no new player-facing activation/grant endpoint in these phases.

Readiness checks require core schema v5, extension version 1, InnoDB tables,
primary/foreign keys and relevant check constraints, and exact immutable-history
trigger bodies. Missing or incompatible storage returns an explicit error.

## Atomic activation and replacement

`clan_buff_activation_commit()` owns one transaction on an exclusively acquired
connection. Nested transactions are rejected and reconnect is disabled by the
core repository checks. Lock order begins with the global store-control row,
matching other authoritative writers, then clan state/membership and family rows.
It validates current SQL state under those locks before any debit.

A successful transaction commits all of:

- Influence subtraction and clan-state revision/timestamp update;
- cancellation of the exact replaced activation, preserving its original expiry,
  paid amount and next-available timestamp;
- the new family's tier, slot, operation/activation ID, price, rules version,
  start, expiration and next-available timestamps;
- an immutable activity/receipt row with actor, request identity, replacement,
  cost, timestamps and before/after Influence.

The persisted expiration is the schedule; event reconciliation is Phase 16.
No gameplay stat changes or runtime cache publication occur here. Later callers
must reconcile committed state before applying effects; an old cached snapshot
cannot overwrite the new clan state because its state revision is stale.

A request supplies one stable random operation ID before attempting the commit.
The same ID with identical clan/actor/family/tier/slot/rules/replacement fields
returns its original receipt and does not spend again. This works after a lost
acknowledgement, later replacement, departure or valid configuration changes,
including disabled activation. Reusing an ID for another request is a conflict.
Retries still require authoritative SQL storage and the schema contracts.
Successful receipts are retained when family rows are later reused.

Failures before COMMIT roll back every change, including the previous active
buff. A failed/ambiguous COMMIT returns `commit-unknown`; callers must retry the
same request rather than generate another operation ID. There is no separate
pending reservation to expire: transaction rollback handles uncommitted work.
A committed retry reads its validated stored proof without recalculating price
or timestamps. Output objects remain unchanged on storage/identity errors;
`ineligible` publishes only the validation decision.

## Verification

`python3 tests/run_clan_buff_activation.py` uses strict C99, warnings as errors,
ASan/UBSan, real production modules and disposable MariaDB only. It covers:

- cumulative reasons, permission and default gates, tier/slot unlock boundaries,
  combat capacity, corruption and clock overflow, with immutable pure inputs;
- current leader/delegated-member/applicant/former access and incarnation/actor
  conflicts;
- all precommit failure/process-crash boundaries, failed replacement preservation,
  original cooldown retention, exact cooldown-boundary reactivation;
- lost postcommit acknowledgement/process death, simultaneous identical retries,
  competing distinct requests for a slot and exactly-once debit/history;
- immutable SQL receipts, invalid receipt proofs and missing schema safeguards.

Phase 13 query and campaign/configuration regressions pass. CMake and traditional
Make server builds are checked. Defaults remain OFF and no live database was used.

Next: Phase 16 cancellation, expiry, login/restart/copyover reconciliation and
member-stat invalidation. Preserve saved cooldowns and operation receipts. Buff
stat effects are Phases 17–23; player-facing lifecycle commands are Phase 24.
