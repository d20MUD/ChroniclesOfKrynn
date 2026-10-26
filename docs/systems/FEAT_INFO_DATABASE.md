# Feat information database export

The MUD creates and refreshes `feat_info` and `feat_info_prerequisites` during
`boot_world()`, immediately after `assign_feats()`. No manual migration is required.
`sql/20261025_feat_info.sql` can optionally create the tables before boot.

`feat_info` mirrors every scalar field in `struct feat_info`, keyed by the game's
numeric `feat_id`: `name`, `in_game`, `can_learn`, `can_stack`, `feat_type`,
`short_description`, `description`, `epic`, `combat_feat`, `event`, and
`teamwork_feat`. All `NUM_FEATS` slots are exported, including reserved ID 0,
inactive feats, and unused slots. Text retains the original MUD color codes and
line endings; null strings remain SQL NULL.

`feat_info_prerequisites` represents `prerequisite_list`, keyed by `feat_id` and
zero-based `prerequisite_order` in linked-list traversal order. Each row stores
`prerequisite_type`, `description`, and the three raw values as `value_0`,
`value_1`, and `value_2`. A feat with no prerequisites has no child rows.

```sql
SELECT * FROM feat_info WHERE in_game = 1 ORDER BY name;
SELECT * FROM feat_info WHERE name LIKE '%Aurak%';
SELECT * FROM feat_info_prerequisites
WHERE feat_id = 1 ORDER BY prerequisite_order;
```

Both tables are generated snapshots: manual changes are replaced at the next
boot, and obsolete rows are removed. Refresh uses one InnoDB transaction with
bound parameters. A failed refresh rolls back both tables and logs an error;
export failure or unavailable MySQL does not abort the MUD boot. Export runs on
an acquired pool connection and releases it after completion.

Run `python3 tests/run_feat_info_export.py` to test the production exporter and
all configured feat definitions against a temporary MariaDB server. The test
never connects to the game's database.
