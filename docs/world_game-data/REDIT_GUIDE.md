# REDIT: room editor and builder reference

This guide describes the room editor and room mechanics in the current Krynn
source, reviewed on 2026-10-23. It covers all 42 `ROOM_` flags, all 37 terrain
sectors, movement costs, exits, traps, descriptions, and saving.

Room flags and sectors are separate settings. A sector describes terrain and
supplies a movement-cost value; flags add restrictions or special behavior.
Neither setting replaces zone configuration, scripts, resets, or special
procedures.

## Opening, creating, and saving rooms

| Command | Purpose |
| --- | --- |
| `redit` | Edit the room you are standing in. |
| `redit <room-vnum>` | Edit that room, or create it if the vnum is unused and belongs to a zone you can edit. |
| `redit save <zone-number>` | Save that zone's rooms to disk. This argument is a zone number, not a room vnum. |
| `redit save` | Attempts to use the configured OLC zone. Use the explicit zone-number form instead; see the parser caveat below. |

The no-argument save path currently converts the configured zone to its bottom
room vnum before looking it up as a zone number. This can fail or resolve the
wrong zone, so use `redit save <zone-number>` explicitly.

You need builder access and permission to edit the destination zone. A room
cannot be edited by two builders simultaneously. Dynamically allocated
wilderness rooms cannot be edited with `redit`; the editor directs you to use
`buildwalk` to create static rooms first.

To create a room, choose an unused **numeric vnum** in your permitted zone.
`redit new` is not supported by this command parser.

New rooms start as `An unfinished room`, with a placeholder description, the
Inside sector, no flags, and coordinates `(0, 0)`.

Choose `Q` to leave the main menu. When prompted to save changes, enter lowercase
`y` to commit or `n` to discard. If automatic OLC disk saving is enabled, the
editor reports `Room saved to disk.` Otherwise it reports `Room saved to memory.`
In the latter case, use `redit save <zone-number>` after leaving the editor to
make the changes survive a restart. Saving a text buffer with `/s` only returns
that text to the room editor; it does not complete the room save.

## Main menu

| Choice | Setting | What it does |
| --- | --- | --- |
| `1` | Name | Room title shown by look, maps and other room displays. Current limit is 75 non-protocol characters, with a 150-byte name buffer. |
| `2` | Description | Main room text, edited in the multiline editor; buffer limit is 2,048 bytes. |
| `3` | Room flags | Toggle room properties and restrictions; see the complete table below. |
| `4` | Sector type | Choose one terrain sector. See the terrain and movement tables. |
| `5` | Exit north | Edit the north exit. |
| `6` | Exit east | Edit the east exit. |
| `7` | Exit south | Edit the south exit. |
| `8` | Exit west | Edit the west exit. |
| `9` | Exit up | Edit the up exit. |
| `A` | Exit down | Edit the down exit. |
| `B` | Exit northwest | Available when diagonal directions are enabled. |
| `C` | Exit northeast | Available when diagonal directions are enabled. |
| `D` | Exit southeast | Available when diagonal directions are enabled. |
| `E` | Exit southwest | Available when diagonal directions are enabled. |
| `F` | Extra descriptions | Add keyword-based details players can examine. |
| `S` | Script | Attach room DG triggers through the script menu. |
| `G` | Coordinates | Set the room's X and Y coordinates for wilderness integration. |
| `Z` | SpecProc | Select a registered room special procedure, or `0` for none. |
| `W` | Copy room | Copy an existing room into the room being edited. Review its exits, scripts, flags and other inherited settings. |
| `X` | Delete room | Confirm deletion. This acts immediately through the deletion path; it is not a normal edit awaiting the later `Q` save prompt. |
| `Q` | Quit | Leave the editor, saving or discarding changes when prompted. |

The current room editor has **no main-menu `Y) Trap Setup` option**. Its trap
settings are under each exit's `7) Exit trap` submenu. Object or mobile editors
can have different menus.

### Writing descriptions

The multiline editor displays its own commands. The most useful are:

- `/s`: save the text buffer and return to the room editor.
- `/a`: abort the text edit.
- `/c`: clear the current buffer.
- `/h`: display the text editor's command list.

