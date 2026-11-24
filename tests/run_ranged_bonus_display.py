#!/usr/bin/env python3
"""Check the production ranged display against per-shot ammunition and feat state."""
from pathlib import Path
import os
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'src/fight.c').read_text()
assert 'show_ranged_attack_details(ch, victim, wielded, TRUE);' in source
assert 'show_ranged_attack_details(ch, victim, wielded, FALSE);' in source
assert 'show_ranged_attack_details(ch, FIGHTING(ch), get_wielded(ch, ATTACK_TYPE_RANGED), FALSE);' in source


def function(name):
    start = source.index(name)
    return source[start:source.index('\n}', start) + 2]


headers = ['conf.h', 'sysdep.h', 'structs.h', 'utils.h', 'spells.h',
           'assign_wpn_armor.h', 'fight.h', 'handler.h', 'comm.h']
code = '\n'.join('#include "' + name + '"' for name in headers)
code += r'''
#include <assert.h>
static char output[8192];
static int feats[NUM_FEATS], feet, base_bab, magic;
static bool fire_ready;
static int rolls, ordinary;
const char *damtypes[NUM_DAM_TYPES + 1] = {[DAM_FIRE] = "fire"};
struct weapon_table weapon_list[NUM_WEAPON_TYPES];
int MAX(int a, int b) {return a > b ? a : b;}
int MIN(int a, int b) {return a < b ? a : b;}
int get_feat_value(struct char_data *ch, int feat) {return feats[feat];}
int BAB(struct char_data *ch) {return base_bab;}
int combat_distance(struct char_data *ch, struct char_data *victim) {return victim ? feet : -1;}
bool affected_by_spell(struct char_data *ch, int spell) {return magic > 0;}
int get_char_affect_modifier(struct char_data *ch, int spell, int location) {return magic;}
bool can_fire_ammo(struct char_data *ch, bool silent) {return fire_ready;}
struct obj_data *get_throwing_source(struct char_data *ch) {return NULL;}
bool is_throwing_weapon(struct char_data *ch, struct obj_data *weapon) {return false;}
size_t send_to_char(struct char_data *ch, const char *format, ...) {
  va_list args; va_start(args, format);
  int n = vsnprintf(output, sizeof(output), format, args); va_end(args); ordinary++; return n;
}
void send_combat_roll_info(struct char_data *ch, const char *format, ...) {
  va_list args; va_start(args, format);
  vsnprintf(output, sizeof(output), format, args); va_end(args); rolls++;
}
'''
a = source.index('struct ranged_shot_context\n')
b = source.index('struct obj_data *active_throwing_weapon(', a)
code += source[a:b]
for name in ['static struct obj_data *ranged_ammunition(', 'int point_blank_shot_bonus(',
             'int deadly_aim_step(', 'int ranged_weapon_increment(', 'int ranged_range_penalty(',
             'static void show_ranged_attack_details(']:
    code += '\n' + function(name)
