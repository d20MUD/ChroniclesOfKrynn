#!/usr/bin/env python3
"""Native skill talent mappings and actual ammo XP bonus path."""
from pathlib import Path
import subprocess,tempfile,re
root=Path(__file__).resolve().parents[1]
s=(root/'src/crafting_new.c').read_text()
def function(signature):
 a=re.search(re.escape(signature)+r'[^;\n]*\n\{',s).start();return s[a:s.index('\n}',a)+2]
code='\n'.join('#include "'+h+'"' for h in ['conf.h','sysdep.h','structs.h','utils.h','comm.h','db.h','constants.h','crafting_new.h','talents.h','spells.h'])+r'''
#include <assert.h>
struct player_special_data dummy_mob;
struct happyhour happy_data;
const char *ability_names[NUM_ABILITIES+1];
static int ranks[MAX_TALENTS];
int get_talent_rank(struct char_data *ch,int talent){assert(talent>0&&talent<MAX_TALENTS);return ranks[talent];}
int get_craft_skill_value(struct char_data *ch,int skill){return 10;}
int craft_skill_level_exp(struct char_data *ch,int level){return INT_MAX;}
void gain_talent_point(struct char_data *ch,int amount){assert(0);}
size_t send_to_char(struct char_data *ch,const char *format,...){return 0;}
void basic_mud_log(const char *format,...){assert(0);}
int MAX(int a,int b){return a>b?a:b;}
'''
code+='\n'.join(function(name) for name in ['int get_proficient_talent_bonus(', 'int get_rapid_talent_bonus(', 'int get_insightful_talent_bonus(', 'int get_efficient_talent_bonus(', 'static int ammo_expertise_chance(', 'void gain_craft_exp('])
code+=r'''
int main(void) {
 struct char_data ch={0};struct player_special_data ps={0};ch.player_specials=&ps;
 int skills[]={ABILITY_CRAFT_WOODWORKING,ABILITY_CRAFT_WEAPONSMITHING};
 int talents[][5]={{TALENT_PROFICIENT_WOODWORKING,TALENT_RAPID_WOODWORKING,TALENT_WOODWORKING_EXPERTISE,TALENT_EFFICIENT_WOODWORKING,TALENT_INSIGHTFUL_WOODWORKING},
 {TALENT_PROFICIENT_WEAPONSMITHING,TALENT_RAPID_WEAPONSMITHING,TALENT_WEAPONSMITHING_EXPERTISE,TALENT_EFFICIENT_WEAPONSMITHING,TALENT_INSIGHTFUL_WEAPONSMITHING}};
 for(int i=0;i<2;i++) {
  int skill=skills[i];ability_names[skill]="Crafting";
  memset(ranks,0,sizeof(ranks));
  gain_craft_exp(&ch,100,skill,true);assert(GET_CRAFT_SKILL_EXP((&ch),skill)==100);
  for(int j=0;j<5;j++)ranks[talents[i][j]]=3;
  assert(get_proficient_talent_bonus(&ch,skill)==3);
  assert(get_rapid_talent_bonus(&ch,skill)==6);
  assert(ammo_expertise_chance(&ch,skill)==3);
  assert(get_efficient_talent_bonus(&ch,skill)==9);
  assert(get_insightful_talent_bonus(&ch,skill)==15);
  assert(!ammo_expertise_chance(&ch,skills[1-i]));
  gain_craft_exp(&ch,100,skill,true);assert(GET_CRAFT_SKILL_EXP((&ch),skill)==215);
  GET_CRAFT((&ch)).craft_specialization[0]=skill;happy_data.crafting_exp_rate=25;
  gain_craft_exp(&ch,100,skill,true);assert(GET_CRAFT_SKILL_EXP((&ch),skill)==455);
  GET_CRAFT((&ch)).craft_specialization[0]=0;happy_data.crafting_exp_rate=0;
 }
 assert(!ammo_expertise_chance(&ch,ABILITY_CRAFT_ALCHEMY));
}
'''
with tempfile.TemporaryDirectory(prefix='ammo-talents-') as tmp:
 p=Path(tmp)/'test.c';binary=Path(tmp)/'test';p.write_text(code)
 subprocess.run(['cc','-std=gnu99','-g','-fsanitize=address,undefined','-fno-pie','-no-pie','-Wno-unused-parameter','-I'+str(root/'src'),str(p),'-o',str(binary)],check=True)
 subprocess.run([str(binary)],check=True)
print('Ammo talents: actual woodworking/weaponsmithing mappings, all five talent types, unrelated-skill isolation and Insightful XP stacking passed.')
