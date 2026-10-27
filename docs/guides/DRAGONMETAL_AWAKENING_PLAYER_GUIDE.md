# Dragonmetal Awakening: a player's guide

> Phase 14B implementation status: smith purchases, ownership enforcement,
> equipped budgets, all sixteen foundation bonuses and sixteen additional
> rank-scaling item passives are implemented. Ordinary
> caps and typed stacking apply; increasing a maximum never fills its pool.
> All eighteen techniques and thirteen signatures can be activated with
> `infuse use`. True-lance powers require an explicitly registered dragonlance;
> no true-lance prototype is registered in the initial release configuration.
> Use `infuse track` to query an active Pursuit mark without another activation.
> Rank-IV modifiers and rank-V upgrades are validated. Confirmation quotes
> show the modifier target and exact resulting power values. Existing cooldowns
> survive reshaping; invalidated choices receive free replacements.
> Security/recovery regressions now include real process interruptions, input
> boundaries, storage/commerce, corpse retrieval and character deletion.
> XP, gold and material purchase costs include the 20× difficulty increase. Native combat balance
> validation remains in interim phase 14B before release.
> Deployment requires schema v10 and Lord Orelor Silverveins (mobile 10500) as
> a smith with the Infusion Smith special procedure. XP/material earning, purchases and major
> powers are independently enabled in stages by the live release process.

**Preview for review: this system is not available until its staged release
controls are enabled.** Commands, prices and powers below describe the current
design. The selected smith and boss-flag rollout are documented in the phase-15
release package.

Awaken a favorite piece of equipment with dragonmetal, then improve it by
adventuring at level 30. You pay earned experience, gold and boss materials.
Your item gains a foundation, one additional rank-scaling item passive, and a choice of powers you activate when
you need them.

## Start here: the short version

Type `infuse` for the short prompt, then `infuse next <item>` for your immediate
next choice and its cost. Repeat `infuse next <item>` after each confirmation.
Use `infuse help` for the complete reference; you do not need to memorize it.

1. **Choose equipment you intend to keep.** Awakening binds it to you permanently.
   Its progress cannot move to a replacement item, and there are no refunds.
2. **Earn XP and gather materials.** Normal adventuring at level 30 builds an
   infusion XP balance. Eligible endgame bosses award personal materials.
3. **Visit Lord Orelor Silverveins in the Whitestone fieldhouse.** Carry your item in inventory and
   bring 2,000,000 infusion XP, 50,000 gold and 500 fragments to awaken it.
4. **Choose a path and a passive bonus.** The path adds power choices and decides
   which essences you need later. Your item slot already gives you several choices.
5. **Upgrade when ready.** Rank II unlocks a technique; III unlocks a signature.
   At the smith, select up to three powers across all your gear to use in play.
6. **Equip the item and use `infuse use`.** You must activate its powers yourself.

You do not need to develop a full outfit at once. Start with one item, one
passive bonus and one power. More ranks give you choices, not an endless pile
of extra commands to use in every fight.

## A first item, step by step

This example awakens a one-handed sword on the **Slayer** path and chooses
**Accuracy**, followed later by **Measured strike**. Replace `sword` with your
item's targeting keyword. Commands are proposed syntax.

At the smith, outside combat, with the unequipped sword in inventory:

```text
infuse inspect sword
infuse paths sword
infuse awaken sword primary slayer
```

The smith shows the exact price and a confirmation command. Enter the quote ID
actually shown to you, for example:

```text
infuse confirm 1842
infuse select sword primary foundation F01
```

Your sword is now rank I. Equip it to receive Accuracy's +1 attack bonus,
provided it fits your passive allowance and normal bonus caps.

After earning the next rank's costs, return to the smith with the sword in
inventory:

```text
infuse advance sword primary
```

Confirm the **new** quote using its displayed ID. Then choose and enable your
first power:

```text
infuse select sword primary technique T01
infuse attune sword primary technique
infuse loadout
```

Equip the sword again. During a fight, use:

```text
infuse use T01
```

Your next ordinary weapon attack within 12 seconds receives +2 attack and, if
it hits, +10 damage. A miss uses up the effect. Measured strike then has a
90-second cooldown. It is not an automatic bonus on every swing.

