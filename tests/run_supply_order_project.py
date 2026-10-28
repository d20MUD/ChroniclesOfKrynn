#!/usr/bin/env python3
"""Check supply recipe conversion, start dispatch, and timed completion."""
from pathlib import Path
import os
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'src/crafting_new.c').read_text()
def function(signature):
    start = source.index(signature + '\n{')
    return source[start:source.index('\n}', start) + 2]
headers = ''.join(f'#include "{h}"\n' for h in
                  ['conf.h', 'sysdep.h', 'structs.h', 'utils.h', 'spells.h',
                   'comm.h', 'interpreter.h', 'crafting_new.h', 'crafting_recipes.h'])
code = '\n'.join(line for line in source.splitlines() if line.startswith(('#define NSUPPLY_ORDER_DURATION ', '#define CREATE_BASE_TIME '))) + '\n'
code += r'''
#include <assert.h>
struct player_special_data dummy_mob;
struct craft_recipe_data crafting_recipes[NUM_CRAFTING_RECIPES];
const char *ability_names[NUM_ABILITIES + 1];
static int awarded_skill, consumed, station_skill;
void basic_mud_log(const char *fmt, ...) { assert(0); }
size_t send_to_char(struct char_data *ch, const char *fmt, ...) { return 0; }
int dice(int n, int size) { return 1; }
int num_supply_order_requisitions_to_go(struct char_data *ch) {
  return GET_CRAFT(ch).supply_num_required - GET_NSUPPLY_NUM_MADE(ch);
}
bool player_has_supply_order(struct char_data *ch) {
  return GET_CRAFT(ch).crafting_method == SCMD_NEWCRAFT_SUPPLYORDER;
}
bool validate_supply_order_materials(struct char_data *ch) { return TRUE; }
bool has_crafting_station_in_room(struct char_data *ch, int skill) {
  station_skill = skill; return TRUE;
}
const char *get_crafting_station_name(int skill) { return "station"; }
bool consume_supply_order_materials(struct char_data *ch) { consumed++; return TRUE; }
void abandon_supply_order(struct char_data *ch) { assert(0); }
char *get_supply_order_item_desc(struct char_data *ch) { return "dagger"; }
int determine_supply_order_exp(struct char_data *ch) { return 50; }
int get_current_craft_project_recipe(struct char_data *ch) { return GET_CRAFT(ch).crafting_recipe; }
void gain_craft_exp(struct char_data *ch, int exp, int skill, bool verbose) {
  assert(exp == 50); awarded_skill = skill;
}
'''
for sig in ['int craft_recipe_by_type(int type)',
            'int recipe_skill_to_actual_crafting_skill(int recipe_skill)',
            'static void update_supply_order_project(struct char_data *ch)',
            'void start_supply_order(struct char_data *ch)',
            'void craft_supplyorder_complete(struct char_data *ch)']:
    code += function(sig)
# Compile the actual normal-craft entry point, stubbing only its unrelated normal-item dependencies.
code += r'''
bool is_craft_ready(struct char_data *ch, bool verbose) { assert(0); return FALSE; }
int get_rapid_talent_bonus(struct char_data *ch, int skill) { assert(0); return 0; }
static bool artificer_efficient_crafter_applies(struct char_data *ch) { assert(0); return FALSE; }
const char *act(const char *str, int hide, struct char_data *ch, struct obj_data *obj, void *vict, int type) { return str; }
'''
code += function('void begin_current_craft(struct char_data *ch)')
code += r'''
int main(void) {
  struct char_data ch = {0}; struct player_special_data ps = {0};
  ch.player_specials = &ps;
  int types[] = {ITEM_WEAPON, ITEM_ARMOR, ITEM_WORN, ITEM_INSTRUMENT};
  int skills[] = {CRAFT_SKILL_WEAPONSMITH, CRAFT_SKILL_ARMORSMITH, CRAFT_SKILL_JEWELER,
    CRAFT_SKILL_TINKER, CRAFT_SKILL_CARPENTER, CRAFT_SKILL_TAILOR, CRAFT_SKILL_BREWING};
  int abilities[] = {ABILITY_CRAFT_WEAPONSMITHING, ABILITY_CRAFT_ARMORSMITHING,
    ABILITY_CRAFT_JEWELCRAFTING, ABILITY_CRAFT_METALWORKING, ABILITY_CRAFT_WOODWORKING,
    ABILITY_CRAFT_TAILORING, ABILITY_CRAFT_ALCHEMY};
  for (int t = 0; t < 4; t++) for (int k = 0; k < 7; k++) {
    memset(&ps, 0, sizeof(ps));
    crafting_recipes[1].object_type = types[t];
    crafting_recipes[1].object_subtype = 12;
    crafting_recipes[1].variant_skill[0] = skills[k];
    crafting_recipes[1].variant_descriptions[0] = "dagger";
    GET_CRAFT((&ch)).crafting_recipe = 1;
    GET_CRAFT((&ch)).craft_variant = 0;
    GET_CRAFT((&ch)).crafting_method = SCMD_NEWCRAFT_SUPPLYORDER;
    GET_CRAFT((&ch)).supply_num_required = 2;
    /* Old saved orders contain the raw recipe enums. Start repairs these. */
    GET_CRAFT((&ch)).crafting_item_type = types[t];
    GET_CRAFT((&ch)).skill_type = skills[k];
    consumed = 0; awarded_skill = 0;
    begin_current_craft(&ch);
    assert(GET_CRAFT((&ch)).crafting_item_type == t + 1);
    assert(GET_CRAFT((&ch)).skill_type == abilities[k]);
    assert(station_skill == abilities[k]);
    assert(GET_CRAFT((&ch)).crafting_method == SCMD_NEWCRAFT_SUPPLYORDER);
    assert(GET_CRAFT((&ch)).craft_duration == NSUPPLY_ORDER_DURATION);
    /* A second start must not reset ongoing progress. */
    GET_CRAFT((&ch)).craft_duration = 1;
    begin_current_craft(&ch);
    assert(GET_CRAFT((&ch)).craft_duration == 1);
    GET_CRAFT((&ch)).craft_duration = 0;
    craft_supplyorder_complete(&ch);
    assert(GET_NSUPPLY_NUM_MADE((&ch)) == 1);
    assert(consumed == 1 && awarded_skill == abilities[k]);
    begin_current_craft(&ch);
    GET_CRAFT((&ch)).craft_duration = 0;
    craft_supplyorder_complete(&ch);
    assert(GET_NSUPPLY_NUM_MADE((&ch)) == 2 && consumed == 2);
    begin_current_craft(&ch);
    assert(GET_CRAFT((&ch)).craft_duration == 0);
  }
  return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='supply-order-') as tmp:
    path = Path(tmp) / 'check.c'
    path.write_text(headers + code)
    binary = Path(tmp) / 'check'
    subprocess.run([os.environ.get('CC', 'cc'), '-std=gnu99', '-Wno-unused-parameter',
                    '-I' + str(root / 'src'), str(path), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print('Supply orders: four item types, seven skills, saved-order repair, craft-start dispatch, progress, XP, and completion passed.')
