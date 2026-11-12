#!/usr/bin/env python3
"""Run production ranged-mode refresh before the single/full attack split."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1]
s=(root/'src/fight.c').read_text();a=s.index('  /* Ammo exhaustion changes a bow');b=s.index('\n  guard_check(',a)
assert a<s.index('return perform_staggered_attack(ch, mode, phase);',a)
code=r'''
#include <assert.h>
#include <stdbool.h>
#define NORMAL_ATTACK_ROUTINE 0
#define TRUE true
#define IS_NPC(ch) ((ch)->npc)
#define THROWING(ch) ((ch)->throwing)
#define FIRING(ch) ((ch)->firing)
struct char_data { bool npc,throwing,firing; };
struct obj_data { int type,value; };
#define ATTACK_TYPE_PRIMARY 0
#define ITEM_WEAPON 1
#define WEAPON_TYPE_HOOPAK 2
#define GET_OBJ_TYPE(o) ((o)->type)
#define GET_OBJ_VAL(o,i) ((o)->value)
static struct obj_data hybrid={1,2};
static bool weapon=true,ammo,throwable,hoopak;
static struct obj_data *get_wielded(struct char_data *ch,int type){return hoopak?&hybrid:0;}
static bool is_throwing_weapon(struct char_data *ch,void *obj){return throwable;}
static void *is_using_ranged_weapon(struct char_data *ch,bool silent){return weapon?ch:0;}
static bool can_fire_ammo(struct char_data *ch,bool silent){if(!ammo)ch->firing=false;return ammo;}
static void refresh(struct char_data *ch,int mode) {
 struct obj_data *wielded;
'''+s[a:b]+r'''
}
int main(void) {
 struct char_data ch={.firing=true};
 refresh(&ch,0);assert(!ch.firing);
 ammo=true;refresh(&ch,0);assert(ch.firing); // refill without stopping combat
 ammo=false;refresh(&ch,0);assert(!ch.firing);
 ammo=true;refresh(&ch,1);assert(!ch.firing); // previews cannot change combat
 weapon=false;refresh(&ch,0);assert(!ch.firing);
 weapon=true;ch.throwing=true;refresh(&ch,0);assert(!ch.firing);
 ch.throwing=false;throwable=true;refresh(&ch,0);assert(!ch.firing);
 throwable=false;hoopak=true;refresh(&ch,0);assert(ch.firing);
 ammo=false;refresh(&ch,0);assert(!ch.firing);
 hoopak=false;ammo=true;ch.npc=true;refresh(&ch,0);assert(!ch.firing);
}
'''
with tempfile.TemporaryDirectory(prefix='ranged-resume-') as tmp:
 p=Path(tmp)/'test.c';binary=Path(tmp)/'test';p.write_text(code)
 subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Wno-unused-parameter','-Werror','-fsanitize=address,undefined',str(p),'-o',str(binary)],check=True)
 subprocess.run([str(binary)],check=True)
print('Ranged resume: exhaustion/refill, single-attack ordering, display isolation, melee weapon, throwing and NPC guards passed.')
