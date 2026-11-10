-- Phase 8: durable accepted events, immutable award receipts and clan budgets.
-- Explicit offline migration after v004; never --force.
DELIMITER //
DROP PROCEDURE IF EXISTS clan_progression_v005_preflight//
CREATE PROCEDURE clan_progression_v005_preflight()
BEGIN
 IF NOT EXISTS (SELECT 1 FROM clan_progression_schema WHERE singleton=1 AND version IN (4,5)) THEN
  SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Clan schema v5 requires version 4 or 5';
 END IF;
END//
CALL clan_progression_v005_preflight()//
DROP PROCEDURE clan_progression_v005_preflight//
DELIMITER ;
CREATE TABLE IF NOT EXISTS clan_weekly_windows (
 clan_uid VARCHAR(32) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
 starts_at_utc BIGINT NOT NULL,
 incoming_xp_eighths BIGINT NOT NULL DEFAULT 0,
 influence_eighths BIGINT NOT NULL DEFAULT 0,
 revision BIGINT NOT NULL DEFAULT 0,
 updated_at_utc BIGINT NOT NULL,
 PRIMARY KEY (clan_uid,starts_at_utc),
 CONSTRAINT cp_weekly_clan FOREIGN KEY (clan_uid) REFERENCES clan_progression_clans(clan_uid),
 CONSTRAINT cp_weekly_monday CHECK (starts_at_utc BETWEEN -259200 AND 9223372036854171007 AND (starts_at_utc+259200) MOD 604800=0),
 CONSTRAINT cp_weekly_totals CHECK (incoming_xp_eighths>=0 AND influence_eighths>=0 AND revision>=0),
 CONSTRAINT cp_weekly_time CHECK (updated_at_utc>=0 AND updated_at_utc>=starts_at_utc)
) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS clan_reward_receipts (
 operation_uid VARCHAR(32) CHARACTER SET ascii COLLATE ascii_bin NOT NULL PRIMARY KEY,
 event_uid VARCHAR(32) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
 clan_uid VARCHAR(32) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
 beneficiary_key VARCHAR(32) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
 account_id BIGINT NOT NULL,
 player_id BIGINT NOT NULL,
 source_id INT NOT NULL,
 occurred_at_utc BIGINT NOT NULL,
 starts_at_utc BIGINT NOT NULL,
 rules_version INT NOT NULL,
 requested_xp_eighths BIGINT NOT NULL,
 requested_influence_eighths BIGINT NOT NULL,
 requested_favor_eighths BIGINT NOT NULL,
 receipt_status INT NOT NULL DEFAULT 0 COMMENT '0 durable pending, 1 SQL committed, 2 canceled; no file-delivered implication',
 accepted_at_utc BIGINT NOT NULL,
 committed_at_utc BIGINT NOT NULL DEFAULT 0,
 decision LONGTEXT NULL,
 UNIQUE KEY cp_reward_claim (event_uid,source_id,beneficiary_key),
 KEY cp_reward_pending (receipt_status,operation_uid),
 KEY cp_reward_player_pending (player_id,receipt_status),
 CONSTRAINT cp_reward_clan FOREIGN KEY (clan_uid) REFERENCES clan_progression_clans(clan_uid),
 CONSTRAINT cp_reward_operation CHECK (OCTET_LENGTH(operation_uid)=32 AND operation_uid REGEXP '^[0-9a-f]{32}$' AND operation_uid<>'00000000000000000000000000000000'),
 CONSTRAINT cp_reward_event CHECK (OCTET_LENGTH(event_uid)=32 AND event_uid REGEXP '^[0-9a-f]{32}$' AND event_uid<>'00000000000000000000000000000000'),
 CONSTRAINT cp_reward_identity CHECK ((source_id BETWEEN 1 AND 6 AND player_id>0 AND account_id>0 AND beneficiary_key=CONVERT(player_id USING ascii) COLLATE ascii_bin) OR (source_id BETWEEN 7 AND 9 AND player_id=0 AND account_id=0 AND beneficiary_key=clan_uid AND requested_favor_eighths=0)),
 CONSTRAINT cp_reward_requested CHECK (requested_xp_eighths>=0 AND requested_influence_eighths>=0 AND requested_favor_eighths>=0 AND rules_version>0),
 CONSTRAINT cp_reward_period CHECK (starts_at_utc BETWEEN -259200 AND 9223372036854171007 AND (starts_at_utc+259200) MOD 604800=0 AND occurred_at_utc>=0 AND occurred_at_utc>=starts_at_utc AND occurred_at_utc<starts_at_utc+604800),
 CONSTRAINT cp_reward_time CHECK (accepted_at_utc>=occurred_at_utc),
 CONSTRAINT cp_reward_status CHECK ((receipt_status=0 AND committed_at_utc=0 AND decision IS NULL) OR (receipt_status IN (1,2) AND committed_at_utc>=accepted_at_utc AND decision IS NOT NULL AND JSON_VALID(decision)))
) ENGINE=InnoDB;
DELIMITER //
DROP TRIGGER IF EXISTS cp_weekly_retained//
CREATE TRIGGER cp_weekly_retained BEFORE DELETE ON clan_weekly_windows FOR EACH ROW
BEGIN
 SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Retain clan budget history';
