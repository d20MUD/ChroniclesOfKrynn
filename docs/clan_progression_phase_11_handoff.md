# Clan progression: Phase 11 handoff

Status: complete as disabled progression/lifecycle integration. Split into 11a
membership/receipt eligibility, 11b retained-history disbanding, and 11c atomic
creation/runtime mirrors. Phase 12 text readouts and restricted inspection is
next. No live schema migration, authority switch, restart, award or progression
feature enablement occurred. Required schema remains version 5.

## Membership and access

`clan_repository_member_access()` reports accepted, applicant, former, or deleted
membership for a stable character ID and immutable clan incarnation. It reads a
checked consistent snapshot; malformed storage and missing identities return
explicit errors with unchanged output. Positive membership rank must be valid
for the clan. Historical Favor and a reused numeric vnum never establish access.

`clan_persistence_progression_access()` performs this current SQL check for a
character, rejects NPCs/applicants/former/deleted memberships, and fails closed
on storage outages. This is a membership query, not a release gate: future buff,
contract and quartermaster callers must also use their feature, permission and
resource checks. No current buff or equipment effect was added in this phase.

Existing application/enrolment, leave and online/offline expulsion paths already
use the checked membership adapter through `set_clan()`/`save_char_checked()`.
Their membership/statistic snapshot commits do not modify Favor, family timers,
account contribution usage or receipt history. They now compose with current
SQL progression access and lifecycle operations. Joining as an applicant grants
no progression services; enrolment grants accepted access. Departure removes
access immediately; rejoining the same incarnation can use that retained Favor.
Changing clans never moves Favor or resets account-global weekly limits.

A membership replacement can append a smaller player ID. Phase 11 canonicalizes
candidates before publishing the committed cache, fixing an ordering mismatch
that otherwise made the next leave/rejoin/rank update falsely fail refresh.
Player-file membership fields remain compatibility mirrors; SQL commits are not
rolled back because a later character-file mirror save fails. Load reconciliation
repairs those mirrors. Personal legacy clan points remain unchanged.

## Receipt acceptance and departure

`clan_reward_prepare_member()` is the membership-checked acceptance API for
future personal source adapters. Under the same control/receipt transaction it
requires current accepted membership in the event's exact incarnation, checking
rank against the saved rank list. Existing identical receipts are returned first,
so recovery does not become a new claim after departure. `CLAN_REPO_INELIGIBLE`
is an appended result; existing result IDs retain their meanings.

The lower-level `clan_reward_prepare()` remains trusted transaction/recovery
infrastructure. Source adapters must establish actual account ownership, source
eligibility and membership at occurrence, use the checked acceptance API for new
personal facts, and apply release gates before exposing any gameplay action.
Positive numeric IDs or arbitrary requested amounts are not proof of eligibility.

An already accepted ordinary contribution retains its occurrence-time clan and
can finish into that clan's historical Favor after departure. It never pays the
new clan. An uncommitted personal expedition claim additionally requires current
accepted membership in the same incarnation at completion. Its check runs under
the coordinator's control lock, serialized with membership changes. Leaving or
switching blocks it; rejoining does not alter any accepted occurrence/expiry
facts. The later expedition adapter must still enforce its original entitlement
expiry and family/outing eligibility.

Committed receipt replay bypasses current membership checks and preserves the
original recipient and exact award. A pending operation for a deleted clan is
retained/discoverable but cannot credit a replacement incarnation; explicit
cancellation remains available. No committed or pending economic delivery is
deleted or redirected by lifecycle changes.

## Atomic creation and retained-history disbanding

SQL `clan create` now uses `clan_persistence_create()`: validate that the leader
has no current membership/application, generate a fresh immutable UID, and commit
the new zero-XP/Influence clan plus leader membership in one snapshot transaction.
Only after success is the draft added to runtime arrays and the character mirror
reconciled. Draft structs are initialized before use. Existing clan command
permissions still authorize the action. Creation does not charge player-owned
resources or introduce an officer award endpoint.

SQL `clan destroy` now commits via `clan_persistence_disband()` before touching
runtime topology. The candidate removes current memberships, incoming
relationships and active zone ownership/popularity references. The snapshot
writer tombstones the omitted UID while retaining its core/progression, Favor,
family cooldowns, weekly windows and immutable reward receipts. No character
inventory is traversed or removed. Offline mirrors are repaired on load rather
than requiring every player file to succeed before disbanding.

After acknowledgement, `remove_sql_clan()` revokes online/index mirrors and
compacts relationship columns, runtime clan rows, hashes and zone popularity
indices together. It preserves other clans' index meanings. The staff command
also uses its parsed vnum argument correctly when the caller has no clan.

`clan_repository_disband()` exposes the same pruning and revision-checked commit
for trusted server tooling, keyed by UID rather than vnum. Stale/conflicting
revisions and injected failures roll back the entire transaction. Deleted UID
retries report deleted; they cannot target a new clan using the old vnum.

Creation/disband failures and uncertain acknowledgements leave MySQL authority
pinned and unavailable rather than publishing speculative runtime changes or
falling back to files. Reconcile/reboot against SQL before retrying; never infer
failure from a lost acknowledgement or compensate against another incarnation.
After SQL succeeds, a rare runtime publication failure likewise requires reload.
No deployment or operational authority transition is performed automatically.

## Verification and next entry point

- `tests/run_clan_lifecycle.py`: strict C99/ASan/UBSan real SQL tests for accepted,
  applicant, malformed-rank, former and deleted access; checked acceptance;
  occurrence-time rewards after departure; uncommitted expedition refusal and
  committed replay; clan switching/rejoining; transactional disband rollback;
  retained Favor/cooldowns/pending/terminal receipts; relation/claim cleanup; and
  fresh UID/economy isolation when reusing a vnum.
- `tests/run_clan_persistence.py`: actual adapter plus extracted production
  creation/removal/list helpers; creation rollback, leave/apply/enrol access changes, index/mirror
  revocation, runtime relationship/popularity compaction, atomic replacement
  creation, differing runtime/SQL sort orders and subsequent safe saves.
- Prior receipt/crash/concurrency, personal reward, boot/cutover/Favor,
  configuration/campaign and actual clan login command regressions.
- CMake and Automake builds; documentation links and whitespace checks.

Phase 12 should expose checked text status and privileged inspection without
mint commands. Use immutable identities, cumulative historical Favor, retained
account windows and exact receipt explanations. Keep feature controls OFF.
Later buff/stat phases must combine current membership with active-incarnation,
release and expiry checks; they must not apply a retained deleted-clan timer.
