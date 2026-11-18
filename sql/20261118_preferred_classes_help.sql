-- Preferred class perk routing help; safe to reapply.
START TRANSACTION;
INSERT INTO help_entries(tag, category, entry, min_level, max_level, auto_generated)
VALUES ('PREFERRED_CLASSES', 'general', REPLACE('PREFERRED CLASSES

Use STUDY, option A (Preferred Classes), to select:
  Arcane:  Wizard, Sorcerer, Bard or Summoner.
  Divine:  Cleric, Druid or Inquisitor.
  Martial: Warrior, Berserker, Paladin, Blackguard, Ranger or Monk.

When a class has two distinct perk-tree assignments, each assignment uses
its matching preferred class. Arcane Archer uses the preferred arcane and
martial classes. Mystic Theurge uses preferred arcane and divine classes.
Duelist uses preferred martial plus its fixed Rogue assignment. Sacred
Fist uses preferred martial and divine classes.

Examples for Arcane Archer:
  Sorcerer + Warrior preferences: one point in each of those perk trees.
  Wizard + Ranger preferences: one point in each of those perk trees.
You may select these preferences without already having levels in them.

An unset preference is selected automatically when you have class ranks in
exactly one eligible class in that category. For example, Sorcerer/Warrior
selects Sorcerer for arcane and Warrior for martial. With no ranked class
or multiple ranked classes in a category, choose in Study. Existing valid
choices are retained when you gain another class or change preferences.

Set any remaining preferences required by the prestige class before advancing it.
Unset or invalid required preferences block GAIN and pause that class''s
perk-stage advancement. XP is retained. After selecting the preferences,
your earned XP stages can advance normally; no partial awards are lost.
Only the preferences used by that class are required, not all three.

The existing award timing is unchanged: two perk points per completed
quarter-level XP stage (stages 1-3). Classes with a single perk-tree
assignment continue receiving both points in that tree. Loremaster keeps
its existing spellcasting-progression routing. Dragon Disciple retains
its two-highest-base-class selection, applying preferences when those
assignments differ.

Choices are saved and can be changed for future awards. Already earned
points and purchased perks are retained and remain accessible. Historical
legacy-point compensation retains its existing routing.

See also: STUDY, GAIN, PERKS', CHAR(10), CONCAT(CHAR(13), CHAR(10))), 0, 1000, FALSE)
ON DUPLICATE KEY UPDATE category=VALUES(category), entry=VALUES(entry),
  min_level=VALUES(min_level), max_level=VALUES(max_level), auto_generated=FALSE;
INSERT IGNORE INTO help_keywords(help_tag, keyword) VALUES
('PREFERRED_CLASSES', 'preferred-classes'),
('PREFERRED_CLASSES', 'preferred-class'),
('PREFERRED_CLASSES', 'preferred-martial');
COMMIT;
