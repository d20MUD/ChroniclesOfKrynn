# Dragonmetal Awakening — Phase 3 persistence and item identity

Status: implemented; no awakening purchases or gameplay powers enabled.

Related: [implementation plan](LEGENDARY_OBJECT_IMPLEMENTATION_PLAN.md),
[Phase 1 contracts](LEGENDARY_OBJECT_PHASE_1_AUDIT.md),
[Phase 2 rules](LEGENDARY_OBJECT_PHASE_2_RULES.md).

## Delivered

- [Versioned migration](../../sql/20261023_legendary_objects.sql), applied twice
  during tests to verify same-version idempotency; not applied to the live game.
- [Storage API](../../src/legendary_store.h) and
  [implementation](../../src/legendary_store.c) for owner/item/channel records,
  version checks, optimistic concurrency, tombstones and content registries.
- [Identity API](../../src/legendary_identity.h) and
  [implementation](../../src/legendary_identity.c): cryptographic 128-bit UUIDs,
  optional save tags, live verification, duplicate quarantine and unload cleanup.
- UUID support in all existing object record readers/writers, clone/prototype
  behavior and actual equipment/inventory/room placement integration.
- [Persistence integration test](../../tests/legendary_persistence.c) and
  [runner](../../tests/run_legendary_persistence.py), using disposable MariaDB
  and production codecs rather than a substitute legendary object format.

## Database schema

The original 11 tables use InnoDB; Phase 5 adds a twelfth source-receipt table.
The base migration records version 1, Phase 4 upgrades to version 2, and Phase 5
to version 3. Current runtime requires all three migrations and all expected
tables; it does not create/upgrade them at login. Missing schema, missing records and query errors are distinct outcomes.

| Table | Purpose |
| --- | --- |
| `legendary_schema` | Migration/schema compatibility version. |
| `legendary_characters` | Personal balances, six essence types, allowance date/counters, future loadout serialization and optimistic version. |
| `legendary_items` | Unique UUID, immutable owner identity/prototype/slot, pending/active/deleted state and version. |
| `legendary_channels` | Primary/supporting rank, path, family, selected properties, modifier, first-choice flags, investment totals and independent version. |
| `legendary_cooldowns` | Per-character power/family expiry keys. |
| `legendary_budgets` | Timestamped rolling-budget debits with unique operation identity. |
| `legendary_boss_claims` | Unique character/death claims and per-prototype reward history. |
| `legendary_quest_xp_claims` | Character/quest/day uniqueness and award operation identity. |
| `legendary_operations` | Future payment/recovery journal, before/after versions and gold, exact costs, estate delivery link and recovery payload. |
| `legendary_boss_registry` | Legacy boss configuration table from schema v1; dropped by schema v8 in favor of the native `MOB_LEGENDARY_BOSS` flag. |
| `legendary_dragonlances` | Explicit true-lance vnum, enabled flag and revision. |

Constraints include identity uniqueness, channel-to-item foreign keys, valid
rank/path/family ranges, nonnegative unsigned balances and prescribed personal
balance/daily-acquisition ceilings. No foreign key to a nonexistent authoritative
SQL character table is invented: normal characters still use player files.

The character/reward/cooldown/budget/operation tables are prepared storage, not
implemented earning/spending engines. Those services belong to later phases.
No production bosses, lances or smith vnums are populated by this migration.

## Storage API and transaction boundaries

`legendary_store_load()` loads a complete owner/item/two-channel snapshot and
validates all selected IDs, rank requirements, slot/path membership and modifier
compatibility. Error and missing-record results leave the caller's output
unchanged. Disabled/missing true-lance registration prevents live verification
of a channel selecting a true-lance-only signature.

`legendary_store_insert()` reserves a UUID as a pending item. It cannot overwrite
an existing identity, including a tombstone. Channels are inserted separately;
a supporting channel requires a primary record and a weapon parent. Each
channel update compares expected item ownership/version and channel version.
Stale or foreign-owner updates fail. Item activation requires a stored primary
channel. Tombstones retain investment/history and cannot be reactivated.

`legendary_registry_load()` and `legendary_registry_save()` provide inspection,
validation and enable/disable updates with expected versions. They are internal
administrative service APIs in this phase, not newly exposed staff commands.
Selection of actual world vnums remains the later content/deployment task.

**Later supersession:** schema v8 removes the boss-registry half of this API.
Bosses are now designated only with `MOB_LEGENDARY_BOSS`; the remaining registry
API is for explicit true-dragonlance object records.

These APIs **do not begin or commit transactions and do not charge gold**.
Callers own the SQL transaction and must follow Phase 4's durable economic
protocol. Successful channel/state or lance-registry writes fail closed by
invalidating matching live caches. After commit, the owning service must reload
and deliberately revalidate the same object; a rollback must also reconcile
any conservative cache invalidation. A cache never supplies authoritative
proof that a purchase committed.

## Object identity and save compatibility

A physical object's optional `legendary_uuid[33]` stores 32 lowercase hex digits
plus a terminator. Nil UUIDs are invalid. Generation uses OpenSSL `RAND_bytes`
and the UUID-v4 version/variant bits. Neither object vnum, SQL save-row ID nor
estate delivery receipt is used as the physical identity.

