# Spellhelp batch 151–200

All 50 numeric IDs have reviewed core-mechanics records: 49 player spell
registrations and the internal Deathcloud pulse (161). Acid Fog (163) was already
documented and was verified again; 49 new records were added.

The catalog now has **212 mechanics records: 203 registered player spells/powers,
four unavailable legacy IDs, and five internal abilities/effects**. Full
cross-cutting feat/perk/equipment interaction audits remain a separate phase.

| ID | Spellhelp name | Availability |
| ---: | --- | --- |
| 151 | symbol of pain | Registered |
| 152 | dominate person | Registered |
| 153 | feeblemind | Registered |
| 154 | nightmare | Registered |
| 155 | mind fog | Registered |
| 156 | faithful hound | Registered |
| 157 | dismissal | Registered |
| 158 | cone of cold | Registered |
| 159 | telekinesis | Registered |
| 160 | firebrand | Registered |
| 161 | deathcloud | **Internal damage pulse; not player-castable** |
| 162 | freezing sphere | Registered |
| 163 | acid fog | Registered |
| 164 | summon creature vi | Registered |
| 165 | transformation | Registered |
| 166 | eyebite | Registered |
| 167 | mass haste | Registered |
| 168 | greater heroism | Registered |
| 169 | anti magic field | Registered |
| 170 | greater mirror image | Registered |
| 171 | true seeing | Registered |
| 172 | globe of invuln | Registered |
| 173 | greater dispelling | Registered |
| 174 | grasping hand | Registered |
| 175 | summon creature vii | Registered |
| 176 | power word blind | Registered |
| 177 | waves of exhaustion | Registered |
| 178 | mass hold person | Registered |
| 179 | mass fly | Registered |
| 180 | displacement | Registered |
| 181 | prismatic spray | Registered |
| 182 | power word stun | Registered |
| 183 | protection from spells | Registered |
| 184 | thunderclap | Registered |
| 185 | spell mantle | Registered |
| 186 | mass wisdom | Registered |
| 187 | mass charisma | Registered |
| 188 | clenched fist | Registered |
| 189 | incendiary cloud | Registered |
| 190 | summon creature viii | Registered |
| 191 | horrid wilting | Registered |
| 192 | greater animation | Registered |
| 193 | irresistible dance | Registered |
| 194 | mass domination | Registered |
| 195 | scint pattern | Registered |
| 196 | refuge | Registered |
| 197 | banish | Registered |
| 198 | sunburst | Registered |
| 199 | spell turning | Registered |
| 200 | mind blank | Registered |

## Findings reflected in descriptions

