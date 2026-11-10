# Clan progression: Phases 5a, 5b and 6 handoff

Status: complete in code and disposable storage. Follow-on [Phase 8](clan_progression_phase_8_handoff.md) raises the current required schema to version 5; version-3 references below describe this historical phase. Phase 5b was completed as the prerequisite for requested Phase 6. No game database migration, live import/cutover, restart or progression enablement was performed. Next: Phase 7 account contribution windows, then Phase 8 durable award/economic receipts.

## Boot selection and defaults

`CLAN_STORE_MODE` selects `legacy` (also the absent-variable default), `shadow`, or `mysql`. The process environment takes precedence over the existing `.env`/`lib/.env` reader, then absence defaults to legacy. It is documented in `lib/.env_example`; this task does not change the deployed environment. Invalid values stop startup. The normal server boot calls `clan_persistence_boot()` after the world and player index exist, before clients/copyover characters load. Already spawned NPCs are permitted; live player characters prevent reattachment.

Legacy loads the existing clan/claim files and keeps legacy behavior. Shadow loads those files and compares checked SQL metadata without SQL writes; mismatch/unavailable storage stops that requested boot. MySQL pins authority before reading SQL. It requires checked schema version 3 and persistent store mode 2; it never imports or changes authority implicitly. Outage, inconsistent metadata, missing/deleted player identity, unsupported timer rules, conflicting occupied slots or attachment failure stops startup, with no legacy fallback. Existing runtime outage/access guards remain in effect.

The loader builds complete temporary clan/claim data before publication, preserving vnums, strings/colors, treasury, legacy statistics, ranks, explicit privilege indices, incarnation relationships and popularity. It rebuilds the clan hash and attaches the checked adapter. Player-index clan fields and subsequently loaded character clan/rank fields are reconciled from SQL membership; personal legacy clan points stay separate. No new Favor is inferred from them. Core/progression records load existing persisted values; a fresh authoritative store safely loads empty. Future-dated creation timestamps that cannot be reconciled with the server clock fail closed rather than being rewritten.

All progression configuration remains OFF and feature flags remain false. SQL-mode economic/file transactions still require Phase 8 receipts; SQL clan creation/destruction still require Phase 11 lifecycle integration. The boot change does not remove these gates.

## Schema and family timers

Separately reviewed offline migration order is v001, v002, v003. Existing version-2 installations apply only v003; do not replay older migrations against a newer receipt. Version 3 reruns preserve populated data. The generated repository contract checks columns/defaults, composite keys, foreign keys, constraints and retained-history triggers. No migration is run automatically at server boot. MariaDB 11.8.6 is behaviorally tested; MySQL 8.0.16+ remains a target, not independently verified here.

`clan_buff_families` retains one current activation record per incarnation/family, including activation UID, tier/slot/rules version, saved start/expiry/next-available times, price and cancellation. Fresh/imported clans have no synthetic activations or cooldowns. Boot loads supported, validated records into a read-only cache; canceled/expired rows remain persisted. A buff is active exactly at `started_at <= now < expires_at` and only if uncanceled. Its saved `next_available_at` survives expiry/cancellation/restart unchanged. Boot never extends or recomputes timers from current definitions. Activation/cancellation services and gameplay eligibility/effects remain later phases; the cache accessor alone is not an eligibility service.

## Explicit offline cutover

Use the Phase 5 importer against a quiesced, reviewed snapshot, compare it, keep its private raw backups and take a complete SQL backup. Stop all clan writers for the transition. The native bridge now exposes a checked `cutover` operation, taking JSON on stdin and database/user/host/socket arguments. Credentials remain environment-only. Examples below use placeholders and do not target a game database:

```sh
python3 scripts/build_clan_repository_tool.py /private/clan_repository_tool
# Imported staging store: revision from a fresh bridge read, fingerprint from import.
printf '%s\n' '{"expected_revision":1,"source_digest":"REVIEWED_64_HEX_DIGEST"}' | \
  /private/clan_repository_tool cutover STAGING_DATABASE IMPORT_USER '' /path/to/staging.sock
# Fresh empty staging store instead: an explicit fresh=true acknowledgement.
printf '%s\n' '{"expected_revision":0,"fresh":true}' | \
  /private/clan_repository_tool cutover STAGING_DATABASE IMPORT_USER '' /path/to/staging.sock
```

