# Supply Orders

Supply orders are available crafting jobs from a quartermaster. Make the requested items through the supply-order commands, then return to a quartermaster for gold, bonus crafting experience and artisan points.

## Request a job

Finish or clear any existing crafting project, visit a quartermaster and use:

```
supplyorder request
supplyorder show
```

A new order requests two to four items. Read its recipe, required materials and crafting requirements before beginning. You can have one active supply order at a time.

## Assign materials and work

```
supplyorder material add all
supplyorder show
supplyorder start
```

Assign materials before starting. `all` chooses suitable available materials from your balances. You can instead use `supplyorder material add <material>` to choose a suitable material yourself.

Work at the crafting station required for the recipe. Let the task finish, inspect `supplyorder show`, and supply more materials and repeat `supplyorder start` as needed until the required quantity is complete. You need materials for each item you make.

These are supply-order projects: follow their commands rather than trying to fill an order by handing over unrelated items from your inventory.

## Collect the reward

Return to a quartermaster and use:

```
supplyorder complete
```

You receive gold, bonus experience in the associated crafting skill, and artisan points. The base artisan-point reward is ten points per requested item; an active crafting happy hour can increase it. Your completion message shows the actual rewards.

Artisan points can buy materials and motes at an artisan shop service. Check the shop's displayed stock, prices and command help when you visit.

## Cooldowns

You can complete three orders during a two-hour window. The window starts when you turn in the first completed order. After all three are used, requesting another order is blocked until the window expires.

```
supplyorder cooldown
```

Use this command to check your current timing restrictions.

## Change or abandon an order

| Command | Purpose |
|---|---|
| `supplyorder show` | Review requirements and progress |
| `supplyorder material remove` | Remove assigned materials |
| `supplyorder reset` | Clear the current work setup while keeping the order |
| `supplyorder abandon` | Cancel the whole order, including its progress |
| `supplyorder cooldown` | Show timing information |

Resetting or abandoning returns materials still assigned to the setup. It does not restore ingredients already consumed by completed work. Read the game messages before replacing a project.

The request command is `supplyorder request`; there is no `supplyorder list` or `supplyorder select` step.

[Crafting Tutorial](https://krynn.d20mud.com/wiki/crafting-tutorial) · [Crafting Guide](https://krynn.d20mud.com/wiki/crafting)
