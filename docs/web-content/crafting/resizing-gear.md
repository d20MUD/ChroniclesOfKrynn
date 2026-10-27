# Resizing Gear

Resizing changes the size of eligible equipment. It can help clothing fit your character or change a weapon's handling. The best weapon size depends on your character's size, feats and fighting style.

## Select a carried item

Keep the item in your inventory, then choose its target size:

```
resize sword small
resize show
resize check
resize begin
```

Replace `sword` with a keyword for your item and `small` with the desired size. Type `resize` for command help. If you enter an invalid size, the game lists the available size names.

Let the task finish and keep the item with you until it completes. `resize start` is also accepted in place of `resize begin`.

## Requirements

- The item must be in your inventory when you select it and remain available while resizing.
- Choose a different size from its current size.
- The item must be eligible for transformation. Legendary equipment cannot be resized.
- Finish or cancel other active crafting work before starting another task.

Resizing has no separate material-allocation step. You do not use `resize add` or `resize remove`.

## Change your selection

```
resize reset
```

This clears the selected resizing project so you can choose again. To stop an active crafting task, use `cancel craft`.

Review the resized item's size and your equipment setup before relying on it in combat.

[Crafting Tutorial](https://krynn.d20mud.com/wiki/crafting-tutorial) · [Crafting Guide](https://krynn.d20mud.com/wiki/crafting)
