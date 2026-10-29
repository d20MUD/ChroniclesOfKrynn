# Spellhelp batch 451–500

This range contains 48 spells and two unused slots, 485 and 490. All 48 spells
now have reviewed records: 47 new records and the existing Glitterdust entry.
Repulsion's registration is commented out, so its retained handler is documented
as unavailable. The other 47 spells have public registrations. The catalog now
contains 445 distinct mechanics records.

| ID | Spellhelp name | Availability |
| ---: | --- | --- |
| 451 | planar healing | Registered |
| 452 | cushioning bands | Registered |
| 453 | ghost wolf | Registered |
| 454 | gird allies | Registered |
| 455 | glitterdust | Registered; existing entry verified |
| 456 | protection from arrows | Registered |
| 457 | spider climb | Registered |
| 458 | warding weapon | Registered |
| 459 | aqueous orb | Registered |
| 460 | summon mount | Registered |
| 461 | communal summon mount | Registered |
| 462 | human potential | Registered |
| 463 | mass human potential | Registered |
| 464 | black tentacles | Registered |
| 465 | greater black tentacles | Registered |
| 466 | control summoned creature | Registered |
| 467 | charm monster | Registered |
| 468 | mass enlarge person | Registered |
| 469 | mass reduce person | Registered |
| 470 | communal protection from arrows | Registered |
| 471 | rage | Registered |
| 472 | communal resist energy | Registered |
| 473 | siphon might | Registered |
| 474 | communal spider climb | Registered |
| 475 | caustic blood | Registered |
| 476 | greater planar healing | Registered |
| 477 | mass daze monster | Registered |
| 478 | hold monster | Registered |
| 479 | overland flight | Registered |
| 480 | communal stone skin | Registered |
| 481 | hostile juxtaposition | Registered |
| 482 | greater hostile juxtaposition | Registered |
| 483 | banishing blade | Registered |
| 484 | planar soul | Registered |
| 485 | Unused slot | No spell registration or implemented spell |
| 486 | repulsion | Unavailable; unregistered retained handler |
| 487 | mass charm monster | Registered |
| 488 | geniekind | Registered |
| 489 | grand destiny | Registered |
| 490 | Unused slot | No spell registration or implemented spell |
| 491 | touch of fatigue | Registered |
| 492 | lesser rejuvenate eidolon | Registered |
| 493 | rejuvenate eidolon | Registered |
| 494 | greater rejuvenate eidolon | Registered |
| 495 | lesser evolution surge | Registered |
| 496 | evolution surge | Registered |
| 497 | greater evolution surge | Registered |
| 498 | lesser restore eidolon | Registered |
| 499 | restore eidolon | Registered |
| 500 | purified calling | Registered |

The review follows actual registration dispatch, group/manual argument handling,
summon scaling and lifetimes, shared damage reduction, periodic damage,
reactive combat hooks, movement, and eidolon ownership restrictions. School,
class access, domain access, and registered target information continue to come
from live metadata. Existing cyan labels, wrapping, and aligned descriptions
are preserved.

Notable current behavior documented without gameplay changes:

- Planar Healing and Greater Planar Healing are counted both by name and by
  their fast-healing applies in the shared helper, contributing 2 and 8.
- Rage overwrites Constitution and Will affect slots, leaving Strength and AC.
- Siphon Might halves its transfer on a failed Fortitude save, and passes the
  spell ID as its casting-type argument.
- Greater Hostile Juxtaposition's reflection branch is unreachable because the
  combat hook repeats the lesser ID check.
- Repulsion is unregistered; its retained non-ranged attack hook rejects
  attackers before reaching the intended saving-throw branch.
- Mass Charm Monster is registered for area dispatch, while its charm consumer
  exists only in the group dispatcher, so normal casting charms nothing.
- Evolution Surge and Greater Evolution Surge use legacy AC modifiers that
  divide to zero. Lesser Evolution Surge uses the newer +2 AC modifier.
- Both Restore Eidolon variants share the nonfunctional restoration whitelist
  test already documented for Restoration.
- Human Potential's mass handler and Gird Allies compare group pointers,
  which can also match ungrouped characters/pet masters.
- Banishing Blade's current branch skips player attackers with condensed output.

Damage descriptions distinguish ordinary HP damage from healing, ability-score
transfer, damage reduction, and reflection. Delayed/periodic/retaliation effects
omit a guaranteed original casting-class global damage bonus. Summon entries,
including manual Geniekind and communal mounts, show current cedit summon
scaling. Applicable AC buffs show the live total AC cap.

Validation: sanitizer command/renderer tests load production registrations through
500, test every defined entry and unused slot, verify new entries through their
actual registered command names, and check unavailable status, configuration,
colors, alignment, audit coverage, source paths, and read-only state. The full
CMake build passes. No gameplay changes or server restart are included.
