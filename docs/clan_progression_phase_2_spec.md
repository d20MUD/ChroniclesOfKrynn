# Phase 2 — Initial Balance and Configuration Specification

Status: complete as an implementation specification, version 1. These are the initial staging/pilot defaults, not claims about measured live balance. No mechanics, SQL schemas, data imports, or live settings are implemented by this document.

References: [design](clan_progression_design.md), [implementation plan](clan_progression_implementation_plan.md), [Phase 1 audit](clan_progression_phase_1_audit.md).

This document resolves the provisional values needed for the first implementation slice. If it differs from an illustrative value in the concept document, use this specification for implementation. Later balance changes must be versioned and reflected in tests and player help.

## 1. Initial Scope and Release Controls

The first slice has five buff families: adventuring XP, spell preparation/recovery, out-of-combat PSP regeneration, maximum HP, and crafting checks. It includes normal combat contribution, supply orders, deliveries, gathering contracts, bounties, two expedition patterns, Favor standings, and a small quartermaster catalog.

Movement, lore, fear-only buffs, automatic buff renewal, conventional quest rewards, territory-required progression, and personal service privileges are excluded from this slice. Offense, general saves, healing received, gathering yield, and elaborate expedition stages remain subsequent additions.

| Proposed control | Default | Meaning |
| --- | --- | --- |
| `clan_store_mode` | `legacy` | Explicit `legacy` / `mysql_shadow` / `mysql` storage authority; only `mysql` may run live progression. |
| `clan_progression_mode` | `off` | `off` / `shadow` / `live`; shadow records bounded diagnostic observations, with no awards, consumed claims, costs, or effects. |
| `clan_buffs_enabled` | false | Gates activations and effects; requires live progression and a ready MySQL schema. |
| `clan_contracts_enabled` | false | Gates personal contract acceptance/claims. |
| `clan_expeditions_enabled` | false | Gates expedition enrollment/claims. |
| `clan_quartermaster_enabled` | false | Gates new catalog purchases. |
| `clan_web_enabled` | false | Gates the new report/action interface for the Krynn preset. |
| `clan_pilot_allowlist` | empty | Immutable clan identities allowed during a controlled pilot; an empty pilot allowlist enables no clans. |
| `clan_public_release` | false | When explicitly enabled, replaces the pilot allowlist restriction. |
| `clan_rules_version` | 1 | Identifies the rules used for new events and activations. |

Use the current campaign configuration to restrict the initial feature to Krynn/Dragonlance. Keep other campaigns and web presets unaffected. Imported data alone must never enable rewards or effects.

`mysql_shadow` compares against legacy authority; it does not perform concurrent authoritative writes. General `shadow` progression does not advance counters that would reduce later real rewards. Put an upper bound on diagnostic retention and avoid per-pulse SQL queries.

## 2. Identity, MySQL Authority, and Migration

### Stable identities

- **Clan:** persist a server-generated random 32-character hexadecimal incarnation ID with a unique database constraint. Preserve the existing numeric vnum for commands, world references, and migration, but never use it alone for Favor or cooldown history.
- **Character:** use saved `GET_IDNUM`, not the character name or runtime DG ID.
- **Account:** use numeric MySQL `account_data.id`; missing or invalid account identity makes new earning/claims unavailable, with a clear explanation. Do not substitute a character ID.
- **Encounter:** use one stable UUID per actual spawned encounter/death. Reuse the existing encounter-ID pattern where appropriate, without inheriting legendary level-30 or boss-only gates. Copies/new spawns receive new IDs.
- **Contract and outing:** use stable definition IDs plus generated accepted-contract/outing IDs. Store the rules version and exact reward/requirements at acceptance.
- **Claim:** include source identity, beneficiary, clan incarnation, and reset period as appropriate. An uncertain commit retries the same claim, never a newly generated one.

After deletion, retain a clan tombstone and historical Favor/receipt references. Reusing a numeric vnum creates a new incarnation with no inherited progression or cooldowns. Historical Favor remains inspectable but cannot be redeemed against the new clan.

### Storage ownership

MySQL transactional tables are authoritative for migrated clan core records, ranks, privileges, membership, treasury, progression, active buffs/cooldowns, Favor, account windows, contracts/outings, catalog entitlements, and operation receipts. Reuse the existing database connection and query/recovery conventions.

New progression quantities use checked signed-64-bit-compatible arithmetic and transactional integer storage. Do not change the units or meaning of the legacy clan bank or personal clan points. Do not derive new balances from those existing currencies.

