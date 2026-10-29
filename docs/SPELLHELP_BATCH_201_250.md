# Spellhelp batch 201–250

All 50 numeric IDs have reviewed core-mechanics records. Six healing records
(220, 221, 225, 228, 232, 245) already existed and were verified again; 44 new
records were added. The batch includes the internal Incendiary pulse (219),
three unfinished/unregistered IDs (241–243), and campaign-dependent Plane Shift
(239). Gate remains registered but has no creation handler in Dragonlance or
Forgotten Realms builds.

The catalog now has **256 distinct mechanics records**: 243 entries with player
registrations (including campaign-dependent Plane Shift), seven unregistered
legacy/unfinished IDs, and six internal abilities/effects. In Dragonlance builds,
242 catalog entries are public; the audited IDs 1–250 contain 236 public entries,
seven unregistered IDs, six internal IDs, and unavailable Plane Shift.
Full feat/perk/equipment interaction audits remain a separate phase.

| ID | Spellhelp name | Availability |
| ---: | --- | --- |
| 201 | iron skin | Registered |
| 202 | mass cunning | Registered |
| 203 | blade of disaster | Registered |
| 204 | summon creature ix | Registered |
| 205 | gate | Registered; creation handler excluded in DL/FR |
| 206 | wail of the banshee | Registered |
| 207 | power word kill | Registered |
| 208 | enfeeblement | Registered |
| 209 | weird | Registered |
| 210 | shadow shield | Registered |
| 211 | prismatic sphere | Registered |
| 212 | implode | Registered |
| 213 | timestop | Registered |
| 214 | greater spell mantle | Registered |
| 215 | mass enhance | Registered |
| 216 | portal | Registered |
| 217 | gust of wind | Registered |
| 218 | holy weapon | Registered |
| 219 | incendiary | **Internal pulse; not player-castable** |
| 220 | cure moderate | Registered |
| 221 | cure serious | Registered |
| 222 | remove fear | Registered |
| 223 | cure deafness | Registered |
| 224 | faerie fog | Registered |
| 225 | mass cure light | Registered |
| 226 | aid | Registered |
| 227 | bravery | Registered |
| 228 | mass cure moderate | Registered |
| 229 | regeneration | Registered |
| 230 | free movement | Registered |
| 231 | strengthen bones | Registered |
| 232 | mass cure serious | Registered |
| 233 | prayer | Registered |
| 234 | remove disease | Registered |
| 235 | word of faith | Registered |
| 236 | dimensional lock | Registered |
| 237 | salvation | Registered |
| 238 | spring of life | Registered |
| 239 | plane shift | **Registration/dispatch excluded in DL/FR** |
| 240 | storm of vengeance | Registered |
| 241 | death shield | **Unfinished; unregistered** |
| 242 | command | **Unfinished; unregistered** |
| 243 | air walker | **Unfinished; unregistered** |
| 244 | group summon | Registered |
| 245 | mass cure critic | Registered |
| 246 | charm animal | Registered |
| 247 | faerie fire | Registered |
| 248 | goodberry | Registered |
| 249 | jump | Registered |
| 250 | magic fang | Registered |

## Findings reflected in descriptions

