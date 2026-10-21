#!/usr/bin/env python3
"""Run production identity, disguise commands, and pfile codecs with deterministic rolls.

Usage: python3 tests/run_introductions_disguises.py
No database, running server, or player files are required.
"""
from pathlib import Path
import os
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
src = root / 'src'

def function(file, signature):
    text = (src / file).read_text()
    start = text.index(signature + '\n{')
    return text[start:text.index('\n}', start) + 2] + '\n'

headers = '\n'.join('#include "' + h + '"' for h in [
    'conf.h', 'sysdep.h', 'structs.h', 'utils.h', 'comm.h', 'interpreter.h',
    'db.h', 'handler.h', 'act.h', 'spells.h', 'char_descs.h', 'roleplay.h',
    'race.h', 'mud_event.h', 'spec_procs.h', 'dg_scripts.h', 'domains_schools.h', 'constants.h'])
stubs = r'''
#include <assert.h>
#define MAX(a,b) ((a)>(b)?(a):(b))
#define MIN(a,b) ((a)<(b)?(a):(b))
#undef str_cmp
#undef ACMD
#define ACMD(name) void name(struct char_data *ch, const char *argument, int cmd, int subcmd)
#undef CAN_SEE
#define CAN_SEE(o,s) (!AFF_FLAGGED(o, AFF_BLIND) && (!AFF_FLAGGED(s, AFF_INVISIBLE) || has_true_sight(o)))
#undef CAN_INFRA
#define CAN_INFRA(o,s) 0
#undef CREATE
#define CREATE(p,t,n) do { (p) = calloc((n), sizeof(t)); assert(p); } while (0)
#define USE_STANDARD_ACTION(ch) ((void)0)
#define USE_MOVE_ACTION(ch) ((void)0)
struct index_data test_mob_index[4], *mob_index = test_mob_index;
mob_rnum top_of_mobt = 3;
struct char_data *character_list;
bool can_blindsense_creature(struct char_data *ch, struct char_data *target) { return FALSE; }
size_t strlcpy(char *dest, const char *source, size_t size) {
  size_t n=strlen(source); if (size) snprintf(dest,size,"%s",source); return n;
}
struct config_data config_info;
struct player_special_data dummy_mob;
void basic_mud_log(const char *fmt, ...) {}
char *CAP(char *text) { if (*text) *text=toupper((unsigned char)*text); return text; }
struct race_data race_list[NUM_EXTENDED_RACES];
struct room_data rooms[2], *world = rooms;
const char *character_ages[] = {"adult", "adolescent", "middle-aged", "old-aged", "venerable", "\n"};
const char *dirs[NUM_OF_DIRS] = {"north", "east", "south", "west", "up", "down"};
const char *genders[] = {"neutral", "male", "female", "\n"};
static bool test_poisonable=TRUE, test_blindable=TRUE;
static int test_save, test_ray_hit=1, test_ray_attacks, test_ray_damage;
int attack_roll(struct char_data *ch, struct char_data *vict, int type, int touch, int mode) {
  assert(type==ATTACK_TYPE_RANGED && touch); test_ray_attacks++; return test_ray_hit;
}
int mag_damage(int level, struct char_data *ch, struct char_data *vict, struct obj_data *obj, int spell, int meta, int save, int type) { test_ray_damage++; return 1; }
void mag_affects(int level, struct char_data *ch, struct char_data *vict, struct obj_data *obj, int spell, int save, int type, int meta) {}
void mag_points(int level, struct char_data *ch, struct char_data *vict, struct obj_data *obj, int spell, int save, int type) {}

int dice(int number, int size) { return number*size; }
bool can_poison(struct char_data *ch) { return test_poisonable; }
bool can_blind(struct char_data *ch) { return test_blindable; }
int get_feat_value(struct char_data *ch, int feat) { return HAS_REAL_FEAT(ch,feat); }
int savingthrow(struct char_data *ch, struct char_data *vict, int type, int modifier, int casttype, int level, int school) { return test_save; }
int savingthrow_full(struct char_data *ch, struct char_data *vict, int type, int modifier, int casttype, int level, int school, int spell) { return test_save; }
static int rolls, saves, roll_value = 10, disguise_bonus, perception_bonus;
static char output[65536];
static struct char_data *alice, *bob, *carol;
int str_cmp(const char *a, const char *b) { return strcasecmp(a, b); }
int is_abbrev(const char *a, const char *b) { return *a && !strncasecmp(a,b,strlen(a)); }
void skip_spaces(char **s) { while (isspace((unsigned char)**s)) ++*s; }
void skip_spaces_c(const char **s) { while (isspace((unsigned char)**s)) ++*s; }
int d20(struct char_data *ch) { rolls++; return roll_value; }
int compute_ability(struct char_data *ch, int ability) {
  struct affected_type *af; int bonus=ability==ABILITY_DISGUISE?disguise_bonus:perception_bonus;
  for(af=ch->affected;af;af=af->next)
    if(af->location==APPLY_SKILL && af->specific==ability) bonus+=af->modifier;
  return bonus;
}
bool has_true_sight(struct char_data *ch) { return AFF_FLAGGED(ch, AFF_TRUE_SIGHT) || HAS_REAL_FEAT(ch,FEAT_TRUE_SIGHT); }
void save_char(struct char_data *ch, int mode) { saves++; }
size_t send_to_char(struct char_data *ch, const char *fmt, ...) {
  va_list ap; int n; va_start(ap,fmt);
  n = vsnprintf(output+strlen(output), sizeof(output)-strlen(output),fmt,ap);
  va_end(ap); return n;
}
const char *act(const char *s, int hide, struct char_data *ch, struct obj_data *obj, void *vict, int type) { return s; }
size_t write_to_output(struct descriptor_data *d, const char *fmt, ...) {
  va_list ap; int n; va_start(ap,fmt);
  n=vsnprintf(output+strlen(output),sizeof(output)-strlen(output),fmt,ap);
  va_end(ap); return n;
}
int isname(const char *needle, const char *list) {
  char text[MAX_STRING_LENGTH], *word;
  snprintf(text,sizeof(text),"%s",list);
  for(word=strtok(text," \t\r\n");word;word=strtok(NULL," \t\r\n"))
    if (!strcasecmp(word,needle)) return 1;
  return 0;
}
const char *two_arguments(const char *arg, char *one, size_t n1, char *two, size_t n2) {
  size_t n=0; skip_spaces_c(&arg);
  while (*arg && !isspace((unsigned char)*arg)) { if(n+1<n1) one[n++]=*arg; arg++; } one[n]=0;
  skip_spaces_c(&arg); n=0;
  while (*arg && !isspace((unsigned char)*arg)) { if(n+1<n2) two[n++]=*arg; arg++; } two[n]=0;
  return arg;
}
struct mud_event_data *char_has_mud_event(struct char_data *ch, event_id id) { return NULL; }
void column_list(struct char_data *ch, int n, const char **list, int len, bool sort) {}
int get_line(FILE *f, char *s) {
  if (!fgets(s,MAX_INPUT_LENGTH,f)) return 0;
  s[strcspn(s,"\r\n")]=0; return 1;
}
'''
stubs += r'''
char cast_arg2[MAX_INPUT_LENGTH];
bool affected_by_spell(struct char_data *ch, int spell) {
  struct affected_type *af; for(af=ch->affected;af;af=af->next) if(af->spell==spell) return TRUE;
  return FALSE;
}
void new_affect(struct affected_type *af) { memset(af,0,sizeof(*af)); }
void affect_total(struct char_data *ch) {}
void affect_to_char(struct char_data *ch, struct affected_type *af) {
  struct affected_type *copy=malloc(sizeof(*copy)); assert(copy); *copy=*af;
  copy->next=ch->affected; ch->affected=copy;
}
void affect_from_char(struct char_data *ch, int spell) {
  struct affected_type **cursor=&ch->affected;
  while(*cursor) {
    struct affected_type *af=*cursor;
    if(af->spell==spell) { *cursor=af->next; free(af); } else cursor=&af->next;
  }
  /* The production affect_remove cleanup is injected below. */
    if (!IS_NPC(ch) && spell==SPELL_ALTER_SELF && !affected_by_spell(ch,spell))
      ch->player_specials->saved.alter_self_active=FALSE;

}

'''
tables = (src / 'constants.c').read_text()
for signature in ('const char *shape_types[', 'const char *race_family_types['):
    table_start = tables.index(signature)
    stubs += tables[table_start:tables.index(';', table_start) + 1] + '\n'
