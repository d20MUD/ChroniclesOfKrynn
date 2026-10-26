# Legendary Objects: Dragonmetal Awakening — Full Implementation Proposal

Status: proposed specification for review, **not implemented**.
Revision: 2026-10-23. All numbers below are explicit proposed launch values.
They are design decisions, not claims that the current economy has been measured
or that these abilities already exist. Implement this specification only after
approval; changes following playtesting should be versioned and documented.

## 1. Goals and scope

Give level-30 characters permanent, item-specific progression using normally
earned XP, gold and boss materials. Support weapons, arms, legs, head, body,
about, shield, held, feet and hands. Provide varied tactical choices and modest
existing bonuses without another +10 damroll or uncapped defensive stacking.

World-building requirements: one smith NPC in one existing forge, existing
eligible equipment, and a centrally configured list of existing bosses. No
mandatory new zones, dungeon rooms, equipment prototypes or reagent objects.

The Keeper of the Silver Forge establishes a dragonmetal lattice within gear.
True dragonlances receive additional powers through a dedicated registered
item identity. An ordinary infused weapon does not become a true dragonlance.

## 2. Fixed rules

- Player characters must be exactly level 30 to purchase upgrades. NPCs and
  immortals cannot earn production progression; staff testing uses separate
  test data and never generates tradable rewards.
- Gear must be permanent equipment, have item level **20–30**, and fit a supported
  slot. Exclude consumables, containers, ammunition, temporary/conjured objects,
  extraction-on-logout objects, transformation equipment and items scheduled to
  expire. Held equipment must be a permanent equipment type, not a held scroll,
  wand, staff or potion. Existing permanent magical staves wielded as weapons
  qualify as weapons rather than charged casting items.
- Existing restrictions and level requirements remain in force. Infusion neither
  lowers an item's required level nor changes its ordinary stats or prototype.
- Initial awakening binds the object instance permanently to the purchasing
  character. No sale, auction, stall listing, gift, inheritance or account-alt
  transfer. Ordinary owner storage is allowed.
- **No awakening transfer, extraction, salvage refund or recovery of investment.**
  Replacement gear starts from zero. Reshaping choices affects the same item only.
- Item ranks and all currencies persist across death, reconnect and reboot.
  Death does not remove infusion XP or materials. No offline passive earnings.
- Ordinary bonuses and legendary bonuses continue to obey existing stat caps.
  This system does not raise an existing cap or grant a class feat automatically.

## 3. Contribution channels and two-handed parity

A normal supported item has one channel. An actually two-handed weapon can
purchase a second, supporting channel on the same object instance.

| Loadout | Channels |
| --- | --- |
| One-handed weapon and shield | Weapon channel plus shield channel. |
| One-handed weapon and held focus | Weapon channel plus held channel. |
| Two wielded one-handed weapons | Two weapon channels. |
| One weapon wielded in both hands | Primary weapon channel plus supporting channel. |

The supporting channel selects a permanent family on awakening: **assault**
(weapon pool), **guard** (shield pool), or **focus** (held pool). Changing that
family uses the path/family reshape fee in section 7. These are legendary
abilities, not physical equipment: no ordinary shield AC, shield bash, focus
permissions or shield-only feats are granted.

Each channel has its **own rank, foundation, technique, signature, modifier,
path and paid investment**. Costs are identical to a normal item channel.
Advancing one does not unlock anything on the other. No common payment,
discount or credited purchase requirement counts for both channels.

```text
Two-handed investment = primary cumulative cost + supporting cumulative cost
Sword-and-shield investment = sword cumulative cost + shield cumulative cost
```

The totals must match exactly at matching ranks, separately for XP, gold,
fragments, essences and catalysts. Primary III/supporting I costs exactly the
same as sword III/shield I. Daily caps remain per character, not per channel.

The supporting channel functions only when the equipment code confirms that
both hands are occupied by that weapon. Switching to one-handed use makes it
dormant. An off-hand item and the supporting channel cannot contribute at the
same time. Weapon size or an empty off-hand alone does not establish eligibility.
Two-handed held nonweapons are outside this launch specification; they qualify
for one held channel only, and the UI explicitly states this limitation.

## 4. Progression and exact costs

Costs are **incremental per channel**, not cumulative. Every row must be paid
in full. No random upgrade rolls, failure chances or item destruction.

| Rank purchased | Earned infusion XP | Gold | Fragments | Essences | Catalysts | Unlock |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| I — Kindled | 100,000 | 2,500 | 25 | 0 | 0 | Path and one foundation. |
| II — Awakened | 250,000 | 5,000 | 50 | 10 | 0 | One technique. |
| III — Tempered | 500,000 | 10,000 | 100 | 20 | 1 | One signature. |
| IV — Exalted | 1,000,000 | 20,000 | 200 | 40 | 2 | One modifier on technique or signature. |
| V — Legendary | 2,000,000 | 40,000 | 325 | 70 | 3 | Final values for the selected technique and signature. |
| **Total I–V** | **3,850,000** | **77,500** | **700** | **140** | **6** | One completed channel. |

| Completed development | XP | Gold | Fragments | Essences | Catalysts |
| --- | ---: | ---: | ---: | ---: | ---: |
| One channel through III | 850,000 | 17,500 | 175 | 30 | 1 |
| Two channels through III | 1,700,000 | 35,000 | 350 | 60 | 2 |
| One channel through V | 3,850,000 | 77,500 | 700 | 140 | 6 |
| Greatsword primary V + supporting V | 7,700,000 | 155,000 | 1,400 | 280 | 12 |
| Sword V + shield V | 7,700,000 | 155,000 | 1,400 | 280 | 12 |
| Nine channels through V | 34,650,000 | 697,500 | 6,300 | 1,260 | 54 |

Nine channels represents seven pieces (arms, legs, head, body, about, feet and
hands) plus two hand contributions. This is capacity accounting, not permission
to equip an otherwise illegal combination.

