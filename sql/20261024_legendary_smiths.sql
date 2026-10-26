-- Phase 6 / schema v4. Apply offline after v1-v3; no world content is created.
-- Register the chosen facilitator's prototype explicitly, never by NPC name.
CREATE TABLE IF NOT EXISTS legendary_smiths (
 mob_vnum INT PRIMARY KEY, enabled BOOLEAN NOT NULL DEFAULT TRUE,
 version BIGINT UNSIGNED NOT NULL DEFAULT 0, CHECK(mob_vnum>=0)
) ENGINE=InnoDB;
UPDATE legendary_schema SET version=4 WHERE singleton=1 AND version=3;