**You can stop here and keep using this item.** Later ranks are optional goals.

## Which equipment can I awaken?

You must be **level 30**. The item must be permanent equipment with an item level
of **20–30**, suitable for one of these categories:

**Weapon, arms, legs, head, body, about, shield, held, feet or hands.**

Consumables, ammunition, containers, temporary/conjured gear and items that
expire do not qualify. A permanent held focus can qualify; a scroll, potion,
charged wand or charged casting staff does not. A permanent staff wielded as a
weapon can qualify as a weapon. Rings, neck, waist and other unlisted slots are
not included in this version.

Your normal class, proficiency, item-level and equipment restrictions still
apply. Awakening does not turn an ordinary weapon into a true dragonlance.

**Choose carefully:** awakened items cannot be sold, given away, put in a player
stall, auctioned or passed to another character on your account. Owner storage
is allowed. You cannot reforge or convert the item into a different object.
Permanently destroying it also loses the investment. You may change its selected
bonuses at the smith for a fee, but may never move its awakening to another item.

## What each rank gives you

A **foundation** is a small passive bonus. A **technique** and a **signature** are
powers you activate yourself. A signature is a stronger option, not an extra
power slot. Each item chooses one of each.

| Rank | You gain |
| --- | --- |
| I — Kindled | Choose a path and one foundation. |
| II — Awakened | Choose one technique. |
| III — Tempered | Choose one signature. |
| IV — Exalted | Modify either your technique or your signature. |
| V — Legendary | Improve your technique and signature to their final listed values. |

Foundations do not grow with rank. The separate item passive does grow with rank; see [item passives](../systems/LEGENDARY_ITEM_PASSIVES.md) for every bonus and slot menu. Reaching V does not give you five stacking
stat bonuses, nor does it let you activate more powers at once.

## Costs

Each row is the price **for that next rank**, for one item progression track.
Pay every rank in order. Upgrading never randomly fails or destroys the item.

| Rank purchased | Earned infusion XP | Gold | Fragments | Essences | Catalysts | Unlock |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| I — Kindled | 2,000,000 | 50,000 | 500 | 0 | 0 | Path and one foundation. |
| II — Awakened | 5,000,000 | 100,000 | 1,000 | 200 | 0 | One technique. |
| III — Tempered | 10,000,000 | 200,000 | 2,000 | 400 | 20 | One signature. |
| IV — Exalted | 20,000,000 | 400,000 | 4,000 | 800 | 40 | One modifier on technique or signature. |
| V — Legendary | 40,000,000 | 800,000 | 6,500 | 1,400 | 60 | Final values for the selected technique and signature. |
| **Total I–V** | **77,000,000** | **1,550,000** | **14,000** | **2,800** | **120** | One completed channel. |

Half of each essence payment must match your path; the other half can be any
essences, including more of the matching type. For example, a Guardian rank-II
upgrade requires **100 martial essences plus 100 essences of any type**.
The smith shows which essences will be spent before you confirm.

The initial awakening and every advancement spend **infusion XP**, not your
character's level. Carry the gold you plan to spend; the smith does not withdraw
money from your bank, auctions or stall. Quotes expire after 60 seconds, so
check the price and use the confirmation command promptly.

## How to earn XP and materials

### Infusion experience

At level 30, eligible normal XP also credits your saved infusion XP balance,
point for point after the usual bonuses, group splits and award limits. You
can earn it before awakening your first item. Your old accumulated XP is not
converted when the system launches.

Normal PvE kills, trap disarming and automatic quest completions qualify.
The same quest only credits infusion XP **once per UTC day**. Crafting,
reforging, PvP, damage-only awards, energy drain and generic scripted rewards
do not qualify. Killing pets or summoned creatures does not qualify.

There is **no daily infusion-XP limit**. Spending it does not reduce your level.
Death does not remove this balance. It is personal to your character.

### Boss materials

Eligible endgame bosses award materials directly to your character's saved
balance. You do not need reagent inventory space or to win a shared loot roll.

| Boss tier | Fragments | Essences | Catalyst |
| --- | ---: | ---: | --- |
| Lesser endgame | 10 | 2 | None. |
| Major endgame | 20 | 4 | None. |
| Pinnacle | 30 | 6 | One, if your daily catalyst allowance remains. |

