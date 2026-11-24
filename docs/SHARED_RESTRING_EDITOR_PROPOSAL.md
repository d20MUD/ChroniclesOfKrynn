# Shared clan and religion restring editor

**Status:** Implemented in religion phase 3; see [the usage and persistence guide](RELIGION_PHASE3.md).
This document records the shared editor and player-service requirements.

## Scope

One catalogue supports Clan Favor rewards and Religion Standing rewards. Staff
maintain approved text through a menu editor. Eligible players apply those
entries to existing items for a level-based gold fee.

The service changes an individual item's descriptions and clears **all of its
extra descriptions**. It never creates replacement gear, changes combat stats,
or modifies the shared object prototype.

## Staff editor

Proposed entry command: `restringedit`, restricted to authorized builders/staff.
Ordinary players can browse/apply approved entries but cannot edit the catalogue.

```text
Restring Menu:
1) Clans
2) Religion

Q) Quit
```

Choosing Clans:

```text
Clan Restring Ranks:
1) Clan Favor Rank 1
2) Clan Favor Rank 2
3) Clan Favor Rank 3
4) Clan Favor Rank 4
5) Clan Favor Rank 5

B) Back
Q) Quit
```

Use the clan system's supported Favor standings, not clan officer ranks or
legacy personal clan points. Religion lists Religion Rank 1 through Religion
Rank 8, with the church rank names alongside the numbers where useful. The
menus derive supported ranks from the relevant systems rather than inventing
another rank ladder.

Choosing a rank shows entries for that category/rank. Each row identifies its
specific clan or deity, so the editor can maintain many organizations:

```text
Religion Rank 3 Restrings:
ID    Deity       Position  Match          Short description
1042  Reorx       Body      Heavy armor    ...
1043  Mishakal    Neck      Symbol style   ...

A) Add new
E) Edit existing <ID>
D) Delete existing <ID>
F) Filter by clan/deity
B) Back
Q) Quit
```

Filter is a convenience within the same rank menu; category → rank → entries
remains the main navigation. Empty lists still offer Add New. Show IDs and
paginate long lists. List drafts separately from published entries so unfinished
text cannot accidentally become a player reward.

### Adding or editing an entry

1. Choose the specific clan or deity from the existing identity list.
2. Confirm the category and rank inherited from the menu.
3. Select a supported wear position by name.
4. Choose the item matching rule appropriate to that position/type.
5. Edit the appearance text and preview the result.
6. Save as a draft, or validate and publish.

```text
Edit Restring #1042:
1) Clan / Deity
2) Required Rank
3) Wear Position
4) Armor Category / Shield Type / Weapon Family
5) Keywords
6) Short Description
7) Room Description
8) Inspection Description
9) Preview

S) Save Draft
P) Publish
B) Back / discard unsaved changes
```

Changing wear position clears incompatible matching selections and requires
revalidation. Do not retain a weapon family on a body-armor entry, for example.
Warn before discarding unsaved text. Editing an existing published entry uses a
draft revision; the published version remains usable until the new one is saved
and published successfully.

### Item matching choices

| Item/position | Builder choices | How application matches |
| --- | --- | --- |
| Armor clothing/armor positions | Clothing, Light, Medium, Heavy | Native armor category of the target item; a heavy item cannot use a clothing appearance. |
| Shield | Buckler, Small Shield, Large Shield, Tower Shield | Native shield subtype, not the wearer's highest armor proficiency. |
| Wielded weapon | Existing weapon families, including Heavy-Blade, Axe, Hammer, Light-Blade, Polearm, Ranged, Double-Weapon and Monk where supported | The weapon table's family ID; do not infer family from the item's name or current attack mode. |
| Other supported positions | Slot-specific cosmetic entry | The actual item's type and compatible wear flags, without assigning fake armor or weapon statistics. |

Use the game's live native lists for weapon families and shield types, including
any supported families beyond these examples. Unsupported or nonsensical
combinations are unavailable rather than silently coerced.

An actual shield uses the shield selector, and a weapon uses the weapon-family
selector; neither is classified as ordinary Clothing/Light/Medium/Heavy merely
because it is carried in an equipment slot. Document how non-armor clothing is
classified using native item types and wear flags.

