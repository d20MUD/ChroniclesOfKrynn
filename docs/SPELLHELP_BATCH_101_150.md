# Spellhelp batch 101–150

All 50 numeric IDs have reviewed core-mechanics records: 47 player spell
registrations and three internal shield-retaliation IDs (142–144). Fog Cloud
(128) was already documented and was verified again; 49 new records were added.

The catalog now has **163 mechanics records: 155 registered player spells/powers,
four unavailable legacy IDs, and four internal abilities/effects**. Full
cross-cutting feat/perk/equipment interaction audits remain a separate phase.

| ID | Spellhelp name | Availability |
| ---: | --- | --- |
| 101 | scorching ray | Registered |
| 102 | deafness | Registered |
| 103 | false life | Registered |
| 104 | grace | Registered |
| 105 | resist energy | Registered |
| 106 | energy sphere | Registered |
| 107 | water breathe | Registered |
| 108 | phantom steed | Registered |
| 109 | stinking cloud | Registered |
| 110 | summon creature iii | Registered |
| 111 | halt undead | Registered |
| 112 | deathly heroism | Registered |
| 113 | vampiric touch | Registered |
| 114 | hold person | Registered |
| 115 | deep slumber | Registered |
| 116 | invisibility sphere | Registered |
| 117 | daylight | Registered |
| 118 | clairvoyance | Registered |
| 119 | nondetection | Registered |
| 120 | haste | Registered |
| 121 | slow | Registered |
| 122 | dispel magic | Registered |
| 123 | circle against evil | Registered |
| 124 | circle against good | Registered |
| 125 | cunning | Registered |
| 126 | wisdom | Registered |
| 127 | charisma | Registered |
| 128 | fog cloud | Registered |
| 129 | acid splash | Registered |
| 130 | ray of frost | Registered |
| 131 | wizard eye | Registered |
| 132 | fire shield | Registered |
| 133 | cold shield | Registered |
| 134 | billowing cloud | Registered |
| 135 | summon creature iv | Registered |
| 136 | greater invisibility | Registered |
| 137 | rainbow pattern | Registered |
| 138 | locate creature | Registered |
| 139 | minor globe | Registered |
| 140 | enlarge person | Registered |
| 141 | reduce person | Registered |
| 142 | fire shield damage | **Internal retaliation; not player-castable** |
| 143 | cold shield damage | **Internal retaliation; not player-castable** |
| 144 | acid sheath damage | **Internal retaliation; not player-castable** |
| 145 | acid sheath | Registered |
| 146 | interposing hand | Registered |
| 147 | wall of force | Registered |
| 148 | cloudkill | Registered |
| 149 | summon creature v | Registered |
| 150 | waves of fatigue | Registered |

## Findings reflected in descriptions

