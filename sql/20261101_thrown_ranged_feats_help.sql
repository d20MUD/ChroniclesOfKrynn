-- Player and builder guides for ranged combat, ammunition, shops and room distance.
-- Source text is mirrored in lib/text/help/ranged_combat.hlp and build_ranged.hlp.
-- Apply after the normal help schema exists. Builder entry requires level 31.

INSERT INTO help_entries(tag, category, entry, min_level, max_level, auto_generated)
VALUES ('RANGED_COMBAT', 'combat', REPLACE('RANGED COMBAT AND AMMUNITION

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

AMMUNITION
A bundle may hold many shots. Each shot uses one piece of ammunition,
including a miss or a shot that is deflected. When the bundle runs out,
you need another supply of the correct ammunition.

Enchanted ammunition can improve accuracy and damage. Some ammunition also
adds damage such as fire or cold when it hits. A foe''s resistances still
matter. The extra elemental damage is not multiplied on a critical hit.
Rare supplies may never run out.

Some fired ammunition can be recovered with collect. Your ammo pouch must
have room, and broken ammunition cannot be recovered. Autocollect can help
retrieve your ammunition after combat. Recovery does not refill an old
bundle: recovered shots are separate ammunition.

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
Wield a throwable weapon, such as a dagger or shuriken, and put matching
throwing ammunition in your worn pouch. Then use:
  throw goblin

The wielded weapon stays in your hand and supplies the attack''s weapon
properties. Each throw spends matching ammunition from your pouch. Daggers
need dagger ammunition; shuriken need shuriken ammunition. Enhancement bonuses
on the weapon and ammunition use the better bonus rather than adding together.
Thrown attacks use Dexterity to hit and Strength for damage.

Without Quick Draw, you can throw once per round. With Quick Draw, you can
throw at your normal full attack rate, including eligible Rapid Shot attacks.
Throwing mode continues during combat. Use throw off, or use hit, to return
to melee use of a weapon that supports melee. You may need to advance first.

Returning on your weapon OR throwing ammunition works like unlimited ammo:
you can keep throwing without spending the supply. There is no delay or
per-round supply limit from Returning. You still need matching ammunition
in your worn pouch. Returning does not grant Quick Draw or extra attacks.

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

See also: FORMATION, ADVANCE, FIRE, RELOAD, COLLECT, AUTORELOAD, AUTOCOLLECT
', CHAR(10), CONCAT(CHAR(13), CHAR(10))), 0, 1000, FALSE)
ON DUPLICATE KEY UPDATE
  category=VALUES(category), entry=VALUES(entry),
  min_level=VALUES(min_level), max_level=VALUES(max_level), auto_generated=FALSE;

INSERT IGNORE INTO help_keywords(help_tag, keyword) VALUES
('RANGED_COMBAT', 'ranged'),
('RANGED_COMBAT', 'ranged_combat'),
('RANGED_COMBAT', 'ammo'),
('RANGED_COMBAT', 'ammunition'),
('RANGED_COMBAT', 'throw'),
('RANGED_COMBAT', 'throwing'),
('RANGED_COMBAT', 'returning'),
('RANGED_COMBAT', 'ranged_feats');

INSERT INTO help_entries(tag, category, entry, min_level, max_level, auto_generated)
VALUES ('BUILD_RANGED', 'building', REPLACE('BUILDING RANGED WEAPONS, AMMO, VENDORS, AND ROOMS

This guide covers oedit, medit, sedit, and redit setup for the ranged system.
Use help ranged for the player rules and help advance for combat movement.
Use vnums assigned to your building area in the examples below.

1. BUILD THE WEAPON
Open oedit <weapon-vnum> and choose the normal Weapon item type. In C) Values,
select an actual bow, crossbow, or sling weapon type, then set its enhancement.
Use the weapon subtype to choose shortbow, longbow, composite bow, crossbow,
or sling behavior. Do not use the separate legacy Fire Weapon item type.

Selecting the weapon type supplies its base damage dice, weight, material,
size, cost, and take/wield wear flags. Check those fields after selection,
then set the name, descriptions, minimum level, price, and desired bonuses.
Choosing a type again can replace values you previously customized.

A ranged-sounding description alone does not make a weapon ranged. Likewise,
a long polearm description does not give reach: choose a reach weapon type,
such as a halberd or glaive. PCs use a 10-foot range for reach weapons.

Ammo matching is fixed by the weapon type:
  Bow       -> arrows
  Crossbow  -> bolts
  Sling     -> bullets

Crossbows and slings use the reload system. Do not treat ammunition quantity
as a weapon value; quantities belong to the ammunition item.

2. BUILD AN AMMO POUCH OR QUIVER
Create an Ammo Pouch item, not an ordinary Container. Set Take and Ammo-Pouch
wear flags, an appropriate weight, and a positive capacity in C) Values.

The first capacity prompt mentions weight. The same capacity also limits
how many ammo items fit, so allow room for both their weight and their item
count. A multi-shot bundle occupies one item slot. Use a positive capacity;
do not rely on the menu''s -1 wording for unlimited capacity.

The pouch accepts ammunition only. It must be worn to supply a weapon, and
the first item inside must match the weapon. A quiver''s name does not decide
which ammunition it supplies.

3. BUILD AMMUNITION
Create a Missile/ammo item in oedit. Set Take, descriptive keywords, weight,
minimum level, and a cost for the WHOLE supply. It does not need a wield flag.

In C) Values, the editor asks for:
  Ammo type: 1 arrows, 2 bolts, 3 sling bullets, or 5 thrown weapons.
    Type 4 is retained for legacy dart ammunition.
  Breaking probability: chance a missed projectile breaks; 30 is a useful
    ordinary value. This does not change the number consumed per shot.
  Enhancement bonus: 0 for mundane ammunition, up to 10 in the editor.
  Minimum ammunition on load: the smallest quantity a new supply contains.
  Maximum ammunition on load: the largest quantity; at least the minimum.
  Current ammunition: the quantity in the edited item, from 0 to maximum.
  Bonus damage type: choose from the displayed list; 0 disables bonus damage.
  Bonus damage die size: 6 gives +1d6, 8 gives +1d8; 0 disables bonus damage.
  Unlimited ammunition: 0 for a finite supply; 1 for a supply that never
    empties.

Newly loaded supplies receive a quantity between their minimum and maximum.
Use the same number for both to make a fixed-size bundle. Set the current
quantity too when preparing an item for immediate use. A zero current quantity
makes a finite supply with a positive maximum unusable.

Example: a bundle of +2 flaming arrows
  Type: arrows; breaking probability: 30; enhancement: 2.
  Minimum: 20; maximum: 20; current: 20.
  Bonus damage type: fire; die size: 6; unlimited: 0.

Every fresh copy contains 20 arrows. Each committed shot consumes one,
including misses and deflections. A hit can add +1d6 fire, subject to the
foe''s resistance. Elemental bonus damage is not multiplied by critical hits.

Useful bonus damage choices include fire (1), cold (2), acid (5), electricity
(7), and force (28). Use the editor''s names when selecting them.

Unlimited ammunition is a separate property of the AMMO, not the pouch or
weapon. It does not remove a crossbow''s or sling''s reload requirement.
Projectiles from an unlimited supply disappear after their shot resolves;
they do not become free permanent ammunition for players to collect.

For checking the main menu''s value display, positions start at zero:
  0 ammo type                 1 existing imbued spell
  2 breaking probability      3 reserved
  4 enhancement               5 minimum quantity on load
  6 maximum quantity on load  7 current quantity
  8 bonus damage type         9 bonus damage die size
 10 unlimited ammunition
 11 compatible thrown weapon 12 Returning ammunition

Existing ammo with a maximum of 0 remains a single projectile. Convert it
by setting a positive minimum, maximum, and current quantity. Normal supplies
need no imbued spell; use the bonus damage settings for simple flaming or
frost ammunition.

THROWN WEAPON SUPPLIES AND RETURNING
Use the normal Weapon item type for a dagger, shuriken, throwing axe, javelin,
or another weapon type marked for throwing. The player wields that weapon
and uses throw <target>; melee-capable weapons remain usable with hit.

For its ammunition, choose type 5 (thrown weapon). After the Unlimited prompt,
choose the exact compatible weapon type from the displayed throwing list,
then set Returning to 0 or 1. Match dagger to dagger and shuriken to shuriken.
These choices occupy values 11 and 12. Finish all prompts before saving.

Example: shuriken weapon plus a pouch of 50 shuriken
  Weapon: Weapon item, shuriken subtype, chosen enhancement and properties.
  Ammo: Missile item, type 5, min/max/current 50, compatible type shuriken.
  Ammo Returning: 0 for consumable shuriken, 1 for an inexhaustible supply.

Alternatively, add the existing Returning special ability to the equipped
throwing weapon in oedit''s Special Abilities menu. Returning on either source
prevents consumption, with the same immediate availability as unlimited ammo.
It does not create a return timer or limit throws per round. Matching ammo
in a worn pouch is still required, even with a Returning weapon.

The equipped weapon supplies base damage and weapon properties and stays
equipped. Ammo supplies the quantity and optional ammunition bonuses. Use the
higher enhancement of the weapon and ammo, not their sum. Quick Draw permits
normal full-rate throwing; without it, a character can throw once per round.
Returning does not remove that feat requirement.

Weapon range comes from its subtype: for example, dagger 10 feet and javelin
30 feet per increment. Thrown weapons reach five increments; other ranged
weapons reach ten. Additional increments impose -2 each, reduced to -1 by
Far Shot. Room size still bounds all engagements. Test long custom rooms
with the intended weapon; maximum room distance can exceed its weapon range.

Point-Blank Shot is +1 attack and damage within 30 feet, not a front-row
penalty exemption. Precise Shot handles firing into another player''s melee.
Rapid Shot applies to full weapon attacks; Manyshot adds a second arrow to
the first bow attack using the same roll, not an extra independent attack.
The Manyshot feat is distinct from the ranger perk''s active volley command.

4. BUILD AN AMMUNITION VENDOR
Create or select a shopkeeper mob and a shop room. In zedit, arrange for the
keeper to load in that room. Give the keeper starting stock with object-give
resets following its mob reset. For example, stock a bow, an ammo pouch, and
one supply each of arrows, bolts, and bullets.

In sedit <shop-vnum>:
  0) Keeper: select the shopkeeper''s mob vnum.
  R) Rooms Menu: add the room where the keeper will trade.
  P) Products Menu: add the object vnums to be continually produced.
  T) Accept Types Menu: choose the item types the keeper will buy from players.
  Set opening hours, sell/buy rates, and customer restrictions as appropriate.

