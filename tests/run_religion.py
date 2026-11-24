#!/usr/bin/env python3
"""Exercise the religion module and native persistence/movement integration in isolation."""
from pathlib import Path
import os
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
players = (root / 'src/players.c').read_text()
movement = (root / 'src/movement.c').read_text()
genmob = (root / 'src/genmob.c').read_text()
genobj = (root / 'src/genobj.c').read_text()
objects = (root / 'src/objsave.c').read_text()
shop = (root / 'src/shop.c').read_text()
specials = (root / 'src/spec_procs.c').read_text()
database = (root / 'src/db.c').read_text()
roleplay = (root / 'src/roleplay.c').read_text()
interpreter = (root / 'src/interpreter.c').read_text()


def function(source, signature):
    start = source.index(signature)
    return source[start:source.index('\n}', start) + 2] + '\n'


def between(source, first, last):
    start = source.index(first)
    return source[start:source.index(last, start)]


preamble = r'''
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "db.h"
#include "handler.h"
#include "spells.h"
#include "act.h"
#include <assert.h>
#undef ACMD
#define ACMD(name) void name(struct char_data *ch, const char *argument, int cmd, int subcmd)
static time_t clock_now = 1700000000;
static time_t religion_test_time(time_t *out) { if (out) *out=clock_now; return clock_now; }
#define time religion_test_time
#include "religion.c"
#undef time

struct player_special_data dummy_mob;
struct config_data config_info;
struct deity_info deity_list[NUM_DEITIES];
struct room_data room_data[2], *world=room_data;
struct char_data mob_data[2], *mob_proto=mob_data;
struct index_data index_data[2], *mob_index=index_data;
mob_rnum top_of_mobt=1;
struct index_data object_indices[1], *obj_index=object_indices;
obj_rnum top_of_objt=0;
struct command_info commands[2] = {{.command="buy"},{.command="list"}};
struct command_info *complete_cmd_info=commands;
static int purchase_commits, follower_checks;

struct descriptor_data *descriptor_list;
int communication_output_depth;
struct test_player {
  struct char_data ch;
  struct player_special_data ps;
  struct descriptor_data desc;
  char output[65536];
};
static struct test_player pc[6];
static char persisted[32768];
static int fail_saves, save_calls;
static bool clan_allowed=true;
static int observed_clan_checks;

void basic_mud_log(const char *fmt, ...) { (void)fmt; }
void mudlog(int type,int level,int file,const char *fmt,...) { (void)fmt; }
size_t strlcpy(char *dst,const char *src,size_t size) {
  size_t n=strlen(src), copy=size ? MIN(n,size-1) : 0;
  if (size) {memmove(dst,src,copy);dst[copy]=0;} return n;
}
int is_abbrev(const char *a,const char *b) {return *a && strlen(a)<=strlen(b) && !strncasecmp(a,b,strlen(a));}
const char *one_argument(const char *p,char *out,size_t cap) {
  size_t n=0; while (isspace((unsigned char)*p)) ++p;
  while (*p && !isspace((unsigned char)*p)) {if (n+1<cap) out[n++]=tolower((unsigned char)*p);++p;}
  out[n]=0;while(isspace((unsigned char)*p)) ++p;return p;
}
const char *two_arguments(const char *s,char *a,size_t na,char *b,size_t nb) {
  s=one_argument(s,a,na);return one_argument(s,b,nb);
}
struct char_data *get_char_room(char *name,int *num,room_rnum room) {return world[room].people;}
bool can_add_follower(struct char_data *ch,int vnum) {++follower_checks;return true;}
int decrease_gold(struct char_data *ch,int amount) {GET_GOLD(ch)-=amount;return GET_GOLD(ch);}
room_rnum real_room(room_vnum vnum) {return vnum==100 ? 0 : vnum==101 ? 1 : NOWHERE;}
mob_rnum real_mobile(mob_vnum vnum) {return vnum==200 ? 0 : vnum==201 ? 1 : NOBODY;}
int MIN(int a,int b) {return a<b?a:b;}
int MAX(int a,int b) {return a>b?a:b;}
struct sivak_form *active_sivak_form(struct char_data *ch) {return NULL;}
bool room_is_dark(room_rnum room) {return false;}
bool char_has_ultra(struct char_data *ch) {return false;}
bool char_has_infra(struct char_data *ch) {return false;}
bool is_outdoors(struct char_data *ch) {return false;}
bool has_true_sight(struct char_data *ch) {return false;}
bool has_blindsight(struct char_data *ch) {return false;}
bool can_see_hidden(struct char_data *ch,struct char_data *obj) {return !AFF_FLAGGED(obj,AFF_HIDE);}
char *show_pers(struct char_data *ch,struct char_data *viewer) {return CAN_SEE(viewer,ch) ? ch->player.name : "someone";}
size_t send_to_char(struct char_data *ch,const char *fmt,...) {
  for(int i=0;i<6;i++) if(ch==&pc[i].ch) {
    size_t len=strlen(pc[i].output);va_list ap;va_start(ap,fmt);
    vsnprintf(pc[i].output+len,sizeof(pc[i].output)-len,fmt,ap);va_end(ap);
    return strlen(pc[i].output)-len;
  }
  return 0;
}
static void clear_output(void) {for(int i=0;i<6;i++) pc[i].output[0]=0;}
static void run(int i,const char *command) {clear_output();do_religion(&pc[i].ch,command,0,0);}
const char *religion_rank_title(int deity,int rank) {(void)deity;return religion_rank_name(rank);}
bool religion_blessing_command(struct char_data *ch,const char *a,const char *b) {(void)ch;(void)a;(void)b;return false;}
bool religion_blessing_remove(struct char_data *ch) {(void)ch;return true;}
void religion_tasks_restore(struct char_data *ch) {(void)ch;}
void religion_tasks_status(struct char_data *ch) {(void)ch;}
void religion_tasks_command(struct char_data *ch,const char *s) {(void)ch;(void)s;}
void religion_tasks_admin(struct char_data *ch,int t,int d,int p,const char *s) {(void)ch;(void)t;(void)d;(void)p;(void)s;}
void religion_rank_admin(struct char_data *ch,const char *s) {(void)ch;(void)s;}
void temple_chest_admin(struct char_data *ch,const char *argument) {(void)ch;(void)argument;}
static void admin(const char *command) {clear_output();do_religionadmin(&pc[5].ch,command,0,0);}
void show_character_rp_menu(struct descriptor_data *d) {}
void display_deity_menu(struct descriptor_data *d) {}
void display_deity_info(struct descriptor_data *d) {}
void free_mobile_strings(struct char_data *mob) {}
int check_mobile_strings(struct char_data *mob) {return 1;}
int copy_mobile_strings(struct char_data *to,struct char_data *from) {return 1;}
bool clan_access_check(struct char_data *ch,const struct clan_requirement *r,bool explain) {
  ++observed_clan_checks;return clan_allowed;
}
'''
code = preamble
code += '\n' + function(genmob, 'int copy_mobile(')
code += '\n#undef SPECIAL\n#define SPECIAL(n) int n(struct char_data *ch,void *me,int cmd,const char *argument)\n'
code += '#define PET_PRICE(pet) (GET_LEVEL(pet)*300)\n'
code += between(specials, 'SPECIAL(pet_shops)\n{', '    /* success! */')
code += '++purchase_commits;decrease_gold(ch,PET_PRICE(pet));return TRUE;\n} return FALSE;\n}\n'
code += function(shop, 'static int same_obj(struct obj_data *obj1, struct obj_data *obj2)\n{')
code += '\nstatic void write_object_world(FILE *fp,struct obj_data *obj) {\n'
code += between(genobj, '      if (obj->religion_requirement.deity', '      /* B:  Do we have spells? */')
code += '}\nstatic void write_object_record(FILE *fp,struct obj_data *obj) {\n'
code += between(objects, '  /* Always write zeros too,', '  fprintf(fp, "CReq:')
code += '}\n'

