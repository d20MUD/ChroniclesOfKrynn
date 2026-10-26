#!/usr/bin/env python3
"""Static safety checks for Phase-17 aggregate observation assets."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

def need(path: Path, text: str) -> None:
    if text not in path.read_text():
        raise RuntimeError(f'{path.relative_to(ROOT)} is missing {text!r}')

def main() -> None:
    report = ROOT / 'scripts/legendary_phase17_report.sh'
    query = ROOT / 'sql/20261026_legendary_phase17_observability.sql'
    for marker in ('LEGENDARY_PHASE17_READONLY', 'SELECT DATABASE()', 'mysql', 'report='):
        need(report, marker)
    for marker in ('daily-awards-last-7-utc-days', 'active-loadouts',
                   'two-handed-investment-proxy', 'major-power-usage-last-24-hours',
                   'rolling-effective-benefit-proxy'):
        need(query, marker)
    forbidden = ('INSERT ', 'UPDATE ', 'DELETE ', 'DROP ', 'ALTER ', 'CREATE ')
    data = query.read_text().upper()
    for word in forbidden:
        if word in data:
            raise RuntimeError(f'observation SQL must be read-only; found {word.strip()}')
    print('PASS: Phase-17 aggregate report is guarded and read-only')

if __name__ == '__main__':
    main()