code = ''
intro = (src / 'introduce.c').read_text()
code += function('utils.c', 'int compute_current_size(struct char_data *ch)')
code += function('introduce.c', 'bool knows_character(struct char_data *ch, struct char_data *vict)')
code += function('introduce.c', 'bool add_introduction(struct char_data *ch, struct char_data *vict)')
code += intro[intro.index('/* True sight is a temporary override'):]
descs = (src / 'char_descs.c').read_text()
code += descs[descs.index('const char *const eye_descriptions'):descs.index('void short_desc_descriptors_menu')]
code += function('char_descs.c', 'int count_adjective_types(int which_desc)')
code += function('char_descs.c', 'void short_desc_descriptors_menu(struct char_data *ch)')
code += function('char_descs.c', 'void short_desc_adjectives_menu(struct char_data *ch, int which_desc)')
code += function('utils.c', 'char *which_desc(struct char_data *ch, struct char_data *target)')
code += function('char_descs.c', 'char *show_pers(struct char_data *ch, struct char_data *vict)')
code += function('act.other.c', 'int display_eligible_disguise_races(struct char_data *ch, const char *argument, int silent)')
code += function('handler.c', 'int get_number(char **name)')
handler = (src / 'handler.c').read_text()
a = handler.index('struct char_data *get_player_no_vis(')
b = handler.index('struct char_data *get_char_vis(', a)
code += handler[a:b]
code += function('handler.c', 'struct char_data *get_char_vis(struct char_data *ch, char *name, int *number, int where)')
spells = (src / 'spells.c').read_text()
a = spells.index('int find_alter_self_race(')
code += spells[a:spells.index('ASPELL(spell_polymorph)', a)]
code += function('act.other.c', 'ACMD(do_disguise)')
code += function('introduce.c', 'ACMD(do_introduce)')
code += function('players.c', 'static void load_introductions(FILE *fl, struct char_data *ch)')
players = (src / 'players.c').read_text()
a = players.index('  if (ch->player_specials->saved.disguise_generation)\n    BUFFER_WRITE')
b = players.index('  /* Save introduction list */', a)
load_a = players.index('        if (!strcmp(tag, "AlRc"))')
load_a = players.index('{', load_a) + 1
load_b = players.index('\n        }', load_a)
code += '\nstatic void load_active_race(struct char_data *ch, const char *line) {' + players[load_a:load_b] + '\n}\n'
code += '\nstatic void save_identity(struct char_data *ch) {\n#define BUFFER_WRITE(...) do { char part[2048]; int len=snprintf(part,sizeof(part),__VA_ARGS__); assert(len>=0 && len<sizeof(part)); send_to_char(ch,"%s",part); } while(0)\n' + players[a:b] + '\n#undef BUFFER_WRITE\n}\n'
# Exercise the actual racial spell eligibility, cooldown and effect code.
code += function('spell_parser.c', 'bool isAurakMagic(struct char_data *ch, int spellnum)')
code += function('spell_parser.c', 'bool prepare_aurak_magic(struct char_data *ch, int spellnum, int casttype)')
magic = (src / 'magic.c').read_text()
a = magic.index('void mag_loops(')
code += magic[a:magic.index('\n}', a) + 2] + '\n'

def spell_case(start, end):
    a = magic.index('  case ' + start + ':')
    b = magic.index('  case ' + end + ':', a)
    return magic[a:b]
