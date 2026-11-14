# Clan progression: live-port migration plan

Prepared 2026-11-13. This is a deployment plan, not a record of a production
migration. No live files, SQL, processes or feature settings were changed while
preparing it. The staffed pilot was waived; retain the approved conservative
pacing and use rehearsal, automated checks and dev verification instead.

## Target and boundaries

- Source code: `/home/krynn/code` and `/home/krynn/LuminariWebClient`.
- Production game: `/home/krynn/live/mud`, port **4300**.
- Development game: `/home/krynn/dev/mud`, port **4301**, database `krynn_dev`.
- Determine the production database from live's protected `lib/mysql_config`;
  verify it independently before any SQL import. Never infer it from a dev script.
- Preserve production player accounts, character identities, inventory, clans,
  claims, treasury, world, help customizations, credentials and launcher settings.
- Deploy reviewed code/catalog/help assets, not dev player files or dev SQL data.
  Do not replace live's `lib` or world with dev's tree. Apply any intended world
  edits as individually reviewed changes.

The migration needs a maintenance window and an initial **cold startup**. A
copyover cannot perform the initial change from legacy to SQL authority.

## 1. Prepare everything before the maintenance window

1. Inventory the actual live database schema, authority mode, clan/member/claim
   counts and any existing progression or pending economic operations. If live
   already has SQL authority or progression activity, stop using the initial
   import path and prepare an incremental upgrade for its actual state.
2. Freeze a release containing the latest MUD and web changes. Older phase-52
   packages predate the new quartermaster, gear fields and help fixes. Record
   exact source overlays, hashes, builds and configuration; a base Git commit
   alone does not identify the current uncommitted work.
3. Run release validation and produce a new package in new output directories:

   ```sh
   cd /home/krynn/code
   python3 scripts/validate_clan_release.py \
     --web-root /home/krynn/LuminariWebClient \
     --output /tmp/clan-live-validation --jobs 3
   python3 scripts/package_clan_release.py \
     --web-root /home/krynn/LuminariWebClient \
     --validation /tmp/clan-live-validation \
     --output /home/krynn/clan-live-release-candidate
   ```

   These output paths must be unused. Include the newer quartermaster service,
   gear codec/access/guard, enchanted combat, crafted delivery, and web help
   formatting checks. Inspect the validator's coverage rather than assuming an
   old passing report includes them all. Verify the package manifest includes
   the final binary, web assets, all migrations and current catalogs/help.
4. Prepare production-specific import, cutover and install wrappers. The existing
   `/home/krynn/dev/import-clans-to-sql.py`, `cutover-clans-to-sql.py` and
   `install-clan-progression.py` deliberately enforce dev paths, database, port,
   and sometimes **8 clans / 38 members**. Do not run them against production or
   simply remove their guards. New wrappers must check the actual live target,
   stopped writers, reviewed manifest, backup, source digest and expected SQL
   revision. Obtain counts from live, not the dev example. Default to check-only;
   require explicit apply for changes. Keep passwords out of output/history.
5. Prepare a private, credential-safe database client configuration and a complete
   ordered SQL set. Validate the intended database within each import session.
   In phpMyAdmin select the game database explicitly; the earlier unknown-table
   error came from querying `information_schema` as the selected database.
6. Compare required runtime files against live's `lib/etc`, `lib/misc`, and
   `lib/text` (including help). Install only required missing/updated release
   files. Preserve site-specific files. Check live world indexes and every
   expedition/contract/quartermaster target against the release catalogs.
7. Identify the actual production watchdog/service and gateway deployment before
   scheduling downtime. Live's `checkmud.sh` delegates to
   `/usr/local/sbin/server-checkmud --directory /home/krynn/live/mud --port 4300
   --launcher autorun.sh`. Record how automatic restarts are suspended and
   restored. Identify whether the gateway also serves dev before restarting it.

Deliverable: a verified release plus target-specific, rehearsed commands. The
production wrappers and precise stop/start commands are preparation work still
required; this plan does not pretend those helpers already exist.

## 2. Rehearse with a production snapshot

1. Take a consistent production snapshot using the existing backup procedure.
   Restore SQL and the matching file tree into a private, isolated rehearsal
   environment with separate database, files and port. Prevent access to live
   credentials and external writers. Never use the active dev progression store
   as the destination for a legacy import rehearsal.
