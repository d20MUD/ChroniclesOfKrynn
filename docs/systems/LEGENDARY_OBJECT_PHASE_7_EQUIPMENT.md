# Phase 7: ownership, equipment eligibility and cached loadouts

Implemented 2026-10-24. This phase provides a shared eligibility/ownership
adapter and an equipped-channel cache. It evaluates which foundations and saved
major selections are usable; Phase 8 still integrates actual stat contributions,
and Phases 9–12 implement activation and power effects.

## Permanent ownership and movement

`legendary_equipment.c` supplies a recursive guard covering the item, contained
objects and both sheath children. Ordinary binding flags cannot override the
immutable UUID owner. Active records must match the object's prototype and
binding; missing, deleted, mismatched or quarantined identities fail closed.
Detached storage previews are checked without claiming a duplicate live identity.
A pending record is accepted for owner recovery only when the character's durable
operation explicitly identifies that same UUID and owner.

The guard distinguishes owner storage/recovery from transfers and conversions:

- Give, pet handoff, trade offers and final trade validation reject awakened
  trees. Give/drop/put recheck after their script triggers.
- NPC shop purchase/sale, auction listings and player-stall listings reject them
  before payment or escrow. Estate delivery validates the recipient before
  creating a delivery or applying its debit, including recovery of older rows.
- Normal drop/donate/junk, sacrifice, salvage, disenchantment, legacy kit
  consumption/conversions and both reforge systems reject them before their
  economic mutations. New resizing validates assignment, start and completion.
  Enchantment and low-level weapon/armor replacement helpers reject them too.
- Genuine owner-carried bags, personal housing chest storage and estate recovery
  preserve UUIDs. Shared chest access does not grant ownership. Ground/container
  recovery checks every awakened child; death/corpse system movement remains
  permitted and preserves the physical objects. The dump spec no longer destroys
  awakened trees found on the ground.
- Cosmetic legacy restring/redesc operations require ownership and preserve the
  same physical object. Automatic resizing on receipt skips awakened equipment.
  Active crafting projects cannot be awakened midway through conversion.
- Staff bind/unbind and DG `objbind` cannot change legendary ownership.

Core equip rejects a wrong owner. Core character/container placement also checks
owner transitions. If an internal/scripted bypass has already detached an item,
it is retained for recovery and its awakened tree is quarantined with a log;
these void primitives do not silently discard it. Normal user/service routes
reject the operation before detaching the object. Ordinary unload/extraction for
storage continues to release runtime identity rather than delete progression.
No new permanent-destruction command or refund/transfer route was introduced.

## Equipped channel and passive planning

The actual wear position must match the item's purchased slot. Weapons on the
back or held instead of wielded do not activate a weapon channel. Both one-handed
weapon positions support their own primary track. Supporting channels require
an explicit two-hand wield slot, or an actually wielded mandatory-two-handed
ranged weapon. Weapon size and a free offhand are not evidence of this grip.

Hand occupancy includes shields, both held positions, both wielded positions
and explicit two-hand positions. Impossible combinations suppress hand-slot
channels. A shield/held item cannot remain active alongside a two-handed
weapon. Both paid channels remain stored when grip changes suppress supporting
activation; no rank or investment transfers to another item.

Wildshape equipment, ordinary race/class/alignment/size restrictions, loss of
ownership, death, invalid identity and invalid grip suppress channels. The
adapter delegates ordinary wear rules to the game's existing functions and
retains the approved level/type/permanence restrictions. Typed statistical caps
and superseded bonus contributions belong to Phase 8.

Equipped channels are ordered by the persisted UUID/channel priority list,
followed by stable wear order for unlisted channels. The pure Phase 2 planner
applies the twelve-point limit and category/subtype duplicate ceilings. F08 must
be a supported trained roll skill; F11 requires an actual resource class. F12
uses the matching acid/cold/electricity/fire/sonic resistance type. The cache
reports each foundation's reason, point cost and planned value. Combat entry
latches the active allocation: unequipping an allocated item cannot activate a
previously dormant passive during that fight. Re-equipping the same unchanged
channel can restore its allocation; outside combat the priority plan is recalculated.

