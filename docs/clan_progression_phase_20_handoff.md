# Phase 20: Psionic Focus

Phase 19 remains complete and its prepared/spontaneous recovery tests pass.
Phase 20 adds Psionic Focus only to the out-of-combat `regen_psp()` path.

The checked runtime family view now carries the active immutable activation UID.
Runtime hydration publishes a separate unsaved Psionic percentage, absolute expiry
and activation UID from accepted SQL membership under the existing campaign,
store, feature and pilot/public gates. New activations, membership changes,
expiry, storage failure and disablement discard fractional carry. No schema or
live configuration changes are needed.

For each ordinary eligible tick, calculate normal PSP recovery in the existing
order: base point, shared helper, recovery feat and resting-position recovery,
with their existing maximum checks. For a positive recovery R, award
`floor((R * percentage + remainder) / 100)` extra PSP and retain the modulo-100
fraction. Tier percentages are 10/20/30. The bonus never feeds back into R.
Use 64-bit intermediate arithmetic and clamp to maximum PSP. At/above maximum,
including a tick that reaches maximum, discard unused fractions. Ordinary
over-max drain is preserved and receives no bonus.

The remainder is unsaved per-character runtime state associated with the cached
activation UID. Combat entry clears it immediately in `set_fighting`, even when
combat ends before the next recovery tick. The tick also rejects combat and
clears carry defensively. Expiry is checked against absolute time in the tick,
not just heartbeat cache refresh. Logout may lose a fraction, but grants no extra
point. The shared `get_psp_regen_amount` helper and movement/passive recovery paths
are unchanged.

Validation:

- `tests/run_clan_psionic.py` compiles the production tick with ASan/UBSan and
  checks default-off parity across positions, feat ranks, helper amounts and cap
  boundaries; all tiers, small-value fractions, ordinary recovery components,
  combat, expiry, caps, over-max values, NPC exclusion, character isolation and
  bounded arithmetic.
- `tests/run_clan_buff_lifecycle.py` tests actual disposable SQL hydration,
  same-activation fraction retention, new activation reset, departure, disablement,
  outage and exact expiry, alongside prior lifecycle/Arcane/Vitality regressions.
- Prepared/spontaneous timing and queue-codec regressions and pure buff-query
  regressions pass; Make and CMake server builds pass.

No live migration, enablement, deployment or server restart. Defaults remain OFF.
Next: Phase 21 Scholar's Encouragement adventuring XP.
