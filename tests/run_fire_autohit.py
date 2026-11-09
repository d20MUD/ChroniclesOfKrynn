#!/usr/bin/env python3
"""Exercise the production fire command's automatic and explicit targeting."""
from pathlib import Path
import subprocess
import tempfile
root = Path(__file__).resolve().parents[1]
source = (root / 'src/act.offensive.c').read_text()
a = source.index('ACMD(do_fire)')
command = source[a:source.index('\n}', a) + 2]
code = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#define ACMD(name) void name(struct char_data *ch, const char *argument, int cmd, int subcmd)
#define MAX_INPUT_LENGTH 256
#define NOWHERE -1
#define PRF_AUTOHIT 1
#define AFF_CHARM 1
#define FALSE false
#define TRUE true
#define TYPE_UNDEFINED 0
#define DAM_RESERVED_DBC 0
#define ATTACK_TYPE_RANGED 2
#define PERK_RANGER_QUICK_DRAW 1
#define FEAT_QUICK_DRAW 1
#define IN_ROOM(ch) ((ch)->room)
#define IS_NPC(ch) ((ch)->npc)
#define IS_PET(ch) ((ch)->pet)
#define GROUP(ch) ((ch)->group)
#define FIGHTING(ch) ((ch)->fighting)
#define FIRING(ch) ((ch)->firing)
#define PRF_FLAGGED(ch, flag) ((ch)->autohit)
#define AFF_FLAGGED(ch, flag) ((ch)->charmed)
#define CAN_SEE(ch, vict) ((vict)->visible)
#define HAS_FEAT(ch, feat) false
#define PREREQ_NOT_NPC() do { if (ch->npc) return; } while (0)
#define PREREQ_NOT_PEACEFUL_ROOM() do { if (peaceful) return; } while (0)
typedef int room_rnum;
struct group_data { void *members; };
struct char_data { bool npc, pet, autohit, charmed, visible, sensed, firing; int room; struct char_data *master, *next_in_room, *fighting; struct group_data *group; };
static struct { struct char_data *people; } world[2];
static bool peaceful, ammunition, opening_manyshot;
static int shots, lookups;
static char output[512];
static struct char_data *last_target, *explicit_target;
static void two_arguments(const char *arg, char *first, size_t n, char *second, size_t m) { sscanf(arg,"%255s %255s",first,second); }
static void send_to_char(struct char_data *ch, const char *format, ...) { va_list ap; va_start(ap,format); vsnprintf(output,sizeof(output),format,ap); va_end(ap); }
static struct char_data *get_char_room_vis(struct char_data *ch, const char *name, void *number) { lookups++; return explicit_target; }
static bool can_blindsense_creature(struct char_data *ch, struct char_data *vict) { return vict->sensed; }
static void *simple_list(void *members) { return NULL; }
static bool can_fire_ammo(struct char_data *ch, bool silent) { return ammunition; }
static void hit(struct char_data *ch, struct char_data *vict, int type, int damage, int penalty, int attack) { assert(attack==ATTACK_TYPE_RANGED); shots++; last_target=vict; ch->fighting=vict; }
static bool has_perk(struct char_data *ch, int perk) { return false; }
static int is_using_ranged_weapon(struct char_data *ch, bool silent) { return 0; }
static bool is_throwing_weapon(struct char_data *ch, int weapon) { return false; }
static int get_ranger_quick_draw_proc_chance(struct char_data *ch) { return 0; }
static int rand_number(int low, int high) { return high; }
static void act(const char *text, bool hide, struct char_data *ch, int obj, int vict, int room) {}
#define TO_ROOM 1
static bool try_opening_manyshot(struct char_data *ch,struct char_data *vict) {
  if(!opening_manyshot)return false;
  last_target=vict;ch->fighting=vict;return true;
}
''' + command + r'''
static void reset(struct char_data *ch) { ch->fighting=NULL; ch->firing=false; shots=lookups=0; last_target=NULL; output[0]=0; }
int main(void) {
  struct char_data ch={.room=0,.autohit=true}, player={.visible=true}, charm={.npc=true,.charmed=true,.visible=true}, pet={.npc=true,.master=&ch,.visible=true}, hidden={.npc=true}, first={.npc=true,.visible=true}, second={.npc=true,.visible=true};
  world[0].people=&player; player.next_in_room=&charm; charm.next_in_room=&pet; pet.next_in_room=&hidden; hidden.next_in_room=&first; first.next_in_room=&second;
  ammunition=true;
  do_fire(&ch,"",0,0); assert(shots==1 && last_target==&first && !lookups && ch.firing);
  reset(&ch);opening_manyshot=true;
  do_fire(&ch,"",0,0);assert(!shots && last_target==&first && !lookups && ch.firing);
  opening_manyshot=false;reset(&ch);
  reset(&ch); hidden.sensed=true;
  do_fire(&ch,"",0,0); assert(shots==1 && last_target==&hidden); hidden.sensed=false;
  reset(&ch); ch.autohit=false;
  do_fire(&ch,"",0,0); assert(!shots && !lookups && strstr(output,"Fire at who?"));
  reset(&ch); explicit_target=&second;
  do_fire(&ch,"second",0,0); assert(shots==1 && last_target==&second && lookups==1);
  reset(&ch); ch.autohit=true; explicit_target=NULL;
  do_fire(&ch,"missing",0,0); assert(!shots && lookups==1); /* A typo never auto-targets. */
  reset(&ch); hidden.next_in_room=NULL;
  do_fire(&ch,"",0,0); assert(!shots && strstr(output,"no eligible mobs")); hidden.next_in_room=&first;
  reset(&ch); ammunition=false;
  do_fire(&ch,"",0,0); assert(!shots && !ch.firing); ammunition=true;
  reset(&ch); ch.fighting=&second;
  do_fire(&ch,"",0,0); assert(!shots && strstr(output,"too busy"));
  reset(&ch); peaceful=true;
  do_fire(&ch,"",0,0); assert(!shots); peaceful=false;
  reset(&ch);
  do_fire(&ch,"first north",0,0); assert(!shots && strstr(output,"current room"));
  reset(&ch); ch.room=NOWHERE;
  do_fire(&ch,"",0,0); assert(!shots);
  puts("Fire auto-hit: eligible room order, visibility/blindsense, pet/charm/player exclusions, explicit targets, preference, ammo and combat guards passed");
}
'''
with tempfile.TemporaryDirectory(prefix='fire-autohit-') as directory:
    c = Path(directory) / 'fire.c'; binary = Path(directory) / 'fire'
    c.write_text(code)
    subprocess.run(['cc','-std=gnu99','-fsanitize=address,undefined',str(c),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
