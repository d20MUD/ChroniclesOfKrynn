# Spellhelp batch 051–100

All 50 numeric IDs have reviewed core-mechanics records. This batch contains
49 player spell registrations and one reserved internal creature ability,
Fire Breathe (76). Mage Shield (89) and Summon Creature I (90) were already
documented and were checked again; 48 new records were added.

The catalog now has **114 mechanics records: 109 registered player spells/powers,
four unavailable legacy IDs, and one internal ability**. Core mechanics and
availability are covered; full cross-cutting feat/perk/equipment audits remain
a separate phase.

| ID | Spellhelp name | Availability |
| ---: | --- | --- |
| 51 | waterwalk | Registered |
| 52 | identify | Registered |
| 53 | fly | Registered |
| 54 | blur | Registered |
| 55 | mirror image | Registered |
| 56 | stone skin | Registered |
| 57 | endurance | Registered |
| 58 | mummy dust | Registered |
| 59 | dragon knight | Registered |
| 60 | greater ruin | Registered |
| 61 | hellball | Registered |
| 62 | epic mage armor | Registered |
| 63 | epic warding | Registered |
| 64 | cause light wound | Registered |
| 65 | cause moderate wound | Registered |
| 66 | cause serious wound | Registered |
| 67 | cause critical wound | Registered |
| 68 | flame strike | Registered |
| 69 | destruction | Registered |
| 70 | ice storm | Registered |
| 71 | ball of lightning | Registered |
| 72 | missile storm | Registered |
| 73 | chain lightning | Registered |
| 74 | meteor swarm | Registered |
| 75 | protection from good | Registered |
| 76 | fire breathe | **Internal creature ability; not player-castable** |
| 77 | polymorph self | Registered |
| 78 | endure elements | Registered |
| 79 | expeditious retreat | Registered |
| 80 | grease | Registered |
| 81 | horizikauls boom | Registered |
| 82 | ice dagger | Registered |
| 83 | iron guts | Registered |
| 84 | mage armor | Registered |
| 85 | negative energy ray | Registered |
| 86 | ray of enfeeblement | Registered |
| 87 | scare | Registered |
| 88 | shelgarns blade | Registered |
| 89 | mage shield | Registered |
| 90 | summon creature i | Registered |
| 91 | true strike | Registered |
| 92 | wall of fog | Registered |
| 93 | darkness | Registered |
| 94 | summon creature ii | Registered |
| 95 | web | Registered |
| 96 | acid arrow | Registered |
| 97 | daze monster | Registered |
| 98 | hideous laughter | Registered |
| 99 | touch of idiocy | Registered |
| 100 | continual flame | Registered |

## Findings reflected in descriptions

- **Identify (52):** identifies objects or displays character lore. The audit scanner now ignores commented-out registrations so an obsolete `!UNUSED!` example cannot replace its real name.
- **Mirror Image (55):** describes image count, per-hit redirection, image loss, and the exclusion of spell damage.
- **Stoneskin (56):** documents DR 10/adamantine and its 60–150 damage absorption budget, rather than the disabled older 15-point implementation.
- **Mummy Dust (58), Dragon Knight (59):** describe level 30–35 epic followers, mutually exclusive epic ally slots, separate preparation-cast costs, and the three-round special-action interval. Mummy dread penalizes attack/AC; dragon breath has its own level-based fire formula.
- **Greater Ruin (60), Hellball (61):** include the shared 150% epic multiplier, spell-specific saves/resistance, epic cast costs/metamagic restrictions, and defense fracture or lingering hellfire.
- **Epic Mage Armor (62), Epic Warding (63):** cover bonus types and replacement rules, ward pool and per-event absorption, and the initial concentration/knockdown-protection surge. Absorption is invoked on more than melee damage despite an older comment in the combat handler.
- **Cause Wounds (64–67):** retain the current holy damage types and document conversion to matching Cure spells against undead. Harm remains unholy following the separate requested fix.
- **Flame Strike (68), Destruction (69), Ice Storm (70), Ball of Lightning (71), Missile Storm (72), Chain Lightning (73), Meteor Swarm (74):** describe actual damage dice, caps, types, single-target versus room/repeated-hit behavior, and saves instead of assuming tabletop mechanics.
- **Fire Breathe (76):** remains internal/unused for player casting. Its generic handler and Dragon Knight’s distinct breath formula are explained without enabling it.
- **Polymorph Self (77):** follows the shapechanging engine, its form restrictions/bonuses, the small entry heal, and the manual return command; no timed polymorph affect is invented.
- **Endure Elements (78):** protects against periodic lava-terrain damage. The later 101–150 shared-path audit additionally found and documented its 10-percentage-point reduction across eight damage types.
- **Expeditious Retreat (79), Grease (80):** alter movement-point pools; the later 101–150 audit additionally documented Expeditious Retreat’s +30 speed bonus, nonstacking with Haste. Grease does not apply area knockdown.
- **Horizikaul’s Boom (81):** separately documents sonic damage and deafness, saves, duration, and casting/hearing consequences.
- **Mage Armor (84), Ray of Enfeeblement (86), Scare (87), True Strike (91), Web (95), Daze Monster (97), Hideous Laughter (98), Touch of Idiocy (99):** cover actual bonus/penalty magnitudes, durations, immunities, level cutoffs, consumption/removal, and combat consequences.
- **Wall of Fog (92), Darkness (93):** describe room affections, visibility, dispelling, and the actual precedence of lighting checks. Darkness is not advertised as the stronger permanent magic-dark room flag.
- **Acid Arrow (96):** is registered as manual timed events: 3d6 arrows at the caster’s current opponent, each six seconds apart. The unrelated direct 4d6 case is not used for its help formula; original casting-class bonuses are not guaranteed to survive the events.
- **Continual Flame (100):** creates a light-template object; no invented attack or spell-managed expiration timer.

## Validation

- The command test compiles the actual `spello()` initializer and 96 live registration calls for IDs 1–100, excluding commented-out calls. Of these, 95 are public spell registrations and one is reserved internal.
- Every ID from 1 through 100 is rendered under sanitizers, with availability checks, colored column/wrapping checks, and unchanged registration/player/config snapshots.
- Explicit checks cover batch-specific formulas, wards, summons, epic effects, timed Acid Arrow targeting, internal Fire Breathe, and existing global-modifier behavior.
- Production epic regression tests cover pools/recovery, persistence, saves, lingering damage, and summon actions. Undead routing and Deadly Fear regressions remain passing.

Regenerate the numeric source index with:

```sh
python3 tools/audit_spellhelp.py --range 51 100 --json
```

No new player spell was enabled, no damage type was changed in this batch, and
the gameplay epic damage multiplier is shared with help through its existing
constant value.
