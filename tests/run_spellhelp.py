#!/usr/bin/env python3
"""Compile the real spellhelp command/renderer with controlled registrations."""
from pathlib import Path
import argparse
import json
import os
import re
import runpy
import subprocess
import tempfile
root = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--sanitize', action='store_true')
args = parser.parse_args()
extra = ['-fsanitize=address,undefined', '-fno-omit-frame-pointer'] if args.sanitize else []
# Compile the production initializer and actual batch registration calls, rather
# than supplying invented flags/names for the batch coverage check.
c_code = runpy.run_path(str(root/'tools/audit_spellhelp.py'))['c_code']
source = c_code((root/'src/spell_parser.c').read_text())
numbers = {name:int(number) for name,number in re.findall(
    r'^#define ((?:SPELL_\w+|ALC_DISC_AFFECT_PSYCHOKINETIC|PSYCHOKINETIC_FEAR)) (\d+)\b', (root/'src/spells.h').read_text(), re.M)}
spello = re.search(r'^void spello\([^;]*?\)\n\{.*?^}', source, re.M|re.S).group()
calls = [(match[1],match[0]) for match in re.finditer(
    r'\bspello\(\s*((?:SPELL_\w+|ALC_DISC_AFFECT_PSYCHOKINETIC|PSYCHOKINETIC_FEAR))\s*,.*?\);', source, re.S)
    if 1 <= numbers.get(match[1],0) <= 550]
assert len(calls)==463
headers = ''.join('#include "'+header+'"\n' for header in
    ['conf.h','sysdep.h','structs.h','utils.h','spells.h','domains_schools.h','psionics.h'])
production = headers + '\nconst char *unused_wearoff="!UNUSED WEAROFF!";\n' + spello
production += '''
void load_batch_registration(void) {
  int i;
  for(i=1;i<=550;i++) {
    memset(&spell_info[i],0,sizeof(spell_info[i]));
    spell_info[i].name="!UNUSED!";
  }
'''
# Plane Shift's production registration is conditional on campaign selection.
production += '\n'.join(
    '#if !defined(CAMPAIGN_DL) && !defined(CAMPAIGN_FR)\n'+call+'\n#endif'
    if symbol == 'SPELL_PLANE_SHIFT' else
    '#ifdef CAMPAIGN_FR\n'+call+'\nspell_info['+symbol+'].cant_cast = true;\nspell_info['+symbol+'].no_player = true;\n#endif'
    if symbol in {'SPELL_SILVERYMOON_RECALL','SPELL_TRIBOAR_RECALL','SPELL_LUSKAN_RECALL','SPELL_MIRABAR_RECALL'}
    else '#if defined(CAMPAIGN_DL) && !defined(CAMPAIGN_FR)\n'+call+'\nspell_info['+symbol+'].cant_cast = true;\nspell_info['+symbol+'].no_player = true;\n#endif'
    if symbol in {'SPELL_PALANTHAS_RECALL','SPELL_SANCTION_RECALL','SPELL_SOLACE_RECALL'}
    else call+'\nspell_info['+symbol+'].cant_cast = true;\nspell_info['+symbol+'].no_player = true;'
    if symbol in {'SPELL_MINOR_RAPID_BUFF','SPELL_RAPID_BUFF','SPELL_GREATER_RAPID_BUFF'}
    else call for symbol,call in calls)
for match in re.finditer(r'spell_info\[((?:SPELL_\w+|ALC_DISC_AFFECT_PSYCHOKINETIC|PSYCHOKINETIC_FEAR))\]\.(?:touch_spell|is_cantrip|no_player|cant_cast|ritual_spell)\s*=\s*(?:TRUE|FALSE|true|false)\s*;', source):
    if 1 <= numbers.get(match[1],0) <= 550:
        production += '\n' + match[0]
production += '''
}
void free_batch_registration(void) {
  int i;
  for(i=1;i<=550;i++) {
    free((char *)spell_info[i].wear_off_msg);
    spell_info[i].wear_off_msg=NULL;
  }
}
'''
# Compile actual psiono() and all active literal registrations too.
psi_source = c_code((root/'src/psionics.c').read_text())
psi_calls = re.findall(r'\bpsiono\(\s*PSIONIC_\w+\s*,.*?\);', psi_source, re.S)
assert len(psi_calls) == 87
for function in ('unused_psionic_power', 'psiono', 'epic_psionic_power'):
    production += '\n' + re.search(r'^void '+function+r'\([^;]*?\)\n\{.*?^}', psi_source, re.M|re.S).group()
