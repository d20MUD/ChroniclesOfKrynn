# Spellhelp batch 351–400

This numeric batch contains **31 unused slots (351–381)** and **19 defined
entries (382–400)**. All 19 defined entries now have reviewed mechanics records;
all are new. These include three internal retaliation/breath effects and two
alchemist effect IDs whose symbols do not start with `SPELL_` or `PSIONIC_`.
The audit index and registration harness now include those two symbols explicitly.

The catalog now has **349 distinct mechanics records**: 321 entries with
public-style registrations (including campaign-dependent Plane Shift), fifteen
unregistered legacy/unfinished IDs, and thirteen internal abilities/effects.
In Dragonlance builds, 320 catalog entries are public. The audited range through
400 contains 343 defined IDs and 57 unused slots (325–381): 314 public entries,
fifteen unregistered IDs, thirteen internal IDs, and unavailable Plane Shift.
Full cross-cutting feat/perk/equipment interaction audits remain a separate phase.

| ID | Spellhelp name | Availability |
| ---: | --- | --- |
| 351–381 | No defined names or registrations | **Unused slots** |
| 382 | electric shield damage | **Internal effect; not player-castable** |
| 383 | psychokinetic tincture | Registered alchemist effect; applied through command |
| 384 | augury | Registered |
| 385 | psychokinetic fear | Registered alchemist effect; applied through command |
| 386 | mass false life | Registered |
| 387 | acid breathe | **Internal effect; not player-castable** |
| 388 | poison breathe | **Internal effect; not player-castable** |
| 389 | entangle | Registered |
| 390 | dragon fear | Registered ID; command path applies documented ability effects |
| 391 | cause fear | Registered |
| 392 | shadow walk | Registered |
| 393 | circle of death | Registered |
| 394 | undeath to death | Registered |
| 395 | grasp of the dead | Registered |
| 396 | grave touch | Registered ID; command path applies documented ability effects |
| 397 | incorporeal form (undead bloodline) | Registered ID; command path applies documented ability effects |
| 398 | lesser missile storm | Registered |
| 399 | shadow jump | Registered |
| 400 | unholy weapon | Registered |

## Findings reflected in descriptions