Compatibility player-file clan fields may be reconciled mirrors after membership cutover. They are not permission or membership authority at that point. Existing unrelated player/inventory/queue saves can remain in their current subsystem; a transaction spanning them and MySQL needs durable checkpoint/receipt recovery.

### Import and cutover

1. Back up the clan file and relevant membership/index data; record an import manifest and source digest.
2. Dry-run import and reconcile every clan field and membership reference. Include bank, ranks, privileges, halls, relationships, statistics, applicants, and offline characters.
3. Assign each imported clan one incarnation ID and preserve that mapping on importer retries.
4. Import existing state with zero new Clan XP, Influence, and Favor. All clans start at level 1; no inferred historical awards or retroactive cooldowns. Existing clan points and bank balances remain unchanged.
5. Reconcile disagreements between membership sources before cutover. Do not silently choose whichever source grants greater access.
6. Quiesce clan mutations for the final import comparison/cutover; switch authority explicitly to MySQL. Further normal saves must not overwrite MySQL from the legacy file.
7. Rerunning import must not overwrite newer balances, revive deleted clans, duplicate relationships, or reset timestamps. An already cut-over manifest is not a fresh import request.
8. Rollback requires reconciliation/export of post-cutover writes or recovery from the committed ledger. Reopening the old clan file alone is forbidden.

Database outages reject new mutations before consuming resources. While its feature gates remain enabled, a validated, committed in-memory buff snapshot can continue until its saved expiration; missing/corrupt state gives no benefit. No query may extend an expired effect because storage is unavailable. Do not switch to stale legacy data or disconnect ordinary players merely because the new progression subsystem is unavailable.

## 3. Clocks, Resets, Rounding, and Limits

- Store expiration, availability, occurrence, and reset boundaries as signed-64-bit-compatible UTC epoch seconds. Use an injectable server clock in tests.
- Weekly windows reset Monday at 00:00 UTC; daily farming windows reset at 00:00 UTC. The web/text interface should show the player's local equivalent and time remaining.
- Windows are half-open: `[start, end)`. An event at the exact boundary belongs to the new period.
- Events use server occurrence time and membership at occurrence, not retry time. A pending operation retains its original window, clan identity, reward snapshot, and cap decision on recovery.
- Account contribution counters are global across that account's characters and clans. Clan aggregate counters belong to the clan incarnation. Switching characters or clans does not open a fresh account budget.
- Clan XP, Influence, and Favor awards can have fractions after diminishing returns. Represent new reward quantities internally in eighths of a point, with checked integer arithmetic; display whole earned points with the fractional progress retained. Thresholds and prices below are whole points, scaled by eight internally.
- No floating-point currency math and no per-event rounding that discards half-point kill rewards. Prices must be whole points; a displayed Influence balance is the floor of the actual spendable balance.
- Restrict stored/internal values to `0..INT64_MAX`. Reject configuration or additions whose scaling/multiplication would overflow. Lifetime Clan XP is capped at the level-20 threshold; Influence/Favor may continue to accrue under their own rules afterward.
- Definition changes apply to newly accepted contracts and activations. They must not retroactively extend old buffs, shorten saved cooldowns, or change the terms of already committed rewards.

## 4. Clan XP and Levels

Clan level is derived from cumulative Clan XP, never a separately spendable balance:

`XP required for level L = 100 × L × (L − 1)`, for levels 1–20.

| Level | Cumulative Clan XP | Launch milestone |
| --- | ---: | --- |
| 1 | 0 | Two buff slots; Tier I XP, preparation, PSP, and maximum-HP families; initial contract choices. |
| 2 | 200 | Recognized quartermaster collection eligibility. |
| 3 | 600 | Tier I crafting buff. |
| 4 | 1,200 | Additional bounty/region choices, without increasing claim limits. |
| 5 | 2,000 | Tier II versions of unlocked initial buff families. |
| 6 | 3,000 | Additional delivery/gathering choices, without increasing claim limits. |
| 7 | 4,200 | Trusted utility collection eligibility. |
| 8 | 5,600 | Proven equipment commissions eligibility. |
| 9 | 7,200 | Additional expedition target choices, without increasing shared reward slots. |
| 10 | 9,000 | Third active buff slot. |
| 11 | 11,000 | Further proficiency/armor variants in the Proven collection. |
| 12 | 13,200 | Distinguished commission collection eligibility. |
| 13 | 15,600 | Additional contract choices; same reward budget. |
| 14 | 18,200 | Additional expedition alternatives; same reward budget. |
| 15 | 21,000 | Advanced quartermaster collection eligibility. |
| 16 | 24,000 | Additional advanced equipment variants. |
| 17 | 27,200 | Additional cosmetic/insignia variants. |
| 18 | 30,600 | Exalted commission collection eligibility. |
| 19 | 34,200 | Additional prestige appearances. |
| 20 | 38,000 | Tier III initial buffs and final prestige collection eligibility. |

