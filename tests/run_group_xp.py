#!/usr/bin/env python3
"""Exercise the production group XP calculation without a running server."""
from pathlib import Path
import os
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'src/fight.c').read_text()
start = source.index('static void group_gain(struct char_data *ch, struct char_data *victim)\n{')
code = source[start:source.index('\n}', start) + 2]
harness = r'''
#include <assert.h>
#include <stddef.h>
#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))
#define GROUP(ch) ((ch)->group)
#define IS_PET(ch) ((ch)->pet)
#define IS_NPC(ch) ((ch)->npc)
#define IN_ROOM(ch) ((ch)->room)
#define GET_LEVEL(ch) ((ch)->level)
#define GET_EXP(ch) ((ch)->exp)
#define CONFIG_EXP_LEVEL_DIFFERENCE 10
#define CONFIG_MAX_EXP_LOSS 9000
#ifdef CAMPAIGN_DL
#define BONUS_PER_MEMBER 10
#else
#define BONUS_PER_MEMBER 2
#endif
struct char_data;
struct group { struct char_data **members; };
struct char_data { struct group *group; int pet, npc, room, level, exp, reward, calls; };
static int cursor;
static void *simple_list(struct char_data **members) {
  if (!members) { cursor=0; return NULL; }
  return members[cursor] ? members[cursor++] : NULL;
}
static void perform_group_gain(struct char_data *ch, int base, struct char_data *victim) {
  ch->reward=base; ch->calls++;
}
'''
checks = r'''
int main(void) {
  struct char_data players[14] = {0}, victim = {0};
  struct char_data *members[15] = {0};
  struct group party = {members};
  int sizes[] = {1,2,3,6,12}, s, i, n, expected;
  victim.npc=1; victim.level=20; victim.exp=3000;
  for (s=0;s<5;s++) {
    n=sizes[s];
    for (i=0;i<14;i++) {
      players[i]=(struct char_data){.group=&party,.room=1,.level=20};
      members[i]=NULL;
    }
    for (i=0;i<n;i++) members[i]=&players[i];
    players[n].pet=1; members[n]=&players[n];
    players[n+1].room=2; members[n+1]=&players[n+1];
    members[n+2]=NULL;
    group_gain(&players[0], &victim);
    expected=n>1 ? 1+1000*((100+n*BONUS_PER_MEMBER)/100) : 1000;
    for (i=0;i<n;i++) {
      assert(players[i].calls==1);
      assert(players[i].reward==expected);
      assert(players[i].reward>=1000);
    }
    assert(players[n].calls==0); assert(players[n+1].calls==0);
  }
  /* Existing party-level bonus and PvP reward cap still apply. */
  members[1]=NULL;
  players[0].level=16; victim.level=20;
  group_gain(&players[0], &victim); assert(players[0].reward==1500);
  victim.npc=0; victim.exp=90000;
  group_gain(&players[0], &victim); assert(players[0].reward==6000);
  return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='group-xp-') as tmp:
    path = Path(tmp) / 'check.c'
    path.write_text(harness + code + checks)
    for campaign in ([], ['-DCAMPAIGN_DL']):
        binary = Path(tmp) / 'check'
        subprocess.run([os.environ.get('CC','cc'), '-std=c99', '-Wall', '-Wextra',
                        '-Wno-unused-parameter', *campaign, str(path), '-o', str(binary)], check=True)
        subprocess.run([str(binary)], check=True)
print('Group XP: full rewards for 1–12 members, eligibility, level bonus, and PvP cap passed in both campaign modes.')
