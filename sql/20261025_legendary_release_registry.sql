-- Phase 15 reviewed service configuration. This file is intentionally NOT a
-- world-file edit and does not enable the global earnings/purchase/effect gates.
-- Apply only after schema v8 and the content-verification script pass.

-- Lord Orelor is an existing Elder Forger of Reorx: mob 10500, reset into room
-- 8795 by zone 87.  His service remains inert until purchases are enabled.
INSERT INTO legendary_smiths(mob_vnum,enabled) VALUES (10500,TRUE)
ON DUPLICATE KEY UPDATE enabled=VALUES(enabled),version=version+1;

-- Bosses are no longer database configuration. Set the MEDIT mobile flag
-- Legendary-Boss (MOB_LEGENDARY_BOSS) on approved encounter prototypes, then
-- save their existing zone mobile files through the normal builder workflow.
-- Do not flag summons, mounts, familiars, pets, quest-only actors or mobs that
-- players cannot normally fight. Tier derives from level; each eligible death
-- rolls and durably records one of the six essence types.
