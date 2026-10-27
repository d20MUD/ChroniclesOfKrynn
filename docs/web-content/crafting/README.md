# Crafting website audit — 27 October 2026

Scope: the live crafting hub and all seven pages linked from it on https://krynn.d20mud.com/wiki/crafting. All eight need replacement or expansion. The supplied Markdown is player-facing article content; paste it into each existing page body. Do not include site navigation, sidebars or this audit report in the article bodies.

The implementation reviewed is the current source in this checkout. This audit does not establish which binary or room/NPC assignments are deployed on the live game server. Station, exchanger and artisan services depend on their placement in the game world. No website pages were published or changed.

## Replacement files

| Existing route | Replacement Markdown |
|---|---|
| `/wiki/crafting` | [crafting.md](crafting.md) |
| `/wiki/crafting-tutorial` | [crafting-tutorial.md](crafting-tutorial.md) |
| `/wiki/harvesting-crafting-materials` | [harvesting-crafting-materials.md](harvesting-crafting-materials.md) |
| `/wiki/crafting-motes` | [crafting-motes.md](crafting-motes.md) |
| `/wiki/refining-materials` | [refining-materials.md](refining-materials.md) |
| `/wiki/creating-equipment` | [creating-equipment.md](creating-equipment.md) |
| `/wiki/resizing-gear` | [resizing-gear.md](resizing-gear.md) |
| `/wiki/supply-orders` | [supply-orders.md](supply-orders.md) |

## Findings

### crafting

Expand the hub to cover the current resource sources, skill progression and available systems; retain all chapter links.

### crafting-tutorial

Replace unsupported resize add/remove and supplyorder list/select instructions. Add jewelry, batch refining, catalysts, specialization, modern supply-order flow and golem appearance controls. Correct motes versus materials command descriptions.

### harvesting-crafting-materials

Replace the claim that materials come only from the wild or other players. Add climate-specific timber/fiber, marine resources, compatible exchanges, artisan purchases and treasure. Avoid conflating node tiers with actual material grades. Remove the unconditional claim that a natural 20 always succeeds: the handler checks the total before its critical-success branch.

### crafting-motes

Add actionable allocation/removal commands and in-game reference use. Correct armor enhancement to water motes rather than separate types by armor weight, and include Fortitude and psionic regeneration associations. Retain the verified mote-cost bands. Replace fixed level arithmetic and blanket stacking claims with actual project review.

### refining-materials

Add the missing workflow, batch controls, cold-iron station, dragonmetal recipe and catalyst recipe. Include all nine recipe definitions and their skills, stations and difficulty.

### creating-equipment

Replace the introduction-only article with item selection, descriptions, a working first-item example, material allocation, enhancements, bonuses, motes, level adjustment, catalysts and disenchanting.

### resizing-gear

Replace the promise to publish instructions later with supported select/show/check/begin/reset commands. Remove generic weapon-handling assertions from the crafting instructions and explain transformation eligibility.

### supply-orders

Remove the incorrect NOT YET IMPLEMENTED notice. Document request/material/start/complete, repeat work, gold/skill XP/artisan rewards, the three-order two-hour window, reset versus abandon and timing checks.

## Implementation checked

- `src/crafting_new.c`: command menus and handlers; harvesting and regional/marine substitutions; mote selection and costs; refinement batching; resizing eligibility; disenchanting; supply order requests, rewards, reset/abandon and cooldowns; artisan shop and material exchanges.
- `src/crafting_recipes.c`: refining recipes and broad-sword material requirements.
- `src/crafting_new.h` and `src/constants.c`: mote and material definitions.
- `src/treasure.c`: direct material and mote awards, level-gated material quality and canonical-material selection.
- `src/interpreter.c`: command routing, including the artisan service command.
- `docs/climate_material_varieties.md`, `docs/random_crafting_treasure.md` and `docs/CRAFTING_TUTORIAL.md`: supporting documentation, cross-checked against source rather than treated as authoritative when inconsistent.

## Verification

All eight live routes were fetched and their article bodies reviewed. Every replacement was parsed as CommonMark; headings, code fences and internal crafting links were checked. All linked crafting routes returned HTTP 200. No code changes or build are needed for these replacement content files.