Use the room description for what is immediately visible. Use extra descriptions
for details such as carvings, signs, statues, and inscriptions. In the extra
description menu, `1` sets keywords, `2` edits the text, `3` advances to the next
entry, and `0` returns. Complete the current entry before advancing. To remove an
entry, clear its description with `/c`, save the empty buffer with `/s`, and leave
that submenu as instructed by the editor.

## Exits, doors, and keys

Exits are directional. Creating room A's north exit to room B does **not**
automatically create room B's south exit to room A. Edit both ends if travel
should work both ways. Also configure door names, keys and traps on the
appropriate sides.

| Exit choice | Setting | Purpose |
| --- | --- | --- |
| `1` | Exit to | Destination room vnum. The destination must already exist. `-1` leaves the destination unset. |
| `2` | Description | Text displayed when examining that direction; multiline buffer limit is 2,048 bytes. |
| `3` | Door name | Keywords used to refer to the door, such as `oak door` or `iron gate`. |
| `4` | Key | Object vnum of the required key. A negative value means no key. |
| `5` | Door flags | Choose the door behavior from the table below. |
| `6` | Purge exit | Remove this exit and its associated directional traps. |
| `7` | Exit trap | Create, remove or configure a directional trap. |
| `0` | Back | Return to the main room menu. |

| Door flag choice | Behavior |
| --- | --- |
| `0` | No door: a plain passage. |
| `1` | Closeable door. |
| `2` | Closeable, pickproof door. |
| `3` | Hidden, closeable door. |
| `4` | Hidden, pickproof, closeable door. |

These choices replace the door's flag combination. A closeable door is not the
same as a door that resets closed or locked. Use the zone editor's door-state
reset commands to establish recurring closed/locked states. A key vnum alone
does not make a door start locked.

### Exit traps

When no trap exists, choice `1` creates a default **average spike trap** triggered
by leaving the room in that exit's direction. Once a trap exists, the menu is:

| Choice | Setting | Purpose |
| --- | --- | --- |
| `1` | Remove trap | Delete the directional trap. |
| `2` | Trigger type | Choose movement/leave-room, opening the door, or unlocking the door. |
| `3` | Trap type | Select the damage/effect type from the displayed list. |
| `4` | Severity | Select the trap's severity from the displayed list. |
| `5` | Detect DC | Difficulty of detecting the trap. |
| `6` | Disarm DC | Difficulty of disarming the trap. |
| `7` | Save DC | Difficulty of the victim's saving throw. |
| `0` | Back | Return to the exit menu. |

DCs are clamped to a minimum of 1. Trigger choices in this submenu are 1–3;
the trap-type and severity lists use their displayed **zero-based** numbers.
Changing a trap's type, severity, or trigger recreates the trap with defaults:
set those first, then set custom DCs. A leave-room trap is attached to the
**source** room and exit, not to arrival at the destination.

`ROOM_HASTRAP` is maintained by the trap system. Setting that flag by itself
does not create an actual trap. `ROOM_RANDOM_TRAP` requests generated traps
instead of defining a specific directional trap.

## Complete room flag reference

The code indices are zero-based. The room editor's choices are **index + 1**.
For example, `ROOM_FOG` has code index 27 but is choice **28** in `redit`.
Choose one flag number at a time to toggle it, `0` to return, or `-1` to clear
all flags. Clearing all flags also removes system-managed bits from the edited
copy; use individual toggles when changing an existing room.

