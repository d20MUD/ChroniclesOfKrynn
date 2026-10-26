-- Phase 9. Apply after the smith migration. No resolved effects are replayed.
CREATE TABLE IF NOT EXISTS legendary_activations (
 activation_id CHAR(32) CHARACTER SET ascii COLLATE ascii_bin PRIMARY KEY,
 player_id BIGINT NOT NULL, source_uuid CHAR(32) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
 channel_id TINYINT UNSIGNED NOT NULL, power_id INT NOT NULL,
 target_player_id BIGINT NOT NULL DEFAULT 0, started_utc BIGINT UNSIGNED NOT NULL,
 KEY source_player(player_id,started_utc),
 CHECK(player_id>0 AND channel_id<2)
) ENGINE=InnoDB;
UPDATE legendary_schema SET version=5 WHERE singleton=1 AND version=4;
