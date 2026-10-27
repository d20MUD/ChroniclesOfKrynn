# Harvesting Materials

Harvesting supplies the resources used to create equipment. Materials are kept in your stored crafting balances, which you can view with `materials`. This command also shows your elemental motes.

## Your first harvest

```
craft tools
craft score
survey
harvest
materials
```

Equip the correct harvesting tool before starting. Find a suitable resource area, use `survey` and let it finish. A successful survey gives you survey rooms, which are used up as you travel. Survey again when they run out.

Use `harvest` in a room with resources. You do not need to name a material after the command. The room must have resources remaining, you must have survey rooms available, and the proper tool must be equipped. The game tells you when a requirement is missing.

Allow the harvesting task to finish. Successful harvesting adds materials to your balances and can also uncover [elemental motes](https://krynn.d20mud.com/wiki/crafting-motes). Talents and active happy-hour bonuses can improve your results.

Resources can be depleted or replaced as the world refreshes. If a room is empty, keep exploring rather than repeatedly trying to harvest it.

## Where to look

Terrain determines the resource groups that can appear. Forests commonly provide timber and game; fields provide fibers and game; hills, mountains and caves can provide metals. Zone levels and random resource quality also affect what appears. Survey the actual room to discover its current resources.

Water areas can provide coral, marine hides, fibers and occasional metals. They use the ordinary harvesting process. Coral uses forestry tools, marine hides use hunting tools, and marine fibers use gathering tools.

## Regional materials

The climate and terrain of an area can change the material names you find. These are distinct stored resources with their own names, but they inherit the crafting properties of their underlying material family.

| Climate | Examples of timber you may find |
|---|---|
| Polar | Driftwood, dwarf willow, birch driftwood, larch driftwood |
| Subarctic | Spruce, pine, birch, larch |
| Temperate | Ash, beech, willow, maple, oak |
| Subtropical | Pine, willow, cypress, cedar, live oak |
| Tropical | Bamboo, teak, mahogany, ebony |
| Arid | Tamarisk, acacia, mesquite |
| Alpine | Fir, pine, birch, cedar, larch |

Higher-tier timber includes valenwood and ironwood. Tropical areas can provide jute and arid areas agave fiber as alternatives to hemp. Different names do not automatically mean a stronger crafting material.

## Marine equivalents

Marine resources keep their own identities and balances while serving the same crafting families as these land materials.

| Marine material | Land equivalent |
|---|---|
| Branch coral | Ash |
| Fan coral | Maple |
| Blood coral | Mahogany |
| Deepwater coral | Valenwood |
| Adamant coral | Ironwood |
| Thin fishskin | Low grade hide |
| Tough eelskin | Medium grade hide |
| Sharkskin | High grade hide |
| Pristine sharkskin | Pristine grade hide |
| Kelp fiber | Hemp |
| Seagrass fiber | Flax |
| Sponge fiber | Wool |
| Sea cotton | Cotton |
| Sea silk | Silk |

## Exchange compatible varieties

At a materials exchange service, use:

```
exchange list
exchange 10 maple for birch
exchange 10 branch coral for ash
exchange 10 flax for seagrass fiber
```

Exchanges are one-for-one between compatible varieties of the same underlying material and grade. Being in the same broad group, such as wood, is not enough. You must have the source quantity and cannot exchange while fighting.

Use the exact material names shown in `materials`. Some refining recipes require specific ingredients; exchange a compatible variety for the named ingredient first if necessary.

## Other sources of resources

Random treasure can also award crafting materials and motes directly to your stored balances. Treasure material quality depends on the defeated creature's level. Artisan shop services offer materials and motes for artisan points earned through supply orders. Harvesting is therefore one source of resources, rather than the only source.

## Put your materials to use

Higher-quality materials can improve a project's final level and difficulty. The result depends on the whole recipe, including how much of each material it uses. Check `craft show` instead of treating an area's resource tier as a universal material-grade or item-level rule.

Continue with [Refining Materials](https://krynn.d20mud.com/wiki/refining-materials) or [Creating Equipment](https://krynn.d20mud.com/wiki/creating-equipment). The [Crafting Tutorial](https://krynn.d20mud.com/wiki/crafting-tutorial) follows the entire process.
