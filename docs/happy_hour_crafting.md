# Happy-hour crafting and gathering

The existing happy-hour rates now apply consistently:

* `happyhour craftingexp <percent>` applies once to crafting skill XP.
* `happyhour harvestingexp <percent>` applies once to mining, hunting,
  forestry, gathering, surveying and butchering skill XP.
* `happyhour harvestingmaterials <percent>` increases successful refining,
  harvesting and butchering material output, including critical and material
  talent bonuses. Bonus harvesting units do not consume additional node stock.
* `happyhour harvestingmotechance <percent>` retains its existing increase
  to the mote discovery chance.
* `happyhour harvestingmotesobtained <percent>` increases discovered mote
  quantities, including mote synergy bonuses.

Material and mote quantities are whole numbers. A positive yield bonus on
a positive output grants at least one extra unit, matching the existing
harvesting policy. Larger quantities use the percentage rounded down.
Zero output and failed production attempts do not generate bonus resources.

XP bonuses use the base skill XP, add to specialization and Insightful bonuses,
and round down. Refining XP uses its recipe's skill: a mining recipe receives
the harvesting XP rate; tailoring and alchemy recipes receive the crafting XP
rate. Yield bonuses do not increase a project's base XP a second time.

Single crafted equipment items remain single items; happy hour improves their
crafting XP. Refining output and gathered resources receive quantity bonuses.
Exchanges, storage, refunds, shop purchases and recovered golem materials are
not production rewards and do not receive happy-hour yield bonuses.

All bonus rates clear when happy hour expires or staff set `happyhour time 0`.
Projects use the bonuses active when the XP or output is awarded.

Verification: `tests/run_happy_crafting.py`, `tests/run_insightful_harvesting.py`
and `tests/run_batch_smelting.py` cover production XP/yields, criticals, talents,
disabled rates, node depletion and happy-hour bonus cleanup.
