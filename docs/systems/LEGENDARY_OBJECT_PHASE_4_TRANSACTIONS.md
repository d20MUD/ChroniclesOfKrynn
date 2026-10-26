# Dragonmetal Awakening — Phase 4 transactions and recovery

Status: implemented backend. Phase 5 adds earning hooks with schema v3; player
purchases and gameplay powers remain disabled. See the [implementation plan](LEGENDARY_OBJECT_IMPLEMENTATION_PLAN.md).

## Delivered

- [Transaction API](../../src/legendary_transaction.h) and
  [coordinator](../../src/legendary_transaction.c): opaque, single-use 60-second
  quotes for purchases, reshapes, exchanges and grants.
- Quotes bind a cryptographic operation ID to the character, definition version,
  exact signed ledger changes, current gold, wallet version, item UUID, both
  channel versions and the proposed channel record. Quotes cannot be edited
  through the public API. Confirming checks expiry and authoritative versions.
- Per-character SQL serialization with a unique nullable `held_player` key,
  row locks, compare-and-swap writes, and a durable `LgTx` character-file lock.
- One ordinary-gold payment journal: `legendary_operations.estate_delivery_id`
  references the existing `estate_deliveries` receipt stream. Legendary recovery
  never independently subtracts the recorded gold cost.
- Atomic character and object-file replacements: checked writes, file fsync,
  rename and parent-directory fsync. Object SQL replacement is transactional and
  propagates failed inventory, bag and nested-child writes. All save wrappers
  remain available to existing callers.
- Login recovery before commerce, pets and login scripts; an unresolved login
  discards the loaded copy without saving and closes the connection. Normal,
  description-setup and copyover entry paths all honor this result.
- During a pending in-game operation, commands attempt recovery before executing;
  estate spending, trades, incoming gifts and theft cannot bypass the lock.
  Wear/remove scripts are suppressed while persistence re-equips locked gear.
- No success return until the SQL outcome, gold receipt, object checkpoint and
  cleared character lock are durable. Failure messages identify the operation.

## Database migration

Apply [schema v1](../../sql/20261023_legendary_objects.sql), then
[Phase 4 / schema v2](../../sql/20261024_legendary_transactions.sql), using the
normal offline migration process. Current runtime also requires the subsequent
[Phase 5 / schema v3](../../sql/20261024_legendary_earnings.sql) migration. These scripts are safe to rerun on MariaDB.
**None of these migrations was applied to the live database.**

The second migration adds operation ownership, new-identity recovery metadata,
quote expiry and definition version to `legendary_operations`, then advances
`legendary_schema.version` to 2. Phase 5 subsequently requires version 3. Economic
operations also require `estate_deliveries` and `player_save_objs` to use InnoDB;
the coordinator does not silently convert existing tables. There is no new gold
journal, automatic schema installation or balance initialization at login.

## State machine and recovery

1. **Quote:** snapshot balances, versions, proposed channel and exact changes.
   A new awakening generates a proposed UUID but changes neither its item nor
   its binding. Expiry and repeat confirmations cannot reuse a payment.
2. **Durable lock:** checkpoint `LgTx` before the first operation insert. If the
   process stops before that insert, recovery sees no operation or competing
   owner, clears the lock durably and charges nothing.
3. **Prepared:** reserve the character's unique SQL ownership. For a first
   awakening, insert an empty pending identity, tag/bind the actual carried or
   equipped instance, then checkpoint it. An existing item must have exactly one
   owned live instance and its UUID must appear in its saved object record.
4. **Committed:** lock the operation, wallet and item in a consistent order;
   recheck versions/expiry/gold; execute the trusted participant; update balances
   and the selected channel; keep the item pending; queue the estate receipt;
   and mark the operation committed, all in one SQL transaction.
5. **Applied:** replay the ordered estate receipt stream, checkpoint the gold and
   receipt together, and verify acknowledgment. Activate the item and mark the
   operation applied in one SQL transaction. Refresh and checkpoint its identity
   cache, then clear and checkpoint `LgTx`.

An interrupted **prepared** operation cancels with no deductions. A newly assigned
UUID is removed and the previous binding restored and saved; its pending SQL
identity becomes a tombstone. An existing item's previous channel is unchanged.

A **committed** operation always finishes its existing receipt and SQL outcome;
it does not calculate another price or enqueue another payment. A missing,
duplicated or incorrectly bound committed item blocks recovery. An **applied**
operation only finishes object/lock cleanup. Terminal rows remain as audit data.

A failed file replacement after rename is ambiguous. Recovery retains the
in-memory gold receipt instead of rolling it back. It retries the checkpoint
before acknowledging delivery even if that receipt already advanced. A restart
loads whichever complete file survived and charges only receipts newer than the
saved receipt. Missing schema, disconnected SQL, rejected writes, insufficient
gold or missing identity checkpoints retain the lock.

## Contracts for Phases 5 and 6

This is a **server-only coordinator**, not a client pricing API. A transaction
plan supplies exact deltas and, optionally, a desired channel. The future award,
exchange and purchase handlers must derive these from the Phase 2 rules; player
input must never supply prices or balances directly. Initialization of the
personal ledger, earning provenance, allowance reset/caps and the command syntax
remain in their scheduled phases.

The optional `participate` callback runs inside the same transaction and can
validate eligibility and atomically insert source claims or update allowance
counters. Its failure rolls back the wallet, channel and gold receipt too. It
must not commit, save files/objects or independently change the wallet/version.
Its caller-owned context must live through quote confirmation/discard. Recovery
never repeats this callback: committed claims are already durable; prepared
operations cancel.

For first awakening, pass an empty desired UUID plus the ordinary carried or
equipped object. The coordinator handles assignment/binding and cancellation.
For later investments, pass the existing UUID and its expected record versions.
Each operation changes one channel, so two-handed primary/supporting channels
remain independently purchased. The caller owns/destroys the opaque quote.

All callers must preserve the game's existing single active character writer
rule, increment authoritative versions when changing records, and honor the
pending lock. Frontends must revalidate gameplay eligibility through the
participant and start investments outside combat. Direct administrative/database
edits are not a substitute for this transaction API.

## Verification

Run:

```sh
python3 tests/run_legendary_transactions.py
ASAN_OPTIONS=detect_leaks=0 \
ESTATE_TEST_CFLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer -g' \
python3 tests/run_legendary_transactions.py
```

The runner creates disposable MariaDB instances; it never connects to the live
MUD database. It also runs the existing housing, auction, player-stall and
legendary persistence regressions. Production estate, legendary, durable-file,
object codec, recursive object save and checked object checkpoint code are used.
The small-world adapter emits the gold/receipt/lock fields using the production
atomic replacement helper; it does not emulate the full player serializer.

Coverage includes 20 interruption boundaries for both existing identities and
first awakenings, file failure boundaries before writes/fsync/rename and after
rename, SQL mutation failures, actual saved-object and character-field reloads,
repeat confirmations/recovery, expired/stale/competing quotes, source-claim
rollback and replay, grant/exchange operations, overflow, SQL outage and lost
estate acknowledgment. Test-only clock/fault controls are absent from production.
Leak detection is disabled for this shared fixture's retained mock world; address
and undefined-behavior checks remain enabled.

Both full CMake and Make builds are checked. These tests simulate interrupted
operations and failed persistence calls; they are not physical power-cut tests
of a particular filesystem or database durability configuration.