Collection and alternate-contract milestones are content gates, not free stat bonuses. A milestone is only advertised as stocked once its validated content exists; the initial small catalog need not populate every later cosmetic variant. Source references and actual item templates are selected in phases 38–39.

All clans can use the two initial expedition patterns at level 1 when expeditions are enabled. Higher levels offer choices, not an obligation to unlock the only viable earning activity.

No level loss, XP decay, officer XP grants, bank-funded XP purchases, or rank-based XP multiplier.

### Clan-wide weekly budgets

| Quantity | Weekly policy |
| --- | --- |
| Account-adjusted incoming Clan XP | First 6,000 points at 100%; next 6,000 at 25%; further incoming XP gives no Clan XP that week. Maximum realized Clan XP is 7,500/week. |
| Influence credited to a clan | Maximum 5,000/week; applies to all contribution sources combined. |
| Stored Influence | Maximum 20,000. Excess credit is reported/discarded rather than held as an invisible backlog. |

Shared contract rewards count toward these clan-wide budgets. Clan XP diminishing returns do not reduce personal Favor. Reaching level 20 or a clan XP budget does not by itself stop allowed Influence or Favor earnings. Every truncated reward must be visible in the receipt/readout.

## 5. Personal Contribution Rewards and Account Budgets

### Reward table

These are base rewards before account/clan budget reductions. Each valid character award credits its chosen clan incarnation and that character's Favor with the same clan.

| Source | Clan XP | Influence | Personal Favor | Per-account weekly source limit |
| --- | ---: | ---: | ---: | --- |
| Qualifying ordinary kill | 2 | 1 | 1 | First 200 eligible encounters; also subject to prototype farming limits below. |
| Successful supply-order turn-in | 100 | 50 | 50 | Three completed orders. Existing normal order cooldowns/rewards remain unchanged. |
| Quartermaster delivery contract | 100 | 50 | 50 | Two different accepted delivery definitions. |
| Fresh gathering contract | 100 | 50 | 50 | Two different accepted gathering definitions. |
| Personal bounty completion | 100 | 50 | 50 | Two different accepted bounty definitions. |
| Expedition personal completion | 0 | 0 | 100 | Two qualifying outings, with at most one per expedition family. |

A single encounter can legitimately advance an accepted bounty and expedition while also awarding its ordinary kill contribution. These are distinct, capped achievements with separate receipts, not duplicate awards of the kill itself.

### Account diminishing returns

For the five sources with a positive base Clan XP reward:

- First **600 base Clan XP** per account/week: 100% of their XP, Influence, and Favor.
- Next **600 base Clan XP**: 50% of those three rewards.
- Further base contribution: no personal-source clan XP, Influence, or Favor that week.
- Maximum account-adjusted Clan XP from those sources: **900/week**.
- Maximum realized Favor from all sources combined: **600/account/week**, across all characters/clans. Expedition Favor is independent of the base-XP bands but still uses this Favor cap and its own two-claim limit.

Split an event exactly at a band boundary, prorating its tuple by base Clan XP. Example: with 550 base XP used, a 100/50/50 supply order gives `50 + 25 = 75` Clan XP and `25 + 12.5 = 37.5` Influence/Favor. Record 650 base XP used. Keep the fractional credit in eighths.

Apply processing in this order: source eligibility/limits → account bands → clan XP bands and Influence capacity → Favor cap → single committed receipt/counters/balances. Clan reductions do not consume extra account base budget beyond the actual qualifying source award. Personal source limits still apply even when another cap truncates its payout.

For manual turn-ins, show the exact reduced payout before confirmation/commit. If all requested awards would be zero, reject without consuming goods. Automatic combat contributions that have exhausted a limit grant zero and remain deduplicated; they do not affect normal character XP or loot.

### Ordinary-kill farming limits

Per account, enemy prototype, and UTC day, across all clans:

- First 20 qualifying deaths of that prototype: normal source credit.
- Next 20: 50% source credit, before account bands.
- Further deaths: no ordinary-kill contribution that day.

