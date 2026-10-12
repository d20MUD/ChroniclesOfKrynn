-- Install during deployment; preserve the existing refining help.
INSERT INTO help_entries(tag, category, entry, min_level, max_level, auto_generated)
VALUES ('BATCHSMELTING', 'general', REPLACE('BATCH REFINING AND SMELTING

  refine batch <material> [quantity|all]
  refine smelt <material> [quantity|all]

REFINE BATCH prepares and begins any refining recipe with one command.
You must have the required ingredients and the recipe''s station in your
room: a smelter, loom or alchemy lab. Quantity means recipe
batches, not individual output units. Omitting it makes one batch.
The maximum batch quantity is 20.

Examples:
  refine batch linen 10        Start ten linen batches at a loom.
  refine batch satin all       Refine silk and cotton into satin.
  refine batch catalyst all    Combine fragments at an alchemy lab.
  refine smelt steel 10        Start ten steel batches.
  refine smelt steel all       Start as many as your materials permit,
                              up to twenty batches.
  refine smelt brass 5         Make ten brass units (two per batch).
  refine smelt cold iron 3     Multi-word material names work directly.

Smelting recipes are bronze, steel, brass, alchemical silver, cold iron
and dragonmetal. Dragonmetal uses dragonblood and a single type of grade
3 or higher hard metal; that chosen metal must cover the whole batch.

Materials are reserved when the batch is prepared. Time and output scale
with the recipe output: refining takes ten seconds per output unit.
The starting message shows the batch count, total output and duration.

Existing refining commands also support batches:
  refine add <material> [quantity|all]   Prepare without starting.
  refine show                          Show ingredients and output.
  refine begin                         Start or retry a prepared batch.
  refine remove                        Cancel and recover reserved inputs.

These commands also work for loom and alchemy recipes at their own
required stations. REFINE BATCH starts any of these recipes directly.
REFINE SMELT remains available for smelter recipes.

Each batch project uses one existing refining skill check. Ordinary
failure leaves the prepared inputs available for another REFINE BEGIN.
A natural 1 loses the reserved materials. A natural 20 grants one extra
output unit. Successful XP scales with total output, using the recipe''s
normal skill (Mining for smelting), and shows current XP and TNL.
', CHAR(10), CONCAT(CHAR(13), CHAR(10))), 0, 1000, FALSE)
ON DUPLICATE KEY UPDATE
  category = VALUES(category), entry = VALUES(entry),
  min_level = VALUES(min_level), max_level = VALUES(max_level), auto_generated = FALSE;
INSERT IGNORE INTO help_keywords(help_tag, keyword) VALUES
('BATCHSMELTING', 'batchsmelting'), ('BATCHSMELTING', 'batch smelting'),
('BATCHSMELTING', 'smelting'), ('BATCHSMELTING', 'batch refining'),
('BATCHSMELTING', 'batchrefining'), ('BATCHSMELTING', 'refining batches');
UPDATE help_entries
SET entry = CONCAT(entry, CHAR(13), CHAR(10), CHAR(13), CHAR(10),
  'For one-command smelting and batch quantities, see help batchsmelting.',
  CHAR(13), CHAR(10))
WHERE tag IN ('REFINE', 'REFINING')
  AND LOCATE('help batchsmelting', entry) = 0;