# Exercise the actual enhancement blocks from attack and damage calculation.
a = source.index('    int ammo_contribution = ')
a = source.rindex('  if (ammo)\n', 0, a)
b = source.index('  if (IS_WILDSHAPED(ch)', a)
code += '\nstatic int attack_enhancement(struct char_data *ch, struct obj_data *ammo, int base) {\n'
code += 'int bonuses[NUM_BONUS_TYPES] = {0}, calc_bab = 0; bool display = false;\n'
code += 'bonuses[BONUS_TYPE_ENHANCEMENT] = base;\n' + source[a:b]
code += 'return bonuses[BONUS_TYPE_ENHANCEMENT] + calc_bab;\n}\n'
a = source.index('  /* ranged includes arrow enhancement bonus + special ranged bonus')
a = source.index('  if (ammo)\n', a)
b = source.index('    /* favored enemy */', a)
code += '\nstatic int damage_enhancement(struct char_data *ch, struct obj_data *ammo, struct obj_data *wielded, int base) {\n'
code += 'int dambonus = base; bool display_mode = false;\n' + source[a:b]
code += '} return dambonus;\n}\n'
code += r'''
int main(void) {
  struct char_data ch = {0}, victim = {0};
  struct obj_data bow = {0}, pouch = {0}, arrow = {0}, next = {0};
  struct ranged_shot_context shot = {.attacker = &ch, .projectile = &arrow};
  GET_OBJ_TYPE(&bow) = ITEM_WEAPON;
  GET_OBJ_VAL(&bow, 0) = WEAPON_TYPE_LONG_BOW;
  weapon_list[WEAPON_TYPE_LONG_BOW].range = 100;
  GET_OBJ_TYPE(&arrow) = GET_OBJ_TYPE(&next) = ITEM_MISSILE;
  GET_OBJ_VAL(&arrow, 0) = GET_OBJ_VAL(&next, 0) = AMMO_TYPE_ARROW;
  GET_OBJ_VAL(&arrow, 4) = 3; GET_OBJ_VAL(&bow, 4) = 2; GET_OBJ_VAL(&next, 4) = 7;
  arrow.short_description = "last flaming arrow"; next.short_description = "next stack";
  GET_OBJ_VAL(&arrow, AMMO_VAL_DAMAGE_TYPE) = DAM_FIRE;
  GET_OBJ_VAL(&arrow, AMMO_VAL_DAMAGE_DIE) = 6;
  GET_EQ(&ch, WEAR_AMMO_POUCH) = &pouch; pouch.contains = &next;
  feats[FEAT_ENHANCE_ARROW_MAGIC] = 4;
  feats[FEAT_POINT_BLANK_SHOT] = feats[FEAT_PRECISE_SHOT] = 1;
  feats[FEAT_IMPROVED_PRECISE_SHOT] = feats[FEAT_FAR_SHOT] = 1;
  feats[FEAT_DEADLY_AIM] = feats[FEAT_RAPID_SHOT] = 1;
  SET_BIT_AR(AFF_FLAGS(&ch), AFF_DEADLY_AIM);
  SET_BIT_AR(AFF_FLAGS(&ch), AFF_RAPID_SHOT);
  base_bab = 12; feet = 30; current_shot = &shot;
  /* The fired last arrow is used even if no ammunition remains ready. */
  show_ranged_attack_details(&ch, &victim, &bow, true);
  assert(rolls == 1 && !ordinary);
  assert(!strcmp(output, " [Ammo:+3 EA:+4 PBS:+1 DA:-4/+8 +1d6 fire]"));
  assert(!strstr(output, "last flaming arrow") && !strstr(output, "Precise Shot"));
  show_ranged_attack_details(&ch, &victim, &bow, false);
  assert(strstr(output, "last flaming arrow") && !strstr(output, "next stack"));
  assert(strstr(output, "ammo enhancement +3"));
  assert(strstr(output, "weapon/ammo enhancement +3 (highest equipment/spell only)"));
  assert(strstr(output, "Enhance Arrow +4 attack/damage (stacks with ammo)"));
  assert(strstr(output, "Point Blank Shot +1 attack/damage"));
  assert(strstr(output, "Precise Shot: no -4") && !strstr(output, "Precise Shot +4"));
  assert(strstr(output, "ignores concealment below 50%"));
  assert(strstr(output, "Deadly Aim -4 attack/+8 damage"));
  assert(strstr(output, "Rapid Shot mode"));
  assert(strstr(output, "Far Shot: 150 ft increments"));
  assert(strstr(output, "ammo +1d6 fire damage"));
  /* Range-dependent feats report the actual target; no target is conditional. */
  feet = 31; show_ranged_attack_details(&ch, &victim, &bow, false);
  assert(!strstr(output, "Point Blank Shot"));
  feet = 151; show_ranged_attack_details(&ch, &victim, &bow, false);
  assert(strstr(output, "range -1 attack"));
  show_ranged_attack_details(&ch, NULL, &bow, false);
  assert(strstr(output, "within 30 ft (target required)"));
  /* Enhance Arrow adds to physical ammo enhancement on attack AND damage. */
  assert(attack_enhancement(&ch, &arrow, 2) == 7);
  assert(damage_enhancement(&ch, &arrow, &bow, 2) == 7);
  magic = 5;
  assert(attack_enhancement(&ch, &arrow, 5) == 9);
  assert(damage_enhancement(&ch, &arrow, &bow, 5) == 9);
  magic = 0;
  /* A distinct Manyshot arrow reports its own damage bonuses, sharing the roll. */
  shot.extra_projectile = &next;
  show_ranged_attack_details(&ch, &victim, &bow, true);
  assert(strstr(output, "MS-ammo:+7") && !strstr(output, "shares first roll"));
  show_ranged_attack_details(&ch, &victim, &bow, false);
  assert(strstr(output, "Manyshot extra ammo: next stack, enhancement +7 damage"));
  assert(strstr(output, "shares first roll"));
  shot.extra_projectile = NULL;
  magic = 5; show_ranged_attack_details(&ch, &victim, &bow, false);
  assert(strstr(output, "weapon/ammo enhancement +5 (highest equipment/spell only)"));
  /* Outside a shot, report the stack selected for the next attack. */
  current_shot = NULL; fire_ready = true;
  show_ranged_attack_details(&ch, &victim, &bow, false);
  assert(strstr(output, "next stack") && !strstr(output, "last flaming arrow"));
  GET_OBJ_VAL(&next, 0) = AMMO_TYPE_BOLT;
  show_ranged_attack_details(&ch, &victim, &bow, false);
  assert(!strstr(output, "Enhance Arrow"));
  output[0] = '\0'; fire_ready = false;
  show_ranged_attack_details(&ch, &victim, &bow, false);
  assert(!*output);
  puts("PASS: ranged bonus display, final/next/Manyshot ammunition, enhancement stacking, feats and range.");
}
'''
with tempfile.TemporaryDirectory(prefix='ranged-display-') as directory:
    cfile = Path(directory) / 'display.c'
    binary = Path(directory) / 'display'
    cfile.write_text(code)
    subprocess.run([os.environ.get('CC', 'cc'), '-std=c11', '-g',
                    '-fsanitize=address,undefined', '-fno-omit-frame-pointer',
                    '-I' + str(root / 'src'), str(cfile), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
