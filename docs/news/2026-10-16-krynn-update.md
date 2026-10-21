# New paths, lasting companions, and a place to call home

*Chronicles of Krynn development roundup — June 7–October 14, 2026*

The latest months of development bring new ways to build your character, adventure
with friends, and invest in life beyond the next fight. Two prestige classes,
player housing and auctions, growing companions, and permanent account rewards
lead this roundup of changes committed since June 7.

## Discover Dragon Disciple and Loremaster

Embrace draconic power with **Dragon Disciple**, a ten-level prestige class with
natural attacks, breath, wings, blindsense, and Dragon Form. Its bloodline and
spellcasting progression support the path from spellcaster to draconic adventurer.

For those drawn to knowledge, **Loremaster** offers secrets, bonus languages,
and increasingly powerful lore abilities. Each level advances an eligible existing
spellcasting class, giving scholarly characters room to shape their own progression.
Both classes are now part of the merged codebase.

## Build a home and trade with other adventurers

The new neighbourhood housing system lets characters buy homes, expand them with
new rooms, customize their surroundings, invite guests, and add persistent storage
chests. A fixed-price auction house gives players a way to list equipment and buy
from one another. These features become available where staff configure
neighbourhood entrances and auction locations.

Direct trading is also available through `trade`: offer items, gold, crafting
materials, and motes, review both sides, and complete the exchange when both
players accept. Prefer to avoid requests? Toggle `rejecttrades`.

## Make progress across your account

Account XP has a new long-term use: **permanent account-wide bonuses** to XP,
treasure drops, gold, and quest points. Each 1% upgrade costs 2,000 account XP,
with a 10% cap in each category. Use `accexp bonus` to see your bonuses and
`accexp bonus <type>` to buy an upgrade.

## Help your companions grow—and bring them back

Newly purchased pets can earn their own combat XP without reducing yours, growing
up to your level or level 30. Eligible permanent class companions instead earn
training ranks and points that you can spend on health, attack, damage, armor,
or saving throws.

Defeat can now lead to recovery for permanent owned companions. Their progression,
equipment, inventory, and gold are stored when they fall. Check `pets fallen`,
then use `pets revive <ID>` to bring one back for 500 carried gold per pet level,
provided you have follower space and are outside combat. Temporary summons keep
their existing behavior. Older purchased pets without the new purchase marker do
not automatically join the purchased-pet leveling system.

Golem owners also gain appearance customization and fixes to healing, repair,
and persistence after login.

## Give your group a battle plan

**Front, middle, and back formations** add a tactical choice to group combat.
Keep melee fighters up front, use reach weapons from the middle, and place ranged
attackers behind them. Group leaders can position companions as well as players.

Enemy size matters: large creatures reach farther, and huge enemies can reach all
three rows. Ranged attacks and area effects still follow their normal rules.
Type `formation` or read `help formation` before your next expedition.

## Watch the skies

Krynn's lunar phases and conjunctions now influence eligible arcane spellcasters'
spell DC, bonus spells, and effective caster level. The Night of the Eye is tied
to a genuine triple conjunction at High Sanction.

Check `score` for your current modifiers, `look moons` when viewing conditions
allow, and `help moons` for the rules. Knights of the Thorn remain exempt.

## Get more from crafting and exploration

Crafting gains disenchanting for catalyst fragments, broader catalyst use, and
batch refining. Climate and terrain support more harvested material varieties,
with same-type, same-grade exchanges available through configured material traders.
Rare materials and motes can also appear alongside normal treasure; the highest
grades remain tied to tougher enemies and rare drops.

Happy-hour bonuses now apply consistently to crafting and gathering XP, refined
and harvested yields, butchering, and mote quantities. Insightful grants 20%
bonus gathering skill XP per rank in its supported skills, reaching 100% at five
ranks. Harvest messages now show your total material yield.

## More ways to cast, fight, and prepare

Clerics gain spontaneous casting and a clearer `domain spells` listing.
Practiced Spellcaster helps eligible multiclass casters recover up to four caster
levels, capped by total character level, without adding spell circles or slots.
Epic spells have received damage, effect, and access improvements.

Alchemists gain clearer bomb counts and combat feedback, proper ranged-feat
support, and Faster Bomb Preparation. Ordinary extracts work in silence.
Dragon riders gain Dragon Flight, while suitable aquatic creatures and forms
swim automatically in ordinary swimmable water.

Everyday preparation is smoother too: keep up to ten buff lists, see remaining
Stoneskin, Ironskin, and Epic Warding protection in `affects`, and use information
or chat commands while building or repairing devices. Group quest objectives
now share completion among members on the same quest, and companion kills credit
their owner.

One editor change to remember: **use `/s` to save**. `@` and `/@` no longer save,
so lines beginning with those characters or color codes stay in your text.

---

*This roundup describes committed development through October 14. Availability
on the live game depends on deployment and, for housing, auctions, and material
exchanges, world setup. Read the [detailed changelog](../CHANGELOG-2026-06-07-to-2026-10-14.md)
for fixes, campaign-specific work, and links to the source history.*
