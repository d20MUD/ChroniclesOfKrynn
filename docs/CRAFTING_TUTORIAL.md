# Crafting Tutorial

Start with one plain item, then add enhancements and custom bonuses as your skills and materials improve. The usual loop is survey, harvest, refine when needed, choose a recipe, assign materials, check the project, and craft.

The crafting hub on our website is:
https://krynn.d20mud.com/wiki/crafting

The web version of this tutorial is:
https://krynn.d20mud.com/wiki/crafting-tutorial

## 1) Check your skills, tools and materials

```
craft score
craft tools
craft equipment
materials
motes
```

- `craft score` shows crafting and harvesting ranks, progress and specializations.
- `craft tools` and `craft equipment` show your equipped crafting gear and tools.
- `materials` shows your stored crafting materials; `motes` shows your motes.
- Harvesting needs the proper tool equipped. Refining and item creation may require the appropriate station in your room.
- Read the choices shown by a command when you leave out its selection. You do not need to memorize every recipe, material or bonus name.

## 2) Survey and harvest

Find a suitable outdoor area, then use:

```
survey
harvest
```

Let surveying finish before harvesting. A successful survey gives you survey rooms, which are used up as you travel. Survey again when they run out.

Harvesting requires survey rooms, a resource in the current room, and the correct equipped tool. If the room is empty, keep exploring. If your tool is wrong, the game tells you what you need. Allow each harvesting task to finish before starting another.

Materials go into your stored crafting-material balances. Harvesting can also find motes used for magical improvements.

Regional varieties of wood, hides and other materials have distinct names. Use the exact material name shown in your balances and recipe choices. A regional variety retains its underlying material group and grade; its different name does not automatically make it stronger.

More information:
https://krynn.d20mud.com/wiki/harvesting-crafting-materials
https://krynn.d20mud.com/wiki/crafting-motes

## 3) Refine raw materials

Type `refine` to see recipes and their ingredients. Refining needs the appropriate station, such as a smelter, loom or alchemy laboratory.

Prepare, review and start a project:

```
refine add <result material> [batch count|all]
refine show
refine begin
```

For example, `refine add steel 5` prepares five batches if you have the required ingredients. A batch produces the quantity listed in that recipe; five batches do not necessarily mean five units.

You can also prepare and start in one command:

```
refine batch <result material> [batch count|all]
refine smelt <result material> [batch count|all]
```

`refine batch` works with refining recipes generally; `refine smelt` selects smelter recipes. The maximum is 20 batches per project. The `all` option selects the available amount within that limit.

Use `refine remove` to clear a prepared refining project and return its allocated ingredients. For an active task, use `cancel craft` and read the result before preparing another project.

More information:
https://krynn.d20mud.com/wiki/refining-materials

## 4) Create your first item

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

## 5) Add enhancements, bonuses and motes

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

## 6) Optional catalysts and disenchanting

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

## 7) Resize an item

Keep the item in your inventory and select its new size:

```
resize <item keyword> <new size>
resize show
resize check
resize begin
```

For example, `resize sword small` selects a carried sword for resizing. The item must be eligible for transformation; legendary equipment cannot be resized.

Use `resize reset` to clear the selected project. There are no `resize add` or `resize remove` steps. Keep the item with you until the task finishes.

More information:
https://krynn.d20mud.com/wiki/resizing-gear

## 8) Skills, talents and specialization

Crafting and harvesting activities develop their associated skills. Use `craft score` to follow your progress and `talents` to explore the available crafting talent trees.

```
craft score
talents
craft specialize <skill name>
```

You can permanently specialize in up to two crafting or harvesting skills. Each specialization grants +5 to that skill's checks and doubles its experience gains. Check the names in `craft score` before choosing: specializations cannot be changed.

Talents can improve such things as speed, yields and crafting results. Read the individual talent before spending points.

## 9) Complete supply orders

Supply orders are crafting jobs from a quartermaster. Request the job there, inspect its requirements, assign materials, and work at the required crafting station.

```
supplyorder request
supplyorder show
supplyorder material add all
supplyorder start
```

You can assign a particular suitable material with `supplyorder material add <material>` instead of `all`. Allow the work to finish, then check `supplyorder show`. Add more materials and repeat `supplyorder start` as needed until the required quantity is complete.

Return to a quartermaster to collect the reward:

```
supplyorder complete
```

Useful controls:

```
supplyorder material remove
supplyorder reset
supplyorder abandon
supplyorder cooldown
```

`reset` clears the current work setup; `abandon` cancels the order itself. Check the game's messages before replacing an order. `cooldown` shows your current timing restrictions. The current system uses `request`, not `supplyorder list` or `supplyorder select`.

More information:
https://krynn.d20mud.com/wiki/supply-orders

## 10) Craft a golem

Golem construction is a separate path requiring the appropriate construct feats, materials and crafting requirements.

```
craft golem type <wood|stone|iron>
craft golem size <small|medium|large|huge>
craft golem appearance <material|default>
craft golem show
craft golem start
```

Type `craft golem` to see the complete menu. Appearance choices are cosmetic and do not change the golem's construction type or combat abilities.

Other controls include `craft golem reset`, `craft golem recall`, `craft golem deploy`, `craft golem upgrade <size>` and `craft golem shutdown`. Read their messages before changing an existing golem. Features such as stowing an active golem and tactical directives depend on your abilities.

## Getting unstuck

- Review a project with `craft show` and diagnose missing requirements with `craft check`.
- Use `cancel craft` to stop an active task before changing projects.
- Use `craft reset` to clear item-project setup, `refine remove` for refining, and `resize reset` for resizing. Read the messages showing what was returned; cancellation does not promise to recover ingredients already consumed by completed work.
- Check your equipped tools and the station required by the recipe.
- If you cannot allocate a material, check its exact name, available quantity, group and grade.
- Use `supplyorder show` for job progress rather than normal item-project commands.

## Website guides

Crafting hub:
https://krynn.d20mud.com/wiki/crafting

Step-by-step tutorial:
https://krynn.d20mud.com/wiki/crafting-tutorial

Harvesting materials:
https://krynn.d20mud.com/wiki/harvesting-crafting-materials

Motes:
https://krynn.d20mud.com/wiki/crafting-motes

Refining:
https://krynn.d20mud.com/wiki/refining-materials

Creating equipment:
https://krynn.d20mud.com/wiki/creating-equipment

Resizing:
https://krynn.d20mud.com/wiki/resizing-gear

Supply orders:
https://krynn.d20mud.com/wiki/supply-orders
