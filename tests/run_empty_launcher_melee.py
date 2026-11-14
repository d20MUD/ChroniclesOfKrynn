#!/usr/bin/env python3
"""Exercise empty-launcher eligibility and non-mutating melee combat profiles."""
from pathlib import Path
import subprocess
import re
import tempfile
root=Path(__file__).resolve().parents[1]
weapons=(root/'src/assign_wpn_armor.c').read_text()
fight=(root/'src/fight.c').read_text()
def function(text,signature):
    a=text.index(signature)
    return text[a:text.index('\n}',a)+2]
headers='\n'.join('#include "'+h+'"' for h in ['conf.h','sysdep.h','structs.h','utils.h','spells.h','fight.h','assign_wpn_armor.h'])
code=headers+r'''
#include <assert.h>
#define act(...) ((void)0)
#define send_to_char(...) ((void)0)
#define obj_has_special_ability(...) false
static struct obj_data *shot;
int recorded_combat_distance(struct char_data *ch,struct char_data *victim) {return -1;}
#define ranged_ammunition(ch) shot
struct weapon_table weapon_list[NUM_WEAPON_TYPES];
int MAX(int a,int b) {return a>b?a:b;}
int MIN(int a,int b) {return a<b?a:b;}
'''
for sig in ['int current_weapon_type(', 'bool is_throwing_weapon(', 'bool ammunition_is_unlimited(', 'bool usable_ammunition(', 'bool hoopak_uses_ranged(', 'int ranged_weapon_melee_type(']:
    code+=function(weapons,sig)+'\n'
code+=function(fight,'static bool empty_launcher_melee_profile(')+'\n'
code+=function(fight,'int combat_melee_range(')+'\n'
code+=r"""
#undef GROUP
#define GROUP(ch) true
static int test_row;
#define get_formation_row(ch) test_row
"""+function((root/'src/handler.c').read_text(),'bool formation_allows_attack(')+'\n'

