-- Existing rooms default to Inside until explicitly changed.
CREATE TABLE IF NOT EXISTS housing_room_settings (
  room_id BIGINT PRIMARY KEY,
  sector_type INT NOT NULL DEFAULT 0,
  CONSTRAINT housing_sector_allowed CHECK (sector_type IN (0,1,2,3,6,9,29)),
  FOREIGN KEY(room_id) REFERENCES housing_rooms(room_id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