The 200-encounter weekly source limit counts actual credited-eligible encounters, not scaled reward points. Daily prototype reductions scale the base tuple and the base XP charged to the account bands equally. Contract objectives have their own quotas and cannot turn a capped ordinary kill into a second ordinary reward.

## 6. Encounter and Participation Eligibility

Use these launch rules independently of the character XP actually received:

- A legitimate world NPC death must pass normal kill/death checks. No player kills, owned/charmed/summoned creatures, player pets/followers, test encounters, or staff-created test-credit targets.
- A character must be alive, actively playing, an accepted member, and have an authoritative account ID and clan incarnation at occurrence.
- Resolve controlled creatures to their player owner for credit; multiple controlled creatures never create extra account rewards.
- Let `H` be the highest level among player participants in the encounter, including eligible grouped outsiders and pet owners. The victim must be at least `max(1, H − 3)`.
- A recipient must be within five character levels of `H`. Lower-level members can progress through appropriate separate outings and crafting, rather than tagging a much higher-level carry group.
- Recent meaningful participation must occur in the encounter room within the preceding 120 seconds. Being grouped, entering the room, overhealing, or applying an already-present non-improving effect is insufficient.
- A recipient qualifies through at least one of: effective damage totaling 5% of the victim's encounter-start maximum HP; effective healing totaling 10% of an actually engaged ally's non-clan maximum HP; effective interception/mitigation totaling 5% of such an ally's maximum; or two distinct useful support/defense actions. Successfully receiving two hostile damaging attacks while engaged counts as defensive participation.
- Damage and healing totals exclude overkill/overheal. A useful control/debuff action must actually change the live enemy's state under normal combat rules. Two control/defense actions must occur in distinct combat rounds; repeated packets from the same application are one action.
- Support requires an ally actually engaged in this encounter. The healer need not land the killing blow or have been the enemy's target.
- The same participation result feeds ordinary contribution, bounties, and expeditions. Do not require the existing legendary level-30/boss eligibility checks.

These are configurable pilot thresholds. Phase 25 must check them against representative short fights, longer fights, damage dealers, healers, defenders, and mixed-level parties before enabling awards.

## 7. Contracts, Shared Rewards, and Expeditions

### Personal contracts

- Maximum three accepted personal contracts at once, at most one per family: delivery, gathering, bounty. Supply orders remain in their existing system.
- Weekly rotation supplies at least two alternatives in each family. Choices vary, but the reward tuple and account source limits do not grow with clan level.
- Definitions, rewards, required grades, counters, acceptance time, and source identities are server-owned. One character's accepted contract cannot be completed by another character's event history.
- Defaults: a bounty requires 20 qualifying designated kills; a gathering contract requires 20 base units from fresh, designated harvest events; a material delivery requires 50 units of specified material/grade; an ammunition delivery requires 100 remaining finite shots of the specified type/grade.
- Infinite/Returning ammunition cannot satisfy finite ammunition deliveries. Actual usable quantity, not the number of stack objects, is consumed.
- Delivery goods may be bought or transferred; the weekly quota makes this a bounded economic contribution. Fresh gathering contracts must originate from actual harvesting and never count purchased/transferred inventory or buff-added output.
- Support-crafting and combat assignments use suitable definitions for character/skill bands. Delivery grade and recipe requirements are explicit in the definition, never inferred from a colored short description.
- Expire personal contracts at the weekly boundary. No partial completion payout or automatic consumption. Abandonment returns no spent effort and does not reset source eligibility/claim receipts.
- Accepting/abandoning a contract grants no reward. Old completed turn-ins cannot be repeated after reacceptance.

### Shared clan rewards

| Shared source | Clan XP | Influence | Shared weekly limit |
| --- | ---: | ---: | --- |
| First completed qualifying personal bounty of a weekly bounty definition | 200 | 100 | Two reward slots total per clan/week; one claim per definition. |
| First qualifying Stronghold Assault completion | 600 | 300 | One shared claim per clan/week for this family. |
| First qualifying Regional Hunt completion | 600 | 300 | One shared claim per clan/week for this family. |

The maximum shared base reward is **1,600 Clan XP and 800 Influence per clan/week**, before clan-wide budgets. It does not multiply by party size. A later member's qualifying personal bounty or expedition completion can still grant their personal-source tuple subject to account limits; it cannot repeat the shared claim.

### Expedition defaults

