# Dragonmetal Awakening — staging test checklist

Run this checklist on **staging**, never directly on production. Use a
level-30 test character, a disposable eligible weapon, and a sanitized staging
database. Stop and investigate any result that differs from the stated expected
outcome.

## 1. Staff setup

Apply migrations through schema v8, then configure the smith:

```sh
mysql --defaults-extra-file=/etc/krynn/staging-mysql.cnf krynn_stage < sql/20261023_legendary_objects.sql
mysql --defaults-extra-file=/etc/krynn/staging-mysql.cnf krynn_stage < sql/20261024_legendary_transactions.sql
mysql --defaults-extra-file=/etc/krynn/staging-mysql.cnf krynn_stage < sql/20261024_legendary_earnings.sql
mysql --defaults-extra-file=/etc/krynn/staging-mysql.cnf krynn_stage < sql/20261024_legendary_smiths.sql
mysql --defaults-extra-file=/etc/krynn/staging-mysql.cnf krynn_stage < sql/20261024_legendary_effects.sql
mysql --defaults-extra-file=/etc/krynn/staging-mysql.cnf krynn_stage < sql/20261025_legendary_item_passives.sql
mysql --defaults-extra-file=/etc/krynn/staging-mysql.cnf krynn_stage < sql/20261025_legendary_release_controls.sql
mysql --defaults-extra-file=/etc/krynn/staging-mysql.cnf krynn_stage < sql/20261026_legendary_mob_bosses.sql
mysql --defaults-extra-file=/etc/krynn/staging-mysql.cnf krynn_stage < sql/20261025_legendary_release_registry.sql
```

Replace `krynn_stage` with the real staging database name. Do not use a literal
placeholder such as `your_database_name`.

Confirm the initial state:

```sql
SELECT `version` FROM legendary_schema WHERE singleton=1;
SELECT feature_name, enabled FROM legendary_features ORDER BY feature_name;
SELECT mob_vnum, enabled FROM legendary_smiths WHERE mob_vnum=10500;
SELECT COUNT(*) AS obsolete_boss_registry_tables
FROM information_schema.tables
WHERE table_schema=DATABASE() AND table_name='legendary_boss_registry';
```

Expected: schema version `8`, all three features disabled, smith 10500 enabled,
and zero obsolete boss-registry tables.

Create or prepare a level-30 test character with an eligible permanent item of
level 20–30. A two-handed greatsword is recommended. Begin with no shield or
held item equipped. Use normal staff tooling to give the character adequate
gold, then seed only its staging legendary balance. Run `stat player <name>`
while the character is online and copy the positive numeric `IDNum` shown in
that output. Replace `PLAYER_ID` below with that number; it is not a literal
word, a character name, account ID, vnum, or zero:

```sql
INSERT INTO legendary_characters
(player_id, infusion_xp, fragments, catalysts,
 draconic, martial, arcane, spiritual, primal, shadow,
 allowance_date, fragments_today, essences_today, catalysts_today)
VALUES
(PLAYER_ID, 200000000, 200000, 400,
 40000, 40000, 40000, 40000, 40000, 40000,
 UTC_DATE(), 0, 0, 0)
ON DUPLICATE KEY UPDATE
 infusion_xp=VALUES(infusion_xp), fragments=VALUES(fragments),
 catalysts=VALUES(catalysts), draconic=VALUES(draconic),
 martial=VALUES(martial), arcane=VALUES(arcane),
 spiritual=VALUES(spiritual), primal=VALUES(primal), shadow=VALUES(shadow),
 allowance_date=UTC_DATE(), fragments_today=0, essences_today=0,
 catalysts_today=0, version=version+1;
```

### Flag a test boss

Use an approved existing boss prototype, such as Tombfyre (mobile 9356):

```text
medit 9356
A
```

