# Clan progression — Phase 4 handoff

Status: complete. Schema migration and disposable-database validation only. No game
database was modified, data imported, runtime storage switched, or feature enabled.

## Deliverables

- `sql/20261110_clan_progression_v001.sql`: explicit schema version 1, matching
  `CLAN_PROGRESSION_SCHEMA_VERSION` in `src/clan_progression.h`.
- `tests/run_clan_progression_schema.py`: applies that actual SQL through the
  MariaDB client to a private temporary server with networking disabled. It reads
  no game credentials and destroys its temporary data directory after shutdown.

## Storage contract

| Table | Purpose |
| --- | --- |
| `clan_progression_schema` | Singleton version receipt, written after all tables and triggers exist. Reruns preserve the original receipt. A different installed version rejects v1 before modifying clan tables/triggers. |
| `clan_progression_clans` | Immutable nonzero lowercase 32-hex incarnation UID and preserved numeric vnum; active/deleted status; name/description/abbreviation and saved leader ID; legacy gold treasury; timestamps; revision. |
| `clan_progression_state` | One state row per incarnation, with Clan XP and Influence in signed 64-bit eighth-points, update time, and revision. Foreign key references the incarnation, never the vnum. |

All three tables use InnoDB. Identity columns use ASCII binary collation. Vnums
are restricted to `1..65534`, excluding both absent-clan sentinels used by current
code. Core name and abbreviation limits match `MAX_CLAN_NAME` and
`MAX_CLAN_ABREV`; description remains TEXT. Zero leader ID and zero legacy
founding/activity timestamps represent unassigned/unknown values. Import must
validate malformed legacy records, rather than truncating or inventing values.

`active_vnum` is a generated nullable column with a unique index. Active rows
reserve their numeric vnum; tombstones do not. Thus repeated deletion/recreation
can retain multiple historical incarnations under one vnum while only one is
active. UIDs, vnums, and creation timestamps cannot be changed by UPDATE. A
soft delete must atomically set status 2, deletion time, and update time. Once
deleted, the core tombstone is immutable and cannot be revived. Physical DELETE
is rejected, including on progression state, preserving historical balances and
future references. UID replacement via REPLACE likewise cannot erase a clan.

Gold, XP, Influence, revisions and persisted Unix timestamps are signed BIGINTs.
Negative balances/revisions/timestamps are rejected; strict writes also reject
values beyond `INT64_MAX`. Gameplay timestamps can exceed 2038. Treasury remains
legacy gold, and must never be scaled into eighth-points. XP/Influence default to
zero and are independent of treasury. There is no stored level: later services
derive it from XP. Schema constraints protect storage ranges; configurable XP and
Influence caps remain the responsibility of checked award services.

The tables deliberately do not yet represent every legacy clan field. Application
settings, ranks/privileges, members/applicants, halls/claims, relationships,
statistics and legacy lock/cache reconciliation belong to Phase 4b and its
versioned extensions. Buff family/cooldown, Favor, account-window and receipt
schemas belong to their later persistence phases. This foundation is **not** a
complete legacy importer or readiness detector.

## Applying and retrying

No automatic boot/login migration is connected. Future operator execution must:

1. Back up the selected database and review its explicit name and storage mode.
2. Keep progression disabled and quiesce clan writers; serialize migration runs.
3. Use the mysql/mariadb CLI with the selected database and this file on standard
   input. Never pass `--force`: the client must stop at the first SQL error.
4. Require successful completion and verify the version, InnoDB engines, column
   definitions, foreign keys, checks, unique index and all four triggers before
   allowing a future repository to report storage readiness.

The script requires enforced CHECK support: MariaDB 10.2.1+ or MySQL 8.0.16+;
it rejects earlier server versions. The behavioral suite was run on MariaDB
11.8.6. MySQL syntax is targeted but was not independently executed against a
MySQL server. Operator privileges need table creation, SELECT/INSERT, and
routine/trigger create/drop permissions. Strict SQL mode is used during the
migration and restored afterward; Phase 4a repository sessions must themselves
require strict writes and checked conversions.

DDL implicitly commits. A fresh interrupted install has no final version receipt;
rerunning completes the missing objects without resetting populated rows. An
already-installed v1 can also be rerun offline without changing records or its
installation receipt. Triggers are recreated during a rerun, so do not run this
against concurrent writers. `CREATE TABLE IF NOT EXISTS` does not repair drifted
or incompatible preexisting definitions. A receipt alone is not proof of a
healthy schema, especially after an interrupted rerun. If schema drift is found,
stop and repair it through a reviewed migration, not a blind version change.

There is no destructive down migration. Do not drop history or reopen a stale
legacy file as rollback once MySQL-authoritative writes exist. Until cutover,
legacy storage remains unchanged and authoritative. Cutover/rollback rehearsal
is still required in Phases 5a–5b.

## Checks and results

- Disposable schema suite passed: fresh/repeated install; interrupted DDL/rerun;
  populated rerun preserving all rows and version receipt; three incarnations
  sharing a vnum with only one active; no inherited balances; invalid/uppercase/
  zero/short/long IDs; invalid/reserved vnums; duplicate keys; missing-parent FK;
  malformed tombstones; immutable identities; no resurrection or physical delete;
  negative and overflowing values; signed-64-bit boundary/large IDs and timestamps;
  transactional treasury/progression rollback; future-version refusal.
- `python3 tests/run_clan_progression_config.py` passed with strict compiler
  warnings, ASan and UBSan for Dragonlance, default and Forgotten Realms.
- `python3 tests/run_clan_login_notifications.py` passed.
- No compiled server target changed in this phase, so no server rebuild or
  deployment was needed. Existing unrelated work was preserved.

## Next entry point: Phase 4a

Implement checked MySQL repository reads/writes for these core fields, legacy
treasury, progression and timestamps, following current connection/transaction
conventions. Validate the actual schema before setting readiness; return explicit
unavailable/malformed/conflict results. Generate a random server-side UID, retain
it across retries, and rely on uniqueness rather than deriving it from a name or
vnum. Create the core and zeroed progression rows in one transaction. Preserve
all 64-bit values with checked conversion, and use revision predicates for
optimistic writes. A successful SQL round trip must not imply membership or
permission authority yet. Ordinary mutations must reject deleted clans; later
operation-recovery services must respect retained tombstones and receipts.

Do not connect boot import, gameplay awards, buffs, web output, or feature
activation as part of Phase 4a. Continue with ranks/membership/schema extensions
in Phase 4b and gated legacy caller integration in Phase 4c.
