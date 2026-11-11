# Clan Progression and Personal Favor

Status: concept design with the Phase 2 pilot balance specification. Phases 3–27 (including 4a–4c and 5a–5b) provide disabled configuration, checked version-5 SQL persistence, explicit boot authority, migration/recovery rehearsal, Favor/account windows, durable exactly-once SQL award receipts, and checked Clan XP/level/Influence policy with cache reconciliation, personal Favor/account award policy and standings, and membership/lifecycle eligibility with retained-history SQL creation/disbanding. No live migration, award or progression enablement occurred. Read-only personal/staff commands and pure buff unlock/slot/cooldown queries are complete. Disabled activation validation and atomic replacement/receipt services are complete, using a version-1 activation extension to core schema v5. Durable cancellation/expiration, runtime hydration/invalidation and Company Vitality final HP integration are complete with disabled defaults. Prepared-caster Arcane Readiness timing and saved cycle snapshots are complete; spontaneous slot recovery is also complete; Psionic Focus out-of-combat PSP recovery is complete; Scholar's Encouragement source-qualified adventuring XP is complete; Artisan's Guidance production checks and the five-family stacking audit are complete; Player-facing buff commands and the shared combat eligibility query are complete; live kill and successful supply-order contributions are integrated with durable receipts and disabled defaults; next is Phase 28 delivery definitions. See the [Phases 26–27 handoff](clan_progression_phase_26_27_handoff.md). See the [Phases 24–25 handoff](clan_progression_phase_24_25_handoff.md). See the [Phases 22–23 handoff](clan_progression_phase_22_23_handoff.md). See the [Phase 21 handoff](clan_progression_phase_21_handoff.md). See the [Phase 20 handoff](clan_progression_phase_20_handoff.md). See the [Phase 19 handoff](clan_progression_phase_19_handoff.md). See the [Phase 18 handoff](clan_progression_phase_18_handoff.md). See the [Phases 16–17 handoff](clan_progression_phase_16_17_handoff.md). Illustrative values below are superseded where necessary by the [Phase 2 specification](clan_progression_phase_2_spec.md); pilot values remain subject to measured tuning.

## Goals

- Give clans experience and levels through members' activity.
- Make progression primarily unlock a library of selectable, temporary clan buffs.
- Limit simultaneous buffs so clans make meaningful choices.
- Give individual characters a parallel Favor progression that unlocks equipment and services.
- Support small clans, solo contributions, and different play styles.
- Build on Krynn's existing gameplay rather than require a separate clan-only activity loop.

The central idea is clan-wide progress that unlocks choices, alongside personal Favor that recognizes contribution. Higher clan levels provide a broader buff selection and better versions of selected buffs. Individual members earn access to equipment and services through their own participation.

## Existing Foundations

The codebase already provides useful starting points:

- Clan banks, membership ranks, permissions, halls, and activity records.
- Personal clan points and an officer command that awards them by spending clan bank funds.
- Existing mobs, named enemies, adventuring areas, and grouped combat.
- Quest infrastructure exists, but the current quest catalog is too small to support the initial progression loop.
- Crafting, supply orders, materials, motes, ammunition crafting, and crafting tools.
- Equipment proficiencies, skill bonuses, activated spells, and item binding data.
- Definitions and functions for clan territory benefits; their gameplay integration should be audited before designing stacking behavior around them.

Existing clan points should retain their existing uses. They should not convert directly into Favor or Clan XP: their bank-funded officer awards would otherwise allow accumulated wealth to purchase progression.

## Persistence Direction — MySQL

Use the project's existing MySQL database wherever practical for saving clan and progression data. Moving current clan data from the flat-file clan save into MySQL is explicitly in scope, rather than preserving that file as the long-term authority.

- Make MySQL authoritative for clan identity, core clan data, membership/ranks/privileges, treasury, progression, buff state/cooldowns, Favor, contribution windows, contracts, and reward receipts as those areas are migrated.
- Use transactional tables and database transactions for related changes, with stable IDs and unique event/claim keys to prevent duplicate rewards.
- Preserve existing clan vnums, membership, bank balances, ranks, privileges, halls, relationships, statistics, and legacy clan-point behavior during migration. Add an immutable clan incarnation identity so a reused vnum cannot inherit old Favor or cooldown history.
- Provide a repeatable importer, dry-run comparison, backups, and a controlled cutover from legacy saves. After cutover, do not maintain two competing authoritative clan stores or silently fall back to stale flat-file balances.
- Retain flat files where needed for compatibility, exports, backups, or player/inventory systems that have not been migrated. Changes spanning MySQL and those remaining files still require durable receipts and recovery; an SQL transaction alone cannot make a file write atomic.
- Prefer the existing database connection and persistence conventions. This does not require moving every unrelated character or world-data system into MySQL as part of clan progression.