Each eligible boss death awards one randomly selected essence: draconic,
martial, arcane, spiritual, primal or shadow. Only mobiles marked
**Legendary-Boss** count; an ordinary high-level mob does not become a material
source simply because it is difficult. The essence is recorded with the award
before it is delivered, so reconnecting or retrying cannot change it.

You can exchange three essences of one type for one essence of another at the
smith if you need to complete a particular infusion.

You must be level 30, alive, connected and in the boss's room at death, either
as the credited solo killer or in the killer's same-room group. Participate
with at least **three useful actions in the final 30 seconds**. Every real
qualifying combat packet counts, so normal multi-attack combat qualifies without
artificial delays. Damage, effective healing, actual damage
protection, successful debuffs and useful defensive buffs can count. Overhealing,
reapplying an already-active buff or landing one opening hit is insufficient.
Your pet can contribute for you; it does not receive its own materials.

After receiving any materials from a boss, you cannot receive another material
reward from that same boss type for **20 hours**, even from another copy of it.
Fight different eligible bosses while that reward is on cooldown.

### Daily material limits

| Material | Maximum acquired per character each UTC day |
| --- | ---: |
| Fragments | 200 |
| Essences | 40 total across all types. |
| Catalysts | 2 |

Allowances reset at **midnight UTC**. Saved materials remain yours across days.
Unused allowance does not carry forward, and spending materials does not restore
it. Relogging, death and reboot do not reset it. Rewards above a remaining daily
allowance are reduced or blocked; they are not saved for tomorrow.

Characters on the same account have separate balances and allowances. Materials
cannot be traded between them. With no stock saved, a rank-V item requires at
least **70 daily allowance windows** of fragments and essences, plus its XP,
gold and other requirements. This is not a promise of 70 days of playtime.

Check everything with:

```text
infuse materials
```

The saved balance limit is two billion each for infusion XP and each material
balance. New credit beyond a full balance is discarded; normal character XP is
not changed because your infusion balance is full.

### Exchange materials at the smith

- Trade **three essences of one type for one of a different type**.
- Trade **100 fragments and 20 essences for one catalyst**.

The output counts toward today's acquisition limit. Spending the inputs does
not refund their earlier allowance. If today's output allowance is exhausted,
the exchange is refused without taking your materials. There is no purchase of
materials with gold or infusion XP.

## Why some bonuses are dormant

You can develop more equipment than you can fully activate at once. Your
character has two limits shared across the whole outfit:

| Limit | Meaning |
| --- | --- |
| **12 passive points** | Each active foundation costs 1–3 points. Pick a priority order at the smith; foundations over budget remain dormant. |
| **Three active powers** | Choose any three unlocked techniques/signatures across all equipped gear. |

For example, a weapon technique, shield signature and boots technique use all
three power slots. You may still have passive foundations on other equipment,
within your 12-point allowance and bonus caps.

You choose these at the smith. Rearranging already-unlocked active selections
is free. Selecting a different bonus on the item costs a reshape fee.
Unequipping a chosen item makes its effects dormant; it does not automatically
substitute another power. Re-equipping never resets cooldowns.

```text
infuse loadout
infuse prioritize sword primary 1
infuse attune sword primary technique
infuse unattune sword primary technique
```

You do not need to memorize the limits: `infuse loadout` should show the total,
what is active, and the reason each dormant choice is inactive.

## Two-handed weapons

A greatsword can develop two separate tracks, called **primary** and
**supporting channels**. Think of these as its weapon side and its off-hand
replacement.

The supporting channel can offer weapon-like (**assault**), shield-like
(**guard**) or held-focus-like (**focus**) choices. It grants legendary choices,
not a physical shield or its ordinary equipment bonuses.

```text
infuse awaken greatsword primary slayer
infuse awaken greatsword supporting guard guardian
infuse advance greatsword supporting
```

Each purchase requires its own confirmation and full costs. Upgrading primary
does not upgrade supporting. Fully developing both costs **154,000,000 infusion
XP, 3,100,000 gold, 28,000 fragments, 5,600 essences and 240 catalysts**—exactly the
same as fully developing a sword and shield.