Each essence payment is **half the path's associated essence, half any essence**.
The unrestricted half may use the same type. Thus rank II requires five matching
and five unrestricted essences; III requires 10+10; IV 20+20; V 35+35. Shared
utility powers do not change the selected path's material requirement.

### Earned infusion XP

- Credit **100% of eligible positive XP actually awarded after ordinary splits,
  modifiers and per-award caps**, while the character is level 30.
- Credit one character balance once, regardless of how many items are worn.
  Normal character XP behavior remains unchanged; spending infusion XP cannot
  lower a level. Historical XP is not converted retroactively.
- Eligible sources: normal solo/group PvE kill XP, trap-disarm XP, and normal
  automatic quest completion XP. A quest vnum credits infusion XP at most once
  per UTC day, including repeatable quests.
- Exclude generic scripted XP by default, crafting/reforging XP, damage-only XP,
  energy-drain XP, dump rewards, PvP, staff adjustments, refunds and transfers.
  A scripted quest must use the recognized quest-completion route to qualify.
- Summoned, charmed and player-owned victims never produce infusion XP or boss
  materials. Zero/negative awards do not credit it. Death does not debit it.
- No daily infusion-XP cap. Saved balance ceiling: **2,000,000,000**; overflow is
  discarded with a message. Do not change ordinary XP because this balance is full.
- Add explicit award provenance to the XP interface. Existing generic modes alone
  do not establish that a reward is legitimate. Hook credit after final award
  calculation and before any maximum-level advancement suppression.

### Upgrade purchase

At the smith, require the eligible item in the owner's inventory, no combat,
no pending infusion transaction, and all costs available. Show current rank,
new benefits, exact deductions and permanent binding before confirmation.
Quote expires after **60 seconds** or an inventory/currency/configuration change.
Confirmation revalidates everything. Commit the item and all deductions once or
not at all. Do not charge for inspection or menus. Spend carried gold; do not
automatically withdraw bank, auction or stall proceeds.

## 5. Boss materials, eligibility and daily limits

Three personal, bound ledger resources: dragonmetal fragments; typed resonant
essences; legendary catalysts. They are not item prototypes or account balances.
Six essence types: **draconic, martial, arcane, spiritual, primal, shadow**.

### Daily limits

| Resource | Per-character acquisition cap per UTC day |
| --- | ---: |
| Fragments | 100 |
| Essences | 20 across all six types combined |
| Catalysts | 1 |

Reset at **00:00 UTC**, using a saved date and counters. Balances carry over;
unused daily allowances do not. Spending, death, relogging, storage and reboot
never replenish allowance. Each currency balance is capped at **2,000,000,000**.
Counters measure acquisitions, not current holdings.

All sources use one award function. Clamp boss rewards separately to the
remaining daily allowance and balance capacity; report blocked amounts, with no
queued overflow or next-day delivery. The daily allowance is consumed only by
amounts actually credited. Acquisition through exchanges also consumes allowance.

### Boss reward table

| Configured boss tier | Fragments per eligible character | Typed essences | Catalyst |
| --- | ---: | ---: | --- |
| I — lesser endgame boss | 10 | 0 | None. |
| II — major endgame boss | 20 | 0 | None. |
| III — pinnacle boss | 30 | 0 | 1, if that day's catalyst allowance remains. |

**Superseded by the flag-based implementation:** boss eligibility is now the
explicit native `MOB_LEGENDARY_BOSS` flag, rather than a prototype-vnum registry.
Its tier derives from native level and its successful award rolls one essence
type before the result is durably recorded. Never infer eligibility from name,
race, level or XP alone. Reject summoned/player-owned/charmed instances even if
they reuse a flagged prototype. No encounter credit from forced staff kills or
test instances.

A character can receive rewards from a given boss prototype **once per rolling
20 hours**. Different clones of that prototype share the cooldown. Track death
instance/reward identifiers to prevent duplicate processing. A death while all
three allowances are exhausted grants nothing and does not start a new boss
cooldown; if any material is awarded, the full 20-hour cooldown starts.

### Participation

Eligible characters must be level 30, alive and in the boss's room at death,
have joined combat or performed a qualifying group-support action, and be in
the killer's same-room group (or be the solo credited killer). Record activity
against this encounter within the last **120 seconds**. Require at least
**three qualifying actions in the final 30 seconds**, so one opening hit is
insufficient while ordinary multi-attack combat counts naturally.

Qualifying actions: damaging the boss; effectively healing a participating ally;
actually absorbing/intercepting boss damage for an ally; applying a successful
hostile control/debuff to the boss; or restoring a missing eligible defensive
buff on an engaged ally. Fully overhealed, fully resisted, duplicate already-
active buff and zero-effect actions do not count. Support requires the ally to
be participating in this encounter. Boss-owned adds do not count as the boss.
A character's controlled pet damage can count as the owner's action, never as
an additional recipient.

Do not require a percentage of group damage; that penalizes tanks and healers.
Participation data expires **180 seconds** after the last encounter activity.
No material mail for absent, dead or linkdead characters; linkdead characters
are ineligible at death. These conditions require ordinary play, not automated
presence detection alone.

### Material exchanges

At the smith, outside combat:

- **3 essences of one type → 1 essence of a chosen different type.**
- **100 fragments + 20 essences of any mix → 1 catalyst.**
- No reverse conversion and no gold-to-material or XP-to-material purchase.

Each output consumes its category's daily allowance, even though its inputs
were acquired on an earlier day. Never refund previous acquisition counters.
Check full output capacity before consuming inputs; reject instead of partially
exchanging. Catalyst exchange offers an alternative to pinnacle bosses, at a
substantial additional cost. Costs in section 4 exclude optional exchange losses.

### Minimum material acquisition time

With no starting stock and no exchange losses, one rank-V channel needs at
least **seven daily allowances** (fragments and essences); two need **14**;
nine need **63**. These are numbers of daily allowance windows, not guaranteed
elapsed 24-hour periods or playtime estimates. Gold, XP, boss availability,
matching essences and per-boss cooldowns can extend progression.