For main-hand/off-hand/two-hand weapons and duplicate neck slots, store explicit
supported wear positions or a deliberate normalized slot group. The editor must
show what the entry matches. Never treat wear-bit indexes and equipment-location
indexes as interchangeable.

The religion wardrobe still supports four visual styles of neck holy symbols
per deity/rank, alongside its clothing/light/medium/heavy sets. For a neck item
without an armor classification, those are **appearance variants**, not invented
armor categories on the item. If several eligible variants match, let the player
choose from a preview menu; do not guess or always select the first record.

### Text, publication and deletion

Each entry stores keywords, a short description, a room description, and one
primary inspection description. Follow existing string length/color rules and
preview visible text. Changing the optional primary inspection text is separate
from preserving old keyword-based extra descriptions: those are always cleared
when the service is applied.

If the engine stores all inspection text through extra-description records,
provide the catalogue's primary inspection text through an explicit supported
primary-description field/override. Do not quietly retain or rebuild old extra
description records and claim they were erased. Determine the native storage
mapping during implementation and expose only supported fields in the editor.

Publishing requires a real clan/deity, valid rank, compatible wear position and
matcher, valid required text, and a stable catalogue ID. Never publish arbitrary
scripts or object-prototype changes through description fields.

Delete asks for confirmation, identifying the entry and its preview. A deleted
entry disappears from future applications, while items already restrung keep
their descriptions. Retain an audit record for staff; no player item depends on
the catalogue row remaining present. If multiple published styles share a match,
show a selection menu rather than allowing nondeterministic database row order
to decide the applied appearance.

## Player service

Proposed syntax, preserving the current craft-kit `restring` command:

```text
restringapply <item> clan <rank>
restringapply <item> religion <rank>
```

Support normal item disambiguation and multiword or quoted item names according
to the game's parser. The rank is numeric and the type must be explicit. Do not
reinterpret this as a request to rewrite a prototype or edit catalogue text.

The player selects an owned item currently carried or worn. No remote items,
items worn by someone else, room objects, clan storage, or pet equipment are
edited through this command. Religion clothing examples may target currently
worn pieces, but the shared service also accepts compatible carried equipment.

### Eligibility and lookup

1. Find and validate the exact target item, retaining its unique identity.
2. Determine the player's current clan or deity from the authoritative system.
3. For clan entries, require accepted membership in that specific clan and
   personal **Favor standing** at least equal to the requested rank.
4. For religion entries, require current devotion to that specific deity and
   Church Standing rank at least equal to the requested rank.
5. Match published entries at **exactly the requested category, organization and
   rank**, using the item's type, wear compatibility and subtype/family.
6. Build player options only from those matching published entries. Omit any
   rank or wear-slot option with no eligible entries; do not show empty or
   disabled placeholders. If one entry matches, show it. If several match,
   show a short selection and preview menu. If none match, report that no
   authored appearance is available and do nothing.

Staff editors still show all supported ranks and slots, including missing
entries, so builders can fill gaps. Player menus hide missing content. A church
can launch with a partial wardrobe; missing ranks or slots do not block earned
rank advancement. Revalidate direct requests and confirmations against current
published entries, and never charge for a removed or missing match.

An eligible higher-ranked character can request a lower-rank appearance. There
is no automatic fallback to a lower rank or a different clan/deity. The player
must deliberately request the desired rank. No Standing or Favor is consumed.

For a carried item compatible with several supported wear positions, ask the
player to select a matching position if necessary, listing only positions that
have at least one eligible published appearance for the requested rank. For a worn item, start with
its actual equipment location. Do not silently move or equip it as part of the
service.

Application is outside combat at an authorized restring service NPC. An
appropriately configured clan quartermaster or temple priest can provide it.
Require the service to support the requested reward category. Membership alone
does not make the command a remote customization service anywhere in the world.

### Gold price

Recommended initial shared formula:

`cost = 100 × max(1, item level)` gold per item.

| Item level | Gold cost |
| ---: | ---: |
| 1 | 100 |
| 5 | 500 |
| 10 | 1,000 |
| 20 | 2,000 |
| 30 | 3,000 |

Use the target item's authoritative native level, not character level, clan
rank, church rank, armor category, item sale value or the template's rank.
Check numeric bounds before multiplication. Treat ordinary valid level-zero
items as level one; reject invalid/corrupt levels rather than allowing overflow
or a negative fee. Keep one configurable gold-per-level rate for this service,
not independent prices on thousands of text entries.