The product list identifies replenishable merchandise. The keeper also
needs a matching stock item in its inventory for players to find and buy.
A product entry alone does not replace the keeper and inventory resets.
Keep merchandise in the keeper''s carried inventory, not worn as equipment.

Put normal finite ammo supplies in Products for a vendor that keeps selling
fresh bundles. This does NOT require unlimited ammunition on those bundles.
Each new produced purchase uses the ammo''s quantity-on-load settings. Price
the complete bundle, including its enhancement and elemental damage.

For a limited-stock vendor, leave that item off Products and give the keeper
only the stock that should be available. Restocking then follows your resets.
The Accept Types list controls purchases FROM players, not the stock sold TO
players. Save the shop, objects, mobs, and zone reset changes.

5. BUILD A RANGED MOB
In medit, enable Ranged-Attack (MOB_RANGED_ATTACK) if the mob should attack
before closing to melee distance. A mob without it must reach 5 feet first,
even if it has a bow or a reach weapon.

For an armed archer, set up these zone resets in order:
  Load the mob into its room (M).
  Equip the bow in the appropriate wield slot (E).
  Equip an Ammo Pouch in the ammo-pouch slot (E).
  Put matching arrows inside that pouch (P).

For a crossbow or sling, substitute the matching weapon and ammo types.
The P reset identifies its container by the pouch''s vnum; use suitable load
limits and verify which pouch receives the ammunition after a zone reset.
Loose ammo given to the mob''s inventory is not enough. A setup script can
also establish the equipment and contents. Crossbows and slings need a reload
plan as well. Test the actual mob rather than just its inventory list.

