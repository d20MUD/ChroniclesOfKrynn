# Dragonmetal Awakening: Your Guide to Legendary Items

Turn a favorite piece of equipment into a lasting part of your character's story. Dragonmetal Awakening lets level-30 adventurers strengthen their gear through five ranks, choosing bonuses and powers to suit the way they play.

You supply infusion experience, gold and materials earned from eligible bosses. You choose which item to develop and how far to take it. Each upgrade has a fixed price, with no random chance of failure.

## Getting started

Your first goal is simple: choose one item you want to keep and awaken it to rank I.

You will need:

- A **level-30 character**.
- A suitable, permanent piece of **level 20–30 equipment**.
- **2,000,000 infusion XP**.
- **50,000 gold**, carried with you.
- **500 fragments**.

Visit **Lord Orelor Silverveins in the Whitestone fieldhouse** when you are ready. You must be conscious and out of combat to make changes at the smith.

Start with these commands, replacing `sword` with your item's keyword:

```text
infuse materials
infuse inspect sword
infuse next sword
```

`infuse materials` shows your savings and today's remaining material allowances. `infuse inspect` shows the item, while `infuse next` explains its next step. You can return to these whenever you need a reminder.

If the game reports that earning rewards, purchases or power use are unavailable, that part of the system has not been enabled yet. Check the game's announcements for availability.

## Choose an item you intend to keep

You can awaken weapons and equipment worn on the arms, legs, head, body, about the body, feet and hands, as well as shields and suitable held items.

Rings, neck items, belts and other unlisted equipment slots are not included. Consumables, ammunition, containers and temporary or conjured equipment do not qualify. A permanent held focus may qualify; scrolls, potions, charged wands and charged casting staffs do not. A permanent staff used as a weapon can qualify.

**Awakening permanently binds the item to your character.** You cannot sell it, trade it, give it to another character, drop it or reforge it into something else. This includes passing it to another character on your own account. Its awakening cannot be moved to a replacement item or refunded.

You can keep it in your carried bags or personal housing chest. Your normal equipment and class restrictions still apply.

## What your item gains

An awakened item brings together four kinds of benefits:

| Benefit | What it does |
| --- | --- |
| **Foundation** | A small bonus that works while the item is equipped and within your outfit's allowance. Examples include accuracy, health, defenses and travel benefits. |
| **Item passive** | An additional bonus that grows as the item gains ranks. Choices depend on the equipment slot. |
| **Technique** | A power you activate yourself, unlocked at rank II. |
| **Signature** | Another power choice, unlocked at rank III. |

The foundation and item passive are separate choices. Your foundation stays at its original strength; your item passive improves with rank.

Techniques and signatures require you to choose them for use and activate them during play. They do not happen automatically on every attack.

## Awaken your first item

Here is an example for a sword on the **Slayer** path. At the smith, look through the paths, then request awakening:

```text
infuse paths sword
infuse awaken sword primary slayer
```

The smith shows the price and a confirmation command. Check the details, then enter that command using the confirmation ID actually displayed:

```text
infuse confirm <quote-id>
```

Nothing is spent until you confirm. Quotes last **60 seconds**. Stay with the smith and avoid changing your inventory or equipment before confirming. If a quote expires, request a new one. Use `infuse cancel` to cancel it.

After awakening, view your choices:

```text
infuse choices sword primary
```

For this example, choose **Accuracy** as your foundation and **Vital thread** as your growing item passive:

```text
infuse select sword primary foundation F01
```

Confirm the displayed quote, then request the passive:

```text
infuse select sword primary passive P01
```

Confirm that quote as well. These first selections are free, but still require confirmation. Accuracy provides +1 to ordinary weapon attack rolls; Vital thread begins with +5 maximum health at rank I and reaches +25 at rank V.

Equip the sword to benefit from its bonuses. Increasing maximum health does not heal you or fill the extra space immediately.

Use `primary` for ordinary equipment choices. It simply identifies the item's main progression; two-handed weapons have an optional second progression described below.

## Grow through five ranks

You buy each rank in order. Each row below is the cost of that upgrade alone.

| Rank | What opens up | Infusion XP | Gold | Fragments | Essences | Catalysts |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| I — Kindled | Path, foundation and first item-passive choice | 2,000,000 | 50,000 | 500 | 0 | 0 |
| II — Awakened | Technique | 5,000,000 | 100,000 | 1,000 | 200 | 0 |
| III — Tempered | Signature | 10,000,000 | 200,000 | 2,000 | 400 | 20 |
| IV — Exalted | One change to how your technique or signature works | 20,000,000 | 400,000 | 4,000 | 800 | 40 |
| V — Legendary | Stronger technique and signature effects | 40,000,000 | 800,000 | 6,500 | 1,400 | 60 |
| **Total I–V** | **One completed progression** | **77,000,000** | **1,550,000** | **14,000** | **2,800** | **120** |

