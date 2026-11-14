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

  clan quartermaster list                  Your services and remaining limits
  clan quartermaster info                  Prices, limits and unlocks
  clan quartermaster motes <type> <number>  Preview crafting motes
  clan quartermaster quest <number>         Preview gold for quest points
  clan quartermaster enhance <weapon>       Preview a random enchantment
  clan quartermaster reroll <weapon>        Preview replacing an enchantment
  clan quartermaster inspect <weapon>       Effect, command word and reroll
  clan quartermaster confirm                Accept the current preview

Visit a quartermaster outside combat and timed crafting. Accepted clan
membership is required to buy. Confirm within 60 seconds; previews disappear
on reconnect or the weekly reset. Favor unlocks services and is never spent.
The old quartermaster gear shop has been replaced by these services.

Weekly mote and quest-point allowances are SEPARATE FOR EACH CHARACTER,
across all clans. They reset Monday at midnight UTC. Changing clans does not
reset an allowance. The following are the initial limits and prices; use
clan quartermaster info for the current settings.

  Favor rank     Motes/week     Quest points/week
      0              1                  5
      1              2                 10
      2              3                 15
      3              4                 20
      4              5                 25
      5              6                 30

Motes cost 2,500 gold each. Types: air, dark, earth, fire, ice, light,
lightning and water. All mote types share the weekly mote allowance.
Quest points cost 1,000 gold each and use a separate weekly allowance.

A weapon enchantment costs 250,000 gold and requires Favor rank 1 or higher.
Carry the weapon in your inventory. It must be a permanent, ordinary weapon
without existing special abilities, spells, scripts or a legendary identity.
Normal weapon enhancement bonuses can remain. Weapons bound to someone
else cannot be enchanted. One clan enchantment is allowed per weapon.

The roll ONLY draws effects unlocked by your CURRENT FAVOR RANK and suited
to the weapon. Each eligible effect has weight 6 minus its minimum rank:
rank 1 weight 5, rank 2 weight 4, rank 3 weight 3, rank 4 weight 2, rank 5
weight 1. Thus each rank 1 effect is five times as likely as each rank 5
one when both are eligible. Previewing never reveals or changes a roll.

New effects start INACTIVE. Inspect or identify the weapon to learn its
random command word, then equip it and UTTER <word> to toggle it on/off.
For example, a Flaming weapon might use inferno, blaze or searing.
Passive effects also switch off when the clan enchantment is inactive.

A reroll costs 500,000 gold. It replaces the clan effect and command word,
starts inactive, and can roll the same effect again. Each individual weapon
can reroll ONCE PER WEEK, even if traded to another character. The initial
enchantment does not consume that reroll. Favor unlocks still apply when
rerolling. The weapon keeps its other properties and normal bonuses.

For exact mechanics and current limitations: HELP WEAPONEFFECTS.
Some named effects have incomplete mechanics; read that guide before rolling.

Weapon effects -- number is the minimum Favor rank:

  1 Anarchic        Extra damage against lawful enemies.
  1 Axiomatic       Extra damage against chaotic enemies.
  1 Unholy          Extra damage against good enemies.
  1 Flaming         Fire damage on hits.
  1 Frost           Cold damage on hits.
  1 Ghost Touch     Effective against incorporeal enemies.
  1 Holy            Extra damage against evil enemies.
  1 Corrosive       Acid damage on hits.
  1 Keen            Improves a sharp weapon''s critical threat range.
  1 Ki Focus        Channels monk ki abilities through a melee weapon.
  1 Shock           Electricity damage on hits.

  2 Bane            Extra damage against a randomly chosen creature family.
  2 Thundering      Sonic damage on critical hits.
  2 Throwing        Allows an ordinary melee weapon to be thrown.
  2 Defending       Trades weapon effectiveness for armor protection.
  2 Disruption      Disrupts undead; blunt weapons only.
  2 Bewildering     Can confuse enemies on critical hits.
  2 Exhausting      Fatigues enemies and drains movement on critical hits.
  2 Mighty Cleaving Gives access to cleave attacks and one extra cleave.
  2 Returning       Returns a thrown weapon rather than consuming it.
  2 Seeking         Helps ranged attacks overcome concealment.

  3 Speed           Extra attack; follows normal haste stacking rules.
  3 Dancing         An additional weapon attack during a full attack.
  3 Vicious         Extra damage to enemies, with backlash to the wielder.
  3 Flaming Burst   Fire damage plus a burst on critical hits.
  3 Agile           Uses enhancement-capped Dexterity with light/finesse weapons.
  3 Icy Burst       Cold damage plus a burst on critical hits.
  3 Corrosive Burst Acid damage plus a burst on critical hits.
  3 Invigorating    Recovers movement on hits and critical hits.
  3 Shocking Burst  Electricity damage plus a burst on critical hits.

  4 Wounding        Currently inactive under standard settings; see weaponeffects.
  4 Blinding        Can blind enemies on critical hits.
  4 Adaptive        Adds 1.5x positive Strength damage to non-thrown ranged attacks.
  4 Vampiric        Drains living enemies and heals the wielder.
  4 Lucky           10% chance of an enhancement-based bonus on shared d20 rolls.

  5 Vorpal          5% killing-blow chance on eligible critical hits.

Accepted purchases recover automatically after reconnecting, even if you
have since left the clan. If recovery remains pending, contact staff.
Leaving a clan does not remove an enchantment; builder-set item clan
requirements still apply when purchasing or equipping restricted gear.',CHAR(10),CONCAT(CHAR(13),CHAR(10))),0,1000,FALSE)
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