Both tracks share your ordinary 12 passive points, three active powers and
daily material caps. The supporting track only works while the weapon actually
uses both hands. Switching to one hand makes it dormant. Two-handed held items
that are not weapons receive only one track in this version.

## Choose a path

Your item slot supplies its normal power menu. A path adds the following choices
and determines your matching essence type. It does not automatically grant any
of its powers: you still select a technique/signature, unlock the required rank
and enable it within your three-power allowance.

| Path | Essence to collect | Extra choices added to your item |
| --- | --- | --- |
| Dragonbane | Draconic | Breath brace (T17); Dragonward (S10); Grounding lance (S11) — true dragonlance only; Rider's defiance (S12) — true dragonlance only |
| Guardian | Martial | Interpose (T07); Bulwark (S01); Formation ward (S02) |
| Spellweaver | Arcane | Failed weave (T15); Runic economy (S07); Shared focus (S08) |
| Pursuer | Primal | Pursuit mark (T13); Hunter's opening (S09) |
| Lifebinder | Spiritual | Field dressing (T09); Renewing touch (S05) |
| Trickster | Shadow | Veiled retreat (T18); Decoy stance (S06) |
| Ironwill | Spiritual | Resolve (T10); Mental bastion (S04) |
| Wayfarer | Primal | Trail stance (T12); Sure passage (S03) |
| Runekeeper | Arcane | Focused reserve (T16); Runic economy (S07) |
| Commander | Martial | Coordinate (T14); Formation ward (S02) |
| Slayer | Martial | Measured strike (T01); Hunter's opening (S09) |
| Renewal | Spiritual | Clean wound (T11); Renewing touch (S05) |

Paths do not provide weapons, spells, mounts or class abilities you lack. A
weapon-attack power still needs a weapon, even if selected on headgear. A power
already available from your item slot remains available without its associated
path. Choose the path for its extra options and preferred materials.

## Foundation choices: passive bonuses

All supported slots can select any foundation below. Choose its ability, skill
or element when needed. Bonuses obey ordinary stacking rules and existing stat
caps; an existing stronger bonus may make a choice unhelpful. The ceilings are
for your **whole character's legendary equipment**, not per item.

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

Repeated identical choices generally do not add together. Force, Vitality and
Endurance allow two copies up to their stated ceilings. Aptitude, Expertise
and Elemental weave distinguish your selected ability, skill or element.
Maximum-resource bonuses do not fill the resource when equipped.

Example: Accuracy plus two Force foundations plus Warding uses all **12 passive
points**, for +1 attack, +2 damage and +1 deflection AC. It cannot become +10
damage by awakening more pieces.

## Power choices by item slot

The IDs make selection unambiguous; names are shown alongside them. Your path
adds its choices to these menus. Foundations are not restricted by this table.

| Item worn | Technique choices | Signature choices |
| --- | --- | --- |
| Weapon | Measured strike (T01); Steady hands (T02); Opening (T03); Assess (T04) | Hunter's opening (S09); Dragonward (S10); Rallying strike (S13) |
| Arms | Steady hands (T02); Brace (T05); Hold ground (T06); Interpose (T07) | Bulwark (S01); Formation ward (S02); Rallying strike (S13) |
| Legs | Brace (T05); Hold ground (T06); Recover footing (T08); Trail stance (T12) | Formation ward (S02); Sure passage (S03); Decoy stance (S06) |
| Head | Assess (T04); Resolve (T10); Coordinate (T14); Focused reserve (T16) | Mental bastion (S04); Runic economy (S07); Shared focus (S08) |
| Body | Brace (T05); Interpose (T07); Field dressing (T09); Clean wound (T11) | Bulwark (S01); Mental bastion (S04); Renewing touch (S05) |
| About | Hold ground (T06); Resolve (T10); Trail stance (T12); Veiled retreat (T18) | Formation ward (S02); Decoy stance (S06); Dragonward (S10) |
| Shield | Brace (T05); Hold ground (T06); Interpose (T07); Breath brace (T17) | Bulwark (S01); Formation ward (S02); Dragonward (S10) |
| Held | Field dressing (T09); Coordinate (T14); Failed weave (T15); Focused reserve (T16) | Renewing touch (S05); Runic economy (S07); Shared focus (S08) |
| Feet | Hold ground (T06); Recover footing (T08); Trail stance (T12); Pursuit mark (T13) | Sure passage (S03); Decoy stance (S06); Hunter's opening (S09) |
| Hands | Steady hands (T02); Opening (T03); Field dressing (T09); Clean wound (T11) | Renewing touch (S05); Shared focus (S08); Rallying strike (S13) |