## 6. Paths and choice pools

Paths determine flavor, matching essence and access to their listed powers.
They grant no additional automatic passive, feat, damage or ability use.

| Path | Essence | Path-only power choices |
| --- | --- | --- |
| Dragonbane | Draconic | T17, S10; registered dragonlances also access S11 and S12. |
| Guardian | Martial | T07, S01, S02. |
| Spellweaver | Arcane | T15, S07, S08. |
| Pursuer | Primal | T13, S09. |
| Lifebinder | Spiritual | T09, S05. |
| Trickster | Shadow | T18, S06. |
| Ironwill | Spiritual | T10, S04. |
| Wayfarer | Primal | T12, S03. |
| Runekeeper | Arcane | T16, S07. |
| Commander | Martial | T14, S02. |
| Slayer | Martial | T01, S09. |
| Renewal | Spiritual | T11, S05. |

An item's slot grants all slot-listed powers in section 10 without needing a
matching path. The path adds its own listed powers to that channel's menu,
regardless of slot, provided the character can meet the power's normal action
requirements. For example, a weapon attack still requires a wielded weapon,
including when its power comes from a head item. Path unlocks do not manufacture
weapons, shields, mounts, spells or resources.

All channels can choose any foundation in section 8. Each channel holds exactly
one foundation, one technique and one signature, unlocked at I, II and III.
Selection from a larger menu replaces the old selection; no duplicate identical
power selection on one channel. A repeated power on separate channels still
shares character-wide cooldown and stacking rules.

## 7. Selection, attunement and reshaping

### Active budgets

- **12 passive points per character.** Active foundations consume the costs in
  section 8. Foundations over budget remain dormant according to the player's
  saved priority order. There is no automatic replacement during combat.
- **Three major powers per character**, selected from all unlocked techniques
  and signatures. Any mixture is allowed. An upgraded power occupies one slot.
- Major selection and foundation priorities change only at the smith, outside
  combat. No gold/material fee merely to rearrange already selected powers.
- Equipment removal makes its contribution dormant. Re-equipping restores it
  only if the saved budgets permit it. No substitution of a fourth power when
  a selected one is unequipped.
- Character-wide cooldowns persist through death, logout, item changes and
  reshaping. Time-based cooldowns use saved UTC expiry timestamps.
- Character capability determines whether a power is usable. No extra spell
  slots, BAB, class levels, feats or permanently raised stat caps.

### Reshape fees per channel

| Change | Gold | Fragments | Other materials/XP |
| --- | ---: | ---: | --- |
| Replace foundation | 1,000 | 5 | None. |
| Replace technique | 1,500 | 10 | None. |
| Replace signature | 2,500 | 15 | None. |
| Change rank-IV modifier | 1,000 | 5 | None. |
| Change path or supporting family | 5,000 | 25 | 10 essences matching the new path; no XP. |

First selections when unlocked are free. A path/family change that invalidates
selections includes free replacement of those invalid selections; do not charge
a second reshape fee. Rank and paid progression remain on the same channel,
without refund. Both two-handed channels pay independently when both change.

## 8. Foundation catalog: exact passive bonuses

Each choice is one property, not a bundle. Values do not grow with rank.
Only active foundations contribute. The global passive budget and the category
caps both apply. All fractional calculations below round down.

| ID | Choice | Effect | Passive points | Character-wide legendary ceiling |
| --- | --- | --- | ---: | --- |
| F01 | Accuracy | +1 attack bonus on ordinary weapon attacks. | 3 | +1. |
| F02 | Force | +1 weapon damage per successful attack, including ranged; no spell or proc damage. | 3 | +2. |
| F03 | Warding | +1 deflection AC. Normal typed stacking applies. | 3 | +1. |
| F04 | Fortitude | +1 Fortitude save. | 2 | +1. |
| F05 | Reflex | +1 Reflex save. | 2 | +1. |
| F06 | Will | +1 Will save. | 2 | +1. |
| F07 | Aptitude | +1 STR, DEX, CON, INT, WIS or CHA, selected on purchase. Ordinary caps apply. | 3 | +1 per ability; at most two distinct abilities. |
| F08 | Expertise | +2 to one selected trained skill; excludes crafting progression ranks and custom/non-roll abilities. | 2 | +2 per skill; at most three distinct skills. |
| F09 | Vitality | +20 maximum HP. Removal cannot heal or grant current HP. | 2 | +40 maximum HP. |
| F10 | Endurance | +40 maximum movement. Removal cannot restore movement. | 1 | +80 maximum movement. |
| F11 | Reservoir | +20 maximum spell/psionic points in the character's actual resource pool. No resource or spell access if none exists. | 2 | +20 maximum points. |
| F12 | Elemental weave | Resistance 3 percentage points against one of acid, cold, electricity, fire or sonic. | 2 | +3 percentage points per type, two types maximum; effective resistance cannot be raised above 75% by this property. |
| F13 | Sure footing | +2 on actual checks resisting trip or forced movement. No check means no automatic immunity. | 1 | +2. |
| F14 | Steady grip | +2 on actual checks resisting disarm. | 1 | +2. |
| F15 | Trailwise | Reduce ordinary walking movement expenditure by 10%, minimum cost 1. Applies after normal reductions. | 1 | 10%; no mounted/teleport/resource refund. |
| F16 | Recovery | +5 HP on a normal regeneration tick while out of combat. | 1 | +5 per tick; no change to tick frequency. |

F01–F08 are infusion contributions to existing calculations, with normal typed
stacking wherever a type already exists. They are not separate uncontrolled
object-apply entries. F02 is flat damage, not a percentage multiplier. F08's
skill is chosen explicitly; the UI lists only supported real skill checks.
Repeated same-choice values use the strongest contribution except F02, F09 and
F10, whose explicitly listed ceilings allow two copies. F07/F08/F12 count their
selected subtype when determining duplication.

