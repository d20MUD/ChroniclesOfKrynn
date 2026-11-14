# Promote a saved build zone to live

Senior staff (`LVL_GRSTAFF`, level 33, or higher) can run these commands on build or live:

```text
zpromote 700 check
zpromote 700
```

`check` previews file creates, replacements and removals without writing files
or creating a backup. The second command completes a timestamped live backup
before promoting any files. It reports the backup directory on success.
Promotion refuses zones with pending OLC saves. Save your edits first.

The command accepts either configured world directory and is not tied to a
hardcoded TCP port number. The source and destination remain fixed as build to
live on either port. It copies saved files, not unsaved in-memory editor contents.
The game invokes Python directly with separate arguments, without a shell.

## Files and backups

All eight zone file types are included: `wld`, `mob`, `obj`, `zon`, `trg`,
`shp`, `qst`, and `hlq`. Both a build `.zon` and `.wld` file are required. The
source zone number, basic file terminators and index membership are checked.
This is a file transfer, not a complete world syntax/dependency validator.

The live result mirrors this zone's saved build file set. An optional file absent
on build is backed up and removed from live, including its live index entry.
Indexes retain other zones' entries and add this zone when necessary. Existing
live file permissions are retained. No other zones' data files are copied.

By default, files come from `~/build/mud/lib/world` and are promoted to
`~/live/mud/lib/world`, relative to the OS account running the MUD.
The separate backup tree is `~/live/mud/lib/zone-backups`, for example:

```text
zone-backups/
  700/
    20261107T180000.123456Z-<unique suffix>/
      manifest.20261107T180000.123456Z.json
      wld/
        700.wld.20261107T180000.123456Z.bak
        index.20261107T180000.123456Z.bak
      mob/
        700.mob.20261107T180000.123456Z.bak
        index.20261107T180000.123456Z.bak
      ... other file types ...
```

Every existing live file for the selected zone and all eight live indexes are
backed up, even if only some files need replacement. The manifest records
absent files as absent, along with the actor, UTC timestamp, original modes,
SHA-256 hashes, source/destination paths and operation status. Timestamped
batch directories have unique suffixes, and backup files are never overwritten.
A no-op promotion reports that build and live already match and creates no backup.

Each backup file and its directory are synced before promotion. All new files
are staged, then individually replaced by atomic rename. Ordinary I/O failures
roll back files already changed, retaining the backup and a failure status.
If rollback itself fails, the error names the backup for manual recovery.
A process crash or host failure during a multi-file transfer can still require
manual recovery: the files are individually atomic, not a single filesystem
transaction. The manifest remains available in the backup tree.

A single lock serializes promotions across zones because indexes are shared.
Changes detected during backup/staging cause the operation to abort. Coordinate
live editing with the transfer: it does not lock the live server's OLC editors.

## Activation and recovery

The command does not reload rooms or prototypes in the running live server and
does not restart it. Reload/restart live after a successful promotion before
saving or editing that zone there, so old in-memory data cannot overwrite the
promoted files. No build room/prototype records are changed by promotion.

## Restore a backup

List this zone's backup IDs, preview the restore, then restore:

```text
zpromote 700 backups
zpromote 700 restore <backup-ID> check
zpromote 700 restore <backup-ID>
```

Use the complete, case-sensitive directory ID printed by `backups`, such as
`20261107T180000.123456Z-AbCd1234`. Paths are not accepted. Listing and previewing
are read-only. Listing reports each snapshot's status, operation and actor.

Before restoring, the helper validates the selected manifest, all sixteen
zone/index entries, and every saved file's SHA-256 checksum. Incomplete,
corrupt, missing or symlinked snapshot files are refused before live changes.
Snapshots from completed backups remain usable even when the original transfer
failed or was interrupted after its backup finished.

**Every actual restore first creates a new timestamped backup of the current
live zone and all eight live indexes.** This also happens when live already
matches the selected snapshot. The returned backup ID can undo the restore.
Its manifest records `operation: restore` and the selected `restore_from` ID.
The same promotion lock, staging, syncing and rollback mechanisms protect
both operations. A backup failure prevents the restore from starting.

Restoring applies the snapshot's original zone-file bytes and permissions,
removes files that were previously absent, and restores only this zone's
membership in the current live indexes. It does not overwrite full indexes
with historical copies, so later additions of other zones remain registered.
Restoring a snapshot made before a zone first existed therefore removes that
zone's promoted files and index membership. Current build zone files and OLC
saves are not required for restoration and are not changed.

A restore does not reload or restart live. Reload/restart live after success
before editing or saving the restored zone there, just as for promotion.

For manual recovery if necessary, the manifest's `before` and `backup` fields
identify the original files and absent-file records. Full index snapshots are
provided for reference; avoid replacing an entire shared index with an old
snapshot if other zones have been added since that backup. A
`rollback-incomplete` or `copying` manifest identifies an interrupted transfer.

## Installation

Build the server with the new `zpromote` handler and install/restart the
server binary using the usual process. Install the helper for both ports:

```sh
install -m 0644 lib/misc/zone_promote.py /home/krynn/build/mud/lib/misc/zone_promote.py
install -m 0644 lib/misc/zone_promote.py /home/krynn/live/mud/lib/misc/zone_promote.py
```

The command finds it as `misc/zone_promote.py` relative to the MUD's data
working directory. It requires Python 3.8 or later on PATH and Unix file locking.
The OS account running the command must be able to read build/live files and write
live world directories and the backup tree. The game-level command permission
is separate from these filesystem permissions.

For other installations, set service environment variables before starting each server:

```text
KRYNN_BUILD_WORLD=/absolute/path/to/build/lib/world
KRYNN_LIVE_WORLD=/absolute/path/to/live/lib/world
KRYNN_ZONE_BACKUPS=/absolute/path/to/live/lib/zone-backups
```

The source and destination must be separate trees, and backups must be outside
both world trees. Symlinked type directories and zone/index files are refused.
An optional help entry is in `sql/20261107_zone_promotion_help.sql`.

Run `python3 tests/run_zone_promote.py` for disposable-directory I/O, rollback,
backup-order, restore/undo, index, permissions, origin, and staff-command dispatch tests.
Tests do not promote anything to the real live world.
