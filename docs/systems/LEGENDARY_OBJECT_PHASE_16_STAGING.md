# Phase 16 — guarded staging rehearsal

Status: implemented as a staging-only rehearsal harness. It is not a production
deployment and does not grant authority to edit live world files, apply live SQL
or enable any feature gate.

The harness is [`scripts/legendary_phase16_rehearsal.sh`](../../scripts/legendary_phase16_rehearsal.sh).
It intentionally refuses to run unless both explicit staging/sanitization
acknowledgements are present and the database name ends in `_stage` or
`_staging`.

## Preconditions

- A sanitized database copy, named for example `krynn_stage`.
- A compatible binary and this source tree on the staging host.
- An empty, absolute snapshot destination with enough room for a SQL dump and
  optional player/object save-root archives.
- A MySQL defaults file limited to the staging database credentials.
- Reviewed MEDIT changes for any `Legendary-Boss` candidates. Do not make those
  edits as part of a production rollout without separate content approval.

## Run

```sh
LEGENDARY_PHASE16_STAGING=YES \
LEGENDARY_PHASE16_SANITIZED=YES \
LEGENDARY_PHASE16_DB=krynn_stage \
LEGENDARY_PHASE16_SNAPSHOT_DIR=/var/backups/krynn/legendary-phase16-stage-20261026 \
LEGENDARY_PHASE16_PLAYER_ROOT=/srv/krynn/lib/plrfiles \
LEGENDARY_PHASE16_OBJECT_ROOT=/srv/krynn/lib/objs \
scripts/legendary_phase16_rehearsal.sh --defaults-file /etc/krynn/staging-mysql.cnf
```

The two save-root variables are optional, but production-equivalent rehearsal
should provide both. The script captures them and a consistent SQL dump before
it applies schema v1–v10. It then proves that schema version is `10`, the obsolete
`legendary_boss_registry` table is absent, and all three feature gates remain
disabled. It runs the read-only diagnostics and the automated release,
security, flow and build checks.

## Manual staging checks

While all gates are disabled, boot the staging MUD and verify a legacy character,
ordinary saved object, player shop, housing object and auction object load
unchanged. Confirm `infuse inspect` is read-only and that Lord Orelor's service
is correctly available.

Then enable one gate at a time on staging only:

1. `earnings`: validate one XP source, one flagged-boss reward, a daily cap and
   a reconnect/retry.
2. `purchases`: awaken an item, advance both channels of a two-handed item,
   replace a passive, exchange materials, and restart during one operation.
3. `major_effects`: validate a technique, a signature, cooldown persistence,
   rolling budget limits and source removal.

Return every feature row to `enabled=0`, restore the coherent snapshot, and run
the rehearsal a second time. Record the diagnostics output and test logs with
the release ticket.

## Production boundary

Phase 16 cannot close until the incomplete Phase-14B seven-role combat gate has
passed and a separately authorized production maintenance window exists. The
staging harness deliberately has no production bypass: a non-staging database
name is rejected before any backup or SQL command runs.
