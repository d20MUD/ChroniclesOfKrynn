# Crafting resources in random treasure

Successful random-treasure rolls can award one crafting material (20% chance)
and one crafting mote (10% chance). These are independent additions to existing
magic-item and coin rewards. Resources are added directly to the existing
crafting balances and saved immediately. All eight mote types are eligible.

Material quality depends exclusively on the defeated mob's level:

| Material grade | Minimum mob level | Selection share at level 30+ |
| --- | ---: | ---: |
| 1 | 1 | 18% |
| 2 | 5 | 20% |
| 3 | 10 | 30% |
| 4 | 20 | 20% |
| 5 | 30 | 10% |
| 6 (dragonmetal) | 30 | 2% |

When a higher grade is unavailable, its roll falls to an eligible lower grade.
Materials are selected uniformly within the chosen grade, using the existing
crafting material-grade definitions. Ungraded materials are excluded.

At a base 5% treasure chance, materials drop on approximately 1% of kills and
motes on 0.5%. For level 30+ mobs, any grade-5 material drops on approximately
0.1% of kills and dragonmetal on 0.02%. Happyhour and account treasure bonuses
increase the initial treasure chance, but never bypass the mob-level gates.

`materials` shows the material and mote balances. Each drop awards one unit
or one mote. Crafting/harvesting bonuses do not multiply these treasure awards.