| Editor choice | Code index | Flag | Purpose and current behavior |
| --- | --- | --- | --- |
| 1 | 0 | `ROOM_DARK` | Natural darkness. Light sources and light effects can overcome it; Inside/City sector lighting takes precedence in the normal darkness helper. See lighting notes. |
| 2 | 1 | `ROOM_DEATH` | Death-trap marker. NPC wandering and several travel effects avoid it; descriptions are forced to display. With death-traps-as-dumps configured, a dump special removes dropped gear. The flag alone does not implement instant death on ordinary walking in the current movement path. |
| 3 | 2 | `ROOM_NOMOB` | Prevents ordinary NPC random wandering into the room. It is not a universal ban on NPC resets, scripted placement, following or summoning. |
| 4 | 3 | `ROOM_INDOORS` | Marks the room as indoors for outdoor/weather and related ability checks. Set this along with an appropriate indoor sector. |
| 5 | 4 | `ROOM_PEACEFUL` | Safe-room restriction used by combat, aggressive abilities and offensive magic checks. Does not itself provide healing, housing ownership or environmental hazard protection. |
| 6 | 5 | `ROOM_SOUNDPROOF` | Blocks communication paths and bardic performance checks that respect soundproofing. Also prevents ordinary spellcasting speech; psionic powers and alchemist extracts have explicit casting exceptions. |
| 7 | 6 | `ROOM_NOTRACK` | Blocks tracking/pathfinding paths that respect the no-track flag. Not an invisible barrier or a replacement for missing exits. |
| 8 | 7 | `ROOM_NOMAGIC` | Suppresses magic through casting/resolution checks, including checks of caster and target rooms. Weapon-poison resolution has an explicit exception. Does not remove every existing magical affect. |
| 9 | 8 | `ROOM_TUNNEL` | Limits occupancy using the configured tunnel size, counting PCs; mounted entry is refused. The capacity is a server setting, not inherently one occupant. |
| 10 | 9 | `ROOM_PRIVATE` | Privacy/travel restriction: blocks ordinary magical arrival/portals through relevant checks. Some staff travel checks treat a room with two occupants as private. Normal walking has no blanket two-person limit. |
| 11 | 10 | `ROOM_STAFFROOM` | Requires LVL_STAFF or higher for ordinary entry and restricts several travel destinations. |
| 12 | 11 | `ROOM_HOUSE` | Legacy house-system room marker. Used with actual house ownership/guest records; setting the flag alone does not create a house. |
| 13 | 12 | `ROOM_HOUSE_CRASH` | Internal legacy house dirty/save marker, set when house contents change. Do not manually use as a gameplay setting. |
| 14 | 13 | `ROOM_ATRIUM` | Legacy house entrance marker. Movement from an atrium invokes house access checks. Coordinate with the house system. |
| 15 | 14 | `ROOM_OLC` | OLC/preservation marker used by administrative room handling. Not a substitute for builder permissions or the editor save process. |
| 16 | 15 | `ROOM_BFS_MARK` | Internal breadth-first-search visited marker, displayed as `*`. Do not permanently set it. |
| 17 | 16 | `ROOM_WORLDMAP` | Worldmap display/routing marker. Coordinate with wilderness/zone configuration and coordinates; this alone does not construct a wilderness map. |
| 18 | 17 | `ROOM_REGEN` | Doubles the natural HP regeneration calculation, with a minimum of 2 before doubling, and contributes a 25% spell-preparation time reduction. Does not universally double every resource. |
| 19 | 18 | `ROOM_FLY_NEEDED` | Falling hazard when an unsupported character or object has a downward exit. Flight/levitation and mount checks apply; build a valid downward path. Distinct from the In Flight sector entry requirement. |
| 20 | 19 | `ROOM_NORECALL` | Blocks recall/return spell paths that check it. Recall also checks no-teleport. Does not stop ordinary walking. |
| 21 | 20 | `ROOM_SINGLEFILE` | Narrow-passage ordering and combat restrictions: characters can obstruct movement and many attacks/abilities are restricted to adjacent positions. Distinct from group formation rows and tunnel capacity. |
| 22 | 21 | `ROOM_NOTELEPORT` | Blocks supported teleport/travel paths. Exact source/destination checks depend on the spell or command; test each intended travel method. |
| 23 | 22 | `ROOM_MAGICDARK` | Magical darkness, with explicit vision restrictions beyond ordinary darkness. Normal lights do not reliably defeat it. See lighting precedence notes. |
| 24 | 23 | `ROOM_MAGICLIGHT` | Magical illumination. The normal darkness helper treats it as lit before checking magical darkness, but some viewing paths directly check the magical-darkness bit. Avoid combining contradictory light flags. |
| 25 | 24 | `ROOM_NOSUMMON` | Blocks summon checks when either the caster or target is in a flagged room. Not a general prohibition on NPC resets or every companion ability. |
| 26 | 25 | `ROOM_NOHEAL` | Sets natural HP regeneration to zero. Does not itself prohibit healing spells, spell preparation or every other resource recovery. |
| 27 | 26 | `ROOM_NOFLY` | Refuses ordinary entry by a character currently flying. Does not automatically dispel flight; it is an entry restriction. |
| 28 | 27 | `ROOM_FOG` | Fog obscures mortal room contents/descriptions, directional viewing, scanning and map paths. Immortals can bypass fog vision restrictions. Gust of Wind and Wind Wall clear the room fog flag at runtime. |
| 29 | 28 | `ROOM_AIRY` | Provides breathable air underwater: accepted by scuba/entry checks and prevents the underwater drowning hazard. Also displays air bubbles. Not merely decorative wind flavor. |
| 30 | 29 | `ROOM_OCCUPIED` | Internal dynamic wilderness occupancy/reservation marker. Maintained by wilderness allocation and character placement; stripped during room-file saving. Do not manually reserve static rooms with it. |
| 31 | 30 | `ROOM_SIZE_TINY` | Ordinary entry requires Tiny size or smaller. An actual size restriction. |
| 32 | 31 | `ROOM_SIZE_DIMINUTIVE` | Ordinary entry requires Diminutive size or smaller. An actual size restriction. |
| 33 | 32 | `ROOM_CLIMB_NEEDED` | Entry Climb check with DC 13 + destination zone minimum level; Grasp reduces DC by 2. Failure without flight inflicts falling damage and denies entry. Flight bypasses the obstacle. |
| 34 | 33 | `ROOM_HASTRAP` | Trap-presence marker maintained with actual trap records. Does not create a trap by itself. |
| 35 | 34 | `ROOM_GENDESC` | Use generated wilderness descriptions for a static wilderness room. Requires wilderness context; intended for fixed rooms integrated with generated terrain. |
| 36 | 35 | `ROOM_PLAYER_SHOP` | Legacy player-shop marker currently used to prevent house sorting. Does not create a rentable stall; use `stall admin add` for the persistent player-stall system. |
| 37 | 36 | `ROOM_RANDOM_TRAP` | Requests automatic room-trap generation in zone trap generation/reset processing, regardless of the zone-wide random-trap flag. Generation rules and existing traps still apply. |
| 38 | 37 | `ROOM_RANDOM_CHEST` | Allows the random-chest placement routine to select this room without the zone random-chest flag; an existing random chest prevents another. Actual loading still goes through the chest/reset system. |
| 39 | 38 | `ROOM_HARVEST_NODE` | Forces the harvest-node selection check to succeed. Material selection, valid terrain and resource-generation rules still matter; it does not specify a material or quantity. |
| 40 | 39 | `ROOM_ROAD` | Reduces the movement base by 1 when LEAVING the flagged room, before multipliers, skills and the movement floor. This is separate from choosing a road sector. |
| 41 | 40 | `ROOM_VEHICLE` | Vehicle/vessel marker used for generated vessel rooms and vessel-specific command checks. Flagging an ordinary room does not create or register a vessel. |
| 42 | 41 | `ROOM_DOCKABLE` | Declared docking marker (also aliased as DOCKABLE). No direct flag check was found in the current vessel C implementation; actual docking uses vessel records, room selection and terrain rules. Do not rely on this flag alone to enable docking. |

