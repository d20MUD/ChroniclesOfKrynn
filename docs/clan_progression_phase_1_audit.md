# Phase 1 — Clan Progression Integration Audit

Status: complete. Read-only code audit plus documentation; no gameplay, storage, configuration, or world data changed.

References: [design](clan_progression_design.md), [implementation plan](clan_progression_implementation_plan.md).

## Findings That Affect Implementation

1. **Existing clan transactions are not crash-durable cross-entity transactions.** They provide in-memory operations/rollback and sequential saves, not a persistent reward receipt or atomic clan-plus-character commit.
2. **Clan vnums are not sufficient lifetime identities.** Creating a clan uses `highest_clan_vnum() + 1`. Deleting the highest-numbered clan permits reuse. Favor and cooldown history need a persistent clan incarnation identity, not a list index or vnum alone.
3. **The project already has durable persistence building blocks.** Checked character saves, atomic file replacement, SQL receipts, and recovery code exist. Reuse these patterns rather than create an unrelated storage platform.
4. **PSP regeneration shares a helper with movement recovery.** Adding Psionic Focus directly to that helper would also increase movement recovery.
5. **Crafting skill values are also used for progression and requirements.** Adding Artisan's Guidance directly to the raw skill getter risks changing skill advancement and eligibility instead of just crafting checks.
6. **Existing participation recording is endgame-specific.** Legendary encounter code already records damage/support and stable encounter IDs, but its recipient and encounter filters do not cover the proposed all-level clan system.
7. **Territory benefit declarations do not establish active integration.** The general `apply_zone_*` functions currently have no callers outside their definitions in `src/clan.c`. Clan shop discounts/tax have separate, real callers in `src/shop.c`.
8. **Two relevant existing test harnesses currently fail to link.** Record and repair their missing mocks before relying on them for later integration gates. These failures occurred before any progression code was introduced.

## 1. Identity and Membership

| Identity | Current authoritative source | Integration constraint |
| --- | --- | --- |
| Clan runtime position | `clan_rnum`, `clan_list[]`, `real_clan()` in `src/clan.c` | List positions can change on removal; never persist a runtime index as identity. |
| Clan saved number | `struct clan_data.vnum`, `#<vnum>` records, `highest_clan_vnum()`, `do_clancreate()` | Useful for existing commands/world links, but reusable after deletion; add a persisted incarnation ID for progression history. |
| Character | `GET_IDNUM(ch)`, `Id  :` player-file tag in `src/players.c` | Use the saved character ID, not display name or transient DG/runtime `GET_ID(ch)`. |
| Account | `struct account_data.id`, `ch->desc->account->id`, SQL `account_data` and `player_data.account_id` | Account name is not the cap key. Use database account ID; do not fall back to character identity for missing account data. |
| Encounter | `legendary_encounter_id` and `legendary_xp_kill_context()` in `src/legendary_earnings.c` | Reusable stable-event pattern; audit/generate identity for clan-eligible encounters independently of endgame reward eligibility. |

Membership is stored through `GET_CLAN(ch)` and `GET_CLANRANK(ch)`, with matching player-index information. The implementation must handle applicants as distinct from accepted members: rank zero is not ordinary reward eligibility. Existing code uses both zero and `NO_CLAN` for absent membership; normalize queries without silently treating either as a real clan.

Relevant entry points in `src/clan.c`:

- `set_clan()`, `do_clanenrol()`, `do_clanleave()`, `do_clanexpel()`.
- `do_clandestroy()`, `remove_clan()`, `do_clancreate()`.
- `do_clanpromote()` and `do_clandemote()` for privilege changes.
- `do_clan()` and `check_clanpriv()` for command dispatch/authorization.
- `copy_clan_data()`, `clear_clan_vals()`, `free_clan()` for any new allocated state; check their actual ownership rules before adding a pointer.
- Offline membership updates and login notifications: `src/clan_transactions.c`, `src/players.c`, and `tests/run_clan_login_notifications.py`.

Appending a clan privilege must preserve existing persisted privilege indices and rank semantics. Favor standing must remain independent of officer rank.

## 2. Existing Persistence

### Clan records

`src/clan_edit.c` owns `save_clans()`, `save_single_clan()`, `load_clans()`, and `mark_clan_modified()`. `CLAN_FILE` is `LIB_ETC "clans"`, defined in `src/db.h`.

The file is human-readable tagged text: `#<vnum>` begins a clan; examples include `Name:`, `Lder:`, `Bank:`, `Rank:`, and `Priv:`; `$` terminates the file. Descriptions and some lists use tilde-terminated blocks. Loading starts from `clear_clan_vals()` defaults. Unknown tags are not a general extension mechanism: the loader reports unknown tags.