code += '\nstatic void write_tags(FILE *fl,struct char_data *ch) { int i;\n#define BUFFER_WRITE(...) fprintf(fl,__VA_ARGS__)\n'
code += between(players, '  BUFFER_WRITE("God :', '  if (GET_AUTOCQUEST_VNUM')
code += '\n#undef BUFFER_WRITE\n}\n'
code += r'''
bool save_char_checked(struct char_data *ch,int mode) {
  FILE *f;size_t n;
  ++save_calls;if(fail_saves) {--fail_saves;return false;}
  f=tmpfile();assert(f);write_tags(f,ch);rewind(f);n=fread(persisted,1,sizeof(persisted)-1,f);
  persisted[n]=0;fclose(f);return true;
}
static void reset_player_religion(struct char_data *ch) {
'''
code += between(players, '    religion_clear_confirmation(ch);', '    memset(ch->player_specials->clan_objective_pending')
code += r'''
}
static int random_step, hospitality_gifts, fail_provisions, live_gifts;
int rand_number(int low,int high) {return low + random_step++ % (high-low+1);}
int can_carry_weight_limit(struct char_data *ch) {(void)ch;return 1000;}
struct obj_data *create_obj(void) {++live_gifts;return calloc(1,sizeof(struct obj_data));}
void extract_obj(struct obj_data *obj) {
  --live_gifts;free(obj->name);free(obj->short_description);free(obj->description);free(obj);
}
int estate_hospitality_claim(struct char_data *ch,struct obj_data *food,struct obj_data *drink,time_t now,int cooldown) {
  assert(GET_OBJ_TYPE(food)==ITEM_FOOD && GET_OBJ_TYPE(drink)==ITEM_DRINK);
  assert(food->affected[0].bonus_type==BONUS_TYPE_FOOD && drink->affected[0].bonus_type==BONUS_TYPE_DRINK);
  if(fail_provisions)return 0;
  ch->player_specials->saved.religion_hospitality_after=now+cooldown;
  save_char_checked(ch,0);hospitality_gifts+=2;return 1;
}
void estate_recover(struct char_data *ch) {(void)ch;}
static void read_tags(struct char_data *ch,const char *text) {
  char copy[32768],*record,*next,*line,tag[5];
  reset_player_religion(ch);strlcpy(copy,text,sizeof(copy));
  for(record=strtok_r(copy,"\n",&next);record;record=strtok_r(NULL,"\n",&next)) {
    assert(strlen(record)>=6);memcpy(tag,record,4);tag[4]=0;line=record+6;
    if(!strcmp(tag,"God ")) GET_DEITY(ch)=atoi(line);
    else if(!strcmp(tag,"RlSt")) assert(religion_load_player_standing(ch,line));
    else if(!strcmp(tag,"RlCd")) assert(religion_load_player_cooldowns(ch,line));
  }
}
static bool move_requirement(struct char_data *ch,struct char_data *mob) {
  bool block=true;
  do {
'''
code += between(movement, '    if (block && (MOB_FLAGGED(mob, MOB_BLOCK_CLAN)', '    if (block && MOB_FLAGGED(mob, MOB_BLOCK_RACE)')
code += '\n} while(0);return !block;\n}\n'
# Exercise the entire production directional guard loop, including multiple guards.
code += '\nstatic int stealth_attempts;\n'
code += r"""
bool has_ghost(struct char_data *ch) {(void)ch;return true;}
static bool blocker_stealth_passes(struct char_data *ch,struct char_data *mob) {(void)ch;(void)mob;++stealth_attempts;return true;}
const char *act(const char *s,int hide,struct char_data *ch,struct obj_data *obj,void *vict,int type) {(void)hide;(void)ch;(void)obj;(void)vict;(void)type;return s;}
static bool guard_loop_allows(struct char_data *ch,int dir) {
"""
code += between(movement, '  struct char_data *mob;\n  sbyte block = FALSE;', '  if (block && !IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_NOHASSLE))')
code += '\nreturn !block || IS_NPC(ch) || PRF_FLAGGED(ch,PRF_NOHASSLE);\n}\n'
# Execute the production mob field writer/parser without unrelated world-file sections.
code += '\nstatic void write_mob(FILE *fd,struct char_data *mob) {\n'
code += between(genmob, '  if (mob->mob_deity)', '  if (mob->clan_requirement.clan)')
code += '\n}\nstatic void parse_mob_field(const char *keyword,const char *value,int i) {\n'
code += '#define CASE(k) if (!strcasecmp(keyword,k))\n'
code += between(database, '  CASE("MobDeity")', '  CASE("Clan")')
code += '\n#undef CASE\n}\n'
code += function(roleplay, 'void HandleStateCharacterDeityParseMenuChoice(')
code += function(roleplay, 'void HandleStateCharacterDeityConfirmParseMenuChoice(')
code += '\nstatic struct json_object *creation_preview(struct descriptor_data *d) {\n'
code += 'const char *step,*title;const char *pantheons[]={"None","All"};\n'
code += 'struct json_object *event=json_object_new_object();do {\n'
code += between(interpreter, '    case CON_CHARACTER_DEITY_CONFIRM:\n    {', '    case CON_CHAR_RP_DECIDE:').replace('    case CON_CHARACTER_DEITY_CONFIRM:', '', 1)
code += '} while(0);return event;\n}\n'
code += r'''
static void init(void) {
  for(int i=0;i<6;i++) {
    pc[i].ch.player_specials=&pc[i].ps;
    pc[i].ch.desc=&pc[i].desc;pc[i].desc.character=&pc[i].ch;STATE(&pc[i].desc)=CON_PLAYING;
    GET_LEVEL(&pc[i].ch)=20;GET_POS(&pc[i].ch)=POS_STANDING;IN_ROOM(&pc[i].ch)=0;
    pc[i].ch.player.name=i==0?"Alice":i==1?"Bob":i==2?"Otherfaith":i==3?"Invisible":i==4?"HiddenStaff":"Builder";
    if(i<5)pc[i].desc.next=&pc[i+1].desc;
  }
  GET_LEVEL(&pc[5].ch)=LVL_BUILDER;
  descriptor_list=&pc[0].desc;CONFIG_LEVEL_CAN_SHOUT=1;CONFIG_OK="Okay.\r\n";
  for(int i=0;i<2;i++) {
    mob_data[i].nr=i;mob_data[i].player_specials=&dummy_mob;SET_BIT_AR(MOB_FLAGS(&mob_data[i]),MOB_ISNPC);
    mob_data[i].mob_deity=i+1;mob_data[i].mob_temple_rank=3;
    GET_POS(&mob_data[i])=POS_STANDING;IN_ROOM(&mob_data[i])=i;
    index_data[i].vnum=200+i;
    deity_list[i+1].name=i==0?"Mishakal":"Reorx";deity_list[i+1].pantheon=DEITY_PANTHEON_ALL;
    deity_list[i+1].portfolio="Test portfolio";deity_list[i+1].symbol="Test symbol";
    deity_list[i+1].description="Test deity description";
    world[i].name=i==0?"Public entrance":"Other shrine";
  }
  world[0].people=&mob_data[0];world[1].people=&mob_data[1];
}
static void file_text(const char *path,const char *text) {FILE *f=fopen(path,"w");assert(f);assert(fputs(text,f)>=0);assert(!fclose(f));}
int main(int argc,char **argv) {
  struct char_data guard={0},copied={0},stock={0};
  struct obj_data object={0},other_object={0};
  struct religion_requirement requirement;
  struct player_special_data loaded={0};
  struct char_data reload={0};
  time_t deadline;
  int before,hunger,thirst;
  char saved_disk[32768],mobtext[1024],key[100],value[100],*colon,*record,*saveptr;
  FILE *f;
  struct json_object *preview,*details,*name;
  assert(argc==2);init();assert(religion_load_temples(argv[1]));
  reload.player_specials=&loaded;
  /* Legacy God-only records migrate to Devotee, preserve domains, and do not
   * inherit a recycled character's Standing, cooldown or preference. */
  GET_1ST_DOMAIN(&reload)=DOMAIN_HEALING;
  loaded.saved.religion_standing[1]=336;loaded.saved.religion_devote_after=clock_now;
  loaded.saved.religion_channel_off=true;
  read_tags(&reload,"God : 1\n");
  assert(religion_rank(&reload)==1 && !loaded.saved.religion_devote_after && !loaded.saved.religion_channel_off);
  assert(GET_1ST_DOMAIN(&reload)==DOMAIN_HEALING);
  assert(!religion_load_player_standing(&reload,"120 100"));
  assert(!religion_load_player_standing(&reload,"1 337"));
  assert(!religion_load_player_standing(&reload,"1 1 extra"));
  assert(!religion_load_player_cooldowns(&reload,"-1 0 0"));
  assert(!religion_load_player_cooldowns(&reload,"9223372036854775808 0 0"));
  assert(!religion_load_player_cooldowns(&reload,"0 0 2"));
  assert(!religion_load_player_cooldowns(&reload,"0 0 1 trailing"));
  /* Pfile round-trip includes historical faiths unavailable in this campaign. */
  loaded.saved.religion_standing[1]=72;loaded.saved.religion_standing[119]=336;
  loaded.saved.religion_devote_after=clock_now+100;loaded.saved.religion_hospitality_after=clock_now+200;
  loaded.saved.religion_channel_off=true;
  assert(save_char_checked(&reload,0));strlcpy(saved_disk,persisted,sizeof(saved_disk));
  read_tags(&reload,saved_disk);
  assert(religion_rank(&reload)==4 && loaded.saved.religion_standing[119]==336);
  assert(loaded.saved.religion_channel_off && loaded.saved.religion_devote_after==clock_now+100);
  assert(loaded.saved.religion_hospitality_after==clock_now+200);
  /* First devotion has no cooldown, but changes only on confirmation. */
  run(0,"devote Mishakal");assert(!GET_DEITY((&pc[0].ch)));assert(strstr(pc[0].output,"not yet available"));
  run(0,"confirm");assert(GET_DEITY((&pc[0].ch))==1 && religion_rank(&pc[0].ch)==1);
  run(0,"leave");run(0,"cancel");run(0,"confirm");assert(GET_DEITY((&pc[0].ch))==1);
  run(0,"leave");clock_now+=121;run(0,"confirm");assert(GET_DEITY((&pc[0].ch))==1);
  /* Class/domain changes made after preview are rechecked before committing. */
  run(0,"leave");CLASS_LEVEL((&pc[0].ch),CLASS_CLERIC)=1;run(0,"confirm");assert(GET_DEITY((&pc[0].ch))==1);
  CLASS_LEVEL((&pc[0].ch),CLASS_CLERIC)=0;
  pc[0].ps.saved.religion_standing[1]=72;
  run(0,"leave");fail_saves=1;run(0,"confirm");assert(GET_DEITY((&pc[0].ch))==1 && !pc[0].ps.saved.religion_devote_after);
  run(0,"leave");run(0,"confirm");assert(!GET_DEITY((&pc[0].ch)));
  deadline=pc[0].ps.saved.religion_devote_after;assert(deadline==clock_now+RELIGION_CHANGE_SECONDS);
  run(0,"devote Reorx");assert(!pc[0].ps.religion_pending_action);
  clock_now=deadline;run(0,"devote Reorx");run(0,"confirm");assert(GET_DEITY((&pc[0].ch))==2);
  /* A staff edit between preview and confirmation invalidates it. */
  run(1,"devote Mishakal");GET_DEITY((&pc[1].ch))=2;run(1,"confirm");assert(GET_DEITY((&pc[1].ch))==2);
  GET_DEITY((&pc[1].ch))=0;
  GET_1ST_DOMAIN(&pc[1].ch)=DOMAIN_HEALING;run(1,"devote Reorx");assert(!pc[1].ps.religion_pending_action);
  GET_1ST_DOMAIN(&pc[1].ch)=0;
  /* Builder records are persisted, validated, linked to a unique priest, and
   * service configuration edits invalidate existing previews. */
  admin("create 7 1 East Temple");assert(temple_count==1 && !temples[0].enabled);
  admin("set 7 entrance 100");admin("set 7 priest 200");admin("set 7 hospitality 1");admin("set 7 enabled 1");
  assert(temples[0].enabled && temple_live(&temples[0]));
  assert(religion_load_temples(argv[1]) && temple_count==1 && temples[0].hospitality);
  admin("create 8 2 West Temple");admin("set 8 entrance 101");admin("set 8 priest 200");assert(!temples[1].priest);
  admin("set 8 priest 201");admin("set 8 hospitality 1");admin("set 8 enabled 1");assert(temple_live(&temples[1]));
  run(1,"devote Mishakal");admin("set 7 name Renamed Temple");run(1,"confirm");assert(!GET_DEITY((&pc[1].ch)));
  IN_ROOM(&pc[1].ch)=1;run(1,"devote Mishakal");assert(!pc[1].ps.religion_pending_action);
  IN_ROOM(&pc[1].ch)=0;run(1,"devote Mishakal");IN_ROOM(&pc[1].ch)=1;run(1,"confirm");assert(!GET_DEITY((&pc[1].ch)));
  IN_ROOM(&pc[1].ch)=0;run(1,"devote Mishakal");run(1,"confirm");assert(GET_DEITY((&pc[1].ch))==1);
  /* Hospitality produces both consumables even with hunger and thirst disabled. */
  GET_COND(&pc[1].ch,HUNGER)=-1;GET_COND(&pc[1].ch,THIRST)=-1;
  GET_HIT(&pc[1].ch)=17;GET_MOVE(&pc[1].ch)=31;GET_PSP(&pc[1].ch)=7;
  fail_provisions=1;run(1,"hospitality");assert(!pc[1].ps.saved.religion_hospitality_after);
  assert(!hospitality_gifts && !live_gifts);fail_provisions=0;
  IS_CARRYING_N(&pc[1].ch)=CAN_CARRY_N(&pc[1].ch);
  run(1,"hospitality");assert(!hospitality_gifts);IS_CARRYING_N(&pc[1].ch)=0;
  fail_saves=1;run(1,"hospitality");assert(hospitality_gifts==2 && !live_gifts);
  assert(GET_COND(&pc[1].ch,HUNGER)==-1 && GET_COND(&pc[1].ch,THIRST)==-1);
  assert(GET_HIT(&pc[1].ch)==17 && GET_MOVE(&pc[1].ch)==31 && GET_PSP(&pc[1].ch)==7);
  deadline=pc[1].ps.saved.religion_hospitality_after;
  GET_DEITY((&pc[1].ch))=2;IN_ROOM(&pc[1].ch)=1;run(1,"hospitality");assert(hospitality_gifts==2);
  assert(pc[1].ps.saved.religion_hospitality_after==deadline);
  clock_now=deadline;run(1,"hospitality");assert(hospitality_gifts==4);
  assert(pc[1].ps.saved.religion_hospitality_after==deadline+RELIGION_HOSPITALITY_SECONDS);
  for(int j=0;j<100;j++)for(int f=0;f<2;f++) {
    struct obj_data *gift=hospitality_item(f);
    assert(gift && GET_OBJ_VAL(gift,0)==100 && gift->affected[0].modifier>0);
    assert(!!apply_type_food_or_drink[gift->affected[0].location]==f);
    assert(*apply_type_food_names[gift->affected[0].location]);extract_obj(gift);
  }
  assert(!live_gifts);
  /* Privacy and delivery use actual CAN_SEE/show_pers behavior, not raw names. */
  for(int i=0;i<6;i++){GET_DEITY((&pc[i].ch))=1;IN_ROOM(&pc[i].ch)=0;}
  GET_DEITY((&pc[2].ch))=2;
  SET_BIT_AR(AFF_FLAGS(&pc[3].ch),AFF_INVISIBLE);GET_LEVEL(&pc[4].ch)=LVL_IMPL;GET_INVIS_LEV(&pc[4].ch)=LVL_IMPL;
  run(0,"who");assert(strstr(pc[0].output,"Bob") && !strstr(pc[0].output,"Otherfaith"));
  assert(!strstr(pc[0].output,"Invisible") && !strstr(pc[0].output,"HiddenStaff"));
  run(0,"say hello %s $n");assert(strstr(pc[1].output,"hello %s $n") && !*pc[2].output);
  assert(!communication_output_depth);
  run(3,"say hidden message");assert(strstr(pc[0].output,"someone") && !strstr(pc[0].output,"Invisible"));
  run(1,"channel off");run(0,"say message");assert(!*pc[1].output);
  assert(save_char_checked(&pc[1].ch,0));strlcpy(saved_disk,persisted,sizeof(saved_disk));
  read_tags(&pc[1].ch,saved_disk);assert(pc[1].ps.saved.religion_channel_off);
  run(1,"channel on");SET_BIT_AR(AFF_FLAGS(&pc[0].ch),AFF_SILENCED);run(0,"say muted");assert(!*pc[1].output);
  REMOVE_BIT_AR(AFF_FLAGS(&pc[0].ch),AFF_SILENCED);SET_BIT_AR(PLR_FLAGS(&pc[0].ch),PLR_NOSHOUT);
  run(0,"say sanctioned");assert(!*pc[1].output);REMOVE_BIT_AR(PLR_FLAGS(&pc[0].ch),PLR_NOSHOUT);
  SET_BIT_AR(ROOM_FLAGS(0),ROOM_SOUNDPROOF);run(0,"say blocked");assert(!*pc[1].output);
  REMOVE_BIT_AR(ROOM_FLAGS(0),ROOM_SOUNDPROOF);SET_BIT_AR(AFF_FLAGS(&pc[1].ch),AFF_DEAF);
  run(0,"say unheard");assert(!*pc[1].output);REMOVE_BIT_AR(AFF_FLAGS(&pc[1].ch),AFF_DEAF);
  /* Staff monitoring has the existing clan-monitor opt out, and no roster leak. */
  GET_LEVEL(&pc[5].ch)=LVL_STAFF;GET_DEITY((&pc[5].ch))=2;run(0,"say monitor");assert(strstr(pc[5].output,"monitor"));
  SET_BIT_AR(PRF_FLAGS(&pc[5].ch),PRF_NOCLANTALK);run(0,"say no monitor");assert(!*pc[5].output);
  STATE(&pc[1].desc)=CON_MEDIT;run(0,"say editing");assert(!*pc[1].output);STATE(&pc[1].desc)=CON_PLAYING;
  pc[1].desc.original=&pc[1].ch;run(0,"who");assert(!strstr(pc[0].output,"Bob"));pc[1].desc.original=NULL;
  /* Legacy mobs keep ordinary behavior. Faith and rank requirements compose
   * with the production clan branch and never authorize a different faith. */
  guard.player_specials=&dummy_mob;SET_BIT_AR(MOB_FLAGS(&guard),MOB_ISNPC);
  assert(religion_mob_valid(&guard) && religion_guard_allows(&pc[0].ch,&guard));
  SET_BIT_AR(MOB_FLAGS(&guard),MOB_BLOCK_RELIGION);assert(!religion_mob_valid(&guard));
  assert(!religion_guard_allows(&pc[0].ch,&guard));guard.mob_deity=1;assert(religion_mob_valid(&guard));
  guard.mob_temple_rank=4;SET_BIT_AR(MOB_FLAGS(&guard),MOB_BLOCK_TEMPLE_RANK);
  pc[0].ps.saved.religion_standing[1]=36;assert(!religion_guard_allows(&pc[0].ch,&guard));
  pc[0].ps.saved.religion_standing[1]=72;assert(religion_guard_allows(&pc[0].ch,&guard));
  GET_DEITY((&pc[0].ch))=2;pc[0].ps.saved.religion_standing[2]=336;assert(!religion_guard_allows(&pc[0].ch,&guard));
  REMOVE_BIT_AR(MOB_FLAGS(&guard),MOB_BLOCK_RELIGION);assert(!religion_guard_allows(&pc[0].ch,&guard));
  GET_DEITY((&pc[0].ch))=1;SET_BIT_AR(MOB_FLAGS(&guard),MOB_BLOCK_CLAN);guard.clan_requirement.clan=1;
  clan_allowed=false;assert(!move_requirement(&pc[0].ch,&guard));clan_allowed=true;
  assert(move_requirement(&pc[0].ch,&guard));pc[0].ps.saved.religion_standing[1]=0;
  assert(!move_requirement(&pc[0].ch,&guard));assert(observed_clan_checks==3);
  SET_BIT_AR(PRF_FLAGS(&pc[0].ch),PRF_NOHASSLE);assert(religion_guard_allows(&pc[0].ch,&guard));
  REMOVE_BIT_AR(PRF_FLAGS(&pc[0].ch),PRF_NOHASSLE);
  guard.mob_temple_rank=0;assert(!religion_mob_valid(&guard));guard.mob_temple_rank=8;
  assert(religion_guard_allows(&mob_data[0],&guard));
  /* All faith/rank combinations, including exact-rank boundaries. */
  {
    const int thresholds[]={0,12,36,72,120,180,252,336};
    struct char_data first={0},second={0};
    SET_BIT_AR(MOB_FLAGS(&first),MOB_ISNPC);SET_BIT_AR(MOB_FLAGS(&second),MOB_ISNPC);
    first.mob_deity=second.mob_deity=1;first.mob_temple_rank=2;second.mob_temple_rank=4;
    SET_BIT_AR(MOB_FLAGS(&first),MOB_BLOCK_E);SET_BIT_AR(MOB_FLAGS(&second),MOB_BLOCK_N);
    SET_BIT_AR(MOB_FLAGS(&first),MOB_BLOCK_TEMPLE_RANK);SET_BIT_AR(MOB_FLAGS(&second),MOB_BLOCK_TEMPLE_RANK);
    first.next_in_room=&second;world[0].people=&first;IN_ROOM(&pc[0].ch)=0;
    GET_DEITY((&pc[0].ch))=1;pc[0].ps.saved.religion_standing[1]=36;
    assert(guard_loop_allows(&pc[0].ch,EAST));assert(!guard_loop_allows(&pc[0].ch,NORTH));
    REMOVE_BIT_AR(MOB_FLAGS(&second),MOB_BLOCK_N);SET_BIT_AR(MOB_FLAGS(&second),MOB_BLOCK_E);
    assert(!guard_loop_allows(&pc[0].ch,EAST));assert(!stealth_attempts);
    pc[0].ps.saved.religion_standing[1]=72;assert(guard_loop_allows(&pc[0].ch,EAST));
    second.mob_deity=2;assert(!guard_loop_allows(&pc[0].ch,EAST));
    assert(guard_loop_allows(&pc[0].ch,WEST)); /* outward direction stays open */
    second.mob_deity=1;
    for(int flags=0;flags<4;flags++)for(int faith=0;faith<3;faith++)for(int need=1;need<=8;need++)for(int rank=1;rank<=8;rank++) {
      REMOVE_BIT_AR(MOB_FLAGS(&first),MOB_BLOCK_RELIGION);REMOVE_BIT_AR(MOB_FLAGS(&first),MOB_BLOCK_TEMPLE_RANK);
      if(flags&1)SET_BIT_AR(MOB_FLAGS(&first),MOB_BLOCK_RELIGION);
      if(flags&2)SET_BIT_AR(MOB_FLAGS(&first),MOB_BLOCK_TEMPLE_RANK);
      GET_DEITY((&pc[0].ch))=faith;first.mob_temple_rank=need;
      pc[0].ps.saved.religion_standing[faith]=thresholds[rank-1];
      assert(religion_guard_allows(&pc[0].ch,&first)==(!flags || (faith==1 && (!(flags&2) || rank>=need))));
    }
    world[0].people=&mob_data[0];
  }
  /* Purchase gates are metadata-driven, independent of blocker flags and
   * checked against the buyer's active faith. Partial settings are unrestricted. */
  REMOVE_BIT_AR(MOB_FLAGS(&guard),MOB_BLOCK_RELIGION);
  REMOVE_BIT_AR(MOB_FLAGS(&guard),MOB_BLOCK_TEMPLE_RANK);
  GET_DEITY((&pc[0].ch))=1;pc[0].ps.saved.religion_standing[1]=36;
  assert(!religion_purchase_allows(&pc[0].ch,1,4,false));
  SET_BIT_AR(PRF_FLAGS(&pc[0].ch),PRF_NOHASSLE);
  assert(!religion_purchase_allows(&pc[0].ch,1,4,false));
  REMOVE_BIT_AR(PRF_FLAGS(&pc[0].ch),PRF_NOHASSLE);
  pc[0].ps.saved.religion_standing[1]=72;assert(religion_purchase_allows(&pc[0].ch,1,4,false));
  pc[0].ps.saved.religion_standing[1]=336;assert(religion_purchase_allows(&pc[0].ch,1,4,false));
  GET_DEITY((&pc[0].ch))=2;assert(!religion_purchase_allows(&pc[0].ch,1,4,false));
  GET_DEITY((&pc[0].ch))=0;assert(!religion_purchase_allows(&pc[0].ch,1,1,false));
  assert(religion_purchase_allows(&pc[0].ch,1,0,false));
  assert(religion_purchase_allows(&pc[0].ch,0,4,false));
  assert(!religion_purchase_allows(&pc[0].ch,-1,0,false));
  assert(!religion_purchase_allows(&pc[0].ch,1,9,false));
  assert(religion_parse_requirement("1 4",&requirement) && requirement.deity==1 && requirement.rank==4);
  assert(!religion_parse_requirement("1 4 junk",&requirement) && requirement.deity==-1);
  assert(!religion_parse_requirement("999999999999999999999 1",&requirement));
  assert(!religion_parse_requirement("1 -2",&requirement));
  assert(religion_parse_requirement("0 0",&requirement) && !requirement.deity && !requirement.rank);
  object.short_description=other_object.short_description="a temple item";
  object.religion_requirement=(struct religion_requirement){1,4};
  assert(!religion_object_wear_allows(&pc[0].ch,&object,false));
  GET_DEITY((&pc[0].ch))=1;pc[0].ps.saved.religion_standing[1]=36;
  assert(!religion_object_wear_allows(&pc[0].ch,&object,false));
  pc[0].ps.saved.religion_standing[1]=72;
  assert(religion_object_wear_allows(&pc[0].ch,&object,false));
  pc[0].ps.saved.religion_standing[1]=336;
  assert(religion_object_wear_allows(&pc[0].ch,&object,false));
  GET_DEITY((&pc[0].ch))=0;

  other_object.religion_requirement=object.religion_requirement;
  assert(same_obj(&object,&other_object));
  other_object.clan_requirement.clan=1;assert(!same_obj(&object,&other_object));
  other_object.clan_requirement.clan=0;
  other_object.clan_requirement.rank=1;assert(!same_obj(&object,&other_object));
  other_object.clan_requirement.rank=0;
  other_object.clan_requirement.favor=1;assert(!same_obj(&object,&other_object));
  other_object.clan_requirement.favor=0;
  other_object.religion_requirement.rank=3;
  assert(!same_obj(&object,&other_object));
  assert(!religion_object_purchase_allows(&pc[0].ch,&object,false));
  GET_DEITY((&pc[0].ch))=1;assert(religion_object_purchase_allows(&pc[0].ch,&object,false));
  f=tmpfile();assert(f);write_object_world(f,&object);rewind(f);
  assert(fgets(mobtext,sizeof(mobtext),f) && !strcmp(mobtext,"F\n"));
  assert(fgets(mobtext,sizeof(mobtext),f));fclose(f);
  assert(religion_parse_requirement(mobtext,&other_object.religion_requirement));
  assert(other_object.religion_requirement.deity==1 && other_object.religion_requirement.rank==4);
  f=tmpfile();assert(f);write_object_record(f,&object);rewind(f);
  assert(fgets(mobtext,sizeof(mobtext),f) && !strncmp(mobtext,"RReq: 1 4",9));fclose(f);
  /* Bag/sheath packaging must not bypass a contained item's restriction. */
  other_object.religion_requirement=(struct religion_requirement){0,0};
  other_object.contains=&object;GET_DEITY((&pc[0].ch))=2;
  assert(!religion_object_purchase_allows(&pc[0].ch,&other_object,false));
  other_object.contains=NULL;other_object.sheath_primary=&object;
  assert(!religion_object_purchase_allows(&pc[0].ch,&other_object,false));
  other_object.sheath_primary=NULL;other_object.sheath_secondary=&object;
  assert(!religion_object_purchase_allows(&pc[0].ch,&other_object,false));
  other_object.sheath_secondary=NULL;other_object.next_content=&object;
  assert(religion_object_purchase_allows(&pc[0].ch,&other_object,false));
  other_object.next_content=NULL;other_object.contains=&other_object;
  assert(!religion_object_purchase_allows(&pc[0].ch,&other_object,false));
  other_object.contains=NULL;GET_DEITY((&pc[0].ch))=1;
  /* Token restrictions AND the delivered pet prototype must both pass. */
  GET_OBJ_TYPE(&object)=ITEM_PET;GET_OBJ_RNUM(&object)=0;object_indices[0].vnum=200;
  mob_data[0].mob_deity=2;mob_data[0].mob_temple_rank=3;
  assert(!religion_object_purchase_allows(&pc[0].ch,&object,false));
  mob_data[0].mob_deity=1;assert(religion_object_purchase_allows(&pc[0].ch,&object,false));
  object.religion_requirement.deity=2;assert(!religion_object_purchase_allows(&pc[0].ch,&object,false));
  /* The actual pet-shop disqualifiers run before payment or follower creation. */
  stock=mob_data[1];stock.player.name="templepet";stock.mob_deity=1;stock.mob_temple_rank=4;
  GET_LEVEL(&stock)=2;world[1].people=&stock;
  mob_data[1].mob_deity=1;mob_data[1].mob_temple_rank=4;
  GET_GOLD(&pc[0].ch)=10000;IN_ROOM(&pc[0].ch)=0;
  pc[0].ps.saved.religion_standing[1]=36;
  assert(pet_shops(&pc[0].ch,NULL,0,"templepet"));
  assert(GET_GOLD(&pc[0].ch)==10000 && !purchase_commits && !follower_checks);
  pc[0].ps.saved.religion_standing[1]=72;
  assert(pet_shops(&pc[0].ch,NULL,0,"templepet"));
  assert(GET_GOLD(&pc[0].ch)==9400 && purchase_commits==1 && follower_checks==1);
  mob_data[1].mob_deity=2;
  assert(pet_shops(&pc[0].ch,NULL,0,"templepet"));assert(GET_GOLD(&pc[0].ch)==9400 && purchase_commits==1);
  SET_BIT_AR(MOB_FLAGS(&guard),MOB_BLOCK_TEMPLE_RANK);
  /* Actual copy_mobile keeps religion fields and flags; actual world tags round trip. */
  assert(copy_mobile(&copied,&guard));assert(copied.mob_deity==1 && copied.mob_temple_rank==8);
  assert(MOB_FLAGGED(&copied,MOB_BLOCK_TEMPLE_RANK));
  f=tmpfile();assert(f);write_mob(f,&copied);rewind(f);before=fread(mobtext,1,sizeof(mobtext)-1,f);mobtext[before]=0;fclose(f);
  for(record=strtok_r(mobtext,"\n",&saveptr);record;record=strtok_r(NULL,"\n",&saveptr)) {
    colon=strchr(record,':');assert(colon);*colon=0;parse_mob_field(record,colon+2,0);
  }
  assert(mob_data[0].mob_deity==1 && mob_data[0].mob_temple_rank==8);
  parse_mob_field("TempleRank","9999999999999999999999999999999",0);assert(mob_data[0].mob_temple_rank==-1);
  parse_mob_field("MobDeity","-4",0);assert(mob_data[0].mob_deity==-1);
  parse_mob_field("MobDeity","bad",0);assert(mob_data[0].mob_deity==-1);
  /* Creation confirms before mutating GET_DEITY. Existing characters cannot
   * use the login roleplay menu to evade departure cooldowns or priests. */
  GET_LEVEL(&pc[2].ch)=0;GET_DEITY((&pc[2].ch))=0;pc[2].ps.saved.religion_devote_after=0;
  HandleStateCharacterDeityParseMenuChoice(&pc[2].desc,"1bad");
  assert(!GET_DEITY((&pc[2].ch)) && STATE(&pc[2].desc)!=CON_CHARACTER_DEITY_CONFIRM);
  HandleStateCharacterDeityParseMenuChoice(&pc[2].desc,"1");assert(!GET_DEITY((&pc[2].ch)));
  preview=creation_preview(&pc[2].desc);
  assert(json_object_object_get_ex(preview,"deity",&details));
  assert(json_object_object_get_ex(details,"name",&name) && !strcmp(json_object_get_string(name),"Mishakal"));
  json_object_put(preview);
  HandleStateCharacterDeityConfirmParseMenuChoice(&pc[2].desc,"n");assert(!GET_DEITY((&pc[2].ch)));
  HandleStateCharacterDeityParseMenuChoice(&pc[2].desc,"1");HandleStateCharacterDeityConfirmParseMenuChoice(&pc[2].desc,"y");
  assert(GET_DEITY((&pc[2].ch))==1);
  GET_LEVEL(&pc[2].ch)=20;GET_DEITY((&pc[2].ch))=0;pc[2].ps.saved.religion_devote_after=clock_now+100;
  HandleStateCharacterDeityParseMenuChoice(&pc[2].desc,"2");assert(!GET_DEITY((&pc[2].ch)));
  /* Durable registry failures before and after rename reconcile to disk. */
  before=temple_count;durable_file_fault=3;admin("create 9 1 Failed Temple");
  assert(temple_count==before && temple_index(9)<0);
  durable_file_fault=5;admin("create 9 1 Reconciled Temple");
  assert(temple_count==before+1 && temple_index(9)>=0);
  durable_file_fault=0;assert(religion_load_temples(argv[1]) && temple_index(9)>=0);
  /* Registry errors fail closed and cannot overwrite unknown content. */
  file_text(argv[1],"{\"version\":1,\"temples\":[{\"id\":\"bad\"}]}");
  assert(!religion_load_temples(argv[1]) && !temples_ready);
  assert(!present_priest(&pc[1].ch,1,true));GET_DEITY((&pc[1].ch))=0;
  run(1,"devote Mishakal");assert(!pc[1].ps.religion_pending_action);
  file_text(argv[1],"{\"version\":1,\"temples\":[]}");assert(religion_load_temples(argv[1]));
  puts("religion: persistence, membership, temples, hospitality, channel privacy, mobs, purchases and guard integration passed");
  return 0;
}
'''
constants = (root / 'src/constants.c').read_text()
for first, last in [('const char *apply_type_food_names[]', 'CHECK_TABLE_SIZE(apply_type_food_names'),
                    ('const int apply_type_food_or_drink[]', 'CHECK_TABLE_SIZE(apply_type_food_or_drink')]:
    code += between(constants, first, last)

with tempfile.TemporaryDirectory(prefix='religion-test-') as directory:
    directory = Path(directory)
    source = directory / 'test.c'
    source.write_text(code)
    binary = directory / 'test'
    flags = os.environ.get('RELIGION_TEST_FLAGS', '').split()
    subprocess.run(['cc', '-std=gnu99', '-g', '-ffunction-sections', '-fdata-sections',
                    '-DLEGENDARY_TRANSACTION_TEST',
                    '-I', str(root / 'src'), *flags, str(source), str(root / 'src/durable_file.c'),
                    '-Wl,--gc-sections', '-ljson-c', '-o', str(binary)], check=True)
    subprocess.run([str(binary), str(directory / 'temples.json')], check=True)
