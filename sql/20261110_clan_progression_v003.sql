-- Phases 5a/6: retained family timers and cumulative personal Favor.
-- Explicit offline migration; apply v001, v002, then v003. Never --force.
DELIMITER //
DROP PROCEDURE IF EXISTS clan_progression_v003_preflight//
CREATE PROCEDURE clan_progression_v003_preflight()
BEGIN
 IF NOT EXISTS (SELECT 1 FROM clan_progression_schema WHERE singleton=1 AND version IN (2,3)) THEN
  SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Clan schema v3 requires version 2 or 3';
 END IF;
END//
CALL clan_progression_v003_preflight()//
DROP PROCEDURE clan_progression_v003_preflight//
DELIMITER ;
CREATE TABLE IF NOT EXISTS clan_buff_families (
 clan_uid VARCHAR(32) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
 family_id INT NOT NULL,
 activation_uid VARCHAR(32) CHARACTER SET ascii COLLATE ascii_bin NOT NULL UNIQUE,
 tier INT NOT NULL,
 slot_id INT NOT NULL,
 rules_version INT NOT NULL,
 started_at BIGINT NOT NULL,
 expires_at BIGINT NOT NULL,
 next_available_at BIGINT NOT NULL,
 paid_influence_eighths BIGINT NOT NULL,
 canceled INT NOT NULL DEFAULT 0,
 PRIMARY KEY (clan_uid,family_id),
 CONSTRAINT cp_buff_clan FOREIGN KEY (clan_uid) REFERENCES clan_progression_clans(clan_uid),
 CONSTRAINT cp_buff_family CHECK (family_id BETWEEN 1 AND 5),
 CONSTRAINT cp_buff_activation CHECK (OCTET_LENGTH(activation_uid)=32 AND activation_uid REGEXP '^[0-9a-f]{32}$' AND activation_uid<>'00000000000000000000000000000000'),
 CONSTRAINT cp_buff_tier CHECK (tier BETWEEN 1 AND 3),
 CONSTRAINT cp_buff_slot CHECK (slot_id BETWEEN 1 AND 3),
 CONSTRAINT cp_buff_rules CHECK (rules_version>0),
 CONSTRAINT cp_buff_times CHECK (started_at>=0 AND expires_at>started_at AND next_available_at>=expires_at),
 CONSTRAINT cp_buff_paid CHECK (paid_influence_eighths>=0),
 CONSTRAINT cp_buff_canceled CHECK (canceled IN (0,1))
) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS clan_personal_favor (
 player_id BIGINT NOT NULL,
 clan_uid VARCHAR(32) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
 favor_eighths BIGINT NOT NULL DEFAULT 0,
 revision BIGINT NOT NULL DEFAULT 0,
 updated_at_utc BIGINT NOT NULL,
 PRIMARY KEY (player_id,clan_uid),
 CONSTRAINT cp_favor_clan FOREIGN KEY (clan_uid) REFERENCES clan_progression_clans(clan_uid),
 CONSTRAINT cp_favor_player CHECK (player_id>0),
 CONSTRAINT cp_favor_balance CHECK (favor_eighths>=0),
 CONSTRAINT cp_favor_revision CHECK (revision>=0),
 CONSTRAINT cp_favor_time CHECK (updated_at_utc>=0)
) ENGINE=InnoDB;
DELIMITER //
DROP TRIGGER IF EXISTS cp_favor_retained//
CREATE TRIGGER cp_favor_retained BEFORE DELETE ON clan_personal_favor FOR EACH ROW
BEGIN
 SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Retain historical Favor';
END//
DROP TRIGGER IF EXISTS cp_favor_monotonic//
CREATE TRIGGER cp_favor_monotonic BEFORE UPDATE ON clan_personal_favor FOR EACH ROW
BEGIN
 IF NEW.player_id<>OLD.player_id OR BINARY NEW.clan_uid<>BINARY OLD.clan_uid OR
    NEW.favor_eighths<OLD.favor_eighths OR NEW.revision<>OLD.revision+1 OR
    NEW.updated_at_utc<OLD.updated_at_utc THEN
  SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Favor identity/history is immutable and cumulative';
 END IF;
END//
DELIMITER ;
UPDATE clan_progression_schema SET version=3 WHERE singleton=1 AND version=2;
