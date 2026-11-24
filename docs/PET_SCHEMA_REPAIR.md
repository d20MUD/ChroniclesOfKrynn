# Pet purchase and revival schema repair

The live server reported `Unknown column 'epic_doctrine'` when saving companions
and loading fallen pets. Companion persistence now reads/writes `epic_doctrine`,
`epic_expiry`, and `epic_identity` for every companion, including ordinary bought
pets and mercenaries. The deployed pet table lacked all three fields.

This made purchases roll back and refund their cost, and made revival fail before
loading the selected fallen pet. The earlier initialization helper knew about
the new fields, but the explicit pet migration did not include them. Merely
updating the executable therefore did not repair an already-running database.

Changes:

- `sql/20261119_pet_epic_persistence.sql` adds only the missing fields. It is safe
  to reapply and preserves live/fallen records, progression, gold and saved gear.
- The original `sql/20261027_pet_persistence.sql` now includes those fields too.
- New-table creation in `src/db_init.c` declares the fields directly; its existing
  upgrade path still handles older tables.
- The companion integration test reads the actual SQL files, reproduces the
  missing-column schema, applies the repair twice, checks existing-row defaults,
  and exercises real save/load, paid revival, failure refunds, and nested gear
  against a disposable MariaDB server.

## Live repair

Applied the three-column migration on 2026-11-19 after a pet-table backup.
The live database retained 314 companion records and 94 saved object records.
Both fallen pet IDs shown in the report remained fallen, ready for the player to
revive normally. No player was charged or revived by the repair itself.

Backup: `/home/krynn/krynn-backups/pet-schema-20261119T155955Z/pet_tables.sql`.
The backup directory and file are restricted to the account owner.

The running server can use the fields immediately; no restart is required for
this database repair. Normal player commands should be retried to verify the
in-game result. The server was not restarted and no player command was injected.

Validation: `python3 tests/run_purchased_pet_progression.py` passed using a
disposable MariaDB database; the full CMake build also passed.

## Copyover restoration and legacy gear tables

The copyover log also reported `Unknown column 'id' in 'ORDER BY'` for pet gear.
The production `pet_save_objs` table has an auto-increment `idnum`, whereas new
schemas declare `id`. The gear loader now supports either spelling while
preserving insertion order for equipped items and nested containers. It escapes
the owner name in both queries. No live-table rename is necessary.

Incomplete normal pet restoration now prevents subsequent companion saves from
replacing the owner's saved roster. This protects records and gear if a mobile
prototype is missing or saved equipment cannot be restored. A successful restore
on a later login/copyover clears the safeguard. A failed paid revival continues
to use its existing refund and fallen-record recovery behavior.

Regression coverage runs the database persistence suite against both `idnum`
and `id` tables, using the production gear-selection query, and checks that an
incomplete normal restoration leaves saved companion records intact.
