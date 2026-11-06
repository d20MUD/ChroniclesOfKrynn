# Room size

Room size represents the maximum distance within a room. Builders can select
`H) Room size` in `redit`, enter a positive integer for a custom distance, or
enter `0` to use the terrain default. The menu and room stats show the effective
size and whether it is custom or automatic.

Automatic sizes follow terrain changes, including generated wilderness terrain.
Custom sizes remain in place when the sector changes. Both redit copying and
`copyroom` preserve this setting.

| Default | Terrain sectors |
| --- | --- |
| 50 | Field, desert, ocean, tundra, beach |
| 40 | Flying |
| 30 | Hills, underwater, roads, dirt roads, planes/outer planes |
| 20 | Forest, mountain, marshland, underdark wilderness, lava, taiga, seaport |
| 10 | Inside, city, swimmable water, nonswimmable water, zone start, high mountain, underdark city/interior/water/nonswimmable water/no ground, cave, jungle, inside room, river |

## Storage and API

`room_data.room_size` stores the custom value; `0` means automatic. Use
`get_room_size(room_pointer)` or `ROOM_SIZE(room_rnum)` to read the effective
maximum distance. `default_room_size(sector)` returns the sector default.
Invalid sectors or room references fall back to 10.

World files store positive overrides as an optional record before the room's
terminating `S`:

```text
R
75
```

Rooms without this record use the terrain default. Saving an automatic room
omits the record. Existing world files require no conversion. Invalid records
are rejected during loading. Editor input rejects negative values, fractions,
trailing text, and integer overflow.

Validation: `python3 tests/run_room_size.py` checks all 37 terrain IDs, aliases,
automatic terrain updates, custom overrides, editor input, save/load, and copying
under AddressSanitizer and UndefinedBehaviorSanitizer.
