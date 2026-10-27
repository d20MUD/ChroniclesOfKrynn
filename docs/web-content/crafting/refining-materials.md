# Refining Materials

Refining combines stored ingredients into another material. You need the ingredients and the appropriate refining station in your room.

## Prepare and start

Type `refine` to see the available recipes. Then use:

```
refine add steel 5
refine show
refine begin
```

This prepares five batches of steel, shows the project, then starts work. Review the ingredients, output and difficulty before beginning.

To prepare and start in one step:

```
refine batch steel 5
refine smelt steel all
```

`refine batch` works with refining recipes generally. `refine smelt` is for smelter recipes. You can request 1–20 batches; `all` chooses the available amount up to 20 batches. A batch produces the recipe's listed output: five batches of brass produce ten brass, not five.

## Recipes

Quantities below are for one batch. Difficulty is the number your refining check must meet.

| Result | Ingredients | Output | Station | Skill | Difficulty |
|---|---|---|---|---|---|
| Bronze | 1 tin + 1 copper | 1 | Smelter | Mining | 15 |
| Steel | 1 iron + 1 coal | 1 | Smelter | Mining | 20 |
| Brass | 1 copper + 1 zinc | 2 | Smelter | Mining | 15 |
| Satin | 1 silk + 1 cotton | 1 | Loom | Tailoring | 30 |
| Linen | 2 flax | 2 | Loom | Tailoring | 20 |
| Alchemical silver | 2 steel + 2 silver | 1 | Smelter | Mining | 25 |
| Cold iron | 2 iron + 1 zinc + 1 silver | 1 | Smelter | Mining | 25 |
| Dragonmetal | 1 dragonblood + 2 eligible hard metal | 1 | Smelter | Mining | 30 |
| Crafting catalyst | 10 catalyst fragments | 1 | Alchemy laboratory | Alchemy | 25 |

Dragonmetal accepts hard metal of grade 3 or higher. The ordinary recipe uses iron; the game chooses an eligible substitute when applicable. Review `refine show` to see what has actually been allocated. You need enough of one eligible metal for the selected batch count.

Regional and marine varieties are separate balances. If a recipe needs flax, for example, exchange compatible seagrass fiber for flax at a materials exchange service before refining.

## Change or cancel a project

```
refine remove
```

This clears a prepared project and returns its allocated ingredients. To stop active work, use `cancel craft` and read the result before preparing another project. Completed refining has already consumed its ingredients.

## Where catalyst fragments come from

At an alchemy laboratory, `disenchant <item>` permanently destroys an eligible magical item in your inventory and gives catalyst fragments. Restricted equipment, including legendary items, cannot be disenchanted.

Refine ten fragments into one crafting catalyst. Catalysts are optional additions to [equipment projects](https://krynn.d20mud.com/wiki/creating-equipment); they are separate from the legendary materials used by Dragonmetal Awakening.

[Back to the Crafting Guide](https://krynn.d20mud.com/wiki/crafting)
