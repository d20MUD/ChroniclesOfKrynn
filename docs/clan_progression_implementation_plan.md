# Clan Progression Implementation Plan

Status: Phases 1–11, including 4a–4c and 5a–5b, are complete as disabled foundations. Version-5 persistence and reward policy now include membership eligibility, retained-history lifecycle operations and atomic SQL creation/disbanding. Phase 12 text readouts and restricted inspection is next. No game database migration, live award, deployment or progression enablement has occurred.

Design reference: [Clan Progression and Personal Favor](clan_progression_design.md).

Implementation defaults: [Phase 2 balance/configuration specification](clan_progression_phase_2_spec.md), version 1. Read this before Phase 3; it takes precedence over illustrative concept values.

## Scope

Implement the proposed clan progression system in small, independently reviewable steps:

- Clan XP and levels, spendable shared Influence, and cumulative personal Favor associated with a character and a specific clan.
- Two early active buff slots and a maximum of three later, with at most one combat buff active.
- Useful buffs for spell preparation/recovery, PSP regeneration, maximum HP, adventuring XP, and crafting.
- Clan-wide buff cooldowns that enforce downtime and cannot be bypassed by changing tier, slot, officer, or membership.
- Contribution from qualifying combat, supply orders, capped deliveries, gathering, and bounties.
- A small set of group expeditions using existing world areas and enemies.
- Favor standings, quartermaster equipment unlocks, and purchases.
- Matching text-command and web-client information, including explicit eligibility and cooldown explanations.

Do not depend on expansion of the conventional quest catalog. Do not add movement, lore-check, or fear-only buffs to the initial library. Preserve existing clan point uses; do not convert officer-awarded clan points or bank deposits into Favor or Clan XP.

## Timeboxing and Execution Rules

Each numbered phase is intended to fit an approximately **15–20-minute AI work block**, including its focused validation and handoff notes. These are estimates, not guarantees. A phase that exceeds the timebox should be split at a working boundary and recorded as two smaller phases; do not skip checks to meet the estimate.

There are 52 numbered phases plus five MySQL migration subphases (57 work blocks): approximately **14–19 hours of active work**, excluding waiting for decisions, pilot observation, maintenance windows, and unusually expensive builds. Update the estimate after the first few phases.

For every phase:

1. Read the preceding handoff and inspect the relevant current code. Preserve unrelated work.
2. Implement only the listed deliverable and its necessary integration.
3. Add or run focused behavioral checks appropriate to the change. Tests should exercise actual production code and outcomes, not duplicate its calculations in a separate test implementation.
4. Build the affected target when code or integration changes require it. Keep incomplete features disabled by default.
5. Record changed files, checks/results, assumptions, and the next entry point in the progress log at the end of this document.

Dependencies refer to completed phases. Parallel execution is not assumed. Intermediate phases must leave the repository buildable; gameplay hooks remain disabled until their prerequisites are complete.

## Working Defaults

These make implementation concrete without treating provisional balance values as final:

| Topic | Working decision |
| --- | --- |
| Clan level range | 1–20; XP thresholds defined in phase 2 |
| Buff slots | Two initially, three at clan level 10 |
| Combat category | At most one combat buff; maximum HP belongs to this category |
| Buff duration | 24 real hours |
| Buff downtime | 12 additional real hours after scheduled expiration |
| Cooldown identity | Stable clan ID plus buff-family ID; all tiers share it |
| Next activation | Activation time + duration + downtime |
| Early cancellation/replacement | No refund and no reduction of the saved next-activation time |
| Active refresh | Disallowed; no extension or automatic renewal in the initial release |
| Personal Favor | Cumulative and not spent; stored per character and clan |
| Membership departure | Stop clan buffs/services; retain that clan's Favor history and already purchased equipment |
| New economy | No conversion from legacy clan points or unlimited bank donations |
| Conventional quests | Optional future contribution source, not a launch dependency |
| Persistence | Prefer existing MySQL for clan-related data; migrate existing clan saves with a verified cutover |
| Deployment | Feature disabled initially; enable after acceptance checks and a controlled pilot |

A different buff family may occupy a free slot during another family's cooldown, subject to its own cooldown and category restrictions. This rotation is intended. A clan level-up does not reset timestamps or unlock a second copy of an already active family.

Phase 2 has settled the initial XP/Favor thresholds, Influence costs, contribution budgets, buff tier values, reset rules, and pilot flags in the linked specification. Changes after that should update both the specification and the affected tests.

## Repository Entry Points

Verify these against the working tree in phase 1; they are investigation starting points, not instructions to spread logic across all of them.

| Area | Starting points |
| --- | --- |
| Clan membership, commands, privileges, persistence | `src/clan.c`, `src/clan.h`, `src/clan_edit.c`, `src/clan_transactions.c`, `src/clan_economy.c` |
| Character persistence and account identity | `src/players.c`, `src/structs.h`, `src/utils.h`; locate the authoritative account identity and account storage |
| Combat and XP rewards | `src/fight.c`, `src/limits.c`, relevant XP award paths located during the audit |
| Prepared and spontaneous spell recovery | `src/spell_prep.c`, particularly `compute_spells_prep_time()` and the recovery queues |
| PSP regeneration | `get_psp_regen_amount()` in `src/utils.c`, its callers in `src/limits.c` |
| Maximum HP recalculation | Existing character stat/affect recalculation paths; identify their authoritative entry points before adding a bonus |
| Crafting, supply orders, gathering | `src/crafting_new.c` and the actual harvest award paths identified during the audit |
| Existing territory benefits | `src/clan.c`, `src/clan_benefits.h`; audit callers before assuming benefits currently apply |
| MSDP serialization and update loops | `src/comm.c`, `src/protocol.c`, `src/protocol.h` |
| Web gateway and client | Adjacent repository `/home/krynn/LuminariWebClient`: `shared/mud.ts`, `server/index.ts`, `src/App.tsx`, `src/App.css`, `tests/` |
| World content | Indexed live/staging world definitions; choose existing areas, enemies, and item templates without modifying production data during planning or development |

