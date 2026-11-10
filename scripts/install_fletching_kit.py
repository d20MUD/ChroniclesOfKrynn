#!/usr/bin/env python3
"""Install the authored portable kit in an already-indexed world object file."""
import argparse
from datetime import datetime, timezone
from pathlib import Path
import os
import re

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('world_objects', type=Path, help='Target lib/world/obj directory')
args = parser.parse_args()
source = Path(__file__).resolve().parents[1] / 'lib/world/obj/fletching_kit.obj'
kit = source.read_text().split('$~')[0].rstrip() + '\n'
target = args.world_objects / '400.obj'
original = target.read_text(encoding="latin-1")
already_installed = False
index = args.world_objects / 'index'
indexed_files = set(index.read_text().split()) if index.exists() else set()
for path in args.world_objects.glob('*.obj'):
    text = path.read_text(encoding="latin-1")
    match = re.search(r'^#40448\s*\n.*?(?=^#\d+\s*$|^\$~)', text, re.M | re.S)
    if match:
        # The repository ships this unindexed authoring template alongside the
        # live object files. It is the input, not a second loaded prototype.
        # Still refuse duplicates if someone indexed it independently.
        if (path.resolve() == source.resolve() and path.name not in indexed_files
                and match.group().strip() == kit.strip()):
            continue
        if path == target and match.group().strip() == kit.strip():
            already_installed = True
            continue
        raise SystemExit(f'Refusing to overwrite existing object 40448 in {path}')
if already_installed:
    print(f'Kit 40448 already installed in {target}')
    raise SystemExit(0)
if len(re.findall(r'^\$~\s*$', original, re.M)) != 1:
    raise SystemExit(f'Expected one object-file terminator in {target}')
updated = re.sub(r'^\$~\s*$', lambda _: kit + '$~\n', original, flags=re.M)
stamp = datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%S%fZ')
backup = target.with_name(target.name + '.backup.' + stamp)
backup.write_text(original, encoding="latin-1")
next_file = target.with_name(target.name + '.next')
next_file.write_text(updated, encoding="latin-1")
next_file.chmod(target.stat().st_mode)
os.replace(next_file, target)
print(f'Installed kit 40448 in {target}; backup: {backup}')
