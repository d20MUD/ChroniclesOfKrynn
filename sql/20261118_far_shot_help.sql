-- Far Shot effective weapon increments; safe to reapply.
START TRANSACTION;
INSERT INTO help_entries(tag, category, entry, min_level, max_level, auto_generated)
VALUES ('FAR_SHOT', 'general', REPLACE('FAR SHOT

Prerequisite: Point-Blank Shot. Passive combat feat; one rank.

Far Shot improves physical ranged weapons using their subtype''s range:
  Projectile weapons: range increment increases by 50%, rounded down.
  Thrown weapons: range increment doubles.
Beyond the first effective increment, each additional increment gives
-1 to hit instead of the normal -2. The first increment is inclusive.

Maximum range uses the increased increment too: ten increments for
projectile weapons, five for thrown weapons. The maximum is inclusive.
For example:
  Bow with a 100-foot increment: 150 feet per increment, 1500 feet maximum.
  Thrown dagger with a 10-foot increment: 20 feet per increment, 100 feet maximum.
  Bow at 150 feet: no range penalty; at 151 feet: -1; at 301 feet: -2.

Targets must be in the same room; room size still bounds combat distance.
The feat does not increase melee reach or the range of spells/powers.
Point-Blank Shot retains its 30-foot limit. Normal ammunition, reload,
throwing-mode and attack-action requirements still apply.

See also: RANGED, THROW, POINT-BLANK-SHOT, FEATS', CHAR(10), CONCAT(CHAR(13), CHAR(10))), 0, 1000, FALSE)
ON DUPLICATE KEY UPDATE category=VALUES(category), entry=VALUES(entry),
  min_level=VALUES(min_level), max_level=VALUES(max_level), auto_generated=FALSE;
INSERT IGNORE INTO help_keywords(help_tag, keyword) VALUES
('FAR_SHOT', 'far shot'), ('FAR_SHOT', 'far-shot'), ('FAR_SHOT', 'farshot');
UPDATE help_entries SET entry=REPLACE(entry,
  'Far Shot reduces this to -1.',
  'Far Shot reduces this to -1 and extends weapon range increments (see HELP FAR SHOT).')
WHERE entry LIKE '%Far Shot reduces this to -1.%';
COMMIT;