This is a design requirement for future implementation. No schema, stored data, or live persistence behavior is changed by this document.

## Progression Measures

| Measure | Purpose | Spending |
| --- | --- | --- |
| Clan XP | Raises clan level and unlocks buffs, contracts, and quartermaster stock | Never spent |
| Clan Influence | Shared resource earned through activity; pays for activating buffs | Spendable |
| Personal Clan Favor | Measures a character's service to a particular clan; unlocks personal rewards | Never spent |

Favor is cumulative standing rather than a wallet. Buying an item must not lower a character below an unlock threshold. Purchases can use existing currencies or materials, with their exact costs determined later.

## Earning Progress

Members contribute through ordinary play, including solo play. Playing with clanmates provides additional opportunities, but small clans should still progress.

Contracts and appropriately challenging adventuring are the main sources of progression. The initial system must work without adding conventional quests or relying on the existing quest catalog. Clan contracts are proposed repeatable objectives layered onto existing combat, gathering, and crafting; they are not already implemented content.

| Activity | Proposed contribution |
| --- | --- |
| Defeat appropriately challenging enemies | A small, bounded background contribution while adventuring |
| Complete a supply order | Crafting contribution to Clan XP, Influence, and personal Favor |
| Complete a clan bounty | Larger reward for designated enemy types or named enemies |
| Deliver requested materials or crafted ammunition | Quartermaster contract rewards |
| Complete a clan expedition | Shared completion reward and personal participation rewards |
| Complete a gathering contract | A contribution route for resource-focused characters |

### Example Contracts

- Supply the quartermaster with a requested quantity of crafted arrow bundles.
- Defeat a selection of designated enemies appropriate to the participant's level.
- Gather requested materials of an appropriate grade.
- Complete a weaponsmithing, armorsmithing, or woodworking supply order.
- Complete the objectives of a designated expedition with clanmates.

Rewards should reflect assignment difficulty rather than directly copying character XP earned. Lower-level characters should contribute meaningfully, while trivial encounters should not become profitable for high-level characters. Characters at the level cap must still be able to earn Clan XP and Favor through qualifying activity.

The small background contribution from combat should be based on encounter eligibility and difficulty, before character XP bonuses. It must not become more profitable because a clan activates an XP buff. Repeated farming of the same easy enemies should have diminishing returns.

For shared contracts, award the clan's main completion reward once per reset period. Later participants can still earn their own personal Favor. This avoids rewarding only members present for the first completion.

### What Counts as an Expedition?

An expedition is a declared clan outing with a recorded set of objectives in an existing adventuring area. Simply grouping up or entering a zone does not complete one. It does not require a new dungeon, an instance, or a conventional quest chain.

A proposed first version would let members select an expedition contract, form a party, and work toward a short objective list. For example:

> **Stronghold Assault** — In a designated existing area, defeat 20 qualifying defenders and two designated named enemies. Complete the objectives with at least two eligible clan members participating. Return to the clan quartermaster to claim the completion reward.

The title and counts are illustrative. Actual targets would be selected from existing world content after an area audit.

Other expedition patterns:

| Expedition | Example completion conditions |
| --- | --- |
| Stronghold Assault | Defeat a quota of qualifying enemies and designated named enemies in one area |
| Regional Hunt | Defeat several designated enemy types across two or three existing areas |
| Named Enemy Circuit | Defeat a short list of named enemies, with alternate targets where availability is limited |
| Resource Expedition | Gather a requested set of materials and complete a related delivery; useful as a later extension |

For launch, Stronghold Assault and Regional Hunt are the most direct fits for the current combat loop. They need a curated contract list and progress tracking, but can reuse existing areas and enemies.

Proposed expedition rules:

- Require two participating clan members for a group expedition, with no large raid-size requirement.
- Keep solo bounties, gathering contracts, and supply orders available so small or quiet clans have other progression routes.
- Count legitimate kills and objectives, not time logged in, room visits alone, or total character XP earned.
- Credit healing, support, and other meaningful combat participation; do not require the killing blow.
- Select level-appropriate variants. Trivial enemies should not qualify for a stronger expedition simply because a low-level clanmate is present.
- Track a bounded outing, provisionally around 30–60 minutes of intended play, rather than requiring an entire zone to remain cleared at once. Exact timing and any expiry need tuning.
- Preserve completed objectives across ordinary mob respawns. Do not require exclusive ownership of a public zone.
- Offer alternate targets or sufficient choices when named enemies are unavailable.
- Apply shared reward limits per expedition/reset period and personal reward limits per participant/account. Repeating the same outing must not generate unlimited progression.

Once this loop works, longer expeditions could combine combat and resupply stages. Those should be later additions, not prerequisites for launching clan progression.

## Clan Levels

Proposed initial scope: 20 clan levels, two active buff slots early, and a maximum of three later.

| Clan level | Example unlock |
| --- | --- |
| 1 | Two active buff slots; initial XP, spell preparation, PSP recovery, and maximum-HP choices |
| 3 | Crafting buff and gathering contracts; gathering-yield buff deferred |
| 5 | Second-tier versions of selected buffs; additional contracts |
| 10 | Third active buff slot |
| 15 | Advanced quartermaster stock; additional support buff families remain deferred |
| 20 | Final buff tiers, prestigious commissions, and cosmetic rewards |

Most levels should unlock something useful. The main progression is a wider selection and improved versions, rather than an ever-growing number of simultaneous bonuses. The [Phase 2 specification](clan_progression_phase_2_spec.md) defines the initial level curve, intermediate gates, prices, budgets, and exact buff behavior.

## Active Clan Buffs

### Proposed Rules

- Buffs last 24 real hours and benefit eligible clan members wherever they adventure.
- Authorized clan ranks can activate or replace buffs once their cooldown permits. Active buffs cannot be refreshed, extended, or renewed.
- Activation costs Influence; selected buffs may also require gold or materials.
- Higher tiers replace lower tiers rather than stacking with them.
- Replacing a buff ends the previous buff without a refund.
- Each buff family has a clan-wide cooldown after its scheduled duration ends. No automatic renewal is included in the initial release.
- At most one combat-oriented buff can be active. Maximum HP, attack/damage, general defenses, and incoming-healing buffs belong to this category.
- Spell preparation and out-of-combat PSP recovery are recovery buffs and can accompany a combat buff.
- Personal Favor does not increase the strength of the clan's active buffs.

The intended choice is between supporting an expedition, a crafting session, leveling, or another current clan priority.

### Clan-Wide Buff Cooldowns

Buffs should have meaningful downtime rather than being immediately activated again when they expire.

Proposed initial timing, subject to balance tuning:

- **Active duration:** 24 real hours.
- **Cooldown:** 12 additional real hours after the scheduled expiration.
- **Next activation:** 36 hours after the successful activation, whether or not the buff was ended early.

For example, Company Vitality activated at noon is scheduled to expire at noon the next day. Its next activation is available at midnight afterward. Canceling it early does not move that next-activation time forward.

Cooldown rules:

- Cooldown belongs to the clan and buff family, not the activating character or occupied slot.
- All tiers of the same buff share a cooldown. Changing tier, officer, or slot cannot bypass it.
- An active buff cannot be refreshed or extended by paying again.
- Canceling or replacing it preserves the original next-activation timestamp and provides no refund.
- A different, unlocked buff family can fill an available slot if that family's own cooldown and the category limits permit it. Rotating different benefits is an intended choice.
- Cooldowns and expiration times use saved real-world timestamps and continue while members are offline. Restart, copyover, a clan level-up, or membership changes cannot clear them.
- Invalid activation attempts consume no Influence or materials and start no cooldown.
- The interface shows active time remaining and each family's next available activation time, including a clear reason when activation is blocked.

The 24-hour duration and 12-hour downtime are proposed tuning defaults, not fixed requirements. A cooldown always includes actual downtime after the scheduled active period; a cooldown equal to the duration and starting at activation would allow immediate repetition and would not meet the goal.


### Example Buff Library

