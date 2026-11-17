#!/usr/bin/env python3
"""Exercise production bloodline progression, spell grants, previews and active powers."""
from pathlib import Path
import os
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
sources = {name: (root / 'src' / name).read_text() for name in
           ['class.c', 'feats.c', 'study.c', 'spell_prep.c', 'utils.c', 'act.offensive.c', 'players.c', 'fight.c', 'movement_cost.c', 'spec_procs.c']}

def function(filename, signature):
    text = sources[filename]
    start = text.index(signature + '\n{')
    return text[start:text.index('\n}', start) + 2] + '\n'

headers = '\n'.join('#include "' + h + '"' for h in
                   ['conf.h', 'sysdep.h', 'structs.h', 'utils.h', 'comm.h', 'db.h',
                    'spells.h', 'class.h', 'bloodlines.h', 'feats.h', 'spell_prep.h',
                    'handler.h', 'interpreter.h', 'act.h', 'fight.h', 'mud_event.h', 'actions.h',
                    'constants.h', 'missions.h', 'domains_schools.h', 'oasis.h', 'perks.h', 'alchemy.h', 'assign_wpn_armor.h'])
code = r'''
#include <assert.h>
#undef OLC_MODE
#define OLC_MODE(d) mode
static int mode;
#undef ACMD
#define ACMD(name) void name(struct char_data *ch, const char *argument, int cmd, int subcmd)
#undef IS_UNDEAD
#define IS_UNDEAD(ch) (GET_RACE(ch) == RACE_TYPE_UNDEAD)
#undef IS_CONSTRUCT
#define IS_CONSTRUCT(ch) (GET_RACE(ch) == RACE_TYPE_CONSTRUCT)
struct player_special_data dummy_mob;
struct feat_info feat_list[NUM_FEATS];
struct spell_info_type spell_info[TOP_SPELL_DEFINE + 1];
struct room_data rooms[3], *world = rooms;
room_rnum top_of_world = 2;
const char *dirs[] = {"north", "east", "south", "west", "up", "down", "\n"};
const char *ability_names[NUM_ABILITIES + 1];
const char *draconic_heritage_names[NUM_DRACONIC_HERITAGE_TYPES + 1];
const char *spell_schools[NUM_SCHOOLS];
static char output[50000];
static int menus;
static void set_sorcerer_bloodline(struct descriptor_data *d) { menus++; }
static void display_main_menu(struct descriptor_data *d) { menus++; }
static int add_levelup_feat(struct descriptor_data *d, int feat) { LEVELUP(d->character)->feats[feat] = 1; return 1; }
static int used[NUM_FEATS], action_ok = 1, pvp_allowed = 1, mission_ok = 1;
static int standard_actions, swift_actions, move_actions, damage_calls, save_pass, dark = 1;
static int move_ok = 1, magic_ok = 1, attack_result = 1, rolled = 8, grouped = 1;
static int damage_types[4], damage_amounts[4];
static struct char_data *room_target;
int MAX(int a, int b) { return a > b ? a : b; }
int MIN(int a, int b) { return a < b ? a : b; }
#undef HAS_COMBAT_FEAT
#define HAS_COMBAT_FEAT(ch, feat, weapon) 0
#undef KNOWS_DISCOVERY
#define KNOWS_DISCOVERY(ch, discovery) 0
bool has_perk(struct char_data *ch, int perk) { return false; }
#undef CAN_SEE
#define CAN_SEE(ch, vict) 1
#undef HAS_EVOLUTION
#define HAS_EVOLUTION(ch, evo) 0
static bool blindsight;
bool has_blindsight(struct char_data *ch) { return blindsight; }
bool can_see_invisible_creatures(struct char_data *ch) { return false; }
bool can_strike_ethereal_creatures(struct char_data *ch) { return false; }
bool room_is_daylit(room_rnum room) { return false; }
bool is_outdoors(struct char_data *ch) { return true; }
int get_perk_critical_confirmation_bonus(struct char_data *ch) { return 0; }
int get_bard_supreme_style_crit_confirm_bonus(struct char_data *ch) { return 0; }
bool is_judgement_possible(struct char_data *ch, struct char_data *vict, int type) { return false; }
int get_judgement_bonus(struct char_data *ch, int type) { return 0; }
int determine_threat_range(struct char_data *ch, struct obj_data *weapon, struct char_data *vict) { return 20; }
static int proc_chance = 100, last_die_sides;
int rand_number(int from, int to) { return proc_chance; }
int get_feat_value(struct char_data *ch, int feat) { return HAS_REAL_FEAT(ch, feat); }
struct sivak_form *active_sivak_form(struct char_data *ch) { return NULL; }
int compute_current_size(struct char_data *ch) { return SIZE_MEDIUM; }
int compute_strength_bonus(struct char_data *ch) { return GET_STR(ch) / 2 - 5; }
int apply_damage_reduction(struct char_data *ch, struct char_data *vict, struct obj_data *weapon,
                           int dam, bool display, int type) { return dam; }
bool process_healing(struct char_data *ch, struct char_data *vict, int spell, int healing, int move, int psp) {
  int old = GET_HIT(vict); GET_HIT(vict) = MIN(GET_MAX_HIT(vict), old + healing); return old != GET_HIT(vict);
}
int compute_charisma_bonus(struct char_data *ch) { return GET_CHA(ch); }
int movement_loss[NUM_ROOM_SECTORS];
int skill_roll(struct char_data *ch, int skill) { return 0; }
int get_speed(struct char_data *ch, sbyte display) { return 30; }
bool is_flying(struct char_data *ch) { return AFF_FLAGGED(ch, AFF_FLYING); }
bool has_inquisitor_wilderness_stride(struct char_data *ch) { return false; }
int get_inquisitor_wilderness_stride_rank(struct char_data *ch) { return 0; }
int legendary_foundation_walk(struct char_data *ch, int cost, bool mounted) { return cost; }
int d20(struct char_data *ch) { return rolled; }
int dice(int count, int sides) { last_die_sides = sides; return count; }
void basic_mud_log(const char *fmt, ...) { assert(0); }
static size_t append(const char *fmt, va_list args) {
  size_t n = strlen(output);
  return vsnprintf(output + n, sizeof(output) - n, fmt, args);
}
size_t send_to_char(struct char_data *ch, const char *fmt, ...) {
  va_list args; va_start(args, fmt); size_t n = append(fmt, args); va_end(args); return n;
}
size_t write_to_output(struct descriptor_data *d, const char *fmt, ...) {
  va_list args; va_start(args, fmt); size_t n = append(fmt, args); va_end(args); return n;
}
const char *act(const char *str, int hide, struct char_data *ch, struct obj_data *obj,
                void *vict, int type) { return str; }
void new_affect(struct affected_type *af) { memset(af, 0, sizeof(*af)); }
bool affected_by_spell(struct char_data *ch, int spell) {
  for (struct affected_type *af = ch->affected; af; af = af->next)
    if (af->spell == spell) return true;
  return false;
}
void affect_from_char(struct char_data *ch, int spell) {
  struct affected_type **p = &ch->affected;
  while (*p) {
    if ((*p)->spell == spell) { struct affected_type *old = *p; *p = old->next; free(old); }
    else p = &(*p)->next;
  }
  bloodline_restore_summon_ward(ch);
}
void affect_to_char(struct char_data *ch, struct affected_type *af) {
  struct affected_type *copy = malloc(sizeof(*copy)); assert(copy); *copy = *af;
  copy->next = ch->affected; ch->affected = copy;
  bloodline_restore_summon_ward(ch);
}
static void clear_affects(struct char_data *ch) {
  while (ch->affected) affect_from_char(ch, ch->affected->spell);
}
int get_daily_uses(struct char_data *ch, int featnum);
int daily_uses_remaining(struct char_data *ch, int feat) { return get_daily_uses(ch, feat) - used[feat]; }
int start_daily_use_cooldown(struct char_data *ch, int feat) { return ++used[feat]; }
int savingthrow_flatdc(struct char_data *ch, struct char_data *vict, int type,
                        int mod, int dc, int casttype, int school, int spell) { return save_pass; }
int compute_spells_circle(struct char_data *ch, int class, int spell, int meta, int domain) {
  int circle = extended_bloodline_spell_circle(ch, spell); return circle ? circle : 4;
}
const char *two_arguments(const char *argument, char *first, size_t first_size, char *second, size_t second_size) {
  char a[200] = "", b[200] = ""; sscanf(argument, "%199s %199s", a, b);
  snprintf(first, first_size, "%s", a); snprintf(second, second_size, "%s", b); return argument;
}
struct char_data *get_char_room_vis(struct char_data *ch, char *name, int *number) { return room_target; }
int is_player_grouped(struct char_data *ch, struct char_data *vict) { return grouped && vict == room_target; }
bool pvp_ok(struct char_data *ch, struct char_data *vict, bool display) { return pvp_allowed; }
bool is_mission_mob(struct char_data *ch, struct char_data *vict) { return mission_ok; }
bool is_action_available(struct char_data *ch, action_type action, bool msg) { return action_ok; }
void start_action_cooldown(struct char_data *ch, action_type action, int duration) {
  if (action == atSTANDARD) standard_actions++; else if (action == atSWIFT) swift_actions++; else move_actions++;
}
bool room_is_dark(room_rnum room) { return dark; }
bool char_has_ultra(struct char_data *ch) { return HAS_REAL_FEAT(ch, FEAT_NIGHTEYE); }
bool char_has_infra(struct char_data *ch) { return false; }
int attack_roll(struct char_data *ch, struct char_data *vict, int type, int touch, int count) { return attack_result; }
bool set_fighting(struct char_data *ch, struct char_data *vict) { FIGHTING(ch) = vict; return true; }
void stop_fighting(struct char_data *ch) { FIGHTING(ch) = NULL; }
int search_block(char *arg, const char *const *list, bool exact) { return !strcmp(arg, "north") ? 0 : -1; }
int perform_move_full(struct char_data *ch, int dir, int specials, bool recursive) {
  if (move_ok) IN_ROOM(ch) = 1;
  return move_ok;
}
int call_magic(struct char_data *ch, struct char_data *vict, struct obj_data *obj,
               int spell, int meta, int level, int type) { return magic_ok; }
int damage(struct char_data *ch, struct char_data *vict, int amount, int spell, int type, int offhand) {
  assert(damage_calls < 4); damage_types[damage_calls] = type; damage_amounts[damage_calls++] = amount;
  GET_HIT(vict) -= bloodline_modify_damage(vict, amount, spell, type); return GET_HIT(vict) <= 0 ? -1 : amount;
}
int aoe_effect(struct char_data *ch, int spell, int (*callback)(struct char_data *, struct char_data *, void *), void *data) {
  return room_target ? callback(ch, room_target, data) : 0;
}
'''
code += r'''
static struct mud_event_data mock_event;
static char event_variables[30];
static struct char_data *healed_recipient;
struct mud_event_data *char_has_mud_event(struct char_data *ch, event_id id) {
  if (id == eBLOODLINE_HEAVENLY_HEAL_RECEIVED && ch == healed_recipient) {
    mock_event.iId = id; mock_event.sVariables = event_variables;
    snprintf(event_variables, sizeof(event_variables), "uses:1"); return &mock_event;
  }
  for (int feat = 1; feat < NUM_FEATS; feat++)
    if (feat_list[feat].event == (int)id && used[feat]) {
      mock_event.iId = id; mock_event.sVariables = event_variables;
      snprintf(event_variables, sizeof(event_variables), "uses:%d", used[feat]);
      return &mock_event;
    }
  return NULL;
}
struct mud_event_data *new_mud_event(event_id id, void *ch, const char *vars) {
  mock_event.iId = id; mock_event.pStruct = ch; mock_event.sVariables = event_variables;
  snprintf(event_variables, sizeof(event_variables), "%s", vars ? vars : "");
  return &mock_event;
}
void attach_mud_event(struct mud_event_data *event, long time) {
  if (event->iId == eBLOODLINE_HEAVENLY_HEAL_RECEIVED) {
    assert(time == 42 || time == BLOODLINE_WINDOW_SECONDS RL_SEC); healed_recipient = event->pStruct; return;
  }
  assert(time == 42);
  for (int feat = 1; feat < NUM_FEATS; feat++)
    if (feat_list[feat].event == (int)event->iId)
      assert(sscanf(event->sVariables, "uses:%d", &used[feat]) == 1);
}
long event_time(struct event *event) { return 42; }
int get_line(FILE *fl, char *buf) { return fgets(buf, MAX_INPUT_LENGTH, fl) != NULL; }
#define BUFFER_WRITE(...) send_to_char(ch, __VA_ARGS__)
'''
a = sources['players.c'].index('    /* The new bloodline cooldowns use the same persisted daily-use events. */')
b = sources['players.c'].index('    BUFFER_WRITE("-1 -1\\n");', a)
code += 'static void save_test_events(struct char_data *ch) { int i; struct mud_event_data *pMudEvent;\n' + sources['players.c'][a:b] + 'BUFFER_WRITE("-1 -1\\n"); }\n'
code += function('players.c', 'static void load_events(FILE *fl, struct char_data *ch)')
# Use the real catalogue and helpers without pulling in unrelated class code.
text = sources['class.c']
a = text.index('const struct sorcerer_bloodline extended_bloodlines[')
code += text[a:text.index('};', a) + 2] + '\n'
for sig in [
    'const struct sorcerer_bloodline *extended_bloodline_by_feat(int feat)',
    'const struct sorcerer_bloodline *extended_bloodline_by_type(int type)',
    'void grant_extended_bloodline_powers(struct char_data *ch)',
    'int bloodline_affect_bonus(struct char_data *ch, int spell)',
    'int bloodline_fated_bonus(struct char_data *ch)',
    'int bloodline_reroll(struct char_data *ch, int roll)',
    'int bloodline_retry_spell_resistance(struct char_data *ch, int challenge, int resistance, int first_die)',
    'int bloodline_shadow_spell(int spellnum)',
    'int bloodline_spell_dc_bonus(struct char_data *ch, int spellnum)',
    'void bloodline_cast_arcana(struct char_data *ch, int spellnum, struct char_data *target)',
    'int bloodline_modify_damage(struct char_data *victim, int dam, int w_type, int dam_type)',
    'int extended_bloodline_spell_circle(struct char_data *ch, int spellnum)',
    'bool bloodline_sees_weather(struct char_data *ch)',
    'bool bloodline_sees_magical_dark(struct char_data *ch)',
    'int bloodline_element(struct char_data *ch)',
    'const char *bloodline_element_name(int element)',
    'int bloodline_element_damage(struct char_data *ch)',
    'bool bloodline_elemental_movement(struct char_data *ch, int element)',
    'int bloodline_convert_energy(struct char_data *ch, int spellnum, int element)',
    'int bloodline_energy_resistance(struct char_data *ch, int element)',
    'bool bloodline_energy_immunity(struct char_data *ch, int element)',
    'int bloodline_poison_save_bonus(struct char_data *ch)',
    'int bloodline_strength_bonus(struct char_data *ch)',
    'int bloodline_conviction_roll(struct char_data *ch, int roll, int category)',
    'int bloodline_cooldown_seconds(int feat)',
    'int bloodline_knockdown_bonus(struct char_data *ch)',
    'int bloodline_reduce_concealment(struct char_data *attacker, int concealment)',
    'bool bloodline_summon_creature_spell(int spellnum)',
    'bool bloodline_summoning_spell(int spellnum)',
    'void bloodline_restore_summon_ward(struct char_data *mob)',
    'void bloodline_summon_benefits(struct char_data *ch, struct char_data *mob, int spellnum)',

]:
    code += function('class.c', sig)
