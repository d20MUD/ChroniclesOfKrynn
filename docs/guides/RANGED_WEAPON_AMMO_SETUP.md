# Setting up and using ranged weapons and ammunition

Based on the current code as of November 7, 2026. This guide covers a builder-created longbow, a 20-arrow bundle, and a wearable quiver, followed by the player commands to use them.

## How the ammunition system works

A working setup needs three objects:

1. A normal **Weapon** with a ranged weapon subtype, such as longbow.
2. An **Ammo Pouch**, which can be described as a quiver, worn in the ammo-pouch slot.
3. Matching **Missile/ammo** objects inside that worn pouch.

Carrying arrows in your inventory is insufficient. The weapon checks the **first ammunition object in the worn pouch**. If that object is the wrong ammunition type, firing fails even if matching ammunition is elsewhere inside it. Use one ammunition family per pouch for an uncomplicated setup.

| Weapon | Required ammunition type |
| --- | --- |
| Bow | 1 — arrows |
| Crossbow | 2 — bolts |
| Sling | 3 — bullets |
| Throwable weapon | 5 — thrown weapon, with the exact matching weapon subtype |

Type 4 remains available for legacy dart ammunition.

An ammunition object can represent a whole bundle. Each committed shot spends one unit, including misses and deflected shots. The bundle remains in the pouch until its final unit is fired. The next matching bundle then supplies subsequent attacks. Attacks rejected before a shot is committed do not spend ammunition.

## 1. Create the sample longbow

The following vnums are **examples, not reserved numbers**. First verify that they are unused and belong to your assigned building area; substitute your own if necessary. Perform the setup on the dev port.

| Example vnum | Object | Keywords |
| --- | --- | --- |
| 70480 | A sample longbow | `samplebow bow longbow` |
| 70481 | A bundle of sample arrows | `samplearrows arrows bundle` |
| 70482 | A sample leather quiver | `samplequiver quiver ammopouch` |

Enter:

```text
oedit 70480
```

Set **Type** to **Weapon** (item type 5). In **C) Values**, select **long bow** (weapon subtype 43), then set its enhancement to **0**.

Select the subtype before customizing other fields: choosing a weapon subtype supplies its damage, weight, material, size, price, and Take/Wield flags, and can replace earlier customizations. The longbow subtype supplies **1d8 base damage** and a **100-foot range increment**.

Set or verify:

| Field | Sample setting |
| --- | --- |
| Keywords/name | `samplebow bow longbow` |
| Short description | `a sample longbow` |
| Room description | `A sample longbow lies here.` |
| Minimum level | 1 |
| Wear flags | Take and Wield |
| Enhancement | 0 |

Keep the subtype's generated size, material, weight, and damage for this example. Use the normal **Weapon** type; the separate legacy **Fire Weapon** type is not the setup used here.

Quit the editor with `Q` and answer `Y` to save.

## 2. Create the wearable quiver

Enter:

```text
oedit 70482
```

Set:

| Field | Sample setting |
| --- | --- |
| Type | Ammo Pouch (item type 36) |
| Keywords/name | `samplequiver quiver ammopouch` |
| Short description | `a sample leather quiver` |
| Room description | `A sample leather quiver lies here.` |
| Wear flags | Take and Ammo-Pouch |
| Weight | 1 |
| Minimum level | 1 |

In **C) Values**, use a capacity of **100**, no container flags, and key vnum **-1**. Leave other fields at their defaults. Keep the pouch open for the test.

The capacity prompt mentions weight, but capacity also limits the number of ammunition objects inside. A 20-arrow bundle occupies one object slot. Use a positive capacity rather than relying on the prompt's `-1` unlimited wording.

A normal Container named “quiver” will not work. The object must actually be an **Ammo Pouch** and have its matching wear flag. A character cannot wear an ammo pouch and a weapon sheath at the same time.

Quit and save.

## 3. Create a bundle of 20 arrows

Enter:

```text
oedit 70481
```

Set:

| Field | Sample setting |
| --- | --- |
| Type | Missile/ammo (item type 14) |
| Keywords/name | `samplearrows arrows bundle` |
| Short description | `a bundle of sample arrows` |
| Room description | `A bundle of sample arrows lies here.` |
| Wear flags | Take |
| Weight | 1 |
| Minimum level | 1 |
| Cost | 20, as an example price for the entire bundle |

In **C) Values**, answer the prompts in this order:

| Prompt | Enter | Meaning |
| --- | --- | --- |
| Ammunition type | `1` | Arrows for the bow |
| Breaking probability | `30` | Breakage chance for a missed projectile; unrelated to quantity consumed |
| Enhancement | `0` | Mundane ammunition |
| Minimum quantity on load | `20` | Minimum arrows in a fresh copy |
| Maximum quantity on load | `20` | Maximum arrows in a fresh copy |
| Current quantity | `20` | Quantity in the edited object |
| Bonus damage type | `0` | No additional damage type |
| Bonus damage die size | `0` | No bonus damage die |
| Unlimited ammunition | `0` | Finite supply |

