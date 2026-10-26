-- Phase 17: aggregate, read-only observation report.
-- Run with a database account that has SELECT only.  No player ID, character
-- name, item UUID, target, or raw recovery payload is selected by this report.
SET time_zone = '+00:00';

SELECT 'release-health' AS section;
SELECT version AS schema_version
  FROM legendary_schema WHERE singleton=1;
SELECT feature_name, enabled
  FROM legendary_features ORDER BY feature_name;
SELECT COUNT(*) AS obsolete_boss_registry_tables
  FROM information_schema.tables
 WHERE table_schema=DATABASE() AND table_name='legendary_boss_registry';

SELECT 'daily-awards-last-7-utc-days' AS section;
SELECT DATE(FROM_UNIXTIME(occurred_utc)) AS utc_day,
       CASE source_kind WHEN 1 THEN 'kill-xp' WHEN 2 THEN 'quest-xp'
                        WHEN 3 THEN 'trap-xp' WHEN 4 THEN 'boss' ELSE 'unknown' END AS source,
       COUNT(*) AS awards,
       SUM(received_xp) AS received_xp,
       SUM(received_fragments) AS fragments,
       SUM(received_essences) AS essences,
       SUM(received_catalysts) AS catalysts
  FROM legendary_awards
 WHERE occurred_utc >= UNIX_TIMESTAMP(UTC_DATE() - INTERVAL 7 DAY)
 GROUP BY utc_day, source
 ORDER BY utc_day, source;

SELECT 'award-and-recovery-health' AS section;
SELECT state, COUNT(*) AS awards,
       COALESCE(MAX(UNIX_TIMESTAMP(UTC_TIMESTAMP())-occurred_utc),0) AS oldest_age_seconds
  FROM legendary_awards GROUP BY state ORDER BY state;
SELECT state, COUNT(*) AS operations,
       COALESCE(MAX(TIMESTAMPDIFF(SECOND,created_at,UTC_TIMESTAMP())),0) AS oldest_age_seconds
  FROM legendary_operations GROUP BY state ORDER BY state;

SELECT 'daily-cap-saturation-current-utc-day' AS section;
SELECT COUNT(*) AS tracked_characters,
       COALESCE(SUM(fragments_today>=100),0) AS fragment_cap_reached,
       COALESCE(SUM(essences_today>=20),0) AS essence_cap_reached,
       COALESCE(SUM(catalysts_today>=1),0) AS catalyst_cap_reached
  FROM legendary_characters
 WHERE allowance_date=UTC_DATE();

SELECT 'current-material-holdings-and-shortages' AS section;
SELECT essence, SUM(amount) AS total_held,
       SUM(amount=0) AS characters_without_any
  FROM (
    SELECT draconic AS amount,'draconic' AS essence FROM legendary_characters
    UNION ALL SELECT martial,'martial' FROM legendary_characters
    UNION ALL SELECT arcane,'arcane' FROM legendary_characters
    UNION ALL SELECT spiritual,'spiritual' FROM legendary_characters
    UNION ALL SELECT primal,'primal' FROM legendary_characters
    UNION ALL SELECT shadow,'shadow' FROM legendary_characters
  ) AS wallets
 GROUP BY essence ORDER BY essence;

SELECT 'purchases-and-recovery-last-7-utc-days' AS section;
SELECT DATE(created_at) AS utc_day, kind, state, COUNT(*) AS operations,
       SUM(xp_cost) AS xp_spent, SUM(gold_cost) AS gold_spent,
       SUM(fragment_cost) AS fragments_spent, SUM(essence_cost) AS essences_spent,
       SUM(catalyst_cost) AS catalysts_spent
  FROM legendary_operations
 WHERE created_at >= UTC_TIMESTAMP() - INTERVAL 7 DAY
 GROUP BY utc_day, kind, state
 ORDER BY utc_day, kind, state;

SELECT 'active-loadouts' AS section;
SELECT CASE i.slot_id
         WHEN 0 THEN 'weapon' WHEN 1 THEN 'arms' WHEN 2 THEN 'legs'
         WHEN 3 THEN 'head' WHEN 4 THEN 'body' WHEN 5 THEN 'about'
         WHEN 6 THEN 'shield' WHEN 7 THEN 'held' WHEN 8 THEN 'feet'
         WHEN 9 THEN 'hands' ELSE 'unknown' END AS slot,
       c.channel_id AS channel, c.rank_id AS rank, c.path_id AS path,
       c.family_id AS family, COUNT(*) AS channels
  FROM legendary_items AS i
  JOIN legendary_channels AS c ON c.uuid=i.uuid
 WHERE i.state='active'
 GROUP BY slot,c.channel_id,c.rank_id,c.path_id,c.family_id
 ORDER BY slot,c.channel_id,c.rank_id,c.path_id,c.family_id;

SELECT 'two-handed-investment-proxy' AS section;
SELECT 'weapons-with-supporting-channel' AS configuration,
       COUNT(DISTINCT p.uuid) AS active_items
  FROM legendary_items AS i
  JOIN legendary_channels AS p ON p.uuid=i.uuid AND p.channel_id=0
  JOIN legendary_channels AS s ON s.uuid=i.uuid AND s.channel_id=1
 WHERE i.state='active' AND i.slot_id=0
UNION ALL
SELECT 'owners-with-active-weapon-and-shield', COUNT(DISTINCT w.owner_id)
  FROM legendary_items AS w
  JOIN legendary_items AS s ON s.owner_id=w.owner_id
 WHERE w.state='active' AND s.state='active' AND w.slot_id=0 AND s.slot_id=6;

SELECT 'major-power-usage-last-24-hours' AS section;
SELECT power_id, COUNT(*) AS activations,
       COUNT(DISTINCT player_id) AS users
  FROM legendary_activations
 WHERE started_utc >= UNIX_TIMESTAMP(UTC_TIMESTAMP() - INTERVAL 1 DAY)
 GROUP BY power_id ORDER BY activations DESC,power_id;

SELECT 'rolling-effective-benefit-proxy' AS section;
SELECT budget_kind, COUNT(*) AS debits, SUM(amount) AS actual_amount,
       COUNT(DISTINCT player_id) AS affected_characters
  FROM legendary_budgets
 GROUP BY budget_kind ORDER BY budget_kind;