code += "\nstatic void test_aurak_breath(struct char_data *ch, struct char_data *victim, struct affected_type *af) {\nint casttype=CAST_INNATE, level=10, spellnum=SPELL_AURAK_BREATH; const char *to_vict=NULL,*to_room=NULL; (void)to_vict; (void)to_room;\nswitch(SPELL_AURAK_BREATH) {\n" + spell_case('SPELL_AURAK_BREATH', 'SPELL_BLINDNESS') + "default: assert(0); } }\n"
code += "\nstatic void test_aurak_damage(int spell, int *count, int *element_out, int *save_out) {\nint num_dice=0,size_dice=0,bonus=0,element=0,save=0,mag_resist=0;\nswitch(spell) {\n" + spell_case('SPELL_AURAK_DEATH_THROES', 'SPELL_SCORCHING_RAY') + "default: assert(0); } assert(size_dice==6 && bonus==0); assert(mag_resist==(spell==SPELL_AURAK_ENERGY_RAY)); *count=num_dice; *element_out=element; *save_out=save; }\n"
players_text = (src / 'players.c').read_text()
a = players_text.index('        if (!strcmp(tag, "AuDm")')
b = players_text.index('        if (!strcmp(tag, "AiFm")', a)
code += '\nstatic void load_aurak_timers(struct char_data *ch,const char *tag,const char *line) {\n' + players_text[a:b] + '\n}\n'
# Gate invocation must remain after combat/room protection and before effect dispatch.
parser = (src / 'spell_parser.c').read_text()
assert parser.index('if (!prepare_aurak_magic(caster, spellnum, casttype))') > parser.index('/* PVP CHECK')
assert parser.index('if (!prepare_aurak_magic(caster, spellnum, casttype))') < parser.index('/* the rest of the routine handling follows: */')
checks = r'''
static struct char_data *new_pc(char *name, long id) {
  struct char_data *ch = calloc(1,sizeof(*ch)); assert(ch);
  ch->player_specials=calloc(1,sizeof(*ch->player_specials)); assert(ch->player_specials);
  ch->player.name=name; GET_IDNUM(ch)=id; GET_LEVEL(ch)=5;
  GET_POS(ch)=POS_STANDING; GET_SEX(ch)=SEX_MALE; GET_REAL_RACE(ch)=0; ch->points.size=SIZE_MEDIUM;
  return ch;
}
static void free_pc(struct char_data *ch) {
  int i; free_disguise_observations(ch); free_sivak_forms(ch);
  while(ch->affected) { struct affected_type *af=ch->affected; ch->affected=af->next; free(af); }
  for(i=0;i<MAX_INTROS;i++) free(ch->player_specials->saved.intro_list[i]);
  free(ch->player_specials); free(ch);
}
int main(void) {
  int i, before, count; char saved[65536], *line, *desc;
  struct char_data *reloaded;
  FILE *file;
  alice=new_pc("Alice",1); bob=new_pc("Bob",2); carol=new_pc("Carol",3);
  CONFIG_USE_INTRO_SYSTEM=1;
  character_list=alice; alice->next=bob; bob->next=carol;
  world[0].people=alice; alice->next_in_room=bob; bob->next_in_room=carol;
  for(i=0;i<NUM_EXTENDED_RACES;i++) race_list[i].name="unavailable";
  race_list[0].name="human"; race_list[0].family=RACE_TYPE_HUMANOID; race_list[0].size=SIZE_MEDIUM;
  race_list[1].name="elf"; race_list[1].family=RACE_TYPE_HUMANOID; race_list[1].size=SIZE_MEDIUM;
  assert(!knows_character(bob,alice)); assert(knows_character(alice,alice));
  do_introduce(alice,"human",0,0); assert(knows_character(bob,alice));
  assert(strstr(output,"Alice introduces himself"));
  assert(!knows_character(alice,bob)); assert(!add_introduction(bob,alice));
  { char name[]="2.human"; assert(get_char_vis(alice,name,NULL,FIND_CHAR_ROOM)==carol); }
  { char name[]="Bob"; assert(!get_char_vis(alice,name,NULL,FIND_CHAR_ROOM)); }
  { char name[]="0.Bob"; assert(!get_char_vis(alice,name,NULL,FIND_CHAR_ROOM)); }
  { char name[]="Alice"; assert(get_char_vis(bob,name,NULL,FIND_CHAR_ROOM)==alice); }
  world[0].people=NULL;
  assert(!identity_matches(carol,alice,"Alice")); assert(identity_matches(carol,alice,"human"));
  assert(!strcmp(PERS(alice,bob),"Alice")); assert(strstr(PERS(alice,carol),"human"));
  SET_BIT_AR(MOB_FLAGS(carol),MOB_ISNPC); assert(knows_character(bob,carol));
  REMOVE_BIT_AR(MOB_FLAGS(carol),MOB_ISNPC);
  GET_LEVEL(carol)=LVL_IMMORT; assert(knows_character(carol,alice)); GET_LEVEL(carol)=5;
  CONFIG_USE_INTRO_SYSTEM=0; assert(knows_character(carol,alice)); CONFIG_USE_INTRO_SYSTEM=1;
  /* Disguise selection and menus use the natural description tables. */
  before=rolls; *output=0; do_disguise(alice,"descriptors hair",0,0);
  assert(strstr(output,"43) short silver hair") && rolls==before);
  for(i=1;i<=NUM_FEATURE_TYPES;i++) {
    int last=count_adjective_types(i)-1;
    assert(valid_short_desc_feature(i,1) && valid_short_desc_feature(i,last));
    assert(!valid_short_desc_feature(i,0) && !valid_short_desc_feature(i,last+1));
  }
  assert(valid_short_desc_feature(0,0) && !valid_short_desc_feature(99,1));
  { int f,a; assert(parse_short_desc_feature("hair:43",&f,&a) && f==FEATURE_TYPE_HAIR && a==43);
    assert(parse_short_desc_feature("1:2",&f,&a) && f==FEATURE_TYPE_EYES && a==2);
    assert(!parse_short_desc_feature("hair:9999",&f,&a));
    assert(!parse_short_desc_feature("hair:-1",&f,&a));
    assert(!parse_short_desc_feature("hair:1junk",&f,&a)); }
  /* Human race zero is a valid disguise. Actual race, age, gender stay unchanged. */
  GET_ABILITY(alice,ABILITY_DISGUISE)=1;
  disguise_bonus=20; before=rolls;
  do_disguise(alice,"human | adolescent | female | hair:43 | eyes:2",0,0);
  assert(rolls==before+1 && HAS_DISGUISE(alice));
  assert(COSMETIC_DISGUISE(alice).roll==30 && COSMETIC_DISGUISE(alice).instance==1);
  assert(GET_RACE(alice)==0 && GET_CH_AGE(alice)==0 && GET_SEX(alice)==SEX_MALE);
  desc=current_disguise_desc(alice); assert(!strcmp(desc,"an adolescent female human with short silver hair and green eyes")); free(desc);
  assert(COSMETIC_DISGUISE(alice).descriptor1==FEATURE_TYPE_HAIR);
  assert(COSMETIC_DISGUISE(alice).adjective1==43);
  assert(GET_PC_DESCRIPTOR_1(alice)==0 && GET_PC_ADJECTIVE_1(alice)==0);
  /* Exact shared formatting, including complexion and the second feature. */
  {
    struct disguise_appearance saved_appearance=COSMETIC_DISGUISE(alice);
    char *normal, *disguised;
    for(i=1;i<=NUM_FEATURE_TYPES;i++) {
      GET_PC_DESCRIPTOR_1(alice)=i; GET_PC_ADJECTIVE_1(alice)=1;
      GET_PC_DESCRIPTOR_2(alice)=FEATURE_TYPE_COMPLEXION; GET_PC_ADJECTIVE_2(alice)=1;
      COSMETIC_DISGUISE(alice).race=GET_RACE(alice);
      COSMETIC_DISGUISE(alice).age=GET_CH_AGE(alice); COSMETIC_DISGUISE(alice).sex=GET_SEX(alice);
      COSMETIC_DISGUISE(alice).descriptor1=i; COSMETIC_DISGUISE(alice).adjective1=1;
      COSMETIC_DISGUISE(alice).descriptor2=FEATURE_TYPE_COMPLEXION; COSMETIC_DISGUISE(alice).adjective2=1;
      normal=current_short_desc(alice); disguised=current_disguise_desc(alice);
      assert(!strcmp(normal,disguised)); free(normal); free(disguised);
    }
    GET_PC_DESCRIPTOR_1(alice)=GET_PC_ADJECTIVE_1(alice)=0;
    GET_PC_DESCRIPTOR_2(alice)=GET_PC_ADJECTIVE_2(alice)=0;
    COSMETIC_DISGUISE(alice)=saved_appearance;
  }
  /* A failed observer cannot recognize a previously introduced character. */
  before=rolls; assert(!disguise_detected(bob,alice)); assert(rolls==before+1);
  assert(!identity_known(bob,alice)); assert(!identity_matches(bob,alice,"Alice"));
  assert(strstr(PERS(alice,bob),"silver hair")); assert(visible_sex(alice,bob)==SEX_FEMALE);
  perception_bonus=100;
  for(i=0;i<20;i++) assert(!disguise_detected(bob,alice));
  assert(rolls==before+1);
  IN_ROOM(bob)=1; assert(!disguise_detected(bob,alice)); IN_ROOM(bob)=0;
  /* True sight overrides failure without a new roll or cached permanent success. */
  SET_BIT_AR(AFF_FLAGS(bob),AFF_TRUE_SIGHT);
  assert(disguise_detected(bob,alice)); assert(!strcmp(PERS(alice,bob),"Alice"));
  assert(visible_sex(alice,bob)==SEX_MALE); assert(rolls==before+1);
  *output=0; show_disguise_notice(alice,bob);
  assert(strstr(output,"Alice\'s true form is an adult male human"));
  assert(strstr(output,"Alice is disguised as an adolescent female human with short silver hair and green eyes.\r\n"));
  REMOVE_BIT_AR(AFF_FLAGS(bob),AFF_TRUE_SIGHT); assert(!disguise_detected(bob,alice));
  *output=0; show_disguise_notice(alice,bob); assert(!*output);
  /* A successful stranger still never learns the real name. Ties succeed. */
  perception_bonus=20; before=rolls; assert(disguise_detected(carol,alice));
  assert(rolls==before+1 && !identity_known(carol,alice));
  *output=0; show_disguise_notice(alice,carol);
  assert(!strstr(output,"Alice") && strstr(output,"is disguised as"));
  assert(!strstr(PERS(alice,carol),"silver hair"));
  /* Cache success survives distance; unseen targets do not consume a check. */
  IN_ROOM(carol)=1; assert(disguise_detected(carol,alice)); IN_ROOM(carol)=0;
  /* Actual production save block round-trips appearance and observer outcome. */
  *output=0; save_identity(alice); snprintf(saved,sizeof(saved),"%s",output);
  reloaded=new_pc("Alice",1);
  line=strstr(saved,"DgA2: "); assert(line); line[strcspn(line,"\n")]=0;
  load_cosmetic_disguise(reloaded,line+6);
  assert(!memcmp(&COSMETIC_DISGUISE(alice),&COSMETIC_DISGUISE(reloaded),sizeof(struct disguise_appearance)));
  assert(reloaded->player_specials->saved.disguise_generation==1);
  *output=0; save_identity(bob); snprintf(saved,sizeof(saved),"%s",output);
  free_disguise_observations(bob); line=strstr(saved,"DgOb: "); assert(line);
  load_disguise_observation(bob,line+6); before=rolls;
  assert(!disguise_detected(bob,reloaded) && rolls==before);
  free_pc(reloaded);
  /* Bad persistent values are ignored; duplicate observer entries are updated. */
  load_cosmetic_disguise(alice,"12 9999 0 1 20 invalid"); assert(COSMETIC_DISGUISE(alice).instance==1);
  load_cosmetic_disguise(alice,"12 0 0 1 20 6 9999 1 2"); assert(COSMETIC_DISGUISE(alice).instance==1);
  load_disguise_observation(bob,"1 1 0"); load_disguise_observation(bob,"1 1 0");
  count=0; { struct disguise_observation *e; for(e=bob->player_specials->saved.disguise_observations;e;e=e->next) count++; }
  assert(count==1);
  /* Remove and reapply creates exactly one new opportunity for each observer. */
  do_disguise(alice,"remove",0,0); assert(!HAS_DISGUISE(alice));
  assert(!strcmp(PERS(alice,bob),"Alice"));
  do_disguise(alice,"elf | adult | female | hair:31",0,0);
  assert(COSMETIC_DISGUISE(alice).instance==2 && GET_RACE(alice)==0);
  before=rolls; assert(disguise_detected(bob,alice)); assert(rolls==before+1);
  assert(!strcmp(PERS(alice,bob),"Alice"));
  do_disguise(alice,"remove",0,0);
  /* First sight through true sight stores no perception result. */
  do_disguise(alice,"human",0,0);
  SET_BIT_AR(AFF_FLAGS(carol),AFF_TRUE_SIGHT); before=rolls;
  assert(disguise_detected(carol,alice) && rolls==before);
  *output=0; show_pending_disguise_notice(alice,carol); assert(strstr(output,"is disguised as"));
  *output=0; show_pending_disguise_notice(alice,carol); assert(!*output);
  REMOVE_BIT_AR(AFF_FLAGS(carol),AFF_TRUE_SIGHT);
  assert(disguise_detected(carol,alice) && rolls==before+1);
  assert(disguise_detected(carol,alice) && rolls==before+1);
  do_disguise(alice,"remove",0,0);
  before=rolls; do_disguise(alice,"elf | bad-age | female",0,0); assert(!HAS_DISGUISE(alice)&&rolls==before);
  do_disguise(alice,"elf | adult | female | $n injection",0,0); assert(!HAS_DISGUISE(alice)&&rolls==before);
  do_disguise(alice,"elf | adult | female | ok | extra",0,0); assert(!HAS_DISGUISE(alice)&&rolls==before);
  SET_BIT_AR(AFF_FLAGS(alice),AFF_WILD_SHAPE); GET_DISGUISE_RACE(alice)=1;
  do_disguise(alice,"human",0,0); assert(!HAS_DISGUISE(alice)&&rolls==before);
  REMOVE_BIT_AR(AFF_FLAGS(alice),AFF_WILD_SHAPE); GET_DISGUISE_RACE(alice)=0;
  do_disguise(alice,"elf",0,0);
  *output=0; do_introduce(alice,"human",0,0); assert(strstr(output,"Remove your disguise"));
  /* Adjacent-room scanning is actual sight, while a remote listing is not. */
  IN_ROOM(carol)=1; before=rolls;
  assert(!disguise_detected(carol,alice) && rolls==before);
  observe_disguise(carol,alice); assert(disguise_detected(carol,alice) && rolls==before+1);
  IN_ROOM(carol)=0;
  SET_BIT_AR(AFF_FLAGS(bob),AFF_BLIND); before=rolls;
  assert(!disguise_detected(bob,alice)&&rolls==before);
  REMOVE_BIT_AR(AFF_FLAGS(bob),AFF_BLIND);
  IN_ROOM(bob)=1; assert(!disguise_detected(bob,alice)&&rolls==before); IN_ROOM(bob)=0;
  assert(disguise_detected(bob,alice)&&rolls==before+1);
  /* A full introduction list consumes its terminator and leaves the next tag. */
  file=tmpfile(); assert(file);
  for(i=0;i<MAX_INTROS;i++) fprintf(file,"Person%d\n",i);
  fprintf(file,"~\nNEXT\n"); rewind(file);
  reloaded=new_pc("Reloaded",4); load_introductions(file,reloaded);
  assert(reloaded->player_specials->saved.intro_list[MAX_INTROS-1]);
  { char next[32]; assert(fgets(next,sizeof(next),file)&&!strcmp(next,"NEXT\n")); }
  fclose(file); free_pc(reloaded);
  file=tmpfile(); assert(file); fputs("~\nNEXT\n",file); rewind(file);
  reloaded=new_pc("Reloaded",4); load_introductions(file,reloaded);
  { char next[32]; assert(fgets(next,sizeof(next),file)&&!strcmp(next,"NEXT\n")); }
  fclose(file); free_pc(reloaded);
  /* Default copies natural IDs, and explicit none clears both slots. */
  do_disguise(alice,"remove",0,0);
  GET_PC_DESCRIPTOR_1(alice)=FEATURE_TYPE_HAIR; GET_PC_ADJECTIVE_1(alice)=1;
  GET_PC_DESCRIPTOR_2(alice)=FEATURE_TYPE_EYES; GET_PC_ADJECTIVE_2(alice)=2;
  do_disguise(alice,"human",0,0);
  assert(COSMETIC_DISGUISE(alice).descriptor1==FEATURE_TYPE_HAIR && COSMETIC_DISGUISE(alice).adjective1==1);
  assert(COSMETIC_DISGUISE(alice).descriptor2==FEATURE_TYPE_EYES && COSMETIC_DISGUISE(alice).adjective2==2);
  GET_PC_ADJECTIVE_1(alice)=2;
  desc=current_disguise_desc(alice); assert(strstr(desc,"short red hair")); free(desc);
  do_disguise(alice,"remove",0,0);
  do_disguise(alice,"human | adult | male | none | none",0,0);
  assert(HAS_DISGUISE(alice) && !COSMETIC_DISGUISE(alice).descriptor1 && !COSMETIC_DISGUISE(alice).descriptor2);
  do_disguise(alice,"remove",0,0); before=rolls;
  do_disguise(alice,"human | adult | male | hair:9999999999999999999999",0,0);
  assert(!HAS_DISGUISE(alice) && rolls==before);
  do_disguise(alice,"human | adult | male | hair:1 | eyes:2 | extra",0,0);
  assert(!HAS_DISGUISE(alice) && rolls==before);
  /* Disguise Self is only a timed +10 skill effect; it never changes race or appearance. */
  {
    struct affected_type af[4]={{0}};
    const char *to_vict=NULL;
    int level=3, accum_affect=0, accum_duration=0;
    struct char_data *victim=alice;
    (void)accum_affect; (void)accum_duration; (void)victim;
    switch(SPELL_DISGUISE_SELF) {
        case SPELL_DISGUISE_SELF:
    af[0].location = APPLY_SKILL;
    af[0].specific = ABILITY_DISGUISE;
    af[0].modifier = 10;
    af[0].duration = MAX(1, level) * 100;
    af[0].bonus_type = BONUS_TYPE_COMPETENCE;
    to_vict = "Your ability to disguise yourself is enhanced.";
    break;


      default: assert(0);
    }
    assert(af[0].location==APPLY_SKILL && af[0].specific==ABILITY_DISGUISE);
    assert(af[0].modifier==10 && af[0].duration==300);
    assert(to_vict); af[0].spell=SPELL_DISGUISE_SELF;
    affect_to_char(alice,&af[0]);
    assert(compute_ability(alice,ABILITY_DISGUISE)==disguise_bonus+10);
    assert(!IS_ALTER_SELF(alice) && GET_RACE(alice)==GET_REAL_RACE(alice));
  }
  {
    struct affected_type *af;
    unsigned long long instance;
    int old_rolls, found;
    race_list[0].is_pc=TRUE;
    race_list[1].is_pc=TRUE;
    race_list[5].name="smallfolk"; race_list[5].is_pc=TRUE;
    race_list[5].family=RACE_TYPE_HUMANOID; race_list[5].size=SIZE_SMALL;
    race_list[6].name="wolf"; race_list[6].is_pc=FALSE;
    race_list[6].family=RACE_TYPE_ANIMAL; race_list[6].size=SIZE_MEDIUM;
    race_list[7].name="largefolk"; race_list[7].is_pc=TRUE;
    race_list[7].family=RACE_TYPE_GIANT; race_list[7].size=SIZE_LARGE;
    assert(find_alter_self_race("human")==0 && find_alter_self_race(" smallfolk ")==5);
    assert(find_alter_self_race("wolf")==RACE_UNDEFINED);
    perception_bonus=-100;
    GET_ABILITY(alice,ABILITY_DISGUISE)=1;
    do_disguise(alice,"human | adult | male | hair:1 | eyes:2",0,0);
    instance=COSMETIC_DISGUISE(alice).instance;
    assert(!disguise_detected(bob,alice)); old_rolls=rolls;
    /* Full per-caster race choices survive another caster overwriting the global argument. */
    snprintf(alice->player_specials->appearance_cast_args,sizeof(alice->player_specials->appearance_cast_args),"smallfolk");
    snprintf(cast_arg2,sizeof(cast_arg2),"wolf");
    spell_alter_self(4,alice,alice,NULL,CAST_SPELL);
    assert(IS_ALTER_SELF(alice) && GET_RACE(alice)==5 && GET_REAL_RACE(alice)==0 && GET_SIZE(alice)==SIZE_SMALL);
    assert(rolls==old_rolls && COSMETIC_DISGUISE(alice).instance==instance);
    assert(affected_by_spell(alice,SPELL_DISGUISE_SELF)); /* The skill bonus coexists. */
    /* A failed perception check still sees the changed race and disguised descriptors. */
    desc=which_desc(bob,alice); assert(strstr(desc,"smallfolk") && strstr(desc,"short red hair")); free(desc);
    assert(!disguise_detected(bob,alice) && rolls==old_rolls);
    SET_BIT_AR(AFF_FLAGS(bob),AFF_TRUE_SIGHT);
    desc=which_desc(bob,alice); assert(strstr(desc,"human") && !strstr(desc,"smallfolk") && !strstr(desc,"short red hair")); free(desc);
    assert(perceived_race(alice,bob)==0 && GET_RACE(alice)==5 && GET_SIZE(alice)==SIZE_SMALL);
    *output=0; show_disguise_notice(alice,bob);
    assert(strstr(output,"true form is") && strstr(output,"altered form is") && strstr(output,"smallfolk") && strstr(output,"(alter self)"));
    assert(strstr(output,"is disguised as"));
    REMOVE_BIT_AR(AFF_FLAGS(bob),AFF_TRUE_SIGHT);
    assert(perceived_race(alice,bob)==5);
    found=0;
    for(af=alice->affected;af;af=af->next) if(af->spell==SPELL_ALTER_SELF) {
      assert(af->duration==40 && af->location==APPLY_DEX && af->modifier==2);
      assert(af->bonus_type==BONUS_TYPE_RACIAL && !IS_SET_AR(af->bitvector,AFF_MIND_BLANK)); found++;
    }
    assert(found==1);
    /* NPC-only forms are rejected without changing the active race or rolling perception. */
    spell_alter_self(4,alice,alice,NULL,CAST_SPELL); assert(GET_RACE(alice)==5 && rolls==old_rolls);
    /* Large player-selectable forms work and receive the Strength racial bonus. */
    snprintf(cast_arg2,sizeof(cast_arg2),"largefolk"); CASTING_METAMAGIC(alice)=METAMAGIC_EXTEND;
    spell_alter_self(4,alice,alice,NULL,CAST_SPELL); CASTING_METAMAGIC(alice)=0;
    assert(GET_RACE(alice)==7 && GET_SIZE(alice)==SIZE_LARGE);
    found=0; for(af=alice->affected;af;af=af->next) if(af->spell==SPELL_ALTER_SELF) {
      assert(af->location==APPLY_STR && af->modifier==2 && af->bonus_type==BONUS_TYPE_RACIAL && af->duration==80); found++;
    } assert(found==1);
    *output=0; save_identity(alice); assert(strstr(output,"AlRc: 7\n"));
    reloaded=new_pc("Alice",1); load_active_race(reloaded,"7");
    for(af=alice->affected;af;af=af->next) if(af->spell==SPELL_ALTER_SELF) affect_to_char(reloaded,af);
    reconcile_alter_self(reloaded); assert(GET_RACE(reloaded)==7 && GET_SIZE(reloaded)==SIZE_LARGE); free_pc(reloaded);
    reloaded=new_pc("Alice",1); load_active_race(reloaded,"6"); assert(!IS_ALTER_SELF(reloaded));
    load_active_race(reloaded,"garbage"); assert(!IS_ALTER_SELF(reloaded)); free_pc(reloaded);
    /* Expiration/dispel restores original race, preserving mundane disguise and the skill spell. */
    affect_from_char(alice,SPELL_ALTER_SELF);
    assert(!IS_ALTER_SELF(alice) && GET_RACE(alice)==0 && GET_SIZE(alice)==SIZE_MEDIUM);
    assert(COSMETIC_DISGUISE(alice).instance==instance && affected_by_spell(alice,SPELL_DISGUISE_SELF));
    /* Medium form: Strength; human race zero is valid. */
    snprintf(cast_arg2,sizeof(cast_arg2),"human"); spell_alter_self(2,alice,alice,NULL,CAST_SPELL);
    assert(IS_ALTER_SELF(alice) && GET_RACE(alice)==0);
    found=0; for(af=alice->affected;af;af=af->next) if(af->spell==SPELL_ALTER_SELF) {
      assert(af->location==APPLY_STR && af->modifier==2 && af->bonus_type==BONUS_TYPE_RACIAL); found++;
    } assert(found==1);
    /* Loading a stale/orphaned active-race field cannot leave permanent transformation. */
    reloaded=new_pc("Alice",1); ALTER_SELF_RACE(reloaded)=7;
    reloaded->player_specials->saved.alter_self_active=TRUE;
    reconcile_alter_self(reloaded); assert(!IS_ALTER_SELF(reloaded)); free_pc(reloaded);
    GET_ABILITY(alice,ABILITY_DISGUISE)=0;
    do_disguise(alice,"dismiss alter self",0,0);
    assert(!IS_ALTER_SELF(alice) && affected_by_spell(alice,SPELL_DISGUISE_SELF));
    do_disguise(alice,"dismiss disguise self",0,0);
    assert(!affected_by_spell(alice,SPELL_DISGUISE_SELF) && HAS_DISGUISE(alice));
    do_disguise(alice,"remove",0,0);
  }
  /* True sight exposes original traits and retains every transformed guise as context. */
  {
    int before_rolls=rolls;
    race_list[RACE_WOLF].name="wolf"; race_list[RACE_WOLF].family=RACE_TYPE_ANIMAL;
    race_list[RACE_BLACK_BEAR].name="black bear"; race_list[RACE_BLACK_BEAR].family=RACE_TYPE_ANIMAL;
    GET_DISGUISE_RACE(alice)=RACE_WOLF; IS_MORPHED(alice)=RACE_TYPE_ANIMAL;
    SET_BIT_AR(AFF_FLAGS(alice),AFF_WILD_SHAPE);
    desc=which_desc(bob,alice); assert(strstr(desc,"wolf") && strstr(desc,"wildshaped")); free(desc);
    SET_BIT_AR(AFF_FLAGS(bob),AFF_TRUE_SIGHT);
    desc=which_desc(bob,alice); assert(strstr(desc,"human") && !strstr(desc,"wolf")); free(desc);
    assert(GET_RACE(alice)==RACE_WOLF && GET_REAL_RACE(alice)==0);
    *output=0; show_disguise_notice(alice,bob);
    assert(strstr(output,"Alice's true form is") && strstr(output,"human"));
    assert(strstr(output,"Alice's altered form is a wolf (wildshaped)."));
    *output=0; show_pending_disguise_notice(alice,bob); assert(!*output);
    GET_DISGUISE_RACE(alice)=RACE_BLACK_BEAR;
    show_pending_disguise_notice(alice,bob); assert(strstr(output,"a black bear (wildshaped)"));
    *output=0; show_pending_disguise_notice(alice,bob); assert(!*output);
    /* An unintroduced viewer learns a true description, never the person's name. */
    SET_BIT_AR(AFF_FLAGS(carol),AFF_TRUE_SIGHT);
    *output=0; show_disguise_notice(alice,carol);
    assert(!strstr(output,"Alice") && strstr(output,"true form is") && strstr(output,"black bear"));
    /* Legacy animal-shape subtypes and generic creature-family shapechanges. */
    REMOVE_BIT_AR(AFF_FLAGS(alice),AFF_WILD_SHAPE); GET_DISGUISE_RACE(alice)=0;
    SUBRACE(alice)=PC_SUBRACE_BADGER;
    *output=0; show_disguise_notice(alice,bob);
    assert(strstr(output,"a badger (shapechanged)"));
    IS_MORPHED(alice)=RACE_TYPE_DRAGON; SUBRACE(alice)=0;
    *output=0; show_disguise_notice(alice,bob); assert(strstr(output,"a dragon (shapechanged)"));
    assert(rolls==before_rolls); /* No Perception rolls for true-sight form recognition. */
    /* Losing true sight returns to the altered view; feat-based sight works identically. */
    REMOVE_BIT_AR(AFF_FLAGS(bob),AFF_TRUE_SIGHT);
    desc=which_desc(bob,alice); assert(strstr(desc,"dragon") && !strstr(desc,"human")); free(desc);
    HAS_REAL_FEAT(bob,FEAT_TRUE_SIGHT)=1;
    desc=which_desc(bob,alice); assert(strstr(desc,"human") && !strstr(desc,"dragon")); free(desc);
    *output=0; show_disguise_notice(alice,bob); assert(strstr(output,"true form is") && strstr(output,"dragon"));
    SET_BIT_AR(AFF_FLAGS(bob),AFF_BLIND); *output=0; show_disguise_notice(alice,bob); assert(!*output);
    REMOVE_BIT_AR(AFF_FLAGS(bob),AFF_BLIND); HAS_REAL_FEAT(bob,FEAT_TRUE_SIGHT)=0;
    REMOVE_BIT_AR(AFF_FLAGS(carol),AFF_TRUE_SIGHT);
    IS_MORPHED(alice)=0; *output=0; show_disguise_notice(alice,bob); assert(!*output);
  }
  /* Irda innate appearance uses all shared selections, with no opposed rolls. */
  {
    int before_rolls=rolls;
    struct char_data *loaded;
    race_list[DL_RACE_IRDA].name="irda"; race_list[DL_RACE_IRDA].type="Irda";
    race_list[DL_RACE_IRDA].family=RACE_TYPE_HUMANOID;
    race_list[DL_RACE_IRDA].size=SIZE_MEDIUM;
    GET_REAL_RACE(alice)=DL_RACE_IRDA;
    GET_ABILITY(alice,ABILITY_DISGUISE)=0;
    do_disguise(alice,"form human | old-aged | female | hair:43 | eyes:2",0,0);
    assert(IS_IRDA_FORM(alice) && GET_RACE(alice)==0 && GET_REAL_RACE(alice)==DL_RACE_IRDA);
    assert(IRDA_APPEARANCE(alice).age==3 && IRDA_APPEARANCE(alice).sex==SEX_FEMALE);
    assert(IRDA_APPEARANCE(alice).descriptor1==FEATURE_TYPE_HAIR && IRDA_APPEARANCE(alice).adjective1==43);
    assert(IRDA_APPEARANCE(alice).descriptor2==FEATURE_TYPE_EYES && IRDA_APPEARANCE(alice).adjective2==2);
    desc=which_desc(bob,alice); assert(strstr(desc,"human") && strstr(desc,"silver hair")); free(desc);
    assert(visible_sex(alice,bob)==SEX_FEMALE && perceived_race(alice,bob)==0);
    assert(!identity_known(bob,alice) && !strstr(PERS(alice,bob),"Alice"));
    *output=0; show_disguise_notice(alice,bob); assert(!*output);
    SET_BIT_AR(AFF_FLAGS(bob),AFF_TRUE_SIGHT);
    desc=which_desc(bob,alice); assert(strstr(desc,"irda") && !strstr(desc,"silver hair")); free(desc);
    assert(visible_sex(alice,bob)==SEX_MALE && perceived_race(alice,bob)==DL_RACE_IRDA);
    *output=0; show_disguise_notice(alice,bob);
    assert(strstr(output,"true form is") && strstr(output,"altered form is") && strstr(output,"silver hair"));
    *output=0; save_identity(alice); assert(strstr(output,"AiFm:"));
    loaded=new_pc("Alice",1); GET_REAL_RACE(loaded)=DL_RACE_IRDA;
    { char *tag=strstr(output,"AiFm:"); load_irda_form(loaded,tag+5); }
    assert(IS_IRDA_FORM(loaded) && IRDA_APPEARANCE(loaded).sex==SEX_FEMALE);
    assert(IRDA_APPEARANCE(loaded).adjective1==43 && IRDA_APPEARANCE(loaded).adjective2==2);
    free_pc(loaded);
    loaded=new_pc("Alice",1); GET_REAL_RACE(loaded)=DL_RACE_IRDA;
    load_irda_form(loaded,"1 0 0 1 6 99999 1 2"); assert(!IS_IRDA_FORM(loaded));
    load_irda_form(loaded,"1 6 0 1 0 0 0 0"); assert(!IS_IRDA_FORM(loaded));
    load_irda_form(loaded,"1 0 99 1 0 0 0 0"); assert(!IS_IRDA_FORM(loaded));
    load_irda_form(loaded,"garbage"); assert(!IS_IRDA_FORM(loaded));
    free_pc(loaded);
    SET_BIT_AR(AFF_FLAGS(carol),AFF_TRUE_SIGHT);
    *output=0; show_disguise_notice(alice,carol); assert(!strstr(output,"Alice") && strstr(output,"irda") && strstr(output,"silver hair"));
    REMOVE_BIT_AR(AFF_FLAGS(carol),AFF_TRUE_SIGHT);
    REMOVE_BIT_AR(AFF_FLAGS(bob),AFF_TRUE_SIGHT);
    /* Small humanoids allowed; no size restriction and no racial stat spell. */
    do_disguise(alice,"form smallfolk | adult | neutral | none | none",0,0);
    assert(GET_RACE(alice)==5 && GET_SIZE(alice)==SIZE_SMALL);
    assert(!affected_by_spell(alice,SPELL_ALTER_SELF));
    do_disguise(alice,"form wolf",0,0); assert(GET_RACE(alice)==5);
    do_disguise(alice,"form human | adult | female | hair:99999",0,0); assert(GET_RACE(alice)==5);
    do_disguise(alice,"form remove",0,0); assert(!IS_IRDA_FORM(alice) && GET_RACE(alice)==DL_RACE_IRDA);
    do_disguise(bob,"form human",0,0); assert(!IS_IRDA_FORM(bob));
    assert(rolls==before_rolls);
  }
  /* Sivaks learn exact appearance snapshots from their own humanoid kills. */
  {
    struct char_data *npc, *loaded;
    struct sivak_form *form;
    unsigned long long pc_id, npc_id, generation;
    int old_rolls=rolls, n;
    char choice[64], record[512];
    race_list[DL_RACE_SIVAK_DRACONIAN].name="sivak draconian";
    race_list[DL_RACE_SIVAK_DRACONIAN].family=RACE_TYPE_HUMANOID;
    race_list[DL_RACE_SIVAK_DRACONIAN].size=SIZE_LARGE;
    GET_REAL_RACE(alice)=DL_RACE_SIVAK_DRACONIAN; alice->points.size=SIZE_LARGE;
    GET_REAL_RACE(bob)=0; GET_SEX(bob)=SEX_FEMALE;
    bob->player.description="A distinctive humanoid.\r\n";
    remember_sivak_kill(bob,alice); assert(!bob->player_specials->saved.sivak_forms);
    remember_sivak_kill(alice,alice); assert(!alice->player_specials->saved.sivak_forms);
    remember_sivak_kill(NULL,bob); remember_sivak_kill(alice,NULL);
    remember_sivak_kill(alice,bob);
    form=alice->player_specials->saved.sivak_forms;
    assert(form && !form->npc && form->source==GET_IDNUM(bob));
    assert(form->sex==SEX_FEMALE && form->race==0 && form->size==SIZE_MEDIUM);
    pc_id=form->id; assert(strstr(form->short_desc,"human"));
    assert(strstr(form->description,"distinctive"));
    snprintf(choice,sizeof(choice),"form %llu",pc_id); do_disguise(alice,choice,0,0);
    assert(IS_SIVAK_FORM(alice) && GET_RACE(alice)==0 && GET_SIZE(alice)==SIZE_MEDIUM);
    assert(GET_REAL_RACE(alice)==DL_RACE_SIVAK_DRACONIAN && visible_sex(alice,carol)==SEX_FEMALE);
    assert(!identity_known(bob,alice) && !strstr(PERS(alice,bob),"Alice"));
    desc=which_desc(carol,alice); assert(!strcmp(desc,form->short_desc)); free(desc);
    *output=0; show_disguise_notice(alice,carol); assert(!*output);
    SET_BIT_AR(AFF_FLAGS(carol),AFF_TRUE_SIGHT);
    desc=which_desc(carol,alice); assert(strstr(desc,"sivak draconian")); free(desc);
    *output=0; show_disguise_notice(alice,carol);
    assert(strstr(output,"true form is") && strstr(output,"altered form is") && strstr(output,"Sivak stolen form"));
    assert(!strstr(output,"Alice"));
    REMOVE_BIT_AR(AFF_FLAGS(carol),AFF_TRUE_SIGHT);
    /* Mob family IDs are not player race IDs. NPC text and actual size survive extraction. */
    npc=new_pc("guard",GET_IDNUM(bob)); SET_BIT_AR(MOB_FLAGS(npc),MOB_ISNPC);
    npc->nr=0; mob_index[0].vnum=GET_IDNUM(bob);
    GET_REAL_RACE(npc)=RACE_TYPE_HUMANOID; npc->points.size=SIZE_SMALL;
    npc->player.short_descr="a scarred watch captain";
    npc->player.description="A captain with a scar.\r\n";
    remember_sivak_kill(alice,npc);
    form=alice->player_specials->saved.sivak_forms; npc_id=form->id;
    assert(form->npc && form->source==GET_IDNUM(bob) && form->next && form->next->id==pc_id);
    assert(!strcmp(form->short_desc,"a scarred watch captain"));
    assert(strstr(form->description,"captain with a scar"));
    remember_sivak_kill(alice,npc);
    assert(alice->player_specials->saved.sivak_forms==form && form->next->id==pc_id && !form->next->next);
    assert(form->id==npc_id);
    /* No animal forms, arena defeats, or other people's kills. */
    generation=alice->player_specials->saved.disguise_generation;
    GET_REAL_RACE(npc)=RACE_TYPE_ANIMAL; remember_sivak_kill(alice,npc);
    GET_REAL_RACE(npc)=RACE_TYPE_HUMANOID;
    world[1].number=ARENA_VNUM_START; IN_ROOM(npc)=1; remember_sivak_kill(alice,npc); IN_ROOM(npc)=0;
    assert(alice->player_specials->saved.disguise_generation==generation);
    free_pc(npc);
    do_disguise(alice,"form remove",0,0); assert(!IS_SIVAK_FORM(alice) && GET_SIZE(alice)==SIZE_LARGE);
    snprintf(choice,sizeof(choice),"form %llu",npc_id); do_disguise(alice,choice,0,0);
    desc=which_desc(carol,alice); assert(!strcmp(desc,"a scarred watch captain")); free(desc);
    assert(GET_SIZE(alice)==SIZE_SMALL);
    do_disguise(alice,"form 999999",0,0); assert(active_sivak_form(alice)->id==npc_id);
    do_disguise(bob,choice,0,0); assert(!IS_SIVAK_FORM(bob));
    /* Removing a form preserves the collection; forms remain reusable. */
    do_disguise(alice,"form remove",0,0); do_disguise(alice,choice,0,0);
    assert(IS_SIVAK_FORM(alice) && active_sivak_form(alice)->id==npc_id);
    *output=0; save_identity(alice); assert(strstr(output,"SvFm:") && strstr(output,"SvAc:"));
    loaded=new_pc("Alice",1); GET_REAL_RACE(loaded)=DL_RACE_SIVAK_DRACONIAN;
    file=tmpfile(); assert(file); fputs(output,file); rewind(file);
    while(fgets(record,sizeof(record),file)) {
      if(!strncmp(record,"SvFm:",5)) load_sivak_form(loaded,record+5,file);
      if(!strncmp(record,"SvAc:",5)) sscanf(record+5,"%llu",&loaded->player_specials->saved.sivak_active_form);
    }
    fclose(file); assert(active_sivak_form(loaded) && active_sivak_form(loaded)->id==npc_id);
    assert(!strcmp(active_sivak_form(loaded)->short_desc,"a scarred watch captain"));
    n=0; for(form=loaded->player_specials->saved.sivak_forms;form;form=form->next) n++; assert(n==2);
    assert(loaded->player_specials->saved.disguise_generation>=npc_id);
    free_pc(loaded);
    /* Maximum-length snapshots cross save chunks without truncation or buffer overread. */
    form=active_sivak_form(alice);
    memset(form->short_desc,'s',sizeof(form->short_desc)-1); form->short_desc[sizeof(form->short_desc)-1]=0;
    memset(form->description,'d',sizeof(form->description)-1); form->description[sizeof(form->description)-1]=0;
    memcpy(form->description+500,"\nSvAc: 999\n",11); /* Text is not parsed as pfile tags. */
    *output=0; save_identity(alice);
    loaded=new_pc("Alice",1); GET_REAL_RACE(loaded)=DL_RACE_SIVAK_DRACONIAN;
    file=tmpfile(); assert(file); fputs(output,file); rewind(file);
    while(fgets(record,sizeof(record),file)) {
      if(!strncmp(record,"SvFm:",5)) load_sivak_form(loaded,record+5,file);
      if(!strncmp(record,"SvAc:",5)) sscanf(record+5,"%llu",&loaded->player_specials->saved.sivak_active_form);
    }
    fclose(file);
    assert(active_sivak_form(loaded) && active_sivak_form(loaded)->id==npc_id);
    assert(!strcmp(active_sivak_form(loaded)->short_desc,form->short_desc));
    assert(!strcmp(active_sivak_form(loaded)->description,form->description));
    free_pc(loaded);
    /* Invalid records consume their strings without corrupting the next tag. */
    loaded=new_pc("Alice",1); GET_REAL_RACE(loaded)=DL_RACE_SIVAK_DRACONIAN;
    file=tmpfile(); assert(file); fputs("a fake~\na fake description~\nNEXT\n",file); rewind(file);
    load_sivak_form(loaded,"999 2 0 999999 5 1",file);
    assert(!loaded->player_specials->saved.sivak_forms);
    assert(fgets(record,sizeof(record),file) && !strcmp(record,"NEXT\n")); fclose(file); free_pc(loaded);
    assert(rolls==old_rolls); /* Innate form learning and viewing never roll Disguise/Perception. */
    do_disguise(alice,"form remove",0,0);
  }
  /* Aurak access is racial and level-gated; repeated UI queries never spend charges. */
  {
    struct char_data *loaded;
    struct affected_type af[MAX_SPELL_AFFECTS]={{0}};
    int old_saves, count, element, save;
    time_t ready, now=time(NULL);
    GET_REAL_RACE(alice)=DL_RACE_AURAK_DRACONIAN;
    HAS_REAL_FEAT(alice,FEAT_AURAK_MAGIC)=1; GET_LEVEL(alice)=1;
    assert(isAurakMagic(alice,SPELL_AURAK_ENERGY_RAY));
    assert(isAurakMagic(alice,SPELL_AURAK_BREATH) && isAurakMagic(alice,SPELL_DISGUISE_SELF));
    assert(!isAurakMagic(alice,SPELL_MIRROR_IMAGE) && !isAurakMagic(alice,SPELL_GREATER_INVIS));
    GET_LEVEL(alice)=3; assert(isAurakMagic(alice,SPELL_MIRROR_IMAGE) && isAurakMagic(alice,SPELL_CHARM));
    assert(!isAurakMagic(alice,SPELL_FIREBALL));
    GET_LEVEL(alice)=5; assert(isAurakMagic(alice,SPELL_FIREBALL) && isAurakMagic(alice,SPELL_DISPEL_MAGIC));
    assert(!isAurakMagic(alice,SPELL_GREATER_INVIS));
    GET_LEVEL(alice)=7; assert(isAurakMagic(alice,SPELL_GREATER_INVIS) && !isAurakMagic(alice,SPELL_DOMINATE_PERSON));
    GET_LEVEL(alice)=9; assert(isAurakMagic(alice,SPELL_DOMINATE_PERSON));
    assert(!isAurakMagic(alice,SPELL_HEAL) && !isAurakMagic(alice,SPELL_AURAK_DEATH_THROES));
    assert(!isAurakMagic(bob,SPELL_AURAK_ENERGY_RAY));
    assert(!prepare_aurak_magic(bob,SPELL_AURAK_ENERGY_RAY,CAST_INNATE));
    HAS_REAL_FEAT(alice,FEAT_AURAK_MAGIC)=0; assert(!isAurakMagic(alice,SPELL_AURAK_ENERGY_RAY));
    HAS_REAL_FEAT(alice,FEAT_AURAK_MAGIC)=1;
    old_saves=saves;
    for(i=0;i<10;i++) assert(isAurakMagic(alice,SPELL_DOMINATE_PERSON));
    assert(saves==old_saves && !alice->player_specials->saved.aurak_dominate_ready);
    assert(prepare_aurak_magic(alice,SPELL_DOMINATE_PERSON,CAST_INNATE));
    ready=alice->player_specials->saved.aurak_dominate_ready;
    assert(ready>=now+86400 && ready<=time(NULL)+86400 && saves==old_saves+1);
    assert(!prepare_aurak_magic(alice,SPELL_DOMINATE_PERSON,CAST_INNATE));
    assert(alice->player_specials->saved.aurak_dominate_ready==ready && saves==old_saves+1);
    /* Prepared class casts remain available after the racial daily use is spent. */
    assert(prepare_aurak_magic(alice,SPELL_DOMINATE_PERSON,CAST_SPELL));
    assert(alice->player_specials->saved.aurak_dominate_ready==ready);
    /* Independent devices don't consume racial daily uses. */
    assert(prepare_aurak_magic(alice,SPELL_DOMINATE_PERSON,CAST_DEVICE));
    assert(alice->player_specials->saved.aurak_dominate_ready==ready);
    assert(prepare_aurak_magic(alice,SPELL_AURAK_BREATH,CAST_INNATE));
    assert(alice->player_specials->saved.aurak_breath_ready>=now+6 && alice->player_specials->saved.aurak_breath_ready<=time(NULL)+24);
    assert(!prepare_aurak_magic(alice,SPELL_AURAK_BREATH,CAST_INNATE));
    assert(prepare_aurak_magic(alice,SPELL_AURAK_ENERGY_RAY,CAST_INNATE));
    *output=0; save_identity(alice); assert(strstr(output,"AuDm:") && strstr(output,"AuBr:"));
    loaded=new_pc("Alice",1); GET_REAL_RACE(loaded)=DL_RACE_AURAK_DRACONIAN;
    HAS_REAL_FEAT(loaded,FEAT_AURAK_MAGIC)=1; GET_LEVEL(loaded)=9;
    { char *tag=strstr(output,"AuDm:"); load_aurak_timers(loaded,"AuDm",tag+5);
      tag=strstr(output,"AuBr:"); load_aurak_timers(loaded,"AuBr",tag+5); }
    assert(loaded->player_specials->saved.aurak_dominate_ready==ready);
    assert(!prepare_aurak_magic(loaded,SPELL_DOMINATE_PERSON,CAST_INNATE));
    assert(!prepare_aurak_magic(loaded,SPELL_AURAK_BREATH,CAST_INNATE));
    loaded->player_specials->saved.aurak_dominate_ready=time(NULL)-1;
    assert(prepare_aurak_magic(loaded,SPELL_DOMINATE_PERSON,CAST_INNATE)); free_pc(loaded);
    test_ray_attacks=test_ray_damage=0;
    mag_loops(9,alice,bob,NULL,SPELL_AURAK_ENERGY_RAY,0,0,CAST_INNATE);
    assert(test_ray_attacks==1 && test_ray_damage==1);
    test_ray_hit=-1; test_ray_attacks=test_ray_damage=0;
    mag_loops(9,alice,bob,NULL,SPELL_AURAK_ENERGY_RAY,0,0,CAST_INNATE);
    assert(test_ray_attacks==1 && !test_ray_damage); test_ray_hit=1;
    test_aurak_damage(SPELL_AURAK_ENERGY_RAY,&count,&element,&save);
    assert(count==4 && element==DAM_FORCE && save==-1);
    test_aurak_damage(SPELL_AURAK_DEATH_THROES,&count,&element,&save);
    assert(count==3 && element==DAM_FORCE && save==SAVING_REFL);
    test_aurak_breath(alice,bob,af);
    assert(af[0].location==APPLY_STR && af[0].modifier==-2 && af[0].duration==4);
    assert(IS_SET_AR(af[1].bitvector,AFF_BLIND) && IS_SET_AR(af[2].bitvector,AFF_BLIND));
    memset(af,0,sizeof(af)); test_save=1; test_aurak_breath(alice,bob,af); assert(!af[0].duration); test_save=0;
    test_poisonable=FALSE; test_aurak_breath(alice,bob,af); assert(!af[0].duration); test_poisonable=TRUE;
    test_blindable=FALSE; test_aurak_breath(alice,bob,af); assert(af[0].duration && !af[1].duration); test_blindable=TRUE;
  }
  free_pc(alice); free_pc(bob); free_pc(carol);
  puts("Introductions/disguises: identity, opposed rolls, Irda/Sivak forms, Aurak racial magic, true sight, persistence, targeting, validation, and full intro lists passed.");
  return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='identity-tests-') as tmp:
    path = Path(tmp) / 'check.c'
    path.write_text(headers + stubs + code + checks)
    binary = Path(tmp) / 'check'
    subprocess.run([os.environ.get('CC', 'cc'), '-std=gnu99', '-Wall', '-Wextra', '-g', '-fsanitize=address,undefined',
                    '-Wno-unused-parameter', '-Wno-sign-compare', '-I' + str(src),
                    str(path), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
