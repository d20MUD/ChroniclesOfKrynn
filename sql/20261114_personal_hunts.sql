-- Personal hunts: one versioned row per character, updated only on changes.
-- Select the intended game database before applying. Safe to reapply.
CREATE TABLE IF NOT EXISTS personal_hunt_progress (
  character_id BIGINT NOT NULL PRIMARY KEY,
  expires_at BIGINT UNSIGNED NOT NULL,
  targets TEXT NOT NULL COMMENT '35 space-separated integers: five seven-field target slots',
  revision BIGINT UNSIGNED NOT NULL DEFAULT 1,
  updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
) ENGINE=InnoDB;
