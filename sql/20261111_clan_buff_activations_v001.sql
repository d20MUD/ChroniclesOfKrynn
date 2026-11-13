-- Phases 14–15 extension to core clan schema v5. Offline, quiesced writers only.
-- Apply core v001..v005 first. No database selection, grants or enablement.
DELIMITER //
DROP PROCEDURE IF EXISTS clan_buff_activation_preflight//
CREATE PROCEDURE clan_buff_activation_preflight()
BEGIN
 IF NOT EXISTS(SELECT 1 FROM clan_progression_schema WHERE singleton=1 AND version IN (5,6)) THEN
  SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Buff activation requires core clan schema v5';
 END IF;
END//
CALL clan_buff_activation_preflight()//
DROP PROCEDURE clan_buff_activation_preflight//
DELIMITER ;
CREATE TABLE IF NOT EXISTS clan_buff_management (
 clan_uid VARCHAR(32) CHARACTER SET ascii COLLATE ascii_bin NOT NULL PRIMARY KEY,
 privilege_id INT NOT NULL DEFAULT 21,
 max_rank INT NOT NULL DEFAULT 0 COMMENT '0 leader only; otherwise accepted ranks 1..max_rank',
 CONSTRAINT cbm_clan FOREIGN KEY(clan_uid) REFERENCES clan_progression_clans(clan_uid),
 CONSTRAINT cbm_privilege CHECK(privilege_id=21),
 CONSTRAINT cbm_rank CHECK(max_rank BETWEEN 0 AND 15)
) ENGINE=InnoDB;
-- One immutable row is both the exactly-once receipt and the activation activity
-- log. Historical replacements remain here when the current family row changes.
CREATE TABLE IF NOT EXISTS clan_buff_activations (
 operation_uid VARCHAR(32) CHARACTER SET ascii COLLATE ascii_bin NOT NULL PRIMARY KEY,
 clan_uid VARCHAR(32) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
 actor_id BIGINT NOT NULL,
 family_id INT NOT NULL,
 tier INT NOT NULL,
 slot_id INT NOT NULL,
 rules_version INT NOT NULL,
 expected_activation_uid VARCHAR(32) CHARACTER SET ascii COLLATE ascii_bin NOT NULL DEFAULT '',
 receipt LONGTEXT CHARACTER SET utf8mb4 COLLATE utf8mb4_bin NOT NULL,
 CONSTRAINT cba_clan FOREIGN KEY(clan_uid) REFERENCES clan_progression_clans(clan_uid),
 CONSTRAINT cba_uid CHECK(OCTET_LENGTH(operation_uid)=32 AND operation_uid REGEXP '^[0-9a-f]{32}$' AND operation_uid<>'00000000000000000000000000000000'),
 CONSTRAINT cba_request CHECK(actor_id>0 AND family_id BETWEEN 1 AND 5 AND tier BETWEEN 1 AND 3 AND slot_id BETWEEN 1 AND 3 AND rules_version=1),
 CONSTRAINT cba_expected CHECK(expected_activation_uid='' OR (OCTET_LENGTH(expected_activation_uid)=32 AND expected_activation_uid REGEXP '^[0-9a-f]{32}$' AND expected_activation_uid<>'00000000000000000000000000000000')),
 CONSTRAINT cba_json CHECK(JSON_VALID(receipt)),
 KEY cba_clan_history(clan_uid,operation_uid)
) ENGINE=InnoDB;
DELIMITER //
DROP TRIGGER IF EXISTS cba_immutable//
CREATE TRIGGER cba_immutable BEFORE UPDATE ON clan_buff_activations FOR EACH ROW
BEGIN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Buff activation receipts are immutable'; END//
DROP TRIGGER IF EXISTS cba_retained//
CREATE TRIGGER cba_retained BEFORE DELETE ON clan_buff_activations FOR EACH ROW
BEGIN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Buff activation history is retained'; END//
DELIMITER ;
CREATE TABLE IF NOT EXISTS clan_buff_activation_schema (
 singleton INT NOT NULL PRIMARY KEY CHECK(singleton=1),
 version INT NOT NULL CHECK(version=1)
) ENGINE=InnoDB;
INSERT INTO clan_buff_activation_schema VALUES(1,1) ON DUPLICATE KEY UPDATE version=1;
