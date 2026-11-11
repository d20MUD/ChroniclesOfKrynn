# Reforge item-state audit

The inventory `reforge` command in `crafting_new.c` and the older kit-based
`reforge()` in `craft.c` reshape the existing `obj_data`. Neither creates a
replacement object or copies a selected list of magical properties.

## Item 4214

The checked-in prototype is a blade of shining yellow light, a long sword
with a `blade` restring identifier. Its `K` record is `10 117 0 2 0`: level-10
daylight, two maximum charges, initially zero current charges and no cooldown.
The normal activate/lore code initializes an unused zero-charge item when its
cooldown is zero; reforging must not itself refresh charges or reset a cooldown.

`tests/run_reforge.py` executes the actual current reforge implementation and
actual weapon/armor setters with native object structures. It checks the
long-sword-to-longbow case, including partial restringing, and an armor case.
The complete object structure is compared before and after, allowing only the
intended shape and description changes. Daylight, charges and cooldown remain
unchanged. This did not reproduce a direct loss of the activated spell; the
specific live instance has not been inspected or repaired.

## Property disposition

| Property | Reforge behavior |
| --- | --- |
| Activated spell, caster level, current/max charges, cooldown | Preserved in place and checked through full-record save/load |
| Proc spells, proc chance/combat gating/remaining uses, channel spells | Preserved in place; all slots persisted |
| Special abilities, command words, levels, activation methods and values | Entire linked list preserved; full-record serializer writes every entry |
| Stat/skill/feat effects, specific targets, bonus types, permanent flags | Preserved |
| Enhancement, minimum level, binding, price/rent, instance flags, timers, poison, tinkering | Preserved; base price is restored after shape setup |
| Prototype/runtime identity, legendary identity/cache, scripts, contents and other runtime pointers | Same live object; no object replacement or runtime pointer copying |
| Action/extra descriptions, arcane mark | Preserved |
| Keywords, short/room description and restring identifier | Existing partial restring logic replaces the old subtype token |
| Weapon subtype and base damage dice, armor subtype/base AC, wear placement, base weight/weapon size | Recomputed from the selected shape, as before |
| Material | Original material retained when compatible with the new shape's material family, as before |

Runtime events and scripts retain the normal game persistence rules; this audit
does not add serialization of arbitrary runtime pointers. Legendary transform
restrictions remain in effect. Incompatible material conversion and base damage
changes are existing reforge behavior, not missing copied powers.

## Fixes

The current inventory reforge path changed object weight without updating the
carrier's weight total. It now applies the exact delta immediately.

The pet and sheath SQL writers used incomplete duplicate serializers. They
could omit proc/channel spells, poison, timers and other properties, write only
the first special ability, and fail on apostrophes in customized descriptions.
They now quote the complete canonical object record once, using the same writer
as inventory/house saves. New records use the full file parser on reload,
including multiline descriptions and all ability entries. Legacy unmarked rows
retain their compatibility parser. Missing data already lost from a saved
instance cannot be reconstructed by this change.

## Validation

- `python3 tests/run_reforge.py`: native command and setters, complete live
  structure comparison, partial restringing, daylight charges/cooldown,
  carrier-weight accounting, ASan/UBSan.
- `python3 tests/run_legendary_persistence.py`: disposable MariaDB only; real
  file/player/house/pet/sheath codecs plus existing estate and identity tests.
  Rich-record checks compare every object flag/value, effect, proc/channel and
  activated spell component, poison, tinkering, descriptions and full ability
  chain after save/load. Test adapters were brought up to date for current
  description helpers, legacy-bow normalization and restore-only object reads.
- CMake server build.

No live database updates or server restart were performed.
