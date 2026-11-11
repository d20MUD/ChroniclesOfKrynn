# Phases 22–23: Artisan's Guidance and stacking audit

Both phases are complete behind the existing disabled gates. No schema changes,
live migration, enablement, deployment or server restart occurred.

## Artisan's Guidance

The checked runtime snapshot now publishes a separate unsaved Artisan flat bonus
and absolute expiry, using the existing accepted membership, campaign, SQL,
feature and release gates. Tiers provide +2/+3/+4. Departure, outage, disablement
and expiration remove eligibility; each actual check independently checks expiry.

`get_craft_production_bonus` is a check-only boundary. It does not modify
`get_craft_skill_value`, raw ranks, specializations, tools, talents, materials,
recipe access, skill-up thresholds, crafting XP, yields or rewards. The helper
accepts production professions and the harvesting professions used by existing
refining recipes; harvesting/surveying/butchering never call it.

Integration points:

- Shared create check, including ammunition, portable kits and ordinary item
  creation, plus the ammunition check-feasibility preview. Critical success/failure
  rules and finite-stack/resource accounting remain unchanged.
- Actual refining completion check, including recipes using mining/other resource
  professions. Non-clan specialization/talent/feat bonuses retain their behavior.
- Potion brewing's actual completion roll, after loading the saved ordinary job
  skill. The clan bonus is not embedded in the queued job or used in its XP.

A craft check snapshots its current flat bonus for both its total and explanatory
message. The same value is added once alongside other check bonuses.

Existing supply orders complete without a skill roll, as their UI explicitly
states. No new failure mechanic was introduced to those jobs. Their ranks, fixed
work completion, limits, rewards and turn-in calculation remain unchanged. Any
later supply-order production checks should use this same check-only boundary;
clan contracts themselves are later phases of the implementation plan.

## Stacking audit and enforced policy

| Family | Authoritative application and non-clan ordering | Duplicate protection |
| --- | --- | --- |
| Company Vitality | Final ordinary maximum plus legendary/gear flat HP, then one clan percentage; current HP never healed by activation. | Runtime publishes one checked family value; final rebuild starts from ordinary values, not prior buffed HP. |
| Arcane Readiness | Existing feat/perk, room, class/config and domain timing, then one rounded clan reduction with circle minimum. Focused Mind keeps its existing event cadence. | Saved actual head-cycle marker prevents rescaling on repeated start, login or buff changes; Arcane Recovery retains its existing exception without double scaling. |
| Psionic Focus | Ordinary eligible tick includes base, helper, feat and resting recovery; one clan percentage with activation-specific fraction, then PSP cap. | Bonus never feeds back into the recovery base; remainder resets on activation changes, combat, expiry and cap. Shared movement helper is untouched. |
| Scholar's Encouragement | Existing character modifiers, happy hour/account order, then one clan addition before final XP cap. | Explicit NPC source/mode pairs; no multiplier on crafting/quests/admin or clan reward facts; no extra aggregate reward. |
| Artisan's Guidance | Ordinary production skill/check bonuses plus one flat clan value. | Check-only call sites; ranks, gathering and XP/reward calculations do not include it. |

The pure query rejects duplicate family rows, even at different tiers/slots or
activation IDs. SQL activation replaces the selected slot atomically and preserves
family cooldowns; it cannot create additive copies. The runtime publishes one
value per family. Approved distinct families coexist within the checked total and
combat-category slot limits. Native query tests now cover every family's duplicate
tiers and all three-family combinations at the appropriate unlock level; SQL
runtime tests additionally hydrate Scholar and Artisan together.

Legacy territory bonus functions (`apply_zone_regen_bonus`, `apply_zone_exp_bonus`,
`apply_zone_skill_bonus`, etc.) have definitions but no production call sites.
They remain dormant. No territory skill/XP/regeneration helpers were enabled.
There is currently no second live source for these same clan modifiers. A future
territory/clan source must merge the same modifier with MAX, not addition, before
these authoritative application points; simply wiring the legacy additive helper
would violate this policy. Ordinary non-clan effects continue to stack in the
orders above.

No buff changes source reward tuples, account windows, Favor amounts, shared
rewards, contract quotas or claim allowances. Successful assisted work remains
ordinary legitimate completion.

## Validation

- Production create-check native tests under ASan/UBSan: all crafting professions
  and tiers, specialization/talent/feat/versatility/artificer overlap, exact DC
  boundaries, expiry/NPC exclusion, natural rolls, rank and failure-XP isolation.
- Production refining/harvesting tests: Artisan changes refining success at its
  DC boundary but not gathering success; existing happy-hour XP/yields, criticals,
  talents, mote grade and depletion behavior pass unchanged.
- Ammunition regression suite: finite recipes, costs, quality, bounds, bonuses,
  template sanitization, guards and resource consumption.
- Prepared/spontaneous preparation, Psionic Focus and Scholar production-path
  regressions; extended pure family/slot query tests.
- Disposable SQL lifecycle/runtime tests: Artisan unlock/hydration, coexistence,
  membership, expiry, outage and disablement plus previous family regressions.
- Legendary foundations/equipment regressions and Make/CMake server builds.

Next: Phase 24 player-facing buff list/activate/cancel commands. The internal
activation and checked runtime services remain disabled by default until explicitly
configured; this work does not enable them on a running game.
