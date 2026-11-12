-- Optional Phases 28-29 extension to core v5. Offline application only.
-- This does not enable contracts. Keep history through disbanding and retries.
CREATE TABLE IF NOT EXISTS clan_delivery_schema(version INT PRIMARY KEY) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS clan_delivery_contracts (
 accepted_uid VARCHAR(32) CHARACTER SET ascii COLLATE ascii_bin PRIMARY KEY,
 clan_uid VARCHAR(32) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
 player_id BIGINT NOT NULL, account_id BIGINT NOT NULL,
 definition_id VARCHAR(32) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
 terms JSON NOT NULL, accepted_at BIGINT NOT NULL, expires_at BIGINT NOT NULL,
 week_start BIGINT NOT NULL, state INT NOT NULL DEFAULT 0,
 active_player BIGINT AS (IF(state=0,player_id,NULL)) PERSISTENT,
 UNIQUE KEY cp_delivery_slot(active_player),
 FOREIGN KEY(clan_uid) REFERENCES clan_progression_clans(clan_uid),
 CHECK(player_id>0 AND account_id>0 AND state BETWEEN 0 AND 3 AND expires_at>accepted_at)
) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS clan_delivery_operations (
 operation_uid VARCHAR(32) CHARACTER SET ascii COLLATE ascii_bin PRIMARY KEY,
 accepted_uid VARCHAR(32) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
 player_id BIGINT NOT NULL, quote JSON NOT NULL, state INT NOT NULL DEFAULT 0,
 held_player BIGINT AS (IF(state IN (0,1),player_id,NULL)) PERSISTENT,
 UNIQUE KEY cp_delivery_hold(held_player), UNIQUE KEY cp_delivery_event(accepted_uid),
 FOREIGN KEY(operation_uid) REFERENCES clan_reward_receipts(operation_uid),
 FOREIGN KEY(accepted_uid) REFERENCES clan_delivery_contracts(accepted_uid),
 CHECK(state BETWEEN 0 AND 3)
) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS clan_delivery_claims (
 account_id BIGINT NOT NULL, week_start BIGINT NOT NULL,
 definition_id VARCHAR(32) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
 operation_uid VARCHAR(32) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
 PRIMARY KEY(account_id,week_start,definition_id), UNIQUE KEY cp_delivery_claim(operation_uid),
 FOREIGN KEY(operation_uid) REFERENCES clan_delivery_operations(operation_uid)
) ENGINE=InnoDB;
DELIMITER //
CREATE OR REPLACE TRIGGER cp_delivery_terms BEFORE UPDATE ON clan_delivery_contracts FOR EACH ROW
BEGIN
 IF NEW.accepted_uid<>OLD.accepted_uid OR NEW.clan_uid<>OLD.clan_uid OR NEW.player_id<>OLD.player_id
 OR NEW.account_id<>OLD.account_id OR NEW.definition_id<>OLD.definition_id OR NEW.terms<>OLD.terms
 OR NEW.accepted_at<>OLD.accepted_at OR NEW.expires_at<>OLD.expires_at OR NEW.week_start<>OLD.week_start
 OR (OLD.state<>0 AND NEW.state<>OLD.state) THEN
 SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Delivery terms and terminal states are immutable'; END IF;
END//
CREATE OR REPLACE TRIGGER cp_delivery_operation_facts BEFORE UPDATE ON clan_delivery_operations FOR EACH ROW
BEGIN
 IF NEW.operation_uid<>OLD.operation_uid OR NEW.accepted_uid<>OLD.accepted_uid OR NEW.player_id<>OLD.player_id
 OR NEW.quote<>OLD.quote OR NOT (NEW.state=OLD.state OR (OLD.state=0 AND NEW.state IN (1,3)) OR (OLD.state=1 AND NEW.state=2))
 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Invalid delivery transition'; END IF;
END//
CREATE OR REPLACE TRIGGER cp_delivery_contract_retained BEFORE DELETE ON clan_delivery_contracts FOR EACH ROW
BEGIN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Retain delivery contracts'; END//
CREATE OR REPLACE TRIGGER cp_delivery_operation_retained BEFORE DELETE ON clan_delivery_operations FOR EACH ROW
BEGIN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Retain delivery operations'; END//
CREATE OR REPLACE TRIGGER cp_delivery_claim_retained BEFORE DELETE ON clan_delivery_claims FOR EACH ROW
BEGIN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Retain delivery claims'; END//
CREATE OR REPLACE TRIGGER cp_delivery_claim_immutable BEFORE UPDATE ON clan_delivery_claims FOR EACH ROW
BEGIN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Immutable delivery claims'; END//
DELIMITER ;
INSERT IGNORE INTO clan_delivery_schema VALUES(1);
