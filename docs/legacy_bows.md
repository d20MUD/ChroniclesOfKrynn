# Legacy composite bows

Composite long bows and short bows, including Strength ratings 1–5, retain their
stored weapon IDs, proficiency support and original combat rules for existing
saved weapons. Ordinary long bows and short bows apply the full Strength damage
modifier without a rating cap.

Fresh generation uses `read_object` and `set_weapon_object`. These map retired
composite types to ordinary bows. Loading old world prototypes keeps enchantments,
custom damage dice and other item properties, while removing “composite” and the
Strength rating from the name and descriptions. This covers resets, fixed shop
stock, quest rewards, scripted loads and builder loads. Random treasure excludes
retired types entirely.

Saved inventories, storage, market listings and ship cargo restore through
`read_object_for_restore`; stat initialization there uses
`set_weapon_object_for_restore`. These restoration entry points must only be used
for already-existing items or serialization comparisons, never fresh rewards.
Existing prototypes and enum IDs stay intact so old saves remain compatible.

Builder and bazaar numeric menus retain actual weapon IDs with gaps for retired
types. Their inputs, crafting, reforging, outfit selection, molds, weapon vendors
and holy-weapon selection reject composite bows. Supply orders exclude their
recipes, including cached offers. Copying a legacy prototype creates an ordinary
bow prototype; maintenance edits to the original remain possible. Such edits do
not turn fresh ordinary instances back into composite bows.

Run `python3 tests/run_legacy_bows.py` for native AddressSanitizer/UBSan coverage
of generation versus restoration, all legacy IDs, crafting/recipe selection,
menu numbering, prototype updates and ranged Strength damage.