In the NPC-flags menu, toggle the entry named `Legendary-Boss`; do not rely on
a fixed numeric position. Exit and save through the normal MEDIT workflow. Reset
the zone or restart/reload staging so a new spawned mobile receives the flag.

## 2. Disabled-gate checks

With all feature gates disabled, log in as the test character:

```text
infuse materials
infuse inspect greatsword
infuse next greatsword
infuse awaken greatsword primary Slayer
```

Expected:

- `materials`, `inspect`, `next`, `paths`, `choices`, and `loadout` are readable.
- `awaken` is refused because purchases are disabled.
- Gold, infusion XP, and materials do not change.

Kill an ordinary eligible NPC and the flagged boss while earnings are disabled.
Normal character XP may behave normally, but `infuse materials` must not gain
infusion XP, fragments, essences, or catalysts.

## 3. Earnings test

Enable earnings only, then wait at least five seconds for the feature cache:

```sql
UPDATE legendary_features SET enabled=TRUE WHERE feature_name='earnings';
```

1. Kill an ordinary eligible NPC at level 30, then run:

   ```text
   infuse materials
   ```

   Expected: infusion XP increases, but no boss materials are granted.

2. Kill the flagged boss. Remain alive, connected, and in the boss room when it
   dies. If grouped, be in the credited killer's same-room group. Perform at
   least three useful actions in the final 30 seconds. Every real qualifying
   combat packet counts; no artificial delay is required.

   Expected for a level-30 flagged boss: 30 fragments, 6 essences of one
   randomly selected type, and 1 catalyst if the daily allowance permits it.
   Do not expect a particular essence type.

3. Kill another copy of the same boss vnum immediately.

   Expected: no new material reward because of the 20-hour per-character,
   per-prototype cooldown.

4. Kill a different flagged boss prototype.

   Expected: it can reward materials immediately, subject to daily caps.

5. Kill a high-level mob that does not have `Legendary-Boss`.

   Expected: no boss-material award.

6. Test partial cap behavior:

   ```sql
   UPDATE legendary_characters
   SET allowance_date=UTC_DATE(), fragments_today=95, essences_today=19,
       catalysts_today=1, version=version+1
   WHERE player_id=PLAYER_ID;
   ```

   Kill a flagged boss. Expected: at most 5 fragments and 1 essence are granted;
   no catalyst is granted. No repeat kill can exceed the cap.

## 4. Purchase, binding, foundation, and passive test

Enable purchases, then wait five seconds:

```sql
UPDATE legendary_features SET enabled=TRUE WHERE feature_name='purchases';
```

Visit Lord Orelor Silverveins, mobile 10500, in room 8795. Be level 30,
conscious, out of combat, and carry the test weapon:

```text
infuse inspect greatsword
infuse paths greatsword
infuse awaken greatsword primary Slayer
infuse confirm <quote-id-shown>
infuse choices greatsword primary
infuse select greatsword primary foundation F01
infuse select greatsword primary passive <Pxx-listed-by-choices>
infuse inspect greatsword
```

Expected:

- The quote displays exact costs; nothing is spent before confirmation.
- The weapon binds permanently after confirmation.
- Foundation and item-passive selections appear in `infuse inspect`.
- When equipped, `affects`, `score`, and `infuse loadout` show the active
  contribution, subject to normal stacking caps.

Attempt prohibited transfers and destruction:

```text
give greatsword <other-player>
drop greatsword
sell greatsword
auction greatsword
reforge greatsword
```

Expected: all prohibited routes fail. Store the weapon in permitted personal
storage, retrieve it, and run `infuse inspect greatsword`; its identity and
choices must persist.

## 5. Rank and power progression test

At the smith, complete the primary Slayer path:

```text
infuse advance greatsword primary
infuse confirm <quote-id>
infuse select greatsword primary technique T01
infuse attune greatsword primary technique

infuse advance greatsword primary
infuse confirm <quote-id>
infuse select greatsword primary signature S09
infuse attune greatsword primary signature

infuse advance greatsword primary
infuse confirm <quote-id>
infuse modify greatsword primary technique Ready

infuse advance greatsword primary
infuse confirm <quote-id>
infuse inspect greatsword
infuse loadout
```

