#!/usr/bin/env python3
"""Release candidate provenance, checksum and private-file exclusion checks."""
from pathlib import Path
import hashlib
import importlib.util
import json
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('clan_package', ROOT/'scripts/package_clan_release.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
with tempfile.TemporaryDirectory(prefix='clan-package-') as folder:
    tmp = Path(folder)
    mud, web, validation = tmp/'mud', tmp/'web', tmp/'validation'
    for repo in [mud, web]:
        repo.mkdir()
        subprocess.run(['git', 'init', '-q', str(repo)], check=True)
        for name, data in [('src/sample.c', 'public'), ('.env', 'PRIVATE_CREDENTIAL'), ('lib/plrfiles/player', 'PRIVATE_PLAYER')]:
            p = repo/name
            p.parent.mkdir(parents=True, exist_ok=True)
            p.write_text(data)
        subprocess.run(['git', 'add', '.'], cwd=repo, check=True)
        subprocess.run(['git', '-c', 'user.name=Fixture', '-c', 'user.email=fixture@example.invalid', 'commit', '-qm', 'fixture'], cwd=repo, check=True)
        (repo/'.env').write_text('CHANGED_PRIVATE_CREDENTIAL')
        (repo/'lib/plrfiles/player').write_text('CHANGED_PRIVATE_PLAYER')
        (repo/'src/sample.c').write_text('changed public')
        (repo/'src/untracked.c').write_text('untracked public')
        (repo/'src/symlink.c').symlink_to(repo/'.env')
    (mud/'bin').mkdir()
    (mud/'bin/circle').write_bytes(b'fixture binary')
    for name in ['server/index.js', 'shared/clan-progress.js', 'client/index.html']:
        p = web/'dist'/name
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_text('built fixture')
    (web/'dist/.env').write_text('PRIVATE_DIST_CREDENTIAL')
    (web/'dist/leak').symlink_to(web/'.env')
    validation.mkdir()
    (validation/'validation.json').write_text(json.dumps(dict(passed=False)))
    module.ROOT = mud
    output = tmp/'candidate'
    sys.argv = ['package', '--web-root', str(web), '--validation', str(validation), '--output', str(output)]
    try:
        module.main()
        raise AssertionError('failed validation accepted')
    except SystemExit as e:
        assert e.code == 2 and not output.exists()
    (validation/'validation.json').write_text(json.dumps(dict(passed=True)))
    module.main()
    manifest = json.loads((output/'manifest.json').read_text())
    assert manifest['status'] == 'review_candidate_not_approved'
    assert not manifest['production_deployed'] and manifest['pilot_observation_review'] == 'pending'
    for name, digest in manifest['files'].items():
        p = output/name
        assert p.is_file() and hashlib.sha256(p.read_bytes()).hexdigest() == digest
        assert b'PRIVATE_' not in p.read_bytes(), name
    assert (output/'source/mud/src/untracked.c').is_file()
    assert not (output/'source/mud/src/symlink.c').exists()
    assert not (output/'source/mud/.env').exists()
    assert not (output/'source/mud/lib/plrfiles').exists()
    assert 'changed public' in (output/'mud-tracked.patch').read_text()
print('Release package: validation prerequisite, exact file checksums, both worktree identities, untracked source and private/symlink exclusion passed.')
