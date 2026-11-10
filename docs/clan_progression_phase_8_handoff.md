# Clan progression: Phase 8 handoff

Status: complete as disabled SQL transaction/recovery foundations. Work was split into 8a receipt identity/durable acceptance, 8b atomic composition and 8c crash/replay/concurrency verification. Next: Phase 9 Clan XP/level/Influence service, then Phase 10 Favor/budget policy. No game database migration, live awards, restart, authority switch or feature enablement occurred.

## Server-only receipt API

`src/clan_rewards.h` exposes the checked coordinator implemented in `src/clan_repository.c`. It owns an exclusively acquired connection for the full transaction, using the existing repository's strict schema/session checks and disabled automatic reconnect. No browser, officer command, gameplay hook or automatic boot worker is connected.

A trusted `clan_reward_event` freezes operation UID, source event/claim UID, clan incarnation, account/player identity, occurrence time, rules version and requested eighth-point reward tuple. Use the actual stable encounter/accepted-contract/definition-period identity, never a fresh event UID per retry. Personal sources use a player beneficiary; shared sources use the clan incarnation. The unique `(event_uid, source_id, beneficiary_key)` constraint prevents the same personal event/source/player being re-awarded through a different clan or operation nonce. Different source achievements and eligible beneficiaries remain distinct. Higher-level definition/family uniqueness remains the corresponding later service's responsibility.

`clan_reward_prepare()` durably accepts these facts before acknowledgement. Replaying identical facts returns the original operation/receipt, including when the caller generated a different operation nonce. Changed target, identity, timestamp, rules or requested tuple conflicts. Existing receipts are checked before capacity and active-clan checks, so retrying accepted work does not create another queue item. Actual eligibility, account ownership and occurrence-time membership are trusted source-service responsibilities; positive numeric identity alone does not establish eligibility.

Pending acceptance is bounded by the configured 100 operations per character and 10,000 globally. A shared operation has no personal beneficiary and uses the global bound. The control lock serializes capacity checks/inserts; `CLAN_REPO_CAPACITY` explicitly rejects excess work. Missing storage cannot durably accept an event; callers must not consume goods or invent a later backlog award. Ordinary combat/loot remains unaffected because no hook is connected.

`clan_reward_pending()` enumerates durable operation UIDs with player filtering, bounded pages and an ordered cursor; `clan_reward_load()` returns the immutable event and status. UID pagination is not a monotonic acceptance sequence: newly accepted random IDs may sort before a prior cursor. A future worker must begin fresh bounded sweeps from the start rather than treat its cursor as proof that no earlier work exists. No in-memory queue is relied on. Status 0 is durable pending, 1 means all SQL rewards/counters committed, and 2 is explicitly canceled. Status 1 does not mean a player/inventory file was delivered.

## One atomic award

`clan_reward_complete()` locks a pending receipt and reads account usage, clan state, personal Favor and the clan weekly budget. A trusted decision callback receives those locked values plus the accepted event and returns credited XP/Influence/Favor, charged base account XP, zero/one source-claim charge, incoming account-adjusted clan XP and versioned cap-explanation flags.

The callback must be pure: no SQL, files, live character mutation, commits or irreversible side effects. It may run again after a rollback; it is never run for a committed replay. Requested reward values/rules are frozen on acceptance, while the successful cap decision is made against locked counters and frozen at commit. A pending conflict/retry must not invent a new occurrence week or requested tuple. Future policy versions must retain the ability to interpret accepted older rules rather than silently applying new definitions.

The coordinator atomically stages:

- The original account/week's base-XP and realized-Favor usage and applicable source counter.
- Clan XP and stored Influence with checked signed-64-bit addition/revisions.
- Cumulative personal Favor for the original player/incarnation.
- Clan/week incoming XP and realized Influence usage.
- The terminal receipt, exact credited tuple/counter charges/cap flags, and before/after balances/counters/revisions.

Zero/capped outcomes still receive a terminal deduplication receipt. All-zero unused rows need not be created. Shared awards do not create account/Favor rows. Account, Favor and weekly counters remain monotonic and retained. The callback supplies reward/cap policy in later phases; this coordinator enforces bounds, consistent source scope, credited amounts no greater than requested, and integer safety, not the eventual level/budget formulas.

The documented lock order is store control → receipt → account window → clan core/state → personal Favor → clan week. The control lock also coordinates existing authoritative writers; more fine-grained concurrency would require a separately reviewed lock-order change. Internal account/Favor write stages neither commit nor roll back their caller's transaction. Existing standalone save APIs retain transaction ownership and reject nesting.

An interrupted stage rolls back every mutation and leaves the previously accepted receipt pending. After a successful commit, every replay returns the original receipt without incrementing a balance/counter or calling policy again. Lost acknowledgement returns COMMIT_UNKNOWN; recover the same operation/event key and inspect its status. Do not refund and retry as a new event, or independently add counters to compensate.

