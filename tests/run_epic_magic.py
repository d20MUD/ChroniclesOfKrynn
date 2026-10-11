#!/usr/bin/env python3
"""Exercise production epic pools, saves, timed effects, and summon actions under sanitizers."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
src = root / 'src'

def function(filename, signature):
    text = (src / filename).read_text()
    a = text.index(signature)
    return text[a:text.index('\n}', a) + 2]

headers = '\n'.join('#include "' + h + '"' for h in
                    ['conf.h', 'sysdep.h', 'structs.h', 'utils.h', 'comm.h', 'db.h',
                     'handler.h', 'spells.h', 'fight.h', 'mud_event.h', 'dg_scripts.h',
                     'domains_schools.h', 'evolutions.h', 'epic_magic.h', 'missions.h',
                     'assign_wpn_armor.h', 'perks.h', 'oasis.h'])
code = headers + r'''
#include <assert.h>
struct config_data config_info;
struct player_special_data dummy_mob;
void basic_mud_log(const char *format, ...) {}
int char_has_evolution(struct char_data *ch, int evo) { return 0; }
struct room_data *world;
struct spell_info_type spell_info[TOP_SPELL_DEFINE + 1];
static struct char_data *known[6];
static struct mud_event_data *events[30];
static int event_count, damage_count, pvp_calls, last_save_modifier;
static bool allow_summon = true, prototype_exists = true, hit_succeeds = true, cooldown;
static bool absorb_damage, extract_attacker;
int MIN(int a, int b) { return a < b ? a : b; }
int MAX(int a, int b) { return a > b ? a : b; }
size_t send_to_char(struct char_data *ch, const char *fmt, ...) { return 0; }
const char *act(const char *s, int hide, struct char_data *ch, struct obj_data *obj,
                void *vict, int type) { return s; }
mob_rnum real_mobile(mob_vnum vnum) { return prototype_exists ? 0 : NOBODY; }
bool can_add_follower_by_flag(struct char_data *ch, int flag) { return allow_summon; }
int aoeOK(struct char_data *ch, struct char_data *target, int spell) {
  return ch != target && IS_NPC(target) && !AFF_FLAGGED(target, AFF_CHARM);
}
bool is_mission_mob(struct char_data *ch, struct char_data *mob) { return true; }
bool pvp_ok(struct char_data *ch, struct char_data *target, bool display) {
  pvp_calls++;
  return false; /* Mimic a non-PVP world: ordinary NPC combat must still work. */
}
bool affected_by_spell(struct char_data *ch, int spell) {
  struct affected_type *af;
  for (af = ch->affected; af; af = af->next) if (af->spell == spell) return true;
  return false;
}
void new_affect(struct affected_type *af) { memset(af, 0, sizeof(*af)); }
void affect_to_char(struct char_data *ch, struct affected_type *af) {
  struct affected_type *copy = malloc(sizeof(*copy));
  *copy = *af; copy->next = ch->affected; ch->affected = copy;
}
void affect_from_char(struct char_data *ch, int spell) {
  struct affected_type **p = &ch->affected;
  while (*p) {
    if ((*p)->spell == spell) { struct affected_type *old = *p; *p = old->next; free(old); }
    else p = &(*p)->next;
  }
}
struct mud_event_data *new_mud_event(event_id id, void *owner, const char *vars) {
  struct mud_event_data *event = calloc(1, sizeof(*event));
  event->iId = id; event->pStruct = owner;
  if (vars) event->sVariables = strdup(vars);
  return event;
}
void attach_mud_event(struct mud_event_data *event, long delay) {
  assert(delay > 0); assert(event_count < 30); events[event_count++] = event;
}
struct mud_event_data *char_has_mud_event(struct char_data *ch, event_id id) {
  static struct mud_event_data dummy;
  return cooldown ? &dummy : NULL;
}
struct char_data *find_char(long id) {
  int i; for (i = 0; i < 6; i++) if (known[i] && GET_ID(known[i]) == id) return known[i];
  return NULL;
}
int damage(struct char_data *ch, struct char_data *victim, int amount, int spell, int type,
            int offhand) {
  int i;
  if (extract_attacker)
    for(i=0;i<6;i++) if(known[i]==ch) known[i]=NULL;
  if (absorb_damage) return -1;
  assert(amount > 0); damage_count++; GET_HIT(victim) -= amount;
  if (GET_HIT(victim) <= 0) { GET_POS(victim) = POS_DEAD; return -1; }
  return amount;
}
int is_player_grouped(struct char_data *a, struct char_data *b) { return a == b; }
bool is_epic_summon_mob(struct char_data *mob) {
  return MOB_FLAGGED(mob, MOB_SUMMON_SOLAR) || MOB_FLAGGED(mob, MOB_MUMMY_DUST) ||
         MOB_FLAGGED(mob, MOB_DRAGON_KNIGHT);
}
bool formation_can_melee_target(struct char_data *mob, struct char_data *victim) { return true; }
int attack_roll(struct char_data *ch, struct char_data *victim, int type, int touch, int mod) {
  return hit_succeeds;
}
void update_pos(struct char_data *ch) {}
int savingthrow(struct char_data *ch, struct char_data *victim, int type, int modifier,
                 int casttype, int level, int school) {
  last_save_modifier = modifier; return true;
}
int get_feat_value(struct char_data *ch, int feat) { return feat == FEAT_STALWART; }
bool weapon_bypasses_dr(struct obj_data *obj, struct damage_reduction_type *dr,
                        struct char_data *ch, int kind) { return false; }
bool is_bare_handed(struct char_data *ch) { return false; }
bool is_monk_weapon(struct obj_data *obj) { return false; }
int get_monk_dr_bypass(struct char_data *ch) { return 0; }
const char *get_wearoff(int spell) { return ""; }
void send_combat_roll_info(struct char_data *ch,const char *format,...) {}
int get_attack_bonus_cap(struct char_data *ch) { return MAX_BAB; }
int formation_ranged_penalty(struct char_data *ch) { return -8; }
void draw_line(struct char_data *ch,int length,char left,char right) {}
int compute_arcane_level(struct char_data *ch) { return CLASS_LEVEL(ch,CLASS_WIZARD); }
int compute_divine_level(struct char_data *ch) { return 0; }
int compute_arcana_golem_level(struct char_data *ch) { return 0; }
int practiced_spellcaster_level(struct char_data *ch,int class,int base_level) { return base_level; }
bool isEpicSpell(int spell) { return IS_EPIC_SPELL(spell); }
bool is_spellnum_psionic(int spell) { return spell>=PSIONIC_POWER_START && spell<=PSIONIC_POWER_END; }
void autoroll_mob(struct char_data *mob,bool realmode,bool summoned) {
  GET_REAL_HITROLL(mob)=GET_HITROLL(mob)=2;
  GET_REAL_DAMROLL(mob)=GET_DAMROLL(mob)=3;
}
'''
for signature in ['int get_epic_spell_casts_max(', 'void normalize_epic_spell_casts(',
                  'void regenerate_epic_spell_cast(', 'int compute_caster_level(']:
    code += function('utils.c', signature)

# Compile the actual saving-throw damage branch, including epic half damage.
magic = (src / 'magic.c').read_text()
a = magic.index('  else if (dam && (save != -1))')
b = magic.index('  /* blinking between', a)
code += r'''
int save_damage(struct char_data *ch, struct char_data *victim, int spellnum, int save, int dam) {
  int race_bonus = 0, dc_mod = IS_EPIC_SPELL(spellnum) ? -4 : 0;
  int casttype = CAST_SPELL, level = 30, spell_school = NECROMANCY;
  bool save_negates = spellnum == PSIONIC_IMPALE_MIND;
  if (0) {}
'''
code += magic[a:b] + '\nreturn dam;\n}\n'

# Exercise the exact player-file branches for the newly saved pool.
players = (src / 'players.c').read_text()
a = players.index('        else if (!strcmp(tag, "EpPc"))')
b = players.index('        else if (!strcmp(tag, "EidB"))', a)
code += 'void load_preparation(struct char_data *ch, const char *tag, const char *line) { if(0) {}\n'
code += players[a:b] + '\n}\n'
a = players.index('  normalize_epic_preparation_casts(ch);')
b = players.index('  if (GET_RETAINER_COOLDOWN(ch) != 0)', a)
code += '''char persisted[256];
#define BUFFER_WRITE(...) snprintf(persisted + strlen(persisted), sizeof(persisted)-strlen(persisted), __VA_ARGS__)
void save_preparation(struct char_data *ch) { persisted[0] = '\\0';
'''
code += players[a:b] + '\n}\n#undef BUFFER_WRITE\n'
fight = (src / 'fight.c').read_text()
a = fight.index('#define STONESKIN_ABSORB 15')
b = fight.index('#undef IRONSKIN_ABSORB', a) + len('#undef IRONSKIN_ABSORB')
code += fight[a:b] + '\n'
code += function('fight.c', 'int apply_damage_reduction(')
a = fight.index('  int maximum_bab = get_attack_bonus_cap(ch);', fight.index('int compute_attack_bonus_full('))
b = fight.index('\n}', a) + 2
code += '''int capped_attack(struct char_data *ch,int attack_type) {
int calc_bab=80, formation_penalty=0; bool display=FALSE;
'''
code += fight[a:b] + '\n'
parser = (src / 'spell_parser.c').read_text()
a = parser.index('  else if (isEpicSpell(spellnum))', parser.index('int cast_spell_with_type_and_slot('))
b = parser.index('  /* concentration check */', a)
code += 'int instant_epic_level(struct char_data *ch,int spellnum) { int clevel=0; if(0) {}\n'
code += parser[a:b] + '\nreturn clevel;\n}\n'
a = magic.index('    case SPELL_MUMMY_DUST:', magic.index('/* give the mobile some bonuses */'))
b = magic.index('    case SPELL_SUMMON_NATURES_ALLY_8:', a)
code += 'void scale_epic_ally(struct char_data *ch,struct char_data *mob,int spellnum,int level) { switch(spellnum) {\n'
code += magic[a:b] + '\n}}\n'

# Exercise the actual armor/ward construction, rather than a copy of their numbers.
a = magic.index('  case SPELL_EPIC_MAGE_ARMOR: // epic')
b = magic.index('  case SPELL_EXPEDITIOUS_RETREAT:', a)
code += '''void epic_protection(struct char_data *ch, struct char_data *victim, int spellnum, int level) {
struct affected_type af[MAX_SPELL_AFFECTS];
bool accum_duration = FALSE; const char *to_vict = NULL, *to_room = NULL;
int i; for(i=0;i<MAX_SPELL_AFFECTS;i++) new_affect(&af[i]);
switch(spellnum) {
'''
code += magic[a:b] + '''}
if(spellnum==SPELL_EPIC_MAGE_ARMOR) {
  assert(af[0].modifier==20 && af[0].bonus_type==BONUS_TYPE_ARMOR);
  assert(af[1].modifier==3 && af[1].bonus_type==BONUS_TYPE_DODGE);
}
af[0].spell=spellnum; affect_to_char(victim,&af[0]);
}
'''
code += r'''
int main(void) {
  struct char_data caster = {0}, enemy = {0}, ally = {0}, stranger = {0};
  struct player_special_data saved = {0};
  struct room_data rooms[2] = {0};
  struct damage_reduction_type defenses = {0};
  int i, before, hp;
  world = rooms;
  caster.player_specials = &saved;
  GET_LEVEL(&caster)=30; saved.saved.class_level[CLASS_WIZARD]=30;
  saved.saved.class_level[CLASS_PSIONICIST]=25;
  assert(instant_epic_level(&caster,SPELL_EPIC_WARDING)==30);
  assert(instant_epic_level(&caster,PSIONIC_IMPALE_MIND)==25);
  GET_ID(&caster) = 100; GET_ID(&enemy) = 101; GET_ID(&ally) = 102;
  known[0] = &caster; known[1] = &enemy; known[2] = &ally;
  SET_BIT_AR(MOB_FLAGS(&enemy), MOB_ISNPC);
  SET_BIT_AR(MOB_FLAGS(&ally), MOB_ISNPC);
  GET_POS(&caster) = GET_POS(&enemy) = GET_POS(&ally) = POS_STANDING;
  GET_HIT(&caster) = GET_MAX_HIT(&caster) = 1000;
  GET_HIT(&enemy) = GET_MAX_HIT(&enemy) = 1000;
  GET_HIT(&ally) = GET_MAX_HIT(&ally) = 1000;
  scale_epic_ally(&caster,&ally,SPELL_SUMMON_SOLAR,21);
  assert(GET_LEVEL(&ally)==30 && MOB_FLAGGED(&ally,MOB_SUMMON_SOLAR));
  scale_epic_ally(&caster,&ally,SPELL_MUMMY_DUST,30);
  assert(GET_LEVEL(&ally)==35 && MOB_FLAGGED(&ally,MOB_MUMMY_DUST));
  assert(!MOB_FLAGGED(&ally,MOB_SUMMON_SOLAR));
  assert(GET_HITROLL(&ally)==12 && GET_REAL_HITROLL(&ally)==12);
  assert(GET_DAMROLL(&ally)==13 && GET_REAL_DAMROLL(&ally)==13);
  REMOVE_BIT_AR(MOB_FLAGS(&ally),MOB_MUMMY_DUST);
  rooms[0].people = &caster; caster.next_in_room = &enemy;
  saved.saved.abilities[ABILITY_SPELLCRAFT] = 50;
  GET_EPIC_SPELL_CASTS((&caster)) = -1;
  GET_EPIC_PREPARATION_CASTS(&caster) = -1;
  normalize_epic_spell_casts(&caster); normalize_epic_preparation_casts(&caster);
  assert(GET_EPIC_SPELL_CASTS((&caster)) == 10);
  assert(GET_EPIC_PREPARATION_CASTS(&caster) == 3);
  assert(epic_spell_cast_cost(SPELL_GREATER_RUIN, METAMAGIC_EMPOWER) == 2);
  assert(epic_spell_cast_cost(SPELL_EPIC_WARDING, METAMAGIC_QUICKEN) == 2);
  assert(epic_spell_cast_cost(SPELL_EPIC_WARDING, METAMAGIC_EMPOWER) == -1);
  assert(epic_spell_cast_cost(SPELL_HELLBALL, METAMAGIC_QUICKEN|METAMAGIC_EMPOWER) == -1);
  assert(epic_spell_cast_cost(SPELL_HELLBALL, METAMAGIC_MAXIMIZE) == -1);
  assert(epic_spell_preflight(&caster, &caster, SPELL_EPIC_WARDING, 0, false));
  spend_epic_spell_casts(&caster, SPELL_EPIC_WARDING, 0);
  assert(GET_EPIC_SPELL_CASTS((&caster)) == 10);
  assert(GET_EPIC_PREPARATION_CASTS(&caster) == 2);
  assert(epic_spell_preflight(&caster, &enemy, SPELL_GREATER_RUIN, METAMAGIC_EMPOWER, false));
  spend_epic_spell_casts(&caster, SPELL_GREATER_RUIN, METAMAGIC_EMPOWER);
  assert(GET_EPIC_SPELL_CASTS((&caster)) == 8);
  assert(GET_EPIC_PREPARATION_CASTS(&caster) == 2);
  for (i=0; i<14; i++) { regenerate_epic_spell_cast(&caster); regenerate_epic_preparation_cast(&caster); }
  assert(GET_EPIC_SPELL_CASTS((&caster)) == 8 && GET_EPIC_PREPARATION_CASTS(&caster) == 2);
  regenerate_epic_spell_cast(&caster); regenerate_epic_preparation_cast(&caster);
  assert(GET_EPIC_SPELL_CASTS((&caster)) == 9 && GET_EPIC_PREPARATION_CASTS(&caster) == 3);
  assert(GET_EPIC_PREPARATION_REGEN_TIMER(&caster) == 0);
  load_preparation(&caster, "EpPc", "1"); load_preparation(&caster, "EpPr", "7");
  assert(GET_EPIC_PREPARATION_CASTS(&caster)==1 && GET_EPIC_PREPARATION_REGEN_TIMER(&caster)==7);
  save_preparation(&caster);
  assert(strstr(persisted,"EpPc: 1\n") && strstr(persisted,"EpPr: 7\n"));
  GET_EPIC_PREPARATION_CASTS(&caster)=3; GET_EPIC_PREPARATION_REGEN_TIMER(&caster)=0;
  save_preparation(&caster); assert(!*persisted);
  GET_EPIC_PREPARATION_CASTS(&caster)=-1;
  normalize_epic_preparation_casts(&caster); assert(GET_EPIC_PREPARATION_CASTS(&caster)==3);
  load_preparation(&caster, "EpPc", "1"); load_preparation(&caster, "EpPr", "7");
  for(i=0;i<8;i++) regenerate_epic_preparation_cast(&caster);
  assert(GET_EPIC_PREPARATION_CASTS(&caster)==2);
  allow_summon = false;
  before = GET_EPIC_PREPARATION_CASTS(&caster);
  assert(!epic_spell_preflight(&caster, NULL, SPELL_SUMMON_SOLAR, 0, false));
  assert(before == GET_EPIC_PREPARATION_CASTS(&caster));
  allow_summon = true; prototype_exists = false;
  assert(!epic_spell_preflight(&caster, NULL, SPELL_SUMMON_SOLAR, 0, false));
  prototype_exists = true;
  assert(epic_spell_preflight(&caster, NULL, SPELL_SUMMON_SOLAR, 0, false));
  assert(!epic_spell_preflight(&caster, NULL, SPELL_EPIC_WARDING, 0, false));
  spell_info[SPELL_GREATER_RUIN].targets=TAR_CHAR_ROOM|TAR_FIGHT_VICT;
  assert(!epic_spell_preflight(&caster,NULL,SPELL_GREATER_RUIN,0,false));
  spell_info[SPELL_HELLBALL].violent = true;
  spell_info[SPELL_HELLBALL].routines = MAG_AREAS;
  assert(epic_spell_preflight(&caster, NULL, SPELL_HELLBALL, 0, false));
  SET_BIT_AR(MOB_FLAGS(&enemy), MOB_NOKILL);
  assert(!epic_spell_preflight(&caster, NULL, SPELL_HELLBALL, 0, false));
  REMOVE_BIT_AR(MOB_FLAGS(&enemy), MOB_NOKILL);
  assert(save_damage(&caster, &enemy, PSIONIC_IMPALE_MIND, SAVING_WILL, 400) == 200);
  assert(last_save_modifier == -4);
  assert(save_damage(&caster, &stranger, SPELL_GREATER_RUIN, SAVING_WILL, 600) == 300);
  assert(save_damage(&caster, &stranger, SPELL_FIREBALL, SAVING_WILL, 400) == 0);
  epic_protection(&caster,&caster,SPELL_EPIC_MAGE_ARMOR,30);
  epic_protection(&caster,&caster,SPELL_EPIC_WARDING,30);
  assert(GET_STONESKIN(&caster)==1800 && epic_ward_surge_active(&caster));
  assert(handle_warding(&enemy,&caster,200)==95 && GET_STONESKIN(&caster)==1695);
  affect_from_char(&caster,AFFECT_EPIC_WARD_SURGE);
  assert(!epic_ward_surge_active(&caster));
  assert(handle_warding(&enemy,&caster,200)==125 && GET_STONESKIN(&caster)==1620);
  epic_protection(&caster,&caster,SPELL_EPIC_WARDING,30);
  affect_from_char(&caster,SPELL_EPIC_WARDING);
  assert(!epic_ward_surge_active(&caster)); /* Dispelled ward gives no immunity. */
  epic_spell_damage_effects(&caster, &enemy, SPELL_GREATER_RUIN, 30, 300);
  assert(affected_by_spell(&enemy, AFFECT_EPIC_RUIN));
  assert(enemy.affected->duration == 3);
  epic_spell_damage_effects(&caster, &enemy, SPELL_GREATER_RUIN, 30, 300);
  assert(!enemy.affected->next); /* Recasting refreshes; it never stacks. */
  defenses.amount=25; defenses.max_damage=-1; GET_DR(&enemy)=&defenses;
  assert(apply_damage_reduction(&caster,&enemy,NULL,100,false,ATTACK_TYPE_PRIMARY)==85);
  affect_from_char(&enemy,AFFECT_EPIC_RUIN);
  assert(apply_damage_reduction(&caster,&enemy,NULL,100,false,ATTACK_TYPE_PRIMARY)==75);
  epic_spell_damage_effects(&caster, &enemy, SPELL_HELLBALL, 30, 500);
  assert(event_count == 1);
  epic_spell_damage_effects(&caster, &enemy, SPELL_HELLBALL, 30, 500);
  assert(event_count == 1); /* No duplicate lingering fire. */
  hp = GET_HIT(&enemy);
  assert(event_epic_hellfire(events[0]) == PULSE_VIOLENCE);
  assert(GET_HIT(&enemy) == hp - 60);
  assert(event_epic_hellfire(events[0]) == 0);
  assert(GET_HIT(&enemy) == hp - 120 && !affected_by_spell(&enemy,AFFECT_EPIC_HELLFIRE));
  epic_spell_damage_effects(&caster, &enemy, SPELL_HELLBALL, 30, 500);
  enemy.in_room = 1; hp = GET_HIT(&enemy);
  assert(event_epic_hellfire(events[1]) == 0 && GET_HIT(&enemy)==hp);
  enemy.in_room = 0; affect_from_char(&enemy,AFFECT_EPIC_HELLFIRE);
  epic_spell_damage_effects(&caster, &enemy, SPELL_HELLBALL, 30, 500);
  known[1] = NULL;
  assert(event_epic_hellfire(events[2]) == 0); /* Purged target, no stale pointer. */
  known[1] = &enemy; affect_from_char(&enemy,AFFECT_EPIC_HELLFIRE);
  epic_spell_damage_effects(&caster, &enemy, SPELL_HELLBALL, 30, 500);
  GET_HIT(&enemy) = 20;
  assert(event_epic_hellfire(events[3]) == 0 && GET_POS(&enemy)==POS_DEAD);
  GET_POS(&enemy)=POS_STANDING; GET_HIT(&enemy)=1000;
  affect_from_char(&enemy,AFFECT_EPIC_HELLFIRE);
  epic_spell_damage_effects(&caster,&enemy,SPELL_HELLBALL,30,500);
  absorb_damage=true;
  assert(event_epic_hellfire(events[4])==PULSE_VIOLENCE); /* Absorption is not death. */
  absorb_damage=false;
  assert(event_epic_hellfire(events[4])==0 && GET_HIT(&enemy)==940);
  ally.master = &caster; FIGHTING(&ally)=&enemy; SET_BIT_AR(AFF_FLAGS(&ally),AFF_CHARM);
  GET_LEVEL(&ally)=35; SET_BIT_AR(MOB_FLAGS(&ally),MOB_SUMMON_SOLAR);
  GET_HIT(&caster)=500;
  assert(epic_summon_combat_turn(&ally)); assert(GET_HIT(&caster)==675);
  cooldown = true; epic_summon_combat_turn(&ally); assert(GET_HIT(&caster)==675);
  cooldown = false; REMOVE_BIT_AR(MOB_FLAGS(&ally),MOB_SUMMON_SOLAR);
  SET_BIT_AR(MOB_FLAGS(&ally),MOB_MUMMY_DUST);
  hit_succeeds = false; epic_summon_combat_turn(&ally); assert(!affected_by_spell(&enemy,AFFECT_EPIC_MUMMY_DREAD));
  hit_succeeds = true; epic_summon_combat_turn(&ally); assert(affected_by_spell(&enemy,AFFECT_EPIC_MUMMY_DREAD));
  assert(capped_attack(&enemy,ATTACK_TYPE_PRIMARY)==51);
  assert(capped_attack(&enemy,ATTACK_TYPE_RANGED)==43);
  REMOVE_BIT_AR(MOB_FLAGS(&ally),MOB_MUMMY_DUST); SET_BIT_AR(MOB_FLAGS(&ally),MOB_DRAGON_KNIGHT);
  before = damage_count; hp = GET_HIT(&enemy);
  epic_summon_combat_turn(&ally);
  assert(damage_count==before+1 && GET_HIT(&enemy)==hp-70); /* Reflex halves level*4. */
  extract_attacker=true; assert(!epic_summon_combat_turn(&ally));
  extract_attacker=false; known[2]=&ally;
  for(i=0;i<event_count;i++) { free(events[i]->sVariables); free(events[i]); }
  while(enemy.affected) affect_from_char(&enemy,enemy.affected->spell);
  while(caster.affected) affect_from_char(&caster,caster.affected->spell);
  puts("Epic pools, recovery, persistence, saves, lingering damage, and summon actions passed.");
  return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='epic-magic-') as temp:
    harness = Path(temp) / 'check.c'
    exe = Path(temp) / 'check'
    harness.write_text(code)
    subprocess.run(['cc', '-I', str(src), '-fsanitize=address,undefined', '-g', '-o', str(exe),
                    str(harness), str(src / 'epic_magic.c')], check=True)
    subprocess.run([str(exe)], check=True)
