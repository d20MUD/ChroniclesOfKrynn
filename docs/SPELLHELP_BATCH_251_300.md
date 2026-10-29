# Spellhelp batch 251–300

All 50 numeric IDs now have reviewed core-mechanics records; all 50 are new.
The batch includes two internal breath IDs (271/275), three unregistered IDs
(274/277/294), and the registered generic area-attack messaging ID (300).

The catalog now has **306 distinct mechanics records**: 288 entries with public-style
registrations (including campaign-dependent Plane Shift), ten unregistered
legacy/unfinished IDs, and eight internal abilities/effects. In Dragonlance builds,
287 catalog entries are public; audited IDs 1–300 contain 281 public entries,
ten unregistered IDs, eight internal IDs, and unavailable Plane Shift.
Full cross-cutting feat/perk/equipment interaction audits remain a separate phase.

| ID | Spellhelp name | Availability |
| ---: | --- | --- |
| 251 | magic stone | Registered |
| 252 | obscuring mist | Registered |
| 253 | produce flame | Registered |
| 254 | natures ally i | Registered |
| 255 | natures ally ii | Registered |
| 256 | natures ally iii | Registered |
| 257 | natures ally iv | Registered |
| 258 | natures ally v | Registered |
| 259 | natures ally vi | Registered |
| 260 | natures ally vii | Registered |
| 261 | natures ally viii | Registered |
| 262 | natures ally ix | Registered |
| 263 | barkskin | Registered |
| 264 | flame blade | Registered |
| 265 | flaming sphere | Registered |
| 266 | hold animal | Registered |
| 267 | call lightning storm | Registered |
| 268 | summon swarm | Registered |
| 269 | contagion | Registered |
| 270 | greater magic fang | Registered |
| 271 | frost breathe | **Internal breath; not player-castable** |
| 272 | spike growth | Registered |
| 273 | blight | Registered |
| 274 | reincarnate | **Unimplemented; unregistered** |
| 275 | lightning breathe | **Internal breath; not player-castable** |
| 276 | spike stone | Registered |
| 277 | baleful polymorph | **Unimplemented; unregistered** |
| 278 | death ward | Registered |
| 279 | hallow | Registered |
| 280 | insect plague | Registered |
| 281 | unhallow | Registered |
| 282 | wall of fire | Registered |
| 283 | wall of thorns | Registered |
| 284 | fire seeds | Registered |
| 285 | confusion | Registered |
| 286 | mass endurance | Registered |
| 287 | mass strength | Registered |
| 288 | mass grace | Registered |
| 289 | magic resistance | Registered |
| 290 | spellstaff | Registered |
| 291 | transport via plants | Registered |
| 292 | creeping doom | Registered |
| 293 | fire storm | Registered |
| 294 | greater scrying | **Unimplemented; unregistered** |
| 295 | sunbeam | Registered |
| 296 | animal shapes | Registered |
| 297 | control plants | Registered |
| 298 | finger of death | Registered |
| 299 | elemental swarm | Registered |
| 300 | aoe attack | Registered generic ability/message marker |

## Findings reflected in descriptions

