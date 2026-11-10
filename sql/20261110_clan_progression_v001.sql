-- Clan progression Phase 4 / schema v1. Explicit, offline migration only.
-- Requires MariaDB >= 10.2.1 or MySQL >= 8.0.16 (enforced CHECK constraints).
-- Run with the mysql/mariadb CLI, WITHOUT --force, against an explicitly chosen
-- database. This file never selects a database, imports legacy data, or enables
-- gameplay. DDL commits implicitly: stop on errors and rerun while writers are
-- quiesced. The version receipt is written LAST; never treat table existence as
-- readiness. See docs/clan_progression_phase_4_handoff.md.
SET @clan_progression_previous_sql_mode = @@SESSION.sql_mode;
SET SESSION sql_mode = 'STRICT_ALL_TABLES,NO_ENGINE_SUBSTITUTION';

CREATE TABLE IF NOT EXISTS clan_progression_schema (
  singleton TINYINT NOT NULL PRIMARY KEY,
  version INT NOT NULL,
  installed_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  CONSTRAINT cp_schema_singleton CHECK (singleton = 1),
  CONSTRAINT cp_schema_version CHECK (version >= 1)
) ENGINE=InnoDB;

DELIMITER //
DROP PROCEDURE IF EXISTS clan_progression_v001_preflight//
CREATE PROCEDURE clan_progression_v001_preflight()
BEGIN
  DECLARE major_version INT;
  DECLARE minor_version INT;
  DECLARE patch_version INT;
  SET major_version = CAST(SUBSTRING_INDEX(VERSION(), '.', 1) AS UNSIGNED);
  SET minor_version = CAST(SUBSTRING_INDEX(SUBSTRING_INDEX(VERSION(), '.', 2), '.', -1) AS UNSIGNED);
  SET patch_version = CAST(SUBSTRING_INDEX(SUBSTRING_INDEX(SUBSTRING_INDEX(VERSION(), '.', 3), '.', -1), '-', 1) AS UNSIGNED);
  IF (LOCATE('MariaDB', VERSION()) > 0 AND
      (major_version < 10 OR (major_version = 10 AND minor_version < 2) OR
       (major_version = 10 AND minor_version = 2 AND patch_version < 1))) OR
     (LOCATE('MariaDB', VERSION()) = 0 AND
      (major_version < 8 OR (major_version = 8 AND minor_version = 0 AND patch_version < 16))) THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'Clan schema requires enforced CHECK constraints';
  END IF;
  IF EXISTS (SELECT 1 FROM clan_progression_schema WHERE singleton = 1 AND version <> 1) THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'Clan schema v1 cannot replace a different schema version';
  END IF;
END//
CALL clan_progression_v001_preflight()//
DROP PROCEDURE clan_progression_v001_preflight//
DELIMITER ;

