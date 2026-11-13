# Phase 51: controlled pilot review

Status: the user has waived a staffed pilot because all clans are small and test
participants are unavailable. Mathematical modeling, conservative starting
pacing and automated/dev checks replace the pilot requirement. See
[the computed pacing proposal](clan_progression_pacing.md). Historical pilot
planning below is retained for optional later observations, not a release gate.
The approved earning/rank tables are implemented in rules 2/core v6; defaults remain disabled.

The user has explicitly requested very long initial progression, preferring to
shorten it later rather than release it too quickly. Treat slow initial pacing
as a release requirement, not merely a tie-breaker between otherwise acceptable
options. Favor slower leveling and later unlocks when evidence is uncertain.
Favor/standing access must also be reviewed for longevity. Do not accelerate progression merely because a
small sample has not reached later unlocks.

The historical rules-1 level-20 threshold was 38000 Clan XP. The Phase 2 modeled rates of
2750 XP/week (small clan) and 7150 XP/week (active clan) imply roughly 14 and
6 weeks respectively if those rates were sustained. These are model estimates,
not observed play, and the active-clan estimate is too fast for the
user's stated preference and must not receive balance sign-off as it stands.
A review-only conservative candidate would multiply level thresholds by ten: level 20 at 380000 XP, roughly 139 and
54 modeled weeks. Rewards/caps would stay fixed in that comparison. No such
change is applied in this candidate: resolve and apply a deliberately slow
starting target, using available pilot evidence and checking frozen rules/version
compatibility before modifying tables. The release review must explicitly resolve this pacing concern rather than treating the
current numbers as balance sign-off.

A pilot means a limited trial with named clans before public release. Compare
at least one small clan with one active clan, using distinct account counts
rather than character counts. Observe a complete UTC contribution week and
buff activation/downtime cycles; record shorter coverage honestly rather than
extrapolating it as measured weekly behavior. Observation time is outside the
implementation work block.

Before starting, record the approved server/client artifact hashes, schema
versions, rules/configuration hash, clan-incarnation allowlist, start/end times,
feature controls, and initial balances. Public release remains false. The
current source supplies a trusted configuration API but no operational loader
for progression feature controls; a reviewed server-owned bootstrap integration
is a prerequisite to running the pilot. `CLAN_STORE_MODE` alone cannot enable it.

Use anonymized clan/account identifiers in exported observations. Keep operational
backups and raw player records outside this review package. Export receipts and
saved state read-only; do not infer earnings solely from screenshots or client
reports. Compare opening/closing balances with committed receipt deltas,
Influence spending with activation receipts, and personal claims with account/
week/family keys. Separate issued, pending, canceled and completed receipts.

| Observation | Small clan | Active clan | Evidence to attach |
| --- | --- | --- | --- |
| UTC coverage and distinct active accounts | pending | pending | start/end and account counts |
| Opening/closing Clan XP, levels and time to unlocks | pending | pending | snapshots and receipt totals |
| Influence earned, spent, net balance and cap reductions | pending | pending | awards, activations, cap flags |
| Favor per character/account and time to standings | pending | pending | retained standing and account windows |
| Buff family/tier/slot choices, uptime and skipped purchases | pending | pending | activation/cancellation receipts and player feedback |
| Source mix, repeated definitions and expedition completion time | pending | pending | source claims, outing proofs, real party timings |
| Alt activity, daily/weekly cap hits and reset boundaries | pending | pending | account/prototype windows and immutable events |
| Repeated-event, reconnect and interrupted-claim behavior | pending | pending | unique IDs, pending/recovery outcomes |
| Economic shortcuts or anomalous reward rates | pending | pending | reconciled totals and reproducible occurrences |

Compare measured results with `clan_progression_phase_2_spec.md`. Evaluate
leveling pace, Influence income versus desired buff cadence, Favor access and
choice diversity separately for both cohorts. A higher activity clan earning
more is expected; extra characters repeating account-limited claims is not.
Track reasons players declined a buff as well as purchases. Do not interpret
an unpurchased family as weak without checking its unlock, permissions and
cooldown availability.

Correctness failures (duplicate awards, receipt loss, HP exploits, stale storage
fallback) stop the trial and become code fixes with regression coverage; they
are not balance adjustments. For balance findings, propose only explicit table/
default changes with before/after values and supporting measurements. Frozen
version-1 receipts and policy validators must be respected: changing an existing
rule table may require a new rules version and migration, outside simple tuning.
Rerun configuration, award, receipt/recovery and affected gameplay suites after
any approved change. Record the decision even if the evidence supports no change.

Completion requires actual evidence for both cohorts, a documented review of
leveling/Influence/Favor/buff choices, and a signed decision on reward-loop
findings and any table changes. A staffed observation period is optional after the user's waiver; the replacement mathematical balance decision and implementation
still require validation. The deployment package remains a review candidate.