These are conservative contributions rather than independent paths to every
maximum. For example, F01 + two F02 + F03 costs all 12 points and yields only
+1 attack, +2 damage and +1 typed AC, leaving no other active foundations.

## 9. Timing, activation and shared rules

- **One round = six seconds** for this system, matching the current combat pulse.
  Durations and cooldowns are stored in seconds rather than assuming a game hour.
- All techniques and signatures are manually activated. There are no autonomous
  procs in the launch version. A power can prepare a conditional response, as
  explicitly described below. This keeps effects understandable and limits spam.
- Activation consumes one combat action: **six seconds of command recovery**.
  It cannot bypass an existing longer recovery or grant another attack/cast.
  It does not cancel normal weapon autoattacks unless the ordinary action rules
  require it. Apply normal incapacitation and command restrictions.
- A successful activation starts both its own cooldown and a **six-second shared
  legendary activation cooldown**. Invalid target or failed preconditions cost
  nothing. A valid use resisted by a target still consumes cooldowns and recovery.
- Protection powers share an additional **60-second family cooldown**; restorative
  powers share **60 seconds**; control powers share **30 seconds**. These apply
  across techniques/signatures and equivalent copies. Individual longer
  cooldowns still apply. Families are listed in the catalogs.
- Self or one visible, willing, same-room group ally is an eligible friendly
  target where listed. No remote rooms, offline targets or non-group players.
  The owner can be affected by their own group powers.
- Hostile effects require a visible same-room legal enemy; obey PvP, peaceful
  room, staff immunity and normal targeting rules. No new PvP exceptions.
- Control saving throw DC is **25**, using the named save, never increased by
  stats, items, spell DC modifiers or rank. Protected bosses remain immune to
  hard control. A resisted or immune control instead applies the specified
  fallback if present; no unlisted fallback exists.
- Temporary modifiers never stack with an identical legendary effect; strongest
  magnitude/longest remaining duration applies, not addition. Different effects
  are also subject to the total damage/prevention ceilings below.
- Major powers on all of a character's gear share an **offensive bonus budget of
  60 additional damage per rolling 60 seconds**, excluding F02. Explicit bonus
  damage is noncritical, nonrecursive and cannot receive damage multipliers.
- Legendary absorption/reduction on a recipient is limited to **100 total damage
  prevented per rolling 60 seconds**, across every source character. Normal
  class/spell protection is separate. Apply legendary reduction last, to remaining
  damage after ordinary defenses, and never convert damage to healing.
- Legendary direct healing/temporary HP on a recipient is limited to **100 total
  points per rolling 60 seconds**, across all sources. Effective healing counts;
  temporary HP counts when granted. Temporary HP uses normal nonstacking rules.
- No resurrection, death prevention, extra turns, automatic crits, resource
  multiplication, unconstrained spell duplication, room teleport or encounter
  cooldown resets in this launch.
- A persistent prepared effect ends on unequip, grip invalidation, death or
  logout of its source. Timed bonuses/wards cast on allies also end if their
  source channel becomes inactive. Already resolved damage/healing is not undone.
  Leaving the source room ends group wards. Pursuit marks end when the owner
  changes rooms; the target may move into an adjacent room for T13 tracking.
  Saved cooldowns
  remain. Re-equipping does not restore expired or removed effects.

## 10. Slot menus

Each slot has four foundations highlighted as recommendations; **all foundations
remain available**. Four techniques and three signatures are available before
path additions. Supporting two-handed families use their corresponding row.

| Slot | Suggested foundations | Techniques | Signatures |
| --- | --- | --- | --- |
| Weapon | F01, F02, F14, F08 | T01, T02, T03, T04 | S09, S10, S13 |
| Arms | F13, F14, F08, F06 | T02, T05, T06, T07 | S01, S02, S13 |
| Legs | F10, F13, F15, F05 | T05, T06, T08, T12 | S02, S03, S06 |
| Head | F06, F08, F11, F04 | T04, T10, T14, T16 | S04, S07, S08 |
| Body | F03, F09, F04, F12 | T05, T07, T09, T11 | S01, S04, S05 |
| About | F05, F08, F12, F15 | T06, T10, T12, T18 | S02, S06, S10 |
| Shield | F03, F13, F04, F12 | T05, T06, T07, T17 | S01, S02, S10 |
| Held | F11, F08, F06, F16 | T09, T14, T15, T16 | S05, S07, S08 |
| Feet | F10, F13, F15, F05 | T06, T08, T12, T13 | S03, S06, S09 |
| Hands | F08, F14, F04, F16 | T02, T03, T09, T11 | S05, S08, S13 |

### Technique catalog

Base values unlock at II. The V column replaces only the named base number(s),
not cooldown, range or action cost. Unmentioned values remain unchanged.
A prepared effect triggers at most once, then ends. Durations are real seconds.

