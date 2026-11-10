#!/usr/bin/env python3
"""Check safe kit world-data installation without changing a game instance."""
from pathlib import Path
import tempfile
import subprocess
import shutil
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
    # Exercise the repository layout: the installer and its unindexed source
    # definition live alongside the indexed target. This previously refused
    # installation, leaving craft ammo kit without a loadable prototype.
with tempfile.TemporaryDirectory() as tmp:
    checkout = Path(tmp)
    (checkout/'scripts').mkdir()
    objects = checkout/'lib/world/obj'
    objects.mkdir(parents=True)
    shutil.copy2(root/'scripts/install_fletching_kit.py', checkout/'scripts')
    (objects/'fletching_kit.obj').write_bytes(kit)
    (objects/'400.obj').write_bytes(original)
    (objects/'index').write_text('400.obj\n$\n')
    command = ['python3', str(checkout/'scripts/install_fletching_kit.py'), str(objects)]
    subprocess.run(command, check=True, capture_output=True)
    installed = (objects/'400.obj').read_bytes()
    assert installed.count(b'#40448\n') == 1
    subprocess.run(command, check=True, capture_output=True)
    assert (objects/'400.obj').read_bytes() == installed
    (objects/'index').write_text('400.obj\nfletching_kit.obj\n$\n')
    assert subprocess.run(command, capture_output=True).returncode != 0
    assert (objects/'400.obj').read_bytes() == installed
print('Kit installer: legacy bytes, backups, idempotency and conflicting-vnum refusal passed.')
