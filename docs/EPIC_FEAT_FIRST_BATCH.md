# Epic feat expansion: first batch

Implemented from the approved catalogue on 2026-11-17. See [batch two](EPIC_FEAT_SECOND_BATCH.md) for the subsequent implementation; all later entries are now implemented; see [completion notes](EPIC_FEAT_THIRD_BATCH.md).

| Class/group | Implemented feats |
| --- | --- |
| Psionicist | P01 Epic Power Knowledge; P03 Epic Augmentation Efficiency; P04 Epic Focus Continuity; P07 Epic Mental Bastion; P08 Epic Manifesting Poise |
| Artificer | A01 Epic Artisan's Insight; A02 Epic Efficient Item Creation; A03 Epic Metamagic Science; A04 Epic Device Penetration; A06 Epic Anchored Chassis |
| Summoner | S01 Epic Evolution Reserve; S02 Epic Adaptive Eidolon Ward; S05 Epic Planar Anchorage |
| General | G01 Epic Fortitude; G02 Epic Reflexes; G03 Epic Will; G04 Superior Initiative; G05 Legendary Athlete; G10 Improved Spell Capacity |
| Berserker | O05 Ruinous Rage |

All require character level 21 and their catalogue prerequisites. Class feats are registered in their named class's bonus list and remain available through general epic points. No extra feat points are awarded. Existing feat IDs are preserved; 1326 remains reserved as the former sentinel, and new IDs run from 1327 through 1346.

Study previews the benefit before confirmation. Power Knowledge chooses two distinct ordinary powers per rank, Efficient Item Creation chooses a creation category, and Improved Spell Capacity chooses one casting class. Cancelling discards these choices without spending points; acceptance revalidates eligibility and point availability. Subsequent capacity ranks retain the same class.

## Player commands and help

- `epic artisan-insight`: swift, out of combat, once per 300 seconds; arms the next failed eligible crafting check for 60 seconds.
- `epic efficient-trigger`: swift, once per 60 seconds; arms metamagic charge efficiency for 18 seconds.
- `epic pierce-device`: swift, once per 60 seconds; arms one device resistance-check bonus for 18 seconds.
- `epic eidolon-ward <acid|cold|electricity|fire|sonic>`: swift, once per 120 seconds; protects the primary eidolon for 60 seconds.

Other first-batch feats are passive. Individual `help <feat name>` and `feats info <feat name>` resolve directly to the registered descriptions, matching Study. [The help bundle](../lib/text/help/epic_expansion.hlp) also provides an overview and importable individual entries for installations using database help.

## Runtime details

- Bonus powers are saved separately from ordinary known-power choices. They do not consume that ordinary budget and disappear when the feat is removed; independently learned powers remain in their native list. All five epic power entries are excluded from these selections.
- Augmentation limits and step sizes are calculated before discounts. The passive reduction and existing legendary resource reductions use the larger applicable discount, rather than adding together. Existing reductions to the unaugmented base cost retain their behavior. The feat neither spends the legendary discount budget nor changes the selected augmentation.
- Manifesting Poise benefits the next native Concentration check after damage during an ordinary manifestation. Damage packets do not create extra Concentration checks.
- Save bonuses remain separate by save type. Mental Bastion recognizes actual fear/confusion/charm effects, including the ordinary psionic and relevant warlock effects, and avoids adding another copy over existing insight bonuses. Planar Anchorage does not invent a save against native banishment/dismissal effects that offer none.
- Equipment recipe registrations record their epic creation category. Weapons and armor use the arms/armor category; worn miscellaneous equipment uses wondrous. Other unclassified recipes, ammunition jobs, golem assembly, harvesting, refinement and device/matrix rebuilding gain no epic time reduction. Eligible crafting time reductions share the 50% cap.
- Physical wand/staff activations now charge their parsed metamagic surcharge, matching the existing stored-consumable path; previously physical items parsed metamagic but paid only the base charge. Epic Metamagic Science discounts that surcharge by one, never the base charge.
- Device Penetration is scoped to the first resistance check of the committed manual activation. Exhausted-device failure still spends the arm; echoes and nested proc casts receive no bonus.
- Eidolon Ward is a dispellable affect with an absolute expiry. The owner saves its remaining expiry and energy because pet rows do not store arbitrary spell affects. Restoring the saved pet preserves the original expiry; a fresh summon does not receive the old ward. Mirrors and other followers do not qualify.
- Evolution points enter the normal Study budget. Study rejects finalization of an overspent eidolon. Other form, rank and follower restrictions remain native.
- Ruinous Rage carries one packet's penetration budget through typed and general physical DR. It does not add five to every DR layer, stack another five over a larger existing bypass, or benefit secondary proc damage.
- Active cooldowns and arms use explicit ASCII player tags (`EpCh`, `EpTm`) with absolute timestamps. Reconnecting does not refill them. A class respec clears subchoices and arms while retaining spent cooldowns. Old player files without these tags default to no choices and no spent uses.

## Validation

- Full server build using CMake.
- `unittests/test_epic_feats.sh` exercises all 20 eligibility paths, repeat-rank gates, staged versus live ownership, prerequisite loss, crafting rerolls and time caps, item charge floors, single-use penetration, queued PSP payment/no double payment, discount non-stacking, power exclusions, typed saves, companion categories and ward restoration/expiry/dispel snapshots.
- The same component suite runs with AddressSanitizer and UndefinedBehaviorSanitizer using `EPIC_TEST_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' unittests/test_epic_feats.sh`.

These are build and component checks. No live-world server restart, player migration or database help import is performed by this change. Before deploying, exercise a Study purchase and each active command on a test character against the installation's world and pet database.