END//
DROP TRIGGER IF EXISTS cp_weekly_monotonic//
CREATE TRIGGER cp_weekly_monotonic BEFORE UPDATE ON clan_weekly_windows FOR EACH ROW
BEGIN
 IF BINARY NEW.clan_uid<>BINARY OLD.clan_uid OR NEW.starts_at_utc<>OLD.starts_at_utc OR
    NEW.incoming_xp_eighths<OLD.incoming_xp_eighths OR NEW.influence_eighths<OLD.influence_eighths OR
    NEW.revision<>OLD.revision+1 OR NEW.updated_at_utc<OLD.updated_at_utc THEN
  SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Clan budget identity/history is immutable and cumulative';
 END IF;
END//
DROP TRIGGER IF EXISTS cp_reward_pending_insert//
CREATE TRIGGER cp_reward_pending_insert BEFORE INSERT ON clan_reward_receipts FOR EACH ROW
BEGIN
 IF NEW.receipt_status<>0 THEN
  SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='New reward facts must begin pending';
 END IF;
END//
DROP TRIGGER IF EXISTS cp_reward_retained//
CREATE TRIGGER cp_reward_retained BEFORE DELETE ON clan_reward_receipts FOR EACH ROW
BEGIN
 SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Retain reward event receipts';
END//
DROP TRIGGER IF EXISTS cp_reward_immutable//
CREATE TRIGGER cp_reward_immutable BEFORE UPDATE ON clan_reward_receipts FOR EACH ROW
BEGIN
 IF OLD.receipt_status<>0 OR NEW.receipt_status NOT IN (1,2) OR
    BINARY NEW.operation_uid<>BINARY OLD.operation_uid OR BINARY NEW.event_uid<>BINARY OLD.event_uid OR
    BINARY NEW.clan_uid<>BINARY OLD.clan_uid OR BINARY NEW.beneficiary_key<>BINARY OLD.beneficiary_key OR
    NEW.account_id<>OLD.account_id OR NEW.player_id<>OLD.player_id OR NEW.source_id<>OLD.source_id OR
    NEW.occurred_at_utc<>OLD.occurred_at_utc OR NEW.starts_at_utc<>OLD.starts_at_utc OR
    NEW.rules_version<>OLD.rules_version OR NEW.requested_xp_eighths<>OLD.requested_xp_eighths OR
    NEW.requested_influence_eighths<>OLD.requested_influence_eighths OR
    NEW.requested_favor_eighths<>OLD.requested_favor_eighths OR NEW.accepted_at_utc<>OLD.accepted_at_utc THEN
  SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Reward identity, request and terminal decisions are immutable';
 END IF;
END//
DELIMITER ;
UPDATE clan_progression_schema SET version=5 WHERE singleton=1 AND version=4;
