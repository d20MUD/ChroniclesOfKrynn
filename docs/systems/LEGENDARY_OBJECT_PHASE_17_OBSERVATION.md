# Phase 17 — observation and closure

Status: observation tooling implemented; operational observation and project
closure remain contingent on a separately authorized, healthy production release.
This phase never enables a feature, changes a balance, edits a world file or
mutates legendary data.

## Daily aggregate report

Use a MySQL account with `SELECT` privileges only:

```sh
LEGENDARY_PHASE17_READONLY=YES \
LEGENDARY_PHASE17_DB=krynn \
LEGENDARY_PHASE17_OUTPUT=/var/log/krynn/legendary-observation \
scripts/legendary_phase17_report.sh --defaults-file /etc/krynn/legendary-report.cnf
```

The report is defined by
[`20261026_legendary_phase17_observability.sql`](../../sql/20261026_legendary_phase17_observability.sql).
It intentionally contains aggregates only: no names, player IDs, UUIDs, targets
or recovery payloads. Run it once per UTC day for at least seven days, retaining
each timestamped result with the release record.

## What the report measures

- Seven UTC days of awards, received XP/materials and source mix.
- Current-day cap saturation and current holdings by essence type.
- Purchase/recovery state, costs and unresolved-operation age.
- Active channel/rank/path/family composition.
- Two-handed investment and weapon-plus-shield ownership proxies.
- Major-power activations over the table's retained 24-hour window, and actual
  rolling-budget benefit totals by offense/prevention/healing/resource category.

The last two are deliberately labelled as **usage** and **benefit-category
proxies**. The durable budget ledger is intentionally short-lived and does not
associate every delayed benefit with a power ID; inventing a long-lived,
per-player combat trace would add hot-path persistence and undermine the Phase
13 performance boundary. Use the completed Phase-14B native paired probes for
the 5% outgoing / 8% incoming performance acceptance evidence.

## Daily review checklist

1. Save the report and review feature-gate/schema health first. Stop rollout if
   the obsolete boss-registry table appears or a gate differs from the approved
   stage.
2. Review pending awards, prepared/quarantined operations and their oldest ages.
   Reconcile durable records; never delete progression to clear a dashboard.
3. Compare source rates, cap saturation and material holdings for shortages.
   Use the three-to-one exchange only as the existing relief valve; publish any
   balance change with a definition version and player-facing note.
4. Compare rank/path, passive and two-handed investment distributions. Investigate
   a material divergence before changing costs or bonuses.
5. Review power usage and rolling benefit categories together with gameplay logs
   and player reports. Record confusion cases—particularly cooldown, target and
   reshaping questions—in the release issue tracker.
6. Record save/recovery, UUID quarantine and ownership-rejection incidents from
   server logs alongside the aggregate report. These are not database metrics,
   so do not treat a zero SQL row count as proof that no incident occurred.

## Closure gate

Do not mark the system complete merely because seven reports exist. Closure
requires all of the following:

- Phase 14B’s seven representative 30/600-second combat cases pass their agreed
  5% outgoing and 8% incoming limits.
- The Phase-16 coherent backup/restore rehearsal has passed on sanitized staging.
- Seven consecutive UTC daily reports show no unexplained pending awards or
  unreconciled operations, and all recorded incidents are resolved or explicitly
  accepted.
- Sufficient players have reached ranks III and V to review final-rank choices,
  two-handed parity and major-power use.
- Any balance adjustment is versioned, published and preserves existing player
  progression.

Until those conditions hold, Phase 17 is active observation rather than closure.
