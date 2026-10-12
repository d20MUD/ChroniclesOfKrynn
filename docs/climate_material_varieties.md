# Climate material varieties

Automatically generated crafting nodes use the zone's Climate and Terrain
metadata. Room sectors still select which resource groups can appear. Zone
levels and the existing random grade roll still determine node quality.

## Timber

| Zone climate | Grade 1 | Grade 2 | Grade 3 |
|---|---|---|---|
| Unspecified | Ash | Maple | Mahogany |
| Polar | Driftwood, dwarf willow | Birch driftwood | Larch driftwood |
| Subarctic | Spruce, pine | Birch | Larch |
| Temperate | Ash, beech, willow | Maple | Oak |
| Subtropical | Pine, willow | Cypress, cedar | Live oak |
| Tropical | Bamboo | Teak | Mahogany, ebony |
| Arid | Tamarisk | Acacia | Mesquite |
| Alpine | Fir, pine | Birch, cedar | Larch |

Multiple choices in a cell have equal chances. A temperate swamp chooses willow
for grade 1. Tropical and subtropical swamps choose cypress for grade 2.
Underground, ocean and urban zone terrains exclude automatic timber nodes.

Grade 4 remains valenwood and grade 5 remains ironwood in every eligible climate.
There is no new grade-6 timber or automatic endgame resource placement.

Each variety has its own saved crafting balance and object material ID. These
IDs were appended: existing materials, pfiles and objects retain their IDs.
Bundles and crafted items preserve their variety. New woods inherit the
crafting grade, level adjustment, resource group and harvesting skill of ash
(grade 1), maple (grade 2) or mahogany (grade 3).

## Other resources

Grade-1 hemp nodes become jute in tropical climates and agave fiber in arid
climates. Both are equivalent to hemp for crafting. Other existing fibers
retain their materials and grades. Hunting nodes show climate-specific game
tracks while continuing to yield the existing hide grades. Mining descriptions
reflect cold, tropical, arid and alpine environments; the ore and stone yields
remain the same. This change does not introduce a separate herb crafting system.

Random treasure retains the original material selection probabilities rather
than counting every regional variety as an additional timber/fiber outcome.
New varieties are available through harvesting, the artisan shop and exchange.

## Materials exchanger

Assign **Materials Exchanger** through `Z) Spec Procedure` in `redit`, `medit`
or `oedit`. It supports room exchange services, NPC exchangers and exchange
objects on the ground, carried or equipped. Objects must be visible to the
player. It is saved under that name and available through
`find_spec_func_by_name("Materials Exchanger")` for coded assignments. No NPC
vnum or live world placement is hardcoded by this change.

Players in the NPC's room use:

```
exchange list
exchange 10 maple for birch
exchange 5 "live oak wood" for "mahogany wood"
exchange 8 hemp for jute
```

The exchange uses the player's on-hand crafting balances, not physical bundles
or materials already reserved in projects. It transfers equal quantities for
no gold charge and saves the player immediately. Full names and unique prefixes
are accepted case-insensitively. Quotes are optional for multi-word names.

Only equivalent timber varieties in grades 1–3 and grade-1 fibers qualify.
Different families, different grades, identical source/target materials,
insufficient stock, invalid quantities and destination overflow are rejected.
Materials with different properties are not interchangeable solely because
they share a grade: mithril cannot be exchanged for adamantite, for example.
The player must be out of combat. NPC exchangers must also be out of combat,
awake and visible to the player.

## Deployment and verification

Configure zone Climate and Terrain with `zedit` options 5 and 6. Existing zones
default to Unspecified. Existing nodes acquire the new varieties at the next
normal node regeneration; displaying a node never changes its yield.

The optional `sql/20261012_climate_materials_help.sql` migration installs player
help and aliases. It is prepared for deployment, not applied to the live database.

`tests/run_climate_materials.py` exercises production selection, equivalence,
object conversion, new and legacy pfile balances, node descriptions, exchanger
accounting and special-procedure dispatch.