With no saved materials, one complete progression requires at least **70 daily allowance windows** for fragments and essences, plus its XP, gold and catalysts.

Your item passive also improves as you advance. You can stop at any rank and continue using the item.

At the smith, request the next rank with:

```text
infuse advance sword primary
```

Confirm its quote, then use `infuse next sword` to see what to choose next.

For rank upgrades, **half the required essences must match your path**. The rest can be any type, including more matching essences. For example, Slayer's rank-II upgrade needs 100 martial essences and 100 essences of any type. The quote shows the payment before you agree.

At rank IV, choose one compatible improvement for either your technique or your signature: **Enduring** extends eligible effects, **Ready** shortens the power's own cooldown, and **Distant aid** lets certain self-only powers target a nearby ally instead. The choices menu shows what your selected power supports.

## Earn infusion XP by adventuring

At level 30, qualifying experience also builds a separate **infusion XP** balance. You can start saving before awakening your first item.

Normal monster kills, trap disarming and automatic quest completions can qualify. The same quest contributes infusion XP only once per UTC day. Crafting, reforging, player combat and generic scripted rewards do not qualify, and old accumulated experience is not converted into infusion XP.

There is **no daily infusion-XP limit**. Spending it does not lower your level, and death does not remove your saved balance.

## Gather materials from bosses

Eligible legendary bosses award materials directly to your character. You do not need inventory space for them, and group members do not compete over a single material drop.

| Boss tier | Fragments | Essences | Catalysts |
| --- | ---: | ---: | ---: |
| Lesser endgame | 10 | 2 | 0 |
| Major endgame | 20 | 4 | 0 |
| Pinnacle | 30 | 6 | 1 |

Rewards depend on your remaining daily allowances. Each reward's essences are one randomly chosen type: **draconic, martial, arcane, spiritual, primal or shadow**. Only designated legendary bosses award these materials; a difficult ordinary enemy does not automatically qualify.

To receive a reward, you must be level 30, alive, connected and in the boss's room when it dies. You must be the credited solo killer or part of the killer's group in that room. Contribute at least **three useful actions during the final 30 seconds**. Normal attacks, effective healing and useful protection or weakening effects can count. Overhealing or repeatedly applying an effect that is already active does not count. Your pet can contribute on your behalf.

After receiving materials from a boss, that same boss type cannot reward you again for **20 hours**. Other eligible bosses can still reward you.

### Daily material allowances

Each character can acquire up to:

| Material | Daily allowance |
| --- | ---: |
| Fragments | 200 |
| Essences | 40 across all types combined |
| Catalysts | 2 |

Allowances reset at **midnight UTC**. Saved materials carry over; unused daily allowance does not. Spending materials does not reopen today's allowance. Rewards above the remaining allowance are reduced or lost rather than held for tomorrow.

Balances and allowances belong to each character. Materials cannot be traded or shared between characters.

### Exchange materials you do not need

At the smith, you can exchange:

- **Three essences of one type** for **one essence of another type**.
- **100 fragments and 20 essences** for **one catalyst**.

For example, this requests one martial essence in exchange for three arcane essences:

```text
infuse exchange essence arcane martial 1
```

Check and confirm the quote. Exchange results count toward the same daily allowances as boss rewards. If you have already used today's allowance for the output, the exchange is refused without taking your materials.

## Pick a path that suits your play

Your equipment slot supplies a starting menu of powers. Your path adds more choices and determines your matching essence type. It does not lock every item into the same role or automatically give you all its powers.

| Path | Matching essence | Added choices focus on |
| --- | --- | --- |
| Dragonbane | Draconic | Protection against dragons and breath attacks |
| Guardian | Martial | Protecting yourself and nearby allies |
| Spellweaver | Arcane | Managing magical resources and supporting allies |
| Pursuer | Primal | Tracking a foe and preparing an opening attack |
| Lifebinder | Spiritual | Healing and recovery |
| Trickster | Shadow | Retreat and misdirection |
| Ironwill | Spiritual | Resisting fear and mental threats |
| Wayfarer | Primal | Travel and helping companions move |
| Runekeeper | Arcane | Preserving magical resources |
| Commander | Martial | Coordinating attacks and group protection |
| Slayer | Martial | Preparing stronger weapon attacks |
| Renewal | Spiritual | Treating wounds and restoring health |

