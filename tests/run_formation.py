#!/usr/bin/env python3
"""Exercise production formation rules and combat hooks with isolated game stubs."""
from pathlib import Path
import os
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
src = root / "src"
handler = (src / "handler.c").read_text()
other = (src / "act.other.c").read_text()
fight = (src / "fight.c").read_text()
weapons = (src / "assign_wpn_armor.c").read_text()

def function(text, name):
    start = text.index(name)
    return text[start:text.index("\n}", start) + 2]

constants = "\n".join(line for line in (src / "structs.h").read_text().splitlines()
                      if re.match(r"#define (FORMATION_\w+|NUM_FORMATION_ROWS|SIZE_\w+|POS_\w+|"
                                  r"ATTACK_TYPE_\w+|WEAPON_FLAG_(REACH|RANGED)|NUM_WEAPON_TYPES|"
                                  r"ITEM_WEAPON|GROUP_NPC|FEAT_(POINT_BLANK_SHOT|PRECISE_SHOT|IMPROVED_PRECISE_SHOT))\s", line))
constants += "\n" + next(line for line in (src / "spells.h").read_text().splitlines()
                          if line.startswith("#define TYPE_ATTACK_OF_OPPORTUNITY "))
constants += "\n" + next(line for line in fight.splitlines() if line.startswith("#define HIT_MISS "))
a = handler.index("/* Tactical group formation.")
rules = handler[a:handler.index("/* Group Handlers */", a)]
commands = function(other, "ACMD(do_formation)")
a = fight.index("  /* Enforce formation before consuming ammo")
b = fight.index("  /* hitting pets:", a)
hit_hook = "static struct char_data *prepare_hit(struct char_data *ch, struct char_data *victim, int type, int attack_type, struct obj_data *wielded) {\n" + fight[a:b] + "return victim;\n}\n"