Prefer a focused clan progression module with a small public interface for queries, awards, buff activation, and eligibility. **Use the existing MySQL database for saving data wherever practical, including migrating current clan saved data into MySQL.** Reuse existing connection, SQL migration, transaction, and recovery conventions rather than introducing another database platform.

### MySQL Persistence Requirements

- Target transactional MySQL tables as the authority for migrated core clan records, membership/ranks/privileges, treasury, Clan XP, Influence, buff state/cooldowns, Favor, contribution windows, contracts/expeditions, and reward receipts.
- Preserve existing numeric clan references while introducing an immutable incarnation identity. Key historical Favor and cooldown data to that lifetime identity.
- Move existing clan load/save callers behind a checked repository interface. Compatibility player-file membership fields may remain mirrors where needed, with explicit reconciliation; they must not become a competing authority after membership cutover.
- Use database transactions for related SQL state and unique constraints for event/claim deduplication. Reconcile any remaining character gold/material/inventory file changes through durable receipt/recovery patterns.
- Keep existing personal clan-point behavior intact; migrating clan persistence does not turn clan points into earned Favor.
- Import legacy data with dry-run comparison and backups. Rerunning import must not overwrite newer database balances, recreate deleted clans, reset cooldowns, or duplicate relationships.
- Do not silently use stale legacy saves during a MySQL outage. Define explicit unavailable-storage behavior and recovery before enabling progression.
- Cutover and rollback must specify the authoritative store and handle any writes made after cutover. Reopening the old clan file alone is not a safe rollback.

The Phase 1 audit describes the current flat-file behavior. Its observations remain valid, but its storage recommendations are superseded by this explicit MySQL preference. No whole-game character/world migration is required.

Subphases below are separate 15–20-minute work blocks. A parent persistence phase is not complete until its associated subphases and acceptance checks are complete.

## Stage A — Foundations and Persistence

Phase 1 audit and handoff: [Integration audit](clan_progression_phase_1_audit.md). Read its identity, persistence, and baseline-test findings before starting phase 2.

| Phase | Deliverable for one work block | Acceptance check | Depends on |
| --- | --- | --- | --- |
| 1 | Audit clan/character/account storage, reward hooks, stat recalculation, world content ownership, and current tests. Write a short integration map and identify persistence/transaction constraints. | Every planned hook has an actual function/file entry point; existing clan data format and stable IDs are understood. No mechanics changed. | None |
| 2 | Write the initial balance/configuration specification: thresholds, buff tiers/costs, cooldowns, reward budgets, resets, eligibility rules, and pilot flags. Use the design's provisional values unless deliberately revised. | Representative small-clan and high-activity examples can be worked through; all values needed for the initial slice are specified. Record unresolved decisions before dependent work. | 1 |
| 3 | Add progression data types, stable buff-family IDs, configuration tables, disabled-by-default feature gates, and build integration. | Existing build passes; configuration rejects invalid levels, negative durations/costs, and duplicate families. | 2 |
| 4 | Add versioned MySQL schema migrations for core clan identity/progression, using transactional tables, immutable incarnation IDs, constraints, and preserved clan vnums. | Schema applies repeatably in a disposable database; invalid/duplicate identities and negative balances are rejected. | 3 |
| 4a | Implement checked MySQL repository reads/writes for core clan fields, treasury, progression, and timestamps. | Populated records round-trip with explicit failure results; balances and cooldowns are not truncated or silently reset. | 4 |
| 4b | Persist clan ranks, privileges, membership, and relationships through the repository, preserving existing statistics and hall references. | Representative legacy values round-trip; numeric privilege positions and character/clan references retain their meaning. | 4a |
| 4c | Adapt existing clan save and mutation callers to the repository behind the migration feature gate; specify player-file mirrors and their reconciliation. | Save failures propagate; gated operation preserves legacy behavior before cutover and uses MySQL authority afterward; no unhandled competing writer remains. | 4b |
| 5 | Implement a repeatable legacy clan importer with dry-run validation against the new MySQL records. | Every existing saved field is accounted for; row counts, balances, ranks, membership references, relationships, and statistics compare correctly; rerun cannot overwrite newer live state. | 4c |
| 5a | Implement MySQL boot loading, defaults, and cooldown reconciliation, with explicit legacy import/cutover selection. | Fresh and migrated records load safely; expired buffs retain unexpired cooldowns; malformed records and database outages cannot enable benefits or trigger silent stale-file fallback. | 5 |
| 5b | Rehearse import, cutover, retry, and rollback in disposable/staging storage; document the active authority and compatibility mirrors. | No membership or treasury loss; no deleted-clan resurrection or vnum-history collision; rollback handles post-cutover writes explicitly. | 5a |
| 6 | Persist personal Favor in MySQL keyed by character and immutable clan identity, including records for former clans. | Two clans' standings remain distinct across logout/login and clan switching; purchasing never decreases cumulative Favor. | 3, 5b |
| 7 | Add MySQL account-level contribution-window persistence and deterministic reset calculations. | Alts share the intended limits; logout/restart cannot reset a window; exact reset boundaries behave consistently. | 2, 6 |
| 8 | Add MySQL reward receipts, unique claim/event keys, and transactional award commits using existing database facilities. Define recovery for any remaining player/inventory file updates; split into extra blocks as needed. | Replaying a stable event ID cannot award twice; a simulated interrupted save recovers without losing or duplicating the reward. Cross-file writes are not assumed atomic. | 5b, 6, 7 |
| 9 | Implement the Clan XP/level and Influence award service, without gameplay event hooks. | Level thresholds, level cap, multi-level awards, overflow bounds, and Influence totals behave correctly. | 8 |
| 10 | Implement personal Favor awards, standing lookup, contribution budgets, and reward explanations through the same receipt service. | Repeated events, exhausted account limits, and lower-level contribution examples produce the specified outcomes. | 9 |
| 11 | Integrate membership departure, joining, expulsion, and clan deletion rules with progression records and access queries. | Old Favor is retained but does not transfer; leaving stops eligibility; a deleted/recreated clan cannot inherit another clan's history by accident. | 6, 10 |
| 12 | Add text readouts and restricted inspection tools for clan progress, personal Favor, and contribution windows. | Ordinary members see their own status; privileged inspection follows permissions; no general officer command can mint earned Favor. | 9–11 |