- **Scorching Ray (101):** repeated single-target 4d6 fire hits, no damage save, resistance per hit, and no separate touch roll requested by the current loop. The original casting class is not automatically restored between rays.
- **Deafness (102):** hearing suppression plus the ordinary one-in-five casting fumble, with the Alchemist/Cure Deafness exceptions.
- **False Life (103):** maximum/current HP changes rather than a separate temporary-HP reserve.
- **Grace (104), Heroism (112), Cunning/Wisdom/Charisma (125–127):** actual enhancement bonuses and stacking/duration behavior, including the default enhancement type established by `new_affect()`.
- **Resist Energy (105):** flat 3 absorption against fire/cold/air/earth/acid/electricity/energy, plus lava protection. It is not a blanket percentage resistance.
- **Water Breathe (107):** uses the SCUBA/water-breath flag consumed by underwater hazard checks.
- **Phantom Steed (108), Summon Creature III/IV/V (110/135/149):** template-based followers, capacities, lifetimes, and current global summon scaling. The creatures are phantom steed, dire wolf, dire spider, and dire bear.
- **Stinking Cloud (109):** immediate nausea, not persistent damaging room fog; Fortitude DC reductions at target levels 10/20/30, Wind Wall and nausea immunity, casting restrictions, and the one-in-four combat-cycle interruption.
- **Halt Undead (111), Hold Person (114), Deep Slumber (115), Rainbow Pattern (137):** actual level limits, target restrictions, saves/immunities, conditions, and durations. Deep Slumber retains the pixie-caster exception. Hold Person checks living targets rather than humanoid race.
- **Vampiric Touch (113):** unholy d5 damage and separately rolled d4 caster healing; healing is not proportional to damage dealt. Normal touch attack and damage-save checks still apply.
- **Invisibility Sphere (116), Circle Against Evil/Good (123/124):** group members in the room receive the corresponding individual buff. Normal player casting requires a group; this differs from internal fallback behavior.
- **Clairvoyance (118), Non Detection (119), Wizard Eye (131), Locate Creature (138):** distinguish a remote room view, the specific anti-Clairvoyance ward, player control of a spawned eye, and a visible-name location search. Non Detection does not universally suppress Locate Creature.
- **Haste (120):** extra attack, movement-speed bonus, mutual cancellation with Slow, and the actual forced-movement modifiers.
- **Slow (121):** the applied flag has zero duration because the level-scaled duration is written to a different unused slot; it expires at the next affect update. Its actual effects, save, resistance, and cancellation are documented without changing gameplay.
- **Dispel Magic (122):** opposed caster checks, removal of one affect record on another target, broad self-dispel, and object-wall removal; no claim that it strips arbitrary magic items or reaches the helper’s no-target fog branch.
- **Acid Splash/Ray of Frost (129/130):** 2d4–6d4, extra dice at caster levels 7/15/22/30, Reflex half, and resistance; stale unfinished header comments do not replace implemented behavior.
- **Fire/Cold Shield and Acid Sheath (132/133/145):** retaliation dice/types, mutual exclusion, and percentage damage reduction. Fire Shield reduces cold by 50 percentage points; Cold Shield reduces fire; Acid Sheath currently reduces earth, not acid.
- **Internal shield IDs (142–144):** named help entries explain automatic retaliation while preserving internal/unused casting status.
- **Greater Invisibility (136):** survives normal non-forced appearance calls, is normally self-only, and has no matching object-alteration branch despite the registration flag.
- **Minor Globe (139):** lowest registered class level <=4 target-based protection; device/warlock and room-wide paths are not advertised as universally blocked.
- **Enlarge/Reduce Person (140/141):** actual size, Strength and Dexterity assignments; Reduce Person does not add the usual assumed ability modifiers.
- **Interposing Hand (146):** attack penalty, no save, and resistance; not a summoned hand creature.
- **Wall of Force (147):** directional object wall, open-exit/template checks, creator-based friendly passage checks, object-timer lifetime, and dispelling.
- **Cloudkill (148):** follows the caster, uses five-second affliction pulses and an arcane-magic-level poison formula, and has no instant-kill level cutoff. Original casting-class bonuses are not stored across pulses.
- **Waves of Fatigue (150):** immediate movement drain, attack/damage/AC penalties, blocked movement regeneration, and recovery exceptions.

## Shared-path corrections to earlier records

- Endure Elements (78) also adds 10 percentage points of reduction against fire/cold/air/earth/acid/electricity/energy/water; this was found in type-reduction checks rather than the elemental-protection flag checks.
- Expeditious Retreat (79) also adds 30 movement speed, nonstacking with Haste, in addition to its movement-pool bonus.
- Existing group-healing descriptions now state the normal group requirement instead of implying the internal ungrouped fallback can always be invoked through the player command.

These changes correct descriptions only. Slow, shield resistance types, timing,
and all other gameplay mechanics were left as implemented.

## Validation

- Sanitized command/renderer checks compile the production `spello()` initializer, all 146 literal registration calls for IDs 1–150, and the applicable literal registration setters.
- All 150 IDs render without mutating player, config, or registration state; four legacy IDs and four internal IDs remain unavailable for player casting.
- Checks cover labels/colors/wrapping, live configuration behavior, touch flags, batch-specific mechanics, and unavailable-effect lookup.

Regenerate this batch’s numeric source index with:

```sh
python3 tools/audit_spellhelp.py --range 101 150 --json
```