A flagged mob wielding a throwable weapon can use its matching thrown ammo
automatically. Add Quick Draw to the mob for full-rate thrown attacks.

The Ranged-Attack flag also permits a mob''s ordinary attacks at range without
a ranged weapon. The flag does not supply ammunition for a bow. Giving a mob
a bow does not automatically give it the Ranged-Attack flag.

All mobs, including flagged ranged mobs and charmies, still advance once
per round at their movement speed, stopping within 5 feet. The flag does not
make an archer stand still. An unarmed flagged mob does not automatically
start far away: opening distance depends on the weapons wielded in the fight.

6. BUILD THE ROOM''S RANGED FACETS
In redit <room-vnum>, use 4) Sector type and H) Room size. Room size is the
maximum distance in FEET, not an exit distance, movement cost, or extra room.

Enter 0 for the terrain default, or a positive number for a custom distance.
An automatic size follows later terrain changes; a custom size remains fixed.
Existing rooms use the terrain default. Room copies preserve the setting.

Default distances:
  50: field, desert, ocean, tundra, beach.
  40: flying.
  30: hills, underwater, roads, dirt roads, planes/outer planes.
  20: forest, mountain, marshland, underdark wilderness, lava, taiga, seaport.
  10: inside, city, water swim/noswim, zone start, high mountain, underdark
      city/inside/water/noswim/no ground, cave, jungle, inside room, river.

For example, a field with the automatic setting opens ranged engagements at
50 feet. A 30-foot-speed mob closes to 20 feet on its first round, then to
5 feet on its next. A 10-foot interior lets that mob close on its first round.
A custom 75-foot courtyard gives the same mob more ground to cross.

