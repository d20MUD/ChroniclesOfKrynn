#!/usr/bin/env python3
"""Check natural swimmers, ordinary swim rolls, and boat-only restrictions."""
from pathlib import Path
import os
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'src/movement_validation.c').read_text()
start = source.index('static bool is_natural_swimmer(')
end = source.index('/* Simple function to determine if char can fly.', start)
code = source[start:end]
headers = '\n'.join('#include "' + h + '"' for h in
                    ['conf.h', 'sysdep.h', 'structs.h', 'utils.h', 'spells.h', 'db.h', 'comm.h'])
stubs = r'''
#include <assert.h>
struct room_data *world;
room_rnum top_of_world;
struct zone_data *zone_table;
static int rolls, actions, skill, die = 1;
#define ZONE_MINLVL(rnum) (zone_table[(rnum)].min_level)
#define USE_MOVE_ACTION(ch) (actions++)
bool is_flying(struct char_data *ch) { return FALSE; }
int find_eq_pos(struct char_data *ch, struct obj_data *obj, char *arg) { return -1; }
int d20(struct char_data *ch) { rolls++; return die; }
int compute_ability(struct char_data *ch, int abil) { return skill; }
size_t send_to_char(struct char_data *ch, const char *fmt, ...) { return 0; }
'''
checks = r'''
int main(void) {
  struct char_data ch = {0}; struct room_data rooms[1] = {0};
  struct zone_data zones[1] = {0};
  int races[] = {RACE_CROCODILE, RACE_GIANT_CROCODILE, RACE_DIRE_CROCODILE,
    RACE_POLAR_BEAR, RACE_SMALL_WATER_ELEMENTAL, RACE_MEDIUM_WATER_ELEMENTAL,
    RACE_LARGE_WATER_ELEMENTAL, RACE_WATER_ELEMENTAL, RACE_HUGE_WATER_ELEMENTAL,
    RACE_GARGANTUAN_WATER_ELEMENTAL, RACE_COLOSSAL_WATER_ELEMENTAL,
    RACE_BLACK_DRAGON, RACE_GREEN_DRAGON};
  int sectors[] = {SECT_WATER_SWIM, SECT_UD_WATER, SECT_UNDERWATER};
  int i, j;
  world = rooms; zone_table = zones; GET_LEVEL(&ch) = 10;
  zones[0].min_level = 30;
  assert(!is_natural_swimmer(NULL));
  for (i = 0; i < sizeof(races)/sizeof(races[0]); i++) {
    /* NPC companions use their actual race. */
    SET_BIT_AR(MOB_FLAGS(&ch), MOB_ISNPC); GET_REAL_RACE(&ch) = races[i];
    for (j = 0; j < 3; j++) { rooms[0].sector_type = sectors[j]; assert(is_natural_swimmer(&ch)); assert(has_boat(&ch, 0)); }
    assert(rolls == 0);
    rooms[0].sector_type = SECT_WATER_NOSWIM; assert(!has_boat(&ch, 0));
    rooms[0].sector_type = SECT_UD_NOSWIM; assert(!has_boat(&ch, 0));
    /* PC transformations use the active wildshape body. */
    REMOVE_BIT_AR(MOB_FLAGS(&ch), MOB_ISNPC); GET_REAL_RACE(&ch) = RACE_HUMAN;
    GET_DISGUISE_RACE(&ch) = races[i]; SET_BIT_AR(AFF_FLAGS(&ch), AFF_WILD_SHAPE);
    rooms[0].sector_type = SECT_WATER_SWIM; assert(has_boat(&ch, 0)); assert(rolls == 0);
    REMOVE_BIT_AR(AFF_FLAGS(&ch), AFF_WILD_SHAPE);
  }
  /* A disguise and water breathing alone do not confer swimming skill. */
  assert(!is_natural_swimmer(&ch)); SET_BIT_AR(AFF_FLAGS(&ch), AFF_SCUBA);
  assert(!has_boat(&ch, 0)); assert(rolls == 1 && actions == 1);
  GET_DISGUISE_RACE(&ch) = 0; skill = 50;
  assert(has_boat(&ch, 0)); assert(rolls == 2 && actions == 1);
  /* An aquatic subtype in any of the NPC's three slots grants swimming. */
  SET_BIT_AR(MOB_FLAGS(&ch), MOB_ISNPC);
  for (i = 0; i < 3; i++) {
    GET_SUBRACE(&ch, i) = SUBRACE_AQUATIC; assert(has_boat(&ch, 0));
    GET_SUBRACE(&ch, i) = SUBRACE_UNKNOWN;
  }
  assert(rolls == 2);
  /* Aquatic subtype data cannot grant a PC a swimming benefit. */
  REMOVE_BIT_AR(MOB_FLAGS(&ch), MOB_ISNPC); GET_SUBRACE(&ch, 0) = SUBRACE_AQUATIC;
  assert(!is_natural_swimmer(&ch));
  return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='natural-swimming-') as tmp:
    path = Path(tmp) / 'check.c'; path.write_text(headers + stubs + code + checks)
    binary = Path(tmp) / 'check'
    subprocess.run([os.environ.get('CC', 'cc'), '-std=gnu99', '-Wall', '-Wextra',
                    '-Wno-unused-parameter', '-Wno-sign-compare', '-I' + str(root / 'src'),
                    str(path), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print('Natural swimming: companion races, wildshapes, aquatic subtypes, normal rolls, and boat restrictions passed.')
