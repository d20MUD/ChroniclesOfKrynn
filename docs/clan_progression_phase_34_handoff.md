# Clan progression Phase 34

Implemented behind the existing disabled expedition/contracts gates. No live
migration, installation, enablement, deployment or restart was performed.
Apply `sql/20261112_clan_expeditions_v001.sql` explicitly offline after core v5.
The optional help publication is `sql/20261112_clan_expeditions_help.sql`.
Continue deploying the Phase 33 expedition catalog with the game library.

## Enrollment and commands

`clan expeditions list|create <definition>|join <outing-id>|active|start|leave|abandon`
uses authoritative SQL, accepted membership, pilot/public admission and the
existing expedition gate. Buffs need not be enabled. Create/join/start require a
quartermaster, authenticated character and no combat/timed craft. No command
accepts another player's identity, account or level from its arguments.

Create allocates a server UUID, freezes the validated definition/reward IDs,
original clan incarnation, creator and UTC week, and enrolls the creator. The
clan has at most one recruiting/started outing, enforced by a generated unique
key and the shared control lock. Members opt in individually. Their character
and account identities are retained; two characters of the same account cannot
occupy separate roster slots, including removed entries. The roster holds up
to six current accounts, with a bounded maximum of 32 retained enrollments per
outing to prevent unbounded substitution history.

Only the creator can start. Two to six distinct accepted accounts from the
same active clan must be present in the quartermaster's room and currently
meet the definition's clan unlock and character bands. Start uses fresh server
snapshots of the entire roster and rechecks SQL membership/account identity.
The frozen definition is revalidated against the loaded world before start,
even if the server catalog changed after creation. Missing/removed targets fail
closed. The world availability adapter retains the Phase 33 dataset/reset
meaning; it does not promise a named NPC is currently alive.

Recruiting expires at the weekly boundary. First start sets a fixed two-hour
hard deadline, shortened to the completion week's Monday 00:00 UTC boundary.
Repeated starts never replace the outing ID, start time or deadline. Basic
outing/roster persistence is SQL-only and survives process loss; reconnecting
can inspect the same identity through `active`. Unknown create acknowledgements
are resolved by reading that clan's existing outing, never replacing its ID.

## Roster and membership invariants

Before the first counted objective, members may leave and new accounts may
join. Removed character/account entries cannot reactivate in the same outing.
The server-only `clan_expedition_seal` service rechecks the entire eligible roster
and seals it irreversibly. Phase 35 must invoke this at its first authoritative
counted objective; there is no player command that seals or supplies progress.
After sealing, insertion/substitution is rejected in both the service and the
SQL admission trigger. Idempotent retries of existing enrollment do not add a
new participant or change identity.

Membership DELETE and clan-change/applicant UPDATE triggers permanently mark
affected active enrollments ineligible. Leaving and immediately rejoining
cannot revive one. Current SQL membership and live account/level snapshots are
still checked at start/seal. Reconnect with the same eligible character/account
is allowed. Rank/points changes that preserve accepted membership do not erase
enrollment. Any current enrolled accepted member may abandon, including when
the original creator has departed. Expired/abandoned outings cannot start or
accept new enrollments; creating after expiry releases only the expired slot.
History is retained, and new outings inherit no roster/progress entitlement.

The core repository previously deleted/reinserted every membership during a
snapshot save. It now stages desired rows in a connection-local temporary table,
deletes only departed characters, and upserts actual membership facts in the
same existing transaction. This prevents treasury/description saves from
looking like departures. Real departures invalidate enrollment atomically;
failed snapshot writes roll back both membership and invalidation effects.

The extension checks InnoDB engines, version, unique-key column layouts and all
seven retention/identity/admission/membership triggers. SQL services serialize
under the existing core control lock; frozen outing terms and enrollment
identities cannot be rewritten or physically deleted. No player file or stat
cache mutation occurs while borrowing a SQL pool connection.

## Scope and validation

This phase creates no combat progress, completion, claim or reward path.
Enrollment neither consumes nor replenishes account/source budgets. Ordinary
solo contracts remain independent. Combat contribution and objective sealing
integration are Phase 35; completion/claims and broader reconciliation remain
Phases 36–37. No solo alternative requires enrolling in an expedition.

- `tests/run_clan_expedition_enrollment.py`: production C under ASan/UBSan in
  private MariaDB; opt-in, distinct-account/cap/gate/clan/level checks, stable
  replay, atomic create rollback, concurrent create, full-roster start,
  timer/reset boundaries, pre-objective substitutions, sealed SQL admission,
  leave/rejoin invalidation, retained history and migration/schema checks.
- The same runner tests actual command handlers with authoritative SQL and
  live-actor adapters: quartermaster/gate/account checks, absent/ineligible
  roster members, start, stable readout and abandonment.
- Repository-save preservation and failed-snapshot rollback with enrollment
  triggers installed; existing snapshot and solo-contract regression suites.
- Phase 33 definition/world/rotation tests; full Make and CMake builds.

Next: Phase 35 combat objective tracking from authoritative participation events.
