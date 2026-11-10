-- Phase 7: retained account-global weekly contribution windows.
-- Explicit offline migration after v003; never use --force.
DELIMITER //
DROP PROCEDURE IF EXISTS clan_progression_v004_preflight//
CREATE PROCEDURE clan_progression_v004_preflight()
BEGIN
 IF NOT EXISTS (SELECT 1 FROM clan_progression_schema WHERE singleton=1 AND version IN (3,4)) THEN
  SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Clan schema v4 requires version 3 or 4';
 END IF;
END//
CALL clan_progression_v004_preflight()//
DROP PROCEDURE clan_progression_v004_preflight//
DELIMITER ;
CREATE TABLE IF NOT EXISTS clan_account_windows (
 account_id BIGINT NOT NULL,
 starts_at_utc BIGINT NOT NULL,
 base_xp_eighths BIGINT NOT NULL DEFAULT 0,
 favor_eighths BIGINT NOT NULL DEFAULT 0,
 kill_claims BIGINT NOT NULL DEFAULT 0,
 supply_order_claims BIGINT NOT NULL DEFAULT 0,
 delivery_claims BIGINT NOT NULL DEFAULT 0,
 gathering_claims BIGINT NOT NULL DEFAULT 0,
 bounty_claims BIGINT NOT NULL DEFAULT 0,
 expedition_claims BIGINT NOT NULL DEFAULT 0,
 revision BIGINT NOT NULL DEFAULT 0,
 updated_at_utc BIGINT NOT NULL,
 PRIMARY KEY (account_id,starts_at_utc),
 CONSTRAINT cp_account_identity CHECK (account_id>0),
 CONSTRAINT cp_account_monday CHECK (starts_at_utc BETWEEN -259200 AND 9223372036854171007 AND (starts_at_utc+259200) MOD 604800=0),
 CONSTRAINT cp_account_totals CHECK (base_xp_eighths>=0 AND favor_eighths>=0),
 CONSTRAINT cp_account_claims CHECK (kill_claims>=0 AND supply_order_claims>=0 AND delivery_claims>=0 AND gathering_claims>=0 AND bounty_claims>=0 AND expedition_claims>=0),
 CONSTRAINT cp_account_revision CHECK (revision>=0),
 CONSTRAINT cp_account_time CHECK (updated_at_utc>=0 AND updated_at_utc>=starts_at_utc)
) ENGINE=InnoDB;
DELIMITER //
DROP TRIGGER IF EXISTS cp_account_window_retained//
CREATE TRIGGER cp_account_window_retained BEFORE DELETE ON clan_account_windows FOR EACH ROW
BEGIN
 SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Retain account contribution windows';
END//
DROP TRIGGER IF EXISTS cp_account_window_monotonic//
CREATE TRIGGER cp_account_window_monotonic BEFORE UPDATE ON clan_account_windows FOR EACH ROW
BEGIN
 IF NEW.account_id<>OLD.account_id OR NEW.starts_at_utc<>OLD.starts_at_utc OR
    NEW.base_xp_eighths<OLD.base_xp_eighths OR NEW.favor_eighths<OLD.favor_eighths OR
    NEW.kill_claims<OLD.kill_claims OR NEW.supply_order_claims<OLD.supply_order_claims OR
    NEW.delivery_claims<OLD.delivery_claims OR NEW.gathering_claims<OLD.gathering_claims OR
    NEW.bounty_claims<OLD.bounty_claims OR NEW.expedition_claims<OLD.expedition_claims OR
    NEW.revision<>OLD.revision+1 OR NEW.updated_at_utc<OLD.updated_at_utc THEN
  SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Account window identity/history is immutable and cumulative';
 END IF;
END//
DELIMITER ;
UPDATE clan_progression_schema SET version=4 WHERE singleton=1 AND version=3;
