-- Required by all companion save/load paths, including purchases and revival.
-- Safe to reapply; existing live and fallen pet records and gear are preserved.
ALTER TABLE pet_data
 ADD COLUMN IF NOT EXISTS epic_doctrine INT NOT NULL DEFAULT 0,
 ADD COLUMN IF NOT EXISTS epic_expiry BIGINT NOT NULL DEFAULT 0,
 ADD COLUMN IF NOT EXISTS epic_identity VARCHAR(32) NOT NULL DEFAULT '';
