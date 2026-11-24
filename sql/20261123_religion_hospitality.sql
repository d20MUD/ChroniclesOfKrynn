-- Hospitality rewards and cooldown share the estate delivery transaction.
CREATE TABLE IF NOT EXISTS religion_hospitality (
  player_id BIGINT PRIMARY KEY,
  available_after BIGINT NOT NULL DEFAULT 0
) ENGINE=InnoDB;
