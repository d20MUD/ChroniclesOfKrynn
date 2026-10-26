# Phase 11: signatures and true dragonlances

Implemented on 2026-10-24. All S01–S13 now use the production activation,
source-lease and rolling-budget engine. Schema remains v5; this phase adds no
SQL migration, live world edits or server restart. Rank costs and the independent
cost of a two-handed item's supporting channel are unchanged.

## Gameplay behavior

| Power | Implementation |
| --- | --- |
| S01 Bulwark | Physical-only 20% reduction, 60/80 cumulative pool, 18 seconds. Uses the recipient's shared prevention allowance. |
| S02 Formation ward | Requires an actual group formation. Self first, then up to two legal same-room allies in deterministic group-list order. +2/+3 against trip and forced movement for 18/24 seconds. Checks group validity during use; does not change rows or bypass targeting. |
| S03 Sure passage | Self plus up to two eligible group members, same selection order. Strongest of F15/T12/S03 walking reductions; no multiplication, mounted reduction or terrain bypass. Remains across room changes; allied recipients lose it on leaving the source's group. |
| S04 Mental bastion | +2/+3 only to Will saves against supported fear, charm/domination and confusion IDs. Preserves effect context for item charm/fear/confusion and innate lich/eidolon fear saves. Existing effects and other saves are unchanged. |
| S05 Renewing touch | 20/30 immediate healing, then 10 at 6, 12 and 18 seconds. Effective healing only; existing healing penalties and recipient allowance apply. Room departure, death and invalid sources cancel delayed ticks. No catch-up burst or revival. |
| S06 Decoy stance | +2 in the ordinary dodge AC component, respecting loss of Dexterity/dodge and the ordinary AC cap. Two/three actual ordinary weapon attempts consume charges, including misses. Spell attacks and AC display queries do not spend charges. Distant Aid substitutes one ally. |
| S07 Runic economy | Two real point-paid casts costing at least 20 receive 5/8 discount each. Strongest of T16/S07, never their sum; ties spend T16 first. Only an actual saving consumes a charge. Shared 20/180-second allowance with T15/T16. Prepared slots, items, free casts and cancelled unpaid casts receive no invented discount. |
| S08 Shared focus | Another eligible ally gains +3/+4 Concentration and a separate 20/30 temporary-HP reserve. The reserve absorbs incoming damage after wards, expires with its source, and never changes or serializes real HP. Repeated grants do not add reserves; only additional granted HP spends the healing allowance. `infuse loadout` shows remaining reserve. |
| S09 Hunter's opening | Tracks the designated enemy. The next actual ordinary weapon attempt against that enemy consumes it, including a miss. +2/+3 attack respects the attack cap; +20/+30 post-defense damage respects resistance, immunity, remaining HP and shared offense allowance. |
| S10 Dragonward | 25% of one actual dragon-breath packet, capped at 60/80. Non-breath damage does not consume it. A multi-component breath does not receive protection on all components. |
| S11 Grounding lance | Requires the actual equipped, registered source lance, not its name. A subsequent ordinary hit with that specific item against a dragon makes a fixed DC25 Fortitude check using native Fortitude bonuses and natural 1/20 rules. A failed save blocks ordinary flee, forced-movement maneuvers and formation changes for exactly six seconds; attacks and spells remain available. Enduring only extends preparation. Registered hard-control-immune dragons instead receive 15/25 resistance- and budget-limited bonus damage. Ordinary misses preserve the preparation. A landed, fully absorbed hit can still deliver control or Rallying strike healing; it cannot create bonus damage through a zero-damage packet. |
| S12 Rider's defiance | Requires a registered source lance while riding one's own living, actual dragon mount. Rider and mount each receive a separate one-hit 20% physical ward capped at 30/40. Dismounting, losing ownership, invalid mount identity/location or invalid source revokes protection. No mount attack is granted. |
| S13 Rallying strike | Saves one designated ally; the next ordinary weapon attempt consumes preparation even on a miss. A hit heals 30/40 only if the ally is still visible, grouped and present. Uses the shared healing allowance. |

All values come from the existing catalog resolver, including base/rank-V
values and each permitted modifier. Activation still commits durable cooldowns
before applying gameplay effects and spends the standard action/recovery.
Protection and Restorative family clocks remain shared with techniques.
Temporary reserves and preparations are never replayed at login.

The lance movement restriction is placed on ordinary combat/formation entry
points, not `char_to_room`, extraction or scripted room transitions. Mandatory
encounter movement and staff teleportation therefore retain their normal paths.
Staff registry disablement also invalidates live true-lance source leases.
The existing Aura of Terror save supplied an affect ID as its saving-throw type;
that call now supplies Will and keeps the affect ID as its effect context.

## Code and validation

Catalog adapters live alongside the technique adapters in
`src/legendary_techniques.c`; temporary HP and group/mount lifetime checks extend
`src/legendary_effects.c`. Native integration covers ordinary attacks and typed
AC, saves, Concentration, movement costs, flee, forced movement and formation.
The help entry and player-guide implementation status are updated.

`tests/run_legendary_signatures.py` runs production adapters against disposable
MariaDB, together with prior persistence, transactions, earnings, commands,
equipment, foundation and technique regressions. Signature cases exercise every
base/V/allowed-modifier combination, target rejection without spending,
individual cooldowns, expiry/source removal, group recipient ceilings, travel
room exceptions, exact heal ticks, weapon-attempt charges, designated targets,
mount ownership, real-lance provenance, fixed-DC boundary, six-second control,
boss fallback, resistance and shared prevention/healing/offense/resource limits.
Cross-technique cases cover Protection recovery and T16/S07 discount selection.

Validation commands:

```sh
ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 \
ESTATE_TEST_CFLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer -g' \
python3 tests/run_legendary_signatures.py
python3 tests/run_legendary_effects.py
python3 tests/run_legendary_rules.py
cmake --build /tmp/krynn-legendary-phase8-cmake -j4
make -j4
git diff --check
```

The fixtures execute real effect/catalog/payment/SQL code with deterministic
adapters for native world, dice and stat calculations. They do not replace a
running-server combat/playtest. Full rank-IV/V integration review and rollout
remain phases 12 onward; no live deployment is claimed by this report.
