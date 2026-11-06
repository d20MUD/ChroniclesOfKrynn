# Ranged combat and ammunition

Ranged weapons and offensive spells target characters in the same room.
`fire <target> <direction>` is rejected, including with Far Shot or Longshot.
Eldritch Spear and Ice Rabbit also use same-room targets.
Snipe remains deferred. Thrown weapons use matching ammunition as described below.

Point-Blank Shot grants +1 ranged attack and damage within 30 feet. Missing it
never causes a front/middle-row or tanking penalty. Precise Shot retains this
MUD's -4 penalty rule for shooting a mob tanked by another player; pets and other
NPC tanks do not trigger that penalty. Improved Precise Shot also removes that
penalty and ignores concealment below 50%, preserving total concealment.

## Builder values for ITEM_MISSILE

Ammo remains inside an equipped ammo pouch. The first ammo item must match the
weapon: arrows for bows, bolts for crossbows, bullets for slings. The old stone
ammo ID is retained as the bullet ID; legacy dart items remain readable.

| Value | Meaning |
| --- | --- |
| 0 | Ammo type: 1 arrow, 2 bolt, 3 bullet, 4 legacy dart, 5 thrown weapon |
| 1 | Existing imbued spell ID |
| 2 | Existing projectile breakage chance |
| 3 | Reserved legacy field |
| 4 | Enhancement bonus, added to attack and weapon damage |
| 5 | Minimum quantity when a new object loads |
| 6 | Maximum quantity when a new object loads |
| 7 | Current quantity; saved with the instance |
| 8 | Bonus damage type, using DAM_* IDs; 0 disables |
| 9 | Bonus damage die size: 6 means +1d6; 8 means +1d8 |
| 10 | Unlimited ammo: 1 enabled, 0 disabled |
| 11 | Exact compatible weapon type for thrown ammo |
| 12 | Returning thrown ammo: 1 behaves like unlimited ammo |

The normal object editor exposes these values after enhancement. For example,
set type 1, enhancement 2, minimum 20, maximum 40, damage type 1 (fire),
and die size 6 for +2 arrows with +1d6 fire. New copies load with 20-40 arrows.
Set the current quantity as well when editing an instance for immediate use.

A zero maximum preserves legacy ammo as a single projectile. New supplies
consume one unit per committed attack, including misses and deflections. The
final unit uses the supply object as its projectile. Earlier shots receive
separate objects and retain enhancement, elemental damage, and poison. Fired
projectiles never become new stacks or unlimited supplies. Unlimited supplies
remain equipped, and their projectiles are removed after attack resolution.

Bonus damage uses its own damage type and resistance handling. It is applied
once on a successful weapon hit and is not multiplied by critical hits. Spell
and ability attack rolls do not consume weapon ammo.

All object values already participate in object/prototype persistence; no new
object-file format is required. The SQL help migration is supplied separately.

## Validation

* `python3 tests/run_formation.py`
* `python3 tests/run_ranged_ammo.py`
* `python3 tests/run_ranged_bombs.py`
* `python3 tests/run_ranged_resolution.py`
* `cmake --build build-agent-check-on -j4`

## Combat distance and advancing

On engagement, each opponent pair starts at the effective room size if either
combatant wields a ranged weapon, otherwise at zero. Distance is symmetric and
stored separately per pair. Switching weapons does not reset an engaged pair.
Room departure/extraction clears the character's pairs. Ending one side of a
fight preserves distance while the other side is still attacking that character.

PC melee attacks require 5 feet. Weapons with `WEAPON_FLAG_REACH`, including
polearms such as halberds and glaives, extend this to 10 feet. This uses the weapon
actually making the attack: unarmed attacks and natural attacks cannot borrow
reach from another equipped weapon. Existing formation restrictions also apply.

`advance [opponent]` moves toward a mob involved in the current fight, defaulting
to the current opponent. Each successful use consumes one move action and closes
by at most `get_speed(ch, FALSE)`, stopping at 5 feet or 10 feet with a reach
weapon. Failed attempts and attempts when already in range consume no action.
The existing staff level-setting command remains available outside combat.

All NPCs, including charmies and ranged mobs, auto-advance once per round at
phase 1, before summon-specific combat actions and ordinary NPC AI. They stop
within 5 feet. Immobilization and zero speed prevent this movement. Ordinary
NPC attacks, attack rolls, and targeted offensive spell casting require 5 feet.

`MOB_RANGED_ATTACK` (flag 108, `Ranged-Attack` in medit) permits a mob to attack
at any distance within the same room, including natural attacks without a ranged
weapon. It does not suppress automatic advancement or waive ammunition checks
for an equipped ranged weapon. NPCs use a 5-foot closing distance even with a
reach weapon.