| Buff | Example benefit | Reason to choose it |
| --- | --- | --- |
| Scholar's Encouragement | Adventuring XP bonus | Leveling sessions |
| Arcane Readiness | Reduced spell preparation time and applicable spontaneous spell-slot recovery time | Less downtime for spellcasters |
| Psionic Focus | Increased out-of-combat PSP regeneration | Less downtime for psionic characters |
| Company Vitality | Increased maximum HP | A broadly useful survivability choice |
| Martial Drills | A tightly capped attack and damage bonus | An offensive expedition choice |
| Company Warding | A modest bonus to saving throws generally | A defensive alternative to maximum HP or offense |
| Healer's Covenant | Increased healing received, with clear exclusions for life-drain and resource-conversion loops | Parties tackling sustained incoming damage |
| Artisan's Guidance | A meaningful crafting-check bonus | Difficult crafting and supply work |
| Gatherer's Bounty | Improved gathering yield or a chance of an extra material | Resource collection; extra output does not multiply clan contribution credit |

Movement discounts, lore-check bonuses, and fear-only protection are not part of the proposed initial library. The library should offer benefits noticeable enough to justify occupying one of only two or three active slots.

### Illustrative Buff Tiers

These values illustrate the direction, not approved balance numbers:

| Buff | Tier I | Tier II | Tier III |
| --- | --- | --- | --- |
| Arcane Readiness | 10% shorter preparation/recovery | 15% shorter | 20% shorter |
| Psionic Focus | 10% more PSP regeneration outside combat | 20% more | 30% more |
| Company Vitality | +5% maximum HP | +10% | +15% |

Percentage effects should remain useful at different character levels. Small per-tick PSP gains will need a rounding policy that makes the benefit real at low regeneration rates.

Important behavior to settle before implementation:

- **Spell preparation:** cover the relevant prepared-caster and spontaneous-caster recovery paths consistently. Preserve normal requirements, interruptions, and minimum preparation times. A buff must not grant spell slots, higher circles, or spells the character cannot normally use.
- **PSP regeneration:** the initial proposal improves out-of-combat recovery. An in-combat version would have a different power budget and should count as a combat buff if added later.
- **Maximum HP:** calculate the increase from a defined total excluding this clan buff, so it cannot multiply itself. Raising maximum HP should not immediately heal the character. Expiration or replacement clamps current HP to the new maximum without directly dealing damage or killing the character. Repeated toggling must not become a healing source.
- **Attack, damage, and saves:** even a broad +1 matters in this combat system. These are alternatives to Company Vitality, not effects that can all be stacked through separate clan slots.
- **Healing received:** distinguish actual healing from regeneration and resource-conversion effects so it cannot unintentionally create a self-sustaining loop.
- **Gathering yield:** additional materials may be useful to the economy, but must not generate additional Clan XP, Influence, or Favor simply by activating the buff. Contract credit needs to distinguish base qualifying work from clan-buff bonus output.

Example active selections:

- **Caster expedition:** Company Vitality + Arcane Readiness; add Psionic Focus when the third slot is available and the group includes psionic characters.
- **Offensive expedition:** Martial Drills + Arcane Readiness or Psionic Focus; the third slot can support leveling or another noncombat activity.
- **Crafting session:** Artisan's Guidance + Gatherer's Bounty; the third slot can support adventuring members.

These combinations should give members a reason to discuss what the clan is doing that day.

### Stacking and Territory

- Clan buffs must not increase Clan XP, Influence, or Favor earnings. Otherwise progression buffs become mandatory and established clans accelerate away.
- Define interactions with territory benefits, spells, feats, equipment, and other existing bonuses.
- Two effects providing the same clan benefit should generally use the stronger value.
- Territory control may provide additional contracts or resources, but must not be necessary for normal clan progression.

## Personal Clan Favor

Favor belongs to the character-clan relationship. It is independent of officer rank: administrative authority does not automatically confer veteran reward access.

| Favor standing | Example unlocks |
| --- | --- |
| Recognized | Clan insignia and basic quartermaster supplies |
| Trusted | Utility gear and additional personal contracts |
| Proven | Clan equipment commissions and crafting supplies |
| Distinguished | Advanced commissions and a personal service choice |
| Exalted | Prestigious appearances, titles, and special commissions |

Initial cumulative Favor thresholds are Recognized 100, Trusted 500, Proven 1,500, Distinguished 3,500, and Exalted 6,000. Phase 2 specifies a 600-per-account weekly Favor cap across characters/clans, alongside source limits and contribution diminishing returns.

## Clan Equipment

An equipment purchase can require all of:

- Required clan level.
- Required personal Favor standing.
- Required character level.
- Purchase materials or currency.

