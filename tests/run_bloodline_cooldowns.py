#!/usr/bin/env python3
"""Check actual bloodline event pools and passive rerolls through the native d20 path."""
from pathlib import Path
import os, subprocess, tempfile
root=Path(__file__).resolve().parents[1]
def function(file, signature):
    source=(root/'src'/file).read_text(); a=source.index(signature+'\n{')
    return source[a:source.index('\n}',a)+2]+'\n'
source=(root/'src/class.c').read_text();a=source.index('const struct sorcerer_bloodline extended_bloodlines[')
table=source[a:source.index('};',a)+2]
code='\n'.join('#include "'+h+'"' for h in ['conf.h','sysdep.h','structs.h','utils.h','comm.h','spells.h','class.h','bloodlines.h','feats.h','mud_event.h','assign_wpn_armor.h'])+r'''
#include <assert.h>
int MAX(int a,int b) { return a>b?a:b; }
int MIN(int a,int b) { return a<b?a:b; }
struct player_special_data dummy_mob;
struct feat_info feat_list[NUM_FEATS];
struct mud_event_list mud_event_index[eBLOODLINE_DESTINED_ARCANA + 1];
static struct mud_event_data ev;
static bool live;
static long delay;
static int chance = 100, first = 19, second = 2, die_calls;
#undef GET_PSIONIC_LEVEL
#define GET_PSIONIC_LEVEL(ch) 0
#undef GET_PC_NAME
#define GET_PC_NAME(ch) "test"
void basic_mud_log(const char *fmt, ...) { assert(0); }
size_t send_to_char(struct char_data *ch, const char *fmt, ...) { return 0; }
void send_combat_roll_info(struct char_data *ch, const char *fmt, ...) { }
int get_feat_value(struct char_data *ch, int feat) { return HAS_REAL_FEAT(ch, feat); }
int compute_charisma_bonus(struct char_data *ch) { return 3; }
int get_lucky_weapon_bonus(struct char_data *ch) { return 0; }
int rand_number(int from, int to) { return chance; }
int dice(int count, int sides) {
  if (sides == 20) return die_calls++ == 0 ? first : second;
  return sides; /* Disable unrelated random bonuses. */
}
bool affected_by_spell(struct char_data *ch, int spell) { return false; }
void affect_from_char(struct char_data *ch, int spell) { assert(0); }
int get_daily_uses(struct char_data *ch, int feat) {
  if (feat == FEAT_IT_WAS_MEANT_TO_BE) {
    int level = CLASS_LEVEL(ch, CLASS_SORCERER); return level >= 20 ? 3 : level >= 10 ? 2 : 1;
  }
  return feat == FEAT_SHADOWSTRIKE ? 6 : 1;
}
struct mud_event_data *char_has_mud_event(struct char_data *ch, event_id id) {
  return live && ev.iId == id ? &ev : NULL;
}
struct mud_event_data *new_mud_event(event_id id, void *owner, const char *vars) {
  assert(!live); memset(&ev,0,sizeof(ev)); ev.iId=id; ev.pStruct=owner;
  ev.sVariables=vars ? strdup(vars) : NULL; return &ev;
}
void attach_mud_event(struct mud_event_data *event, long pulses) { assert(event==&ev); live=true;delay=pulses; }
static void clear_event(void) { free(ev.sVariables); ev.sVariables=NULL; live=false; }
'''+table+'\n'
for filename,signature in [
 ('class.c','int bloodline_cooldown_seconds(int feat)'),
 ('utils.c','int start_daily_use_cooldown(struct char_data *ch, int featnum)'),
 ('utils.c','int daily_uses_remaining(struct char_data *ch, int featnum)'),
 ('class.c','int bloodline_reroll(struct char_data *ch, int roll)'),
 ('utils.c','int d20(struct char_data *ch)'),
 ('mud_event.c','EVENTFUNC(event_daily_use_cooldown)')]:
    code+=function(filename,signature)
