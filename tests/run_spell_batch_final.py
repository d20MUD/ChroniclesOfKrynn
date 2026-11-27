#!/usr/bin/env python3
"""Run the production final spell batch with controlled native engine services."""
from pathlib import Path
import re, subprocess, tempfile
root=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='spell-final-') as directory:
    binary=Path(directory)/'test'
    fight=(root/'src/fight.c').read_text()
    blocks=[]
    for signature in ['bool weapon_bypasses_dr(', 'static int apply_damage_reduction_epic(']:
        start=fight.index(signature)
        # Skip the forward declaration of the private implementation.
        if signature.startswith('static'):
            start=fight.index(signature,start+len(signature))
        end=fight.index('\n}',start)+2
        blocks.append(fight[start:end])
    native=Path(directory)/'native_dr.c'
    native.write_text(''.join('#include "'+h+'"\n' for h in [
        'conf.h','sysdep.h','structs.h','utils.h','comm.h','spells.h','fight.h','handler.h',
        'assign_wpn_armor.h','evolutions.h','perks.h','epic_feats.h','religion.h',
        'bloodlines.h','spell_batch_final.h'])+'\n'+ '\n'.join(blocks)+
        '\nint test_native_dr(struct char_data *ch,struct char_data *victim,struct obj_data *weapon,int damage,int type){return apply_damage_reduction_epic(ch,victim,weapon,damage,false,type,NULL); }\n')
    subprocess.run(['cc' ,'-std=gnu99','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer',
        '-fno-pie','-no-pie','-I'+str(root/'src'),str(root/'tests/spell_batch_final.c'),
        str(root/'src/spell_batch_final.c'),str(native),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
classes=(root/'src/class.c').read_text()
expected={
'SPELL_DEFENDING_BONE':{'CLERIC':3,'INQUISITOR':4,'WIZARD':3,'SORCERER':4},
'SPELL_RIGHTEOUS_MIGHT':{'CLERIC':9,'INQUISITOR':13},'SPELL_THORN_BODY':{'ALCHEMIST':7,'DRUID':7},
'SPELL_VINE_STRIKE':{'ALCHEMIST':4,'BLACKGUARD':10,'DRUID':3,'RANGER':10,'WIZARD':3,'SORCERER':4},
'SPELL_DAZZLING_BLADE':{'BARD':1,'WIZARD':1,'SORCERER':1},'SPELL_DEBILITATING_PORTENT':{'CLERIC':7},
'SPELL_BLESSING_OF_FERVOR':{'CLERIC':7},'PSIONIC_CRYSTALLINE_BONDS':{'PSIONICIST':7},'PSIONIC_BATTLESENSE':{'PSIONICIST':5}}
for symbol,want in expected.items():
    got={c:int(n) for c,n in re.findall(r'spell_assignment\(CLASS_(\w+),\s*'+symbol+r',\s*(\d+)\)',classes)}
    assert got==want,(symbol,got,want)
print('PASS: exact class access for the remaining seven spells and two powers')
