# Pet progression, storage and training

## Death, revival and gear recovery

Permanent purchased pets, animal companions, familiars, class mounts, dragon mounts, eidolons/undead cohorts, golems, mercenaries, and other owned mountable creatures are stored when killed. Temporary summons and short-lived eidolon reflections retain their existing behavior.

Stored pets leave no corpse and generate no random loot. The pet's identity, level, XP, training investments, base combat stats, worn gear, inventory, container contents and held gold are saved before the live body is removed. Equipment records retain wear positions. Companion saves run in a database transaction; failed nested-item saves abort the transaction. If death storage fails, the pet is kept alive with its gear rather than being lost.

- `pets` lists active followers, purchased-pet XP and companion training progress.
- `pets fallen` lists stored pet IDs and revival prices.
- `pets revive <ID>` costs 500 carried gold per pet level and returns the same pet at full health with its equipment and inventory restored. Requires follower space and being outside combat.
- `pets discard <ID> CONFIRM` returns stored worn gear and inventory to the owner's inventory, and returns held gold and permanently deletes that pet, including XP and training. No gold fee. Requires enough inventory capacity and being outside combat.

Stored pets survive logout and ordinary saves; login only loads living pets. Owner-scoped record selection and one-time revival/discard prevent retrieving another player's pets or retrieving the same gear twice. Gear rows restore in saved ID order, including items saved during the same second. Failed restoration retains the stored state and refunds revival gold. Discard checks capacity and commits deletion before transferring items, then saves the owner's inventory. Dismissal/replacement normally creates a fresh companion without the old pet's training. The main eidolon is an exception: its training belongs to the owner's continuing eidolon bond and is restored when called again. Temporary reflections never share that training.

## Existing-server upgrade and purchases

Apply `sql/20261027_pet_persistence.sql` to older pet tables before deploying
these companion features. It adds missing progression/death/training columns
without removing existing pets or equipment. Missing `pet_dead` causes both
saving and `pets fallen` to fail; a server reboot alone does not run this
explicit migration.

Successful pet-shop purchases, including the Silver Sword mercenaries in room
15306, save their owner and companion immediately. A failed companion save
cancels the purchase and refunds its gold. Companion purchase objects are kept
if conversion cannot be saved. Purchased companions return automatically on
login; `summon` is not a purchase-recovery command. The shop continues listing
its usual stock and prices after a sale.

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

## Eidolon equipment on dismissal and logout

Dismissing your main eidolon automatically returns its worn equipment, carried
items (including container contents), and gold to you. Logging out or timing out
while linkdead returns those belongings before removing the eidolon as well.
You do not need to order it to unequip and hand back each item individually.

Returns use the estate delivery journal. Removing the eidolon's saved gear and
recording the owner deliveries happen in one database transaction. If that
fails, the eidolon keeps its belongings and dismissal/logout stops. If delivery
is interrupted after the transaction commits, the saved returns recover on
login. Returned gear is not also restored on the eidolon. Equipment on a fallen
eidolon remains in its existing death archive for `pets revive` or `pets discard`.

## Eidolon re-summoning

The main eidolon's rank, partial training XP, and purchased skills survive
`dismiss eidolon`, logout, death, and reboot. Training is saved with the owner
in an `EiTr` player-file record as well as on an existing saved body. Calling a
new main eidolon restores those ranks and skills, applying their bonuses once.
Re-summoning does not grant additional points or train temporary reflections.
Existing living or revived eidolons seed the owner-side record automatically.
Returning to an older saved body uses the current bond's training.

For a fallen companion whose original body is still saved, use `pets fallen`
and `pets revive <ID>` to retrieve that body with its saved training and gear.
Training already overwritten before this fix needs a surviving record or backup
to recover; this change does not invent previously lost XP.

## Persistence and validation

Startup creates/upgrades the pet progression, killed-state, companion kind, training and saved saving-throw columns in `pet_data`. Existing rows receive compatible defaults, with nullable saved saving throws retaining the older derived values until next saved. Known companion flags are restored before class-specific loading. Temporary attack bonuses are excluded from saved base bonuses.

`tests/run_purchased_pet_progression.py` runs production advancement, training, schema, save/load, archival, revival and discard functions against disposable MariaDB. It exercises production recursive gear traversal and restoration with a small payload serializer, including equipped armor, a carried bag and nested contents. Checks cover migration, participation, rank/skill caps, unchanged creature levels/current HP, database and nested-item failures, ownership, costs, capacity, rollback, duplicate prevention and revival of trained companions. It never connects to the live database.
