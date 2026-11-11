# Krynn neighbourhood housing and auction house

This ports the d20StarWars neighbourhood housing and fixed-price auction house
into Krynn's C engine. Prices use **gold**. Housing and auction records live in
MySQL; stored objects use Krynn's object record format, preserving item values,
custom text, typed affections, nested contents, and sheathed weapons.

## Installation and builder setup

Build the complete server and restart it with the new binary. Startup creates
all twelve estate tables automatically. `sql/20261009_housing_auction.sql` also
provides the schema for administrators who prefer to create tables separately.
The optional help migration is `sql/20261009_housing_auction_help.sql`.

No existing world files or live database records were changed during the port.
Choose existing rooms for neighbourhood entrances and create neighbourhoods:

```text
hcontrol neighborhood add 2200 Palanthas Homes
hcontrol neighborhood list
hcontrol houses
hcontrol houses <neighborhood-id>
```

The entrance VNUM must exist and be unique. Each character may own one house per
neighbourhood. Owners use stable character IDs; names are display labels.

In **medit** or **redit**, assign the **MySQL Auction House** special procedure
to the auctioneer mobile or auction room and save that world edit normally.
There are no hardcoded Star Wars room or mobile VNUMs. Players use
`ah <command>` or `auctionhouse <command>` at the configured location.
All auction actions require this prefix, including `list`, `view`, `stats`,
`sell`, `buy`, `cancel`, `recover`, `reprice`, and `collect`.

Staff at `LVL_GRSTAFF` can use the auctionhouse command remotely.
Existing fixed-room houses and their guest commands remain available.
The existing administrative commands work directly or with the
`hcontrol legacy` prefix.

## Housing commands

At a neighbourhood entrance:

```text
house list
house buy My Home
house enter <house-id>
```

Inside a neighbourhood house:

```text
house info
house name The Rosewood Retreat
house guest <player-name>
house build north
house title A Comfortable Sitting Room
house description Sunlight falls across comfortable chairs and a low table.
house extra painting A bright landscape hangs on the wall.
house leave
```

`name` (also `rename`) changes the house's name for free from inside the house.
Names must be 1–100 characters and appear in `house info` and `house list`.
This is separate from `title`, which changes the current room's title.

`guest` toggles access. Guests may enter and explore, but only the owner or
high-level staff can modify the house or use chest storage. Room creation adds
both directions of the exit. Cardinal directions, diagonals, up, and down are
supported. Diagonal movement follows the server's existing diagonal setting.
Krynn has ten directions, so use `house leave` to return to the neighbourhood
entrance. On a Forgotten Realms build with twelve directions, the entrance room
also has an `out` exit.

## Chests

```text
house chest buy
house chest list
house chest keywords <chest-id> lockbox
house chest short <chest-id> a carved oak lockbox
house chest room <chest-id> A carved oak lockbox rests against the wall.
house chest move <chest-id>
house chest contents <chest-id>
put dagger lockbox
get dagger lockbox
look in lockbox
```

`move` relocates a chest to the owner's current house room. Customize or use a
chest from the room containing it. Chest IDs are also keywords; normal numeric
selectors (`2.lockbox`, `2.dagger`) and `all` / `all.<keyword>` work.
For compatibility, `house chest deposit <id> <item>` and
`house chest withdraw <id> <item>` are also supported. Stored item IDs printed
by the contents command can identify a specific item for withdrawal.

Chests are fixed, unlimited storage fixtures. Contents are saved immediately
in `housing_chest_items`. Nested objects and sheath slots remain part of the
stored object tree. Nonpersistent items, money objects, and cursed/nodrop items
cannot be deposited. Bound items can remain in their owner's house storage;
they cannot be auctioned.

## Prices

| Operation | Gold |
| --- | ---: |
| Buy house | 100,000 |
| Build room | 50,000 |
| Change room title | 500 |
| Change room description | 1,000 |
| Add extra description | 750 |
| Buy chest | 10,000 |
| Change chest text | 500 |

Prices are configurable per neighbourhood in `housing_neighborhoods`. Room
titles allow 200 characters, room/extra descriptions 4,000, extra keywords 100,
and chest text 255. House and neighbourhood names allow 100 characters.

## Auctions

```text
auctionhouse list all
auctionhouse list weapons dagger
auctionhouse list body
auctionhouse list on-back
auctionhouse list mine
auctionhouse view <listing-id>
auctionhouse sell dagger 15000
auctionhouse reprice <listing-id> 12000
auctionhouse buy <listing-id>
auctionhouse cancel <listing-id>
auctionhouse recover <expired-listing-id>
auctionhouse collect
```

This is a fixed-price market, separate from Krynn's existing auction chat
channel. Listings last fourteen days; prices must be 1–99,999,999 gold.
`list` supports item types, wear slots, and case-insensitive keyword searches.
Expired listings remain visible to their seller through `list mine` and can be
recovered. Cancellation, recovery, and repricing require the original seller.
Items must be identified, transferable, and unbound, including their contents.
The buyer must have sufficient gold and carrying capacity and cannot buy their
own listing. Seller proceeds are queued in MySQL and collected at login or with
`auctionhouse collect`. Proceeds that would overflow the gold balance stay
pending until there is room in that balance.

## Persistence and recovery

Permanent housing tables:

