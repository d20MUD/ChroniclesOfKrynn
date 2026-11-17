#!/usr/bin/env python3
"""Exercise production guard checks, five-minute caching and player-file records."""
from pathlib import Path
import os
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
movement = (root / 'src/movement.c').read_text()
players = (root / 'src/players.c').read_text()
db = (root / 'src/db.c').read_text()
a = movement.index('void prune_blocker_stealth_checks(')
b = movement.index('static void begin_deferred_room_msdp_update', a)
helpers = movement[a:b]
a = movement.index('  for (mob = world[IN_ROOM(ch)].people;')
b = movement.index('\n  if (block && !IS_NPC(ch)', a)
guards = movement[a:b]
a = players.index('  /* Persist only live stealth-blocker outcomes;')
b = players.index('  if (BLASTING(ch))', a)
save = players[a:b]
a = players.index('        if (!strcmp(tag, "BStl"))')
b = players.index('        else if (!strcmp(tag, "Badp"))', a)
load = players[a:b]
a = db.index('    if (ch->player_specials)\n    {\n      struct blocker_stealth_check *check, *next;')
b = db.index('\n    if (ch->bags)', a)
cleanup = db[a:b]
code = r'''
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "clan_access.h"
#include "spells.h"
#include "comm.h"
#include "blocker_stealth.h"
#include <assert.h>
static time_t now = 100000;
static time_t test_time(time_t *out) { if (out) *out = now; return now; }
#define time test_time
struct room_data rooms[1];
struct room_data *world = rooms;
struct index_data indices[4];
struct index_data *mob_index = indices;
mob_rnum top_of_mobt = 3;
void basic_mud_log(const char *fmt, ...) { assert(0); }
struct player_special_data dummy_mob;
static int rolls, stealth = 30, perception = 20, messages;
static bool ghost;
struct sivak_form *active_sivak_form(struct char_data *ch) { return NULL; }
bool has_ghost(struct char_data *ch) { return ghost; }
bool clan_access_check(struct char_data *ch, const struct clan_requirement *r, bool explain) {
  return !r->clan;
}
int skill_roll(struct char_data *ch, int skill) {
  rolls++; assert(skill == ABILITY_STEALTH || skill == ABILITY_PERCEPTION);
  return skill == ABILITY_STEALTH ? stealth : perception;
}
const char *act(const char *s, int hide, struct char_data *ch, struct obj_data *obj, void *vict, int type) {
  messages++; return s;
}
'''+helpers+r'''
static bool blocked(struct char_data *ch, int dir) {
  struct char_data *mob; sbyte block = FALSE;
'''+guards+r'''
  return block;
}
#define BUFFER_WRITE(...) fprintf(out, __VA_ARGS__)
static void save_checks(FILE *out, struct char_data *ch) {
'''+save+r'''
}
static void load_record(struct char_data *ch, const char *line) {
  const char *tag = "BStl";
'''+load+r'''
}
static void cleanup(struct char_data *ch) {
'''+cleanup+r'''
  ch->player_specials = NULL;
}
static void init_player(struct char_data *ch) {
  memset(ch, 0, sizeof(*ch)); CREATE(ch->player_specials, struct player_special_data, 1);
  GET_LEVEL(ch) = 50; IN_ROOM(ch) = 0;
}
static void init_guard(struct char_data *ch, int index) {
  memset(ch, 0, sizeof(*ch)); SET_BIT_AR(MOB_FLAGS(ch), MOB_ISNPC);
  GET_MOB_RNUM(ch) = index; indices[index].vnum = 1000 + index;
  SET_BIT_AR(MOB_FLAGS(ch), MOB_BLOCK_N); SET_BIT_AR(MOB_FLAGS(ch), MOB_BLOCK_E);
  rooms[0].people = ch;
}
int main(void) {
  struct char_data player, other, guard, second, restored;
  init_player(&player); init_player(&other); init_player(&restored); init_guard(&guard, 0);
  assert(MOB_BLOCK_STEALTH == 110 && NUM_MOB_FLAGS == 111);
  /* Default guards cannot be bypassed by either stealth state or invisibility. */
  SET_BIT_AR(AFF_FLAGS(&player), AFF_HIDE);
  assert(blocked(&player, NORTH) && rolls == 0);
  REMOVE_BIT_AR(AFF_FLAGS(&player), AFF_HIDE); SET_BIT_AR(AFF_FLAGS(&player), AFF_SNEAK);
  assert(blocked(&player, NORTH) && rolls == 0);
  REMOVE_BIT_AR(AFF_FLAGS(&player), AFF_SNEAK); SET_BIT_AR(AFF_FLAGS(&player), AFF_INVISIBLE);
  SET_BIT_AR(MOB_FLAGS(&guard), MOB_BLOCK_STEALTH);
  assert(blocked(&player, NORTH) && rolls == 0);
  /* Unblocked directions never consume a check; Hide alone is sufficient. */
  SET_BIT_AR(AFF_FLAGS(&player), AFF_HIDE);
  assert(!blocked(&player, SOUTH) && rolls == 0);
  assert(!blocked(&player, NORTH) && rolls == 2);
  assert(player.player_specials->blocker_stealth_checks->expires == now + 300);
  stealth = 1; perception = 100; now += 299;
  int old_messages = messages;
  assert(!blocked(&player, EAST) && rolls == 2 && messages == old_messages);
  /* Cached success still requires hiding or sneaking. */
  REMOVE_BIT_AR(AFF_FLAGS(&player), AFF_HIDE);
  assert(blocked(&player, NORTH) && rolls == 2);
  SET_BIT_AR(AFF_FLAGS(&player), AFF_SNEAK);
  assert(!blocked(&player, NORTH) && rolls == 2);
  /* At exactly five minutes the check expires and failure is also cached. */
  now++;
  assert(blocked(&player, NORTH) && rolls == 4);
  stealth = 100; perception = 1;
  assert(blocked(&player, EAST) && rolls == 4);
  /* Logging out/in keeps the result and absolute expiration, without refresh. */
  FILE *saved = tmpfile(); assert(saved); save_checks(saved, &player); rewind(saved);
  char line[200]; assert(fgets(line, sizeof(line), saved)); assert(!strncmp(line, "BStl: ", 6));
  load_record(&restored, line + 6); fclose(saved);
  SET_BIT_AR(AFF_FLAGS(&restored), AFF_HIDE);
  time_t expiry = player.player_specials->blocker_stealth_checks->expires;
  now += 100; assert(blocked(&restored, NORTH) && rolls == 4);
  assert(restored.player_specials->blocker_stealth_checks->expires == expiry);
  /* Different players roll independently; copies/respawns of one prototype do not. */
  SET_BIT_AR(AFF_FLAGS(&other), AFF_HIDE);
  assert(!blocked(&other, NORTH) && rolls == 6);
  init_guard(&second, 0); SET_BIT_AR(MOB_FLAGS(&second), MOB_BLOCK_STEALTH);
  assert(blocked(&player, NORTH) && rolls == 6);
  /* Separate guard prototypes require separate successful checks. */
  init_guard(&guard, 1); SET_BIT_AR(MOB_FLAGS(&guard), MOB_BLOCK_STEALTH);
  second.next_in_room = &guard; rooms[0].people = &second;
  assert(!blocked(&other, NORTH) && rolls == 8);
  /* A second guard without opt-in still blocks after passing the first. */
  REMOVE_BIT_AR(MOB_FLAGS(&guard), MOB_BLOCK_STEALTH);
  assert(blocked(&other, NORTH) && rolls == 8);
  /* Builder hard-block and clan requirements take precedence over stealth. */
  SET_BIT_AR(MOB_FLAGS(&second), MOB_NO_BLOCK_BYPASS);
  assert(blocked(&other, NORTH) && rolls == 8);
  REMOVE_BIT_AR(MOB_FLAGS(&second), MOB_NO_BLOCK_BYPASS);
  SET_BIT_AR(MOB_FLAGS(&second), MOB_BLOCK_CLAN); second.clan_requirement.clan = 7;
  assert(blocked(&other, NORTH) && rolls == 8);
  /* Matching ordinary exemptions do not consume stealth rolls. */
  REMOVE_BIT_AR(MOB_FLAGS(&second), MOB_BLOCK_CLAN); second.next_in_room = NULL;
  SET_BIT_AR(MOB_FLAGS(&second), MOB_BLOCK_RACE);
  assert(!blocked(&player, NORTH) && rolls == 8);
  REMOVE_BIT_AR(MOB_FLAGS(&second), MOB_BLOCK_RACE);
  /* Reject malformed, expired, invalid-state and excessive-expiry saved records. */
  load_record(&restored, "bad"); load_record(&restored, "-1 100600 1");
  load_record(&restored, "2000 99999 1"); load_record(&restored, "2000 200000 1");
  load_record(&restored, "2000 100600 2");
  assert(!restored.player_specials->blocker_stealth_checks->next);
  /* Duplicate records cannot overwrite a cached failure or extend its timer. */
  remember_blocker_stealth_check(&restored, 1000, now + 300, true);
  assert(!restored.player_specials->blocker_stealth_checks->passed);
  assert(restored.player_specials->blocker_stealth_checks->expires == expiry);
  now = expiry; stealth = perception = 20;
  assert(blocked(&restored, NORTH) && rolls == 10); /* ties favor the guard */
  now += 300;
  saved = tmpfile(); assert(saved); save_checks(saved, &restored);
  assert(ftell(saved) == 0 && !restored.player_specials->blocker_stealth_checks); fclose(saved);
  cleanup(&player); cleanup(&other); cleanup(&restored);
  puts("Stealth blockers: opt-in flags, hide/sneak, opposed checks, multi-guard rules, 300-second cache, persistence and cleanup passed.");
}
'''
with tempfile.TemporaryDirectory(prefix='blocker-stealth-') as tmp:
    path = Path(tmp); (path / 'check.c').write_text(code)
    subprocess.run([os.environ.get('CC', 'cc'), '-std=gnu99', '-Wall', '-Wextra',
                    '-Wno-unused-parameter', '-Werror=implicit-function-declaration',
                    '-fsanitize=address,undefined', '-fno-pie', '-no-pie', '-I'+str(root/'src'),
                    str(path/'check.c'), '-o', str(path/'check')], check=True)
    subprocess.run([str(path/'check')], check=True)
