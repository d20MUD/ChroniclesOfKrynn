# Phase 21: Scholar's Encouragement

Scholar's Encouragement now adjusts positive, source-qualified character
adventuring XP by the checked active tier: 5%, 8% or 10%. The existing runtime
reconciler hydrates a separate unsaved percentage and absolute expiry from SQL
membership/family state. Campaign, feature, store and pilot/public gates remain
unchanged. Departure, disablement, storage failure and expiration clear the cache;
the award path also checks absolute expiry itself.

The authoritative `gain_exp_with_source` path applies Scholar once after the
existing leadership/newbie/feat, happy-hour and account modifiers and before
`CONFIG_MAX_EXP_GAIN`. Existing mode-specific caps retain their position before
happy-hour/account adjustments. The clan addition uses 64-bit arithmetic, floors
fractional XP and saturates at INT_MAX before the final normal cap. Normal
character-level caps/rejections, losses and NPC recipient handling are unchanged.

Kill awards use the existing server-generated eligible NPC kill source for both
solo and group recipients. Actual damage awards use `gain_exp_from_damage`, which
builds explicit damage provenance using the same real-NPC filters: player-owned,
charmed, familiar/companion/mount/eidolon/golem/solar, test and owned-instance
victims are excluded. Damage provenance generates no encounter UUID or legendary
kill receipt. It is rejected by legendary earnings, even if passed directly.
Physical and spell damage already pass through the shared damage award point;
separate XP merely for casting a spell stays context-free and receives no bonus.

Context-free `gain_exp`, conventional quests, scripts/administrative grants,
crafting, traps, energy drain, dump rewards and XP losses are excluded. PvP does
not qualify. Each legitimate damage/kill award gets one character adjustment;
there is no additional aggregate award or bonus on later legendary processing.
Pet-owner damage routing and ordinary reward distribution remain as before.

No Clan XP, Influence or Favor calculation calls this multiplier, and source facts
are not rewritten. Character XP cap rejection does not decide contribution
eligibility. The actual encounter/participation eligibility query and durable
combat-contribution gameplay adapters remain planned Phases 25–26; those must
consume their independent original source facts rather than the return value of
character XP awards.

Validation:

- `tests/run_clan_scholar.py`: production XP award and damage adapter under
  ASan/UBSan; all tiers, solo/group/damage, absent/ineligible/mismatched sources,
  all context-free modes, PvP, existing modifier order, newbie bonus, mode and
  final caps, exact expiry, disabled cache, NPC recipients, capped and max-level
  characters, losses, integer limits and damage/kill receipt separation.
- Disposable SQL lifecycle/runtime suite: Scholar default gates, membership,
  storage outages, disablement and expiry, plus all prior buff regressions.
- Group XP distribution regressions in both campaign configurations.
- Legendary earnings/source/replay/persistence regressions, including damage
  provenance creating neither encounter IDs nor legendary XP credit.
- Traditional Make and CMake builds.

An initial lifecycle crash-injection run returned before its expected injected
exit during concurrent build/database tests. Its isolated standard rerun passed
all crash/retry and Scholar checks. Crash assertions now retain return code and
stdout as well as stderr to diagnose recurrence.

No live migration, enablement, deployment or server restart. Defaults remain OFF.
Next: Phase 22 Artisan's Guidance crafting checks.