| ID / name | Exact base effect | Rank V | Cooldown | Family |
| --- | --- | --- | --- | --- |
| T01 Measured strike | Your next ordinary weapon attack within 12 seconds gains +2 attack; on a hit add 10 damage. Miss consumes it. | +3 attack; 15 damage. | 90s | Offense |
| T02 Steady hands | Self gains +3 to opposed trip, disarm and shove checks for 18 seconds; does not add attack or damage. | +4 for 24s. | 90s | Utility |
| T03 Opening | Your next weapon hit within 12 seconds marks the foe for 12 seconds. The next ordinary attack by another group member consumes the mark for +2 attack. One mark per target. | +3 attack. | 90s | Offense |
| T04 Assess | Display target's currently effective fire/cold/acid/electricity/sonic resistances and whether it is hard-control immune. No hidden scripts, gear or vulnerabilities invented. | Also display physical resistance. | 60s | Utility |
| T05 Brace | Self absorbs 20% of the next physical hit, maximum 30 damage, within 12 seconds. | Maximum 40. | 90s | Protection |
| T06 Hold ground | Self gains +4 to checks against forced movement and trip for 18 seconds. No immunity to effects without a check. | +6 for 24s. | 90s | Protection |
| T07 Interpose | Prepare for 12 seconds to redirect 20% of the next physical hit on one ally to yourself, maximum 30. Redirected damage bypasses your additional reductions, cannot recurse, and cannot reduce you below 1 HP; excess stays on the ally. Recipient prevention budget applies. | Maximum 40. | 120s | Protection |
| T08 Recover footing | Remove a supported trip/knockdown condition from self and restore standing if ordinarily allowed; cannot act while unconscious, stunned, paralyzed or otherwise action-locked. No recovery bypass. | Also +2 against trip for 12s. | 120s | Utility |
| T09 Field dressing | Heal self or one ally for 30 HP, capped by missing HP. | 40 HP. | 120s | Restorative |
| T10 Resolve | Remove one fear effect with 12s or less remaining; longer fear instead loses 12s. Permanent, scripted and boss-locked fear is unaffected. Must otherwise be able to act. | Threshold/reduction 18s. | 120s | Restorative |
| T11 Clean wound | Remove one supported nonmagical bleed, oldest first, and heal 10 HP. No poison/disease/curse removal. | Heal 20 HP. | 120s | Restorative |
| T12 Trail stance | For 30s, reduce ordinary walking movement cost by 20%, minimum 1; does not stack with F15. No teleport or mounted refund. | Duration 45s. | 180s | Utility |
| T13 Pursuit mark | Mark one enemy for 30s; show its direction only while it is in an immediately adjacent room connected by a traversable exit. No auto-movement or visibility bypass. | Duration 45s. | 120s | Utility |
| T14 Coordinate | One ally's next ordinary weapon attack or supported skill check within 12s gains +2, then consumes the effect. Excludes crafting, infusion, saving throws and spell DC. | +3. | 90s | Utility |
| T15 Failed weave | For 18s, prepare a refund for the next eligible spell/power that spends at least 20 points and fails concentration before producing any effect. Refund 20% of actual expenditure, maximum 10 points. Excludes scrolls, wands, free casts, resisted spells and cancelled casts. | Maximum 15. | 180s | Resource |
| T16 Focused reserve | For 18s, your next eligible spell/power costing at least 20 points costs 5 fewer, minimum final cost 1. No refund, metamagic discount multiplication or effect on item casts. | Reduction 8. | 180s | Resource |
| T17 Breath brace | Self absorbs 20% of the next dragon-breath damage packet within 18s, maximum 40. One packet, not every element or victim. | Maximum 50. | 120s | Protection |
| T18 Veiled retreat | Self gains +3 stealth for 18s. Does not grant invisibility, disengage, concealment or a free flee attempt. | +4 for 24s. | 120s | Utility |

Resource effects T15/T16/S07 share a character ceiling of **20 points saved or
restored per rolling 180 seconds**, counted on actual expenditure/refund and
limited to the real maximum resource pool. They cannot grant spell slots or
retain unused discounts after expiry.

### Signature catalog

Base values unlock at III. Rank V replaces the listed values. All retain the
same three-power budget, action cost, global cooldown and family restrictions.

| ID / name | Exact base effect | Rank V | Cooldown | Family |
| --- | --- | --- | --- | --- |
| S01 Bulwark | Self or one ally absorbs 20% of incoming physical damage for 18s, up to 60 total damage. Ends on pool exhaustion. | Pool 80. | 300s | Protection |
| S02 Formation ward | Up to three same-room group members including self gain +2 on checks resisting trip and forced movement for 18s. Targets must be in an actual formation; no row bypass. | +3 for 24s. | 300s | Protection |
| S03 Sure passage | Self and up to two allies reduce ordinary walking cost by 20% for 45s, minimum 1. Does not bypass doors, terrain access, travel restrictions or F15/T12 stacking. Ends for a recipient leaving the source's group; room departure alone does not end this travel effect. | Duration 60s. | 300s | Utility |
| S04 Mental bastion | Self or one ally gains +2 Will saves against fear, charm and confusion for 18s. No immunity or removal of existing effects. | +3 for 24s. | 300s | Protection |
| S05 Renewing touch | Self or one ally heals 20 HP immediately and 10 HP at 6, 12 and 18 seconds, total at most 50. No revival; later ticks end on source invalidation or recipient death/room departure. | Initial heal 30, total 60. | 300s | Restorative |
| S06 Decoy stance | Self gains +2 dodge AC against the next two ordinary weapon attacks within 18s. Each attack attempt consumes a charge even if it misses; ordinary dodge stacking applies. | Three attacks. | 300s | Protection |
| S07 Runic economy | For 24s, your next two eligible spells/powers costing at least 20 points each cost 5 fewer, minimum 1. Same exclusions as T16. | Discount 8 each. | 300s | Resource |
| S08 Shared focus | One ally gains +3 concentration checks for 18s, and 20 temporary HP for the same duration. | +4 and 30 temporary HP. | 300s | Protection |
| S09 Hunter's opening | Your next ordinary attack within 18s against one designated foe gains +2 attack and +20 damage. Miss consumes it. | +3 attack and +30 damage. | 300s | Offense |
| S10 Dragonward | Self or one ally absorbs 25% of the next dragon-breath damage packet within 18s, maximum 60. | Maximum 80. | 300s | Protection |
| S11 Grounding lance | Registered dragonlance only: next ordinary lance hit within 12s forces DC25 Fortitude. On failure prevent flee and forced positional/formation relocation for 6s; do not prevent attacks/casts. Hard-control-immune dragons instead take 15 bonus damage; save success has no extra effect. Applies only to dragons. | Immune fallback 25 damage; duration remains 6s. | 300s | Control |
| S12 Rider's defiance | Registered dragonlance, riding your own living dragon mount: rider and mount each absorb 20% of the next physical hit within 18s, maximum 30 each. No extra mount attack. | Maximum 40 each. | 300s | Protection |
| S13 Rallying strike | Your next ordinary weapon hit within 12s heals one ally designated at activation for 30 HP. Miss consumes the effect. Heal requires ally still visible, grouped and in the room. | Heal 40. | 300s | Restorative |

