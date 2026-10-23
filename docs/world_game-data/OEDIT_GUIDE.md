# OEDIT: object editor and builder reference

This guide describes the current Krynn object editor, reviewed on 2026-10-23.
It covers its menus, all 119 extra flags, all 34 wear flags, all 58 item-type
entries, applies, spells, traps, and saving. It replaces earlier instructions
that described unsupported creation syntax and older menu limits.

An object prototype defines an item vnum. A live object is an individual copy
of that prototype. OEDIT edits the prototype; IEDIT edits an individual object.
Room movement costs belong to room sectors and flags; see the
[REDIT guide](REDIT_GUIDE.md) for those. Object weight, equipment, capabilities
and applies can affect the character using them.

## Contents

- [Opening, creating, and saving](#opening-creating-and-saving)
- [Main menu](#main-menu)
- [Menu numbering](#menu-numbering)
- [Object types](#object-types)
- [Values](#values-what-the-numbers-mean)
- [Wear flags](#wear-flags)
- [Extra flags](#complete-extra-flag-reference)
- [Applies and bonus types](#applies-and-bonus-types)
- [Permanent affects](#permanent-affects-p-and-v)
- [Weapon spell procs](#weapon-spell-procs-f)
- [Structured special abilities](#structured-special-abilities-j)
- [Activated spells](#activated-spells-k)
- [Spellbooks](#spellbooks-t)
- [Attached traps](#attached-traps-y)
- [Materials and size](#materials-and-size)
- [Scripts and population](#scripts-special-procedures-and-population)
- [Building workflow](#recommended-building-workflow)
- [Limitations and common mistakes](#current-limitations-and-common-mistakes)
- [Source references](#source-references)

## Opening, creating, and saving

| Command | Purpose |
| --- | --- |
| `oedit <object-vnum>` | Edit an existing prototype, or create one at an unused numeric vnum in a zone you may edit. |
| `oedit save <zone-number>` | Save that zone's object prototypes to disk. Supply the zone number, not an object vnum. |
| `oedit save` | Attempts to use the configured OLC zone; use an explicit zone number to avoid the parser caveat below. |

OEDIT requires an object vnum: bare `oedit` does not edit an inventory item.
`oedit new` and `oedit create <vnum>` are not supported syntax. You need builder
access and permission for the prototype's zone. Another builder cannot edit the
same prototype simultaneously.

The shorthand save path converts the configured OLC zone to its bottom room
vnum before looking it up as a zone number. This can fail or resolve the wrong
zone. Prefer `oedit save <zone-number>`.

New objects have placeholder descriptions, Takeable enabled, Medium size,
no binding, no designated NPC recipient, and material initially unset.

Choose `Q` to finish. At the confirmation, `Y` saves, `N` discards, and `A`
returns to editing. If automatic OLC saving is disabled, the result is saved
**to memory**; follow with `oedit save <zone-number>` for disk persistence.
When enabled, the editor reports `Object saved to disk.` Object templates
normally live in the zone's `.obj` world file. `redit save` saves rooms, not
objects.

Saving an existing prototype calls `update_all_objects()`: matching live
objects are updated as well, retaining location, ownership pointers, contents
and other listed runtime links. Do not assume an OEDIT save only affects future
loads or preserves every customized property on existing copies. Load and test
new copies after major changes. Separately serialized objects in storage have
their own restoration behavior.

## Main menu

| Choice | Setting | Purpose |
| --- | --- | --- |
| `1` | Keywords | Names players use to target the item, such as `sword steel longsword`. |
| `2` | S-Desc | Short description used in inventory, equipment and action messages, usually `a steel longsword`. |
| `3` | L-Desc | Ground/room description, usually a complete sentence. This is a single-line entry. |
| `4` | A-Desc | Multiline action text consumed by item-specific behavior; not a replacement for keyword extra descriptions. |
| `5` | Type | Select the item handler and type-dependent value prompts. |
| `6` | Extra flags | Toggle restrictions, behavior markers and special item state. |
| `7` | Wear flags | Select takeability and permitted equipment locations. |
| `8` | Weight | Item weight used by carrying, capacity and related checks. |
| `9` | Cost | Base object value used by shops and other mechanics; not a player's stall asking price. |
| `A` | Cost/Day | Legacy rent/upkeep value. Not the cost of renting a player stall. |
| `B` | Timer | Countdown used by timer triggers and type-specific expiry behavior. |
| `C` | Values | Run the item-type-specific value setup. **Immediately clears all 16 value slots first.** |
| `D` | Applies menu | Configure up to six numeric/typed equipment bonuses. |
| `E` | Extra descriptions | Keyword-based details players can examine. |
| `F` | Weapon Spells | Configure up to three spell procs. |
| `H` | Material | Select the object's material. |
| `I` | Size | Select object size; affects applicable equipment/capability checks. |
| `J` | Special Abilities | Attach structured weapon/armor abilities and their activation rules. |
| `K` | Activated Spells | Configure a rechargeable activated spell. |
| `M` | Min Level | Set the item level used by equipment checks, suggestions and item mechanics. |
| `P` | Perm Affects | Toggle first-bank character affects provided through equipment. |
| `V` | Perm2 Affects (AFF2) | Toggle second-bank character affects provided through equipment. |
| `R` | Mob Recipient | Restrict giving the object to a particular NPC vnum; `0` means unrestricted. Does not restrict giving to players. |
| `S` | Script | Attach object DG triggers. Disabled while editing an individual object with IEDIT. |
| `T` | Spellbook menu | Edit stored spellbook entries. |
| `U` | Restring ID | Set the object's restring identifier, separate from its ordinary targeting keywords. |
| `W` | Copy object | Copy another prototype into the current edit buffer; review all inherited settings. |
| `X` | Delete object | Confirm prototype deletion. This uses the deletion path immediately, rather than waiting for the later `Q` save prompt. |
| `Y` | Trap Setup | Create and configure a trap attached to this object. |
| `Z` | SpecProc | Select one registered object special procedure, or `0` for none. |
| `Q` | Quit | Finish editing and save/discard when prompted. |

The proficiency option `G` is commented out in the current main menu. Weapon
and armor subtype setup supplies relevant template information instead.
EQ Rating and Suggested affections are derived guidance, marked as under
development; save and reopen to refresh them. They are not hard balance rules.

### Text editing and practical limits

Multiline text uses `/s` to save the buffer, `/a` to abort, `/c` to clear, and
`/h` for the editor command list. Do not finish with a bare `~` as a substitute
for the displayed editor commands. Saving a text buffer does not save the whole
object.

Extra descriptions use keywords and description text, with a next-entry option.
Complete both fields before advancing. Clearing an entry's text and leaving
that submenu removes incomplete entries according to the editor's cleanup.

The numeric clamps currently include:

| Field | Accepted range |
| --- | --- |
| Weight | 0–1,000,000 |
| Cost | 0–2,000,000 |
| Cost/Day | 0–2,000,000 |
| General timer | 0–1,071,000 |
| Minimum level | 0–`LVL_IMPL` |
| Container capacity | -1–10,000; -1 means unlimited in the capacity prompt |
| Weapon damage dice when loaded | Capped at 2 dice, 12 sides per die |

Timers decrement during `point_update()`, once per MUD-hour tick, rather than
every combat pulse. Reaching zero invokes timer-trigger/type-specific logic.
A timer of zero is not a universal guarantee that every object type lives
forever: decaying portals, corpses and other special types have their own rules.
Lights also have a separate fuel-hours value.

## Menu numbering

Not every list uses the same convention:

| Menu | Numbering |
| --- | --- |
| Item type | Direct type ID, starting at 0. |
| Extra flags and wear flags | Code index + 1; `0` returns. Extra flags also accept `-1` to clear all. |
| Applies | Apply code + 1; `0` or `1` clears the selected slot. |
| Material | Direct material ID. `0` is displayed as unset but is rejected by the material parser. |
| Size | Size index + 1. |
| Permanent AFF/AFF2 | Direct displayed flag index, starting at 1; `0` returns. |
| Bonus type | Direct bonus-type ID, starting at 0. |
| Spell, skill and feat selections | The actual IDs shown by the relevant picker. |

Follow the displayed number for that specific submenu. For example, the extra
flag Can-Only-Equip-One is **105**, but its code index is **104**.

## Object types

Set the type before configuring values. Selecting a type clears values 0–3;
it does **not** clear all the higher slots. Entering `C) Values` clears all
values 0–15 and starts a fresh guided setup. Selecting Treasure Chest also
removes the Takeable wear flag.

The type list contains specialized and legacy handlers. A selectable name does
not guarantee that OEDIT provides a complete value wizard for it. Types lacking
an explicit wizard need an established working prototype and verification of
the consuming subsystem; avoid guessing value meanings.

| Type ID / menu choice | Constant | Label | Purpose |
| --- | --- | --- | --- |
| 0 | — | UNDEFINED | Unconfigured type; choose a real type before building gameplay behavior. |
| 1 | `ITEM_LIGHT` | Light | Light source with separate fuel hours. |
| 2 | `ITEM_SCROLL` | Scroll | Consumable multi-spell scroll. |
| 3 | `ITEM_WAND` | Wand | Charged spell wand. |
| 4 | `ITEM_STAFF` | Staff | Charged spell staff. |
| 5 | `ITEM_WEAPON` | Weapon | Weapon using the weapon subtype table, including modern ranged weapons. |
| 6 | `ITEM_FURNITURE` | Furniture | Furniture with an occupancy value. |
| 7 | `ITEM_FIREWEAPON` | Ranged-Weapon | Legacy ranged-weapon handler; prefer a supported modern Weapon subtype for new equipment. |
| 8 | `ITEM_TREASURE` | Treasure | Treasure/value object. |
| 9 | `ITEM_ARMOR` | Armor/Shield | Armor or shield using the armor subtype table. |
| 10 | `ITEM_POTION` | Potion | Consumable spell potion. |
| 11 | `ITEM_WORN` | Wearable | General wearable, including special worn-gear values. |
| 12 | `ITEM_OTHER` | Other | Miscellaneous object. |
| 13 | `ITEM_TRASH` | Trash | Trash; ordinary shop trade rejects trash types. |
| 14 | `ITEM_MISSILE` | Ammo | Ammunition for compatible ranged weapons. |
| 15 | `ITEM_CONTAINER` | Container | Container with capacity, flags and key. |
| 16 | `ITEM_NOTE` | Note | Written note; contextual action text may be used. |
| 17 | `ITEM_DRINKCON` | Liquid-Cont | Legacy liquid container. |
| 18 | `ITEM_KEY` | Key | Key object; its prototype vnum is referenced by locks. |
| 19 | `ITEM_FOOD` | Food | Food handled by food-duration/value mechanics. |
| 20 | `ITEM_MONEY` | Money | Gold coin object. |
| 21 | `ITEM_PEN` | Pen | Writing implement. |
| 22 | `ITEM_BOAT` | Boat | Boat item recognized by water-travel checks; not a complete registered vessel. |
| 23 | `ITEM_FOUNTAIN` | Fountain | Legacy liquid source. |
| 24 | `ITEM_CLANARMOR` | Clan-Armor | Legacy clan-restricted armor. |
| 25 | `ITEM_CRYSTAL` | Crafting Crystal | Crafting-system crystal. |
| 26 | `ITEM_ESSENCE` | Essence | Crafting-system essence. |
| 27 | `ITEM_MATERIAL` | Crafting Material | Crafting material object. |
| 28 | `ITEM_SPELLBOOK` | Spellbook | Wizard-style spellbook with separate spell entries. |
| 29 | `ITEM_PORTAL` | Portal | Portal using a selected destination mode. |
| 30 | `ITEM_PLANT` | Plant | Plant/travel-via-plants object. |
| 31 | `ITEM_TRAP` | Trap | Legacy trap object using numeric values; distinct from an attached Y-menu trap. |
| 32 | `ITEM_TELEPORT` | Teleport | Command-triggered teleport type; subsystem-specific setup. |
| 33 | `ITEM_POISON` | Poison | Weapon poison, distinct from Weapon Oil. |
| 34 | `ITEM_SUMMON` | Summon | Command-triggered summoning type; subsystem-specific setup. |
| 35 | `ITEM_SWITCH` | Switch | Pull/push mechanism controlling a room exit. |
| 36 | `ITEM_AMMO_POUCH` | Ammo-Pouch | Container for ammunition. |
| 37 | `ITEM_PICK` | Pick | Lock-picking tool type. |
| 38 | `ITEM_INSTRUMENT` | Instrument | Bardic instrument with skill modifier, level and breakability. |
| 39 | `ITEM_DISGUISE` | Disguise | Disguise kit type. |
| 40 | `ITEM_WALL` | Wall | Spell-created wall type; normally managed by spell behavior. |
| 41 | `ITEM_BOWL` | Bowl | Legacy recipe-mixing bowl. |
| 42 | `ITEM_INGREDIENT` | Ingredient | Ingredient for recipe/mixing systems. |
| 43 | `ITEM_BLOCKER` | Blocker | Directional movement blocker; requires matching subsystem configuration. |
| 44 | `ITEM_WAGON` | Wagon | Trade/resource wagon. |
| 45 | `ITEM_RESOURCE` | Resources | Wagon-trade resource object. |
| 46 | `ITEM_PET` | Pet | Shop pet object converted into a mobile follower on purchase. |
| 47 | `ITEM_BLUEPRINT` | Blueprint | Legacy/newcraft recipe ID reference. |
| 48 | `ITEM_TREASURE_CHEST` | Treasure Chest | Lootable chest with tier, type and DC settings. |
| 49 | `ITEM_HUNT_TROPHY` | Hunt Trophy | Hunt trophy/target marker used by the hunt system. |
| 50 | `ITEM_WEAPON_OIL` | Weapon Oil | Weapon oil; do not substitute it for a poison merely by naming it oil. |
| 51 | `ITEM_GEAR_OUTFIT` | Gear Outfit | Outfit crate that creates a weapon or armor set. |
| 52 | `ITEM_DRINK` | Drink | Current drink-effect type, separate from legacy liquid containers/fountains. |
| 53 | `ITEM_VEHICLE` | Vehicle | Vehicle-system object. |
| 54 | `ITEM_SHIP_OBJECT` | Ship-Object | Ship-object subsystem type. |
| 55 | `ITEM_VESSEL` | Vessel | Unified vessel-system object. |
| 56 | `ITEM_GREYHAWK_SHIP` | Greyhawk-Ship | Greyhawk ship object with interior room and ship index. |
| 57 | `ITEM_CRAFTING_TOOL` | Crafting-Tool | Craft/harvest tool with associated skill and bonus. |

## Values: what the numbers mean

The main menu displays **16 zero-based slots**, `value[0]` through `value[15]`.
The source's first value prompt is named `VALUE_1` but edits slot 0. These are
not the six applies, weapon-spell slots, activated-spell data, or attached traps:
those live in separate structures.

Before entering `C`, record existing values if you need them. There is no
non-destructive “just inspect the values” mode behind `C`. Changing weapon or
armor subtype calls template helpers that can overwrite weight, cost, material,
size, wear flags and base combat values. Set the subtype first; customize other
fields afterward. Returning to `C` repeats setup and clears prior enhancement
and other value data.

| Type | Important slot meanings / setup order |
| --- | --- |
| Light | Slot 2: fuel hours; 0 burnt out, -1 infinite. Slots 0 and 1 are skipped. |
| Scroll, Potion | Slot 0: caster level. Slots 1, 2, 3: up to **three** spells. Use 0 or -1 at a spell prompt for no spell. There is no fourth spell in slot 4. |
| Wand, Staff | Slot 0: caster level; 1: maximum charges; 2: charges remaining; 3: spell ID. Remaining-charge setup is clamped to 0–20. |
| Weapon | Slot 0: subtype; 1: damage dice count; 2: die size; 3: attack type; 4: enhancement. The normal wizard chooses the subtype, populates template values, then jumps to enhancement 0–10. It does not normally offer independent dice editing. |
| Armor | Slot 0: template/base armor information; 1: armor subtype; 4: enhancement 0–10. Choosing the subtype calls `set_armor_object()` and jumps to enhancement. |
| Clan Armor | Slot 1: clan ID. This is a separate legacy path, not the normal armor-subtype picker. |
| Ammo | Slot 0: ammunition category; 2: break probability, clamped to 2–98; 4: enhancement 0–10. The wizard skips the unused/other intermediate slots. |
| Legacy Ranged Weapon | The current wizard configures slot 0: ranged category; 1: dice count; 2: break probability, then returns without exposing the later attack/enhancement fields. Prefer supported modern Weapon subtypes for new bows/crossbows. |
| Container, Ammo Pouch | Slot 0: capacity (-1 unlimited); 1: container flag bitmask; 2: key vnum (-1 none). |
| Legacy Liquid Container, Fountain | Slot 0: maximum units (-1 unlimited); 1: initial units; 2: liquid type; 3: spell ID, 0 none. Follow the displayed liquid selection number. |
| Food, Drink | Slot 0 is prompted as duration in six-second rounds. The general value wizard does not configure all food/drink bonus structures; use the appropriate food/drink system or a verified prototype for those effects. |
| Money | Slot 0: number of gold coins. |
| Furniture | Slot 0: occupancy. |
| Worn | Slot 0: special worn-item enhancement value, currently clamped to 1–10; e.g. monk glove behavior. Not a universal substitute for applies. |
| Poison | Slot 0: poison spell; 1: poison level; 2: applications; 3: hits per application. Its type must actually be Poison. |
| Instrument | Slot 0: instrument category; 1: difficulty reduction 0–30; 2: instrument level 0–10; 3: breakability 0–2000. 0 is unbreakable; the prompt recommends 0–30 and describes 2000 as breaking on first use. |
| Portal | Slot 0: mode. Normal/check-flags modes use slot 1 for destination vnum; random mode uses slots 1 and 2 for lowest/highest destination vnums. Clan-hall mode selects the user's clan hall without a room argument. |
| Switch | Slot 0: 0 pull / 1 push; 1: affected room vnum; 2: direction 0 north, 1 east, 2 south, 3 west, 4 up, 5 down; 3: operation 0 unhide, 1 unlock, 2 open. |
| Legacy Trap | Values request trigger, applicable direction/object vnum, effect and DC. This is not the modern attached-trap data edited with `Y`. |
| Blueprint | Slot 0: craft recipe ID, clamped to 0–1000; references a recipe rather than defining its ingredients. |
| Treasure Chest | Slot 0: loot tier; 1: loot type; 2: random-load boolean; 3: search DC 0–80 (0 not hidden); 4: pick DC 0–80 (0 unlocked); 5: chest trap-effect selection (0 none). |
| Gear Outfit | Slots 0–2: outfit kind (weapon/armor set), enhancement, material. Slots 3–5: body apply/modifier/bonus type; 6–8 head; 9–11 arms; 12–14 legs. Typed feat, skill and spell-circle applies are refused in this wizard. |
| Greyhawk Ship | Slot 0: first interior room vnum; slot 1: ship index, as prompted. A ship object alone is not a fully configured vessel. |
| Crafting Tool | Slot 0: associated craft/harvest skill ID; 1: tool bonus; 2: optional quality/durability; 3: optional special flags. Use the matching tool wear slot too. |

For Boat, Key, Note, Other, Plant, Pen, Trash and Treasure, the first value menu
returns without a dedicated setup sequence. Several other specialized types,
including Weapon Oil and some vehicle types, lack a complete generic wizard.
Use working examples and their subsystem code rather than inventing values.
Item names do not change types: naming a vial “poison” does not configure poison
mechanics, and a poison is not automatically a Weapon Oil.

### Container flags

Inside the container value submenu, toggle each choice and enter `0` to continue
to the key prompt:

| Choice | Flag | Purpose |
| --- | --- | --- |
| 1 | `CONT_CLOSEABLE` | Container can be opened/closed. |
| 2 | `CONT_PICKPROOF` | Lock cannot be picked normally. |
| 3 | `CONT_CLOSED` | Container starts closed. |
| 4 | `CONT_LOCKED` | Container starts locked. Configure a coherent closed/closeable state and a valid key. |

These are bits stored in container slot 1, separate from object extra flags.
Unlike room door reset state, the container's closed/locked bits are item data.

### Treasure chest notes

Use the tier/type menus rather than treating the tier as a raw character level.
Random chests are extracted after looting and do not use the regular persistent
chest loot cooldown. Random placement needs the relevant room/zone setup.
Regular chests and chest generation have additional loot/cooldown behavior.

The chest's **minimum/item level** and its loot tier are separate fields. The
loot code caps generated equipment's item level at the chest level. Recheck
both tier and item level when building a chest. The legacy chest trap-effect
value is also separate from `Y) Trap Setup`; choose the intended system rather
than accidentally combining unrelated trap mechanisms.

## Wear flags

Wear flags permit locations; they do not add armor, proficiency, spells or
bonuses by themselves. Takeable is independent of equippable: normally enable
it on equipment intended for player inventories. A weapon should have Wield;
an instrument should use the Instrument slot rather than merely an appropriate
name. Instrument loading also adds that slot in the current loader.

Toggle with the **editor choice** below; enter `0` to leave.

| Editor choice | Code index | Flag | Equipment role |
| --- | --- | --- | --- |
| 1 | 0 | `ITEM_WEAR_TAKE` | Allows ordinary taking/getting; not an equipment slot. |
| 2 | 1 | `ITEM_WEAR_FINGER` | Finger equipment location. |
| 3 | 2 | `ITEM_WEAR_NECK` | Neck equipment location. |
| 4 | 3 | `ITEM_WEAR_BODY` | Body equipment location. |
| 5 | 4 | `ITEM_WEAR_HEAD` | Head equipment location. |
| 6 | 5 | `ITEM_WEAR_LEGS` | Legs equipment location. |
| 7 | 6 | `ITEM_WEAR_FEET` | Feet equipment location. |
| 8 | 7 | `ITEM_WEAR_HANDS` | Hands equipment location. |
| 9 | 8 | `ITEM_WEAR_ARMS` | Arms equipment location. |
| 10 | 9 | `ITEM_WEAR_SHIELD` | Shield equipment location. |
| 11 | 10 | `ITEM_WEAR_ABOUT` | About-Body equipment location. |
| 12 | 11 | `ITEM_WEAR_WAIST` | Waist equipment location. |
| 13 | 12 | `ITEM_WEAR_WRIST` | Wrist equipment location. |
| 14 | 13 | `ITEM_WEAR_WIELD` | Wield equipment location. |
| 15 | 14 | `ITEM_WEAR_HOLD` | Hold equipment location. |
| 16 | 15 | `ITEM_WEAR_FACE` | Face equipment location. |
| 17 | 16 | `ITEM_WEAR_AMMO_POUCH` | Ammo-Pouch equipment location. |
| 18 | 17 | `ITEM_WEAR_EAR` | Ears equipment location. |
| 19 | 18 | `ITEM_WEAR_EYES` | Eyes equipment location. |
| 20 | 19 | `ITEM_WEAR_BADGE` | Badge equipment location. |
| 21 | 20 | `ITEM_WEAR_INSTRUMENT` | Instrument equipment location. |
| 22 | 21 | `ITEM_WEAR_SHOULDERS` | Shoulders equipment location. |
| 23 | 22 | `ITEM_WEAR_ANKLE` | Ankle equipment location. |
| 24 | 23 | `ITEM_WEAR_SHEATH` | Sheath equipment location. |
| 25 | 24 | `ITEM_WEAR_CRAFT_SICKLE` | Craft/harvest tool slot: Gathering-Tool. |
| 26 | 25 | `ITEM_WEAR_CRAFT_AXE` | Craft/harvest tool slot: Forestry-Tool. |
| 27 | 26 | `ITEM_WEAR_CRAFT_KNIFE` | Craft/harvest tool slot: Hunting-Tool. |
| 28 | 27 | `ITEM_WEAR_CRAFT_PICKAXE` | Craft/harvest tool slot: Mining-Tool. |
| 29 | 28 | `ITEM_WEAR_CRAFT_ALCHEMY` | Craft/harvest tool slot: Alchemy-Tool. |
| 30 | 29 | `ITEM_WEAR_CRAFT_ARMOR_HAMMER` | Craft/harvest tool slot: Armorsmithing-Tool. |
| 31 | 30 | `ITEM_WEAR_CRAFT_JEWEL_PLIERS` | Craft/harvest tool slot: Jewelcrafting-Tool. |
| 32 | 31 | `ITEM_WEAR_CRAFT_NEEDLE` | Craft/harvest tool slot: Tailoring-Tool. |
| 33 | 32 | `ITEM_WEAR_CRAFT_WEAPON_HAMMER` | Craft/harvest tool slot: Weaponsmithing-Tool. |
| 34 | 33 | `ITEM_WEAR_ON_BACK` | On-Back equipment location. |

## Complete extra-flag reference

Extra flags are separate from wear flags, permanent character affects and
attached special-ability records. Toggle by **editor choice**, not code index.
`0` returns; `-1` clears all extra flags, including bookkeeping flags in the
edited copy. Avoid clearing everything on an established item casually.

Race/alignment and required-class flags affect supported eligibility checks;
anti-class flags are currently bypassed by their standard handler. These flags
are not a universal prohibition on carrying an item. Class predicates
and multiclass membership follow the game's class/race eligibility routines.
Multiple required-class flags can impose multiple requirements; they do not
form a menu of alternatives.

| Editor choice | Code index | Flag | Purpose and current behavior |
| --- | --- | --- | --- |
| 1 | 0 | `ITEM_GLOW` | Glow display and room-light contribution; distinct from a fueled Light item. |
| 2 | 1 | `ITEM_HUM` | Humming display marker; does not attach a spell or ability. |
| 3 | 2 | `ITEM_NORENT` | No-rent/persistence restriction used by rent/storage paths. Also refused by player stalls and auction transfer eligibility. |
| 4 | 3 | `ITEM_NODONATE` | Donation restriction; donation handling can dispose of an ineligible item. Not an ownership binding. |
| 5 | 4 | `ITEM_NOINVIS` | Prevents the object-invisibility spell path from making the item invisible. |
| 6 | 5 | `ITEM_INVISIBLE` | Object invisibility; visibility depends on the viewer's detection abilities. |
| 7 | 6 | `ITEM_MAGIC` | Magical-item marker used by displays, crafting and combat/equipment logic. Does not itself set enhancement, a spell, or an apply. |
| 8 | 7 | `ITEM_NODROP` | Cursed/no-drop handling restricts ordinary dropping and transfers. Not the same as a bound-player ID. |
| 9 | 8 | `ITEM_BLESS` | Blessed-item marker and blessing interaction; not a complete Holy weapon special ability. |
| 10 | 9 | `ITEM_ANTI_GOOD` | Anti-Good eligibility restriction, enforced by applicable alignment, class or race checks. |
| 11 | 10 | `ITEM_ANTI_EVIL` | Anti-Evil eligibility restriction, enforced by applicable alignment, class or race checks. |
| 12 | 11 | `ITEM_ANTI_NEUTRAL` | Anti-Neutral eligibility restriction, enforced by applicable alignment, class or race checks. |
| 13 | 12 | `ITEM_ANTI_WIZARD` | Intended anti-Wizard class restriction. The standard anti-class handler currently returns false before these checks, so it does not enforce this restriction. |
| 14 | 13 | `ITEM_ANTI_CLERIC` | Intended anti-Cleric class restriction. The standard anti-class handler currently returns false before these checks, so it does not enforce this restriction. |
| 15 | 14 | `ITEM_ANTI_ROGUE` | Intended anti-Rogue class restriction. The standard anti-class handler currently returns false before these checks, so it does not enforce this restriction. |
| 16 | 15 | `ITEM_ANTI_WARRIOR` | Intended anti-Warrior class restriction. The standard anti-class handler currently returns false before these checks, so it does not enforce this restriction. |
| 17 | 16 | `ITEM_NOSELL` | Ordinary shop purchase restriction; also refused by player stalls/auction eligibility. |
| 18 | 17 | `ITEM_QUEST` | Quest-item/special shop handling. Player stalls refuse quest items. Does not itself create or advance a quest. |
| 19 | 18 | `ITEM_ANTI_HUMAN` | Anti-Human eligibility restriction, enforced by applicable alignment, class or race checks. |
| 20 | 19 | `ITEM_ANTI_ELF` | Anti-Elf eligibility restriction, enforced by applicable alignment, class or race checks. |
| 21 | 20 | `ITEM_ANTI_DWARF` | Anti-Dwarf eligibility restriction, enforced by applicable alignment, class or race checks. |
| 22 | 21 | `ITEM_ANTI_HALF_TROLL` | Anti-Half Troll eligibility restriction, enforced by applicable alignment, class or race checks. |
| 23 | 22 | `ITEM_ANTI_MONK` | Intended anti-Monk class restriction. The standard anti-class handler currently returns false before these checks, so it does not enforce this restriction. |
| 24 | 23 | `ITEM_ANTI_DRUID` | Intended anti-Druid class restriction. The standard anti-class handler currently returns false before these checks, so it does not enforce this restriction. |
| 25 | 24 | `ITEM_MOLD` | Legacy crafting mold marker, used by mold/craft handling. |
| 26 | 25 | `ITEM_ANTI_CRYSTAL_DWARF` | Anti-Crystal Dwarf eligibility restriction, enforced by applicable alignment, class or race checks. |
| 27 | 26 | `ITEM_ANTI_HALFLING` | Anti-Halfling eligibility restriction, enforced by applicable alignment, class or race checks. |
| 28 | 27 | `ITEM_ANTI_H_ELF` | Anti-H Elf eligibility restriction, enforced by applicable alignment, class or race checks. |
| 29 | 28 | `ITEM_ANTI_H_ORC` | Anti-H Orc eligibility restriction, enforced by applicable alignment, class or race checks. |
| 30 | 29 | `ITEM_ANTI_GNOME` | Anti-Gnome eligibility restriction, enforced by applicable alignment, class or race checks. |
| 31 | 30 | `ITEM_ANTI_BERSERKER` | Intended anti-Berserker class restriction. The standard anti-class handler currently returns false before these checks, so it does not enforce this restriction. |
| 32 | 31 | `ITEM_ANTI_TRELUX` | Anti-Trelux eligibility restriction, enforced by applicable alignment, class or race checks. |
| 33 | 32 | `ITEM_ANTI_SORCERER` | Intended anti-Sorcerer class restriction. The standard anti-class handler currently returns false before these checks, so it does not enforce this restriction. |
| 34 | 33 | `ITEM_DECAY` | Marks objects for timer-based decay, including spell-created walls; configure the relevant timer/type. |
| 35 | 34 | `ITEM_ANTI_PALADIN` | Intended anti-Paladin class restriction. The standard anti-class handler currently returns false before these checks, so it does not enforce this restriction. |
| 36 | 35 | `ITEM_ANTI_RANGER` | Intended anti-Ranger class restriction. The standard anti-class handler currently returns false before these checks, so it does not enforce this restriction. |
| 37 | 36 | `ITEM_ANTI_BARD` | Intended anti-Bard class restriction. The standard anti-class handler currently returns false before these checks, so it does not enforce this restriction. |
| 38 | 37 | `ITEM_ANTI_ARCANA_GOLEM` | Anti-Arcana Golem eligibility restriction, enforced by applicable alignment, class or race checks. |
| 39 | 38 | `ITEM_FLOAT` | Prevents the dropped object from falling through fly-needed rooms. Does not grant its bearer flight. |
| 40 | 39 | `ITEM_HIDDEN` | Hidden-object/search marker used by visibility and discovery logic. Not the same as a treasure chest's search DC. |
| 41 | 40 | `ITEM_MAGLIGHT` | Magical light contribution from the item; distinct from the room magical-light flag. |
| 42 | 41 | `ITEM_NOLOCATE` | Intended anti-location marker. No direct runtime C check was found outside administrative flag handling; do not assume locate immunity. |
| 43 | 42 | `ITEM_NOBURN` | Intended anti-destruction marker. No direct runtime C check was found outside administrative flag handling; do not assume disintegration immunity. |
| 44 | 43 | `ITEM_TRANSIENT` | Intended vanish-on-drop marker. No direct runtime C check was found; do not rely on automatic extraction just from this bit. |
| 45 | 44 | `ITEM_AUTOPROC` | Enables periodic object special-procedure processing where the proc loop requires it. Attach a suitable procedure too. |
| 46 | 45 | `ITEM_FLAMING` | Active flaming weapon state used by the matching special ability. Configure its J-menu ability/activation; the flag is not a complete ability record. |
| 47 | 46 | `ITEM_FROST` | Active frost weapon state used by the matching special ability; configure its ability record too. |
| 48 | 47 | `ITEM_KI_FOCUS` | Allows applicable monk/ki attack paths with a wielded weapon; not a grant of every monk feat. |
| 49 | 48 | `ITEM_ANTI_WEAPONMASTER` | Intended anti-Weaponmaster class restriction. The standard anti-class handler currently returns false before these checks, so it does not enforce this restriction. |
| 50 | 49 | `ITEM_ANTI_DROW` | Anti-Drow eligibility restriction, enforced by applicable alignment, class or race checks. |
| 51 | 50 | `ITEM_MASTERWORK` | Masterwork marker. The attack-bonus path can supply the masterwork fallback when no enhancement bonus applies; not a stacking extra enhancement. |
| 52 | 51 | `ITEM_ANTI_DUERGAR` | Anti-Duergar eligibility restriction, enforced by applicable alignment, class or race checks. |
| 53 | 52 | `ITEM_SEEKING` | Seeking ranged-weapon state; the ranged concealment check bypasses concealment when active. |
| 54 | 53 | `ITEM_ADAPTIVE` | Adaptive weapon state; permits applicable ranged attacks to use the wielder's positive Strength bonus. |
| 55 | 54 | `ITEM_AGILE` | Agile light-weapon state; uses the supported Dexterity damage calculation when better than Strength, with weapon/attack restrictions. |
| 56 | 55 | `ITEM_CORROSIVE` | Active corrosive weapon state; requires the relevant special-ability processing for its effect. |
| 57 | 56 | `ITEM_DISRUPTION` | Active disruption weapon state; requires the relevant special-ability processing for its effect. |
| 58 | 57 | `ITEM_DEFENDING` | Defending weapon state used by the equipment/combat bonus calculations. |
| 59 | 58 | `ITEM_VICIOUS` | Active vicious weapon state. Its matching on-hit ability can damage both victim and wielder. |
| 60 | 59 | `ITEM_VORPAL` | Active vorpal weapon state; requires the matching ability and qualifying attack processing. |
| 61 | 60 | `ITEM_ANTI_LAWFUL` | Anti-Lawful eligibility restriction, enforced by applicable alignment, class or race checks. |
| 62 | 61 | `ITEM_ANTI_CHAOTIC` | Anti-Chaotic eligibility restriction, enforced by applicable alignment, class or race checks. |
| 63 | 62 | `ITEM_REQ_WIZARD` | Requires Wizard class membership through the class-required equipment check. |
| 64 | 63 | `ITEM_REQ_CLERIC` | Requires Cleric class membership through the class-required equipment check. |
| 65 | 64 | `ITEM_REQ_ROGUE` | Requires Rogue class membership through the class-required equipment check. |
| 66 | 65 | `ITEM_REQ_WARRIOR` | Requires Warrior class membership through the class-required equipment check. |
| 67 | 66 | `ITEM_REQ_MONK` | Requires Monk class membership through the class-required equipment check. |
| 68 | 67 | `ITEM_REQ_DRUID` | Requires Druid class membership through the class-required equipment check. |
| 69 | 68 | `ITEM_REQ_BERSERKER` | Requires Berserker class membership through the class-required equipment check. |
| 70 | 69 | `ITEM_REQ_SORCERER` | Requires Sorcerer class membership through the class-required equipment check. |
| 71 | 70 | `ITEM_REQ_PALADIN` | Requires Paladin class membership through the class-required equipment check. |
| 72 | 71 | `ITEM_REQ_RANGER` | Requires Ranger class membership through the class-required equipment check. |
| 73 | 72 | `ITEM_REQ_BARD` | Requires Bard class membership through the class-required equipment check. |
| 74 | 73 | `ITEM_REQ_WEAPONMASTER` | Requires Weaponmaster class membership through the class-required equipment check. |
| 75 | 74 | `ITEM_REQ_ARCANE_ARCHER` | Requires Arcane Archer class membership through the class-required equipment check. |
| 76 | 75 | `ITEM_REQ_STALWART_DEFENDER` | Requires Stalwart Defender class membership through the class-required equipment check. |
| 77 | 76 | `ITEM_REQ_SHIFTER` | Requires Shifter class membership through the class-required equipment check. |
| 78 | 77 | `ITEM_REQ_DUELIST` | Requires Duelist class membership through the class-required equipment check. |
| 79 | 78 | `ITEM_REQ_MYSTIC_THEURGE` | Requires Mystic Theurge class membership through the class-required equipment check. |
| 80 | 79 | `ITEM_REQ_ALCHEMIST` | Requires Alchemist class membership through the class-required equipment check. |
| 81 | 80 | `ITEM_REQ_ARCANE_SHADOW` | Requires Arcane Shadow class membership through the class-required equipment check. |
| 82 | 81 | `ITEM_REQ_SACRED_FIST` | Requires Sacred Fist class membership through the class-required equipment check. |
| 83 | 82 | `ITEM_REQ_ELDRITCH_KNIGHT` | Requires Eldritch Knight class membership through the class-required equipment check. |
| 84 | 83 | `ITEM_ANTI_ARCANE_ARCHER` | Intended anti-Arcane Archer class restriction. The standard anti-class handler currently returns false before these checks, so it does not enforce this restriction. |
| 85 | 84 | `ITEM_ANTI_STALWART_DEFENDER` | Intended anti-Stalwart Defender class restriction. The standard anti-class handler currently returns false before these checks, so it does not enforce this restriction. |
| 86 | 85 | `ITEM_ANTI_SHIFTER` | Intended anti-Shifter class restriction. The standard anti-class handler currently returns false before these checks, so it does not enforce this restriction. |
| 87 | 86 | `ITEM_ANTI_DUELIST` | Intended anti-Duelist class restriction. The standard anti-class handler currently returns false before these checks, so it does not enforce this restriction. |
| 88 | 87 | `ITEM_ANTI_MYSTIC_THEURGE` | Intended anti-Mystic Theurge class restriction. The standard anti-class handler currently returns false before these checks, so it does not enforce this restriction. |
| 89 | 88 | `ITEM_ANTI_ALCHEMIST` | Intended anti-Alchemist class restriction. The standard anti-class handler currently returns false before these checks, so it does not enforce this restriction. |
| 90 | 89 | `ITEM_ANTI_ARCANE_SHADOW` | Intended anti-Arcane Shadow class restriction. The standard anti-class handler currently returns false before these checks, so it does not enforce this restriction. |
| 91 | 90 | `ITEM_ANTI_SACRED_FIST` | Intended anti-Sacred Fist class restriction. The standard anti-class handler currently returns false before these checks, so it does not enforce this restriction. |
| 92 | 91 | `ITEM_ANTI_ELDRITCH_KNIGHT` | Intended anti-Eldritch Knight class restriction. The standard anti-class handler currently returns false before these checks, so it does not enforce this restriction. |
| 93 | 92 | `ITEM_SHOCK` | Active shocking weapon state; configure the matching special ability. |
| 94 | 93 | `ITEM_ANTI_LICH` | Anti-Lich eligibility restriction, enforced by applicable alignment, class or race checks. |
| 95 | 94 | `ITEM_ANTI_VAMPIRE` | Anti-Vampire eligibility restriction, enforced by applicable alignment, class or race checks. |
| 96 | 95 | `ITEM_VAMPIRE_ONLY` | Vampire-only eligibility restriction. |
| 97 | 96 | `ITEM_REQ_WARLOCK` | Requires Warlock class membership through the class-required equipment check. |
| 98 | 97 | `ITEM_ANTI_WARLOCK` | Intended anti-Warlock class restriction. The standard anti-class handler currently returns false before these checks, so it does not enforce this restriction. |
| 99 | 98 | `ITEM_SET_STATS_AT_LOAD` | Rebuild weapon/armor template stats during load; can overwrite customized fields, restore enhancement, zero enhancement for very low-cost items, and clamp level to 1–30. Use deliberately. |
| 100 | 99 | `ITEM_EXTRACT_AFTER_USE` | Single-use key handling extracts a key after applicable lock use. Not a universal consume-after-any-command flag. |
| 101 | 100 | `ITEM_NOSAC` | Prevents applicable sacrifice/salvage paths. Not a general invulnerability flag. |
| 102 | 101 | `ITEM_DOWNGRADED` | Bookkeeping that the downgrade action has already been applied; do not use as a normal power bonus. |
| 103 | 102 | `ITEM_IDENTIFIED` | Marks the item identified. Player stall stocking requires identification. |
| 104 | 103 | `ITEM_CRAFTED` | Crafted-item marker used by display and crafting-related handling; does not build a craft recipe. |
| 105 | 104 | `ITEM_ONLY_EQUIP_ONE` | Prevents equipping another copy of the SAME prototype/rnum if either copy has the flag. Does not restrict every item with this flag as one shared category. |
| 106 | 105 | `ITEM_ONLY_POSSES_ONE` | Intended one-copy possession restriction. No direct runtime C check was found; do not promise that this bit enforces inventory uniqueness. |
| 107 | 106 | `ITEM_CRAFTING_SMELTER` | Smelter station marker used by applicable refining recipes. |
| 108 | 107 | `ITEM_CRAFTING_LOOM` | Loom station marker used by applicable crafting/refining checks. |
| 109 | 108 | `ITEM_CRAFTING_FORGE` | Forge station marker used by applicable crafting checks. |
| 110 | 109 | `ITEM_CRAFTING_ALCHEMY_LAB` | Alchemy lab station marker used by applicable crafting/refining checks. |
| 111 | 110 | `ITEM_CRAFTING_JEWELCRAFTING_STATION` | Jewelry station marker used by applicable crafting checks. |
| 112 | 111 | `ITEM_CRAFTING_TANNERY` | Tannery station marker used by applicable crafting checks. |
| 113 | 112 | `ITEM_CRAFTING_CARPENTRY_TABLE` | Carpentry station marker used by applicable crafting checks. |
| 114 | 113 | `ITEM_TRAPPED` | Legacy trapped marker. Attached traps use actual trap records; setting this flag alone does not create one. |
| 115 | 114 | `ITEM_ACCOUNT_EXP` | Special shop currency marker: account experience pricing/handling, not ordinary gold pricing. |
| 116 | 115 | `ITEM_REFORGEABLE` | Allows selection by the reforge system. Does not define the reward or recipe on its own. |
| 117 | 116 | `ITEM_ARTISANPOINTS` | Special shop currency marker: artisan-point pricing/handling. |
| 118 | 117 | `ITEM_BUTCHERED` | Corpse-processing bookkeeping: marks a corpse already butchered. |
| 119 | 118 | `ITEM_NO_SALVAGE` | Prevents supported crafting salvage operations. |

## Applies and bonus types

`D) Applies menu` has **six** slots. Each entry stores an apply location,
modifier, bonus type and, when needed, a specific skill/class/feat subtype.

1. Choose slot 1–6 (`0` returns).
2. Choose the apply from the numbered list. `0` or `1` clears that slot.
3. Enter the modifier, or the feat ID for Grant-Feat.
4. Choose the bonus type by its displayed direct ID.
5. For a skill bonus, select the skill. For a spell-circle bonus, select the
   spellcasting class.

Grant-Feat uses the **feat ID as its modifier**, not a numeric rank bonus. The
picker only permits item-valid feats. For typed feats, `random_feat_specific()`
assigns a subtype, such as weapon family, skill or spell school. The apply menu
shows that subtype. There is no ordinary follow-up picker for choosing it
manually in this path. Check the result, especially for weapon feats: Weapon
Focus with the wrong weapon family will not benefit the intended attack.

Positive modifiers improve most beneficial stats; penalties are possible in
supported paths. Use **Armor-Class (`APPLY_AC_NEW`)** for modern AC bonuses,
not the deprecated `APPLY_AC`. The general apply picker shows code + 1, so
Armor-Class is editor choice **28**, Grant-Feat **50**, and Skill-Bonus **51**.

Bonuses follow their apply's supported stacking rules, equipped-item handling
and character caps. An item advertised as +4 does not necessarily raise a
capped total by four. A displayed suggestion is guidance, not automatic
validation of every possible item build. Avoid duplicate applies and reserved
locations; below-implementor duplicate checks also exist in the editor.

| Apply menu choice | Code index | Constant | Meaning |
| --- | --- | --- | --- |
| 1 | 0 | `APPLY_NONE` | Clear/no apply. |
| 2 | 1 | `APPLY_STR` | Strength modifier through the corresponding character/equipment calculation. |
| 3 | 2 | `APPLY_DEX` | Dexterity modifier through the corresponding character/equipment calculation. |
| 4 | 3 | `APPLY_INT` | Intelligence modifier through the corresponding character/equipment calculation. |
| 5 | 4 | `APPLY_WIS` | Wisdom modifier through the corresponding character/equipment calculation. |
| 6 | 5 | `APPLY_CON` | Constitution modifier through the corresponding character/equipment calculation. |
| 7 | 6 | `APPLY_CHA` | Charisma modifier through the corresponding character/equipment calculation. |
| 8 | 7 | `APPLY_CLASS` | Reserved; not a way to grant a class. |
| 9 | 8 | `APPLY_LEVEL` | Reserved; not a way to grant character levels. |
| 10 | 9 | `APPLY_AGE` | Age modifier through the corresponding character/equipment calculation. |
| 11 | 10 | `APPLY_CHAR_WEIGHT` | Weight modifier through the corresponding character/equipment calculation. |
| 12 | 11 | `APPLY_CHAR_HEIGHT` | Height modifier through the corresponding character/equipment calculation. |
| 13 | 12 | `APPLY_PSP` | Max-PSP modifier through the corresponding character/equipment calculation. |
| 14 | 13 | `APPLY_HIT` | Max-HP modifier through the corresponding character/equipment calculation. |
| 15 | 14 | `APPLY_MOVE` | Max-Move modifier through the corresponding character/equipment calculation. |
| 16 | 15 | `APPLY_GOLD` | Reserved; not a passive gold generator. |
| 17 | 16 | `APPLY_EXP` | Reserved; not a passive experience generator. |
| 18 | 17 | `APPLY_AC` | Deprecated/unused legacy AC location; use APPLY_AC_NEW. |
| 19 | 18 | `APPLY_HITROLL` | Hitroll modifier through the corresponding character/equipment calculation. |
| 20 | 19 | `APPLY_DAMROLL` | Damroll modifier through the corresponding character/equipment calculation. |
| 21 | 20 | `APPLY_SAVING_FORT` | Save-Fortitude modifier through the corresponding character/equipment calculation. |
| 22 | 21 | `APPLY_SAVING_REFL` | Save-Reflex modifier through the corresponding character/equipment calculation. |
| 23 | 22 | `APPLY_SAVING_WILL` | Save-Will modifier through the corresponding character/equipment calculation. |
| 24 | 23 | `APPLY_SAVING_POISON` | Save-Poison modifier through the corresponding character/equipment calculation. |
| 25 | 24 | `APPLY_SAVING_DEATH` | Save-Death modifier through the corresponding character/equipment calculation. |
| 26 | 25 | `APPLY_SPELL_RES` | Spell-Resist modifier through the corresponding character/equipment calculation. |
| 27 | 26 | `APPLY_SIZE` | Size modifier through the corresponding character/equipment calculation. |
| 28 | 27 | `APPLY_AC_NEW` | Modern armor class bonus; bonus type determines the applicable AC category. |
| 29 | 28 | `APPLY_RES_FIRE` | Resist-Fire modifier through the corresponding character/equipment calculation. |
| 30 | 29 | `APPLY_RES_COLD` | Resist-Cold modifier through the corresponding character/equipment calculation. |
| 31 | 30 | `APPLY_RES_AIR` | Resist-Air modifier through the corresponding character/equipment calculation. |
| 32 | 31 | `APPLY_RES_EARTH` | Resist-Earth modifier through the corresponding character/equipment calculation. |
| 33 | 32 | `APPLY_RES_ACID` | Resist-Acid modifier through the corresponding character/equipment calculation. |
| 34 | 33 | `APPLY_RES_HOLY` | Resist-Holy modifier through the corresponding character/equipment calculation. |
| 35 | 34 | `APPLY_RES_ELECTRIC` | Resist-Electric modifier through the corresponding character/equipment calculation. |
| 36 | 35 | `APPLY_RES_UNHOLY` | Resist-Unholy modifier through the corresponding character/equipment calculation. |
| 37 | 36 | `APPLY_RES_SLICE` | Resist-Slashing modifier through the corresponding character/equipment calculation. |
| 38 | 37 | `APPLY_RES_PUNCTURE` | Resist-Piercing modifier through the corresponding character/equipment calculation. |
| 39 | 38 | `APPLY_RES_BLUDGEON` | Resist-Bludgeoning modifier through the corresponding character/equipment calculation. |
| 40 | 39 | `APPLY_RES_SOUND` | Resist-Sound modifier through the corresponding character/equipment calculation. |
| 41 | 40 | `APPLY_RES_POISON` | Resist-Poison modifier through the corresponding character/equipment calculation. |
| 42 | 41 | `APPLY_RES_DISEASE` | Resist-Disease modifier through the corresponding character/equipment calculation. |
| 43 | 42 | `APPLY_RES_NEGATIVE` | Resist-Negative modifier through the corresponding character/equipment calculation. |
| 44 | 43 | `APPLY_RES_ILLUSION` | Resist-Illusion modifier through the corresponding character/equipment calculation. |
| 45 | 44 | `APPLY_RES_MENTAL` | Resist-Mental modifier through the corresponding character/equipment calculation. |
| 46 | 45 | `APPLY_RES_LIGHT` | Resist-Light modifier through the corresponding character/equipment calculation. |
| 47 | 46 | `APPLY_RES_ENERGY` | Resist-Energy modifier through the corresponding character/equipment calculation. |
| 48 | 47 | `APPLY_RES_WATER` | Resist-Water modifier through the corresponding character/equipment calculation. |
| 49 | 48 | `APPLY_DR` | Damage-reduction apply, subject to its equipment and cap rules. |
| 50 | 49 | `APPLY_FEAT` | Grant the feat identified by modifier; typed feats also use specific subtype data. |
| 51 | 50 | `APPLY_SKILL` | Skill bonus of modifier to the selected skill in specific. |
| 52 | 51 | `APPLY_SPECIAL` | Special-purpose apply; use only with a verified consumer. |
| 53 | 52 | `APPLY_POWER_RES` | Power-Resist modifier through the corresponding character/equipment calculation. |
| 54 | 53 | `APPLY_HP_REGEN` | HP-Regen modifier through the corresponding character/equipment calculation. |
| 55 | 54 | `APPLY_MV_REGEN` | MV-Regen modifier through the corresponding character/equipment calculation. |
| 56 | 55 | `APPLY_PSP_REGEN` | PSP-Regen modifier through the corresponding character/equipment calculation. |
| 57 | 56 | `APPLY_ENCUMBRANCE` | Encumbrance modifier through the corresponding character/equipment calculation. |
| 58 | 57 | `APPLY_FAST_HEALING` | Fast-Healing modifier through the corresponding character/equipment calculation. |
| 59 | 58 | `APPLY_INITIATIVE` | Initiative modifier through the corresponding character/equipment calculation. |
| 60 | 59 | `APPLY_ELDRITCH_SHAPE` | Eldritch shape-related apply; verify the intended warlock subsystem consumer. |
| 61 | 60 | `APPLY_ELDRITCH_ESSENCE` | Eldritch essence-related apply; verify the intended warlock subsystem consumer. |
| 62 | 61 | `APPLY_SPELL_CIRCLE_1` | Additional spell-circle 1 slots for the spellcasting class selected in specific. |
| 63 | 62 | `APPLY_SPELL_CIRCLE_2` | Additional spell-circle 2 slots for the spellcasting class selected in specific. |
| 64 | 63 | `APPLY_SPELL_CIRCLE_3` | Additional spell-circle 3 slots for the spellcasting class selected in specific. |
| 65 | 64 | `APPLY_SPELL_CIRCLE_4` | Additional spell-circle 4 slots for the spellcasting class selected in specific. |
| 66 | 65 | `APPLY_SPELL_CIRCLE_5` | Additional spell-circle 5 slots for the spellcasting class selected in specific. |
| 67 | 66 | `APPLY_SPELL_CIRCLE_6` | Additional spell-circle 6 slots for the spellcasting class selected in specific. |
| 68 | 67 | `APPLY_SPELL_CIRCLE_7` | Additional spell-circle 7 slots for the spellcasting class selected in specific. |
| 69 | 68 | `APPLY_SPELL_CIRCLE_8` | Additional spell-circle 8 slots for the spellcasting class selected in specific. |
| 70 | 69 | `APPLY_SPELL_CIRCLE_9` | Additional spell-circle 9 slots for the spellcasting class selected in specific. |
| 71 | 70 | `APPLY_SPELL_POTENCY` | Percentage spell-potency modifier, through supported spell scaling. |
| 72 | 71 | `APPLY_SPELL_DC` | Spell-DC modifier through the corresponding character/equipment calculation. |
| 73 | 72 | `APPLY_SPELL_DURATION` | Percentage spell-duration modifier, through supported duration calculations. |
| 74 | 73 | `APPLY_SPELL_PENETRATION` | Spell-Penetration modifier through the corresponding character/equipment calculation. |
| 75 | 74 | `APPLY_MOVE_SPEED` | Movement-speed percentage modifier; this is not a room movement-cost override. |
| 76 | 75 | `APPLY_RES_FORCE` | Resist-Force modifier through the corresponding character/equipment calculation. |

### Bonus-type list

Use the displayed **direct ID**, not ID + 1. Same-type bonuses commonly use
the strongest applicable value, with category-specific exceptions. An Untyped
item apply is converted to Enhancement by the object load path when its modifier
is nonzero, so do not rely on it remaining untyped.

| ID | Bonus type |
| --- | --- |
| 0 | Untyped |
| 1 | Alchemical |
| 2 | Armor |
| 3 | Circumstance |
| 4 | Competence |
| 5 | Deflection |
| 6 | Dodge |
| 7 | Enhancement |
| 8 | Inherent |
| 9 | Insight |
| 10 | Luck |
| 11 | Morale |
| 12 | Natural Armor |
| 13 | Profane |
| 14 | Racial |
| 15 | Resistance |
| 16 | Sacred |
| 17 | Shield |
| 18 | Size |
| 19 | Trait |
| 20 | Food |
| 21 | Drink |
| 22 | Eidolon |
| 23 | Universal |
| 24 | Alchemist Quintessential |

## Permanent affects: P and V

See the [AFF/AFF2 flag guide](AFFECT_FLAGS_GUIDE.md) for every flag and its
current behavior. In particular, stored AFF2 equipment vectors are not merged
into the wearer's live second-bank flags by the current common handler; do not
assume a V-menu setting is active simply because it is saved or displayed.

These menus store character affect flags on the item, separate from its extra
flags. Use `P` for the original AFF bank and `V` for AFF2. Enter the flag number
**as displayed**; these menus do not use the extra-flag index + 1 rule. Enter
`0` to leave. The original AFF picker explicitly refuses Charm.

Examples of first-bank gameplay affects include Detect Invisible, Detect Magic,
Water Walk, Flying, Underwater Breathing, Climb and Sanctuary. The lists also
contain combat modes, harmful states and internal flags. A listed flag is not
necessarily appropriate as permanent equipment: toggling a condition bit does
not recreate all the linked events, durations, saves, resources or cleanup of
the spell/ability that normally supplies it. Use supported item abilities or
scripts when an effect requires more than a passive flag.

Do not confuse:

- `ITEM_INVISIBLE`: the object is invisible.
- A permanent Invisible character affect: equipment grants invisibility to its
  wearer through affect handling.
- An activated Invisibility spell: the player explicitly casts the item effect.

## Weapon spell procs: F

The current limit is **three** `wpn_spells` entries, not four.

1. Select spell slot 1–3.
2. Select a spell by its ID. At this prompt, `-1` clears the selected slot.
3. Set a cast level of at least 1.
4. Set the chance from **1–50 percent**.
5. Choose `1` for combat-only/offensive or `0` for random/defensive processing.

The prompt describes a chance per processing round, not a guarantee on every
weapon strike. These are separate from `J) Special Abilities`, `K) Activated
Spells`, and the spells in scroll/wand value slots. Test the intended trigger
and target with the equipped item.

## Structured special abilities: J

This menu edits actual ability records. An elemental extra flag often
represents an ability's active state; it does not replace that record.

| Choice | Action |
| --- | --- |
| `N` | Add a new ability record. |
| `E` | Choose an existing record by its displayed position to edit. |
| `C` | Clear **all** assigned abilities. This is not a single-ability delete. |
| `Q` | Return to the main menu. |

Within an ability record:

| Choice | Setting |
| --- | --- |
| `A` | Ability type from the registered ability list. |
| `L` | Ability level, prompted as 1–34. |
| `M` | Activation method flags: On Wear, On Use, Command Word, On Hit, On Crit. Toggle the displayed choices; `0` returns. |
| `C` | Command word, when that activation method needs one. |
| `V` | Ability-specific values, such as Bane NPC race/player subrace. |
| `Q` | Return to the ability list. |

Different abilities consume different activation methods and values. For a
weapon effect that can be switched on and then triggered on a hit, configure
both the appropriate activation behavior and hit processing. Do not assume
arbitrary method combinations work just because they can be toggled.

## Activated spells: K

The item has one activated-spell configuration, separate from its weapon procs:

1. Set the caster level **1–30**, or enter **0** to erase the activation.
2. Choose the spell by its displayed ID.
3. Set maximum uses **1–5**.

The configured system recharges uses on its own five-minute-per-use cooldown.
Current uses and recharge state are runtime data. The maximum-use setting does
not create wand charges in `value[1]`/`value[2]`; those are different mechanics.

## Spellbooks: T

Spellbooks have **200** stored spell entries. The menu groups existing spells
by wizard circle, shows their entry positions, and suggests the next empty
slot. Select an entry, then choose the spell ID; `0` clears a spell. Page usage
is computed by the spellbook parser rather than entered as a free-form OEDIT
value. A book's stored spells are not the same as a scroll's three spell slots.

## Attached traps: Y

`Y) Trap Setup` edits the modern trap record attached to an object, including a
container. It is separate from the legacy `ITEM_TRAP` type, chest trap-effect
values, and the `ITEM_TRAPPED` extra bit.

When no trap exists, `1` creates an **average spike trap** triggered by opening
the object/container. Once attached, the menu is:

| Choice | Setting | Meaning |
| --- | --- | --- |
| `1` | Remove trap | Delete the attached trap. |
| `2` | Trigger | Open object/container, unlock object/container, or get object. |
| `3` | Trap type | Select the damage/effect type from its displayed list. |
| `4` | Severity | Select the severity from its displayed list. |
| `5` | Detect DC | Difficulty of finding the trap. |
| `6` | Disarm DC | Difficulty of disarming it. |
| `7` | Save DC | Difficulty of the victim's saving throw. |
| `0` | Back | Return to the main object menu. |

Trigger choices use their displayed 1–3 selections. Trap-type and severity
lists use their displayed zero-based IDs. Custom DCs are clamped to at least 1.
Unlike the current REDIT directional-trap recreation path, OEDIT preserves
custom detect/disarm/save DCs when changing trap type or severity.

A trigger must fit the item: an opening trap on an item with no opening action
will not necessarily fire. Use a get-object trigger for an ordinary pickup
trap, or a closeable/locked container for appropriate opening/unlocking traps.

## Materials and size

Material and size can affect equipment, crafting, damage/repair, and special
capability checks. Picking a weapon/armor subtype can replace material and
other template fields, so do that first. Set the final material afterward.

The material picker shows direct IDs, including `0) not-set`, but its parser
requires a value **greater than 0**. Avoid deliberately leaving a functional
item unset. Materials are not the same as the separate craft-project material
IDs or station flags.

### Material IDs

| ID | Material label |
| --- | --- |
| 0 | not-set |
| 1 | cotton |
| 2 | leather |
| 3 | glass |
| 4 | gold |
| 5 | organic |
| 6 | paper |
| 7 | steel |
| 8 | wood |
| 9 | bone |
| 10 | crystal |
| 11 | ether |
| 12 | adamantine |
| 13 | mithril |
| 14 | iron |
| 15 | copper |
| 16 | ceramic |
| 17 | satin |
| 18 | silk |
| 19 | dragonhide |
| 20 | burlap |
| 21 | velvet |
| 22 | platinum |
| 23 | obsidian |
| 24 | wool |
| 25 | onyx |
| 26 | ivory |
| 27 | brass |
| 28 | marble |
| 29 | bronze |
| 30 | pewter |
| 31 | ruby |
| 32 | sapphire |
| 33 | emerald |
| 34 | gemstone |
| 35 | granite |
| 36 | stone |
| 37 | energy |
| 38 | hemp |
| 39 | diamond |
| 40 | earth |
| 41 | silver |
| 42 | alchemical silver |
| 43 | cold iron |
| 44 | darkwood |
| 45 | dragonscale |
| 46 | dragonbone |
| 47 | sea ivory |
| 48 | tin |
| 49 | coal |
| 50 | dragonmetal |
| 51 | ash wood |
| 52 | maple wood |
| 53 | mahogany |
| 54 | valenwood |
| 55 | ironwood |
| 56 | linen |
| 57 | zinc |
| 58 | flax |
| 59 | dragonblood |
| 60 | bamboo |
| 61 | pine wood |
| 62 | tamarisk wood |
| 63 | spruce wood |
| 64 | fir wood |
| 65 | willow wood |
| 66 | beech wood |
| 67 | teak wood |
| 68 | birch wood |
| 69 | acacia wood |
| 70 | cypress wood |
| 71 | cedar wood |
| 72 | oak wood |
| 73 | larch wood |
| 74 | mesquite wood |
| 75 | ebony wood |
| 76 | live oak wood |
| 77 | driftwood |
| 78 | dwarf willow wood |
| 79 | jute |
| 80 | agave fiber |

### Size choices

Use the displayed choice (index + 1). RESERVED is a sentinel, not an intended
normal equipment size.

| Editor choice | Code index | Size |
| --- | --- | --- |
| 1 | 0 | RESERVED |
| 2 | 1 | Fine |
| 3 | 2 | Diminutive |
| 4 | 3 | Tiny |
| 5 | 4 | Small |
| 6 | 5 | Medium |
| 7 | 6 | Large |
| 8 | 7 | Huge |
| 9 | 8 | Gargantuan |
| 10 | 9 | Colossal |

## Scripts, special procedures, and population

`S) Script` opens DG object-trigger editing. Use triggers intended for object
owners and their supported variables. Prototype scripts are separate from room
or mobile scripts; IEDIT explicitly refuses editing an individual object's
script list through this menu.

`Z) SpecProc` selects a registered C special procedure. Use `0` to clear it and
`Q` to return. A procedure that needs periodic processing can also need
`ITEM_AUTOPROC`; setting that flag alone does not attach the procedure.

Creating a prototype does not place it in the world, put it in a shop's stock,
or make a mobile carry it. Configure zone resets, mobile equipment/inventory,
shop production, loot tables or scripts through their corresponding systems.
Similarly, a crafting station flag does not define a recipe, a Pet type does
not define a mobile, and a vessel object does not define every interior room or
persistent vessel record.

Use `W` for a variant of a known working item, then inspect all inherited flags,
applies, spells, values, descriptions and scripts before saving. Deletion with
`X` immediately invokes the prototype-deletion routine after confirmation; do
not assume discarding later editor changes will undo it.

## Recommended building workflow

1. Choose an unused object vnum in your assigned zone and open `oedit <vnum>`.
2. Set the type. For weapons/armor, run `C` and select the subtype and
   enhancement before customizing other fields.
3. Set keywords and descriptions, final material/size/weight, minimum level,
   value and wear slots.
4. Add appropriate extra flags and six-slot applies. Use the required skill,
   class or generated feat subtype data where applicable.
5. Add spells, structured abilities, an attached trap or scripts only when
   the intended behavior needs them.
6. Save with `Q`, then `Y`. If only saved to memory, run
   `oedit save <zone-number>`.
7. Load a test copy and inspect it with staff object tools and player `lore`.
   Test the actual equip/use action with an eligible mortal character.

### Example: an enhanced sword with a Strength bonus

- Set type **5) Weapon**, then use `C` to choose a sword subtype and +2
  enhancement. This supplies its base weapon data and Takeable/Wield slots.
- Set its name, descriptions, level, final material, and cost afterward.
- In `D`, select an empty apply slot. Select **2) Strength**, enter modifier
  `2`, then choose **7) Enhancement** from the bonus-type picker.
- If adding Weapon Focus, use **50) Grant-Feat**, select the actual feat ID
  from the item-valid list, and inspect the randomly assigned weapon-family
  subtype. Verify that it matches the intended use.
- Save the object and zone as needed, then test its attack bonus, equipment
  requirements and Strength stacking. The sword's +2 weapon enhancement and
  its +2 Strength enhancement are different bonuses to different calculations.

## Current limitations and common mistakes

- Do not confuse type IDs with one-based extra/wear/apply choices.
- Entering `C` clears all 16 values immediately; it is not a harmless inspection.
- Selecting a type clears only the first four slots, leaving possible stale
  higher values until configured.
- Weapon/armor subtype helpers reset wear flags and other properties. Review
  custom settings after subtype selection.
- Anti-class flags are currently bypassed by the early return in
  `is_class_anti_object()`. Required-class checks are separate and remain active.
- Can-Only-Equip-One applies to copies of the same prototype. The separately
  named Can-Only-Possess-One has no direct runtime enforcement found.
- No-Locate, No-Burn and Transient have intended names but no direct runtime
  flag checks found outside administrative handling. Do not rely on them as
  implemented protections.
- A magical flag, a permanent affect, a numeric apply, a weapon proc and a
  special ability are different data. One does not automatically fill the others.
- There are six apply slots, three weapon-spell slots, and 200 spellbook slots.
- A general object timer is not measured in six-second combat rounds. Food's
  value-duration prompt and light fuel are separate settings.
- Item EQ ratings and suggested modifiers do not replace actual balance or
  eligibility testing.
- OEDIT saves can update live copies. Use care when changing an existing vnum
  already used for customized player equipment.

## Source references

- [oedit.c](../../src/oedit.c): syntax, menus, value prompts, parsing, traps,
  applies, spells and editor save handling.
- [structs.h](../../src/structs.h): item types, wear/extra flags, applies and limits.
- [constants.c](../../src/constants.c): labels, materials, sizes and bonus types.
- [oasis.h](../../src/oasis.h): object editor bounds.
- [genobj.c](../../src/genobj.c): prototype updates, live-copy updates and deletion.
- [db.c](../../src/db.c): item loading, instrument slots and stats-at-load handling.
- [treasure.c](../../src/treasure.c): weapon/armor template initialization and loot.
- [handler.c](../../src/handler.c): equipment applies, stacking and one-copy equip check.
- [class.c](../../src/class.c) and [race.c](../../src/race.c): class/race restrictions.
- [act.item.c](../../src/act.item.c): wearing, giving, using and other item commands.
- [feats.c](../../src/feats.c): item-valid feats and typed feat subtype assignment.
- [spec_abilities.c](../../src/spec_abilities.c): structured abilities and activation.
- [spells.h](../../src/spells.h): activated-spell limits and recharge configuration.
- [limits.c](../../src/limits.c): timer processing and type-specific expiry.
- [traps_new.c](../../src/traps_new.c): attached trap behavior.
- [crafting_new.c](../../src/crafting_new.c): tools, stations, salvage and reforge.
- [player_shop.c](../../src/player_shop.c): player-stall item eligibility and trading.
