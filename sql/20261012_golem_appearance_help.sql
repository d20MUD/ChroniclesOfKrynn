-- Install during deployment. Appearance choices have no mechanical effects.
INSERT INTO help_entries(tag, category, entry, min_level, max_level, auto_generated)
VALUES ('GOLEMAPPEARANCE', 'general', REPLACE('GOLEM MATERIAL APPEARANCE

  craft golem appearance list
  craft golem appearance <material>
  craft golem appearance default

Choose a fixed material appearance for your active or stored golem.
For example: CRAFT GOLEM APPEARANCE GOLD
For multi-word materials: CRAFT GOLEM APPEARANCE MAPLE WOOD

This updates the short description and the description shown in the room.
A medium stone golem can appear as "a medium gold golem" while retaining
its original stone-golem statistics and abilities. It costs no materials.
There are no custom descriptions: only the fixed choices listed below.

  wood, stone, iron, steel, copper, bronze, silver, gold, platinum,
  mithril, adamantite, dragonmetal, ash wood, maple wood, mahogany wood,
  valenwood, ironwood, bone, dragonbone, dragonscale, clay, crystal, obsidian

DEFAULT restores the normal descriptions for the actual golem type.
The original targeting keywords remain available regardless of appearance.

Your selection is saved, used when recalling or upgrading your golem,
and passed along when transferring it to another player. Ordinary pet
saving and revival also preserve the changed descriptions.

Appearance does not change the actual golem type, level, size, stats,
resistances, attacks, repair materials, abilities, equipment or inventory.
To change your golem project''s actual type, use CRAFT GOLEM TYPE instead.
', CHAR(10), CONCAT(CHAR(13), CHAR(10))), 0, 1000, FALSE)
ON DUPLICATE KEY UPDATE
  category = VALUES(category), entry = VALUES(entry),
  min_level = VALUES(min_level), max_level = VALUES(max_level), auto_generated = FALSE;
INSERT IGNORE INTO help_keywords(help_tag, keyword) VALUES
('GOLEMAPPEARANCE', 'golemappearance'), ('GOLEMAPPEARANCE', 'golem appearance'),
('GOLEMAPPEARANCE', 'golem material appearance'), ('GOLEMAPPEARANCE', 'golem description');
UPDATE help_entries
SET entry = CONCAT(entry, CHAR(13), CHAR(10), CHAR(13), CHAR(10),
  'For cosmetic material descriptions, see help golemappearance.', CHAR(13), CHAR(10))
WHERE tag IN ('GOLEM', 'GOLEMCRAFT', 'GOLEMCRAFTING')
  AND LOCATE('help golemappearance', entry) = 0;
