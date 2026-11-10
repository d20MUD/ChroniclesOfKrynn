# Clan progression: Phase 9 handoff

Status: Clan XP/level/Influence policy implemented without gameplay hooks. The
work is split into pure policy, receipt integration, and cache reconciliation.
Phase 10 is next: personal Favor, account bands/source budgets, and explanations.
All default progression controls remain OFF; no migration or live award occurred.

## Policy and receipt service

`src/clan_awards.{c,h}` provides checked pure level and award projections.
Levels are derived from the validated cumulative threshold table; level 20 is
38,000 whole XP. Multi-level awards need no separate level writes. Negative,
over-cap stored balances, invalid configurations and unrepresentable incoming
counter additions fail without modifying the caller's result.

Clan-wide policy uses Phase 2 version 1: the first 6,000 account-adjusted incoming
XP per occurrence week credits fully, the next 6,000 at one quarter, and further
incoming XP credits zero. Lifetime XP is capped at the final level threshold.
Influence credits independently, limited to 5,000 per occurrence week and 20,000
stored. It can accrue at level 20 or after the weekly XP allowance is exhausted;
full Influence storage does not prevent XP. The pure projection accepts
already account-adjusted incoming amounts; it does not grant Favor or apply
account eligibility/budgets.

Quarter-band credit uses the difference of cumulative integer quotients, so
splitting an event cannot repeatedly discard sub-eighth remainders. Eighth-point
balances and subtraction-before-addition preserve integer safety. Requested
amounts truncated by clan rules receive versioned receipt flag bits:

- 1: weekly XP reduction/exhaustion.
- 2: lifetime level cap truncation.
- 4: weekly Influence capacity truncation.
- 8: stored Influence capacity truncation.

`clan_award_complete_shared()` composes this policy through Phase 8's locked
receipt coordinator. Pending decisions require enabled earnings, supported
campaign/schema/pilot readiness, immutable rules version 1, a shared source,
and that source's exact configured reward tuple. Version-1 balance policy comes
from immutable defaults; runtime controls govern release, not reinterpretation
of accepted rewards. Personal sources fail closed until Phase 10 supplies
account/Favor policy. This is a trusted server service, not a command or public
mint endpoint. Accepted source eligibility, shared definition/period quotas and
stable event identity remain the responsibility of later source services.

The coordinator commits credited balances, original-week incoming usage and
receipt atomically. Zero/capped outcomes remain deduplicated. Replays return the
original committed receipt even after earnings is disabled, without rerunning
policy. COMMIT_UNKNOWN must be reconciled by the original operation/event IDs,
never by compensating or preparing a new claim. No player-file mutation occurs.

## Runtime cache reconciliation

The persistence adapter's `clan_persistence_refresh_progression()` reads a checked
SQL snapshot and accepts only forward-revision progression changes. It preserves
runtime dirty legacy metadata; unrelated SQL metadata/membership/store-revision
changes fail closed rather than being silently merged. Database outages and
backward revisions disable readiness; the authority remains MySQL.

Future runtime award callers must call this refresh after OK or COMMIT_UNKNOWN,
and before serving cached progression. Explicit refresh can recover a transient
outage when the committed metadata still matches. Ordinary clan saves and
membership writes also refresh before constructing their proposed snapshot.
Their existing revision predicates still reject an award racing after refresh.
Tests verify that actual receipt awards survive explicit refresh, automatic
refresh and later metadata saves. This is not a full boot reload or lifecycle
conflict resolver; Phase 11 handles lifecycle policy.

## Verification and next entry point

- `tests/run_clan_awards.py`: strict C99/ASan/UBSan production-module checks of all
  thresholds, multi-level transitions, weekly band crossings/exhaustion, independent
  caps, fractional splitting, zero rewards, malformed state/config and int64 bounds.
- `tests/run_clan_rewards.py`: existing transaction/crash/concurrency suite plus
  actual policy commits, disabled/unready rejection, rollback, exact tuple checks,
  personal-source refusal, capped receipts and replay after disabling earnings.
- `tests/run_clan_persistence.py`: actual receipt-to-cache-to-save retention checks
  alongside existing legacy/MySQL/membership/outage behavior.
- Configuration/campaign gate regressions and both server builds.

Phase 10 should compose account source limits and account bands before calling
`clan_award_project()`, then apply the Favor cap independently. Preserve accepted
rules and occurrence weeks, source eligibility, exact eighth-point arithmetic,
terminal receipts and fail-closed gates. Keep manual resource-consuming turn-ins
disconnected until their file/SQL escrow and acknowledgement phases are complete.
No schema change is needed for Phase 9; required schema remains version 5.
