-- Schema v10: double daily material acquisitions to 200 fragments,
-- 40 essences across all types, and 2 catalysts. Preserve today's usage.
-- Find the old unnamed check as well as this migration's named check so this
-- works for existing installations and remains repeatable.
SET @legendary_daily_check = (
 SELECT CONSTRAINT_NAME FROM information_schema.CHECK_CONSTRAINTS
 WHERE CONSTRAINT_SCHEMA=DATABASE() AND TABLE_NAME='legendary_characters'
   AND CHECK_CLAUSE LIKE '%fragments_today%'
 LIMIT 1
);
SET @legendary_daily_drop = IF(@legendary_daily_check IS NULL, 'SELECT 1',
 CONCAT('ALTER TABLE legendary_characters DROP CONSTRAINT `',
 REPLACE(@legendary_daily_check, '`', '``'), '`'));
PREPARE legendary_daily_stmt FROM @legendary_daily_drop;
EXECUTE legendary_daily_stmt;
DEALLOCATE PREPARE legendary_daily_stmt;
ALTER TABLE legendary_characters ADD CONSTRAINT legendary_daily_material_limits
 CHECK(fragments_today<=200 AND essences_today<=40 AND catalysts_today<=2);
UPDATE legendary_schema SET version=10 WHERE singleton=1 AND version=9;