a = fight.index("  if (attack_type == ATTACK_TYPE_RANGED || attack_type == ATTACK_TYPE_BOMB_TOSS)\n    formation_penalty")
bonus_hook = "static int formation_bonus_hook(struct char_data *ch, int attack_type, bool display, int calc_bab, int maximum_bab) { int formation_penalty=0;\n" + fight[a:fight.index("\n}", a)] + "\n}\n"
stubs = r'''
#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#define TRUE true
#define FALSE false
#define NOWHERE (-1)
#define MAX_INPUT_LENGTH 512
#define MIN(a,b) ((a)<(b)?(a):(b))
#define IS_SET(a,b) ((a)&(b))
#define SET_BIT(a,b) ((a)|=(b))
#define REMOVE_BIT(a,b) ((a)&=~(b))
#define GROUP(ch) ((ch)->group)
#define GROUP_FLAGS(group) ((group)->group_flags)
#define GROUP_LEADER(group) ((group)->leader)
#define IS_NPC(ch) ((ch)->npc)
#define IN_ROOM(ch) ((ch)->in_room)
#define GET_POS(ch) ((ch)->pos)
#define GET_HIT(ch) ((ch)->hp)
#define GET_SIZE(ch) ((ch)->size)
#define GET_NAME(ch) ((ch)->name)
#define GET_OBJ_TYPE(o) ((o)->type)
#define GET_OBJ_VAL(o,i) ((o)->value[i])
#define CAN_SEE(ch,v) ((v)->visible)
#define FIGHTING(ch) ((ch)->fighting)
#define HAS_FEAT(ch,f) ((f)==FEAT_POINT_BLANK_SHOT ? (ch)->point_blank : (f)==FEAT_PRECISE_SHOT ? (ch)->precise : (ch)->improved_precise)
#define FIRING(ch) ((ch)->firing)
#define GET_ATTACKS_THIS_ROUND(ch) ((ch)->attacks)
#define IS_WILDSHAPED(ch) ((ch)->transformed)
#define IS_MORPHED(ch) ((ch)->transformed)
#define WEAR_WIELD_1 0
#define WEAR_WIELD_2H 1
#define GET_EQ(ch,i) ((ch)->equipment[i])
#define ACMD(name) void name(struct char_data *ch, const char *argument, int cmd, int subcmd)
#define atMOVE 0
#define eMSDP_GROUP 0
#define USE_MOVE_ACTION(ch) ((ch)->move_ready=false, moves++)
struct char_data;
struct obj_data { int type, value[1]; };
struct list_data { struct char_data *members[8]; int iSize; };
struct group_data { struct char_data *leader; struct list_data *members; int group_flags; };
struct char_data {
  struct group_data *group;
  int formation_row, in_room, pos, hp, size, attacks;
  const char *name;
  bool npc, visible, firing, transformed, move_ready, preserve_organs_procced;
  bool point_blank, precise, improved_precise;
  struct char_data *fighting, *next_in_room;
  struct obj_data *equipment[2];
  void *desc;
};
struct iterator_data { struct list_data *list; int index; };
static struct { struct char_data *people; } world[3];
static struct { int weaponFlags; } weapon_list[NUM_WEAPON_TYPES];
static int moves, msdp_updates, ammo_checks;
static char output[1024];
static void strlcpy(char *d, const char *s, size_t n) {
  if (n) { size_t copy=strlen(s); if(copy>=n)copy=n-1; memcpy(d,s,copy); d[copy]=0; }
}
static void skip_spaces_c(const char **s) { while(isspace((unsigned char)**s)) ++*s; }
static const char *one_argument(const char *s, char *d, size_t n) {
  size_t i=0; skip_spaces_c(&s);
  while(*s && !isspace((unsigned char)*s)) { if(i+1<n)d[i++]=*s; s++; }
  d[i]=0;skip_spaces_c(&s);return s;
}
static bool is_abbrev(const char *s,const char *t) { return *s && !strncasecmp(s,t,strlen(s)); }
static void send_to_char(struct char_data *ch,const char *fmt,...) {
  va_list args;va_start(args,fmt);vsnprintf(output,sizeof(output),fmt,args);va_end(args);
}
static void draw_line(struct char_data *ch, int n, char c, char end) {}
static void send_to_group(struct char_data *ch,struct group_data *g,const char *fmt,...) {
  va_list args;va_start(args,fmt);vsnprintf(output,sizeof(output),fmt,args);va_end(args);
}
static void *merge_iterator(struct iterator_data *it,struct list_data *list) {
  it->list=list;it->index=0;return list->iSize?list->members[0]:NULL;
}
static void *next_in_list(struct iterator_data *it) {
  return ++it->index<it->list->iSize?it->list->members[it->index]:NULL;
}
static void remove_iterator(struct iterator_data *it) {}
static void add_to_list(struct char_data *ch,struct list_data *list) { list->members[list->iSize++]=ch; }
static void remove_from_list(struct char_data *ch,struct list_data *list) {
  for(int i=0;i<list->iSize;i++) if(list->members[i]==ch) {
    for(int j=i+1;j<list->iSize;j++)
      list->members[j-1]=list->members[j];
    list->iSize--;
    return;
  }
}
static void *random_from_list(struct list_data *list) { return list->members[0]; }
static void free_group(struct group_data *g) {}
static void update_msdp_group(struct char_data *ch) { msdp_updates++; }
static void MSDPFlush(void *d,int key) {}
static bool is_action_available(struct char_data *ch,int action,bool msg) { return ch->move_ready; }
static struct char_data *get_char_room_vis(struct char_data *ch,char *name,int *number) {
  if(!strcasecmp(name,"self") || !strcasecmp(name,"me"))return ch;
  for(struct char_data *v=world[ch->in_room].people;v;v=v->next_in_room)
    if(v->visible && !strcasecmp(name,v->name))return v;
  return NULL;
}
static struct obj_data *is_using_ranged_weapon(struct char_data *ch,bool silent) { return ch->equipment[0]; }
static bool has_ammo_in_pouch(struct char_data *ch,struct obj_data *weapon,bool silent) { ammo_checks++;return true; }
'''

