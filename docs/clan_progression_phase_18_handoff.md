# Phase 18: prepared-caster Arcane Readiness

Arcane Readiness now reduces prepared-caster head-cycle durations by the checked
runtime tier (10%, 15%, or 20%). The duration rounds upward and cannot fall below
the effective spell circle. Normal preparation prerequisites, metamagic/domain
handling, feats, room/configuration/perk reductions and event interruptions remain
in force. Spontaneous slot recovery remains Phase 19.

The existing authoritative runtime reconciler hydrates a separate unsaved Arcane
percentage and absolute expiry from accepted SQL membership and checked active
family state. Defaults, campaign, pilot/public and storage gates still apply.
Departure, disablement or unavailable storage clears eligibility for future cycles.
No new schema migration or activation command was added.

A cycle snapshots when a prepared spell actually becomes the queue head and
starts preparing. Its normal time is recalculated then, before applying the clan
multiplier. Every started cycle is marked, even without a buff: activation cannot
shorten an already-running cycle. Expiry and membership changes do not rescale
one that already started. Unstarted queued spells reserve no discount.

The prepared-queue file codec appends a seventh boolean field identifying a
started cycle; the saved remaining duration is authoritative for that cycle.
Legacy five/six-field rows load without a snapshot. New saves restore each
entry's own marker through the existing player-file/checkpoint path, including
the existing queue loader (Phase 19 subsequently corrected saved-row ordering). No separate persistence store is introduced.

Ordinary interruptions clear the snapshot and reset the normal duration; the
next genuine start computes a fresh cycle rather than multiplying old progress.
Arcane Recovery's existing exception preserves interrupted progress, so it also
preserves that same cycle's snapshot, preventing repeated clan discounts.

Validation: `python3 tests/run_clan_preparation.py` compiles production timing,
reset and queue-codec functions under ASan/UBSan. It covers tier rounding,
minimums, domains, saved progress, activation/expiry during a cycle, ordinary
restart, Arcane Recovery and old-format loading. The disposable-MariaDB lifecycle
suite additionally checks SQL Arcane hydration, default-off gates, departure,
outage/recovery and exact expiration, alongside prior lifecycle/HP regressions.

Next: Phase 19 spontaneous-caster recovery. No live database changes, feature
enablement, deployment or server restart were performed.
