# Climate material varieties

Automatically generated land crafting nodes use the zone's Climate and Terrain
metadata. Aquatic rooms use marine equivalents regardless of zone metadata. Room sectors still select which resource groups can appear. Zone
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
Underground, ocean and urban zone terrains exclude automatic land timber nodes.
Aquatic sectors in those zones can still generate coral.

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

## Marine materials

Water-swim, water-noswim, underwater, ocean, river, underdark-water and
underdark-noswim sectors qualify in **every** zone terrain, including unspecified
and mixed. Dry rooms in ocean zones retain their land behavior. Survey and normal
world node regeneration use the same sector eligibility rule.

Aquatic nodes use 30% coral, 30% hides, 30% fibers, 5% hard metals and 5% soft
metals. Existing zone-level and random node-tier rolls choose the ordinary
material first; wood, hides and cloth then map to marine equivalents without
land climate substitutions. Mineral identities and selection remain unchanged.

| Node tier | Coral → land wood | Marine hide → land hide | Marine fiber → land fiber |
|---|---|---|---|
| 1 | Branch Coral → Ash | Thin Fishskin → Low Grade | Kelp Fiber → Hemp |
| 2 | Fan Coral → Maple | Tough Eelskin → Medium Grade | Seagrass Fiber → Flax |
| 3 | Blood Coral → Mahogany | Sharkskin → High Grade | Sponge Fiber → Wool |
| 4 | Deepwater Coral → Valenwood | Pristine Sharkskin → Pristine Grade | Sea Cotton → Cotton |
| 5 | Adamant Coral → Ironwood | Pristine Sharkskin → Pristine Grade | Sea Silk → Silk |

These are node tiers, not overrides to material grades: sea silk inherits silk's
actual material grade of 4. Each equivalent inherits the land material's level
adjustment, crafting family, harvesting skill, tools and artisan-point price.
Coral is harvested using forestry tools; marine hides use hunting and marine
fibers use gathering. Marine node descriptions show coral colonies, marine
animals and resource beds under every climate. Viewing nodes never changes them.

All 14 variants have appended crafting and object IDs, distinct saved balances,
and identities retained in bundles and crafted objects. Older player files
initialize new balances to zero. Node frequency, refresh, duration, yields,
skill checks, critical behavior, motes and canonical treasure weights use their
existing paths. Linen and satin recipes are unchanged; exchange marine fibers
for their land ingredients before refining.

```
exchange 10 branch coral for ash
exchange 10 ash for branch coral
exchange 5 tough eelskin for medium grade hide
exchange 10 flax for seagrass fiber
exchange 10 sea silk for silk
```

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

Equivalent regional and marine varieties qualify only when they share the exact
canonical land material. This includes all five timber and fiber node tiers and
all four hide materials.
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

`tests/run_climate_materials.py` exercises every aquatic sector across all zone
terrains and climates, dry sectors in ocean zones, survey, normal world node
regeneration, all marine node tiers, production selection, equivalence,
object conversion, new and legacy pfile balances, node descriptions, exchanger
accounting and special-procedure dispatch.
