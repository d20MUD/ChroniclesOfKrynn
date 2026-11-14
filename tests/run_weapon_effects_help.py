#!/usr/bin/env python3
"""Require complete WEAPON_SPECAB coverage and repeatable, matching SQL help."""
from clan_database import ROOT,database
import re

header=(ROOT/'src/spec_abilities.h').read_text()
effects=re.findall(r'^#define WEAPON_SPECAB_(\w+)\b',header,re.M)
assert len(effects)==40
body=(ROOT/'docs/help/weapon_effects.hlp').read_text()
for effect in effects:
 name=effect.replace('_',' ')
 assert re.search(r'^'+re.escape(name)+r'(?: --[^\n]*)?$',body,re.M),name
assert (ROOT/'lib/text/help/weapon_effects.hlp').read_text()==body
for required in ['BRILLIANT ENERGY -- NOT IMPLEMENTED','DISTANCE -- NOT IMPLEMENTED',
                 'MERCIFUL -- NOT IMPLEMENTED','SPELL STORING -- NOT IMPLEMENTED',
                 'WOUNDING -- INACTIVE UNDER STANDARD SETTINGS','5%','1.5 times','10%']:
 assert required in body,required
with database(()) as (_,_,sql,_):
 sql('CREATE TABLE help_entries(tag VARCHAR(80) PRIMARY KEY,category VARCHAR(40),entry LONGTEXT,min_level INT,max_level INT,auto_generated BOOL);CREATE TABLE help_keywords(help_tag VARCHAR(80),keyword VARCHAR(80),PRIMARY KEY(help_tag,keyword))')
 migration=(ROOT/'sql/20261113_weapon_effects_help.sql').read_text()
 sql(migration);first=sql('SELECT * FROM help_entries;SELECT * FROM help_keywords')
 sql(migration);assert sql('SELECT * FROM help_entries;SELECT * FROM help_keywords')==first
 expected=body.removesuffix('#0\n').rstrip().replace('\n','\r\n')
 assert bytes.fromhex(sql("SELECT HEX(entry) FROM help_entries WHERE tag='WEAPONEFFECTS'")).decode()==expected
 assert sql("SELECT COUNT(*) FROM help_keywords WHERE help_tag='WEAPONEFFECTS'")=='6'
 for name in ['20261112_clan_quartermaster_help.sql','20261112_clan_progression_services_help.sql']:
  sql((ROOT/'sql'/name).read_text())
  expected=(ROOT/'docs/help/clan_quartermaster.hlp').read_text().removesuffix('#0\n').rstrip().replace('\n','\r\n')
  assert bytes.fromhex(sql("SELECT HEX(entry) FROM help_entries WHERE tag='CLANQUARTERMASTER'")).decode()==expected
print('All 40 weapon abilities documented; limitations covered; SQL publication is repeatable and matches both help files.')