Only S11 and S12 require the dragonlance registry; S10 and T17 can be used by
ordinary Dragonbane equipment. A two-handed dragonlance can buy both channels
normally, with no special cost reduction. All anti-dragon checks use the real
NPC race/subtype classification, not keywords or description text.

### Effect accounting details

- Multi-component attacks resolve in their ordinary order. A one-packet ward
  affects the first eligible nonzero component, consumes itself, and cannot
  absorb additional components from that same attack.
- Percentage reduction rounds down before its per-effect and recipient budget
  limits. Zero prevented damage does not consume a one-hit ward; expiry still
  applies. Redirection counts the actual amount shifted against the recipient's
  prevention allowance, never more than the source can safely accept.
- No hostile effect creates a vulnerability or converts immunity into damage.
  T04 only reports actual current defenses. S11's immunity fallback still obeys
  ordinary damage immunity, offensive budget and legal-target rules.
- HP/movement/resource maximum increases do not fill their pools when equipped,
  activated or reloaded. Recalculate current values as the lesser of their prior
  value and the new maximum. Removing/re-equipping cannot refill a reserve.
- Explicit utility reductions such as F15, T12 and S03 use the strongest active
  legendary reduction, never multiplication. Other ordinary reductions retain
  their existing order.
- Group powers target self first, then up to the stated number of allies in a
  saved explicit target order (default group-list order). No random selection.
  Range, group and source validity are rechecked before each delayed effect.
- S11 prevents ordinary flee and positional relocation only; it does not alter
  scripted boss phase transitions, extraction, staff movement or mandatory
  encounter mechanics. Those always take precedence.

### Rank-IV modifier

Choose exactly one modifier on either the channel's Technique or Signature.
It is not an additional power and cannot be attached to both at once.

| Modifier | Exact effect | Restrictions |
| --- | --- | --- |
| Enduring | Add 6 seconds to its timed effect/preparation window. | Only a listed duration of at least 12s; excludes S05 and never extends S11's control duration. |
| Ready | Reduce the power's individual cooldown by 15%. | Round up to a whole second; global/family cooldowns do not change. |
| Distant aid | A self-only friendly power may target one eligible same-room ally instead. | Only T02, T05, T06, T08, T10, T11, T17, T18 and S06. No offensive/resource powers or mount requirement bypass. |

Ready is available on every power. Thus every channel has a valid IV choice.
Rank V applies its catalog improvements automatically; it grants no fourth
property, further flat-stat growth or increased active-power budget.

## 11. Player commands and smith workflow

All names below are proposed syntax. Register `infuse` without replacing an
existing unrelated command; resolve aliases and quoted multiword object names
through ordinary object targeting. No live command changes are part of this doc.

| Command | Behavior |
| --- | --- |
| `infuse` / `infuse help` | Show syntax, supported equipment, permanent investment warning and smith location. |
| `infuse inspect <item> [primary|supporting]` | Rank, owner, family, path, selected properties, activation state, cooldowns, paid totals and next costs. Read-only, usable anywhere. |
| `infuse materials` | Six essence balances, fragments/catalysts, today's counts/caps, reset countdown and infusion XP. |
| `infuse paths <item> [channel]` | Valid paths, matching essences and path-specific choices. |
| `infuse choices <item> [channel]` | Available foundation/technique/signature IDs with exact values, restrictions and modifier eligibility. |
| `infuse awaken <item> primary <path>` | Quote rank I and bind on confirmed payment. Ordinary one-channel items may omit `primary`. |
| `infuse awaken <item> supporting <family> <path>` | Quote rank I for a two-handed supporting channel; primary must already exist. |
| `infuse advance <item> [channel]` | Quote and purchase exactly the next rank, never skip ranks. |
| `infuse select <item> <channel> foundation <ID> [subtype]` | Choose foundation, including the ability, skill or damage type when required. |
| `infuse select <item> <channel> technique|signature <ID>` | Choose an unlocked power. First choice free; replacements quote reshape fee. |
| `infuse modify <item> <channel> technique|signature <modifier>` | Select/change the unlocked IV modifier with a quote if a fee applies. |
| `infuse reshape <item> <channel> path|family <choice>` | Quote permanent selection change on this same item; list invalidated choices. |
| `infuse loadout` | All equipped channels, passive priorities/costs, three major selections, dormant reasons and category totals/caps. |
| `infuse prioritize <item> <channel> <position>` | Set foundation priority order, starting at 1. Only at smith. |
| `infuse attune <item> <channel> technique|signature` | Select an unlocked major power if one of three slots is free. Only at smith. |
| `infuse unattune <item> <channel> technique|signature` | Remove a major selection at smith; does not reset cooldown. |
| `infuse exchange essence <from> <to> <quantity>` | Quantity is the desired whole output, consuming three input essences each. |
| `infuse exchange catalyst <quantity>` | Show and confirm exact mixed-essence inputs, fragments and output allowance. |
| `infuse confirm <quote-id>` | Execute the still-valid quote once. |
| `infuse cancel` | Discard a pending quote without charge. |
| `infuse use <power-ID> [target]` | Activate a selected power anywhere it is otherwise legal. Defaults to self for friendly powers; hostile powers default to current opponent. |

`infuse use` preserves the existing `invoke` staff command and is usable away
from the smith, subject to ordinary combat restrictions.

All purchases, selections, exchanges, priorities and attunement changes require
the smith. Inspect, help, choices, paths, materials and loadout do not. Changing
choices on an inventory item is permitted; benefits still require legal equip.

A rank purchase may leave a new choice unselected. This avoids a second paid
operation if a player disconnects during a menu. A newly unlocked selection
remains free until its first valid choice; a stored boolean records that fact.

### Example advancement preview

