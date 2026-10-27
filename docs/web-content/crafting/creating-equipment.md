# Creating Equipment

Create weapons, armor, jewelry, instruments and miscellaneous equipment with your own descriptions and material choices. Start with a plain item, then try magical improvements.

## Choose and describe an item


Begin by choosing the item category, specific type and variant:

```
craft itemtype <weapon|armor|jewelry|instrument|misc>
craft specifictype <type>
craft variant <variant name>
```

The variant determines the recipe, material groups, quantities and crafting skill. Use `craft show` to review those requirements.

Set the text other players will see:

```
craft keywords <targeting keywords>
craft shortdesc <short description>
craft roomdesc <description when the item is on the ground>
craft extradesc <extra description>
```

Assign suitable materials from your balances:

```
craft materials add <material name>
craft materials remove <material name>
```

Start with a plain version of the item. You can explore magical bonuses after you understand its material and skill requirements.

Example: a plain iron broad sword

```
craft itemtype weapon
craft specifictype long sword
craft variant broad sword
craft keywords iron broad sword
craft shortdesc an iron broad sword
craft roomdesc An iron broad sword lies here.
craft materials add iron
craft materials add low grade hide
craft show
craft check
craft start
```

This is an example recipe, not a grant of materials or skill. You need enough iron and hide, the appropriate tool and station, and the ability to attempt the project. If `craft check` reports a problem, resolve it before starting.

## Add enhancements, bonuses and motes

Once you can make plain equipment, add improvements that suit your character.

```
craft enhancement <amount>
craft bonuses <slot> <bonus location> <bonus type> <modifier> [specific]
craft motes add enhancement
craft motes add <bonus slot number>
craft show
craft check
```

For example:

```
craft enhancement 2
craft bonuses 1 strength universal 2
craft motes add enhancement
craft motes add 1
```

These choices require the relevant materials, motes and crafting ability. Higher bonuses can make a project harder and increase the finished item's required level. Your project display shows its actual requirements.

- Use `applies` to see bonus locations. The supported bonus types are enhancement and universal.
- A specific selection is required for bonuses such as a particular skill or spell circle. Follow the command's help for those choices.
- Adding motes to a numbered slot assigns the required mote type and quantity for that bonus.
- Use `craft motes remove <slot>` or `craft motes remove enhancement` to reclaim an allocation before crafting.
- Instrument projects have their own quality, effectiveness and breakability settings through `craft instrument`.
- Crafting does not support granting arbitrary feats to an item.

You can adjust the finished item's level with:

```
craft leveladjust <adjustment>
```

Reducing the final level by one increases the crafting difficulty by five. Increasing the final level by one reduces the difficulty by five. Review `craft show` and `craft check` after making changes.

More information:
https://krynn.d20mud.com/wiki/creating-equipment

## Optional catalysts and disenchanting

Crafting catalysts can give a successful item craft a small chance of an extra improvement. They are optional; they do not replace the required materials or motes.

```
disenchant <item>
refine add catalyst
refine show
refine begin
craft catalysts 1
craft show
```

Disenchanting permanently destroys an eligible magical item from your inventory and gives catalyst fragments. It requires an alchemy laboratory. Restricted items cannot be disenchanted, including legendary equipment.

The catalyst recipe uses 10 catalyst fragments to make one crafting catalyst. Allocate zero to five catalysts to an item project: each allocated catalyst adds one percentage point to its critical-success chance, up to 5%. They do not guarantee success or an extra improvement.

```
craft catalysts add <number>
craft catalysts remove [number]
```

Set these before crafting begins. Crafting catalysts and their fragments are separate from the boss-earned materials used by Dragonmetal Awakening.


## Change a project

Use `craft reset` to clear an item project's setup. You can also reset individual parts, such as `craft reset bonuses` or `craft reset descriptions`. Read the command output before rebuilding your project.

Use `cancel craft` to stop active work. Do not assume that stopping work recovers ingredients already consumed.

## Continue learning

- [Harvesting Materials](https://krynn.d20mud.com/wiki/harvesting-crafting-materials)
- [Crafting Motes](https://krynn.d20mud.com/wiki/crafting-motes)
- [Refining Materials](https://krynn.d20mud.com/wiki/refining-materials)
- [Crafting Tutorial](https://krynn.d20mud.com/wiki/crafting-tutorial)