| Setting | Default |
| --- | --- |
| Patterns | Stronghold Assault and Regional Hunt. |
| Required participants | Two different accounts with accepted members of the same clan, meaningfully participating; mixed-clan allies can help but do not satisfy this requirement. |
| Active outings | One outing per clan at a time; it can be repeated later for other members' eligible personal claims. |
| Intended play length | Approximately 30–60 minutes. |
| Hard expiry | Two real hours from start; no retroactive extension on disconnect/restart. |
| Roster size | Up to six distinct accounts from the same clan; ordinary group rules still apply. |
| Stronghold Assault | 20 qualifying designated defenders plus two named-target objectives in one existing area; each named objective has validated alternatives. |
| Regional Hunt | Three designated enemy categories across at least two areas, ten qualifying kills per category. |
| Personal participation | Credit on at least half the outing's counted combat objectives, rounded up, and participation in the final objective. Two same-clan accounts must independently satisfy this rule. |
| Claims | Objective completion locks an immutable eligible roster. Each eligible character may claim at the quartermaster until the end of the completion week's reset period. |

The final eligible roster must still be accepted members of the outing clan at completion. An uncommitted personal claim requires current membership in that same incarnation; leaving and rejoining does not extend its expiry. A committed pending delivery follows the original receipt even if membership changes afterward.

One qualifying kill advances at most one primary expedition objective; a named kill does not simultaneously satisfy a defender quota. Accept an outing only when all references/alternatives are valid and at least one alternative for each named objective exists in the selected world dataset; availability can change later and must be explained to players. Retrying an unavailable target never grants credit without a legitimate completion.

Any counted expedition objective requires the two eligible clan accounts to participate in that encounter. Roster changes are allowed before the first counted objective only. Thereafter a member who disconnects may reconnect to the same roster; replacing them does not preserve their personal claim. Abandon/restart grants no progress transfer.

If an outing spans a weekly boundary, abandon it at the boundary without reward; display the earlier expiry at acceptance. This avoids moving earlier work into a new shared reset. A completed outing's receipt retains the original period even if recovery occurs after reset; unclaimed expired personal entitlements are not revived by recovery. Already committed pending deliveries must still recover.

Actual area, enemy, material, and item references are deliberately deferred to the staged content audits in phases 38–39. Until definitions are validated, the affected feature stays disabled and does not advertise fictitious targets.

## 8. Favor Standings and Quartermaster Gates

| Standing | Cumulative Favor | Minimum clan level for its initial stock | Initial purpose |
| --- | ---: | ---: | --- |
| Unproven | 0 | 1 | Contribute and receive ordinary active clan buffs. |
| Recognized | 100 | 2 | Insignia and basic quartermaster supplies. |
| Trusted | 500 | 7 | Utility equipment and tools. |
| Proven | 1,500 | 8 | Normal clan weapon/armor commissions. |
| Distinguished | 3,500 | 12 | Advanced commissions; advanced variants at clan level 15. |
| Exalted | 6,000 | 18 | Prestige commissions and appearances; final variants at level 20. |

Favor never decays or decreases on purchases. No direct Favor awards from an ordinary officer command or from legacy clan points. All accepted members receive eligible clan-wide active buffs regardless of Favor standing; the system does not withhold basic clan benefits from a new recruit.

For launch, gear uses gold rather than a new personal currency. Initial pricing policy for phase 39: supplies at their normal shop-equivalent price, utility equipment at twice its approved normal value, equipment commissions at three times their approved normal value, prestige appearances at five times their approved normal value; minimum one gold. Store an explicit validated catalog price, not a fluctuating player-supplied value. Do not stack clan discounts onto these prices. Exact item templates, approved values, and character minimum levels must be specified per catalog row before purchases are enabled.

No repeat-purchase discounts and no resale/profit loop: resale must not exceed the actual gold paid. Purchased gear is bound to its acquiring character under the existing binding system, keeps normal class/proficiency/level rules, and remains usable after leaving. Favor and clan-level gates control acquisition, not an extra stat budget. No required PvE-best-in-slot upgrade or free extra activated-spell power.

Favor records survive departure for the same clan incarnation; rejoining can use that standing. New purchases, commissions, and buffs require current accepted membership. A transaction committed before departure delivers to the original recipient, not the clan or replacement member.

## 9. Buff Catalog, Prices, and Cooldowns

Family IDs below are stable and never reused after retirement. Family zero is invalid. Tier is a separate value, not part of family/cooldown identity.