## Stage B — Buff Lifecycle and Gameplay Effects

| Phase | Deliverable for one work block | Acceptance check | Depends on |
| --- | --- | --- | --- |
| 13 | Implement pure queries for unlocked buffs, tiers, slots, categories, active state, and family cooldowns. | All tiers share one cooldown; slot and combat-category limits are enforced; queries do not mutate state or spend resources. | 5, 9, 11 |
| 14 | Implement activation validation and structured failure reasons, including permissions, unlocks, affordability, current activity, and cooldown. | Validation explains every rejection and consumes nothing. Switching officer, tier, or slot cannot bypass cooldown. | 13 |
| 15 | Implement a persisted activation/replacement commit: resources, active slot, scheduled expiration, cooldown, and activity log. | A successful activation charges once; interruption/retry cannot double-charge or omit cooldown; failed replacement leaves the old buff intact. | 8, 14 |
| 16 | Implement expiration, cancellation, login/restart/copyover reconciliation, and member-stat invalidation notifications. | Cancellation preserves the original next-use timestamp; offline time advances normally; expiration occurs exactly once; expired buffs stop affecting queries immediately. | 15 |
| 17 | Apply Company Vitality through the authoritative maximum-HP recalculation path. | Repeated recalculation does not compound the bonus; activation does not heal; expiry, leave, death/recovery, and replacement clamp HP safely without directly dealing damage. | 16 |
| 18 | Apply Arcane Readiness to prepared-caster preparation timing. Define how active queues react to activation and expiration without repeated rescaling. | Normal prerequisites and interruptions still apply; no instant preparation; repeatedly changing buffs cannot accelerate a queue multiple times. | 16 |
| 19 | Cover the applicable spontaneous-caster recovery paths with the same buff semantics. | Representative prepared and spontaneous classes receive the intended benefit; unaffected resource systems remain unaffected. | 18 |
| 20 | Apply Psionic Focus to out-of-combat PSP regeneration with a defined small-value rounding policy. | Low regeneration receives a real, bounded benefit; combat does not receive the bonus; repeated regen calls do not duplicate fractions or exceed PSP maximum. | 16 |
| 21 | Apply Scholar's Encouragement at the authoritative adventuring XP award point. | Character XP receives the correct bonus once; Clan XP, Influence, and Favor do not inherit it; max-level contribution eligibility remains independent. | 16 |
| 22 | Apply Artisan's Guidance through the shared relevant crafting-check calculation. | Selected crafting and supply-order checks receive one bonus; the effect does not grant materials or multiply clan reward credit. | 16 |
| 23 | Audit and enforce stacking with territory effects, gear, spells, feats, and other clan buffs for the initial library. | Document the chosen stacking policy and test representative overlapping sources; no duplicate same-family benefit. | 17–22 |
| 24 | Add player-facing buff list/activate/cancel commands, permission integration, and cooldown/resource messages. | Every rejected action explains why; active and next-available times are shown; text commands cannot bypass the validated commit service. | 14–23 |

The initial slice implements the five buff families above. Martial Drills, Company Warding, Healer's Covenant, and Gatherer's Bounty remain planned additions after their combat/economy budgets are tuned. Add each as its own 15–20-minute block plus any necessary integration/test block; do not hide them inside phase 23.

## Stage C — Contributions and Contracts

| Phase | Deliverable for one work block | Acceptance check | Depends on |
| --- | --- | --- | --- |
| 25 | Implement a shared combat-contribution eligibility query using actual encounter difficulty and participation data. Include support roles and account identity. | Trivial, summoned, manipulated, or otherwise ineligible kills are rejected according to the specification; eligibility is not determined by killing blow alone. | 1, 2, 10 |
| 26 | Connect qualifying kills to the small background contribution award, using stable event receipts and account limits. | One kill awards at most once per eligible recipient; character XP buffs do not raise clan rewards; capped characters can contribute. | 8, 25 |
| 27 | Add a supply-order completion award hook after actual successful turn-in. | Merely crafting an item, previewing an order, or repeating a completed turn-in gives no extra award; existing order cooldowns still apply. | 8, 10 |
| 28 | Add data-driven delivery contract definitions, bounded quantities/grades, access rules, and readouts. | Available contracts display exact requirements and credit limits; invalid definitions fail validation rather than creating unlimited donation sinks. | 10, 12 |
| 29 | Implement delivery turn-in with resource removal and progression awards in a recoverable commit. | Wrong items and insufficient quantities consume nothing; a successful delivery consumes the specified resources once; retry cannot duplicate awards. | 8, 28 |
| 30 | Connect gathering contracts to authoritative base gathering events and persist progress. | Purchased/transferred materials do not masquerade as fresh gathering; bonus yield does not multiply clan contribution credit; reconnect preserves progress. | 8, 10, 28 |
| 31 | Implement bounty objective progress and completion from qualifying combat events. | Only designated eligible targets count; support participants receive credit; completion awards once and displays remaining objectives. | 25, 26, 28 |
| 32 | Implement contract rotation/reset behavior and repeat limits across all initial contract sources. | Reset cannot duplicate claims or erase committed rewards; alternate characters respect shared limits; expired contracts have explicit handling. | 27–31 |

