-- Add Abyssal, Elemental and Celestial help and refresh the bloodline overview.
-- Apply after 20261117_sorcerer_bloodlines_help.sql; safe to run again.

INSERT INTO help_entries(tag, category, entry, min_level, max_level, auto_generated)
VALUES ('ABYSSAL-BLOODLINE', 'general', REPLACE('ABYSSAL-BLOODLINE

Demonic ancestry connects your innate magic with the Abyss. Your alignment
remains your own. Class skill: Arcana.

Level 1: Summoned creatures gain DR equal to half sorcerer level (minimum
1), bypassed by good-aligned attacks. This uses normal non-stacking DR.
Use bloodline abyssal-claws <enemy> with both hands free for two melee
claw attacks as a full-round action. Each hit deals 1d4 + Strength bonus
slashing damage; 1d6 from level 7. Claws count as magical from level 5;
each hit adds 1d6 fire from level 11. Smaller bodies use smaller dice.
Uses: 3 + Charisma bonus per day, minimum 1.
Level 3: Electricity resistance 5 and +2 against poison; 10 and +4 at 9.
Level 9: Inherent Strength +2, +4 at 13, +6 at 17. Normal caps apply;
other inherent Strength bonuses do not stack.
Level 15: Summon creature spells calling evil-subtype creatures produce
one extra creature, subject to normal follower limits.
Level 20: Electricity/poison immunity, acid/cold/fire resistance 10, and
universal speech/comprehension as the MUD adaptation of telepathy.

Free spells at levels 3/5/7/9/11/13/15/17/19: cause fear, strength, rage,
stoneskin, dismissal, transformation, teleport, protection from spells,
summon creature IX. Protection from spells replaces unholy aura.
Bonus feat choices: Augment Summoning, Cleave, Empower Spell, Great
Fortitude, Improved Bull Rush, Improved Sunder, Power Attack, Skill Focus.

Summon wards are reapplied to restored living conjured companions using
the owner''s current sorcerer level. They do not grant bonuses to bought
pets, animated undead or constructs on restore.
See also: SORCERER-BLOODLINES, STUDY, FEATS, SPELLS
', CHAR(10), CONCAT(CHAR(13), CHAR(10))), 0, 1000, FALSE)
ON DUPLICATE KEY UPDATE category=VALUES(category), entry=VALUES(entry),
  min_level=VALUES(min_level), max_level=VALUES(max_level), auto_generated=FALSE;

INSERT IGNORE INTO help_keywords(help_tag, keyword) VALUES
('ABYSSAL-BLOODLINE', 'abyssal-bloodline'),
('ABYSSAL-BLOODLINE', 'abyssal'),
('ABYSSAL-BLOODLINE', 'abyssal-claws'),
('ABYSSAL-BLOODLINE', 'demonic-might');

INSERT INTO help_entries(tag, category, entry, min_level, max_level, auto_generated)
VALUES ('ELEMENTAL-BLOODLINE', 'general', REPLACE('ELEMENTAL-BLOODLINE

Choose a permanent heritage in study before reviewing and confirming it:
Air: electricity; Earth: acid; Fire: fire; Water: cold. Class skill: Arcana.

Level 1: bloodline energy-conversion [on/off] toggles conversion of your
sorcerer spells'' fire/cold/acid/electricity/sonic damage to heritage energy.
Burning hands and scorching ray always use your heritage energy.
bloodline elemental-ray <enemy> deals 1d6 + half sorcerer level energy
damage on a ranged touch. Uses: 3 + Charisma bonus/day, minimum 1.
Level 3: Heritage energy resistance 10, increasing to 20 at level 9.
Level 9: bloodline elemental-blast deals 1d6 per sorcerer level (cap 30d6)
to legal enemies in your room. Reflex halves, DC 10 + half sorcerer level
+ Charisma bonus. Failed saves cause 50% vulnerability to that energy
for 2 rounds, after the blast. Uses: 1/day, 2 at 17, 3 at 20.
Level 15: Air grants flight at speed 60; Fire adds 30 ground speed;
Water grants natural swimming at speed 60; Earth reduces land movement
cost to 1 and ignores difficult terrain. Earth uses legal exits and
respects doors; Water does not grant water breathing or bypass boat-only
sectors. Normal movement restrictions still apply.
Level 20: Immunity to heritage energy, critical hits and sneak attacks.

Free spells at levels 3/5/7/9/11/13/15/17/19: burning hands, scorching ray,
protection from energy, stoneskin, fire shield, transformation, spell
mantle, summon creature VIII, elemental swarm. Stoneskin, fire shield,
transformation and spell mantle replace unavailable elemental body forms.
Bonus feats: Dodge, Empower Spell, Great Fortitude, Improved Initiative,
Lightning Reflexes, Power Attack, Skill Focus, Weapon Finesse.
See also: SORCERER-BLOODLINES, STUDY, FEATS, SPELLS
', CHAR(10), CONCAT(CHAR(13), CHAR(10))), 0, 1000, FALSE)
ON DUPLICATE KEY UPDATE category=VALUES(category), entry=VALUES(entry),
  min_level=VALUES(min_level), max_level=VALUES(max_level), auto_generated=FALSE;

INSERT IGNORE INTO help_keywords(help_tag, keyword) VALUES
('ELEMENTAL-BLOODLINE', 'elemental-bloodline'),
('ELEMENTAL-BLOODLINE', 'elemental'),
('ELEMENTAL-BLOODLINE', 'elemental-ray'),
('ELEMENTAL-BLOODLINE', 'elemental-blast'),
('ELEMENTAL-BLOODLINE', 'energy-conversion');

INSERT INTO help_entries(tag, category, entry, min_level, max_level, auto_generated)
VALUES ('CELESTIAL-BLOODLINE', 'general', REPLACE('CELESTIAL-BLOODLINE

Inherited celestial influence connects you with Krynn''s celestial servants.
This is innate magic; no deity or alignment is required. Class skill: Heal.

Level 1: Summoned creatures gain DR equal to half sorcerer level (minimum
1), bypassed by evil-aligned attacks. This uses normal non-stacking DR.
bloodline heavenly-fire <target> deals 1d4 + half sorcerer level holy
damage to an evil enemy on a ranged touch. Native holy defenses apply.
It instead heals yourself if good, or a good grouped ally. Neutral targets
are unaffected. Healing is limited to once per recipient per game day
across all casters; player-owned companions share their owner''s receipt
limit, which survives logout. Uses: 3 + Charisma bonus/day, minimum 1.
Level 3: Acid/cold resistance 5, increasing to 10 at level 9.
Level 9: bloodline wings grants speed-60 flight for 10 rounds. Uses per
day equal sorcerer level. bloodline wings off retracts them.
Level 15: bloodline conviction arms a reroll of your next attack, skill
check or save within 10 rounds. Keep the second result, even if worse.
Once per day; arming takes a swift action before the roll.
Level 20: Acid/cold immunity, fire/electricity resistance 10, +4 on poison
saves, universal speech/comprehension, and unlimited wings.

Free spells at levels 3/5/7/9/11/13/15/17/19: bless, resist energy, magic
circle against evil, remove curse, flame strike, greater dispelling,
banishment, sunburst, gate.
Bonus feats: Dodge, Extend Spell, Iron Will, Mobility, Mounted Combat,
Ride-By Attack, Skill Focus, Weapon Finesse.
Summon wards are reapplied to restored living conjured companions at the
owner''s current sorcerer level; bought pets, undead and constructs are
excluded on restore.
See also: SORCERER-BLOODLINES, STUDY, FEATS, SPELLS
', CHAR(10), CONCAT(CHAR(13), CHAR(10))), 0, 1000, FALSE)
ON DUPLICATE KEY UPDATE category=VALUES(category), entry=VALUES(entry),
  min_level=VALUES(min_level), max_level=VALUES(max_level), auto_generated=FALSE;

INSERT IGNORE INTO help_keywords(help_tag, keyword) VALUES
('CELESTIAL-BLOODLINE', 'celestial-bloodline'),
('CELESTIAL-BLOODLINE', 'celestial'),
('CELESTIAL-BLOODLINE', 'heavenly-fire'),
('CELESTIAL-BLOODLINE', 'wings-of-heaven'),
('CELESTIAL-BLOODLINE', 'conviction');

INSERT INTO help_entries(tag, category, entry, min_level, max_level, auto_generated)
VALUES ('SORCERER-BLOODLINES', 'general', REPLACE('SORCERER-BLOODLINES

Sorcerers choose one bloodline in study. The choice screen shows its
benefits before asking you to accept or reject it. Powers unlock at
sorcerer levels 1, 3, 9, 15 and 20. Dragon Disciples must choose Draconic.

The bloodline command lists unlocked Shadow, Stormborn, Destined, Abyssal,
Elemental and Celestial powers and remaining daily uses. Use feat info <power name> for exact effects.
These bloodlines also grant a class skill and bloodline bonus feat choices.
Their bonus spells become known for free at levels 3, 5, 7, 9, 11, 13, 15,
17 and 19, without using study spell selections. Spells use normal slots.

Bloodline powers follow normal combat, group, peaceful-room and movement
rules. Active powers normally take a standard action. Destined reroll and
penetration take a swift action; ride-the-lightning takes a full round.
Daily uses recover through the game''s normal daily-use system and their
cooldowns are saved across logout.

See also: SHADOW-BLOODLINE, STORMBORN-BLOODLINE, DESTINED-BLOODLINE,
ABYSSAL-BLOODLINE, ELEMENTAL-BLOODLINE, CELESTIAL-BLOODLINE,
STUDY, FEATS, SPELLS
', CHAR(10), CONCAT(CHAR(13), CHAR(10))), 0, 1000, FALSE)
ON DUPLICATE KEY UPDATE category=VALUES(category), entry=VALUES(entry),
  min_level=VALUES(min_level), max_level=VALUES(max_level), auto_generated=FALSE;

INSERT IGNORE INTO help_keywords(help_tag, keyword) VALUES
('SORCERER-BLOODLINES', 'sorcerer-bloodlines'),
('SORCERER-BLOODLINES', 'bloodlines'),
('SORCERER-BLOODLINES', 'bloodline');