| ID | Family | Category | First unlock | Tier I / II / III |
| --- | --- | --- | ---: | --- |
| 1 | Scholar's Encouragement | Growth | Level 1 | +5% / +8% / +10% adventuring XP. |
| 2 | Arcane Readiness | Recovery | Level 1 | 10% / 15% / 20% shorter preparation/recovery. |
| 3 | Psionic Focus | Recovery | Level 1 | +10% / +20% / +30% out-of-combat PSP recovery. |
| 4 | Company Vitality | Combat | Level 1 | +5% / +10% / +15% maximum HP. |
| 5 | Artisan's Guidance | Craft | Level 3 | +2 / +3 / +4 to relevant crafting checks. |

For these families, Tier II unlocks at level 5 and Tier III at level 20. Initial slots: two through level 9, three from level 10. At most one combat-category family may be active.

| Tier | Influence cost per activation | Duration | Additional downtime |
| --- | ---: | --- | --- |
| I | 100 | 24 hours | 12 hours |
| II | 150 | 24 hours | 12 hours |
| III | 225 | 24 hours | 12 hours |

No extra gold or material activation costs in the initial slice. Influence cannot be bought using clan bank funds. A clan starting with zero Influence earns its first activation; no historical or starter grant is inferred during migration.

At successful activation time `T`, save `expires_at = T + 86,400` and `next_available_at = T + 129,600`, together with family, tier, price, rules version, and operation receipt. A buff is active exactly while `T <= now < expires_at`; it is available again when `now >= next_available_at`, subject to other checks.

All tiers share the same clan/family cooldown. No refresh, extension, automatic renewal, cooldown refund, or tier-change bypass. Canceling/replacing a buff keeps the original saved `next_available_at`. Different families can fill freed slots if their own rules allow. Level-up, officer changes, leaving/rejoining, server restart, and copyover cannot reset a timestamp.

Activation and cancellation require a new appended buff-management privilege, defaulting to leader-only until explicitly delegated. Preserve existing privilege numeric positions. Regular members can inspect status; staff can inspect and use isolated test fixtures but normal operator tools must not reset live economic receipts/cooldowns casually.

Replacement validates the new family, costs, slot/category result, permissions, and current state before changing anything. Lock/revalidate and commit atomically; no double click can charge twice or replace a buff after a stale eligibility check. Failure changes neither resources nor the previous active effect.

## 10. Exact Buff Semantics and Stacking

### Company Vitality

Define `B` as the character's effective maximum HP after ordinary ability, class, gear, spell, perk, configuration, and legendary flat-HP contributions, excluding all clan/territory maximum-HP percentage modifiers. The new maximum is `B + floor(B × P / 100)`.

Calculate from the non-clan base on every authoritative stat rebuild. Do not add the percentage to an already boosted maximum. Preserve the existing ordinary versus effective/legendary separation and avoid serializing this temporary bonus into ordinary maximum HP.

Activation never increases current HP. When the new maximum falls, set current HP to `min(current_HP, new_maximum)`, without dealing damage or changing a negative current-HP value to a positive one. Apply the same rule to expiration, cancellation, departure, feature disablement, and base-stat changes. No bonus above the normal clan-adjusted maximum and no fresh healing from repeated recalculation.

### Arcane Readiness

Let `N` be the normal preparation/slot-recovery time after existing feats, room bonuses, perks, and server configuration. The buffed time is `max(circle, ceil(N × (100 − P) / 100))`. Preserve any stronger existing minimum requirement. Do not grant new spells, slots, circles, or permission to recover in an invalid state.

Apply to the actual cycle starting at the head of prepared and spontaneous queues, including domain handling; do not grant the same reduction once on enqueue and again when it becomes the head. Preserve existing Arcane Recovery and other non-clan reductions without repeatedly reapplying the clan multiplier.

**Current-cycle rule:** snapshot eligibility/tier when that actual preparation/recovery cycle starts. A running cycle retains its fixed duration through later buff activation, expiry, or membership changes. Future cycles use current eligibility. Merely enqueueing many spells while a buff is active does not reserve reduced times for the whole queue. Persist enough current-cycle state through the existing queue/checkpoint mechanism to prevent reduction being applied again on login/copyover. A genuinely restarted cycle computes from its normal base; it does not multiply the old remaining time.

This deliberately permits one already-started cycle to finish under its original terms. It gives no further cycles access to an expired or former-clan buff.

### Psionic Focus

