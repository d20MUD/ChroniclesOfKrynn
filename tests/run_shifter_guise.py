#!/usr/bin/env python3
"""Exercise production Shifter Guise gating, casting routes, and login backfill."""
from pathlib import Path
import re
import subprocess
import tempfile
root=Path(__file__).resolve().parents[1]
parser=(root/'src/spell_parser.c').read_text()
classes=(root/'src/class.c').read_text()
players=(root/'src/players.c').read_text()
prep=(root/'src/spell_prep.c').read_text()
def function(source, signature):
    a=source.index(signature)
    return source[a:source.index('\n}',a)+2]+'\n'
helper=function(parser,'bool isShifterMagic(')
atwill=function(parser,'sbyte canCastAtWill(')
# Disable unrelated existing at-will sources while testing the production dispatcher.
other_helpers=set(re.findall(r'\b([a-zA-Z_]\w*)\(',atwill))-{
    'if','canCastAtWill','isShifterMagic','GET_LEVEL','time','find_cantrip_class'}
constants=sorted(set(re.findall(r'\b(?:SPELL|CLASS|FEAT)_\w+\b',helper+atwill)))
constants=sorted(set(constants+['CLASS_SORCERER', 'CLASS_DRAGONRIDER', 'SPELL_OVERLAND_FLIGHT'])-{'CLASS_UNDEFINED'})
code=r'''
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>
typedef signed char sbyte;
#define TRUE true
#define FALSE false
#define CLASS_UNDEFINED -1
#define LVL_IMMORT 50
#define CAST_SPELL 1
#define CAST_INNATE 2
struct player_specials { struct { time_t aurak_dominate_ready; } saved; };
struct char_data {
  bool npc; int shifter_level,level,feat,gear_feat,casting_class;
  struct player_specials *player_specials;
};
#define IS_NPC(ch) ((ch)->npc)
#define CLASS_LEVEL(ch,c) ((ch)->shifter_level)
#define HAS_REAL_FEAT(ch,f) ((ch)->feat)
#define GET_LEVEL(ch) ((ch)->level)
#define CASTING_CLASS(ch) ((ch)->casting_class)
#define GET_CASTING_CLASS(ch) CASTING_CLASS(ch)
#define find_cantrip_class(...) CLASS_UNDEFINED
'''
code+='enum { '+', '.join(constants)+' };\n'
for name in sorted(other_helpers):
    code+=f'#define {name}(...) 0\n'