## Stage D — Expeditions

| Phase | Deliverable for one work block | Acceptance check | Depends on |
| --- | --- | --- | --- |
| 33 | Add expedition definitions and validation for Stronghold Assault and Regional Hunt: area/target lists, quotas, alternatives, difficulty bands, and reward IDs. | Invalid or missing world references are reported clearly; definitions do not require whole-zone clears, new instances, or conventional quests. | 25, 28, 32 |
| 34 | Implement expedition enrollment, roster eligibility, and a stable outing identity. | Two eligible clan participants can start; clan changes and roster substitutions cannot bypass participation requirements; ordinary solo contracts remain available. | 11, 33 |
| 35 | Track expedition combat objectives from the same authoritative participation events. | Only relevant kills advance objectives; support credit works; ordinary mob respawns do not erase progress or multiply a single event. | 25, 34 |
| 36 | Implement expedition completion and shared/personal claims with separate reset limits. | The clan receives its shared award once per period; later eligible participants can earn personal Favor; repeat claims cannot duplicate rewards. | 8, 32, 35 |
| 37 | Persist and reconcile active outings, expiry/abandonment, member reconnects, and server restarts. | A restart resumes valid progress; expired/abandoned outings cannot be claimed; partial saves do not reset reward receipts. | 34–36 |
| 38 | Curate and validate a small staging expedition roster using existing areas and alternate targets. | Each contract is completable with actual world content, including a fallback when a named enemy is unavailable; level bands support small clans. | 33–37 |

## Stage E — Favor Rewards and Equipment

| Phase | Deliverable for one work block | Acceptance check | Depends on |
| --- | --- | --- | --- |
| 39 | Define an initial small quartermaster catalog with clan level, Favor standing, character level, price, proficiency coverage, and normal item power budgets. Use existing templates where suitable. | Each item has an explicit budget and eligibility explanation; activated spells pay for their utility; catalog validation catches bad item references. | 2, 10, 11 |
| 40 | Implement quartermaster purchase/commission validation and a recoverable cost-plus-item delivery commit. | Failed purchases consume nothing; retry does not duplicate items; Favor is not spent; existing item restrictions apply. | 8, 39 |
| 41 | Verify item ownership/binding policy, departure behavior, and access boundaries for the initial catalog. | Purchased gear remains usable after leaving as designed; new purchases and clan services require current membership; ordinary item restrictions are preserved. | 11, 40 |

Personal service choices and multi-stage expeditions are subsequent features. Before implementing a personal service, specify its cost, cooldown, persistence, and interaction with clan buffs. Implement one service per small phase rather than silently expanding the quartermaster purchase phase.

## Stage F — Web Client and Documentation

| Phase | Deliverable for one work block | Acceptance check | Depends on |
| --- | --- | --- | --- |
| 42 | Add a bounded, server-authoritative clan progression read model and MSDP report, including active buffs, family cooldowns, contribution state, and lock reasons. Integrate both initial and periodic refresh paths. | A late subscriber receives current state; shooting/crafting/completion/expiry/membership changes update the relevant state; private data is limited to the requesting member. | 12, 24, 32, 37, 41 |
| 43 | Wire subscription defaults, normalization, state mapping, and reconnect/copyover resubscription through the web gateway. | Actual serialized reports survive the gateway; old or absent fields fail gracefully; unrelated MUD presets do not subscribe unintentionally. | 42 |
| 44 | Add a compact clan panel showing Clan XP/level, Influence, Favor standing, and next unlocks. | Both narrow and wide layouts remain readable; colors and item names render safely; ordinary members see appropriate data. | 43 |
| 45 | Add active buff/cooldown displays and authorized activation/cancellation actions. | Disabled actions explain cooldown, slot, permission, or cost restrictions; the server revalidates every click; stale UI cannot spend twice. | 24, 44 |
| 46 | Add contract/expedition progress and quartermaster eligibility displays; route supported actions through the existing command method. | Progress agrees with text output; blocked rewards explain all relevant gates; the client cannot submit reward amounts or declare its own completion. | 38, 41, 44 |
| 47 | Update help files, applicable SQL help entries, operator configuration notes, and the design/implementation handoff. | Commands, actual cooldowns, buff categories, earned currencies, and reset behavior match the implementation; sample workflows can be followed without undocumented steps. | 24, 32, 38, 41, 46 |

## Stage G — Acceptance and Rollout

| Phase | Deliverable for one work block | Acceptance check | Depends on |
| --- | --- | --- | --- |
| 48 | Run an integrated buff lifecycle scenario: activation, replacement, expiry, reactivation lockout, tier changes, membership departure, and copyover/restart. Fix only issues within the timebox; split additional fixes into new phases. | No repeat activation bypass, duplicate charge, persistent expired stat bonus, HP exploit, or timer reset. | 23, 24, 45 |
| 49 | Run integrated progression scenarios: two accounts, an alt, low- and high-level members, capped character, duplicate events, mixed-role expedition, and interrupted claims. | Shared and personal rewards remain consistent; limits hold; no gold-to-Favor shortcut; a small clan can progress. | 32, 37, 41, 46 |
| 50 | Prepare a staging MySQL migration/rollback rehearsal with old-format records, unavailable-database scenarios, and feature flags. Run the relevant full build/test suites once the final fixes pass. | Migration preserves legacy data and authority; no stale-file fallback; rollback handles post-cutover writes; disabling effects safely recalculates HP and retains progression/history; updated server and gateway/browser versions are identified. | 47–49 |
| 51 | Review controlled pilot observations and adjust only the balance tables and documented defaults. Observation time is outside this work block. | Actual clan leveling rate, Influence income/upkeep, Favor pace, and buff choices are evaluated for small and active clans; unintended reward loops are addressed. | 50 |
| 52 | Prepare the concrete release checklist and reviewable deployment package; perform the project's release approval process before production changes. | Backups, migration steps, copyover/restart requirements, feature enablement, monitoring, and rollback are specified. Mark release complete only after the deployed behavior is checked. | 51 |

