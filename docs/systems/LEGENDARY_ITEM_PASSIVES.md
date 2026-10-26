# Legendary item passives

An awakened item can now have **one additional passive effect**, alongside its
existing foundation, technique and signature. The passive improves with legendary
rank. It is not a second foundation and does not consume foundation points or an
active-power slot. Existing foundations keep their existing rules.

## Choosing and changing it

At a legendary smith:

```text
infuse choices sword primary
infuse select sword primary passive P08
infuse confirm <confirmation-id>
infuse inspect sword
```

Use the item's keyword in place of `sword`. Choices only show passives allowed
for that item's actual wear slot. The first choice is free after awakening.
Replacing it costs **20,000 gold and 100 fragments**, using the existing confirmation
and recovery system. No extra XP, essences or catalysts are charged for selection.
Rank advancement still pays its ordinary XP, gold and material costs.

The effect requires a valid, owned, eligible equipped legendary item. Removing,
storing, transferring to a wrong owner, invalidating or quarantining the source
removes the benefit. Maximum-pool increases never grant current HP, movement or
spell/psionic points. A resource passive cannot give a noncaster spellcasting;
skill bonuses require the corresponding skill to be trained and do not grant
ranks. Save bonuses obey ordinary caps. Maneuver bonuses only improve an actual
resistance check and do not grant immunity.

## Exact bonuses

Values are the contribution of **one paid rank track**. The last column is the
maximum additional bonus of this passive across the entire equipped outfit,
separate from ordinary bonuses and foundation limits. Matching passives add only
up to that ceiling. These choices deliberately avoid adding flat attack/damage,
extra attacks or free spell casts.

| ID | Passive | Effect | I | II | III | IV | V | Outfit ceiling |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| P01 | Vital thread | maximum HP | 5 | 10 | 15 | 20 | 25 | 50 |
| P02 | Mystic reservoir | maximum spell/psionic points; requires an existing resource class | 4 | 8 | 12 | 16 | 20 | 40 |
| P03 | Enduring stride | maximum movement | 8 | 16 | 24 | 32 | 40 | 80 |
| P04 | Renewing thread | HP per ordinary out-of-combat regeneration tick | 1 | 2 | 3 | 4 | 5 | 10 |
| P05 | Wayfarer | percent reduction to ordinary unmounted walking cost | 2 | 4 | 6 | 8 | 10 | 20 |
| P06 | Watchful | trained Perception checks | 1 | 2 | 3 | 4 | 5 | 10 |
| P07 | Veiled | trained Stealth checks | 1 | 2 | 3 | 4 | 5 | 10 |
| P08 | Centered | trained Concentration checks | 1 | 2 | 3 | 4 | 5 | 10 |
| P09 | Healer's touch | trained Heal checks | 1 | 2 | 3 | 4 | 5 | 10 |
| P10 | Runic insight | trained Spellcraft checks | 1 | 2 | 3 | 4 | 5 | 10 |
| P11 | Artificer's touch | trained Use Magic Device checks | 1 | 2 | 3 | 4 | 5 | 10 |
| P12 | Unyielding grip | checks resisting disarm; does not grant immunity | 1 | 2 | 3 | 4 | 5 | 10 |
| P13 | Rooted | checks resisting trip/forced movement; does not grant immunity | 1 | 2 | 3 | 4 | 5 | 10 |
| P14 | Stalwart | Fortitude saves (+1/+1/+2/+2/+3 by rank) | 1 | 1 | 2 | 2 | 3 | 3 |
| P15 | Agile | Reflex saves (+1/+1/+2/+2/+3 by rank) | 1 | 1 | 2 | 2 | 3 | 3 |
| P16 | Resolute | Will saves (+1/+1/+2/+2/+3 by rank) | 1 | 1 | 2 | 2 | 3 | 3 |

