-- Phase 5 / schema v3. Apply after the v1 and v2 migrations, offline.
-- Accepted source events survive a failed/prepared operation and are retried.
CREATE TABLE IF NOT EXISTS legendary_awards (
 player_id BIGINT NOT NULL, source_kind TINYINT UNSIGNED NOT NULL,
 award_id CHAR(32) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
 source_vnum INT NOT NULL, occurred_utc BIGINT UNSIGNED NOT NULL,
 xp BIGINT UNSIGNED NOT NULL DEFAULT 0, fragments INT UNSIGNED NOT NULL DEFAULT 0,
 essences INT UNSIGNED NOT NULL DEFAULT 0, catalysts INT UNSIGNED NOT NULL DEFAULT 0,
 essence_id TINYINT UNSIGNED NOT NULL DEFAULT 0,
 received_xp BIGINT UNSIGNED NOT NULL DEFAULT 0, received_fragments INT UNSIGNED NOT NULL DEFAULT 0,
 received_essences INT UNSIGNED NOT NULL DEFAULT 0, received_catalysts INT UNSIGNED NOT NULL DEFAULT 0,
 state ENUM('pending','credited','notified') NOT NULL DEFAULT 'pending',
 operation_id CHAR(32) CHARACTER SET ascii COLLATE ascii_bin NULL UNIQUE,
 PRIMARY KEY(player_id,source_kind,award_id), KEY recovery(player_id,state,occurred_utc),
 CHECK(player_id>0 AND source_kind BETWEEN 1 AND 4 AND source_vnum>=0 AND essence_id<6),
 CHECK(xp<=2147483647 AND fragments<=100 AND essences<=20 AND catalysts<=1),
 CHECK(received_xp<=xp AND received_fragments<=fragments AND received_essences<=essences AND received_catalysts<=catalysts)
) ENGINE=InnoDB;
UPDATE legendary_schema SET version=3 WHERE singleton=1 AND version=2;