## Acceptance Scenarios to Preserve

### Buff cooldown example

Given a buff with a 24-hour duration and 12-hour downtime activated at time T:

- It is active until T + 24 hours.
- It cannot be refreshed while active.
- The same family cannot be activated again before T + 36 hours.
- Canceling at T + 1 hour still leaves its next activation at T + 36 hours.
- Selecting a different tier, slot, or activating officer does not change that timestamp.
- Restarting at T + 30 hours restores six hours of remaining cooldown, not a fresh duration and not an available buff.
- At T + 36 hours, activation is allowed only if permissions, resources, unlocks, slot limits, and combat-category limits still permit it.
- A failed attempt neither spends resources nor extends/starts a cooldown.

### Reward and membership examples

- Replaying a supply-order completion cannot grant a second reward.
- The clan's first expedition completion and a later member's personal claim have distinct receipts and limits.
- An account cannot multiply a contribution cap through alternate characters.
- A max-level member can earn clan contribution without earning character XP.
- XP and gathering buffs do not create a feedback loop into clan progression.
- Leaving a clan stops its effects, preserves that clan's Favor record, and does not transfer standing to a new clan.
- Buying equipment does not reduce Favor or make the item stop working after a later clan departure.

## Deferred Work

Add separate timeboxed phases after the initial slice has been evaluated:

- Martial Drills, Company Warding, and Healer's Covenant, with explicit combat budgets and stacking tests.
- Gatherer's Bounty, with base-yield contribution accounting and economy checks.
- Individual Favor-unlocked service choices, each with a defined resource cost and cooldown.
- Multi-stage resource/combat expeditions.
- Conventional quest contribution when enough quests exist.
- Any future scheduling/automatic activation feature; it must respect cooldowns and spending permissions and must not refresh an active buff.

## Progress Log

Append a short handoff after each completed or split phase:

```text
Phase:
Status: complete / split / blocked
Files changed:
Behavior delivered:
Checks and results:
Assumptions or decisions:
Remaining work and next entry point:
```

A phase is complete only when its deliverable and acceptance check are satisfied. Source changes alone do not establish that a feature is active on the running game or web gateway.

### Phase 1

- **Status:** complete.
- **Files changed:** `docs/clan_progression_phase_1_audit.md`, this implementation plan.
- **Behavior delivered:** none; audited the concrete integration points and documented persistence formats, identity rules, transaction/recovery constraints, gameplay hooks, world ownership, and web refresh paths.
- **Checks:** clan login notification baseline passed. Existing account-bonus and supply-order harnesses fail to link because of missing extraction mocks; exact dependencies are recorded in the audit. Documentation whitespace checked. No build required for documentation-only changes.
- **Key findings:** clan vnums can be reused; current clan transactions are not crash-durable; durable file/SQL receipt patterns already exist; PSP recovery shares a helper with movement; crafting rank getters also drive skill progression; legendary participation gates are endgame-specific.
- **Next entry point:** phase 2 balance/configuration specification, incorporating the audit's eight handoff requirements. Keep the two harness repairs separate from mechanics. No source, world, database, or live configuration changes were made.

### Persistence direction update

- **Requirement:** use MySQL for saving data wherever practical, including migration of existing clan saved data.
- **Plan changes:** phases 4–8 now target MySQL persistence, with five additional 15–20-minute migration subphases (4a–4c and 5a–5b). Existing numbered phase IDs are retained.
- **Compatibility:** preserve current clan records, identifiers, memberships, bank balances, ranks, privileges, relationships, halls, statistics, and legacy clan-point behavior. Remaining file-backed player/inventory state requires explicit recovery across SQL/file boundaries.
- **Status:** documentation updated only; no SQL migrations, imports, code changes, or deployment performed.

### Phase 2

- **Status:** complete.
- **Files changed:** `docs/clan_progression_phase_2_spec.md`, `docs/clan_progression_design.md`, this implementation plan.
- **Behavior delivered:** none; specified version-1 staging/pilot values for levels, currencies, source rewards, diminishing returns, account/clan caps, shared claims, Favor standings, quartermaster policy, buff tiers/costs, and family cooldowns.
- **Audit decisions carried forward:** immutable clan incarnation IDs and numeric account IDs; MySQL authority and no stale-file fallback; separate current-cycle spell timing; HP calculated from the non-clan effective base; PSP-only rounding; check-only crafting bonuses; independent clan earning for capped characters; durable event/claim recovery.
- **Worked examples:** a three-account baseline clan earns 2,750 Clan XP and 1,375 Influence per modeled week; a ten-account high-activity clan earns 7,150 Clan XP and reaches the 5,000 Influence weekly cap. These are rate models to measure in the pilot, not observed live timings.
- **Checks:** independently checked the 20-level threshold table, five family IDs, reward arithmetic, account/clan cap examples, cooldown durations, and local Markdown links/whitespace. No source changed and no server/client build was required.
- **Still deferred:** actual world/catalog references and per-item prices in phases 38–39; later buff families/personal services; empirical tuning in phase 51. Existing account-bonus and supply-order harness link failures remain separate maintenance work.
- **Next entry point:** Phase 3 data types, stable IDs, validated configuration tables, feature gates, and disabled build integration. Use Phase 2 version 1 for initial values. No SQL migration, data import, award hook, active buff, or production enablement is authorized by this documentation-only phase.


