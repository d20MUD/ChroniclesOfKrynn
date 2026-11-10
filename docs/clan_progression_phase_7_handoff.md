# Clan progression: Phase 7 handoff

Status: complete as disabled persistence foundations. Follow-on [Phase 8](clan_progression_phase_8_handoff.md) raises the current schema requirement to version 5 and adds transactional receipt composition; version-4 references below describe this historical phase. Next is Phase 8: durable reward receipts and atomic counter/balance composition. No game database migration, live account/window writes, restart, authority change or progression enablement occurred.

## Retained account windows

Offline migration `sql/20261110_clan_progression_v004.sql` adds `clan_account_windows`, keyed by positive numeric `account_data.id` and occurrence-time Monday at 00:00 UTC. No character ID or clan identity forms part of this key. Every alt and clan affiliation on that account therefore addresses the same weekly budget; other accounts and weeks remain independent. Trusted future callers must supply the real account identity and reject missing/invalid accounts, never substitute a character ID.

Each row stores base Clan XP charged to account bands and realized Favor in eighth-points, six explicit account-source claim counters, optimistic revision and last write time. Source counters cover ordinary kills, supply orders, deliveries, gathering, personal bounties and personal expedition completions. Index zero in the C claims array is reserved and must stay zero; indices 1–6 are the existing explicit source IDs. Shared clan-source counters belong to later clan aggregate persistence, not this record.

Windows are retained rather than cleared. Reaching a boundary selects a new key; old rows survive for delayed operations, receipt recovery and audit. There is no periodic reset writer, login writer, new character budget, or clan-switch reset. Source-definition uniqueness, expedition-family uniqueness, daily prototype counts and claim receipts remain the later event/contract services' responsibilities. This phase supplies weekly usage storage and daily boundary arithmetic, not those event trackers.

## Deterministic UTC boundaries

`clan_progression_week_at()` and `clan_progression_day_at()` take trusted occurrence-time epoch seconds and return half-open `[starts_at, ends_at)` periods. Exactly Monday at midnight selects the new week; exactly midnight selects the new day. Calculations use checked integer arithmetic, independent of local timezone, DST, `time_t`, SQL session timezone or process restart. Tests inject timestamps directly; helpers never read the wall clock or accept browser authority.

Negative occurrence times and periods whose next boundary cannot fit signed 64-bit fail without changing output. Unix epoch's first partial week correctly starts Monday 1969-12-29 (`-259200`); this is the sole permitted negative weekly start. Counters, account IDs, revisions and write times retain their normal nonnegative/positive constraints. The next boundary must remain representable, including near INT64_MAX. The SQL alignment constraint matches the C calculation.

An operation retries against its original occurrence week, not the week in which recovery runs. A delayed write may update a historical week using a current write timestamp; it does not change the new week's budget. An account-wide wall-clock rollback does not erase either week's retained rows. Future receipt services must freeze occurrence identity, rewards and cap decisions rather than recalculate them after a reset.

## Checked repository API

`clan_repository_account_window_load(db, account_id, occurred_at, out)` computes the week and reads its authoritative totals/counters. A missing row returns NOT_FOUND with output unchanged. `clan_progression_account_window_init()` prepares a proposed zero record without writing SQL; a read or reconnect cannot create or reset a window.

`clan_repository_account_window_save(db, record, expected_revision, written_at)` owns its transaction and requires checked MySQL authority. Expected revision -1 inserts revision zero; existing updates require the exact loaded revision and advance it once. All totals/counters and write timestamps are monotonic. Duplicate inserts, stale revisions, attempted decreases and backward write times fail without resetting usage. Identity/time/range errors are explicit. SQL triggers independently prevent re-keying, reductions, revision resets and deletion. The actual schema, keys, constraints and trigger bodies are checked by the generated contract; a version receipt alone is insufficient.

Missing/unavailable storage has an explicit failure result with no file/cache substitute. Corrupt stored values fail closed and leave output unchanged. Client automatic reconnect remains disabled on the repository connection. An uncertain commit requires re-reading the same account/week and reconciling the operation; never blindly add usage again.

These are trusted persistence primitives, not award endpoints. They do not validate content completion, enforce contribution caps, compute diminished reward tuples or mint XP/Favor. The existing specification's 600/600 base-XP bands, 600 realized Favor cap and source quotas remain unchanged. Counters are stored as signed-64-bit bounded values rather than hard-coded SQL balance caps, allowing future versioned service policy.

Before connecting awards, Phase 8 must compose account counters, clan/personal balances and unique receipts in one SQL transaction. The current standalone save rejects an already active caller transaction; it must not be called as an independent commit before/after an award receipt. Extend transaction composition deliberately in Phase 8. A revision conflict detects competing callers, but does not replace event deduplication or prove an uncertain award was committed.

## Migration and compatibility

Required schema version is now 4. Existing checked version-3 stores apply only v004 offline; fresh staging stores install v001, v002, v003 then v004 in order. Migration reruns preserve populated windows; unknown/future versions are refused. The server never installs a migration automatically. Prior v003 buff/Favor data and all default feature gates remain unchanged. Update the checked binary and schema together during a separately reviewed maintenance operation; this task does not alter the deployed environment.

The schema generator, native bridge, import/boot/persistence regression fixtures and environment example use the new contract. MariaDB 11.8.6 is behaviorally verified; MySQL 8.0.16+ remains the target and was not independently exercised here.

## Verification

`tests/run_clan_account_windows.py` compiles real production code with strict C99 warnings, ASan and UBSan in a private socket-only MariaDB fixture. It checks shared usage through independent connections, independent accounts, retained weeks, delayed writes, reconnects, insert/update conflicts, injected write rollback, monotonic totals/source counters, missing/invalid/corrupt records, caller-transaction refusal, history guards and signed-64-bit boundaries. SQL constraints, populated migration reruns and missing-trigger detection are exercised.

Weekly/daily boundaries are compared with an independent Python UTC datetime oracle for over 1,000 timestamps, including epoch, exact Mondays/midnights, leap dates, DST transition dates and post-2038 dates, under UTC, America/Halifax and Pacific/Auckland. Additional cases cover the representable int64 edge without relying on platform datetime limits.

Existing core repository, full snapshots, gated persistence, legacy importer, boot/cutover/recovery/Favor, campaign/configuration and clan-login regressions pass. CMake and Automake server builds pass. All SQL work uses disposable databases, not game credentials or configuration.
