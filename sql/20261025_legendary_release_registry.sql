-- Phase 15 reviewed service configuration. This file is intentionally NOT a
-- world-file edit and does not enable the global earnings/purchase/effect gates.
-- Apply only after schema v9 and the content-verification script pass.

-- Assign Infusion Smith to Lord Orelor Silverveins (mob 10500, room 8795)
-- through MEDIT's Special Procedure menu and save the mobile's zone file.
-- This is world content, not database configuration. Purchases remain gated.

-- Bosses are no longer database configuration. Set the MEDIT mobile flag
-- Legendary-Boss (MOB_LEGENDARY_BOSS) on approved encounter prototypes, then
-- save their existing zone mobile files through the normal builder workflow.
-- Do not flag summons, mounts, familiars, pets, quest-only actors or mobs that
-- players cannot normally fight. Tier derives from level; each eligible death
-- rolls and durably records one of the six essence types.