code+=helper+atwill
# Test the actual player-file migration, including characters who already passed level one.
a=players.index('  /* Existing shifters also receive the new level-one class ability. */')
b=players.index('  reconcile_alter_self(ch);',a)
code+='static void backfill(struct char_data *ch) {\n'+players[a:b]+'}\n'
# Extract the class-ability branch before prepared spell extraction.
a=parser.index('      else if (treat_as_at_will && isShifterMagic(ch, spellnum))')
b=parser.index('      else if (isWarlockMagic(ch, spellnum))',a)
code+='''static int immediate(struct char_data *ch,int spellnum,int *slots) {
  bool treat_as_at_will=canCastAtWill(ch,spellnum); int clevel=0;
  if (false) {}
'''+parser[a:b]+'''  else { --*slots; clevel=1; }
  return clevel;
}
'''
# Delayed casts use the same caster level as instant/quickened casts.
a=parser.index('    if (CASTING_CLASS(ch) == CLASS_SHIFTER && isShifterMagic(ch, spellnum))')
b=parser.index('\n',parser.index('      cast_level = GET_LEVEL(ch);',a))+1
code+='static int delayed(struct char_data *ch,int spellnum) { int cast_level=1;\n'+parser[a:b]+'return cast_level; }\n'
# Real circle selection handles both ability spells before class-list validation.
a=prep.index('  /* Shifter Guise grants abilities, not a prepared class spell list. */')
b=prep.index('  /* Aurak signature powers',a)
code+='''static int calculate_metamagic_modifier(struct char_data *ch,int c,int m) { return m; }
static int circle(struct char_data *ch,int char_class,int spellnum,int metamagic) {
'''+prep[a:b]+'  return 99;\n}\n'
# Concentration does not use an unavailable-class spell level for these abilities.
a=parser.index('  if (CASTING_CLASS(ch) == CLASS_SHIFTER && isShifterMagic(ch, spellnum))')
b=parser.index('\n',parser.index('    spell_level = spellnum == SPELL_ALTER_SELF ? 2 : 1;',a))+1
code+='static int concentration_level(struct char_data *ch,int spellnum) { int spell_level=50;\n'+parser[a:b]+' return spell_level; }\n'
# Parse and exercise the production UI's at-will class routing.
a=parser.index('        if (isWarlockMagic(ch, spellnum))\n          atwill_class')
b=parser.index('\n        if (atwill_class == CLASS_UNDEFINED)',a)
code+='static int selected_class(struct char_data *ch,int spellnum) { int atwill_class=CLASS_UNDEFINED;\n'+parser[a:b]+' return atwill_class; }\n'
code+=r'''
int main(void) {
  struct player_specials special={0};
  struct char_data ch={.level=11,.shifter_level=1,.player_specials=&special};
  assert(!isShifterMagic(NULL,SPELL_ALTER_SELF));
  assert(!canCastAtWill(&ch,SPELL_DISGUISE_SELF));
  ch.gear_feat=1; assert(!canCastAtWill(&ch,SPELL_ALTER_SELF));
  backfill(&ch); assert(ch.feat==1);
  assert(canCastAtWill(&ch,SPELL_DISGUISE_SELF));
  assert(canCastAtWill(&ch,SPELL_ALTER_SELF));
  assert(!canCastAtWill(&ch,SPELL_ANIMATE_DEAD));
  for(int spell=0;spell<=1;spell++) {
    int sn=spell ? SPELL_ALTER_SELF : SPELL_DISGUISE_SELF;
    int slots=3;
    for(int n=0;n<5;n++) {
      assert(selected_class(&ch,sn)==CLASS_SHIFTER);
      assert(immediate(&ch,sn,&slots)==11 && slots==3);
      assert(ch.casting_class==CLASS_SHIFTER);
      assert(delayed(&ch,sn)==11);
    }
    assert(circle(&ch,CLASS_SHIFTER,sn,0)==(spell ? 2 : 1));
    assert(circle(&ch,CLASS_SHIFTER,sn,3)==(spell ? 5 : 4));
    assert(concentration_level(&ch,sn)==(spell ? 2 : 1));
  }
  ch.feat=0; assert(!canCastAtWill(&ch,SPELL_ALTER_SELF));
  assert(circle(&ch,CLASS_SHIFTER,SPELL_ALTER_SELF,0)==99);
  ch.shifter_level=10; backfill(&ch); assert(ch.feat==1);
  ch.shifter_level=0; assert(!canCastAtWill(&ch,SPELL_ALTER_SELF));
  ch.feat=0; backfill(&ch); assert(ch.feat==0);
  ch.shifter_level=1; ch.feat=1; ch.npc=true;
  assert(!isShifterMagic(&ch,SPELL_ALTER_SELF));
  puts("Shifter Guise: feat gating, at-will access, class routing, no slot consumption, caster levels, circles, concentration, and existing-player backfill passed.");
}
'''
# Verify the existing class skill and automatic level-one feat registration.
a=classes.index('  assign_class_abils(CLASS_SHIFTER,')
b=classes.index(');',a)
args=re.sub(r'/\*.*?\*/','',classes[a:b],flags=re.S).split('(',1)[1].split(',')
assert args[18].strip()=='CA', 'Disguise must be a shifter class skill'
assert 'feat_assignment(CLASS_SHIFTER, FEAT_SHIFTER_GUISE, Y, 1, N);' in classes
with tempfile.TemporaryDirectory(prefix='shifter-guise-') as tmp:
    c=Path(tmp)/'guise.c'; c.write_text(code); exe=Path(tmp)/'guise'
    subprocess.run(['cc','-std=gnu99','-Wall','-Wextra','-Wno-unused-parameter','-fsanitize=address,undefined',str(c),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
