# Clan progression Phases 30–31

Implemented as disabled foundations. No live migration, feature enablement,
installation or process restart was performed. Apply the optional
`sql/20261111_clan_objectives_v001.sql` explicitly offline after core v5. The
Phase 28–29 delivery extension is independent and unchanged. Deploy
`lib/clan/objectives.json` with the game library. The optional help publication
is `sql/20261111_clan_objectives_help.sql`.

## Definitions, access and commands

`clan contracts gathering|bounty list|accept <id>|active|abandon|retry` exposes
requirements, exact target/grade, character bands, clan unlocks, remaining
progress, expiry and account eligibility. Acceptance requires an authenticated
account, current accepted SQL membership, authoritative MYSQL/live progression,
Krynn, pilot/public admission, contracts enabled, validated catalog/extension,
a quartermaster and no combat or timed craft. Buffs need not be enabled.

Definitions have unique stable IDs and frozen version-1 terms, including the
20-unit/kill requirement, material grade/mob prototype, character band, original
account and clan incarnation. Never repurpose an ID for a different objective.
The catalog parser bounds the pool at 32, rejects malformed/duplicate IDs,
unsupported families, wrong quantities and invalid level/grade/type fields.
The game validates material grades and loaded mob prototypes. Initial choices
cover ash/hemp (and level-6 maple) harvesting, and low/middle/high combat bands
with two designated bounty alternatives per band. These are fixed pools;
rotation and broader reset administration remain Phase 32.

SQL provides one active or reward-pending slot per character per objective
family. Combined with the delivery slot, this enforces three personal contracts,
one each for delivery/gathering/bounty. Account/week/family/definition claims
span alts and clans and allow two different completions per family. Abandonment
and reacceptance do not erase claims. Partial objectives expire at the weekly
boundary; new acceptance can close an expired slot. Reward-pending objectives
retain their slot until completion and cannot be abandoned.

## Authoritative occurrences and recovery

`harvest_complete` captures an eligible accepted contract before the attempt.
Only its successful branch records the actual positive base quantity, bounded
by the node remainder. Critical, Efficient, happy-hour, mote and other bonus
output do not enter that counter. Failures never emit progress. Refine, buy,
transfer, delivery and inventory commands have no progress hook. The source
material is retained even if the successful harvest depleted its node.

The successful harvest checkpoints normal material/mote gains, task clearing
and its frozen occurrence together through `save_char_checked`. Player tags
`ClGQ` and `ClBQ` retain one bounded pending occurrence per family. SQL is fed
only after a successful checkpoint. A save failure leaves the same in-memory
marker for retry, with no SQL progress. Before another harvest starts or
finishes, unresolved markers are retried; storage outages pause harvesting.
No automatic repeat overwrites an unacknowledged gathering occurrence.

The Phase 25/26 live combat collector supplies bounty facts only for recipients
whose contribution query succeeds. It retains the encounter UUID and exact
prototype, current accepted account/incarnation, room and occurrence time.
Damage, healing, tanking and defense eligibility share the existing policy;
trivial/owned/summoned/test/manipulated encounters, inactive recipients and
same-account duplicate participants are excluded. Killing blow is irrelevant.
The bounty capture is independent of the small ordinary-kill reward's quota
or optional prototype-source extension, allowing both capped achievements to
coexist. Player checkpoints occur after releasing the collector's SQL connection.

Each frozen occurrence selects its original accepted UID; retry cannot select a
new contract. SQL serializes event deduplication, monotonic bounded progress and
the final pending personal receipt under the core control lock. The unique
account/family/event key prevents an encounter being credited to another alt
or clan. Identical replays remain no-ops after completion; changed facts conflict.
Progress is not reconstructed from command history, carried goods or XP.

Login/copyover entry, periodic source recovery, contract commands and subsequent
harvests retry saved markers. Once SQL acknowledges an event, clearing its
player marker is safe even if that final file acknowledgement fails. A ready
contract has a durable reward receipt, so a crash between counter completion
and payout cannot erase or repeat its award. Automatic kill occurrences that
could not be captured before the encounter is gone are not reconstructed later.
One pending bounty occurrence is retained; further kills while it cannot be
flushed are explicitly not queued. These bounds prevent an unbounded backlog.

## Personal and shared completion

The source tuple is fixed at 100 XP / 50 Influence / 50 Favor. The existing pure
personal policy supplies exact account bands, Favor/source quotas and clan caps.
A SQL-only reward participant commits the receipt, balances/counters, completed
contract and unique definition claim together. The generic personal worker
cannot bypass this participant for gathering or bounties. A concurrent alt that
loses a definition/source allowance is completed with a retained zero receipt;
no repeated acceptance can manufacture another claim. Character bonuses never
multiply progression credit. Pending work retains its original occurrence week
and clan identity through membership changes. Deleted clans do not receive new
credit; retained records support reconciliation instead of vnum redirection.

On the first qualifying personal bounty claim for a clan/week/definition, that
same transaction prepares one deterministic shared bounty receipt and unique
shared claim. A second member can earn personal credit without another shared
receipt. At most two different shared bounty definitions are accepted per
clan/week, including concurrent members. The existing shared award policy later
commits 200 XP / 100 Influence, with clan caps and no Favor/account beneficiary.
Pending receipts are recovered by the bounded source worker. Default-off or
unadmitted clans do not acquire new occurrences or rewards; saved source facts
may be persisted while decisions await enabled earnings.

The optional InnoDB extension has retained, immutable source/claim facts,
monotonic contract terms, foreign keys and unique family/event/definition keys.
Readiness checks required engines, version, retention triggers and unique index
column layouts. SQL and file/cache operations never share a borrowed pool lock.
Corrupt markers fail closed for staff reconciliation; no client field supplies
identity, source counts, eligibility or reward amounts.

## Validation

- `tests/run_clan_objectives.py`: production SQL services under ASan/UBSan in
  private MariaDB; strict catalogs, independent slots, access/level checks,
  target/grade/identity rejection, event replay/altered facts, pre/post-commit
  crashes, atomic reward/claim rollback, ambiguous award acknowledgement,
  shared bounty dedup, account/week limits and original-incarnation completion.
- `tests/run_clan_objective_runtime.py`: actual harvest handoff and live combat
  collector with private SQL and checkpoint adapters; base-vs-bonus yield,
  failed player save, saved-marker reconnect before/after SQL commit,
  damage/support/heal/tank qualification, alt dedup, wrong/trivial/owned targets,
  disabled collection and player-facing commands.
- `tests/run_happy_crafting.py`: actual harvest function verifies base-only
  counters for normal/critical/talent/happy-hour output, partial nodes, failed
  checks and blocked source capture. Autoharvest and ordinary source regressions.
- Prior delivery recovery, personal award/lifecycle regressions; Make and CMake.

Next: Phase 32 rotation/reset behavior. No new live settings were enabled.