for sig in ['int get_sorcerer_bloodline_type(struct char_data *ch)',
            'int get_levelup_sorcerer_bloodline_type(struct char_data *ch)',
            'bool isSorcBloodlineFeat(int featnum)']:
    code += function('feats.c', sig)
for sig in ['int get_sorc_bloodline(struct char_data *ch)',
            'bool is_sorc_bloodline_spell(int bloodline, int spellnum)',
            'bool has_sorc_bloodline_spell(struct char_data *ch, int spellnum)']:
    code += function('spell_prep.c', sig)
a = sources['utils.c'].index('  case FEAT_ABYSSAL_CLAWS:')
b = sources['utils.c'].index('  case FEAT_QUICK_CHANT:', a)
code += 'int get_daily_uses(struct char_data *ch, int featnum) { int daily_uses = 0; switch (featnum) {\n'
code += sources['utils.c'][a:b] + 'default: break; } return daily_uses; }\n'
code += function('fight.c', 'int compute_concealment(struct char_data *ch, struct char_data *attacker)')
code += function('fight.c', 'int is_critical_hit(struct char_data *ch, struct obj_data *wielded, int diceroll, int calc_bab,\n                    int victim_ac, struct char_data *victim, int spell_threat)')
code += function('study.c', 'static void display_bloodline_benefits(struct descriptor_data *d)')
study = sources['study.c']
a = study.index('      case 4:\n      case 5:\n      case 6:\n')
b = study.index('      case 2:', a)
code += 'static void choose(struct descriptor_data *d, int number) { struct char_data *ch = d->character; switch(number) {\n' + study[a:b] + 'default: return; }}\n'
a = study.index('  case STUDY_SET_ELEMENTAL_BLOODLINE:\n')
b = study.index('  case STUDY_SELECT_RACIAL_ABILITY:', a)
code += 'static void confirm(struct descriptor_data *d, const char *arg) { struct char_data *ch = d->character; switch(mode) {\n' + study[a:b] + 'default: return; }}\n'
a = sources['act.offensive.c'].index('/* Shared player command for the additional sorcerer bloodlines. */')
code += sources['act.offensive.c'][a:]
code += function('movement_cost.c', 'static bool bloodline_burrowing_sector(int sector)')
code += function('movement_cost.c', 'int calculate_movement_cost(struct char_data *ch, room_rnum from_room, room_rnum to_room,\n                            int riding)')
skills = sources['spec_procs.c']
a = skills.index('  struct affected_type *af = NULL;\n  int competence =')
b = skills.index('  /* Check for crafting tool bonuses */', a)
code += 'static int skill_affect_bonus(struct char_data *ch, int abilityNum) { int value = 0;\n' + skills[a:b] + 'return value; }\n'
# Use real descriptions to verify useful preview text rather than placeholder data.
code += function('feats.c', 'static void feato(int featnum, const char *name, int in_game, int can_learn, int can_stack,\n                  int feat_type, const char *short_description, const char *description)')
a = sources['feats.c'].index('  /* Additional PF1 sorcerer bloodlines')
b = sources['feats.c'].index('  // Sorcerer Fey Bloodline', a)
registrations = sources['feats.c']
r = registrations.index('  dailyfeat(FEAT_SHADOWSTRIKE,')
rend = registrations.index('  dailyfeat(FEAT_DESTINED_BLOODLINE_ARCANA,', r)
rend = registrations.index(';', rend) + 1
code += function('feats.c', 'void dailyfeat(int featnum, event_id event)')
code += 'static void init_test_feats(void) {\n' + sources['feats.c'][a:b] + registrations[r:rend] + '\n}\n'
code += r'''
static void reset(struct char_data *ch, struct player_special_data *ps, int lineage, int level) {
  clear_affects(ch); memset(ch, 0, sizeof(*ch)); memset(ps, 0, sizeof(*ps)); ch->player_specials = ps;
  GET_POS(ch) = POS_STANDING; GET_LEVEL(ch) = 20; GET_CHA(ch) = 3;
  CLASS_LEVEL(ch, CLASS_SORCERER) = level; IN_ROOM(ch) = 0; GET_HIT(ch) = GET_MAX_HIT(ch) = 100;
  if (lineage) SET_FEAT(ch, lineage, 1);
  memset(used, 0, sizeof(used)); output[0] = 0; damage_calls = 0;
  grant_extended_bloodline_powers(ch);
}
int main(void) {
  struct char_data ch = {0}, ally = {0};
  struct player_special_data ps = {0}, aps = {0};
  struct level_data pending = {0};
  struct descriptor_data d = {0};
  struct obj_data weapon = {0};
  struct room_direction_data exit = {0};
  struct char_data enemy = {0};
  struct player_special_data eps = {0};
  static const int thresholds[] = {1, 1, 3, 9, 15, 20};
  init_test_feats();
  assert(bloodline_burrowing_sector(SECT_FOREST) && bloodline_burrowing_sector(SECT_CAVE));
  assert(!bloodline_burrowing_sector(SECT_UD_WATER) && !bloodline_burrowing_sector(SECT_OCEAN));
  assert(!bloodline_burrowing_sector(SECT_FLYING) && !bloodline_burrowing_sector(SECT_LAVA));
  for (int i = 0; i < NUM_EXTENDED_BLOODLINES; i++) {
    const struct sorcerer_bloodline *bl = &extended_bloodlines[i];
    assert(extended_bloodline_by_feat(bl->feat) == bl);
    assert(extended_bloodline_by_type(bl->type) == bl);
    reset(&ch, &ps, bl->feat, 1);
    for (int level = 1; level <= 20; level++) {
      CLASS_LEVEL((&ch), CLASS_SORCERER) = level; output[0] = 0;
      grant_extended_bloodline_powers(&ch); grant_extended_bloodline_powers(&ch);
      for (int j = 0; j < 6; j++) assert(HAS_REAL_FEAT(&ch, bl->powers[j]) == (level >= thresholds[j]));
    }
    assert(isSorcBloodlineFeat(bl->feat));
    assert(get_sorcerer_bloodline_type(&ch) == bl->feat);
    assert(get_sorc_bloodline(&ch) == bl->type);
    SET_FEAT(&ch, bl->feat, 0); ps.levelup = &pending; pending.feats[bl->feat] = 1;
    assert(get_levelup_sorcerer_bloodline_type(&ch) == bl->feat);
    assert(get_sorc_bloodline(&ch) == bl->type);
    for (int j = 0; j < 9; j++) {
      assert(is_sorc_bloodline_spell(bl->type, bl->spells[j]));
      assert(extended_bloodline_spell_circle(&ch, bl->spells[j]) == j + 1);
      CLASS_LEVEL((&ch), CLASS_SORCERER) = 2*j+2;
      assert(!has_sorc_bloodline_spell(&ch, bl->spells[j]));
      CLASS_LEVEL((&ch), CLASS_SORCERER)++;
      assert(has_sorc_bloodline_spell(&ch, bl->spells[j]));
      spell_info[bl->spells[j]].name = "bonus spell";
    }
    /* Previews use only the temporary choice, never granting a feat. */
    pending.feats[bl->feat] = 0; pending.tempFeat = bl->feat; d.character = &ch;
    for (int j = 0; j < 8; j++) feat_list[bl->bonus_feats[j]].name = "bonus feat";
    ability_names[bl->skill] = "class skill";
    output[0] = 0; display_bloodline_benefits(&d);
    assert(strstr(output, "Level 1 - ") && strstr(output, "Level 20 - "));
    assert(strstr(output, "Free bloodline spells") && strstr(output, "Bonus feat choices"));
    assert(strstr(output, feat_list[bl->powers[1]].description));
    assert(!HAS_REAL_FEAT(&ch, bl->feat) && !pending.feats[bl->feat]);
  }
  for (int i = 0; i < NUM_EXTENDED_BLOODLINES; i++) {
    reset(&ch, &ps, 0, 1); memset(&pending, 0, sizeof(pending)); ps.levelup = &pending;
    d.character = &ch; output[0] = 0;
    choose(&d, i + 4);
    if (i == 4) confirm(&d, "1");
    assert(mode == STUDY_CONFIRM_BLOODLINE);
    assert(strstr(output, "Bloodline benefits") < strstr(output, "Choose the "));
    assert(strstr(output, "Level 20 - ") < strstr(output, "Choose the "));
    assert(!HAS_REAL_FEAT(&ch, extended_bloodlines[i].feat));
    assert(!pending.feats[extended_bloodlines[i].feat]);
    confirm(&d, "N"); assert(!pending.feats[extended_bloodlines[i].feat]);
    choose(&d, i + 4); if (i == 4) confirm(&d, "1"); confirm(&d, "Y");
    assert(pending.feats[extended_bloodlines[i].feat]);
    assert(!HAS_REAL_FEAT(&ch, extended_bloodlines[i].feat));
  }
  /* Invalid elemental input cannot advance to confirmation or set a subtype. */
  reset(&ch, &ps, 0, 1); memset(&pending, 0, sizeof(pending)); ps.levelup = &pending;
  d.character = &ch; choose(&d, 8);
  confirm(&d, "0"); confirm(&d, "5"); confirm(&d, "1junk");
  assert(mode == STUDY_SET_ELEMENTAL_BLOODLINE && !pending.sorcerer_bloodline_subtype);
  confirm(&d, "4"); assert(mode == STUDY_CONFIRM_BLOODLINE);
  assert(pending.sorcerer_bloodline_subtype == BLOODLINE_ELEMENT_WATER);
  assert(strstr(output, "Water")); confirm(&d, "N");
  assert(!pending.feats[FEAT_SORCERER_BLOODLINE_ELEMENTAL]);
  /* Dragon Disciples retain their required draconic heritage. */
  reset(&ch, &ps, 0, 1); memset(&pending, 0, sizeof(pending)); ps.levelup = &pending;
  CLASS_LEVEL((&ch), CLASS_DRAGON_DISCIPLE) = 1; d.character = &ch;
  choose(&d, 4); confirm(&d, "Y"); assert(!pending.feats[FEAT_SORCERER_BLOODLINE_SHADOW]);
  assert(eBLOODLINE_SHADOWSTRIKE == eEPIC_SUMMON_ACTION + 1);
  assert(feat_list[FEAT_SHADOWSTRIKE].event == eBLOODLINE_SHADOWSTRIKE);
  assert(!feat_list[FEAT_DESTINY_REALIZED].event);
  assert(!extended_bloodline_by_feat(FEAT_SHADOW_MASTER)); /* Shadowdancer feat is preserved. */
  assert(get_sorc_bloodline(NULL) == SORC_BLOODLINE_NONE);
  reset(&ch, &ps, FEAT_SORCERER_BLOODLINE_DESTINED, 20);
  assert(bloodline_fated_bonus(&ch) == 5);
  CLASS_LEVEL((&ch), CLASS_SORCERER) = 3; assert(bloodline_fated_bonus(&ch) == 1);
  CLASS_LEVEL((&ch), CLASS_SORCERER) = 7; assert(bloodline_fated_bonus(&ch) == 2);
  assert(get_daily_uses(&ch, FEAT_TOUCH_OF_DESTINY) == 6);
  CLASS_LEVEL((&ch), CLASS_SORCERER) = 9; assert(get_daily_uses(&ch, FEAT_IT_WAS_MEANT_TO_BE) == 1);
  CLASS_LEVEL((&ch), CLASS_SORCERER) = 10; assert(get_daily_uses(&ch, FEAT_IT_WAS_MEANT_TO_BE) == 2);
  CLASS_LEVEL((&ch), CLASS_SORCERER) = 20; assert(get_daily_uses(&ch, FEAT_IT_WAS_MEANT_TO_BE) == 3);
  CLASS_LEVEL((&ch), CLASS_SORCERER) = 17; assert(get_daily_uses(&ch, FEAT_IT_WAS_MEANT_TO_BE) == 2);
  for (int level = 9; level <= 20; level++) {
    CLASS_LEVEL((&ch), CLASS_SORCERER) = level;
    assert(get_daily_uses(&ch, FEAT_THUNDERBOLT) == (level >= 20 ? 3 : level >= 17 ? 2 : 1));
  }
  /* Fatal damage succeeds or fails once per day; ordinary damage spends no use. */
  save_pass = 1; assert(bloodline_modify_damage(&ch, 10, SPELL_MAGIC_MISSILE, DAM_FORCE) == 10);
  assert(!used[FEAT_WITHIN_REACH]);
  assert(bloodline_modify_damage(&ch, 150, SPELL_MAGIC_MISSILE, DAM_FORCE) == 99);
  assert(used[FEAT_WITHIN_REACH] == 1);
  assert(bloodline_modify_damage(&ch, 150, SPELL_MAGIC_MISSILE, DAM_FORCE) == 150);
  used[FEAT_WITHIN_REACH] = 0; save_pass = 0;
  assert(bloodline_modify_damage(&ch, 150, SPELL_MAGIC_MISSILE, DAM_FORCE) == 150);
  assert(used[FEAT_WITHIN_REACH] == 1);
  used[FEAT_WITHIN_REACH] = 0;
  assert(bloodline_modify_damage(&ch, 150, AFFECT_SHADOWSTRIKE, DAM_ILLUSION) == 99);
  assert(!used[FEAT_WITHIN_REACH]);
  reset(&ch, &ps, FEAT_SORCERER_BLOODLINE_STORMBORN, 20); GET_HIT(&ch) = 50;
  assert(bloodline_modify_damage(&ch, 30, SPELL_LIGHTNING_BOLT, DAM_ELECTRIC) == 0);
  assert(GET_HIT(&ch) == 60 && used[FEAT_STORM_LORD] == 1);
  assert(bloodline_modify_damage(&ch, 30, SPELL_THUNDERCLAP, DAM_SOUND) == 30);
  assert(bloodline_sees_weather(&ch));
  assert(bloodline_spell_dc_bonus(&ch, SPELL_LIGHTNING_BOLT) == 1);
  assert(bloodline_spell_dc_bonus(&ch, SPELL_THUNDERCLAP) == 1);
  assert(!bloodline_spell_dc_bonus(&ch, SPELL_FIREBALL));
  /* Actual command: ally buff validation, durations, actions and exhausted uses. */
  reset(&ch, &ps, FEAT_SORCERER_BLOODLINE_DESTINED, 9);
  reset(&ally, &aps, 0, 1); room_target = &ally;
  grouped = 0; do_bloodline(&ch, "touch-of-destiny ally", 0, 0);
  assert(!ally.affected && !used[FEAT_TOUCH_OF_DESTINY]);
  grouped = 1; action_ok = 0; do_bloodline(&ch, "touch-of-destiny ally", 0, 0);
  assert(!ally.affected); action_ok = 1;
  do_bloodline(&ch, "touch-of-destiny ally", 0, 0);
  assert(bloodline_affect_bonus(&ally, AFFECT_TOUCH_OF_DESTINY) == 4);
  assert(ally.affected->duration == 3 && used[FEAT_TOUCH_OF_DESTINY] == 1);
  do_bloodline(&ch, "touch-of-destiny ally", 0, 0);
  assert(ally.affected && !ally.affected->next); /* no stacking */
  used[FEAT_TOUCH_OF_DESTINY] = get_daily_uses(&ch, FEAT_TOUCH_OF_DESTINY);
  do_bloodline(&ch, "touch-of-destiny ally", 0, 0);
  assert(used[FEAT_TOUCH_OF_DESTINY] == 6);
  do_bloodline(&ch, "reroll", 0, 0); assert(!used[FEAT_IT_WAS_MEANT_TO_BE]);
  proc_chance = 2; assert(bloodline_reroll(&ch, 19) == 19);
  proc_chance = 1; assert(bloodline_reroll(&ch, 19) == 1);
  assert(used[FEAT_IT_WAS_MEANT_TO_BE] == 1 && !affected_by_spell(&ch, AFFECT_DESTINED_REROLL));
  assert(bloodline_reroll(&ch, 19) == 19); /* level 9 has one proc per window */
  proc_chance = 100;
  reset(&ch, &ps, FEAT_SORCERER_BLOODLINE_STORMBORN, 9); clear_affects(&ally);
  GET_OBJ_TYPE(&weapon) = ITEM_FIREWEAPON; GET_EQ(&ally, WEAR_WIELD_1) = &weapon;
  do_bloodline(&ch, "thunderstaff ally", 0, 0); assert(!ally.affected);
  GET_OBJ_TYPE(&weapon) = ITEM_WEAPON;
  do_bloodline(&ch, "thunderstaff ally", 0, 0);
  assert(ally.affected->duration == 4 && ally.affected->modifier == 9);
  used[FEAT_THUNDERBOLT] = 0; damage_calls = 0; grouped = 0; save_pass = 0; GET_HIT(&ally) = 100;
  do_bloodline(&ch, "thunderbolt", 0, 0);
  assert(damage_calls == 2 && damage_types[0] == DAM_ELECTRIC && damage_types[1] == DAM_SOUND);
  assert(damage_amounts[0] + damage_amounts[1] == 9);
  assert(affected_by_spell(&ally, AFFECT_THUNDERBOLT));
  used[FEAT_THUNDERBOLT] = 0; damage_calls = 0; pvp_allowed = 0;
  do_bloodline(&ch, "thunderbolt", 0, 0); assert(!damage_calls); pvp_allowed = 1;
  used[FEAT_THUNDERBOLT] = 0; damage_calls = 0; save_pass = 1; clear_affects(&ally);
  do_bloodline(&ch, "thunderbolt", 0, 0);
  assert(damage_amounts[0] + damage_amounts[1] == 4 && !ally.affected);
  /* Darkness rejects friendly targets and movement rejects blocked/peaceful exits. */
  reset(&ch, &ps, FEAT_SORCERER_BLOODLINE_SHADOW, 15); grouped = 1;
  do_bloodline(&ch, "enveloping-darkness", 0, 0); assert(!ally.affected);
  assert(bloodline_sees_magical_dark(&ch));
  used[FEAT_ENVELOPING_DARKNESS] = 0; grouped = 0;
  do_bloodline(&ch, "enveloping-darkness", 0, 0);
  assert(ally.affected && IS_SET_AR(ally.affected->bitvector, AFF_ENTANGLED));
  reset(&ch, &ps, FEAT_SORCERER_BLOODLINE_SHADOW, 9); clear_affects(&ally); grouped = 0;
  damage_calls = 0; do_bloodline(&ch, "shadowstrike ally", 0, 0);
  assert(damage_calls == 1 && used[FEAT_SHADOWSTRIKE] == 1);
  assert(ally.affected && IS_SET_AR(ally.affected->bitvector, AFF_DAZZLED));
  used[FEAT_SHADOWSTRIKE] = 0; damage_calls = 0; attack_result = -1;
  do_bloodline(&ch, "shadowstrike ally", 0, 0);
  assert(!damage_calls && used[FEAT_SHADOWSTRIKE] == 1);
  /* A missed touch starts combat, and a tie against touch AC still hits. */
  assert(FIGHTING(&ch) == &ally && FIGHTING(&ally) == &ch);
  damage_calls = 0; attack_result = 0;
  do_bloodline(&ch, "shadowstrike ally", 0, 0); assert(damage_calls == 1);
  /* Shadow well grants concealment without redirecting combat and defaults to self. */
  reset(&ch, &ps, FEAT_SORCERER_BLOODLINE_SHADOW, 9);
  reset(&enemy, &eps, 0, 1); clear_affects(&ally); grouped = 1; room_target = &ally;
  world[0].people = &enemy; FIGHTING(&enemy) = &ally; FIGHTING(&ally) = &enemy;
  dark = 0; do_bloodline(&ch, "shadow-well ally", 0, 0);
  assert(!used[FEAT_SHADOW_WELL] && !ally.affected);
  dark = 1; grouped = 0;
  do_bloodline(&ch, "shadow-well ally", 0, 0); assert(!used[FEAT_SHADOW_WELL]);
  grouped = 1; do_bloodline(&ch, "shadow-well ally", 0, 0);
  assert(used[FEAT_SHADOW_WELL] == 1 && ally.affected->spell == AFFECT_SHADOW_WELL);
  assert(ally.affected->duration == 4 && FIGHTING(&enemy) == &ally && FIGHTING(&ally) == &enemy);
  used[FEAT_SHADOW_WELL] = 0; do_bloodline(&ch, "shadow-well", 0, 0);
  assert(affected_by_spell(&ch, AFFECT_SHADOW_WELL));
  /* Ride the lightning respects both movement failure and blocked/peaceful exits. */
  reset(&ch, &ps, FEAT_SORCERER_BLOODLINE_STORMBORN, 15);
  exit.to_room = 1; world[0].dir_option[0] = &exit; grouped = 0; room_target = &ally;
  move_ok = 0; do_bloodline(&ch, "ride-the-lightning north", 0, 0);
  assert(!used[FEAT_RIDE_THE_LIGHTNING] && IN_ROOM(&ch) == 0);
  move_ok = 1; SET_BIT(exit.exit_info, EX_CLOSED);
  do_bloodline(&ch, "ride-the-lightning north", 0, 0); assert(!used[FEAT_RIDE_THE_LIGHTNING]);
  REMOVE_BIT(exit.exit_info, EX_CLOSED); SET_BIT_AR(world[1].room_flags, ROOM_PEACEFUL);
  do_bloodline(&ch, "ride-the-lightning north", 0, 0); assert(!used[FEAT_RIDE_THE_LIGHTNING]);
  REMOVE_BIT_AR(world[1].room_flags, ROOM_PEACEFUL); damage_calls = 0;
  do_bloodline(&ch, "ride-the-lightning north", 0, 0);
  assert(IN_ROOM(&ch) == 1 && used[FEAT_RIDE_THE_LIGHTNING] == 1 && damage_calls == 2);
  assert(move_actions && standard_actions);
  /* Arcana trigger only on the intended spell categories and refresh without stacking. */
  reset(&ch, &ps, FEAT_SORCERER_BLOODLINE_SHADOW, 20);
  bloodline_cast_arcana(&ch, SPELL_DARKNESS, &ch);
  assert(bloodline_affect_bonus(&ch, AFFECT_SHADOW_ARCANA) == 3);
  assert(ch.affected->duration == 50 && ch.affected->bonus_type == BONUS_TYPE_COMPETENCE);
  bloodline_cast_arcana(&ch, SPELL_FIREBALL, &ally);
  assert(ch.affected && !ch.affected->next);
  reset(&ch, &ps, FEAT_SORCERER_BLOODLINE_DESTINED, 20);
  spell_info[SPELL_TIMESTOP].targets = TAR_SELF_ONLY;
  bloodline_cast_arcana(&ch, SPELL_TIMESTOP, &ally); assert(!ch.affected);
  bloodline_cast_arcana(&ch, SPELL_TIMESTOP, &ch);
  assert(bloodline_affect_bonus(&ch, AFFECT_DESTINED_ARCANA) == 9 && ch.affected->duration == 10);
  assert(used[FEAT_DESTINED_BLOODLINE_ARCANA] == 1);
  bloodline_cast_arcana(&ch, SPELL_TIMESTOP, &ch); assert(used[FEAT_DESTINED_BLOODLINE_ARCANA] == 1);
  do_bloodline(&ch, "penetration", 0, 0);
  assert(!used[FEAT_DESTINY_REALIZED] && !affected_by_spell(&ch, AFFECT_DESTINY_PENETRATION));
  /* Destined defense checks the natural confirmation die, not the attack total. */
  reset(&ch, &ps, 0, 20); reset(&ally, &aps, FEAT_SORCERER_BLOODLINE_DESTINED, 20);
  rolled = 19; assert(!is_critical_hit(&ch, NULL, 20, 100, 10, &ally, 0));
  rolled = 20; assert(is_critical_hit(&ch, NULL, 20, 0, 100, &ally, 0));
  /* Spell threats auto-confirm, including an expanded threat range. */
  reset(&ch, &ps, FEAT_SORCERER_BLOODLINE_DESTINED, 20); reset(&ally, &aps, 0, 20);
  rolled = 2; assert(is_critical_hit(&ch, NULL, 19, 0, 100, &ally, 19));
  assert(!is_critical_hit(&ch, NULL, 20, 0, 100, &ally, 0)); /* no weapon auto-confirm */
  /* Each elemental heritage converts only eligible energy, and scales defenses. */
  for (int element = BLOODLINE_ELEMENT_AIR; element <= BLOODLINE_ELEMENT_WATER; element++) {
    reset(&ch, &ps, FEAT_SORCERER_BLOODLINE_ELEMENTAL, 3);
    GET_BLOODLINE_SUBTYPE(&ch) = element;
    int energy = bloodline_element_damage(&ch);
    assert(energy && bloodline_energy_resistance(&ch, energy) == 10);
    assert(bloodline_convert_energy(&ch, SPELL_BURNING_HANDS, DAM_FIRE) == energy);
    assert(bloodline_convert_energy(&ch, SPELL_FIREBALL, DAM_FIRE) == DAM_FIRE);
    do_bloodline(&ch, "energy-conversion on", 0, 0);
    assert(bloodline_convert_energy(&ch, SPELL_FIREBALL, DAM_FIRE) == energy);
    assert(bloodline_convert_energy(&ch, SPELL_FIREBALL, DAM_SLASHING) == DAM_SLASHING);
    do_bloodline(&ch, "energy-conversion off", 0, 0);
    assert(!affected_by_spell(&ch, AFFECT_ELEMENTAL_CONVERSION));
    CLASS_LEVEL((&ch), CLASS_SORCERER) = 9; grant_extended_bloodline_powers(&ch);
    assert(bloodline_energy_resistance(&ch, energy) == 20);
    assert(!bloodline_elemental_movement(&ch, element));
    CLASS_LEVEL((&ch), CLASS_SORCERER) = 15; grant_extended_bloodline_powers(&ch);
    assert(bloodline_elemental_movement(&ch, element));
    CLASS_LEVEL((&ch), CLASS_SORCERER) = 20; grant_extended_bloodline_powers(&ch);
    assert(bloodline_energy_immunity(&ch, energy));
    reset(&ally, &aps, 0, 20); room_target = &ally; grouped = 0;
    damage_calls = 0; attack_result = 0; save_pass = 0;
    int swift_before = swift_actions, standard_before = standard_actions;
    do_bloodline(&ch, "elemental-ray ally", 0, 0);
    assert(swift_actions == swift_before + 1 && standard_actions == standard_before);
    assert(damage_calls == 1 && damage_types[0] == energy && used[FEAT_ELEMENTAL_RAY] == 1);
    damage_calls = 0; do_bloodline(&ch, "elemental-blast", 0, 0);
    assert(damage_calls == 1 && damage_types[0] == energy);
    assert(affected_by_spell(&ally, AFFECT_ELEMENTAL_VULNERABILITY));
    assert(bloodline_modify_damage(&ally, 10, AFFECT_ELEMENTAL_RAY, energy) == 15);
    clear_affects(&ally); damage_calls = 0; save_pass = 1;
    do_bloodline(&ch, "elemental-blast", 0, 0);
    assert(!affected_by_spell(&ally, AFFECT_ELEMENTAL_VULNERABILITY));
    assert(damage_amounts[0] == 10); /* 20 deterministic dice, halved by save */
  }
  /* Abyssal progression, paired claw attacks and alignment-bypassed summon DR. */
  reset(&ch, &ps, FEAT_SORCERER_BLOODLINE_ABYSSAL, 3);
  assert(bloodline_energy_resistance(&ch, DAM_ELECTRIC) == 5 && bloodline_poison_save_bonus(&ch) == 2);
  CLASS_LEVEL((&ch), CLASS_SORCERER) = 9; grant_extended_bloodline_powers(&ch);
  assert(bloodline_strength_bonus(&ch) == 2 && bloodline_poison_save_bonus(&ch) == 4);
  CLASS_LEVEL((&ch), CLASS_SORCERER) = 13; assert(bloodline_strength_bonus(&ch) == 4);
  CLASS_LEVEL((&ch), CLASS_SORCERER) = 17; grant_extended_bloodline_powers(&ch);
  assert(bloodline_strength_bonus(&ch) == 6);
  reset(&ally, &aps, 0, 20); room_target = &ally; ch.aff_abils.str = 18; attack_result = 0;
  GET_EQ(&ch, WEAR_HOLD_1) = &weapon;
  do_bloodline(&ch, "abyssal-claws ally", 0, 0); assert(!used[FEAT_ABYSSAL_CLAWS]);
  GET_EQ(&ch, WEAR_HOLD_1) = NULL;
  int claws_swift = swift_actions, claws_standard = standard_actions, claws_move = move_actions;
  do_bloodline(&ch, "abyssal-claws ally", 0, 0);
  assert(swift_actions == claws_swift + 1 && standard_actions == claws_standard && move_actions == claws_move);
  assert(used[FEAT_ABYSSAL_CLAWS] == 1 && damage_calls == 4);
  assert(damage_types[0] == DAM_SLASHING && damage_types[1] == DAM_FIRE);
  bloodline_summon_benefits(&ch, &ally, SPELL_FIREBALL); assert(!GET_DR(&ally));
  bloodline_summon_benefits(&ch, &ally, SPELL_SUMMON_CREATURE_1);
  assert(GET_DR(&ally)->amount == 5 && GET_DR(&ally)->bypass_val[0] == DR_ALIGNTYPE_GOOD);
  bloodline_summon_benefits(&ch, &ally, SPELL_SUMMON_CREATURE_1);
  assert(GET_DR(&ally) && !GET_DR(&ally)->next); /* refresh cannot stack */
  GET_ALIGNMENT(&ally) = -1000; GET_SUBRACE(&ally, 0) = 0;
  bloodline_summon_benefits(&ch, &ally, SPELL_SUMMON_CREATURE_1);
  int boosts = 0;
  for (struct affected_type *af = ally.affected; af; af = af->next)
    if (af->spell == AFFECT_ABYSSAL_SUMMON_BOOST) { assert(af->modifier == 2); boosts++; }
  assert(boosts == 3); /* Alignment, not evil subtype, controls all three bonuses. */
  assert(bloodline_summon_creature_spell(SPELL_SUMMON_CREATURE_9));
  assert(!bloodline_summon_creature_spell(SPELL_SUMMON_NATURES_ALLY_1));
  assert(!bloodline_summon_creature_spell(SPELL_FIREBALL));
  clear_affects(&ally); assert(!GET_DR(&ally));
  CLASS_LEVEL((&ch), CLASS_SORCERER) = 20; grant_extended_bloodline_powers(&ch);
  assert(bloodline_energy_immunity(&ch, DAM_ELECTRIC) && bloodline_energy_immunity(&ch, DAM_POISON));
  assert(bloodline_energy_resistance(&ch, DAM_FIRE) == 10);
  /* Healing is limited on the recipient, independent of which caster provides it. */
  reset(&ch, &ps, FEAT_SORCERER_BLOODLINE_CELESTIAL, 9);
  reset(&ally, &aps, 0, 20); room_target = &ally; grouped = 1; healed_recipient = NULL;
  GET_ALIGNMENT(&ally) = 1000; GET_HIT(&ally) = 50;
  do_bloodline(&ch, "heavenly-fire ally", 0, 0);
  assert(GET_HIT(&ally) == 55 && used[FEAT_HEAVENLY_FIRE] == 1 && healed_recipient == &ally);
  assert(last_die_sides == 12);
  used[FEAT_HEAVENLY_FIRE] = 0;
  do_bloodline(&ch, "heavenly-fire ally", 0, 0);
  assert(GET_HIT(&ally) == 55 && !used[FEAT_HEAVENLY_FIRE]);
  /* A companion shares its player's persisted receipt cooldown. */
  healed_recipient = NULL;
  SET_BIT_AR(MOB_FLAGS(&ally), MOB_ISNPC); ally.master = &ch;
  do_bloodline(&ch, "heavenly-fire ally", 0, 0);
  assert(healed_recipient == &ch && used[FEAT_HEAVENLY_FIRE] == 1);
  output[0] = 0; save_test_events(&ch);
  FILE *receipt = tmpfile(); assert(receipt); fputs(output, receipt); rewind(receipt);
  healed_recipient = NULL; load_events(receipt, &ch); fclose(receipt);
  assert(healed_recipient == &ch);
  REMOVE_BIT_AR(MOB_FLAGS(&ally), MOB_ISNPC); ally.master = NULL;
  used[FEAT_HEAVENLY_FIRE] = 0;
  healed_recipient = NULL; GET_ALIGNMENT(&ally) = 0;
  do_bloodline(&ch, "heavenly-fire ally", 0, 0); assert(!used[FEAT_HEAVENLY_FIRE]);
  GET_ALIGNMENT(&ally) = -1000; grouped = 0; damage_calls = 0;
  do_bloodline(&ch, "heavenly-fire ally", 0, 0);
  assert(damage_calls == 1 && damage_types[0] == DAM_HOLY);
  do_bloodline(&ch, "wings", 0, 0); assert(ch.affected->duration == 10 && used[FEAT_WINGS_OF_HEAVEN] == 1);
  do_bloodline(&ch, "wings off", 0, 0); assert(!affected_by_spell(&ch, AFFECT_WINGS_OF_HEAVEN));
  CLASS_LEVEL((&ch), CLASS_SORCERER) = 20; grant_extended_bloodline_powers(&ch);
  do_bloodline(&ch, "wings", 0, 0); assert(ch.affected->duration == -1 && used[FEAT_WINGS_OF_HEAVEN] == 1);
  do_bloodline(&ch, "conviction", 0, 0); rolled = 2;
  assert(bloodline_conviction_roll(&ch, 19, BLOODLINE_ROLL_ATTACK) == 2); /* second result must be kept */
  assert(bloodline_conviction_roll(&ch, 19, BLOODLINE_ROLL_ATTACK) == 19);
  assert(bloodline_energy_immunity(&ch, DAM_ACID) && bloodline_energy_immunity(&ch, DAM_COLD));
  bloodline_summon_benefits(&ch, &ally, SPELL_SUMMON_CREATURE_1);
  assert(GET_DR(&ally)->amount == 5 && GET_DR(&ally)->bypass_val[0] == DR_ALIGNTYPE_EVIL);
  /* Shadow Well uses native concealment, its counters, and strongest-effect behavior. */
  reset(&ch, &ps, FEAT_SORCERER_BLOODLINE_SHADOW, 9);
  reset(&ally, &aps, 0, 20); dark = 1;
  do_bloodline(&ch, "shadow-well", 0, 0);
  assert(compute_concealment(&ch, &ally) == 50);
  SET_BIT_AR(AFF_FLAGS(&ch), AFF_BLUR); SET_BIT_AR(AFF_FLAGS(&ch), AFF_DISPLACE);
  assert(compute_concealment(&ch, &ally) == 50);
  blindsight = true; assert(compute_concealment(&ch, &ally) == 0); blindsight = false;
  reset(&ally, &aps, FEAT_SORCERER_BLOODLINE_STORMBORN, 9);
  assert(compute_concealment(&ch, &ally) == 40);
  CLASS_LEVEL((&ally), CLASS_SORCERER) = 8;
  assert(compute_concealment(&ch, &ally) == 50);
  /* Competence Stealth bonuses use the highest effect/equipment contribution. */
  reset(&ch, &ps, FEAT_SORCERER_BLOODLINE_SHADOW, 20);
  bloodline_cast_arcana(&ch, SPELL_DARKNESS, &ch); /* circle 3 */
  assert(skill_affect_bonus(&ch, ABILITY_STEALTH) == 3);
  struct affected_type competence; new_affect(&competence);
  competence.spell = SPELL_GUIDANCE; competence.location = APPLY_SKILL;
  competence.specific = ABILITY_STEALTH; competence.bonus_type = BONUS_TYPE_COMPETENCE;
  competence.modifier = 2; affect_to_char(&ch, &competence);
  assert(skill_affect_bonus(&ch, ABILITY_STEALTH) == 3);
  GET_EQ(&ch, WEAR_HOLD_1) = &weapon;
  weapon.affected[0].location = APPLY_SKILL; weapon.affected[0].specific = ABILITY_STEALTH;
  weapon.affected[0].bonus_type = BONUS_TYPE_COMPETENCE; weapon.affected[0].modifier = 6;
  assert(skill_affect_bonus(&ch, ABILITY_STEALTH) == 6);
  competence.modifier = 8; affect_to_char(&ch, &competence);
  assert(skill_affect_bonus(&ch, ABILITY_STEALTH) == 8);
  competence.bonus_type = BONUS_TYPE_CIRCUMSTANCE; competence.modifier = 3; affect_to_char(&ch, &competence);
  assert(skill_affect_bonus(&ch, ABILITY_STEALTH) == 11);
  assert(skill_affect_bonus(&ch, ABILITY_ARCANA) == 0);
  /* Earth discounts ten final movement points, retaining terrain and cost floors. */
  reset(&ch, &ps, FEAT_SORCERER_BLOODLINE_ELEMENTAL, 15);
  GET_BLOODLINE_SUBTYPE(&ch) = BLOODLINE_ELEMENT_EARTH;
  world[0].sector_type = world[1].sector_type = SECT_FOREST;
  movement_loss[SECT_FOREST] = 4;
  assert(calculate_movement_cost(&ch, 0, 1, 0) == 30);
  SET_BIT(world[1].room_affections, RAFF_DIFFICULT_TERRAIN);
  assert(calculate_movement_cost(&ch, 0, 1, 0) == 30);
  GET_BLOODLINE_SUBTYPE(&ch) = BLOODLINE_ELEMENT_FIRE;
  assert(calculate_movement_cost(&ch, 0, 1, 0) == 80);
  REMOVE_BIT(world[1].room_affections, RAFF_DIFFICULT_TERRAIN);
  GET_BLOODLINE_SUBTYPE(&ch) = BLOODLINE_ELEMENT_EARTH; movement_loss[SECT_FOREST] = 1;
  assert(calculate_movement_cost(&ch, 0, 1, 0) == 5);
  /* Conviction waits for the selected category and validates input before spending. */
  reset(&ch, &ps, FEAT_SORCERER_BLOODLINE_CELESTIAL, 15);
  do_bloodline(&ch, "conviction invalid", 0, 0);
  assert(!used[FEAT_CELESTIAL_CONVICTION] && !ch.affected);
  for (int category = BLOODLINE_ROLL_ATTACK; category <= BLOODLINE_ROLL_SKILL; category++) {
    const char *commands[] = {"", "conviction attack", "conviction save", "conviction skill"};
    used[FEAT_CELESTIAL_CONVICTION] = 0;
    do_bloodline(&ch, commands[category], 0, 0);
    assert(used[FEAT_CELESTIAL_CONVICTION] == 1);
    int other = category == BLOODLINE_ROLL_ATTACK ? BLOODLINE_ROLL_SAVE : BLOODLINE_ROLL_ATTACK;
    assert(bloodline_conviction_roll(&ch, 19, other) == 19);
    assert(affected_by_spell(&ch, AFFECT_CELESTIAL_CONVICTION));
    rolled = 2; assert(bloodline_conviction_roll(&ch, 19, category) == 2);
    assert(!affected_by_spell(&ch, AFFECT_CELESTIAL_CONVICTION));
  }
  assert(bloodline_cooldown_seconds(FEAT_CELESTIAL_CONVICTION) == 30);
  assert(bloodline_cooldown_seconds(FEAT_DRACONIC_HERITAGE_CLAWS) == 300);
  assert(bloodline_cooldown_seconds(FEAT_METAMAGIC_ADEPT) == 300);
  assert(!bloodline_cooldown_seconds(FEAT_RAGE));
  assert(!bloodline_knockdown_bonus(&ch));
  reset(&ch, &ps, FEAT_SORCERER_BLOODLINE_STORMBORN, 3);
  assert(bloodline_knockdown_bonus(&ch) == 4);
  /* Failed SR rerolls preserve all fixed modifiers and only trigger on <=5%. */
  reset(&ch, &ps, FEAT_SORCERER_BLOODLINE_DESTINED, 20); rolled = 20;
  proc_chance = 6; assert(bloodline_retry_spell_resistance(&ch, 15, 25, 5) == 15);
  proc_chance = 5; assert(bloodline_retry_spell_resistance(&ch, 15, 25, 5) == 30);
  assert(bloodline_retry_spell_resistance(&ch, 30, 25, 5) == 30);
  rolled = 1; assert(bloodline_retry_spell_resistance(&ch, 15, 25, 5) == 11);
  proc_chance = 100;
  /* All seventeen appended cooldown types survive a character-event round trip. */
  for (int feat = 1; feat < NUM_FEATS; feat++)
    if (feat_list[feat].event >= eBLOODLINE_SHADOWSTRIKE && feat_list[feat].event <= eBLOODLINE_DESTINED_ARCANA)
      used[feat] = feat == FEAT_SHADOWSTRIKE ? 2 : 1;
  output[0] = 0; save_test_events(&ch);
  FILE *saved = tmpfile(); assert(saved); fputs(output, saved); rewind(saved);
  memset(used, 0, sizeof(used)); load_events(saved, &ch); fclose(saved);
  int restored = 0;
  for (int feat = 1; feat < NUM_FEATS; feat++)
    if (feat_list[feat].event >= eBLOODLINE_SHADOWSTRIKE && feat_list[feat].event <= eBLOODLINE_DESTINED_ARCANA) {
      assert(used[feat] == (feat == FEAT_SHADOWSTRIKE ? 2 : 1)); restored++;
    }
  assert(restored == 17);
  clear_affects(&ch); clear_affects(&ally); clear_affects(&enemy);
  return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='sorcerer-bloodlines-') as tmp:
    path = Path(tmp) / 'check.c'
    path.write_text(headers + '\n' + code)
    binary = Path(tmp) / 'check'
    subprocess.run([os.environ.get('CC', 'cc'), '-std=gnu99', '-Wall', '-Wextra',
                    '-g', '-fsanitize=address,undefined', '-Werror=implicit-function-declaration', '-Wno-unused-parameter',
                    '-I' + str(root / 'src'), str(path), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print('Sorcerer bloodlines: progression, previews, spells, swift actions, concealment, summons, healing, selected rerolls, criticals and cooldown persistence passed.')
