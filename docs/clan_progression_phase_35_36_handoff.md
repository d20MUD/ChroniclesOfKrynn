# Clan progression Phases 35–36

Implemented behind the existing disabled expedition/contracts gates. No live
migration, enablement, deployment, installation or restart was performed.
Apply `sql/20261112_clan_expeditions_v002.sql` explicitly offline after core v5
and expedition v1. Current expedition commands require both extensions. The
optional help publication remains `sql/20261112_clan_expeditions_help.sql`.
The v1 membership triggers now also invalidate retained completed rosters, so
reapplying v1 cannot restore the old unclaimed-entitlement rejoin loophole.

## Phase 35: authoritative combat progress

The existing live encounter collector passes its frozen encounter to
`clan_expedition_combat` before applying ordinary kill prototype/source quotas.
The same pure participation query qualifies damage, effective healing,
mitigation, useful support/defense and hostile-hit tanking. No killing-blow
privilege, character XP or legendary boss qualification is added. Trivial,
owned/charmed/summoned/pet, manipulated, test and staff-test encounters remain
excluded. The victim's loaded room resolves the actual area; no command or
inventory action can emit expedition progress.

Each clan advances only its own started outing with at least two distinct,
currently eligible enrolled accounts in the encounter. Original account/character
identity, accepted SQL membership, retained roster eligibility, frozen character
bands and start/expiry are checked under the existing control lock. Ordinary
solo contracts remain independent. A kill may separately earn ordinary credit,
bounty credit and one expedition objective within each source's own limits.

Immutable event rows retain original outing/clan/encounter identity, occurrence
time, area/prototype/room, selected primary objective, canonical qualified facts
and credited participants. The unique clan/encounter key prevents transferring
one event to another outing. Exact replay is a no-op, including after completion;
changed frozen facts conflict. New legitimate respawns have new identities and
can fill remaining quotas without exceeding them. Named targets cannot also
fill a defender quota; the Phase 33 parser prohibits prototype overlap between
primary objectives.

The first counted event and irreversible roster sealing share one SQL
transaction. Counters derive from the bounded retained event ledger (22 assault
or 30 hunt objectives). `active` displays completed/required values. Restarting
cannot erase a committed event or unseal a roster. An event that never reached
durable SQL is not reconstructed from commands or XP. The collector does not
create an unbounded offline event backlog; broader reconciliation is Phase 37.

## Phase 36: completion and separate claims

On the final counted objective, eligible personal finishers must have at least
half the outing's counted objectives, rounded up, and participate in that final
objective. They must still be eligible accepted members of the original clan.
At least two distinct enrolled accounts must meet the full rule. A final
attempt without two qualifying finishers rolls back its final event; it cannot
lock an invalid completion. A later legitimate final encounter can satisfy the
rule, or the group can abandon and restart without transferred progress.

The final event, completion record, immutable personal entitlement identities,
first-family shared claim/receipt and closure of the active outing commit
atomically. They cannot be split by a process interruption. Closure releases the
clan's outing slot while keeping all history. New outings have new identities.

The first qualifying completed outing of each family per clan/UTC week prepares
one shared receipt for 600 clan XP / 300 Influence. The existing clan projection
applies weekly, level and storage caps. The unique clan/week/family claim is
independent of roster size and later outings. At most one assault and one hunt
shared award are reserved per clan/week; bounty shared claims remain separate.

`clan expeditions claims` lists only the authenticated member's retained personal
completions. `claim <outing-id>` at a quartermaster accepts a 100-Favor claim
with no clan XP/Influence or character XP. Accepted membership in the original
incarnation, original account/character and unexpired entitlement are checked
under lock. The account/week/family reservation, durable pending receipt and
prepared entitlement commit together. Alts and clans share the reservation;
concurrent alts cannot reserve the same weekly family twice. The core two-outing
source limit and account Favor cap apply as well, including fractional capped
payouts and retained zero outcomes. Repeating a paid claim returns its original
receipt, never an extra award.

Unclaimed entitlements expire at their original weekly boundary. Departure,
clan change or applicant status permanently invalidates them, even after an
immediate rejoin. Once a claim is durably accepted, the SQL-only completion
participant verifies its prepared entitlement and original claim reservation;
its receipt can recover after membership changes or reset without changing its
beneficiary or week. Generic personal/shared workers cannot pay expedition
sources without the dedicated proof participant. The generic coordinator's
membership exception applies only through a dedicated participant, whose proof
is verified before any payout commits.

The bounded source worker dispatches prepared personal and shared receipts to
the expedition coordinator. Feature admission and schema readiness still gate
payouts. Both shared and personal completion use the existing atomic receipt,
balance, account/Favor and clan-week transaction. A deleted clan cannot receive
new credit or redirect a receipt through reused vnums. Pool connections are
released before refreshing live progression caches.

## Storage and validation

The optional InnoDB extension adds retained immutable events, completions,
entitlements and separate shared/personal claim keys. Readiness checks version,
engines, all ten new retention/identity triggers and nine exact unique index
layouts in addition to expedition v1. Reapplying extensions preserves data.

- `tests/run_clan_expedition_progress.py`: actual services under ASan/UBSan in
  private MariaDB; both patterns, wrong area/target, two-account/level/exclusion
  rules, all contribution roles, encounter replay/altered facts, pre/post-commit
  process interruption, bounded counters and sealed substitutions.
- Atomic final-event rollback, half-participation and final-objective eligibility,
  later-member personal credit without another shared reward, both-family keys,
  capped Favor, concurrent-alt family reservations, generic-worker proof refusal,
  payout rollback/lost ACK and original-identity/week recovery after departure.
- Actual live collector/healing integration and source-worker shared recovery;
  actual discovery/claim command handlers, paid replay and safe pool release.
- Prior enrollment/roster, personal award, lifecycle, and ordinary bounty collector
  regressions, plus Make/CMake builds. Pure personal reward policy tests continue
  to verify expedition Favor caps; legacy generic source-6 tests now explicitly
  require the dedicated entitlement proof.

Next: Phase 37 broader outing expiry/abandonment and restart reconciliation.
Defaults remain unchanged and disabled.
