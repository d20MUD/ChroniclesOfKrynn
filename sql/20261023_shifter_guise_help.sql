-- Install help for the automatic Shifter Guise class feat.
START TRANSACTION;
INSERT INTO help_entries(tag, category, entry, min_level, max_level, auto_generated)
VALUES ('shifter-guise', 'classes', REPLACE('	g>Description:	n

Shifter Guise is an automatic class feat granted at shifter level 1. It lets
you cast Disguise Self and Alter Self at will, without preparing either spell
or using spell slots. Existing shifters receive the feat when they log in.
Disguise is a shifter class skill.

	g>Usage:	n

  cast ''disguise self''
  cast ''alter self'' <player-selectable race>

Use your total character level as caster level for these abilities. Casting
still uses the spells'' normal actions and restrictions. At will means there
is no daily use limit; it does not make the spells instantaneous or permanent.
Prepared copies from your other classes are not consumed by Shifter Guise.

	g>Disguise Self:	n

Grants +10 to your Disguise ability for the normal spell duration. It does not
choose an appearance for you. Use disguise to apply your visible disguise.

  disguise human | adult | female | hair:43 | eyes:2
  disguise remove

Observers still use the normal Perception rules to see through a disguise.

	g>Alter Self:	n

Changes your active race for the normal spell duration, with the usual size
bonuses and eligible race choices. It does not grant the racial abilities of
your chosen race. Normal restrictions on combining transformations apply.

True Sight can reveal your natural form. You can dismiss either spell early:

  disguise dismiss disguise self
  disguise dismiss alter self

	ySee also:	n SHIFTER, DISGUISE, DISGUISESELF, ALTERSELF, TRUEFORM
', CHAR(10), CONCAT(CHAR(13), CHAR(10))), 0, 1000, FALSE)
ON DUPLICATE KEY UPDATE category=VALUES(category), entry=VALUES(entry),
  min_level=VALUES(min_level), max_level=VALUES(max_level), auto_generated=VALUES(auto_generated);

INSERT IGNORE INTO help_keywords(help_tag, keyword) VALUES
('shifter-guise', 'shifterguise'),
('shifter-guise', 'shifter-guise'),
('shifter-guise', 'shifter guise');
COMMIT;
