-- Add the three PF1 sorcerer bloodlines and their player command help.
-- Apply during deployment; safe to run again.

INSERT INTO help_entries(tag, category, entry, min_level, max_level, auto_generated)
VALUES ('SORCERER-BLOODLINES', 'general', REPLACE('SORCERER-BLOODLINES

Sorcerers choose one bloodline in study. The choice screen shows its
benefits before asking you to accept or reject it. Powers unlock at
sorcerer levels 1, 3, 9, 15 and 20. Dragon Disciples must choose Draconic.

The bloodline command lists unlocked Shadow, Stormborn or Destined powers
and remaining daily uses. Use feat info <power name> for exact effects.
These bloodlines also grant a class skill and bloodline bonus feat choices.
Their bonus spells become known for free at levels 3, 5, 7, 9, 11, 13, 15,
17 and 19, without using study spell selections. Spells use normal slots.

Bloodline powers follow normal combat, group, peaceful-room and movement
rules. Active powers normally take a standard action. Destined reroll and
penetration take a swift action; ride-the-lightning takes a full round.
Daily uses recover through the game''s normal daily-use system and their
cooldowns are saved across logout.

See also: SHADOW-BLOODLINE, STORMBORN-BLOODLINE, DESTINED-BLOODLINE,
STUDY, FEATS, SPELLS
', CHAR(10), CONCAT(CHAR(13), CHAR(10))), 0, 1000, FALSE)
ON DUPLICATE KEY UPDATE category=VALUES(category), entry=VALUES(entry),
  min_level=VALUES(min_level), max_level=VALUES(max_level), auto_generated=FALSE;

INSERT IGNORE INTO help_keywords(help_tag, keyword) VALUES
('SORCERER-BLOODLINES', 'sorcerer-bloodlines'),
('SORCERER-BLOODLINES', 'bloodlines'),
('SORCERER-BLOODLINES', 'bloodline');

INSERT INTO help_entries(tag, category, entry, min_level, max_level, auto_generated)
VALUES ('SHADOW-BLOODLINE', 'general', REPLACE('SHADOW-BLOODLINE

Your magic draws on Krynn''s shadows and the mysteries of the Shadow Plane.
Class skill: Stealth.

Level 1: Shadow arcana grants a Stealth bonus equal to spell circle for
1d4 rounds after casting darkness, shadow shield, shadow walk or shadow
jump. Shadowstrike is a nonlethal melee touch, usable 3 + Charisma bonus
(minimum 1) times per day, dealing 1d4 + half sorcerer level damage.
Targets lacking darkvision or low-light vision are dazzled for 10 rounds.
Nonlethal damage cannot reduce a target below 1 HP; undead and constructs
are immune.
Level 3: Nighteye grants sight in natural darkness.
Level 9: Shadow well permits hiding in combat in darkness and can rescue
a grouped ally in the same dark room by drawing its attackers to you.
Neither participant can be held or entangled. Uses: 1/day, 2 at level 17,
3 at level 20. This is the room-based version of exchanging positions.
Level 15: Enveloping darkness darkens the room for 15 rounds and entangles
legal enemies without freedom of movement for that duration. You see
through the darkness. Once per day; normal area containment rules apply.
Level 20: Shadow bloodline master grants sight through magical darkness,
+2 illusion spell DC and 20% longer shadow shield duration. These replace
the tabletop bonuses to shadow-conjuration realism.

Commands:
  bloodline shadowstrike <enemy>
  bloodline shadow-well <group ally>
  bloodline enveloping-darkness

Free bonus spells, in level order: ray of enfeeblement, infravision,
darkness, greater invisibility, shadow shield, shadow walk, power word
blind, scintillating pattern, weird. Some spells substitute for tabletop
spells absent from the game''s spell catalogue.

Bonus feat choices: Acrobatic, Blind Fight, Dodge, Quick Draw, Silent
Spell, Skill Focus, Stealthy, Weapon Finesse.
See also: BLOODLINES, STEALTH, STUDY, FEATS
', CHAR(10), CONCAT(CHAR(13), CHAR(10))), 0, 1000, FALSE)
ON DUPLICATE KEY UPDATE category=VALUES(category), entry=VALUES(entry),
  min_level=VALUES(min_level), max_level=VALUES(max_level), auto_generated=FALSE;

INSERT IGNORE INTO help_keywords(help_tag, keyword) VALUES
('SHADOW-BLOODLINE', 'shadow-bloodline'),
('SHADOW-BLOODLINE', 'shadowstrike'),
('SHADOW-BLOODLINE', 'nighteye'),
('SHADOW-BLOODLINE', 'shadow-well'),
('SHADOW-BLOODLINE', 'enveloping-darkness');

INSERT INTO help_entries(tag, category, entry, min_level, max_level, auto_generated)
VALUES ('STORMBORN-BLOODLINE', 'general', REPLACE('STORMBORN-BLOODLINE

Storm spirits and Krynn''s elemental skies awaken in your blood.
Class skill: Survival (the game''s equivalent of Knowledge [nature]).

Level 1: Stormborn arcana adds +1 to electricity and sonic spell DCs.
Thunderstaff grants a grouped ally''s wielded melee weapons +1d6 electricity
for half sorcerer level rounds (minimum 1). From level 9, a critical adds
1d10 more electricity, but duration is halved. Uses: 3 + Charisma bonus
(minimum 1) per day. No target means yourself.
Level 3: Stormchild grants electricity and sonic resistance 5 and protects
against gust-of-wind knockdown. From level 9 it removes fog/mist combat
concealment and allows looking through fog.
Level 9: Thunderbolt strikes legal enemies in your room for 1d6 damage
per sorcerer level (up to 30d6), split electricity/sonic. Reflex DC 10 +
half sorcerer level + Charisma bonus halves damage; failed saves deafen
for one round. Uses: 1/day, 2 at level 17, 3 at level 20.
Level 15: Ride the lightning moves through one open adjacent exit and
releases a thunderbolt on arrival. Once per day, separate from thunderbolt.
You must be out of combat. Doors, hidden exits, peaceful rooms and normal
movement restrictions still apply. Area containment settings still apply.
Level 20: Storm lord grants stun/deafness immunity and weather sight.
The first electricity or sonic damage packet each day is absorbed and
heals one third of its damage after other defenses. This is automatic.

Commands:
  bloodline thunderstaff [group ally]
  bloodline thunderbolt
  bloodline ride-the-lightning <direction>

Free bonus spells, in level order: shocking grasp, gust of wind,
lightning bolt, thunderclap, overland flight, chain lightning, control
weather, whirlwind, storm of vengeance. Thunderclap replaces shout.
Bonus feat choices: Deadly Aim, Dodge, Enlarge Spell, Far Shot, Great
Fortitude, Point Blank Shot, Skill Focus, Mobility (replaces Wind Stance).
See also: BLOODLINES, STUDY, FEATS
', CHAR(10), CONCAT(CHAR(13), CHAR(10))), 0, 1000, FALSE)
ON DUPLICATE KEY UPDATE category=VALUES(category), entry=VALUES(entry),
  min_level=VALUES(min_level), max_level=VALUES(max_level), auto_generated=FALSE;

INSERT IGNORE INTO help_keywords(help_tag, keyword) VALUES
('STORMBORN-BLOODLINE', 'stormborn-bloodline'),
('STORMBORN-BLOODLINE', 'thunderstaff'),
('STORMBORN-BLOODLINE', 'stormchild'),
('STORMBORN-BLOODLINE', 'thunderbolt'),
('STORMBORN-BLOODLINE', 'ride-the-lightning'),
('STORMBORN-BLOODLINE', 'storm-lord');

INSERT INTO help_entries(tag, category, entry, min_level, max_level, auto_generated)
VALUES ('DESTINED-BLOODLINE', 'general', REPLACE('DESTINED-BLOODLINE

Prophecy, auspicious signs and the threads of fate protect your future.
Class skill: History.

Level 1: Casting a personal-range sorcerer spell grants its spell circle
as a luck bonus to saves for one round. This does not stack with Fated.
Touch of destiny grants yourself or a grouped ally an insight bonus to
attacks, saves and skill checks equal to half sorcerer level (minimum 1)
for one round. Uses: 3 + Charisma bonus (minimum 1) per day.
Level 3: Fated grants +1 luck to saves, and to AC while flat-footed or
unaware of an attack. Increases at levels 7, 11, 15 and 19, maximum +5.
Level 9: It was meant to be readies a reroll of your next attack,
critical confirmation or spell-resistance check, keeping the second
result even if worse. Uses: 1/day, 2 at level 17. Readiness lasts 10 rounds.
Level 15: Within reach automatically offers a DC 20 Will save against
lethal damage once per day. Success leaves you alive at 1 HP. A failed
save also spends the use. This adapts tabletop stabilization to the MUD.
Level 20: Destiny realized requires a natural 20 to confirm criticals
against you and automatically confirms your spell critical threats.
You can also ready automatic success on your next spell-resistance
check once per day. Readiness lasts 10 rounds.

Commands:
  bloodline touch-of-destiny [group ally]
  bloodline reroll
  bloodline penetration

Free bonus spells, in level order: true strike, blur, protection from
energy, free movement, greater dispelling, greater heroism, spell turning,
protection from spells, timestop. True strike, greater dispelling, greater
heroism, protection from spells and timestop substitute for alarm, break
enchantment, mislead, moment of prescience and foresight respectively.
Bonus feat choices: Combat Casting (replaces Arcane Strike), Diehard,
Endurance, Leadership, Lightning Reflexes, Maximize Spell, Skill Focus,
Weapon Focus.
See also: BLOODLINES, STUDY, FEATS
', CHAR(10), CONCAT(CHAR(13), CHAR(10))), 0, 1000, FALSE)
ON DUPLICATE KEY UPDATE category=VALUES(category), entry=VALUES(entry),
  min_level=VALUES(min_level), max_level=VALUES(max_level), auto_generated=FALSE;

INSERT IGNORE INTO help_keywords(help_tag, keyword) VALUES
('DESTINED-BLOODLINE', 'destined-bloodline'),
('DESTINED-BLOODLINE', 'touch-of-destiny'),
('DESTINED-BLOODLINE', 'fated'),
('DESTINED-BLOODLINE', 'it-was-meant-to-be'),
('DESTINED-BLOODLINE', 'within-reach'),
('DESTINED-BLOODLINE', 'destiny-realized');