### Lighting, fog, and indoor settings

Lighting depends on sector, flags, room affects, carried light, abilities, and
vision type. In `room_is_dark()`, Inside, Inside Room and City sectors return
"lit" before flag checks. Magical light and room light affects are then checked
before magical darkness; ordinary room light is checked before natural darkness.
Some look/scan paths also check `ROOM_MAGICDARK` directly. Therefore:

- Use a non-automatically-lit sector, such as Cave, when designing darkness.
- Set `ROOM_INDOORS` separately when a cave/building should count as indoors.
- Do not assume `ROOM_DARK` makes every City or Inside room pitch black.
- Avoid setting both `ROOM_MAGICDARK` and `ROOM_MAGICLIGHT`.
- Test with a mortal character and the intended light/vision equipment.

Fog is independently checked in several viewing and mapping paths. Wind spells
remove the active flag; they do not rewrite the saved room file, so a permanent
flag in world data can return when that data is loaded again.

### System-managed and legacy flags

Leave `ROOM_BFS_MARK`, `ROOM_OCCUPIED`, `ROOM_HOUSE_CRASH` and trap-presence
bookkeeping to their systems. Use house tools for legacy house ownership rather
than trying to construct it solely by toggling `ROOM_HOUSE` and `ROOM_ATRIUM`.
The newer persistent housing and rented-stall services also need their own
records. Neither a house flag nor `ROOM_PLAYER_SHOP` creates those records.