## Techniques: your rank-II choices

These are activated powers. Base values apply from rank II; the rank-V column
replaces only the listed values when that item reaches V. `s` means real seconds.
A prepared next-hit effect is consumed once and expires if unused.

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

## Signatures: your rank-III choices

Signatures compete for the same three active slots as techniques; they do not
have a separate allowance. Base values apply from rank III. All signatures have
a **five-minute cooldown** before rank-IV modifiers.

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

Only Grounding lance and Rider's defiance require a recognized true dragonlance.
Infusing another spear does not qualify it. Breath brace and Dragonward are
available through the ordinary slot/path menus.

## Rank IV: change how one power works

Choose one modifier for either your item's technique or signature:

| Modifier | What it does |
| --- | --- |
| **Enduring** | Add six seconds to an eligible effect/preparation window of at least 12 seconds. Not available for Renewing touch; never extends Grounding lance's six-second control. |
| **Ready** | Reduce that power's individual cooldown by 15%, rounded up to a whole second. For example, 300 seconds becomes 255 seconds. Shared cooldowns remain unchanged. |
| **Distant aid** | Turn one eligible self-only friendly power into a power you may use on one same-room ally instead. It does not grant remote targeting. |

Distant aid is available for Steady hands, Brace, Hold ground, Recover footing,
Resolve, Clean wound, Breath brace, Veiled retreat and Decoy stance. You get
one modifier per channel, not one for each power. Ready is always an available
choice.

## Using powers in a fight

Enable a power at the smith, equip its item, then activate it by ID:

```text
infuse use T01
infuse use T09 companion
infuse use S09 enemy
```

Friendly powers default to yourself where their descriptions allow it. Hostile
powers default to your current opponent. Allies must normally be willing,
visible group members in the same room. Ordinary combat, movement and target
restrictions still apply.

Use `infuse use` for legendary powers; the existing `invoke` command continues
to activate staffs. Power use does not require visiting the smith.

Every activation gives **six seconds of command recovery** and a six-second
shared legendary cooldown. It does not grant a free attack or spell. You cannot
use powers while ordinary action restrictions prevent you from acting.

There are also shared cooldowns between related powers:

| Family | Shared cooldown |
| --- | --- |
| Protection | 60 seconds. |
| Restorative | 60 seconds. |
| Control | 30 seconds. |

For example, using Brace also delays Bulwark and Dragonward, even if their own
cooldowns are ready. Their longer individual cooldowns still apply. An invalid
target does not spend a use; an enemy resisting a valid use does.

These character-wide limits prevent combinations from growing out of control:

- At most **60 extra damage per rolling 60 seconds** from major powers; Force's
  ordinary +1/+2 damage is separate. Bonus power damage cannot critically multiply.
- At most **100 damage prevented per recipient per rolling 60 seconds** by
  legendary absorption/reduction, including protection from multiple players.
- At most **100 healing/temporary HP per recipient per rolling 60 seconds** from
  legendary powers, including other players' powers. Ordinary healing is separate.
- At most **20 resource points saved/refunded per character per rolling 180
  seconds** through Failed weave, Focused reserve and Runic economy combined.

Identical temporary effects do not stack. These caps can reduce a listed power's
actual benefit if some allowance has already been used. Ordinary immunities and
boss protections still apply. Grounding lance uses **DC 25 Fortitude** and
cannot stop scripted boss phases. Its listed immunity fallback is its only
alternative effect.

Unequipping an item, changing grip so its supporting track stops working, death
or logout ends its prepared effects and ongoing sourced benefits. Most ally
wards also end when the recipient leaves the source's room; Sure passage is the
travel exception described above. Pursuit mark ends if you change rooms, but can
show a marked foe in a traversable adjacent room. None of these events resets
a cooldown. Restart does not refresh powers.

