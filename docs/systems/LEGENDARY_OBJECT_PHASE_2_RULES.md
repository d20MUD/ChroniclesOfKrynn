# Dragonmetal Awakening — Phase 2 definitions and pure rules

Status: implemented, 2026-10-23. No player commands, persistent state or combat
effects are enabled. This is the reusable rules layer for later phases.

Related: [plan](LEGENDARY_OBJECT_IMPLEMENTATION_PLAN.md),
[proposal](LEGENDARY_OBJECT_SYSTEM_PROPOSAL.md),
[Phase 1 contracts](LEGENDARY_OBJECT_PHASE_1_AUDIT.md).

## Delivered code

- [legendary.h](../../src/legendary.h): explicit stable IDs, typed definitions,
  immutable rule-table interfaces, input snapshots and pure-helper contracts.
- [legendary.c](../../src/legendary.c): the full approved catalog and calculations.
- [legendary_rules.c](../../tests/legendary_rules.c) and
  [test runner](../../tests/run_legendary_rules.py): tests link production code
  directly, without database/world stubs or copied implementations.
- Make/Automake and CMake source lists include the new module. Automake's generated
  build files were regenerated locally; only repository-tracked source inputs
  belong in the change.

## Immutable definitions

The catalog contains 16 foundations, 18 techniques, 13 signatures, 12 paths,
10 slot menus, all rank/reshape costs, six essences, supporting families,
modifier compatibility, daily limits, boss tiers and shared power budgets.
Numeric base/rank-V values are structured fields rather than parsed prose.

Stable persistent ID ranges: F01–F16 are 1–16, T01–T18 are 101–118, and S01–S13
are 201–213. Ranks are 0–5 with zero unawakened. Slots, paths, essences,
abilities, elements and families have explicit IDs in the header; adapters must
map them to game IDs rather than assume matching enum layouts.

`legendary_validate_definitions()` verifies identities, unique/reachable powers,
slot menus, path references, a valid rank-IV option for every power, foundation
ceilings and approved cumulative costs. It is exercised by the tests; startup
registration is intentionally deferred until the system is integrated.

## Helper contracts

| Interface | Behavior |
| --- | --- |
| `legendary_cost_between()` | Incremental sum between valid ordered ranks, including a zero-cost equal-rank interval. |
| `legendary_pair_cost()` | Independent cumulative primary + supporting costs. No shared payments or family discounts. |
| `legendary_add()`, `legendary_subtract()`, `legendary_to_int()` | Reject negative inputs, overflow, underflow and invalid narrowing; preserve output on failure. |
| `legendary_plan_essences()` | Reserve half the even rank cost from the matching type, then spend most abundant remaining balances first; ties favor lower stable essence ID. |
| `legendary_allocate_essences()` | Explicit matching requirement, supporting the all-matching path/family reshape fee without incorrectly applying rank's half split. |
| `legendary_can_afford()` | Check complete XP/gold/material affordability and return an essence payment plan; caller explicitly selects all-matching versus rank payment. Never debit the input wallet. |
| `legendary_item_eligible()` | Validate a factual snapshot: level, permanent equipment kind/slot, ownership and ordinary restrictions. Excludes temporary, expiring, transforming or unsupported items. |
| `legendary_supporting_active()` | Require eligible actually wielded weapon, actual two-hand slot or mandatory-two-hand ranged use, and no competing off-hand equipment. |
| `legendary_power_available()` | Slot menu plus chosen path, with true-dragonlance restrictions. Does not invent class capabilities or grant powers automatically. |
| `legendary_resolve_power()` | Validate rank and modifier, select base/V values, apply exactly one modifier, and return individual/family cooldowns. |
| `legendary_plan_passives()` | Process saved-priority requests, enforce 12 points, duplicate/subtype/category limits and resistance ceiling; explain each inactive request. |

The passive planner has a bounded 16-channel input capacity, above the nine
normal outfit contributions in the design. It does not infer typed stacking,
trained skill ranks, normal ability caps or owner/equip state from global game
data. Later adapters supply those through the request's eligibility flag and
current nonlegendary resistance; invalid/ineligible requests contribute nothing.
The item-facts structure likewise represents checked inputs, not a live-object
adapter or an ownership security boundary on its own.

Only Force, Vitality and Endurance allow two identical foundation copies.
Aptitude allows two distinct abilities, Expertise three distinct validated skills,
and Elemental weave two distinct elements. The resistance contribution cannot
raise effective resistance above 75%; a partially applicable contribution still
costs its normal passive points.

Enduring prefers a prepared trigger window when a power also has a later effect
duration. It never lengthens S11 control or S05 healing. T08 gains a timed benefit
only at V, so Enduring resolves only at V for that power. Readiness remains a
valid IV modifier for every power. `legendary_modifier_allowed()` describes
power-level compatibility; menus must use `legendary_resolve_power()` to check
rank-specific availability too.

All helpers are side-effect-free with respect to game state. Documented output
buffers are the only mutations; invalid operation calls do not partially publish
payments, costs or resolved powers. Passive planning reports invalid individual
requests instead of rejecting otherwise valid requests in the same outfit.

## Validation

Run from repository root:

```sh
python3 tests/run_legendary_rules.py
LEGENDARY_TEST_CFLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' python3 tests/run_legendary_rules.py
make -j4
cmake --build build-agent-check -j4
```

The focused suite compiles with GNU90, `-Wall -Wextra -Werror`. It covers:

- Every cumulative/interval rank price and all 36 rank pairs across all three
  supporting families, against independent expected cost fixtures.
- Complete rank-V totals: 3,850,000 XP, 775,000 gold, 700 fragments, 140 essences
  and six catalysts per channel, doubled exactly for two complete channels.
- Signed 64-bit overflow/underflow, negative data, narrowing and unchanged outputs
  on failed economic calculations.
- Matching shortages, all-matching reshape payments, deterministic allocation,
  affordability failures and 10,000 randomized essence-conservation cases.
- All slot categories, item/character level boundaries, binding, temporary gear,
  ordinary restrictions and actual two-hand/mandatory ranged eligibility.
- Complete catalog validation, path/slot availability, true-dragonlance gating,
  every final-value change, cooldown rounding and modifier exceptions.
- Passive priorities, point exhaustion, duplicate damage caps, ability/skill
  subtype ceilings, partial resistance contributions and invalid requests.

Both focused runs passed, including address/undefined-behavior sanitizers. Both
production build systems were checked; no new warning/error diagnostics were
reported. No live database migration or server restart was performed.

## Handoff

Phase 3 implements UUID serialization and persistent schemas; Phase 4 implements
payment/recovery. Later phases supply the live eligibility/context adapters,
command dispatch, activation and effects. This module intentionally does not
modify the command table, `gain_exp()`, ordinary item state or gameplay bonuses.
The Phase-1 formation fixture failure remains an unrelated later regression
prerequisite; this phase does not claim to have repaired it.