# Run the actual mandatory-penalty statements and existing final damage clamp.
a=fight.index('  if (empty_launcher)\n  {',fight.index('int compute_damage_bonus('))
b=fight.index('\n  if (wielded',a)
code+='int damage_bonus(bool empty_launcher,int dambonus) {bool display_mode=false;\n'+fight[a:b]+'\nreturn dambonus;}\n'
penalty=re.search(r'  if \(empty_launcher\)\s+calc_bab -= 8;',fight)
assert penalty
code+='int attack_bonus(bool empty_launcher,int calc_bab) {\n'+penalty.group(0)+'\nreturn calc_bab;}\n'
assert 'return MAX(1, dam); // min damage of 1' in function(fight,'int compute_hit_damage(')
code+=r'''
int main(void) {
 struct char_data ch={0};struct player_special_data specials={0};ch.player_specials=&specials;
 struct obj_data weapon={0},pouch={0},ammo={0},wrong={0},profile;
 GET_OBJ_TYPE(&weapon)=ITEM_WEAPON;GET_OBJ_TYPE(&ammo)=GET_OBJ_TYPE(&wrong)=ITEM_MISSILE;
 GET_OBJ_VAL(&wrong,0)=AMMO_TYPE_DART;
 weapon_list[WEAPON_TYPE_QUARTERSTAFF].numDice=1;weapon_list[WEAPON_TYPE_QUARTERSTAFF].diceSize=6;
 weapon_list[WEAPON_TYPE_CLUB].numDice=1;weapon_list[WEAPON_TYPE_CLUB].diceSize=6;
 weapon_list[WEAPON_TYPE_WHIP].numDice=1;weapon_list[WEAPON_TYPE_WHIP].diceSize=3;
 weapon_list[WEAPON_TYPE_WHIP].weaponFlags=WEAPON_FLAG_REACH;
 int types[]={WEAPON_TYPE_LONG_BOW,WEAPON_TYPE_SHORT_BOW,WEAPON_TYPE_COMPOSITE_LONGBOW,
  WEAPON_TYPE_COMPOSITE_SHORTBOW_5,WEAPON_TYPE_HEAVY_CROSSBOW,WEAPON_TYPE_LIGHT_CROSSBOW,
  WEAPON_TYPE_HAND_CROSSBOW,WEAPON_TYPE_HEAVY_REP_XBOW,WEAPON_TYPE_LIGHT_REP_XBOW,WEAPON_TYPE_SLING};
 for(unsigned i=0;i<sizeof(types)/sizeof(*types);i++) {
  int expected=i<4?WEAPON_TYPE_QUARTERSTAFF:i<9?WEAPON_TYPE_CLUB:WEAPON_TYPE_WHIP;
  GET_OBJ_VAL(&weapon,0)=types[i];GET_OBJ_VAL(&weapon,1)=9;GET_OBJ_VAL(&weapon,2)=12;
  weapon.tinker_bonus=3;FIRING(&ch)=true;GET_EQ(&ch,WEAR_AMMO_POUCH)=NULL;
  assert(ranged_weapon_melee_type(&ch,&weapon)==expected && FIRING(&ch));
  GET_EQ(&ch,WEAR_AMMO_POUCH)=&pouch;pouch.contains=&wrong;wrong.next_content=NULL;
  assert(ranged_weapon_melee_type(&ch,&weapon)==expected);
  GET_OBJ_VAL(&ammo,0)=i<4?AMMO_TYPE_ARROW:i<9?AMMO_TYPE_BOLT:AMMO_TYPE_STONE;
  GET_OBJ_VAL(&ammo,AMMO_VAL_MAX)=10;GET_OBJ_VAL(&ammo,AMMO_VAL_QUANTITY)=0;
  wrong.next_content=&ammo;assert(ranged_weapon_melee_type(&ch,&weapon)==expected);
  struct obj_data *effective=&weapon;struct obj_data original=weapon;
  assert(empty_launcher_melee_profile(&ch,&effective,&profile,ATTACK_TYPE_PRIMARY));
  assert(effective==&profile && GET_OBJ_VAL(effective,0)==expected);
  assert(GET_OBJ_VAL(effective,1)==1 && GET_OBJ_VAL(effective,2)==(i==9?3:6));
  assert(effective->tinker_bonus==3 && !memcmp(&weapon,&original,sizeof(weapon)));
  assert(combat_melee_range(&ch,&weapon)==(i==9?10:5));
  test_row=FORMATION_MIDDLE;assert(formation_allows_attack(&ch,ATTACK_TYPE_PRIMARY,&weapon)==(i==9));
  test_row=FORMATION_BACK;assert(!formation_allows_attack(&ch,ATTACK_TYPE_PRIMARY,&weapon));
  assert(formation_allows_attack(&ch,ATTACK_TYPE_RANGED,&weapon));
  effective=&weapon;assert(!empty_launcher_melee_profile(&ch,&effective,&profile,ATTACK_TYPE_RANGED));
  shot=&ammo;assert(!empty_launcher_melee_profile(&ch,&effective,&profile,ATTACK_TYPE_PRIMARY));shot=NULL;
  GET_OBJ_VAL(&ammo,AMMO_VAL_QUANTITY)=1;
  assert(!ranged_weapon_melee_type(&ch,&weapon));
  assert(pouch.contains==&wrong && wrong.next_content==&ammo && FIRING(&ch));
  GET_OBJ_VAL(&ammo,AMMO_VAL_QUANTITY)=0;GET_OBJ_VAL(&ammo,AMMO_VAL_UNLIMITED)=1;
  assert(!ranged_weapon_melee_type(&ch,&weapon));GET_OBJ_VAL(&ammo,AMMO_VAL_UNLIMITED)=0;
  SET_BIT_AR(MOB_FLAGS(&ch),MOB_ISNPC);assert(!ranged_weapon_melee_type(&ch,&weapon));
  REMOVE_BIT_AR(MOB_FLAGS(&ch),MOB_ISNPC);
 }
 GET_OBJ_VAL(&weapon,0)=WEAPON_TYPE_LONG_SWORD;assert(!ranged_weapon_melee_type(&ch,&weapon));
 assert(attack_bonus(true,20)==12 && attack_bonus(false,20)==20);
 assert(damage_bonus(true,10)==6 && damage_bonus(false,10)==10);
 for(int base=0;base<5;base++) assert(MAX(1,base+damage_bonus(true,0))==1);
 puts("PASS: all launcher mappings, depleted/mismatched/missing/unlimited ammo, final-shot protection, preserved objects and pouch order, melee dice/reach, -8 attack, -4 damage and minimum 1.");
}
'''
with tempfile.TemporaryDirectory(prefix='empty-launcher-') as directory:
    p=Path(directory);(p/'test.c').write_text(code)
    subprocess.run(['cc','-std=gnu99','-g','-fsanitize=address,undefined','-fno-pie','-no-pie',
                    '-I'+str(root/'src'),str(p/'test.c'),'-o',str(p/'test')],check=True)
    subprocess.run([str(p/'test')],check=True)
