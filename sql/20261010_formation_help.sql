-- Install or update the manual FORMATION help entry.
-- Apply to the game database after the normal help schema exists.
-- Text mirrors lib/text/help/formation.hlp; line endings are stored as CRLF.
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

Front-row ranged attacks take -4 without Point Blank Shot and another -4
without Precise Shot. Improved Precise Shot also removes the second penalty.
These penalties stack: lacking both feats gives -8. You can still shoot
while being attacked in melee. Normal feat bonuses still apply.

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
', CHAR(10), CONCAT(CHAR(13), CHAR(10))),
        0, 1000, FALSE)
ON DUPLICATE KEY UPDATE
  category = VALUES(category), entry = VALUES(entry),
  min_level = VALUES(min_level), max_level = VALUES(max_level), auto_generated = FALSE;

INSERT IGNORE INTO help_keywords(help_tag, keyword) VALUES
('FORMATION', 'formation'),
('FORMATION', 'formations'),
('FORMATION', 'groupformation');
