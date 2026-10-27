-- Schema v9: smith eligibility is the mobile prototype's Infusion Smith
-- special procedure. Assign it through MEDIT and save the mobile normally.
-- Remove any registry left by older installations; player progress is unchanged.
DROP TABLE IF EXISTS legendary_smiths;
UPDATE legendary_schema SET version=9 WHERE singleton=1 AND version=8;