production += """
void load_psionic_registration(void) {
  int i;
  for(i=PSIONIC_POWER_START;i<=PSIONIC_POWER_END;i++) {
    memset(&spell_info[i],0,sizeof(spell_info[i]));
    memset(&psionic_powers[i],0,sizeof(psionic_powers[i]));
    spell_info[i].name="!UNUSED!";
    unused_psionic_power(i);
  }
"""
production += '\n'.join(psi_calls)
production += '\n'.join(re.findall(r'\bepic_psionic_power\(PSIONIC_\w+\);', psi_source))
production += """
}
void free_psionic_registration(void) {
  int i;
  for(i=PSIONIC_POWER_START;i<=PSIONIC_POWER_END;i++) {
    free((char *)spell_info[i].wear_off_msg);
    spell_info[i].wear_off_msg=NULL;
  }
}
"""
with tempfile.TemporaryDirectory(prefix='spellhelp-') as tmp:
    binary = Path(tmp)/'spellhelp'
    registrations = Path(tmp)/'registrations.c'
    registrations.write_text(production)
    subprocess.run([os.environ.get('CC','cc'), '-std=gnu89', '-Wall', '-Wextra', '-Wno-unused-parameter',
        *extra, '-I'+str(root/'src'), str(root/'src/spellhelp.c'), str(root/'tests/spellhelp.c'), str(registrations),
        '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)

entries = json.loads(subprocess.check_output([
    'python3', str(root/'tools/audit_spellhelp.py'), '--json', '--since', 'HEAD'], text=True))
reviewed = [entry for entry in entries if entry['coverage'].startswith('core entry')]
assert len(reviewed) == 586
for entry in reviewed:
    assert entry['references'] and isinstance(entry['review_needed'], bool)
    assert set(entry['review_sources']) & {'src/magic.c', 'src/spells.c'}
batch = sorted((entry for entry in entries if entry.get('number') in range(1,51)), key=lambda entry:entry['number'])
assert [entry['number'] for entry in batch] == list(range(1,51))
assert all(entry['coverage'].startswith('core entry') for entry in batch)
assert [entry['number'] for entry in batch if entry['availability'] != 'registered'] == [1,41,47,49]
for entry in reviewed:
    for path in entry['review_sources']:
        assert (root/path).is_file(), path
second = sorted((entry for entry in entries if entry.get('number') in range(51,101)), key=lambda entry:entry['number'])
assert [entry['number'] for entry in second] == list(range(51,101))
assert all(entry['coverage'].startswith('core entry') for entry in second)
assert [entry['number'] for entry in second if entry['availability'] != 'registered'] == [76]
assert next(entry for entry in second if entry['number']==52)['name']=='identify'
assert c_code('/* spello(fake); */\n"// literal" // ignore\n').count('\n')==2
third = sorted((entry for entry in entries if entry.get('number') in range(101,151)), key=lambda entry:entry['number'])
assert [entry['number'] for entry in third] == list(range(101,151))
assert all(entry['coverage'].startswith('core entry') for entry in third)
assert [entry['number'] for entry in third if entry['availability'] != 'registered'] == [142,143,144]
fourth = sorted((entry for entry in entries if entry.get('number') in range(151,201)), key=lambda entry:entry['number'])
assert [entry['number'] for entry in fourth] == list(range(151,201))
assert all(entry['coverage'].startswith('core entry') for entry in fourth)
assert [entry['number'] for entry in fourth if entry['availability'] != 'registered'] == [161]
fifth = sorted((entry for entry in entries if entry.get('number') in range(201,251)), key=lambda entry:entry['number'])
assert [entry['number'] for entry in fifth] == list(range(201,251))
assert all(entry['coverage'].startswith('core entry') for entry in fifth)
assert [entry['number'] for entry in fifth if entry['availability'] != 'registered'] == [219,241,242,243]
sixth = sorted((entry for entry in entries if entry.get('number') in range(251,301)), key=lambda entry:entry['number'])
assert [entry['number'] for entry in sixth] == list(range(251,301))
assert all(entry['coverage'].startswith('core entry') for entry in sixth)
assert [entry['number'] for entry in sixth if entry['availability'] != 'registered'] == [271,274,275,277,294]
seventh = sorted((entry for entry in entries if entry.get('number') in range(301,351)), key=lambda entry:entry['number'])
assert [entry['number'] for entry in seventh] == list(range(301,325))
assert all(entry['coverage'].startswith('core entry') for entry in seventh)
assert [entry['number'] for entry in seventh if entry['availability'] != 'registered'] == [302,304,306,315,317,318,321]
assert not set(numbers.values()) & set(range(325,351))
eighth = sorted((entry for entry in entries if entry.get('number') in range(351,401)), key=lambda entry:entry['number'])
assert [entry['number'] for entry in eighth] == list(range(382,401))
assert all(entry['coverage'].startswith('core entry') for entry in eighth)
assert [entry['number'] for entry in eighth if entry['availability'] != 'registered'] == [382,387,388]
assert eighth[1]['id']=='ALC_DISC_AFFECT_PSYCHOKINETIC' and eighth[3]['id']=='PSYCHOKINETIC_FEAR'
assert not set(numbers.values()) & set(range(351,382))
ninth = sorted((entry for entry in entries if entry.get('number') in range(401,451)), key=lambda entry:entry['number'])
assert [entry['number'] for entry in ninth] == list(range(401,451))
assert all(entry['coverage'].startswith('core entry') for entry in ninth)
assert [entry['number'] for entry in ninth if entry['availability'] != 'registered'] == [420,441]
tenth = sorted((entry for entry in entries if entry.get('number') in range(451,501)), key=lambda entry:entry['number'])
assert [entry['number'] for entry in tenth] == [n for n in range(451,501) if n not in (485,490)]
assert all(entry['coverage'].startswith('core entry') for entry in tenth)
assert [entry['number'] for entry in tenth if entry['availability'] != 'registered'] == [486]
eleventh = sorted((entry for entry in entries if entry.get('number') in range(501,551)), key=lambda entry:entry['number'])
assert [entry['number'] for entry in eleventh] == list(range(501,541))
assert all(entry['coverage'].startswith('core entry') for entry in eleventh)
assert not set(numbers.values()) & set(range(541,551))
powers = sorted((entry for entry in entries if entry.get('number') in range(1500,1605)), key=lambda entry:entry['number'])
assert [entry['number'] for entry in powers] == list(range(1500,1605))
assert all(entry['coverage'].startswith('core entry') for entry in powers)
assert sum(entry['availability'] == 'registered' for entry in powers) == 87
print('Spellhelp audit index: 586 records; ordinary spells complete; all 87 registered powers and 18 retired/perk IDs reviewed.')
