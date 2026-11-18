#!/usr/bin/env python3
"""Exercise the production Study review without committing character changes."""
from pathlib import Path
import os
import subprocess
import tempfile
root = Path(__file__).resolve().parents[1]
source = (root / 'src/study.c').read_text()
def function(name):
    start = source.index(name + '\n{')
    return source[start:source.index('\n}', start) + 2]
quit_case = source[source.index('  case STUDY_GEN_MAIN_MENU:'):]
assert quit_case.index('study_changes_summary(d);') < quit_case.index('Do you wish to save your changes?')
assert 'study_setting_values(ch, LEVELUP(ch)->study_initial_settings);' in source
headers = ['conf.h', 'sysdep.h', 'structs.h', 'utils.h', 'spells.h', 'comm.h', 'db.h',
           'oasis.h', 'feats.h', 'class.h', 'constants.h', 'assign_wpn_armor.h',
           'domains_schools.h', 'alchemy.h', 'evolutions.h', 'epic_feats.h', 'bloodlines.h']
code = '\n'.join(f'#include "{name}"' for name in headers) + r'''
#include <assert.h>
struct player_special_data dummy_mob;
struct feat_info feat_list[NUM_FEATS];
struct weapon_table weapon_list[NUM_WEAPON_TYPES];
struct class_table class_list[NUM_CLASSES];
struct spell_info_type spell_info[TOP_SPELL_DEFINE+1];
struct domain_info domain_list[NUM_DOMAINS];
struct evolution_info evolution_list[NUM_EVOLUTIONS];
struct char_data *mob_proto;
const char *const school_names[NUM_SCHOOLS+1] = {"None", "Abjuration"};
const char *ability_names[MAX_ABILITIES+1];
const char *languages[NUM_LANGUAGES];
const char *alchemical_discovery_names[NUM_ALC_DISCOVERIES];
const char *paladin_mercies[NUM_PALADIN_MERCIES];
const char *blackguard_cruelties[NUM_BLACKGUARD_CRUELTIES];
const char *draconic_heritage_names[NUM_DRACONIC_HERITAGE_TYPES+1];
const char *dragon_bond_types[10];
static char output[100000];
void basic_mud_log(const char *format, ...) { (void)format; }
size_t write_to_output(struct descriptor_data *d,const char *format,...) {
  va_list args;size_t used=strlen(output);(void)d;
  va_start(args,format);int count=vsnprintf(output+used,sizeof(output)-used,format,args);va_end(args);
  assert(count>=0 && (size_t)count<sizeof(output)-used);return count;
}
mob_rnum real_mobile(mob_vnum vnum) { (void)vnum;return NOBODY; }
int feat_to_cfeat(int feat) { (void)feat;return -1; }
int feat_to_sfeat(int feat) { (void)feat;return -1; }
int feat_to_skfeat(int feat) { (void)feat;return -1; }
int has_feat_requirement_check(struct char_data *ch,int feat) { (void)ch;(void)feat;return 0; }
const char *bloodline_element_name(int element) { (void)element;return "Fire"; }
'''
for name in ['static const char *loremaster_secret_name(int secret)',
             'static void study_setting_values(struct char_data *ch, int *values)',
             'static const char *study_setting_name(int field, int value)',
             'static void study_changes_summary(struct descriptor_data *d)']:
    code += '\n' + function(name)
code += r'''
int main(void) {
  struct char_data pc={0};struct player_special_data player={0};
  struct level_data pending={0}, original;
  struct descriptor_data descriptor={0};
  pc.player_specials=&player;player.levelup=&pending;descriptor.character=&pc;
  pending.class=CLASS_WARRIOR;
  GET_REAL_STR(&pc)=GET_REAL_DEX(&pc)=GET_REAL_CON(&pc)=10;
  GET_REAL_INT(&pc)=GET_REAL_WIS(&pc)=GET_REAL_CHA(&pc)=10;
  pending.str=pending.dex=pending.con=pending.inte=pending.wis=pending.cha=10;
  GET_PREFERRED_ARCANE(&pc)=GET_PREFERRED_DIVINE(&pc)=GET_PREFERRED_MARTIAL(&pc)=CLASS_UNDEFINED;
  study_setting_values(&pc,pending.study_initial_settings);
  study_changes_summary(&descriptor);
  assert(strstr(output,"No pending training changes"));
  output[0]=0;
  pending.str=12;pending.boosts[1]=1;
  pending.feats[FEAT_GREAT_INTELLIGENCE]=1;feat_list[FEAT_GREAT_INTELLIGENCE].name="great intelligence";
  pending.skills[START_GENERAL_ABILITIES]=2;ability_names[START_GENERAL_ABILITIES]="Acrobatics";
  pending.spells_learned[SPELL_MAGIC_MISSILE]=1;spell_info[SPELL_MAGIC_MISSILE].name="magic missile";
  pending.languages[1]=1;languages[1]="Elven";
  pending.epic_choices.discipline=6;
  pending.eidolon_evolutions[1]=1;evolution_list[1].name="Claws";
  original=pending;
  study_changes_summary(&descriptor);
  assert(strstr(output,"Strength: 10 -> 12"));
  assert(strstr(output,"Dexterity: 10 -> 11"));
  assert(strstr(output,"Intelligence: 10 -> 11"));
  assert(strstr(output,"Acrobatics: 0 -> 2"));
  assert(strstr(output,"Learn: magic missile"));
  assert(strstr(output,"Language: Elven"));
  assert(strstr(output,"Epic discipline: Telepathy"));
  assert(strstr(output,"Eidolon Claws: 0 -> 1"));
  assert(!strstr(output,"already applied"));
  assert(!memcmp(&pending,&original,sizeof(pending))); /* review is read-only */
  assert(GET_REAL_STR(&pc)==10 && !GET_ABILITY(&pc,START_GENERAL_ABILITIES));
  output[0]=0;GET_PREFERRED_MARTIAL(&pc)=CLASS_RANGER;class_list[CLASS_RANGER].name="Ranger";
  study_changes_summary(&descriptor);
  assert(strstr(output,"not undone by discarding training"));
  assert(strstr(output,"Preferred martial class: Not set -> Ranger"));
  puts("PASS: Study summary precedes save, lists net choices and suboptions, labels live settings and does not commit changes.");
}
'''
with tempfile.TemporaryDirectory(prefix='study-summary-') as directory:
    cfile=Path(directory)/'summary.c'; binary=Path(directory)/'summary'
    cfile.write_text(code)
    subprocess.run([os.environ.get('CC','cc'),'-std=gnu11','-g','-fsanitize=address,undefined',
                    '-fno-omit-frame-pointer','-I'+str(root/'src'),str(cfile),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
