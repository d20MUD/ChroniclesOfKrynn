# Phase 8: foundation gameplay integration

Implemented 2026-10-24. All sixteen foundations now contribute to gameplay at
their fixed catalog values. The Phase 7 owner/equipment checks, saved priority
order, twelve-point budget, duplicate ceilings and combat allocation lock still
determine eligibility. Technique/signature activation remains Phases 9–12.

## Calculation map

| ID | Integration and limits |
| --- | --- |
| F01 Accuracy | Adds +1 to ordinary wielded-weapon attack calculations, including ranged, before the existing total attack cap. No unarmed, natural or eldritch-blast bonus. Attack breakdown identifies it. |
| F02 Force | Adds the globally planned +1 per permitted copy, maximum +2 across the outfit, once per ordinary weapon hit. Added after critical, position and other damage multipliers; no spell/proc bonus. Existing ordinary damage-bonus cap also limits the addition. Damage previews include the same capped contribution. |
| F03 Warding | Competes as +1 deflection AC inside the existing typed AC calculation. A stronger positive deflection bonus supersedes it; deflection penalties remain. Touch AC includes it and the ordinary total AC cap remains authoritative. |
| F04 Fortitude | Adds +1 to the Fortitude bonus used by normal saves, subject to the ordinary save-bonus cap. |
| F05 Reflex | Adds +1 to the Reflex bonus under the same rules. |
| F06 Will | Adds +1 to the Will bonus under the same rules. |
| F07 Aptitude | Adds +1 to the selected effective attribute, then enforces existing caps. Real/base attributes remain unchanged. Normal derived modifiers, including CON-dependent HP, use the resulting attribute. At most two distinct selected abilities. |
| F08 Expertise | Adds +2 in `compute_ability_full`, which feeds actual skill checks. Requires positive trained ranks in a supported roll skill; does not alter ranks, class prerequisites or crafting progression. At most three distinct skills. |
| F09 Vitality | Adds +20 maximum HP per allowed copy, maximum +40, after ordinary HP calculations and percentage bonuses. Does not enter saved real/base maximum HP and never fills current HP. |
| F10 Endurance | Adds +40 maximum movement per allowed copy, maximum +80, under the ordinary maximum-movement cap. Never fills current movement. |
| F11 Reservoir | Adds +20 maximum points in the existing spell/psionic PSP pool under its ordinary cap. Requires an actual spellcasting/psionic resource class, including artificer. No spell slots, class access or current-point grant. |
| F12 Elemental weave | Adds up to three percentage points for the selected acid/cold/electricity/fire/sonic resistance, never raising that resistance above 75. Existing resistance above 75 is preserved. At most two distinct elements. |
| F13 Sure footing | Adds +2 on actual knockdown/trip defense and opposed forced-movement checks. Does not modify the attacker's roll or grant immunity where no check exists. |
| F14 Steady grip | Adds +2 to actual disarm CMD defense, without affecting unrelated maneuvers. |
| F15 Trailwise | Applies the final 10% ordinary walking-cost reduction after existing terrain/skill/speed reductions, minimum one for positive costs. Both movement implementations use it. Mounted/ridden movement, teleports and already-free movement receive no refund. |
| F16 Recovery | Adds exactly five HP to the existing normal regeneration tick outside combat, after regeneration multipliers. Normal anti-healing/no-heal restrictions still suppress regeneration. No new tick or faster cadence. |

No foundation scales with rank. Values continue to come from the Phase 2 catalog
and the Phase 7 planned passive result; the mechanics do not independently sum
raw object properties or multiply a two-handed weapon's contributions.

F08 support is centralized for menus, selection validation, cache eligibility and
mechanics. Religion, Concentration, Discipline and Total Defense have real rolls
in the current code and are supported despite stale UNUSED/UNIQUE comments.
Linguistics is a language counter and remains excluded, as do unused IDs and
crafting/harvesting progression abilities. Aliases share the same underlying ID.
This corrects the earlier front end's overly narrow supported-skill list.

## Stat rebuilds, caps and persistence

`legendary_foundations.c` reads the shared cached loadout and supplies runtime
contributions. Managed attributes, saves, movement, PSP and resistances are added
through the normal `affect_total` rebuild after ordinary contributions have been
capped; the existing cap function then caps the augmented values as well.
Tracked actual deltas support accurate cap/supersession reporting. For allowed
duplicates that collectively reach a cap, the displayed actual contribution is
assigned in saved priority order rather than reported twice on both channels.

