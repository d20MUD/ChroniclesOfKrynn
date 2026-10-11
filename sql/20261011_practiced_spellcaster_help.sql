-- Install the Practiced Spellcaster help entry during deployment.
INSERT INTO help_entries(tag, category, entry, min_level, max_level, auto_generated)
VALUES ('PRACTICEDSPELLCASTER', 'general', REPLACE('PRACTICED SPELLCASTER

Practiced Spellcaster increases your caster level by up to four for a
chosen spellcasting class. Its bonus cannot raise caster level above your
Hit Dice, represented by your total character level.

Select Practiced Spellcaster from the feats menu in study, confirm it,
then choose a class by number or name. You must possess the chosen class
(or be gaining its first level). Each choice costs one feat point. You may
take the feat again for a different spellcasting class, once per class.
Your selections appear in feats and are saved with your character.

Example: a level 10 character with caster level 6 can gain the full +4.
A level 8 character with caster level 6 gains only +2. The remaining bonus
becomes available automatically as the character gains more Hit Dice.

Existing prestige-class spellcasting advancement counts toward the cap.
A character whose caster level already equals Hit Dice gains no increase.

This feat improves caster-level-dependent spell effects and penetration.
It does not grant higher spell circles, spell slots, spells known, class
levels, or manifester levels. It does not increase scroll, wand, staff,
potion, or device caster levels. Existing game caster-level limits apply.

Equipment granting this feat must specify the spellcasting class. Its
bonus applies only when you have levels in that class. Multiple items or
a learned copy of the feat do not stack above the single +4 benefit.

See also: STUDY, FEATS, SPELLCRAFT, SPELLS
', CHAR(10), CONCAT(CHAR(13), CHAR(10))), 0, 1000, FALSE)
ON DUPLICATE KEY UPDATE
  category = VALUES(category), entry = VALUES(entry),
  min_level = VALUES(min_level), max_level = VALUES(max_level), auto_generated = FALSE;
INSERT IGNORE INTO help_keywords(help_tag, keyword) VALUES
('PRACTICEDSPELLCASTER', 'practicedspellcaster'),
('PRACTICEDSPELLCASTER', 'practiced-spellcaster'),
('PRACTICEDSPELLCASTER', 'practiced spellcaster');