checks = r'''
static void command(struct char_data *ch,const char *s) { do_formation(ch,s,0,0); }
int main(void) {
  struct list_data members={0};
  struct group_data group={.members=&members};
  struct char_data front={.in_room=0,.pos=POS_STANDING,.hp=100,.size=SIZE_MEDIUM,.name="front",.visible=true,.move_ready=true};
  struct char_data middle=front, back=front, npc=front, outsider=front;
  middle.name="middle"; back.name="back"; npc.npc=true;npc.name="enemy";
  front.next_in_room=&middle; middle.next_in_room=&back;back.next_in_room=&npc;
  world[0].people=&front;
  join_group(&front,&group);join_group(&middle,&group);join_group(&back,&group);
  middle.formation_row=FORMATION_MIDDLE;back.formation_row=FORMATION_BACK;
  struct obj_data sword={ITEM_WEAPON,{1}}, spear={ITEM_WEAPON,{2}}, bow={ITEM_WEAPON,{3}}, invalid={ITEM_WEAPON,{NUM_WEAPON_TYPES}};
  weapon_list[2].weaponFlags=WEAPON_FLAG_REACH;weapon_list[3].weaponFlags=WEAPON_FLAG_RANGED;
  for(int row=0;row<NUM_FORMATION_ROWS;row++) {
    front.formation_row=row;
    assert(formation_allows_attack(&front,ATTACK_TYPE_PRIMARY,&sword)==(row==FORMATION_FRONT));
    assert(formation_allows_attack(&front,ATTACK_TYPE_OFFHAND,&spear)==(row!=FORMATION_BACK));
    assert(formation_allows_attack(&front,ATTACK_TYPE_RANGED,&bow)==true);
    assert(formation_allows_attack(&front,ATTACK_TYPE_BOMB_TOSS,NULL)==true);
    assert(formation_allows_attack(&front,ATTACK_TYPE_UNARMED,NULL)==(row==FORMATION_FRONT));
    assert(formation_allows_attack(&front,ATTACK_TYPE_PRIMARY_EVO_BITE,&spear)==(row==FORMATION_FRONT));
    assert(formation_allows_attack(&front,ATTACK_TYPE_PSIONICS,NULL));
    assert(formation_allows_attack(&front,ATTACK_TYPE_ELDRITCH_BLAST,NULL));
  }
  front.formation_row=FORMATION_FRONT;
  for(int feats=0;feats<8;feats++) {
    front.point_blank=feats&1;front.precise=feats&2;front.improved_precise=feats&4;
    int expected=(front.point_blank?0:-4)+(front.precise || front.improved_precise?0:-4);
    assert(formation_ranged_penalty(&front)==expected);
    assert(formation_bonus_hook(&front,ATTACK_TYPE_RANGED,false,40,30)==30+expected);
    assert(formation_bonus_hook(&front,ATTACK_TYPE_BOMB_TOSS,true,40,30)==30+expected);
    assert(formation_bonus_hook(&front,ATTACK_TYPE_PRIMARY,false,40,30)==30);
    middle.point_blank=front.point_blank;middle.precise=front.precise;
    assert(formation_ranged_penalty(&middle)==0 && formation_ranged_penalty(&back)==0);
  }
  assert(formation_ranged_penalty(&outsider)==0);
  front.point_blank=false;front.precise=false;front.improved_precise=false;
  for(int size=SIZE_FINE;size<=SIZE_COLOSSAL;size++) {
    npc.size=size;
    assert(formation_can_melee_target(&npc,&front));
    assert(formation_can_melee_target(&npc,&middle)==(size>=SIZE_LARGE));
    assert(formation_can_melee_target(&npc,&back)==(size>=SIZE_HUGE));
  }
  npc.size=SIZE_MEDIUM; npc.fighting=&back; npc.attacks=1;
  assert(!is_tanking(&back));
  assert(prepare_hit(&npc,&back,0,ATTACK_TYPE_PRIMARY,NULL)==&front);
  assert(npc.fighting==&front && is_tanking(&front));
  npc.fighting=&back;
  assert(!prepare_hit(&npc,&back,TYPE_ATTACK_OF_OPPORTUNITY,ATTACK_TYPE_PRIMARY,NULL));
  assert(npc.fighting==&back);
  assert(prepare_hit(&npc,&back,0,ATTACK_TYPE_RANGED,&bow)==&back);
  assert(prepare_hit(&npc,&back,0,ATTACK_TYPE_ELDRITCH_BLAST,NULL)==&back);
  front.visible=false;
  assert(!formation_melee_target(&npc,&back));front.visible=true;
  front.pos=POS_SLEEPING;
  assert(formation_can_melee_target(&npc,&middle));assert(!formation_can_melee_target(&npc,&back));
  npc.size=SIZE_LARGE;assert(formation_can_melee_target(&npc,&back));npc.size=SIZE_MEDIUM;
  front.pos=POS_STANDING;front.hp=0;
  assert(formation_can_melee_target(&npc,&middle));assert(!formation_can_melee_target(&npc,&back));
  front.hp=100;front.in_room=1;world[0].people=&middle;
  assert(formation_can_melee_target(&npc,&middle));assert(!formation_can_melee_target(&npc,&back));
  middle.pos=POS_STUNNED;
  assert(formation_can_melee_target(&npc,&back));middle.pos=POS_STANDING;
  front.in_room=0;world[0].people=&front;
  assert(formation_allows_attack(&outsider,ATTACK_TYPE_RANGED,&bow));
  assert(formation_can_melee_target(&npc,&outsider));
  assert(formation_can_melee_target(&front,&back)); /* NPC-only screening rule. */
  middle.equipment[0]=&spear;middle.attacks=1;
  assert(prepare_hit(&middle,&npc,0,ATTACK_TYPE_PRIMARY,&spear)==&npc);
  assert(!prepare_hit(&middle,&npc,0,ATTACK_TYPE_OFFHAND,&sword));
  assert(!formation_allows_attack(&middle,ATTACK_TYPE_PRIMARY,&invalid));
  assert(formation_melee_skill_allowed(&middle,&npc,true));
  assert(!formation_melee_skill_allowed(&middle,&npc,false));
  assert(!formation_melee_skill_allowed(&npc,&back,false));
  middle.transformed=true;assert(!formation_melee_skill_allowed(&middle,&npc,true));middle.transformed=false;
  front.equipment[0]=&bow;
  assert(can_fire_ammo(&front,true) && ammo_checks==1);
  middle.equipment[0]=&bow;assert(can_fire_ammo(&middle,true) && ammo_checks==2);
  command(&middle,"back");assert(middle.formation_row==FORMATION_BACK);
  command(&middle,"front back");assert(back.formation_row==FORMATION_BACK); /* Not the leader. */
  command(&front,"middle back");assert(back.formation_row==FORMATION_MIDDLE);
  command(&front,"front nonexistent");assert(strstr(output,"not here"));
  command(&front,"sideways");assert(strstr(output,"Choose"));
  back.fighting=&npc;back.move_ready=false;
  command(&front,"back back");assert(back.formation_row==FORMATION_MIDDLE && !moves);
  back.move_ready=true;command(&front,"back back");
  assert(back.formation_row==FORMATION_BACK && moves==1 && !back.move_ready && front.move_ready);
  back.fighting=NULL;back.pos=POS_SLEEPING;
  command(&front,"middle back");assert(back.formation_row==FORMATION_BACK);back.pos=POS_STANDING;
  command(&outsider,"back");assert(!outsider.group);
  command(&front,"");assert(msdp_updates>0);
  leave_group(&back);assert(!back.group && back.formation_row==FORMATION_FRONT);
  back.formation_row=FORMATION_BACK;join_group(&back,&group);assert(back.formation_row==FORMATION_FRONT);
  puts("PASS: formation weapon/size matrices, screening, collapse, NPC retarget/AOO, ammo, skills, permissions, move actions and lifecycle.");
}
'''
code = constants + "\n" + stubs + rules
code += function(handler, "void join_group(") + "\n" + function(handler, "void leave_group(")
code += "\n" + function(fight, "bool is_tanking(") + "\n" + commands + "\n" + hit_hook
code += "\n" + function(weapons, "bool can_fire_ammo(") + "\n" + bonus_hook + "\n" + checks
with tempfile.TemporaryDirectory(prefix="formation-check-") as directory:
    source = Path(directory) / "formation.c"
    source.write_text(code)
    binary = Path(directory) / "formation"
    subprocess.run([os.environ.get("CC", "cc"), "-std=c11", "-g", "-Wall", "-Wextra",
                    "-Wno-unused-parameter", "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
                    str(source), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
