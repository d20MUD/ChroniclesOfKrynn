-- Phase 15 / schema v7.  Apply after v1-v6 while the game is offline.
-- All gameplay paths start disabled so migration alone cannot grant rewards,
-- debit wallets, or enable major activations.
CREATE TABLE IF NOT EXISTS legendary_features (
 feature_name VARCHAR(32) PRIMARY KEY,
 enabled BOOLEAN NOT NULL DEFAULT FALSE,
 CHECK (feature_name IN ('earnings','purchases','major_effects'))
) ENGINE=InnoDB;
INSERT IGNORE INTO legendary_features(feature_name,enabled) VALUES
 ('earnings',FALSE),('purchases',FALSE),('major_effects',FALSE);
UPDATE legendary_schema SET version=7 WHERE singleton=1 AND version=6;
