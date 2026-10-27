# Crafting Progression

Every harvesting trip and crafting project can help you grow as a crafter. Your progress has several parts: individual skill ranks, talent points, permanent specializations and artisan points.

Start by checking your character:

```
craft score
talents
materials
```

`craft score` shows your skills, experience progress, specializations, unspent talent points and artisan points. `talents` shows the available talent categories. `materials` shows your stored materials and elemental motes.

## Understand your progress

| Progress or resource | How you earn it | What it does |
|---|---|---|
| Skill experience | Perform activities that use that skill | Builds toward the next skill rank |
| Skill ranks | Earn enough experience in an individual skill | Improves your ability in that skill and awards talent points |
| Talent points | Gain crafting or harvesting skill ranks | Pays for crafting and harvesting talents, together with gold |
| Specializations | Choose up to two skills with `craft specialize` | Gives +5 to checks and doubles base experience gains in the chosen skills |
| Artisan points | Complete supply orders | Buys materials and motes at artisan shop services |

## Improve your skills through use

Crafting and harvesting skills progress individually. Mining experience improves mining; it does not automatically improve tailoring or weaponsmithing.

Activities that develop skills include:

- **Surveying:** find resources and develop surveying through successful surveys.
- **Harvesting:** gather resources and gain experience in the associated harvesting skill.
- **Butchering:** process suitable corpses using the butchering system.
- **Refining:** gain experience in the skill used by the refining recipe. Metal refining commonly uses mining, cloth refining uses tailoring, and catalyst refining uses alchemy.
- **Creating equipment:** develop the crafting skill used by your selected item variant.
- **Supply orders:** gain experience while making the requested items, plus bonus crafting experience when you turn in the completed order.

Experience rewards depend on the activity and result. Some failed attempts can still award experience, but you should not assume every attempt will do so. Watch the result messages and use `craft score` to follow your progress.

Later ranks require more experience. The game's progress display shows how much you still need for your next rank.

### Make your practice productive

Choose projects you can reasonably complete with the tools, materials and skills you have. Start with plain equipment before adding several magical bonuses.

```
craft tools
craft equipment
craft show
craft check
```

Check the recipe's required skill and station. A different variant can use a different crafting skill, so review the project before spending resources.

Higher-quality materials can change a project's level and difficulty. Use the actual requirements shown by `craft show` and `craft check` when deciding what to make next.

## Earn and spend talent points

Each time a crafting or harvesting skill increases, you gain **one talent point**. Improving different skills can therefore contribute to the same pool of unspent talent points.

Talent points buy lasting improvements to your crafting and harvesting activities. Learning a talent also costs gold.

### Browse before spending

```
talents
talents available
talents all
talents info <number or name>
```

- `talents` lists the categories you can explore.
- `talents available` shows talents you can currently learn.
- `talents all` shows the broader talent list.
- `talents info <number or name>` explains a talent, its ranks and the cost of the next rank.

When you have chosen a talent, use its number:

```
talents learn <number>
```

Use the number displayed in your own talent list. Learning uses a number even though the information command also accepts a name.

### Talent ranks and costs

Many talents have multiple ranks. Buying another rank improves that talent until its maximum is reached.

The cost rises with the rank you are buying. For a talent whose first rank costs one talent point and 1,000 gold, the second rank costs two points and 2,000 gold, and the third costs three points and 3,000 gold. These are separate purchases, not the total cost of all ranks combined.

Different talents have different starting costs and maximum ranks. Always read `talents info` before buying.

### What talents can improve

Skill-specific talents include improvements to checks, task speed, efficiency, experience gains and crafting results. Their effects depend on the activity and talent involved.

For example:

- **Proficient talents** improve checks for their associated skill.
- **Rapid talents** reduce the time taken by associated tasks.
- **Insightful talents** increase experience earned in the associated skill.
- **Efficient talents** can improve resource use or harvesting yields, depending on the skill.
- **Expertise talents** improve the associated skill's critical-success potential.
- **Mote synergy talents** add motes when harvesting awards the corresponding mote type. They do not guarantee that type will drop on every harvest.

