#!/usr/bin/env python3
"""Run production throw sources, stats, command, shot lifetime and hybrid routing."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1]
w=(root/'src/assign_wpn_armor.c').read_text();f=(root/'src/fight.c').read_text();o=(root/'src/act.offensive.c').read_text()
def function(text,sig):
 a=text.index(sig);return text[a:text.index('\n}',a)+2]
headers='\n'.join('#include "'+h+'"' for h in ['conf.h','sysdep.h','structs.h','utils.h','spells.h','fight.h','assign_wpn_armor.h','spec_abilities.h','mud_event.h','actions.h','clan.h'])
code=headers+r'''
#include <assert.h>
#undef CHECK_PLAYER_SPECIAL
#define CHECK_PLAYER_SPECIAL(ch,var) (var)
#undef GET_OBJ_VNUM
#define GET_OBJ_VNUM(obj) ((obj)->item_number)
#undef ACMD
#define ACMD(name) void name(struct char_data *ch,const char *argument,int cmd,int subcmd)
#define act(...) ((void)0)
#define send_to_char(...) ((void)0)
#define PREREQ_NOT_NPC() do {if(IS_NPC(ch))return;}while(0)
#define PREREQ_CAN_FIGHT() ((void)0)
#define PREREQ_NOT_PEACEFUL_ROOM() ((void)0)
#define one_argument(text,buf,len) snprintf(buf,len,"%s",text)
#define get_char_vis(...) selected
#define is_using_double_weapon(...) false
#define HIT_MISS 0
#define NORMAL_ATTACK_ROUTINE 0
static struct char_data *selected;
static int feats[NUM_FEATS], shots, removed, distance=50, expected_type;
static bool returning, rejected, move_ready=true, swift_ready=true;
static int move_used,swift_used,subtype_choice,weapon_choice;
int rand_number(int low,int high) {if(low==0 && high==5)return subtype_choice;int value=weapon_choice++;if(weapon_choice>high)weapon_choice=low;return value;}
int get_feat_value(struct char_data *ch,int feat) {return feats[feat];}
int MAX(int a,int b) {return a>b?a:b;}
int MIN(int a,int b) {return a<b?a:b;}
struct weapon_table weapon_list[NUM_WEAPON_TYPES];
const char *weapon_type[NUM_WEAPON_TYPES];
static struct obj_data ammo,pouch,held,other;
static struct obj_data *last_missile;
bool obj_has_special_ability(struct obj_data *obj,int ability) {return obj && returning && GET_OBJ_TYPE(obj)==ITEM_WEAPON && ability==WEAPON_SPECAB_RETURNING;}
bool is_action_available(struct char_data *ch,action_type type,bool msg) {return type==atMOVE?move_ready:type==atSWIFT?swift_ready:true;}
int combat_distance(struct char_data *ch,struct char_data *victim) {return distance;}
int combat_melee_range(struct char_data *ch,struct obj_data *weapon) {return 5;}
struct obj_data *create_obj(void) {return calloc(1,sizeof(struct obj_data));}
static void obj_from_obj(struct obj_data *obj) {
 struct obj_data **link=&obj->in_obj->contains;
 while(*link && *link!=obj)link=&(*link)->next_content;
 assert(*link==obj);*link=obj->next_content;obj->in_obj=NULL;obj->next_content=NULL;
}
static void obj_to_char(struct obj_data *obj,struct char_data *ch) {obj->carried_by=ch;}
static struct obj_data *unequip_char(struct char_data *ch,int where) {struct obj_data *obj=GET_EQ(ch,where);assert(obj);GET_EQ(ch,where)=NULL;obj->worn_by=NULL;return obj;}
static void extract_obj(struct obj_data *obj);
bool manyshot_ready(struct char_data *ch,struct obj_data *obj) {return false;}
static int resolve_weapon_hit(struct char_data *,struct char_data *,int,int,int,int);
'''
a=f.index('struct ranged_shot_context\n');b=f.index('bool manyshot_ready(',a)
code+=f[a:b]+'\n'
for sig in ['static void setweapon(', 'void initialize_weapons(', 'void load_weapons(', 'int thrown_weapon_type(', 'bool is_pure_thrown_weapon_type(', 'bool is_legacy_composite_bow(', 'bool ammunition_is_unlimited(', 'int ammunition_remaining(', 'struct obj_data *get_throwing_source(', 'bool throwing_allows_full_attack(', 'struct obj_data *make_thrown_weapon_profile(', 'struct obj_data *get_throwing_weapon(', 'bool is_throwing_weapon(', 'struct obj_data *take_ammo_projectile(']:
 code+=function(w,sig)+'\n'
treasure=(root/'src/treasure.c').read_text()
code+=function(treasure,'void set_ammo_object(')+'\n'
a=treasure.index('  do { roll = rand_number(1, NUM_WEAPON_TYPES - 1); }');b=treasure.index('\n',a)
code+='static int drop_weapon_type(void) {int roll;\n'+treasure[a:b]+'\nreturn roll;}\n'
a=w.index('  if (HAS_FEAT(ch, FEAT_WEAPON_PROFICIENCY_WIZARD)');b=w.index('  if (HAS_FEAT(ch, FEAT_WEAPON_PROFICIENCY_PSIONICIST)',a)
code+='static bool wizard_proficiency(struct char_data *ch,int weapon) {\n'+w[a:b]+'return false;}\n'
# The new can-fire branch is run without substituting a launcher for the held weapon.
a=w.index('  if (THROWING(ch))\n  {',w.index('bool can_fire_ammo('));b=w.index('  if (!(wielded',a)
code+='bool can_fire_ammo(struct char_data *ch,bool silent) {\n'+w[a:b]+'return false;}\n'
code+=function(f,'struct obj_data *get_wielded(struct char_data *ch, /* Wielder */')+'\n'
code+=function(f,'static void release_ranged_shot(')+'\n'+function(f,'static void finish_thrown_weapon(')+'\n'+function(f,'int hit(struct char_data *ch,')+'\n'
code+=r'''
static void extract_obj(struct obj_data *obj) {
 forget_ranged_projectile(obj);removed++;
 if(obj!=&ammo) {free(obj->name);free(obj->short_description);free(obj->description);free(obj);}
}
static int resolve_weapon_hit(struct char_data *ch,struct char_data *victim,int type,int damage,int penalty,int attack_type) {
 if(rejected)return HIT_MISS;
 assert(current_shot->thrown && get_wielded(ch,ATTACK_TYPE_RANGED));
 struct obj_data *weapon=get_wielded(ch,ATTACK_TYPE_RANGED);
 assert(GET_OBJ_TYPE(weapon)==ITEM_WEAPON && GET_OBJ_VAL(weapon,0)==expected_type);
 struct obj_data *supply=current_shot->throwing_source;
 current_shot->projectile=take_ammo_projectile(supply,weapon);
 current_shot->thrown_used=true;
 assert(GET_OBJ_VAL(get_wielded(ch,ATTACK_TYPE_RANGED),0)==expected_type);
 if(GET_OBJ_TYPE(supply)==ITEM_MISSILE)assert(ranged_ammunition(ch)==current_shot->projectile);
 else assert(!ranged_ammunition(ch));
 FIGHTING(ch)=victim;shots++;return 1;
}
'''
code+=function(o,'ACMD(do_throw)')+'\n'
code+=r"""
#undef USE_MOVE_ACTION
#undef USE_SWIFT_ACTION
#define USE_MOVE_ACTION(ch) (move_used++,move_ready=false)
#define USE_SWIFT_ACTION(ch) (swift_used++,swift_ready=false)
#define compute_ability(...) 0
#undef CAN_WEAR
#define CAN_WEAR(...) true
#undef OBJ_FLAGGED
#define OBJ_FLAGGED(...) false
#define perform_wear(ch,obj,where) do {(obj)->worn_by=(ch);GET_EQ(ch,where)=(obj);}while(0)
"""+function((root/'src/act.item.c').read_text(),'static bool perform_wield_at(')+'\n'

a=f.index('  if (mode == NORMAL_ATTACK_ROUTINE && THROWING(ch) && FIGHTING(ch))');b=f.index('  /* A standard action permits',a)
code+='static int route(struct char_data *ch) {int mode=NORMAL_ATTACK_ROUTINE;struct obj_data *wielded;\n'+f[a:b]+'return 1;}\n'
# Verify proficiency and focus use the selected weapon, and Dex/Strength remain split.
bonus=function(f,'int compute_attack_bonus_full(')
assert 'wielded = get_wielded(ch, attack_type);' in bonus
assert 'is_proficient_with_weapon(ch, GET_WEAPON_TYPE(wielded))' in bonus
assert 'calc_bab += GET_DEX_BONUS(ch);' in bonus
assert 'weapon_list[GET_OBJ_VAL(wielded, 0)].weaponFamily : WEAPON_FAMILY_RANGED' in bonus
damage=function(f,'int compute_damage_bonus(')
a=damage.index('    if (is_throwing_weapon(ch, wielded))');b=damage.index('    else',a)
assert 'dambonus += str_bonus;' in damage[a:b]
code+=r'''
int main(void) {
 load_weapons();
 assert(weapon_list[WEAPON_TYPE_THROWING_KNIFE].weaponFamily==WEAPON_FAMILY_SMALL_BLADE);
 assert(weapon_list[WEAPON_TYPE_JAVELIN].weaponFamily==WEAPON_FAMILY_POLEARM);
 assert(weapon_list[WEAPON_TYPE_THROWING_HAMMER].weaponFamily==WEAPON_FAMILY_HAMMER);
 assert(weapon_list[WEAPON_TYPE_THROWING_KNIFE].weaponFlags&WEAPON_FLAG_SIMPLE);
 assert(weapon_list[WEAPON_TYPE_THROWING_HAMMER].weaponFlags&WEAPON_FLAG_MARTIAL);
 int pure_types[]={WEAPON_TYPE_SHURIKEN,WEAPON_TYPE_DART,WEAPON_TYPE_THROWING_AXE,WEAPON_TYPE_THROWING_KNIFE,WEAPON_TYPE_JAVELIN,WEAPON_TYPE_THROWING_HAMMER};
 for(int i=0;i<6;i++) {
  struct obj_data generated={0};subtype_choice=i;set_ammo_object(&generated,AMMO_TYPE_THROWN);
  assert(thrown_weapon_type(&generated)==pure_types[i]);
  assert(GET_OBJ_VAL(&generated,AMMO_VAL_QUANTITY)==50 && GET_OBJ_TYPE(&generated)==ITEM_MISSILE);
  assert(GET_OBJ_MATERIAL(&generated)==weapon_list[pure_types[i]].material);
  weapon_choice=pure_types[i];int dropped=drop_weapon_type();assert(!is_pure_thrown_weapon_type(dropped) && !is_legacy_composite_bow(dropped));
 }
 struct char_data ch={0},victim={0};struct player_special_data specials={0};ch.player_specials=&specials;
 feats[FEAT_WEAPON_PROFICIENCY_WIZARD]=1;
 assert(wizard_proficiency(&ch,WEAPON_TYPE_THROWING_KNIFE) && !wizard_proficiency(&ch,WEAPON_TYPE_THROWING_HAMMER));
 selected=&victim;IN_ROOM(&ch)=IN_ROOM(&victim)=0;
 GET_OBJ_TYPE(&held)=ITEM_WEAPON;GET_OBJ_VAL(&held,0)=WEAPON_TYPE_LONG_SWORD;
 GET_EQ(&ch,WEAR_WIELD_1)=&held;
 do_throw(&ch,"target",0,0);assert(!THROWING(&ch) && shots==0);
 GET_EQ(&ch,WEAR_WIELD_1)=NULL;do_throw(&ch,"target",0,0);assert(!THROWING(&ch) && shots==0);
 GET_EQ(&ch,WEAR_AMMO_POUCH)=&pouch;pouch.worn_by=&ch;
 GET_OBJ_TYPE(&ammo)=ITEM_MISSILE;GET_OBJ_VAL(&ammo,0)=AMMO_TYPE_THROWN;
 GET_OBJ_VAL(&ammo,AMMO_VAL_WEAPON_TYPE)=WEAPON_TYPE_THROWING_KNIFE;
 GET_OBJ_VAL(&ammo,AMMO_VAL_MAX)=GET_OBJ_VAL(&ammo,AMMO_VAL_QUANTITY)=3;
 GET_OBJ_VAL(&ammo,4)=2;ammo.name="throwing knife";ammo.short_description="a throwing knife";ammo.description="A knife.";
 pouch.contains=&ammo;ammo.in_obj=&pouch;expected_type=WEAPON_TYPE_THROWING_KNIFE;
 do_throw(&ch,"target",0,0);assert(THROWING(&ch) && shots==1 && GET_OBJ_VAL(&ammo,AMMO_VAL_QUANTITY)==2);
 assert(GET_EQ(&ch,WEAR_WIELD_1)==NULL); // bare handed pouch throwing
 GET_EQ(&ch,WEAR_WIELD_1)=&held;
 assert(GET_OBJ_VAL(get_wielded(&ch,ATTACK_TYPE_RANGED),0)==WEAPON_TYPE_THROWING_KNIFE);
 assert(GET_OBJ_VAL(get_wielded(&ch,ATTACK_TYPE_RANGED),4)==2);
 hit(&ch,&victim,0,0,0,ATTACK_TYPE_RANGED);hit(&ch,&victim,0,0,0,ATTACK_TYPE_RANGED);
 assert(shots==3 && !pouch.contains && GET_EQ(&ch,WEAR_WIELD_1)==&held);
 assert(!route(&ch) && THROWING(&ch)); // exhausted at range: no melee or extra shots
 distance=5;assert(route(&ch) && !THROWING(&ch)); // depleted ammo permits melee immediately
 // Opening volley remains ranged through its first round while ammo lasts.
 GET_OBJ_VAL(&ammo,AMMO_VAL_MAX)=GET_OBJ_VAL(&ammo,AMMO_VAL_QUANTITY)=3;ammo.in_obj=&pouch;pouch.contains=&ammo;
 THROWING(&ch)=true;ch.char_specials.throw_opening_round=true;
 assert(route(&ch) && THROWING(&ch));ch.char_specials.throw_opening_round=false;
 assert(route(&ch) && !THROWING(&ch));
 // A nested shot must build from its own source, not inherit the outer type.
 THROWING(&ch)=true;
 struct ranged_shot_context outer={0};outer.attacker=&ch;outer.thrown=true;
 GET_OBJ_TYPE(&outer.throwing_profile)=ITEM_WEAPON;GET_OBJ_VAL(&outer.throwing_profile,0)=WEAPON_TYPE_THROWING_KNIFE;
 current_shot=&outer;GET_OBJ_VAL(&ammo,AMMO_VAL_WEAPON_TYPE)=WEAPON_TYPE_JAVELIN;
 expected_type=WEAPON_TYPE_JAVELIN;hit(&ch,&victim,0,0,0,ATTACK_TYPE_RANGED);
 assert(current_shot==&outer && GET_OBJ_VAL(active_throwing_weapon(&ch),0)==WEAPON_TYPE_THROWING_KNIFE);
 current_shot=NULL;
 // A real throwable weapon needs no pouch and is retained in inventory after a miss/hit.
 GET_EQ(&ch,WEAR_AMMO_POUCH)=NULL;GET_OBJ_VAL(&held,0)=WEAPON_TYPE_DAGGER;GET_OBJ_VAL(&held,1)=1;GET_OBJ_VAL(&held,2)=4;
 held.name="dagger";held.short_description="a dagger";held.description="A dagger.";
 held.worn_by=&ch;held.worn_on=WEAR_WIELD_1;THROWING(&ch)=true;expected_type=WEAPON_TYPE_DAGGER;distance=50;
 hit(&ch,&victim,0,0,0,ATTACK_TYPE_RANGED);
 assert(!GET_EQ(&ch,WEAR_WIELD_1) && held.carried_by==&ch && !get_throwing_source(&ch));
 // Returning remains wielded and supports a full attack without Quick Draw.
 held.carried_by=NULL;held.worn_by=&ch;GET_EQ(&ch,WEAR_WIELD_1)=&held;returning=true;
 assert(throwing_allows_full_attack(&ch));
 hit(&ch,&victim,0,0,0,ATTACK_TYPE_RANGED);hit(&ch,&victim,0,0,0,ATTACK_TYPE_RANGED);
 assert(GET_EQ(&ch,WEAR_WIELD_1)==&held && held.worn_by==&ch);
 returning=false;assert(!throwing_allows_full_attack(&ch));feats[FEAT_QUICK_DRAW]=1;assert(throwing_allows_full_attack(&ch));
 // A rejected/out-of-range attack must not unequip the weapon.
 rejected=true;hit(&ch,&victim,0,0,0,ATTACK_TYPE_RANGED);assert(GET_EQ(&ch,WEAR_WIELD_1)==&held);rejected=false;
 // Individual pouch weapons return to inventory; source extraction invalidates pending returns.
 GET_EQ(&ch,WEAR_WIELD_1)=NULL;held.worn_by=NULL;held.in_obj=&pouch;pouch.contains=&held;GET_EQ(&ch,WEAR_AMMO_POUCH)=&pouch;
 hit(&ch,&victim,0,0,0,ATTACK_TYPE_RANGED);assert(!pouch.contains && held.carried_by==&ch);
 struct ranged_shot_context context={0};context.attacker=&ch;context.throwing_source=&held;context.thrown_used=true;current_shot=&context;
 forget_ranged_projectile(&held);assert(!context.throwing_source);finish_thrown_weapon(&context);current_shot=NULL;
 // Re-wielding in combat costs move, or swift with Quick Draw, only on success.
 GET_EQ(&ch,WEAR_WIELD_1)=NULL;held.worn_by=NULL;feats[FEAT_QUICK_DRAW]=0;
 assert(perform_wield_at(&ch,&held,false,WEAR_WIELD_1));assert(move_used==1 && swift_used==0);
 assert(!perform_wield_at(&ch,&held,false,WEAR_WIELD_1));
 feats[FEAT_QUICK_DRAW]=1;GET_EQ(&ch,WEAR_WIELD_1)=NULL;held.worn_by=NULL;
 assert(perform_wield_at(&ch,&held,false,WEAR_WIELD_1));assert(swift_used==1);
 assert(!perform_wield_at(&ch,&held,false,WEAR_WIELD_1));
 for(int i=0;i<NUM_WEAPON_TYPES;i++)free((void*)weapon_type[i]);
 puts("PASS: barehand/melee pouch throwing, weapon stats/families/proficiency routing, Dex/Strength, final-ammo snapshots, exhaustion/replenishment, opening volley/melee transition, actual weapon inventory, Returning, Quick Draw and invalidated sources.");
}
'''
with tempfile.TemporaryDirectory(prefix='throwing-combat-') as directory:
 p=Path(directory);(p/'test.c').write_text(code)
 subprocess.run(['cc','-std=gnu99','-g','-fsanitize=address,undefined','-fno-pie','-no-pie','-I'+str(root/'src'),str(p/'test.c'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