Use the positive actual out-of-combat PSP recovery the current `regen_psp()` tick would provide without this clan buff, including its normal base point, helper contribution, applicable recovery feat, and resting-position recovery. Exclude over-max drain and passive gains outside this defined PSP recovery path.

Add `floor((R × P + remainder) / 100)` PSP and retain the modulo-100 remainder. `R` is the ordinary tick's positive recovery before the final PSP cap; do not count the clan bonus in `R`. At or above maximum PSP, including after final clamping of a tick that reaches maximum, clear the fractional remainder and do not bank unused recovery. Never exceed maximum PSP.

Keep the remainder by character and buff activation ID. Clear it on combat entry, buff activation change/expiration, membership departure, or disabling effects; do not transfer it between characters/clans. Losing a sub-point remainder on logout is acceptable; logout must not reset a counter in a way that grants an extra point. Do not put the multiplier in the shared helper that also feeds movement recovery.

### Scholar's Encouragement

Apply once to positive character adventuring XP from actual eligible NPC damage/kill reward sources, preserving their ordinary caps and existing modifiers. Exclude crafting XP, administrative grants, losses/death penalties, clan progression, and conventional quest rewards in the first slice.

Use a percentage of the otherwise calculated positive character award before its final ordinary cap. Existing account/happy-hour/newbie effects keep their documented order; the clan multiplier is applied once afterward. XP from a source distributed through multiple legitimate damage/kill awards receives one clan adjustment per actual award, not an additional duplicate aggregate award.

Clan eligibility and base rewards are calculated separately, before character XP modifications and cap rejection. A capped character earns qualifying clan contribution even when the character XP function returns zero.

### Artisan's Guidance

Add the selected flat value once to actual create, refine, ammunition, and supply-order production checks, across their appropriate professions. Do not add it to raw ranks, specialization storage, recipe access requirements, skill-up thresholds, crafting XP amounts, talents, tools, materials, or gathering checks.

A successful buff-assisted craft is legitimate work. It receives the same fixed, limited completion reward as an unbuffed successful contract, with no increased reward amount or claim allowance.

### Common stacking policy

A clan family never stacks with another tier/copy of itself. Different approved categories can coexist within slot limits. Initial clan buffs stack with existing non-clan effects in the calculation order above; if a future active territory/clan effect provides the same clan modifier, use the stronger clan value rather than summing. Do not activate currently dormant territory helpers as a side effect of this implementation.

No clan buff changes source reward tuples, contribution caps, contract quotas, Favor earnings, or shared clan rewards. A beneficial buff may help players complete content but does not directly multiply progression for the same completed work.

## 11. Worked Balance Examples

These examples assume eligibility, source quotas, suitable content availability, and even activity across reset periods. They are rate models, not guarantees or measured playtime.

### Three-account small clan

Per week, each of three different accounts contributes:

- 100 qualifying kills, spread across prototypes to avoid daily farming reductions: 200 base Clan XP.
- Two supply-order turn-ins: 200.
- One delivery, one gathering contract, and one bounty: 300.

Each account has 700 base XP: `600 + 100 × 0.5 = 650` realized Clan XP, `325` Influence, and `325` Favor. One qualifying expedition personal claim adds 100 Favor, for **425 Favor per account**.

The clan completes one expedition and one shared bounty definition:

- Clan XP: `3 × 650 + 600 + 200 = 2,750/week`.
- Influence: `3 × 325 + 300 + 100 = 1,375/week`.
- Level 5 (2,000 XP): during the first modeled week.
- Level 10 (9,000 XP): about **3.3 weeks**.
- Level 20 (38,000 XP): about **13.8 weeks**.
- Proven standing at 1,500 Favor: about **3.5 weeks** for each baseline contributor; Exalted at 6,000: about **14.1 weeks**.

At steady state, one family can be activated `168 / 36 ≈ 4.67` times/week. Two Tier I buffs cost about **933 Influence/week**, funded by this activity. Two Tier II buffs cost **1,400/week**, slightly above this baseline; the clan can earn a little more, choose occasional lower tiers, or use fewer activations. Third-slot access offers another choice, not a guarantee that minimal activity funds three top-tier buffs continuously.

### Ten-account highly active clan

Each account completes enough eligible personal work to use all 1,200 base-XP budget: `600 + 600 × 0.5 = 900` realized Clan XP and 450 Influence, plus up to the remaining allowed personal Favor. Complete both shared bounties and both expedition families.