There are two separate serialization paths, full-save and single-clan rewrite. Both must preserve progression fields and the new incarnation identity. The single-clan writer reconstructs the changed record, so adding a tag to only the full writer will lose data on later updates.

Current durability limitations:

- `save_clans()` opens the destination directly with `"w"`.
- `save_single_clan()` writes a temporary file, moves the original to a backup, then renames the temporary file into place. The original pathname can be absent between those renames; startup does not provide a general journal replay for progression transactions.
- The APIs return `void`, so their callers do not receive a positive durable-save acknowledgment.
- Neither path is a multi-record reward transaction with character/account/inventory storage.

Phases 4–5 must decide on checked writes and safe file replacement, not merely add fields to the existing unchecked save calls.

### Character records

`src/players.c` provides `load_char()`, `save_char()`, and `save_char_checked()`.

- Player files are tagged text. Existing identifiers/membership include `Id  :`, `Cln :`, and `CPts:` for legacy clan points.
- `save_char()` discards the checked result; economic/progression commits should use the checked interface where a durable acknowledgment is required.
- `save_char_checked()` serializes through a memory stream and calls `durable_file_replace()` after checking the stream.
- `src/durable_file.c` writes a temporary file, syncs it, renames it, and syncs the parent directory. Reuse this implementation and its fault-injection patterns.
- A rename followed by a failed directory sync can leave the new file visible even though the helper returns failure. Recovery must inspect receipts/state; a false return does not prove that no write became visible.
- Saving temporarily strips/restores derived character effects. New transient clan bonuses must not become permanent serialized base statistics.
- New allocated personal records need initialization, reset, load/save, and cleanup coverage, including `free_char()` in `src/db.c` and offline `load_char()` use.

Favor needs multiple records keyed by character ID and clan incarnation. A single scalar alongside `GET_CLANPOINTS` cannot retain distinct former-clan standings.

### Account identity and limits

`src/account.c` loads/saves SQL-backed accounts with `load_account()` and `save_account()`. The account struct is in `src/structs.h`; `player_data.account_id` associates characters with the numeric account ID.

Existing per-descriptor account structs are not a suitable sole source of truth for shared contribution windows. Different sessions/alts can hold separate account objects. New limits require an authoritative account-keyed store/cache and durable reset state. Define how unavailable identity/storage disables new awards without affecting ordinary play.

No account credentials or private player records were needed for this audit.

## 3. Transaction and Recovery Reuse

| Existing component | What it provides | What it does not provide automatically |
| --- | --- | --- |
| `src/clan_transactions.c` | Validation, in-memory operation lists, rollback, sequential saves, activity logging | Persistent transaction IDs, restart replay, crash-safe shared/personal reward commits, or durable save acknowledgment |
| `src/durable_file.c` | Checked replacement of one file, with file/directory sync | Atomic changes across multiple files or files plus SQL |
| `src/legendary_transaction.c` | Persisted operation states, checked character checkpoints, SQL commits and recovery | A general clan reward service; it is domain-specific and must not be called as if clan rewards were legendary upgrades |
| `src/legendary_earnings.c` and its SQL migrations | Stable source-event IDs, award receipts, retry/recovery patterns | All-level clan reward rules, clan-owned Influence, or account contribution windows |
| `src/estate.h` and estate receipt/recovery implementation | Durable money/delivery receipt APIs and recovery ordering | Automatic atomicity with newly added clan fields or arbitrary material decrements |

Recommended approach for phase 8: reuse the project's existing database and durable checkpoint/receipt patterns. First define the authoritative balances, operation states, replay order, and acknowledgment semantics. Do not maintain competing authoritative totals in SQL and flat files without explicit reconciliation.

Every economic event should identify the character, numeric account, clan incarnation, source instance/claim, configured reward, and reset window. Retry/restart must resolve the same event, not generate a fresh ID. Delivery/purchase transactions must also cover consumed materials/currency and item delivery. Activity logs alone are not reward receipts.

**Timebox risk:** phase 8 is likely to require multiple 15–20-minute blocks: receipt schema/interface, commit/replay service, and interrupted-write validation. Do not claim the full durability acceptance check from an in-memory deduplication set. Phases 15, 29, 36, and 40 depend on this being real.

## 4. Gameplay Hook Map

