-- Phase 15 follow-up / schema v8. Boss eligibility and reward routing are now
-- native mobile data: MOB_LEGENDARY_BOSS.  Remove the obsolete SQL registry.
-- Existing boss claims remain intact for cooldown/audit history.
DROP TABLE IF EXISTS legendary_boss_registry;
UPDATE legendary_schema SET version=8 WHERE singleton=1 AND version=7;