### Phase 3

- **Status:** complete.
- **Files changed:** `src/clan_progression.h`, `src/clan_progression.c`, `CMakeLists.txt`, `tests/clan_progression_config.c`, `tests/run_clan_progression_config.py`, design status, this plan, and [Phase 3 handoff](clan_progression_phase_3_handoff.md).
- **Behavior delivered:** stable IDs, persistence-ready data types, checked eighth-point conversion, immutable Phase 2 configuration tables, validation, and feature gates. The module is included in the server build; no gameplay or storage caller is connected.
- **Defaults:** progression OFF; legacy storage remains unchanged; all individual feature flags and public release disabled; pilot allowlist empty. Live gates require MySQL authority, the expected schema readiness, supported campaign, valid configuration, and pilot/public eligibility. Contracts, expeditions, and quartermaster also require their content readiness.
- **Checks:** full CMake server build passed. Production-module tests passed with strict compiler warnings, AddressSanitizer, and UndefinedBehaviorSanitizer for Dragonlance, default campaign, and Forgotten Realms. Invalid configuration, overflow, stable-ID lookup, identities, and release gates covered. Existing clan login notification regression passed.
- **Next entry point:** Phase 4 versioned MySQL schema migrations, tested in a disposable database. No schema was installed, legacy data imported, running server restarted, or production feature enabled in Phase 3.


### Phase 4

- **Status:** complete.
- **Files changed:** `sql/20261110_clan_progression_v001.sql`, `tests/run_clan_progression_schema.py`, [Phase 4 handoff](clan_progression_phase_4_handoff.md), design status, this plan.
- **Behavior delivered:** explicit version-1 InnoDB schema for core clan identity/header/legacy treasury and independent XP/Influence state. Immutable nonzero lowercase hexadecimal incarnation IDs; preserved numeric vnums; unique active-vnum reservation; retained immutable tombstones; signed-64-bit balances, revisions and Unix timestamps; enforced checks, foreign key and history-protection triggers. Schema receipt is written last; populated reruns preserve existing data and different-version reruns fail closed.
- **Checks:** actual migration passed against a private socket-only MariaDB 11.8.6 instance, including repeat/interrupted installs, duplicate/invalid identities, negative/overflow values, 64-bit boundaries, transactional rollback, tombstone/vnum reuse without inherited balances, immutable/history guards, and future-version refusal. Phase 3 configuration/campaign/sanitizer tests and existing clan login notification regression passed. Documentation links/whitespace checked. No compiled target changed, so no server rebuild was required.
- **Assumptions:** schema version 1 matches the Phase 3 contract; gameplay caps remain checked service policy rather than SQL balance caps. Only one active incarnation may hold a vnum. Names/abbreviations retain existing limits. Repository readiness must verify actual structure, not just a receipt. MySQL 8.0.16+ is targeted but not independently exercised; MariaDB is behaviorally verified.
- **Scope boundary:** no runtime repository, full legacy schema, import, game-database migration, server restart, feature enablement, or production change. The disposable server/data were removed after the tests.
- **Next entry point:** Phase 4a checked repository reads/writes, creation transactions, strict 64-bit conversion and revision conflicts; see the handoff. Phase 4b adds remaining legacy fields and child schemas; Phase 4c adapts callers behind the migration gate.


### Phase 4a

- **Status:** complete.
- **Files changed:** `src/clan_repository.{c,h}`, `src/clan_repository_schema.inc`, `scripts/generate_clan_schema_contract.py`, repository tests and build manifests.
- **Behavior delivered:** checked core reads/writes, identity creation, transaction boundaries, signed-64-bit conversions, actual-schema validation, revision conflicts and retained tombstones.
- **Checks:** production C repository strict/sanitizer tests in disposable MariaDB; full server builds.

### Phase 4b

- **Status:** complete; split into 4b-1 schema/read model and 4b-2 atomic writes/validation at working boundaries.
- **Files changed:** version-2 migration, `src/clan_legacy_fields.h`, repository snapshot APIs, snapshot fixtures/tests.
- **Behavior delivered:** full legacy settings/statistics/ranks/privileges, stable membership mirrors, incarnation relationships and zone claims; atomic snapshot replacement with rollback and global/core revision protection.
- **Checks:** complete round trips, invalid payloads, injected failures, schema drift and tombstone protection in disposable storage. See [4a–4c handoff](clan_progression_phase_4abc_handoff.md).

### Phase 4c

- **Status:** complete; split into 4c-1 adapter and 4c-2 caller audit.
- **Files changed:** `src/clan_persistence.{c,h}`, clan/edit/economy/transaction callers, player save/load hooks, mob commands, integration regression tests, build manifests.
- **Behavior delivered:** explicit authority attachment, checked writers, membership mirrors, readiness/access guards and no stale-file fallback. Legacy remains default. Ordinary unrelated character saves remain possible during clan outages.
- **Scope:** boot hydration remains 5a; SQL economic mutations remain gated until Phase 8 and creation/destruction until Phase 11. No live authority switch.
- **Checks:** real repository/adapter tests, existing clan login regression, CMake and Automake builds.

### Phase 5

- **Status:** complete; split into 5-1 parser/dry-run and 5-2 backup/import receipts.
- **Files changed:** `scripts/clan_legacy_import.py`, native bridge/build helper, importer and native-description tests, `src/db.c` reader buffer bound, [Phase 5 handoff](clan_progression_phase_5_handoff.md), plan/design status.
- **Behavior delivered:** strict full legacy import, native description decoding, dry-run comparisons, private durable raw backups, source rechecks, immutable transactional receipts and unchanged reruns. Newer SQL state/tombstones/cutover cannot be overwritten. Ambiguous investment ownership fails for review.
- **Checks:** ASan/UBSan native reader/repository/adapter/bridge checks; disposable MariaDB import, rerun and protection scenarios; configuration/schema and existing clan regressions; both full server builds. Fixed a one-byte native long-line reader overrun discovered by these checks.
- **Next entry point:** Phase 5a SQL boot loading/configuration and mirror reconciliation, then 5b operational cutover/rollback rehearsal. No live database changes, restart or feature enablement.


