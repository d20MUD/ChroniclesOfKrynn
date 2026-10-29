#!/usr/bin/env python3
"""List spell registrations, documentation coverage, and source references for review.

This is a review index, not a C parser or proof that all behavior has been found.
It also tracks the two nonstandard alchemist effect symbols at IDs 383 and 385,
and explicitly documented unregistered spell and psionic definitions.
Use --spell PSIONIC_DEADLY_FEAR to narrow the report, or --json for tooling.
"""
import argparse
import json
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]

def c_code(text):
    """Blank comments while preserving strings, offsets, and line numbers."""
    token = re.compile(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|//[^\n]*|/\*[\s\S]*?\*/')
    return token.sub(lambda match: ''.join('\n' if ch == '\n' else ' ' for ch in match[0])
                     if match[0].startswith(('//', '/*')) else match[0], text)

def report():
    registrations = {}
    for name in ('spell_parser.c', 'psionics.c'):
        text = c_code((ROOT/'src'/name).read_text())
        for match in re.finditer(r'\b(?:spello|psiono)\(\s*((?:SPELL_|PSIONIC_)\w+|ALC_DISC_AFFECT_PSYCHOKINETIC|PSYCHOKINETIC_FEAR)\s*,\s*"([^"\n]+)"', text):
            registrations[match[1]] = {'id': match[1], 'name': match[2],
                'registration': f'src/{name}:{text.count(chr(10), 0, match.start())+1}',
                'availability': 'internal effect (not player castable)' if match[2].startswith('!') else 'registered',
                'coverage': 'registration only', 'references': []}
    numbers = {symbol: int(number) for symbol, number in re.findall(
        r'^#define ((?:SPELL_|PSIONIC_)\w+|ALC_DISC_AFFECT_PSYCHOKINETIC|PSYCHOKINETIC_FEAR) (\d+)\b', (ROOT/'src/spells.h').read_text(), re.M)}
    rules = (ROOT/'src/spellhelp.c').read_text()
    for symbol, name in re.findall(r'case ((?:SPELL_|PSIONIC_)\w+): return "([^"]+)";', rules):
        if symbol not in registrations:
            registrations[symbol] = {'id': symbol, 'name': name, 'registration': None,
                'coverage': 'registration only', 'references': [], 'availability': 'unregistered legacy ID'}
        elif registrations[symbol]['name'].startswith('!'):
            registrations[symbol]['registered_name'] = registrations[symbol]['name']
            registrations[symbol]['name'] = name
    for symbol, entry in registrations.items():
        entry['number'] = numbers.get(symbol)
        entry.setdefault('availability', 'registered')
    for symbol, body in re.findall(r'^  \{((?:SPELL_|PSIONIC_)\w+|ALC_DISC_AFFECT_PSYCHOKINETIC|PSYCHOKINETIC_FEAR),([\s\S]*?)(?=^  \{|^};)', rules, re.M):
        if symbol in registrations:
            registrations[symbol]['coverage'] = 'core entry; full interaction audit pending'
            registrations[symbol]['review_sources'] = sorted(set(re.findall(r'src/[a-zA-Z0-9_.]+\.(?:c|h|cpp)', body)))
    token = re.compile(r'\b(?:(?:SPELL_|PSIONIC_)\w+|ALC_DISC_AFFECT_PSYCHOKINETIC|PSYCHOKINETIC_FEAR)\b')
    for path in sorted((ROOT/'src').glob('*')):
        if path.suffix not in ('.c', '.h', '.cpp'):
            continue
        for number, line in enumerate(path.read_text(errors='replace').splitlines(), 1):
            for symbol in set(token.findall(line)) & registrations.keys():
                registrations[symbol]['references'].append({
                    'location': f'{path.relative_to(ROOT)}:{number}', 'text': line.strip()})
    return sorted(registrations.values(), key=lambda item: item['id'])

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--spell', help='exact symbolic spell ID')
    parser.add_argument('--json', action='store_true')
    parser.add_argument('--range', nargs=2, type=int, metavar=('FIRST', 'LAST'), help='inclusive numeric ID range')
    parser.add_argument('--since', metavar='REV', help='flag entries referencing files changed since a git revision')
    args = parser.parse_args()
    entries = report()
    if args.since:
        changed = set(subprocess.check_output(['git', 'diff', '--name-only', args.since, '--', 'src'], cwd=ROOT, text=True).splitlines())
        for entry in entries:
            entry['review_needed'] = bool(changed & (set(entry.get('review_sources', [])) | {ref['location'].split(':', 1)[0] for ref in entry['references']}))
    if args.range:
        first, last = args.range
        if first < 1 or last < first:
            parser.error('range must be positive and ascending')
        entries = [entry for entry in entries if entry.get('number') is not None and first <= entry['number'] <= last]
        entries.sort(key=lambda entry: entry['number'])
    if args.spell:
        entries = [entry for entry in entries if entry['id'] == args.spell]
        if not entries:
            parser.error('no literal registration found for that symbolic ID')
    if args.json:
        print(json.dumps(entries, indent=2))
    else:
        for entry in entries:
            print(f"{entry['id']} ({entry['name']}): {entry['coverage']}")
            print(f"  Registration: {entry['registration'] or entry['availability']}")
            if entry.get('review_sources'):
                print('  Reviewed handlers: ' + ', '.join(entry['review_sources']))
            if entry.get('review_needed'):
                print('  Review needed: a referenced source file changed.')
            for ref in entry['references']:
                print(f"  {ref['location']}: {ref['text']}")
    return 0

if __name__ == '__main__':
    raise SystemExit(main())
