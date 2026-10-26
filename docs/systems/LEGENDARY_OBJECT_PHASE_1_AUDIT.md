# Dragonmetal Awakening — Phase 1 audit and interface decisions

Status: **Phase 1 completed: repository audit and interface contracts**.
Reviewed: 2026-10-23. Gameplay, schema, command registration and deployment are
not implemented by this phase. Later phases must implement and test the contracts
below before enabling the system.

Related: [implementation plan](LEGENDARY_OBJECT_IMPLEMENTATION_PLAN.md),
[design](LEGENDARY_OBJECT_SYSTEM_PROPOSAL.md),
[player guide](../guides/DRAGONMETAL_AWAKENING_PLAYER_GUIDE.md).

## Baseline and scope

Baseline HEAD: `955e4eb57a90e6ed8a7216019bc7fa8d37dbd760`.
No applicable `AGENTS.md` was found in the repository/ancestor instruction search.

Pre-existing working changes, preserved:

- `src/oasis_copy.c`, `docs/world_game-data/REDIT_GUIDE.md` and
  `tests/run_buildwalk_overlap.py`: the preceding buildwalk work.
- Legendary proposal, implementation plan and player guide: existing untracked
  design documents.

This phase adds this audit, marks Phase 1 in the plan, and synchronizes documented
activation syntax. It does not change runtime source or live world data. Local
build/test artifacts are not a deployment or server restart.

## Decisions that unblock implementation

| Question | Decision |
| --- | --- |
| Command collision | Keep existing staff `invoke` unchanged. Legendary activation is **`infuse use <power-ID> [target]`**. `infuse` is not currently registered. |
| Action model | `infuse` dispatch itself uses no automatic action charge. Its `use` branch validates targets/action availability, then consumes a standard action and at least six real seconds of command recovery only on successful activation. Read-only subcommands never consume actions. |
| Gold | Extend the existing ordered estate money-delivery/receipt mechanism, with a legendary operation linked to its delivery ID, rather than introducing a competing gold journal. Harden player-file checkpoint persistence before claiming crash-safe purchases. |
| Item identity | Add an optional 128-bit instance UUID, distinct from vnum, database row ID and estate delivery ID. Database channels are authoritative; codecs carry the UUID reference. |
| Two-handed eligibility | Explicit two-hand wield slot, or an actually wielded mandatory-two-hand ranged weapon. Do not use weapon size plus an empty off-hand as proof. No automatic multiplication of legendary properties. |
| Conditions | Support explicitly identified timed fear/nonmagical bleed effects and recorded ordinary knockdown, through condition adapters. Do not clear every fear/bleed bit or interpret all sitting characters as tripped. |
| Source context | Add explicit combat/cast/XP provenance rather than guessing from damage element, spell name or an existing generic XP mode. |
| Ownership | Preserve normal binding but enforce a separate immutable legendary owner check, recursively for nested/sheathed objects, at all transfer/destructive routes. |

These are implementation decisions, not additional player progression costs or
changes to rank prices, daily caps, power values or two-handed cost parity.

## 1. XP award map and provenance

Primary interface: [`gain_exp()` in limits.c](../../src/limits.c).
Source modes: [mudlim.h](../../src/mudlim.h).

Current positive awards apply leadership/background/adaptability modifiers,
mode-specific caps, happy hour, account rewards and the absolute gain cap before
`GET_EXP(ch) += gain`. Place the new credit after final gain calculation, before
ordinary advancement handling; do not copy the raw caller argument. Preserve
normal XP semantics and the ordinary return value.

