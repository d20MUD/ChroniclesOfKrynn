-- Phase 3: one real-time church blessing and the shared appearance catalogue.
CREATE TABLE IF NOT EXISTS religion_blessings (
  player_id BIGINT PRIMARY KEY,
  deity INT NOT NULL,
  application_rank INT NOT NULL,
  expires_at BIGINT NOT NULL,
  available_after BIGINT NOT NULL
) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS restring_catalogue (
  id BIGINT UNSIGNED PRIMARY KEY AUTO_INCREMENT,
  revision INT NOT NULL DEFAULT 0,
  draft MEDIUMTEXT NULL,
  published MEDIUMTEXT NULL,
  deleted TINYINT NOT NULL DEFAULT 0
) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS restring_audit (
  audit_id BIGINT UNSIGNED PRIMARY KEY AUTO_INCREMENT,
  entry_id BIGINT UNSIGNED NOT NULL,
  revision INT NOT NULL,
  editor BIGINT NOT NULL,
  action VARCHAR(16) NOT NULL,
  record MEDIUMTEXT NOT NULL,
  created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB;
