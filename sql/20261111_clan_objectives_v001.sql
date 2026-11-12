-- Optional Phases 30-31 extension to core v5. Apply explicitly offline.
-- No enablement, live imports or existing delivery schema changes.
CREATE TABLE IF NOT EXISTS clan_objective_schema(version INT PRIMARY KEY) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS clan_objective_contracts (
 contract_uid VARCHAR(32) CHARACTER SET ascii COLLATE ascii_bin PRIMARY KEY,
 clan_uid VARCHAR(32) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
 player_id BIGINT NOT NULL,account_id BIGINT NOT NULL,family INT NOT NULL,
 definition_id VARCHAR(32) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
 terms JSON NOT NULL,accepted_at BIGINT NOT NULL,expires_at BIGINT NOT NULL,week_start BIGINT NOT NULL,
 progress INT NOT NULL DEFAULT 0,state INT NOT NULL DEFAULT 0,
 active_player BIGINT AS (IF(state IN (0,1),player_id,NULL)) PERSISTENT,
 UNIQUE KEY co_slot(active_player,family),
 FOREIGN KEY(clan_uid) REFERENCES clan_progression_clans(clan_uid),
 CHECK(player_id>0 AND account_id>0 AND family IN (4,5) AND progress BETWEEN 0 AND 20 AND state BETWEEN 0 AND 4 AND expires_at>accepted_at AND (state NOT IN (1,2) OR progress=20))
) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS clan_objective_events (
 contract_uid VARCHAR(32) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
 event_uid VARCHAR(32) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
 account_id BIGINT NOT NULL,family INT NOT NULL,player_id BIGINT NOT NULL,
 occurred_at BIGINT NOT NULL,target INT NOT NULL,grade INT NOT NULL,base_units INT NOT NULL,room BIGINT NOT NULL,
 PRIMARY KEY(contract_uid,event_uid),UNIQUE KEY co_event_account(account_id,family,event_uid),
 FOREIGN KEY(contract_uid) REFERENCES clan_objective_contracts(contract_uid),
 CHECK(family IN (4,5) AND base_units BETWEEN 1 AND 20 AND target>0 AND room>=0 AND player_id>0 AND account_id>0 AND occurred_at>=0 AND ((family=4 AND grade BETWEEN 1 AND 10) OR (family=5 AND grade=0 AND base_units=1)))
) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS clan_objective_claims (
 account_id BIGINT NOT NULL,week_start BIGINT NOT NULL,family INT NOT NULL,
 definition_id VARCHAR(32) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
 contract_uid VARCHAR(32) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
 PRIMARY KEY(account_id,week_start,family,definition_id),UNIQUE KEY co_claim_contract(contract_uid),
 FOREIGN KEY(contract_uid) REFERENCES clan_objective_contracts(contract_uid)
) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS clan_bounty_shared_claims (
 clan_uid VARCHAR(32) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,week_start BIGINT NOT NULL,
 definition_id VARCHAR(32) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
 operation_uid VARCHAR(32) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
 PRIMARY KEY(clan_uid,week_start,definition_id),UNIQUE KEY co_shared_operation(operation_uid),
 FOREIGN KEY(operation_uid) REFERENCES clan_reward_receipts(operation_uid),
 FOREIGN KEY(clan_uid) REFERENCES clan_progression_clans(clan_uid)
) ENGINE=InnoDB;
DELIMITER //
CREATE OR REPLACE TRIGGER co_terms BEFORE UPDATE ON clan_objective_contracts FOR EACH ROW
BEGIN
 IF NEW.contract_uid<>OLD.contract_uid OR NEW.clan_uid<>OLD.clan_uid OR NEW.player_id<>OLD.player_id OR NEW.account_id<>OLD.account_id OR NEW.family<>OLD.family OR NEW.definition_id<>OLD.definition_id OR NEW.terms<>OLD.terms OR NEW.accepted_at<>OLD.accepted_at OR NEW.expires_at<>OLD.expires_at OR NEW.week_start<>OLD.week_start OR NEW.progress<OLD.progress OR (OLD.state<>0 AND NEW.progress<>OLD.progress) OR (OLD.state<>0 AND NOT (OLD.state=1 AND NEW.state=2) AND NEW.state<>OLD.state) THEN
 SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Immutable objective terms/progress'; END IF;
END//
CREATE OR REPLACE TRIGGER co_contract_retained BEFORE DELETE ON clan_objective_contracts FOR EACH ROW
BEGIN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Retain objective contracts'; END//
CREATE OR REPLACE TRIGGER co_event_retained BEFORE DELETE ON clan_objective_events FOR EACH ROW
BEGIN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Retain objective events'; END//
CREATE OR REPLACE TRIGGER co_event_immutable BEFORE UPDATE ON clan_objective_events FOR EACH ROW
BEGIN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Immutable objective events'; END//
CREATE OR REPLACE TRIGGER co_claim_retained BEFORE DELETE ON clan_objective_claims FOR EACH ROW
BEGIN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Retain objective claims'; END//
CREATE OR REPLACE TRIGGER co_claim_immutable BEFORE UPDATE ON clan_objective_claims FOR EACH ROW
BEGIN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Immutable objective claims'; END//
CREATE OR REPLACE TRIGGER co_shared_retained BEFORE DELETE ON clan_bounty_shared_claims FOR EACH ROW
BEGIN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Retain shared bounty claims'; END//
CREATE OR REPLACE TRIGGER co_shared_immutable BEFORE UPDATE ON clan_bounty_shared_claims FOR EACH ROW
BEGIN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Immutable shared bounty claims'; END//
DELIMITER ;
INSERT IGNORE INTO clan_objective_schema VALUES(1);
