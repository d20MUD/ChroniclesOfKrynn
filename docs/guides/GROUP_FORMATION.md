# Group formation

Group members can position themselves in three rows. Use `formation` to see the
rows, or `group` to see each member's row alongside their status. The same
commands are available with the `group formation` prefix.

```
formation
formation front
formation middle
formation back
formation middle companion
formation back archer
```

You can change your own row. The group leader can also position other members,
including companions and mounts, who are in the same room. Members must be awake
to change rows. During combat, changing rows costs the moving member a move
action; the move fails if that action is unavailable. Changing to the current row
costs nothing.

| Row | Allowed physical attacks |
| --- | --- |
| Front | Melee, reach and ranged weapons; unarmed and natural attacks |
| Middle | Reach weapons and ranged attacks |
| Back | Ranged attacks |

Front-row ranged attacks take a -4 attack penalty without Point Blank Shot and
another -4 without Precise Shot. Improved Precise Shot also removes the Precise
Shot penalty. These penalties stack: lacking both feats gives -8. They apply to
regular, staggered and special ranged attacks, including thrown bombs. Front-row
members can fire while being attacked in melee; missing these feats penalizes
the attack instead of preventing it. Ordinary feat bonuses still apply.

Reach comes from the weapon used for that specific attack. A reach weapon in one
hand does not permit attacks with an ordinary weapon in the other hand. Natural
attacks do not inherit reach from an equipped weapon. Ranged attacks include
thrown bombs. Contact actions such as kick, headbutt, shield punch and slam
require the front row. Weapon maneuvers such as trip, backstab, disarm and sunder
use the same row restrictions as their weapon.

Spells, psionic attacks and eldritch blasts retain their normal targeting rules.
Formation does not screen allies from ranged attacks, spells or area effects.

## NPC melee reach

Ordinary NPCs can attack only the nearest occupied row of an opposing group.
Large NPCs can reach two rows. Huge, gargantuan and colossal NPCs can reach all
three. Current size is used, including size changes from effects.

Only living, awake group members in the same room screen allies. When the front
row is empty, sleeping, unconscious or absent, the middle row becomes exposed
and protects the back row from ordinary NPC melee attacks. With only the back
row present, it becomes exposed. A large NPC can reach the back row when the
front row is absent.

If an NPC's melee target is protected, its normal attack switches to an
accessible, visible member of that group. It cannot attack through a protected
row if no accessible target is visible. An opportunity attack aimed at a
protected character fails rather than switching to a different target. NPC
contact maneuvers also respect screening. Players' own allowed melee attacks
retain their existing target rules.

Companions participate as ordinary group members. Newly joined members start in
the front row. Positions reset when leaving a group and are not saved across
logins. Ungrouped characters retain their existing combat behavior.

The MSDP group table includes a `FORMATION` field (`Front`, `Middle`, or `Back`).

Validation: `python3 tests/run_formation.py` exercises the production rule and
command functions and combat hooks with isolated game stubs under AddressSanitizer
and UndefinedBehaviorSanitizer. It checks weapon/size matrices, absent or
unconscious front rows, retargeting, opportunity attacks, ammo checks, contact
maneuvers, leader permissions, move actions, and group join/leave behavior.
