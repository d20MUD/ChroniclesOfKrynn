# Clan progression Phases 50–52

Phase 50 is complete: migration/recovery rehearsal and the full relevant
validation suite pass. Phase 51 has a concrete observation/review plan but remains pending actual
controlled player observations. Phase 52 prepares a local review candidate and
operator checklist; production approval, deployment and post-deployment checks
remain pending. None of these statuses claims a completed public release.

## Phase 50 rehearsal

Final results: 39 clan suites and 130 browser/gateway tests pass; both CMake and
Make server builds and the TypeScript/Vite build pass. Lint has zero errors and
one pre-existing `App.tsx` hook-dependency warning. The final validation report
and individual logs are included in the review candidate. Development snapshot
migration and package privacy/checksum tests also pass.

The test runner `scripts/validate_clan_release.py` executes every
`tests/run_clan_*.py` suite, the server build, all browser/gateway tests, the
TypeScript/Vite production build and ESLint. It retains one log per command and
an aggregate `validation.json`, including exit codes and durations. Failed
validation prevents candidate generation. The first full run exposed two stale
test expectations, corrected before the final run:

- A generic receipt test tried to complete a raw bounty source without its
  required objective participant. That service guard is intentional. The
  cross-source ledger example now uses a supply source, while specialized
  contract/expedition tests retain their proof and bypass-rejection coverage.
- The coherent purchase-restore test compared delivery messages as well as
  economic state. Recovery correctly announces delivery once and is silent on a
  paid retry. The assertion now compares success, gold, item count and lock state;
  both restores retain exactly one item, one charge and an acknowledged journal.

`tests/run_clan_release_rehearsal.py` reuses integrated production services, then
backs up/restores all 38 clan tables after post-cutover writes. Every table's
rows, SQL authority, balances, Favor, windows, buff timers, receipts, expedition
proofs/claims, purchase journals and retention trigger names agree after restore.

The quartermaster suite additionally backs up SQL and independent player/
inventory files at one interrupted checkpoint, restores them into a separate
private database/file tree, and proves repeated recovery delivers one item and
charges once. The original instance's later acknowledgement is not required by
the restored checkpoint.

Legacy-format import, private source backup manifests, cutover compare-and-swap,
stale-file rejection, post-cutover SQL recovery, unavailable-database boot and
feature/pilot gates are covered by the complete existing suites. HP removal and
retained progression/history on disable are checked by the lifecycle scenarios.

## Development instance evidence

The user authorized development-port testing. The identified instance listens on
4301 and uses `/home/krynn/dev/mud/lib`; the production instance is separate.
A read-only inspection found core clan schema v2 with eight tables and default
legacy authority configuration. A consistent dev clan snapshot was restored to
a separate temporary, non-networked MariaDB instance and upgraded through core
v5 plus all optional schemas. All original data remained equal; the clone had
38 clan tables afterward. No source DDL/DML or feature enablement was performed.

Reproduce that opt-in check with:

```sh
python3 scripts/rehearse_clan_dev_snapshot.py --dev-lib /home/krynn/dev/mud/lib
```

The script reads credentials internally and emits only aggregate results, never
credentials or raw records. Raw dumps live only in temporary memory/private
fixtures and are excluded from the review package. Telnet checks also reached
the dev account prompt and negotiated MSDP; the currently running older binary
has no `CLAN_PROGRESS` registration. This is not a claim that the candidate has
been installed or that an authenticated pilot has run.

## Phase 51 and release boundary

`clan_progression_pilot_review.md` specifies small/active-clan cohorts, UTC
coverage, exact balance/receipt reconciliation, Influence income/upkeep, Favor
pace, buff choices and reward-loop checks. No observed player measurements were
found. Fixtures and modeled rates do not replace observation. The user requested a long-progression bias; the pilot review records that
preference and flags the current modeled active-clan pace for review, with a
slower threshold-only comparison clearly labeled as unobserved and unapplied.
Balance tables, rules version and disabled defaults remain unchanged.

The current source has a trusted configuration API but no operational loader
that installs a pilot policy. A reviewed bootstrap integration, approved pilot
allowlist and actual observations are prerequisites, not imaginary environment
variables. No phase is marked as a completed pilot or deployed release.

## Phase 52 review candidate

`scripts/package_clan_release.py` collects public source overlays and tracked
patches against both base commits, server and gateway/browser artifacts, SQL,
catalogs, help, documentation and passing validation logs. The manifest hashes
each file and identifies the exact dirty source snapshot; base commit IDs alone
would omit the uncommitted progression work. Credentials, player files,
operational backups and node_modules are excluded. The package is explicitly
`review_candidate_not_approved`, with pilot review pending and defaults disabled.
Package tests check the prerequisite, hashes, untracked source inclusion and
private-file/symlink exclusion.

`clan_progression_release_checklist.md` specifies quiesced backups, schema order,
checked importer/cutover, initial cold restart versus copyover, matching web
artifacts, trusted feature controls, monitoring and rollback after SQL writes.
There is no project-specific clan approval automation; the checklist requires a
concrete operator decision against the manifest, target, window, backup proof,
pilot evidence and restore owner before production changes. The generic setup
script is not used as an upgrade or approval shortcut.

Reproduce local preparation (choose new output directories):

```sh
python3 scripts/validate_clan_release.py --web-root /home/krynn/LuminariWebClient --output /tmp/clan-validation --jobs 3
python3 scripts/package_clan_release.py --web-root /home/krynn/LuminariWebClient --validation /tmp/clan-validation --output /home/krynn/clan-release-candidate
```

After any source or configuration change, rerun affected checks and refresh the
candidate/hashes. Production approval and the post-deployment smoke record are
required before Phase 52 can be marked released.

Local review artifacts are written to
`/home/krynn/clan-progression-release-candidate-20261112` and its adjacent archive.
Open `manifest.json` for current hashes and `validation/validation.json` for
command evidence. These artifacts are review candidates, not signed releases.
