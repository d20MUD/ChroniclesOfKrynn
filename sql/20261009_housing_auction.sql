-- Native Krynn housing and auction persistence. Also created automatically at startup.
-- Use the Krynn database; this does not migrate Star Wars player records.

CREATE TABLE IF NOT EXISTS housing_neighborhoods (neighborhood_id BIGINT PRIMARY KEY AUTO_INCREMENT,
  name VARCHAR(100) NOT NULL,
  entrance_room_vnum INT NOT NULL UNIQUE,
  house_price BIGINT NOT NULL DEFAULT 100000,
  room_price BIGINT NOT NULL DEFAULT 50000,
  title_price BIGINT NOT NULL DEFAULT 500,
  description_price BIGINT NOT NULL DEFAULT 1000,
  extra_description_price BIGINT NOT NULL DEFAULT 750,
  chest_price BIGINT NOT NULL DEFAULT 10000,
  chest_customization_price BIGINT NOT NULL DEFAULT 500,
  active TINYINT NOT NULL DEFAULT 1) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS housing_houses (house_id BIGINT PRIMARY KEY AUTO_INCREMENT,
  neighborhood_id BIGINT NOT NULL,
  owner_id BIGINT NOT NULL,
  owner_name VARCHAR(64) NOT NULL,
  name VARCHAR(100) NOT NULL,
  UNIQUE KEY owner_neighborhood(neighborhood_id,
  owner_id),
  FOREIGN KEY(neighborhood_id) REFERENCES housing_neighborhoods(neighborhood_id)) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS housing_rooms (room_id BIGINT PRIMARY KEY AUTO_INCREMENT,
  house_id BIGINT NOT NULL,
  room_order INT NOT NULL,
  title VARCHAR(200) NOT NULL,
  description TEXT NOT NULL,
  is_entrance TINYINT NOT NULL DEFAULT 0,
  UNIQUE KEY room_order(house_id,
  room_order),
  UNIQUE KEY house_room(house_id,
  room_id),
  FOREIGN KEY(house_id) REFERENCES housing_houses(house_id) ON DELETE CASCADE) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS housing_room_exits (from_room_id BIGINT NOT NULL,
  direction INT NOT NULL,
  to_room_id BIGINT NOT NULL,
  PRIMARY KEY(from_room_id,
  direction),
  FOREIGN KEY(from_room_id) REFERENCES housing_rooms(room_id) ON DELETE CASCADE,
  FOREIGN KEY(to_room_id) REFERENCES housing_rooms(room_id) ON DELETE CASCADE) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS housing_room_extra_descriptions (extra_description_id BIGINT PRIMARY KEY AUTO_INCREMENT,
  room_id BIGINT NOT NULL,
  keyword VARCHAR(100) NOT NULL,
  description TEXT NOT NULL,
  FOREIGN KEY(room_id) REFERENCES housing_rooms(room_id) ON DELETE CASCADE) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS housing_guests (house_id BIGINT NOT NULL,
  guest_id BIGINT NOT NULL,
  PRIMARY KEY(house_id,
  guest_id),
  FOREIGN KEY(house_id) REFERENCES housing_houses(house_id) ON DELETE CASCADE) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS housing_storage_chests (chest_id BIGINT PRIMARY KEY AUTO_INCREMENT,
  house_id BIGINT NOT NULL,
  room_id BIGINT NOT NULL,
  keywords VARCHAR(255) NOT NULL DEFAULT 'storage chest',
  short_description VARCHAR(255) NOT NULL DEFAULT 'a storage chest',
  room_description VARCHAR(255) NOT NULL DEFAULT 'A sturdy storage chest rests here.',
  FOREIGN KEY(house_id,
  room_id) REFERENCES housing_rooms(house_id,
  room_id) ON DELETE CASCADE) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS housing_chest_items (item_id BIGINT UNSIGNED PRIMARY KEY AUTO_INCREMENT,
  chest_id BIGINT NOT NULL,
  item_name VARCHAR(255) NOT NULL,
  keywords TEXT NOT NULL,
  serialized_obj LONGTEXT NOT NULL,
  KEY chest(chest_id),
  FOREIGN KEY(chest_id) REFERENCES housing_storage_chests(chest_id) ON DELETE CASCADE) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS auction_listings (listing_id BIGINT UNSIGNED PRIMARY KEY AUTO_INCREMENT,
  seller_id BIGINT NOT NULL,
  seller_name VARCHAR(64) NOT NULL,
  item_name VARCHAR(255) NOT NULL,
  keywords TEXT NOT NULL,
  price BIGINT NOT NULL,
  item_type INT NOT NULL,
  wear_flags VARCHAR(128) NOT NULL,
  serialized_obj LONGTEXT NOT NULL,
  listing_state ENUM('active',
  'sold',
  'cancelled',
  'expired') NOT NULL DEFAULT 'active',
  buyer_id BIGINT NULL,
  expires_at TIMESTAMP NOT NULL,
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  KEY active(listing_state,
  expires_at),
  KEY seller(seller_id,
  listing_state)) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS auction_credit_settlements (settlement_id BIGINT UNSIGNED PRIMARY KEY AUTO_INCREMENT,
  listing_id BIGINT UNSIGNED NOT NULL UNIQUE,
  beneficiary_id BIGINT NOT NULL,
  amount BIGINT NOT NULL,
  settlement_state ENUM('pending',
  'paid') NOT NULL DEFAULT 'pending',
  KEY beneficiary(beneficiary_id,
  settlement_state),
  FOREIGN KEY(listing_id) REFERENCES auction_listings(listing_id)) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS estate_deliveries (delivery_id BIGINT UNSIGNED PRIMARY KEY AUTO_INCREMENT,
  player_id BIGINT NOT NULL,
  debit BIGINT NOT NULL DEFAULT 0,
  serialized_obj LONGTEXT NOT NULL,
  delivered TINYINT NOT NULL DEFAULT 0,
  KEY pending(player_id,
  delivered)) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS estate_outgoing (escrow_id BIGINT UNSIGNED PRIMARY KEY AUTO_INCREMENT,
  player_id BIGINT NOT NULL,
  escrow_state ENUM('prepared',
  'committed',
  'saved') NOT NULL DEFAULT 'prepared',
  KEY pending(player_id,
  escrow_state)) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