| Existing caller | Current mode | Legendary treatment |
| --- | --- | --- |
| `fight.c`: `perform_group_gain()` / group gain callers and `solo_gain()` | GROUP/SOLO | Eligible only with a real PvE victim/encounter context; exclude summoned, charmed, owned and test victims before credit. |
| `quest.c`: `complete_quest()` | QUEST | Eligible with quest vnum and unique completion context; one infusion-XP credit per quest per character per UTC day. |
| `missions.c`: mission reward | QUEST | **Not automatically eligible.** It shares the mode but lacks an automatic-quest vnum; exclude unless a future explicit policy adds it. |
| `traps_new.c` disarm rewards; legacy `traps.c` | TRAP | Eligible only for a genuine successful disarm with source identity, not arbitrary TRAP-mode calls. |
| `craft.c` crafting/reforging | CRAFT | Excluded. |
| `fight.c` damage-credit awards | DAMAGE | Excluded, including awards forwarded to pet masters. |
| `dg_variables.c` XP assignment | SCRIPT | Excluded. Normal automatic quest completion invoked by a script still uses the quest completion route above. |
| `magic.c` energy drain | EDRAIN | Excluded. |
| `spec_procs.c` dump rewards | DUMP | Excluded. |
| Death penalties, staff/direct XP mutations, generic DEFAULT | DEATH/other | Excluded. Never infer eligibility merely from a positive amount. |

**New interface contract:** introduce an optional structured XP context alongside
the existing API (a wrapper can preserve callers). It contains recognized origin,
victim/quest/trap identity, unique award identity and test/ownership exclusions.
Existing context-free calls remain ordinary XP but do not earn infusion XP.
Credit only when the recipient is exactly level 30 at award time. Never credit
once per equipped item. Keep award and daily-quest claims idempotent.

Do not credit ordinary account XP as a second legendary award; it is already a
separate feature in the same pipeline. Baseline code can continue earning XP at
level 30, but the new ledger must still be placed before any future max-level
suppression. Preserve 64-bit ledger arithmetic even though `gain_exp` takes an
`int` award.

## 2. Character state, gold and recovery

### What is saved now

- [`players.c`: `load_char()` / `save_char()`](../../src/players.c) read/write the
  character file selected by `get_filename(..., PLR_FILE, ...)` in
  [`utils.c`](../../src/utils.c); `LIB_PLRFILES` is `plrfiles/` under the game root.
- Carried gold is the `Gold:` player-file tag. Estate currency receipts are
  `ECrd:` and `EDbt:` tags. `player_data` SQL metadata is not the authoritative
  carried-gold record.
- Current `save_char()` returns `void`, opens the file for writing, writes its
  buffer and closes it. It does not provide an atomic replacement/durability
  success contract. Read-back alone cannot establish power-loss durability.
- [`estate.c`](../../src/estate.c) already provides `estate_queue_money()`,
  `estate_recover()`, `estate_receipt_saved()` and `estate_player_ready()`.
  Positive queued debit subtracts gold; negative debit credits it. Delivery IDs
  are ordered and receipts prevent reapplication after file save.
- [`interpreter.c`](../../src/interpreter.c) login currently calls `Crash_load()`,
  then estate recovery and auction collection, before pet loading/login triggers.

### Selected purchase protocol

1. Require `estate_player_ready()`, no earlier legendary operation, sufficient
   carried gold, exact rank/item/currency versions and valid ownership.
2. In one SQL transaction, insert the legendary operation, debit its XP/material
   balances, record the pending channel change, and queue the positive estate
   money debit. Persist the exact **estate delivery ID** on the operation.
   Extend the queue interface to return that ID; do not infer it from a name or
   a later unrelated `mysql_insert_id()` call.
3. Serialize this character's economic changes until the linked receipt finishes.
   The upgraded channel and item ownership changes remain unavailable while
   pending. A first awakening also checkpoints the UUID on the physical item
   before the operation may be acknowledged.
4. Apply the money delivery using the existing ordered estate receipt. Require a
   checked, atomic player-file save: temporary file in the same directory,
   complete write, flush/fsync, atomic rename and parent-directory fsync, with
   checked errors. Reuse this checkpoint in estate recovery; preserve unrelated
   save behavior and existing callers through an appropriate checked helper.
5. Verify the item reference and currency receipt are saved, acknowledge the
   delivery, then activate the pending channel and mark the operation applied.
   Failures remain recoverable; no benefit is granted while a debit is pending.
