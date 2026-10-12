-- Install during deployment; retain the existing PETS command help.
INSERT INTO help_entries(tag, category, entry, min_level, max_level, auto_generated)
VALUES ('PETPROGRESSION', 'general', REPLACE('PET PROGRESSION, STORAGE AND TRAINING

COMMANDS
  pets                            List active pets and progression.
  pets fallen                     List stored killed pets and prices.
  pets revive <ID>                Revive a stored pet for gold.
  pets discard <ID> CONFIRM        Recover gear and permanently discard it.
  pets train <pet> <skill>         Spend companion training points.

DEATH AND RECOVERY
Permanent owned pets are stored when killed, rather than leaving a corpse.
This includes bought pets, animal companions, familiars, mounts, eidolons,
undead cohorts, golems and mercenaries. Temporary summons retain their
normal rules.

Stored pets retain their identity, level, XP, training, equipped gear,
inventory, container contents and held gold. They remain saved through logout and
are not automatically revived. Revival costs 500 carried gold per level:
a level 10 pet costs 5,000 gold. It returns at full health with its gear.
You must be outside combat and have space for another follower.

PETS DISCARD <ID> CONFIRM returns the pet''s equipment and inventory to your
inventory, returns its held gold, then permanently removes its stored state
and progression.
There is no fee. Make enough room to carry all of its gear first.
The pet can no longer be revived after being discarded.

BOUGHT PET LEVELS
Newly bought pets earn their own XP by fighting alongside you when you
receive kill XP. This never takes XP from you or your group. Reaching the
next level costs 2,000 times that level; levels advance automatically.
Growth stops at your level, capped at level 30, without banking excess XP.
Each gained level adds 10 maximum HP and 1 AC. Even levels add +1 attack
and damage; levels divisible by four improve saving throws by 1.
Level growth does not heal wounds. Old saves cannot reliably distinguish
bought pets from summons, so this applies to newly bought pets.

COMPANION TRAINING RANKS
Permanent class companions earn training XP instead of raising their
creature level through XP. Their usual class-based level rules still apply.
Combat companions must fight the rewarded enemy. Ridden mounts and nearby
familiars can also earn training XP by supporting their owner.
Killing players, charmed enemies or bought pets does not train pets.
One-XP rewards do not provide pet XP either. Temporary summons and short-lived eidolon reflections cannot train.

Training ranks are capped at 10. Each rank costs 2,000 times the next rank
in training XP and grants one skill point. Rank 1 costs 2,000 XP; rank 2
costs another 4,000 XP. Spend these points with PETS TRAIN; no gold is needed.
Each purchase costs one pet skill point, with at most five in each skill:

  health   +20 maximum hit points (does not heal current wounds)
  attack   +1 attack bonus
  damage   +1 damage bonus
  armor    +1 armor class
  saves    +1 to every saving throw

For example: PETS TRAIN CROCODILE HEALTH
Use the pet''s targeting keyword; 2.keyword selects a second matching pet.
Both you and your pet must be outside combat, and the pet must be here.
Ranks and skills belong to that individual companion, surviving logout,
death and revival. Discarding or replacing the pet loses its training.
', CHAR(10), CONCAT(CHAR(13), CHAR(10))), 0, 1000, FALSE)
ON DUPLICATE KEY UPDATE
  category = VALUES(category), entry = VALUES(entry),
  min_level = VALUES(min_level), max_level = VALUES(max_level), auto_generated = FALSE;
INSERT IGNORE INTO help_keywords(help_tag, keyword) VALUES
('PETPROGRESSION', 'petprogression'), ('PETPROGRESSION', 'pet progression'),
('PETPROGRESSION', 'pet xp'), ('PETPROGRESSION', 'bought pets'),
('PETPROGRESSION', 'purchased pets'), ('PETPROGRESSION', 'pet revival'),
('PETPROGRESSION', 'revive pet'), ('PETPROGRESSION', 'fallen pets'),
('PETPROGRESSION', 'pet training'), ('PETPROGRESSION', 'companion training'),
('PETPROGRESSION', 'pet skills'), ('PETPROGRESSION', 'discard pet');

-- Make the feature discoverable from the existing command help, without replacing it.
UPDATE help_entries
SET entry = CONCAT(entry, CHAR(13), CHAR(10), CHAR(13), CHAR(10),
  'For pet XP, companion training, stored gear, revival and discard, see help petprogression.',
  CHAR(13), CHAR(10))
WHERE tag = 'PETS' AND LOCATE('help petprogression', entry) = 0;
