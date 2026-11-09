# Random ammunition power

`award_magic_ammo` assigns power from `source_level`, independently of the
recipient and the random enchantment conversion used by other equipment.
Source levels above 30 use the level-30 tier. When a caller supplies no
source level, the existing treasure grade selects the corresponding tier.

| Treasure source level | Enhancement | Elemental bonus, when rolled |
| --- | --- | --- |
| 1–5 | +1 | +1d4 |
| 6–10 | +2 | +1d6 |
| 11–15 | +3 | +1d8 |
| 16–20 | +4 | +1d10 |
| 21–25 | +5 | +1d12 |
| 26+ | +6 | +1d12 |

Common (`RARE_GRADE_NORMAL`) ammo never rolls elemental damage. Rare,
Legendary and Mythical have a 50%, 75% and 100% chance respectively. A
successful roll chooses uniformly among fire, cold, acid, electricity and
sonic. The die follows source level, not the rarity or player level.

The enhancement is passed through native object bonus/delivery handling;
elemental type/die use the same ammunition values as crafted ammunition.
Descriptions include enhancement and any damage bonus before loot delivery,
so terminal drop messages, web loot popups and lore agree with combat.
Existing rare-item additional properties are retained.

Every reward is still exactly one finite 50-use ammo object, with unlimited
and returning values cleared. Crafting recipes are unaffected.

`python3 tests/run_random_ammo_reward.py` covers every tier boundary and
rarity, all damage types and ammo subtypes, chance boundaries, descriptive
bonuses, fallback grades, bounds, missing prototypes and single delivery.
Adjacent ranged, crafting and loot harnesses cover the shared damage and
notification paths.