6. On login and before other economic actions, drain older ordered estate work
   and reconcile linked legendary operations before access. DB-unavailable or
   unresolved state fails closed for the affected economic operations.

Never apply a separate absolute-gold replay after the receipt system has already
paid. Never acknowledge SQL and later attempt an unchecked file save. A process
crash during first UUID persistence requires operation-to-item reconciliation;
ambiguous duplicate/missing references quarantine the operation, not a guessed
replacement item. Implementation and fault-injection proof belong to phases 3–4.

## 3. Object serialization and location coverage

[`objsave.c`](../../src/objsave.c) has a shared extension reader:
`restore_persistent_object()`. It is used by the file parser and the separate
SQL player/house, pet and sheath parsers. Add the validated optional UUID tag
there, and to **every writer**, not only the main writer.

| Location | Existing path | Required adapter/test |
| --- | --- | --- |
| Player inventory/equipment and nested containers | `Crash_crashsave()` / `Crash_load()`; `objsave_save_obj_record_db()`; `objsave_parse_objects_db()`; SQL `player_save_objs` | UUID round trip; actual object location separate from SQL row IDs; legacy parser and full-record branch. |
| Legacy object files | `objsave_save_obj_record()` / `objsave_parse_objects()` | Optional tag accepted; old records continue loading. |
| Legacy house contents | `house.c`: `House_save()` / `House_load()`; SQL `house_data` | Same record codec, recursive container test, ownership checks for shared rooms. |
| New housing chests | `housing.c`; SQL `housing_chest_items` | `estate_pack()`/`estate_unpack()`, escrow and queued delivery preserve UUID; guests cannot withdraw another owner's awakened gear. |
| Auction house | `auction_house.c`; SQL `auction_listings` | Refuse awakened input before escrow. Codec remains UUID-compatible for recovery and diagnostic records. |
| Player stalls | `player_shop.c`; SQL `player_shop_listings` | Refuse awakened input before escrow; never rely only on ordinary bind flags. |
| Estate delivery/outgoing journal | `estate.c`; SQL `estate_deliveries` / `estate_outgoing` | Full nested/sheathed payload and stable item identity through recovery. An estate receipt ID is not the item UUID. |
| Pet inventory | Separate `objsave_save_obj_record_db_pet()` / parser; SQL `pet_save_objs` | Codec must understand identity, but player-owned awakened items cannot be given/equipped to a pet. Existing inconsistent data is quarantined for owner recovery. |
| Sheathed equipment | Separate sheath writer/parser; SQL `player_save_objs_sheathed` | UUID on each contained physical weapon, not only its sheath; estate framing also carries sheath children. |
| Corpses and dropped containers | `fight.c` corpse construction, `act.item.c` get/put, ordinary world/storage serialization | Preserve same instance during death/recovery; enforce recursive owner access. No separate durable corpse SQL store was identified in this audit; do not promise offline corpse recovery. |
| Ordinary mail | `new_mail.c` / `mail.c` | SQL text mail and letter objects, not general gear attachments. Gear delivery belongs to estate; no fictional mail-attachment codec to add. |

`estate_pack()` wraps the production `estate_write_record()` codec in the `EOBJ1`
recursive format, including container children and both sheath objects. Reuse it
rather than inventing a legendary-only alternative object format.

**Lifecycle decision:** [`genobj.c`: `copy_object_main()`](../../src/genobj.c)
currently performs `*to = *from`. Blindly adding a UUID field would clone it.
Define distinct copy intents: prototype/clone creation clears UUID; an in-place
cosmetic update retains the destination UUID only after eligibility validation;
serialization restores the saved identity. Prototype records never own an UUID.

[`handler.c`: `extract_obj()`](../../src/handler.c) is also used for escrow,
shutdown/save/unload and temporary parse cleanup. It must not always tombstone
awakening. Explicit permanent-destruction paths tombstone; ordinary unloading
only releases memory and cancels sourced effects. Missing/duplicate identities
are errors to quarantine, not reasons to assign another item's investment.

## 4. Ownership, transfers and transformations

