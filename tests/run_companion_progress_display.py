#!/usr/bin/env python3
"""Check owner-visible companion advancement and group TNL source selection."""
from pathlib import Path
import os
import subprocess
import tempfile
root = Path(__file__).resolve().parents[1]
mobile = (root / 'src/mob_utils.c').read_text()
other = (root / 'src/act.other.c').read_text()
def function(text, signature):
    a = text.index(signature + '\n{')
    return text[a:text.index('\n}', a) + 2] + '\n'
headers = ''.join('#include "' + h + '"\n' for h in [
    'conf.h', 'sysdep.h', 'structs.h', 'utils.h', 'spells.h', 'mud_event.h', 'mob_utils.h', 'class.h'])
code = r'''
#include <assert.h>
struct player_special_data dummy_mob;
static int temporary;
int MIN(int a, int b) { return a < b ? a : b; }
int MAX(int a, int b) { return a > b ? a : b; }
struct mud_event_data *char_has_mud_event(struct char_data *ch, event_id id) {
  static struct mud_event_data event; return temporary && id == ePURGEMOB ? &event : NULL;
}
long int level_exp(struct char_data *ch, int level) { assert(!IS_NPC(ch)); return 10000; }
'''
for signature in [
    'int purchased_pet_level_cost(struct char_data *pet)',
    'int companion_training_kind(struct char_data *pet)',
    'bool is_persistent_pet(struct char_data *pet)',
    'bool format_companion_tnl(struct char_data *owner, struct char_data *pet, char *buf, size_t size)',
]:
    code += function(mobile, signature)
a = other.index('    if (!IS_NPC(k))', other.index('static void print_group(struct char_data *ch)\n{'))
b = other.index('    send_to_char(', a)
code += 'static void group_tnl(struct char_data *ch, struct char_data *k, char *out) { char tnl[64];\n' + other[a:b] + 'strcpy(out, tnl); }\n'
code += r'''
int main(void) {
  struct char_data owner = {0}, stranger = {0}, pet = {0};
  struct player_special_data ps = {0};
  char text[64], small[4];
  owner.player_specials = &ps; stranger.player_specials = &ps;
  GET_LEVEL(&owner) = 20; GET_LEVEL(&pet) = 4;
  SET_BIT_AR(MOB_FLAGS(&pet), MOB_ISNPC); SET_BIT_AR(AFF_FLAGS(&pet), AFF_CHARM);
  pet.master = &owner; pet.mob_specials.purchased_pet = true;
  pet.mob_specials.pet_experience = 2500; GET_EXP(&pet) = 987654321;
  group_tnl(&owner, &pet, text); assert(!strcmp(text, "7500 TNL"));
  group_tnl(&stranger, &pet, text); assert(!strcmp(text, "-- TNL"));
  group_tnl(&owner, &owner, text); assert(!strcmp(text, "10000 TNL"));
  /* Actual advancement XP, never the NPC's kill bounty, determines progress. */
  GET_EXP(&pet) = 0;
  group_tnl(&owner, &pet, text); assert(!strcmp(text, "7500 TNL"));
  pet.mob_specials.pet_experience = 11000;
  group_tnl(&owner, &pet, text); assert(!strcmp(text, "0 TNL"));
  pet.mob_specials.pet_experience = -100;
  group_tnl(&owner, &pet, text); assert(!strcmp(text, "10000 TNL"));
  GET_LEVEL(&pet) = 20;
  group_tnl(&owner, &pet, text); assert(!strcmp(text, "Level cap"));
  GET_LEVEL(&owner) = 35; GET_LEVEL(&pet) = 30;
  group_tnl(&owner, &pet, text); assert(!strcmp(text, "Level cap"));
  GET_LEVEL(&pet) = 4; pet.mob_specials.purchased_pet = false;
  SET_BIT_AR(MOB_FLAGS(&pet), MOB_C_ANIMAL);
  pet.mob_specials.pet_training_rank = 2; pet.mob_specials.pet_training_experience = 1200;
  group_tnl(&owner, &pet, text); assert(!strcmp(text, "4800 Training TNL"));
  pet.mob_specials.pet_training_rank = PET_TRAINING_RANK_CAP;
  group_tnl(&owner, &pet, text); assert(!strcmp(text, "Training cap"));
  temporary = true;
  group_tnl(&owner, &pet, text); assert(!strcmp(text, "-- TNL"));
  temporary = false; REMOVE_BIT_AR(MOB_FLAGS(&pet), MOB_C_ANIMAL);
  group_tnl(&owner, &pet, text); assert(!strcmp(text, "-- TNL"));
  pet.mob_specials.purchased_pet = true;
  assert(format_companion_tnl(&owner, &pet, small, sizeof(small)) && small[3] == '\0');
  assert(!format_companion_tnl(NULL, &pet, text, sizeof(text)) && !*text);
  assert(!format_companion_tnl(&owner, NULL, text, sizeof(text)));
  assert(!format_companion_tnl(&owner, &pet, NULL, 64));
  assert(!format_companion_tnl(&owner, &pet, text, 0));
  return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='companion-display-') as tmp:
    path = Path(tmp) / 'check.c'; path.write_text(headers + code)
    binary = Path(tmp) / 'check'
    subprocess.run([os.environ.get('CC', 'cc'), '-std=gnu99', '-Wall', '-Wextra',
                    '-Wno-unused-parameter', '-I' + str(root / 'src'), str(path),
                    '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print('Companion display: pet XP, training XP, owner-only group TNL, caps, temporary summons, player TNL, and safe formatting passed.')