Save bonuses use discrete steps: ranks II/IV do not add another save point.
Other passives improve at every rank. Walking discounts apply to ordinary
unmounted movement after existing reductions, with a minimum final cost of 1.
Recovery adds to the normal out-of-combat regeneration tick, never a separate
rapid timer.

## Choices by wear slot

| Wear slot | Available passives |
| --- | --- |
| Weapons | P01 Vital thread, P04 Renewing thread, P13 Rooted, P14 Stalwart, P02 Mystic reservoir, P06 Watchful, P08 Centered, P10 Runic insight, P12 Unyielding grip, P16 Resolute |
| Arms | P01 Vital thread, P03 Enduring stride, P08 Centered, P09 Healer's touch, P11 Artificer's touch, P12 Unyielding grip, P14 Stalwart |
| Legs | P01 Vital thread, P03 Enduring stride, P05 Wayfarer, P07 Veiled, P13 Rooted, P14 Stalwart, P15 Agile |
| Head | P02 Mystic reservoir, P06 Watchful, P08 Centered, P09 Healer's touch, P10 Runic insight, P11 Artificer's touch, P16 Resolute |
| Body | P01 Vital thread, P02 Mystic reservoir, P03 Enduring stride, P04 Renewing thread, P08 Centered, P13 Rooted, P14 Stalwart |
| About | P01 Vital thread, P04 Renewing thread, P05 Wayfarer, P06 Watchful, P07 Veiled, P15 Agile, P16 Resolute |
| Shields | P01 Vital thread, P04 Renewing thread, P12 Unyielding grip, P13 Rooted, P14 Stalwart, P16 Resolute |
| Held | P02 Mystic reservoir, P06 Watchful, P08 Centered, P09 Healer's touch, P10 Runic insight, P11 Artificer's touch, P16 Resolute |
| Feet | P03 Enduring stride, P05 Wayfarer, P07 Veiled, P13 Rooted, P15 Agile |
| Hands | P02 Mystic reservoir, P04 Renewing thread, P07 Veiled, P09 Healer's touch, P10 Runic insight, P11 Artificer's touch, P12 Unyielding grip, P15 Agile |

## Class and role examples

- **Weapon users:** Vital thread or Stalwart for frontline survival, Watchful for awareness, Unyielding grip against disarm,
  Centered for concentration-dependent abilities, or Resolute against mental effects.
- **Tanks:** Vital thread, Stalwart, Rooted and Resolute spread across armor and
  shield slots; select different effects when a shared ceiling is already reached.
- **Casters and psionicists:** Mystic reservoir uses their existing point pool;
  Centered and Runic insight support trained concentration and magical knowledge.
- **Healers and support:** Healer's touch improves trained Heal checks, Renewing
  thread helps between fights, and defensive choices improve survival.
- **Rogues and scouts:** Veiled, Watchful, Agile and Wayfarer support stealth,
  detection, Reflex saves and travel.
- **Artificers and device users:** Artificer's touch improves trained Use Magic
  Device checks, with Runic insight and Mystic reservoir as alternatives.

None of these selections grants a class feature, skill training, power or spell
that the character does not already possess.

## Two-handed weapons

A two-handed weapon still has **one selected passive**, chosen on its primary
track. Its supporting track inherits that choice; it cannot select a second one.
Each valid, independently paid track contributes its own rank value, while the
same outfit ceiling applies. Primary V/supporting I Centered contributes 5 + 1;
V/V contributes 5 + 5. A one-handed weapon and held focus with those same ranks
and that same passive contribute the same amounts for the same rank costs.
No supporting benefit applies with an invalid grip or occupied offhand. Merely
awakening the primary track does not grant a second contribution for free.

## Persistence and release status

Selection lives in the primary channel's `item_passive_id`; supporting rows must
keep it zero. Apply `sql/20261025_legendary_item_passives.sql` explicitly after
schema v5 to reach schema v6. Existing items start unselected and receive their
free first choice. No migration is executed by server boot. Definition version
is 3. This change requires fresh phase-14B encounter measurements before the full
legendary combat balance gate can be closed.
