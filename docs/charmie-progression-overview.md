# Charmie Progression: A Player Overview

Your permanent followers can grow stronger as they adventure with you. There are two ways they progress: purchased pets gain levels, while permanent class companions earn training points. Temporary summons and ordinary charmed enemies do not use these progression systems.

## Purchased pets gain levels

A purchased pet earns its own experience when it helps fight an enemy that rewards you with experience. It must be alive, in your room, and fighting that enemy. Its experience does not take anything away from your reward.

Pets level automatically when they have enough experience. Each new level costs **2,000 times the level being reached**. For example, reaching level 5 costs 10,000 pet XP.

A purchased pet can advance up to **your level or level 30, whichever is lower**. It cannot stockpile extra experience beyond its current advancement limit.

As it levels, it gains:

- **Every level:** 10 maximum health and 1 armor class.
- **Every even level:** +1 to attack and damage.
- **Every fourth level:** +1 to saving throws, helping it resist harmful effects.

Gaining maximum health does not heal existing wounds.

## Permanent companions earn training points

Animal companions, familiars, class mounts, dragon mounts, main eidolons, golems, and other eligible permanent companions use **training ranks**. A mercenary marked as a purchased pet uses purchased-pet leveling instead.

Training improves a companion without changing its creature level. Any level growth provided by your class still follows that class's normal rules.

Companions earn training experience from worthwhile enemies that reward you with experience. They must be alive and present in your room. Usually they must fight that enemy, but a mount you are riding or a familiar in the room can earn training experience while supporting you.

There are **10 training ranks**, each granting **one skill point**:

- Rank 1 costs 2,000 training XP.
- Rank 2 costs another 4,000 training XP.
- Each later rank costs 2,000 times that rank, up to 20,000 XP for rank 10.

That means a fully trained companion earns **10 points total**. Training stops at rank 10; extra training experience is not saved for later.

## Choose your companion's improvements

Spend points with `pets train <pet> <skill>`, using your companion's name or targeting keyword.

| Skill | Benefit for each point | Example command |
| --- | --- | --- |
| Health | +20 maximum health | `pets train wolf health` |
| Attack | +1 to attack rolls | `pets train wolf attack` |
| Damage | +1 damage | `pets train wolf damage` |
| Armor | +1 armor class | `pets train wolf armor` |
| Saves | +1 to all saving throws | `pets train wolf saves` |

Each improvement costs **one point** and can be bought **up to five times**. You choose how to divide your ten points. For example, five health purchases and five damage purchases give +100 maximum health and +5 damage.

Your companion must be in your room, and both of you must be out of combat to train. Health training increases maximum health without healing current damage.

## Check progress

- **`pets`** shows your followers, their progress, and available training points.
- **`group`** shows how much experience your own companions need for their next level or training rank. “TNL” means experience needed for the next advancement.

Killing players, charmed enemies, or purchased pets does not provide progression experience. Trivial rewards of one experience point do not count either.

## Death, revival, and keeping progress

Eligible permanent companions are stored when they die, keeping their progress, equipment, inventory, and gold. They can be revived rather than replaced.

| Command | What it does |
| --- | --- |
| `pets fallen` | Lists fallen companions and their revival prices. |
| `pets revive <ID>` | Revives that companion at full health with its belongings. |
| `pets discard <ID> CONFIRM` | Permanently gives up that companion and returns its belongings to you. |

Revival costs **500 carried gold per companion level**. You must be out of combat and have room for another follower. Discarding also requires being out of combat and enough inventory space for the returned items; the discarded companion's progress is lost.

Progress survives logout and revival. Usually, dismissing or replacing a companion means starting over with a new individual. **Your main eidolon is the exception:** its training belongs to your continuing bond and returns when you call it again. Temporary eidolon reflections do not share that training.

If a companion has died, use `pets fallen` and revive its saved body to recover that individual with its progress and gear.
