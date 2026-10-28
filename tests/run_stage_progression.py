#!/usr/bin/env python3
"""Keep perk-stage thresholds and level readiness aligned with actual level XP."""
from pathlib import Path
import os
import subprocess
import tempfile

root=Path(__file__).resolve().parents[1]
perks=(root/'src/perks.c').read_text()
def function(text,signature):
    start=text.index(signature+'\n{')
    return text[start:text.index('\n}',start)+2]+'\n'
headers=''.join('#include "'+h+'"\n' for h in ['conf.h','sysdep.h','structs.h','utils.h',
    'comm.h','db.h','spells.h','perks.h','constants.h','class.h'])
code=next(line for line in (root/'src/class.c').read_text().splitlines() if line.startswith('#define EXP_MAX '))+'\n'
code+=r'''
#include <assert.h>
struct player_special_data dummy_mob;
struct config_data config_info;
const char *class_names[NUM_CLASSES+1];
static char output[8192];
static int synthetic;
void basic_mud_log(const char *fmt,...) {assert(0);}
int MAX(int a,int b) {return a>b?a:b;}
size_t send_to_char(struct char_data *ch,const char *fmt,...) {
  va_list ap;va_start(ap,fmt);int n=vsnprintf(output+strlen(output),sizeof(output)-strlen(output),fmt,ap);va_end(ap);return n;
}
char *add_commas(long number) {
  static char strings[4][64];static int index;
  char *result=strings[(index++)%4];snprintf(result,64,"%ld",number);return result;
}
void get_award_perk_classes(struct char_data *ch,int award_class,int *first,int *second) {
  *first=*second=award_class;
}
'''
code+=function((root/'src/class.c').read_text(),'long int level_exp(struct char_data *ch, int level)').replace('long int level_exp(', 'static long int production_level_exp(',1)
code+=r'''
long int level_exp(struct char_data *ch,int level) {
  if(synthetic)return level==GET_LEVEL(ch)?1000:1101;
  return production_level_exp(ch,level);
}
'''
for signature in ['long stage_exp_threshold(struct char_data *ch, int stage)',
                  'int calculate_stage_xp_needed(struct char_data *ch)',
                  'void init_stage_data(struct char_data *ch)',
                  'void update_stage_data(struct char_data *ch)',
                  'bool check_stage_advancement(struct char_data *ch, int *perk_points_awarded)',
                  'static int calculate_stage_awards_from_current_xp(struct char_data *ch, int *resulting_stage)']:
    code+=function(perks,signature)