## Terrain sectors and movement costs

Sector-menu choices are also **code index + 1**. Select one sector; they are
not independent toggles.

The **base** column below is the value in `movement_loss[]`. It is not the final
number deducted from a character's movement points. The **same-terrain subtotal**
shows base × 10 for travel between two rooms of that sector, before feats, road
flags, skills, speed, and other modifiers.

| Editor choice | Code index | Sector | Menu label | Base | Same-terrain subtotal |
| --- | --- | --- | --- | --- | --- |
| 1 | 0 | `SECT_INSIDE` | Inside | 1 | 10 |
| 2 | 1 | `SECT_CITY` | City | 1 | 10 |
| 3 | 2 | `SECT_FIELD` | Field | 2 | 20 |
| 4 | 3 | `SECT_FOREST` | Forest | 3 | 30 |
| 5 | 4 | `SECT_HILLS` | Hills | 4 | 40 |
| 6 | 5 | `SECT_MOUNTAIN` | Low Mountains | 7 | 70 |
| 7 | 6 | `SECT_WATER_SWIM` | Water (Swim) | 4 | 40 |
| 8 | 7 | `SECT_WATER_NOSWIM` | Water (No Swim) | 3 | 30 |
| 9 | 8 | `SECT_FLYING` | In Flight | 1 | 10 |
| 10 | 9 | `SECT_UNDERWATER` | Underwater | 5 | 50 |
| 11 | 10 | `SECT_ZONE_START` | Zone Entrance | 1 | 10 |
| 12 | 11 | `SECT_ROAD_NS` | Road North-South | 1 | 10 |
| 13 | 12 | `SECT_ROAD_EW` | Road East-West | 1 | 10 |
| 14 | 13 | `SECT_ROAD_INT` | Road Intersection | 1 | 10 |
| 15 | 14 | `SECT_DESERT` | Desert | 3 | 30 |
| 16 | 15 | `SECT_OCEAN` | Ocean | 11 | 110 |
| 17 | 16 | `SECT_MARSHLAND` | Marshland | 5 | 50 |
| 18 | 17 | `SECT_HIGH_MOUNTAIN` | High Mountains | 10 | 100 |
| 19 | 18 | `SECT_PLANES` | Outer Planes | 6 | 60 |
| 20 | 19 | `SECT_UD_WILD` | Underdark - Wild | 3 | 30 |
| 21 | 20 | `SECT_UD_CITY` | Underdark - City | 1 | 10 |
| 22 | 21 | `SECT_UD_INSIDE` | Underdark - Inside | 1 | 10 |
| 23 | 22 | `SECT_UD_WATER` | Underdark - Water (Swim) | 4 | 40 |
| 24 | 23 | `SECT_UD_NOSWIM` | Underdark - Water (No Swim) | 3 | 30 |
| 25 | 24 | `SECT_UD_NOGROUND` | Underdark - In Flight | 1 | 10 |
| 26 | 25 | `SECT_LAVA` | Lava | 6 | 60 |
| 27 | 26 | `SECT_D_ROAD_NS` | Dirt Road North-South | 1 | 10 |
| 28 | 27 | `SECT_D_ROAD_EW` | Dirt Road East-West | 1 | 10 |
| 29 | 28 | `SECT_D_ROAD_INT` | Dirt Road Intersection | 1 | 10 |
| 30 | 29 | `SECT_CAVE` | Cave | 3 | 30 |
| 31 | 30 | `SECT_JUNGLE` | Jungle | 4 | 40 |
| 32 | 31 | `SECT_TUNDRA` | Tundra | 4 | 40 |
| 33 | 32 | `SECT_TAIGA` | Taiga | 4 | 40 |
| 34 | 33 | `SECT_BEACH` | Beach | 2 | 20 |
| 35 | 34 | `SECT_SEAPORT` | Sea Port | 1 | 10 |
| 36 | 35 | `SECT_INSIDE_ROOM` | Inside Room | 1 | 10 |
| 37 | 36 | `SECT_RIVER` | River | 4 | 40 |

The source also defines `SECT_OUTTER_PLANES` as an alias for `SECT_PLANES`, and
`SECT_UD_WATER_NOSWIM` as an alias for `SECT_UD_NOSWIM`. These are not additional
menu entries.