2. Run the legacy importer in dry-run mode on the restored files. The reusable
   importer is `scripts/clan_legacy_import.py`; its `--help` documents supported
   options. It can validate source files without a database, and compare against
   the isolated schema when database arguments are provided.
3. Resolve index/player identity mismatches before import. Dev previously failed
   with `Player identity mismatch: zzzii`. Compare the index, actual active
   character file and ID references; back up before repair and keep legitimate
   active characters. Do not drop members or assign IDs merely to pass import.
4. Check leaders, applicants, officer ranks, clan names, legacy points, treasury,
   diplomacy and claims. Do not copy dev's leader removals into production.
   Preserve legitimate leaderless clans; record unresolved leadership decisions.
5. Perform the full schema/import/cutover/install/cold-boot sequence below on the
   clone. Test restore into a separate destination, not just archive listing.
6. Record actual duration, counts, discrepancies, commands and expected output.
   If production changes afterward, repeat final checks against the stopped
   production snapshot rather than reusing a rehearsal digest or revision.

## 3. Maintenance: stop writers and back up

1. Announce maintenance. Suspend automatic restarts and stop live cleanly through
   the identified service procedure. Stop other database/file writers, admin jobs
   and importers. Confirm port 4300 is closed and the MUD process is gone; an
   absent listener alone does not prove the process stopped.
2. At this quiesced checkpoint back up the entire game SQL database, including
   triggers/routines/events, and matching player/account/inventory files. Include
   the complete live `lib`, old binary, launch scripts and relevant web deployment
   configuration. Store secrets and dumps privately, outside release archives.
3. Record checksums, backup locations, schema/authority revisions, source counts,
   identity checks and pending receipts. Keep live stopped through cutover and
   installation. If any source changes, revalidate the snapshot before applying.

## 4. Apply schema, import live clans, then cut over

Apply only migrations required by the observed schema. A core version of 6 does
**not** prove optional service tables or retention triggers exist. Do not rerun
all historical migrations blindly.

Ordered schema checklist:

1. `sql/20261110_clan_progression_v001.sql` through `v005.sql`, sequentially.
2. `sql/20261113_clan_progression_v006.sql` — unused-economy pacing migration.
   If it refuses existing progression, investigate; never bypass its preflight.
3. `sql/20261111_clan_buff_activations_v001.sql`.
4. `sql/20261111_clan_buff_lifecycle_v001.sql`.
5. `sql/20261111_clan_kill_sources_v001.sql`.
6. `sql/20261111_clan_delivery_v001.sql`.
7. `sql/20261111_clan_objectives_v001.sql`.
8. `sql/20261112_clan_expeditions_v001.sql`, then `v002.sql`.
9. `sql/20261112_clan_quartermaster_v001.sql` — retain old receipt recovery.
10. `sql/20261113_clan_quartermaster_services_v001.sql` — required for the new
    storefront; check the `cqs_no_delete` and `cqs_immutable` retention triggers.

Then, while all writers remain stopped:

1. Confirm core version 6 and the expected legacy-mode destination state. An
   unexpected nonempty SQL clan store requires investigation, not truncation.
2. Dry-run the importer on **live's** clans, claims, player index and player
   directory. Review actual counts and every discrepancy.
3. Apply the import with a private backup directory and the importer's explicit
   writers-quiesced option, using the prepared production wrapper.
4. Re-run comparison; require no differences. Save source digest, incarnation
   mapping, import UID, SQL revision and verification report.
5. Perform the repository bridge's checked `cutover` using that exact digest and
   expected revision. Require SQL store mode **2**, completed matching import
   receipt, unchanged clan/member/claim contents, and expected revision advance.
   The generic bridge is built by `scripts/build_clan_repository_tool.py`; the
   production wrapper must supply and verify these facts as the dev wrapper did.
6. On uncertain acknowledgement, inspect persisted receipt/state before retrying.
   Do not use the `fresh` operation for existing clans.

Publish the current reviewed help migrations, including readouts, deliveries,
objectives, expeditions, services, quartermaster and weapon effects. Merge
intentionally with any live help edits. `sql/20261113_weapon_effects_help.sql`
is part of this release. Check aliases and displayed text after publication.

## 5. Install and cold-boot with progression OFF

1. Install the verified binary and explicit release catalogs under `lib/clan`,
   including `quartermaster_services.json` and the crafted-delivery catalog.
   Preserve live database settings and world. Check ownership and permissions.
