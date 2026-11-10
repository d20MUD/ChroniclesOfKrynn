# Clan progression: Phase 5 import handoff

Status: complete. The offline importer and repeatability checks are implemented; no live import or authority switch was performed. Next is Phase 5a SQL boot loading/configuration and reconciliation, followed by Phase 5b cutover/rollback rehearsal. See [persistence prerequisites](clan_progression_phase_4abc_handoff.md).

## Operator workflow

Use a reviewed, quiesced copy of the legacy clan file, claims, player index and player tree. The tool does not stop writers. First run a dry run; no database is needed:

```sh
python3 scripts/clan_legacy_import.py \
  --clans /snapshot/clans --claims /snapshot/claims \
  --index /snapshot/index --players /snapshot/players
```

After separately installing schema versions 1 then 2 into an empty staging database, add `--database STAGING_DATABASE --user IMPORT_USER --socket /path/to/staging.sock` to compare against SQL. Passwords, when required, use `MYSQL_PWD`, not command arguments. The tool builds the native bridge automatically, or accepts `--tool /path/to/clan_repository_tool`.

To perform the reviewed staging import, add:

```sh
--apply --writers-quiesced --backup-dir /private/clan-import-backups
```

Apply requires an empty, legacy-mode staging store. There is no force/overwrite option. The importer never installs schema, changes authority, or enables gameplay. Keep backups outside the player tree. Secure the invoking environment and backup parent directory.

## Preservation and repeatability

The parser validates all persisted clan scalars, rank/privilege records, positional alliances/wars, claims, player IDs, membership and applicant ranks. It maps positional references using original file order, not sorted vnums. Player free-text blocks cannot masquerade as membership tags. Invalid IDs, inconsistent index/player records, unsupported records or missing members fail before writes. Deleted entries are excluded and inconsistent deletion state is rejected.

Missing historical founding/activity timestamps remain zero (unknown); no historical XP, Influence or Favor is invented. Personal legacy clan points remain mirrors. Nonempty legacy investments block import because the legacy format has no reliable clan owner; resolve ownership separately rather than assigning it speculatively.

Inputs must be reviewed UTF-8. Description conversion matches the native reader's color escaping, CRLF continuations, blank lines, trailing spaces and long-line chunking. Ambiguous native termination or split UTF-8 fails for review. Raw source files are never rewritten.

Before committing, the importer rechecks captured source bytes and creates a unique private backup with raw files, hashes, source paths and vnum-to-UID mapping. Files are mode 0600, new backup directories 0700; files and directories are fsynced. A source fingerprint identifies an immutable receipt committed atomically with all imported data.

An unchanged rerun uses the saved mapping and compares committed data, reporting already imported without changing revisions. Changed source fingerprints, newer SQL balances, tombstones, changed memberships or a cutover receipt cannot be overwritten. If commit acknowledgement is uncertain, rerun the same snapshot to reconcile the receipt; do not create a new fingerprint or manually erase receipts.

## Validation and limits

Tests exercise dry runs without SQL writes, full-field comparison, reversed file order, malformed sources, free-text tags, backups and hashes, unchanged reruns, newer-state protection, tombstones and cutover refusal. The bridge uses the production C repository rather than independent Python SQL writes. All database tests ran in disposable socket-only MariaDB instances; no game credentials were used.

Phase 5a must load authoritative SQL data at boot, attach the runtime adapter and reconcile mirrors before any authority switch. Phase 5b must rehearse operational cutover and rollback. Existing economic/lifecycle gates and all progression defaults remain disabled.