### How the actual movement deduction is calculated

Ordinary directional walking uses the calculation in `do_simple_move()` in
`movement.c`. A matching helper exists in `movement_cost.c`, but the walking
path currently contains its own calculation.

The order is:

1. Average the source and destination sector bases, using integer division:
   `base = (source_base + destination_base) / 2`.
2. Woodland Stride sets that starting base to 1 when the character is outdoors
   and the relevant character/mount has the feat.
3. A PC's Fast Movement feat subtracts 1. `ROOM_ROAD` on the **source** room
   subtracts another 1.
4. Difficult terrain on the destination usually doubles the result. Inquisitor
   Wilderness Stride can ignore the penalty outdoors; its higher rank uses
   × 1.5, with integer truncation, for indoor/magical difficult terrain.
5. Spot mode doubles it. Listen mode doubles it again. Moving while reclining
   or lower multiplies it by 4.
6. Multiply by 10.
7. Subtract the movement skill roll. On foot, this is Survival. Entering Hills,
   Low Mountains or High Mountains also subtracts half a Climb skill roll.
8. Subtract `speed - 30`. Speed above 30 reduces cost; speed below 30 increases it.
9. Apply the minimum: normally **5 movement points**, or **1** with Shadow Walk
   on the relevant character/mount.

Mounted movement has an additional current-code exception: after this
calculation, the ordinary walking path sets the rider's `need_movement` to **0**.
Do not use the terrain subtotals as a prediction of mounted movement deductions.
Special travel modes and other movement costs can have their own rules.

Examples, before skills, speed, feats and other modifiers:

| Trip | Calculation | Subtotal |
| --- | --- | --- |
| City → City | `(1 + 1) / 2 × 10` | 10 |
| Field → Forest | Integer average of 2 and 3 is 2; then × 10 | 20 |
| Forest → Hills | Integer average of 3 and 4 is 3; then × 10 | 30 |
| City → Low Mountains | `(1 + 7) / 2 × 10` | 40 |
| Low Mountains → Low Mountains | `(7 + 7) / 2 × 10` | 70 |
| Forest → Forest, leaving a `ROOM_ROAD` room | `(3 - 1) × 10` | 20 |
| Forest → Forest, Spot and Listen both active | `3 × 2 × 2 × 10` | 120 |

The last two examples illustrate separate settings: a road **sector** supplies
a base of 1, while the `ROOM_ROAD` **flag** subtracts 1 from the averaged base.
A cheap sector does not make travel free because the final movement floor
normally still applies.

### Terrain-specific requirements and hazards

| Terrain or flag | Additional behavior |
| --- | --- |
| Water (Swim), Underdark Water (Swim) | Swimming/boat/water-walking/flight checks apply when entering or leaving these water sectors. Natural swimmers bypass the Athletics roll. The swim DC uses `13 + destination zone minimum level`. |
| Water (No Swim), Underdark Water (No Swim) | Ordinary swimming is insufficient; boat or an accepted equivalent is required. Checks can apply to both character and mount. |
| In Flight, Underdark In Flight | Flight capability is required when moving between these sectors and neighboring rooms. Separate from the falling flag. |
| Underwater | Water movement plus underwater breathing/scuba entry checks. The periodic hazard can inflict drowning damage on living characters without water breathing or `ROOM_AIRY`. |
| High Mountains | Climbing capability is required when entering or leaving this terrain. Flight, climbing equipment/affects and Spider Climb are among the accepted paths. This capability check is separate from the `ROOM_CLIMB_NEEDED` skill check. |
| Ocean | Ordinary mortal movement requires an ocean-ready ship; do not treat it as merely another swim sector. Vessel travel has its own terrain rules. |
| Lava | Periodic hazard inflicts 1–50 fire damage on PCs without the elemental-protection affect; normal damage protections can affect the result. |
| Cave and Underdark sectors | Affect environmental/light handling. Underdark sectors are excluded from the outdoors helper; Cave is not, so set `ROOM_INDOORS` when a cave should count as indoors. |
| River, Sea Port, Beach | Have their listed terrain values; do not assume every water-looking sector enters the same swimming/drowning branch. Vessel handling also has separate terrain checks. |
| `ROOM_FLY_NEEDED` | Falling uses this flag and a downward exit. Plan the landing rooms and test the full fall route. |

