/* Exercise production racial commands with deterministic combat boundaries. */
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "handler.h"
#include "interpreter.h"
#include "comm.h"
#include "race.h"
#include "spells.h"
#include "mud_event.h"
#include "actions.h"
#include "fight.h"
#include "domains_schools.h"
#include <assert.h>

void irda_test_load(struct char_data *ch, const char *tag, const char *line);
void irda_test_write(struct char_data *ch, char *output, size_t size);
void irda_test_backfill(struct char_data *ch);

static struct room_data rooms[2];
struct room_data *world = rooms;
static struct char_data actor, target;
static struct player_special_data specials;
static struct affected_type applied[4];
static int affects, standard, swift, saved, attacks, damage_calls, last_damage, save_dc, dazes;
static int hit_result, save_result, daze_allowed, kill_target, actions_allowed, pvp_allowed;
static time_t now = 1000;

time_t time(time_t *out) { if (out) *out = now; return now; }
void PERF_prof_sect_init(struct PERF_prof_sect **p, const char *id) {}
void PERF_prof_sect_enter(struct PERF_prof_sect *p) {}
void PERF_prof_sect_exit(struct PERF_prof_sect *p) {}
size_t strlcpy(char *dst, const char *src, size_t size)
{
  size_t len = strlen(src);
  if (size) { size_t n = len < size - 1 ? len : size - 1; memcpy(dst, src, n); dst[n] = 0; }
  return len;
}
size_t send_to_char(struct char_data *ch, const char *format, ...) { return 0; }
const char *act(const char *s, int hide, struct char_data *ch, struct obj_data *obj, void *v, int t)
{ return s; }
bool is_action_available(struct char_data *ch, action_type type, bool msg) { return actions_allowed; }
void start_action_cooldown(struct char_data *ch, action_type type, int duration)
{
  assert(duration == 6 * PASSES_PER_SEC);
  if (type == atSTANDARD) standard++; else { assert(type == atSWIFT); swift++; }
}
void save_char(struct char_data *ch, int mode) { saved++; }
void new_affect(struct affected_type *af) { memset(af, 0, sizeof(*af)); }
void affect_to_char(struct char_data *ch, struct affected_type *af)
{
  assert(ch == &actor && affects < 3); applied[affects++] = *af;
  if (af->location == APPLY_SIZE) ch->points.size += af->modifier;
}
void affect_join(struct char_data *ch, struct affected_type *af, bool a, bool b, bool c, bool d)
{
  assert(ch == &target && af->spell == ABILITY_MIRAGE_FLARE && af->duration == 1);
  assert(IS_SET_AR(af->bitvector, AFF_DAZED));
  assert(!a && !b && !c && !d);
  dazes++;
}
bool affected_by_spell(struct char_data *ch, int type)
{
  return type == ABILITY_AUGMENT_PHYSICALITY && affects > 0;
}
struct sivak_form *active_sivak_form(struct char_data *ch) { return NULL; }
int ok_damage_shopkeeper(struct char_data *ch, struct char_data *vict) { return TRUE; }
bool is_mission_mob(struct char_data *ch, struct char_data *vict) { return TRUE; }
int compute_charisma_bonus(struct char_data *ch) { return 3; }
const char *one_argument(const char *arg, char *out, size_t n) { strlcpy(out, arg, n); return arg + strlen(arg); }
struct char_data *get_char_vis(struct char_data *ch, char *name, int *number, int where)
{
  assert(where == FIND_CHAR_ROOM);
  return !strcmp(name, "target") ? &target : !strcmp(name, "self") ? &actor : NULL;
}
bool pvp_ok(struct char_data *ch, struct char_data *vict, bool display) { return pvp_allowed; }
int attack_roll(struct char_data *ch, struct char_data *vict, int type, int touch, int number)
{
  assert(type == ATTACK_TYPE_RANGED && touch && number == 1);
  attacks++;
  return hit_result;
}
int dice(int number, int size) { assert(number == 2 && size == 6); return 7; }
int damage(struct char_data *ch, struct char_data *vict, int amount, int ability, int type, int offhand)
{
  assert(ability == ABILITY_MIRAGE_FLARE && type == DAM_ENERGY && !offhand);
  damage_calls++;
  last_damage = amount;
  return kill_target ? -1 : amount;
}
bool can_daze(struct char_data *ch) { return daze_allowed; }
int savingthrow_flatdc(struct char_data *ch, struct char_data *vict, int type, int mod, int dc,
                       int cast, int school, int spell)
{
  assert(type == SAVING_WILL && mod == 0 && cast == CAST_INNATE);
  assert(school == ILLUSION && spell == ABILITY_MIRAGE_FLARE);
  save_dc = dc;
  return save_result;
}
static void reset(void)
{
  memset(&actor, 0, sizeof(actor)); memset(&target, 0, sizeof(target));
  memset(&specials, 0, sizeof(specials)); memset(rooms, 0, sizeof(rooms));
  actor.player_specials = &specials;
  GET_REAL_RACE(&actor) = DL_RACE_IRDA;
  GET_LEVEL(&actor) = 11;
  actor.points.size = SIZE_MEDIUM;
  IN_ROOM(&actor) = IN_ROOM(&target) = 1;
  HAS_REAL_FEAT(&actor, FEAT_AUGMENT_PHYSICALITY) = 1;
  HAS_REAL_FEAT(&actor, FEAT_MIRAGE_FLARE) = 1;
  FIGHTING(&actor) = &target;
  affects = standard = swift = saved = attacks = damage_calls = last_damage = save_dc = dazes = 0;
  hit_result = 0; save_result = kill_target = 0;
  daze_allowed = actions_allowed = pvp_allowed = 1;
  now = 1000;
}
static void flare(void) { do_mirage_flare(&actor, "target", 0, 0); }
int main(void)
{
  int i;
  char encoded[256];
  struct obj_data item, container;
  memset(&item, 0, sizeof(item));
  memset(&container, 0, sizeof(container));
  GET_OBJ_SIZE(&item) = SIZE_MEDIUM;
  reset();
  do_augment_physicality(&actor, "", 0, 0);
  assert(affects == 3 && standard == 1 && saved == 1);
  assert(applied[0].location == APPLY_SIZE && applied[0].modifier == 1);
  assert(applied[1].location == APPLY_STR && applied[1].modifier == 4);
  assert(applied[2].location == APPLY_CON && applied[2].modifier == 2);
  for (i = 0; i < 3; i++)
    assert(applied[i].duration == 10 && applied[i].bonus_type == BONUS_TYPE_SIZE);
  assert(specials.saved.irda_augment_ready == 1300);
  do_augment_physicality(&actor, "", 0, 0);
  assert(standard == 1);
  assert(GET_SIZE(&actor) == SIZE_LARGE);
  item.worn_by = &actor;
  assert(compute_object_size(&item) == SIZE_LARGE);
  item.worn_by = NULL;
  assert(compute_object_size(&item) == SIZE_MEDIUM);
  container.carried_by = &actor;
  item.in_obj = &container;
  assert(compute_object_size(&item) == SIZE_LARGE);
  assert(GET_OBJ_SIZE(&item) == SIZE_MEDIUM); /* Saved base size stays intact. */
  IRDA_APPEARANCE(&actor).instance = 1;
  IRDA_APPEARANCE(&actor).race = DL_RACE_HUMAN;
  race_list[DL_RACE_HUMAN].size = SIZE_SMALL;
  assert(GET_SIZE(&actor) == SIZE_MEDIUM); /* An assumed form still gains a size. */
  IRDA_APPEARANCE(&actor).instance = 0;
  affects = 0; actor.points.size = SIZE_MEDIUM; now = 1299;
  assert(compute_object_size(&item) == SIZE_MEDIUM);
  do_augment_physicality(&actor, "", 0, 0); assert(standard == 1);
  now = 1300;
  do_augment_physicality(&actor, "", 0, 0); assert(standard == 2);

  reset(); flare();
  assert(last_damage == 12 && save_dc == 18 && dazes == 1 && swift == 1);
  assert(specials.saved.irda_mirage_ready[0] == 1300);
  now = 1010; flare(); now = 1020; flare();
  assert(attacks == 3 && swift == 3);
  now = 1299; flare(); assert(attacks == 3);
  now = 1300; flare(); assert(attacks == 4);
  assert(specials.saved.irda_mirage_ready[0] == 1600);
  assert(specials.saved.irda_mirage_ready[1] == 1310);
  flare(); assert(attacks == 4);
  now = 1310; flare(); assert(attacks == 5);

  reset(); hit_result = -1; flare();
  assert(damage_calls == 1 && last_damage == 0 && !dazes && !save_dc && swift == 1);
  reset(); save_result = 1; flare(); assert(last_damage == 12 && !dazes);
  reset(); daze_allowed = 0; flare(); assert(last_damage == 12 && !dazes && !save_dc);
  reset(); kill_target = 1; flare(); assert(!dazes && !save_dc);
  reset(); actions_allowed = 0; flare(); do_augment_physicality(&actor, "", 0, 0);
  assert(!attacks && !affects && !saved);
  reset(); pvp_allowed = 0; flare(); assert(!saved);
  reset(); SET_BIT_AR(ROOM_FLAGS(1), ROOM_PEACEFUL); flare(); assert(!saved);
  reset(); SET_BIT_AR(ROOM_FLAGS(1), ROOM_NOMAGIC); flare();
  do_augment_physicality(&actor, "", 0, 0); assert(!saved && !affects);
  reset(); SET_BIT(ROOM_AFFECTIONS(1), RAFF_ANTI_MAGIC); flare(); assert(!saved);
  reset(); do_mirage_flare(&actor, "self", 0, 0); assert(!saved);
  reset(); do_mirage_flare(&actor, "missing", 0, 0); assert(!saved);
  reset(); SET_BIT_AR(MOB_FLAGS(&target), MOB_ISNPC); SET_BIT_AR(MOB_FLAGS(&target), MOB_NOKILL);
  flare(); assert(!saved);
  reset(); GET_REAL_RACE(&actor) = DL_RACE_HUMAN; flare();
  do_augment_physicality(&actor, "", 0, 0); assert(!saved && !affects);
  reset(); HAS_REAL_FEAT(&actor, FEAT_MIRAGE_FLARE) = 0; flare(); assert(!saved);
  reset(); do_mirage_flare(&actor, "", 0, 0); assert(attacks == 1);
  reset();
  specials.saved.irda_augment_ready = 1300;
  specials.saved.irda_mirage_ready[0] = 1300;
  specials.saved.irda_mirage_ready[1] = 1310;
  specials.saved.irda_mirage_ready[2] = 1320;
  irda_test_write(&actor, encoded, sizeof(encoded));
  assert(!strcmp(encoded, "AiAu: 1300\nAiMf: 1300 1310 1320\n"));
  reset();
  irda_test_load(&actor, "AiAu", "1300");
  irda_test_load(&actor, "AiMf", "1300 1310 1320");
  flare(); do_augment_physicality(&actor, "", 0, 0);
  assert(!attacks && !affects && !saved);
  irda_test_write(&actor, encoded, sizeof(encoded));
  assert(!strcmp(encoded, "AiAu: 1300\nAiMf: 1300 1310 1320\n"));
  irda_test_load(&actor, "AiMf", "bad data");
  irda_test_load(&actor, "AiAu", "-1");
  assert(specials.saved.irda_augment_ready == 1300 && specials.saved.irda_mirage_ready[1] == 1310);
  now = 1300; flare(); assert(attacks == 1);
  HAS_REAL_FEAT(&actor, FEAT_AUGMENT_PHYSICALITY) = 0;
  HAS_REAL_FEAT(&actor, FEAT_MIRAGE_FLARE) = 0;
  irda_test_backfill(&actor);
  assert(HAS_REAL_FEAT(&actor, FEAT_IRDA_RACIAL_ADJUSTMENT) == 1);
  assert(HAS_REAL_FEAT(&actor, FEAT_IRDA_CHANGE_SHAPE) == 1);
  assert(HAS_REAL_FEAT(&actor, FEAT_AUGMENT_PHYSICALITY) == 1);
  assert(HAS_REAL_FEAT(&actor, FEAT_MIRAGE_FLARE) == 1);
  puts("Irda racial commands: effects, actions, cooldowns, hit/save/daze, target guards and persistence passed.");
  return 0;
}