- Incoming account-adjusted Clan XP: `10 × 900 + 400 + 1,200 = 10,600`.
- After clan bands: `6,000 + 4,600 × 0.25 = 7,150 Clan XP/week`.
- Incoming Influence: `10 × 450 + 200 + 600 = 5,300`; weekly cap credits **5,000**.
- Level 10: about **1.3 weeks**; level 20: about **5.3 weeks**.
- Personal Favor remains capped at **600 per account/week**, so Exalted requires at least **10 weeks**, irrespective of clan size.
- Three Tier III buffs cost about **3,150 Influence/week** at steady-state rotations, within that clan's earnings. Each individual family still has its own 12-hour downtime.

A larger clan progresses faster, but adding members does not multiply the shared rewards and the clan band prevents ten accounts from earning ten times the small clan's rate. These are pilot tuning points; reduce the clan XP band/threshold rate if measured large-clan progression is too fast.

## 12. Receipt, Concurrency, and Failure Requirements

- Use unique MySQL event/claim keys and transactions for authoritative balances, caps, receipt state, and SQL-owned entitlements. Lock affected account/clan rows in a documented consistent order.
- Persist occurrence-time identity, reward snapshot, exact cap decisions/fractions, and rules version. A retry applies the existing operation or reports it, never recalculates a more favorable window.
- For file-backed gold/material/inventory changes, use the audited durable checkpoint/delivery receipt patterns. Do not acknowledge delivery merely because SQL state committed.
- A failed/unknown commit enters recovery; it cannot be treated as both refunded and delivered. No duplicate consumption on replay.
- Manual actions with an unavailable database or invalid account/clan state fail before resource changes. Automatic earning can retain only bounded, durable pending events; an in-memory queue alone does not satisfy exactly-once recovery.
- Bound pending/outstanding operations at 100 per character and 10,000 per server. At capacity, reject new manual progression actions and stop accepting additional automatic progression events with rate-limited diagnostics; ordinary combat/loot continues. Do not later invent backlog rewards for events that were never durably accepted.
- Imports, deletion, and membership mutation must use the same identity/authority rules as progression operations. Do not delete a pending economic delivery solely because its clan later disbands.
- Read-only effects use validated cached committed data and live timestamp checks. Do not query MySQL for every attack, skill check, or regeneration helper call.

## 13. Configuration Validation and Player Feedback

Phase 3 should express these values as validated configuration/catalog data, not copy numbers into unrelated gameplay hooks.

Reject activation of the new subsystem if:

- Rules/schema versions or storage authority do not match.
- Family IDs are zero/duplicated/reused, tiers are missing, unlocks decrease incorrectly, or a slot count exceeds three.
- XP/Favor thresholds are not strictly increasing; level 1 is not zero; level cap differs from the threshold table.
- Prices/durations/downtime are invalid, timestamp arithmetic overflows, or cooldown does not include actual downtime.
- Percentages/check bonuses exceed their declared launch maxima (XP 10%, prep 20%, PSP 30%, HP 15%, crafting +4).
- Contribution ratios, caps, weekly windows, internal eighth-point scaling, or reward multiplications cannot be represented safely.
- Contracts lack validated quantities, grades, quota families, reward snapshots, or source identifiers.
- A configured live content feature references missing world/catalog definitions.

An unavailable contract/catalog does not disable basic clan inspection. The UI/text should distinguish not implemented/unstocked, locked by level/Favor, insufficient Influence, exhausted quota, active-family lock, cooldown, permission, storage unavailable, and stale/invalid request.

Always show actual reduced rewards, the next reset, active buff expiration, family next-available time, and why a claim/activation is blocked. No browser may supply authoritative rewards, eligibility, elapsed time, or completion.

## 14. Remaining Content Work and Phase 3 Handoff

The numeric economy, buff rules, clocks, identity direction, and release controls above are the staging defaults to implement. These intentionally remain outside Phase 2:

- Actual expedition area/target references and validated alternatives: phase 38.
- Exact quartermaster prototypes, approved item values/prices, minimum levels, proficiency variants, and final power review: phase 39.
- Live schema changes/import/cutover: phases 4–5b.
- Existing missing-mock repairs for account-bonus and supply-order harnesses: separate small maintenance blocks before their later acceptance gates.
- Later buff families and personal services: deferred phases with their own balance specifications.
- Empirical progression/uptime tuning: controlled pilot, phase 51.

Phase 3 can proceed with types, stable IDs, configuration tables, validation, and disabled build integration. Do not add reward hooks, make migrations live, import data, or turn on buffs in that phase.
