# Pet progression, storage and training

## Death, revival and gear recovery

Permanent purchased pets, animal companions, familiars, class mounts, dragon mounts, eidolons/undead cohorts, golems, mercenaries, and other owned mountable creatures are stored when killed. Temporary summons and short-lived eidolon reflections retain their existing behavior.

Stored pets leave no corpse and generate no random loot. The pet's identity, level, XP, training investments, base combat stats, worn gear, inventory, container contents and held gold are saved before the live body is removed. Equipment records retain wear positions. Companion saves run in a database transaction; failed nested-item saves abort the transaction. If death storage fails, the pet is kept alive with its gear rather than being lost.

- `pets` lists active followers, purchased-pet XP and companion training progress.
- `pets fallen` lists stored pet IDs and revival prices.
- `pets revive <ID>` costs 500 carried gold per pet level and returns the same pet at full health with its equipment and inventory restored. Requires follower space and being outside combat.
- `pets discard <ID> CONFIRM` returns stored worn gear and inventory to the owner's inventory, and returns held gold and permanently deletes that pet, including XP and training. No gold fee. Requires enough inventory capacity and being outside combat.

Stored pets survive logout and ordinary saves; login only loads living pets. Owner-scoped record selection and one-time revival/discard prevent retrieving another player's pets or retrieving the same gear twice. Gear rows restore in saved ID order, including items saved during the same second. Failed restoration retains the stored state and refunds revival gold. Discard checks capacity and commits deletion before transferring items, then saves the owner's inventory. Dismissal/replacement creates a fresh companion and does not carry over an old pet's training.

## Purchased pet levels

Newly purchased pets have a purchase marker. Existing saved followers cannot reliably be identified as old purchases and remain unmarked. Bought pets earn independent XP while alive, present and fighting enemies whose deaths reward their owner. This takes no XP from players. Charmed enemies, purchased pets, player kills and one-point rewards do not provide pet XP.

Advancement costs 2,000 times the next level, with automatic advancement up to owner level or level 30. Growth adds 10 maximum HP and 1 AC per level, 1 attack/damage bonus at even levels, and 1 to saves at levels divisible by four. Growth does not heal existing damage. Purchased pets retain this system; class companions use training ranks instead of XP-based level increases.

## Companion training

Permanent companion types earn training XP from meaningful owner kill rewards before personal/happy-hour modifiers. Combat companions must be fighting that enemy; ridden mounts and present familiars can earn XP by supporting the owner. Temporary summons/reflections cannot earn training. Normal class-based companion level rules remain; training never changes their creature level.

Maximum training rank is 10. Each rank requires 2,000 times that next rank in XP and grants one pet skill point. Rank 1 costs 2,000 XP, rank 2 another 4,000, through rank 10. XP cannot be banked above the cap. Each skill can be purchased five times; owners choose how to spend ten points across the following skills:

| Command | Cost | Benefit per purchase |
| --- | --- | --- |
| `pets train <pet> health` | 1 pet skill point | +20 maximum HP |
| `pets train <pet> attack` | 1 pet skill point | +1 attack bonus |
| `pets train <pet> damage` | 1 pet skill point | +1 damage bonus |
| `pets train <pet> armor` | 1 pet skill point | +1 AC |
| `pets train <pet> saves` | 1 pet skill point | +1 to all saving throws |

Use the pet's targeting keyword, including `2.keyword` for duplicates. Training requires ownership, presence in the owner's room and neither owner nor pet being in combat. Health training preserves current HP. Training ranks, remaining XP, skill purchases and improved stats persist across logout, death and revival; they belong to that individual pet.

## Persistence and validation

Startup creates/upgrades the pet progression, killed-state, companion kind, training and saved saving-throw columns in `pet_data`. Existing rows receive compatible defaults, with nullable saved saving throws retaining the older derived values until next saved. Known companion flags are restored before class-specific loading. Temporary attack bonuses are excluded from saved base bonuses.

`tests/run_purchased_pet_progression.py` runs production advancement, training, schema, save/load, archival, revival and discard functions against disposable MariaDB. It exercises production recursive gear traversal and restoration with a small payload serializer, including equipped armor, a carried bag and nested contents. Checks cover migration, participation, rank/skill caps, unchanged creature levels/current HP, database and nested-item failures, ownership, costs, capacity, rollback, duplicate prevention and revival of trained companions. It never connects to the live database.
