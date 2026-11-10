#!/usr/bin/env python3
"""Exercise hoopak mode selection, ammunition, distance transitions and persistence."""
from pathlib import Path
import subprocess
import tempfile
root=Path(__file__).resolve().parents[1]
weapons=(root/'src/assign_wpn_armor.c').read_text()
fight=(root/'src/fight.c').read_text()
offense=(root/'src/act.offensive.c').read_text()
players=(root/'src/players.c').read_text()
def function(text,sig):
    a=text.index(sig)
    return text[a:text.index('\n}',a)+2]
headers='\n'.join('#include "'+h+'"' for h in ['conf.h','sysdep.h','structs.h','utils.h','spells.h','fight.h','assign_wpn_armor.h'])
code=headers+r'''
#include <assert.h>
#undef CHECK_PLAYER_SPECIAL
#define CHECK_PLAYER_SPECIAL(ch,var) (var)
#undef CREATE
#define CREATE(ptr,type,count) do {(ptr)=calloc(count,sizeof(type));assert(ptr);}while(0)
#define act(...) ((void)0)
#define obj_has_special_ability(...) false
#define is_using_double_weapon(...) false
#undef ACMD
#undef GET_OBJ_VNUM
#define GET_OBJ_VNUM(obj) ((obj)->item_number)
#define ACMD(name) void name(struct char_data *ch,const char *argument,int cmd,int subcmd)
#define PREREQ_NOT_NPC() do {if(IS_NPC(ch))return;}while(0)
#define one_argument(text,buf,len) snprintf(buf,len,"%s",text)
#define str_cmp strcasecmp
static int saves;
#define get_throwing_source(...) NULL
#define get_throwing_weapon(...) NULL
#define save_char(...) (saves++)
static char output[4096];
#define send_to_char(ch,...) snprintf(output,sizeof(output),__VA_ARGS__)
struct weapon_table weapon_list[NUM_WEAPON_TYPES];
struct room_data rooms[1],*world=rooms;
room_rnum top_of_world=0;
int get_room_size(const struct room_data *room) {return 50;}
int MAX(int a,int b) {return a>b?a:b;}
int MIN(int a,int b) {return a<b?a:b;}
static struct obj_data *shot;
#define ranged_ammunition(ch) shot
'''
for sig in ['int current_weapon_type(', 'bool is_throwing_weapon(', 'bool ammunition_is_unlimited(', 'bool usable_ammunition(', 'bool hoopak_uses_ranged(', 'int ranged_weapon_melee_type(', 'bool has_ammo_in_pouch(', 'struct obj_data *is_using_ranged_weapon(', 'bool can_fire_ammo(']:
    code+=function(weapons,sig)+'\n'
