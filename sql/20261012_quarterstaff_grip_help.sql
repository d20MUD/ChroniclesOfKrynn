-- Install during deployment.
INSERT INTO help_entries(tag, category, entry, min_level, max_level, auto_generated)
VALUES ('QUARTERSTAFFGRIP', 'general', REPLACE('QUARTERSTAFF GRIP

  wield <staff> onehand
  wield <staff> twohand

Quarterstaffs can be wielded in one or two hands. WIELD without a grip
uses one hand when the staff is an appropriate size for your character.
An oversized staff may still require two hands or be too large to use.

One hand leaves your other hand available for a shield, another weapon,
or a held item. It uses normal one-handed Strength and item bonuses,
even if the other hand is empty.

Two hands require two free hands and use the existing two-handed combat
and item bonuses. An occupied hand prevents taking a two-handed grip.

To change an equipped staff''s grip, remove it, then wield it again with
the desired option. Normal equipment requirements and triggers apply.

See also: WIELD, REMOVE, EQUIPMENT.
', CHAR(10), CONCAT(CHAR(13), CHAR(10))), 0, 1000, FALSE)
ON DUPLICATE KEY UPDATE category = VALUES(category), entry = VALUES(entry),
  min_level = VALUES(min_level), max_level = VALUES(max_level), auto_generated = VALUES(auto_generated);

INSERT IGNORE INTO help_keywords(help_tag, keyword) VALUES
  ('QUARTERSTAFFGRIP', 'quarterstaffgrip'),
  ('QUARTERSTAFFGRIP', 'staffgrip'),
  ('QUARTERSTAFFGRIP', 'quarterstaff');
