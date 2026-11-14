#!/usr/bin/env python3
"""Regression: a missing I3 config must not crash the game heartbeat."""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix="i3-disabled-") as directory:
    binary = Path(directory) / "check"
    subprocess.run([
        "cc", "-std=gnu90", "-g", "-Wall", "-Wextra",
        "-ffunction-sections", "-fdata-sections",
        "-fsanitize=address,undefined", "-fno-pie", "-no-pie",
        "-I" + str(ROOT / "src"),
        str(ROOT / "tests/i3_disabled.c"),
        str(ROOT / "src/systems/intermud3/i3_client.c"),
        "-Wl,--gc-sections", "-ljson-c", "-lpthread", "-o", str(binary)
    ], check=True)
    subprocess.run([str(binary)], cwd=directory, check=True)
print("I3 disabled initialization/heartbeat and initialized FIFO regression passed.")