- `housing_neighborhoods`, `housing_houses`, `housing_rooms`
- `housing_room_exits`, `housing_room_extra_descriptions`, `housing_guests`
- `housing_storage_chests`, `housing_chest_items`

Auction tables: `auction_listings`, `auction_credit_settlements`.
Transfer journals: `estate_outgoing`, `estate_deliveries`.

Objects entering escrow receive a stable transfer identity that is saved in the
player's inventory before the escrow commit. On login, committed outgoing
transfers remove the exact saved object if the previous inventory save was
interrupted. Incoming transfers retain their object payload until the object
is confirmed in the saved inventory and its debit receipt is confirmed in the
player file. Seller payout receipts are saved with the gold balance before the
SQL acknowledgement. An incoming item stays in journal storage if its save or acknowledgement fails,
so it cannot be given away while still pending. A pending transfer blocks further
estate transactions until recovery succeeds.

The `EId` object tag preserves transfer identities in inventory files and SQL
object records. Player-file tags `ECrd` and `EDbt` store currency receipts;
`Hous` and `Hrom` store the permanent house and room IDs. Existing player files
without these tags continue to load normally.

House rooms receive temporary VNUMs above the existing world (starting at
2,000,000). These VNUMs are never written into zone files or persistent player
login locations. Saved login locations use the neighbourhood entrance, and the
permanent house/room IDs restore the character inside the house after restart.
The periodic house pass releases empty layouts using Krynn's room reference
reindexing machinery. Layouts with occupants, loose floor objects, or active
room effects remain loaded.

As in the Star Wars system, **loose floor objects are not persistent storage**.
Use chests for valuables. This port adapts the requested in-game systems;
Star Wars browser widgets and its separate consumable farming subsystem are
not imported.

## Validation

```sh
cmake --build build-agent-check --target circle -j4
python3 tests/run_estate_integration.py
```

The integration test starts a disposable MariaDB instance with networking
disabled. It never connects to the game's database. It tests neighbourhood
purchases, duplicate ownership, reciprocal room exits, text changes, guests,
chest access, nested and sheathed objects, layout reload, category/wear searches,
auction escrow, purchase ownership, cancellation, expiration, repricing,
payout replay, and interrupted incoming/outgoing transfers. It also checks paused
DG room scripts after dynamic room allocation/release. It runs with both
a lightweight game adapter and Krynn's actual object record writer/parser,
including SQL persistence of apostrophes, multiline text, typed affections,
item values, binding/proficiency, timers, weapon effects, complete special-ability
lists, and transfer identities. SQL inventory reloads are checked as well. Full live-server playtesting remains a
post-installation check.

## Owner room sectors and layout reuse

Use `house sector <type>` in a room you own. Allowed values are `Inside`, `City`,
`Field`, `Forest`, `Water (Swim)` (also `water` or `swim`), `Underwater`, and `Cave`.
Inside, Underwater and Cave set `ROOM_INDOORS`; the other choices remove only that
flag. The house flag and other room settings remain intact. Guests cannot edit
sectors. Invalid choices list the available values without changing the room.

Sectors are persisted in `housing_room_settings`; existing rooms default to Inside.
The normal estate bootstrap creates this table when permitted. Install
`sql/20261111_housing_room_sectors.sql` before startup if the runtime database user
has no DDL permission. Master and fresh-install schemas include the table as well.
No live database was modified while implementing this change.

`house build <direction>` now searches the connected house layout before charging
for a room. Positions derive from cardinal/diagonal/up/down exits, separately from
public-zone rooms and other houses. When an existing room occupies the target
position, the command saves a two-way connection instead of creating or charging
for another room. An existing matching reverse exit keeps its runtime door/key
settings; conflicting exits are not overwritten. Ambiguous or contradictory maps
are refused. Existing forward exits report that they already exist. New room
construction retains its normal price and behavior. Linking exits is transactional
and survives leaving/re-entering and house materialization.

Validation: disposable estate SQL integration tests cover all sectors and indoor
flag transitions, invalid/guest edits, rehydration from SQL, cardinal/diagonal/floor
room reuse, free linking with zero gold, saved exits and existing reverse settings.
`tests/run_house_geometry.py` tests the production geometry/sector helpers with
ASan/UBSan, including ambiguity, inconsistent cycles and other-house isolation.
Make and CMake builds pass. No live migration or game restart was performed.

## Discounted room-description corrections

The first `house description <text>` customization in each room costs the normal
neighborhood description price. Subsequent description edits in that room cost
10% of the current normal price, rounded up (minimum 1 gold for a nonzero price;
a free neighborhood remains free). For example, a 1,000-gold first edit is followed
by 100-gold corrections. Titles and extra descriptions retain their existing prices.
Successful description edits report the amount charged.

`housing_room_description_edits` permanently records that the room has been
customized, even if its text is later reset to the default. Rooms customized before
this feature also qualify automatically when their stored description differs from
the unfinished-room default. Another untouched room still pays its own first-edit
price. The text update and eligibility marker commit together; failed saves do not
change runtime text or deduct gold. Existing owner and affordability checks remain.

The estate bootstrap and installation schemas include the new table. For database
users without DDL privileges, install `sql/20261111_housing_description_edits.sql`
before startup. No live schema changes were made during implementation.

Disposable SQL tests cover first/repeat prices, separate rooms, old customized
rooms, re-entry, default-text restoration, invalid/unaffordable attempts and rollback
of a forced marker-write failure. Make and CMake builds pass.