Normal saved form:

```text
LgId: 1 31cf4e6497c74d3dbe7fd2fa629d7b08
```

`LgQn: 1` additionally preserves quarantine when necessary. Malformed tags,
unknown versions and duplicate identity tags retain a canonical UUID when one
can be recovered; otherwise they preserve a nil invalid marker and quarantine.
They never silently become ordinary unawakened items on the next save. Legacy
records without legendary tags still load without an identity or SQL lookup.

| Save path | Integration |
| --- | --- |
| File records | Main writer/file parser and common optional-property reader. |
| Player SQL | Main full-record writer plus full-record and legacy tokenized reader branches. |
| Legacy house SQL | Same object-record writer/parser as player saves. |
| Pet SQL | Separate production pet writer/parser now preserve the tag. This is codec compatibility, not permission to give awakened equipment to pets. |
| Sheathed SQL | Separate sheath writer/parser now preserve each physical weapon's tag. |
| Housing chest, auction/stall escrow, estate delivery | Existing `estate_pack()`/`estate_unpack()` framing uses the production object codec and preserves nested contents and both sheath positions. |

The codec metadata exists even on rejected/recovery records. Later ownership
controls still forbid auctioning, trading or giving awakened equipment; persistence
support does not authorize those player operations.

## Live verification and quarantine

Actual `obj_to_char()`, `obj_to_room()`, `obj_to_obj()` and `equip_char()` placement
paths finalize identity verification. Detached estate/parser previews never claim
identities. A finalized container also verifies its nested and sheathed items.

Verification requires an active valid database record, matching original
prototype and binding/authoritative owner, and a valid lance registration when
needed. Records are cached only on tagged objects; ordinary untagged objects do
not trigger legendary schema/database queries.

A live UUID registry detects two finalized objects claiming the same identity.
Both are quarantined and their legendary caches removed. Ordinary equipment is
retained. Missing records, pending/deleted records, unavailable storage and
invalid identity data similarly disable legendary participation and log a
specific diagnostic. No alternate item is guessed, no UUID is silently replaced,
and no investment is transferred. A later authorized recovery service must
resolve the stored problem and explicitly revalidate the original item.

## Copy, edit and extraction behavior

- `db.c:read_object()` clears prototype-derived identity/cache fields on every
  newly created physical instance.
- `genobj.c:copy_object()` and `copy_object_preserve()` clear the new destination's
  identity and cache. Their existing prototype/clone callers cannot duplicate
  the source's investment.
- The actual `update_all_objects()` prototype-refresh path retains each live
  destination's UUID, quarantine, binding and owned cache. It does not inherit
  a prototype UUID. IEDIT's full-structure commit likewise retains the original
  physical object's identity/cache/binding.
- `handler.c:extract_obj()` unregisters and frees only runtime identity/cache
  data. Serialization previews, escrow removal and shutdown extraction are not
  permanent destruction and never tombstone investment automatically.
- Explicit `legendary_store_tombstone()` records permanent destruction. Wiring
  allowed/rejected player destructive operations remains the later ownership
  phase; no new player-facing destruction mechanism is introduced here.

Rank/ability recalculation and ongoing effect cancellation do not exist yet;
those must use this identity/source lifecycle in the later gameplay phases.

## Validation and reproducible checks

```sh
python3 tests/run_legendary_rules.py
python3 tests/run_legendary_persistence.py
ASAN_OPTIONS=detect_leaks=0 ESTATE_TEST_CFLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer -g' python3 tests/run_legendary_persistence.py
make -j4
```

CMake was also configured/built in an isolated temporary build directory.
Both game builds completed successfully. New persistence/identity modules also
compile separately with GNU90, `-Wall -Wextra -Werror`.

Integration coverage:

- Same-version migration applied twice, required schema/version detection,
  foreign keys, unique IDs, daily-cap constraints and rollback.
- Independent rank-V primary/supporting records, ownership guards, stale channel
  and registry updates, registry inspection and enable/disable behavior.
- File, player SQL, legacy tokenized player SQL, house SQL, pet SQL and sheath
  production codec round trips, including old records without legendary tags.
- Nested/sheathed estate framing and detached previews.
- Actual production clone and prototype-refresh functions preserve the correct
  identities; unload leaves persistent records intact.
- Missing records, duplicate finalized UUIDs, deleted UUIDs, invalid tags and
  future schema versions fail closed while ordinary items remain available.
- Tombstoning immediately invalidates an existing live cache.

The runner also reuses the established estate housing/auction/stall integration
suite. Its expected duplicate-key, constraint and injected database-failure
messages are intentional assertions, not unhandled test failures.

Address/undefined-behavior sanitizer runs passed with leak detection disabled:
the reused estate/world fixture deliberately retains baseline world allocations
at process exit. This is not a claim that the whole server or those pre-existing
fixtures are leak-free. No live database, zone files or running game were changed.

## Handoff to Phase 4

The next phase must link pending item/UUID persistence to the estate gold receipt
and operation journal, implement checked player-file checkpoints and prove
exactly-once economic recovery. It must not expose awakening purchases before
those guarantees exist. Later phases implement actor-aware ownership guards,
loadout rules, content administration commands and actual powers.
