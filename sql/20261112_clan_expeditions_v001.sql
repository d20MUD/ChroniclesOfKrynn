-- Optional Phase 34 extension over core v5. Apply explicitly offline.
CREATE TABLE IF NOT EXISTS clan_expedition_schema(version INT PRIMARY KEY) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS clan_expedition_outings (
 outing_uid VARCHAR(32) CHARACTER SET ascii COLLATE ascii_bin PRIMARY KEY,
 clan_uid VARCHAR(32) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
 leader_id BIGINT NOT NULL, terms JSON NOT NULL,
 created_at BIGINT NOT NULL, started_at BIGINT NOT NULL DEFAULT 0, expires_at BIGINT NOT NULL, week_start BIGINT NOT NULL,
 state INT NOT NULL DEFAULT 0, roster_locked INT NOT NULL DEFAULT 0,
 active_clan VARCHAR(32) CHARACTER SET ascii COLLATE ascii_bin AS (IF(state IN (0,1),clan_uid,NULL)) PERSISTENT,
 UNIQUE KEY ce_active(active_clan),
 FOREIGN KEY(clan_uid) REFERENCES clan_progression_clans(clan_uid),
 CHECK(leader_id>0 AND created_at>=0 AND expires_at>created_at AND (started_at=0 OR (started_at>=created_at AND expires_at>started_at AND expires_at-started_at<=7200)) AND state IN (0,1,3,4) AND roster_locked IN (0,1))
) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS clan_expedition_roster (
 outing_uid VARCHAR(32) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
 player_id BIGINT NOT NULL, account_id BIGINT NOT NULL, enrolled_at BIGINT NOT NULL,
 inactive INT NOT NULL DEFAULT 0,
 PRIMARY KEY(outing_uid,player_id), UNIQUE KEY ce_account(outing_uid,account_id),
 FOREIGN KEY(outing_uid) REFERENCES clan_expedition_outings(outing_uid),
 CHECK(player_id>0 AND account_id>0 AND enrolled_at>=0 AND inactive IN (0,1))
) ENGINE=InnoDB;
DELIMITER //
CREATE OR REPLACE TRIGGER ce_outing_terms BEFORE UPDATE ON clan_expedition_outings FOR EACH ROW
BEGIN
 IF NEW.outing_uid<>OLD.outing_uid OR NEW.clan_uid<>OLD.clan_uid OR NEW.leader_id<>OLD.leader_id OR NEW.terms<>OLD.terms OR NEW.created_at<>OLD.created_at OR (NEW.expires_at<>OLD.expires_at AND NOT (OLD.state=0 AND NEW.state=1 AND NEW.expires_at<=OLD.expires_at)) OR (NEW.started_at<>OLD.started_at AND NOT (OLD.state=0 AND NEW.state=1 AND OLD.started_at=0 AND NEW.started_at>=OLD.created_at)) OR NEW.week_start<>OLD.week_start OR NEW.roster_locked<OLD.roster_locked OR (OLD.state IN (3,4) AND NEW.state<>OLD.state) OR (OLD.state=1 AND NEW.state=0) THEN
 SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Immutable expedition identity/terms'; END IF;
END//
CREATE OR REPLACE TRIGGER ce_outing_retained BEFORE DELETE ON clan_expedition_outings FOR EACH ROW
BEGIN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Retain expedition outings'; END//
CREATE OR REPLACE TRIGGER ce_roster_admission BEFORE INSERT ON clan_expedition_roster FOR EACH ROW
BEGIN
 IF NOT EXISTS (SELECT 1 FROM clan_expedition_outings o JOIN clan_memberships m ON m.clan_uid=o.clan_uid JOIN clan_progression_legacy l ON l.clan_uid=m.clan_uid WHERE o.outing_uid=NEW.outing_uid AND o.state IN (0,1) AND o.roster_locked=0 AND NEW.enrolled_at>=o.created_at AND NEW.enrolled_at<o.expires_at AND m.player_id=NEW.player_id AND m.rank_id BETWEEN 1 AND JSON_LENGTH(JSON_EXTRACT(l.details,'$.ranks'))) OR
 (SELECT COUNT(*) FROM clan_expedition_roster WHERE outing_uid=NEW.outing_uid AND inactive=0)>=6 OR
 (SELECT COUNT(*) FROM clan_expedition_roster WHERE outing_uid=NEW.outing_uid)>=32 THEN
 SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Ineligible or sealed expedition roster'; END IF;
END//
CREATE OR REPLACE TRIGGER ce_roster_terms BEFORE UPDATE ON clan_expedition_roster FOR EACH ROW
BEGIN
 IF NEW.outing_uid<>OLD.outing_uid OR NEW.player_id<>OLD.player_id OR NEW.account_id<>OLD.account_id OR NEW.enrolled_at<>OLD.enrolled_at OR NEW.inactive<OLD.inactive THEN
 SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Immutable expedition enrollment'; END IF;
END//
CREATE OR REPLACE TRIGGER ce_roster_retained BEFORE DELETE ON clan_expedition_roster FOR EACH ROW
BEGIN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Retain expedition enrollments'; END//
-- Persist loss of eligibility even when a character immediately rejoins.
CREATE OR REPLACE TRIGGER ce_membership_delete AFTER DELETE ON clan_memberships FOR EACH ROW
BEGIN
 UPDATE clan_expedition_roster r JOIN clan_expedition_outings o ON o.outing_uid=r.outing_uid
 SET r.inactive=1 WHERE r.player_id=OLD.player_id AND o.clan_uid=OLD.clan_uid AND r.inactive=0;
END//
CREATE OR REPLACE TRIGGER ce_membership_update AFTER UPDATE ON clan_memberships FOR EACH ROW
BEGIN
 IF NEW.clan_uid<>OLD.clan_uid OR NEW.rank_id=0 THEN
 UPDATE clan_expedition_roster r JOIN clan_expedition_outings o ON o.outing_uid=r.outing_uid
 SET r.inactive=1 WHERE r.player_id=OLD.player_id AND o.clan_uid=OLD.clan_uid AND r.inactive=0;
 END IF;
END//
DELIMITER ;
INSERT IGNORE INTO clan_expedition_schema VALUES(1);
