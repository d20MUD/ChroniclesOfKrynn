# Spellhelp batch 001–050

All 50 numeric IDs have reviewed core-mechanics records. Of these, 46 are
registered spells and four are retired/unregistered IDs. The retired entries
remain searchable in `spellhelp` with an explicit unavailable status; they are
not added to casting, preparation, class lists, or spell registrations.

This batch brings the catalog to **66 mechanics records: 62 registered entries
and four unavailable legacy entries**. Full cross-cutting feat/perk/equipment
interaction audits remain a separate phase.

| ID | Spellhelp name | Availability |
| ---: | --- | --- |
| 1 | armor | **Unavailable legacy ID** |
| 2 | teleport | Registered |
| 3 | bless | Registered |
| 4 | blindness | Registered |
| 5 | burning hands | Registered |
| 6 | call lightning | Registered |
| 7 | charm person | Registered |
| 8 | chill touch | Registered |
| 9 | clone | Registered |
| 10 | color spray | Registered |
| 11 | control weather | Registered; weather effect is a stub |
| 12 | create food | Registered |
| 13 | create water | Registered |
| 14 | cure blind | Registered |
| 15 | cure critic | Registered |
| 16 | cure light | Registered |
| 17 | bestow curse | Registered |
| 18 | detect alignment | Registered |
| 19 | detect invisibility | Registered |
| 20 | detect magic | Registered |
| 21 | detect poison | Registered |
| 22 | dispel evil | Registered |
| 23 | earthquake | Registered |
| 24 | enchant item | Registered |
| 25 | energy drain | Registered |
| 26 | fireball | Registered |
| 27 | harm | Registered |
| 28 | heal | Registered |
| 29 | invisibility | Registered |
| 30 | lightning bolt | Registered |
| 31 | locate object | Registered |
| 32 | magic missile | Registered |
| 33 | poison | Registered |
| 34 | protection from evil | Registered |
| 35 | remove curse | Registered |
| 36 | sanctuary | Registered |
| 37 | shocking grasp | Registered |
| 38 | sleep | Registered |
| 39 | strength | Registered |
| 40 | summon | Registered |
| 41 | ventriloquate | **Unavailable legacy ID** |
| 42 | word of recall | Registered |
| 43 | remove poison | Registered |
| 44 | sense life | Registered |
| 45 | animate dead | Registered |
| 46 | dispel good | Registered |
| 47 | group armor | **Unavailable legacy ID** |
| 48 | group heal | Registered |
| 49 | group recall | **Unavailable legacy ID** |
| 50 | infravision | Registered |

## Audit findings reflected in help

- **Armor (1), Ventriloquate (41), Group Armor (47), Group Recall (49):** no active registration. Armor directs readers to Mage Armor (84). Group Recall documents its retained handler without advertising it as castable.
- **Teleport (2):** moves the caster to the named target, subject to travel and protection checks.
- **Burning Hands (5), Color Spray (10), Lightning Bolt (30):** follow their current single-target implementations, not tabletop cone/line assumptions. Damage dice and caps match the damage handler.
- **Chill Touch (8), Color Spray (10):** damage and status effects have separate save/resistance paths.
- **Clone (9):** creates a template-based follower with the caster’s name; it does not copy stats or gear and needs no undefined entrails component.
- **Control Weather (11):** accepts improve/worsen but does not alter weather.
- **Curse (17), Invisibility (29), Poison (33):** include character and object branches, durations, removal, and combat effects.
- **Detection spells:** describe the visibility/display checks actually enabled, not automatic identification or full darkvision.
- **Dispel Evil (22), Dispel Good (46):** damaging spells with alignment protections and possible self-targeting, not generic dispels.
- **Earthquake (23):** excludes flying targets and does not dispatch a separate knockdown.
- **Enchant Item (24):** covers eligibility, permanent enhancement, and resulting item-level requirements.
- **Energy Drain (25):** documents negative-energy damage and the separately rolled XP changes that precede resistance and saves; Death Ward blocks the operation.
- **Harm (27):** deals unholy damage; against undead it converts to Heal.
- **Sanctuary (36):** its aura has no connected damage-reduction/attack-prevention rule; the paladin perk of the same name is separate.
- **Sleep (38):** respects the level cutoff/immunities, ends combat, and cannot be broken by ordinary waking.
- **Animate Dead (45):** documents corpse consumption, corpse-content transfer, creature thresholds, failure/follower checks, and summon scaling.
- **Group Heal (48):** heals the group but does not dispatch Heal’s blindness-removal effect.

## Routing regression corrected during review

The earlier effective-routine refactor selected routines before undead spell
conversion. Selection now happens after conversion, preserving existing
Harm/cause-wound-to-healing behavior while retaining Deadly Fear’s conditional
area dispatch. No legacy spell was activated and no intended tabletop mechanics
were substituted for existing game behavior.

## Validation

`tests/run_spellhelp.py --sanitize` compiles the actual `spello()` initializer
and the 46 literal registration calls for this batch. It checks all 50 rendered
entries, unavailable-ID lookup, key formulas/conditions, colored column layout,
live configuration behavior, and unchanged registration/player/config state.

The source index can be regenerated with:

```sh
python3 tools/audit_spellhelp.py --range 1 50 --json
```

Separate regression checks cover undead routine selection and Deadly Fear’s
augmentation and save sequence.
