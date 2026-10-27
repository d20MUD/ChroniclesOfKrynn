#!/usr/bin/env python3
"""Read-only Phase-15 registry preflight for the live world source tree."""
from pathlib import Path
import argparse
import re

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_WORLD = Path('/home/krynn/live/mud/lib/world')
SMITH = (10500, 8795, 'Lord Orelor Silverveins')
BOSSES = {
    9356: (9462, 'Tombfyre'), 5316: (5382, 'Blacksnake'),
    34503: (34516, 'Gadrok'), 15607: (15619, 'storm lord'),
    4714: (5121, 'necrotheurge'), 3231: (3280, 'archmage'),
    5909: (5955, 'priestess'), 5918: (5940, 'paladin'),
    630: (676, 'Azurathira'), 13530: (13595, 'Adadyr'),
    4712: (4730, 'dracolich'), 5402: (5435, 'Thorian'),
}

def mob_text(world: Path, vnum: int) -> str:
    for path in (world / 'mob').glob('*.mob'):
        data = path.read_text(errors='replace')
        found = re.search(rf'(?m)^#{vnum}$([\s\S]*?)(?=^#\d+$|\Z)', data)
        if found:
            return found.group(1)
    raise RuntimeError(f'missing mobile {vnum}')

def spawn_rooms(world: Path, vnum: int) -> set[int]:
    rooms = set()
    for path in (world / 'zon').glob('*.zon'):
        for line in path.read_text(errors='replace').splitlines():
            fields = line.split()
            if len(fields) >= 5 and fields[0] == 'M' and fields[2] == str(vnum):
                rooms.add(int(fields[4]))
    return rooms

def checked(world: Path, vnum: int, room: int, phrase: str) -> None:
    text = mob_text(world, vnum)
    if phrase.casefold() not in text.casefold():
        raise RuntimeError(f'mobile {vnum} does not contain expected identity {phrase!r}')
    if room not in spawn_rooms(world, vnum):
        raise RuntimeError(f'mobile {vnum} is not normally reset into room {room}')

def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--world', type=Path, default=DEFAULT_WORLD)
    args = parser.parse_args()
    checked(args.world, *SMITH)
    for vnum, data in BOSSES.items():
        checked(args.world, vnum, *data)
    # No MySQL registry may sneak back into the release configuration.
    registry = (ROOT / 'sql/20261025_legendary_release_registry.sql').read_text()
    if 'legendary_boss_registry' in registry or 'legendary_smiths' in registry:
        raise RuntimeError('release configuration must not contain a MySQL boss or smith registry')
    print(f'PASS: verified smith and {len(BOSSES)} approved mobile candidates; builder flags remain a reviewed world edit')

if __name__ == '__main__':
    main()
