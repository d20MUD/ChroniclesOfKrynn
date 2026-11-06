-- Install combat ADVANCE help. Apply after the normal help schema exists.
INSERT INTO help_entries(tag, category, entry, min_level, max_level, auto_generated)
VALUES ('COMBAT_ADVANCE', 'combat', REPLACE('ADVANCE

Usage: advance [opponent]

Advance toward a mob you are fighting in the same room. With no target,
you advance toward your current opponent. Advancing costs one move action
and closes the distance by up to your current movement speed.

Ordinary melee attacks require a distance of 5 feet or less. Reach weapons,
such as polearms flagged for reach, allow PCs to attack from up to 10 feet.
Advance stops at the range of your currently wielded weapon. Unarmed and
natural attacks do not inherit a polearm''s reach. Formation restrictions
still apply.

Combat begins at the room''s maximum distance if either opponent wields a
ranged weapon. Otherwise, it begins at 0 feet. Changing weapons during a
fight does not reset this distance. Each opponent has its own distance.

All mobs, including charmies, automatically advance toward their opponents
once per round at their current movement speed, stopping within 5 feet.
Mobs without the Ranged-Attack flag must close to 5 feet before attacking.
Mobs with this flag may attack at range, but still advance each round.
A mob wielding a ranged weapon still needs matching ammunition.

Stunning, paralysis, grappling, entanglement, grounding, and other movement
restrictions can prevent advancing. Attacks and movement stay in one room.

For authorized staff, advance outside combat retains its level-setting use.

See also: FORMATION, FIRE, FLEE
', CHAR(10), CONCAT(CHAR(13), CHAR(10))), 0, 1000, FALSE)
ON DUPLICATE KEY UPDATE category=VALUES(category), entry=VALUES(entry),
  min_level=VALUES(min_level), max_level=VALUES(max_level), auto_generated=FALSE;

INSERT IGNORE INTO help_keywords(help_tag, keyword) VALUES
('COMBAT_ADVANCE', 'advance'),
('COMBAT_ADVANCE', 'combat_advance'),
('COMBAT_ADVANCE', 'combat_distance');