Cutover locks control, checks the expected revision, and atomically advances both import receipt and authority. Imported mode requires the named uncut receipt; fresh mode requires empty clan/history/member/claim data. A stale revision, unknown receipt, already cut-over store or nonempty fresh request fails. Failed writes roll back the receipt and mode together. On an uncertain acknowledgement, read control and receipt first: mode 2 plus the expected advanced revision and receipt status 2 reconciles success; do not blindly retry or reset history.

After the approved offline transition, preserve `CLAN_STORE_MODE=mysql` in the deployment configuration. Restart using SQL authority and compare runtime metadata/mirrors. Store selection is a boot setting, not a player command, and no reverse-cutover API is supplied.

## Phase 5b recovery rehearsal and rollback policy

The disposable test exercises import, failed cutover rollback, successful cutover, rejected stale retry, repeated boot, a post-cutover treasury write, and rejection of a stale legacy reimport. It then takes a complete transactional SQL dump with triggers, restores into a separate private database, and compares its authoritative snapshot including the post-cutover write, memberships and incarnation mapping, with separate checks of restored Favor and cooldown values. Existing repository/import tests cover tombstones and vnum reuse without resurrection.

Before cutover, keep legacy as authority; failed staging work can be discarded without changing game files. After cutover, the old files are stale mirrors/backups: changing the environment to legacy or merely reopening those files is not a rollback. Stop writers, capture the latest complete SQL state (including new clan tables, receipts, Favor and timers), and use SQL-preserving recovery with the compatible checked binary. Reconcile player mirrors from that recovered SQL authority. A binary/schema downgrade or reverse export needs a separately reviewed conversion and accounting for all post-cutover writes; none is automated. Never reset control/receipts to make an old import fit. The test's reset to an empty fixture is explicitly private test setup, not an operational rollback recipe.

## Personal Favor persistence

`clan_personal_favor` is keyed by positive signed-64-bit player ID plus immutable clan incarnation UID. It contains cumulative Favor in eighth-points, optimistic revision and update timestamp. `clan_repository_favor_load()` addresses any historical pair independently of current membership; missing data returns NOT_FOUND without modifying the caller's output. It never derives standing from clan points or another incarnation.

The checked persistence writer requires MySQL authority, active target incarnation, nonnegative signed-64-bit values, revision comparison and nondecreasing timestamps/balance. Expected revision -1 inserts; existing updates advance revision once. SQL triggers forbid reductions, identity changes and deletion. Equal-value persistence is permitted; there is no spend/debit API. Leaving/switching clans does not erase rows; a tombstoned clan remains readable, cannot receive writes, and a new clan reusing its vnum inherits no Favor. No officer/award/purchase command is added. Actual earned awards, receipts, budgets and quartermaster actions remain later phases; future award services must compose this data in their receipt transaction, not make separate non-atomic commits.

Repository operations disable client automatic reconnect on their exclusively acquired connection so a lost transaction cannot silently continue in autocommit. Unavailable storage and uncertain commits remain explicit results; callers reconcile persisted revisions/receipts.

## Verification

`tests/run_clan_boot_favor.py` compiles the real boot/adapter/repository with strict warnings and ASan/UBSan, using private socket-only MariaDB. It checks selection/defaults, NPC-populated boot, fresh/imported hydration, relationships/claims, mirrors, malformed timers, missing players, outage/no fallback, cutover atomicity/retries and complete SQL recovery. Favor checks cover missing rows, two clan standings, leaving, reconnect, cumulative monotonicity, revision races, backward time, INT64_MAX, retained tombstones and vnum reuse. Schema reruns preserve Favor/timers. Existing repository/snapshot/persistence/import, configuration/campaign and clan login regressions pass; CMake and Automake server builds pass. No live configuration or credentials are used by tests.