### Phase 5a

- **Status:** complete; split into 5a-1 schema/retained timers and 5a-2 boot hydration/authority attachment.
- **Files changed:** `src/clan_boot.c`, persistence/repository APIs, `src/db.c`, `src/clan_progression.h`, version-3 migration and generated contract, environment example, build manifests and boot regression tests.
- **Behavior delivered:** explicit legacy/shadow/MySQL boot selection; complete SQL metadata/claim hydration, hash rebuild, stable-player validation and membership mirror reconciliation. Empty stores load safely. Unsupported/malformed records and SQL failures stop requested SQL boot without stale-file fallback. Expiry/cancellation preserves saved family cooldowns, without enabling effects.
- **Checks:** strict/sanitizer production-module tests in disposable MariaDB, including already spawned NPCs, fresh/imported state, missing/deleted player IDs, outages and disabled defaults; both server builds.
- **Scope:** no automatic schema migration/import/cutover, no gameplay awards/effects. Economic/lifecycle gates remain. See [handoff](clan_progression_phase_5a_6_handoff.md).

### Phase 5b

- **Status:** complete as required prerequisite for requested Phase 6; split into 5b-1 atomic transition/retry and 5b-2 post-cutover SQL recovery rehearsal.
- **Files changed:** checked repository/native bridge cutover operation, `tests/run_clan_boot_favor.py`, recovery policy in the handoff.
- **Behavior delivered:** revision-checked explicit fresh/imported offline authority selection; receipt/control update in one transaction. Repeated/stale transitions fail; uncertain acknowledgements reconcile through reads. No reverse-cutover API or reopening stale files.
- **Checks:** injected transition failure rolls back both receipt and control; successful transition preserves treasury/membership, repeated boot preserves timers, stale import cannot overwrite a post-cutover write. Complete transactional SQL dump restored to a second disposable database compares equal, including that write. Existing tombstone/incarnation regressions pass.
- **Rollback policy:** preserve SQL authority after cutover and recover the complete latest store; binary downgrade/reverse export requires a separate reviewed conversion accounting for post-cutover writes. No operational cutover occurred.

### Phase 6

- **Status:** complete; split into 6-1 retained Favor schema and 6-2 checked reads/writes and lifecycle regression.
- **Files changed:** version-3 `clan_personal_favor` table/triggers, repository Favor APIs, `tests/clan_favor.c`, combined boot/recovery runner, design/plan/handoff documentation.
- **Behavior delivered:** cumulative eighth-point Favor keyed by stable character and clan incarnation, optimistic revisions, monotonic balances/timestamps, retained former-clan/tombstone records and no inherited standing on vnum reuse. No debit/purchase or officer-award endpoint.
- **Checks:** independent standings, leave/reconnect persistence, missing rows, conflicts, backward time, int64 limits, no decreases/deletion, tombstone read-only behavior, new-incarnation isolation and migration reruns; existing persistence/import/configuration/clan login regressions and both builds.
- **Next entry point:** Phase 7 account-level contribution windows, then Phase 8 durable receipts. Favor awards and eligibility remain gated pending their later services. No live database or feature enablement changes.


### Phase 7

- **Status:** complete; split into 7-1 deterministic UTC periods and 7-2 retained account-window schema/repository validation.
- **Files changed:** `src/clan_progression.{c,h}`, `src/clan_repository.{c,h}`, version-4 migration and generated schema contract/generator, current-schema regression fixtures, `tests/clan_account_windows.c`, `tests/run_clan_account_windows.py`, environment example, build distribution list and [Phase 7 handoff](clan_progression_phase_7_handoff.md).
- **Behavior delivered:** account-global weekly eighth-point base-XP/Favor totals and six explicit source counters; retained account/week keys, optimistic revisions, monotonic usage and history guards. Reconnect/alt/clan changes cannot reset a budget. Pure UTC daily/Monday-weekly calculations use occurrence time, exact half-open boundaries and checked int64 arithmetic. Delayed operations retain their original week.
- **Checks:** strict C99/ASan/UBSan tests against private socket-only MariaDB; independent UTC datetime oracle across >1,000 timestamps and three process timezones, exact resets/leap/DST/post-2038/int64 boundaries; independent connections/shared account state, conflicts, injected rollback, retained weeks, corrupt data, migration reruns and schema drift. Existing repository/snapshot/persistence/import/boot/Favor/configuration/clan-login regressions and both full builds pass.
- **Scope:** no live writes, account gameplay hooks, reward/cap calculation, daily prototype event tracking or feature enablement. Future callers must use actual account_data.id. All previous economic/lifecycle gates remain.
- **Next entry point:** Phase 8 durable receipts and a composable atomic SQL transaction for account counters, clan/personal balances and event identity. Do not connect the standalone window writer to rewards as a separate commit.


### Phase 8