Every freshly loaded copy now contains exactly **20 arrows**. Using different minimum and maximum quantities randomizes each newly loaded supply within those bounds.

Set a positive maximum to enable bundle behavior. Legacy ammunition with a maximum of zero acts as a single projectile. A finite bundle with a positive maximum and current quantity zero is empty.

The normal arrow editor does not ask for an imbued spell or the reserved legacy field. Leave both at zero; do not insert extra answers for them into the prompt sequence.

Quit, confirm the save, and persist the edited objects:

```text
saveall
```

Load fresh copies after editing prototypes; do not assume objects already in a character's inventory acquire the new settings.

## 4. Load and equip the sample kit

As staff, load one of each object:

```text
load obj 70480 1
load obj 70481 1
load obj 70482 1
```

Depending on server configuration, loaded objects appear in your inventory or the room. If they appear in the room, pick them up:

```text
get samplebow
get samplearrows
get samplequiver
```

If another character will test the kit, give them all three objects and have that character perform the following steps. Use a character that can wield the bow and meets its normal equipment requirements.

Remove other wielded weapons and any worn weapon sheath as needed, then:

```text
wield samplebow
wear samplequiver
put samplearrows samplequiver
equipment
look in samplequiver
```

Confirm that the bow is wielded, the quiver occupies the ammo-pouch slot, and the arrows are inside it. Spare matching bundles can be placed inside before battle. Avoid mixing arrows, bolts, and bullets in the same pouch.

## 5. Fight with the bow

Use a valid enemy in the **same room**, in an area where combat is allowed. Replace `goblin` with that enemy's actual keyword:

```text
fire goblin
```

This begins ranged combat. Subsequent attacks run through the combat rounds automatically; you do not need to enter `fire` repeatedly. The command rejects attempts to start firing while already marked as fighting/firing. Use `fire`, rather than `hit`, to initiate an attack with this bow.

A bow does **not** need `reload`. The arrows come directly from the worn pouch. You can shoot a nearby enemy in the same room; shooting does not require an adjacent room or a direction.

This does not work:

```text
fire goblin north
```

Shooting through exits is unsupported, including with Far Shot or Longshot.

### Distance during the fight

When a fight begins with either opponent wielding a ranged weapon, their initial separation uses the room's effective size. Most NPCs advance each round until they reach 5 feet, where they can make ordinary attacks. Some enemies can attack while farther away.

For a controlled builder test, use a non-peaceful test room and set **H) Room size** in `redit` to **60 feet**. That starts within the sample longbow's first 100-foot range increment, allowing you to see an enemy close the distance without an initial range penalty.

Weapon range still matters: additional increments penalize accuracy, and projectile weapons have a maximum of ten increments. Changing weapons during a fight does not reset your separation. If you switch to a melee weapon while the enemy is distant, use `advance` or `advance goblin` to close the gap; this uses a move action.

### Consumption and resupply

Without extra attacks or Manyshot, 20 arrows support 20 committed shots. Misses still consume arrows. Rapid Shot can use more arrows per round. Manyshot can spend two arrows on the first eligible bow attack of a full attack; with only one arrow left, that attack fires normally.

When the final arrow is fired, its bundle disappears from the pouch. Another matching bundle can supply the next shot. If no usable ammunition remains, the ranged combat loop stops firing. Resupply before battle or once combat ends; when no longer marked as fighting, start again with `fire <target>`.

To recover surviving ammunition after battle:

```text
collect
```

Your worn pouch needs room. Broken ammunition cannot be recovered. Recovered arrows are separate ammunition objects: collecting them does not refill an existing bundle. `autocollect` toggles automatic collection.

## 6. Optional: make +2 flaming arrows

Create a separate Missile/ammo prototype with distinct keywords such as `samplefirearrows`, using the same object fields as the mundane bundle. In **C) Values**, enter:

| Prompt | Enter |
| --- | --- |
| Ammunition type | `1` |
| Breaking probability | `30` |
| Enhancement | `2` |
| Minimum quantity | `20` |
| Maximum quantity | `20` |
| Current quantity | `20` |
| Bonus damage type | `1` — Fire |
| Bonus damage die size | `6` — +1d6 |
| Unlimited | `0` |

Save and load a fresh copy. Remove other ammunition from the quiver before putting these arrows inside so the intended bundle is first.