Choose talents that support the work you actually do. Read each talent's description rather than assuming similarly named talents have identical effects across every activity.

## Choose up to two specializations

A specialization is a permanent choice of a crafting or harvesting skill.

```
craft score
craft specialize <skill name>
```

Use the skill name shown in `craft score`. You can choose **up to two different skills**.

Each chosen skill receives:

- **+5 to its skill checks.**
- **Double its base experience gains.**

Specializations cannot be changed. You do not need to choose both immediately; spend some time harvesting and crafting before deciding which skills you want to focus on.

Specialization, Insightful talents and happy-hour experience bonuses can all contribute to an experience award. These bonuses are calculated from the base award, so do not assume that each bonus multiplies the entire result of the others.

## Earn artisan points through supply orders

Supply orders give you a reason to practice crafting while earning gold, crafting experience and artisan points.

Visit a quartermaster with no other crafting project set up, then use:

```
supplyorder request
supplyorder show
supplyorder material add all
supplyorder start
```

Assign materials before starting, and work at the station the recipe requires. Let the task finish, check your progress, and add materials and repeat the work as needed.

Once the required quantity is complete, return to a quartermaster:

```
supplyorder complete
```

The base reward is **10 artisan points per requested item**. An order for three items therefore awards 30 artisan points before any happy-hour bonus. Completion also awards gold and bonus experience in the associated crafting skill.

### Supply-order timing

You can complete three orders during a two-hour window. The window starts with your first completed turn-in. Once you have used all three, you must wait for the window to expire before requesting another order.

```
supplyorder cooldown
```

Read [Supply Orders](https://krynn.d20mud.com/wiki/supply-orders) for the full workflow and the difference between resetting work and abandoning an order.

## Spend artisan points on supplies

Visit an artisan shop service and use:

```
artisanshop
artisanshop buy 5 fire
artisanshop buy 2 iron
```

The service must be available where you are, and you cannot shop while fighting. Purchases require enough artisan points and add the resources to your stored balances.

The full command is `artisanshop`. The shop may display abbreviated examples beginning with `artisan`; using the full command avoids ambiguity.

### Current prices

| Purchase | Artisan points per unit |
|---|---:|
| Grade 1 material | 10 |
| Grade 2 material | 25 |
| Grade 3 material | 40 |
| Grade 4 material | 75 |
| Grade 5 material | 180 |
| Grade 6 material | 300 |
| Elemental mote | 20 |

Check the shop's list for available materials, their exact names and prices. Use the material's listed grade; a harvesting area's resource tier is not always the same as the material's actual grade.

Artisan points give you another way to obtain ingredients between harvesting trips. They do not replace talent points when learning talents.

## Make use of happy hours

When the relevant happy-hour bonuses are active, they can increase crafting or harvesting experience, harvested material yields, the chance to find motes, and artisan points from supply orders.

The active bonuses determine which benefits apply. Read the game's reward messages for the amounts actually awarded; a harvesting bonus does not automatically apply to every other source of materials.

## A practical progression path

1. **Learn the basics.** Use `craft tutorial`, equip harvesting tools, survey and gather materials.
2. **Pick an initial craft.** Choose a plain item recipe and check which skill it develops.
3. **Build skill ranks.** Gather, refine and make useful equipment while checking `craft score`.
4. **Read your talent options.** Compare the benefits and costs, then spend points on activities you use regularly.
5. **Choose specializations carefully.** Commit when you know which two skills suit your plans.
6. **Complete supply orders.** Earn additional crafting experience, gold and artisan points.
7. **Reinvest your rewards.** Buy missing ingredients or motes and attempt more ambitious projects as your skills improve.

## Related guides

- [Crafting Guide](https://krynn.d20mud.com/wiki/crafting)
- [Crafting Tutorial](https://krynn.d20mud.com/wiki/crafting-tutorial)
- [Harvesting Materials](https://krynn.d20mud.com/wiki/harvesting-crafting-materials)
- [Creating Equipment](https://krynn.d20mud.com/wiki/creating-equipment)
- [Supply Orders](https://krynn.d20mud.com/wiki/supply-orders)
