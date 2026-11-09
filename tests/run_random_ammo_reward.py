#!/usr/bin/env python3
"""Verify random ammo is delivered as exactly one finite 50-use item."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1]
s=(root/'src/treasure.c').read_text()
def function(signature):
 a=s.index(signature+'\n{');return s[a:s.index('\n}',a)+2]
headers='\n'.join('#include "'+h+'"' for h in ['conf.h','sysdep.h','structs.h','utils.h','db.h','constants.h','treasure.h','assign_wpn_armor.h','vnums.h','spells.h'])
stubs=r'''
#include <assert.h>
#undef log
#define log(...) ((void)0)
const char *ammo_descs[NUM_A_AMMO_DESCS+1];
const char *ammo_head_descs[NUM_A_AMMO_HEAD_DESCS+1];
const char *material_name[128];
const char *ammo_types[NUM_AMMO_TYPES+1]={"undefined","arrow","bolt","bullet","dart","throwing ammunition"};
static int reads,awards,draws,selected;
static bool missing;
static int rarity, chance_roll=1, element_roll, expected_level=12;
const char *damtypes[NUM_DAM_TYPES]={[DAM_FIRE]="fire",[DAM_COLD]="cold",[DAM_ACID]="acid",[DAM_ELECTRIC]="electricity",[DAM_SOUND]="sonic"};
int MIN(int a,int b){return a<b?a:b;}
int MAX(int a,int b){return a>b?a:b;}
static struct obj_data *delivered;
struct obj_data *read_object(obj_vnum v,int mode) {
 assert(v==AMMO_PROTO);reads++;if(missing)return NULL;
 struct obj_data *o=calloc(1,sizeof(*o));
 for(int i=0;i<NUM_OBJ_VAL_POSITIONS;i++)GET_OBJ_VAL(o,i)=-1;
 return o;
}
int rand_number(int low,int high) {int result=draws++==0?selected: high==100?chance_roll:low==0 && high==4?element_roll:low;assert(result>=low && result<=high);return result;}
int determine_rare_grade(void) {return rarity;}
const char *label_rare_grade(int g) {return "";}
int possible_material_upgrade(int m,int grade) {return m;}
int cp_convert_grade_enchantment(int grade) {return grade;}
void cp_modify_object_applies(struct char_data*c,struct obj_data*o,int grade,int cp_type,int rare,int silent,int source_level) {
 assert(cp_type==CP_TYPE_AMMO && source_level==expected_level && grade==MAX(1,MIN(6,(expected_level+4)/5)));
 assert(GET_OBJ_TYPE(o)==ITEM_MISSILE && GET_OBJ_VAL(o,0)==selected);
 assert(GET_OBJ_VAL(o,AMMO_VAL_MIN)==50 && GET_OBJ_VAL(o,AMMO_VAL_MAX)==50 && GET_OBJ_VAL(o,AMMO_VAL_QUANTITY)==50);
 assert(!GET_OBJ_VAL(o,AMMO_VAL_UNLIMITED) && !GET_OBJ_VAL(o,AMMO_VAL_RETURNING));
 if(selected==AMMO_TYPE_THROWN)assert(GET_OBJ_VAL(o,AMMO_VAL_WEAPON_TYPE)==WEAPON_TYPE_DAGGER);
 assert(OBJ_FLAGGED(o,ITEM_MAGIC));
 assert(GET_OBJ_VAL(o,4)==grade);awards++;delivered=o;
}
'''
code=function('static void set_random_ammo_bonuses(struct obj_data *obj, int grade, int source_level, int rare_grade)')+'\n'+function('void set_ammo_object(struct obj_data *obj, int type)')+'\n'+function('void award_magic_ammo(struct char_data *ch, int grade, int source_level)')
checks=r'''
int main(void) {
 struct char_data ch={0};
 for(int i=0;i<NUM_A_AMMO_DESCS;i++)ammo_descs[i]="fine";
 for(int i=0;i<NUM_A_AMMO_HEAD_DESCS;i++)ammo_head_descs[i]="grooved";
 for(int i=0;i<128;i++)material_name[i]="wood";
 for(rarity=0;rarity<=3;rarity++)for(selected=1;selected<NUM_AMMO_TYPES;selected++) {
  reads=awards=draws=0;delivered=NULL;
  award_magic_ammo(&ch,2,12);
  assert(reads==1 && awards==1 && delivered);
  assert(GET_OBJ_VAL(delivered,4)==3 && GET_OBJ_VAL(delivered,AMMO_VAL_QUANTITY)==50);
  assert(strstr(delivered->name,ammo_types[selected]));
  free(delivered->name);free(delivered->short_description);free(delivered->description);free(delivered);
 }
 // Every five-level boundary, rarity chance, element and ammo subtype.
 const int levels[]={1,5,6,10,11,15,16,20,21,25,26,30,60};
 const int elements[]={DAM_FIRE,DAM_COLD,DAM_ACID,DAM_ELECTRIC,DAM_SOUND};
 for(unsigned l=0;l<sizeof(levels)/sizeof(*levels);l++)for(rarity=0;rarity<=3;rarity++)for(element_roll=0;element_roll<5;element_roll++)for(int r=0;r<2;r++) {
  expected_level=levels[l];chance_roll=r?100:1;draws=reads=awards=0;selected=AMMO_TYPE_ARROW;
  award_magic_ammo(&ch,2,expected_level);assert(reads==1 && awards==1 && delivered);
  int tier=MIN(5,(expected_level-1)/5);bool elemental=rarity>0 && (!r || rarity==RARE_GRADE_MYTHICAL);
  assert(GET_OBJ_VAL(delivered,4)==tier+1);
  assert(GET_OBJ_VAL(delivered,AMMO_VAL_DAMAGE_DIE)==(elemental?MIN(12,4+tier*2):0));
  assert(GET_OBJ_VAL(delivered,AMMO_VAL_DAMAGE_TYPE)==(elemental?elements[element_roll]:0));
  char bonus[16];snprintf(bonus,sizeof(bonus),"(+%d",tier+1);assert(strstr(delivered->short_description,bonus));
  if(elemental)assert(strstr(delivered->short_description,damtypes[elements[element_roll]]));
  free(delivered->name);free(delivered->short_description);free(delivered->description);free(delivered);delivered=NULL;
 }
 // Grade fallback is independent of recipient; clamp invalid/extreme inputs.
 struct obj_data plain={0};rarity=0;
 for(int g=-1;g<=8;g++){set_random_ammo_bonuses(&plain,g,0,0);assert(GET_OBJ_VAL(&plain,4)==MAX(1,MIN(6,g)));}
 // Rarity probability boundaries are inclusive and consume one type roll only on success.
 for(rarity=1;rarity<=3;rarity++)for(int boundary=0;boundary<2;boundary++) {
  int threshold=rarity==1?50:rarity==2?75:100;
  chance_roll=threshold+boundary;if(chance_roll>100)continue;
  draws=1;element_roll=0;set_random_ammo_bonuses(&plain,2,12,rarity);
  assert((GET_OBJ_VAL(&plain,AMMO_VAL_DAMAGE_DIE)>0)==(boundary==0));
 }
 expected_level=12;
 reads=awards=draws=0;missing=true;delivered=NULL;
 award_magic_ammo(&ch,2,12);assert(reads==1 && !awards && !draws && !delivered);
 return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='random-ammo-') as tmp:
 p=Path(tmp)/'test.c';b=Path(tmp)/'test';p.write_text(headers+'\n'+stubs+code+checks)
 subprocess.run(['cc','-std=gnu99','-fsanitize=address,undefined','-I'+str(root/'src'),str(p),'-o',str(b)],check=True)
 subprocess.run([str(b)],check=True)
print('Random ammo: level tiers, all rarities/types, elemental chance boundaries, damage dice, visible bonuses, grade fallback, one 50-use object and missing-template handling passed.')
