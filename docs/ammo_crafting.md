# Ammunition crafting

`craft ammo` supports a saved three-material recipe. Configure with `type`,
`material 1/2/3`, `enhancement`, and `damage`; inspect with `show` or `cost`,
and attempt with `begin`. `reset` restores defaults. The `AmCf` ASCII player
file tag stores seven integers: stable recipe index (0 arrows, 1 bolts, 2 bullets, 3 knives, 4 axes,
5 hammers, 6 javelins, 7 darts, 8 shuriken), three material IDs (0 means
default), enhancement, element index, damage die. Old player files default
to ordinary arrows. Invalid saved records are ignored.

Slot 1 is wood for shafts/handles, stone/metal for bullet bodies, or hard metal
for shuriken blanks. Slot 2 is metal
for heads/cores (hard metal for throwing weapons). Slot 3 is cloth binding. Default recipes consume 2/1/1
(arrows), 1/2/1 (bolts), or 3/1/1 (bullets). Costs for identical materials
are consolidated before availability checks and spending.

All slots use the existing material grades, including canonical regional
materials. Intrinsic enhancement is `floor(sum(grades)/3)-1`. Final enhancement
is the greater of intrinsic and requested. Base DC is
`max(5, 11-sum(grade-1))`. Enhancement above intrinsic adds its normal crafting
level adjustment; elemental damage adds its die size. These magical DC
additions apply after the base floor. Motes use native crafting costs, with
an intrinsic-quality discount for enhancement. The normal item level cap
of 30 still applies.

Each successful batch is one item with 50 finite uses. Unlimited/returning
flags, stale template effects and poison are cleared. Resource consumption,
normal crafting skill checks, delays, XP, carry limits, combat/trade/project
guards apply. Failed rolls consume resources; failed prerequisites do not.
Bare `craft ammo <type>` commands for all nine recipes use saved materials and magical
settings for that ammo type, without changing the saved recipe. `cost arrows`
(and the other types) previews the same plan. A one-shot switch to/from bullets
uses the target default for an incompatible body/shaft selection. Explicit
material choices are never silently substituted when stock runs out. The
older syntax with explicit enhancement/damage arguments remains available
with its own defaults, without changing saved settings.

## Throwing bundles

`craft throwing` aliases `craft ammo`, including `type`, numbered `material`,
`enhancement`, `damage`, `show`, `cost`, `begin` and `reset`. The aliases share
one saved recipe. Original persisted indices 0–2 remain unchanged; new recipe
indices are appended and validated against `NUM_AMMO_CRAFT_RECIPES` by the
player-file loader.

| Type | Native weapon | Default wood/metal/cloth costs | Skill |
| --- | --- | --- | --- |
| knives | throwing knife | 1 ash, 3 iron, 1 hemp | weaponsmithing |
| axes | throwing axe | 2 ash, 4 iron, 1 hemp | weaponsmithing |
| hammers | throwing hammer | 2 ash, 4 iron, 1 hemp | weaponsmithing |
| javelins | javelin | 4 ash, 2 iron, 1 hemp | woodworking |
| darts | dart | 1 ash, 1 iron, 1 hemp | woodworking |
| shuriken | shuriken | 2 iron blanks, 2 iron edges, 1 hemp | weaponsmithing |

Each bundle is an `ITEM_MISSILE` with 50 finite uses. Throwing weapons store
`AMMO_TYPE_THROWN` and their explicit native subtype in `AMMO_VAL_WEAPON_TYPE`;
darts use `AMMO_TYPE_DART`. Native pouch selection, `throw`, proficiency,
damage and finite-consumption code handles these without a new combat path.
Enhancement and elemental damage use the existing material-quality recipe.
Neither unlimited nor returning properties are introduced. Missing inputs
spend nothing; failed craft checks consume the selected resources as usual.

## Portable kit

Krynn uses object 15324, configured by `FLETCHING_KIT_VNUM` in `src/vnums.h`.
The tracked `src/vnums.example.h` records the Krynn setting and the alternative
world's existing 40448 setting. Carrying the configured kit directly or equipping
it replaces the ammo tool and station checks only. Nested containers are not
searched. The kit must be a miscellaneous item (`ITEM_OTHER`, as authored on
Krynn) or a crafting tool (`ITEM_CRAFTING_TOOL`); keywords and tool bonus values
do not determine its identity.

`craft ammo kit` creates that configured object with DC 15 woodworking,
consuming 2 ash wood, 2 bronze and 2 hemp per attempt. Making the kit requires
the usual equipped woodworking tool and carpentry table, even when already
carrying a kit. A kit is reusable and never substitutes for ammo resources or
motes. Krynn's existing object 15324 needs no additional template installation.
Rebuild/restart the desired game port to use a changed vnum setting.

The authored 40448 template and `scripts/install_fletching_kit.py` remain
available for worlds configured to use that vnum.

Validation: `python3 tests/run_craft_ammo.py` executes production planners,
command handling, material helpers, object construction and saved-record
loading under AddressSanitizer. Adjacent ranged/treasure tests cover firing
and generated ammunition.

## Ammo pouch capacity

Pouch value 0 is the maximum number of directly contained ammo stack objects,
not a weight limit or a count of remaining shots. Existing capacity values
are retained. Zero accepts no stacks; the editor's -1 unlimited setting is
honored. One partially used bundle occupies the same single slot as a full
bundle. Pouches bypass container weight capacity checks, while their actual
weight still contributes normally to player encumbrance. Lore (including
structured web lore) and the object editor identify capacity in stacks.

`python3 tests/run_ammo_pouch_capacity.py` runs the native put handler to
check full/empty boundaries, inventory siblings, heavy bundles, partial
bundles, removal, ammo-only restrictions and ordinary container weight checks.

## Preferred ammo depletion and object lifetime

`ammopref` saves a prototype ID, never an ammo object pointer. When a finite
stack reaches its last shot, the native consumption path unlinks it from
the pouch and keeps it as a transient projectile through damage resolution,
so its enhancement/damage still applies to that shot. End-of-shot cleanup
clears context and legacy projectile references before extraction. The extra
Manyshot arrow remains tracked until its turn to be released, allowing an
extraction callback to invalidate it safely. Aliased shots are freed once.

Other compatible stacks become fallback ammo; a replacement of the preferred
type can be selected later using the saved ID. The sanitizer harness
`tests/run_ammo_cleanup.py` covers depletion, fallback, nested contexts,
Manyshot, extraction callbacks and early projectile removal.

### Timed ammo jobs

Ammo, throwing bundles, and portable fletching kits use `craft_update` with
`SCMD_NEWCRAFT_AMMO`. The base duration is 60 seconds, reduced by the rapid
talent for the recipe skill (minimum one second). Ingredients and motes are
consumed at start; the roll, crafting XP and result are deferred until completion.
`craft ammo cancel` and `craft throwing cancel` stop the job and refund all reserved costs.
An immutable eight-integer `AmJb` snapshot and the normal `CrDu`/`CrMe` fields
preserve the selected recipe and remaining time across logins.

### Ammo roll resource outcomes

Ammo and throwing bundles reserve full material/mote costs when the timer
starts. The native skill check now exposes its natural roll to this caller
without changing the public check API or other crafting outcomes. Ordinary
failures refund all reserved costs; natural 1s consume them. Natural 20s produce
75 finite uses and refund floor(cost/2), charging ceil(cost/2) for each combined
material requirement and each mote type. Ordinary successes produce 50 uses at
full cost. Fletching-kit roll outcomes retain their existing costs. Cancellation of either
an ammo/throwing batch or a kit returns the full reserved material/mote costs.