Expected:

- Rank II unlocks T01, Measured Strike.
- Rank III unlocks S09, Hunter's Opening.
- Rank IV accepts `Ready` for T01.
- Rank V improves displayed technique/signature values.
- Choices and running cooldowns persist across advancement.

Test essence exchange:

```text
infuse exchange essence draconic martial 1
infuse confirm <quote-id>
infuse materials
```

Expected: 3 draconic essences are spent, 1 martial essence is gained, and the
output uses one point of today's essence allowance.

## 6. Two-handed supporting-channel test

Actually wield the greatsword in both hands with no shield or held item:

```text
infuse awaken greatsword supporting assault Slayer
infuse confirm <quote-id>
infuse advance greatsword supporting
infuse confirm <quote-id>
infuse loadout
```

Expected: supporting progression has its own rank and costs.

Equip a shield or held item, then inspect the loadout:

```text
wear shield
infuse loadout
```

Expected: the supporting channel is dormant. Remove the shield, wield the
greatsword with both hands again, and run `infuse loadout`. The supporting
channel must reactivate without losing its purchased rank.

## 7. Major-effect test

Enable major effects and wait five seconds:

```sql
UPDATE legendary_features SET enabled=TRUE WHERE feature_name='major_effects';
```

Equip the weapon, engage a valid NPC target, and test:

```text
infuse use T01 <target>
```

Make one ordinary weapon attack.

Expected: the next attack receives T01's benefit and consumes the preparation
on hit or miss. An immediate second use is refused by cooldown.

After cooldowns expire:

```text
infuse use S09 <target>
```

Make one ordinary attack against the designated target. Expected: Hunter's
Opening improves that attack and then consumes its preparation.

Test invalid targets:

```text
infuse use T01 <friendly-group-member>
infuse use T01 <nonexistent-target>
```

Expected: invalid uses consume neither cooldown nor budget.

Disable the gate again:

```sql
UPDATE legendary_features SET enabled=FALSE WHERE feature_name='major_effects';
```

Then run `infuse use T01 <target>`. Expected: major powers are disabled, with
no activation or cooldown.

## 8. Dragonlance negative test

No true dragonlance is configured in the initial release. An ordinary lance must
not receive Dragonbane powers that require a registered true dragonlance, such
as S11 Grounding Lance. This refusal is expected.

## 9. Recovery/restart test

With purchases enabled:

1. Begin an awakening, advancement, reshape, passive replacement, or exchange.
2. Confirm the quote.
3. Use the approved staging restart/copyover test tooling immediately after the
   confirmation.
4. Log back in and run:

   ```text
   infuse materials
   infuse inspect <item>
   infuse loadout
   ```

Expected: the operation is completed exactly once or is safely recoverable. It
must never duplicate costs, materials, ranks, items, or powers.

## 10. End-of-test checks

Disable all features:

```sql
UPDATE legendary_features SET enabled=FALSE;
```

Generate the aggregate observation report:

```sh
LEGENDARY_PHASE17_READONLY=YES \
LEGENDARY_PHASE17_DB=krynn_stage \
LEGENDARY_PHASE17_OUTPUT=/var/log/krynn/legendary-observation \
scripts/legendary_phase17_report.sh \
  --defaults-file /etc/krynn/legendary-report.cnf
```

Review the report and server logs for:

- Pending awards or unreconciled operations.
- Correct boss reward/cooldown behavior.
- Correct active loadouts and purchase history.
- UUID quarantine, ownership, save, or recovery errors.

Do not promote the system to production until this staging checklist, coherent
backup/restore rehearsal, and the Phase-14B combat-balance acceptance gate have
all passed.
