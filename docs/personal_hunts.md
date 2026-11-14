# Personal hunt targets

`hunts` at a huntsmaster now shows the requesting character's targets, rather
than a list shared by the whole server. The first request starts that
character's two-hour rotation. Reconnecting, copyover, and restarting the
server retain the saved list and completion state.

The existing level bands remain 10, 15, 20, 25, and 30, with one target per
eligible band. For example, a level 17 character receives level 10 and 15 hunts;
a level 30 character receives all five. Characters below level 10 have no
eligible hunts. No target above the character's current level is listed or
spawned. Losing levels removes over-level assignments. Newly unlocked bands
become available on the next rotation.

Only the owner's movement triggers their targets, including when following a
party leader. Spawned targets carry the owner's stable character ID, rotation
expiry, and slot. Another character's target of the same species does not
block spawning or clear an unrelated assignment. Party assistance and the
existing party reward rules remain available. Completion is recorded before
hunt-specific rewards, even if the owner has disconnected.

Hunt state lives in MySQL table `personal_hunt_progress`, keyed by character
ID. Each row holds the rotation expiry, five target slots, and a version.
First access reads the row into memory; movement and repeated list views use
that cache. SQL writes occur only on generation, refresh, level-loss pruning,
or completion. Updates require the expected version, so stale state cannot
overwrite newer progress. Failed writes pause the operation and force a reload
before the next attempt. Progress never falls back to filesystem storage.

Apply `sql/20261114_personal_hunts.sql` to the intended database before starting
the new binary. The migration is additive and can be reapplied without changing
existing rows. It is also included in `sql/master_schema.sql` for new databases.
No player-file rewrite is needed. Development installation changes `krynn_dev`
only; do not copy its progress rows to build or live.

If an earlier filesystem version generated `lib/etc/hunts/<character-id>` files,
stop that game and back up the files and database first. Generate an import:

```sh
python3 /home/krynn/code/scripts/import_personal_hunt_files.py \
  --directory /home/krynn/dev/mud/lib/etc/hunts > /tmp/personal-hunts-import.sql
```

Run that SQL through the usual MariaDB batch client against the matching game
database, without `--force`. The importer inserts only: duplicate character IDs
abort and roll back the batch instead of overwriting SQL progress. It leaves
source files untouched. No existing hunt files were found on dev, build, or
live during the initial conversion to MySQL.

Validation: `python3 tests/run_personal_hunts.py` runs the actual state,
selection, encounter lookup, and completion functions against a private MariaDB
instance with address/undefined behavior sanitizers. It checks levels 1–34,
separate locations and completions, same-species ownership, offline completion,
restart persistence, expiry, invalid records, failed writes, concurrent-update
conflicts, uncommitted connection rejection, cached reads without extra SQL,
and legacy-file imports including rollback on duplicate identities.
