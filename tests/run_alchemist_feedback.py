#!/usr/bin/env python3
"""Exercise production bomb creation, roll feedback, feat eligibility, and silent extracts."""
from pathlib import Path
import os
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
alchemy = (root / 'src/alchemy.c').read_text()
fight = (root / 'src/fight.c').read_text()
feats = (root / 'src/feats.c').read_text()
parser = (root / 'src/spell_parser.c').read_text()

def function(source, signature):
    start = source.index(signature)
    return source[start:source.index('\n}', start)+2]

start = alchemy.index('  else if (is_abbrev(ccmd, "make"))')
make = alchemy[alchemy.index('\n  {', start)+4:alchemy.index('\n  else if (is_abbrev(ccmd, "list"))', start)]
make = make[:make.rfind('\n  }')]
start = fight.index('  attack_bonus += extra_bonus;', fight.index('static int perform_attack_roll('))
roll = fight[start:fight.index('\n}', start)]
start = feats.index('  if (CLASS_LEVEL(ch, CLASS_ALCHEMIST) > 0 &&', feats.index('int feat_is_available('))
filter_code = feats[start:feats.index('\n  if (featnum == FEAT_PRACTICED_SPELLCASTER)', start)]
extract = function(parser, 'static bool is_alchemist_extract(')
start = parser.index('  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SOUNDPROOF)', parser.index('static int cast_spell_with_type_and_slot('))
verbal_checks = parser[start:parser.index('  if (IS_EPIC_SPELL(spellnum) && !epic_spell_preflight', start)]