```text
infuse advance greatsword supporting

Item: a dragonmetal-latticed greatsword
Channel: supporting / guard / Guardian
Rank: II -> III
Unlock: choose one signature

                         Required       Available
Earned infusion XP        500,000         720,000
Gold                       10,000         145,000
Dragonmetal fragments         100             130
Martial essences                10              14
Any essences                    10              18 (after matching payment)
Legendary catalyst               1               1

The primary channel does not advance.
Investment remains permanently attached to this object.
Confirm within 60 seconds: infuse confirm 1842
```

When paying unrestricted essences, let the player choose the mix in the quote.
Default to spending the most abundant remaining types first, ties broken by
fixed essence-type ID. Always show the exact proposed deductions before confirm.

## 12. Persistence, identity and failure recovery

### Existing integration points observed

The current code has a common XP function in `limits.c`, ordinary combat bonus
calculations in `fight.c`, and saved object serialization in `objsave.c` using
MySQL records including `player_save_objs` and `house_data`. Combat pulse length
is six seconds. Existing object movement, shops, storage and serializer paths
must all be audited; adding fields only to one save location is insufficient.

### Proposed persistent records

Use InnoDB tables with numeric character IDs, never character names as the
ownership identity. Store UTC timestamps and nonnegative 64-bit currency/cost
fields, with the gameplay ceilings specified above.

| Table | Required data and uniqueness |
| --- | --- |
| `legendary_characters` | Character-ID primary key; infusion XP; fragments; catalysts; six essence balances; UTC allowance date and three acquisition counters; passive priority list; three major selections; version for optimistic concurrency. |
| `legendary_items` | Object-UUID primary key; owner ID; original prototype vnum; registered equipment family; active/deleted status. One ownership record per physical instance. |
| `legendary_channels` | Unique object UUID + channel index (0 primary, 1 supporting); rank; path; supporting family; foundation/subtype; technique; signature; modifier/target; first-selection flags; cumulative paid amounts and version. |
| `legendary_cooldowns` | Unique character ID + power/family key; UTC expiration. No cooldown keyed only to the item. |
| `legendary_boss_claims` | Unique character + boss-death ID; boss vnum; award amounts/date; last paid reward time indexed by character and boss prototype. |
| `legendary_quest_xp_claims` | Unique character ID + quest vnum + UTC date. Prevent repeated infusion XP credit that day. |
| `legendary_operations` | Unique operation/quote ID; owner, object/channel and versions; exact before/after costs and gold; committed/applied state; timestamps; audit/recovery payload. |

Record a stable generated **128-bit object UUID** in every supported saved-object
representation. The database channel records are authoritative; the object UUID
is a reference, not a license to clone the record. Use a serialization version
and backward-compatible optional tags. Unawakened legacy objects need no
legendary record until first infusion.

If two loaded objects claim one UUID, quarantine their legendary effects,
block purchases and log a staff diagnostic. Do not guess which copy is valid or
silently delete equipment. If a saved item references missing records, retain
its ordinary behavior, disable legendary effects and report the inconsistency.

### Atomicity and cross-save recovery

XP/material/channel changes and their operation row commit in one DB transaction
with locked character/item rows. **Ordinary gold and saved object locations may
use other save paths**; a DB transaction alone does not make those atomic.
Implementation must coordinate them through the persistent operation journal:

1. Serialize all economic mutations for the character while a purchase is pending;
   prevent spending, moving or selling the affected item during commit.
2. Validate and persist an operation with exact pre/post gold and progression
   versions. Commit the progression debit/upgrade and recoverable gold mutation
   together in the authoritative journal transaction.
3. Apply the recorded gold value and item reference to the ordinary save through
   an idempotent operation-ID checkpoint. Persist that checkpoint with the gold
   state, then mark the journal applied. Only then acknowledge success and unlock.
4. Before accepting player commands after reconnect or recovery, finish committed
   unapplied operations exactly once. Uncommitted operations deduct nothing.
   Never replay an old absolute gold value over subsequent legitimate purchases;
   block subsequent mutations until reconciliation completes.

No production launch until the actual character-gold persistence path supports
this checkpoint contract. A crash at each step must be tested. Do not claim
atomicity by performing unrelated saves consecutively.

Cooldowns and temporary-effect source IDs are recorded before applying an active
power. Restart ends temporary effects rather than recreating them; cooldowns
remain. Rolling budgets are persisted as bounded timestamped debit entries:
keep only the last 60s for damage/prevention/healing and last 180s for resources.
Restarts must not replenish budgets early. No healing or damage is replayed
from a recovery journal.

### Equipment transformations and ordinary systems

- Inventory, owner storage, housing chests, corpse recovery and mail-to-self
  recovery preserve the same UUID and records. All other ownership changes reject.
- Reforge, craft conversion, combining, splitting, destruction-for-materials and
  any operation replacing the object must reject awakened input before charging.
- Cosmetic rename/description edits may preserve awakening only on the same
  UUID with unchanged eligibility and equipment family.
- A permanently destroyed item loses its awakening investment. Keep a tombstone
  record to prevent UUID reuse; ordinary staff restoration is not a player refund
  or transfer service. Confirm deliberate player destruction explicitly.
- Existing gear stats, typed bonus stacking, formation rules, racial restrictions,
  proficiency, class requirements and mount rules all remain authoritative.
- Legendary mechanics read centralized active-channel functions rather than
  scanning arbitrary object-affect arrays independently in each attack handler.

## 13. Runtime implementation plan

Create `legendary.c`/`legendary.h` for eligibility, definitions, costs, ledgers,
loadouts, commands, transactions and effect dispatch. Separate static power
configuration from mutable instance data. Every catalog ID is stable; renaming
text does not renumber a saved selection.

Hook only well-defined events:

1. Final positive XP award with recognized source provenance.
2. Registered boss encounter actions and resolved boss death.
3. Equipment/grip changes and active-bonus recalculation.
4. Attack preparation, resolved hit/miss and final damage application.
5. Actual checks for supported saves, skills and combat maneuvers.
6. Resource payment and pre-effect concentration failure.
7. Normal healing, temporary HP, regeneration and movement expenditure.
8. Login/logout, extraction, object serialization and ownership changes.
9. Shop, auction, stall, crafting, reforging and storage acceptance checks.

