-- Optional Phases 35-36 extension. Apply offline after core v5 and expedition v1.
CREATE TABLE IF NOT EXISTS clan_expedition_events (
 outing_uid VARCHAR(32) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
 clan_uid VARCHAR(32) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
 event_uid VARCHAR(32) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
 occurred_at BIGINT NOT NULL, area INT NOT NULL, target INT NOT NULL, objective INT NOT NULL,
 participants JSON NOT NULL, facts JSON NOT NULL, room BIGINT NOT NULL,
 PRIMARY KEY(outing_uid,event_uid), UNIQUE KEY ce_encounter(clan_uid,event_uid),
 FOREIGN KEY(outing_uid) REFERENCES clan_expedition_outings(outing_uid),
 CHECK(occurred_at>=0 AND area>=0 AND target>0 AND objective BETWEEN 0 AND 2 AND JSON_LENGTH(participants) BETWEEN 2 AND 6)
) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS clan_expedition_completions (
 outing_uid VARCHAR(32) CHARACTER SET ascii COLLATE ascii_bin PRIMARY KEY,
 family INT NOT NULL, week_start BIGINT NOT NULL, completed_at BIGINT NOT NULL,
 expires_at BIGINT NOT NULL, objectives INT NOT NULL,
 FOREIGN KEY(outing_uid) REFERENCES clan_expedition_outings(outing_uid),
 CHECK(family IN (1,2) AND completed_at>=week_start AND expires_at>completed_at AND objectives IN (22,30))
) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS clan_expedition_entitlements (
 outing_uid VARCHAR(32) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
 player_id BIGINT NOT NULL, account_id BIGINT NOT NULL,
 operation_uid VARCHAR(32) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
 state INT NOT NULL DEFAULT 0,
 PRIMARY KEY(outing_uid,player_id), UNIQUE KEY ce_entitlement_operation(operation_uid),
 FOREIGN KEY(outing_uid) REFERENCES clan_expedition_completions(outing_uid),
 CHECK(player_id>0 AND account_id>0 AND state IN (0,1,2))
) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS clan_expedition_shared_claims (
 clan_uid VARCHAR(32) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
 week_start BIGINT NOT NULL, family INT NOT NULL,
 operation_uid VARCHAR(32) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
 PRIMARY KEY(clan_uid,week_start,family), UNIQUE KEY ce_shared_operation(operation_uid),
 FOREIGN KEY(operation_uid) REFERENCES clan_reward_receipts(operation_uid), CHECK(family IN (1,2))
) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS clan_expedition_personal_claims (
 account_id BIGINT NOT NULL, week_start BIGINT NOT NULL, family INT NOT NULL,
 operation_uid VARCHAR(32) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
 PRIMARY KEY(account_id,week_start,family), UNIQUE KEY ce_personal_operation(operation_uid),
 FOREIGN KEY(operation_uid) REFERENCES clan_reward_receipts(operation_uid), CHECK(account_id>0 AND family IN (1,2))
) ENGINE=InnoDB;
DELIMITER //
CREATE OR REPLACE TRIGGER ce_event_retained BEFORE DELETE ON clan_expedition_events FOR EACH ROW
BEGIN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Retain expedition events'; END//
CREATE OR REPLACE TRIGGER ce_event_immutable BEFORE UPDATE ON clan_expedition_events FOR EACH ROW
BEGIN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Immutable expedition events'; END//
CREATE OR REPLACE TRIGGER ce_completion_retained BEFORE DELETE ON clan_expedition_completions FOR EACH ROW
BEGIN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Retain expedition completions'; END//
CREATE OR REPLACE TRIGGER ce_completion_immutable BEFORE UPDATE ON clan_expedition_completions FOR EACH ROW
BEGIN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Immutable expedition completions'; END//
CREATE OR REPLACE TRIGGER ce_entitlement_retained BEFORE DELETE ON clan_expedition_entitlements FOR EACH ROW
BEGIN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Retain expedition entitlements'; END//
CREATE OR REPLACE TRIGGER ce_entitlement_terms BEFORE UPDATE ON clan_expedition_entitlements FOR EACH ROW
BEGIN
 IF NEW.outing_uid<>OLD.outing_uid OR NEW.player_id<>OLD.player_id OR NEW.account_id<>OLD.account_id OR NEW.operation_uid<>OLD.operation_uid OR NEW.state<OLD.state OR NEW.state>OLD.state+1 THEN
 SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Immutable expedition entitlement'; END IF;
END//
CREATE OR REPLACE TRIGGER ce_shared_retained BEFORE DELETE ON clan_expedition_shared_claims FOR EACH ROW
BEGIN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Retain shared expedition claims'; END//
CREATE OR REPLACE TRIGGER ce_shared_immutable BEFORE UPDATE ON clan_expedition_shared_claims FOR EACH ROW
BEGIN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Immutable shared expedition claims'; END//
CREATE OR REPLACE TRIGGER ce_personal_retained BEFORE DELETE ON clan_expedition_personal_claims FOR EACH ROW
BEGIN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Retain personal expedition claims'; END//
CREATE OR REPLACE TRIGGER ce_personal_immutable BEFORE UPDATE ON clan_expedition_personal_claims FOR EACH ROW
BEGIN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Immutable personal expedition claims'; END//
-- Invalidate unclaimed entitlements after departure, including immediate rejoin.
-- Prepared receipts retain their original beneficiary and recover independently.
CREATE OR REPLACE TRIGGER ce_membership_delete AFTER DELETE ON clan_memberships FOR EACH ROW
BEGIN UPDATE clan_expedition_roster r JOIN clan_expedition_outings o ON o.outing_uid=r.outing_uid
 SET r.inactive=1 WHERE r.player_id=OLD.player_id AND o.clan_uid=OLD.clan_uid AND r.inactive=0; END//
CREATE OR REPLACE TRIGGER ce_membership_update AFTER UPDATE ON clan_memberships FOR EACH ROW
BEGIN IF NEW.clan_uid<>OLD.clan_uid OR NEW.rank_id=0 THEN
 UPDATE clan_expedition_roster r JOIN clan_expedition_outings o ON o.outing_uid=r.outing_uid
 SET r.inactive=1 WHERE r.player_id=OLD.player_id AND o.clan_uid=OLD.clan_uid AND r.inactive=0;
 END IF; END//
DELIMITER ;
INSERT IGNORE INTO clan_expedition_schema VALUES(2);
