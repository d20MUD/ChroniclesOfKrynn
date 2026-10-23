-- Install help for the poison protection spells during deployment.
INSERT INTO help_entries(tag, category, entry, min_level, max_level, auto_generated)
VALUES
('PROTECTIONFROMPOISON', 'spells', REPLACE('PROTECTION FROM POISON

Cleric 2 / Druid 2 / Inquisitor 2
School: Abjuration
Target: One character in the room
Duration: 10 minutes per caster level

Grants +5 to saving throws against poison, including poison attacks that
use Fortitude saves. This spell does not remove existing poison or grant
immunity. Recasting does not stack the bonus.

Usage: cast \'protection from poison\' <target>
', CHAR(10), CONCAT(CHAR(13), CHAR(10))), 0, 0, FALSE),
('PANACEA', 'spells', REPLACE('PANACEA

Cleric 5 / Druid 5
School: Conjuration
Target: One character in the room
Duration: 1 round per caster level

Removes poison and disease from the target and grants temporary immunity
to poison and disease. May also be cast on a healthy target for protection.

Usage: cast \'panacea\' <target>
', CHAR(10), CONCAT(CHAR(13), CHAR(10))), 0, 0, FALSE)
ON DUPLICATE KEY UPDATE category=VALUES(category), entry=VALUES(entry),
  min_level=VALUES(min_level), max_level=VALUES(max_level), auto_generated=VALUES(auto_generated);

INSERT IGNORE INTO help_keywords(help_tag, keyword) VALUES
('PROTECTIONFROMPOISON', 'protection from poison'),
('PROTECTIONFROMPOISON', 'protectionfrompoison'),
('PANACEA', 'panacea');
