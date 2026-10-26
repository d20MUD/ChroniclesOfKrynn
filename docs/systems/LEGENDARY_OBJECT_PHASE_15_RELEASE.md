# Phase 15 — release configuration and runbook

This package is reviewable release preparation only. It does not change a live
world file, apply SQL to production, start the MUD, or enable any legendary
feature.

## Reviewed existing content

The selected smith is **Lord Orelor Silverveins** (mobile **10500**), an Elder
Forger of Reorx reset by zone 87 into room **8795**. This uses an existing forge
NPC and requires no room, mobile, object, or zone creation.

Boss eligibility is a native **Legendary-Boss** mobile flag, not a MySQL
registry. Every approved candidate below is an existing high-level mobile with
an explicit reset room. Set the flag in MEDIT and save the existing mobile file
as a separate, reviewed world-content change.

| Candidate mobile | Normal reset room |
| --- | --- |
| 9356 Tombfyre | 9462 |
| 5316 Blacksnake | 5382 |
| 34503 Gadrok | 34516 |
| 15607 storm lord | 15619 |
| 4714 necrotheurge | 5121 |
| 3231 archmage shadow | 3280 |
| 5918 Irdan paladin | 5940 |
| 5909 Irdan priestess | 5954/5955 |
| 630 Azurathira | 676 |
| 13530 Archdruid Adadyr | 13595 |
| 4712 dracolich | 4730 |
| 5402 Thorian | 5435 |

Flagged bosses are hard-control immune. This preserves their encounter identity;
Grounding Lance therefore uses its documented limited-damage fallback where it
applies. Tier derives from level (20-24 / 25-29 / 30+) and essence is the stable
uniform roll among Draconic, Martial, Arcane, Spiritual, Primal and Shadow. The
chosen type is durably attached to the award before delivery, so retrying an
award cannot reroll it. The standard per-character 20-hour boss cooldown,
source participation rules, daily caps (100 fragments, 20 essences, one
catalyst), and exchange rules remain the controls on acquisition.

`tests/verify_legendary_release_content.py` reads the live source tree and
verifies each mobile's identity and normal reset room. It never writes world
data. The ready-to-review SQL is
[`20261025_legendary_release_registry.sql`](../../sql/20261025_legendary_release_registry.sql).

### True dragonlances

No current object prototype can be positively identified as a true dragonlance:
the world object text contains ordinary lances but no authoritative true-lance
record. Phase 15 does **not** guess one. The registry intentionally has no
`legendary_dragonlances` entries, so Dragonbane powers that require a true lance
remain unavailable and ordinary lances do not receive them. Add actual reviewed
true-lance content and a separate explicit registry row before enabling those
powers.

## Schema and feature controls

Apply migrations in order through:

1. `20261023_legendary_objects.sql`
2. `20261024_legendary_transactions.sql`
3. `20261024_legendary_earnings.sql`
4. `20261024_legendary_smiths.sql`
5. `20261024_legendary_effects.sql`
6. `20261025_legendary_item_passives.sql`
7. `20261025_legendary_release_controls.sql`
8. `20261026_legendary_mob_bosses.sql`

Schema v7 adds `legendary_features`. It creates three independent, fail-closed
controls, all initially disabled:

| Feature | Controls | Disabled behavior |
| --- | --- | --- |
| `earnings` | XP/material awards from normal XP, bosses, traps and quests | No award is queued or credited. |
| `purchases` | Awakening, advance, reshaping, passive replacement and exchanges | Read-only inspection remains available; no quote can be purchased or confirmed. |
| `major_effects` | `infuse use` techniques/signatures | No activation/cooldown/effect occurs. |

The loader requires exactly these three valid rows. A missing, malformed or
unreadable feature table fails closed. The values refresh at most every five
seconds, so changes take effect without reinterpreting existing investment.
Disabling a gate never removes item identities, balances, cooldowns, selected
powers, or audit rows.

## Safe release sequence

1. Stop normal economic mutations and make one coherent backup of player files,
   object stores, the SQL database, and `lib/etc/legendary-budgets/` together.
2. Deploy the compatible binary, apply schema v8, then run the content verifier.
3. Apply `20261025_legendary_release_registry.sql` and complete the reviewed
   MEDIT flag changes. Confirm all three feature rows are `0`; inspect schema,
   smith and flagged mobile prototypes.
4. Start with all gates disabled. Test old player/object loading, legacy ordinary
   equipment, `infuse inspect`, and staff-visible diagnostics only.
5. Enable `earnings` for a controlled observation window. Confirm exactly-once
   source receipts, participation checks, material caps and journal persistence.
6. Enable `purchases` after the earnings review. Confirm an awakening, one
   advance, a two-handed supporting channel, a passive replacement and an
   exchange, including restart/recovery behavior.
7. Enable `major_effects` only after the completed Phase-14B representative
   combat gate. Confirm source validity, shared cooldowns, rolling budgets and
   removal/disconnect cleanup.

Use these explicit operations, one at a time, only after the preceding check:

```sql
UPDATE legendary_features SET enabled=TRUE WHERE feature_name='earnings';
UPDATE legendary_features SET enabled=TRUE WHERE feature_name='purchases';
UPDATE legendary_features SET enabled=TRUE WHERE feature_name='major_effects';
```

To halt safely, set the relevant row to `FALSE`; do not delete ledger rows or
downgrade schema. Rollback requires a binary that still reads legendary UUID
tags and a coherent backup of both flat-file and SQL sides of each operation.

## Preflight and reconciliation

Run before release and record output:

```sh
python3 tests/verify_legendary_release_content.py
python3 tests/run_legendary_security.py
python3 tests/run_legendary_flow.py
cmake --build build -j2
```

At startup and after each feature change, inspect:

```sql
SELECT version FROM legendary_schema WHERE singleton=1;
SELECT feature_name,enabled FROM legendary_features ORDER BY feature_name;
SELECT mob_vnum,enabled FROM legendary_smiths WHERE mob_vnum=10500;
SELECT state,COUNT(*) FROM legendary_operations GROUP BY state;
SELECT COUNT(*) AS pending_awards FROM legendary_awards WHERE state='pending';
```

Expected production configuration is schema version 8, three feature rows,
smith 10500 enabled, and the approved `MOB_LEGENDARY_BOSS` mobile flags. The
database deliberately contains no boss registry.
Before major effects are enabled, Phase 14B must supply its completed seven-role
native encounter evidence; the current preliminary probes are not sufficient.

## Player-facing text synchronization

The in-game `INFUSE` help and the Markdown player guide use the exact costs,
UTC daily reset, permanent-binding warning, two-handed independent-rank rule,
and additional item-passive rule. Before enabling purchases, change the guide's
deployment note from the old schema-v6/unselected-location wording to schema v8
and Lord Orelor, then publish it with the binary. Do not promise true-lance
powers until their object records are added.