info=(root/'src/act.informative.c').read_text()
start=info.index('      current_stage = ch->player_specials->saved.stage_info.current_stage;')
end=info.index('    }\n  }\n  else',start)
code+='static void display_stage(struct char_data *ch) { int current_stage,stage_xp,stage_xp_needed;\n'+info[start:end]+'}\n'
code+=r'''
int main(void) {
  struct char_data ch={0};struct player_special_data ps={0};int awarded,simulated;
  ch.player_specials=&ps;GET_CLASS(&ch)=CLASS_WARRIOR;GET_REAL_RACE(&ch)=RACE_HUMAN;
  class_names[CLASS_WARRIOR]="warrior";CONFIG_PERK_SYSTEM=1;CONFIG_EXPERIENCE_MULTIPLIER=100;
  for(int level=1;level<=10;level++) {
    GET_LEVEL(&ch)=level;memset(&ps,0,sizeof(ps));
    long base=level_exp(&ch,level),next=level_exp(&ch,level+1);
    GET_EXP(&ch)=base;init_stage_data(&ch);output[0]=0;
    assert(!check_stage_advancement(&ch,&awarded) && !awarded);
    assert(ps.saved.stage_info.current_stage==0 && ps.saved.perk_points[CLASS_WARRIOR]==0);
    display_stage(&ch);assert(strstr(output,"Stage XP:") && !strstr(output,"Ready to level"));
    for(int stage=1;stage<=4;stage++) {
      long boundary=stage_exp_threshold(&ch,stage);
      GET_EXP(&ch)=boundary-1;output[0]=0;
      assert(!check_stage_advancement(&ch,&awarded) && !awarded);
      assert(calculate_stage_awards_from_current_xp(&ch,&simulated)==stage-1 && simulated==stage-1);
      assert(!strstr(output,"Type 'gain'"));
      GET_EXP(&ch)=boundary;output[0]=0;
      assert(check_stage_advancement(&ch,&awarded));
      assert(awarded==(stage<4));assert(ps.saved.stage_info.current_stage==stage);
      assert(ps.saved.perk_points[CLASS_WARRIOR]==(stage<4?stage*2:6));
      assert((strstr(output,"Type 'gain'")!=NULL)==(GET_EXP(&ch)>=next));
      int expected=stage<4?stage:3;
      assert(calculate_stage_awards_from_current_xp(&ch,&simulated)==expected && simulated==stage);
      assert(!check_stage_advancement(&ch,&awarded) && !awarded);
    }
    assert(stage_exp_threshold(&ch,4)==next);
    output[0]=0;display_stage(&ch);assert(strstr(output,"Ready to level!"));
    /* XP loss and old prematurely completed stages must not advertise readiness or re-award points. */
    GET_EXP(&ch)=(base+next)/2;output[0]=0;
    assert(!check_stage_advancement(&ch,&awarded));
    display_stage(&ch);assert(!strstr(output,"Ready to level!") && strstr(output,"XP needed to level"));
    assert(ps.saved.perk_points[CLASS_WARRIOR]==6);
  }
  /* Reproduce the reported level 2 -> 3 midpoint directly. */
  GET_LEVEL(&ch)=2;memset(&ps,0,sizeof(ps));init_stage_data(&ch);
  GET_EXP(&ch)=(level_exp(&ch,2)+level_exp(&ch,3))/2;output[0]=0;
  assert(check_stage_advancement(&ch,&awarded) && awarded==2);
  assert(ps.saved.stage_info.current_stage==2 && !strstr(output,"Type 'gain'"));
  GET_EXP(&ch)=level_exp(&ch,3);output[0]=0;
  assert(check_stage_advancement(&ch,&awarded) && awarded==1 && strstr(output,"Type 'gain'"));
  /* Fractions round up; the final boundary must be the actual next-level XP. */
  synthetic=1;memset(&ps,0,sizeof(ps));init_stage_data(&ch);
  assert(stage_exp_threshold(&ch,1)==1026 && stage_exp_threshold(&ch,2)==1051);
  assert(stage_exp_threshold(&ch,3)==1076 && stage_exp_threshold(&ch,4)==1101);
  for(int xp=1000;xp<=1101;xp++) {
    GET_EXP(&ch)=xp;output[0]=0;check_stage_advancement(&ch,&awarded);
    assert((ps.saved.stage_info.current_stage==4)==(xp==1101));
  }
  int before=ps.saved.perk_points[CLASS_WARRIOR];
  CONFIG_PERK_SYSTEM=0;assert(!check_stage_advancement(&ch,&awarded) && !awarded);
  CONFIG_PERK_SYSTEM=1;SET_BIT_AR(MOB_FLAGS(&ch),MOB_ISNPC);
  assert(!check_stage_advancement(&ch,&awarded) && !awarded);
  assert(ps.saved.perk_points[CLASS_WARRIOR]==before);
  assert(!check_stage_advancement(NULL,&awarded) && !awarded);
  return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='stage-progression-') as tmp:
    path=Path(tmp)/'check.c';path.write_text(headers+code);binary=Path(tmp)/'check'
    subprocess.run([os.environ.get('CC','cc'),'-std=gnu99','-Wall','-Wextra','-Wno-unused-parameter',
        '-I'+str(root/'src'),str(path),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
print('Stage progression: real level 1–10 XP curves, level 2 midpoint, all quarter boundaries, perk totals/no duplicates, legacy calculation, rounding, and readiness display passed.')