For example, a clan could unlock a scout equipment collection at clan level 8, while a member needs Proven standing to commission a piece.

Equipment should provide reliable alternatives and useful specializations:

- Clan-themed weapons covering existing proficiency families.
- Armor choices covering different armor proficiencies.
- Crafting tools, ammunition pouches, and utility items.
- Modest skill-focused equipment.
- Carefully limited activated spells with charges and cooldowns.
- Cosmetic versions and clan heraldry.

These items should remain within the normal equipment power budget. A useful activated spell should be balanced against the item's other properties, rather than added freely to already excellent gear. Class, proficiency, and normal equipment restrictions continue to matter.

## Personal Services

A proposed extension is one selected personal privilege at a time, unlocked through Favor.

Examples:

- A quartermaster requisition.
- Access to a specialized crafting commission.
- Access to a limited spell-preparation or PSP-recovery service at the clan hall, with its relationship to active clan recovery buffs defined before implementation.

This gives individuals a choice without creating another permanent stack of combat bonuses. Service costs, cooldowns, and access conditions remain to be specified.

## Fairness and Membership Changes

- Use weekly diminishing returns for repeatable contributions, with generous limits for normal play.
- Apply contribution limits at the account level where needed to prevent multiplying rewards through alts.
- Reward actual participation, rather than simply being grouped or standing nearby.
- Use requested deliveries with limits instead of unlimited gold or material donations.
- Allow small clans to complete objectives across several sessions.
- Do not decay Favor for inactivity.
- Preserve Favor records for the original clan when a character leaves; do not transfer Favor to another clan.
- Let purchased equipment remain usable after leaving, while clan services and active buffs require current membership.
- Keep officer awards separate from mechanically earned Favor.

## Player Information

The clan interface should show:

- Clan level and XP progress.
- The next unlock.
- Current Influence.
- Active buffs, occupied slots, and expiration times.
- Available contracts and completion progress.

The personal view should show:

- Favor standing and progress toward the next standing.
- Contribution progress and applicable limits.
- Available equipment and services.
- Exact reasons a reward is still locked.

Both text commands and the web client should present the same underlying information.

## Suggested Initial Release

- A focused buff library starting with adventuring XP, spell preparation/recovery, PSP regeneration, maximum HP, and crafting; additional offensive, defensive, and gathering choices can follow.
- Bounties, supply orders, and capped material or ammunition deliveries as the initial contract loop.
- A small curated set of group expeditions using existing areas and enemies; do not require expansion of the conventional quest catalog.
- Five personal Favor standings.
- A small quartermaster equipment collection.
- Clear contribution and reward information.

Multi-stage expeditions and specialized personal services can follow once the progression rate is established. Conventional quest completions can become an additional contribution source when there is enough quest content; they are not an initial progression requirement.

## Implementation Defaults and Remaining Decisions

The [Phase 2 balance/configuration specification](clan_progression_phase_2_spec.md) settles the initial staging defaults, including:

- Level 1–20 Clan XP thresholds and concrete small/large-clan pacing examples.
- Influence prices, contribution budgets, account/clan caps, and UTC reset rules.
- Initial buff families, tiers, categories, stacking, and 24-hour activation plus 12-hour downtime.
- All-level encounter/support eligibility, contracts, and expedition completion/reward rules.
- HP recalculation, prepared/spontaneous cycle behavior, and PSP-specific rounding.
- Favor thresholds, quartermaster price policy, migration identities, and release flags.
- MySQL authority, import/cutover requirements, and receipt/recovery boundaries.

Content and tuning work still required:

- Select and validate existing expedition areas, enemy references, and alternative targets in phase 38.
- Specify actual quartermaster templates, prices, minimum levels, and equipment budgets in phase 39.
- Design individual personal services and additional buff families in later dedicated phases.
- Implement and rehearse the MySQL migration and cutover; documentation does not migrate live data.
- Adjust pilot rates and thresholds using measured progression and actual buff choices before public release.

## Inspiration

- [Final Fantasy XIV: About Free Companies](https://na.finalfantasyxiv.com/lodestone/special/grandcompany/about_freecompany/): company ranks unlock company actions that benefit members.
- [Guild Wars 2: Guild Halls—Missions for All](https://www.guildwars2.com/en/news/guild-halls-missions-for-all/): shared guild mission rewards are distinct from personal participation rewards, and later participants can still receive personal rewards.

These are design inspirations rather than a proposal to copy either game's exact rules.
