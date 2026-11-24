# Religion phase 3: appearances and blessings

Phase 3 adds the shared clan/religion appearance editor, paid instance restrings,
and one cumulative church blessing. The expanded privilege catalogue remains
deferred. Existing craft-kit `restring` customization is preserved.

## Install

Apply `sql/20261123_religion_phase3.sql` to an existing database before starting
this build if its SQL account cannot create tables. The full installation schema
and `estate_init` include the same three InnoDB tables:

- `religion_blessings`: source deity, application rank, absolute UTC expiry and
  shared next-application deadline, keyed by character ID.
- `restring_catalogue`: stable entry ID, published revision and independent draft.
- `restring_audit`: editor ID, action, revision, complete text and timestamp.

Apply `sql/20261123_religion_phase3_help.sql` for SQL-backed help.

The optional **Krynn-only** appearance seed is
`examples/religion_phase3/wardrobes.sql`. It contains all 4,704 entries: 21 deities,
eight ranks, four styles, seven positions. Its stable IDs start above 300000;
`INSERT IGNORE` preserves any existing builder changes at those IDs. Rebuilding
it with `python3 examples/religion_phase3/build_wardrobes.py` uses the reviewed
per-deity themes and rank-specific tailoring in that directory. The seed does
not install temple rooms, create equipment, or change existing player items.
Entries bearing a different campaign's deity name fail publication/matching.

These are authored text options; no additional object prototypes are needed.
Missing or invalid published content is unavailable to players, while builders
can inspect every rank and its missing coverage. Neck entries provide four
visual variants for non-armor holy symbols. Armor neck pieces can have additional
native armor-category entries authored through the editor.

## Builder appearance editor

Run `restringedit` as a builder. Ordinary menu input is captured until `Q`:

1. Select `1` Clans or `2` Religion.
2. Select a Favor rank (1–5) or temple rank (1–8).
3. Use `A` to add, `E <id>` to edit, `D <id>` to delete, `F <org-id>` to filter,
   `N`/`V` to page, or `C <deity-id>` to inspect all 8×4×7 wardrobe cells.
4. In the entry menu, fields `1`–`8` edit organization, rank, position, matcher,
   keywords, short, room and inspection text. Field `9` previews the result.
5. `S` saves a draft; `P` validates and publishes. A saved draft leaves the
   previous published revision available. Deletion and unsaved discards require
   confirmation. Clan changes also require `can_edit_clan` authorization.

Positions accept the displayed canonical name or numeric **equipment position**.
The two neck equipment locations deliberately share the `neck` group; main hand,
offhand and two-hand weapons remain distinct. Changing position clears the
matcher. Supported matcher inputs are:

- `armor 0`, `armor 1`, `armor 2`, `armor 3`: native clothing/light/medium/heavy.
  Non-armor `ITEM_WORN` clothing matches clothing only.
- `symbol 0` through `symbol 3`: four non-armor neck appearance variants.
- `shield <subtype>`: native buckler/small/large/tower subtype IDs shown by the menu.
- `weapon <family-id>`: the menu lists every live native weapon family.
- `other <item-type>`: actual non-weapon, non-armor item type and compatible wear flag.

Inspection text uses multiple lines ending in `@`; `-` clears it. It is separate
from keyword extra descriptions, which are always erased on application.
Required text and native lengths are validated before publication. No script
or prototype edits are accepted through this editor.

## Player appearance service

At a registered priest of the character's faith, outside combat and crafting:

```
restringapply robe religion 2
restringapply "blue robe" religion 2
restringapply 2.robe religion 2
```

At a configured clan quartermaster:

```
restringapply sword clan 3
```

Clan ranks here mean personal Favor, not officer rank. Accepted membership and
Favor use the authoritative clan access checks. Temple ranks use current faith
and Church Standing. Higher ranks may deliberately select a lower-rank entry.
Standing/Favor is not spent.

The target must be carried or worn. Only published exact-rank matches are
listed. Multiple styles or carried-item positions require `restringapply choose
<id>`. A worn item matches its actual position. No empty player slots, automatic
rank fallback, remote inventory, pet equipment, or room items are offered.
Spellbooks and filled bags retain their existing customization restrictions.

The preview shows the item identity/level, replacement text, matcher, resulting
membership requirements and fee. Use `restringapply confirm` within two minutes
or `restringapply cancel`. Confirmation rechecks the item snapshot, ownership,
rank, catalogue revision, service location, fee and gold. A changed quote must
be previewed again.