- **Magic Stone (251):** min(5, (level+1)/2) immediate single-target hits, each 1d6+1 bludgeoning with Reflex half/resistance. The loop has no touch roll, and the original casting-class bonus is not restored between stones.
- **Obscuring Mist (252):** room concealment/visibility behavior, actual divine-level duration, underwater failure, Wind Wall/Gust of Wind cleanup, and normal concealment rules.
- **Produce Flame/Flame Blade (253/264):** immediate single-target fire damage, actual level caps, no save, resistance, and underwater failure; neither creates a held flame or weapon.
- **Nature's Ally I–VI (254–259):** dire badger/boar/wolf/spider/bear/tiger templates, follower limits, lifetime, and current global summon scaling.
- **Nature's Ally VII–IX (260–262):** random fire/earth/air/water elemental templates, final levels 16/18/20, recalculated stats, base failure roll, capacities, lifetime, and summon scaling.
- **Barkskin (263):** natural-armor thresholds +2/+3/+4/+5 at levels below 6/6/9/12, actual 200-times-level round duration, stacking, and live AC cap.
- **Flaming Sphere/Call Lightning Storm (265/267):** immediate area damage rather than persistent effects or scheduled storms; actual formulas, caps, saves, and resistance.
- **Hold Animal (266):** animal-only level-11 cap, paralysis immunity, Will save/resistance, and 1d3+1 round paralysis.
- **Summon Swarm (268):** no-save negative-energy damage with a minimum level bonus of 5; undead heal. Wind Wall prints a dispersal message but does not return before damage. No follower or persistent swarm is created.
- **Contagion (269):** all seven equally likely ability-penalty diseases, 300-round duration, disease immunity, direct affliction HP threshold, and lack of initial save/resistance checks in the disease handler. Blinding sickness does not apply blindness.
- **Greater Magic Fang (270):** animal-only enhancement attack bonus based on actual Druid class level, level-scaled duration, and no extra damage/weapon flag.
- **Frost/Lightning Breathe (271/275):** help-only internal generic breath formulas use source character level d16, Reflex half and no resistance. Alternate racial/evolution ability paths can supply their own damage while reusing these IDs.
- **Spike Growth/Spike Stone (272/276):** room-entry 2d4/4d4 earth damage, mount/rider handling, nature predicate, actual duration, no flight exemption in the entry handler, and no preserved original global damage bonus.
- **Blight (273):** plants only, earth dice plus capped level bonus, Fortitude half, and resistance.
- **Reincarnate/Baleful Polymorph/Greater Scrying (274/277/294):** unavailable documentation only; no registration or corresponding spell handler is added.
- **Death Ward (278):** common death-magic immunity, Energy Drain blocking, flat 10 unholy/20 negative-energy absorption, duration, and limits on claiming universal immunity to all attacks of those types.
- **Hallow/Unhallow (279/281):** normal movement alignment-entry restrictions, 1000-round room duration, mutually canceled holy/unholy predicates when both flags exist, and holiness checks on undead creation.
- **Insect Plague (280):** immediate no-save/no-resistance negative-energy damage, undead healing, actual scaling, and effective Wind Wall protection.
- **Wall of Fire/Thorns (282/283):** directional objects require an open exit/template and do not themselves stop movement. `check_wall()` adds 2d6 to the creator's character level, then `mag_damage()` rolls another 2d6, giving base 4d6+character level fire/earth damage. Fortitude saves, resistance differences, friendly passage, PK/pet restrictions, object timers, fallback creator level, and lack of preserved casting-class bonus are described.
- **Fire Seeds (284):** source-character-level d4 plus caster-level fire damage, Reflex half, no resistance, and no created seeds/delayed explosion.
- **Confusion (285):** Will save/resistance, confusion and mind immunity checks, actual duration, and combat disruption/random targeting.
- **Mass Endurance/Strength/Grace (286–288):** group enhancement scaling/duration, Mass Enhance blocking, removal of individual versions, and normal stacking.
- **Magic Resistance (289):** resistance floor of 12+recipient character level, not caster-level-based additive resistance, with actual duration and requirement that an attack check resistance.
- **Spellstaff (290):** held staff with no remaining charges, supported spell list, one charge, character-level device potency, and the one-word argument limitation for multiword choices.
- **Transport via Plants (291):** moves the caster to the target's room, accepted source/destination terrains (including city), travel/plane/target restrictions, and arrival handling.
- **Creeping Doom (292):** following-swarm counter, duration, and exclusivity. Current pulses call the bite and poison handlers with a null character target; their registrations dispatch single-target damage/affects, whose null-victim checks return without applying effects. The entry therefore reports no delivered pulse damage/poison rather than presenting intended AoE damage as working.
- **Fire Storm (293):** min(20, caster level)d6 immediate fire damage, Reflex half/resistance, and normal area eligibility.
- **Sunbeam (295):** light damage, independent Reflex/resistance checks for blindness, blindness penalties, and room daylight with separate durations.
- **Animal Shapes (296):** independent random legacy forms, all stat/water-breathing/waterwalking bonuses, 100-round duration, expiration reset, Panther +4 Stealth, base 2d6 natural attacks, weapon/dual-wield suppression, and shared BAB interaction. No modern wildshape form list is substituted.
- **Control Plants (297):** plant NPC restriction plus shared charm saves, capacity/immunity checks, and Charisma/NPC-scaled duration; no assumed Charm Person level cap.
- **Finger of Death (298):** failed-save 1+10-times-level unholy damage versus successful-save 3d6+min(25, level); death immunity and resistance apply, without unconditional death.
- **Elemental Swarm (299):** 2d4 followers of one random elemental template type, capacity-at-cast check, template-based levels, lifetime, and global summon scaling.
- **AoE Attack (300):** generic registered messaging/eligibility ID has no dedicated standalone magic-damage formula. The Manticore Tailspikes callback separately deals 3d6+10 piercing damage, requires its race/form and swift action, and does not roll saves/resistance. Spellhelp distinguishes that callback from casting the registered marker.

These changes document existing behavior. Gameplay mechanics, registrations,
world templates, and casting availability were not changed.

## Validation

- Sanitized command/renderer checks compile the production `spello()` initializer,
  290 literal registration calls for IDs 1–300, and applicable literal setters;
  the Plane Shift campaign guard remains intact.
- All 300 IDs render without mutating player, config, or registration state.
- Checks cover each new ID, internal/unregistered availability, colors/alignment,
  formulas, conditions, source/recipient-level distinctions, summon scaling,
  live AC cap, and immediate/environmental damage bonus behavior.
- Catalog audit confirms 306 distinct records with no duplicate numeric symbols.
- Full CMake build passes.

Regenerate this batch's numeric source index with:

```sh
python3 tools/audit_spellhelp.py --range 251 300 --json
```
