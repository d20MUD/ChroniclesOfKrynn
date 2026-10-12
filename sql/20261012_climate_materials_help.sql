-- Install during deployment. This does not place exchanger NPCs in the world.
INSERT INTO help_entries(tag, category, entry, min_level, max_level, auto_generated)
VALUES ('MATERIALVARIETIES', 'general', REPLACE('CLIMATE MATERIAL VARIETIES

Crafting nodes can have regional varieties chosen by the zone climate.
Room terrain determines which resource groups can appear. Zone levels
and the usual grade roll determine their quality.

Climate       Grade 1                 Grade 2           Grade 3
Unspecified   Ash                     Maple             Mahogany
Polar         Driftwood, dwarf willow Birch driftwood   Larch driftwood
Subarctic     Spruce, pine            Birch             Larch
Temperate     Ash, beech, willow      Maple             Oak
Subtropical   Pine, willow            Cypress, cedar     Live oak
Tropical      Bamboo                  Teak              Mahogany, ebony
Arid          Tamarisk                Acacia            Mesquite
Alpine        Fir, pine               Birch, cedar      Larch

Grade 4 remains valenwood; grade 5 remains ironwood. Temperate swamps
favor grade-1 willow, and warm swamps favor grade-2 cypress.
Underground, ocean and urban zone terrain excludes automatic timber.

Varieties within a timber grade have equal crafting properties. They
have separate material balances, and bundles and crafted items preserve
the variety used. Grade-1 tropical jute and arid agave fiber are equivalent
to hemp. Game tracks reflect local animals but yield the usual hide grades.
Climate changes mining descriptions, not which metal or stone is mined.

MATERIALS EXCHANGER

At an NPC offering material exchange:
  exchange list
  exchange 10 maple for birch
  exchange 5 live oak wood for mahogany wood
  exchange 8 hemp for jute

Trade equal quantities of different varieties in the same family and
grade, without a gold fee. Exchange uses your on-hand material balances;
store physical bundles first. Reserved project materials are unavailable.
Full names or unique abbreviations work; multi-word names may be quoted.

Equivalent timber varieties in grades 1-3 and grade-1 fibers qualify.
Unrelated materials cannot be exchanged just because their grades match.
You cannot exchange while you or the NPC are fighting. The exchanger
must be awake and visible. Use exchange list to see available varieties
and how many you own.

See also: MATERIALS, HARVEST, SURVEY, ARTISANSHOP.
', CHAR(10), CONCAT(CHAR(13), CHAR(10))), 0, 1000, FALSE)
ON DUPLICATE KEY UPDATE category = VALUES(category), entry = VALUES(entry),
  min_level = VALUES(min_level), max_level = VALUES(max_level), auto_generated = VALUES(auto_generated);

INSERT IGNORE INTO help_keywords(help_tag, keyword) VALUES
  ('MATERIALVARIETIES', 'materialvarieties'),
  ('MATERIALVARIETIES', 'climatematerials'),
  ('MATERIALVARIETIES', 'materialsexchanger'),
  ('MATERIALVARIETIES', 'materialexchange');