-- UID is the lifetime identity, not an rnum, name, or reusable clan vnum.
-- The generated nullable unique key reserves a vnum only for an active clan.
-- Old tombstones can therefore coexist with a NEW incarnation of that vnum.
CREATE TABLE IF NOT EXISTS clan_progression_clans (
  clan_uid VARCHAR(32) CHARACTER SET ascii COLLATE ascii_bin NOT NULL PRIMARY KEY,
  clan_vnum INT NOT NULL,
  record_status TINYINT NOT NULL DEFAULT 1 COMMENT '1 active; 2 deleted/tombstone',
  active_vnum INT GENERATED ALWAYS AS
    (CASE WHEN record_status = 1 THEN clan_vnum ELSE NULL END) STORED,
  name VARCHAR(60) NOT NULL,
  description TEXT NOT NULL,
  abbreviation VARCHAR(5) NOT NULL DEFAULT '',
  leader_player_id BIGINT NOT NULL DEFAULT 0 COMMENT 'Saved character ID; 0 unassigned',
  treasury_gold BIGINT NOT NULL DEFAULT 0 COMMENT 'Legacy gold, NOT eighth-points',
  founded_at_utc BIGINT NOT NULL DEFAULT 0 COMMENT 'Unix seconds; 0 legacy unknown',
  last_activity_at_utc BIGINT NOT NULL DEFAULT 0,
  created_at_utc BIGINT NOT NULL,
  updated_at_utc BIGINT NOT NULL,
  deleted_at_utc BIGINT NULL,
  revision BIGINT NOT NULL DEFAULT 0,
  UNIQUE KEY cp_one_active_vnum (active_vnum),
  KEY cp_vnum_history (clan_vnum, record_status),
  CONSTRAINT cp_clan_uid CHECK (OCTET_LENGTH(clan_uid) = 32 AND
    clan_uid REGEXP '^[0-9a-f]{32}$' AND clan_uid <> '00000000000000000000000000000000'),
  CONSTRAINT cp_clan_vnum CHECK (clan_vnum BETWEEN 1 AND 65534),
  CONSTRAINT cp_clan_status CHECK (record_status IN (1, 2)),
  CONSTRAINT cp_clan_leader CHECK (leader_player_id >= 0),
  CONSTRAINT cp_clan_treasury CHECK (treasury_gold >= 0),
  CONSTRAINT cp_clan_revision CHECK (revision >= 0),
  CONSTRAINT cp_clan_times CHECK (founded_at_utc >= 0 AND last_activity_at_utc >= 0 AND
    created_at_utc >= 0 AND updated_at_utc >= created_at_utc),
  CONSTRAINT cp_clan_tombstone CHECK
    ((record_status = 1 AND deleted_at_utc IS NULL) OR
     (record_status = 2 AND deleted_at_utc IS NOT NULL AND
      deleted_at_utc >= created_at_utc AND updated_at_utc >= deleted_at_utc))
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- Level is derived from XP. Balances use signed-64-bit eighth-points to match
-- clan_progression_state; configurable gameplay caps belong to the award service.
CREATE TABLE IF NOT EXISTS clan_progression_state (
  clan_uid VARCHAR(32) CHARACTER SET ascii COLLATE ascii_bin NOT NULL PRIMARY KEY,
  xp_eighths BIGINT NOT NULL DEFAULT 0,
  influence_eighths BIGINT NOT NULL DEFAULT 0,
  revision BIGINT NOT NULL DEFAULT 0,
  updated_at_utc BIGINT NOT NULL DEFAULT 0,
  CONSTRAINT cp_state_clan FOREIGN KEY (clan_uid)
    REFERENCES clan_progression_clans (clan_uid) ON UPDATE RESTRICT ON DELETE RESTRICT,
  CONSTRAINT cp_state_balances CHECK (xp_eighths >= 0 AND influence_eighths >= 0),
  CONSTRAINT cp_state_revision CHECK (revision >= 0),
  CONSTRAINT cp_state_time CHECK (updated_at_utc >= 0)
) ENGINE=InnoDB;

-- Recreate stable triggers only while clan writers are quiesced. Never use
-- REPLACE or physical DELETE for clan history; deletion is a status transition.
DELIMITER //
DROP TRIGGER IF EXISTS cp_clan_identity_immutable//
CREATE TRIGGER cp_clan_identity_immutable BEFORE UPDATE ON clan_progression_clans
FOR EACH ROW
BEGIN
  IF BINARY NEW.clan_uid <> BINARY OLD.clan_uid OR NEW.clan_vnum <> OLD.clan_vnum OR
     NEW.created_at_utc <> OLD.created_at_utc THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'Clan incarnation identity is immutable';
  END IF;
  IF OLD.record_status = 2 THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'Deleted clan tombstones are immutable';
  END IF;
END//
DROP TRIGGER IF EXISTS cp_clan_history_no_delete//
CREATE TRIGGER cp_clan_history_no_delete BEFORE DELETE ON clan_progression_clans
FOR EACH ROW
BEGIN
  SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'Retain clan history; use a tombstone';
END//
DROP TRIGGER IF EXISTS cp_state_identity_immutable//
CREATE TRIGGER cp_state_identity_immutable BEFORE UPDATE ON clan_progression_state
FOR EACH ROW
BEGIN
  IF BINARY NEW.clan_uid <> BINARY OLD.clan_uid THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'Clan progression cannot move between incarnations';
  END IF;
END//
DROP TRIGGER IF EXISTS cp_state_history_no_delete//
CREATE TRIGGER cp_state_history_no_delete BEFORE DELETE ON clan_progression_state
FOR EACH ROW
BEGIN
  SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'Retain clan progression history';
END//
DELIMITER ;

-- Reapplying v1 does NOT reset treasury/progression/timestamps or overwrite a
-- receipt. Future migrations must explicitly validate their predecessor.
INSERT INTO clan_progression_schema (singleton, version) VALUES (1, 1)
  ON DUPLICATE KEY UPDATE singleton = VALUES(singleton);
SET SESSION sql_mode = @clan_progression_previous_sql_mode;