- **Symbol of Pain (151):** immediate area unholy damage, not a placed symbol or a persistent pain penalty.
- **Dominate Person/Mass Domination (152/194):** shared charm/follower control, target-level limit, Will saves, follower capacity, immunities, and Charisma-scaled duration (tenfold for NPC targets). No humanoid-only check is assumed.
- **Feeblemind (153):** modifiers based on real Intelligence, Wisdom, and Charisma minus 3, rather than setting two scores to 1.
- **Nightmare (154):** immediate illusion damage without a damage save, followed by a separate Fortitude save against fatigue and movement loss. Mind immunity blocks the debuff, not the independent damage handler.
- **Mind Fog (155):** single-target -10 Will save penalty. Registration lists Enchantment, but the effect handler follows the Illusion save-school path.
- **Faithful Hound/Summon Creature VI (156/164):** hound/dire-tiger templates, follower limits, lifetime, and live global summon scaling.
- **Dismissal/Banish (157/197):** target a master and remove its qualifying nearby NPC followers. Dismissal removes one extraplanar follower; Banish does not require that subrace and has a 50% stop roll after each removal. Neither handler rolls saves or spell resistance.
- **Cone of Cold/Firebrand/Freezing Sphere (158/160/162):** currently single-target; actual dice, caps, additive level bonus, damage types, and saves are documented.
- **Telekinesis (159):** bludgeoning damage plus an independent 59% knockdown chance (strictly less than 60 on d100), with size and Epic Warding surge restrictions.
- **Deathcloud (161):** help-only internal entry for poison-damage pulses used by Cloudkill and other abilities; no new casting registration.
- **Acid Fog (163):** existing room/occupant effects and isolated per-round damage were verified again. Affected characters are not advertised as recasting the area spell.
- **Transformation (165):** total-level BAB and ordinary casting restriction, without invented ability-score/armor buffs.
- **Eyebite (166):** disease and the affliction pulse's direct HP threshold, not a generic sleep/panic/death effect or dice-based attack. No global damage multiplier is shown for the direct HP assignment.
- **Mass Haste/Mass Fly (167/179):** ordinary Haste/Fly effects on group members in the room, group requirement, cancellation/grounding behavior, and actual durations (12 times level/600 rounds).
- **Greater Heroism (168):** morale attack/save bonuses, skill bonuses, fear immunity, maximum/current HP increases, and removal of ordinary Heroism.
- **Anti Magic Field (169):** room-based casting/resolution prevention, duration, weapon-poison exception, and preservation of already active affects.
- **Greater Mirror Image (170):** image count, attack interception probability, consumption, and exclusion of spell damage; no image regeneration is claimed.
- **True Seeing (171):** supported invisibility, hiding, disguise, and altered-form visibility, without promising universal immunity to concealment or illusions.
- **Globe of Invuln/Spell Mantle/Spell Turning (172/185/199):** actual class-level threshold, two mantle charges, and single-reflection flag respectively; selected-target and device/Warlock exceptions where applicable.
- **Greater Dispelling (173):** 2d2 opposed removal attempts, one affect record per success, broad self-dispel, and limitations of the character-only registration despite the shared helper's other branches.
- **Grasping Hand (174):** bludgeoning damage and independent Reflex/resistance checks for entanglement, plus Slip the Bonds protection.
- **Summon Creature VII/VIII (175/190):** random elementals, final level 16/18, base failure roll, capacities, lifetime, and global scaling.
- **Power Word Blind/Stun (176/182):** no save or HP cutoff; actual immunities, resistance, duration, and blindness penalties.
- **Waves of Exhaustion (177):** no-save fatigue and movement drain, using the existing fatigue condition rather than stronger invented exhaustion penalties.
- **Mass Hold Person (178):** paralysis with Will save/resistance/immunity; unlike the individual Hold Person handler, no level-11 or living-target check.
- **Displacement (180):** concealment, caps, and blindsight bypass; true sight alone does not disable it.
- **Prismatic Spray (181):** illusion damage plus a separate random stun/paralysis/blindness/slow outcome, with independent saves and durations.
- **Protection from Spells (183):** +1 enhancement Will bonus and additive +10 spell resistance found in `mag_resistance()`, not blanket save bonuses.
- **Thunderclap (184):** unavoidable-by-save/resistance sound damage, then separately saved/resisted deafness, stun, and knockdown. Immunity early returns and Epic Warding surge behavior are documented.
- **Mass Wisdom/Charisma (186/187):** group enhancement bonuses, scaling, duration, and removal of the corresponding individual buff.
- **Clenched Fist (188):** bludgeoning dice and one-in-three independent movement-blocking event, lasting 100 seconds at the current pulse rate; not a stun flag.
- **Incendiary Cloud (189):** follows the caster with five-second pulses, arcane-level fire formula, remaining pulse count, cloud exclusivity, and Wind Wall. Original casting-class damage bonuses are not retained across pulses.
- **Horrid Wilting (191):** negative-energy area damage with the implemented cap, level bonus, and Fortitude save.
- **Greater Animation (192):** corpse requirement/consumption, template thresholds, final level 18, failure roll, follower capacity, contents transfer, and global summon scaling.
- **Irresistible Dance (193):** the unusual caster-side Paralysis Resistance branch and its unconditional return are stated as implemented. No ordinary target paralysis-immunity check is claimed.
- **Scint Pattern (195):** no-save confusion, resistance, immunity, and combat behavior.
- **Refuge (196):** ends combat, clears eligible enemies' casting/NPC memory, and grants others hide/sneak plus six rounds of Refuge (+15 Move Silently).
- **Sunburst (198):** fire damage using a Will save, independent Reflex save against blindness, and room daylight, with all three durations described.
- **Mind Blank (200):** protection through common mind-affecting/charm immunity helpers, not unconditional immunity to entire spell schools.

These changes document existing behavior. Gameplay mechanics, registrations,
and casting availability were not changed.

## Validation

- Sanitized command/renderer checks compile the production `spello()` initializer,
  all 196 literal registration calls for IDs 1–200, and applicable literal setters.
- All 200 IDs render without mutating player, config, or registration state;
  four legacy IDs and five internal IDs remain unavailable for player casting.
- Checks cover colors/alignment/wrapping, each new entry's mechanics, lookup,
  metadata, global settings, and hidden/internal availability.
- Full CMake build passes.

Regenerate this batch's numeric source index with:

```sh
python3 tools/audit_spellhelp.py --range 151 200 --json
```
