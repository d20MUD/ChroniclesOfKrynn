-- Persistent per-room repeat-description discount eligibility.
CREATE TABLE IF NOT EXISTS housing_room_description_edits (
  room_id BIGINT PRIMARY KEY,
  FOREIGN KEY(room_id) REFERENCES housing_rooms(room_id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
