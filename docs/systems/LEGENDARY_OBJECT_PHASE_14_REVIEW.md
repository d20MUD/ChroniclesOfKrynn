# Phase 14: balance and player-flow review

Status: phase 14A pricing/usability work approved; native combat completion is
in progress as interim [phase 14B](LEGENDARY_OBJECT_PHASE_14B_HANDOFF.md).
Phase 13's security, process-recovery, journal-integrity and build checks passed.
No live world files or production database were changed.

## Economy assumptions approved by the owner

The owner replaced the three measured-route requirement with:

- 20 level-30 mob kills per hour.
- Five total level-30 objects obtained per hour from random and regular drops.
- Each object sells for 10% of its value.

The analysis reads the live world files without editing them. It finds 203
normally spawned, eligible level-30 mob prototypes and 283 positive-value
level-30 object prototypes after native shop exclusion flags. These are prototype
samples, not measured encounter/drop probabilities. The generated reports show
25th/50th/75th-percentile reward scenarios and 0%/50%/100% random-drop mixtures;
the owner has not specified that mixture.

Native reward rules matter: mob XP is divided by three before the level-30 solo
cap, while non-custom mob gold is generated as 40 + 60d3 (expected 160), rather
than using the gold number printed in the prototype. Current normal-race solo
XP cap is 29,183. Account, leadership, racial XP-curve and happy-hour benefits,
travel time, consumable expenses and vendor-specific bargaining are excluded.
The 10% sale price is the owner's assumption, not a change to shop mechanics.
Generated grade-six level-30 gear is valued at 3,130 gold by the current treasure
formula. Regular items use the prototype distribution. Ordinary generated items
and regular drops together total five items/hour, not five of each.

Before the approved price reduction, at median rewards and an equal
random/regular mixture:

| Quantity | Estimate |
| --- | ---: |
| Infusion XP/hour | 450,000 |
| Mob gold/hour | 3,200 |
| Item sales/hour | 5,157.5 |
| Total gold/hour | 8,357.5 |
| One channel to V: XP requirement | 3,850,000; about 8.56 hours |
| One channel to V: gold requirement | 775,000; about 92.73 hours |
| Two channels to V: gold requirement | 1,550,000; about 185.46 hours |
| Nine channels to V: gold requirement | 6,975,000; about 834.58 hours |

At median regular-item value, all-regular drops give 11,950 gold/hour and
all-random drops give 4,765 gold/hour. The acquisition caps still impose a
minimum seven allowance days per rank-V channel if caps are filled efficiently;
this is not a promise that unconfigured bosses will supply those materials.
Two-handed primary/supporting channels retain exactly the same costs as two
independent one-handed/shield channels.

## Owner-approved pricing — applied to new purchases

The owner approved reducing only gold costs to one tenth. This brings one channel's modeled gold
requirement to about 9.27 hours, close to the 8.56-hour XP requirement. XP,
materials, daily caps, rank mechanics and power values stay unchanged.

| Rank purchase | Previous gold | Approved gold |
| --- | ---: | ---: |
| I | 25,000 | 2,500 |
| II | 50,000 | 5,000 |
| III | 100,000 | 10,000 |
| IV | 200,000 | 20,000 |
| V | 400,000 | 40,000 |
| Cumulative V | 775,000 | 77,500 |

To preserve fee proportions, the corresponding approved reshape gold
fees are foundation 1,000; technique 1,500; signature 2,500; modifier 1,000;
path/supporting-family 5,000. Existing fragment/essence fees remain.
Both channels would still pay independently. The production constants, validation totals and independent tests now use the
approved prices; definition version is 3. Existing recorded investments are not
rewritten or refunded.

## Combat work and outstanding gate

`tests/run_legendary_balance.py` produces fixed-seed 30-second/600-second
sensitivity screens for the seven required role labels and one/three group
sources. The bundled profiles are explicitly synthetic. The screen uses the
production-exported foundation definitions and checks a legal 12-point passive
allocation. Multiple group sources do not multiply personal passive bonuses.
It does not invent active rotations, native caster resource behavior, complete
native group interactions or human combat observations. It cannot approve the
5% outgoing/8% incoming gates.

The screen flags 20 of 28 cases. In particular, +1 attack against an uncapped
50% hit chance can add about 10% relative weapon damage before Force or active
powers. Deflection AC is similarly sensitive to hit chance, while real existing
bonuses and caps may suppress it. This is a review finding, not permission to
silently alter Accuracy/Warding or to claim that live builds failed.

`tests/capture_legendary_native_profiles.py` additionally boots copies of the
world/player/object snapshots with real server objects, forces every database
connection to a disposable Unix-socket database, blocks application Internet
sockets/cURL, and never starts a game loop or opens a game port. It exports
pseudonymized native stat captures. Existing saved snapshots and their gear
must be checked for role coverage, legal grips, missing equipment and stable
counterfactual damage before they can become approved encounter baselines.
No passwords, account identifiers or character names are exported.

Balance approval remains open until the required seven valid build/rotation
fixtures, complete outfits and actual native group stacking have paired
30-second/600-second results within the agreed limits. The diagnostic artifacts
are deliberately labeled screening/capture data, rather than a substituted
passing playtest.

## Actual first-use flow

`infuse` now shows a short starting prompt. `infuse next <item> [channel]`
shows the immediate next selection/action and authoritative cost, with permanent
binding, the 12-point budget, the three-power limit and independent channel
progression explained when relevant. Advanced reference/reshape details remain
in `infuse help`, `choices`, `inspect` and `loadout`.

The production command/SQL fixture generated
[the first-use transcript](legendary_balance_results/player_flow.txt), covering
ordinary item → awakening quote/confirmation → free foundation → rank II →
free technique → attunement → equipped source → successful first activation.
This is an executable scripted walkthrough, not an ordinary-human usability
observation. The owner explicitly accepted this scripted walkthrough as the usability
review. That gate is satisfied; native combat balance remains separate. The
player guide now points to the navigator.

## Reproduction

```sh
python3 tests/run_legendary_balance.py
python3 tests/capture_legendary_native_profiles.py
python3 tests/run_legendary_flow.py
```

Raw assumptions, costs, percentile economy scenarios and combat screening rows
are in [results.json](legendary_balance_results/results.json) and the adjacent
CSV files. The tools never mutate prices or live world data. The owner-approved pricing
change is explicit in production definitions, the proposal and player guide.

## Resumed work and additional item passives

Native baseline drift was traced to display damage consuming the once-per-combat
Dark Revelation bonus against the player's own sentinel mobile ID. Inspection
now preserves Dark Revelation and Blackguard Reprisal; the native capture
requires repeatable same-seed controls. This fixes measurement integrity but
is not a completed native encounter playtest.

The owner requested one new rank-scaling passive per item **in addition to the
foundation**. [The implemented specification](LEGENDARY_ITEM_PASSIVES.md) lists
all sixteen options, slot menus, exact I–V values, ceilings and two-handed
handling. SQL schema v6 is required, with existing items unselected. These new
bonuses must be included in the still-open phase-14B combat gate.
