#!/usr/bin/env python3
"""Exercise the production return-object branch with matching and wrong recipients."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'src/quest.c').read_text()
start = source.index('    case AQ_OBJ_RETURN:', source.index('void autoquest_trigger_check('))
branch = source[start:source.index('    case AQ_GIVE_GOLD:', start)]
harness = r'''
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
typedef int mob_rnum;
struct obj_data { int vnum; };
struct char_data { int npc, vnum, remaining; struct obj_data *carrying; };
struct { struct { char *short_descr; } player; } mob_proto[1] = {{{"the infant's healer"}}};
#define AQ_OBJ_RETURN 1
#define NOBODY -1
#define IS_NPC(c) ((c)->npc)
#define GET_MOB_VNUM(c) ((c)->vnum)
#define GET_OBJ_VNUM(o) ((o)->vnum)
#define QST_TARGET(r) 7806
#define QST_RETURNMOB(r) 7811
#define QST_NAME(r) "Mushy Feelings"
static int messages, completed, moved;
static int real_mobile(int v) { return v == 7811 ? 0 : NOBODY; }
static int real_object(int v) { return v; }
static int real_room(int v) { return v; }
static void send_to_char(struct char_data *ch, const char *fmt, ...) {
  char buf[512]; va_list ap; va_start(ap, fmt); vsnprintf(buf, sizeof(buf), fmt, ap); va_end(ap);
  assert(strstr(buf, "does not count")); assert(strstr(buf, "infant's healer")); messages++;
}
static void complete_group_item_objective(struct char_data *ch, int index) {
  assert(ch->remaining > 0); if (--ch->remaining == 0) completed++;
}
static struct obj_data *get_obj_in_list_num(int v, struct obj_data *obj) {
  return obj && obj->vnum == v ? obj : 0;
}
static void obj_from_char(struct obj_data *obj) { moved++; }
static void obj_to_room(struct obj_data *obj, int room) { assert(room == 1); }
static void hand_in(struct char_data *ch, struct char_data *vict, struct obj_data *object) {
  int rnum = 0, index = 0; struct obj_data *obj;
  switch (AQ_OBJ_RETURN) {
''' + branch + r'''
  }
}
int main(void) {
  struct obj_data mushroom = {7806}, other = {7807};
  struct char_data player = {0, 0, 5, 0};
  struct char_data healer = {1, 7820, 0, &mushroom};
  hand_in(&player, &healer, &mushroom);
  assert(player.remaining == 5 && messages == 1 && moved == 0);
  healer.vnum = 7811;
  hand_in(&player, &healer, &other);
  hand_in(&player, &healer, 0);
  assert(player.remaining == 5 && moved == 0);
  for (int i = 0; i < 5; i++) hand_in(&player, &healer, &mushroom);
  assert(player.remaining == 0 && completed == 1 && moved == 5);
  puts("Return-object quest checks passed.");
}
'''
with tempfile.TemporaryDirectory() as tmp:
    c = Path(tmp) / 'returns.c'
    c.write_text(harness)
    exe = Path(tmp) / 'returns'
    subprocess.run(['cc', '-std=c99', str(c), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
