#!/usr/bin/env python3
"""Exercise production movement messages with native per-observer visibility rules."""
from pathlib import Path
import os
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'src/movement_messages.c').read_text()


def function(signature):
    start = source.index(signature)
    return source[start:source.index('\n}', start) + 2]


headers = ['conf.h', 'sysdep.h', 'structs.h', 'utils.h', 'spells.h', 'comm.h', 'constants.h']
code = '\n'.join('#include "' + name + '"' for name in headers)
code += r'''
#include <assert.h>
struct player_special_data dummy_mob;
void basic_mud_log(const char *format, ...) {assert(0);}
struct room_data rooms[1], *world = rooms;
const char *dirs[] = {"north", "east", "south", "west", "up", "down"};
int rev_dir[] = {SOUTH, WEST, NORTH, EAST, DOWN, UP};
static struct char_data *ordinary, *detecting, *last_actor, *ordinary_actor;
static int ordinary_messages, detecting_messages;
static bool darkness, true_sight, blind_sight, hidden_detected, sneak_detected;
bool room_is_dark(room_rnum room) {return darkness;}
bool char_has_ultra(struct char_data *ch) {return false;}
bool char_has_infra(struct char_data *ch) {return false;}
bool is_outdoors(struct char_data *ch) {return false;}
bool has_true_sight(struct char_data *ch) {return ch == detecting && true_sight;}
bool has_blindsight(struct char_data *ch) {return ch == detecting && blind_sight;}
bool can_see_hidden(struct char_data *ch, struct char_data *actor) {
  return !AFF_FLAGGED(actor, AFF_HIDE) || has_true_sight(ch) || has_blindsight(ch) ||
         (ch == detecting && hidden_detected);
}
bool can_hear_sneaking(struct char_data *ch, struct char_data *actor) {
  return !AFF_FLAGGED(actor, AFF_SNEAK) || (ch == detecting && sneak_detected);
}
char *show_pers(struct char_data *actor, struct char_data *observer) {
  return CAN_SEE(observer, actor) ? "a visible traveler" : "someone";
}
size_t send_to_char(struct char_data *ch, const char *format, ...) {return 0;}
/* TO_VICT deliberately ignores hide_invisible, just like production act(). */
const char *act(const char *message, int hide, struct char_data *actor,
                struct obj_data *obj, void *target, int type) {
  last_actor = actor;
  if (type == TO_VICT) {
    if (target == ordinary) {ordinary_messages++; ordinary_actor = actor;}
    if (target == detecting) detecting_messages++;
  } else if (type == TO_ROOM || type == TO_NOTVICT) {
    if ((!hide || CAN_SEE(ordinary, actor)) && target != ordinary) ordinary_messages++;
    if ((!hide || CAN_SEE(detecting, actor)) && target != detecting) detecting_messages++;
  }
  return message;
}
static void clear_messages(void) {ordinary_messages = detecting_messages = 0; last_actor = NULL;}
'''
for signature in ['static void visible_movement_message(', 'void display_leave_messages(',
                  'void display_enter_messages(']:
    code += '\n' + function(signature)
code += r'''
int main(void) {
  struct char_data actor = {0}, observer = {0}, seer = {0}, mount = {0};
  struct player_special_data ps = {0}, os = {0}, ss = {0}, ms = {0};
  actor.player_specials = &ps; observer.player_specials = &os;
  seer.player_specials = &ss; mount.player_specials = &ms;
  ordinary = &observer; detecting = &seer;
  IN_ROOM(&actor) = IN_ROOM(&observer) = IN_ROOM(&seer) = IN_ROOM(&mount) = 0;
  rooms[0].people = &actor; actor.next_in_room = &observer; observer.next_in_room = &seer;
  actor.player.walkin = "$n drifts in from"; actor.player.walkout = "$n drifts";
  SET_BIT_AR(MOB_FLAGS(&actor), MOB_ISNPC);
  clear_messages(); display_enter_messages(&actor, NORTH, false, false);
  assert(ordinary_messages == 1 && detecting_messages == 1);
  SET_BIT_AR(AFF_FLAGS(&actor), AFF_INVISIBLE);
  SET_BIT_AR(AFF_FLAGS(&seer), AFF_DETECT_INVIS);
  clear_messages(); display_enter_messages(&actor, NORTH, false, false);
  assert(!ordinary_messages && detecting_messages == 1);
  /* Successful hearing alone must not reveal an invisible sneaker. */
  SET_BIT_AR(AFF_FLAGS(&actor), AFF_SNEAK); sneak_detected = true;
  REMOVE_BIT_AR(AFF_FLAGS(&seer), AFF_DETECT_INVIS);
  clear_messages(); display_enter_messages(&actor, NORTH, false, false);
  assert(!ordinary_messages && !detecting_messages);
  true_sight = true;
  clear_messages(); display_enter_messages(&actor, NORTH, false, false);
  assert(!ordinary_messages && detecting_messages == 1);
  clear_messages(); display_leave_messages(&actor, NORTH, false, false);
  assert(!ordinary_messages && detecting_messages == 1);
  /* Hiding and sneaking must each be defeated, independently. */
  true_sight = false; REMOVE_BIT_AR(AFF_FLAGS(&actor), AFF_INVISIBLE);
  SET_BIT_AR(AFF_FLAGS(&actor), AFF_HIDE); hidden_detected = false;
  clear_messages(); display_enter_messages(&actor, NORTH, false, false);
  assert(!ordinary_messages && !detecting_messages);
  hidden_detected = true;
  clear_messages(); display_enter_messages(&actor, NORTH, false, false);
  assert(!ordinary_messages && detecting_messages == 1);
  sneak_detected = false;
  clear_messages(); display_enter_messages(&actor, NORTH, false, false);
  assert(!ordinary_messages && !detecting_messages);
  /* The same visibility gate protects targeted mounted arrival/departure paths. */
  memset(AFF_FLAGS(&actor), 0, sizeof(actor.char_specials.saved.affected_by));
  SET_BIT_AR(AFF_FLAGS(&actor), AFF_INVISIBLE);
  RIDING(&actor) = &mount; RIDDEN_BY(&mount) = &actor;
  for (int sneaking = 0; sneaking < 2; sneaking++) {
    if (sneaking) SET_BIT_AR(AFF_FLAGS(&actor), AFF_SNEAK);
    sneak_detected = true; true_sight = true;
    clear_messages(); display_enter_messages(&actor, NORTH, true, false);
    assert(ordinary_messages == sneaking && detecting_messages == 1);
    if (sneaking) assert(ordinary_actor == &mount);
    clear_messages(); display_leave_messages(&actor, NORTH, true, false);
    assert(ordinary_messages == sneaking && detecting_messages == 1);
    if (sneaking) assert(ordinary_actor == &mount);
  }
  puts("PASS: visible/invisible/custom arrivals, detection, hide/sneak and mounted movement privacy.");
}
'''
with tempfile.TemporaryDirectory(prefix='movement-visibility-') as directory:
    cfile = Path(directory) / 'test.c'
    binary = Path(directory) / 'test'
    cfile.write_text(code)
    subprocess.run([os.environ.get('CC', 'cc'), '-std=c11', '-g', '-fsanitize=address,undefined',
                    '-fno-omit-frame-pointer', '-I' + str(root / 'src'), str(cfile), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
