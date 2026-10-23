-- Install the Disguise skill and command help entry during deployment.
START TRANSACTION;
INSERT INTO help_entries(tag, category, entry, min_level, max_level, auto_generated)
VALUES ('disguise', 'skills', REPLACE('	g>Description:	n

Disguise changes your visible appearance, allowing you to pass as a humanoid
of your own size. It conceals your identity from observers who fail to see
through it, even if they already know your name. It grants no racial abilities,
attribute changes, or actual change of race.

	g>Usage:	n

  disguise
  disguise <race>
  disguise <race> | <age> | <gender> | <feature:adjective> | <feature:adjective>
  disguise descriptors [feature]
  disguise remove

Type disguise alone to see eligible races and usage information. You must have
training in Disguise to apply an ordinary disguise. Applying or removing one
uses a standard action. Remove your current disguise before applying another.

	g>Appearance Options:	n

Race:    A humanoid race of your current size, chosen from the eligible list.
Age:     adult, adolescent, middle-aged, old-aged, venerable.
Gender:  neutral, male, female.
Features: Up to two feature:adjective pairs. Use none to omit a feature.

Use disguise descriptors to list feature types, or disguise descriptors hair
to list hair choices. Feature names include eyes, nose, ears, face, scar, hair,
build, complexion, fur, skin, horns, and scales. Numeric feature IDs also work.
A race alone uses your natural age, gender, and descriptor choices.

Example:
  disguise human | adolescent | female | hair:43 | eyes:2

	g>Skill Checks:	n

Applying a disguise rolls 1d20 + your Disguise ability once. Each player who
first sees that disguise receives one opposing check of 1d20 + Perception.
A Perception result equal to or greater than your Disguise result sees through
it. Looking again or reconnecting does not grant another check. Removing and
reapplying your disguise creates a new disguise and new checks.

A fooled observer sees your disguised description. An observer who sees through
it sees your known name or natural short description, with a separate line
describing the disguise. Seeing through a disguise does not teach your name.
True Sight automatically reveals disguises without a Perception roll.

Your disguise and observers'' check results persist across reconnects. You cannot
introduce yourself while disguised. Remove your disguise before using wildshape
or another transformation.

	g>Related Magic:	n

Disguise Self grants +10 to your Disguise ability. It does not create a visible
disguise by itself; apply one with the disguise command to use the bonus.
Alter Self changes your active race and is separate from an ordinary disguise.

  disguise dismiss disguise self
  disguise dismiss alter self

These commands dismiss the named spell and use a standard action. Removing an
ordinary disguise does not itself dismiss these spells.

	g>Racial Forms:	n

Irda and Sivak form changes use the same command but are racial abilities,
requiring neither Disguise training nor an opposed Perception check.

  disguise form <race> | <age> | <gender> | <feature:adjective> | <feature:adjective>
      Irda: assume a humanoid appearance.
  disguise forms
      Sivak: list appearances learned from humanoids you personally killed.
  disguise form <number>
      Sivak: assume a learned appearance.
  disguise form remove
      Irda or Sivak: return to your natural form.

True Sight reveals both natural and assumed forms. See the racial help entries
for full requirements and restrictions.

	ySee also:	n DISGUISESELF, ALTERSELF, TRUEFORM, INTRODUCE, IRDA, SIVAK
', CHAR(10), CONCAT(CHAR(13), CHAR(10))), 0, 1000, FALSE)
ON DUPLICATE KEY UPDATE category=VALUES(category), entry=VALUES(entry),
  min_level=VALUES(min_level), max_level=VALUES(max_level), auto_generated=VALUES(auto_generated);

INSERT IGNORE INTO help_keywords(help_tag, keyword) VALUES
('disguise', 'disguise'),
('disguise', 'disguises'),
('disguise', 'skill-disguise');
COMMIT;