No generic spell duplication engine is required. Previously suggested delayed
spells, unrestricted teleports, free coordinated attacks and death-prevention
powers are replaced by the bounded catalog in this specification. No unspecified
candidate power from earlier drafts is included implicitly.

Create one smith prototype and assign its special procedure through a central
configuration entry. The service checks the actual NPC/service identity, not its
name. A maintenance mode can disable purchases while leaving inspection and
existing equipment benefits available. A separate emergency switch disables
major powers without deleting records or resetting cooldowns.

### Required deployment data

- One smith mob vnum and existing forge room vnum, selected by staff.
- A reviewed explicit boss-vnum list, with tier and essence for every entry.
- A reviewed true-dragonlance prototype registry, independent of keywords.
- Supported skill and condition IDs mapped to real checks/effects, with fixtures.
- Database migration, serializer compatibility version and player-facing help.

These world identifiers cannot be responsibly invented in the proposal. The
mechanical rules and numbers are fixed here; choosing the actual existing
content records is a deployment input. Do not edit live zone/quest files as part
of preparing this design.

## 14. Acceptance tests and balance review

### Required correctness checks

- Verify each rank's incremental and cumulative costs, every mixed-rank pair,
  and exact two-handed/sword-and-shield parity for all five resources.
- Insufficient XP, gold or any material leaves every balance and rank unchanged.
  Duplicate confirmation and concurrent commands charge and grant once.
- Crash/reconnect at every purchase checkpoint preserves exactly one outcome.
- Daily caps hold across multiple bosses, partial awards, exchanges, UTC reset,
  repeated death processing, reboot and two characters on the same account.
- Boss cooldown is per prototype; participation credits support roles and rejects
  ineligible characters, pets, staff tests and low-effort tag attempts.
- Repeatable quest XP credits once per quest per UTC day. Crafted/reforged XP,
  staff XP and historical XP do not become infusion XP.
- All supported slots save/load, activate correctly and reject transfer. Both
  two-handed channels survive storage and grip changes without duplicate benefit.
- Passive budget, every category ceiling and three-power limit hold across nine
  channels, dual wielding, stat cap interaction and invalid equipment states.
- Each catalog power has tests for exact values at III/II and V, its IV modifier,
  action recovery, target legality, timing, caps, source removal and cooldown.
- No recursive redirect, critical multiplication of bonus damage, free resource
  generation, repeated healing replay or resistance beyond the specified ceiling.
- Explicit serializer round trips cover character, house/chest and every other
  saved-object path carrying awakened gear; duplicate UUIDs fail safely.
- Cost/cooldown inspection and messages match authoritative definitions rather
  than duplicating numbers in command text.

### Numerical playtest gates

Use actual representative level-30 builds and fixed seeds/encounters, comparing
identical equipment and actions with and without legendary effects:

- Over a **10-minute** stationary encounter, target no more than **5%** additional
  sustained damage from the entire system on each representative build.
- Over the same encounter, target no more than **8%** reduction in damage received
  from the entire system on each representative defensive build.
- Test at least **one two-handed martial, sword-and-shield martial, dual wielder,
  ranged attacker, offensive caster, psionic character and healer/support build**.
- Test short **30-second** fights separately: the 60-damage offensive budget is
  an absolute cap, not proof that burst damage is appropriate for every build.
- Record actual XP/hour and gold/hour from **three** representative endgame routes
  before approving prices for production. Costs here are concrete proposals;
  seven allowance windows does not prove seven evenings of play is enough.

If these gates fail, revise values before launch rather than promising a percent
improvement based solely on the design. Check how small attribute/save changes
cross existing thresholds; raw bonus size alone does not establish total impact.

Log per-character awards, blocked daily rewards, XP provenance, purchases,
reshapes, activation frequency, damage/prevention/healing/resource contribution,
and failure recovery. Publish player-facing balance changes and keep investments
intact when changing numbers. No rank increase beyond V; future development
adds choices under the same budgets rather than endless vertical scaling.

## 15. Source references

- [limits.c](../../src/limits.c): current `gain_exp` pipeline, modifiers and award caps.
- [mudlim.h](../../src/mudlim.h): current XP source modes requiring provenance refinement.
- [quest.c](../../src/quest.c): automatic quest completion and XP awards.
- [fight.c](../../src/fight.c): attack, damage, kill credit and group combat integration.
- [structs.h](../../src/structs.h): wear locations, equipment definitions and combat pulse.
- [utils.h](../../src/utils.h): current attribute cap and game-hour duration.
- [objsave.c](../../src/objsave.c): serialized object persistence and database save transactions.
- [player_shop.c](../../src/player_shop.c): player stall integration requiring ownership safeguards.

Phase-1 integration decisions, including action recovery, condition adapters and
checked gold receipts, are recorded in the [repository audit](LEGENDARY_OBJECT_PHASE_1_AUDIT.md).
Existing staff invocation remains unchanged.

This document replaces the earlier exploratory draft. All previously agreed
constraints remain: no transfer, earned XP and gold costs, daily personal
material limits, all requested slots, and exact two-handed investment parity.

## Additional item passive — owner-requested amendment

Each legendary item may now select one additional rank-scaling passive alongside
its foundation. This supersedes any earlier suggestion that foundations are the
only passive progression. Foundation bonuses and their 12-point allowance stay
unchanged. The new layer has sixteen choices, distinct wear-slot menus and
explicit outfit ceilings; complete rank I–V numbers and role examples are in
[Legendary item passives](LEGENDARY_ITEM_PASSIVES.md).

First selection is free; changing it uses the existing 1,000-gold/5-fragment
foundation reshape fee. A two-handed weapon has one shared selection with each
independently paid primary/supporting track contributing at its own rank.
Selecting the primary effect does not supply unpaid supporting ranks. SQL schema
v6 adds `item_passive_id` on the primary channel only; existing items start with
no selection. Fresh native encounter measurements must include this layer.