The maximum-HP calculation adds Vitality at its end and preserves its ordinary
result in `GET_REAL_MAX_HIT`. A maximum-HP rebuild now also works for linkdead
players instead of requiring a descriptor. Effective attribute increases still
feed the ordinary HP formula. No foundation is inserted as an object APPLY or
serialized as a base-stat improvement. Runtime state is cleared at character
reset/free and on NPC copying, alongside the existing loadout lifecycle.

Raising a maximum leaves its current pool unchanged. Removal clamps a current
pool only when it exceeds the new maximum. Re-equipping cannot restore points
lost to that clamp. Regeneration/healing remain the ways to fill the larger pool.
Repeated rebuilds, reconnects and owner-storage restoration cannot compound
bonuses or recover their investment as points.

F12's allocation and cache fingerprint subtract the previous applied legendary
resistance before considering a new contribution. This avoids both accumulating
resistance on rebuild and incorrectly suppressing itself at the 75% boundary.
An ordinary buff that already exceeds the cap is not reduced by removing F12.

The loadout cache synchronizes changed managed contributions before returning a
new view; ordinary player command entry and relevant calculation entry points
also refresh it. Stable reads retain the no-SQL-per-attack contract. Nested
source-effect removal can request an affect rebuild; a guarded deferred rerun
prevents recursive/double application while preserving the final ordinary state.

`infuse loadout` shows contribution values and identifies ordinary-cap/typed-bonus
supersession. Situational values remain subject to their documented weapon,
check, walking or regeneration conditions and the existing combat caps.

## Checked forced movement

Push attempts now pass an explicit forced-movement maneuver context, allowing
F13 to affect the defender's actual check without affecting other uses of an
undefined maneuver type. The existing boolean test incorrectly accepted negative
opposed-check results as successful pushes. It now treats all nonpositive
results as failures, matching the documented maneuver return contract. Stunned
or otherwise unopposed cases retain their ordinary bypass behavior; F13 does not
invent an immunity or a new saving throw.

The production push test also exposed a pre-existing undefined signed shift in
`Q_BIT` for bit 31 (`AFF_STUN`). The shared mask now uses unsigned `1U`, preserving
the existing bit layout and serialized flag representation. A high-bit
set/test/clear/toggle regression covers this fix under UBSan.

## Validation

```sh
python3 tests/run_legendary_foundations.py
ASAN_OPTIONS=detect_leaks=0 \
ESTATE_TEST_CFLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer -g' \
python3 tests/run_legendary_foundations.py
python3 tests/run_legendary_rules.py
```

The runner uses disposable MariaDB and includes all preceding ledger,
persistence, transaction fault/replay, earnings, player-command, equipment and
estate/housing/auction/stall regressions. Foundation calculations, cache, codec
and transaction code are production implementations. The surrounding ordinary
stat rebuild/cap/base-HP calculations use the fixture adapter, allowing exact
cap and no-refill assertions without duplicating legendary formulas.

The production Force damage tail and complete push-attempt routine are extracted
verbatim. Tests verify that the Force tail follows critical multiplication,
attack contributions precede the existing attack cap, and negative/zero push
results fail. Full server builds check the integration hooks in the normal
combat, magic, skill, movement and regeneration code.

Coverage includes all sixteen foundations; all six attribute and five elemental
subtypes; actual supported skill checks and excluded counters/crafting; unchanged
trained ranks/base stats; all five ranks without scaling; a nine-channel damage
ceiling; deflection supersession and retained penalties; save/attribute/pool caps;
allocated duplicate contribution reporting; repeated rebuilds and SQL query
counts; a real nested object-codec reconnect round trip; unchanged current pools
on increases and no equip/remove refill loop; caster/resource loss; opposed-only
maneuver bonuses; mounted/free walking exclusions; and out-of-combat-only
regeneration. The existing source-removal test also recalculates affects to
exercise deferred rebuild handling.

Validation completed successfully: the foundation integration suite (including
earlier-phase regressions), pure legendary rules tests, and AddressSanitizer /
UndefinedBehaviorSanitizer run passed. Both Make and CMake server builds passed
without compiler warnings, and `git diff --check` passed.

No additional schema migration is needed beyond v4. No live database or world
files were modified, and the MUD was not restarted. This phase does not enable
`infuse use` or introduce technique/signature effects.
