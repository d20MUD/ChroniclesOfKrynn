# Spellhelp batch 301–350

This numeric batch contains **24 defined IDs (301–324)** and **26 unused slots
(325–350)**. All 24 defined IDs now have reviewed mechanics records; all are new.
The batch includes two internal effects (304/306) and five unregistered IDs
(302/315/317/318/321). The unused slots are documented here, not exposed as
invented spell names or enabled registrations.

The catalog now has **330 distinct mechanics records**: 305 entries with
public-style registrations (including campaign-dependent Plane Shift), fifteen
unregistered legacy/unfinished IDs, and ten internal abilities/effects.
In Dragonlance builds, 304 catalog entries are public. Audited IDs 1–324 contain
298 public entries, fifteen unregistered IDs, ten internal IDs, and unavailable
Plane Shift; IDs 325–350 remain empty. Full cross-cutting feat/perk/equipment
interaction audits remain a separate phase.

| ID | Spellhelp name | Availability |
| ---: | --- | --- |
| 301 | shambler | Registered |
| 302 | shapechange | **Unimplemented; unregistered** |
| 303 | blade barrier | Registered |
| 304 | blades | **Internal effect; not player-castable** |
| 305 | battletide | Registered |
| 306 | innate darkness | **Internal effect; not player-castable** |
| 307 | doom | Registered |
| 308 | whirlwind | Registered |
| 309 | levitate | Registered |
| 310 | draconic bloodline breath weapon | Registered bloodline ability/message ID |
| 311 | vigorize light | Registered |
| 312 | vigorize serious | Registered |
| 313 | vigorize critical | Registered |
| 314 | group vigorize | Registered |
| 315 | embalm | **Unimplemented; unregistered** |
| 316 | continual light | Registered |
| 317 | elemental resistance | **Unimplemented; unregistered** |
| 318 | preserve | **Unimplemented; unregistered** |
| 319 | resurrection | Registered |
| 320 | silence | Registered |
| 321 | minor create | **Unimplemented; unregistered** |
| 322 | dispel invis | Registered |
| 323 | shield of faith | Registered |
| 324 | group shield of faith | Registered |
| 325–350 | No defined spell names or registrations | **Unused slots** |

## Findings reflected in descriptions

- **Shambler (301):** 1d4+2 charmed shambling-mound followers, template-based levels, general follower capacity, lifetime, and live global summon scaling.
- **Shapechange/Embalm/Elemental Resistance/Preserve/Minor Create (302/315/317/318/321):** named unavailable records only. No new gameplay registration, transformation, corpse alteration, protection, or creation effect is added.
- **Blade Barrier (303):** room hazard with max(4, divine level) room-affect duration and five-second pulses. `room_aff_tick()` creates a temporary room caster and calls internal Blades at `DG_SPELL_LEVEL`; the base pulse is currently 25d6+2 slashing damage, not damage scaled to the original caster. `mag_masses()` skips only the temporary source for this effect, so original caster/allies are included, subject to ordinary damage defenses. Reflex half and resistance apply; the original casting-class damage bonus is not preserved.
- **Blades (304):** help-only internal pulse entry with effect-level d6+2 slashing damage, mass-target behavior, and no player casting activation.
- **Battletide (305):** +3 enhancement attack/damage, Haste benefits, 50-round duration, +25 hitroll/damroll caps, self-only registration, and the ordinary-casting block. No hostile aura or extra BAB/AC bonus is invented.
- **Innate Darkness (306):** help-only internal event that sets/clears the room-dark flag, ten-round lifetime, and separation from the ordinary Darkness room affect. Unavailable records now also display their documented duration, allowing this non-damaging timed effect to be explained accurately.
- **Doom (307):** actual targeted Shaken condition despite the stale header comment calling it a Creeping Doom proc. Shared attack/skill paths apply both Shaken and Frightened penalties (total -4 each). The saving-throw fear modifier checks the source caster's fear state; it is not presented as an ordinary blanket penalty to the affected creature's own saves. Will save, resistance, immunities, duration, and lack of fleeing Fear flag are documented.
- **Whirlwind (308):** immediate (caster level+1d3)d6+caster level air damage, Reflex half/resistance, and area eligibility. No persistent cyclone or forced movement is implemented.
- **Levitate (309):** 600-round levitation, falling prevention, boat/water-crossing checks (including mounts), and `land` removal. Does not imply flight-flag or water-breathing benefits.
- **Draconic Bloodline Breath Weapon (310):** generic magic path uses actual Sorcerer class level d6, Reflex half, and no resistance; the heritage command uses effective draconic level d6 through level 15, then d14, with no save/resistance in its callback. Bloodline determines damage type. Heritage uses/actions, common area eligibility, and shared messaging ID reuse are described without conflating the formulas.
- **Vigorize Light/Serious/Critical (311–313):** 20d4+50, 40d4+150, and 60d5+150+10 times caster level movement restoration, capped at recipient maximum. These are not HP or PSP healing. Feeding-state movement modifiers remain applicable; global damage bonuses are not shown as restoration bonuses. The shared point-restoration handler also dissolves Boneshard bleeding.
- **Group Vigorize (314):** group requirement, room recipients, Vigorize Critical dispatch/formula, and shared Boneshard cleanup.
- **Continual Light (316):** creation of the configured light object in inventory, sharing the Continual Flame template, without invented room fire or damage.
- **Resurrection (319):** online/playing corpse-owner matching, player-corpse/location requirements, relocation and combat cleanup, corpse consumption, XP recovery with actual integer rounding/caps, mortal movement/action costs, and caster/recipient wait durations. The handler does not directly refill HP or transfer corpse contents.
- **Silence (320):** targeted silence with Will save/resistance/immunity, duration, supported communication restrictions, psionic/extract exceptions, Automatic Silent Spell exception, and the differing checks for manual Silent metamagic between casting entry paths. No silent-room aura or all-channel block is claimed.
- **Dispel Invis (322):** character invisibility cleanup and object invisibility removal; the shared character handler's inventory sweep applies to the caster's inventory even when another character is targeted. No save/resistance or persistent detection benefit is added.
- **Shield of Faith/Group Shield of Faith (323/324):** min(5, 2+caster level/6) deflection AC, 400-round duration, group dispatch/requirement, stacking, and live global AC cap. Their numeric definitions occur earlier in `spells.h`; they are not unused just because the lower header comment lists an open range.
- **325–350:** no symbolic spell definitions or literal registrations were found. Both the runtime-render harness and numeric source audit assert that these slots remain unavailable/empty.

These changes document existing behavior. Gameplay mechanics, registrations,
world templates, and casting availability were not changed.

## Validation

- Sanitized command/renderer checks compile the production `spello()` initializer,
  309 literal registration calls through ID 350, and applicable literal setters;
  the Plane Shift campaign guard remains intact.
- All 324 defined numeric IDs render without mutating player, config, or
  registration state. All 26 unused slots return no rendered entry and are not
  public, also without state changes.
- Checks cover every new entry, unavailable/internal status, duration, formula,
  target behavior, source/recipient distinctions, summon scaling, colors/alignment,
  global AC cap, and immediate versus periodic damage modifiers.
- Catalog audit confirms 330 distinct records with no duplicate symbols.
- Full CMake build passes.

Regenerate this batch's numeric source index with:

```sh
python3 tools/audit_spellhelp.py --range 301 350 --json
```
