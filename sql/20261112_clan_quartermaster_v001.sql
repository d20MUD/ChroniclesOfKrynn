-- Optional extension to core v5. Explicit offline installation; no feature enablement.
CREATE TABLE IF NOT EXISTS clan_quartermaster_schema(version INT PRIMARY KEY CHECK(version=1)) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS clan_quartermaster_operations (
 operation_uid CHAR(32) CHARACTER SET ascii COLLATE ascii_bin PRIMARY KEY,
 clan_uid CHAR(32) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
 player_id BIGINT NOT NULL CHECK(player_id>0), account_id BIGINT NOT NULL CHECK(account_id>0),
 created_at BIGINT NOT NULL CHECK(created_at>=0), terms JSON NOT NULL,
 payload MEDIUMTEXT NOT NULL, state INT NOT NULL DEFAULT 0 CHECK(state IN(0,1)),
 active_player BIGINT GENERATED ALWAYS AS (CASE WHEN state=0 THEN player_id ELSE NULL END) STORED,
 UNIQUE KEY qm_active(active_player), KEY qm_player(player_id,created_at),
 FOREIGN KEY(clan_uid) REFERENCES clan_progression_clans(clan_uid),
 CHECK(operation_uid REGEXP '^[0-9a-f]{32}$'), CHECK(LENGTH(payload) BETWEEN 1 AND 32768)
) ENGINE=InnoDB;
DELIMITER //
CREATE OR REPLACE TRIGGER qm_terms BEFORE UPDATE ON clan_quartermaster_operations FOR EACH ROW
BEGIN
 IF NEW.operation_uid<>OLD.operation_uid OR NEW.clan_uid<>OLD.clan_uid OR NEW.player_id<>OLD.player_id OR NEW.account_id<>OLD.account_id OR NEW.created_at<>OLD.created_at OR BINARY NEW.terms<>BINARY OLD.terms OR BINARY NEW.payload<>BINARY OLD.payload OR NEW.state<OLD.state THEN
 SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Immutable quartermaster purchase'; END IF;
END//
CREATE OR REPLACE TRIGGER qm_retained BEFORE DELETE ON clan_quartermaster_operations FOR EACH ROW
BEGIN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Retain quartermaster purchase'; END//
DELIMITER ;
INSERT IGNORE INTO clan_quartermaster_schema VALUES(1);
