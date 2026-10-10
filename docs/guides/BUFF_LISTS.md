# Buff lists

Each character has ten saved buff lists, numbered 1–10, with up to 40 spells or
powers per list. Existing characters retain their previous buffs in list 1.

```
buff add 1 mage armor
buff add 2 shield
buff add 3 bless
buff list 1
buff lists
buff remove 2 shield
buff perform 1 self
buff perform 3 companion
buff cancel
```

The list number is optional; omitting it uses list 1. For example, `buff add
mage armor`, `buff list`, and `buff perform` still work. `buff perform companion`
uses list 1 on the named character. Numeric target selectors also work:
`buff perform 3 2.guard` buffs the second matching guard.

Targets must be visible in the same room. Each spell or power still follows its
normal targeting restrictions, including restrictions on self-only buffs.
Both spells and psionic powers receive the chosen target. Psionic augmentation
continues to use the existing augment-buffs preference.

`buff target companion` sets the default target. `buff target`, `buff target
self`, or `buff perform 1 self` resets it to yourself. An explicit target on
`buff perform` becomes the new default. Targets are not saved across logins.

Cancel an active sequence before starting another, changing its target, or
editing its list. You can edit other lists while buffing. Buffing stops if its
target leaves the room, disappears, or becomes unavailable, rather than switching
to yourself or another character with the same name.

Lists are saved in character flat files: the legacy `Buff` block holds list 1;
the new `BfLs` block holds lists 2–10. No database migration is required.

Validation: run `python3 tests/run_buff_lists.py` from the repository root. It
uses production command, save/load, and pulse code with isolated game stubs and
AddressSanitizer/UndefinedBehaviorSanitizer; it does not access live player data.