Use `infuse choices <item> primary` to read the available effects before choosing. Paths do not grant missing class abilities, spells or weapon training. Some Dragonbane powers require a true dragonlance; awakening an ordinary lance does not make it one.

## Choose bonuses that help your character

Foundations offer accuracy, weapon damage, armor protection, saving throws, abilities, trained skills, health, movement, magical reserves, elemental resistance and other benefits.

Item passives offer growing bonuses to health, movement, magical reserves, recovery, travel, trained skills, saving throws and resistance to being disarmed or knocked down. The available choices vary by slot.

A frontline fighter might choose extra health or stronger defenses. A caster might prefer magical reserves or Concentration. A scout might choose Perception, Stealth or cheaper walking. Read your item's menu to find the choices it supports.

Bonuses still obey normal limits. Skill bonuses require the skill to be trained, and extra magical capacity does not teach a noncaster to cast spells. Repeating the same bonus across many items eventually reaches an outfit-wide limit.

## Prepare and use your first power

At rank II, a Slayer sword can choose **Measured strike**:

```text
infuse select sword primary technique T01
```

Confirm the selection, then prepare it for use at the smith:

```text
infuse attune sword primary technique
infuse loadout
```

Equip the sword. When you want to use the power, enter:

```text
infuse use T01
```

Measured strike improves your next ordinary weapon attack within 12 seconds: +2 to the attack roll and +10 damage if it hits. A miss uses up the preparation. Its normal cooldown is 90 seconds. Activating it does not make an extra attack for you.

Other powers protect, heal, aid travel or help allies. Some accept a target, for example:

```text
infuse use T09 companion
```

You must have that power unlocked, selected and prepared, and its item equipped. Power use works away from the smith.

Every activation causes six seconds of recovery and a six-second shared legendary cooldown. Related protection, healing and control powers also share longer cooldowns. Equipping an item again, reconnecting or changing selections does not reset them.

## Plan your outfit

Your legendary equipment shares two main allowances:

- **12 foundation points.** Foundations cost between one and three points each. You can set their priority at the smith; foundations beyond the allowance remain inactive.
- **Three prepared powers.** Choose any combination of unlocked techniques and signatures across your equipment. A signature uses one of these three places too.

The growing item passive uses neither foundation points nor a prepared-power place, but still has its own outfit-wide bonus limits.

Use `infuse loadout` to see what is working and why another choice is inactive. An item must be equipped in its proper place. Removing a selected item does not automatically prepare a different power. During combat, removing gear also does not immediately activate foundations that were outside your allowance.

Choosing which existing powers to prepare and rearranging foundation priorities is free at the smith.

## Two-handed weapons

A two-handed weapon can develop an optional second progression called **supporting**. Choose **assault** for weapon-style choices, **guard** for shield-style choices, or **focus** for held-focus-style choices.

The supporting progression has its own five ranks and pays the full costs separately. Upgrading the main progression does not upgrade the supporting one. Both still share your character's foundation allowance and three prepared powers.

The weapon has one growing item-passive choice, made on its primary progression. Each paid progression contributes to that same passive at its own rank, within the usual outfit limit.

Supporting benefits require the weapon to be wielded with both hands and the off hand to be free. Switching to one hand or equipping a shield or held item makes them inactive. Shield-style choices do not give you a physical shield or its ordinary bonuses.

## Change your choices later

You can change choices at the smith while keeping your purchased ranks. First selections are free; later replacements cost:

| Change | Gold | Fragments | Essences |
| --- | ---: | ---: | --- |
| Foundation or growing item passive | 20,000 | 100 | None |
| Technique | 30,000 | 200 | None |
| Signature | 50,000 | 300 | None |
| Rank-IV improvement | 20,000 | 100 | None |
| Path or supporting style | 100,000 | 500 | 200 matching the resulting path |

Changes cost no infusion XP. If a path or supporting-style change makes an existing selection unavailable, you can replace that affected selection for free. Changing your choices never moves the awakening to another item or refunds earlier spending.

## Commands to keep handy

| Command | What it tells you |
| --- | --- |
| `infuse next <item>` | Your next step and its cost |
| `infuse materials` | Saved infusion XP, materials and daily allowances |
| `infuse inspect <item>` | The item's ranks, choices and power details |
| `infuse paths <item>` | Paths available for the item |
| `infuse choices <item> primary` | Available bonuses and powers, with their selection codes |
| `infuse loadout` | Your prepared powers and active or inactive bonuses |
| `infuse help` | The full command list |

Start with one item. Awaken it, choose its two bonuses, then work toward a technique you will enjoy using. Each rank remains useful while you save for the next.
