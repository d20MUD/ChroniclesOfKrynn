-- Phase 4b: legacy clan fields, memberships, claims and import receipts.
-- Explicit offline migration. Apply v001 first; never use --force.
DELIMITER //
DROP PROCEDURE IF EXISTS clan_progression_v002_preflight//
CREATE PROCEDURE clan_progression_v002_preflight()
BEGIN
 IF NOT EXISTS (SELECT 1 FROM clan_progression_schema WHERE singleton=1 AND version IN (1,2)) THEN
  SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Clan schema v2 requires version 1 or 2';
 END IF;
END//
CALL clan_progression_v002_preflight()//
DROP PROCEDURE clan_progression_v002_preflight//
DELIMITER ;
CREATE TABLE IF NOT EXISTS clan_progression_legacy (
 clan_uid VARCHAR(32) CHARACTER SET ascii COLLATE ascii_bin NOT NULL PRIMARY KEY,
 details LONGTEXT NOT NULL COMMENT 'Validated versioned legacy scalars, ranks, indexed privileges and UID relationships',
 CONSTRAINT cp_legacy_clan FOREIGN KEY (clan_uid) REFERENCES clan_progression_clans(clan_uid),
 CONSTRAINT cp_legacy_json CHECK (JSON_VALID(details))
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE IF NOT EXISTS clan_memberships (
 player_id BIGINT NOT NULL PRIMARY KEY,
 clan_uid VARCHAR(32) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
 rank_id INT NOT NULL COMMENT '0 applicant; 1..15 accepted rank, never Favor',
 legacy_points INT NOT NULL DEFAULT 0 COMMENT 'Import/reconciliation mirror; player-file clan points stay separate',
 CONSTRAINT cp_member_clan FOREIGN KEY (clan_uid) REFERENCES clan_progression_clans(clan_uid),
 CONSTRAINT cp_member_player CHECK (player_id>0),
 CONSTRAINT cp_member_rank CHECK (rank_id BETWEEN 0 AND 15),
 KEY cp_member_clan_lookup (clan_uid)
) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS clan_zone_claims (
 zone_vnum INT NOT NULL PRIMARY KEY,
 controller_uid VARCHAR(32) CHARACTER SET ascii COLLATE ascii_bin NULL,
 claimant_id BIGINT NOT NULL DEFAULT 0,
 popularity LONGTEXT NOT NULL COMMENT 'Validated array of clan_uid/percentage pairs; no runtime rnums',
 CONSTRAINT cp_claim_controller FOREIGN KEY (controller_uid) REFERENCES clan_progression_clans(clan_uid),
 CONSTRAINT cp_claim_zone CHECK (zone_vnum BETWEEN 0 AND 65534),
 CONSTRAINT cp_claim_player CHECK (claimant_id>=0),
 CONSTRAINT cp_claim_popularity CHECK (JSON_VALID(popularity))
) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS clan_import_manifests (
 source_digest VARCHAR(64) CHARACTER SET ascii COLLATE ascii_bin NOT NULL PRIMARY KEY,
 import_uid VARCHAR(32) CHARACTER SET ascii COLLATE ascii_bin NOT NULL UNIQUE,
 import_status INT NOT NULL DEFAULT 1 COMMENT '1 imported, 2 cut over; never reset',
 source_manifest LONGTEXT NOT NULL COMMENT 'Immutable file digests and legacy-vnum/incarnation mapping',
 imported_at_utc BIGINT NOT NULL,
 CONSTRAINT cp_import_digest CHECK (OCTET_LENGTH(source_digest)=64 AND source_digest REGEXP '^[0-9a-f]{64}$'),
 CONSTRAINT cp_import_uid CHECK (OCTET_LENGTH(import_uid)=32 AND import_uid REGEXP '^[0-9a-f]{32}$' AND import_uid<>'00000000000000000000000000000000'),
 CONSTRAINT cp_import_status CHECK (import_status IN (1,2)),
 CONSTRAINT cp_import_json CHECK (JSON_VALID(source_manifest)),
 CONSTRAINT cp_import_time CHECK (imported_at_utc>=0)
) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS clan_store_control (
 singleton INT NOT NULL PRIMARY KEY,
 revision BIGINT NOT NULL DEFAULT 0,
 store_mode INT NOT NULL DEFAULT 0 COMMENT '0 legacy; 1 shadow; 2 MySQL; cutover remains explicit',
 CONSTRAINT cp_store_singleton CHECK (singleton=1),
 CONSTRAINT cp_store_revision CHECK (revision>=0),
 CONSTRAINT cp_store_mode CHECK (store_mode IN (0,1,2))
) ENGINE=InnoDB;
INSERT INTO clan_store_control(singleton) VALUES(1) ON DUPLICATE KEY UPDATE singleton=VALUES(singleton);
DELIMITER //
DROP TRIGGER IF EXISTS cp_import_immutable//
CREATE TRIGGER cp_import_immutable BEFORE UPDATE ON clan_import_manifests FOR EACH ROW
BEGIN
 IF BINARY NEW.source_digest<>BINARY OLD.source_digest OR BINARY NEW.import_uid<>BINARY OLD.import_uid OR
    BINARY NEW.source_manifest<>BINARY OLD.source_manifest OR NEW.imported_at_utc<>OLD.imported_at_utc OR
    NEW.import_status<OLD.import_status THEN
  SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Clan import history is immutable';
 END IF;
END//
DROP TRIGGER IF EXISTS cp_import_no_delete//
CREATE TRIGGER cp_import_no_delete BEFORE DELETE ON clan_import_manifests FOR EACH ROW
BEGIN
 SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Retain clan import receipts';
END//
DELIMITER ;
UPDATE clan_progression_schema SET version=2 WHERE singleton=1 AND version=1;