Sector labels alone do not imply scripted weather damage, encounter levels,
resource types or plane effects. Those require the relevant mechanics, zone
settings, scripts or special procedures.

## Scripts, special procedures, and resets

`S) Script` attaches DG triggers to the room. Use room-appropriate triggers;
mobile and object triggers have different owners and available variables.

`Z) SpecProc` attaches one registered C special procedure. The available list is
provided by the game's special-procedure registry. Choose `0` to clear it, or
`Q` to return. A special procedure can handle room commands or behavior; it is
not the same thing as attaching a DG trigger.

Room editing does not define NPC/object population resets, door reset states,
zone levels, zone-wide flags or reset timing. Configure those through the zone
editor and the appropriate mob/object editors. Zone minimum level matters for
room Climb/Swim DCs and generated content, so review it when creating terrain
challenges.

Permanent static rooms normally save in the world's zone-based `.wld` files.
The room's live inhabitants, current ground items, dynamic wilderness occupancy
and temporary room affects are not equivalent to its editable template.

## Example: a climbable cave passage

1. Create two unused room vnums in a permitted zone with `redit <vnum>`.
2. Set names and descriptions. Use sector **30) Cave** for both.
3. Set room flag **4) Indoors** for both. If the passage should be dark, add
   **1) Dark**; the Cave sector avoids the automatic Inside-sector lighting.
4. Create the outgoing exit on the first room and the reverse exit on the second.
5. Add **33) Climb-Needed** to a destination that requires an entry Climb check.
   Its DC will be `13 + that zone's minimum level`, before applicable modifiers.
6. Add an exit description or keyword-based extra description for the climb.
7. Leave each editor with `Q`, answer lowercase `y`, and ensure the rooms are
   saved to disk. Use `redit save <zone-number>` if only saved to memory.
8. Test both directions with a mortal character: movement points, visibility,
   Climb failure/success and flight bypass. Staff access or vision can hide
   restrictions during testing.

## Common mistakes to avoid

- Confusing editor choices with code indices. Fog is choice 28; Cave is sector
  choice 30. Both have different zero-based code values.
- Assuming a reverse exit is created automatically.
- Expecting door flags or a key assignment to create a locked door reset.
- Treating `ROOM_PRIVATE` as a general walking occupancy cap.
- Treating `ROOM_NOHEAL` as a ban on all healing spells or all regeneration.
- Setting only `ROOM_HASTRAP` and expecting a trap to appear.
- Setting only `ROOM_PLAYER_SHOP` and expecting a rental stall. See the
  [player shop setup guide](../guides/PLAYER_SHOPS.md).
- Assuming a flag labeled Death-Trap guarantees instant death. Build and test
  the actual hazard or scripted death behavior.
- Saving the description with `/s` but never saving the room or zone.

## Source references

These files define the behavior described above:

- [redit.c](../../src/redit.c): command syntax, menus, parsing, copying, trap
  configuration and editor saving.
- [structs.h](../../src/structs.h): room flag indices and sector definitions.
- [constants.c](../../src/constants.c): editor labels and `movement_loss[]`.
- [movement.c](../../src/movement.c): ordinary walking costs and entry checks.
- [movement_cost.c](../../src/movement_cost.c): matching movement-cost helper.
- [movement_validation.c](../../src/movement_validation.c): swimming, flight,
  scuba and climbing capability checks.
- [movement_falling.c](../../src/movement_falling.c): falling rules.
- [utils.c](../../src/utils.c): darkness, environment and chest-placement checks.
- [limits.c](../../src/limits.c): natural HP regeneration and periodic hazards.
- [spell_prep.c](../../src/spell_prep.c): regeneration-room preparation bonus.
- [spell_parser.c](../../src/spell_parser.c) and [spells.c](../../src/spells.c):
  casting and magical travel restrictions.
- [traps_new.c](../../src/traps_new.c): trap records and automatic generation.
- [crafting_new.c](../../src/crafting_new.c): harvest-node selection.
- [genwld.c](../../src/genwld.c): room-file saving and transient flag handling.
- [house.c](../../src/house.c), [vessels_docking.c](../../src/vessels_docking.c)
  and [spec_assign.c](../../src/spec_assign.c): legacy housing, vessel integration
  and special-procedure assignment.
