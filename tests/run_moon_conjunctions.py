#!/usr/bin/env python3
"""Check conjunction geometry, classic modifiers, slot transitions, and Thorn exclusion."""
from pathlib import Path
import os
import subprocess
import tempfile

root=Path(__file__).resolve().parents[1]
moon=(root/'src/moon_bonus_spells.c').read_text()
weather=(root/'src/weather.c').read_text()
a=weather.index('void calc_moon_bonus(void)\n{')
calc=weather[a:weather.index('\n}',a)+2]
headers='\n'.join('#include "'+h+'"' for h in ['conf.h','sysdep.h','structs.h','utils.h','db.h','comm.h','moon_bonus_spells.h'])
stubs=r'''
#include <assert.h>
struct config_data config_info;
struct weather_data weather_info;
struct char_data *character_list;
static char output[4096];
int MIN(int a,int b) { return a<b?a:b; }
int MAX(int a,int b) { return a>b?a:b; }
int compute_arcane_level(struct char_data *ch) { return IS_NPC(ch) ? GET_LEVEL(ch) : CLASS_LEVEL(ch,CLASS_WIZARD); }
size_t send_to_char(struct char_data *ch,const char *fmt,...) {
  va_list ap; va_start(ap,fmt); int n=vsnprintf(output+strlen(output),sizeof(output)-strlen(output),fmt,ap); va_end(ap); return n;
}
static void phases(int sol,int lun,int nuit) {
  weather_info.moons.solinari_phase=sol;
  weather_info.moons.lunitari_phase=lun;
  weather_info.moons.nuitari_phase=nuit;
  calc_moon_bonus();
}
/* Independently sample every angular unit in the 504-unit chart. */
static int reference(int sol,int lun,int nuit) {
  int result=0;
  for(int angle=0;angle<504;angle++) {
    int s=angle*36/504+1==sol, l=angle*28/504+1==lun, n=angle*8/504+1==nuit;
    if(s&&l) result|=MOON_CONJUNCTION_SOL_LUN;
    if(s&&n) result|=MOON_CONJUNCTION_SOL_NUIT;
    if(l&&n) result|=MOON_CONJUNCTION_LUN_NUIT;
    if(s&&l&&n) result|=MOON_CONJUNCTION_ALL;
  }
  return result;
}
'''
checks=r'''
int main(void) {
  struct char_data ch={0}, thorn={0};
  struct player_special_data special={0}, thorn_special={0};
  ch.player_specials=&special; thorn.player_specials=&thorn_special;
  CLASS_LEVEL((&ch),CLASS_WIZARD)=10; CLASS_LEVEL((&thorn),CLASS_WIZARD)=10;
  CLASS_LEVEL((&thorn),CLASS_KNIGHT_OF_THE_THORN)=1;
  GET_ALIGNMENT(&ch)=GET_ALIGNMENT(&thorn)=1000;
  IN_ROOM(&ch)=IN_ROOM(&thorn)=0; ch.next=&thorn; character_list=&ch;
  CONFIG_ARCANE_MOON_PHASES=1;
  for(int s=1;s<=36;s++) for(int l=1;l<=28;l++) for(int n=1;n<=8;n++) {
    phases(s,l,n);
    int mask=reference(s,l,n);
    assert(get_moon_conjunctions()==mask);
    int days[3]={s,l,n}, periods[3]={36,28,8};
    int dc[3]={weather_info.moons.solinari_st,weather_info.moons.lunitari_st,weather_info.moons.nuitari_st};
    int lv[3]={weather_info.moons.solinari_lv,weather_info.moons.lunitari_lv,weather_info.moons.nuitari_lv};
    int sp[3]={weather_info.moons.solinari_sp,weather_info.moons.lunitari_sp,weather_info.moons.nuitari_sp};
    int involved[3]={mask&(MOON_CONJUNCTION_SOL_LUN|MOON_CONJUNCTION_SOL_NUIT),
                     mask&(MOON_CONJUNCTION_SOL_LUN|MOON_CONJUNCTION_LUN_NUIT),
                     mask&(MOON_CONJUNCTION_SOL_NUIT|MOON_CONJUNCTION_LUN_NUIT)};
    int stronger[3]={mask&MOON_CONJUNCTION_SOL_LUN,
                    mask&(MOON_CONJUNCTION_SOL_LUN|MOON_CONJUNCTION_LUN_NUIT),
                    mask&MOON_CONJUNCTION_LUN_NUIT};
    const int phase_mod[4]={-1,0,1,0}, phase_slots[4]={0,0,2,1};
    for(int moon=0;moon<3;moon++) {
      int quarter=(days[moon]-1)/(periods[moon]/4);
      assert(dc[moon]==phase_mod[quarter]+((mask&MOON_CONJUNCTION_ALL) ? 2 : !!involved[moon]));
      assert(lv[moon]==phase_mod[quarter]+((mask&MOON_CONJUNCTION_ALL) ? 1 : !!stronger[moon]));
      assert(sp[moon]==phase_slots[quarter]+((mask&MOON_CONJUNCTION_ALL) ? 2 : !!stronger[moon]));
    }
    struct moon_data once=weather_info.moons;
    calc_moon_bonus(); assert(!memcmp(&once,&weather_info.moons,sizeof(once)));
    assert(get_max_moon_bonus_spells(&ch)==weather_info.moons.solinari_sp);
    assert(get_max_moon_bonus_spells(&thorn)==0);
  }
  phases(20,16,1); assert(get_moon_conjunctions()==MOON_CONJUNCTION_SOL_LUN);
  assert(weather_info.moons.solinari_st==2 && weather_info.moons.solinari_lv==2 && weather_info.moons.solinari_sp==3);
  assert(weather_info.moons.lunitari_st==2 && weather_info.moons.lunitari_lv==2 && weather_info.moons.lunitari_sp==3);
  assert(weather_info.moons.nuitari_st==-1 && weather_info.moons.nuitari_lv==-1 && weather_info.moons.nuitari_sp==0);
  phases(23,1,6); assert(get_moon_conjunctions()==MOON_CONJUNCTION_SOL_NUIT);
  assert(weather_info.moons.solinari_st==2 && weather_info.moons.solinari_lv==1 && weather_info.moons.solinari_sp==2);
  assert(weather_info.moons.nuitari_st==2 && weather_info.moons.nuitari_lv==1 && weather_info.moons.nuitari_sp==2);
  phases(1,18,6); assert(get_moon_conjunctions()==MOON_CONJUNCTION_LUN_NUIT);
  assert(weather_info.moons.lunitari_st==2 && weather_info.moons.lunitari_lv==2 && weather_info.moons.lunitari_sp==3);
  /* Two pairwise alignments without a common triple arc are not the Eye. */
  phases(22,18,6);
  assert(get_moon_conjunctions()==(MOON_CONJUNCTION_SOL_LUN|MOON_CONJUNCTION_LUN_NUIT));
  assert(weather_info.moons.lunitari_st==2 && weather_info.moons.lunitari_lv==2 && weather_info.moons.lunitari_sp==3);
  phases(23,18,6); assert(get_moon_conjunctions() & MOON_CONJUNCTION_ALL);
  assert(weather_info.moons.solinari_st==3 && weather_info.moons.solinari_lv==2 && weather_info.moons.solinari_sp==4);
  assert(weather_info.moons.lunitari_st==3 && weather_info.moons.lunitari_lv==2 && weather_info.moons.lunitari_sp==4);
  assert(weather_info.moons.nuitari_st==3 && weather_info.moons.nuitari_lv==2 && weather_info.moons.nuitari_sp==4);
  output[0]=0; show_moon_conjunctions(&ch); assert(strstr(output,"Night of the Eye"));
  assert(use_moon_bonus_spell(&ch)); assert(get_available_moon_bonus_spells(&ch)==3);
  /* On conjunction end, spent slots stay spent and are capped to the new pool. */
  ch.player_specials->saved.moon_bonus_spells_used=4;
  phases(23,1,6); assert(get_max_moon_bonus_spells(&ch)==2 && get_available_moon_bonus_spells(&ch)==0);
  assert(get_used_moon_bonus_spells(&ch)==2);
  phases(1,1,1);
  assert(weather_info.moons.solinari_st==1 && weather_info.moons.solinari_lv==0 && weather_info.moons.solinari_sp==2);
  output[0]=0; show_moon_conjunctions(&ch); assert(!strstr(output,"Night of the Eye"));
  CONFIG_ARCANE_MOON_PHASES=0; phases(23,18,6);
  assert(!get_moon_conjunctions() && weather_info.moons.solinari_st==1 && weather_info.moons.solinari_sp==2);
  assert(!get_max_moon_bonus_spells(&ch));
  CONFIG_ARCANE_MOON_PHASES=1;
  phases(0,18,6); assert(!get_moon_conjunctions());
  phases(23,29,6); assert(!get_moon_conjunctions());
  phases(23,18,9); assert(!get_moon_conjunctions());
  return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='moon-conjunctions-') as tmp:
    path=Path(tmp)/'check.c'
    # The phases helper needs the production calculation's declaration first.
    path.write_text(headers+'\nvoid calc_moon_bonus(void);\n'+stubs+moon+'\n'+calc+checks)
    binary=Path(tmp)/'check'
    subprocess.run([os.environ.get('CC','cc'),'-std=gnu99','-Wall','-Wextra','-Wno-unused-parameter',
                    '-I'+str(root/'src'),str(path),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
print('All 8,064 moon positions, pair/triple modifiers, false conjunctions, slot transitions, configuration, and Thorn exemption passed.')