- **Electric Shield Damage (382):** automatic 1d6 electricity retaliation in the non-ranged shield checks, not acid despite the stale header comment. Status remains internal/unavailable, with no guaranteed preservation of the original casting-class damage bonus.
- **Psychokinetic Tincture (383):** `psychokinetic apply` discovery/use requirements, initial min(5, max(1, Alchemist level/4)) deflection AC/spirit count, 50-times-Alchemist-level duration, voluntary dispel, successful-launch count reduction, and removal at zero. The entry describes the recorded count without assuming that mutating it recalculates all cached stats immediately. The launch path currently does not require an active tincture before attempting fear; no gameplay guard was added in this documentation batch.
- **Augury (384):** reports possible current-room reset contents (creatures, objects, equipment/contents, treasure chances), rather than tabletop future-action weal/woe or a guarantee that those contents are currently present.
- **Psychokinetic Fear (385):** launch applies -2 morale Wisdom plus either Fear/immediate fleeing or Shaken according to the target's frightened preference; actual Alchemist-level duration, Will save, no resistance check, fear/mind immunities, and spirit consumption only on successful application. The raw registered effect name has no command dispatch.
- **Mass False Life (386):** ordinary False Life applied to group recipients (+30 maximum/current HP, circumstance type, 300 rounds), group requirement, and no invented separate absorption reserve. A separate Recipients label now clarifies group effects whose registration also selects a casting target.
- **Acid/Poison Breathe (387/388):** help-only internal generic source-level d16 formulas, Reflex half and no resistance in the damage handler, with alternate ability formulas acknowledged. Wind Wall blocks poison breath damage. The internal `SPELL_POISON_BREATHE` area path also dispatches affects but has no matching timed poison-effect case; it must not be confused with the separate `SPELL_POISON_BREATH` Constitution-debuff ability.
- **Entangle (389):** single-target vines, -2 Dexterity, entangled movement restrictions/half speed, Reflex save/resistance, Free Movement/Slip the Bonds protection, and actual 10+level duration.
- **Dragon Fear (390):** the command/callback path applies Shaken for 5d6 rounds and attempts fleeing twice, with Will/resistance/fear/mind checks. Its source level is Druid plus Shifter ability casting level. Dragon requirement, swift action, peaceful-room check, current Shaken/Frightened penalties, and lack of a matching standalone spell callback are distinguished.
- **Cause Fear (391):** ordinary single-target Fear, 2d6 rounds, Will/resistance/immunities, and one-in-three combat-cycle flee attempt. A separate fear-aura callback reuses the ID for Shaken/immediate fleeing and is not presented as the ordinary cast's area behavior.
- **Shadow Walk (392):** group +2 inherent Dexterity, min(10, level) rounds, actual base speed 400 before later modifiers, and minimum room movement cost 1 instead of 5. No immediate planar transport is claimed. The explicit Recipients line resolves the self-target/group-dispatch distinction.
- **Circle of Death (393):** capped negative-energy area damage, Fortitude half/resistance, undead healing through the negative-damage path, and absence of an instant-death or target-level cutoff.
- **Undeath to Death (394):** capped holy area damage, Fortitude half/resistance, and no undead-only filter in the damage/area cases; eligible living creatures can also be damaged.
- **Grasp of the Dead (395):** area slashing damage, Reflex half/resistance, and undead-bloodline command use of arcane casting level. No entanglement/paralysis/lingering condition is applied by this damage handler.
- **Grave Touch (396):** `gravemagic touch` applies Shaken and -1 enhancement AC, uses max(2, Sorcerer level/2) duration, checks Fortitude/fear immunity, and does not make a touch attack or resistance check in that command branch. Use consumption and group-target refusal are described; the registered effect does not itself invoke the command implementation.
- **Incorporeal Form (397):** `gravemagic incorporeal-form` grants three rounds of immaterial/incorporeal state and +1 enhancement AC. Shared non-spell damage halving can affect incoming and outgoing attacks; Ghost Touch/incorporeal/Blink exceptions and Wind Wall's approach restriction are reflected. Ordinary spell damage is not halved by that physical-damage check. The area-spell registration alone does not run the transformation command.
- **Lesser Missile Storm (398):** min(10, level) single-target 1d6 energy missiles, no save, per-hit resistance, and consumed casting-class bonus after the first damage event.
- **Shadow Jump (399):** moves the caster to the target's room, with source/target shadow predicates, travel/plane/no-teleport/Dimensional Lock restrictions and normal arrival handling. Shadowdancer shadowcasting uses its feat/use gates; no target/group teleport is claimed.
- **Unholy Weapon (400):** creates the Unholy Avenger template in inventory; `unholyweapon` chooses type/base dice/size, while other properties/lifetime follow the template. Does not enchant an existing item or apply unarmed perks.

These changes document existing behavior. Gameplay mechanics, registrations,
world templates, and casting availability were not changed.

## Validation

- Sanitized command/renderer checks compile the production `spello()` initializer,
  328 literal registration calls through ID 400, and applicable literal setters,
  including both nonstandard alchemist symbols. The Plane Shift guard remains.
- All 343 defined IDs through 400 render without mutating player, config, or
  registration state. All 57 unused slots (325–381) return no entry and are not public.
- Checks cover every new entry, internal availability, command versus spell paths,
  conditions/damage/durations, group recipients, colors/alignment, AC cap, global
  damage bonus hiding/application, and repeated-hit behavior.
- Source audit confirms 349 distinct records with no duplicate symbols.
- Full CMake build passes.

Regenerate this batch's numeric source index with:

```sh
python3 tools/audit_spellhelp.py --range 351 400 --json
```