harness = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#define TRUE 1
#define FALSE 0
#define BOMB_NONE 0
#define BOMB_NORMAL 1
#define NUM_BOMB_TYPES 3
#define ALC_DISC_FAST_BOMBS 1
#define FEAT_FASTER_BOMB_PREPARATION 1274
#define FEAT_ARMORED_SPELLCASTING 153
#define FEAT_SPELL_PENETRATION 97
#define FEAT_ENHANCED_SPELL_DAMAGE 304
#define CLASS_ALCHEMIST 0
#define CLASS_WIZARD 1
#define CLASS_FIGHTER 2
#define NUM_CLASSES 3
#define IS_SPELLCASTER_CLASS(c) ((c)==CLASS_ALCHEMIST || (c)==CLASS_WIZARD)
#define CLASS_LEVEL(ch,c) ((ch)->levels[c])
#define GET_CASTING_CLASS(ch) ((ch)->casting_class)
#define IS_EPIC_SPELL(s) ((s)==999)
#define KNOWS_DISCOVERY(ch,d) ((ch)->fast_bombs)
#define HAS_FEAT(ch,f) ((ch)->faster_prep)
#define RL_SEC * 10
#define DEBUGMODE 0
#define ROOM_SOUNDPROOF 1
#define AFF_SILENCED 1
#define CAST_SPELL 1
#define CAST_SCROLL 2
#define FEAT_AUTOMATIC_SILENT_SPELL 1000
#define IN_ROOM(ch) 0
#define ROOM_FLAGGED(room,flag) soundproof
#define AFF_FLAGGED(ch,flag) silenced
#define ACTION_MOVE 1
#define ACTION_STANDARD 0
#define atMOVE ACTION_MOVE
#define atSTANDARD ACTION_STANDARD
typedef int action_type;
struct player_specials { struct { int bombs[2]; } saved; };
struct char_data {
  struct player_specials *player_specials;
  int fast_bombs, faster_prep, levels[3], casting_class;
};
static char output[4096];
static int silenced, soundproof;
static int saved, cooldown, action, standard_available=1, move_available=1, natural_roll;
static const char *bomb_types[] = {"none","normal","acid"};
static int num_of_bombs_preparable(struct char_data *ch) { return 2; }
static int num_of_bombs_prepared(struct char_data *ch) {
  return !!ch->player_specials->saved.bombs[0]+!!ch->player_specials->saved.bombs[1];
}
static int find_open_bomb_slot(struct char_data *ch) {
  for(int i=0;i<2;i++) if(!ch->player_specials->saved.bombs[i]) return i;
  return -1;
}
static void send_to_char(struct char_data *ch, const char *fmt, ...) {
  va_list ap; va_start(ap,fmt); vsnprintf(output,sizeof(output),fmt,ap); va_end(ap);
}
static int is_abbrev(const char *a,const char *b) { return *a && !strncmp(a,b,strlen(a)); }
static void list_bomb_types_known(struct char_data *ch) {}
static int is_action_available(struct char_data *ch,int type,int msg) {
  return type==atMOVE ? move_available : standard_available;
}
static void save_char(struct char_data *ch,int unused) { saved++; }
static void start_action_cooldown(struct char_data *ch,int type,int duration) {
  action=type; cooldown=duration;
}
static int d20(struct char_data *ch) { return natural_roll; }
static int is_spellnum_psionic(int s) { return s==888; }
static int compute_spells_circle(struct char_data *ch,int c,int s,int m,int unused) { return 4; }
static void act(const char *text,int hide,struct char_data *ch,void *obj,void *victim,int mode) {}
#define TO_ROOM 0
'''
code = '''
static void make_bomb(struct char_data *ch, const char *scmd) {
  int bSlot=0, i=0, type=0;
''' + make + '''
}
static int roll_bomb(struct char_data *ch,int attack_bonus,int extra_bonus,int victim_ac,bool show_roll) {
''' + roll + '''
}
static bool offered(struct char_data *ch,int featnum) {
''' + filter_code + '''
  return TRUE;
}
''' + extract + '''
static int verbal_cast_allowed(struct char_data *ch,int spellnum,int casttype) {
  int metamagic=0;
''' + verbal_checks + '''
  return 1;
}
'''
checks = r'''
int main(void) {
  struct player_specials special={0};
  struct char_data ch={.player_specials=&special};
  int fast, quick;
  for(fast=0;fast<2;fast++) for(quick=0;quick<2;quick++) {
    memset(&special,0,sizeof(special));
    ch.fast_bombs=fast; ch.faster_prep=quick;
    saved=0; cooldown=0; output[0]=0;
    make_bomb(&ch,"normal");
    assert(saved==1 && special.saved.bombs[0]==BOMB_NORMAL);
    assert(strstr(output,"Bombs prepared: 1/2"));
    assert(action==(fast ? atMOVE : atSTANDARD));
    assert(cooldown==(quick ? 30 : 60));
    make_bomb(&ch,"normal");
    assert(saved==2 && strstr(output,"Bombs prepared: 2/2"));
    make_bomb(&ch,"normal");
    assert(saved==2 && strstr(output,"cannot create any more bombs"));
  }
  memset(&special,0,sizeof(special)); saved=0; standard_available=move_available=0;
  make_bomb(&ch,"normal"); assert(saved==0 && num_of_bombs_prepared(&ch)==0);
  standard_available=move_available=1;
  make_bomb(&ch,"unknown"); assert(saved==0);
  natural_roll=10;
  assert(roll_bomb(&ch,4,2,16,true)==0);
  assert(strstr(output,"1d20 = 10, +6 bonus = 16 vs touch AC 16 (hit)"));
  assert(roll_bomb(&ch,4,0,16,true)==-2 && strstr(output,"(miss)"));
  strcpy(output,"unchanged");
  assert(roll_bomb(&ch,4,0,16,false)==-2 && !strcmp(output,"unchanged"));
  ch.levels[CLASS_ALCHEMIST]=1;
  assert(!offered(&ch,FEAT_ARMORED_SPELLCASTING));
  assert(!offered(&ch,FEAT_SPELL_PENETRATION));
  assert(!offered(&ch,FEAT_ENHANCED_SPELL_DAMAGE));
  assert(offered(&ch,FEAT_FASTER_BOMB_PREPARATION));
  ch.levels[CLASS_FIGHTER]=1; assert(!offered(&ch,FEAT_SPELL_PENETRATION));
  ch.levels[CLASS_WIZARD]=1; assert(offered(&ch,FEAT_SPELL_PENETRATION));
  ch.casting_class=CLASS_ALCHEMIST;
  assert(is_alchemist_extract(&ch,1)); assert(!is_alchemist_extract(&ch,999));
  silenced=soundproof=1;
  assert(verbal_cast_allowed(&ch,1,CAST_SPELL));
  assert(!verbal_cast_allowed(&ch,999,CAST_SPELL));
  assert(!verbal_cast_allowed(&ch,1,CAST_SCROLL));
  ch.casting_class=CLASS_WIZARD; assert(!is_alchemist_extract(&ch,1));
  assert(!verbal_cast_allowed(&ch,1,CAST_SPELL));
  soundproof=0; assert(!verbal_cast_allowed(&ch,1,CAST_SPELL));
  silenced=0; assert(verbal_cast_allowed(&ch,1,CAST_SPELL));
  return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='alchemist-feedback-') as tmp:
    path=Path(tmp)/'check.c'
    path.write_text(harness+code+checks)
    binary=Path(tmp)/'check'
    subprocess.run([os.environ.get('CC','cc'),'-std=c99','-Wall','-Wextra',
                    '-Wno-unused-parameter',str(path),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
print('Bomb capacity feedback, preparation speed, attack roll feedback, feat filtering, and extract classification passed.')
