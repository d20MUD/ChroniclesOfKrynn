-- Apply explicitly after schema v5. Existing items start with no item passive.
ALTER TABLE legendary_channels ADD COLUMN IF NOT EXISTS item_passive_id INT UNSIGNED NOT NULL DEFAULT 0;
UPDATE legendary_schema SET version=6 WHERE singleton=1 AND version=5;