code+=r'''
static void map_feat(int feat, event_id event) {
  feat_list[feat].event=event; mud_event_index[event].iEvent_Type=EVENT_CHAR;
  mud_event_index[event].feat_num=feat;
}
int main(void) {
 struct char_data ch={0}; struct player_special_data ps={0};ch.player_specials=&ps;
 map_feat(FEAT_SHADOWSTRIKE,eBLOODLINE_SHADOWSTRIKE);
 map_feat(FEAT_CELESTIAL_CONVICTION,eBLOODLINE_CELESTIAL_CONVICTION);
 map_feat(FEAT_DRACONIC_HERITAGE_CLAWS,eDRACCLAWS);
 map_feat(FEAT_METAMAGIC_ADEPT,eARCANEADEPT);
 map_feat(FEAT_IT_WAS_MEANT_TO_BE,eBLOODLINE_DESTINED_REROLL);
 map_feat(FEAT_RAGE,eRAGE);
 /* The whole N-use pool lasts 300 seconds from first use; retries do not restart it. */
 for(int i=0;i<6;i++) {
  assert(start_daily_use_cooldown(&ch,FEAT_SHADOWSTRIKE)==i+1);
  assert(delay==300*PASSES_PER_SEC && daily_uses_remaining(&ch,FEAT_SHADOWSTRIKE)==5-i);
 }
 assert(event_daily_use_cooldown(&ev)==0); clear_event();
 assert(daily_uses_remaining(&ch,FEAT_SHADOWSTRIKE)==6);
 start_daily_use_cooldown(&ch,FEAT_CELESTIAL_CONVICTION);
 assert(delay==30*PASSES_PER_SEC && !daily_uses_remaining(&ch,FEAT_CELESTIAL_CONVICTION));
 assert(!event_daily_use_cooldown(&ev));clear_event();
 int feats[]={FEAT_DRACONIC_HERITAGE_CLAWS,FEAT_METAMAGIC_ADEPT};
 for(int i=0;i<2;i++) {
  start_daily_use_cooldown(&ch,feats[i]);assert(delay==300*PASSES_PER_SEC);
  assert(!event_daily_use_cooldown(&ev));clear_event();
 }
 /* Other classes retain the generic daily-use recovery timing. */
 start_daily_use_cooldown(&ch,FEAT_RAGE);assert(delay==SECS_PER_MUD_DAY*PASSES_PER_SEC);
 clear_event();
 SET_FEAT(&ch,FEAT_IT_WAS_MEANT_TO_BE,1);
 int levels[]={9,10,20};
 for(int i=0;i<3;i++) {
  CLASS_LEVEL((&ch),CLASS_SORCERER)=levels[i];
  for(int j=0;j<=i;j++) {
   first=j==0 ? 1 : 20;second=2;die_calls=0;chance=1;
   assert(d20(&ch)==2 && die_calls==2); /* Natural 1/20 also eligible; no recursive proc. */
  }
  assert(!daily_uses_remaining(&ch,FEAT_IT_WAS_MEANT_TO_BE));
  die_calls=0;first=19;assert(d20(&ch)==19 && die_calls==1);
  assert(!event_daily_use_cooldown(&ev));clear_event();
 }
 chance=2;die_calls=0;assert(d20(&ch)==19 && !live); /* Exactly 1%, not 2%. */
 puts("Bloodline cooldowns: full five-minute pools, 30-second Conviction, unchanged generic recovery, and native passive d20 procs passed.");
}
'''
with tempfile.TemporaryDirectory(prefix='bloodline-cooldowns-') as tmp:
    p=Path(tmp);(p/'test.c').write_text(code)
    subprocess.run([os.environ.get('CC','cc'),'-std=gnu99','-Wall','-Wextra','-Wno-unused-parameter',
                    '-Werror=implicit-function-declaration','-fsanitize=address,undefined','-fno-pie','-no-pie',
                    '-I'+str(root/'src'),str(p/'test.c'),'-o',str(p/'test')],check=True)
    subprocess.run([str(p/'test')],check=True)
