# Clan progression: Phases 37–38 handoff

Implemented behind the existing disabled defaults. No live migration, enablement,
publication or restart occurred. No additional schema migration is required:
these changes use core v5 and the existing expedition v1/v2 extensions.

## Persistence and reconciliation

SQL remains authoritative for outing identity, frozen definitions, roster,
deadlines, counters, encounter events, completions, entitlements and receipts.
Startup and heartbeat reconcile up to sixteen active outings every thirty seconds,
using a cursor and the active-slot index. Reconnect checks the original character
and account and displays saved progress. Offline presence does not invalidate a
roster. Membership departures are invalidated without allowing later reactivation.

Expiry commits independently of a subsequent rejected command. Expired and
abandoned outings retain history and release their active slot; neither can earn
new completion claims. Restart never extends a deadline or synthesizes combat.
Maintenance also runs with gameplay features disabled when authoritative SQL and
the required schemas are ready. It does not issue awards.

Reconciliation audits frozen terms, roster identities, immutable encounter facts,
quotas, participant credit, completion proofs, account/family/week reservations
and reward receipts. Invalid or partial proofs stop that outing for investigation;
they do not reset progress or mint replacement rewards. A corrupt outing does not
prevent the bounded batch from checking other clans. Prepared personal receipts
are revalidated inside payout and retain their original member, account and week,
including after departure or reset. Unaccepted claims still require eligibility
and their original expiry. Restore the authoritative SQL history when repairing
storage; do not delete receipts or reset counters to clear a conflict.

## Staging roster

Both families unlock at clan level 1; their original quotas, participation rules,
rewards and weekly limits remain unchanged.

| Character band | Stronghold Assault | Regional Hunt |
| --- | --- | --- |
| 5–10 | Smugglers | Sewers |
| 11–16 | Eld | Eld and adjacent countryside |
| 17–22 | Valley | Bugbear territories |
| 23–26 | Icewall approaches | Icewall approaches |
| 27–30 | Icewall | Icewall |

The ten current definitions are in `lib/clan/expeditions.json`. Every objective
has at least two alternatives. The old two v1 Icewall definitions are archived
unchanged in `lib/clan/expeditions_legacy.json`, excluded from new enrollment;
existing outings continue with their SQL-frozen terms. Definition IDs are not
reused. Characters below level 5 retain ordinary contract options.

`scripts/validate_clan_expedition_roster.py --json` produces the detailed target,
level, guaranteed spawn population, reset-wait and walking-route audit against
the indexed checkout world. It checks open areas, ordinary killable uncharmed
NPCs, safe spawn rooms, unconditional guaranteed recurring resets, suitable
levels, reachable alternatives, and bidirectional unlocked routes. Zones reset
within fifteen minutes; projected respawn wait is at most forty-five minutes per
objective and selected route lengths total at most 110 hops per definition.
These are conservative content checks, not measured party completion times.
Actual two-to-six-account combat timing and balance need staging playtests;
world indexes and content must be revalidated against the intended deployment.

Structural catalog checks are independent of whether every target is currently
alive. Create/start require a live eligible target alternative for each selected
objective. Missing targets in unrelated definitions do not disable the catalog.
Active displays target/area names, alternatives, availability and saved counters.

## Validation

- `python3 tests/run_clan_expedition_recovery.py`: private MariaDB fresh-process
  resume, cursor batches, exact expiry, crashes before/after expiry commit,
  feature-off upkeep, abandoned history, corrupt/partial-proof refusal,
  membership invalidation and retained pending receipt payout.
- `python3 tests/run_clan_expedition_staging.py`: all ten definitions against
  indexed world/reset/routes and production live-world adapter under ASan/UBSan,
  including missing primary, live fallback, dead/controlled targets and closed or
  peaceful areas.
- Existing expedition definition, SQL enrollment/command and combat/claim suites;
  CMake and Make server builds.

Help source and optional SQL publication are updated. Applying the optional help
SQL is an operator action and does not enable expedition services.