Only the three saved major references are considered. Unequipped/ineligible
selections remain dormant and retain their places; another equipped power is
never automatically substituted. Weapon powers require a usable wielded weapon,
resource powers require an actual resource class, and the rider signature requires
the owner's living dragon mount. Those capability checks also revoke existing
source leases when their prerequisites disappear. Duplicate selections of the same power do not
grant extra simultaneous activations. Selection kind and source channel remain
explicit. `infuse inspect` and `infuse loadout` use this interface to explain
active/dormant eligibility; they do not claim that later-phase effects exist.

## Cache, invalidation and source leases

The runtime cache is owned by the character and is never serialized or copied
from an NPC prototype. Configuration is loaded at first use or explicit refresh;
committed smith changes refresh only the affected character. Equipment, identity
and capability changes mark/rebuild the calculated view without rereading saved
configuration. Stable combat reads issue no SQL. Identity/configuration changes
refresh authoritative metadata only at those event boundaries.

Equip, forced unequip, affect recalculation, death-position updates, identity
unload/quarantine, channel writes and explicit smith configuration refresh are
hooked. A lightweight read fingerprint additionally catches grip, ownership,
level, capability and trained-skill changes even when a caller bypasses the
normal notification. Duplicate live UUIDs are resolved before calculating any
channel, so the earlier copy cannot retain a benefit from the same rebuild.
Failed configuration loading disables the view until an explicit refresh; it
never enables unsaved defaults or retries SQL on every combat read.

`legendary_source_attach` leases a later effect to an eligible, attuned
UUID/channel/power and channel version. Source invalidation immediately invokes
its removal callback. Rebuilding after unattunement also removes the source.
Callbacks may recalculate affects; invalidation and character cleanup guard
against recursive/double removal. Natural expiry can forget a source lease.
Character reset/logout/free releases all leases and cache memory. These APIs do
not alter cooldowns, rolling budgets, gold, materials or saved progression.

Actual technique/signature effect bodies are intentionally not introduced here.
The lifetime contract is tested with a reversible mock ward; subsequent effects
must attach their source through this interface.

## Validation and deployment

```sh
python3 tests/run_legendary_equipment.py
ASAN_OPTIONS=detect_leaks=0 \
ESTATE_TEST_CFLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer -g' \
python3 tests/run_legendary_equipment.py
python3 tests/run_legendary_rules.py
```

Tests use disposable MariaDB and real persistence/transaction/storage modules.
The surrounding world and ordinary wear checks use the shared fixture adapter;
production give/drop routines are extracted verbatim for transfer tests. The
runner includes all Phase 3–6 and estate/housing/auction/stall regressions.

Coverage includes a nine-channel outfit and twelve-point budget, flat-damage
ceiling, saved priority ordering and combat allocation locking, sword/shield, dual wield, explicit and ranged
two-hand grips, illegal hand occupancy, back-slot dormancy, no fourth-major
substitution, weapon/resource capability and wildshape/death suppression, immediate source
removal and cleanup callback reentrancy, unchanged cooldown/budget rows,
recursive owner barriers, real nested/sheath escrow-delivery recovery, and
same-rebuild duplicate-UUID suppression. One thousand stable cache reads are
checked against the actual SQL session query counter.

No schema change is needed beyond Phase 6's v4 migration and smith registry.
No live database migration, live world edit or server restart was performed.

Validation passed: the scripted equipment and preceding-phase regressions,
AddressSanitizer/UndefinedBehaviorSanitizer (retained fixture world leak checks
disabled), pure-rule tests, and full Make/CMake builds. Changed modules compile
without warnings; `git diff --check` is clean.