| Feature | Actual entry points | Required boundary |
| --- | --- | --- |
| Kill, bounty, expedition progress | `dam_killed_vict()`, `solo_gain()`, `group_gain()`, `perform_group_gain()` in `src/fight.c` | Assign one death-event identity before extraction and after legitimate-death checks. Do not award independently from every XP path. Resolve pets to owners and validate each participant. |
| Support participation | `legendary_record_action()`, `legendary_record_support()`, `legendary_boss_death()` in `src/legendary_earnings.c`; their callers in combat/magic code | Existing `recipient()` requires level 30; action tracking also requires a configured boss. Extract/reuse the pattern or build a separate broader tracker. Simply grouping in the same room is not proof of participation. |
| Character adventuring XP buff | `gain_exp_with_source()` in `src/limits.c`, and source contexts from `src/legendary_earnings.c` | Distinguish adventuring sources from crafting/other rewards. Derive clan contribution before XP modifiers and independently of the character XP return value/cap. Damage XP exists separately from kill XP. |
| Supply-order rewards | `complete_supply_order()` in `src/crafting_new.c` | Hook actual successful turn-in after prerequisites and within the receipt/commit boundary. `craft_supplyorder_complete()` finishes a production unit; it is not the turn-in reward event. |
| Harvest progress | `harvest_complete()` and `butcher_complete()` in `src/crafting_new.c` | Record qualifying base output before expertise/efficiency/happy-hour additions. Inventory/material totals cannot establish that fresh gathering occurred. |
| Crafting buff | Check sites in `craft_create_complete()`, `craft_refine_complete()`, ammo crafting, and `craft_supplyorder_complete()` in `src/crafting_new.c` | `get_craft_skill_value()` returns ranks plus specialization and is also used by `gain_craft_exp()` advancement checks. Add a check-only modifier path; do not boost raw skill level or required-rank calculations. |
| Maximum-HP buff | `calculate_max_hp()` in `src/utils.c`; `affect_total()` / `affect_total_plus()` in `src/handler.c`; `legendary_foundation_hp()` in `src/legendary_foundations.c` | Ordinary maximum is assigned to `GET_REAL_MAX_HIT`; the legendary helper then sets effective maximum. Define the non-clan base and ordering before changing this path. Repeated recalculation must not compound or heal; expiration/departure must invalidate online members safely. |
| Prepared and spontaneous recovery | `compute_spells_prep_time()`, `reset_preparation_time()`, `process_prepared_caster_prep()`, `process_spontaneous_caster_prep()` in `src/spell_prep.c` | Both queue types share timing but have separate processing. Existing feats/perks/room/config reductions and the final `MAX(circle, prep_time)` floor must remain effective. Define active-queue transitions without repeatedly rescaling remaining time. |
| PSP recovery | `regen_psp()` in `src/limits.c`, `get_psp_regen_amount()` in `src/utils.c` | Apply a PSP-specific bonus to the intended actual recovery, with a rounding policy, out-of-combat eligibility, and final maximum clamp. Do not change the movement recovery caller of the same helper. |
| Territory stacking | `get_zone_control_benefits()` / `apply_zone_*()` in `src/clan.c`, `src/clan_benefits.h` | General bonus helper definitions currently lack external callers. Preserve the separately active shop discount/tax paths in `src/shop.c` and `src/clan_economy.c`; do not accidentally activate dormant territory bonuses. |
| Quartermaster items and services | Existing object prototypes and item ownership; `estate_queue_delivery()` / receipt patterns; clan command dispatch | New catalog/eligibility/commit service is needed. Reuse normal item restrictions and delivery behavior; Favor is an unlock, not a spendable balance. |

Cross-cutting constraints:

- Existing group XP eligibility is chiefly group/room based. It is not sufficient for the proposed support-aware clan participation policy.
- Legendary participation has explicit useful-action, recent-activity, owned/test-instance, and pet-owner considerations worth preserving when generalized. Its entire endgame gate must not be copied.
- Generic craft XP, gold changes, inventory additions, and quest completion are not interchangeable clan award hooks.
- Buff eligibility must check membership, live expiration, and feature enablement at query time, even if scheduled cleanup has not run yet.
- Clan-level changes, login, departure, cancellation, expiration, and feature disablement must invalidate affected derived stats. Locate login/copyover reconciliation in `enter_player_game()` and nearby recovery code in `src/interpreter.c`, and `copyover_recover()` in `src/comm.c`.

## 5. World Content and Build Ownership

`boot_world()` / `index_boot()` in `src/db.c` load indexed room, mob, object, zone, shop, and quest data. Use saved vnums for content references, not runtime rnums.

The checked workspace world, development world, and live world are separate paths:

- `/home/krynn/code/lib/world`
- `/home/krynn/dev/mud/lib/world`
- `/home/krynn/live/mud/lib/world`

`lib/world/mob/index` and `lib/world/obj/index` are ignored by Git in this checkout. A checked-in code change or prototype file does not prove that a live indexed world contains that content. Phase 38 must curate against the intended staging world and validate alternatives before deployment. No targets were selected or world files modified in phase 1.

