# Epic feat expansion catalogue for Chronicles of Krynn

**Status:** All 48 designs are implemented in this checkout. See [first batch](EPIC_FEAT_FIRST_BATCH.md), [second batch](EPIC_FEAT_SECOND_BATCH.md), and [completion notes and DC audit](EPIC_FEAT_THIRD_BATCH.md). **Research date:** 2026-11-17.

This catalogue proposes **48 feats: 10 psionicist, 8 artificer, 10 summoner, 12 general, and 8 for other classes**. It combines the epic-feat tradition introduced in D&D 3e's *Epic Level Handbook*, the revised D&D 3.5 SRD and psionics rules, and Pathfinder 1e class abilities and feats. Every numerical rule below is a proposed Chronicles of Krynn rule, not a claim about an unchanged tabletop rule.

The goal is more build choices at levels 21–30: resource management, ally support, control resistance, defensive reactions and companion customization, alongside a smaller amount of additional damage. The proposals retain existing action, follower and immunity limits.


Browse: [Psionicist](#4-psionicist-proposals) · [Artificer](#5-artificer-proposals) · [Summoner](#6-summoner-proposals) · [General](#7-general-epic-proposals) · [Other classes](#8-additional-class-proposals) · [Release requirements](#9-suggested-release-order-and-implementation-requirements).

## 1. Findings from the current code

The premise that psionicist and summoner have no epic support needs a qualification:

| Area | What this checkout contains | Consequence for the catalogue |
| --- | --- | --- |
| Psionicist | Selectable The Psi Mind, Epic Power Penetration, Epic Power Damage, Epic Shambler and four epic-power unlocks. Automatic Epic Psionics at class levels 21/25/29, Epic Augmenting at 23/27, Master of the Mind at 30. | Expand sideways instead of adding another unconditional damage multiplier or PSP-cap increase. |
| Psionic naming | Epic Power Penetration formerly described +6 manifestation DC; the runtime audit found **+6 power-resistance penetration**, and the description is now corrected. | Runtime behavior is preserved; P02 independently adds at most +2 save DC. |
| Psionic focus | A timed, twelve-round affect, with its own use cooldown. It is not the tabletop spend-and-regain focus token. | Focus feats below extend or reference the timed affect; they do not quietly replace the existing focus model. |
| Augmentation | `base_augment_psp_allowed()` already adds five augmentation points per Epic Augmenting rank; individual powers impose additional limits. | Cost reductions do not increase either augmentation limit or a power's legal effect. |
| Summoner | Sixth-circle progression plus an epic-spell feature at 21; Epic Aspect at 25 and Epic Eidolon at 30. Epic Augment Summoning and several general spell feats are in its class selection list. | Add distinct choices; preserve automatic class features. |
| Summoner perks | Evolution pool bonuses, Rapid Summoning, Evolution Surge Enhancement, Eternal Bond, Twin Eidolon, Planar Unity, extra summon slots and several summoning/stat bonuses already exist. | Do not sell those same benefits again under new feat names. Specific overlaps are addressed below. |
| Artificer | Weird Science devices, infusions, metamagic item use, salvage and crafted wood/stone/iron golems exist. Its epic class selection list is largely ability-score feats; iron golem construction is also granted automatically at class level 30. | Add epic engineering choices without selling the automatic iron golem feature twice. |
| Artificer perks | Battery/recharge upgrades, Field Recompiler, Unbound Invention, crafting efficiency, support matrices, emergency survival, construct attacks and damage sharing already exist. | Avoid another device slot, free recharge, extra golem attack or lethal-hit survival copy. |
| Existing unavailable epics | Epic Dodge, Intensify Spell, Epic Spellcasting, Great Smiting and several other entries exist but are disabled or not learnable. | G11 and O04 explicitly propose rehabilitating existing entries rather than allocating duplicate IDs. |
| Advancement | Separate general epic and class epic point pools exist. General epic points follow the current every-three-character-level rule; class cadence is data driven. | Use existing point pools and eligible-class lists; this document does not award extra feat points. |

Code references: [feat definitions and epic registrations](../src/feats.c), [class advancement and class lists](../src/class.c), [augmentation and focus](../src/psionics.c), [Study point spending](../src/study.c), [perk definitions](../src/perks.c), [evolutions](../src/evolutions.c), [follower admission](../src/utils.c), [action timings](../src/actions.h), [existing bloodline rules](SORCERER_BLOODLINES.md).

**Audit limit:** a registration, description or perk identifier proves that the option exists in source, not that every runtime interaction is correct. Before implementation, verify the listed overlap against its actual effect. This catalogue does not assume that disabled feats are available to players.

## 2. Sources and edition boundaries

- **D&D 3e:** *Epic Level Handbook* is the origin of the epic-feat design family used here. The [licensed book listing](https://www.dmsguild.com/product/23683/Epic-Level-Handbook-3e) identifies the original edition; its page was access restricted during this research. This catalogue therefore verifies mechanics through the revised SRD, rather than claiming that every 3.0 and 3.5 prerequisite is identical.
- **D&D 3.5:** [Epic feat rules and entries](https://www.d20srd.org/srd/epic/feats.htm), [epic psionic progression](https://www.d20srd.org/srd/epic/psionicClassProgressions.htm), and [psionic feats](https://www.d20srd.org/srd/psionic/psionicFeats.htm). These are reproductions of the published SRD rules, not community feat proposals. Individual entries link the relevant rule.
- **Pathfinder 1e:** [construct construction and modifications](https://legacy.aonprd.com/ultimatemagic/magic/buildingAndModifyingConstructs.html), [Summoner](https://aonprd.com/ClassDisplay.aspx?ItemName=Summoner), and the individual Archives of Nethys/legacy PRD entries linked below. Pathfinder class features and ordinary feats are being **promoted into custom epic options**. They are not presented as official Paizo epic feats, and no mythic-tier subsystem is assumed.

**Source labels:** **ELH/SRD adaptation** = a D&D epic concept with revised MUD rules; **PF epic promotion** = an ordinary PF feat or class feature redesigned for epic play; **synthesis** = a new MUD feat combining identified source concepts. Names prefixed with Epic are often new names chosen here.

## 3. Rules shared by every proposal

These rules resolve defaults so individual entries can be implemented without inventing their missing timing or stacking behavior.

1. **Eligibility and cost.** All entries require total character level **21+**, one available epic feat point and their additional prerequisites. Class-specific entries may instead consume a point from that class's epic pool when registered as eligible. Each purchase costs one point. Unless an entry gives ranks, it is selectable once. Attribute prerequisites use permanent, unbuffed scores; skill prerequisites mean trained ranks, not the buffed result of `compute_ability`. If a prerequisite is lost, suspend the benefit without refunding the point.
2. **Level notation.** `L` is the class level named in the entry, capped at 30. `INTmod`, `WISmod` and `CHAmod` are the native ability modifiers. `CL`/`ML` are the effective caster/manifester levels of the specific spell/power being used, not the character's best unrelated class. A listed class level of 20 still requires total level 21.
3. **Time and actions.** One round is **six real seconds**. Standard, move and swift use their existing six-second action cooldowns. Full round consumes both standard and move. No feat creates a new action slot. Passive reactions do not consume an action unless specified. A duration of three rounds is eighteen seconds; ten rounds is sixty seconds.
4. **Recovery.** `1/300s` means one activation, recovering 300 real seconds after commitment. `N/300s` is a fixed N-use pool resetting 300 seconds after its first committed use; retries do not extend it. It is not N independently refreshing charges. Entries specifying 30/60/120 seconds use those exact intervals. These are proposals for epic-feat timers, not a blanket change to unrelated existing abilities.
5. **Validation and commitment.** Reject invalid targets, insufficient resources, unavailable actions and illegal destinations without spending a use. Once activation is committed, spend the use/action/resource before callbacks. A miss, successful enemy save, failed dispel check or interrupted committed ability does not refund them. An armed effect's use is spent when armed; cancellation or expiry never refunds it.
6. **Targets.** Hostile effects require normal PvP consent, peaceful-room, mission/quest and AoE-containment checks. Friendly effects require self, an existing group ally or the explicitly allowed owned companion. Unless stated otherwise all targets must be visible, alive and in the same room. A group member is not automatically a legal hostile target.
7. **Movement.** All movement uses normal legal exits, doors, movement points, flight/swim requirements, room/zone restrictions and directional blocker rules. No feat bypasses a blocker the player could not normally pass. A feat referencing another room says exactly how passage is checked. No new cross-zone or line-of-sight geometry is assumed.
8. **Bonus types.** Highest positive bonus of a given non-stacking type applies; penalties retain their normal rules. A feat that **increases an existing feature's bonus** changes that feature's contribution, rather than adding a second copy. Rank bonuses stack only to their stated rank cap. No effect multiplies another feat's bonus unless explicitly stated.
9. **Damage.** Extra damage is a separate typed packet unless an entry specifies modification of the base packet. Separate packets use normal defenses, cannot critically multiply and cannot trigger a second proc, lifesteal or on-hit chain. Per-cast bonuses apply once across all rays, ticks, targets and echoes of that cast unless specifically stated otherwise. No feat bypasses plot invulnerability, native critical-immunity exceptions, or hard boss immunity flags by default.
10. **Resource reductions.** Compute legal power/spell effects and caps before discounts. Never reduce PSP below the unaugmented base PSP cost or spell slots below the spell's native circle. Explicit discount-family limits override permissive stacking; a lower paid cost never authorizes additional augmentation. Refunds cannot exceed the resource actually paid.
11. **Ordinary magic.** Unless specifically included, eligible spells/powers exclude epic spell/power unlocks, item casts, racial abilities, ongoing room hazards and companion spells. A self-only exception in an entry applies only to its named whitelist. Psionic and magical effects respect existing suppression, spell/power resistance, saves and dispelling where applicable.
12. **Armed effects.** Only one pending use of each feat can be armed. A chosen spell/power/category is fixed at arming. If the effect's owner is extracted, the arm is removed. Buffs do not accrue multiple copies from save/load, resummoning, respec or repeated equipment recalculation.
13. **Persistence.** Save cooldowns and spent counters on the **player owner**, including companion-triggered uses. Proposed new timers use absolute expiration timestamps so offline time counts. Save armed effects with their remaining expiry and snapshotted parameters; do not restore spent effects. Resource-max feats never refill current resources on acquisition, respec or login.
14. **Healing and temporary HP.** Healing uses the native healing pipeline and cannot resurrect. Temporary HP is one non-healing shield with a remaining-point counter; repeated applications from the same feat replace only if stronger, never add or refill an existing shield. Expiry or loss of eligibility removes it. Health thresholds use current ordinary max HP; temporary HP does not increase that denominator.
15. **Companions.** Benefits apply only to the specified live companion still owned by the feat holder. Native category/follower limits remain. Twin Eidolon does not duplicate owner cooldowns. No proposal grants another follower slot. Permanent companion bonuses recompute from the owner's feats; temporary buffs retain their original expiry on permitted pet restore.

### Artificer implementation boundary

A01–A08 extend the existing Artificer, not a new casting class. A **device use** means one native Weird Science activation charge; a **wand/staff charge** is the existing item resource. They are never interchangeable unless an entry explicitly consumes that resource. A constructed golem means the Artificer's currently active crafted `MOB_GOLEM` companion, with restored ownership and crafting identity; ordinary summons do not qualify. Golem features require the server's mote crafting system. Disabled crafting modes must hide those purchases or explain their unavailability before point spending.

Native invention allocation, recharge, item metamagic eligibility, crafted gear caps and recipes remain authoritative. No proposal adds a new item tier or permission to manufacture unrestricted epic equipment. Proposed `epic ...` commands below describe player-facing syntax; they are not existing commands.

## 4. Psionicist proposals

### P01 — Epic Power Knowledge

- **Source/status:** ELH/SRD adaptation of [Power Knowledge](https://www.d20srd.org/srd/epic/feats.htm#powerKnowledge); new selectable feat. **Priority:** first wave.
- **Prerequisites:** Psionicist 20, INT 21, trained Spellcraft 20, ninth-circle psionic access.
- **Exact effect:** Learn two additional ordinary powers from the native psionicist catalogue at circles you can manifest. Normal discipline/list restrictions apply. These selections do not consume normal known-power choices and grant no PSP or epic-power unlocks.
- **Ranks/actions:** Up to three ranks, with additional total-level gates 24 and 27. Passive. Select two different powers per rank in Study; no duplicate or already-known choices. On respec, remove only these tagged grants; if a power later becomes known normally it remains known.
- **Integration:** Known-power membership and Study. More breadth without duplicating The Psi Mind or Epic Power Damage.

### P02 — Epic Discipline Focus

- **Source/status:** Synthesis of [Epic Spell Focus's psionic conversion](https://www.d20srd.org/srd/epic/feats.htm#epicSpellFocus) and native disciplines; new feat. **Priority:** second wave.
- **Prerequisites:** Psionicist 20, INT 23, trained Spellcraft 20.
- **Exact effect:** Choose one native psionic discipline in Study. Ordinary powers from it gain **+1 save DC per rank**, maximum +2. No effect on weapon attacks, power resistance checks or powers without saves.
- **Ranks/actions:** Two ranks; second requires total level 27. Passive; both ranks apply to the same discipline.
- **Stacking:** Adds to existing psionic DC features,  but contributes no more than +2 itself. Never doubles other bonuses. **Audit completed:** the existing +6 affects resistance penetration, despite its old DC description. Its runtime behavior is preserved. See the completion notes for enumerated save rates.

### P03 — Epic Augmentation Efficiency

- **Source/status:** Synthesis inspired by [Improved Metapsionics](https://www.d20srd.org/srd/epic/feats.htm#improvedMetapsionics); new feat. This is an augmentation-cost redesign, not the tabletop metapsionic effect. **Priority:** first wave.
- **Prerequisites:** Psionicist 20, INT 21, Expert Augmenting.
- **Exact effect:** Reduce the PSP paid for an ordinary power's augmentation by **2**, never its base PSP cost. The selected augmentation must first fit both the native manifester allowance and that power's maximum/step size. Example: base cost 7 plus 6 legal augmentation costs 11 instead of 13, with exactly the original +6 augmentation effect.
- **Limits/actions:** Passive. Across this feat and any existing perk/item discount applying to augmentation, use the **largest augmentation discount**, not their sum. This feat's maximum discount is 2. No PSP refunds or bonus augmentation points.
- **Integration:** Apply at final PSP payment, after legal augmentation is fixed. Does not replace automatic Epic Augmenting.

### P04 — Epic Focus Continuity

- **Source/status:** MUD redesign inspired by [Epic Psionic Focus](https://www.d20srd.org/srd/epic/feats.htm#epicPsionicFocus); new feat. It does not import tabletop token expenditure. **Priority:** first wave.
- **Prerequisites:** Psionicist 20, Psionic Focus, trained Concentration 20.
- **Exact effect:** A normal successful activation of your existing psionic focus lasts **18 rounds instead of 12**. It retains all native conditions and suppression rules.
- **Limits/actions:** Passive upgrade to that activation. Does not refresh an already active focus, reduce its cooldown, restore a use, stack duration from multiple activations or alter perks that consume the focus affect.
- **Integration:** One duration calculation plus normal persistence. Existing focus-channeling perks remain responsible for their own effects.

### P05 — Epic Psionic Contingency

- **Source/status:** Epic promotion of [psionic contingency](https://www.d20srd.org/srd/psionic/powers/contingencyPsionic.htm); new feat. **Priority:** later, requires a trigger service.
- **Prerequisites:** Psionicist 23, INT 23, trained Spellcraft 23; know at least one of **inertial armor, biofeedback or vigor**.
- **Activation:** `epic psi-contingency <power>`; full round, **1/300s**. Prepay that power's unaugmented PSP cost plus **5 PSP**. Store it for 300 seconds. No metamagic or augmentation.
- **Exact effect:** After a hostile damage packet leaves you alive at or below **25% max HP**, automatically manifest the stored power on yourself once, at the lesser of your current and armed ML. It uses normal buff stacking and a maximum duration of ten rounds. It cannot prevent that packet from killing you.
- **Counters:** Suppression delays the trigger until another eligible packet while unsuppressed; expiry still runs. Dispelling the armed effect cancels it. The trigger does not count as a manual cast for procs or PSP recovery. No resurrection, offensive stored power or resource refund.

### P06 — Epic Resistance Rebuttal

- **Source/status:** Synthesis inspired by [Tenacious Magic](https://www.d20srd.org/srd/epic/feats.htm#tenaciousMagic); new psionic resistance-check option, not another save-DC feat. **Priority:** second wave.
- **Prerequisites:** Psionicist 20, Greater Power Penetration, trained Spellcraft 20.
- **Activation:** `epic psi-rebuttal`; swift, **5 PSP**, **1/60s**; armed for three rounds.
- **Exact effect:** The next ordinary power encountering positive power resistance rolls its penetration d20 twice and keeps the better result. Apply the native fixed penetration modifiers once. The arm is consumed at that resistance check, including failure; it is not consumed by a power that encounters no resistance.
- **Stacking:** If an existing ability already gives advantage on the same check, do not roll a third die. Does not bypass resistance automatically or change save DC. Scope is the next target's check, not every target in an AoE.

### P07 — Epic Mental Bastion

- **Source/status:** Synthesis of [Epic Will](https://www.d20srd.org/srd/epic/feats.htm#epicWill) and psionic mental defenses; new feat. **Priority:** first wave.
- **Prerequisites:** Psionicist 20, WIS 19, trained Concentration 20.
- **Exact effect:** Gain a **+4 insight bonus** on native saves against charm, domination, confusion and fear. A failed or absent save is never converted into immunity. Does not help against unrelated death effects, energy damage or generic Will saves.
- **Limits/actions:** Passive. Highest applicable insight save bonus wins, including Touch of Destiny. Stacks with Epic Will's separate feat bonus when a Will save is actually made.
- **Integration:** Classify by the actual incoming effect, not just its spell school; no guessed immunity flag removal.

### P08 — Epic Manifesting Poise

- **Source/status:** Epic promotion of [Combat Manifestation](https://www.d20srd.org/srd/psionic/psionicFeats.htm#combatManifestation); new feat. **Priority:** first wave.
- **Prerequisites:** Psionicist 20, Combat Manifestation, trained Concentration 20.
- **Exact effect:** Gain **+4 circumstance** to Concentration checks caused by taking damage while manifesting an ordinary power. If one hit causes multiple typed damage packets, this bonus is applied to the native check, not repeatedly per packet.
- **Limits/actions:** Passive. No change to manifestation duration, actions, interruption DC or damage. Does not help checks for spells, item activations or other classes. Combines with Combat Manifestation; does not stack another circumstance bonus on that same check.
- **Integration:** Distinct from Accelerated Manifest and Rapid Manifester perks, which already address speed.

### P09 — Shared Psionic Mantle

- **Source/status:** Synthesis inspired by [Chain Power](https://www.d20srd.org/srd/psionic/psionicFeats.htm#chainPower) and support manifestation; new feat. **Priority:** second wave.
- **Prerequisites:** Psionicist 23, INT 23; know inertial armor, biofeedback or vigor.
- **Activation:** `epic psi-mantle <ally> <power>`; standard, **1/60s**; pay the power's unaugmented base PSP cost **plus 8 PSP**.
- **Exact effect:** Manifest one of those three named powers on one visible grouped ally in your room, including an owned grouped companion. This is an explicit exception to those powers' self-only targeting. Use your current ML, no augmentation/metamagic, duration at most ten rounds.
- **Limits:** Does not also affect you; no copying an existing buff, spell reflection or refund. All native stacking, dispel, suppression and temporary-HP rules apply. It cannot duplicate a summon, heal, offensive power or epic ward.

### P10 — Epic Schism: Sentinel Mind

- **Source/status:** MUD redesign of [Schism](https://www.d20srd.org/srd/psionic/powers/schism.htm); new feat. The second mind performs no autonomous casts. **Priority:** later, condition metadata required.
- **Prerequisites:** Psionicist 25, INT 25, trained Concentration 23.
- **Activation:** `epic sentinel-mind`; swift, **15 PSP**, **1/300s**; lasts ten rounds.
- **Exact effect:** On the first failed save against charm, domination, confusion or fear while armed, make one new save against the **same snapshotted DC and save type**, with an extra +2 insight bonus on the retry. Keep the second result. Consume the sentinel before retrying so it cannot recursively trigger itself.
- **Counters:** Effects without a save, scripted control, stun, paralysis and death effects are excluded. The sentinel can be dispelled or suppressed. Other retry effects cannot retry this same failure again; let the player select this or an existing armed retry before the first roll, not after the result.

## 5. Artificer proposals

### A01 — Epic Artisan's Insight

- **Source/status:** Synthesis of [ELH/SRD epic item creation](https://www.d20srd.org/srd/epic/feats.htm#craftEpicWondrousItem) and PF [construct crafting checks](https://legacy.aonprd.com/ultimatemagic/magic/buildingAndModifyingConstructs.html); new. **Priority:** first wave.
- **Prerequisites:** Artificer 20, INT 23, Craft Wondrous Item, trained Arcana 20.
- **Activation:** `epic artisan-insight`; swift, **1/300s**, out of combat. Arm for ten rounds.
- **Exact effect:** The next failed d20 check for a normal recipe craft or native golem construction/repair rolls a second d20 with the same modifiers and DC, keeping the second result. Consume the arm on that failure. Resolve the recipe's success/failure material consumption exactly once after the reroll.
- **Limits:** No reroll of random item quality, salvage yield, gathering, device instability, or prerequisite validation. Does not waive recipes, skill minimums, materials or item caps. The player must start the check while the arm is active. Commit-time state prevents disconnect/retry rerolls.

### A02 — Epic Efficient Item Creation

- **Source/status:** [ELH/SRD Efficient Item Creation](https://www.d20srd.org/srd/epic/feats.htm#efficientItemCreation), bounded for real-time crafting; new. **Priority:** first wave.
- **Prerequisites:** Artificer 23, Craft Wondrous Item or Craft Magical Arms and Armor, trained Arcana 23.
- **Exact effect:** Choose one of those owned item-creation categories at Study. Reduce eligible normal recipe completion time by **20 percentage points of the unmodified recipe time**. Add this to other time reductions, but cap their combined reduction at **50%**; final duration is rounded up to whole seconds, minimum one second.
- **Ranks/actions:** One rank, passive. Applies only to newly started crafts whose recipe explicitly records the chosen category. Unclassified recipes receive no reduction until categorized.
- **Limits/overlap:** Efficient Crafter's existing 10% reduction participates in the same cap. No material/gold discount, instant completion, recharge benefit, golem construction speed bonus or change to invention/matrix rebuilding. Cancelling retains normal penalties.

### A03 — Epic Metamagic Science

- **Source/status:** [ELH/SRD Improved Metamagic](https://www.d20srd.org/srd/epic/feats.htm#improvedMetamagic), adapted to native Metamagic Science; new. **Priority:** first wave.
- **Prerequisites:** Artificer 23, Metamagic Science, trained Use Magic Device 23; three metamagic feats.
- **Activation:** `epic efficient-trigger`; swift, **1/60s**, arm for three rounds.
- **Exact effect:** The next legal manually activated wand/staff spell with a positive Metamagic Science charge surcharge pays **one fewer surcharge charge**. Base activation charges are unchanged; total charge payment cannot fall below that base. Consume on activation commitment, even if resisted or interrupted.
- **Limits:** No reduction in item-use DC, final spell circle, action cost or target restrictions. Other charge discounts and this feat use the single largest discount, not their sum. No potions, scrolls, Weird Science devices or free activation when base charges are unavailable.

### A04 — Epic Device Penetration

- **Source/status:** Synthesis of [ELH/SRD Epic Spell Penetration](https://www.d20srd.org/srd/epic/feats.htm#epicSpellPenetration) with native Weird Science; new. **Priority:** first wave.
- **Prerequisites:** Artificer 20, Weird Science, Spell Penetration, Greater Spell Penetration, trained Use Magic Device 20.
- **Activation:** `epic pierce-device`; swift, **1/60s**, arm for three rounds.
- **Exact effect:** The next manual Weird Science device activation gains **+4 circumstance on its first spell-resistance check**. Consume the arm when the activation commits, even if no resistance check occurs. A multi-spell device receives the bonus on only the first check in native spell-list/target order.
- **Limits/overlap:** Existing Combat Calibration applies normally subject to bonus typing; no save DC, caster level, immunity bypass or overcharge reliability bonus. No cascade, proc, echo or automatically triggered activation benefits.

### A05 — Epic Charge-Fed Repair

- **Source/status:** PF epic promotion of the [clockwork familiar's energy consumption](https://legacy.aonprd.com/bestiary5/clockworkFamiliar.html), repurposed for crafted golems; new. **Priority:** second wave.
- **Prerequisites:** Artificer 20, Construct Stone Golem, trained Use Magic Device 20.
- **Activation:** `epic charge-repair <golem> <wand>`; standard, **1/300s**; consumes **three charges** from a carried ordinary charged wand with a native spell of circle 1–3. All three must be available.
- **Exact effect:** Repair your active crafted golem for **min(60, 10 + 2 × Artificer level)** HP, capped at its missing HP. May be used in combat. It is a construct repair, with native healing suppression checks; the wand's spell is not cast.
- **Limits:** No destroyed golem recovery, charges from devices/staves/infinite-use items, discount from A03, spell procs, or charge refunds on overhealing. Reject a full-HP golem before commitment. Native out-of-combat material repairs remain available and unchanged.

### A06 — Epic Anchored Chassis

- **Source/status:** Synthesis from PF [construct modifications](https://legacy.aonprd.com/ultimatemagic/magic/buildingAndModifyingConstructs.html), translated into MUD positional defenses; new. **Priority:** first wave.
- **Prerequisites:** Artificer 20, Construct Stone Golem, INT 21.
- **Exact effect:** Your active crafted golem gains **+4 circumstance to defensive checks or saves specifically against trip, knockdown, bull rush or forced directional movement**. If an effect has no defensive roll, this feat grants no new roll or immunity.
- **Ranks/actions:** One rank, passive. Apply once through the defensive contest, not both a skill check and a subsequent save for the same attempt. Highest circumstance bonus applies.
- **Limits/overlap:** No ordinary AC, damage reduction, grappling escape, teleport protection or player-owner bonus. Complements existing chassis HP/AC perks without copying them. Ownership changes remove it.

### A07 — Epic Golem Aegis

- **Source/status:** PF [shield guardian construct modifications](https://legacy.aonprd.com/ultimatemagic/magic/buildingAndModifyingConstructs.html), redesigned as finite protection rather than another damage-sharing loop; new. **Priority:** second wave.
- **Prerequisites:** Artificer 23, Construct Stone Golem, INT 23.
- **Activation:** `epic golem-aegis`; standard, **1/120s**. Your same-room active crafted golem must have at least 31 HP.
- **Exact effect:** Deduct **30 HP directly from the golem** and give yourself **30 temporary HP for three rounds**. The golem payment cannot kill it and bypasses damage redirection, mitigation and damage-triggered effects. The shield follows shared temporary-HP rules.
- **Limits/overlap:** No payment from shields, no repair to owner/golem and no Fortress Engine transfer or Soulbound Infusion trigger from the payment. Reject an already active Aegis shield before spending. Leaving the golem's room or losing ownership removes the remaining shield.

### A08 — Epic Constructbane Infusion

- **Source/status:** Synthesis of [ELH/SRD epic arms creation](https://www.d20srd.org/srd/epic/feats.htm#craftEpicMagicArmsAndArmor) and PF [construct traits](https://legacy.aonprd.com/bestiary/creatureTypes.html); new temporary specialization. **Priority:** second wave.
- **Prerequisites:** Artificer 25, Craft Magical Arms and Armor, trained Arcana 23.
- **Activation:** `epic constructbane <wielded weapon>`; standard, **1/60s**. Duration three rounds; only one weapon per owner may carry this feat.
- **Exact effect:** Your first successful manual weapon hit each six-second interval against a native construct deals an additional **2d6 untyped physical damage**. The packet undergoes ordinary physical DR; this is not precision damage and does not require a critical-vulnerable target.
- **Limits:** Weapon must remain wielded by you. Transfer, disarm, unwield or expiry ends the effect. No follower attacks, spell attacks, retaliation, extra item bonus slots, salvage-value change or permanent enchantment. The six-second proc timer belongs to the owner and cannot be reset by changing weapons.

## 6. Summoner proposals

For these entries, **primary eidolon** means the owner's original eidolon, identified by its stable companion identity; a Twin Eidolon mirror is not a second primary. A normal conjured creature means a temporary creature from the existing summon-creature spell family, excluding eidolons, purchased pets, mounts, undead animation, familiars, constructs built by crafting and charmed NPCs.

### S01 — Epic Evolution Reserve

- **Source/status:** PF epic promotion of [Extra Evolution](https://aonprd.com/FeatDisplay.aspx?ItemName=Extra%20Evolution); new feat. **Priority:** first wave.
- **Prerequisites:** Summoner 20, CHA 21, Eidolon class feature.
- **Exact effect:** Gain **2 permanent evolution points per rank** for the primary eidolon's normal Study evolution budget. All native costs, rank gates, form prerequisites and incompatibilities still apply.
- **Ranks/actions:** Two ranks, second at total level 27. Passive; maximum +4 points. This budget explicitly adds to existing evolution-pool perks; it does not grant another Aspect selection or another eidolon. Twin creation uses its normal mirroring rules, not a separate editable +4 pool.
- **Persistence:** Removing the feat requires a valid evolution respec before finalization; do not leave an overspent creature. Recalculation grants no current HP or resource refill.

### S02 — Epic Adaptive Eidolon Ward

- **Source/status:** Synthesis of PF [eidolon defenses and evolutions](https://aonprd.com/ClassDisplay.aspx?ItemName=Summoner); new feat. **Priority:** first wave.
- **Prerequisites:** Summoner 20, CHA 21, trained Spellcraft 20.
- **Activation:** `epic eidolon-ward <acid|cold|electricity|fire|sonic>`; swift, **1/120s**. Primary eidolon must be in your room.
- **Exact effect:** The primary gains **20 flat resistance to the selected energy for ten rounds**. Selecting another energy cannot preserve the previous ward.
- **Stacking/counters:** Use the highest applicable flat resistance against that energy, not +20 on top of native/perk resistance. No immunity, additional HP, natural armor or mind-affecting protection. Thus it does not reproduce Planar Unity. Dispelling or suppression works normally; dismissing/resummoning does not restore its spent use or duration.

### S03 — Epic Resilient Bond

- **Source/status:** MUD redesign of [Resilient Eidolon](https://aonprd.com/FeatDisplay.aspx?ItemName=Resilient%20Eidolon); new feat. **Priority:** second wave.
- **Prerequisites:** Summoner 23, CHA 23, Life Link class feature.
- **Exact effect:** If a damage packet would kill the primary eidolon **after its normal Life Link survival attempt has failed**, allow it one **DC 20 Will save**. On success reduce only that packet enough to leave it at 1 HP; on failure resolve death normally. Consume the owner's **1/300s** use before the save, either way.
- **Limits/actions:** Passive. No protection against extraction, scripted death, dismissal, starvation or voluntary sacrifice. No second Life Link retry, healing, invulnerability duration or companion resurrection. Does not trigger for the Twin Eidolon mirror.
- **Adaptation:** The tabletop's owner-unconsciousness persistence becomes a bounded damage-survival retry. Eternal Bond's fast healing/recall benefits remain separate.

### S04 — Epic Spell Sharing

- **Source/status:** PF epic promotion of the Summoner's [Share Spells concept](https://aonprd.com/ClassDisplay.aspx?ItemName=Summoner); new feat. **Priority:** second wave.
- **Prerequisites:** Summoner 20, trained Spellcraft 20; know one qualifying spell.
- **Activation:** `epic shared-cast <spell> [normal spell option]`; normal spellcasting action, **1/60s**. Cast using a Summoner slot **one circle higher** than normal; final slot must be circle six or lower.
- **Exact effect:** Cast **barkskin, blur, resist energy or stoneskin** on both yourself and the same-room primary eidolon from that one slot. Resolve each target's stacking/immunity separately. Preserve the selected resist-energy type; use your normal CL and ordinary spell duration. Pay all native material/resource costs once.
- **Limits:** Both targets must be valid before commitment. No healing, temporary-HP grant, offensive spell, summon, epic spell, Twin mirror or third target. Does not duplicate cast-trigger procs or restore missing HP. If interrupted, neither buff is applied.

### S05 — Epic Planar Anchorage

- **Source/status:** Synthesis of PF [summoner bond features](https://aonprd.com/ClassDisplay.aspx?ItemName=Summoner) and banishment resistance; new feat. **Priority:** first wave.
- **Prerequisites:** Summoner 20, CHA 21, trained Spellcraft 20.
- **Exact effect:** Your primary eidolon and normal conjured creatures gain **+4 circumstance on saves specifically against hostile banishment, dismissal and effects that sever your control**. If such an effect instead uses an opposed caster-level check, add **+4 to the defender's check or DC**, not both.
- **Limits/actions:** Passive; applies while the creature remains yours. No protection against its ordinary duration expiring, your dismissal, death/extraction or an effect that grants no save/check. No general spell resistance, mind-affecting immunity or follower-limit exemption. Highest circumstance bonus applies.
- **Integration:** Add effect classification and owner lookup; avoids duplicating Planar Unity's stat/immunity benefits.

### S06 — Epic Eidolon Intercession

- **Source/status:** Room-based synthesis of PF [Shield Ally and Life Bond](https://aonprd.com/ClassDisplay.aspx?ItemName=Summoner); new feat. **Priority:** later, damage ordering required.
- **Prerequisites:** Summoner 23, Greater Shield Ally, CHA 23.
- **Activation:** `epic intercession <group ally>`; standard, **1/60s**. Choose a player group ally other than yourself; ally and primary eidolon must be in your room. Arm for three rounds.
- **Exact effect:** On that ally's next positive hostile damage packet, after ordinary defenses, absorb up to **20 remaining damage** and deduct that amount directly from the primary eidolon's HP as a resource cost. Trigger only if the eidolon has **more HP than the amount**. If not, wait for another packet until expiry. Consume the arm before applying the reduction.
- **Limits:** No redirected damage packet, Life Link/Life Bond chain, proc, lifesteal, critical multiplication or self/group farming. Cannot protect you, another companion, or an ally in another room. One intercession arm per owner and one intercession reduction per packet; strongest eligible arm wins. The trigger uses no additional action.

### S07 — Epic Conjuration Command

- **Source/status:** Synthesis inspired by PF [Summoner's Call](https://aonprd.com/FeatDisplay.aspx?ItemName=Summoner%27s%20Call); new feat. **Priority:** second wave.
- **Prerequisites:** Summoner 20, CHA 21, Augment Summoning.
- **Activation:** `epic conjuration-command <own summon>`; swift, **1/30s**. One normal conjured creature in your room; no eidolon or mirror.
- **Exact effect:** Its next ordinary weapon/natural attack roll within two rounds gains **+4 morale to hit**. If that attack hits, the primary physical damage packet gains **+8 flat damage**, not multiplied on a critical. Miss or hit consumes the order.
- **Limits:** Grants neither an immediate attack nor an action reset. Does not affect spell attacks, each claw separately, AoE, extra damage packets or subsequent attacks. Highest morale attack bonus applies. Existing Beast Commander and stat bonuses retain their own rules; this is a short, one-attack command.

### S08 — Epic Adaptive Aspect

- **Source/status:** PF epic promotion of [Aspect and evolution surge](https://aonprd.com/ClassDisplay.aspx?ItemName=Summoner); new feat. **Priority:** later, temporary evolution accounting required.
- **Prerequisites:** Summoner 23, Aspect class feature, trained Spellcraft 20.
- **Activation:** `epic adaptive-aspect <current aspect> <replacement>`; full round, out of combat, **1/300s**; duration ten rounds.
- **Exact effect:** Temporarily replace one evolution currently assigned to **your own Aspect** with **gills, swim, scent or flight**. The replacement's native point cost cannot exceed the replaced choice's cost; all native prerequisites and form restrictions apply. Restore the original choice at expiry.
- **Limits:** No extra evolution points, Aspect slots, eidolon edits, natural attacks, size/stat changes or bonus spellcasting. Cannot change while another temporary aspect replacement is active. Dismissing an eidolon does not extend the timer. Native movement legality still applies when temporary flight/swimming expires.
- **Overlap:** Adds tactical swapping rather than reproducing Aspect Master or Evolution Surge Enhancement's extra budget.

### S09 — Epic Planar Triage

- **Source/status:** Epic promotion of PF [rejuvenate/restore eidolon support](https://legacy.aonprd.com/advancedPlayersGuide/baseClasses/summoner.html); new feat. **Priority:** first wave.
- **Prerequisites:** Summoner 20, sixth-circle Summoner access, CHA 21.
- **Activation:** `epic planar-triage`; standard, expend **one available sixth-circle Summoner slot**, **1/300s**. Primary eidolon must be alive, injured and in your room.
- **Exact effect:** Heal the primary for **min(100, floor(0.20 × its max HP))**, minimum 1 before the normal healing pipeline. It receives no stat change or temporary HP. Native healing penalties may reduce the result.
- **Limits:** No use on a mirror, purchased pet, other player's eidolon or dismissed/dead creature. No resurrection, lost-slot refund, cooldown reset or extra heal from Life Bond. If native healing suppression rejects all healing before commitment, reject the command without spending a slot.
- **Overlap:** This is an emergency slot conversion, not additional Eternal Bond fast healing.

### S10 — Epic Evolved Summons

- **Source/status:** PF epic promotion of [Evolved Summoned Monster](https://aonprd.com/FeatDisplay.aspx?ItemName=Evolved%20Summoned%20Monster); new feat. **Priority:** second wave.
- **Prerequisites:** Summoner 23, Improved Augment Summoning, trained Spellcraft 23.
- **Exact effect:** Choose one doctrine at Study: **Aquatic** or **Watchful**. Newly created normal conjured creatures receive that doctrine for their original remaining summon lifetime. Aquatic grants native natural-swimmer status and underwater breathing; Watchful grants **+4 competence Perception** and native scent detection.
- **Limits/actions:** Passive. No extra attacks, spellcasting, bonus follower slot, additional summon, increased duration or swimming through boat-only exits. Do not grant a second copy of an ability already possessed. Existing stronger competence Perception wins. Choose once, changed only through a normal feat respec.
- **Persistence:** Store provenance and original summon expiry, so pet restore can reapply the doctrine without creating fresh lifetime or benefits for purchased pets. This uses a restricted doctrine rather than an unrestricted evolution editor on every summoned mob.

## 7. General epic proposals

### G01 — Epic Fortitude

- **Source/status:** [ELH/SRD adaptation](https://www.d20srd.org/srd/epic/feats.htm#epicFortitude); new feat. **Priority:** first wave.
- **Prerequisites:** CON 21, Great Fortitude.
- **Exact effect:** Passive **+4 feat bonus to Fortitude saves**. This adds to Great Fortitude but not another copy of Epic Fortitude. Does not increase the game's separately typed Poison or Death saves unless the actual incoming check uses Fortitude. No action, resource, duration or repeat ranks.
- **Integration:** Save computation; make the separate-save distinction visible in feat info.

### G02 — Epic Reflexes

- **Source/status:** [ELH/SRD adaptation](https://www.d20srd.org/srd/epic/feats.htm#epicReflexes); new feat. **Priority:** first wave.
- **Prerequisites:** DEX 21, Lightning Reflexes.
- **Exact effect:** Passive **+4 feat bonus to Reflex saves**, adding to Lightning Reflexes. No automatic success, evasion, damage reduction or saving throw against an effect that normally has none. Once selectable; no action/resource/duration.

### G03 — Epic Will

- **Source/status:** [ELH/SRD adaptation](https://www.d20srd.org/srd/epic/feats.htm#epicWill); new feat. **Priority:** first wave.
- **Prerequisites:** WIS 21, Iron Will.
- **Exact effect:** Passive **+4 feat bonus to Will saves**, adding to Iron Will. No mind-affecting immunity and no bonus to separately typed Death saves unless they actually resolve as Will. Once selectable; no action/resource/duration.
- **Overlap:** P07 can combine only for its narrow categories because its bonus is insight, not another feat copy.

### G04 — Superior Initiative

- **Source/status:** [ELH/SRD adaptation](https://www.d20srd.org/srd/epic/feats.htm#superiorInitiative); new feat. **Priority:** first wave.
- **Prerequisites:** DEX 21, Improved Initiative.
- **Exact effect:** Increase Improved Initiative's contribution by **+4**, for **+8 total from that feat chain**. Other independently legal initiative modifiers retain their native stacking.
- **Limits/actions:** Passive. Does not reset existing combat timers, grant a free opening attack, clear surprise/flat-footed state or bypass action cooldowns. One rank.
- **Integration:** The actual initiative calculation rather than attack count or combat-event frequency.

### G05 — Legendary Athlete

- **Source/status:** Synthesis of ELH/SRD [Legendary Climber](https://www.d20srd.org/srd/epic/feats.htm#legendaryClimber) and [Legendary Leaper](https://www.d20srd.org/srd/epic/feats.htm#legendaryLeaper); new feat. **Priority:** first wave.
- **Prerequisites:** STR 19, CON 19, trained Athletics 20, Endurance.
- **Exact effect:** Gain **+6 competence** on native climbing and ordinary swimming skill checks. For normal successful movement involving a required climb/swim check, reduce final movement-point cost by **10**, retaining the normal cost floor.
- **Limits:** Passive. No automatic success, new exit, wall climbing, water breathing, flight or passage through boat-only sectors. No discount if mounted, flying over the obstacle, teleported, or already receiving a lower final cost from a non-stacking movement discount; apply the strongest flat movement discount.
- **Integration:** Existing room-based movement checks. Does not alter sailing or bypass blockers.

### G06 — Epic Trailmaster

- **Source/status:** [ELH/SRD adaptation](https://www.d20srd.org/srd/epic/feats.htm#legendaryTracker); new feat. **Priority:** second wave.
- **Prerequisites:** WIS 21, Track, trained Nature 23, trained Perception 20.
- **Exact effect:** Gain **+6 competence** to the native tracking check. While following an already recorded trail through a water or flying sector, ignore up to **10 points of its terrain-related tracking penalty**. Do not remove age, concealment or magical anti-tracking penalties.
- **Limits:** Passive. Does not fabricate a trail, reveal a hidden room/exit, report a remote player's live location or track through teleportation. Where the engine records no trail at all, it provides only the skill bonus. Existing trail visibility, age limits and builder restrictions remain.
- **Integration/overlap:** This deliberately uses a different name from the existing Inquisitor Legendary Tracker perk; it grants no extension to that perk’s tracking distance. Highest competence bonus applies. Add explicit terrain-penalty classification before implementing that second clause. This is a bounded room-trail adaptation, not omniscient tracking across surfaces.

### G07 — Epic Spell Opportunity

- **Source/status:** MUD redesign of [Spell Opportunity](https://www.d20srd.org/srd/epic/feats.htm#spellOpportunity); new feat. **Priority:** later, cast-completion hooks required.
- **Prerequisites:** Trained Spellcraft 23, Combat Reflexes, ability to cast third-circle ordinary spells.
- **Activation:** `epic spell-opportunity <enemy> <spell>`; swift, **1/60s**. Prepay one ordinary first- or second-circle spell slot from one eligible class; arm for three rounds.
- **Exact effect:** When that legal enemy completes an ordinary hostile spell in your room, after its full resolution, cast the reserved **magic missile, shocking grasp or ray of enfeeblement** at it once, if you are alive, able to cast, can still see it, know that spell in the paying class and it remains a legal target. Use normal CL, attack checks, resistance and saves, without metamagic.
- **Limits:** The triggering spell is not cancelled. No feat/proc echo can trigger another opportunity. Cannot use a power, item, epic spell or AoE. Interrupted/suppressed/disqualified reactions expire without refund. Reserve is consumed at arming, not recreated by the reaction.

### G08 — Epic Spell Stowaway

- **Source/status:** MUD redesign of [Spell Stowaway](https://www.d20srd.org/srd/epic/feats.htm#spellStowaway); new feat. **Priority:** later, buff provenance required.
- **Prerequisites:** Trained Spellcraft 23, ability to cast fifth-circle spells.
- **Activation:** `epic stowaway <ally> <spell>`; swift, **1/300s**. Choose one visible grouped player ally and **blur, resist energy or fly**; arm for three rounds.
- **Exact effect:** When that ally manually casts the selected spell **on themselves** in your room, also apply it to you, at **half its resulting duration**, rounded down to at least one round. Use the original caster's CL and chosen energy option. The normal slot remains paid once by that caster.
- **Limits:** No enemy spell theft, haste/teleport, healing, temporary HP, summoned creature, epic ward, item, echo or propagation to another stowaway. One copied target, no copied cast procs. Normal stacking, suppression and dispelling apply. The arm is consumed only by the qualifying cast or its expiry; invalid remote casts do not trigger it.

### G09 — Tenacious Wards

- **Source/status:** MUD redesign of [Tenacious Magic](https://www.d20srd.org/srd/epic/feats.htm#tenaciousMagic); new feat. **Priority:** first wave.
- **Prerequisites:** Trained Spellcraft 23, ability to cast fifth-circle spells.
- **Exact effect:** Your manually cast **mage armor, shield, barkskin, blur, fly, resist energy and protection from energy** gain **+4 effective CL solely when a hostile dispel check tries to remove them**. Preserve ordinary duration, damage absorption, spell circle and other CL scaling.
- **Limits/actions:** Passive. It raises the native dispel DC; it is not immunity and does not restore a successfully removed buff. Antimagic, expiry, voluntary removal, scripts and source extraction retain their normal effects. No bonus to epic wards or item-cast buffs.
- **Integration:** Store the original caster/feat snapshot on eligible affects; do not use the target's feats or let the benefit follow an unrelated recast.

### G10 — Improved Spell Capacity

- **Source/status:** MUD redesign of [Improved Spell Capacity](https://www.d20srd.org/srd/epic/feats.htm#improvedSpellCapacity); new feat. **Priority:** first wave.
- **Prerequisites:** Trained Spellcraft 20; maximum normal spell circle already unlocked in the chosen casting class. For Summoner this is circle six, not nine.
- **Exact effect:** Choose one casting class at purchase. Gain **one additional maximum slot at that class's highest normal circle**. Learn/prepare spells through existing rules; this feat grants no spell knowledge, immediate charged slot or circle above the class's normal maximum.
- **Ranks/actions:** Two ranks, second at total level 27; both must choose the same class. Passive. Never adds a tenth circle, grants an epic spell, or increases PSP. Normal replenishment fills the extra capacity.
- **Overlap:** Existing New Arcana and slot bonuses remain separate and capped by their own rules.

### G11 — Epic Dodge — rehabilitate existing disabled feat

- **Source/status:** Redesigned [ELH/SRD Epic Dodge](https://www.d20srd.org/srd/epic/feats.htm#epicDodge). **Reuse `FEAT_EPIC_DODGE`; do not create a second ID. Priority:** second wave.
- **Prerequisites:** DEX 23, Dodge, Mobility, trained Acrobatics 23.
- **Exact effect:** Once per **30 seconds**, automatically cause the first otherwise successful **noncritical physical weapon/natural attack** against you to miss. The reaction occurs before its physical and attached elemental on-hit packets are applied. Spend the use before resolving attack callbacks.
- **Limits/actions:** Passive. Must be standing, conscious, able to move and neither grappled nor entangled. Does not prevent critical hits, spells, rays, AoE, already-running damage, traps or scripted unavoidable attacks. Does not grant a counterattack. Existing concealment/evasion/deflection resolves first; a hit already avoided spends no Epic Dodge use.
- **Integration:** Enable only after auditing the current disabled entry and adding one once-per-attack interception point.

### G12 — Epic Heroic Recovery

- **Source/status:** PF epic promotion of [Heroic Recovery](https://aonprd.com/FeatDisplay.aspx?ItemName=Heroic%20Recovery); new feat, no mythic tier. **Priority:** later, source save metadata required.
- **Prerequisites:** CON 21, Endurance, Diehard, trained Discipline 20.
- **Activation:** `epic recover <effect>`; standard, **1/300s**. You must be able to act.
- **Exact effect:** Choose one current poison, disease, sickened or fatigued effect whose original application allowed **Fortitude or the native Poison save**. Attempt that original save type against its stored original DC, with a **+4 circumstance bonus**. Success removes the selected condition component; failure leaves it unchanged. Either consumes the use.
- **Limits:** No reroll against instantaneous damage, effects without a save, effects lacking reliable stored DC, sleep, paralysis, stun, death or an affliction's unrelated other components. Does not restore ability damage, HP or lost resources. No automatic cure and no recursive use of other recovery feats on this retry.

## 8. Additional class proposals

### O01 — Epic Inspiration

- **Source/status:** [ELH/SRD Epic Inspiration](https://www.d20srd.org/srd/epic/feats.htm#epicInspiration), restricted to native performances; new. **Priority:** second wave.
- **Prerequisites:** Bard 20, CHA 23, Bardic Music; trained Perform 20.
- **Exact effect:** Increase **Dance of Protection’s AC and Will-save contributions** and **Song of Heroism’s attack-roll and damage-roll contributions** by **+1 per rank**. Modify each original contribution, retaining its native inherent type, targeting and duration. These four numeric components are the complete whitelist.
- **Ranks/actions:** Two ranks, second requires total level 27 and Bard 23. Passive; at most +2 to an eligible contribution. Duplicate singers still use normal highest-bonus rules.
- **MUD boundary:** The tabletop morale/competence/dodge wording is replaced deliberately because these native songs use inherent bonuses. No extra hit dice, ability scores, DR, haste, healing, spell damage, duration, skill success chance or hostile-song DC. Song of Heroism’s existing haste bit is unchanged.

### O02 — Lasting Inspiration

- **Source/status:** [ELH/SRD Lasting Inspiration](https://www.d20srd.org/srd/epic/feats.htm#lastingInspiration), replacing a tabletop duration multiplier with a short real-time tail; new. **Priority:** later, performance provenance required.
- **Prerequisites:** Bard 23, CHA 23, Bardic Music; trained Perform 23.
- **Exact effect:** When you voluntarily stop performing, existing **AC and Will-save components of Dance of Protection**, and **attack-roll and damage-roll components of Song of Heroism** remain on their current legal same-room group recipients for **three additional rounds beyond the expiry they would otherwise have at that stop**. Apply once per performance instance, not on every pulse. Track that baseline expiry without this feat.
- **Ranks/actions:** One rank, passive. Starting the same performance again replaces its tail with the normal active effect, never adds another copy.
- **Limits:** No tail on silence, incapacitation, death, extraction, lost eligibility, dispelling or forced cancellation. Recipients leaving the singer's room/group lose the tail. No retained flight, revelation, healing pulses, ability-score changes, max-HP effects, hostile effects or resource generation. Existing duration perks modify the baseline only once.

### O03 — Positive Energy Bulwark

- **Source/status:** [ELH/SRD Positive Energy Aura](https://www.d20srd.org/srd/epic/feats.htm#positiveEnergyAura), redesigned as a limited channel effect; new. **Priority:** second wave.
- **Prerequisites:** Cleric 20 or Paladin 20, CHA 21, Channel Energy, native permission to channel positive energy.
- **Activation:** `epic positive-bulwark`; standard, **1/300s**, plus **one available native Channel Energy use**. Three-round duration.
- **Exact effect:** At commitment and six and twelve seconds later, deal **2d6 positive-energy damage** to each legal hostile undead in your room, using the native channel-undead damage path. Each pulse allows **Will half, DC 10 + floor(qualifying class level / 2) + CHAmod**. Snapshot the qualifying class and modifier at activation; if both classes qualify choose the higher class level, not their sum.
- **Limits:** No living targets, healing pulses, automatic turning/destruction, negative levels or channel-use refunds. Recheck PvP, room and target legality each pulse. Aura ends on death, unconsciousness or extraction, cannot overlap itself, and generates no secondary channel procs. Do not inherit damage increases from unrelated active Channel Energy uses.

### O04 — Great Smiting — rehabilitate existing disabled feat

- **Source/status:** [ELH/SRD Great Smiting](https://www.d20srd.org/srd/epic/feats.htm#greatSmiting); reuse `FEAT_GREAT_SMITING`, replacing its unbounded repeated-rank description. **Priority:** second wave.
- **Prerequisites:** Paladin 20 or Blackguard 20, CHA 23, the corresponding Smite Evil/Smite Good feature.
- **Activation:** `epic great-smite`; swift, **1/300s**, arm for three rounds. The next native manual smite attack consumes its ordinary smite use and normal action as well as this already committed arm.
- **Exact effect:** If that attack hits a target eligible for the chosen native smite, deal **+L physical damage**, where L is the class level supplying that smite. Apply as a separate packet subject to normal physical defenses, never critical multiplication.
- **Ranks/limits:** One rank. Miss, expiry, wrong-alignment target after commitment or loss of the smite arm gives no refund. No extra smite use, immunity bypass, alignment conversion or bonus on every later attack against a marked target. Audit existing callers of this ID before enabling it so its old per-rank damage cannot also apply.

### O05 — Ruinous Rage

- **Source/status:** [ELH/SRD Ruinous Rage](https://www.d20srd.org/srd/epic/feats.htm#ruinousRage), translating object-hardness destruction into bounded physical DR penetration; new. **Priority:** first wave.
- **Prerequisites:** Berserker 20, STR 23, Greater Rage, Power Attack.
- **Exact effect:** While native rage is active, your own manual melee weapon attacks ignore **up to five points of the target's applicable physical damage reduction**. Effective DR cannot become negative. If several DR layers could apply, use the native selected layer; do not subtract five from every layer.
- **Ranks/actions:** One rank, passive. Multiple flat DR-penetration features use the largest applicable value, not their sum.
- **Limits:** No AC reduction, energy resistance penetration, spell or companion benefit, immunity bypass, extra attack, or permission to destroy protected scenery/doors. Does not change rage cost, exhaustion, duration or legality.

### O06 — Epic Anatomical Savant

- **Source/status:** PF epic promotion of [Anatomical Savant](https://aonprd.com/FeatDisplay.aspx?ItemName=Anatomical%20Savant); new. **Priority:** later, immunity classification required.
- **Prerequisites:** Rogue 20 or Assassin 8, native sneak attack of at least 8d6, DEX 23, Weapon Focus with the chosen weapon type. Total level 21 still applies.
- **Exact effect:** Choose one Weapon Focus weapon type at Study. On your first otherwise valid sneak attack hit with that weapon against a target whose **ordinary anatomical immunity** would cancel sneak damage, roll **d100: 01–50 allows the usual sneak damage; 51–100 cancels it normally**. Consume the opportunity on that qualifying hit, success or failure; recover after **six seconds**.
- **Ranks/actions:** One rank, passive; owner timer shared by all targets, hands and attacks. Normal concealment, positioning and sneak eligibility checks must pass first. This feat does not itself enable sneak attack.
- **Limits:** Never bypass named boss/plot immunity, critical immunity as such, damage immunity, or a separate fortification roll. No additional chance reduction to fortification, critical multiplier or precision dice. If a second feature offers the same anatomical bypass, use the highest single success chance, not multiple rolls. Expose explicit ordinary-versus-hard immunity metadata before enabling it.

### O07 — Legendary Companion: Elemental Bond

- **Source/status:** Synthesis of [ELH/SRD Magical Beast Companion](https://www.d20srd.org/srd/epic/feats.htm#magicalBeastCompanion) and PF companion customization; new, without a new creature roster. **Priority:** second wave.
- **Prerequisites:** Druid 20 or Ranger 20, Animal Companion, WIS 21, trained Nature/Survival 20.
- **Exact effect:** Permanently choose **acid, cold, electricity or fire** at Study. Your single active native animal companion's first successful normal melee attack each six-second interval deals an additional **1d6 of that energy**. Snapshot the type from the owner's feat, not a rerolled summon template.
- **Ranks/actions:** One rank, passive. Owner-level six-second timer persists across pet dismissal, replacement and restoration. No extra attack, follower slot, new companion type or rider on spell/proc/retaliation attacks.
- **Overlap:** Existing Ranger/Druid companion HP, armor, speed and attack perks remain separate; this adds only the typed packet. Secondary companions, eidolons, crafted golems and charmed pets receive no benefit. All normal energy defenses apply; this does not make the creature a magical beast for targeting or immunities.

### O08 — Epic Augmented Alchemy

- **Source/status:** Synthesis of [ELH/SRD Augmented Alchemy](https://www.d20srd.org/srd/epic/feats.htm#augmentedAlchemy) and PF [Alchemist bombs](https://legacy.aonprd.com/advancedPlayersGuide/baseClasses/alchemist.html), using native prepared bomb slots; new. **Priority:** second wave.
- **Prerequisites:** Alchemist 20, INT 23, native bomb feature; trained Craft (Alchemy) 20.
- **Activation:** `epic augment-bomb <prepared bomb slot> <sacrificial bomb slot>`; swift, **1/60s**. Both slots must contain prepared **normal fire bombs** and be distinct. Consume the sacrificial bomb immediately and bind the other slot for three rounds.
- **Exact effect:** Your next manual throw of the bound bomb uses its normal action, attack and consumption rules. On a successful direct hit, add **4d6 fire damage** to the primary target only. Use the bomb's normal direct-hit save outcome, if any, for the extra packet. A miss still consumes the bomb and feat use.
- **Limits/overlap:** Splash, sticky ticks, chain reactions and secondary targets gain nothing. Expiry discards the bonus, not the bound bomb; the sacrificed bomb is never restored. Discarding/changing the bound slot cancels the arm. Existing bomb perks apply to the original attack, not a second proc chain. No bonus to healing/control bombs, extra prepared slots or exception to native bomb-making recovery. Slot identity and prepaid sacrifice require atomic saved state.

## 9. Suggested release order and implementation requirements

The class sections are the primary additions: **28 of 48 proposals** serve Psionicist, Artificer and Summoner. Register each in its named class's epic selection list and the general epic purchase path when prerequisites permit; do not spend both point types. O01–O08 belong to their named class lists. G01–G12 belong to the general list, with class lists added only where appropriate to existing class policy. No new feat points are proposed.

| Release group | Candidates | Reason |
| --- | --- | --- |
| First batch: direct extensions | P01, P03, P04, P07, P08; A01–A04, A06; S01, S02, S05; G01–G05, G10; O05 | Establish meaningful choices with relatively local hooks. P01 still needs correct known-power selection and P03 needs a full cost-order audit. |
| Second batch: explicit activations and companion handling | P06, P09; A05, A07, A08; S03, S04, S07, S09, S10; G06; O01, O03, O04, O07, O08 | Requires reliable target identity, proc exclusions, temporary effects and resource commitment. S03 needs the post-Life-Link lethal-hit hook. |
| Later: reaction/provenance infrastructure | P05, P10; S06, S08; G07–G09, G11, G12; O02, O06 | Needs safe delayed/reaction dispatch, original save metadata, buff provenance or immunity classification. |
| Completed audit | P02 | Existing +6 is resistance penetration; ordinary discipline DC is independently capped at +2. |

These groups are implementation priorities, not claims that the first group needs no testing. Entry-level notes take precedence over the broad grouping. For example, G06 remains second wave until terrain penalties can be classified.

### Required infrastructure

- **Study:** Show the full selected feat's benefit, prerequisites, ranks, cost, action, resource cost, cooldown and stacking restrictions before accept/reject. Revalidate prerequisites and points at acceptance. Multi-choice feats require their discipline, element, item category, class, weapon or spell subchoice before final confirmation. Unsupported or disabled features must not consume points.
- **IDs and help:** Allocate new stable IDs after checking the feat table and save layout. Reuse the disabled Epic Dodge and Great Smiting IDs and replace stale descriptions/callers. Add help entries with the same exact mechanics as Study; regenerate any feat indexes. This catalogue itself does not update live help or enable feats.
- **Actions/resources:** Centralize validate → commit → resolve. Prepaid charge/bomb consumption and delayed effects must be saved atomically before callbacks can retrigger them. Reserve target/object identities safely; do not retain freed pointers. Resolve automatic reactions after the triggering operation finishes, with recursion guards.
- **Timing:** Use absolute seconds for cooldowns and explicit six-second durations. The bard source contains a legacy display comment describing fifteen rounds as a minute; these proposals require eighteen seconds for a three-round tail. Verify actual event scheduling and display instead of inheriting that conversion silently.
- **Provenance:** Store effect source, original owner/class level, spell identity, applied components, original save type/DC and expiry where required. Never infer an original DC from the target's current level. S03 needs a single finalized lethal-hit event after native avoidance/Life Link; O06 needs distinct ordinary anatomical immunity and protected immunity.
- **Companions/devices:** Recompute permanent effects without repeated stacking, restore crafted golem identity before eligibility checks, and keep all proc/cooldown state on the player. Device recompile, transfer, destruction and save/load must preserve native charges/condition and cancel stale arms.
- **Economy:** A02 requires explicit recipe categories and a shared time-discount cap. No proposal changes material yields, sell prices, salvage gold or permanent item bonus ceilings. Do not replace the game's recipes with tabletop gold/XP construction prices.

### Acceptance checks for each implemented batch

1. Purchase with general and eligible class epic points, exact prerequisites, lost prerequisites, rank limits and all Study subchoices. Confirm rejected selections spend nothing and preview matches help/runtime.
2. Check insufficient action/resources, invalid targets, interruption, miss/save/resistance, cancellation and expiry. Each committed use is charged exactly once.
3. Test boundary values: level 21/30, resource floors, full HP, one HP above golem payment, discount caps, rank two, several targets/rays and multiple same-round attacks.
4. Test combinations with listed native perks, metamagic, augmentation, Life Link/Life Bond, Twin Eidolon, Fortress Engine and other survival features. Ensure one original hit cannot recursively trigger a damage/healing chain.
5. Save/reload, reconnect, pet dismissal/restore, device transfer/recompile and respec while cooldowns or arms exist. No reset, duplicated resources or permanent stat accumulation.
6. Verify PvP, peaceful rooms, visibility, group changes and legal movement/blockers. Test suppression, dispelling and protected immunities where relevant.

### Balance review after a trial period

Measure feat selection rates, damage and healing per use, sustained resource efficiency, reaction frequency, boss control uptime and companion survivability. In particular, review the existing psionic DC/damage feats alongside P02, S01's evolution budget alongside all pool perks, A03's charge economy, O06's precision uptime and G07's reaction damage. Change the published numeric rules and help together if tuning is needed; avoid hidden cooldown or proc-rate changes.

## Implementation status

The [first batch](EPIC_FEAT_FIRST_BATCH.md) and [second batch](EPIC_FEAT_SECOND_BATCH.md) are implemented in this working tree. Later and balance-held candidates remain proposals. The batch notes describe native adaptations and deployment requirements.
