#!/usr/bin/env python3
"""Static safety checks for the guarded Phase-16 staging artifacts."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

def need(path: Path, text: str) -> None:
    data = path.read_text()
    if text not in data:
        raise RuntimeError(f'{path.relative_to(ROOT)} is missing required guard: {text}')

def main() -> None:
    harness = ROOT / 'scripts/legendary_phase16_rehearsal.sh'
    diagnostics = ROOT / 'sql/20261026_legendary_phase16_diagnostics.sql'
    migration = ROOT / 'sql/20261026_legendary_mob_bosses.sql'
    for guard in ('LEGENDARY_PHASE16_STAGING', 'LEGENDARY_PHASE16_SANITIZED',
                  '*_stage|*_staging', 'mysqldump', 'legacy_boss_table'):
        need(harness, guard)
    for query in ('legendary_features', 'legendary_operations', 'legendary_awards',
                  'legendary_boss_registry'):
        need(diagnostics, query)
    need(migration, 'DROP TABLE IF EXISTS legendary_boss_registry')
    need(ROOT / 'sql/20261026_legendary_smith_specproc.sql', 'DROP TABLE IF EXISTS legendary_smiths')
    need(ROOT / 'src/spec_assign.c', '"Infusion Smith", infusion_smith')
    print('PASS: Phase-16 staging harness, diagnostics and boss-flag migration guards verified')

if __name__ == '__main__':
    main()