a=fight.index('/* Distances are symmetric');b=fight.index('/* Departure/extraction clears',a)
code+=fight[a:b]+'\n'
code+=function(fight,'struct obj_data *get_wielded(struct char_data *ch, /* Wielder */')+'\n'
code+=function(fight,'static bool empty_launcher_melee_profile(')+'\n'
code+=function(offense,'ACMD(do_hoopak)')+'\n'
# Exercise production load validation and save formatting as well.
a=players.index('        if (!strcmp(tag, "HpMd"))');b=players.index('        if (!strcmp(tag, "Hous"))',a)
code+='void load_mode(struct char_data *ch,const char *line) {const char *tag="HpMd"; switch(0) {case 0:\n'+players[a:b]+'}}\n'
assert 'GET_HOOPAK_MODE(ch) = HOOPAK_MODE_HYBRID;' in players
assert 'BUFFER_WRITE("HpMd: %d\\n", GET_HOOPAK_MODE(ch));' in players
assert 'do_hoopak, 1, 0, TRUE, ACTION_NONE' in (root/'src/interpreter.c').read_text()
# The runtime routine refreshes firing before dispatching the single-attack path.
routine=function(fight,'int perform_attacks(')
assert routine.index('FIRING(ch) = can_fire_ammo(ch, TRUE);') < routine.index('return perform_staggered_attack(')
code+=r'''
int main(void) {
 struct char_data ch={0},mob={0};struct player_special_data specials={0};
 struct obj_data hoopak={0},pouch={0},ammo={0},wrong={0},profile;
 ch.player_specials=&specials;IN_ROOM(&ch)=IN_ROOM(&mob)=0;
 SET_BIT_AR(MOB_FLAGS(&mob),MOB_ISNPC);
 GET_OBJ_TYPE(&hoopak)=ITEM_WEAPON;GET_OBJ_VAL(&hoopak,0)=WEAPON_TYPE_HOOPAK;
 GET_OBJ_VAL(&hoopak,1)=1;GET_OBJ_VAL(&hoopak,2)=6;hoopak.tinker_bonus=3;
 GET_EQ(&ch,WEAR_WIELD_1)=&hoopak;
 GET_OBJ_TYPE(&ammo)=GET_OBJ_TYPE(&wrong)=ITEM_MISSILE;
 GET_OBJ_VAL(&ammo,0)=AMMO_TYPE_STONE;GET_OBJ_VAL(&ammo,AMMO_VAL_MAX)=20;
 GET_OBJ_VAL(&ammo,AMMO_VAL_QUANTITY)=10;
 GET_OBJ_VAL(&wrong,0)=AMMO_TYPE_ARROW;
 for(int mode=0;mode<3;mode++) {
  GET_HOOPAK_MODE(&ch)=mode;GET_EQ(&ch,WEAR_AMMO_POUCH)=NULL;
  assert(!is_using_ranged_weapon(&ch,true));assert(!can_fire_ammo(&ch,true));
  assert(ranged_weapon_melee_type(&ch,&hoopak)==WEAPON_TYPE_HOOPAK);
  GET_EQ(&ch,WEAR_AMMO_POUCH)=&pouch;pouch.contains=NULL;
  assert(!can_fire_ammo(&ch,true));pouch.contains=&wrong;
  assert(!can_fire_ammo(&ch,true));pouch.contains=&ammo;
  GET_OBJ_VAL(&ammo,AMMO_VAL_QUANTITY)=0;assert(!can_fire_ammo(&ch,true));
  GET_OBJ_VAL(&ammo,AMMO_VAL_QUANTITY)=10;
  assert(can_fire_ammo(&ch,true)==(mode!=HOOPAK_MODE_MELEE));
  assert(combat_distance(&ch,&mob)==(mode==HOOPAK_MODE_MELEE?0:50));
  struct obj_data *effective=&hoopak;
  assert(!empty_launcher_melee_profile(&ch,&effective,&profile,ATTACK_TYPE_PRIMARY));
  assert(effective==&hoopak && GET_OBJ_VAL(&hoopak,2)==6 && hoopak.tinker_bonus==3);
 }
 GET_HOOPAK_MODE(&ch)=HOOPAK_MODE_HYBRID;
 FIGHTING(&ch)=&mob;FIGHTING(&mob)=&ch;
 start_combat_distance(&ch,&mob);assert(combat_distance(&ch,&mob)==50);
 assert(can_fire_ammo(&ch,true));
 struct combat_distance_data *entry=find_combat_distance(&ch,&mob);assert(entry);
 entry->distance=6;assert(can_fire_ammo(&ch,true));
 entry->distance=5;assert(!can_fire_ammo(&ch,true));
 assert(ranged_weapon_melee_type(&ch,&hoopak)==WEAPON_TYPE_HOOPAK);
 GET_HOOPAK_MODE(&ch)=HOOPAK_MODE_RANGED;assert(can_fire_ammo(&ch,true));
 GET_HOOPAK_MODE(&ch)=HOOPAK_MODE_MELEE;entry->distance=50;assert(!can_fire_ammo(&ch,true));
 GET_HOOPAK_MODE(&ch)=HOOPAK_MODE_HYBRID;assert(can_fire_ammo(&ch,true));
 entry->distance=0;assert(!can_fire_ammo(&ch,true));
 entry->distance=20;assert(can_fire_ammo(&ch,true));
 // Last stone exhaustion preserves the existing distance and permits native melee.
 GET_OBJ_VAL(&ammo,AMMO_VAL_QUANTITY)=0;
 assert(!can_fire_ammo(&ch,true) && combat_distance(&ch,&mob)==20);
 assert(ranged_weapon_melee_type(&ch,get_wielded(&ch,ATTACK_TYPE_PRIMARY))==WEAPON_TYPE_HOOPAK);
 // Player selection persists, accepts A/B/C, and invalid input changes nothing.
 const char *commands[]={"ranged","hybrid","melee","a","b","c"};
 int modes[]={HOOPAK_MODE_RANGED,HOOPAK_MODE_HYBRID,HOOPAK_MODE_MELEE,HOOPAK_MODE_RANGED,HOOPAK_MODE_HYBRID,HOOPAK_MODE_MELEE};
 for(int i=0;i<6;i++){do_hoopak(&ch,commands[i],0,0);assert(GET_HOOPAK_MODE(&ch)==modes[i]);}
 assert(saves==6);do_hoopak(&ch,"invalid",0,0);assert(saves==6 && GET_HOOPAK_MODE(&ch)==HOOPAK_MODE_MELEE);
 do_hoopak(&ch,"",0,0);assert(strstr(output,"Hoopak mode: melee"));
 load_mode(&ch,"1");assert(GET_HOOPAK_MODE(&ch)==HOOPAK_MODE_RANGED);
 load_mode(&ch,"2");assert(GET_HOOPAK_MODE(&ch)==HOOPAK_MODE_MELEE);
 load_mode(&ch,"-1");assert(GET_HOOPAK_MODE(&ch)==HOOPAK_MODE_HYBRID);
 load_mode(&ch,"99");assert(GET_HOOPAK_MODE(&ch)==HOOPAK_MODE_HYBRID);
 free(combat_distances);combat_distances=NULL;
 puts("PASS: hoopak A/B/C modes, empty/missing/mismatched/depleted ammo, native melee without improvised penalties, opening distance, hybrid transitions, persistent distance, command aliases and save/load validation.");
}
'''
with tempfile.TemporaryDirectory(prefix='hoopak-modes-') as directory:
    p=Path(directory);(p/'test.c').write_text(code)
    subprocess.run(['cc','-std=gnu99','-g','-fsanitize=address,undefined','-fno-pie','-no-pie',
                    '-I'+str(root/'src'),str(p/'test.c'),'-o',str(p/'test')],check=True)
    subprocess.run([str(p/'test')],check=True)
