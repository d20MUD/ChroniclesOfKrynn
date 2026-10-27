# Crafting Motes

Motes are elemental resources used to add magical improvements to crafted equipment. You can uncover them while harvesting materials, find them in random treasure, and buy them for artisan points at artisan shop services.

## Check your motes

```
materials
motes
motes fire
```

`materials` shows your stored quantities, including motes. `motes` lists the available mote types. Add a type, such as `motes fire`, to see its associated enhancements and bonuses.

## Choose a bonus and assign its motes

Configure your item first, then select the improvement:

```
craft bonuses 1 strength universal 2
craft motes add 1
craft show
craft check
```

The game assigns the required type and quantity to that slot. You need enough motes in your balances. For a weapon or armor enhancement, use:

```
craft enhancement 2
craft motes add enhancement
```

Instrument projects use `craft instrument quality`, `craft instrument effectiveness` and `craft instrument breakability` to set their improvements. Allocate the corresponding motes with `craft motes add quality`, `craft motes add effectiveness` or `craft motes add breakability`.

## Mote families

This table lists common uses. `motes <type>` gives the fuller in-game reference, including skill bonuses and equipment enhancements. A listed association does not mean every bonus type can be selected by the ordinary equipment-crafting commands.

| Mote | Common bonus associations |
|---|---|
| Air | Charisma, fast healing, air/force/illusion/energy resistance, social skills |
| Dark | Damage bonus, spell potency, unholy/disease/negative resistance |
| Earth | Constitution, movement, Fortitude saves, movement regeneration, earth/acid resistance |
| Fire | Strength, hit points, hit-point regeneration, spell penetration, fire/slashing resistance |
| Ice | Intelligence, psionic power points and regeneration, cold resistance, power resistance, bonus spell slots, knowledge skills |
| Light | Attack bonus, spell resistance, spell difficulty, holy/mental/light resistance |
| Lightning | Dexterity, Reflex saves, initiative, spell duration, electric/sound resistance, agility skills |
| Water | Wisdom, Will saves, carrying capacity, piercing/poison/water resistance, perception-related skills |

Weapon and armor enhancements have their own associations. Use the game's mote reference and your configured project's `craft show` display to see the requirement for the exact equipment you are making. Armor enhancement uses water motes.

## How many motes are required?

Each bonus is priced separately according to the level adjustment of that bonus. The enhancement or universal bonus type affects that calculation. Better materials can reduce the final item's level and difficulty, but do not reduce the mote cost of its bonuses.

| Level adjustment of an individual bonus | Motes required |
|---|---|
| Below 5 | 3 |
| 5–9 | 8 |
| 10–14 | 15 |
| 15–19 | 25 |
| 20–24 | 40 |
| 25–29 | 60 |
| 30 or more | 80 |

Use `craft show` for the actual requirements of your configured project. You do not need to calculate or enter the mote quantity yourself.

## Reclaim an allocation

Before starting the craft, remove a particular allocation with:

```
craft motes remove 1
craft motes remove enhancement
```

Change the bonus if necessary, assign its motes again and review `craft show`. Stopping an active craft does not promise a refund of resources already consumed.

Motes are separate from crafting catalysts. Catalysts offer a small optional chance of an extra improvement after a successful craft; they do not replace required motes.

[Creating Equipment](https://krynn.d20mud.com/wiki/creating-equipment) · [Harvesting Materials](https://krynn.d20mud.com/wiki/harvesting-crafting-materials) · [Crafting Guide](https://krynn.d20mud.com/wiki/crafting)
