#!/usr/bin/env python3
"""Check safe kit world-data installation without changing a game instance."""
from pathlib import Path
import tempfile
import subprocess
root = Path(__file__).resolve().parents[1]
kit = (root/'lib/world/obj/fletching_kit.obj').read_bytes()
with tempfile.TemporaryDirectory() as tmp:
    target = Path(tmp)/'400.obj'
    original = b'#40000\nlegacy\x90~\n$~\n'
    target.write_bytes(original)
    command = ['python3', str(root/'scripts/install_fletching_kit.py'), tmp]
    subprocess.run(command, check=True, capture_output=True)
    installed = target.read_bytes()
    assert b'legacy\x90~' in installed and installed.count(b'#40448\n') == 1
    assert list(Path(tmp).glob('*.backup.*'))[0].read_bytes() == original
    subprocess.run(command, check=True, capture_output=True)
    assert target.read_bytes() == installed
    (Path(tmp)/'401.obj').write_bytes(kit)
    assert subprocess.run(command, capture_output=True).returncode != 0
    assert target.read_bytes() == installed
print('Kit installer: legacy bytes, backups, idempotency and conflicting-vnum refusal passed.')