- **Status:** complete; split into 8a durable acceptance/unique identity, 8b atomic counter/balance composition and 8c process-crash/replay/concurrency verification.
- **Files changed:** `src/clan_rewards.h`, checked repository/internal account/Favor stages, version-5 migration/generated contract, current-schema fixtures, `tests/clan_rewards.c`, `tests/run_clan_rewards.py`, environment/distribution notes and [Phase 8 handoff](clan_progression_phase_8_handoff.md).
- **Behavior delivered:** bounded SQL pending events (100/character, 10,000/server), stable event/source/recipient deduplication, immutable requested facts and terminal receipt proof; one transaction for account usage/source counters, clan XP/Influence, personal Favor, clan weekly budgets and exact award decision. Replays return the original receipt without policy reruns or new awards. Delayed recovery retains the occurrence week; explicit cancellation preserves claim history.
- **Recovery boundary:** SQL-only progression awards need no player-file changes. Required escrow/checkpoint/outbox/acknowledgement protocol for future gold/inventory actions is documented against the audited estate/legendary/durable-file patterns. Separate SQL connections are not assumed atomic. Existing economic/file gates remain until each concrete adapter is implemented and fault-tested.
- **Checks:** strict C99/ASan/UBSan actual-module tests in disposable MariaDB; eight actual process interruption points, injected rollbacks, unknown acknowledgements, reset recovery, concurrent duplicate/shared-account claims, overflow, pending bounds, retained terminal history, populated migration reruns and schema drift. Prior repository/snapshot/persistence/import/boot/Favor/account/configuration/clan-login regressions and CMake/Automake builds pass.
- **Scope:** trusted server-only persistence coordinator; no reward/cap policy, gameplay hook, file delivery acknowledgement, migration or live enablement. Future callbacks are pure and must honor the accepted rules snapshot. Runtime progression caches must reconcile committed SQL before service integration.
- **Next entry point:** Phase 9 XP/level/Influence policy through the checked coordinator, then Phase 10 personal Favor/account/clan budgets; all default feature gates remain OFF.


### Phase 9

- **Status:** complete; split into 9a pure levels/clan budget policy, 9b shared receipt composition and 9c runtime cache reconciliation.
- **Files changed:** `src/clan_awards.{c,h}`, build manifests, persistence adapter, policy/receipt/cache tests and [Phase 9 handoff](clan_progression_phase_9_handoff.md).
- **Behavior delivered:** derived levels 1–20; multi-level rewards; lifetime XP cap; clan weekly full/quarter XP bands; independent weekly/stored Influence capacity; eighth-point arithmetic and retained truncation flags. Gated shared awards commit through existing atomic receipts; personal sources fail closed pending Phase 10. Progression-only cache refresh and automatic pre-save reconciliation preserve committed awards without merging unrelated SQL metadata.
- **Checks:** strict C99/ASan/UBSan production policy tests; disposable SQL transaction/crash/replay/concurrency and cache/save tests; configuration/campaign gates; CMake and Automake builds.
- **Scope:** no gameplay hook, personal reward service, source eligibility/contract quota adapter, migration, live award, restart or feature enablement. Version-1 policy and occurrence-week receipts remain immutable.
- **Next entry point:** Phase 10 account bands/source budgets, cumulative Favor awards/standing lookup and reward explanations, composing the clan projection inside the same receipt transaction.


### Phase 10

- **Status:** complete; split into 10a pure personal policy/standings/explanations and 10b checked receipt composition/recovery tests.
- **Files changed:** `src/clan_awards.{c,h}`, production policy and receipt test harnesses, `tests/run_clan_personal_awards.py`, design/plan status and [Phase 10 handoff](clan_progression_phase_10_handoff.md).
- **Behavior delivered:** exact 600-full/600-half account bands, account-global source claim limits, independent 600 weekly Favor cap, cumulative per-incarnation Favor standings, versioned cap explanations and atomic personal awards through existing receipts. Full/half/zero kill tuples require trusted source facts; other sources require exact configured tuples. Preview is advisory; committed replay never recalculates or duplicates rewards.
- **Checks:** strict C99/ASan/UBSan actual-module policy checks; disposable SQL alternate-character/clan limits, band examples, zero/capped receipts, stage rollback, lost acknowledgement, replay/reset recovery, concurrent last claim and overflow tests; prior receipt/crash/cache/gate regressions; CMake and Automake builds.
- **Scope:** no source gameplay hook, daily prototype tracker, manual inventory turn-in, lifecycle adapter, live migration/award or enablement. Future resource-consuming actions still require their recoverable file/SQL protocol and zero-payout rejection.
- **Next entry point:** Phase 11 joining/departure/expulsion/deletion and eligibility queries, preserving retained Favor and incarnation identity.


### Phase 11

- **Status:** complete; split into 11a membership/receipt eligibility, 11b retained-history disbanding and 11c atomic creation/runtime mirror integration.
- **Files changed:** clan repository/reward APIs, persistence adapter, clan creation/destruction paths, lifecycle and runtime/persistence tests, design/plan status and [Phase 11 handoff](clan_progression_phase_11_handoff.md).
- **Behavior delivered:** current accepted/applicant/former/deleted access keyed by immutable incarnation; checked personal event acceptance; current-membership validation for uncommitted expedition claims; retained occurrence-time ordinary rewards and committed replay. SQL creation commits fresh identity/leader together; disband commits tombstone/membership/relation/claim changes before compacting runtime arrays and mirrors. Favor, cooldowns, account limits, receipts and inventory remain isolated/retained across departure and vnum reuse. Fixed cache ordering after membership replacement.
- **Checks:** strict C99/ASan/UBSan actual SQL lifecycle and rollback/history/reuse tests; real runtime creation/removal and mirror/index/relationship compaction; prior receipt, personal award, boot/Favor, campaign and clan-login regressions; CMake/Automake builds.
- **Scope:** no live migration, authority switch, restart, gameplay reward hook or progression enablement. Release gates stay OFF. Source occurrence/account/expiry checks and later buff/stat invalidation remain their respective integration phases. SQL uncertainty fails closed and requires reconciliation, without stale-file fallback.
- **Next entry point:** Phase 12 text readouts and restricted inspection tools using checked status, Favor, contribution windows and explanations; no officer mint endpoint.
