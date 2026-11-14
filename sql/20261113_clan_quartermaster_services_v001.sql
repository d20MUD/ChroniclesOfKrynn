-- Additive quartermaster services. Select the game database before importing.
-- Existing gear purchase receipts are retained for recovery.
CREATE TABLE IF NOT EXISTS clan_quartermaster_services (
 operation_uid CHAR(32) CHARACTER SET ascii COLLATE ascii_bin NOT NULL PRIMARY KEY,
 player_id BIGINT NOT NULL,
 account_id BIGINT NOT NULL,
 clan_uid CHAR(32) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
 week_start BIGINT NOT NULL,
 kind INT NOT NULL COMMENT '1 motes; 2 quest points; 3 weapon enchantment',
 quantity INT NOT NULL,
 mote INT NOT NULL DEFAULT 0,
 price INT NOT NULL,
 weapon_uid CHAR(32) CHARACTER SET ascii COLLATE ascii_bin NOT NULL DEFAULT '',
 effect INT NOT NULL DEFAULT 0,
 command_word VARCHAR(23) CHARACTER SET ascii COLLATE ascii_bin NOT NULL DEFAULT '',
 bane_race INT NOT NULL DEFAULT 0,
 reroll TINYINT NOT NULL DEFAULT 0,
 state TINYINT NOT NULL DEFAULT 0 COMMENT '0 accepted/pending; 1 delivered',
 active_player BIGINT GENERATED ALWAYS AS (CASE WHEN state=0 THEN player_id ELSE NULL END) STORED,
 reroll_week BIGINT GENERATED ALWAYS AS (CASE WHEN reroll=1 THEN week_start ELSE NULL END) STORED,
 UNIQUE KEY cqs_active(active_player),
 UNIQUE KEY cqs_weekly_weapon(weapon_uid,reroll_week),
 KEY cqs_player_week(player_id,week_start,kind),
 CONSTRAINT cqs_clan FOREIGN KEY(clan_uid) REFERENCES clan_progression_clans(clan_uid),
 CONSTRAINT cqs_ids CHECK(player_id>0 AND account_id>0 AND week_start>=0),
 CONSTRAINT cqs_values CHECK(kind BETWEEN 1 AND 3 AND quantity BETWEEN 1 AND 1000 AND price>0 AND mote BETWEEN 0 AND 8),
 CONSTRAINT cqs_state CHECK(state IN (0,1) AND reroll IN (0,1)),
 CONSTRAINT cqs_weapon CHECK((kind=3 AND OCTET_LENGTH(weapon_uid)=32 AND effect BETWEEN 1 AND 64 AND OCTET_LENGTH(command_word)>0) OR
                           (kind IN (1,2) AND weapon_uid='' AND effect=0 AND command_word='' AND reroll=0))
) ENGINE=InnoDB;

-- Acquisition facts and weekly usage are retained; only acknowledgement advances.
DELIMITER //
DROP TRIGGER IF EXISTS cqs_no_delete//
CREATE TRIGGER cqs_no_delete BEFORE DELETE ON clan_quartermaster_services FOR EACH ROW
BEGIN
 SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Quartermaster service receipts must be retained';
END//
DROP TRIGGER IF EXISTS cqs_immutable//
CREATE TRIGGER cqs_immutable BEFORE UPDATE ON clan_quartermaster_services FOR EACH ROW
BEGIN
 IF NEW.operation_uid<>OLD.operation_uid OR NEW.player_id<>OLD.player_id OR NEW.account_id<>OLD.account_id OR
    NEW.clan_uid<>OLD.clan_uid OR NEW.week_start<>OLD.week_start OR NEW.kind<>OLD.kind OR
    NEW.quantity<>OLD.quantity OR NEW.mote<>OLD.mote OR NEW.price<>OLD.price OR
    NEW.weapon_uid<>OLD.weapon_uid OR NEW.effect<>OLD.effect OR NEW.command_word<>OLD.command_word OR
    NEW.bane_race<>OLD.bane_race OR NEW.reroll<>OLD.reroll OR NEW.state<OLD.state THEN
   SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Quartermaster service facts are immutable';
 END IF;
END//
DELIMITER ;