2. Set `CLAN_STORE_MODE=mysql` in the actual launch environment. Inspect both
   `lib/.env` and the real launcher so an inherited/overriding setting cannot
   silently select legacy mode. Preserve port 4300 and other production settings.
3. Install a complete `lib/clan/progression.json` with mode `off`, all feature
   booleans false, `public_release: false`, and `pilot_clans: []`, version 1.
   The source default is OFF; the dev file is enabled and must not be copied.
4. Cold-start through the normal production launcher, initially with public
   access held for maintenance verification. Verify core 6, SQL hydration,
   completed import and SQL authority. Confirm every database connection uses
   the strict SQL session and `utf8mb4` setup included in the reviewed binary;
   dev previously failed startup on this configuration requirement.
5. Inspect actual memberships, leaders/ranks, treasury, claims and legacy values
   against the checkpoint. Ensure no stale-file fallback, schema/session errors,
   duplicate hydration or pending recovery errors. Stop and investigate a failed
   boot; do not switch authority back to legacy to make startup proceed.

## 6. Enable the reviewed features and verify

1. Install matching gateway `dist/server` and `dist/shared`, plus browser
   `dist/client`, preserving secrets and site settings. Use the identified
   deployment/service process. Coordinate any shared gateway interruption.
2. Set the reviewed live startup policy: mode `live`; `buffs`, `contracts`,
   `expeditions`, `quartermaster`, `web` true; `public_release` true; empty
   `pilot_clans`; version 1. Keep SQL authority `mysql`.
3. Restart using the compatible binary, or use its supported copyover only now
   that the initial cold authority migration has succeeded. Policy is read at
   boot; editing the JSON alone does not reload it.
4. Before reopening, verify with designated test characters:
   - `clan progress`, `clan favor`, roster and named clan selections agree with
     authoritative SQL and the web Clan panel. Reconnect clears private reports.
   - `help clanbuffs`, `help clanquartermaster`, `help weaponeffects`, and usage
     lists are readable; web usage commands retain separate lines.
   - Catalogs resolve to actual live rooms, mobs, recipes and harvest targets.
     Surveyed harvest nodes show in web room contents when applicable.
   - `clan quartermaster info` and `list` show motes **2,500 gold**, quest points
     **1,000 gold**, enchant **250,000 gold** (Favor 1+), reroll **500,000 gold**.
     Limits are per character; only unlocked effects are eligible; reroll is once
     per weapon per UTC week. Saved enhancement state/word survive reconnect.
   - Exercise an authorized buff, delivery, bounty/gathering progress, and a
     two-account expedition if test accounts are available. Verify exactly one
     charge/award and recoverable state through reconnect/restart. Do destructive
     crash injection on the rehearsal clone, not the production economy.
   - Check clan item purchase/equip restrictions and directional clan guards on
     designated test fixtures. Do not grant test Favor or alter real balances
     merely to perform this check; use rehearsal for unavailable rank scenarios.
5. Reopen public access and restore the normal watchdog once checks pass. Record
   installed hashes, backup, schema/authority, policy and smoke-test results.

## 7. Monitor and recover

Monitor boot/SQL errors, pending receipt age, recovery failures, duplicate awards,
ledger drift, weekly caps, missing catalog targets and web privacy/stale state.
Check through the first UTC daily reset and Monday weekly reset. Preserve the
approved deliberately long pacing; small currencies are not a 1:1 XP conversion.

First response to a gameplay defect: set the relevant feature OFF, or disable
progression with the supported policy and boot/reload procedure, **retaining SQL
authority and compatible recovery code**. Keep receipt/history rows and character
locks; accepted operations still need recovery. Validate Vitality HP recalculation
when disabling buffs. Keep enough administrative access to diagnose the issue.

Before cutover, while no authoritative writes have occurred, restore the coherent
checkpoint if needed. After cutover/boot writes, prefer a compatible forward fix.
Never revert to legacy files, reimport old clans, downgrade to an incompatible
binary, or restore SQL alone. A full restore requires stopping writers, preserving
current evidence, restoring matching SQL plus player/inventory files together,
and validating the restore separately. Explicitly record any lost player progress.

Completion means production is running the recorded release, verified against its
own data with matching web assets and an observed reset cycle—not merely that SQL
import succeeded.