With the mundane sample bow, these arrows provide a **+2 enhancement to attack and weapon damage**, plus **1d6 fire damage on a successful hit**. Normal character modifiers and enemy defenses still apply. Weapon and ammunition enhancement use the **higher bonus**, rather than adding together: a +1 bow with +2 arrows uses +2. Fire damage is resolved separately against resistance and is not multiplied by a critical hit.

Other supported damage choices include Cold (2), Acid (5), Electricity (7), and Force (28). Choose by the editor's displayed names. The die-size field describes one die: `6` means 1d6, not six dice.

Setting ammunition's **Unlimited** field to `1` creates an inexhaustible supply. The worn pouch and correct ammunition type remain required. Projectiles from that supply disappear after resolution and cannot be farmed into permanent ammunition.

## 7. Crossbows, slings, and thrown weapons

For a crossbow, choose its subtype on a normal Weapon and use bolt ammunition (type 2). For a sling, use its subtype and bullet ammunition (type 3). Both require reloading in addition to ammunition in the pouch:

```text
wield crossbow
wear quiver
put bolts quiver
reload
fire goblin
```

These are illustrative keywords; substitute the names on your actual objects. Use `reload` whenever the weapon needs reloading. `autoreload` toggles automatic reloading during combat. Rapid Reload reduces the relevant action cost; it does not create ammunition. Unlimited ammunition also does not remove reload requirements.

For throwing, wield a throwable normal Weapon such as a dagger. Create Missile/ammo with type **5**, then finish its additional editor prompts by choosing the **exact compatible weapon subtype**, such as dagger, and its Returning setting. Put that supply in the worn pouch and use:

```text
throw goblin
```

The wielded weapon remains equipped; each throw consumes the matching ammunition supply. Without Quick Draw, throwing is capped at one attack per round. `throw off`, or `hit` with a melee-capable throwing weapon, returns to melee use.

Returning on either the throwing weapon or its compatible ammunition makes the supply inexhaustible. Matching ammunition in the worn pouch is still required. Returning has no return delay and does not grant Quick Draw.

## Troubleshooting

| Problem | Check |
| --- | --- |
| Weapon is not recognized as ranged | Use normal Weapon type and a genuine bow/crossbow/sling subtype; its description alone is insufficient. |
| Quiver will not supply ammunition | It must be Ammo Pouch type, have the Ammo-Pouch wear flag, and actually be worn. |
| Quiver will not equip | Remove the worn weapon sheath and check normal equipment restrictions. |
| Correct arrows exist but firing fails | Inspect the first ammo object in the worn pouch; remove wrong-type or empty supplies. Inventory ammunition is not enough. |
| A new bundle has only one shot | Set positive minimum, maximum, and current quantities; then save and load a fresh copy. |
| Crossbow or sling will not fire | Put matching ammo in the pouch and use `reload`. |
| Cannot shoot the target | Target must be visible, in the same room, within weapon range, and a legal combat target. Check peaceful-room restrictions. |
| “Too busy fighting” when using `fire` again | Firing already runs through combat rounds; the command does not manually add a shot while fighting. |
| Ammo disappears faster than expected | Count misses, deflections, additional attacks, Rapid Shot, and Manyshot. |
| Prototype changes seem ineffective | Save the prototype and load fresh objects for testing. |

## Builder reference: raw ammunition values

The main editor's object-value display uses zero-based positions. This is a reference for checking your setup, not an additional sequence of editor prompts.

| Value | Meaning | Sample mundane arrows |
| --- | --- | --- |
| 0 | Ammunition type | 1 |
| 1 | Existing imbued spell ID | 0 |
| 2 | Breakage probability | 30 |
| 3 | Reserved legacy field | 0 |
| 4 | Enhancement | 0 |
| 5 | Minimum fresh quantity | 20 |
| 6 | Maximum fresh quantity | 20 |
| 7 | Current quantity | 20 |
| 8 | Bonus damage type | 0 |
| 9 | Bonus damage die size | 0 |
| 10 | Unlimited | 0 |
| 11 | Exact compatible thrown weapon subtype | 0; unused for arrows |
| 12 | Returning ammunition | 0 |

Leave remaining unused values at zero. For the flaming-arrow variant, change value 4 to 2, value 8 to 1, and value 9 to 6.

## Implementation references

- [Ranged system mechanics](../systems/ranged-combat.md)
- [Builder ranged help](../../lib/text/help/build_ranged.hlp)
- [Player ranged help](../../lib/text/help/ranged_combat.hlp)
- [Object editor and value prompts](../../src/oedit.c)
- [Equipment and ammo pouches](../../src/act.item.c)
- [Firing, throwing, reloading, and collection commands](../../src/act.offensive.c)
- [Combat rounds and ammunition resolution](../../src/fight.c)
- [Object and weapon type constants](../../src/structs.h)
