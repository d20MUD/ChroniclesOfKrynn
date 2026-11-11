-- Phase 16 lifecycle extension. Offline, quiesced writers; core v5 plus
-- clan_buff_activations_v001 required. Does not select a DB or enable gameplay.
DELIMITER //
DROP PROCEDURE IF EXISTS clan_buff_lifecycle_preflight//
CREATE PROCEDURE clan_buff_lifecycle_preflight()
BEGIN
 IF NOT EXISTS(SELECT 1 FROM clan_progression_schema WHERE singleton=1 AND version=5)
 OR NOT EXISTS(SELECT 1 FROM clan_buff_activation_schema WHERE singleton=1 AND version=1) THEN
  SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Buff lifecycle requires core v5 and activation v1';
 END IF;
END//
CALL clan_buff_lifecycle_preflight()//
DROP PROCEDURE clan_buff_lifecycle_preflight//
DELIMITER ;
CREATE TABLE IF NOT EXISTS clan_buff_lifecycle_events (
 operation_uid VARCHAR(32) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
 clan_uid VARCHAR(32) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
 activation_uid VARCHAR(32) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
 actor_id BIGINT NOT NULL,
 event_kind INT NOT NULL COMMENT '1 cancel, 2 expire',
 occurred_at BIGINT NOT NULL,
 receipt LONGTEXT CHARACTER SET utf8mb4 COLLATE utf8mb4_bin NOT NULL,
 PRIMARY KEY(event_kind,operation_uid),
 UNIQUE KEY cble_one_transition(activation_uid),
 CONSTRAINT cble_clan FOREIGN KEY(clan_uid) REFERENCES clan_progression_clans(clan_uid),
 CONSTRAINT cble_identity CHECK(OCTET_LENGTH(operation_uid)=32 AND operation_uid REGEXP '^[0-9a-f]{32}$' AND operation_uid<>'00000000000000000000000000000000' AND OCTET_LENGTH(activation_uid)=32 AND activation_uid REGEXP '^[0-9a-f]{32}$' AND activation_uid<>'00000000000000000000000000000000'),
 CONSTRAINT cble_event CHECK((event_kind=1 AND actor_id>0) OR (event_kind=2 AND actor_id=0)),
 CONSTRAINT cble_time CHECK(occurred_at>=0),
 CONSTRAINT cble_json CHECK(JSON_VALID(receipt))
) ENGINE=InnoDB;
DELIMITER //
DROP TRIGGER IF EXISTS cble_immutable//
CREATE TRIGGER cble_immutable BEFORE UPDATE ON clan_buff_lifecycle_events FOR EACH ROW
BEGIN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Buff lifecycle events are immutable'; END//
DROP TRIGGER IF EXISTS cble_retained//
CREATE TRIGGER cble_retained BEFORE DELETE ON clan_buff_lifecycle_events FOR EACH ROW
BEGIN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Buff lifecycle history is retained'; END//
DELIMITER ;
CREATE TABLE IF NOT EXISTS clan_buff_lifecycle_schema (
 singleton INT NOT NULL PRIMARY KEY CHECK(singleton=1),
 version INT NOT NULL CHECK(version=1)
) ENGINE=InnoDB;
INSERT INTO clan_buff_lifecycle_schema VALUES(1,1) ON DUPLICATE KEY UPDATE version=1;