- **Iron Skin (201):** up to 50 damage absorbed per eligible hit from a reserve of 35 times max(10, caster level), duration/depletion, and ward conflicts. The common damage path also invokes this absorption; it is not described as universally melee-only based on an outdated helper comment.
- **Mass Cunning/Mass Enhance (202/215):** group requirement, actual enhancement scaling, individual/mass buff replacement, and durations.
- **Blade of Disaster/Summon Creature IX (203/204):** template-based followers, final level 20, follower limits, lifetime, failure behavior, and live summon scaling. The blade is a creature rather than an inventory weapon.
- **Gate (205):** conditional descriptions distinguish the missing DL/FR creation handler from the two-way planar portal implementation in other builds.
- **Wail of the Banshee (206):** sound damage plus a separate fear condition, independent Fortitude saves/resistance, and different immunity checks; no automatic death.
- **Power Word Kill (207):** no-save mental damage with target-current-HP bonus only at 121 HP or less, subject to spell resistance and death-magic immunity; no unconditional extraction/death.
- **Enfeeblement (208):** area Strength/Dexterity/Constitution penalties and the target's -4 Fortitude-save modifier.
- **Weird (209):** single-target illusion damage, Strength loss, one-round stun, and the unusual caster-side Phantasm Resistance branch that exits regardless of its save result.
- **Shadow Shield (210):** shield AC, percentage negative-energy resistance, adamantine-bypassed physical DR, duration, and current global AC cap. The DR has no finite damage reserve.
- **Prismatic Sphere (211):** creates the sphere creature and runs its load trigger. The manual handler does not establish charm/following, timed expiration, ordinary global summon scaling, or a directional wall object.
- **Implode (212):** schedules integer caster-level/3 attacks at six-second intervals; each targets the current combat opponent and rechecks resistance and Reflex saves. Damage is piercing, not instant death; original casting-class bonus is not retained across events.
- **Timestop (213):** accelerates eligible nonviolent casting and buff sequencing for 14 rounds; no freeze of all other creatures is implemented.
- **Greater Spell Mantle (214):** four charges, level-scaled duration, replacement, selected-target requirement, and device/Warlock exceptions.
- **Portal (216):** paired passages, two object-timer ticks, travel/template/target restrictions, and no immediate teleport on casting.
- **Gust of Wind (217):** immediate room fog removal and occupant-affect cleanup; falling damage/removal of flight includes allies and caster. Small-or-smaller knockdown has a separate Reflex save and Spring Attack exception; falling has Slow Fall mitigation. Affect cleanup does not zero the separate cloud-following counters.
- **Holy Weapon (218):** creates the Holy Avenger template in inventory, with weapon type/base dice/size selected by `holyweapon`. Other properties follow the template; no existing item or unarmed perk state is enchanted.
- **Incendiary (219):** named help-only entry for the internal fire pulse; casting availability is unchanged.
- **Cure Moderate/Serious and Mass Cure variants (220/221/225/228/232/245):** existing formulas and group dispatch rechecked against `mag_points()` and `perform_mag_groups()`.
- **Remove Fear/Cure Deafness/Faerie Fog/Remove Disease (222/223/224/234):** actual condition-record/flag cleanup, immediate behavior, and lack of invented lasting immunity or healing. Faerie Fog strips invisibility rather than applying Faerie Fire or a persistent fog room effect.
- **Aid/Prayer (226/233):** group bonuses, bonus types, maximum/current HP changes, replacement/blocking rules, and 300-round duration. Aid uses max(level, 15), a minimum rather than a cap.
- **Bravery (227):** fear/shaken cleanup and separately applied duration-scaled fear immunity.
- **Regeneration (229):** initial Serious-style healing plus the regeneration multiplier/minimum found in `regen_hps()`, rather than a fixed periodic damage-dice formula.
- **Free Movement (230):** initial entanglement cleanup, paralysis/daze immunity helpers, and paralysis/stun clearing during normal command/combat processing; no universal protection against every movement restriction is assumed.
- **Strengthen Bones (231):** undead-only racial AC, duration, and global AC cap.
- **Word of Faith (235):** registered but missing an effect case. The entry reports the absent effects rather than inventing tabletop outcomes.
- **Dimensional Lock (236):** character debuff checked by selected travel-helper paths, with Will save/resistance and duration; not a universal room barrier.
- **Salvation (237):** first-cast room bookmark, consumed return flag, plane/wilderness/departure restrictions, and absence of a destination-helper recheck on return.
- **Spring of Life/Goodberry (238/248):** actual object creation and subsequent normal fountain/food behavior. Buffs, consumable durations, and object lifetimes come from world templates; no direct healing rolls are invented. The current DL Goodberry template is food with +10 maximum HP and +1 HP regeneration for 100 rounds when eaten; this template data is not hardcoded into spellhelp.
- **Plane Shift (239):** named unavailable help in DL/FR builds without adding a gameplay registration; the documentation also describes the manual travel behavior where enabled. Test registration extraction preserves the campaign guard.
- **Storm of Vengeance (240):** Ice Storm at six seconds followed by Chain Lightning at twelve seconds, with each handler's actual cold/electric damage, save, resistance, and current-room targeting. No acid rain or invented multi-round storm sequence is claimed; original casting-class damage bonuses are not preserved.
- **Death Shield/Command/Air Walker (241–243):** unfinished numeric IDs remain unregistered and are searchable only as unavailable documentation.
- **Group Summon (244):** group members transported from other rooms with no-summon/travel checks and arrival triggers.
- **Charm Animal (246):** animal NPC restriction, shared charm saves/capacity/immunity/duration, and no inherited Charm Person level-7 cap.
- **Faerie Fire (247):** visible outline and -2 AC, with resistance but no saving throw; no connected general concealment suppression.
- **Jump (249):** +10 Acrobatics and duration based on actual Druid + Ranger levels, including zero duration without those classes.
- **Magic Fang (250):** animal-only +1 enhancement attack bonus, without invented damage bonuses or magic-weapon flags.

These changes document existing behavior. Gameplay mechanics, registrations,
world templates, and casting availability were not changed.

## Validation

- Sanitized command/renderer checks compile the production `spello()` initializer,
  243 literal registration calls for IDs 1–250, and applicable literal setters;
  the Plane Shift campaign guard is retained.
- All 250 IDs render without mutating player, config, or registration state.
- Checks cover each numeric ID, unavailable/campaign entries, colors/alignment,
  formulas, effects, target constraints, summon scaling, live AC cap, and damage
  bonus handling for immediate versus delayed damage.
- Full CMake build passes.

Regenerate the numeric source index with:

```sh
python3 tools/audit_spellhelp.py --range 201 250 --json
```
