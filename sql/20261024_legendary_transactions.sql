-- Phase 4, schema v2. Apply AFTER 20261023_legendary_objects.sql, offline.
-- IF NOT EXISTS makes reruns safe with MariaDB; version never downgraded.
ALTER TABLE legendary_operations
 ADD COLUMN IF NOT EXISTS held_player BIGINT NULL,
 ADD COLUMN IF NOT EXISTS new_identity TINYINT UNSIGNED NOT NULL DEFAULT 0,
 ADD COLUMN IF NOT EXISTS previous_bound BIGINT NOT NULL DEFAULT 0,
 ADD COLUMN IF NOT EXISTS quote_expires BIGINT UNSIGNED NOT NULL DEFAULT 0,
 ADD COLUMN IF NOT EXISTS config_version INT UNSIGNED NOT NULL DEFAULT 1;
-- At most one prepared/committed/quarantined operation per character.
CREATE UNIQUE INDEX IF NOT EXISTS legendary_operation_owner ON legendary_operations(held_player);
UPDATE legendary_schema SET version=2 WHERE singleton=1 AND version=1;