Opening distance is the room maximum if either opponent wields a ranged
weapon; otherwise it is 0. Switching weapons does not reset distance.
Formation restrictions still apply. Only weapons with reach let PCs make
melee attacks from 10 feet; ordinary melee requires 5 feet. All targets must
remain in the same room. Room size does not enable shooting through exits.

Peaceful rooms still prohibit combat. Single-file and formation rules still
apply where relevant. There are no separate cover, firing-lane, or sniper
room flags in this system; room size alone does not create those effects.

7. TEST BEFORE RELEASING
Load fresh copies after saving, then test with an ordinary player character.
  Equip the weapon, wear the pouch, put ammo inside, and fire at a test mob.
  Try the wrong ammo type first in the pouch and confirm it blocks firing.
  Fire through a finite bundle, including its last shot; then try reloading.
  Check enchanted damage against a suitable target and inspect the supply.
  Test the vendor with list and repeated purchases; check bundle quantities.
  Reset the zone and confirm the keeper and stock return as intended.
  Check the room size, opening distance, mob advancement, and advance command.
  Compare an ordinary mob with one flagged Ranged-Attack.

See also: RANGED, ADVANCE, FORMATION, OEDIT, MEDIT, SEDIT, ZEDIT, REDIT
', CHAR(10), CONCAT(CHAR(13), CHAR(10))), 31, 1000, FALSE)
ON DUPLICATE KEY UPDATE
  category=VALUES(category), entry=VALUES(entry),
  min_level=VALUES(min_level), max_level=VALUES(max_level), auto_generated=FALSE;

INSERT IGNORE INTO help_keywords(help_tag, keyword) VALUES
('BUILD_RANGED', 'build_ranged'),
('BUILD_RANGED', 'build_ammo'),
('BUILD_RANGED', 'build_archery'),
('BUILD_RANGED', 'ranged_building');


-- Refresh formation help after correcting Point-Blank Shot.
INSERT INTO help_entries(tag, category, entry, min_level, max_level, auto_generated)
VALUES ('FORMATION', 'combat', REPLACE('FORMATION

Arrange your group into front, middle and back rows for tactical combat.
Use formation or group to see the current positions.

Usage:
  formation
  formation <front|middle|back>
  formation <front|middle|back> <member>
  group formation <front|middle|back> [member]

Examples:
  formation front
  formation middle
  formation back
  formation middle companion
  formation back archer

Each member can choose their own row. Only the group leader can position
other members, including companions and mounts. The member must be awake
and in the same room. Changing rows during combat costs the moving member
one move action. Changing to the current row costs nothing.

ROWS AND ATTACKS
  Front  : Melee, reach and ranged weapons; unarmed and natural attacks.
  Middle : Reach weapons and ranged attacks.
  Back   : Ranged attacks only.

Point-Blank Shot grants +1 to ranged attack and damage rolls within 30 feet.
Lacking the feat, standing in a front or middle row, or tanking does not
impose a Point-Blank Shot penalty.
Shooting a mob that is tanked by another player takes -4 without Precise
Shot. Improved Precise Shot also removes that penalty and ignores partial
concealment; total concealment still applies. Ranged attacks stay in one room.

Reach must come from the weapon used for that attack. A reach weapon in
one hand does not let an ordinary weapon in the other hand attack from
the middle row. Natural attacks do not inherit weapon reach.

Contact maneuvers such as kick, headbutt, slam and shield punch require
the front row. Weapon maneuvers follow their weapon restrictions.
Spells, psionic attacks and eldritch blasts keep their normal targeting
rules. Formation does not protect against ranged attacks or area effects.

NPC MELEE REACH
  Medium or smaller : The nearest occupied row.
  Large             : The nearest two rows.
  Huge or larger    : All three rows.

Only living, awake members in the same room protect the rows behind them.
If the front row is empty or unavailable, the middle row becomes exposed
and protects the back row from ordinary NPC melee. If only the back row
remains, it becomes exposed. A large NPC can reach the back row when the
front row is unavailable.

NPC melee attacks aimed at a protected member switch to an accessible,
visible member of that group. Opportunity attacks cannot switch targets;
they fail if the intended target is protected. NPC contact maneuvers also
respect formation protection.

New group members start in the front row. Leaving a group resets your
position. Formation positions are not saved across logins. Ungrouped
characters use the normal combat rules.

See also: GROUP, FOLLOW, RESCUE
', CHAR(10), CONCAT(CHAR(13), CHAR(10))), 0, 1000, FALSE)
ON DUPLICATE KEY UPDATE entry=VALUES(entry), category=VALUES(category),
  min_level=VALUES(min_level), max_level=VALUES(max_level), auto_generated=FALSE;