The distance API is in `fight.h`: `combat_distance` returns feet (`-1` for targets
in different/invalid rooms), `combat_attack_in_range` checks attack eligibility,
and `advance_combat_distance` moves toward one opponent. Active pairs are runtime
data and are not saved to character or mob files.

Validation: `python3 tests/run_combat_distance.py` exercises start distances,
independent opponents, NPC/charmie movement, ranged flags, reach and natural
attacks, action consumption, immobilization, cleanup, and staff access.


## Thrown weapons and Pathfinder 1e feat pass

`throw <target>` enables throwing with the wielded main weapon. It must have
`WEAPON_FLAG_THROWN`; the equipped item remains in place. Missile ammo type 5
must name the exact compatible weapon type in value 11. `throw off` or `hit`
returns a melee-capable weapon to melee use. Without Quick Draw, throw attacks
are capped at one per round. Quick Draw enables the normal full attack rate.

Returning ammo (value 12) or `WEAPON_SPECAB_RETURNING` on the wielded weapon
makes the supply inexhaustible, just like unlimited ammo. There are no in-flight
counters, return delays, or Returning-specific volley limits. A matching supply
in the worn pouch is still required. Emitted projectiles have Returning and
unlimited status cleared and are removed after resolution when sourced from an
inexhaustible supply. Quantity and mode do not replace the actual wielded weapon.

Thrown attacks use Dexterity for accuracy and full Strength for damage. The
wielded weapon's focus/specialization and properties remain applicable. Weapon
and ammo enhancement use the higher value, not their sum. Standalone ranged
touch rolls do not borrow an equipped weapon's ammo, enhancement, or range.

Physical attacks use each weapon subtype's range increment, with -2 for each
additional increment (-1 with Far Shot). The first increment is inclusive;
five increments is the thrown maximum, ten the projectile maximum. Room size
and same-room targeting still apply. Weapon range does not limit spell rays.

Rapid Shot adds one highest-bonus shot at -2 throughout a full weapon attack;
it requires Quick Draw for thrown volleys. Manyshot no longer adds a generic
Rapid Shot attack: the first full bow attack commits two arrows and uses one
attack roll, with no repeated critical or sneak damage. Each arrow resolves its
own DR and elemental damage. A single remaining arrow fires normally. The
ranger perk's active volley remains a separate class ability.

Deadly Aim is a toggle: -1/+2 attack/damage, increasing by another -1/+2 at BAB
+4 and every +4 thereafter. It does not use the shared numeric Power Attack
value. Its damage applies to ranged weapons, not spells or touch attacks.
Rapid Reload retains existing action costs (hand/light crossbow free with the
feat; heavy crossbow move with the feat), including this MUD's broader support
for slings and repeating crossbows.

Feat prerequisites were corrected to PF1 values: Precise Shot requires PBS;
Manyshot requires Dex 17, PBS, Rapid Shot, BAB 6; Improved Precise Shot requires
Dex 19, PBS, Precise Shot, BAB 11; Quick Draw requires BAB 1; Deadly Aim requires
Dex 13 and BAB 1; Far Shot requires PBS. Existing selections are retained.
No new feat IDs or player feat-array format changes are required.

Reference scope: the Pathfinder **first-edition** portion of the
[AshenVault guide](https://ashenvault.com/best-ranged-feats-pathfinder/), checked
against d20PFSRD's [Point-Blank Shot](https://www.d20pfsrd.com/feats/combat-feats/point-blank-shot-combat/),
[Quick Draw](https://www.d20pfsrd.com/feats/combat-feats/quick-draw-combat/),
[Rapid Shot](https://www.d20pfsrd.com/feats/combat-feats/rapid-shot-combat/),
[Manyshot](https://www.d20pfsrd.com/feats/combat-feats/manyshot-combat/),
[Deadly Aim](https://www.d20pfsrd.com/feats/combat-feats/deadly-aim-combat/),
[Far Shot](https://www.d20pfsrd.com/feats/combat-feats/far-shot-combat/),
[Precise Shot](https://www.d20pfsrd.com/feats/combat-feats/precise-shot-combat/),
[Improved Precise Shot](https://www.d20pfsrd.com/feats/combat-feats/improved-precise-shot-combat/),
and [Rapid Reload](https://www.d20pfsrd.com/feats/combat-feats/rapid-reload-combat/).
Shot on the Run, Parting Shot, Snap Shot, and Clustered Shots are outside this
pass. Cover geometry is not implemented; Improved Precise Shot currently acts
on the existing concealment system.

Validation adds `python3 tests/run_ranged_feats.py` for the 30-foot PBS boundary,
Deadly Aim scaling, Far Shot, concealment, thrown volleys/Quick Draw/Rapid Shot,
Manyshot ammo and secondary damage, and reload action costs.