`CMakeLists.txt` explicitly lists server sources, including clan modules and durable persistence helpers. A new focused module will need an explicit build entry. Respect the campaign configuration and keep progression disabled for unaffected campaigns/presets until deliberately enabled.

## 6. Text and Web Integration

Server:

- Clan command dispatcher/privileges: `src/clan.c`, `src/clan.h`.
- Protocol variable registration: matching enum/table entries in `src/protocol.h` and `src/protocol.c`.
- Initial/explicit refresh: `update_msdp_descriptor()` in `src/comm.c`.
- Recurring refresh: `msdp_update()` in `src/comm.c`.
- Login/copyover reconciliation must load cooldown state before effects are queried or reported.

The previous ammo status issue demonstrated that updating only the explicit refresh path can miss clients that subscribe later. Phase 42 must test late subscription and recurring updates, not just serialization in isolation.

Web repository: `/home/krynn/LuminariWebClient`.

- `shared/mud.ts`: state shape, variable defaults, normalization.
- `server/index.ts`: per-MUD reporting subscriptions, state mapping, reconnect/copyover behavior.
- `src/App.tsx` and `src/App.css`: compact clan view and actions.
- Server responses must provide authoritative eligibility/rejection reasons. Browser actions submit an intent and identifiers, never award amounts, elapsed time, or self-certified completion.

The web repository already has unrelated uncommitted changes. None were modified during this audit.

## 7. Test Baseline and Coverage Gaps

Executed against the current checkout, without progression code changes:

| Command | Result |
| --- | --- |
| `python3 tests/run_clan_login_notifications.py` | PASS: online/offline membership notices, decisions, permission checks, persistence ordering, and promotions/demotions. |
| `python3 tests/run_account_bonuses.py` | Existing harness fails to link: missing `web_info_begin`, `web_info_field`, `web_info_finish`, and `show_pers` dependencies from the extracted treasure code. No progression behavior was executed. |
| `python3 tests/run_supply_order_project.py` | Existing harness fails to link: missing `parse_at` dependency from the extracted crafting code. No progression behavior was executed. |

Do not silently classify those two failures as progression regressions or as passing coverage. Repair their extraction mocks in separate small test-maintenance blocks before using them as later acceptance gates.

Useful existing suites identified but not run in this documentation-only phase:

- `tests/run_legendary_earnings.py`, `tests/run_legendary_transactions.py`, `tests/run_legendary_persistence.py`, and `tests/run_estate_integration.py`: existing disposable-database/recovery and fault-injection patterns. Inspect their environment requirements before running; never substitute production storage.
- `tests/run_happy_crafting.py` and `tests/run_insightful_harvesting.py`: production harvest/butcher/XP paths and bonus-yield interactions.
- `tests/run_craft_ammo.py` and `tests/run_crafting_treasure.py`: relevant material and crafting behavior.
- Web `tests/session.test.mjs`, `tests/help-msdp.test.mjs`, `tests/output-windows.test.mjs`, and `tests/heartbeat.test.mjs`: gateway transport/reporting, safe rendering, and lifecycle patterns.

New behavior-specific coverage will be required for clan save/load migration, persistent incarnation IDs, all-level participation, account caps across sessions, receipt recovery, cooldown family identity, HP recalculation, preparation queues, and PSP-only recovery. Existing tests do not establish these guarantees.

No server/client build was necessary: phase 1 changes documentation only. No new tests mirroring an unimplemented system were added.

## 8. Phase 2 Handoff

Phase 1 acceptance is satisfied: the storage formats and identities are identified; each gameplay area has concrete entry points; transaction, lifecycle, and baseline-test limitations are recorded.

Phase 2 should produce the balance/configuration specification, with these audit requirements carried forward:

1. Include an immutable clan incarnation identity alongside the existing vnum. Decide migration and deletion/tombstone behavior before personal history is written.
2. Set numeric account ID as the contribution-cap key; define the scope across characters/clans and unavailable-storage behavior.
3. Specify qualifying encounter difficulty/participation for all levels and capped characters without relying on received character XP.
4. Specify the exact ordinary/legendary/non-clan base for percentage HP, prepared/spontaneous queue transition semantics, and the actual PSP recovery quantity affected.
5. Keep Artisan's Guidance on checks, not raw crafting ranks or advancement thresholds.
6. Keep 24 hours active plus 12 hours downtime as the provisional buff defaults; preserve one family cooldown across all tiers and cancellation.
7. Specify durable transaction requirements using existing persistence primitives; split phase 8 when implementing the actual commit/replay service.
8. Record the two baseline harness repairs as preparation work, separate from balance design and feature mechanics.

Do not implement phase 2 values, schemas, or gameplay hooks as part of this audit.
