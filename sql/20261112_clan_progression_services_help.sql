-- Phase 47: repeatable help publication only; no gameplay enablement.
START TRANSACTION;
INSERT INTO help_entries(tag,category,entry,min_level,max_level,auto_generated) VALUES
('CLANPROGRESSION','general',REPLACE('CLAN PROGRESSION

  clan progress [clan name]
  clan favor [page]

Clan progress shows your clan''s level, Influence, your Favor and weekly
limits. Clan favor lists your saved standing, including former clans.
Use a full clan name or a unique part of its name to view that standing.

Lifetime Influence raises clan level. Available Influence pays for buffs;
spending it never lowers your clan''s level. Your Favor is permanent standing
with that clan and is never spent. Legacy clan points and bank gold remain
separate. A new clan using an old name does not inherit its progress.

Account limits are shared across your characters and clans. Weekly limits
reset Monday at midnight UTC. Suitable natural encounters and clan jobs earn
progress; trivial, summoned or staff-created test encounters do not.

Try clan buffs, clan contracts bounty list, clan contracts gathering list,
clan deliveries list, clan expeditions list or clan quartermaster list.
Former members can view their own Favor but not private clan balances.

Staff inspection remains available through clan inspect
<character-id> <account-id> <clan-incarnation-uid>.',CHAR(10),CONCAT(CHAR(13),CHAR(10))),0,1000,FALSE),
('CLANBUFFS','general',REPLACE('CLAN BUFFS

  clan buffs
  clan buffs activate <name> <tier> <slot>
  clan buffs replace <name> <tier> <slot>
  clan buffs cancel <name or slot>
  clan buffs permission <maximum rank>

WHAT EACH BUFF DOES

All values below are shown as Tier 1 / Tier 2 / Tier 3.

Scholar''s Encouragement
  +5% / +8% / +10% eligible combat experience.
  Helps you level your character. It does not increase quest or crafting
  experience, Favor, Influence, or other clan rewards.

Arcane Readiness
  10% / 15% / 20% shorter spell preparation and spontaneous spell-slot
  recovery times. Each cycle receives its benefit when that cycle starts;
  an already-running cycle keeps its timing. Queued spells do not reserve
  the bonus before their own preparation begins.

Psionic Focus
  +10% / +20% / +30% power-point recovery outside combat.
  Helps replenish PSP; it does not increase maximum PSP or movement recovery.

Company Vitality
  +5% / +10% / +15% maximum hit points.
  This raises your health ceiling without healing you. When the buff ends,
  current health cannot exceed your normal maximum. This is a combat buff.

Artisan''s Guidance
  +2 / +3 / +4 to crafting production and harvesting skill checks.
  These are flat bonuses to checks, not percentages or extra materials.
  It does not directly increase clan rewards.

UNLOCKS, COSTS AND DURATION

             Clan level       Influence cost
  Tier 1     1 (Artisan: 3)        13
  Tier 2     5                     19
  Tier 3     20                    29

Each buff lasts 24 real hours. Its family becomes available again 36 hours
from activation: 24 hours active, followed by 12 hours of downtime. Time
continues while you are offline. Changing tier or cancelling early does
not reset or shorten the family''s next-use time.

A clan starts with two buff slots and unlocks a third at clan level 10.
Only one combat buff may be active at once. Different tiers of the same
buff do not stack. Benefits apply to current accepted clan members while
the buff is active; leaving the clan removes access to its benefits.

Use clan buffs to see your clan''s current unlocks, prices and active buffs.

ACTIVATING AND MANAGING BUFFS

Use the buff''s name, or a unique part of it. For example:
  clan buffs activate Scholar 1 1
  clan buffs cancel Scholar

The list shows tier prices, clan level requirements, active buffs and when
a family becomes available again. Activation spends shared Influence.
Replacement and cancellation give no refund and never reset a cooldown.
Replacement selects the active buff in the specified slot.

Only accepted clan members with buff-management permission may make changes.
A leader can delegate management with clan buffs permission <rank>.
Use zero to restore leader-only management. Staff status does not bypass it.

If a result cannot be confirmed, the game prints a recovery command. Copy
that command exactly; it checks the original request without paying twice.',CHAR(10),CONCAT(CHAR(13),CHAR(10))),0,1000,FALSE),
('CLANDELIVERIES','general',REPLACE('CLAN DELIVERIES

  clan deliveries list
  clan deliveries accept <name>
  clan deliveries active
  clan deliveries abandon
  clan deliveries turnin
  clan deliveries confirm
  clan deliveries retry

Choose a delivery by its name or a unique part of the name. Visit a
quartermaster outside combat and timed crafting to accept and turn in.
You can hold one delivery and complete two different deliveries per account
per week. Unfinished deliveries expire at the displayed weekly reset.

Materials require 50 units of the named material and grade. Ammunition
requires 100 finite shots of the requested kind and enhancement. Purchased
and transferred goods count. Returning or infinite ammunition does not.
Ammunition comes from your equipped pouch and directly carried bundles.

Finished-item contracts also request crafted weapons, armor, shields,
clothing and instruments. Deliver the listed number of matching items
(usually five). Items must be player-crafted, unenhanced and unbound, and
carried directly in your inventory. Equipped items and items inside bags
are not used. Spell-bearing, legendary items and quartermaster purchases do not
qualify. You can use your own work or buy suitable goods from another crafter.

Base rewards are 12.5 Favor and 6.25 Influence, subject to limits. Turnin
shows the actual reward before consuming anything. Type clan deliveries
confirm within 60 seconds to accept the current preview. Changed goods,
membership or rewards require a new preview. A zero reward takes no goods.

Accepted deliveries recover automatically after reconnecting. Use retry
if recovery is pending; contact staff if it remains unavailable.',CHAR(10),CONCAT(CHAR(13),CHAR(10))),0,1000,FALSE),
('CLANCONTRACTS','general',REPLACE('CLAN GATHERING AND BOUNTY CONTRACTS

  clan contracts gathering list
  clan contracts gathering accept <name>
  clan contracts gathering active
  clan contracts gathering abandon

Use bounty instead of gathering for bounty contracts. Names can include
spaces, and a unique part of the name is enough. For example:
  clan contracts gathering accept Fresh ash harvest

Accept at a quartermaster outside combat and timed crafting. You may hold
one gathering contract and one bounty at a time, plus a separate delivery.
Complete up to two different jobs in each family per account each week.
Unfinished contracts expire when the weekly choices change.

Gathering requires 20 fresh base units of the named material and grade.
Bonus yield, purchases and existing stock do not count. You keep the goods.
Bounties require 20 suitable kills of the named target after acceptance.
Meaningful combat or support counts; the killing blow is not required.

Base rewards are 12.5 Favor and 6.25 Influence, subject to limits. The first
completion of each bounty also grants 12.5 shared Influence, up to two
bounties per clan each week. Progress saves automatically; reconnecting does
not repeat a reward. Use clan contracts <family> retry if recovery is pending.',CHAR(10),CONCAT(CHAR(13),CHAR(10))),0,1000,FALSE),
('CLANEXPEDITIONS','general',REPLACE('CLAN EXPEDITIONS

  clan expeditions list
  clan expeditions create <name>
  clan expeditions join [name]
  clan expeditions active
  clan expeditions start
  clan expeditions leave
  clan expeditions abandon
  clan expeditions claims
  clan expeditions claim [name]

Use the displayed expedition name or a unique part of it. Join without a
name to join your clan''s current outing. Claim without a name collects your
only waiting reward; if several are waiting, use the family name shown by
claims, such as Stronghold Assault or Regional Hunt.

Each clan can have one recruiting or active outing. Bring 2-6 accepted
members on separate accounts. Create, join and start at a quartermaster,
outside combat and timed crafting. Everyone must be present to start.
Started outings expire after two hours or at the weekly reset, whichever
comes first. The roster cannot change after starting.

Active shows named targets, areas, progress and party members. At least two
enrolled accounts must meaningfully participate. Personal completion needs
participation in at least half the objectives and the final objective.
Trivial, summoned, manipulated and staff-created test enemies do not count.

Personal completion grants 25 Favor, before limits, once per family per
account each week. Claim at a quartermaster before the weekly reset. The
first clan completion per family each week earns 37.5 shared Influence.
Replaying a paid claim never pays twice. Leaving the clan invalidates an
unclaimed reward; accepted rewards retain their recovery history.',CHAR(10),CONCAT(CHAR(13),CHAR(10))),0,1000,FALSE),
('CLANQUARTERMASTER','general',REPLACE('CLAN QUARTERMASTER

  clan quartermaster list
  clan quartermaster buy <name>
  clan quartermaster commission <name>
  clan quartermaster confirm

Choose an item by its displayed name or a unique part of it. For example:
  clan quartermaster buy Abanasinia field map
  clan quartermaster confirm

Visit a quartermaster outside combat and timed crafting, with enough gold
and room to carry the item. Buy and commission preview the price. Confirm
within 60 seconds to accept that preview. A changed price or expired preview
requires a new preview. Repeating confirmation never buys another copy.

The list shows gold prices, character and clan levels, and required Favor.
Favor unlocks the purchase and is never spent. Purchases bind to your
character and have no resale value. Ordinary class, alignment, proficiency
and level restrictions still apply. Leaving the clan does not remove gear.

Accepted purchases recover automatically after reconnecting. Unconfirmed
previews are lost on reconnect. If recovery remains pending, contact staff.',CHAR(10),CONCAT(CHAR(13),CHAR(10))),0,1000,FALSE)
ON DUPLICATE KEY UPDATE category=VALUES(category),entry=VALUES(entry),min_level=VALUES(min_level),max_level=VALUES(max_level),auto_generated=FALSE;
INSERT IGNORE INTO help_keywords(help_tag,keyword) VALUES
('CLANPROGRESSION','clanprogression'),
('CLANPROGRESSION','clan progression'),
('CLANPROGRESSION','clan progress'),
('CLANPROGRESSION','clanfavor'),
('CLANPROGRESSION','clan favor'),
('CLANPROGRESSION','claninspect'),
('CLANPROGRESSION','clan inspect'),
('CLANBUFFS','clanbuffs'),
('CLANBUFFS','clan buffs'),
('CLANBUFFS','clanbuff'),
('CLANBUFFS','clanbuffmanagement'),
('CLANDELIVERIES','clandeliveries'),
('CLANDELIVERIES','clan deliveries'),
('CLANCONTRACTS','clancontracts'),
('CLANCONTRACTS','clan contracts'),
('CLANEXPEDITIONS','clanexpeditions'),
('CLANEXPEDITIONS','clan expeditions'),
('CLANQUARTERMASTER','clanquartermaster'),
('CLANQUARTERMASTER','clan quartermaster');
INSERT IGNORE INTO help_keywords(help_tag,keyword) VALUES
('CLANBUFFS','clan buff effects'),
('CLANBUFFS','scholar'),
('CLANBUFFS','scholars encouragement'),
('CLANBUFFS','scholar''s encouragement'),
('CLANBUFFS','arcane readiness'),
('CLANBUFFS','psionic focus'),
('CLANBUFFS','company vitality'),
('CLANBUFFS','artisans guidance'),
('CLANBUFFS','artisan''s guidance');
COMMIT;
