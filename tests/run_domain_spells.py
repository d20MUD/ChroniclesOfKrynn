#!/usr/bin/env python3
"""Audit domain completeness and the native circles of the documented additions."""
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
source = (ROOT / 'src/domains_schools.c').read_text()
classes = (ROOT / 'src/class.c').read_text()
parser = (ROOT / 'src/spell_parser.c').read_text()
# Ignore disabled registrations when checking that additions are available.
classes = re.sub(r'/\*.*?\*/|//[^\n]*', '', classes, flags=re.S)
lists = {}
for name, body in re.findall(r'add_domain_spells\(DOMAIN_(\w+),(.*?)\);', source, re.S):
    spells = re.findall(r'SPELL_\w+', body)
    # Travel's ninth circle has campaign-specific alternatives, both of which
    # must remain valid spells; its first eight circles are shared.
    assert len(spells) == 9, (name, spells)
    assert 'SPELL_RESERVED_DBC' not in spells, name
    assert len(set(spells)) == len(spells), (name, 'duplicate spell')
    for spell in spells:
        assert re.search(r'\bspello\(\s*' + spell + r'\s*,', parser), (name, spell)
    lists[name] = spells
assert len(lists) == 15, len(lists)
count = 0
for domain, circle, name in re.findall(
        r'^\| (\w+) \| (\d+) \| ([a-z_ ]+) \|',
        (ROOT / 'docs/CLERIC_DOMAIN_SPELLS.md').read_text(), re.M):
    spell = 'SPELL_' + name.upper().replace(' ', '_')
    circle = int(circle)
    assert lists[domain.upper()][circle - 1] == spell
    level = circle * 2 - 1
    assert re.search(r'spell_assignment\(CLASS_(?:CLERIC|WIZARD|DRUID),\s*'
                     + spell + r',\s*' + str(level) + r'\s*\)', classes), (domain, circle, spell)
    count += 1
assert count == 34, count
# Travel's alternate ninth-circle selection also needs a live registration.
assert 'SPELL_GATE' in source
assert re.search(r'\bspello\(SPELL_GATE,', parser)
print('PASS: all 15 domains have nine registered spells; all 34 additions match native circles.')
