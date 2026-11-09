# Ammunition crafting

`craft ammo` supports a saved three-material recipe. Configure with `type`,
`material 1/2/3`, `enhancement`, and `damage`; inspect with `show` or `cost`,
and attempt with `begin`. `reset` restores defaults. The `AmCf` ASCII player
file tag stores seven integers: recipe index, three material IDs (0 means
default), enhancement, element index, damage die. Old player files default
to ordinary arrows. Invalid saved records are ignored.

Slot 1 is wood for shafts, or stone/metal for bullet bodies. Slot 2 is metal
for heads/cores. Slot 3 is cloth binding. Default recipes consume 2/1/1
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
The previous one-line syntax remains available without changing saved settings.

## Portable kit

Object 40448 is an ITEM_CRAFTING_TOOL portable fletching kit. Carrying it
directly or equipping it replaces the ammo tool and station checks only.
Nested containers are not searched. Identity uses prototype vnum and item
type, not keywords or crafting-tool bonus values.

`craft ammo kit` creates the kit with DC 15 woodworking, consuming 2 ash wood,
2 iron and 2 hemp per attempt. Making the kit requires the usual equipped
woodworking tool and carpentry table, even when already carrying a kit.
A kit is reusable and never substitutes for ammo resources or motes.

Install `lib/world/obj/fletching_kit.obj` into an indexed world file before
restarting the server:

```
python3 scripts/install_fletching_kit.py /path/to/mud/lib/world/obj
```

The installer backs up and atomically updates indexed `400.obj`, is idempotent,
and refuses conflicting object 40448. Build/restart the desired game port
and apply `sql/20261109_craft_ammo_help.sql` to its help database. Updating
this repository alone does not install world data into another game instance.

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
