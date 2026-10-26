#!/usr/bin/env python3
"""Check advanced race data using the locally configured campaign."""
from pathlib import Path
import os
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='krynn-race-registration-') as directory:
    executable = Path(directory) / 'race-registration'
    subprocess.run([
        os.environ.get('CC', 'cc'), '-std=gnu99', '-ffunction-sections',
        '-fdata-sections', '-I' + str(root / 'src'),
        str(root / 'src/race.c'), str(root / 'tests/race_registration.c'),
        '-Wl,--gc-sections', '-o', str(executable),
    ], check=True)
    subprocess.run([str(executable)], check=True)
