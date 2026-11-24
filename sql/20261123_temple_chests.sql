-- Shared temple storage, using the existing estate escrow/delivery journal.
CREATE TABLE IF NOT EXISTS temple_chests (
  room_vnum INT PRIMARY KEY,
  deity INT NOT NULL,
  temple_rank INT NOT NULL,
  item_limit INT NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE IF NOT EXISTS temple_chest_items (
  item_id BIGINT PRIMARY KEY AUTO_INCREMENT,
  chest_id INT NOT NULL,
  item_name TEXT NOT NULL,
  keywords TEXT NOT NULL,
  serialized_obj LONGTEXT NOT NULL,
  item_units INT NOT NULL DEFAULT 1,
  KEY chest(chest_id),
  FOREIGN KEY(chest_id) REFERENCES temple_chests(room_vnum)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