Ordinary `GET_OBJ_BOUND_ID()` is serialized, and equip checks it. Auction/stall
eligibility already rejects bound gear. However `perform_give()` and trade/drop
paths primarily check `ITEM_NODROP`; binding by itself is not a complete
legendary ownership barrier.

Introduce a context-aware recursive operation guard, provisionally
`legendary_can_move_tree(actor, object, destination, operation)`. Include normal
container children and both sheath objects. Revalidate on trade confirmation,
escrow preparation and actual mutation, not just when displaying an offer.

| Route | Decision and source integration |
| --- | --- |
| Give, direct trade, pet handoff | Reject awakened objects or any container containing them; `act.item.c` give and trade offer/confirm paths, plus low-level owner transition checks. |
| Drop, junk, sacrifice, salvage | Reject normal drop/transfer; deliberate permanent destruction requires an explicit confirmed destructive operation before extraction. `perform_drop()`, `do_sac()`, `do_salvage()`. |
| Get/put and shared containers | Owner may move their object into personal inventory, owner-controlled storage or an owner-carried container. Recursive pickup/withdraw checks prevent other characters taking it indirectly. Shared housing access does not confer item ownership. |
| Death/corpse recovery | Explicit system death move retains UUID; only the owner may recover it, including nested containers. No ordinary give/loot exception follows from corpse placement. |
| NPC shops, auction, stalls | Reject before payment/escrow; cover `shop.c`, `auction_house.c`, `player_shop.c` and any NPC service that consumes gear. |
| Reforge and conversions | Guard both legacy `craft.c:reforge()` and `crafting_new.c:do_reforge_new()`, plus crafting/material consumption and downgrade/salvage. In-place stat rewrites also reject, not only replacement objects. |
| Prototype edit/clone | `genobj.c`, `db.c:read_object()/clear_object()`, object load scripts and administrative copies must never duplicate an existing awakened identity. Cosmetic owner edits preserve only that same physical eligible item. |
| Delivery/storage | Permit explicit system movement to the recorded owner only; escrow/delivery recovery never rebinds an item to the current recipient. |

Raw `obj_to_char()`, `obj_to_obj()` and other void movement primitives are not
sufficient user-facing validation points. Add checked service-level guards and
low-level invariant diagnostics for scripted/internal bypasses. Do not simply
block every `extract_obj()` or internal movement call.

## 5. Confirmed commands and action semantics

Existing [`interpreter.c`](../../src/interpreter.c) registers `invoke` as
`do_use_consumable`, `SCMD_INVOKE`, swift-action metadata. In
[`act.item.c`](../../src/act.item.c), it routes to ordinary `do_use()` or stored
`invoke_staff()` depending on the consumable preference. Repurposing it would
break existing staff use and attach the wrong action semantics.

**Confirmed legendary interface:** `infuse use <power-ID> [target]`. All other
`infuse` subcommands retain the proposal's syntax. This branch is usable away
from the smith; purchases/selections still require the smith. The documents
are synchronized in this phase. There is no new `invoke` alias.

Implement `do_infuse` as a dispatcher without a blanket table-level action cost.
The use branch must explicitly enforce ordinary incapacity, paralysis/stun/sleep,
position and target restrictions because the help/read-only branches need a
lower position threshold. Recover footing is an intentional sitting/recorded-
knockdown exception, not permission to act while stunned or unconscious.

Use [`actions.c`](../../src/actions.c): `is_action_available(atSTANDARD)` and
`start_action_cooldown(atSTANDARD, 6 * PASSES_PER_SEC)` (or the existing
`USE_STANDARD_ACTION` macro in `actions.h`). The event timer takes
pulses; passing a bare `6` would be shorter than six real seconds. Apply command
wait as `MAX(existing wait, 6 * PASSES_PER_SEC)` because `WAIT_STATE` overwrites
rather than extends. Do not shorten an existing longer wait. Check before
payment/effects; invalid targets cost nothing. No automatic queueing of stale
legendary target commands and no second action charge from generic metadata.

