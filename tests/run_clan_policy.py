#!/usr/bin/env python3
"""Production startup loader: current policy remains explicitly server-owned."""
from pathlib import Path
import json,shlex,subprocess,tempfile
root=Path(__file__).resolve().parents[1]
flags=shlex.split(subprocess.check_output(['pkg-config','--cflags','--libs','json-c'],text=True))
with tempfile.TemporaryDirectory(prefix='clan-policy-') as directory:
    binary=Path(directory)/'policy'
    subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-pedantic','-fsanitize=address,undefined','-fno-pie','-no-pie','-D_CAMPAIGN_H_','-DCAMPAIGN_DL','-I'+str(root/'src'),str(root/'tests/clan_policy.c'),str(root/'src/clan_policy.c'),str(root/'src/clan_progression.c'),*flags,'-o',str(binary)],check=True)
    subprocess.run([str(binary),directory],check=True)
default=json.loads((root/'lib/clan/progression.json').read_text())
assert default==dict(version=1,mode='off',buffs=False,contracts=False,expeditions=False,quartermaster=False,web=False,public_release=False,pilot_clans=[])
db=(root/'src/db.c').read_text()
assert db.index('clan_persistence_boot_environment()')<db.index('clan_policy_load(')<db.index('clan_expedition_restore((int64_t)time(NULL))')
print('Startup ordering: checked authority before policy, policy before recovery; shipped policy OFF.')
