# Phase 19: spontaneous-caster Arcane Readiness

Arcane Readiness now covers the existing spontaneous slot-recovery event paths:
sorcerer, bard, inquisitor and summoner. Each actual head-slot cycle recalculates
its normal duration through `compute_spells_prep_time`, including existing class,
room, feat, perk, configuration and domain rules, then applies the checked cached
clan percentage. The result rounds upward and respects the slot-circle minimum.
No new slots, known spells or recovery permissions are granted. Existing readiness
checks, event cadence and completion/removal logic remain in place. PSP, movement,
HP and invocation/resource systems outside these recovery paths are unchanged.

Like prepared cycles, spontaneous cycles snapshot once, including when no buff
is active. An activation, replacement, expiry, storage outage or membership change
cannot repeatedly shorten a running cycle. Future head cycles check current
eligibility; enqueued tails reserve no benefit. Ordinary interruption resets the
cycle to normal timing; a genuine restart recomputes instead of scaling the old
remainder. Arcane Recovery's existing progress-preservation exception retains the
same cycle snapshot as well as its remaining time.

Innate queue saves append a seventh boolean field for the current-cycle marker.
Five/six-field player files remain readable. Both prepared and innate queue loaders
now append loaded rows in saved order rather than reversing them through normal
prepend insertion: this keeps a saved running cycle ahead of unstarted tails.
Markers attach only to successfully inserted entries. This also fixes the queue
ordering issue discovered while extending the Phase 18 persistence handling.
No database schema changes are needed; the Phase 18 checked runtime cache and
feature gates are reused.

Validation:

- `python3 tests/run_clan_preparation.py`: production timing/reset/codec functions
  compiled with ASan/UBSan; all four spontaneous classes and all three tiers,
  rounding, circle minimum, domain timing, Arcane Recovery, running-cycle activation
  and expiry, queued tails, restart, saved ordering/markers and old-format loading.
  Prepared-caster tests run alongside these as a regression check.
- `python3 tests/run_clan_buff_lifecycle.py`: disposable database lifecycle,
  eligibility, Arcane cache and Company Vitality regressions.
- Traditional Make and CMake server builds.

Defaults remain OFF. No live migration, feature enablement, deployment or server
restart. Next is Phase 20 Psionic Focus recovery.