Persist the legendary power/family/global expirations separately; ordinary
standard-action events are not durable per-power cooldowns. Update shared
activation state before effect resolution and honor existing recovery. Weapon
autoattacks retain ordinary behavior.

## 6. Mechanics adapters and supported condition semantics

The names below are **new planned adapter contracts**, not functions claimed to
already exist. Each will receive fixtures in its implementation phase.

| Adapter | Existing source and chosen behavior |
| --- | --- |
| `legendary_weapon_uses_two_hands()` | `act.item.c`: `hands_used()`, `hands_available()`, `hands_needed_full()`, `do_wield()`; `assign_wpn_armor.c`: `is_two_handed_ranged_weapon()`; `utils.c`: `is_weapon_wielded_two_handed()`. Require actual two-hand slot or mandatory two-hand ranged grip. Existing size/empty-hand heuristic alone is insufficient; existing generic doubled affections must not multiply legendary channels. |
| `legendary_supported_skill()` | `utils.c:is_valid_skill()` and `spec_procs.c:compute_ability()`. Start with the actual valid noncrafting skill IDs; require positive trained ranks for F08. Exclude custom counters and total-defense pseudo-skill unless it has a real supported roll. Honor aliases to the same underlying ID once. |
| `legendary_maneuver_bonus()` | `act.offensive.c:perform_knockdown()/perform_disarm()` and `utils.c:push_attempt()`. Modify real opposed checks on the correct attacker/defender side. No roll means no automatic immunity. Generic combat-maneuver and legacy manual checks both need hooks. |
| `legendary_knockdown_state()` | `perform_knockdown()` and `change_position()`: add a source/reason marker for genuine knockdowns, cleared on normal standing. Sitting alone is ambiguous. T08 only clears recorded recoverable knockdown, with normal action/position restrictions. |
| `legendary_fear_remaining_seconds()` | `magic.c:affect_update()` runs every `PULSE_VIOLENCE` via `comm.c`. Durations are combat rounds and a zero-duration affect can remain until the next update. Track the next affect-update deadline and compute actual expiry, not `duration` interpreted as seconds. |
| `legendary_reduce_fear()` | Support timed `SPELL_FEAR` and `SPELL_DRAGONFEAR` records carrying `AFF_FEAR`, excluding negative-duration and explicitly scripted/boss-locked effects. Reduce/remove exactly one eligible record using existing affect removal/recalculation. For ties choose oldest application. Maintain a seconds-expiry override so 12/18-second shortening does not gain an extra legacy round. Other fear sources remain unsupported until explicitly mapped. |
| `legendary_clean_bleed()` | Support `SKILL_BLEEDING_ATTACK`, `ABILITY_BLEEDING_CRITICAL`, `SKILL_TIGER_CLAW`, `EVOLUTION_BLEED_EFFECT` records carrying `AFF_BLEED`. Oldest is the tail-most matching insertion in the affected list. Exclude magical shard/psionic, acid and profane bleeding despite sharing the bit. Remove one source, recompute bits and leave other bleeds intact. |
| `legendary_damage_context()` | `fight.c:damage_handling()/damage()`, `act.offensive.c:breath_weapon_callback()` and legacy `call_magic` breath sites. Carry explicit source category, action/packet ID, effective damage type and nonrecursive legendary flags. Apply final protection after ordinary reductions/redirection, before HP debit. Type alone cannot identify dragon breath. |
| `legendary_is_dragon_breath()` | Require actual dragon source and a registered native breath origin. `utils.h:IS_DRAGON()` / `RACE_TYPE_DRAGON` supplies actual race classification. Do not treat every fire attack, dragonborn power or renamed mob as dragon breath. |
| `legendary_dragon_mount()` | `utils.c:is_riding_dragon_mount()`, existing rider/mount links, owner/master identity and real alive/room state. Registry recognition of a true dragonlance is independent of its keywords/material. |
| `legendary_cast_payment()` | `spell_parser.c` PSP expenditure, augmentation, concentration checks and cancellation/refund paths. Record actual cost, eligible points spent, failure stage, refunds already issued and effect delivery by cast ID. Prepared/Vancian slot casts with no point expenditure receive no invented point rebate or extra slot. |
| `legendary_effective_heal()` | `magic.c:process_healing()/mag_points()` and other direct healing paths. Measure actual missing HP restored, not requested healing; delayed ticks revalidate. Maximum-pool foundations do not refill pools. |
| `legendary_formation_state()` | `handler.c` / `handler.h`: `get_formation_row()`, `formation_allows_attack()`, `formation_can_melee_target()` and group membership. S02 adds check bonuses without changing exposed rows or formation reach rules. |