## Changing your mind

You can change an item's choices while keeping its ranks. First choices are
free when unlocked. Later replacements cost:

| Change on one channel | Gold | Fragments | Essences |
| --- | ---: | ---: | --- |
| Foundation | 20,000 | 100 | None. |
| Technique | 30,000 | 200 | None. |
| Signature | 50,000 | 300 | None. |
| Rank-IV modifier | 20,000 | 100 | None. |
| Path or supporting family | 100,000 | 500 | 200 matching the resulting path. |

Changing a path/family includes replacing choices it makes invalid; you are not
charged twice for those replacements. No reshape costs XP. Simply rearranging
which existing choices are active is free at the smith.

**Reshaping never transfers the awakening to another item and never refunds your
investment.**

## Command reference

Most players can start with inspect, awaken, advance, select, attune and use.
The other commands help when planning or changing a build.

| Command | Use |
| --- | --- |
| `infuse help` | Show help and the smith's location once assigned. |
| `infuse inspect <item> [channel]` | Show rank, choices, status, cooldowns and next costs. |
| `infuse materials` | Show saved balances and daily allowances. |
| `infuse paths <item> [channel]` | Show available paths. |
| `infuse choices <item> [channel]` | Show valid choices with their numbers and restrictions. |
| `infuse awaken <item> primary <path>` | Begin awakening the item; confirmation required. |
| `infuse awaken <item> supporting <family> <path>` | Begin a two-handed weapon's second track. |
| `infuse advance <item> [channel]` | Purchase the next rank; confirmation required. |
| `infuse select <item> <channel> foundation <ID> [subtype]` | Choose a passive bonus and any required ability/skill/element. |
| `infuse select <item> <channel> technique <ID>` | Choose a technique. Use `signature` instead for a signature. |
| `infuse modify <item> <channel> technique <modifier>` | Modify the technique at IV; use `signature` to modify that instead. |
| `infuse reshape <item> <channel> path <choice>` | Change path; use `family` for a supporting family's change. |
| `infuse loadout` | Explain active/dormant choices and your allowances. |
| `infuse prioritize <item> <channel> <position>` | Set passive priority, starting at 1. |
| `infuse attune <item> <channel> technique` | Enable the technique; use `signature` for that choice. |
| `infuse unattune <item> <channel> technique` | Free its active slot; use `signature` as appropriate. |
| `infuse exchange essence <from> <to> <quantity>` | Buy that many output essences at three input essences each. |
| `infuse exchange catalyst <quantity>` | Quote catalyst exchange and exact material inputs. |
| `infuse confirm <quote-id>` | Confirm the displayed purchase or paid change. |
| `infuse mix <draconic> <martial> <arcane> <spiritual> <primal> <shadow>` | Replace a pending item quote with an exact essence payment mix; confirm the new ID. |
| `infuse cancel` | Cancel the pending quote without charge. |
| `infuse use <power-ID> [target]` | Use an enabled power while wearing its item. |

Use `primary` for ordinary equipment; `supporting` only exists on a qualifying
two-handed weapon. The ordinary single-track forms of awaken/advance can omit
the channel where the command permits it. Purchases, choice changes, exchanges,
priorities and attunement require the smith and being out of combat. Inspection,
menus, materials and loadout can be viewed elsewhere.

For design details, see the [full implementation proposal](../systems/LEGENDARY_OBJECT_SYSTEM_PROPOSAL.md).

## Additional rank-scaling item passive

Every legendary item may choose one extra passive alongside its foundation.
Use `infuse choices <item> primary`, then
`infuse select <item> primary passive Pxx` and confirm the quote at a smith.
The first choice is free; changing it costs 20,000 gold and 100 fragments.
Choices vary by wear slot and include toughness, resource capacity, recovery,
travel, trained skills and resistance to maneuvers or saving throws.

See [the complete passive list and slot menus](../systems/LEGENDARY_ITEM_PASSIVES.md)
for rank I–V values, outfit ceilings and class/role examples. A two-handed
weapon has one selection; each paid track contributes at its own rank with the
same ceilings as matching one-handed equipment. Foundations retain their
12-point allowance; this additional passive uses no foundation points or
active-power slots.
