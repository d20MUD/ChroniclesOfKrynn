#!/usr/bin/env python3
"""Phase 38: indexed-world roster audit and real loaded-world availability adapter."""
from pathlib import Path
import json,subprocess,tempfile,shlex,sys
ROOT=Path(__file__).resolve().parents[1]
sys.dont_write_bytecode=True
sys.path.insert(0,str(ROOT/'scripts'))
from validate_clan_expedition_roster import audit
catalog=json.loads((ROOT/'lib/clan/expeditions.json').read_text())
report=audit(catalog)
assert len(report)==10
assert not any(d['id'] in ('icewall-assault-v1','icewall-hunt-v1') for d in report)
assert all(len(d['objectives'])==3 and all(o['alternatives']>=2 for o in d['objectives']) for d in report)
for d in report:
    assert max(o['reset_wait_minutes'] for o in d['objectives'])<=45
    assert sum(d['route_hops'])<=110
flags=shlex.split(subprocess.check_output(['pkg-config','--cflags','--libs','json-c'],text=True))
with tempfile.TemporaryDirectory(prefix='expedition-world-')as tmp:
    binary=Path(tmp)/'world'
    subprocess.run(['cc','-std=gnu99','-Wall','-Wextra','-Werror','-Wno-missing-braces','-fsanitize=address,undefined','-fno-pie','-no-pie','-Isrc','tests/clan_expedition_world.c','src/clan_expeditions.c',*flags,'-o',str(binary)],cwd=ROOT,check=True)
    subprocess.run([str(binary)],check=True)
print('Phase 38 staging: ten choices across levels 5-30, initial clan unlocks, bounded real respawns, bidirectional walking routes and reachable fallback targets passed.')