`clan_reward_cancel()` explicitly terminates only pending facts with a retained reason. Cancellation frees pending capacity, preserves the unique event key, and never refunds committed rewards or reopens a canceled claim. Active-state validation rejects new awards to tombstoned incarnations; already committed receipts remain replayable after departure/deletion. Pending work remains discoverable for explicit resolution rather than being silently deleted. Phase 11 must define any additional lifecycle policy without transferring work to a reused vnum.

## Occurrence periods, caches and recovery

All period keys are calculated from the frozen occurrence timestamp with the Phase 7 UTC helper. Recovery after Monday still reads/writes the original retained account/clan week. It does not create a fresh allowance in the new week or recalculate the original request.

These APIs operate on SQL rather than live clan/player objects. Before connecting services, Phase 9 must refresh/invalidate the runtime adapter's committed progression cache after an award. A stale legacy writer's existing state-revision predicate prevents it overwriting newer SQL awards; it must reconcile rather than retry stale data. Feature/readiness gates and all current economic/lifecycle gates remain unchanged.

## File-backed gold/material/inventory protocol

The current reward coordinator changes only SQL-owned progression and usage. It neither consumes inventory nor modifies gold, player files or objects, so its tested recovery has no cross-file write. Existing SQL-mode gold/tax/investment/creation/destruction gates stay closed. Do not remove them based on the presence of a reward receipt.

For future resource-consuming/delivering actions (including Phases 29 and 40), the required protocol is:

1. Validate storage, identity, permissions and resources before changing anything. Persist a bounded, immutable operation/input manifest with stable object/delivery IDs, exact amounts and expected checkpoint versions. Block concurrent mutation of those resources during recovery.
2. Use a durable prepared checkpoint for any file-owned escrow/removal/binding, including operation markers. Persist both file and parent directory before advancing SQL authority. An ambiguous file save remains pending; never assume failure means the rename did not happen.
3. In the same SQL transaction as the authoritative purchase/reward/receipt, enqueue a durable delivery/consumption entitlement referencing that original operation. The receipt remains pending delivery until durable file acknowledgement is proven; SQL commit alone is not delivery.
4. Recover by reading both the original SQL receipt and saved operation markers. Prepared-but-uncommitted escrow can be restored with a checked durable checkpoint. Committed entitlements must be applied/confirmed exactly once, never both refunded and delivered. If the file marker proves application but SQL acknowledgement was lost, acknowledge without applying again. If save/rename/fsync outcome is uncertain, retain the lock/entitlement and retry reconciliation.
5. Acknowledge applied delivery only after re-reading/proving the saved marker and resource result, then release locks. Never regenerate object identities, discard pending deliveries because a clan disbanded, or use a memory-only queue. Refuse new work at pending limits before consuming goods.

The audited patterns are `durable_file_replace()` (file and directory fsync), the ordered estate money/delivery receipt stream, `save_char_checked()` plus `estate_receipt_saved()`, and the legendary transaction checkpoint/recovery flow. `estate_queue_money_receipt()` currently uses estate's own connection; calling it while holding a different pooled clan connection would not make one SQL transaction. A future adapter must accept the same connection or use an explicitly recoverable SQL outbox/checkpoint bridge, with fault tests at each boundary. No generic file-delivered acknowledgement API or unsafe inventory adapter is added here. Those concrete adapters must be completed before enabling an action that touches files.

## Schema and operations

Required schema is version 5. Existing version-4 stores apply only `sql/20261110_clan_progression_v005.sql` offline; fresh staging installs v001 through v005 in order. The new tables are `clan_reward_receipts` and `clan_weekly_windows`. Named constraints/indexes, ASCII binary recipient identities, actual trigger bodies and retained histories are checked by the generated contract. New receipts start pending; terminal receipts cannot be edited/re-keyed/deleted. The migration refuses incompatible versions and populated reruns preserve state. No migration runs at boot.

Use full transactional SQL backups including receipts, budgets and earlier account/Favor tables. Stale legacy imports and resetting receipt/control rows are not recovery. MariaDB 11.8.6 is behaviorally verified; MySQL 8.0.16+ remains the target, not independently exercised here.

## Verification

`tests/run_clan_rewards.py` compiles the real production code with strict C99 warnings, ASan and UBSan in a private socket-only MariaDB instance. It exercises durable acceptance and exact/changed/nonce retries; all-stage rollback; actual process exits at eight acceptance/write/commit boundaries; acknowledged and unknown commits; recovery across a reset; distinct/shared/zero outcomes; malformed decisions and overflow after earlier stages; immutable cancellation/deletion history; concurrent identical and different claims; per-character/global pending capacity; pagination; populated migration reruns and schema drift. Existing repository, snapshot, persistence, importer, boot/cutover/Favor, account-window, configuration/campaign and clan-login regressions pass, as do both server builds. No game credentials or live configuration are used.