The initial price is **100 × max(1, item level)** gold, using item levels 0–1000
and checked arithmetic. Optionally place `{"gold_per_level":100}` in the game's
`etc/restring.json` to change the single shared rate. An invalid configuration
makes applications unavailable. Exact no-op applications charge nothing.

Religion appearances stamp deity and temple rank. Clan appearances stamp clan
and Favor while preserving the officer gate. The other category's restrictions
remain. Purchases (including nested contents) and wear enforce both categories.
Appearance text remains after transfer or departure; it grants no membership.

Only instance text and the previewed requirement change. All keyword extra
descriptions are removed without touching prototypes. An explicit empty-extra
marker and a separate inspection override survive every native object codec.
The outgoing-item escrow and fee-bearing delivery commit in one transaction.
The durable `RsPn` player marker prevents commands from exposing an unresolved
application after a crash; ordered estate receipts prevent duplicate charges.
Recovery normally preserves the live item's equipment position, identity and
scripts; normal restart recovery may return pending equipment to inventory.

## Church blessing

```
religion blessing
religion bless
religion bless confirm
religion bless cancel
religion bless remove
```

`blessing` shows the next full package for current rank, locked future packages,
active application rank, remaining seconds and next availability. `bless`
previews one self-only package at an affiliated registered priest. Confirmation
is valid for two minutes and rechecks the priest, temple, deity and rank.

Every component lasts **86,400 real seconds**, with one shared application
cooldown across all temples and faiths. There is no Standing cost. Removal or
supported dispelling removes the whole package and retains the cooldown.
Death retains only the original expiry. Logout never pauses or renews it.
Rank-up applies on the next application; rank correction trims the existing
package without extending it. Leaving/changing faith removes it immediately.

| Rank | Saves | AC | Max HP | Potency | Duration | Weapon attack | DR | Max move | Initiative |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 1 | 1 | 0 | 0 | 0% | 0% | 0 | 0 | 0 | 0 |
| 2 | 1 | 1 | 0 | 0% | 0% | 0 | 0 | 0 | 0 |
| 3 | 2 | 1 | 15 | 0% | 0% | 0 | 0 | 0 | 0 |
| 4 | 2 | 2 | 20 | 5% | 5% | 0 | 0 | 0 | 0 |
| 5 | 3 | 2 | 25 | 5% | 5% | 1 | 0 | 0 | 0 |
| 6 | 3 | 3 | 30 | 7% | 10% | 1 | 1/– | 0 | 0 |
| 7 | 4 | 3 | 35 | 7% | 10% | 2 | 2/– | 50 | 0 |
| 8 | 4 | 4 | 40 | 10% | 15% | 2 | 3/– | 75 | 2 |

Rows are complete packages, not sums of prior rows. Saves use resistance, AC
uses deflection, and weapon attack uses morale under the native stacking/caps.
Weapon morale excludes rays, psionic attacks and eldritch blasts. DR competes
with native weapon DR and fills only a shortfall beyond existing flat DR.
Potency extends the existing potency calculation once. Duration affects positive
finite spell durations, excluding device applications and permanent affects.
HP and movement are maximums only: application never heals or refills them;
removal clamps current values to reduced maximums without causing death.

## Authored temple extension

`examples/religion_phase3` contains a complete optional replacement for the phase
2 zone 699 pilot. Install this version **instead of** the phase 2 copy after
checking vnum availability. It includes both existing churches plus public
service annexes, rank-3 archives, rank-6 chambers and rank-8 sanctums. Every gated
inward passage has a matching priest/keeper and a free outward path. Protected
rooms prohibit teleport, summon and recall access. Four additional public
service task templates can replace existing board slots at the next weekly
snapshot; the four-choice-per-faith limit remains.

The SQL seed and world examples are staged content, not live deployment.

## Verification

The CMake build passes. Tests cover native armor/shield/all weapon-family
matching, deliberate neck grouping, typed resistance stacking and best DR,
prototype isolation, base-stat serialization, every blessing row, expiry and
cooldown recovery, rank corrections, faith changes, draft/publication/deletion,
clan Favor stamps with preserved officer/religion gates, fees and stale quotes,
full-length inspection codec round trips, failed writes and a simulated crash
immediately after application commit. The disposable MariaDB run also imports
and reapplies the migration, all 4,704 wardrobe entries, and SQL help. Both pilot
versions pass native quest parsing and teleport checks. No live server or world
indexes were changed.
