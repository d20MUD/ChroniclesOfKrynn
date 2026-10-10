-- Install or update the manual STATS help entry.
-- Apply to the game database after the normal help schema exists.
INSERT INTO help_entries(tag, category, entry, min_level, max_level, auto_generated)
VALUES ('STATS', 'general', REPLACE('STATS

Usage: stats

Show your current statistics and the caps that apply to your character.
Caps use your base attributes, current class levels and active effects.

The report includes:
  STR, DEX, CON, INT, WIS and CHA.
  Attack bonus for your current main-hand weapon or unarmed attacks.
  Armor class, hitroll, damroll and total damage-bonus limits.
  Spell resistance and Fortitude, Reflex, Will, Poison and Death saves.
  Maximum PSP, movement, hit points, concealment and damage reduction.
  Energy absorption.

Attack and armor values can depend on your current opponent. Front-row
ranged penalties are included in the ranged attack-bonus cap.

Hitroll and damroll are component statistics, not your complete attack or
damage bonus. The total damage-bonus cap excludes weapon dice. Reduction
and absorption caps apply per damage type. A -- in the Current column
means only the cap is shown. Hit points have no fixed global cap.

These are your limits now, not predictions for future levels, feats or
buffs. Viewing stats does not change your attributes or consume effects.
The older statcap command remains available.

Auction statistics are available through ah stats <listing-id>.

See also: SCORE, STATCAP, ATTRIBUTES, RESISTANCES, FORMATION
', CHAR(10), CONCAT(CHAR(13), CHAR(10))),
        0, 1000, FALSE)
ON DUPLICATE KEY UPDATE
  category = VALUES(category), entry = VALUES(entry),
  min_level = VALUES(min_level), max_level = VALUES(max_level), auto_generated = FALSE;
INSERT IGNORE INTO help_keywords(help_tag, keyword) VALUES
('STATS', 'stats'), ('STATS', 'statlimits');
