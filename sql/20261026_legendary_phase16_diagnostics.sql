-- Phase 16: read-only rollout diagnostics.
-- Run after migrations, while all feature gates are disabled, and again after
-- each gate change.  These queries never mutate player or legendary data.

SELECT version AS legendary_schema_version
  FROM legendary_schema
 WHERE singleton=1;

SELECT feature_name, enabled
  FROM legendary_features
 ORDER BY feature_name;

-- Smith assignments are inspected through MEDIT (Infusion Smith), not SQL.
SELECT COUNT(*) AS obsolete_smith_registry_tables
  FROM information_schema.tables
 WHERE table_schema=DATABASE()
   AND table_name='legendary_smiths';

SELECT COUNT(*) AS obsolete_boss_registry_tables
  FROM information_schema.tables
 WHERE table_schema=DATABASE()
   AND table_name='legendary_boss_registry';

SELECT state, COUNT(*) AS operation_count
  FROM legendary_operations
 GROUP BY state
 ORDER BY state;

SELECT state, COUNT(*) AS award_count
  FROM legendary_awards
 GROUP BY state
 ORDER BY state;

SELECT COUNT(*) AS pending_awards
  FROM legendary_awards
 WHERE state='pending';

SELECT COUNT(*) AS unrecovered_operations
  FROM legendary_operations
 WHERE state NOT IN ('complete','cancelled');
