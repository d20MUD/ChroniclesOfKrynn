-- Dragonmetal Awakening schema v1. Explicit migration; never auto-installed at login.
-- Idempotent base schema; apply subsequent phase migrations for the current runtime.
CREATE TABLE IF NOT EXISTS legendary_schema (singleton TINYINT PRIMARY KEY, version INT NOT NULL, installed_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP, CHECK(singleton=1)) ENGINE=InnoDB;
INSERT IGNORE INTO legendary_schema(singleton,version) VALUES(1,1);
CREATE TABLE IF NOT EXISTS legendary_characters (
 player_id BIGINT PRIMARY KEY, infusion_xp BIGINT UNSIGNED NOT NULL DEFAULT 0,
 fragments BIGINT UNSIGNED NOT NULL DEFAULT 0, catalysts BIGINT UNSIGNED NOT NULL DEFAULT 0,
 draconic BIGINT UNSIGNED NOT NULL DEFAULT 0, martial BIGINT UNSIGNED NOT NULL DEFAULT 0,
 arcane BIGINT UNSIGNED NOT NULL DEFAULT 0, spiritual BIGINT UNSIGNED NOT NULL DEFAULT 0,
 primal BIGINT UNSIGNED NOT NULL DEFAULT 0, shadow BIGINT UNSIGNED NOT NULL DEFAULT 0,
 allowance_date DATE NULL, fragments_today INT UNSIGNED NOT NULL DEFAULT 0,
 essences_today INT UNSIGNED NOT NULL DEFAULT 0, catalysts_today INT UNSIGNED NOT NULL DEFAULT 0,
 passive_priorities TEXT NULL, major_selections TEXT NULL, version BIGINT UNSIGNED NOT NULL DEFAULT 0,
 CHECK(player_id>0), CHECK(infusion_xp<=2000000000 AND fragments<=2000000000 AND catalysts<=2000000000),
 CHECK(draconic<=2000000000 AND martial<=2000000000 AND arcane<=2000000000 AND spiritual<=2000000000 AND primal<=2000000000 AND shadow<=2000000000),
 CHECK(fragments_today<=100 AND essences_today<=20 AND catalysts_today<=1)
) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS legendary_items (
 uuid CHAR(32) CHARACTER SET ascii COLLATE ascii_bin PRIMARY KEY,
 owner_id BIGINT NOT NULL, prototype_vnum INT NOT NULL, slot_id TINYINT UNSIGNED NOT NULL,
 state ENUM('pending','active','deleted') NOT NULL DEFAULT 'pending',
 version BIGINT UNSIGNED NOT NULL DEFAULT 0, created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
 deleted_at DATETIME NULL, KEY owner(owner_id,state),
 CHECK(owner_id>0 AND prototype_vnum>=0 AND slot_id<10)
) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS legendary_channels (
 uuid CHAR(32) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
 channel_id TINYINT UNSIGNED NOT NULL, rank_id TINYINT UNSIGNED NOT NULL,
 path_id TINYINT UNSIGNED NOT NULL, family_id TINYINT UNSIGNED NOT NULL DEFAULT 0,
 foundation_id INT NOT NULL DEFAULT 0, subtype_id INT NOT NULL DEFAULT 0,
 technique_id INT NOT NULL DEFAULT 0, signature_id INT NOT NULL DEFAULT 0,
 modifier_id TINYINT UNSIGNED NOT NULL DEFAULT 0, modifier_target TINYINT UNSIGNED NOT NULL DEFAULT 0,
 first_choices TINYINT UNSIGNED NOT NULL DEFAULT 0,
 paid_xp BIGINT UNSIGNED NOT NULL DEFAULT 0, paid_gold BIGINT UNSIGNED NOT NULL DEFAULT 0,
 paid_fragments BIGINT UNSIGNED NOT NULL DEFAULT 0, paid_essences BIGINT UNSIGNED NOT NULL DEFAULT 0,
 paid_catalysts BIGINT UNSIGNED NOT NULL DEFAULT 0, version BIGINT UNSIGNED NOT NULL DEFAULT 0,
 PRIMARY KEY(uuid,channel_id), FOREIGN KEY(uuid) REFERENCES legendary_items(uuid),
 CHECK(channel_id<2 AND rank_id BETWEEN 1 AND 5 AND path_id BETWEEN 1 AND 12 AND family_id<=3),
 CHECK(modifier_id<=3 AND modifier_target<=2 AND first_choices<=15)
) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS legendary_cooldowns (
 player_id BIGINT NOT NULL, cooldown_key INT NOT NULL, expires_utc BIGINT UNSIGNED NOT NULL,
 PRIMARY KEY(player_id,cooldown_key), KEY expiry(expires_utc), CHECK(player_id>0)
) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS legendary_budgets (
 entry_id BIGINT UNSIGNED PRIMARY KEY AUTO_INCREMENT, player_id BIGINT NOT NULL,
 budget_kind TINYINT UNSIGNED NOT NULL, amount INT UNSIGNED NOT NULL,
 occurred_utc BIGINT UNSIGNED NOT NULL, operation_id CHAR(32) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
 UNIQUE KEY operation_budget(player_id,budget_kind,operation_id), KEY window_lookup(player_id,budget_kind,occurred_utc),
 CHECK(player_id>0 AND budget_kind<4 AND amount>0)
) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS legendary_boss_claims (
 player_id BIGINT NOT NULL, death_id CHAR(32) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
 boss_vnum INT NOT NULL, awarded_utc BIGINT UNSIGNED NOT NULL, essence_id TINYINT UNSIGNED NOT NULL,
 fragments INT UNSIGNED NOT NULL, essences INT UNSIGNED NOT NULL, catalysts INT UNSIGNED NOT NULL,
 PRIMARY KEY(player_id,death_id), KEY boss_cooldown(player_id,boss_vnum,awarded_utc),
 CHECK(player_id>0 AND boss_vnum>=0 AND essence_id<6)
) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS legendary_quest_xp_claims (
 player_id BIGINT NOT NULL, quest_vnum INT NOT NULL, award_date DATE NOT NULL,
 operation_id CHAR(32) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
 PRIMARY KEY(player_id,quest_vnum,award_date), UNIQUE KEY operation(operation_id), CHECK(player_id>0 AND quest_vnum>=0)
) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS legendary_operations (
 operation_id CHAR(32) CHARACTER SET ascii COLLATE ascii_bin PRIMARY KEY,
 player_id BIGINT NOT NULL, uuid CHAR(32) CHARACTER SET ascii COLLATE ascii_bin NULL,
 channel_id TINYINT UNSIGNED NULL, kind VARCHAR(32) NOT NULL,
 state ENUM('prepared','committed','applied','cancelled','quarantined') NOT NULL DEFAULT 'prepared',
 estate_delivery_id BIGINT UNSIGNED NULL UNIQUE,
 before_version BIGINT UNSIGNED NOT NULL, after_version BIGINT UNSIGNED NOT NULL,
 gold_before BIGINT UNSIGNED NOT NULL, gold_after BIGINT UNSIGNED NOT NULL,
 xp_cost BIGINT UNSIGNED NOT NULL DEFAULT 0, gold_cost BIGINT UNSIGNED NOT NULL DEFAULT 0,
 fragment_cost BIGINT UNSIGNED NOT NULL DEFAULT 0, essence_cost BIGINT UNSIGNED NOT NULL DEFAULT 0,
 catalyst_cost BIGINT UNSIGNED NOT NULL DEFAULT 0,
 recovery_payload LONGTEXT NOT NULL, created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
 updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
 KEY pending(player_id,state), CHECK(player_id>0 AND (channel_id IS NULL OR channel_id<2))
) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS legendary_boss_registry (
 boss_vnum INT PRIMARY KEY, tier TINYINT UNSIGNED NOT NULL, essence_id TINYINT UNSIGNED NOT NULL,
 enabled TINYINT UNSIGNED NOT NULL DEFAULT 0, hard_control_immune TINYINT UNSIGNED NOT NULL DEFAULT 1,
 version BIGINT UNSIGNED NOT NULL DEFAULT 0, CHECK(boss_vnum>=0 AND tier BETWEEN 1 AND 3 AND essence_id<6 AND enabled<=1 AND hard_control_immune<=1)
) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS legendary_dragonlances (
 prototype_vnum INT PRIMARY KEY, enabled TINYINT UNSIGNED NOT NULL DEFAULT 0,
 version BIGINT UNSIGNED NOT NULL DEFAULT 0, CHECK(prototype_vnum>=0 AND enabled<=1)
) ENGINE=InnoDB;