Damage/resource context must also cover direct HP mutations that bypass
`damage()`: this audit found such cases in `fight.c`. Explicitly classify them
as unsupported or route eligible ordinary damage through the common adapter;
never silently advertise universal ward coverage while missing real attacks.
The launch definition of eligible physical damage is slashing, piercing and
bludgeoning weapon/natural-hit packets, not bleeding, poison or generic suffering.

Generic hard-control immunity is not currently one universal predicate. Provide
`legendary_control_immune()` combining existing immunities with the registered
boss hard-control policy. S11 must never block script transitions or staff moves.
Use `savingthrow_flatdc()` for its fixed DC25 Fortitude; no caster DC scaling.

Resource and condition work must not rewrite unrelated class mechanics. A
recorded eligible expenditure/failure is required for refund; the current
concentration checks can occur before spending, so failure alone is not proof
of a refundable payment.

## 7. Existing dragonmetal versus legendary currency

The repository already defines `MATERIAL_DRAGONMETAL` and
`CRAFT_MAT_DRAGONMETAL`, plus dragonmetal crafting/refining code. The new personal
**dragonmetal fragment balance is separate**. Existing ore, refined material or
ordinary object material does not automatically grant fragments, bypass daily
limits, establish awakening or qualify a true dragonlance. Use explicit UI names
to distinguish these systems. No recipe/material migration is needed in Phase 1.

## 8. Baseline verification

Run from the repository root, without contacting the live game database:

| Check | Result |
| --- | --- |
| `make -j4` | Passed; no compiler warning/error lines in captured build output. Incremental build, not a clean rebuild. |
| `tests/run_estate_integration.py` | Passed with a disposable MariaDB instance, both stub and production object codec, including journal recovery and housing/stalls/auction. Its deliberate SQL fault-injection diagnostics are expected. |
| `tests/run_quarterstaff_grip.py` | Passed. |
| `tests/run_quest_returns.py` | Passed. |
| `tests/run_quest_sharing.py` | Passed. |
| `tests/run_buildwalk_overlap.py` | Passed, including its address/undefined-behavior sanitizer harness. |
| `tests/run_formation.py` | Baseline failure: `ValueError: substring not found` while extracting an old `formation_penalty` source pattern. No test combat executed; this is a stale fixture, not evidence of a formation gameplay regression. |

The formation fixture must be repaired to match the actual source before the
later formation-related regression gate can pass. It was not changed during this
audit because no legendary runtime implementation has been introduced yet.
Other test suites and full sanitizer/game-server runs were not claimed.

## 9. Phase-1 exit and handoff

All Phase-1 interface questions have explicit decisions above. Remaining work is
implementation and proof in the assigned later phases, not a request to guess
at persistence or condition behavior during power coding.

- Phase 2 can begin with stable definitions, prices and pure rule fixtures.
- Phase 3 must implement every codec/UUID route and proposed storage tables.
- Phase 4 must harden file checkpoints, link estate payment IDs, implement
  pending-channel reconciliation and prove crash recovery before purchases.
- Phases 7–12 must implement ownership, hand/action/condition/context adapters,
  source cleanup and all advertised catalog effects before enabling them.
- Phase 13 must repair the stale formation fixture and run full integration.
- Actual smith, boss and dragonlance vnums remain deployment inputs in Phase 15;
  none are invented or edited in live world files here.

Phase 1 does not certify future runtime correctness, balance, crash safety or a
production launch. It supplies the reviewed integration map and decisions needed
to implement those requirements in the planned order.
