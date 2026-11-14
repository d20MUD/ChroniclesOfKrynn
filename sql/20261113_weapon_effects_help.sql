-- Help publication only; does not change gameplay settings or combat.
START TRANSACTION;
INSERT INTO help_entries(tag,category,entry,min_level,max_level,auto_generated)
VALUES ('WEAPONEFFECTS','general',REPLACE('WEAPON SPECIAL EFFECTS

  help weaponeffects
  help weapon effects
  help clanquartermaster
  clan quartermaster inspect <weapon>

This guide describes the current game mechanics of all weapon special
abilities. Some names describe unfinished abilities; those entries say so.
It does not assume the tabletop rules are implemented.

USING AN EFFECT

Clan enchantments start inactive. Identify or inspect the weapon to learn
its command word, equip it, then UTTER <word> to switch it on or off. All
benefits of that clan enchantment stop while it is inactive, including
otherwise passive effects. Activation does not spend Favor.

Ordinary world items and weapon oils can use different command words and
activation settings. Identify the particular item. An item''s builders can
configure extra triggers; the descriptions below distinguish those from
the standard settings used by new items and clan enchantments.

Damage dice shown here are added effects before applicable protections.
A critical effect requires a critical hit that the target is not immune
to. A critical hit can also trigger the ordinary on-hit effect if the
victim survives; critical amounts are additional unless stated otherwise.
An effect granting extra attacks still follows action and combat limits.

ENHANCEMENT AND DURATION

Enhancement means the weapon''s normal +N bonus, not its clan Favor rank.
Fractions in the formulas below round down. Effect durations are measured
in combat-round updates; they are not minutes.

ADAPTIVE
  While active, a non-thrown ranged attack gains 1.5 times a positive
  Strength damage bonus, rounded down, instead of that launcher''s normal
  Strength allowance. A zero or negative Strength modifier uses the normal
  ranged rules. Thrown attacks already use their separate Strength rules.
  This is a damage benefit, not extra range or a higher attack roll.

AGILE
  With an active Agile weapon that qualifies as light or finesse for you,
  the damage calculation can use a capped Dexterity contribution instead
  of Strength. The cap is the highest normal enhancement bonus among
  your wielded Agile weapons. It uses the larger of that capped positive
  Dexterity bonus and your Strength modifier.
  Example: Dexterity +6, Strength +1 and an Agile +3 weapon give a +3
  contribution, not +6. An unenhanced Agile weapon has a zero cap.
  This does not grant Weapon Finesse. The active property also disables
  the ordinary two-handed Strength contribution for that attack.

ANARCHIC
  Adds 2d6 weapon damage against lawful opponents, including creatures
  marked as lawful. Neutral and chaotic opponents do not qualify.

AXIOMATIC
  Adds 2d6 weapon damage against chaotic opponents, including creatures
  marked as chaotic. Neutral and lawful opponents do not qualify.

BANE
  Adds 2d6 weapon damage against the weapon''s chosen creature family.
  If a subtype is specified, that subtype must also match; no subtype
  means the whole family. Clan Bane chooses its family randomly when
  purchased. The weapon''s happy humming is cosmetic, not a damage roll.

BEWILDERING
  A critical hit can confuse the target for 1 round. A Will save prevents
  it. Confusion immunity and creatures protected from confusion prevent
  the effect. It does not confuse on an ordinary hit.

BLINDING
  A critical hit can blind the target for 1 round, with -4 to attack rolls
  and a -4 armor modifier. A Reflex save prevents it, and the target must
  be susceptible to blindness. Saying the ordinary effect''s word alone
  does not perform a blinding attack.

BRILLIANT ENERGY -- NOT IMPLEMENTED
  The ability is named, but no weapon combat benefit is currently wired
  to it. Do not expect it to ignore armor or shields. It is not offered
  by the clan quartermaster.

CORROSIVE
  Adds 1d6 acid damage on a successful hit while active.

CORROSIVE BURST
  Adds Corrosive''s 1d6 acid damage on hits, plus acid damage on criticals:
  (the weapon type''s critical multiplier minus 1)d10.
  Thus an x2 weapon bursts for 1d10 and an x3 weapon for 2d10.
  On ordinary items the critical burst can occur with the acid coating
  switched off if its critical trigger is enabled. An inactive CLAN
  enchantment suppresses both the hit damage and the critical burst.

DANCING -- CLAN VERSION
  An active, wielded clan Dancing weapon adds one main-hand attack to a
  full attack routine. Normal action requirements still apply; it does
  not give extra attacks during a single-action attack or Vital Strike.
  The weapon stays equipped; it does not float away or fight independently.
  Merely adding Dancing to an ordinary item''s ability list currently
  provides no equivalent combat benefit.

DEFENDING
  Trades attack accuracy for armor protection while its property is on.
  A clan weapon contributes an armor bonus equal to its normal enhancement
  (minimum 1), and the same amount is subtracted from the attack''s
  enhancement-bonus calculation.
  Ordinary Defending weapons instead use half their number of base damage
  dice, rounded down. For example, 2d6 contributes 1; 1d8 contributes 0.
  Contributions from multiple wielded Defending weapons add together.
  This does not directly subtract an equal amount of damage.

DISRUPTION
  Adds 2d6 holy damage on hits against undead while active. It does not
  automatically destroy undead. Clan rolls restrict it to blunt weapons.
  Under the current standard activation settings it has NO additional
  critical-hit effect.
  A specially configured item with a critical trigger can additionally
  deal (half the wielder''s level + 3)d6 holy damage against undead on a
  failed Fortitude save, or 3d6 on a successful save.

DISTANCE -- NOT IMPLEMENTED
  No range increase is currently connected to this named weapon ability.
  It does not double your range. Not offered by the clan quartermaster.

EXHAUSTING
  A critical hit can fatigue the target for 2 rounds and immediately
  remove 20 movement points, to a minimum of zero. A Fortitude save
  prevents both. This applies fatigue, not a separate exhaustion state.

FLAMING
  Adds 1d6 fire damage on a successful hit while active.

FLAMING BURST
  Adds Flaming''s 1d6 fire damage on hits, plus fire damage on criticals:
  (the weapon type''s critical multiplier minus 1)d10.
  An x2 weapon bursts for 1d10; an x3 weapon bursts for 2d10.
  Ordinary items can burst with their flames switched off if configured
  for criticals. An inactive clan enchantment cannot burst.

FROST
  Adds 1d6 cold damage on a successful hit while active.

GHOST TOUCH
  Bypasses the usual half-damage penalty when your physical attacks hit
  an incorporeal opponent. It also permits several physical maneuvers
  that would otherwise fail against incorporeal or ethereal creatures.
  It does not automatically reveal an invisible opponent or remove all
  of the opponent''s other protections.

HOLY
  Adds 2d6 weapon damage against evil opponents, including creatures
  marked as evil. Neutral and good opponents do not qualify.

ICY BURST
  Adds Frost''s 1d6 cold damage on hits, plus cold damage on criticals:
  (the weapon type''s critical multiplier minus 1)d10, minimum 1d10.
  Ordinary items can burst with their frost switched off if configured
  for criticals. An inactive clan enchantment cannot burst.

INVIGORATING
  Restores movement on successful hits, up to your maximum movement.
  Let H be the weapon''s normal enhancement; for a clan enchantment,
  H is at least 2. Each hit restores H/2, rounded down. A critical also
  restores (3 x H)/2, rounded down, if its trigger runs.
  Example: an unenhanced clan weapon restores 1 on a hit, plus 3 for a
  critical; a surviving critical''s ordinary hit trigger can also restore 1.
  This restores movement, not hit points, mana or psionic energy.

KEEN
  Expands the normal critical threat range: 20 becomes 19-20, 19-20
  becomes 17-20, and 18-20 becomes 15-20. It does not raise the critical
  damage multiplier and does not stack with Improved Critical for a
  second doubling. Clan rolls restrict Keen to sharp weapons.
  The current check looks for Keen among your wielded weapons.

KI FOCUS
  Lets a weapon carrying the active Ki Focus property deliver prepared
  monk techniques such as Stunning Fist, Quivering Palm, Water Whip,
  Gong of the Summit and Fist of Unbroken Air. You must already have and
  activate the technique; the weapon grants no new monk abilities.
  Clan activation supplies the required property. On ordinary gear,
  the named ability alone is insufficient unless the item also has
  the Ki Focus property.

LUCKY
  On each eligible shared d20 roll, there is a 10% chance to add a bonus
  equal to half your highest wielded Lucky weapon enhancement, rounded
  down, minimum +1 when the effect has a nonzero enhancement value.
  A clan Lucky weapon uses at least +1 for this calculation even if it
  has no normal enhancement. Ordinary unenhanced Lucky weapons give no
  bonus. Multiple Lucky weapons do not add their bonuses together.
  This can help attacks, critical confirmation, saving throws, initiative
  and other checks using that roll routine. Natural 1 and natural 20
  return before Lucky is checked. It is not a reroll or an always-on bonus.

MERCIFUL -- NOT IMPLEMENTED
  No extra damage or conversion to nonlethal damage is currently connected
  to this named ability. Not offered by the clan quartermaster.

MIGHTY CLEAVING -- CLAN VERSION
  An active, wielded clan weapon enables the melee cleave routine once
  per round, even without the Cleave feat. It needs another enemy in the
  room who is fighting you or a groupmate, other than your current target.
  It attacks that enemy at -4, then makes a second attack without that
  cleave penalty. Relevant perks can reduce the first attack''s penalty.
  It does not stack another extra swing on top of Great Cleave''s existing
  second attack. Ranged combat and being staggered prevent this routine.
  The named ability on an ordinary item''s list has no equivalent hook.

RETURNING
  An individual thrown weapon stays ready rather than being moved from
  its equipped slot or ammunition pouch back into inventory after a throw.
  This also permits a full thrown attack routine without Quick Draw.
  It does not make a non-throwable weapon throwable by itself.
  Without Returning, an individual thrown weapon is kept in inventory
  and must be readied again; it is not destroyed by the throw.

SEEKING
  The active property removes the concealment miss chance from the
  selected ranged attack. In throwing mode the selection follows the
  active thrown weapon, including one selected from an ammunition pouch.
  It does not improve your attack bonus or grant sight through invisibility.

SHOCK
  Adds 1d6 electricity damage on a successful hit while active.

SHOCKING BURST
  Adds Shock''s 1d6 electricity damage on hits, plus electricity damage on
  criticals: (the weapon type''s critical multiplier minus 1)d10.
  An x2 weapon bursts for 1d10; an x3 weapon bursts for 2d10.
  Ordinary items can burst with their lightning switched off if configured
  for criticals. An inactive clan enchantment cannot burst.

SPEED
  Gives one extra attack during an eligible full attack routine, at full
  attack bonus before other applicable modifiers. It shares the same
  extra-attack check as Haste and Blinding Speed, so those do not stack
  with Speed to produce additional attacks. Multiple Speed weapons also
  do not multiply the benefit. Normal action restrictions still apply.

SPELL STORING -- NOT IMPLEMENTED
  Builders can record a spell and spell level, but no weapon combat
  routine currently stores or releases that spell through this ability.
  Do not confuse it with separately configured weapon spells or procs.
  It is not offered by the clan quartermaster.

THROWING
  Makes an otherwise ordinary melee weapon eligible for thrown attacks.
  It still uses normal thrown-weapon selection, actions and range rules.
  It does not grant Returning or Quick Draw. Without those benefits,
  expect to ready the weapon again after a throw.
  Current limitation: some damage checks still use the weapon''s original
  type. A melee weapon made throwable this way may not get the Strength
  damage bonus that a naturally throwable weapon receives.

THUNDERING
  A critical hit adds 2d10 sonic damage. If the target can be deafened,
  a failed Fortitude save also deafens it for 2 rounds. A successful save
  or deafness immunity prevents the deafness, not the sonic damage.

UNHOLY
  Adds 2d6 weapon damage against good opponents, including creatures
  marked as good. Neutral and evil opponents do not qualify.

VAMPIRIC
  Against living targets, adds 1d8 negative-energy damage on hits and
  2d8+2 on criticals. Each successful damage effect attempts to heal
  the wielder for the positive damage amount returned by that effect,
  subject to normal healing rules. Critical and ordinary hit effects
  can both occur if the target survives.
  Undead and constructs are excluded. This is not healing based on the
  weapon''s total physical damage, and the word does not perform a drain
  by itself; it toggles a clan enchantment''s active state.

VICIOUS
  While active, each hit adds 3d6 negative-energy damage to the enemy
  and inflicts 1d4 negative-energy backlash on the wielder.
  The standard settings do not enable its separate critical routine.
  A specially configured critical trigger adds 6d6 to the enemy and
  1d6 backlash, in addition to any ordinary on-hit effect that follows.

VORPAL
  Each eligible critical hit makes a separate 5% roll for a killing blow.
  It is NOT an automatic decapitation on a natural attack roll of 20.
  Undead, constructs and oozes are excluded, as are protected creatures
  marked immune to charm (which includes many bosses and shopkeepers).
  A success attempts negative-energy damage equal to current target hit
  points plus 100; applicable protections can still affect the outcome.
  Clan rolls restrict Vorpal to slashing weapons.

WOUNDING -- INACTIVE UNDER STANDARD SETTINGS
  The current standard assignment, including clan enchantments, does not
  trigger bleeding. Do not count on Wounding for damage in that form.
  A specially configured item with hit/critical triggers can apply a
  1d4 bleeding effect for 3 rounds; each application can add another
  bleeding effect. Undead, constructs and oozes are excluded.
  This is bleeding damage, not Constitution damage.

See HELP CLANQUARTERMASTER for Favor unlocks, prices, weekly allowances
and reroll rules. This guide describes behavior, not changes to combat.',CHAR(10),CONCAT(CHAR(13),CHAR(10))),0,1000,FALSE)
ON DUPLICATE KEY UPDATE category=VALUES(category),entry=VALUES(entry),min_level=VALUES(min_level),max_level=VALUES(max_level),auto_generated=FALSE;
INSERT IGNORE INTO help_keywords(help_tag,keyword) VALUES
('WEAPONEFFECTS','weaponeffects'),
('WEAPONEFFECTS','weapon effects'),
('WEAPONEFFECTS','weapon special effects'),
('WEAPONEFFECTS','weapon abilities'),
('WEAPONEFFECTS','weapon special abilities'),
('WEAPONEFFECTS','weapon_specab');
COMMIT;
