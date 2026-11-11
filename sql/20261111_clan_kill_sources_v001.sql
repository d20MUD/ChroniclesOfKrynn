-- Optional Phase 26 extension to core v5. Apply offline before enabling earnings.
-- The core control-row lock serializes acceptance and the prototype/day quota.
CREATE TABLE IF NOT EXISTS clan_kill_sources (
 operation_uid VARCHAR(32) CHARACTER SET ascii COLLATE ascii_bin NOT NULL PRIMARY KEY,
 event_uid VARCHAR(32) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
 account_id BIGINT NOT NULL,
 prototype BIGINT NOT NULL,
 utc_day BIGINT NOT NULL,
 CONSTRAINT cp_kill_receipt FOREIGN KEY(operation_uid) REFERENCES clan_reward_receipts(operation_uid),
 CONSTRAINT cp_kill_identity CHECK(account_id>0 AND prototype>0 AND utc_day>=0),
 UNIQUE KEY cp_kill_account_event(account_id,event_uid),
 KEY cp_kill_daily(account_id,prototype,utc_day)
) ENGINE=InnoDB;
DELIMITER //
DROP TRIGGER IF EXISTS cp_kill_retained//
CREATE TRIGGER cp_kill_retained BEFORE DELETE ON clan_kill_sources FOR EACH ROW
BEGIN
 SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Retain clan kill source history';
END//
DROP TRIGGER IF EXISTS cp_kill_immutable//
CREATE TRIGGER cp_kill_immutable BEFORE UPDATE ON clan_kill_sources FOR EACH ROW
BEGIN
 SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Clan kill source facts are immutable';
END//
DELIMITER ;
