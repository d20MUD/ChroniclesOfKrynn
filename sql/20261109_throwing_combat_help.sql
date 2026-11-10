-- Thrown ammunition, hybrid combat and equipped-weapon throws; safe to reapply.
START TRANSACTION;
INSERT INTO help_entries(tag,category,entry,min_level,max_level,auto_generated)
VALUES ('THROW','combat',REPLACE('THROWING WEAPONS

Usage: throw <target>
       throw
       throw off

Wear an ammo pouch or quiver containing throwing ammunition, then use
throw <target> to open combat. You may hold a melee weapon or be bare handed.
An equipped matching throwing weapon is not required. The pouch can hold
bundles of thrown ammunition and individual weapons with the thrown flag.
Without throwing ammunition, this form of throwing does not work.

Combat opens with a thrown volley. After the opening round, switch to your
held melee weapon or unarmed attacks as soon as the target is within your
melee reach. If ammunition runs out while the target is still distant,
the remaining throws are skipped. Later rounds cannot throw until you
replenish ammunition; you can still advance to close for melee. Running out
of ammo within melee reach allows melee immediately. Throw off or hit also
returns you to melee use.

Each throw uses the selected ammunition''s weapon type, damage dice,
enhancement and properties, not those of your held melee weapon. Proficiency,
Weapon Focus, specialization and other weapon-specific feats use the thrown
type and family. Attack rolls use Dexterity; damage uses Strength.

Without Quick Draw, ordinary thrown ammunition permits one attack per round.
Quick Draw permits your full attack routine and eligible ranged feat attacks.
Using a move action still limits you to one attack that round.

THROWING AN EQUIPPED WEAPON
If no throwing ammunition is available, an equipped weapon with the thrown
flag (such as a dagger, spear, hand axe or light hammer) can be thrown by
itself. This also works without a worn ammo pouch. After the attack, the
weapon goes to your inventory and must be wielded again before another
throw. Re-wielding in combat uses a move action, or a swift action with
Quick Draw. Misses also return the weapon to inventory; attacks rejected
before the throw do not unequip it.

A Returning weapon stays equipped, or stays in its pouch, and can make
multiple thrown attacks without being re-wielded. Returning thrown ammunition
is also reusable. Unlimited ammunition preserves the supply but does not
itself grant Quick Draw.

THROWN AMMUNITION TYPES
Shuriken, darts, throwing axes, throwing knives, javelins and throwing
hammers drop as ammunition rather than individual random weapon rewards.
Throwing knives use the light blade family, javelins use polearms, and
throwing hammers use hammers. A pouch may also hold normal throwable weapons.
A non-returning individual pouch weapon goes to inventory after its throw.

Thrown weapons can reach five range increments. Range penalties, formation
rules and normal combat protections still apply. Ammo in your inventory or
another container is not used as a pouch supply.

See also: RANGED, AMMUNITION, AMMOPREF, ADVANCE, QUICKDRAW, HOOPAK',CHAR(10),CONCAT(CHAR(13),CHAR(10))),0,1000,FALSE)
ON DUPLICATE KEY UPDATE category=VALUES(category),entry=VALUES(entry),min_level=VALUES(min_level),max_level=VALUES(max_level),auto_generated=FALSE;
DELETE FROM help_keywords WHERE help_tag='RANGED_COMBAT' AND keyword IN ('throw','throwing','returning');
INSERT IGNORE INTO help_keywords(help_tag,keyword) VALUES
('THROW','throw'),('THROW','throwing'),('THROW','thrown'),('THROW','returning');
UPDATE help_entries SET entry=REPLACE('RANGED COMBAT AND AMMUNITION

Usage: fire <target>
       reload
       autoreload
       throw <target>
       throw off

Bows, crossbows, and slings let you attack enemies before they close into
melee. Your target must be in the same room as you. You cannot shoot into
an adjacent room, even with Far Shot or another long-range ability.

GETTING READY
  Bows use arrows.
  Crossbows use bolts.
  Slings use bullets.

Wield your weapon, wear an ammo pouch or quiver, and put matching ammunition
inside it. A pouch cannot be worn together with a weapon sheath.
Carrying ammunition in your inventory is not enough. Your weapon
uses the first ammunition in your pouch, so keep the kind you want to fire
at the top. If the wrong kind is first, take it out of the pouch.

For example, using the keywords on your own equipment:
  wield bow
  wear quiver
  put arrows quiver
  fire goblin

Use fire <target> to begin shooting. Crossbows and slings also need to be
loaded: use reload before firing and whenever the weapon needs reloading.
The autoreload setting can help keep these weapons ready during combat.
Bows do not need reload. Filling a pouch and reloading a weapon are separate
steps. Autoreload toggles automatic weapon reloading.

Fire starts shooting and subsequent attacks continue through combat rounds;
you do not need to repeat it for every shot. You cannot start fire while
already fighting. If you target a group member or your own pet, fire tries
to assist by shooting that character''s current opponent in the same room.

AMMUNITION
A bundle may hold many shots. Each shot uses one piece of ammunition,
including a miss or a shot that is deflected. When the bundle runs out,
you need another supply of the correct ammunition. Keep spare matching
bundles in your pouch; after one is used up, the next supplies your shots.
If no usable ammunition remains, ranged attacks stop. Resupply and, once
you are no longer fighting, use fire <target> to begin again.

Enchanted ammunition can improve accuracy and damage. Some ammunition also
adds damage such as fire or cold when it hits. A foe''s resistances still
matter. The extra elemental damage is not multiplied on a critical hit.
Weapon and ammunition enhancement use the higher bonus, not their sum.
Rare supplies may never run out; they still require the correct worn pouch
and do not remove crossbow or sling reload requirements.

Fired ammunition is permanently spent. It never goes into an enemy''s
inventory, a corpse, or the room, even after a miss, deflection, or catch.
Collect and autocollect have been removed. Unlimited and Returning supplies
remain available without losing quantity; their emitted shots also vanish.

Ammo listings in inventory, rooms, and ammo pouches show the remaining
quantity: a bundle of simple arrows (40) has 40 arrows left in that item.
A legacy single projectile shows (1), an empty bundle (0), and an unlimited
or Returning supply (unlimited). Different remaining counts are listed
separately rather than combined into a misleading single line.

Ordinary spells and abilities do not spend weapon ammunition merely because
they make a ranged attack roll.

DISTANCE AND ADVANCING
If either opponent wields a ranged weapon when battle begins, the fight
starts at the room''s maximum distance. Otherwise, it starts at 0 feet.
Wide fields generally allow more distance than a small interior. Switching
weapons during a fight does not reset your distance from an opponent.

Mobs, including companions, close the distance at their movement speed
once per round. Most must reach 5 feet before they can attack. Some enemies
can attack from a distance and will continue advancing while doing so.

If you change to a melee weapon while your enemy is still far away, use:
  advance
  advance <opponent>

Advancing spends one move action and moves you toward a mob you are fighting.
Ordinary melee attacks need you to be within 5 feet. Reach weapons, such as
polearms, can strike from 10 feet. Your advance stops at your weapon''s reach.
Unarmed attacks cannot borrow the reach of a polearm you are carrying.

THROWING WEAPONS
Use throw <target> with throwing ammunition in your worn pouch while holding
a melee weapon or while bare handed. Your opening round uses thrown attacks;
afterward, switch to melee when your opponent comes within melee reach.
Thrown attacks use the ammunition''s own weapon type and feats, Dexterity to
hit, and Strength for damage. Exhausted ammunition means skipped throws
until you replenish it or close for melee.

If no thrown ammunition is available, you can instead throw an equipped
throwable weapon. A non-returning weapon goes to inventory after the attack;
re-wielding in combat costs a move action, or swift with Quick Draw.
Returning weapons stay equipped and can make multiple thrown attacks.
Use help throw for ammunition types, full attacks and Returning details.

WEAPON RANGE
Weapons have their own range increments. Beyond the first increment, each
additional increment gives -2 to hit; Far Shot reduces this to -1. Thrown
weapons can reach up to five increments, other ranged weapons up to ten.
The room also limits distance, and you can never shoot through an exit.
For example, a dagger''s increment is 10 feet and a javelin''s is 30 feet.

POSITION AND FEATS
Point-Blank Shot grants +1 to ranged attack and damage rolls within 30 feet.
There is no penalty simply for lacking this feat, being in the front or
middle row, or being the enemy''s target.

Precise Shot prevents a separate -4 penalty when another player is tanking
the mob you are shooting. A companion or other mob tank does not cause this
penalty. Improved Precise Shot also prevents it and ignores partial
concealment. Total concealment still applies.

Rapid Shot adds one attack at your highest bonus during a full weapon attack,
with -2 on all attacks in that volley. Use rapidshot to toggle it. A thrown
volley requires Quick Draw. Rapid Shot does not add spell attacks.

Manyshot automatically fires two arrows on the first bow attack of a full
attack, using one attack roll. Both arrows are spent. Critical and precision
damage apply only once; each arrow faces damage reduction separately. This
feat does not give extra dagger, shuriken, sling, or crossbow attacks.
With only the feat, manyshot <target> performs a full bow attack immediately
and uses both your standard and move actions. It still applies automatically
to the first bow attack of normal full attack rounds. Fire automatically
uses this full-attack opening when you have the actions and two arrows;
otherwise it fires a single shot.

The ranger Manyshot perk uses manyshot <target> for a separate active volley:
three bow attacks and a 120-second cooldown, or five attacks and 60 seconds
with Improved Manyshot. This volley uses a standard action and ammunition.
Fire automatically uses the perk volley when its cooldown is ready.
If you own both, the command uses the perk; the feat still applies during
normal full bow attack rounds.

Deadly Aim trades accuracy for weapon damage. Use deadlyaim to toggle it.
It starts at -1 to hit for +2 damage, increasing by -1/+2 at base attack
bonus +4 and every +4 thereafter. Its damage bonus does not apply to touch
attacks or spells. You do not choose the penalty with a number.

Rapid Reload makes hand and light crossbow reloads free actions, and heavy
crossbow reloads move actions. Sling and repeating crossbow support is also
retained. It does not supply ammunition.

In a group, formation back lets a ranged attacker stay behind the front
line. Melee and reach attacks must still obey formation rules after you
advance. Point-Blank Shot does not change which rows permit melee attacks.

IF YOU CANNOT FIRE
Check that you are wielding a ranged weapon, wearing your ammo pouch, and
have the correct ammunition first in the pouch. Check that your supply is
not exhausted and that a crossbow or sling has been reloaded. Your target
must be present in your room, and combat must be permitted there.

See also: FORMATION, ADVANCE, FIRE, RELOAD,, AUTORELOAD,',CHAR(10),CONCAT(CHAR(13),CHAR(10))) WHERE tag='RANGED_COMBAT';
COMMIT;
