#!/usr/bin/env python3
"""Check shop conversion backups/restoration using temporary files only."""
from pathlib import Path
import subprocess
import tempfile

binary = Path(__file__).resolve().parents[1] / "bin" / "shopconv"
with tempfile.TemporaryDirectory(prefix="shopconv-check-") as directory:
    root = Path(directory)
    for name in ["ordinary.shp", "shop with spaces; $(touch marker).shp", "x" * 190 + ".shp"]:
        source = root / name
        original = b"$~\n"
        source.write_bytes(original)
        subprocess.run([str(binary), str(source)], check=True, capture_output=True)
        assert source.read_bytes() == b"LuminariMUD v3.0 Shop File~\n$~\n"
        assert Path(str(source) + ".bak").read_bytes() == original
        assert not Path(str(source) + ".tmp").exists()
    assert not (root / "marker").exists()
    source = root / "already converted.shp"
    original = b"LuminariMUD v3.0 Shop File~\n$~\n"
    source.write_bytes(original)
    subprocess.run([str(binary), str(source)], check=True, capture_output=True)
    assert source.read_bytes() == original
    assert not Path(str(source) + ".tmp").exists()
    missing = root / "missing.shp"
    stale = Path(str(missing) + ".tmp")
    stale.write_bytes(b"leave this untouched")
    subprocess.run([str(binary), str(missing)], check=True, capture_output=True)
    assert not missing.exists() and stale.read_bytes() == b"leave this untouched"
    result = subprocess.run([str(binary), "x" * 300], check=True, capture_output=True)
    assert b"Filename is too long" in result.stderr
print("PASS: shop conversion, backups, restoration, long/spaced filenames and failed backup handling.")
