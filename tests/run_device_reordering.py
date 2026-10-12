#!/usr/bin/env python3
"""Verify stable device moves preserve full records and reject unsafe input/work."""
from pathlib import Path
import os
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'src/act.other.c').read_text()
start = source.index('static void move_artificer_device(')
code = source[start:source.index('\n}', start) + 2]
headers = '\n'.join('#include "' + h + '"' for h in
                    ['conf.h', 'sysdep.h', 'structs.h', 'utils.h', 'comm.h', 'mud_event.h'])
stubs = r'''
#include <assert.h>
static int saves, working;
struct mud_event_data *char_has_mud_event(struct char_data *ch, event_id id) {
  return working == id ? (struct mud_event_data *)ch : NULL;
}
void save_char(struct char_data *ch, int room) { saves++; }
size_t send_to_char(struct char_data *ch, const char *fmt, ...) { return 0; }
'''
checks = r'''
int main(void) {
  struct char_data ch = {0}; struct player_special_data ps = {0};
  struct player_invention original[4] = {0};
  int i, j;
  ch.player_specials = &ps; ps.saved.num_inventions = 4;
  ps.saved.device_recharge_timer = 77;
  ps.saved.device_creation_cooldown = 1234;
  for (i = 0; i < 4; i++) {
    struct player_invention *d = &original[i];
    snprintf(d->keywords, sizeof(d->keywords), "device%d", i);
    snprintf(d->short_description, sizeof(d->short_description), "Device %d", i);
    snprintf(d->long_description, sizeof(d->long_description), "Long device %d", i);
    d->num_spells = 4; d->uses = i; d->reliability = 80 + i; d->duration = 10 + i;
    d->cooldown_expires = 1000 + i; d->disabled_until = 2000 + i;
    d->dc_penalty = i * 2; d->broken = i == 3;
    for (j = 0; j < 4; j++) { d->spell_effects[j] = 10*i+j; d->spell_levels[j] = j+1; }
  }
  memcpy(ps.saved.inventions, original, sizeof(original));
  move_artificer_device(&ch, "4", "1");
  assert(!memcmp(&ps.saved.inventions[0], &original[3], sizeof(original[0])));
  for (i = 1; i < 4; i++)
    assert(!memcmp(&ps.saved.inventions[i], &original[i-1], sizeof(original[0])));
  move_artificer_device(&ch, "1", "4");
  assert(!memcmp(ps.saved.inventions, original, sizeof(original))); assert(saves == 2);
  move_artificer_device(&ch, "2", "3");
  assert(!memcmp(&ps.saved.inventions[2], &original[1], sizeof(original[0])));
  move_artificer_device(&ch, "3", "2");
  assert(!memcmp(ps.saved.inventions, original, sizeof(original))); assert(saves == 4);
  move_artificer_device(&ch, "2", "2");
  {
    const char *invalid[] = {"", "0", "-1", "5", "1x", "cat", "999999999999999999999999"};
    for (i = 0; i < sizeof(invalid)/sizeof(invalid[0]); i++) {
      move_artificer_device(&ch, invalid[i], "1");
      move_artificer_device(&ch, "1", invalid[i]);
    }
  }
  working = eDEVICE_REPAIR; move_artificer_device(&ch, "1", "4");
  working = eDEVICE_CREATION; move_artificer_device(&ch, "1", "4"); working = 0;
  assert(saves == 4 && !memcmp(ps.saved.inventions, original, sizeof(original)));
  assert(ps.saved.num_inventions == 4 && ps.saved.device_recharge_timer == 77);
  assert(ps.saved.device_creation_cooldown == 1234);
  ps.saved.num_inventions = 0; move_artificer_device(&ch, "1", "1"); assert(saves == 4);
  return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='device-reordering-') as tmp:
    path = Path(tmp) / 'check.c'; path.write_text(headers + stubs + code + checks)
    binary = Path(tmp) / 'check'
    subprocess.run([os.environ.get('CC', 'cc'), '-std=gnu99', '-Wall', '-Wextra',
                    '-Wno-unused-parameter', '-Wno-sign-compare', '-I' + str(root / 'src'),
                    str(path), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print('Device reordering: stable moves, complete state preservation, saves, invalid input, and pending work passed.')
