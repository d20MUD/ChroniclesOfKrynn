#!/usr/bin/env python3
"""Compile actual clan configuration/rules in three campaign modes, offline."""
from pathlib import Path
import os
import shlex
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='clan-progression-config-') as directory:
    # Bypass this checkout's local campaign selection and exercise all supported
    # build guards without editing campaign.h or contacting any live service.
    for name, defines in [('dragonlance', ['-DCAMPAIGN_DL', '-DCLAN_TEST_DRAGONLANCE']),
                          ('default', []), ('forgotten-realms', ['-DCAMPAIGN_FR'])]:
        binary = Path(directory) / name
        subprocess.run([os.environ.get('CC', 'cc'), '-std=c99', '-Wall', '-Wextra', '-Werror',
                        '-pedantic', '-fsanitize=address,undefined', '-fno-pie', '-no-pie',
                        *shlex.split(os.environ.get('CLAN_PROGRESSION_TEST_CFLAGS', '')),
                        '-D_CAMPAIGN_H_', *defines, '-I' + str(root / 'src'),
                        str(root / 'src/clan_progression.c'),
                        str(root / 'tests/clan_progression_config.c'), '-o', str(binary)], check=True)
        subprocess.run([str(binary)], check=True)
        print('Campaign checked:', name)
