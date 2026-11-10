# Clan progression: Phases 4a–4c handoff

Status: complete as disabled persistence foundations. No live database migration, server restart, authority switch, or gameplay enablement was performed. Next: [Phase 5 import handoff](clan_progression_phase_5_handoff.md), then implementation Phase 5a.

## Repository and schema

Apply `sql/20261110_clan_progression_v001.sql`, then `sql/20261110_clan_progression_v002.sql` only through a separately scheduled offline migration. Never rerun version 1 after version 2. Version 2 adds full legacy details, memberships, claims, import receipts and store control. The control row defaults to legacy authority; configuration and progression defaults remain disabled.

`src/clan_repository.h` exposes checked core and full-snapshot operations on a caller-owned exclusive MySQL connection. Results distinguish invalid input, missing rows, unavailable storage, schema mismatch, revision conflict, deleted identity and uncertain commit. Callers must reconcile uncertain commits rather than blindly retry. Writes use InnoDB transactions, global snapshot revision and core/state revision comparisons. Balances, timestamps and revisions use checked signed 64-bit integers; XP and Influence use eighths. New incarnations start with zero progression. Tombstones retain identities and cannot be revived; vnum reuse gets a new UID.

Readiness verifies actual columns, defaults, keys, checks, foreign keys, generated expressions and trigger bodies, not just the version receipt. `scripts/generate_clan_schema_contract.py --check` verifies the generated contract. MariaDB 11.8.6 was exercised; MySQL 8.0.16+ is targeted but was not independently tested.

## Full legacy coverage

`src/clan_legacy_fields.h` is the shared mapping for application settings, taxes, war timer, hall, member limit and historical statistics. Core data preserves names, descriptions, abbreviations, leaders, treasury and available timestamps. Fixed, validated JSON details preserve ordered rank names, all 21 explicit privilege IDs and relationship incarnation IDs. Separate membership rows preserve stable player IDs, applicant rank zero, accepted ranks and legacy point mirrors. Zone claims preserve controller, claimant and popularity mapped to incarnation IDs. Legacy personal points are not new Favor authority.

Full snapshots validate every field, reference, bound and duplicate before transactional replacement. Canonical ordering makes comparisons independent of legacy array order. Existing progression is preserved; failed child writes roll back the entire snapshot.

## Gated integration

`src/clan_persistence.c` adapts clan writers, player load/save mirrors and existing mutation callers. Default legacy behavior remains in place. Selecting MySQL pins that authority before contacting storage: failure never falls back to stale files. Attachment requires a checked MySQL-authoritative store and already reconciled runtime metadata. Shadow attachment compares metadata without SQL writes; continuous shadow diagnostics are not implemented.

Successful SQL changes update caches and clear dirty flags. Failed writes preserve dirty state and invalidate clan access. Membership and rank fields in player files/indexes become mirrors. Ordinary unrelated character saves can continue during an outage when membership is unchanged or disabled; attempted membership changes fail closed.

SQL boot hydration/configuration remains Phase 5a. SQL-mode loading explicitly refuses legacy file hydration. Gold transfers, taxes, investments and legacy multi-store transactions remain gated until Phase 8 durable receipts. SQL clan creation/destruction remain gated until Phase 11 incarnation lifecycle work. These gates are intentional and must not be removed to enable a pilot prematurely.

## Validation

Production repository and adapter tests use private socket-only MariaDB instances, strict compiler warnings and ASan/UBSan. Coverage includes full round trips, 64-bit boundaries, transactional rollback, conflicts, tombstones, schema drift, mirrors, outage handling and absence of legacy fallback. Existing clan login regression and both CMake and Automake server builds pass. Native description-reader tests also exposed and fixed a one-byte long-line buffer overrun without changing decoding rules.
