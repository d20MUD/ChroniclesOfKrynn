-- Offline, pre-release cutover to rules 2. Never run against an enabled server.
-- This migration deliberately refuses an already-used economy: legacy XP is
-- not evidence of lifetime Influence, and receipts must never be rewritten.
DELIMITER //
DROP PROCEDURE IF EXISTS clan_progression_v006_preflight//
CREATE PROCEDURE clan_progression_v006_preflight()
BEGIN
 IF NOT EXISTS (SELECT 1 FROM clan_progression_schema WHERE singleton=1 AND version IN (5,6)) THEN
  SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Clan schema v6 requires version 5 or 6';
 END IF;
 IF EXISTS (SELECT 1 FROM clan_progression_schema WHERE singleton=1 AND version=5) THEN
  IF EXISTS (SELECT 1 FROM clan_progression_state WHERE xp_eighths<>0 OR influence_eighths<>0)
     OR EXISTS (SELECT 1 FROM clan_personal_favor WHERE favor_eighths<>0)
     OR EXISTS (SELECT 1 FROM clan_account_windows)
     OR EXISTS (SELECT 1 FROM clan_weekly_windows)
     OR EXISTS (SELECT 1 FROM clan_reward_receipts WHERE receipt_status=0)
     OR EXISTS (SELECT 1 FROM clan_buff_families WHERE activation_uid IS NOT NULL) THEN
   SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Rules 2 requires an unused economy; preserve backup and reconcile earned progression before cutover';
  END IF;
 END IF;
END//
CALL clan_progression_v006_preflight()//
DROP PROCEDURE clan_progression_v006_preflight//
DELIMITER ;
-- Under rules 2 xp_eighths is lifetime earned Influence, independent of spending.
UPDATE clan_progression_schema SET version=6 WHERE singleton=1 AND version=5;
