#!/usr/bin/env python3
"""Native production PSP tick: fractional recovery, isolation and ordinary parity."""
import pathlib, subprocess, tempfile
root=pathlib.Path(__file__).resolve().parents[1]
s=(root/'src/limits.c').read_text();a=s.index('static void recover_psp_tick(');f=s[a:s.index('\n}',a)+2]
header=r'''
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#define MAX(a,b) ((a)>(b)?(a):(b))
#define POS_SLEEPING 1
#define POS_RECLINING 2
#define POS_RESTING 3
#define POS_SITTING 4
#define FEAT_PSIONIC_RECOVERY 1
struct char_data {int psp,max,helper,feat,pos,level,npc,fighting;int clan_psionic_percent,clan_psionic_remainder;int64_t clan_psionic_expires_at;char clan_psionic_activation_uid[33];};
#define GET_PSP(ch) ((ch)->psp)
#define GET_MAX_PSP(ch) ((ch)->max)
#define GET_POS(ch) ((ch)->pos)
#define GET_PSIONIC_LEVEL(ch) ((ch)->level)
#define FIGHTING(ch) ((ch)->fighting)
#define IS_NPC(ch) ((ch)->npc)
#define HAS_FEAT(ch,f) ((ch)->feat)
static int get_psp_regen_amount(struct char_data *ch) {return ch->helper;}
'''
main=r'''
static int ordinary(struct char_data ch) {
 if(ch.fighting)return ch.psp;
 if(ch.psp<ch.max)ch.psp++;
 ch.psp+=ch.helper;
 if(ch.psp<ch.max&&ch.feat)ch.psp+=ch.feat*2;
 if(ch.pos>=1&&ch.pos<=4&&ch.psp<ch.max)ch.psp+=2+ch.level/7;
 if(ch.psp>ch.max)ch.psp--;
 if(ch.psp>ch.max)ch.psp=ch.max;
 return ch.psp;
}
int main(void) {
 struct char_data ch={0};
 // Default-off matches the existing tick over positions, feats and cap boundaries.
 for(int pos=0;pos<=5;pos++)for(int psp=-2;psp<=104;psp++)for(int feat=0;feat<=2;feat++)for(int helper=0;helper<=3;helper++) {
  ch=(struct char_data){.psp=psp,.max=100,.helper=helper,.feat=feat,.pos=pos,.level=21};int expected=ordinary(ch);
  recover_psp_tick(&ch,100);assert(ch.psp==expected&&ch.clan_psionic_remainder==0);
 }
 for(int percent=10;percent<=30;percent+=10) {
  ch=(struct char_data){.max=1000,.clan_psionic_percent=percent,.clan_psionic_expires_at=200};strcpy(ch.clan_psionic_activation_uid,"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
  for(int tick=1;tick<=10;tick++){recover_psp_tick(&ch,100);assert(ch.psp==tick+(tick*percent)/100);assert(ch.clan_psionic_remainder==(tick*percent)%100);}
 }
 ch=(struct char_data){.max=1000,.helper=2,.feat=1,.pos=POS_RESTING,.level=14,.clan_psionic_percent=30,.clan_psionic_expires_at=200};strcpy(ch.clan_psionic_activation_uid,"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
 // Base 1 + helper 2 + feat 2 + resting 4 = 9, not a compounded bonus.
 recover_psp_tick(&ch,100);assert(ch.psp==11&&ch.clan_psionic_remainder==70);
 ch.fighting=1;recover_psp_tick(&ch,100);assert(ch.psp==11&&ch.clan_psionic_remainder==0);ch.fighting=0;
 ch.clan_psionic_remainder=90;recover_psp_tick(&ch,200);assert(ch.psp==20&&ch.clan_psionic_remainder==0);
 ch.psp=99;ch.max=100;ch.clan_psionic_remainder=90;recover_psp_tick(&ch,100);assert(ch.psp==100&&ch.clan_psionic_remainder==0);
 ch.psp=105;ch.clan_psionic_remainder=90;recover_psp_tick(&ch,100);assert(ch.psp==100&&ch.clan_psionic_remainder==0);
 ch.psp=0;ch.npc=1;ch.clan_psionic_remainder=90;recover_psp_tick(&ch,100);assert(ch.psp==9&&ch.clan_psionic_remainder==0);
 // Separate characters never share partial points.
 struct char_data other=ch;other.npc=0;other.psp=0;other.helper=0;other.feat=0;other.pos=0;other.clan_psionic_remainder=0;
 recover_psp_tick(&other,100);assert(other.psp==1&&other.clan_psionic_remainder==30);assert(ch.clan_psionic_remainder==0);
 ch=(struct char_data){.max=INT_MAX,.helper=INT_MAX,.feat=INT_MAX,.clan_psionic_percent=30,.clan_psionic_expires_at=200};strcpy(ch.clan_psionic_activation_uid,"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");recover_psp_tick(&ch,100);assert(ch.psp==INT_MAX&&ch.clan_psionic_remainder==0);
 puts("PSP ordinary parity, all tiers, fractional carry, caps, combat/expiry and isolation passed");
}
'''
with tempfile.TemporaryDirectory(prefix='clan-psionic-') as tmp:
 p=pathlib.Path(tmp);(p/'test.c').write_text(header+f+main)
 subprocess.run(['cc','-std=gnu99','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-pie','-no-pie',str(p/'test.c'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
