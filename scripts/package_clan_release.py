#!/usr/bin/env python3
"""Create a local, disabled release-review candidate; never install or deploy.

Includes public source/configuration, SQL, catalogs, built artifacts and validation.
Player records, credentials, backups, node_modules and running-service state are excluded.
"""
import argparse
from datetime import datetime, timezone
import hashlib
import json
import platform
import re
from pathlib import Path
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def version(root):
    return subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=root, text=True).strip()


def public_sources(root, web=False):
    paths = subprocess.check_output(['git', 'ls-files', '--cached', '--others', '--exclude-standard', '-z'], cwd=root).decode().split('\0')
    if web:
        roots = ('src/', 'server/', 'shared/', 'public/', 'tests/', 'docs/')
        files = {'package.json', 'package-lock.json', 'index.html', 'eslint.config.js', 'vite.config.ts', 'tsconfig.json', 'tsconfig.app.json', 'tsconfig.server.json', 'ecosystem.config.cjs', 'readme.md'}
    else:
        roots = ('src/', 'lib/clan/', 'sql/', 'scripts/', 'tools/', 'tests/', 'docs/clan_progression', 'docs/help/clan_')
        files = {'CMakeLists.txt', 'Makefile.am', 'configure.ac', 'docs/help/weapon_effects.hlp', 'docs/weapon_effects_mechanics_review.md'}
        # These local build inputs are intentionally ignored by git, not secrets.
        paths += ['src/campaign.h', 'src/mud_options.h', 'src/vnums.h', 'src/conf.h']
    for name in sorted(set(paths)):
        p = root / name
        if not name or not (name in files or name.startswith(roots)):
            continue
        if p.is_symlink() or not p.is_file() or any(part.startswith('.env') and part != '.env_example' for part in p.parts):
            continue
        if '__pycache__' in p.parts or p.suffix in {'.o', '.pyc', '.log', '.so', '.a'}:
            continue
        yield name, p


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--web-root', type=Path, required=True)
    parser.add_argument('--validation', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    report = json.loads((args.validation / 'validation.json').read_text())
    if not report.get('passed'):
        parser.error('Validation must pass before packaging a release candidate')
    web = args.web_root.resolve()
    if not (ROOT/'bin/circle').is_file() or not (web/'dist/server/index.js').is_file() or not (web/'dist/client/index.html').is_file():
        parser.error('Build both server and gateway/browser first')
    out = args.output.resolve()
    if out.is_relative_to(ROOT) or out.is_relative_to(web):
        parser.error('Choose an output directory outside both source repositories')
    out.mkdir(parents=True, exist_ok=False)
    constants = (ROOT/'src/constants.c').read_text() if (ROOT/'src/constants.c').exists() else ''
    display = re.search(r'luminari_version = "([^"]+)"', constants)
    manifest = dict(version=1, created_at=datetime.now(timezone.utc).isoformat(),
        status='review_candidate_not_approved', production_deployed=False,
        pilot_observation_review='pending', feature_defaults='disabled',
        mud_base_commit=version(ROOT), web_base_commit=version(web),
        platform=dict(system=platform.system(), machine=platform.machine()),
        web_package_version=json.loads((web/'package.json').read_text()).get('version') if (web/'package.json').exists() else 'fixture',
        mud_display_version=display.group(1) if display else 'fixture',
        protocol_version=1, rules_version=2, clan_core_schema=6, files={})
    for label, root, is_web in [('mud', ROOT, False), ('web', web, True)]:
        selected = list(public_sources(root, is_web))
        for name, source in selected:
            target = out/'source'/label/name
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(source, target)
        # This captures reviewed worktree deltas, including untracked source above.
        (out/f'{label}-tracked.patch').write_bytes(subprocess.check_output(['git', 'diff', '--binary', 'HEAD', '--', *[name for name, _ in selected]], cwd=root))
    (out/'artifacts').mkdir()
    shutil.copy2(ROOT/'bin/circle', out/'artifacts/circle')
    for source in sorted((web/'dist').rglob('*')):
        relative = source.relative_to(web/'dist')
        if not source.is_file() or source.is_symlink() or any(part.startswith('.env') for part in relative.parts):
            continue
        target = out/'artifacts/web-dist'/relative
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, target)
    shutil.copytree(args.validation, out/'validation')
    (out/'README.txt').write_text(
        'CLAN PROGRESSION: REVIEW CANDIDATE ONLY\n'
        'Not approved, not deployed, features remain disabled.\n'
        'Read source/mud/docs/clan_progression_release_checklist.md.\n'
        'Phase 51 needs real controlled pilot observations and a reviewed bootstrap.\n'
        'Source overlays and patches identify both dirty worktrees against base commits.\n'
        'This excludes world/player/inventory data, credentials and operational backups.\n'
        'Do not use this package as a player-data backup or execute scripts/deploy.sh.\n')
    for p in sorted(out.rglob('*')):
        if p.is_file():
            manifest['files'][str(p.relative_to(out))] = sha(p)
    source_files = {k:v for k,v in manifest['files'].items() if k.startswith('source/')}
    manifest['source_sha256'] = hashlib.sha256(json.dumps(source_files, sort_keys=True).encode()).hexdigest()
    (out/'manifest.json').write_text(json.dumps(manifest, indent=2)+'\n')
    print(json.dumps(dict(output=str(out), status=manifest['status'], source_sha256=manifest['source_sha256'], files=len(manifest['files']))))


if __name__ == '__main__':
    main()