The fee is a starting design value to confirm before implementation. This new
catalogue service does not change the existing craft-kit customization formula.

### Preview and confirmation

Show:

- Exact item identity, level and current description.
- Category, specific clan/deity, requested rank, matched wear position and
  armor category/shield type/weapon family.
- New descriptions and keywords.
- Gold price.
- **All existing extra descriptions on this item will be erased.**
- A clear confirm/cancel choice.

On confirmation, recheck item identity, ownership, eligibility, item matcher,
service location, available gold and template revision. An item swap, rank
change, edit/delete of the catalogue record, or price change invalidates the
quote and requires a fresh preview. Do not apply different text or charge a
new price under an old confirmation.

### Safe application

- Change the chosen object instance's supported appearance fields and stamp the
  catalogue entry's membership requirement on that instance. Religion entries set
  `religion_requirement.deity` and `.rank` to the entry's deity and temple rank.
  Clan Favor entries set `clan_requirement.clan` and `.favor` to the entry's clan
  and Favor rank; Favor ranks are distinct from the existing officer `.rank`.
  Preserve requirements from the other category and any existing officer gate.
  Preview the resulting requirements alongside the description and price.
- Purchases and wear require the same organization and at least the stamped
  rank. If both clan and religion requirements exist, both must pass. Check the
  resulting requirements before applying to an already worn item.
- Remove its entire extra-description list, including prototype-derived entries,
  without modifying or freeing shared prototype descriptions used by other items.
- Record an explicit empty-extra-description override if needed by object-save
  loading; merely saving no extra-description rows must not cause prototype text
  to reappear on login, death recovery, container loading or pet saving.
- Preserve vnum, unique identity, level, type, wear flags, armor/shield subtype,
  weapon family, enchantments, material, stats, ownership and scripts. Preserve
  restrictions except for the explicitly previewed category requirement above.
- Charge gold only for a successfully persisted change. Handle inventory and
  equipment saves using the existing checked save paths; failure restores the
  old item text, extra descriptions, membership requirements and gold.
- Ensure the item mutation and debit cannot be split by disconnect, repeated
  confirmation or a crash. Use an idempotent application record/journal if the
  native save mechanism cannot commit both together; do not rely solely on
  compensating in-memory refunds for crash safety.
- A request that would leave the same descriptions and already-empty extra list
  unchanged, with identical membership requirements, reports that no change is needed and charges no fee. If extra
  descriptions remain, clearing them is a real change and is included in preview.

Restringing does not grant clan membership, church rank, special equipment
permissions, buffs or admission through blockers. Existing protected-item
restrictions still apply. A completed item's appearance and membership requirements
remain on it after transfer or a change of faith/clan. Transfer does not remove
purchase or wear gates; future applications recheck current eligibility.

## Minimal data model and implementation boundaries

Catalogue entry: stable ID, reward category, clan/deity ID, required reward rank,
wear position/group, matching kind and value, optional visual variant label,
appearance fields, publication state, revision and audit timestamps/editor ID.

Existing item-instance storage persists the applied descriptions and explicit
extra-description clearing. Optional application provenance can record the
catalogue ID/revision for diagnostics, but rendering must use the saved item
text rather than a live lookup that changes whenever staff edit the catalogue.

The shared catalogue and clan application can be implemented independently.
Religion application remains unavailable until actual devotion/Standing checks
exist; an existing deity choice alone must not be treated as earned rank eight.
The editor can still prepare religion entries in advance.

## Validation before release

- Menu add/edit/delete, drafts, publication, rank navigation, filters and empty lists.
- Wrong clan/deity, insufficient Favor/Standing, inactive membership and invalid ranks.
- All armor categories, native shield subtypes and weapon families; mismatched
  positions and items with several valid wear positions; duplicate neck slots.
- Exact-rank lookup, alternate styles, missing entries and disabled templates.
- Correct level-based fees, insufficient gold, repeated confirmations, changed
  quotes and no-op requests.
- Shared prototype descriptions remain untouched when one item loses extras.
- Changed text and intentionally cleared extras survive equipment/inventory,
  nested-container, pet, logout, death and restart save/load paths.
- Checked-save failure and interrupted/crashed application do not lose items,
  charge twice, provide unpaid changes or restore deleted extra descriptions.
