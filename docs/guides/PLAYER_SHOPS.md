# Player stalls

Players rent fixed-price stalls in staff-designated rooms. The market continues
operating while owners are offline. The `stall` command contains all actions;
it does not require changes to existing shopkeeper commands or mobile prototypes.
An occupied stall appears in the room contents with its merchant, name, and
browse command. The merchant is a persistent stall display rather than an
attackable mobile. Multiple stalls can share a room.

## Opening a market

Senior staff (`LVL_GRSTAFF` or higher) configure stalls:

```
stall admin add 2200 5
stall admin add 6530 3 2000 30
stall admin set 1 1500 25
stall admin disable 1
stall admin enable 1
stall admin history 1
stall admin evict 1
```

`add <room vnum> <count> [weekly rent] [capacity]` creates independently rented
slots. Count is 1–100 per invocation, capacity is 1–200, and weekly rent is
1–99,999,999 gold. The defaults are 1,000 gold and 20 listings. `set` changes
future rental/renewal charges and the stock limit. Reducing capacity preserves
existing stock but prevents additions until below capacity. Disable suspends
sales, rentals and owner customization/renewal; owners can still withdraw
items and collect earnings. Evict releases the account and slot immediately,
preserving all stock and proceeds. Staff can browse the directory and inspect
the transaction history for a stall.

No market is automatically installed in a live room. Run `admin add` at the
locations you want to use. All market configuration lives in MySQL; this
feature does not edit world/zone files. Adding slots refreshes the room cache.

## Player commands

```
stall list
stall rent 1
stall name The Rusty Dragon
stall sell sword 25000
stall info
stall reprice 1 18000
stall renew
stall earnings
stall collect
stall withdraw 1
stall close

stall directory
stall search sword
stall browse 1
stall inspect 1
stall buy 1
```

Stall IDs and listing IDs are separate. Listings accept 1–99,999,999 gold,
without valuation-based limits or commission. Stock must be identified,
unbound, transferable and empty: quest items, money and no-rent/no-sell/no-drop
items are refused. Containers and weapon sheaths must be empty. Items retain
their saved properties, including typed feat affections and customization.

Rent, renew, name, sell, reprice and close require visiting the owner's enabled
stall. Naming, stocking and repricing require an unexpired lease. Closing and renewal
are also allowed during the grace period. Each account can hold one
stall, managed only by the renting character. Renew costs one week at the
stall's current rate and adds seven real days to the later of expiry or now.
Lease times use database wall-clock time, including time the MUD is offline.

Sales require an enabled, paid stall and a buyer in its room. Search and the
directory work globally. Buyers must have enough carried gold and inventory
capacity and cannot purchase their own listings.

Expiration stops sales immediately. The slot and account reservation remain
for 48 hours; after that, the next stall command releases the reservation.
Room displays also treat elapsed reservations as available. Old stock stays
attached to its historical lease, so a new tenant cannot access it. Owners
can list and withdraw old stock anywhere with `info` and `withdraw`. Earnings
and collection also work globally. Closing and eviction preserve stock and
sales proceeds, without refunding rent.

## Persistence and recovery

`player_shop.c` initializes five InnoDB tables (also provided in
`sql/20261023_player_shops.sql` for preinstallation under a schema administrator):

- `player_shop_stalls`: room assignment, rent, capacity, enabled status.
- `player_shop_leases`: historical tenants and expiry; nullable unique account
  and stall reservations enforce one active/grace lease per account/slot.
- `player_shop_listings`: exact serialized stock, price, and disposition.
- `player_shop_sales`: immutable buyer/seller/price and proceeds queue state.
- `player_shop_events`: staff and player lease/stock transaction history.

The existing estate serializer and outgoing journal protect stock transfers.
Row locks serialize purchases and lease/stock changes. Purchase transactions
record a single sale and an estate item delivery with its gold debit. Recovery
saves the item, gold, and receipt before acknowledging delivery. Unsaved outgoing
objects are removed by their durable identity on recovery.

Rent uses currency-only estate deliveries (empty object payload, positive
debit). Collection atomically marks sales queued and creates currency-only
credit deliveries (negative debit). They share the existing ordered delivery
receipt, saved with gold in the player file, avoiding a second payout counter.
Repeated recovery, including a failed SQL acknowledgment, does not charge or
pay again. Uncollected sales stay pending until `stall collect`; already queued
transfers recover on login. Gold overflow leaves a credit pending until the
player makes room. Estate trading waits while an earlier transfer is pending.

Room contents query stock only in configured market rooms, using a cache of
market room numbers. The schema and cache initialize lazily; no new player-file
fields, mobile vnums, heartbeat processing, or live world edits are needed.

## Installation and validation

Build normally with `make -j4`. The first stall use creates missing tables;
preinstall the schema instead if the game's SQL user lacks CREATE permission.
Apply `sql/20261023_player_shops_help.sql` to install `help stall` in the game's
MySQL help system. The matching reference help is `lib/text/help/stalls.hlp`.
No SQL help migration or live server restart is performed by the source change.

Run `python3 tests/run_estate_integration.py` from the repository root. It runs
the production estate, housing, auction, and stall services against a temporary,
isolated MariaDB, with both a lightweight codec and the actual object codec.
Stall cases cover staff configuration, account limits, customized names, typed
feat gear, capacity, local purchase and ownership checks, insufficient gold,
carrying limits, duplicate purchases and competing database locks, proceeds overflow/retry, expiry and grace
renewal, historical tenant isolation, disabled markets, closure, eviction, and
currency delivery acknowledgment failures.

For AddressSanitizer/UndefinedBehaviorSanitizer checking, run:

```
ESTATE_TEST_CFLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer -no-pie' \
ASAN_OPTIONS=detect_leaks=0 python3 tests/run_estate_integration.py
```

Leak checking is disabled because the lightweight game adapter intentionally
leaves its world/character fixtures allocated until process exit.
