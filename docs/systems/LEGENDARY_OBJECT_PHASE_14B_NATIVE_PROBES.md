# Phase 14B native combat probes

Status: incomplete release validation. Phase 15 has not started. These probes
exercise native combat rather than substituting Python damage formulas. They
are **stationary-target outgoing checks**, not the required seven-role,
complete-outfit, active-rotation, incoming-damage or group-stacking suite.

## Method

`tests/capture_legendary_native_profiles.py --probe` links the full server's
CMake objects, replacing only `fight.c.o` with that same production source
compiled with `LEGENDARY_NATIVE_PROBE`. The conditional hook observes final
HP damage after native reductions/budgets and before the ordinary HP mutation.
It does not change the damage, dodge, critical, mitigation or death logic.
Overkill does not count as additional HP removed.

Each baseline/control/legendary case boots independently from the same private
world/player/object copies and fixed seed. Saved player/object files are restored
between cases. Runtime SQL is forced to disposable MariaDB, Internet sockets
and cURL are blocked, and no game port/game loop is opened. Player identities
are not exported. Native combat queues and lookup entries are initialized as
for an entering PC.

The actor executes the native violence routine's three phases every six seconds.
Native event timers, affect rounds and legendary pulses advance. The target is
an aware level-30 NPC, prototype 626; normal HP loss, deaths and replacement
occur. Its automatic swings are suppressed for this outgoing check. There is
no claim about incoming survival or complete native NPC AI in these probes.

The legendary variant uses a prioritized, legal 12-point foundation allocation:
Accuracy, two Force contributions, and Warding. The fixture asserts the resulting
+1 Accuracy/+2 Force/+1 Warding before combat. Other foundations over budget
remain dormant. Eligible items also receive valid slot-specific new item
passives. Saved armor below the awakening threshold is not secretly promoted;
these snapshots have six/seven eligible channels, not the full nine required
for final acceptance. Major powers are not selected or invoked.

The original stationary fixture could retain the target's initial flat-footed
state. Those provisional numbers were discarded; the artifacts below explicitly
put the target in combat and remove flat-footed status before each actor phase.

## Corrected results

| Fixture | Seconds | Baseline HP damage | Legendary HP damage | Change |
| --- | ---: | ---: | ---: | ---: |
| Dual-wield fighter, candidate_012 | 30 | 1,409 | 1,457 | +3.41% |
| Dual-wield fighter, candidate_012 | 600 | 22,608 | 22,930 | +1.42% |
| Dual-wield assassin, candidate_014 | 30 | 3,131 | 3,153 | +0.70% |
| Dual-wield assassin, candidate_014 | 600 | 62,568 | 61,138 | -2.29% |
| Provisional ranged fallback, candidate_029 | 30 | 71 | 101 | +42.25% |
| Provisional ranged fallback, candidate_029 | 600 | 1,142 | 1,714 | +50.09% |

Every independently reloaded control exactly matched its baseline. The same seed
is not a promise of identical later critical/proc rolls once an additional hit
or earlier kill changes random consumption. Negative differences in one seeded
trajectory do not establish that legendary bonuses intrinsically lower damage.
The final suite must avoid interpreting such trajectory noise as a universal
balance result.

The ranged snapshot originally wielded a two-handed melee weapon, not a bow
(`ATTACK_TYPE_TWOHAND=4`, `ATTACK_TYPE_RANGED=2`). For this provisional test the
fixture supplies an existing level-20–30 world-prototype bow/crossbow in which
the actor is proficient, equips it through native wear rules, and stocks equal
finite supplies of ordinary, zero-weight native ammunition. Its real Point Blank
Shot feat is required. No feat is granted and no spell/resource expenditure is
bypassed. Its very low bow damage makes it a fallback setup rather than an
approved representative endgame archer. The extra sustained damage is only
572 / 600 = **0.953 DPS**, despite the large percentage.

This demonstrates that a percentage-only gate needs an explicit representative
build definition. It does not justify silently nerfing fixed bonuses, raising
the accepted gate, or declaring the ranged role passed. The owner was asked
whether to use geared representative role builds or require the limit for every
legal level-30 build, including weak fallback weapons. Record that answer in the
handoff before changing production definitions.

## Reproduction

```sh
cmake --build /tmp/krynn-legendary-phase8-cmake -j2
python3 tests/capture_legendary_native_profiles.py --probe --candidate candidate_012
python3 tests/capture_legendary_native_profiles.py --probe --candidate candidate_014 \
  --output docs/systems/legendary_balance_results/native_assassin_gate_probe.json
python3 tests/capture_legendary_native_profiles.py --probe --role ranged_attacker \
  --candidate candidate_029 \
  --output docs/systems/legendary_balance_results/native_ranged_gate_probe.json
```

Run from the repository root. Optional `--debug` runs the native child in GDB;
any signal or abnormal child exit fails the capture. Anonymous candidate numbers
are derived from sorted copied snapshots, so check the role/gear prerequisites
and provenance again when the source snapshots change. `--probe` defaults to
`native_gate_probe.json`, never overwriting the general snapshot report.

## Still required before closing phase 14B

- Resolve the representative-build interpretation and construct/validate seven
  complete nine-channel outfits, including independent two-handed support ranks.
- Exercise 30-second/600-second encounters with actual caster preparation,
  resource-limited psionic casting, healer/support behavior and legal recovery.
- Include both passive-only and active-power rotations, three-source stacking,
  and incoming reduction against the 8% gate.
- Resolve all genuine balance failures with synchronized definitions/tests/docs;
  do not waive a gate because these preliminary checks passed.
- Run appropriate integration/security checks and supported builds, record the
  final results, then proceed to phase 15's reviewable release artifacts.

## Owner decision on benchmark scope

The owner selected **representative geared role builds; retain fixed bonuses**.
The 5% target is not a universal cap for weak fallback weapon setups. Do not
repeat this question, nerf the fixed bonuses to fit the fallback result, or
raise the agreed 5%/8% targets for the representative suite. The provisional
low-damage bow result remains a diagnostic case, not a failed representative
endgame-archer gate. Build the remaining fixtures with ordinary native endgame
gear and real class/role capabilities.
