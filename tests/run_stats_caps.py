#!/usr/bin/env python3
"""Check production stat-cap reporting against enforcement with real game structures."""
from pathlib import Path
import os
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
src = root / "src"
handler = (src / "handler.c").read_text()
fight = (src / "fight.c").read_text()
other = (src / "act.informative.c").read_text()
def function(text, signature):
    a = text.index(signature)
    return text[a:text.index("\n}", a) + 2]
a = handler.index("#define BASE_STAT_CAP ")
b = handler.index("#undef BASE_STAT_CAP", a)
cap_code = handler[a:b]
headers = '\n'.join('#include "'+name+'"' for name in
                    ['conf.h','sysdep.h','structs.h','utils.h','comm.h','db.h','handler.h',
                     'spells.h','fight.h','mud_event.h','assign_wpn_armor.h'])
stubs = r'''
#include <assert.h>
int MIN(int a,int b) { return a<b?a:b; }
int MAX(int a,int b) { return a>b?a:b; }
struct config_data config_info;
struct weapon_table weapon_list[NUM_WEAPON_TYPES];
static char output[20000];
static size_t used;
static bool raging, battletide, sacred, vanish, improved, protected_target;
size_t send_to_char(struct char_data *ch,const char *fmt,...) {
  va_list ap;va_start(ap,fmt);
  used+=vsnprintf(output+used,sizeof(output)-used,fmt,ap);va_end(ap);
  assert(used<sizeof(output));
  return used;
}
void text_line(struct char_data *ch,const char *text,int n,char a,char b) {}
void draw_line(struct char_data *ch,int n,char a,char b) {}
int compute_current_size(struct char_data *ch) { return SIZE_MEDIUM; }
bool affected_by_spell(struct char_data *ch,int spell) {
  if (spell==SKILL_RAGE)return raging;
  if (spell==SPELL_BATTLETIDE)return battletide;
  if (spell==SKILL_SACRED_FLAMES)return sacred;
  if (spell==SPELL_IRONSKIN || spell==SPELL_EPIC_WARDING)return protected_target;
  return false;
}
int get_feat_value(struct char_data *ch,int feat) { return feat==FEAT_IMPROVED_VANISH && improved; }
struct mud_event_data *char_has_mud_event(struct char_data *ch,event_id id) {
  static struct mud_event_data event;
  return id==eVANISH && vanish ? &event : NULL;
}
int compute_attack_bonus(struct char_data *ch,struct char_data *vict,int kind) {
  return kind==ATTACK_TYPE_RANGED ? 25 : 30;
}
int formation_ranged_penalty(struct char_data *ch) { return -8; }
int compute_armor_class(struct char_data *attacker,struct char_data *ch,int touch,int mode) { return 35; }
int compute_mag_saves(struct char_data *ch,int type,int modifier) {
  return modifier==MAX_GOLD ? 8 : MIN(99,8+GET_SAVE(ch,type));
}
int compute_concealment(struct char_data *ch,struct char_data *attacker) { return 20; }
#undef ACMD
#define ACMD(name) void name(struct char_data *ch,const char *argument,int cmd,int subcmd)
'''
checks = r'''
static void reset_output(void) { used=0; output[0]=0; }
static void row(const char *name,int current,int cap) {
  const char *p=strstr(output,name);int actual,limit;
  assert(p && sscanf(p+strlen(name),"%d %d",&actual,&limit)==2);
  assert(actual==current && limit==cap);
}
int main(void) {
  struct char_data *ch=calloc(1,sizeof(*ch));
  struct player_special_data *special=calloc(1,sizeof(*special));
  assert(ch && special);ch->player_specials=special;
  struct char_data before;
  struct player_special_data *saved=malloc(sizeof(*saved));assert(saved);
  GET_REAL_STR(ch)=GET_REAL_DEX(ch)=GET_REAL_CON(ch)=18;
  GET_REAL_INT(ch)=GET_REAL_WIS(ch)=GET_REAL_CHA(ch)=18;
  ch->aff_abils.str=ch->aff_abils.dex=ch->aff_abils.con=30;
  GET_INT(ch)=GET_WIS(ch)=GET_CHA(ch)=30;
  GET_REAL_MAX_PSP(ch)=100;GET_MAX_PSP(ch)=300;
  GET_REAL_MAX_MOVE(ch)=500;GET_MAX_MOVE(ch)=900;
  GET_MAX_HIT(ch)=700;ch->points.size=SIZE_SMALL;
  config_info.player_config.armor_class_cap=65;
  CLASS_LEVEL(ch,CLASS_WARRIOR)=20;CLASS_LEVEL(ch,CLASS_WIZARD)=10;
  before=*ch;*saved=*special;
  do_stats(ch,"",0,0);
  assert(!memcmp(&before,ch,sizeof(*ch)) && !memcmp(saved,special,sizeof(*special)));
  row("STR",30,46);row("CON",30,49);row("DEX",30,43);
  row("INT",30,43);row("WIS",30,40);row("CHA",30,40);
  row("Attack bonus (melee)",30,55);row("Armor class",35,65);
  row("Save - Fortitude",8,23);row("Maximum PSP",300,700);row("Maximum movement",900,4500);
  assert(strstr(output,"No fixed cap") && strstr(output,"Total damage bonus"));
  struct obj_data bow={0};GET_OBJ_TYPE(&bow)=ITEM_WEAPON;GET_OBJ_VAL(&bow,0)=1;
  weapon_list[1].weaponFlags=WEAPON_FLAG_RANGED;GET_EQ(ch,WEAR_WIELD_1)=&bow;
  reset_output();do_stats(ch,"",0,0);row("Attack bonus (ranged)",25,47);
  /* Class caps displayed are the same values enforced on over-cap attributes. */
  ch->aff_abils.str=ch->aff_abils.dex=ch->aff_abils.con=90;GET_INT(ch)=GET_WIS(ch)=GET_CHA(ch)=90;
  compute_char_cap(ch,0);
  assert(GET_STR(ch)==46 && GET_CON(ch)==49 && GET_DEX(ch)==43);
  assert(GET_INT(ch)==43 && GET_WIS(ch)==40 && GET_CHA(ch)==40);
  memset(special->saved.class_level,0,sizeof(special->saved.class_level));
  CLASS_LEVEL(ch,CLASS_BERSERKER)=30;raging=true;battletide=true;sacred=true;
  reset_output();do_stats(ch,"",0,0);
  row("STR",46,60);row("CON",49,60);row("Hitroll (stat)",0,60);row("Damroll (stat)",0,80);
  vanish=true;improved=true;reset_output();do_stats(ch,"",0,0);row("Concealment (%)",20,150);
  /* Shared NPC attack cap follows existing powerful-being exceptions. */
  struct char_data npc={0}, target={0};struct char_data *n=&npc;
  SET_BIT_AR(MOB_FLAGS(n),MOB_ISNPC);FIGHTING(n)=&target;
  GET_LEVEL(n)=31;assert(get_attack_bonus_cap(n)==MAX_BAB+2);
  GET_LEVEL(n)=34;assert(get_attack_bonus_cap(n)==MAX_BAB+7);
  protected_target=true;assert(get_attack_bonus_cap(n)==MAX_BAB);
  protected_target=false;FIGHTING(n)=NULL;assert(get_attack_bonus_cap(n)==MAX_BAB);
  assert(get_attack_bonus_cap(NULL)==MAX_BAB);
  reset_output();do_stats(n,"",0,0);assert(!used);
  free(saved);free(special);free(ch);
  puts("PASS: real character structures, class/effect caps, report/enforcement agreement, read-only stats and combat cap exceptions.");
}
'''
code=headers+'\n'+stubs+function(fight,'int get_attack_bonus_cap(')+'\n'+cap_code
code+='\n'+function(other,'ACMD(do_stats)')+'\n'+checks
with tempfile.TemporaryDirectory(prefix='stats-caps-') as directory:
    path=Path(directory)/'stats.c';path.write_text(code)
    binary=Path(directory)/'stats'
    subprocess.run([os.environ.get('CC','cc'),'-std=gnu99','-g','-Wall','-Wextra','-Wno-unused-parameter',
                    '-fsanitize=address,undefined','-fno-omit-frame-pointer','-I'+str(src),
                    str(path),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
