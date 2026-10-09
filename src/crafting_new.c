// New LuminariMUD Crafting System by Steve Squires aka Gicker


#include "conf.h"
#include "sysdep.h"
#include <time.h>
#include "structs.h"
#include "bool.h"
#include "mysql.h"
#include "utils.h"
#include "comm.h"
#include "spells.h"
#include "interpreter.h"
#include "constants.h"
#include "handler.h"
#include "db.h"
#include "craft.h"
#include "spells.h"
#include "mud_event.h"
#include "modify.h" // for parse_at()
#include "treasure.h"
#include "mudlim.h"
#include "spec_procs.h" /* For GET_ABILITY() */
#include "act.h"
#include "fight.h"
#include "item.h"
#include "quest.h"
#include "assign_wpn_armor.h"
#include "genolc.h"
#include "crafting_new.h"
#include "oasis.h"
#include "feats.h"
#include "class.h"
#include "improved-edit.h"
#include "talents.h" /* crafting talent system */
#include "dg_scripts.h"
#include "resource_system.h"
#include "domains_schools.h"
#include "brew.h"
#include "perks.h"

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#include "vnums.h"
#include "crafting_recipes.h"

#define GOLEM_RECALL_COOLDOWN 300  // 5 minutes in seconds
#define GOLEM_BATTLEFIELD_RETRIEVAL_COOLDOWN 30
#define GOLEM_SIEGE_FRAME_SLAM_COOLDOWN 30
#define GOLEM_OMNI_FORGE_COMMANDER_COOLDOWN (60 * 60)
#define DISENCHANT_MIN_LEVEL 5

ACMD_DECL(do_practice);

int copy_object(struct obj_data *to, struct obj_data *from);
void process_craft_critical_success(struct char_data *ch, struct obj_data *obj);
int get_rapid_talent_bonus(struct char_data *ch, int skill);
int get_insightful_talent_bonus(struct char_data *ch, int skill);
int get_efficient_talent_bonus(struct char_data *ch, int skill);
void return_efficient_saved_materials(struct char_data *ch);
void save_char_pets(struct char_data *ch);
bool can_recall_golem(struct char_data *ch);
static bool craft_project_has_catalyst_target(struct char_data *ch);
static bool craft_has_pending_noncreate_project(struct char_data *ch);
static bool obj_has_catalyst_target(struct obj_data *obj);
static bool is_catalyst_affect_eligible(const struct obj_affected_type *affect);
static int catalyst_modifier_increase(int location);
static int disenchant_fragment_yield(struct obj_data *obj);
static bool can_disenchant_obj(struct obj_data *obj);

int materials_sort_info[NUM_CRAFT_MATS];

#define NEWCRAFT_RESIZE_SYNTAX                                                                     \
  "Syntax is as follows: resize (object-name|show|check|begin|start|reset) (new-size)\r\n"

#define CRAFT_MOTE_NOARG                                                                           \
  "Please specity add or remove, and the bonus slot.\r\n"                                          \
  "Eg. craft motes add 1\r\n"                                                                      \
  "-- This will add the required type and number of motes to bonus slot 1.\r\n"

#define NEWCRAFT_CREATE_NOARG1                                                                     \
  "See HELP CRAFTING for more information on how to craft new items.\r\n"                          \
  "Options are:\r\n"                                                                               \
  "craft tutorial\r\n"                                                                             \
  "craft itemtype (weapon|armor|jewelry|instrument|misc)\r\n"                                      \
  "craft specifictype (type)\r\n"                                                                  \
  "craft variant (variant name)\r\n"                                                               \
  "craft keywords (keyword string)\r\n"                                                            \
  "craft shortdesc (short desc string)\r\n"                                                        \
  "craft roomdesc (room desc string)\r\n"                                                          \
  "craft extradesc (extra desc string\r\n"                                                         \
  "craft bonuses (slot) (bonus location) (bonus type) (modifier) [specific]\r\n"                   \
  "craft enhancement (enhancement modifier\r\n"                                                    \
  "craft instrument (quality|effectiveness|breakability) (amount)\r\n"                             \
  "craft materials (add|remove) (material type)\r\n"                                               \
  "craft motes (add|remove) (enhancement|quality|effectiveness|breakability|bonus slot #)\r\n"     \
  "craft catalysts (0-5|add #|remove [#])\r\n"                                                     \
  "craft leveladjust (level adjustment)\r\n"                                                       \
  "craft score\r\n"                                                                                \
  "craft specialize (skill name) - Choose up to 2 skills for +5 bonus and 2x exp\r\n"             \
  "craft show\r\n"                                                                                 \
  "craft check\r\n"                                                                                \
  "craft reset (no "                                                                               \
  "argument|motes|materials|enhancement|instrument|bonuses|descriptions|refine|resize|catalysts)\r\n" \
  "craft start\r\n"                                                                                \
  "\r\n"                                                                                           \
  "Other commands:\r\n"                                                                            \
  "disenchant <item> - Convert a magical item into catalyst fragments\r\n"                         \
  "craft equipment - Show your equipped crafting gear\r\n"                                         \
  "craft tools - Show your equipped crafting/harvesting tools\r\n"

#define CRAFT_TUTORIAL_FILE "docs/CRAFTING_TUTORIAL.md"
#define CRAFT_TUTORIAL_FILE_ALT "../docs/CRAFTING_TUTORIAL.md"
#define CRAFT_TUTORIAL_MAX_WIDTH 90

static void append_tutorial_output(char **buf, size_t *buf_size, size_t *buf_len, const char *text)
{
  size_t add_len = strlen(text);
  size_t needed = *buf_len + add_len + 1;

  if (needed > *buf_size)
  {
    size_t new_size = (*buf_size == 0) ? 4096 : *buf_size;
    while (new_size < needed)
      new_size *= 2;
    *buf = realloc(*buf, new_size);
    *buf_size = new_size;
  }

  memcpy(*buf + *buf_len, text, add_len);
  *buf_len += add_len;
  (*buf)[*buf_len] = '\0';
}

static void append_wrapped_tutorial_line(char **buf, size_t *buf_size, size_t *buf_len,
                                         const char *line, int width)
{
  char current[MAX_STRING_LENGTH];
  char prefix_first[16];
  char prefix_next[16];
  char word[256];
  const char *p = line;
  int prefix_len = 0;
  int line_len = 0;
  bool first_word = true;

  prefix_first[0] = '\0';
  prefix_next[0] = '\0';

  if (!p || !*p)
  {
    append_tutorial_output(buf, buf_size, buf_len, "\r\n");
    return;
  }

  if (strncmp(p, "- ", 2) == 0)
  {
    strcpy(prefix_first, "- ");
    strcpy(prefix_next, "  ");
    p += 2;
  }
  else
  {
    while (*p == ' ' && prefix_len < (int)sizeof(prefix_first) - 1)
    {
      prefix_first[prefix_len++] = ' ';
      p++;
    }
    prefix_first[prefix_len] = '\0';
    strcpy(prefix_next, prefix_first);
  }

  while (*p == ' ')
    p++;

  snprintf(current, sizeof(current), "%s", prefix_first);
  line_len = strlen(current);

  while (*p)
  {
    int wlen = 0;

    while (*p == ' ')
      p++;
    if (!*p)
      break;

    while (*p && *p != ' ' && wlen < (int)sizeof(word) - 1)
      word[wlen++] = *p++;
    word[wlen] = '\0';

    if (!first_word && (line_len + 1 + wlen) > width)
    {
      append_tutorial_output(buf, buf_size, buf_len, current);
      append_tutorial_output(buf, buf_size, buf_len, "\r\n");
      snprintf(current, sizeof(current), "%s", prefix_next);
      line_len = strlen(current);
      first_word = true;
    }

    if (!first_word)
    {
      current[line_len++] = ' ';
      current[line_len] = '\0';
    }

    if (line_len + wlen >= (int)sizeof(current))
      wlen = (int)sizeof(current) - line_len - 1;

    memcpy(current + line_len, word, wlen);
    line_len += wlen;
    current[line_len] = '\0';
    first_word = false;
  }

  append_tutorial_output(buf, buf_size, buf_len, current);
  append_tutorial_output(buf, buf_size, buf_len, "\r\n");
}

static void show_craft_tutorial(struct char_data *ch)
{
  FILE *fp = NULL;
  char line[MAX_STRING_LENGTH];
  char cleaned[MAX_STRING_LENGTH];
  char *out = NULL;
  size_t out_size = 0;
  size_t out_len = 0;
  bool in_code_block = false;

  if (!ch)
    return;

  fp = fopen(CRAFT_TUTORIAL_FILE, "r");
  if (!fp)
    fp = fopen(CRAFT_TUTORIAL_FILE_ALT, "r");

  if (!fp)
  {
    send_to_char(ch, "Crafting tutorial file not found.\r\n");
    return;
  }

  while (fgets(line, sizeof(line), fp))
  {
    size_t len = strlen(line);
    int header_level = 0;
    while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
      line[--len] = '\0';

    if (strncmp(line, "```", 3) == 0)
    {
      in_code_block = !in_code_block;
      continue;
    }

    if (!*line)
    {
      append_tutorial_output(&out, &out_size, &out_len, "\r\n");
      continue;
    }

    if (in_code_block)
    {
      append_wrapped_tutorial_line(&out, &out_size, &out_len, line, CRAFT_TUTORIAL_MAX_WIDTH);
      continue;
    }

    {
      const char *p = line;
      int i = 0;

      if (*p == '#')
      {
        while (*p == '#')
        {
          header_level++;
          p++;
        }
        while (*p == ' ')
          p++;
      }

      while (*p && i < (int)sizeof(cleaned) - 1)
      {
        if (*p != '`')
          cleaned[i++] = *p;
        p++;
      }
      cleaned[i] = '\0';
    }

    if (header_level > 0)
    {
      append_tutorial_output(&out, &out_size, &out_len,
                             (header_level <= 2) ? "\tY" : "\tC");
      append_tutorial_output(&out, &out_size, &out_len, cleaned);
      append_tutorial_output(&out, &out_size, &out_len, "\tn\r\n");
      continue;
    }

    if (*cleaned)
    {
      size_t clen = strlen(cleaned);
      if (clen > 0 && cleaned[clen - 1] == ':')
      {
        append_tutorial_output(&out, &out_size, &out_len, "\tC");
        append_tutorial_output(&out, &out_size, &out_len, cleaned);
        append_tutorial_output(&out, &out_size, &out_len, "\tn\r\n");
        continue;
      }
    }

    append_wrapped_tutorial_line(&out, &out_size, &out_len, cleaned, CRAFT_TUTORIAL_MAX_WIDTH);
  }

  fclose(fp);

  if (!out || !*out)
  {
    send_to_char(ch, "Crafting tutorial is empty.\r\n");
    free(out);
    return;
  }

  if (ch->desc)
    page_string(ch->desc, out, TRUE);
  else
    send_to_char(ch, "%s", out);

  free(out);
}

#define NEWCRAFT_CREATE_TYPES                                                                      \
  ("What item type do you wish to make?\r\n"                                                       \
   "-- armor       includes shields\r\n"                                                           \
   "-- weapons     all types\r\n"                                                                  \
   "-- instruments lyres, drums, etc. for bards\r\n"                                               \
   "-- misc        rings, necklaces, earrings, cloaks, belts, etc.\r\n")
#define NEWCRAFT_CREATE_BONUSES_NOARG                                                              \
  "You need to specify all of the bonus information:\r\n"                                          \
  "Usage: craft bonuses (slot) (bonus location) (bonus type) (modifier) [specific]\r\n"               \
  "-- slot needs to be 1-6 as an item can only have 6 bonuses total\r\n"                           \
  "-- bonus location is what the bonus affects. Eg. strength, hit-points, reflex-save, etc. Type " \
  "'applies' for a list.\r\n"                                                                      \
  "-- bonus type is either enhancement or universal. See HELP CRAFTING for more info\r\n"          \
  "-- modifier is how much the bonus will affect that associated stat\r\n"                         \
  "-- specific is only required for skill and spell slot bonus locations\r\n"                   \
  "-- grant-feat crafting is not supported\r\n"                                                  \
  "Eg. craft bonuses 1 armor-class universal 1\r\n"                                               \
  "\r\n"                                                                                           \
  "To reset a bonus slot type: craft bonus [slot] reset\r\n"

#define SUPPLY_ORDER_NOARG1                                                                        \
  "Please specify what supply order action you'd like to take.\r\n"                                \
  "supplyorder request    : Request a random supply order project.\r\n"                            \
  "supplyorder show       : Show information on current supply order project.\r\n"                 \
  "supplyorder start      : Begin working on your supply order.\r\n"                               \
  "supplyorder material   : Add/remove materials (add [material] or remove).\r\n"                  \
  "supplyorder complete   : Turn in a completed supply order for reward.\r\n"                      \
  "supplyorder reset      : Reset current supply order project.\r\n"                               \
  "supplyorder abandon    : Cancel existing supply order project entirely.\r\n"                    \
  "supplyorder cooldown   : Display timing information and cooldowns.\r\n"                         \
  "\r\n"

#define HARVEST_NODE_PERCENT_CHANCE 33
#define HARVEST_BASE_TIME 20
#define CREATE_BASE_TIME 60
#define SURVEY_BASE_TIME 3
#define HARVEST_BASE_DC 5
#define CREATE_BASE_DC 10
#define HARVEST_MOTE_DICE_SIZE 4
#define HARVEST_MOTE_CHANCE 20
#define HARVEST_BASE_AMOUNT dice(2, 2)
#define HARVEST_BASE_EXP 20
#define CREATE_BASE_EXP 50
#define REFINE_BASE_EXP 10
#define NSUPPLY_ORDER_DURATION 60
#define NSUPPLY_ORDER_NUM_REQUIRED 5
#define NSUPPLY_ORDER_BASE_EXP 50
#define HARVEST_MOTE_MULTIPLIER 150 // Motes gathered are multiplied by this value / 100

// Supply order enhancement constants
#define SUPPLY_BASE_REWARD 100
#define SUPPLY_MATERIAL_BONUS_MULTIPLIER 25
#define SUPPLY_SKILL_BONUS_MULTIPLIER 2
#define SUPPLY_QUANTITY_BONUS 10
#define SUPPLY_RUSH_BONUS_MULTIPLIER 150    // 50% bonus for rush orders
#define SUPPLY_BULK_BONUS_MULTIPLIER 75     // -25% per item but more total
#define SUPPLY_QUALITY_BONUS_MULTIPLIER 200 // 100% bonus for quality orders

#define CRAFT_MOTES_REQ_30 80
#define CRAFT_MOTES_REQ_25 60
#define CRAFT_MOTES_REQ_20 40
#define CRAFT_MOTES_REQ_15 25
#define CRAFT_MOTES_REQ_10 15
#define CRAFT_MOTES_REQ_5 8
#define CRAFT_MOTES_REQ_1 3

// Contract generation functions - structure defined in crafting_new.h
struct supply_contract *generate_available_contracts(struct char_data *ch, int *num_contracts);
void free_contract_list(struct supply_contract *contracts, int num_contracts);
int select_contract_by_id(struct char_data *ch, int contract_id);
int reject_contract_by_id(struct char_data *ch, int contract_id);
void update_supply_slots_for_all_players(void);


void assign_harvest_materials_to_word(void)
{
  ssize_t cnt = 0;

  for (cnt = 0; cnt <= top_of_world; cnt++)
  {
    if (is_someone_harvesting_room(cnt))
      continue;
    // erase all harvest materials
    wipe_room_harvest_materials(cnt);
    // check valid sector type
    if (!is_valid_harvesting_sector(world[cnt].sector_type))
      continue;
    // check random chance
    if (!will_room_have_harvest_materials(cnt))
      continue;
    // assign materials
    assign_harvest_materials_to_room(cnt);
  }
}

void assign_harvest_materials_to_room(room_rnum room)
{
  if (room == NOWHERE)
    return;

  int material_type = determine_harvest_material_for_room(room);

  if (material_type != CRAFT_MAT_NONE)
  {
    world[room].harvest_material = material_type;
    world[room].harvest_material_amount = determine_number_of_harvest_units_for_room();
  }
}

int determine_harvest_material_for_room(room_rnum room)
{
  if (room == NOWHERE)
    return 0;

  int zone_low = zone_table[world[room].zone].min_level;
  int zone_high = zone_table[world[room].zone].max_level;
  int zone_level = MIN(30, MAX(1, ((zone_high - zone_low) / 2) + zone_low));
  int grade = determine_grade_by_zone_level(zone_level);
  int group = determine_random_material_group_by_sector_type(world[room].sector_type);

  grade = determine_random_grade(grade);

  // small chance for the grade level to be bumped up by one
  if (dice(1, 20) == 1)
  {
    grade++;
  }

  grade = MIN(5, MAX(1, grade));

  int material = determine_material_type_by_group_and_grade(group, grade);

  return material;
}

int determine_material_type_by_group_and_grade(int group, int grade)
{
  switch (group)
  {
  case CRAFT_GROUP_HARD_METALS:
    if (dice(1, 4) == 1)
      return CRAFT_MAT_STONE;
    switch (grade)
    {
    case 1:
    case 2:
      return dice(1, 3) == 1 ? CRAFT_MAT_ZINC : CRAFT_MAT_TIN;
    case 3:
    case 4:
      return dice(1, 3) == 1 ? CRAFT_MAT_COAL : CRAFT_MAT_IRON;
    case 5:
      return dice(1, 2) == 1 ? CRAFT_MAT_MITHRIL : CRAFT_MAT_ADAMANTINE;
    }
    break;
  case CRAFT_GROUP_SOFT_METALS:
    if (dice(1, 4) == 1)
      return CRAFT_MAT_STONE;
    switch (grade)
    {
    case 1:
    case 2:
      return CRAFT_MAT_COPPER;
    case 3:
      return CRAFT_MAT_SILVER;
    case 4:
      return CRAFT_MAT_GOLD;
    case 5:
      return CRAFT_MAT_PLATINUM;
    }
    break;
  case CRAFT_GROUP_WOOD:
    switch (grade)
    {
    case 1:
      return CRAFT_MAT_ASH_WOOD;
    case 2:
      return CRAFT_MAT_MAPLE_WOOD;
    case 3:
      return CRAFT_MAT_MAHAGONY_WOOD;
    case 4:
      return CRAFT_MAT_VALENWOOD;
    case 5:
      return CRAFT_MAT_IRONWOOD;
    }
    break;
  case CRAFT_GROUP_HIDES:
    switch (grade)
    {
    case 1:
      return CRAFT_MAT_LOW_GRADE_HIDE;
    case 2:
      return CRAFT_MAT_MEDIUM_GRADE_HIDE;
    case 3:
      return CRAFT_MAT_HIGH_GRADE_HIDE;
    case 4:
    case 5:
      return CRAFT_MAT_PRISTINE_GRADE_HIDE;
    }
    break;
  case CRAFT_GROUP_CLOTH:
    switch (grade)
    {
    case 1:
      return CRAFT_MAT_HEMP;
    case 2:
      return CRAFT_MAT_FLAX;
    case 3:
      return CRAFT_MAT_WOOL;
    case 4:
      return CRAFT_MAT_COTTON;
    case 5:
      return CRAFT_MAT_SILK;
    }
    break;
  }
  return CRAFT_MAT_NONE;
}

int craft_material_level_adjustment(int material)
{
  switch (material)
  {
  case CRAFT_MAT_TIN:
    return -2;

  case CRAFT_MAT_BRONZE:
  case CRAFT_MAT_COPPER:
  case CRAFT_MAT_LOW_GRADE_HIDE:
  case CRAFT_MAT_ASH_WOOD:
  case CRAFT_MAT_HEMP:
  case CRAFT_MAT_STONE:
  case CRAFT_MAT_COAL:
    return 0;

  case CRAFT_MAT_IRON:
  case CRAFT_MAT_BRASS:
  case CRAFT_MAT_MEDIUM_GRADE_HIDE:
  case CRAFT_MAT_MAPLE_WOOD:
  case CRAFT_MAT_WOOL:
    return 2;

  case CRAFT_MAT_STEEL:
  case CRAFT_MAT_SILVER:
  case CRAFT_MAT_MAHAGONY_WOOD:
  case CRAFT_MAT_LINEN:
    return 4;

  case CRAFT_MAT_COLD_IRON:
  case CRAFT_MAT_ALCHEMAL_SILVER:
  case CRAFT_MAT_GOLD:
  case CRAFT_MAT_HIGH_GRADE_HIDE:
  case CRAFT_MAT_VALENWOOD:
  case CRAFT_MAT_COTTON:
    return 5;

  case CRAFT_MAT_MITHRIL:
  case CRAFT_MAT_SILK:
    return 7;

  case CRAFT_MAT_ADAMANTINE:
  case CRAFT_MAT_PLATINUM:
  case CRAFT_MAT_PRISTINE_GRADE_HIDE:
  case CRAFT_MAT_IRONWOOD:
  case CRAFT_MAT_SATIN:
    return 8;

  case CRAFT_MAT_DRAGONMETAL:
  case CRAFT_MAT_DRAGONSCALE:
  case CRAFT_MAT_DRAGONBONE:
    return 10;
  }
  return 0;
}

int harvesting_skill_by_material(int material)
{
  switch (material)
  {
  case CRAFT_MAT_TIN:
  case CRAFT_MAT_BRONZE:
  case CRAFT_MAT_IRON:
  case CRAFT_MAT_STEEL:
  case CRAFT_MAT_COLD_IRON:
  case CRAFT_MAT_ALCHEMAL_SILVER:
  case CRAFT_MAT_MITHRIL:
  case CRAFT_MAT_ADAMANTINE:
  case CRAFT_MAT_DRAGONMETAL:
  case CRAFT_MAT_COPPER:
  case CRAFT_MAT_SILVER:
  case CRAFT_MAT_GOLD:
  case CRAFT_MAT_PLATINUM:
  case CRAFT_MAT_COAL:
  case CRAFT_MAT_ZINC:
  case CRAFT_MAT_STONE:
    return ABILITY_HARVEST_MINING;

  case CRAFT_MAT_LOW_GRADE_HIDE:
  case CRAFT_MAT_MEDIUM_GRADE_HIDE:
  case CRAFT_MAT_HIGH_GRADE_HIDE:
  case CRAFT_MAT_PRISTINE_GRADE_HIDE:
    return ABILITY_HARVEST_HUNTING;

  case CRAFT_MAT_ASH_WOOD:
  case CRAFT_MAT_MAPLE_WOOD:
  case CRAFT_MAT_MAHAGONY_WOOD:
  case CRAFT_MAT_VALENWOOD:
  case CRAFT_MAT_IRONWOOD:
    return ABILITY_HARVEST_FORESTRY;

  case CRAFT_MAT_HEMP:
  case CRAFT_MAT_WOOL:
  case CRAFT_MAT_LINEN:
  case CRAFT_MAT_FLAX:
  case CRAFT_MAT_SATIN:
  case CRAFT_MAT_COTTON:
  case CRAFT_MAT_SILK:
    return ABILITY_HARVEST_GATHERING;

  case CRAFT_MAT_DRAGONSCALE:
  case CRAFT_MAT_DRAGONBONE:
  case CRAFT_MAT_DRAGONBLOOD:
    return ABILITY_HARVEST_BUTCHERING;
  }
  return 0;
}

int determine_random_material_group_by_sector_type(room_rnum sector)
{
  int chance = dice(1, 100);

  switch (sector)
  {
  case SECT_FIELD:
    if (chance <= 75)
      return CRAFT_GROUP_CLOTH;
    else
      return CRAFT_GROUP_HIDES;

  case SECT_FOREST:
  case SECT_TAIGA:
    if (chance <= 75)
      return CRAFT_GROUP_WOOD;
    else
      return CRAFT_GROUP_HIDES;

  case SECT_HILLS:
  case SECT_UD_WILD:
    if (chance <= 25)
      return CRAFT_GROUP_CLOTH;
    else if (chance <= 50)
      return CRAFT_GROUP_HIDES;
    else if (chance <= 75)
      return CRAFT_GROUP_HARD_METALS;
    else
      return CRAFT_GROUP_SOFT_METALS;

  case SECT_MOUNTAIN:
    if (chance <= 20)
      return CRAFT_GROUP_HIDES;
    else if (chance <= 60)
      return CRAFT_GROUP_HARD_METALS;
    else
      return CRAFT_GROUP_SOFT_METALS;

  case SECT_HIGH_MOUNTAIN:
    if (chance <= 50)
      return CRAFT_GROUP_HARD_METALS;
    else
      return CRAFT_GROUP_SOFT_METALS;

  case SECT_DESERT:
    if (chance <= 25)
      return CRAFT_GROUP_CLOTH;
    else
      return CRAFT_GROUP_HIDES;

  case SECT_MARSHLAND:
    if (chance <= 25)
      return CRAFT_GROUP_CLOTH;
    else if (chance <= 50)
      return CRAFT_GROUP_WOOD;
    else
      return CRAFT_GROUP_HIDES;

  case SECT_CAVE:
    if (chance <= 50)
      return CRAFT_GROUP_HARD_METALS;
    else
      return CRAFT_GROUP_SOFT_METALS;

  case SECT_JUNGLE:
    if (chance <= 25)
      return CRAFT_GROUP_CLOTH;
    else if (chance <= 50)
      return CRAFT_GROUP_HIDES;
    else
      return CRAFT_GROUP_WOOD;

  case SECT_TUNDRA:
    if (chance <= 60)
      return CRAFT_GROUP_HIDES;
    else if (chance <= 80)
      return CRAFT_GROUP_HARD_METALS;
    else
      return CRAFT_GROUP_SOFT_METALS;

    // case SECT_BEACH:
    // case SECT_RIVER:
    // case SECT_UD_WATER:
    // case SECT_OCEAN:
    // case SECT_WATER_SWIM:
    // case SECT_WATER_NOSWIM:
    // case SECT_UNDERWATER:
  }

  return CRAFT_GROUP_NONE;
}

int determine_random_grade(int grade)
{
  int chance = dice(1, 100);

  switch (grade)
  {
  case 1:
    return 1;
  case 2:
    if (chance <= 50)
      return 1;
    else
      return 2;
  case 3:
    if (chance <= 33)
      return 1;
    else if (chance <= 66)
      return 2;
    else
      return 3;
  case 4:
    if (chance <= 25)
      return 1;
    else if (chance <= 50)
      return 2;
    else if (chance <= 75)
      return 3;
    else
      return 4;
  case 5:
    if (chance <= 15)
      return 1;
    else if (chance <= 30)
      return 2;
    else if (chance <= 50)
      return 3;
    else if (chance <= 75)
      return 4;
    else
      return 5;
  }
  return 1;
}

int determine_grade_by_zone_level(int zone_level)
{
  if (zone_level <= 5)
    return 1;
  else if (zone_level <= 10)
    return 2;
  else if (zone_level <= 15)
    return 3;
  else if (zone_level <= 20)
    return 4;
  else
    return 5;
}

int determine_number_of_harvest_units_for_room(void)
{
  return dice(2, 6);
}

bool will_room_have_harvest_materials(room_rnum room)
{
  if (room == NOWHERE)
    return FALSE;

  if (ROOM_FLAGGED(room, ROOM_HARVEST_NODE))
    return TRUE;

  int chance = HARVEST_NODE_PERCENT_CHANCE;

  if (dice(1, 100) <= chance)
    return TRUE;

  return FALSE;
}

bool is_valid_harvesting_sector(int sector)
{
  switch (sector)
  {
  case SECT_FIELD:
  case SECT_FOREST:
  case SECT_HILLS:
  case SECT_MOUNTAIN:
  case SECT_DESERT:
  case SECT_MARSHLAND:
  case SECT_HIGH_MOUNTAIN:
  case SECT_UD_WILD:
  case SECT_CAVE:
  case SECT_JUNGLE:
  case SECT_TUNDRA:
  case SECT_TAIGA:

    // case SECT_UD_WATER:
    // case SECT_BEACH:
    // case SECT_RIVER:
    // case SECT_OCEAN:
    // case SECT_WATER_SWIM:
    // case SECT_WATER_NOSWIM:
    // case SECT_UNDERWATER:
    return TRUE;
  }
  return FALSE;
}

bool room_has_harvest_materials(room_rnum room)
{
  if (room == NOWHERE)
    return FALSE;

  int i = 0;

  for (i = 1; i < NUM_CRAFT_MATS; i++)
    if (world[room].harvest_material > 0)
      return TRUE;

  return FALSE;
}

bool is_someone_harvesting_room(room_rnum room)
{
  struct char_data *ch = NULL;

  if (room == NOWHERE)
    return FALSE;

  for (ch = world[room].people; ch; ch = ch->next_in_room)
  {
    if (GET_CRAFT(ch).crafting_method == SCMD_NEWCRAFT_HARVEST && GET_CRAFT(ch).craft_duration > 0)
      return TRUE;
  }

  return FALSE;
}

void wipe_room_harvest_materials(room_rnum room)
{
  if (room == NOWHERE)
    return;

  int i = 0;

  for (i = 1; i < NUM_CRAFT_MATS; i++)
    world[room].harvest_material = 0;
}

int material_grade(int material)
{
  switch (material)
  {
  case CRAFT_MAT_COPPER:
  case CRAFT_MAT_TIN:
  case CRAFT_MAT_LOW_GRADE_HIDE:
  case CRAFT_MAT_ASH_WOOD:
  case CRAFT_MAT_HEMP:
  case CRAFT_MAT_ZINC:
  case CRAFT_MAT_STONE:
    return 1;

  case CRAFT_MAT_BRONZE:
  case CRAFT_MAT_MEDIUM_GRADE_HIDE:
  case CRAFT_MAT_MAPLE_WOOD:
  case CRAFT_MAT_LINEN:
  case CRAFT_MAT_FLAX:
    return 2;

  case CRAFT_MAT_IRON:
  case CRAFT_MAT_COAL:
  case CRAFT_MAT_SILVER:
  case CRAFT_MAT_HIGH_GRADE_HIDE:
  case CRAFT_MAT_MAHAGONY_WOOD:
  case CRAFT_MAT_WOOL:
    return 3;

  case CRAFT_MAT_STEEL:
  case CRAFT_MAT_COLD_IRON:
  case CRAFT_MAT_ALCHEMAL_SILVER:
  case CRAFT_MAT_GOLD:
  case CRAFT_MAT_PRISTINE_GRADE_HIDE:
  case CRAFT_MAT_VALENWOOD:
  case CRAFT_MAT_SILK:
    return 4;

  case CRAFT_MAT_MITHRIL:
  case CRAFT_MAT_PLATINUM:
  case CRAFT_MAT_ADAMANTINE:
  case CRAFT_MAT_DRAGONSCALE:
  case CRAFT_MAT_DRAGONBONE:
  case CRAFT_MAT_IRONWOOD:
  case CRAFT_MAT_SATIN:
    return 5;

  case CRAFT_MAT_DRAGONMETAL:
    return 6;
  }
  return 0;
}

int craft_group_by_material(int material)
{
  switch (material)
  {
  case CRAFT_MAT_TIN:
  case CRAFT_MAT_BRONZE:
  case CRAFT_MAT_IRON:
  case CRAFT_MAT_STEEL:
  case CRAFT_MAT_COLD_IRON:
  case CRAFT_MAT_ALCHEMAL_SILVER:
  case CRAFT_MAT_MITHRIL:
  case CRAFT_MAT_ADAMANTINE:
  case CRAFT_MAT_DRAGONMETAL:
  case CRAFT_MAT_ZINC:
    return CRAFT_GROUP_HARD_METALS;

  case CRAFT_MAT_COPPER:
  case CRAFT_MAT_BRASS:
  case CRAFT_MAT_SILVER:
  case CRAFT_MAT_GOLD:
  case CRAFT_MAT_PLATINUM:
    return CRAFT_GROUP_SOFT_METALS;

  case CRAFT_MAT_LOW_GRADE_HIDE:
  case CRAFT_MAT_MEDIUM_GRADE_HIDE:
  case CRAFT_MAT_HIGH_GRADE_HIDE:
  case CRAFT_MAT_PRISTINE_GRADE_HIDE:
  case CRAFT_MAT_DRAGONSCALE:
    return CRAFT_GROUP_HIDES;

  case CRAFT_MAT_ASH_WOOD:
  case CRAFT_MAT_MAPLE_WOOD:
  case CRAFT_MAT_MAHAGONY_WOOD:
  case CRAFT_MAT_VALENWOOD:
  case CRAFT_MAT_IRONWOOD:
  case CRAFT_MAT_DRAGONBONE:
    return CRAFT_GROUP_WOOD;

  case CRAFT_MAT_HEMP:
  case CRAFT_MAT_WOOL:
  case CRAFT_MAT_LINEN:
  case CRAFT_MAT_FLAX:
  case CRAFT_MAT_COTTON:
  case CRAFT_MAT_SATIN:
  case CRAFT_MAT_SILK:
    return CRAFT_GROUP_CLOTH;

  case CRAFT_MAT_STONE:
    return CRAFT_GROUP_STONE;

  case CRAFT_MAT_COAL:
  case CRAFT_MAT_DRAGONBLOOD:
  case CRAFT_MAT_CATALYST_FRAGMENT:
  case CRAFT_MAT_CATALYST:
    return CRAFT_GROUP_REFINING;
  }
  return CRAFT_GROUP_NONE;
}

/* Check if a corpse can be butchered */
static bool can_butcher_corpse(struct obj_data *corpse)
{
  if (!IS_CORPSE(corpse))
    return false;

  /* Check if PC corpse (can't butcher PC corpses) */
  if (GET_OBJ_VAL(corpse, 4) != 0)
    return false;

  /* Check if player charmed creature (can't butcher player pets) */
  if (GET_OBJ_VAL(corpse, 7) != 0)
    return false;

  /* Check if already butchered */
  if (OBJ_FLAGGED(corpse, ITEM_BUTCHERED))
    return false;

  /* Check if the corpse race type is butcherable */
  int race_type = GET_OBJ_VAL(corpse, 1);
  if (race_type == RACE_TYPE_DRAGON || race_type == RACE_TYPE_ANIMAL || race_type == RACE_TYPE_MAGICAL_BEAST)
    return true;

  return false;
}

/* Get the level from corpse */
static int get_corpse_level(struct obj_data *corpse)
{
  /* Level is stored in val[6] */
  int level = GET_OBJ_VAL(corpse, 6);
  
  /* Sanity check - if level is 0 or invalid, return a default */
  if (level <= 0)
    return 10;
  
  return level;
}

/* Get race type from corpse */
static int get_corpse_race_type(struct obj_data *corpse)
{
  return GET_OBJ_VAL(corpse, 1);
}

/* Determine hide quality based on level */
static int get_hide_quality_by_level(int level)
{
  if (level <= 5)
    return CRAFT_MAT_LOW_GRADE_HIDE;
  else if (level <= 10)
    return CRAFT_MAT_MEDIUM_GRADE_HIDE;
  else if (level <= 20)
    return CRAFT_MAT_HIGH_GRADE_HIDE;
  else
    return CRAFT_MAT_PRISTINE_GRADE_HIDE;
}

/* Determine material based on corpse race and level */
static int determine_butcher_material(struct obj_data *corpse)
{
  int race_type = get_corpse_race_type(corpse);
  int level = get_corpse_level(corpse);
  
  if (race_type == RACE_TYPE_DRAGON)
  {
    switch(dice(1, 3))
    {
    case 1:
      return CRAFT_MAT_DRAGONBLOOD;
    case 2:
      return CRAFT_MAT_DRAGONBONE;
    case 3:
      return CRAFT_MAT_DRAGONSCALE;
    }
  }
  else if (race_type == RACE_TYPE_ANIMAL || race_type == RACE_TYPE_MAGICAL_BEAST)
  {
    /* Return hide based on level */
    return get_hide_quality_by_level(level);
  }
  
  return CRAFT_MAT_NONE;
}

/* Find any butcherable corpse in the room */
static struct obj_data *find_butcherable_corpse_in_room(struct char_data *ch)
{
  struct obj_data *obj;

  for (obj = world[IN_ROOM(ch)].contents; obj; obj = obj->next_content)
  {
    if (can_butcher_corpse(obj))
      return obj;
  }
  return NULL;
}

void survey_complete(struct char_data *ch)
{
  int roll = 0, talent_bonus = 0, skill_bonus = 0;
  ch->player_specials->surveyed_room = TRUE;
  if (world[IN_ROOM(ch)].harvest_material != CRAFT_MAT_NONE &&
      world[IN_ROOM(ch)].harvest_material_amount > 0)
  {
    if (GET_LEVEL(ch) >= LVL_IMMORT)
    {
      send_to_char(ch, "There are %d units of %s to be harvested here.\r\n",
                   world[IN_ROOM(ch)].harvest_material_amount,
                   crafting_materials[world[IN_ROOM(ch)].harvest_material]);
      send_to_char(ch, "Players will see:\r\n");
    }
    send_to_char(ch, "You find %s here.\r\n",
                 crafting_material_nodes[world[IN_ROOM(ch)].harvest_material]);
  }
  else
  {
    send_to_char(ch, "There is nothing here to harvest.\r\n");
  }
  GET_CRAFT(ch).crafting_method = 0;
  GET_CRAFT(ch).craft_duration = 0;
  roll = d20(ch);
  talent_bonus = get_talent_rank(ch, TALENT_SURVEYOR);
  skill_bonus = get_craft_skill_value(ch, ABILITY_HARVEST_SURVEYING);
  GET_SURVEY_ROOMS(ch) = roll + talent_bonus + skill_bonus;
  
  char talent_bonus_text[128];
  char skill_bonus_text[128];
  
  if (talent_bonus > 0)
    snprintf(talent_bonus_text, sizeof(talent_bonus_text), " + surveyor talent [%d]", talent_bonus);
  else
    talent_bonus_text[0] = '\0';
  
  if (skill_bonus > 0)
    snprintf(skill_bonus_text, sizeof(skill_bonus_text), " + skill bonus [%d]", skill_bonus);
  else
    skill_bonus_text[0] = '\0';
  
  send_to_char(ch, "Roll [%d]%s%s = %d rooms surveyed.\r\n",
               roll, talent_bonus_text, skill_bonus_text, GET_SURVEY_ROOMS(ch));
  
  /* Check for survey exp cooldown */
  time_t now = time(NULL);
  if (GET_SURVEY_EXP_COOLDOWN(ch) > now)
  {
    int seconds_left = GET_SURVEY_EXP_COOLDOWN(ch) - now;
    int minutes_left = seconds_left / 60;
    seconds_left = seconds_left % 60;
    send_to_char(ch, "\tYSurvey exp cooldown active - you can gain exp again in %d minute%s %d second%s.\tn\r\n",
                 minutes_left, (minutes_left == 1) ? "" : "s", seconds_left, (seconds_left == 1) ? "" : "s");
  }
  else
  {
    /* Cooldown has expired or doesn't exist, set new cooldown (2 minutes) */
    GET_SURVEY_EXP_COOLDOWN(ch) = now + (2 * 60);
    gain_craft_exp(ch, (10 + GET_SURVEY_ROOMS(ch)) * 2, ABILITY_HARVEST_SURVEYING, TRUE);
  }
  
  act("$n finishes surveying.", FALSE, ch, 0, 0, TO_ROOM);
}

void set_crafting_itemtype(struct char_data *ch, char *arg2)
{
  int i = 0;

  if (GET_CRAFT(ch).crafting_item_type != CRAFT_TYPE_NONE)
  {
    send_to_char(ch, "You have already set the item type. You need to reset the crafting project "
                     "to change this, by typing 'craft reset'.\r\n");
    return;
  }

  if (!*arg2)
  {
    send_to_char(ch, "%s", NEWCRAFT_CREATE_TYPES);
    return;
  }
  for (i = 1; i < NUM_CRAFT_TYPES; i++)
  {
    if (is_abbrev(arg2, crafting_types[i]))
      break;
  }
  if (i >= NUM_CRAFT_TYPES)
  {
    send_to_char(ch, "That is not a valid crafting type.\r\n");
    send_to_char(ch, "%s", NEWCRAFT_CREATE_TYPES);
    return;
  }

  if (i == CRAFT_TYPE_INSTRUMENT)
  {
    GET_CRAFT(ch).instrument_breakability = INSTRUMENT_BREAKABILITY_DEFAULT;
  }

  send_to_char(ch, "Crafting item type set to: %s\r\n", crafting_types[i]);

  if (GET_CRAFT(ch).crafting_item_type == CRAFT_TYPE_WEAPON ||
      GET_CRAFT(ch).crafting_item_type == CRAFT_TYPE_ARMOR)
  {
    if (GET_CRAFT(ch).crafting_item_type != i)
    {
      send_to_char(ch, "The enhancement bonus has been reset to zero.\r\n");
      GET_CRAFT(ch).enhancement = 0;
    }
  }

  GET_CRAFT(ch).crafting_item_type = i;
  GET_CRAFT(ch).craft_variant = -1; // Initialize variant to "not set"
}

bool is_valid_craft_weapon(int weapon)
{
  switch (weapon)
  {
  case WEAPON_TYPE_COMPOSITE_LONGBOW:
  case WEAPON_TYPE_COMPOSITE_LONGBOW_2:
  case WEAPON_TYPE_COMPOSITE_LONGBOW_3:
  case WEAPON_TYPE_COMPOSITE_LONGBOW_4:
  case WEAPON_TYPE_COMPOSITE_SHORTBOW:
  case WEAPON_TYPE_COMPOSITE_SHORTBOW_2:
  case WEAPON_TYPE_COMPOSITE_SHORTBOW_3:
  case WEAPON_TYPE_COMPOSITE_SHORTBOW_4:
    return FALSE;
  }
  return TRUE;
}

void craft_show_weapon_types(struct char_data *ch)
{
  int i = 0, count = 0;

  for (i = 1; i < NUM_WEAPON_TYPES; i++)
  {
    if (!is_valid_craft_weapon(i))
      continue;
    send_to_char(ch, "%-25s ", weapon_list[i].name);
    if ((count % 3) == 0)
      send_to_char(ch, "\r\n");
    count++;
  }
  if ((count % 3) != 0)
    send_to_char(ch, "\r\n");
}

void set_craft_weapon_type(struct char_data *ch, char *arg2)
{
  int i = 0;

  if (!*arg2)
  {
    send_to_char(ch, "\tCYou need to specify a weapon type:\tn\r\n");
    craft_show_weapon_types(ch);
    return;
  }

  for (i = 1; i < NUM_WEAPON_TYPES; i++)
  {
    if (is_abbrev(arg2, weapon_list[i].name))
      break;
  }

  if (i >= NUM_WEAPON_TYPES)
  {
    send_to_char(ch, "That is not a valid weapon type.\r\n");
    craft_show_weapon_types(ch);
    return;
  }

  GET_CRAFT(ch).crafting_specific = i;
  send_to_char(ch, "Crafting weapon type set to: %s\r\n", weapon_list[i].name);
}

void craft_show_armor_types(struct char_data *ch)
{
  int i = 0;

  for (i = 1; i < NUM_SPEC_ARMOR_TYPES; i++)
  {
    send_to_char(ch, "%-25s ", armor_list[i].name);
    if ((i % 3) == 0)
      send_to_char(ch, "\r\n");
  }
  if ((i % 3) != 0)
    send_to_char(ch, "\r\n");
}

void set_craft_armor_type(struct char_data *ch, char *arg2)
{
  int i = 0;

  if (!*arg2)
  {
    send_to_char(ch, "\tCYou need to specify an armor type:\tn\r\n");
    craft_show_armor_types(ch);
    return;
  }

  for (i = 1; i < NUM_SPEC_ARMOR_TYPES; i++)
  {
    if (is_abbrev(arg2, armor_list[i].name))
      break;
  }

  if (i >= NUM_SPEC_ARMOR_TYPES)
  {
    send_to_char(ch, "That is not a valid armor type.\r\n");
    craft_show_armor_types(ch);
    return;
  }

  GET_CRAFT(ch).crafting_specific = i;
  send_to_char(ch, "Crafting armor type set to: %s\r\n", armor_list[i].name);
}

void craft_show_instrument_types(struct char_data *ch)
{
  int i = 0;

  for (i = 1; i < NUM_CRAFT_INSTRUMENT_TYPES; i++)
  {
    send_to_char(ch, "%-25s ", crafting_instrument_types[i]);
    if ((i % 3) == 0)
      send_to_char(ch, "\r\n");
  }
  if ((i % 3) != 0)
    send_to_char(ch, "\r\n");
}

void set_craft_instrument_type(struct char_data *ch, char *arg2)
{
  int i = 0;

  if (!*arg2)
  {
    send_to_char(ch, "\tCYou need to specify an instrument type:\tn\r\n");
    craft_show_instrument_types(ch);
    return;
  }

  for (i = 1; i < NUM_CRAFT_INSTRUMENT_TYPES; i++)
  {
    if (is_abbrev(arg2, crafting_instrument_types[i]))
      break;
  }

  if (i >= NUM_CRAFT_INSTRUMENT_TYPES)
  {
    send_to_char(ch, "That is not a valid instrument type.\r\n");
    craft_show_instrument_types(ch);
    return;
  }

  GET_CRAFT(ch).crafting_specific = i;
  send_to_char(ch, "Crafting instrument type set to: %s\r\n", crafting_instrument_types[i]);
}

void craft_show_misc_types(struct char_data *ch)
{
  int i = 0;

  for (i = 1; i < NUM_CRAFT_MISC_TYPES; i++)
  {
    send_to_char(ch, "%-25s ", crafting_misc_types[i]);
    if ((i % 3) == 0)
      send_to_char(ch, "\r\n");
  }
  if ((i % 3) != 0)
    send_to_char(ch, "\r\n");
}

void set_craft_misc_type(struct char_data *ch, char *arg2)
{
  int i = 0;

  if (!*arg2)
  {
    send_to_char(ch, "\tCYou need to specify a misc type:\tn\r\n");
    craft_show_misc_types(ch);
    return;
  }

  for (i = 1; i < NUM_CRAFT_MISC_TYPES; i++)
  {
    if (is_abbrev(arg2, crafting_misc_types[i]))
      break;
  }

  if (i >= NUM_CRAFT_MISC_TYPES)
  {
    send_to_char(ch, "That is not a valid misc type.\r\n");
    craft_show_misc_types(ch);
    return;
  }

  GET_CRAFT(ch).crafting_specific = i;
  send_to_char(ch, "Crafting misc type set to: %s\r\n", crafting_misc_types[i]);
}

void set_crafting_keywords(struct char_data *ch, const char *arg2)
{
  if (!*arg2)
  {
    send_to_char(ch,
                 "You need to specify the keyword list. Do not add dashes to the keywords.\r\n");
    return;
  }
  if (strstr(arg2, "-"))
  {
    send_to_char(ch, "Please do not use dashes in the keyword list, as it can affect the ability "
                     "to target the object.\r\n");
    return;
  }
  if (strlen(arg2) > 100)
  {
    send_to_char(ch, "The keyword list must be less than 100 characters.\r\n");
    return;
  }
  if (GET_CRAFT(ch).crafting_item_type == 0 || GET_CRAFT(ch).crafting_specific == 0 ||
      GET_CRAFT(ch).craft_variant == -1 || GET_CRAFT(ch).crafting_recipe == 0)
  {
    send_to_char(ch, "You must set item type, specific type and variant first.\r\n");
    return;
  }
  if (GET_CRAFT(ch).materials[crafting_recipes[GET_CRAFT(ch).crafting_recipe]
                                  .materials[0][GET_CRAFT(ch).craft_variant][0]][1] == 0)
  {
    send_to_char(ch, "You must add the primary material before you can set keywords.\r\n");
    return;
  }
  if (!strstr(arg2, crafting_recipes[GET_CRAFT(ch).crafting_recipe]
                        .variant_descriptions[GET_CRAFT(ch).craft_variant]) ||
      !strstr(arg2,
              crafting_material_descriptions
                  [GET_CRAFT(ch).materials[crafting_recipes[GET_CRAFT(ch).crafting_recipe]
                                               .materials[0][GET_CRAFT(ch).craft_variant][0]][0]]))
  {
    send_to_char(
        ch,
        "You need to have the variant type '%s' and material type '%s' in your keyword list.\r\n",
        crafting_recipes[GET_CRAFT(ch).crafting_recipe]
            .variant_descriptions[GET_CRAFT(ch).craft_variant],
        crafting_material_descriptions
            [GET_CRAFT(ch).materials[crafting_recipes[GET_CRAFT(ch).crafting_recipe]
                                         .materials[0][GET_CRAFT(ch).craft_variant][0]][0]]);
    return;
  }
  GET_CRAFT(ch).keywords = strdup(arg2);
  send_to_char(ch, "You have set the keywords for your crafting item to:\r\n-- %s\r\n", arg2);
  return;
}

void set_crafting_short_desc(struct char_data *ch, const char *arg2)
{
  if (!*arg2)
  {
    send_to_char(ch, "You need to specify the object's short description.\r\n");
    return;
  }

  if (strlen(arg2) > 100)
  {
    send_to_char(ch, "The short description must be less than 100 characters.\r\n");
    return;
  }
  if (GET_CRAFT(ch).crafting_item_type == 0 || GET_CRAFT(ch).crafting_specific == 0 ||
      GET_CRAFT(ch).craft_variant == -1 || GET_CRAFT(ch).crafting_recipe == 0)
  {
    send_to_char(ch, "You must set item type, specific type and variant first.\r\n");
    return;
  }
  if (GET_CRAFT(ch).materials[crafting_recipes[GET_CRAFT(ch).crafting_recipe]
                                  .materials[0][GET_CRAFT(ch).craft_variant][0]][1] == 0)
  {
    send_to_char(ch, "You must add the primary material before you can set short description.\r\n");
    return;
  }
  if (!strstr(arg2, crafting_recipes[GET_CRAFT(ch).crafting_recipe]
                        .variant_descriptions[GET_CRAFT(ch).craft_variant]) ||
      !strstr(arg2,
              crafting_material_descriptions
                  [GET_CRAFT(ch).materials[crafting_recipes[GET_CRAFT(ch).crafting_recipe]
                                               .materials[0][GET_CRAFT(ch).craft_variant][0]][0]]))
  {
    send_to_char(
        ch,
        "You need to have the variant type '%s' and material type '%s' in your short "
        "description.\r\n",
        crafting_recipes[GET_CRAFT(ch).crafting_recipe]
            .variant_descriptions[GET_CRAFT(ch).craft_variant],
        crafting_material_descriptions
            [GET_CRAFT(ch).materials[crafting_recipes[GET_CRAFT(ch).crafting_recipe]
                                         .materials[0][GET_CRAFT(ch).craft_variant][0]][0]]);
    return;
  }
  GET_CRAFT(ch).short_description = strdup(arg2);
  send_to_char(ch, "You have set the short description for your crafting item to:\r\n-- %s\r\n",
               arg2);
  return;
}

void set_crafting_room_desc(struct char_data *ch, const char *arg2)
{
  if (!*arg2)
  {
    send_to_char(ch, "You need to specify the object's short description. This displays as the "
                     "name of the item.\r\n");
    return;
  }

  if (strlen(arg2) > 120)
  {
    send_to_char(ch, "The room description must be less than 120 characters. This is what shows "
                     "when you type 'look' in a room.\r\n");
    return;
  }
  if (GET_CRAFT(ch).crafting_item_type == 0 || GET_CRAFT(ch).crafting_specific == 0 ||
      GET_CRAFT(ch).craft_variant == -1 || GET_CRAFT(ch).crafting_recipe == 0)
  {
    send_to_char(ch, "You must set item type, specific type and variant first.\r\n");
    return;
  }
  if (GET_CRAFT(ch).materials[crafting_recipes[GET_CRAFT(ch).crafting_recipe]
                                  .materials[0][GET_CRAFT(ch).craft_variant][0]][1] == 0)
  {
    send_to_char(
        ch, "You must add the primary material before you can set the long/room description\r\n");
    return;
  }
  if (!strstr(arg2, crafting_recipes[GET_CRAFT(ch).crafting_recipe]
                        .variant_descriptions[GET_CRAFT(ch).craft_variant]) ||
      !strstr(arg2,
              crafting_material_descriptions
                  [GET_CRAFT(ch).materials[crafting_recipes[GET_CRAFT(ch).crafting_recipe]
                                               .materials[0][GET_CRAFT(ch).craft_variant][0]][0]]))
  {
    send_to_char(
        ch,
        "You need to have the variant type '%s' and material type '%s' in your long/room "
        "description.\r\n",
        crafting_recipes[GET_CRAFT(ch).crafting_recipe]
            .variant_descriptions[GET_CRAFT(ch).craft_variant],
        crafting_material_descriptions
            [GET_CRAFT(ch).materials[crafting_recipes[GET_CRAFT(ch).crafting_recipe]
                                         .materials[0][GET_CRAFT(ch).craft_variant][0]][0]]);
    return;
  }
  GET_CRAFT(ch).room_description = strdup(arg2);
  send_to_char(ch, "You have set the room description for your crafting item to:\r\n-- %s\r\n",
               arg2);
  return;
}

void set_crafting_extra_desc(struct char_data *ch, const char *arg2)
{
  if (GET_CRAFT(ch).keywords == NULL)
  {
    send_to_char(
        ch, "You must set the object's keywords before you can add the extra description.\r\n");
    return;
  }

  if (!*arg2)
  {
    send_to_char(ch, "You need to specify the object's extra description. This displays when you "
                     "type 'look (item)'.\r\n");
    return;
  }

  if (strlen(arg2) > MAX_EXTRA_DESC)
  {
    send_to_char(ch,
                 "The extra description must be less than %d characters. This is what shows when "
                 "you type 'look (item)'.\r\n",
                 MAX_EXTRA_DESC);
    return;
  }

  GET_CRAFT(ch).ex_description = strdup(arg2);
  send_to_char(ch, "You have set the extra description for your crafting item to:\r\n-- %s\r\n",
               arg2);
  return;
}

int get_enhancement_mote_type(struct char_data *ch, int type, int spec)
{
  switch (type)
  {
  case CRAFT_TYPE_WEAPON:
    switch (weapon_list[spec].weaponFamily)
    {
    case WEAPON_FAMILY_MONK:
      return CRAFTING_MOTE_AIR;
    case WEAPON_FAMILY_LIGHT_BLADE:
      return CRAFTING_MOTE_DARK;
    case WEAPON_FAMILY_HAMMER:
      return CRAFTING_MOTE_EARTH;
    case WEAPON_FAMILY_RANGED:
      return CRAFTING_MOTE_FIRE;
    case WEAPON_FAMILY_HEAVY_BLADE:
      return CRAFTING_MOTE_ICE;
    case WEAPON_FAMILY_POLEARM:
      return CRAFTING_MOTE_LIGHT;
    case WEAPON_FAMILY_DOUBLE:
      return CRAFTING_MOTE_LIGHTNING;
    case WEAPON_FAMILY_AXE:
      return CRAFTING_MOTE_WATER;
    default:
      return CRAFTING_MOTE_NONE;
    }
    break;
  case CRAFT_TYPE_ARMOR:
    switch (armor_list[spec].armorType)
    {
    case ARMOR_TYPE_HEAVY:
      return CRAFTING_MOTE_LIGHTNING;
    case ARMOR_TYPE_LIGHT:
      return CRAFTING_MOTE_FIRE;
    case ARMOR_TYPE_MEDIUM:
      return CRAFTING_MOTE_EARTH;
    case ARMOR_TYPE_SHIELD:
      return CRAFTING_MOTE_WATER;
    case ARMOR_TYPE_TOWER_SHIELD:
      return CRAFTING_MOTE_AIR;
    case ARMOR_TYPE_NONE:
      return CRAFTING_MOTE_ICE;
    default:
      return CRAFTING_MOTE_NONE;
    }
    break;
  default:
    return CRAFTING_MOTE_NONE;
  }
  return CRAFTING_MOTE_NONE;
}

void set_crafting_motes(struct char_data *ch, const char *argument)
{
  int slot = 0, enhancement = 0, method = 0;
  int have = 0, required = 0, mote_type = 0, allocated = 0;
  int location = 0, modifier = 0, bonus_type = 0, specific = 0;
  char arg1[100], arg2[100];

  two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

  if (!*arg1 || !*arg2)
  {
    send_to_char(ch, "%s", CRAFT_MOTE_NOARG);
    return;
  }

  // weapon / armor / shield enhancement bonus
  if (is_abbrev(arg2, "enhancement"))
  {
    enhancement = GET_CRAFT(ch).enhancement;

    if (enhancement <= 0)
    {
      send_to_char(ch, "The enhancement bonus on this project is zero. If it is an armor, shield "
                       "or weapon, you can set it using 'craft enhancement'\r\n");
      return;
    }

    if (GET_CRAFT(ch).crafting_item_type == 0 || GET_CRAFT(ch).crafting_specific == 0)
    {
      send_to_char(ch, "You must set the item type and specific type before continuing.\r\n");
      return;
    }

    if (GET_CRAFT(ch).crafting_item_type != CRAFT_TYPE_ARMOR &&
        GET_CRAFT(ch).crafting_item_type != CRAFT_TYPE_WEAPON)
    {
      send_to_char(ch, "You can only set motes for armor, shields and weapons.\r\n");
      return;
    }

    mote_type = get_enhancement_mote_type(ch, GET_CRAFT(ch).crafting_item_type,
                                          GET_CRAFT(ch).crafting_specific);
    have = GET_CRAFT_MOTES(ch, mote_type);
    required = craft_motes_required(0, 0, 0, enhancement);
    method = 1;
  }
  else if (is_abbrev(arg2, "quality"))
  {
    if (GET_CRAFT(ch).crafting_item_type != CRAFT_TYPE_INSTRUMENT)
    {
      send_to_char(ch, "You can only set motes for the quality of an instrument.\r\n");
      return;
    }

    mote_type = get_crafting_instrument_motes(ch, 1, FALSE);
    have = GET_CRAFT_MOTES(ch, mote_type);
    required = get_crafting_instrument_motes(ch, 1, TRUE);
    method = 3;
  }
  else if (is_abbrev(arg2, "effectiveness"))
  {
    if (GET_CRAFT(ch).crafting_item_type != CRAFT_TYPE_INSTRUMENT)
    {
      send_to_char(ch, "You can only set motes for the effectiveness of an instrument.\r\n");
      return;
    }

    mote_type = get_crafting_instrument_motes(ch, 2, FALSE);
    have = GET_CRAFT_MOTES(ch, mote_type);
    required = get_crafting_instrument_motes(ch, 2, TRUE);
    method = 4;
  }
  else if (is_abbrev(arg2, "breakability"))
  {
    if (GET_CRAFT(ch).crafting_item_type != CRAFT_TYPE_INSTRUMENT)
    {
      send_to_char(ch, "You can only set motes for the breakability of an instrument.\r\n");
      return;
    }

    mote_type = get_crafting_instrument_motes(ch, 3, FALSE);
    have = GET_CRAFT_MOTES(ch, mote_type);
    required = get_crafting_instrument_motes(ch, 3, TRUE);
    method = 5;
  }
  else
  {
    slot = atoi(arg2);

    if (slot < 1 || slot > (MAX_OBJ_AFFECT + 1))
    {
      send_to_char(ch, "Please select a bonus slot between 1 and 6.\r\n");
      return;
    }

    // bonus array is 0-5, but for user simplicity we have them enter in 1-6
    slot--;

    if (!is_valid_apply(GET_CRAFT(ch).affected[slot].location) ||
        GET_CRAFT(ch).affected[slot].modifier == 0)
    {
      send_to_char(ch, "There is no bonus set in that slot. Type craft show or help craft-bonuses "
                       "for more info.\r\n");
      return;
    }

    location = GET_CRAFT(ch).affected[slot].location;
    modifier = GET_CRAFT(ch).affected[slot].modifier;
    bonus_type = GET_CRAFT(ch).affected[slot].bonus_type;
    specific = GET_CRAFT(ch).affected[slot].specific;

    mote_type = crafting_mote_by_bonus_location(location, specific, bonus_type);

    have = GET_CRAFT_MOTES(ch, mote_type);
    required = craft_motes_required(location, modifier, bonus_type, 0);

    method = 2;
  }

  if (is_abbrev(arg1, "add"))
  {
    if (GET_CRAFT(ch).enhancement_motes_required > 0 && method == 1)
    {
      send_to_char(ch, "You have already assigned motes for the enhancement bonus.\r\n");
      return;
    }
    else if (GET_CRAFT(ch).instrument_motes[1] > 0 && method == 3)
    {
      send_to_char(ch, "You have already assigned motes for the instrument quality.\r\n");
      return;
    }
    else if (GET_CRAFT(ch).instrument_motes[2] > 0 && method == 4)
    {
      send_to_char(ch, "You have already assigned motes for the instrument effectiveness.\r\n");
      return;
    }
    else if (GET_CRAFT(ch).instrument_motes[3] > 0 && method == 5)
    {
      send_to_char(ch, "You have already assigned motes for the instrument breakability.\r\n");
      return;
    }
    else if (GET_CRAFT(ch).motes_required[slot] > 0 && method == 2)
    {
      send_to_char(ch, "You have already assigned motes for bonus slot %d.\r\n", slot + 1);
      return;
    }
    if (have < required)
    {
      send_to_char(ch, "You require %d %ss, but only have %d.\r\n", required,
                   crafting_motes[mote_type], have);
      return;
    }
    if (method == 2)
      GET_CRAFT(ch).motes_required[slot] = required;
    else if (method == 3)
      GET_CRAFT(ch).instrument_motes[1] = required;
    else if (method == 4)
      GET_CRAFT(ch).instrument_motes[2] = required;
    else if (method == 5)
      GET_CRAFT(ch).instrument_motes[3] = required;
    else
      GET_CRAFT(ch).enhancement_motes_required = required;
    GET_CRAFT_MOTES(ch, mote_type) -= required;
    send_to_char(ch,
                 "You assign %d %ss to your project. Type craft show to review your projects.\r\n",
                 required, crafting_motes[mote_type]);
  }
  else if (is_abbrev(arg1, "remove"))
  {
    if (method == 2)
    {
      allocated = GET_CRAFT(ch).motes_required[slot];
      if (allocated <= 0)
      {
        send_to_char(ch, "There are no motes assigned to that bonus slot yet.\r\n");
        return;
      }
      GET_CRAFT_MOTES(ch, mote_type) += allocated;
      GET_CRAFT(ch).motes_required[slot] = 0;
    }
    if (method == 3)
    {
      allocated = GET_CRAFT(ch).instrument_motes[1];
      if (allocated <= 0)
      {
        send_to_char(ch, "There are no motes assigned for your instrument quality yet.\r\n");
        return;
      }
      GET_CRAFT_MOTES(ch, mote_type) += allocated;
      GET_CRAFT(ch).instrument_motes[1] = 0;
    }
    else if (method == 4)
    {
      allocated = GET_CRAFT(ch).instrument_motes[2];
      if (allocated <= 0)
      {
        send_to_char(ch, "There are no motes assigned for your instrument effectiveness yet.\r\n");
        return;
      }
      GET_CRAFT_MOTES(ch, mote_type) += allocated;
      GET_CRAFT(ch).instrument_motes[2] = 0;
    }
    else if (method == 5)
    {
      allocated = GET_CRAFT(ch).instrument_motes[3];
      if (allocated <= 0)
      {
        send_to_char(ch, "There are no motes assigned for your instrument breakability yet.\r\n");
        return;
      }
      GET_CRAFT_MOTES(ch, mote_type) += allocated;
      GET_CRAFT(ch).instrument_motes[3] = 0;
    }
    else
    {
      allocated = GET_CRAFT(ch).enhancement_motes_required;
      if (allocated <= 0)
      {
        send_to_char(ch, "There are no motes assigned for your item enhancement yet.\r\n");
        return;
      }
      GET_CRAFT_MOTES(ch, mote_type) += allocated;
      GET_CRAFT(ch).enhancement_motes_required = 0;
    }
    send_to_char(
        ch, "You recover %d %ss from your project. Type craft show to review your projects.\r\n",
        required, crafting_motes[mote_type]);
  }
  else
  {
    send_to_char(ch, "%s", CRAFT_MOTE_NOARG);
  }
}

void set_crafting_catalysts(struct char_data *ch, const char *argument)
{
  char arg1[100], arg2[100];
  int amount = 0, target = 0, delta = 0;

  two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

  if (GET_CRAFT(ch).craft_duration > 0)
  {
    send_to_char(ch, "You cannot change catalysts after crafting has begun.\r\n");
    return;
  }

  if (!*arg1)
  {
    send_to_char(ch, "Usage: craft catalysts <0-%d|add #|remove [#]>.\r\n",
                 CRAFT_CATALYST_MAX);
    send_to_char(ch, "You have %d catalyst%s stored and %d allocated to this project.\r\n",
                 GET_CRAFT_MAT(ch, CRAFT_MAT_CATALYST),
                 GET_CRAFT_MAT(ch, CRAFT_MAT_CATALYST) == 1 ? "" : "s",
                 GET_CRAFT(ch).catalysts_used);
    return;
  }

  if (is_abbrev(arg1, "remove") || is_abbrev(arg1, "reset") || is_abbrev(arg1, "clear"))
  {
    amount = *arg2 ? atoi(arg2) : GET_CRAFT(ch).catalysts_used;
    if (amount <= 0)
    {
      send_to_char(ch, "You do not have any catalysts allocated to remove.\r\n");
      return;
    }
    amount = MIN(amount, GET_CRAFT(ch).catalysts_used);
    GET_CRAFT_MAT(ch, CRAFT_MAT_CATALYST) += amount;
    GET_CRAFT(ch).catalysts_used -= amount;
    send_to_char(ch, "You recover %d catalyst%s from your project. %d remain allocated.\r\n",
                 amount, amount == 1 ? "" : "s", GET_CRAFT(ch).catalysts_used);
    return;
  }

  if (craft_has_pending_noncreate_project(ch))
  {
    send_to_char(ch, "Catalysts can only be added to new item crafting projects.\r\n");
    return;
  }

  if (is_abbrev(arg1, "add"))
  {
    if (!*arg2)
    {
      send_to_char(ch, "Add how many catalysts? You may allocate up to %d.\r\n",
                   CRAFT_CATALYST_MAX);
      return;
    }
    amount = atoi(arg2);
    if (amount <= 0)
    {
      send_to_char(ch, "You must add at least one catalyst.\r\n");
      return;
    }
    target = GET_CRAFT(ch).catalysts_used + amount;
  }
  else
  {
    if (!is_number(arg1))
    {
      send_to_char(ch, "Usage: craft catalysts <0-%d|add #|remove [#]>.\r\n",
                   CRAFT_CATALYST_MAX);
      return;
    }
    target = atoi(arg1);
    if (target < 0)
    {
      send_to_char(ch, "You may allocate between 0 and %d catalysts.\r\n", CRAFT_CATALYST_MAX);
      return;
    }
  }

  if (target > CRAFT_CATALYST_MAX)
  {
    send_to_char(ch, "You may allocate no more than %d catalysts for a %d%% critical chance.\r\n",
                 CRAFT_CATALYST_MAX, CRAFT_CATALYST_MAX);
    return;
  }

  if (target > 0 && !craft_project_has_catalyst_target(ch))
  {
    send_to_char(ch, "Catalysts require a new item crafting project.\r\n");
    return;
  }

  delta = target - GET_CRAFT(ch).catalysts_used;
  if (delta > 0)
  {
    if (GET_CRAFT_MAT(ch, CRAFT_MAT_CATALYST) < delta)
    {
      send_to_char(ch, "You need %d more catalyst%s, but only have %d stored.\r\n",
                   delta, delta == 1 ? "" : "s", GET_CRAFT_MAT(ch, CRAFT_MAT_CATALYST));
      return;
    }
    GET_CRAFT_MAT(ch, CRAFT_MAT_CATALYST) -= delta;
  }
  else if (delta < 0)
  {
    GET_CRAFT_MAT(ch, CRAFT_MAT_CATALYST) += -delta;
  }

  GET_CRAFT(ch).catalysts_used = target;
  send_to_char(ch, "You have %d catalyst%s allocated for a %d%% critical chance.\r\n",
               GET_CRAFT(ch).catalysts_used,
               GET_CRAFT(ch).catalysts_used == 1 ? "" : "s",
               GET_CRAFT(ch).catalysts_used);
}

int get_crafting_instrument_dc_modifier(struct char_data *ch)
{
  int dc_mod = 0;
  int quality = GET_CRAFT(ch).instrument_quality;
  int effectiveness = GET_CRAFT(ch).instrument_effectiveness;
  int breakability = GET_CRAFT(ch).instrument_breakability;

  dc_mod += quality / 3;
  dc_mod += effectiveness;
  if (breakability == 0)
    dc_mod += 10;
  else
    dc_mod += (INSTRUMENT_BREAKABILITY_DEFAULT - breakability) / 5;

  return dc_mod;
}

// type is the object value associated with the query: 1 = quality, 2 = effectiveness, 3 = breakability
// get_amount is FALSE if you want to get the type of mote, or TRUE if you want to get the number of motes
int get_crafting_instrument_motes(struct char_data *ch, int type, bool get_amount)
{
  switch (type)
  {
  case 1: // quality
    if (!get_amount)
    {
      return CRAFTING_MOTE_AIR;
    }
    else
    {
      return GET_CRAFT(ch).instrument_quality / 3; // 1-30 quality, so 0-10 motes
    }
    break;
  case 2: // effectiveness
    if (!get_amount)
    {
      return CRAFTING_MOTE_WATER;
    }
    else
    {
      return GET_CRAFT(ch).instrument_effectiveness; // 1-10 effectiveness, so 1-10 motes
    }
    break;
  case 3: // breakability
    if (!get_amount)
    {
      return CRAFTING_MOTE_EARTH;
    }
    else
    {
      if (GET_CRAFT(ch).instrument_breakability == 0)
        return 15; // 0 breakability means unbreakable, so 15 motes
      else
        return (INSTRUMENT_BREAKABILITY_DEFAULT - GET_CRAFT(ch).instrument_breakability) /
               5; // 0-30 breakability, so 0-6 motes
    }
    break;
  }
  return 0;
}

void set_crafting_instrument(struct char_data *ch, char *arg2)
{
  int value = 0;
  char arg3[200], arg4[200];

  if (GET_CRAFT(ch).craft_variant == -1)
  {
    send_to_char(ch, "You must set the crafting variant first.\r\n");
    return;
  }

  two_arguments(arg2, arg3, sizeof(arg3), arg4, sizeof(arg4));

  if (!*arg3 || !*arg4)
  {
    send_to_char(ch,
                 "Please enter one of the following:\r\n"
                 "-- craft instrument quality [1-30]\r\n"
                 "-- craft instrument effectiveness [1-10]\r\n"
                 "-- craft instrument breakability [0-%d]\r\n",
                 INSTRUMENT_BREAKABILITY_DEFAULT);
    return;
  }

  // intrument type val0 - this will be set upon craft instrument complete

  if (is_abbrev(arg3, "quality"))
  {
    value = atoi(arg4);
    if (value < 1 || value > 30)
    {
      send_to_char(ch, "The quality must be between 1 and 30.\r\n"
                       "This will reduce the DC of the instrument performance by that amount.\r\n");
      return;
    }
    GET_CRAFT(ch).instrument_quality = value;
    send_to_char(ch, "You set the instrument quality to %d.\r\n", value);
  }
  else if (is_abbrev(arg3, "effectiveness"))
  {
    value = atoi(arg4);
    if (value < 1 || value > 10)
    {
      send_to_char(
          ch, "The effectiveness must be between 1 and 10.\r\n"
              "This will increase the effect of the instrument performance by that amount.\r\n");
      return;
    }
    GET_CRAFT(ch).instrument_effectiveness = value;
    send_to_char(ch, "You set the instrument effectiveness to %d.\r\n", value);
    return;
  }
  else if (is_abbrev(arg3, "breakability"))
  {
    value = atoi(arg4);
    if (value < 0 || value > INSTRUMENT_BREAKABILITY_DEFAULT)
    {
      send_to_char(
          ch,
          "The breakability must be between 0 and %d.\r\n"
          "This will set the chance of breaking the instrument by that amount in 11,111.\r\n"
          "Ie. breakability 0 means unbreakable, 1 means 1 in 11,111 chance to break, 30 means 30 "
          "in 11,111 chance to break.\r\n",
          INSTRUMENT_BREAKABILITY_DEFAULT);
      return;
    }
    GET_CRAFT(ch).instrument_breakability = value;
    send_to_char(ch, "You set the instrument breakability to %d.\r\n", value);
    return;
  }
  else
  {
    send_to_char(ch,
                 "You need to specify one of the following:\r\n"
                 "-- craft instrument quality [1-30]\r\n"
                 "-- craft instrument effectiveness [1-10]\r\n"
                 "-- craft instrument breakability [0-%d]\r\n",
                 INSTRUMENT_BREAKABILITY_DEFAULT);
    return;
  }
}

int get_craft_level_adjust_dc_change(int adjust)
{
  return -(adjust * 5);
}

void set_craft_level_adjust(struct char_data *ch, char *arg2)
{
  int adjust = 0;

  if (!*arg2)
  {
    send_to_char(
        ch,
        "You need to specify the level adjustment for the crafting item. Each -1 to the final "
        "object level adds +5 to the craft dc, and vice versa for increasing object level.\r\n");
    return;
  }

  adjust = atoi(arg2);

  send_to_char(ch, "You've set the crafting level adjustment to %d.\r\n", adjust);
  send_to_char(
      ch, "This will adjust the final object level by %s%d and the dc will change by %s%d.\r\n",
      adjust > 0 ? "+" : "", adjust, get_craft_level_adjust_dc_change(adjust) > 0 ? "+" : "",
      get_craft_level_adjust_dc_change(adjust));
  GET_CRAFT(ch).level_adjust = adjust;
}

void set_crafting_bonuses(struct char_data *ch, const char *argument)
{
  char arg1[100], // bonus slot (0-5)
      arg2[100],  // bonus location
      arg3[100],  // bonus type (enhancement or universal)
      arg4[100],  // bonus modifier
      arg5[100],  // bonus specific
      temp[100], spectext[100];
  int i = 0, j = 0, slot = 0, location = 0, modifier = 0, max_modifier = 0, bonus_type = 0,
      specific = 0, cr_type = 0, cr_spec_type = 0, cr_variant = 0, cr_recipe = -1, wear_loc = 0;

  if (!ch)
    return;

  cr_type = GET_CRAFT(ch).crafting_item_type;
  cr_spec_type = GET_CRAFT(ch).crafting_specific;
  cr_variant = GET_CRAFT(ch).craft_variant;
  cr_recipe = GET_CRAFT(ch).crafting_recipe;

  if (!cr_type)
  {
    send_to_char(ch, "You must set the crafting item type before setting bonuses.\r\n");
    return;
  }
  else if (!cr_spec_type)
  {
    send_to_char(ch, "You must set the crafting specific type before setting bonuses.\r\n");
    return;
  }
  else if (cr_variant == -1)
  {
    send_to_char(ch, "You must set the crafting variant before setting bonuses.\r\n");
    return;
  }
  else if (cr_recipe == -1)
  {
    send_to_char(ch, "You must set the crafting recipe before setting bonuses.\r\n");
    return;
  }

  if (!*argument)
  {
    send_to_char(ch, "%s", NEWCRAFT_CREATE_BONUSES_NOARG);
    return;
  }

  five_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2), arg3, sizeof(arg3), arg4,
                 sizeof(arg4), arg5, sizeof(arg5));

  if (!*arg1)
  {
    send_to_char(ch, "%s", NEWCRAFT_CREATE_BONUSES_NOARG);
    return;
  }

  // determine bonus slot
  slot = atoi(arg1);

  if (slot < 1 || slot > 6)
  {
    send_to_char(ch, "The slot must be between 1 and 6.\r\n");
    return;
  }

  slot--; // array is 0-5

  if (*arg2 && is_abbrev(arg2, "reset"))
  {
    location = GET_CRAFT(ch).affected[slot].location;
    bonus_type = GET_CRAFT(ch).affected[slot].bonus_type;
    modifier = GET_CRAFT(ch).affected[slot].modifier;
    specific = GET_CRAFT(ch).affected[slot].specific;
    int mote_type = crafting_mote_by_bonus_location(location, specific, bonus_type);
    int num_motes = GET_CRAFT(ch).motes_required[slot];
    send_to_char(ch, "You've reset the bonus in slot %d.\r\n", slot + 1);
    if (num_motes > 0 && mote_type != CRAFTING_MOTE_NONE)
    {
      GET_CRAFT_MOTES(ch, mote_type) += num_motes;
      send_to_char(ch, "You've recovered %d %s.\r\n", num_motes, crafting_motes[mote_type]);
    }
    GET_CRAFT(ch).affected[slot].location = 0;
    GET_CRAFT(ch).affected[slot].bonus_type = 0;
    GET_CRAFT(ch).affected[slot].modifier = 0;
    GET_CRAFT(ch).affected[slot].specific = 0;
    GET_CRAFT(ch).motes_required[slot] = 0;
    return;
  }

  if (!*arg1 || !*arg2 || !*arg3 || !*arg4)
  {
    send_to_char(ch, "Specify the bonus slot, location, type, and modifier.\r\n"
                     "The specific argument is optional except for skills and spell slots.\r\n");
    send_to_char(ch, "%s", NEWCRAFT_CREATE_BONUSES_NOARG);
    return;
  }

  // determine bonus location

  if (is_abbrev(arg2, apply_types[APPLY_FEAT]))
  {
    send_to_char(ch, "Grant-feat bonuses cannot currently be crafted.\r\n");
    return;
  }

  if (GET_CRAFT(ch).affected[slot].location != APPLY_NONE)
  {
    send_to_char(ch,
                 "You have already set the apply type for this bonus slot. You'll have to do "
                 "'craft bonus %d reset' to change it.\r\n",
                 slot + 1);
    return;
  }

  for (i = 1; i < NUM_APPLIES; i++)
  {
    if (!is_valid_apply(i))
      continue;
    if (i == APPLY_FEAT)
      continue;
    snprintf(temp, sizeof(temp), "%s", apply_types[i]);
    for (j = 0; j < strlen(temp); j++)
    {
      temp[j] = tolower(temp[j]);
    }
    if (is_abbrev(arg2, temp))
      break;
  }

  if (i >= NUM_APPLIES)
  {
    send_to_char(ch, "You need to specify a valid bonus location. Type 'applies' for a list.\r\n");
    send_to_char(ch, "Note: Feats cannot be set as a bonus location.\r\n");
    return;
  }

  location = i;

  if (does_craft_apply_type_have_specific_value(location) && !*arg5)
  {
    send_to_char(ch, "This bonus requires a specific skill or spellcasting class as the fifth "
                     "argument.\r\n");
    return;
  }

  wear_loc = get_craft_wear_loc(ch);

  if (!is_bonus_valid_for_where_slot(location, wear_loc))
  {
    send_to_char(ch, "You cannot set a bonus of type %s on a %s.\r\n", apply_types[location],
                 wear_bits[wear_loc]);
    send_to_char(ch, "You can see valid bonus types for wear slots by typing: wearapplies.\r\n");
    return;
  }

  if (GET_CRAFT(ch).affected[slot].bonus_type != BONUS_TYPE_UNDEFINED)
  {
    send_to_char(ch,
                 "You have already set the bonus type for this bonus slot. You'll have to do "
                 "'craft bonus %d reset' to change it.\r\n",
                 slot + 1);
    return;
  }

  // determine bonus type
  for (i = 0; i < NUM_BONUS_TYPES; i++)
  {
    snprintf(temp, sizeof(temp), "%s", bonus_types[i]);
    for (j = 0; j < strlen(temp); j++)
    {
      temp[j] = tolower(temp[j]);
    }
    if (is_abbrev(arg3, temp))
      break;
  }

  if (i >= NUM_BONUS_TYPES)
  {
    send_to_char(ch, "You need to specify a valid bonus type. Valid bonus types are:\r\n"
                     "Armor Class: Enhancement, Deflection, Natural, Universal\r\n"
                     "Other Bonuses: Enhancement, Universal\r\n"
                     "Universal bonuses are divided by 3, since it stacks.\r\n");
    return;
  }

  bonus_type = i;

  switch (location)
  {
  case APPLY_AC_NEW:
    switch (bonus_type)
    {
    case BONUS_TYPE_ENHANCEMENT:
    case BONUS_TYPE_DEFLECTION:
    case BONUS_TYPE_NATURALARMOR:
    case BONUS_TYPE_UNIVERSAL:
      break;
    default:
      send_to_char(ch, "Armor class bonus types are restricted to: enhancement, natural, "
                       "deflection and universal.\r\n");
      return;
    }
    break;
  default:
    switch (bonus_type)
    {
    case BONUS_TYPE_ENHANCEMENT:
    case BONUS_TYPE_UNIVERSAL:
      break;
    default:
      send_to_char(ch, "Bonus types are restricted to: enhancement and universal.\r\n");
      return;
    }
    break;
  }

  if (GET_CRAFT(ch).affected[slot].modifier != 0)
  {
    send_to_char(ch,
                 "You have already set the stat modifier for this bonus slot. You'll have to do "
                 "'craft bonus %d reset' to change it.\r\n",
                 slot + 1);
    return;
  }

  // determine bonus modifier
  modifier = atoi(arg4);

  if (modifier < 1)
  {
    send_to_char(ch, "Bonus modifier must be greater than zero.\r\n");
    return;
  }

  max_modifier = get_gear_bonus_amount_by_level(location, 30);

  if (bonus_type == BONUS_TYPE_ENHANCEMENT)
    max_modifier *= 2;

  if (modifier > max_modifier)
  {
    send_to_char(ch, "The max bonus modifier for %s of type (%s) is %d.\r\n", apply_types[location],
                 bonus_types[bonus_type], max_modifier);
    return;
  }

  if (does_craft_apply_type_have_specific_value(location) &&
      GET_CRAFT(ch).affected[slot].specific != 0)
  {
    send_to_char(ch,
                 "You have already set the specific modifier for this bonus slot. You'll have to "
                 "do 'craft bonus %d reset' to change it.\r\n",
                 slot + 1);
    return;
  }

  // determine specifier for certain bonus locations
  snprintf(spectext, sizeof(spectext), "N/A");

  if (*arg5)
  {
    switch (location)
    {
    case APPLY_SKILL:
      for (i = START_GENERAL_ABILITIES; i <= NUM_ABILITIES; i++)
      {
        if (!is_valid_craft_ability(i))
          continue;
        snprintf(temp, sizeof(temp), "%s", ability_names[i]);
        for (j = 0; j < strlen(temp); j++)
        {
          temp[j] = tolower(temp[j]);
        }
        if (is_abbrev(arg5, temp))
          break;
      }
      if (i > NUM_ABILITIES)
      {
        send_to_char(
            ch, "That is not a valid skill. For a list of skills type: skills and craftskills\r\n");
        return;
      }
      specific = i;
      snprintf(spectext, sizeof(spectext), "%s", ability_names[specific]);
      break;
    case APPLY_FEAT:
      for (i = 1; i < FEAT_LAST_FEAT; i++)
      {
        if (!is_valid_craft_feat(i))
          continue;
        snprintf(temp, sizeof(temp), "%s", feat_list[i].name);
        for (j = 0; j < strlen(temp); j++)
        {
          temp[j] = tolower(temp[j]);
        }
        if (is_abbrev(arg5, temp))
          break;
      }
      if (i >= FEAT_LAST_FEAT)
      {
        send_to_char(
            ch,
            "That is not a valid feat. For a list of feats type: feats all\r\n"
            "Note that not all feats are allowed on gear. A general rule is:\r\n"
            "-- not epic, not a class or race feat, doesn't require a specific subtype\r\n"
            "-- such as weapon focus or skill focus, is of type general, combat, spellcasting,\r\n"
            "psionic, metamagic and teamwork.\r\n");
        return;
      }
      specific = i;
      snprintf(spectext, sizeof(spectext), "%s", feat_list[specific].name);
      break;
    case APPLY_SPELL_CIRCLE_1:
    case APPLY_SPELL_CIRCLE_2:
    case APPLY_SPELL_CIRCLE_3:
    case APPLY_SPELL_CIRCLE_4:
    case APPLY_SPELL_CIRCLE_5:
    case APPLY_SPELL_CIRCLE_6:
    case APPLY_SPELL_CIRCLE_7:
    case APPLY_SPELL_CIRCLE_8:
    case APPLY_SPELL_CIRCLE_9:
      for (i = CLASS_WIZARD; i < NUM_CLASSES; i++)
      {
        if (!is_valid_craft_class(i, location))
          continue;
        snprintf(temp, sizeof(temp), "%s", class_list[i].name);
        for (j = 0; j < strlen(temp); j++)
        {
          temp[j] = tolower(temp[j]);
        }
        if (is_abbrev(arg5, temp))
          break;
      }
      if (i >= NUM_CLASSES)
      {
        send_to_char(ch, "That is either not a valid spellcasting class, or the spell slot is too "
                         "high for that class.\r\n");
        return;
      }
      specific = i;
      snprintf(spectext, sizeof(spectext), "%s", class_list[specific].name);
      break;
    default:
      break;
    }
  }
  else
  {
    specific = 0;
  }

  // ok we have our info, let's start assigning it
  send_to_char(ch,
               "You have set your crafting object's affect in slot %d to the following:\r\n"
               "-- Bonus Location: %s\r\n"
               "-- Bonus Type    : %s\r\n"
               "-- Bonus Modifier: +%d\r\n"
               "-- Bonus Specific: %s\r\n",
               slot, apply_types[location], bonus_types[bonus_type], modifier, spectext);

  GET_CRAFT(ch).affected[slot].location = location;
  GET_CRAFT(ch).affected[slot].bonus_type = bonus_type;
  GET_CRAFT(ch).affected[slot].modifier = modifier;
  GET_CRAFT(ch).affected[slot].specific = specific;
}

int craft_motes_required(int location, int modifier, int bonus_type, int enhancement)
{
  int level;

  if (enhancement)
    level = get_level_adjustment_by_enhancement_bonus(enhancement);
  else
    level = get_level_adjustment_by_apply_and_modifier(location, modifier, bonus_type);

  if (level >= 30)
    return CRAFT_MOTES_REQ_30;
  else if (level >= 25)
    return CRAFT_MOTES_REQ_25;
  if (level >= 20)
    return CRAFT_MOTES_REQ_20;
  if (level >= 15)
    return CRAFT_MOTES_REQ_15;
  if (level >= 10)
    return CRAFT_MOTES_REQ_10;
  if (level >= 5)
    return CRAFT_MOTES_REQ_5;
  else
    return CRAFT_MOTES_REQ_1;
}

int crafting_mote_by_bonus_location(int location, int specific, int bonus_type)
{
  switch (location)
  {
  case APPLY_STR:
  case APPLY_HIT:
  case APPLY_RES_FIRE:
  case APPLY_RES_SLICE:
  case APPLY_HP_REGEN:
  case APPLY_SPELL_PENETRATION:
    return CRAFTING_MOTE_FIRE;
  case APPLY_DEX:
  case APPLY_SAVING_REFL:
  case APPLY_RES_ELECTRIC:
  case APPLY_RES_SOUND:
  case APPLY_INITIATIVE:
  case APPLY_SPELL_DURATION:
    return CRAFTING_MOTE_LIGHTNING;
  case APPLY_INT:
  case APPLY_PSP:
  case APPLY_RES_COLD:
  case APPLY_POWER_RES:
  case APPLY_PSP_REGEN:
  case APPLY_SPELL_CIRCLE_1:
  case APPLY_SPELL_CIRCLE_2:
  case APPLY_SPELL_CIRCLE_3:
  case APPLY_SPELL_CIRCLE_4:
  case APPLY_SPELL_CIRCLE_5:
  case APPLY_SPELL_CIRCLE_6:
  case APPLY_SPELL_CIRCLE_7:
  case APPLY_SPELL_CIRCLE_8:
  case APPLY_SPELL_CIRCLE_9:
    return CRAFTING_MOTE_ICE;
  case APPLY_WIS:
  case APPLY_SAVING_WILL:
  case APPLY_RES_PUNCTURE:
  case APPLY_RES_POISON:
  case APPLY_RES_WATER:
  case APPLY_ENCUMBRANCE:
    return CRAFTING_MOTE_WATER;
  case APPLY_CON:
  case APPLY_MOVE:
  case APPLY_SAVING_FORT:
  case APPLY_RES_EARTH:
  case APPLY_RES_ACID:
  case APPLY_MV_REGEN:
    return CRAFTING_MOTE_EARTH;
  case APPLY_CHA:
  case APPLY_RES_AIR:
  case APPLY_RES_FORCE:
  case APPLY_RES_ILLUSION:
  case APPLY_RES_ENERGY:
  case APPLY_FAST_HEALING:
    return CRAFTING_MOTE_AIR;
  case APPLY_HITROLL:
  case APPLY_SPELL_RES:
  case APPLY_RES_HOLY:
  case APPLY_RES_MENTAL:
  case APPLY_RES_LIGHT:
  case APPLY_SPELL_DC:
    return CRAFTING_MOTE_LIGHT;
  case APPLY_DAMROLL:
  case APPLY_RES_UNHOLY:
  case APPLY_RES_DISEASE:
  case APPLY_RES_NEGATIVE:
  case APPLY_SPELL_POTENCY:
    return CRAFTING_MOTE_DARK;

  case APPLY_AC_NEW:
    switch (bonus_type)
    {
    case BONUS_TYPE_UNIVERSAL:
    case BONUS_TYPE_ENHANCEMENT:
      return ARMOR_ENHANCEMENT_MOTE;
    case BONUS_TYPE_DEFLECTION:
      return CRAFTING_MOTE_FIRE;
    case BONUS_TYPE_NATURALARMOR:
      return CRAFTING_MOTE_EARTH;
    case BONUS_TYPE_DODGE:
      return CRAFTING_MOTE_LIGHTNING;
    }
    break;

  case APPLY_SKILL:
    switch (specific)
    {
    case ABILITY_ACROBATICS:
    case ABILITY_STEALTH:
    case ABILITY_RIDE:
    case ABILITY_SLEIGHT_OF_HAND:
    case ABILITY_DISABLE_DEVICE:
      return CRAFTING_MOTE_LIGHTNING;
    case ABILITY_RELIGION:
    case ABILITY_MEDICINE:
    case ABILITY_SPELLCRAFT:
    case ABILITY_APPRAISE:
    case ABILITY_ARCANA:
    case ABILITY_HISTORY:
    case ABILITY_NATURE:
      return CRAFTING_MOTE_ICE;
    case ABILITY_PERCEPTION:
    case ABILITY_DISCIPLINE:
    case ABILITY_HANDLE_ANIMAL:
    case ABILITY_INSIGHT:
      return CRAFTING_MOTE_WATER;
    case ABILITY_ATHLETICS:
      return CRAFTING_MOTE_FIRE;
    case ABILITY_CONCENTRATION:
    case ABILITY_TOTAL_DEFENSE:
      return CRAFTING_MOTE_EARTH;
    case ABILITY_INTIMIDATE:
    case ABILITY_DECEPTION:
    case ABILITY_PERSUASION:
    case ABILITY_DISGUISE:
    case ABILITY_USE_MAGIC_DEVICE:
    case ABILITY_PERFORM:
      return CRAFTING_MOTE_AIR;
    }
    break;
  }
  return CRAFTING_MOTE_NONE;
};

void show_current_craft(struct char_data *ch)
{
  char spec_item_type[100];
  char temp[MAX_EXTRA_DESC];
  char extra_desc[MAX_EXTRA_DESC + 5];
  char spectext[100];
  int i = 0;
  bool found = FALSE;
  int base_group = 0;
  int base_amount = 0;
  int project_material = 0;
  int project_amount = 0;
  int skill = 0, dc = 0, spec_type = 0;

  snprintf(extra_desc, sizeof(extra_desc), " ");

  if (GET_CRAFT(ch).ex_description != NULL)
  {
    snprintf(temp, sizeof(temp), "%s", GET_CRAFT(ch).ex_description);
    snprintf(extra_desc, sizeof(extra_desc), "\r\n%s", strfrmt(temp, 80, 1, FALSE, FALSE, FALSE));
  }

  snprintf(spec_item_type, sizeof(spec_item_type), " ");

  switch (GET_CRAFT(ch).crafting_item_type)
  {
  case CRAFT_TYPE_WEAPON:
    snprintf(spec_item_type, sizeof(spec_item_type), "%s",
             weapon_list[GET_CRAFT(ch).crafting_specific].name);
    break;
  case CRAFT_TYPE_ARMOR:
    snprintf(spec_item_type, sizeof(spec_item_type), "%s",
             armor_list[GET_CRAFT(ch).crafting_specific].name);
    break;
  case CRAFT_TYPE_INSTRUMENT:
    snprintf(spec_item_type, sizeof(spec_item_type), "%s",
             crafting_instrument_types[GET_CRAFT(ch).crafting_specific]);
    break;
  case CRAFT_TYPE_MISC:
    snprintf(spec_item_type, sizeof(spec_item_type), "%s",
             crafting_misc_types[GET_CRAFT(ch).crafting_specific]);
    break;
  }

  send_to_char(ch, "\tCCurrent Craft Project:\tn\r\n");
  send_to_char(ch, "\tc   GENERAL INFO\tn\r\n");
  send_to_char(ch, "-- craft type: %s\r\n", crafting_types[GET_CRAFT(ch).crafting_item_type]);
  send_to_char(ch, "-- item type : %s\r\n", spec_item_type);
  if (GET_CRAFT(ch).crafting_item_type && GET_CRAFT(ch).crafting_specific &&
      GET_CRAFT(ch).crafting_recipe)
    send_to_char(ch, "-- variant   : %s\r\n",
                 GET_CRAFT(ch).craft_variant == -1
                     ? "not-selected"
                     : crafting_recipes[GET_CRAFT(ch).crafting_recipe]
                           .variant_descriptions[GET_CRAFT(ch).craft_variant]);
  send_to_char(ch, "\r\n");
  send_to_char(ch, "\tc   DESCRIPTIONS: \tn\r\n");
  // send_to_char(ch, "-- keywords  : %s\r\n", GET_CRAFT(ch).keywords != NULL ? (strlen(GET_CRAFT(ch).keywords) < 5 ? " " : GET_CRAFT(ch).keywords) : " ");
  send_to_char(ch, "-- keywords  : %s\r\n",
               (!GET_CRAFT(ch).keywords || is_abbrev(GET_CRAFT(ch).keywords, "(null)"))
                   ? "not set"
                   : GET_CRAFT(ch).keywords);
  send_to_char(
      ch, "-- short desc: %s\r\n",
      (!GET_CRAFT(ch).short_description || is_abbrev(GET_CRAFT(ch).short_description, "(null)"))
          ? "not set"
          : GET_CRAFT(ch).short_description);
  send_to_char(
      ch, "-- room desc : %s\r\n",
      (!GET_CRAFT(ch).room_description || is_abbrev(GET_CRAFT(ch).room_description, "(null)"))
          ? "not set"
          : GET_CRAFT(ch).room_description);
  send_to_char(ch, "-- extra desc: %s\r\n",
               (strstr(extra_desc, "(null)")) ? "not set" : extra_desc);
  send_to_char(ch, "\r\n");
  send_to_char(ch, "\tc   MATERIALS: \tn\r\n");
  if (GET_CRAFT(ch).crafting_item_type && GET_CRAFT(ch).crafting_specific &&
      GET_CRAFT(ch).craft_variant != -1 && GET_CRAFT(ch).crafting_recipe)
  {
    base_group = crafting_recipes[GET_CRAFT(ch).crafting_recipe]
                     .materials[0][GET_CRAFT(ch).craft_variant][0];
    base_amount = crafting_recipes[GET_CRAFT(ch).crafting_recipe]
                      .materials[0][GET_CRAFT(ch).craft_variant][1];
    project_amount = GET_CRAFT(ch).materials[base_group][1];
    if (base_group != CRAFT_GROUP_NONE)
    {
      send_to_char(ch, "-- %d %-12s: %d %s allocated (%s%d level adjustment)\r\n", base_amount,
                   crafting_material_groups[base_group], project_amount,
                   project_material ? crafting_materials[project_material] : "units",
                   -craft_material_level_adjustment(project_material) > 0 ? "+" : "",
                   -craft_material_level_adjustment(project_material));
    }
    base_group = crafting_recipes[GET_CRAFT(ch).crafting_recipe]
                     .materials[1][GET_CRAFT(ch).craft_variant][0];
    base_amount = crafting_recipes[GET_CRAFT(ch).crafting_recipe]
                      .materials[1][GET_CRAFT(ch).craft_variant][1];
    project_amount = GET_CRAFT(ch).materials[base_group][1];
    if (base_group != CRAFT_GROUP_NONE)
    {
      send_to_char(ch, "-- %d %-12s: %d %s allocated (%s%d level adjustment)\r\n", base_amount,
                   crafting_material_groups[base_group], project_amount,
                   project_material ? crafting_materials[project_material] : "units",
                   - -craft_material_level_adjustment(project_material) > 0 ? "+" : "",
                   -craft_material_level_adjustment(project_material));
    }
    base_group = crafting_recipes[GET_CRAFT(ch).crafting_recipe]
                     .materials[2][GET_CRAFT(ch).craft_variant][0];
    base_amount = crafting_recipes[GET_CRAFT(ch).crafting_recipe]
                      .materials[2][GET_CRAFT(ch).craft_variant][1];
    project_amount = GET_CRAFT(ch).materials[base_group][1];
    if (base_group != CRAFT_GROUP_NONE)
    {
      send_to_char(ch, "-- %d %-12s: %d %s allocated (%s%d level adjustment)\r\n", base_amount,
                   crafting_material_groups[base_group], project_amount,
                   project_material ? crafting_materials[project_material] : "units",
                   -craft_material_level_adjustment(project_material) > 0 ? "+" : "",
                   -craft_material_level_adjustment(project_material));
    }
    send_to_char(
        ch, "-- Final level adjustment for material quality: %s%d. (average of all materials)\r\n",
        get_craft_material_final_level_adjustment(ch) > 0 ? "+" : "",
        get_craft_material_final_level_adjustment(ch));
  }
  else
  {
    send_to_char(ch,
                 "-- You must set craft type, item type and variant type to view materials.\r\n");
  }

  if (GET_CRAFT(ch).crafting_item_type == CRAFT_TYPE_INSTRUMENT)
  {
    send_to_char(ch, "\r\n");
    send_to_char(ch, "\tc   INSTRUMENT INFO:\tn\r\n");
    send_to_char(ch, "-- quality       : %d (motes: %2d/%2d %s%s)\r\n",
                 GET_CRAFT(ch).instrument_quality, GET_CRAFT(ch).instrument_motes[1],
                 get_crafting_instrument_motes(ch, 1, TRUE),
                 crafting_motes[get_crafting_instrument_motes(ch, 1, FALSE)],
                 get_crafting_instrument_motes(ch, 1, TRUE) == 1 ? "" : "s");
    send_to_char(ch, "-- effectiveness : %d (motes: %2d/%2d %s%s)\r\n",
                 GET_CRAFT(ch).instrument_effectiveness, GET_CRAFT(ch).instrument_motes[2],
                 get_crafting_instrument_motes(ch, 2, TRUE),
                 crafting_motes[get_crafting_instrument_motes(ch, 2, FALSE)],
                 get_crafting_instrument_motes(ch, 1, TRUE) == 2 ? "" : "s");
    send_to_char(ch, "-- breakability  : %d (motes: %2d/%2d %s%s)\r\n",
                 GET_CRAFT(ch).instrument_breakability, GET_CRAFT(ch).instrument_motes[3],
                 get_crafting_instrument_motes(ch, 3, TRUE),
                 crafting_motes[get_crafting_instrument_motes(ch, 3, FALSE)],
                 get_crafting_instrument_motes(ch, 1, TRUE) == 3 ? "" : "s");
  }

  if (GET_CRAFT(ch).crafting_item_type == CRAFT_TYPE_WEAPON ||
      GET_CRAFT(ch).crafting_item_type == CRAFT_TYPE_ARMOR)
  {
    send_to_char(ch, "\r\n");
    send_to_char(ch, "\tc   ENHANCEMENT BONUS:\tn %s%d", GET_CRAFT(ch).enhancement > 0 ? "+" : "",
                 GET_CRAFT(ch).enhancement);
    if (GET_CRAFT(ch).enhancement > 0)
      send_to_char(ch, " %d/%d %ss required", GET_CRAFT(ch).enhancement_motes_required,
                   craft_motes_required(0, 0, 0, GET_CRAFT(ch).enhancement),
                   crafting_motes[get_enhancement_mote_type(ch, GET_CRAFT(ch).crafting_item_type,
                                                            GET_CRAFT(ch).crafting_specific)]);
    send_to_char(ch, "\r\n");
    send_to_char(ch, "\r\n");
  }
  send_to_char(ch, "\r\n");
  send_to_char(ch, "\tc   BONUSES:\tn\r\n");

  for (i = 0; i < 6; i++)
  {
    if (GET_CRAFT(ch).affected[i].location != APPLY_NONE)
    {
      found = TRUE;
      snprintf(spectext, sizeof(spectext), " ");
      switch (GET_CRAFT(ch).affected[i].location)
      {
      case APPLY_FEAT:
        snprintf(spectext, sizeof(spectext), " [%s]",
                 feat_list[GET_CRAFT(ch).affected[i].specific].name);
        break;
      case APPLY_SKILL:
        snprintf(spectext, sizeof(spectext), " [%s]",
                 ability_names[GET_CRAFT(ch).affected[i].specific]);
        break;
      case APPLY_SPELL_CIRCLE_1:
      case APPLY_SPELL_CIRCLE_2:
      case APPLY_SPELL_CIRCLE_3:
      case APPLY_SPELL_CIRCLE_4:
      case APPLY_SPELL_CIRCLE_5:
      case APPLY_SPELL_CIRCLE_6:
      case APPLY_SPELL_CIRCLE_7:
      case APPLY_SPELL_CIRCLE_8:
      case APPLY_SPELL_CIRCLE_9:
        snprintf(spectext, sizeof(spectext), " [%s]",
                 class_list[GET_CRAFT(ch).affected[i].specific].name);
        break;
      }
      send_to_char(ch, "-- slot %d: +%d to %s%s (%s) %d/%d %ss required.\r\n", i + 1,
                   GET_CRAFT(ch).affected[i].modifier,
                   apply_types[GET_CRAFT(ch).affected[i].location], spectext,
                   bonus_types[GET_CRAFT(ch).affected[i].bonus_type],
                   GET_CRAFT(ch).motes_required[i],
                   craft_motes_required(GET_CRAFT(ch).affected[i].location,
                                        GET_CRAFT(ch).affected[i].modifier,
                                        GET_CRAFT(ch).affected[i].bonus_type, 0),
                   crafting_motes[crafting_mote_by_bonus_location(
                       GET_CRAFT(ch).affected[i].location, GET_CRAFT(ch).affected[i].specific,
                       GET_CRAFT(ch).affected[i].bonus_type)]);
    }
  }
  if (!found)
  {
    send_to_char(ch, "-- none\r\n");
  }

  send_to_char(ch, "\r\n");
  send_to_char(ch, "\tc   CATALYSTS:\tn\r\n");
  send_to_char(ch, "-- %d catalyst%s allocated (%d%% critical chance). You have %d stored.\r\n",
               GET_CRAFT(ch).catalysts_used,
               GET_CRAFT(ch).catalysts_used == 1 ? "" : "s",
               MIN(GET_CRAFT(ch).catalysts_used, CRAFT_CATALYST_MAX),
               GET_CRAFT_MAT(ch, CRAFT_MAT_CATALYST));

  if (GET_CRAFT(ch).crafting_item_type && GET_CRAFT(ch).crafting_specific)
  {
    spec_type = GET_CRAFT(ch).crafting_specific;
    if (GET_CRAFT(ch).crafting_item_type == CRAFT_TYPE_WEAPON)
    {
      setup_craft_weapon(ch, spec_type);
    }
    if (GET_CRAFT(ch).crafting_item_type == CRAFT_TYPE_ARMOR)
    {
      setup_craft_armor(ch, spec_type);
    }
    if (GET_CRAFT(ch).crafting_item_type == CRAFT_TYPE_MISC)
    {
      setup_craft_misc(ch, craft_misc_spec_to_vnum(spec_type));
    }
    if (GET_CRAFT(ch).crafting_item_type == CRAFT_TYPE_INSTRUMENT)
    {
      setup_craft_instrument(ch, spec_type);
    }
    skill = GET_CRAFT(ch).skill_type;
    dc = GET_CRAFT(ch).dc + get_craft_level_adjust_dc_change(GET_CRAFT(ch).level_adjust);
    send_to_char(ch, "\r\n");
    send_to_char(ch, "Skill Required: %s [rank %d].\r\n", ability_names[skill],
                 get_craft_skill_value(ch, skill));
    if (GET_CRAFT(ch).level_adjust)
      send_to_char(ch, "Level Adjust  : %s%d (%s%d to dc).\r\n",
                   GET_CRAFT(ch).level_adjust > 0 ? "+" : "", GET_CRAFT(ch).level_adjust,
                   get_craft_level_adjust_dc_change(GET_CRAFT(ch).level_adjust) > 0 ? "+" : "",
                   get_craft_level_adjust_dc_change(GET_CRAFT(ch).level_adjust));
    send_to_char(ch, "Project DC    : %d.\r\n", dc);
    send_to_char(ch, "Object Level  : %d.\r\n", GET_CRAFT(ch).obj_level);
    send_to_char(ch, "\r\n");
  }

  send_to_char(ch, "\r\n");

  if (!is_craft_ready(ch, FALSE))
  {
    send_to_char(ch, "\tRThis craft is not yet ready to begin. Type 'craft check' to see what's "
                     "missing.\tn\r\n");
  }
  else
  {
    send_to_char(ch, "\tGThis craft is ready to begin, though you still may want to add more to "
                     "it. Type 'craft start' to begin crafting.\tn\r\n");
  }

  send_to_char(ch, "\r\n");
}

void reset_craft_materials(struct char_data *ch, bool verbose, bool reimburse)
{
  int i = 0;

  // reimburse materials
  for (i = 1; i < NUM_CRAFT_GROUPS; i++)
  {
    if (GET_CRAFT(ch).materials[i][0] == 0 || GET_CRAFT(ch).materials[i][1] == 0)
      continue;
    if (reimburse)
    {
      if (verbose)
      {
        send_to_char(ch, "You have recovered %d unit%s of %s.\r\n", GET_CRAFT(ch).materials[i][1],
                     GET_CRAFT(ch).materials[i][1] > 0 ? "s" : "",
                     crafting_materials[GET_CRAFT(ch).materials[i][0]]);
      }
      GET_CRAFT_MAT(ch, GET_CRAFT(ch).materials[i][0]) += GET_CRAFT(ch).materials[i][1];
    }
    GET_CRAFT(ch).materials[i][0] = 0;
    GET_CRAFT(ch).materials[i][1] = 0;
  }
}

#define CR_RESET_ALL 0
#define CR_RESET_MOTES 1
#define CR_RESET_MATERIALS 2
#define CR_RESET_ENHANCEMENT 3
#define CR_RESET_INSTRUMENT 4
#define CR_RESET_BONUSES 5
#define CR_RESET_DESCRIPTIONS 6
#define CR_RESET_REFINE 7
#define CR_RESET_RESIZE 8
#define CR_RESET_CATALYSTS 9

void reset_current_craft(struct char_data *ch, char *arg2, bool verbose, bool reimburse)
{
  int i = 0, mote;
  int mode = 0;

  if (arg2 != NULL)
  {
    if (is_abbrev(arg2, "motes"))
      mode = CR_RESET_MOTES;
    else if (is_abbrev(arg2, "materials"))
      mode = CR_RESET_MATERIALS;
    else if (is_abbrev(arg2, "enhancement"))
      mode = CR_RESET_ENHANCEMENT;
    else if (is_abbrev(arg2, "instrument"))
      mode = CR_RESET_INSTRUMENT;
    else if (is_abbrev(arg2, "bonuses"))
      mode = CR_RESET_BONUSES;
    else if (is_abbrev(arg2, "descriptions"))
      mode = CR_RESET_DESCRIPTIONS;
    else if (is_abbrev(arg2, "refine"))
      mode = CR_RESET_REFINE;
    else if (is_abbrev(arg2, "resize"))
      mode = CR_RESET_RESIZE;
    else if (is_abbrev(arg2, "catalysts") || is_abbrev(arg2, "catalyst"))
      mode = CR_RESET_CATALYSTS;
  }

  // reimburse motes

  if (mode == CR_RESET_ALL || mode == CR_RESET_MOTES || mode == CR_RESET_ENHANCEMENT)
  {
    if (GET_CRAFT(ch).enhancement_motes_required > 0)
    {
      mote = get_enhancement_mote_type(ch, GET_CRAFT(ch).crafting_item_type,
                                       GET_CRAFT(ch).crafting_specific);
      if (mote != CRAFTING_MOTE_NONE && reimburse)
      {
        GET_CRAFT_MOTES(ch, mote) += GET_CRAFT(ch).enhancement_motes_required;
        if (verbose)
        {
          send_to_char(ch, "You have recovered %d %ss.\r\n",
                       GET_CRAFT(ch).enhancement_motes_required, crafting_motes[mote]);
        }
      }
      GET_CRAFT(ch).enhancement_motes_required = 0;
    }
  }

  if (mode == CR_RESET_ALL || mode == CR_RESET_ENHANCEMENT)
    GET_CRAFT(ch).enhancement = 0;

  if (mode == CR_RESET_ALL || mode == CR_RESET_MOTES || mode == CR_RESET_BONUSES)
  {
    for (i = 0; i < MAX_OBJ_AFFECT; i++)
    {
      if (GET_CRAFT(ch).motes_required[i] == 0)
        continue;
      mote = crafting_mote_by_bonus_location(GET_CRAFT(ch).affected[i].location,
                                             GET_CRAFT(ch).affected[i].specific,
                                             GET_CRAFT(ch).affected[i].bonus_type);
      if (mote != CRAFTING_MOTE_NONE && reimburse)
      {
        GET_CRAFT_MOTES(ch, mote) += GET_CRAFT(ch).motes_required[i];
        if (verbose)
        {
          send_to_char(ch, "You have recovered %d %ss.\r\n", GET_CRAFT(ch).motes_required[i],
                       crafting_motes[mote]);
        }
      }
      GET_CRAFT(ch).motes_required[i] = 0;
    }

    // bonuses / applies
    for (i = 0; i < MAX_OBJ_AFFECT; i++)
    {
      GET_CRAFT(ch).affected[i].location = 0;
      GET_CRAFT(ch).affected[i].modifier = 0;
      GET_CRAFT(ch).affected[i].bonus_type = 0;
      GET_CRAFT(ch).affected[i].specific = 0;
    }
  }

  if (mode == CR_RESET_ALL || mode == CR_RESET_MATERIALS || mode == CR_RESET_DESCRIPTIONS)
  {
    // reimburse materials
    reset_craft_materials(ch, verbose, reimburse);
  }

  if (mode == CR_RESET_ALL || mode == CR_RESET_CATALYSTS)
  {
    if (GET_CRAFT(ch).catalysts_used > 0)
    {
      if (reimburse)
      {
        GET_CRAFT_MAT(ch, CRAFT_MAT_CATALYST) += GET_CRAFT(ch).catalysts_used;
        if (verbose)
        {
          send_to_char(ch, "You have recovered %d catalyst%s.\r\n",
                       GET_CRAFT(ch).catalysts_used,
                       GET_CRAFT(ch).catalysts_used == 1 ? "" : "s");
        }
      }
      GET_CRAFT(ch).catalysts_used = 0;
      if (verbose && mode == CR_RESET_CATALYSTS)
        send_to_char(ch, "You have reset catalyst use to zero.\r\n");
    }
  }

  if (mode == CR_RESET_ALL || mode == CR_RESET_MATERIALS || mode == CR_RESET_REFINE)
  {
    for (i = 0; i < 3; i++)
    {
      if (GET_CRAFT(ch).refining_materials[i][1] > 0)
      {
        if (reimburse)
        {
          GET_CRAFT_MAT(ch, GET_CRAFT(ch).refining_materials[i][0]) +=
              GET_CRAFT(ch).refining_materials[i][1];
          if (verbose)
          {
            send_to_char(ch, "You have recovered %d %s.\r\n",
                         GET_CRAFT(ch).refining_materials[i][1],
                         crafting_materials[GET_CRAFT(ch).refining_materials[i][0]]);
          }
        }
        GET_CRAFT(ch).refining_materials[i][0] = GET_CRAFT(ch).refining_materials[i][1] = 0;
      }
    }

    GET_CRAFT(ch).refining_result[0] = GET_CRAFT(ch).refining_result[1] = 0;
    GET_CRAFT(ch).refining_batch_quantity = 1;
    if (verbose && mode != CR_RESET_ALL)
      send_to_char(ch, "You have reset refining values to the default.\r\n");
  }

  if (mode == CR_RESET_ALL || mode == CR_RESET_RESIZE)
  {
    if (GET_CRAFT(ch).new_size)
    {
      GET_CRAFT(ch).new_size = 0;
      reset_crafting_obj(ch);

      if (verbose && mode != CR_RESET_ALL)
        send_to_char(ch, "You have reset resizing values to the default.\r\n");
    }
  }

  if (mode == CR_RESET_ALL || mode == CR_RESET_INSTRUMENT || mode == CR_RESET_MOTES)
  {
    for (i = 0; i < 4; i++)
    {
      if (GET_CRAFT(ch).instrument_motes[i] > 0 && reimburse)
      {
        GET_CRAFT_MOTES(ch, get_crafting_instrument_motes(ch, i, FALSE)) +=
            GET_CRAFT(ch).instrument_motes[i];
        if (verbose)
        {
          send_to_char(ch, "You have recovered %d %s.\r\n", GET_CRAFT(ch).instrument_motes[i],
                       crafting_motes[get_crafting_instrument_motes(ch, i, FALSE)]);
        }
      }
      GET_CRAFT(ch).instrument_motes[i] = 0;
    }
  }

  if (mode == CR_RESET_INSTRUMENT || mode == CR_RESET_ALL)
  {
    GET_CRAFT(ch).instrument_quality = 0;
    GET_CRAFT(ch).instrument_effectiveness = 0;
    GET_CRAFT(ch).instrument_breakability = INSTRUMENT_BREAKABILITY_DEFAULT;
    if (verbose && mode != CR_RESET_ALL)
      send_to_char(ch, "You have reset instrument values to the default.\r\n");
  }

  if (mode == CR_RESET_ALL || mode == CR_RESET_DESCRIPTIONS || mode == CR_RESET_MATERIALS)
  {
    /* Free old strings before allocating new ones to prevent memory leaks */
    if (GET_CRAFT(ch).keywords)
      free(GET_CRAFT(ch).keywords);
    if (GET_CRAFT(ch).short_description)
      free(GET_CRAFT(ch).short_description);
    if (GET_CRAFT(ch).room_description)
      free(GET_CRAFT(ch).room_description);
    if (GET_CRAFT(ch).ex_description)
      free(GET_CRAFT(ch).ex_description);

    GET_CRAFT(ch).keywords = strdup("not set");
    GET_CRAFT(ch).short_description = strdup("not set");
    GET_CRAFT(ch).room_description = strdup("not set");
    GET_CRAFT(ch).ex_description = NULL;
    if (verbose && mode != CR_RESET_ALL)
      send_to_char(ch, "You have reset the descriptions to default values.\r\n");
  }

  if (mode == CR_RESET_ALL)
  {
    GET_CRAFT(ch).crafting_method = 0;
    GET_CRAFT(ch).crafting_item_type = 0;
    GET_CRAFT(ch).crafting_specific = 0;
    GET_CRAFT(ch).skill_type = 0;
    GET_CRAFT(ch).skill_roll = 0;
    GET_CRAFT(ch).dc = 0;
    GET_CRAFT(ch).craft_variant = -1;
    GET_CRAFT(ch).level_adjust = 0;
    GET_CRAFT(ch).catalysts_used = 0;
  }

  if (verbose)
  {
    send_to_char(ch, "Your project has been reset to default values. All materials and motes have "
                     "been refunded.\r\n");
  }
}
void reset_crafting_obj(struct char_data *ch)
{
  GET_CRAFT(ch).craft_obj_rnum = NOTHING;
}

bool is_craft_ready(struct char_data *ch, bool verbose)
{
  bool ready = TRUE;
  int i = 0;
  int required = 0;
  ;
  int location = 0, modifier = 0, bonus_type = 0, specific = 0;
  int base_group, base_amount, project_amount;

  if (verbose)
    send_to_char(ch, "\r\n");

  if (!GET_CRAFT(ch).keywords || is_abbrev(GET_CRAFT(ch).keywords, "(null)") ||
      !strcmp(GET_CRAFT(ch).keywords, "not set"))
  {
    ready = FALSE;
    if (verbose)
      send_to_char(ch, "There are no keywords set.\r\n");
  }
  if (!GET_CRAFT(ch).short_description || is_abbrev(GET_CRAFT(ch).short_description, "(null)") ||
      !strcmp(GET_CRAFT(ch).short_description, "not set"))
  {
    ready = FALSE;
    if (verbose)
      send_to_char(ch, "The short description is not set.\r\n");
  }
  if (!GET_CRAFT(ch).room_description || is_abbrev(GET_CRAFT(ch).room_description, "(null)") ||
      !strcmp(GET_CRAFT(ch).room_description, "not set"))
  {
    ready = FALSE;
    if (verbose)
      send_to_char(ch, "The item's room description is not set.\r\n");
  }
  if (GET_CRAFT(ch).crafting_item_type == 0)
  {
    ready = FALSE;
    if (verbose)
      send_to_char(ch, "The crafting item type is not set.\r\n");
  }
  if (GET_CRAFT(ch).crafting_specific == 0)
  {
    ready = FALSE;
    if (verbose)
      send_to_char(ch, "The crafting item specific type is not set.\r\n");
  }
  if (GET_CRAFT(ch).craft_variant == -1)
  {
    if (verbose)
      send_to_char(ch, "The crafting variant type is not set.\r\n");
  }

  if (GET_CRAFT(ch).crafting_item_type && GET_CRAFT(ch).crafting_specific &&
      GET_CRAFT(ch).craft_variant != -1 && GET_CRAFT(ch).crafting_recipe)
  {
    base_group = crafting_recipes[GET_CRAFT(ch).crafting_recipe]
                     .materials[0][GET_CRAFT(ch).craft_variant][0];
    base_amount = crafting_recipes[GET_CRAFT(ch).crafting_recipe]
                      .materials[0][GET_CRAFT(ch).craft_variant][1];
    project_amount = GET_CRAFT(ch).materials[base_group][1];
    if (base_group != CRAFT_GROUP_NONE)
    {
      if (project_amount < base_amount)
      {
        ready = FALSE;
        if (verbose)
          send_to_char(ch, "The project requires %d unit%s of %s allocated\r\n", base_amount,
                       base_amount == 1 ? "s" : "", crafting_material_groups[base_group]);
      }
    }
    base_group = crafting_recipes[GET_CRAFT(ch).crafting_recipe]
                     .materials[1][GET_CRAFT(ch).craft_variant][0];
    base_amount = crafting_recipes[GET_CRAFT(ch).crafting_recipe]
                      .materials[1][GET_CRAFT(ch).craft_variant][1];
    project_amount = GET_CRAFT(ch).materials[base_group][1];
    if (base_group != CRAFT_GROUP_NONE)
    {
      if (project_amount < base_amount)
      {
        ready = FALSE;
        if (verbose)
          send_to_char(ch, "The project requires %d unit%s of %s allocated\r\n", base_amount,
                       base_amount == 1 ? "s" : "", crafting_material_groups[base_group]);
      }
    }
    base_group = crafting_recipes[GET_CRAFT(ch).crafting_recipe]
                     .materials[2][GET_CRAFT(ch).craft_variant][0];
    base_amount = crafting_recipes[GET_CRAFT(ch).crafting_recipe]
                      .materials[2][GET_CRAFT(ch).craft_variant][1];
    project_amount = GET_CRAFT(ch).materials[base_group][1];
    if (base_group != CRAFT_GROUP_NONE)
    {
      if (project_amount < base_amount)
      {
        ready = FALSE;
        if (verbose)
          send_to_char(ch, "The project requires %d unit%s of %s allocated\r\n", base_amount,
                       base_amount == 1 ? "s" : "", crafting_material_groups[base_group]);
      }
    }
  }

  if (GET_CRAFT(ch).crafting_item_type == CRAFT_TYPE_INSTRUMENT)
  {
    if (GET_CRAFT(ch).instrument_quality > 0)
    {
      if (GET_CRAFT(ch).instrument_motes[1] != get_crafting_instrument_motes(ch, 1, TRUE))
      {
        ready = FALSE;
        if (verbose)
          send_to_char(ch, "The instrument quality requires %d %ss.\r\n",
                       get_crafting_instrument_motes(ch, 1, TRUE),
                       crafting_motes[get_crafting_instrument_motes(ch, 1, FALSE)]);
      }
    }
    if (GET_CRAFT(ch).instrument_effectiveness > 0)
    {
      if (GET_CRAFT(ch).instrument_motes[2] != get_crafting_instrument_motes(ch, 2, TRUE))
      {
        ready = FALSE;
        if (verbose)
          send_to_char(ch, "The instrument effectiveness requires %d %ss.\r\n",
                       get_crafting_instrument_motes(ch, 2, TRUE),
                       crafting_motes[get_crafting_instrument_motes(ch, 2, FALSE)]);
      }
    }
    if (GET_CRAFT(ch).instrument_breakability != INSTRUMENT_BREAKABILITY_DEFAULT)
    {
      if (GET_CRAFT(ch).instrument_motes[3] != get_crafting_instrument_motes(ch, 3, TRUE))
      {
        ready = FALSE;
        if (verbose)
          send_to_char(ch, "The instrument breakability requires %d %ss.\r\n",
                       get_crafting_instrument_motes(ch, 3, TRUE),
                       crafting_motes[get_crafting_instrument_motes(ch, 3, FALSE)]);
      }
    }
  }

  for (i = 0; i < MAX_OBJ_AFFECT; i++)
  {
    if (GET_CRAFT(ch).affected[i].location != APPLY_NONE || GET_CRAFT(ch).affected[i].modifier != 0)
    {
      if (GET_CRAFT(ch).motes_required[i] !=
          craft_motes_required(GET_CRAFT(ch).affected[i].location,
                               GET_CRAFT(ch).affected[i].modifier,
                               GET_CRAFT(ch).affected[i].bonus_type, 0))
      {
        ready = FALSE;
        if (verbose)
        {
          location = GET_CRAFT(ch).affected[i].location;
          modifier = GET_CRAFT(ch).affected[i].modifier;
          bonus_type = GET_CRAFT(ch).affected[i].bonus_type;
          specific = GET_CRAFT(ch).affected[i].specific;
          required = craft_motes_required(location, modifier, bonus_type, 0);
          send_to_char(
              ch, "The bonus in slot %d requires %d %ss.\r\n", i + 1, required,
              crafting_motes[crafting_mote_by_bonus_location(location, specific, bonus_type)]);
        }
      }
    }
  }

  if (GET_CRAFT(ch).enhancement > 0)
  {
    if (GET_CRAFT(ch).enhancement_motes_required <
        craft_motes_required(0, 0, 0, GET_CRAFT(ch).enhancement))
    {
      ready = FALSE;
      send_to_char(ch, "You require %d %ss for the object's enhancement bonus.\r\n",
                   craft_motes_required(0, 0, 0, GET_CRAFT(ch).enhancement),
                   crafting_motes[get_enhancement_mote_type(ch, GET_CRAFT(ch).crafting_item_type,
                                                            GET_CRAFT(ch).crafting_specific)]);
    }
  }

  if (GET_CRAFT(ch).catalysts_used < 0 || GET_CRAFT(ch).catalysts_used > CRAFT_CATALYST_MAX)
  {
    ready = FALSE;
    if (verbose)
      send_to_char(ch, "You can allocate between 0 and %d catalysts.\r\n", CRAFT_CATALYST_MAX);
  }
  else if (GET_CRAFT(ch).catalysts_used > 0 && !craft_project_has_catalyst_target(ch))
  {
    ready = FALSE;
    if (verbose)
    {
      send_to_char(ch, "Catalysts require a new item crafting project.\r\n");
    }
  }

  if (get_craft_project_level(ch) > 30)
  {
    send_to_char(ch, "The object level based on the existing bonuses and enhancement bonus "
                     "(weapons, armor, shields only) is too high.\r\n"
                     "You must downgrade the enhanceent bonus, some of the other bonuses or try "
                     "adding higher quality materials.\r\n");
    ready = FALSE;
  }

  // We need to check if they're wielding the proper crafting tool. There's a bit of setup to get the values we want.
  // It's dirty as heck... could use some optimization here in the future so there's fewer hoops to jump
  // through to get this information.
  int skill = 0, ability = 0, recipe = get_current_craft_project_recipe(ch);
  if (recipe == 0)
  {
    ready = FALSE;
    if (verbose)
      send_to_char(ch, "The crafting recipe is not set.\r\n");
  }
  else
  {
    skill = crafting_recipes[recipe].variant_skill[GET_CRAFT(ch).craft_variant];
  }
  if (skill == 0)
  {
    ready = FALSE;
    if (verbose)
      send_to_char(ch, "The crafting recipe does not have a valid skill associated with it.\r\n");
  }
  else
  {
    ability = recipe_skill_to_actual_crafting_skill(skill);
  }
  if (ability == 0)
  {
    ready = FALSE;
    if (verbose)
      send_to_char(ch, "The crafting recipe does not have a valid ability associated with it.\r\n");
  }
  else
  {
    if (!is_wearing_tool_for_crafting_ability(ch, ability))
    {
      ready = FALSE;
      if (verbose)
        send_to_char(ch, "You are not wearing the proper tool to craft this item.\r\n");
    }
  }

  return ready;
}

bool is_wearing_tool_for_crafting_ability(struct char_data *ch, int ability)
{
  if (!ch)
    return false;

  bool has_tool = FALSE;

  switch (ability)
  {
  case ABILITY_CRAFT_TAILORING:
    has_tool = GET_EQ(ch, WEAR_CRAFT_NEEDLE);
    break;
  case ABILITY_CRAFT_ALCHEMY:
    has_tool = GET_EQ(ch, WEAR_CRAFT_ALCHEMY);
    break;
  case ABILITY_CRAFT_ARMORSMITHING:
    has_tool = GET_EQ(ch, WEAR_CRAFT_ARMOR_HAMMER);
    break;
  case ABILITY_CRAFT_WEAPONSMITHING:
    has_tool = GET_EQ(ch, WEAR_CRAFT_WEAPON_HAMMER);
    break;
  case ABILITY_CRAFT_JEWELCRAFTING:
    has_tool = GET_EQ(ch, WEAR_CRAFT_JEWEL_PLIERS);
    break;
  }
  return has_tool;
}

static bool artificer_efficient_crafter_applies(struct char_data *ch)
{
  return ch && !IS_NPC(ch) && has_artificer_efficient_crafter(ch) &&
         GET_CRAFT(ch).crafting_recipe > CRAFT_RECIPE_NONE &&
         GET_CRAFT(ch).crafting_item_type != CRAFT_TYPE_GOLEM;
}

void begin_current_craft(struct char_data *ch)
{
  if (!is_craft_ready(ch, TRUE))
  {
    send_to_char(ch, "\tCPlease fix the above errors before continuing.\tn\r\n");
    return;
  }

  // Check if the player is in a room with the required crafting station
  int skill = GET_CRAFT(ch).skill_type;
  if (!has_crafting_station_in_room(ch, skill))
  {
    send_to_char(ch, "You need to be in a room with %s to craft this item.\r\n",
                 get_crafting_station_name(skill));
    return;
  }

  int seconds = CREATE_BASE_TIME;
  int rapid_reduction = get_rapid_talent_bonus(ch, skill);

  /* Apply rapid talent reduction, but don't go below 1 second */
  seconds -= rapid_reduction;
  if (artificer_efficient_crafter_applies(ch))
    seconds = (seconds * 90) / 100;
  if (seconds < 1)
    seconds = 1;

  GET_CRAFT(ch).craft_duration = seconds;
  GET_CRAFT(ch).crafting_method = SCMD_NEWCRAFT_CREATE;

  send_to_char(ch,
               "You begin creating %s. This will take a total of %d minutes and %d seconds.\r\n",
               GET_CRAFT(ch).short_description, seconds / 60, seconds % 60);
  act("$n starts crafting.", FALSE, ch, 0, 0, TO_ROOM);
}

void set_craft_item_descs(struct char_data *ch, struct obj_data *obj)
{
  obj->name = strdup(GET_CRAFT(ch).keywords);
  obj->short_description = strdup(GET_CRAFT(ch).short_description);
  obj->description = strdup(GET_CRAFT(ch).room_description);
  if (GET_CRAFT(ch).ex_description != NULL)
  {
    struct extra_descr_data *new_descr;
    char extra[MAX_EXTRA_DESC + 10];
    CREATE(new_descr, struct extra_descr_data, 1);
    new_descr->keyword = strdup(obj->name);
    snprintf(extra, sizeof(extra), "%s\n", GET_CRAFT(ch).ex_description);
    new_descr->description = strdup(extra);
    new_descr->next = obj->ex_description;
    obj->ex_description = new_descr;
  }
}

void set_craft_item_affects(struct char_data *ch, struct obj_data *obj)
{
  int i = 0;

  for (i = 0; i < MAX_OBJ_AFFECT; i++)
  {
    if (GET_CRAFT(ch).affected[i].location != APPLY_NONE)
    {
      obj->affected[i].location = GET_CRAFT(ch).affected[i].location;
      obj->affected[i].modifier = GET_CRAFT(ch).affected[i].modifier;
      obj->affected[i].bonus_type = GET_CRAFT(ch).affected[i].bonus_type;
      obj->affected[i].specific = GET_CRAFT(ch).affected[i].specific;
    }
  }
}

void set_craft_item_flags(struct char_data *ch, struct obj_data *obj)
{
  SET_OBJ_FLAG(obj, ITEM_CRAFTED);
  SET_OBJ_FLAG(obj, ITEM_IDENTIFIED);
  REMOVE_OBJ_FLAG(obj, ITEM_MOLD);
}

int material_to_craft_skill(int item_type, int material)
{
  switch (item_type)
  {
  case ITEM_WEAPON:
    if (IS_WOOD(material))
      return ABILITY_CRAFT_WOODWORKING;
    else if (IS_LEATHER(material))
      return ABILITY_CRAFT_LEATHERWORKING;
    else
      return ABILITY_CRAFT_WEAPONSMITHING;

  case ITEM_ARMOR:
    if (IS_CLOTH(material))
      return ABILITY_CRAFT_TAILORING;
    else if (IS_LEATHER(material))
      return ABILITY_CRAFT_LEATHERWORKING;
    else
      return ABILITY_CRAFT_ARMORSMITHING;

  case ITEM_WORN:
    if (IS_CLOTH(material))
      return ABILITY_CRAFT_TAILORING;
    else if (IS_LEATHER(material))
      return ABILITY_CRAFT_LEATHERWORKING;
    else
      return ABILITY_CRAFT_JEWELCRAFTING;
    break;

  case ITEM_INSTRUMENT:
    if (IS_WOOD(material))
      return ABILITY_CRAFT_WOODWORKING;
    else if (IS_LEATHER(material))
      return ABILITY_CRAFT_LEATHERWORKING;
    else if (IS_PRECIOUS_METAL(material))
      return ABILITY_CRAFT_JEWELCRAFTING;
    else
      return ABILITY_CRAFT_METALWORKING;
    break;
  }
  return ABILITY_CRAFT_METALWORKING;
}

/* Get the proficient talent bonus for a given crafting/harvesting skill */
int get_proficient_talent_bonus(struct char_data *ch, int skill)
{
  int talent = TALENT_NONE;

  /* Map skill to corresponding proficient talent */
  switch (skill)
  {
  case ABILITY_CRAFT_WOODWORKING:
    talent = TALENT_PROFICIENT_WOODWORKING;
    break;
  case ABILITY_CRAFT_TAILORING:
    talent = TALENT_PROFICIENT_TAILORING;
    break;
  case ABILITY_CRAFT_ALCHEMY:
    talent = TALENT_PROFICIENT_ALCHEMY;
    break;
  case ABILITY_CRAFT_ARMORSMITHING:
    talent = TALENT_PROFICIENT_ARMORSMITHING;
    break;
  case ABILITY_CRAFT_WEAPONSMITHING:
    talent = TALENT_PROFICIENT_WEAPONSMITHING;
    break;
  case ABILITY_CRAFT_BOWMAKING:
    talent = TALENT_PROFICIENT_BOWMAKING;
    break;
  case ABILITY_CRAFT_JEWELCRAFTING:
    talent = TALENT_PROFICIENT_JEWELCRAFTING;
    break;
  case ABILITY_CRAFT_LEATHERWORKING:
    talent = TALENT_PROFICIENT_LEATHERWORKING;
    break;
  case ABILITY_CRAFT_TRAPMAKING:
    talent = TALENT_PROFICIENT_TRAPMAKING;
    break;
  case ABILITY_CRAFT_POISONMAKING:
    talent = TALENT_PROFICIENT_POISONMAKING;
    break;
  case ABILITY_CRAFT_METALWORKING:
    talent = TALENT_PROFICIENT_METALWORKING;
    break;
  case ABILITY_CRAFT_FISHING:
    talent = TALENT_PROFICIENT_FISHING;
    break;
  case ABILITY_CRAFT_COOKING:
    talent = TALENT_PROFICIENT_COOKING;
    break;
  case ABILITY_HARVEST_MINING:
    talent = TALENT_PROFICIENT_MINING;
    break;
  case ABILITY_HARVEST_HUNTING:
    talent = TALENT_PROFICIENT_HUNTING;
    break;
  case ABILITY_HARVEST_FORESTRY:
    talent = TALENT_PROFICIENT_FORESTRY;
    break;
  case ABILITY_HARVEST_GATHERING:
    talent = TALENT_PROFICIENT_GATHERING;
    break;
  case ABILITY_HARVEST_BUTCHERING:
    talent = TALENT_PROFICIENT_BUTCHERING;
    break;
  default:
    return 0;
  }

  if (talent == TALENT_NONE)
    return 0;

  /* Each rank gives +1 bonus */
  return get_talent_rank(ch, talent);
}

/* Get the rapid talent time reduction for a given crafting/harvesting skill */
int get_rapid_talent_bonus(struct char_data *ch, int skill)
{
  int talent = TALENT_NONE;
  int seconds_per_rank = 0;

  /* Map skill to corresponding rapid talent */
  switch (skill)
  {
  case ABILITY_CRAFT_WOODWORKING:
    talent = TALENT_RAPID_WOODWORKING;
    seconds_per_rank = 2;
    break;
  case ABILITY_CRAFT_TAILORING:
    talent = TALENT_RAPID_TAILORING;
    seconds_per_rank = 2;
    break;
  case ABILITY_CRAFT_ALCHEMY:
    talent = TALENT_RAPID_ALCHEMY;
    seconds_per_rank = 2;
    break;
  case ABILITY_CRAFT_ARMORSMITHING:
    talent = TALENT_RAPID_ARMORSMITHING;
    seconds_per_rank = 2;
    break;
  case ABILITY_CRAFT_WEAPONSMITHING:
    talent = TALENT_RAPID_WEAPONSMITHING;
    seconds_per_rank = 2;
    break;
  case ABILITY_CRAFT_BOWMAKING:
    talent = TALENT_RAPID_BOWMAKING;
    seconds_per_rank = 2;
    break;
  case ABILITY_CRAFT_JEWELCRAFTING:
    talent = TALENT_RAPID_JEWELCRAFTING;
    seconds_per_rank = 2;
    break;
  case ABILITY_CRAFT_LEATHERWORKING:
    talent = TALENT_RAPID_LEATHERWORKING;
    seconds_per_rank = 2;
    break;
  case ABILITY_CRAFT_TRAPMAKING:
    talent = TALENT_RAPID_TRAPMAKING;
    seconds_per_rank = 2;
    break;
  case ABILITY_CRAFT_POISONMAKING:
    talent = TALENT_RAPID_POISONMAKING;
    seconds_per_rank = 2;
    break;
  case ABILITY_CRAFT_METALWORKING:
    talent = TALENT_RAPID_METALWORKING;
    seconds_per_rank = 2;
    break;
  case ABILITY_CRAFT_FISHING:
    talent = TALENT_RAPID_FISHING;
    seconds_per_rank = 2;
    break;
  case ABILITY_CRAFT_COOKING:
    talent = TALENT_RAPID_COOKING;
    seconds_per_rank = 2;
    break;
  case ABILITY_HARVEST_MINING:
    talent = TALENT_RAPID_MINING;
    seconds_per_rank = 1;
    break;
  case ABILITY_HARVEST_HUNTING:
    talent = TALENT_RAPID_HUNTING;
    seconds_per_rank = 1;
    break;
  case ABILITY_HARVEST_FORESTRY:
    talent = TALENT_RAPID_FORESTRY;
    seconds_per_rank = 1;
    break;
  case ABILITY_HARVEST_GATHERING:
    talent = TALENT_RAPID_GATHERING;
    seconds_per_rank = 1;
    break;
  case ABILITY_HARVEST_BUTCHERING:
    talent = TALENT_RAPID_BUTCHERING;
    seconds_per_rank = 1;
    break;
  default:
    return 0;
  }

  if (talent == TALENT_NONE)
    return 0;

  /* Each rank reduces time by seconds_per_rank (2 for crafting, 1 for harvesting) */
  return get_talent_rank(ch, talent) * seconds_per_rank;
}

/* Get the insightful talent experience bonus percentage for a given crafting/harvesting skill */
int get_insightful_talent_bonus(struct char_data *ch, int skill)
{
  int talent = TALENT_NONE;

  /* Map skill to corresponding insightful talent */
  switch (skill)
  {
  case ABILITY_CRAFT_WOODWORKING:
    talent = TALENT_INSIGHTFUL_WOODWORKING;
    break;
  case ABILITY_CRAFT_TAILORING:
    talent = TALENT_INSIGHTFUL_TAILORING;
    break;
  case ABILITY_CRAFT_ALCHEMY:
    talent = TALENT_INSIGHTFUL_ALCHEMY;
    break;
  case ABILITY_CRAFT_ARMORSMITHING:
    talent = TALENT_INSIGHTFUL_ARMORSMITHING;
    break;
  case ABILITY_CRAFT_WEAPONSMITHING:
    talent = TALENT_INSIGHTFUL_WEAPONSMITHING;
    break;
  case ABILITY_CRAFT_BOWMAKING:
    talent = TALENT_INSIGHTFUL_BOWMAKING;
    break;
  case ABILITY_CRAFT_JEWELCRAFTING:
    talent = TALENT_INSIGHTFUL_JEWELCRAFTING;
    break;
  case ABILITY_CRAFT_LEATHERWORKING:
    talent = TALENT_INSIGHTFUL_LEATHERWORKING;
    break;
  case ABILITY_CRAFT_TRAPMAKING:
    talent = TALENT_INSIGHTFUL_TRAPMAKING;
    break;
  case ABILITY_CRAFT_POISONMAKING:
    talent = TALENT_INSIGHTFUL_POISONMAKING;
    break;
  case ABILITY_CRAFT_METALWORKING:
    talent = TALENT_INSIGHTFUL_METALWORKING;
    break;
  case ABILITY_CRAFT_FISHING:
    talent = TALENT_INSIGHTFUL_FISHING;
    break;
  case ABILITY_CRAFT_COOKING:
    talent = TALENT_INSIGHTFUL_COOKING;
    break;
  case ABILITY_HARVEST_MINING:
    talent = TALENT_INSIGHTFUL_MINING;
    break;
  case ABILITY_HARVEST_HUNTING:
    talent = TALENT_INSIGHTFUL_HUNTING;
    break;
  case ABILITY_HARVEST_FORESTRY:
    talent = TALENT_INSIGHTFUL_FORESTRY;
    break;
  case ABILITY_HARVEST_GATHERING:
    talent = TALENT_INSIGHTFUL_GATHERING;
    break;
  case ABILITY_HARVEST_BUTCHERING:
    talent = TALENT_INSIGHTFUL_BUTCHERING;
    break;
  default:
    return 0;
  }

  if (talent == TALENT_NONE)
    return 0;

  /* Each rank gives 5% bonus experience */
  return get_talent_rank(ch, talent) * 5;
}

/* Get the efficient talent bonus percentage for a given crafting/harvesting skill */
int get_efficient_talent_bonus(struct char_data *ch, int skill)
{
  int talent = TALENT_NONE;

  /* Map skill to corresponding efficient talent */
  switch (skill)
  {
  case ABILITY_CRAFT_WOODWORKING:
    talent = TALENT_EFFICIENT_WOODWORKING;
    break;
  case ABILITY_CRAFT_TAILORING:
    talent = TALENT_EFFICIENT_TAILORING;
    break;
  case ABILITY_CRAFT_ALCHEMY:
    talent = TALENT_EFFICIENT_ALCHEMY;
    break;
  case ABILITY_CRAFT_ARMORSMITHING:
    talent = TALENT_EFFICIENT_ARMORSMITHING;
    break;
  case ABILITY_CRAFT_WEAPONSMITHING:
    talent = TALENT_EFFICIENT_WEAPONSMITHING;
    break;
  case ABILITY_CRAFT_BOWMAKING:
    talent = TALENT_EFFICIENT_BOWMAKING;
    break;
  case ABILITY_CRAFT_JEWELCRAFTING:
    talent = TALENT_EFFICIENT_JEWELCRAFTING;
    break;
  case ABILITY_CRAFT_LEATHERWORKING:
    talent = TALENT_EFFICIENT_LEATHERWORKING;
    break;
  case ABILITY_CRAFT_TRAPMAKING:
    talent = TALENT_EFFICIENT_TRAPMAKING;
    break;
  case ABILITY_CRAFT_POISONMAKING:
    talent = TALENT_EFFICIENT_POISONMAKING;
    break;
  case ABILITY_CRAFT_METALWORKING:
    talent = TALENT_EFFICIENT_METALWORKING;
    break;
  case ABILITY_CRAFT_FISHING:
    talent = TALENT_EFFICIENT_FISHING;
    break;
  case ABILITY_CRAFT_COOKING:
    talent = TALENT_EFFICIENT_COOKING;
    break;
  case ABILITY_HARVEST_MINING:
    talent = TALENT_EFFICIENT_MINING;
    break;
  case ABILITY_HARVEST_HUNTING:
    talent = TALENT_EFFICIENT_HUNTING;
    break;
  case ABILITY_HARVEST_FORESTRY:
    talent = TALENT_EFFICIENT_FORESTRY;
    break;
  case ABILITY_HARVEST_GATHERING:
    talent = TALENT_EFFICIENT_GATHERING;
    break;
  case ABILITY_HARVEST_BUTCHERING:
    talent = TALENT_EFFICIENT_BUTCHERING;
    break;
  default:
    return 0;
  }

  if (talent == TALENT_NONE)
    return 0;

  /* Each rank gives 3% chance */
  return get_talent_rank(ch, talent) * 3;
}

/* Return saved materials from efficient talent upon successful crafting completion */
void return_efficient_saved_materials(struct char_data *ch)
{
  int i;
  int total_saved = 0;

  for (i = 0; i < NUM_CRAFT_GROUPS; i++)
  {
    if (GET_CRAFT(ch).efficient_saved_materials[i][1] > 0)
    {
      GET_CRAFT_MAT(ch, GET_CRAFT(ch).efficient_saved_materials[i][0]) +=
          GET_CRAFT(ch).efficient_saved_materials[i][1];
      total_saved += GET_CRAFT(ch).efficient_saved_materials[i][1];
      /* Clear the saved materials */
      GET_CRAFT(ch).efficient_saved_materials[i][0] = 0;
      GET_CRAFT(ch).efficient_saved_materials[i][1] = 0;
    }
  }

  if (total_saved > 0)
  {
    send_to_char(ch,
                 "\tC*EFFICIENT*\tn Your efficient crafting has saved %d unit%s of materials!\r\n",
                 total_saved, total_saved == 1 ? "" : "s");
  }
}

bool create_craft_skill_check(struct char_data *ch, struct obj_data *obj, int skill, char *method,
                              int exp, int dc)
{
  if (!ch || !obj)
    return FALSE;
  int roll, skill_mod, proficiency_bonus = 0;
  int base_skill = GET_ABILITY(ch, skill);
  int specialization_bonus = 0;
  int elbow_grease_bonus = 0;

  roll = d20(ch);
  skill_mod = get_craft_skill_value(ch, skill);
  
  /* Check if specialized for message display */
  if (GET_CRAFT(ch).craft_specialization[0] == skill || 
      GET_CRAFT(ch).craft_specialization[1] == skill)
  {
    specialization_bonus = 5;
  }

  /* Add proficient talent bonus */
  proficiency_bonus += get_proficient_talent_bonus(ch, skill);
  skill_mod += proficiency_bonus;

  /* Add +5 bonus for Craft Wondrous Item feat when crafting misc items */
  if (HAS_FEAT(ch, FEAT_CRAFT_WONDEROUS_ITEM) &&
      GET_CRAFT(ch).crafting_item_type == CRAFT_TYPE_MISC)
  {
    skill_mod += 5;
  }

  /* Add +5 bonus for Craft Magical Arms and Armor feat when crafting weapons/armor */
  if (HAS_FEAT(ch, FEAT_CRAFT_MAGICAL_ARMS_AND_ARMOR) &&
      (GET_CRAFT(ch).crafting_item_type == CRAFT_TYPE_WEAPON ||
       GET_CRAFT(ch).crafting_item_type == CRAFT_TYPE_ARMOR))
  {
    skill_mod += 5;
  }

  /* Add elbow grease bonus for artificers */
  if (HAS_FEAT(ch, FEAT_ELBOW_GREASE))
  {
    int artificer_level = CLASS_LEVEL(ch, CLASS_ARTIFICER);
    if (artificer_level >= 10)
      elbow_grease_bonus = 6;
    else if (artificer_level >= 5)
      elbow_grease_bonus = 4;
    else
      elbow_grease_bonus = 2;
    
    skill_mod += elbow_grease_bonus;
  }

  if (artificer_efficient_crafter_applies(ch))
    skill_mod += 2;

  if ((20 + skill_mod) < dc)
  {
    send_to_char(ch, "You don't have the skill to craft %s.\r\n", obj->short_description);
    return FALSE;
  }

  // critical failure. Lose the item, materials and motes.
  if (roll == 1)
  {
    send_to_char(ch,
                 "\tM[CRITICAL FAILURE]\tn You rolled a natural 1! The %s failed and you lost your "
                 "materials and motes.\r\n",
                 method);
    reset_current_craft(ch, NULL, FALSE, FALSE);
    return FALSE;
  }
  // critical success. Item is masterwork quality.
  else if (roll == 20)
  {
    send_to_char(ch,
                 "\tM[CRITICAL SUCCESS]\tn You rolled a natural 20! The %s succeeded and is of "
                 "masterwork quality.\r\n",
                 method);
    SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_MASTERWORK);
    return TRUE;
  }
  else if ((roll + skill_mod) < dc)
  {
    char bonus_text[256];
    char *ptr = bonus_text;
    
    if (specialization_bonus > 0)
      ptr += snprintf(ptr, sizeof(bonus_text) - (ptr - bonus_text), " + specialization [%d]", specialization_bonus);
    if (proficiency_bonus > 0)
      ptr += snprintf(ptr, sizeof(bonus_text) - (ptr - bonus_text), " + proficiency [%d]", proficiency_bonus);
    
    if (HAS_FEAT(ch, FEAT_CRAFT_WONDEROUS_ITEM) &&
        GET_CRAFT(ch).crafting_item_type == CRAFT_TYPE_MISC)
    {
      ptr += snprintf(ptr, sizeof(bonus_text) - (ptr - bonus_text), " + Craft Wondrous Item [5]");
    }
    else if (HAS_FEAT(ch, FEAT_CRAFT_MAGICAL_ARMS_AND_ARMOR) &&
             (GET_CRAFT(ch).crafting_item_type == CRAFT_TYPE_WEAPON ||
              GET_CRAFT(ch).crafting_item_type == CRAFT_TYPE_ARMOR))
    {
      ptr += snprintf(ptr, sizeof(bonus_text) - (ptr - bonus_text), " + Craft Magical Arms and Armor [5]");
    }
    
    if (elbow_grease_bonus > 0)
      ptr += snprintf(ptr, sizeof(bonus_text) - (ptr - bonus_text), " + elbow grease [%d]", elbow_grease_bonus);

    send_to_char(ch,
                 "You rolled %d + base skill %d%s = total of %d vs. dc %d. The %s "
                 "attempt failed, but you may try again.\r\n",
                 roll, base_skill, bonus_text,
                 roll + skill_mod, dc, method);
    gain_craft_exp(ch, exp, skill, TRUE);
    return FALSE;
  }
  else
  {
    char bonus_text[256];
    char *ptr = bonus_text;
    
    if (specialization_bonus > 0)
      ptr += snprintf(ptr, sizeof(bonus_text) - (ptr - bonus_text), " + specialization [%d]", specialization_bonus);
    if (proficiency_bonus > 0)
      ptr += snprintf(ptr, sizeof(bonus_text) - (ptr - bonus_text), " + proficiency [%d]", proficiency_bonus);
    
    if (HAS_FEAT(ch, FEAT_CRAFT_WONDEROUS_ITEM) &&
        GET_CRAFT(ch).crafting_item_type == CRAFT_TYPE_MISC)
    {
      ptr += snprintf(ptr, sizeof(bonus_text) - (ptr - bonus_text), " + Craft Wondrous Item [5]");
    }
    else if (HAS_FEAT(ch, FEAT_CRAFT_MAGICAL_ARMS_AND_ARMOR) &&
             (GET_CRAFT(ch).crafting_item_type == CRAFT_TYPE_WEAPON ||
              GET_CRAFT(ch).crafting_item_type == CRAFT_TYPE_ARMOR))
    {
      ptr += snprintf(ptr, sizeof(bonus_text) - (ptr - bonus_text), " + Craft Magical Arms and Armor [5]");
    }
    
    if (elbow_grease_bonus > 0)
      ptr += snprintf(ptr, sizeof(bonus_text) - (ptr - bonus_text), " + elbow grease [%d]", elbow_grease_bonus);

    send_to_char(ch,
                 "You rolled %d + base skill %d%s = total of %d vs. dc %d. The %s "
                 "attempt succeeded!\r\n",
                 roll, base_skill, bonus_text,
                 roll + skill_mod, dc, method);
    return TRUE;
  }
  return FALSE;
}

int obj_material_to_craft_material(int material)
{
  switch (material)
  {
  case MATERIAL_COPPER:
    return CRAFT_MAT_COPPER;
  case MATERIAL_TIN:
    return CRAFT_MAT_TIN;
  case MATERIAL_BRONZE:
    return CRAFT_MAT_BRONZE;
  case MATERIAL_IRON:
    return CRAFT_MAT_IRON;
  case MATERIAL_COAL:
    return CRAFT_MAT_COAL;
  case MATERIAL_STEEL:
    return CRAFT_MAT_STEEL;
  case MATERIAL_COLD_IRON:
    return CRAFT_MAT_COLD_IRON;
  case MATERIAL_ALCHEMAL_SILVER:
    return CRAFT_MAT_ALCHEMAL_SILVER;
  case MATERIAL_MITHRIL:
    return CRAFT_MAT_MITHRIL;
  case MATERIAL_ADAMANTINE:
    return CRAFT_MAT_ADAMANTINE;
  case MATERIAL_SILVER:
    return CRAFT_MAT_SILVER;
  case MATERIAL_GOLD:
    return CRAFT_MAT_GOLD;
  case MATERIAL_PLATINUM:
    return CRAFT_MAT_PLATINUM;
  case MATERIAL_DRAGONMETAL:
    return CRAFT_MAT_DRAGONMETAL;
  case MATERIAL_DRAGONSCALE:
  case MATERIAL_DRAGONHIDE:
    return CRAFT_MAT_DRAGONSCALE;
  case MATERIAL_DRAGONBONE:
    return CRAFT_MAT_DRAGONBONE;
  case MATERIAL_LEATHER:
    return CRAFT_MAT_LOW_GRADE_HIDE;
  case MATERIAL_ASH:
    return CRAFT_MAT_ASH_WOOD;
  case MATERIAL_MAPLE:
    return CRAFT_MAT_MAPLE_WOOD;
  case MATERIAL_MAHAGONY:
    return CRAFT_MAT_MAHAGONY_WOOD;
  case MATERIAL_VALENWOOD:
    return CRAFT_MAT_VALENWOOD;
  case MATERIAL_IRONWOOD:
    return CRAFT_MAT_IRONWOOD;
  case MATERIAL_HEMP:
    return CRAFT_MAT_HEMP;
  case MATERIAL_WOOL:
    return CRAFT_MAT_WOOL;
  case MATERIAL_LINEN:
    return CRAFT_MAT_LINEN;
  case MATERIAL_SATIN:
    return CRAFT_MAT_SATIN;
  case MATERIAL_SILK:
    return CRAFT_MAT_SILK;
  case MATERIAL_ZINC:
    return CRAFT_MAT_ZINC;
  case MATERIAL_COTTON:
    return CRAFT_MAT_COTTON;
  case MATERIAL_BRASS:
    return CRAFT_MAT_BRASS;
  case MATERIAL_FLAX:
    return CRAFT_MAT_FLAX;
  case MATERIAL_BONE:
    return CRAFT_MAT_BONE;
  case MATERIAL_STONE:
    return CRAFT_MAT_STONE;
  case MATERIAL_DRAGONBLOOD:
    return CRAFT_MAT_DRAGONBLOOD;
  }
  return CRAFT_MAT_NONE;
}
int craft_material_to_obj_material(int craftmat)
{
  switch (craftmat)
  {
  case CRAFT_MAT_COPPER:
    return MATERIAL_COPPER;
  case CRAFT_MAT_TIN:
    return MATERIAL_TIN;
  case CRAFT_MAT_BRONZE:
    return MATERIAL_BRONZE;
  case CRAFT_MAT_IRON:
    return MATERIAL_IRON;
  case CRAFT_MAT_COAL:
    return MATERIAL_COAL;
  case CRAFT_MAT_STEEL:
    return MATERIAL_STEEL;
  case CRAFT_MAT_COLD_IRON:
    return MATERIAL_COLD_IRON;
  case CRAFT_MAT_ALCHEMAL_SILVER:
    return MATERIAL_ALCHEMAL_SILVER;
  case CRAFT_MAT_MITHRIL:
    return MATERIAL_MITHRIL;
  case CRAFT_MAT_ADAMANTINE:
    return MATERIAL_ADAMANTINE;
  case CRAFT_MAT_SILVER:
    return MATERIAL_SILVER;
  case CRAFT_MAT_GOLD:
    return MATERIAL_GOLD;
  case CRAFT_MAT_PLATINUM:
    return MATERIAL_PLATINUM;
  case CRAFT_MAT_DRAGONMETAL:
    return MATERIAL_DRAGONMETAL;
  case CRAFT_MAT_DRAGONSCALE:
    return MATERIAL_DRAGONSCALE;
  case CRAFT_MAT_DRAGONBONE:
    return MATERIAL_DRAGONBONE;
  case CRAFT_MAT_LOW_GRADE_HIDE:
    return MATERIAL_LEATHER;
  case CRAFT_MAT_MEDIUM_GRADE_HIDE:
    return MATERIAL_LEATHER;
  case CRAFT_MAT_HIGH_GRADE_HIDE:
    return MATERIAL_LEATHER;
  case CRAFT_MAT_PRISTINE_GRADE_HIDE:
    return MATERIAL_LEATHER;
  case CRAFT_MAT_DRAGONBLOOD:
    return MATERIAL_DRAGONBLOOD;
  case CRAFT_MAT_ASH_WOOD:
    return MATERIAL_ASH;
  case CRAFT_MAT_MAPLE_WOOD:
    return MATERIAL_MAPLE;
  case CRAFT_MAT_MAHAGONY_WOOD:
    return MATERIAL_MAHAGONY;
  case CRAFT_MAT_VALENWOOD:
    return MATERIAL_VALENWOOD;
  case CRAFT_MAT_IRONWOOD:
    return MATERIAL_IRONWOOD;
  case CRAFT_MAT_HEMP:
    return MATERIAL_HEMP;
  case CRAFT_MAT_WOOL:
    return MATERIAL_WOOL;
  case CRAFT_MAT_LINEN:
    return MATERIAL_LINEN;
  case CRAFT_MAT_SATIN:
    return MATERIAL_SATIN;
  case CRAFT_MAT_SILK:
    return MATERIAL_SILK;
  case CRAFT_MAT_ZINC:
    return MATERIAL_ZINC;
  case CRAFT_MAT_COTTON:
    return MATERIAL_COTTON;
  case CRAFT_MAT_BRASS:
    return MATERIAL_BRASS;
  case CRAFT_MAT_FLAX:
    return MATERIAL_FLAX;
  case CRAFT_MAT_BONE:
    return MATERIAL_BONE;
  case CRAFT_MAT_STONE:
    return MATERIAL_STONE;
  }
  return MATERIAL_UNDEFINED;
}

struct obj_data *setup_craft_weapon(struct char_data *ch, int w_type)
{
  struct obj_data *obj;
  int skill = 0;
  int dc = 0;

  if ((obj = read_object(WEAPON_PROTO, VIRTUAL)) == NULL)
  {
    return NULL;
  }

  // set up default values for the weapon type
  set_weapon_object(obj, w_type);

  // set descriptions
  set_craft_item_descs(ch, obj);

  // set obj affects
  set_craft_item_affects(ch, obj);

  // set enhancement bonus
  GET_OBJ_VAL(obj, 4) = GET_CRAFT(ch).enhancement;

  // set obj flags
  set_craft_item_flags(ch, obj);

  skill = material_to_craft_skill(GET_OBJ_TYPE(obj), GET_OBJ_MATERIAL(obj));

  // set the obj material to the main craft material used
  GET_OBJ_MATERIAL(obj) = craft_material_to_obj_material(
      GET_CRAFT(ch).materials[crafting_recipes[GET_CRAFT(ch).crafting_recipe]
                                  .materials[0][GET_CRAFT(ch).craft_variant][0]][0]);

  GET_CRAFT(ch).obj_level = MAX(1, GET_OBJ_LEVEL(obj) = get_craft_obj_level(obj, ch));

  dc = (CREATE_BASE_DC + GET_OBJ_LEVEL(obj) - GET_CRAFT(ch).level_adjust);

  GET_CRAFT(ch).skill_type = skill;
  GET_CRAFT(ch).dc = dc;

  return obj;
}

void create_craft_weapon(struct char_data *ch)
{
  int w_type = GET_CRAFT(ch).crafting_specific;
  struct obj_data *obj;
  int skill = ABILITY_CRAFT_WEAPONSMITHING;
  int dc = 0;

  if ((obj = setup_craft_weapon(ch, w_type)) == NULL)
  {
    log("SYSERR: create_craft_weapon created NULL object");
    return;
  }

  dc = GET_CRAFT(ch).dc + get_craft_level_adjust_dc_change(GET_CRAFT(ch).level_adjust);

  GET_CRAFT(ch).skill_type = skill;

  // skill check to determine success or failure
  if (!create_craft_skill_check(ch, obj, skill, "craft", CREATE_BASE_EXP / 2, dc))
  {
    // failure means we end things here.
    return;
  }

  gain_craft_exp(ch, MAX(CREATE_BASE_EXP, GET_OBJ_LEVEL(obj) * CREATE_BASE_EXP), skill, TRUE);

  /* Check for critical success before giving to player */
  process_craft_critical_success(ch, obj);

  /* Return any materials saved by efficient crafting talent */
  return_efficient_saved_materials(ch);

  send_to_char(ch, "You've created %s!\r\n", obj->short_description);
  obj_to_char(obj, ch);
  reset_current_craft(ch, NULL, FALSE, FALSE);
}

struct obj_data *setup_craft_armor(struct char_data *ch, int a_type)
{
  struct obj_data *obj;
  int skill = 0;
  int dc = 0;

  if ((obj = read_object(ARMOR_PROTO, VIRTUAL)) == NULL)
  {
    return NULL;
  }

  // set up default values for the weapon type
  set_armor_object(obj, a_type);

  // set descriptions
  set_craft_item_descs(ch, obj);

  // set obj affects
  set_craft_item_affects(ch, obj);

  // set enhancement bonus
  GET_OBJ_VAL(obj, 4) = GET_CRAFT(ch).enhancement;

  // set obj flags
  set_craft_item_flags(ch, obj);

  skill = material_to_craft_skill(GET_OBJ_TYPE(obj), GET_OBJ_MATERIAL(obj));

  // set the obj material to the main craft material used
  // GET_OBJ_MATERIAL(obj) = craft_material_to_obj_material(GET_CRAFT(ch).materials[0][0]);
  GET_OBJ_MATERIAL(obj) = craft_material_to_obj_material(
      GET_CRAFT(ch).materials[crafting_recipes[GET_CRAFT(ch).crafting_recipe]
                                  .materials[0][GET_CRAFT(ch).craft_variant][0]][0]);

  GET_CRAFT(ch).obj_level = MAX(1, GET_OBJ_LEVEL(obj) = get_craft_obj_level(obj, ch));

  dc = (CREATE_BASE_DC + GET_OBJ_LEVEL(obj) - GET_CRAFT(ch).level_adjust);

  GET_CRAFT(ch).skill_type = skill;
  GET_CRAFT(ch).dc = dc;

  return obj;
}

void create_craft_armor(struct char_data *ch)
{
  int a_type = GET_CRAFT(ch).crafting_specific;
  struct obj_data *obj;
  int skill = ABILITY_CRAFT_ARMORSMITHING;
  int dc = 0;

  if ((obj = setup_craft_armor(ch, a_type)) == NULL)
  {
    log("SYSERR: create_craft_armor created NULL object");
    return;
  }

  dc = GET_CRAFT(ch).dc + get_craft_level_adjust_dc_change(GET_CRAFT(ch).level_adjust);

  GET_CRAFT(ch).skill_type = skill;

  // skill check to determine success or failure
  if (!create_craft_skill_check(ch, obj, skill, "craft", CREATE_BASE_EXP / 2, dc))
  {
    // failure means we end things here.
    return;
  }

  gain_craft_exp(ch, MAX(CREATE_BASE_EXP, GET_OBJ_LEVEL(obj) * CREATE_BASE_EXP), skill, TRUE);

  /* Check for critical success before giving to player */
  process_craft_critical_success(ch, obj);

  /* Return any materials saved by efficient crafting talent */
  return_efficient_saved_materials(ch);

  send_to_char(ch, "You've created %s!\r\n", obj->short_description);
  obj_to_char(obj, ch);
  reset_current_craft(ch, NULL, FALSE, FALSE);
}

void set_craft_instrument_object(struct obj_data *obj, struct char_data *ch)
{
  int wear_inc;

  GET_OBJ_TYPE(obj) = ITEM_INSTRUMENT;

  // Instrument Type
  GET_OBJ_VAL(obj, 0) = craft_instrument_type_to_actual(GET_CRAFT(ch).crafting_specific);
  // Quality
  GET_OBJ_VAL(obj, 1) = GET_CRAFT(ch).instrument_quality;
  // Effecitveness
  GET_OBJ_VAL(obj, 2) = GET_CRAFT(ch).instrument_effectiveness;
  // Breakability
  GET_OBJ_VAL(obj, 3) = GET_CRAFT(ch).instrument_breakability;

  /* for convenience we are going to go ahead and set some other values */
  GET_OBJ_COST(obj) = 100;
  GET_OBJ_WEIGHT(obj) = 1;

  /* going to go ahead and reset all the bits off */
  for (wear_inc = 0; wear_inc < NUM_ITEM_WEARS; wear_inc++)
  {
    REMOVE_BIT_AR(GET_OBJ_WEAR(obj), wear_inc);
  }

  /* now set take bit */
  TOGGLE_BIT_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_TAKE);
  TOGGLE_BIT_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_INSTRUMENT);
}

/**
 * @brief Converts a given instrument type to its actual representation.
 *
 * This function takes an integer representing an instrument type and
 * returns the corresponding actual instrument value. The mapping of
 * types to actual values is defined within the function.
 *
 * @param type An integer representing the instrument type to be converted.
 * @return An integer representing the actual instrument value.
 */
int craft_instrument_type_to_actual(int type)
{
  switch (type)
  {
  case CRAFT_INSTRUMENT_LYRE:
    return INSTRUMENT_LYRE;
  case CRAFT_INSTRUMENT_FLUTE:
    return INSTRUMENT_FLUTE;
  case CRAFT_INSTRUMENT_HARP:
    return INSTRUMENT_HARP;
  case CRAFT_INSTRUMENT_DRUM:
    return INSTRUMENT_DRUM;
  case CRAFT_INSTRUMENT_HORN:
    return INSTRUMENT_HORN;
  case CRAFT_INSTRUMENT_MANDOLIN:
    return INSTRUMENT_MANDOLIN;
  }
  return INSTRUMENT_LYRE; // default to lyre if not found
}

struct obj_data *setup_craft_instrument(struct char_data *ch, int a_type)
{
  struct obj_data *obj;
  int skill = 0;
  int dc = 0;

  if ((obj = read_object(INSTRUMENT_PROTO, VIRTUAL)) == NULL)
  {
    return NULL;
  }

  // set up default values for the weapon type
  set_craft_instrument_object(obj, ch);

  // set descriptions
  set_craft_item_descs(ch, obj);

  // set obj affects
  set_craft_item_affects(ch, obj);

  // set obj flags
  set_craft_item_flags(ch, obj);

  // set the obj material to the main craft material used
  // GET_OBJ_MATERIAL(obj) = craft_material_to_obj_material(GET_CRAFT(ch).materials[0][0]);
  GET_OBJ_MATERIAL(obj) = craft_material_to_obj_material(
      GET_CRAFT(ch).materials[crafting_recipes[GET_CRAFT(ch).crafting_recipe]
                                  .materials[0][GET_CRAFT(ch).craft_variant][0]][0]);

  skill = material_to_craft_skill(GET_OBJ_TYPE(obj), GET_OBJ_MATERIAL(obj));

  GET_CRAFT(ch).obj_level = MAX(1, GET_OBJ_LEVEL(obj) = (get_craft_obj_level(obj, ch) +
                                                         get_crafting_instrument_dc_modifier(ch)));

  dc = (CREATE_BASE_DC + GET_OBJ_LEVEL(obj) - GET_CRAFT(ch).level_adjust);

  GET_CRAFT(ch).skill_type = skill;
  GET_CRAFT(ch).dc = dc;

  return obj;
}

void create_craft_instrument(struct char_data *ch)
{
  int i_type = GET_CRAFT(ch).crafting_specific;
  struct obj_data *obj;
  int skill = 0;
  int dc = 0;

  if ((obj = setup_craft_instrument(ch, i_type)) == NULL)
  {
    log("SYSERR: create_craft_instrument created NULL object");
    return;
  }

  skill = GET_CRAFT(ch).skill_type;

  dc = GET_CRAFT(ch).dc + get_craft_level_adjust_dc_change(GET_CRAFT(ch).level_adjust);

  // skill check to determine success or failure
  if (!create_craft_skill_check(ch, obj, skill, "craft", CREATE_BASE_EXP / 2, dc))
  {
    // failure means we end things here.
    return;
  }

  gain_craft_exp(ch, MAX(CREATE_BASE_EXP, GET_OBJ_LEVEL(obj) * CREATE_BASE_EXP), skill, TRUE);

  /* Check for critical success before giving to player */
  process_craft_critical_success(ch, obj);

  /* Return any materials saved by efficient crafting talent */
  return_efficient_saved_materials(ch);

  send_to_char(ch, "You've created %s!\r\n", obj->short_description);
  obj_to_char(obj, ch);
  reset_current_craft(ch, NULL, FALSE, FALSE);
}

struct obj_data *setup_craft_misc(struct char_data *ch, int vnum)
{
  struct obj_data *obj;
  int skill = 0;
  int dc = 0;

  if ((obj = read_object(vnum, VIRTUAL)) == NULL)
  {
    return NULL;
  }

  // set descriptions
  set_craft_item_descs(ch, obj);

  // set obj affects
  set_craft_item_affects(ch, obj);

  // set obj flags
  set_craft_item_flags(ch, obj);

  // set the obj material to the main craft material used
  // GET_OBJ_MATERIAL(obj) = craft_material_to_obj_material(GET_CRAFT(ch).materials[0][0]);
  GET_OBJ_MATERIAL(obj) = craft_material_to_obj_material(
      GET_CRAFT(ch).materials[crafting_recipes[GET_CRAFT(ch).crafting_recipe]
                                  .materials[0][GET_CRAFT(ch).craft_variant][0]][0]);
  send_to_char(ch, "Mat: %d\r\n", GET_OBJ_MATERIAL(obj));

  skill = material_to_craft_skill(GET_OBJ_TYPE(obj), GET_OBJ_MATERIAL(obj));

  GET_CRAFT(ch).obj_level = MAX(1, GET_OBJ_LEVEL(obj) = get_craft_obj_level(obj, ch));

  dc = (CREATE_BASE_DC + GET_OBJ_LEVEL(obj) - GET_CRAFT(ch).level_adjust);

  GET_CRAFT(ch).skill_type = skill;
  GET_CRAFT(ch).dc = dc;

  return obj;
}

int craft_misc_spec_to_vnum(int s_type)
{
  int vnum = 0;

  switch (s_type)
  {
  case CRAFT_JEWELRY_BRACELET:
    vnum = WRIST_MOLD;
    break;
  case CRAFT_JEWELRY_EARRING:
    vnum = EARS_MOLD;
    break;
  case CRAFT_JEWELRY_GLASSES:
    vnum = EYES_MOLD;
    break;
  case CRAFT_JEWELRY_NECKLACE:
    vnum = NECKLACE_MOLD;
    break;
  case CRAFT_JEWELRY_RING:
    vnum = RING_MOLD;
    break;
  case CRAFT_MISC_BELT:
    vnum = BELT_MOLD;
    break;
  case CRAFT_MISC_BOOTS:
    vnum = BOOTS_MOLD;
    break;
  case CRAFT_MISC_CLOAK:
    vnum = CLOAK_MOLD;
    break;
  case CRAFT_MISC_GLOVES:
    vnum = GLOVES_MOLD;
    break;
  case CRAFT_MISC_MASK:
    vnum = FACE_MOLD;
    break;
  case CRAFT_MISC_SHOULDERS:
    vnum = SHOULDERS_MOLD;
    break;
  case CRAFT_MISC_ANKLET:
    vnum = ANKLET_MOLD;
    break;
    break;
  }
  return vnum;
}

void create_craft_misc(struct char_data *ch)
{
  int m_type = GET_CRAFT(ch).crafting_item_type;
  int s_type = GET_CRAFT(ch).crafting_specific;
  struct obj_data *obj;
  int vnum = 0;
  int skill = ABILITY_CRAFT_TAILORING;
  int dc = 0;

  switch (m_type)
  {
  case CRAFT_TYPE_MISC:
    vnum = craft_misc_spec_to_vnum(s_type);
    break;
  }

  if ((obj = setup_craft_misc(ch, vnum)) == NULL)
  {
    log("SYSERR: create_craft_misc created NULL object");
    return;
  }

  skill = GET_CRAFT(ch).skill_type;

  dc = GET_CRAFT(ch).dc + get_craft_level_adjust_dc_change(GET_CRAFT(ch).level_adjust);

  // skill check to determine success or failure
  if (!create_craft_skill_check(ch, obj, skill, "craft", CREATE_BASE_EXP / 2, dc))
  {
    // failure means we end things here.
    return;
  }

  gain_craft_exp(ch, MAX(CREATE_BASE_EXP, GET_OBJ_LEVEL(obj) * CREATE_BASE_EXP), skill, TRUE);

  /* Check for critical success before giving to player */
  process_craft_critical_success(ch, obj);

  /* Return any materials saved by efficient crafting talent */
  return_efficient_saved_materials(ch);

  send_to_char(ch, "You've created %s!\r\n", obj->short_description);
  obj_to_char(obj, ch);
  reset_current_craft(ch, NULL, FALSE, FALSE);
}

static bool is_catalyst_affect_eligible(const struct obj_affected_type *affect)
{
  if (!affect)
    return FALSE;
  if (!is_valid_apply(affect->location))
    return FALSE;
  if (affect->modifier <= 0)
    return FALSE;

  switch (affect->location)
  {
  case APPLY_FEAT:
  case APPLY_SPECIAL:
  case APPLY_ELDRITCH_SHAPE:
  case APPLY_ELDRITCH_ESSENCE:
    return FALSE;
  }

  return TRUE;
}

static bool craft_project_has_catalyst_target(struct char_data *ch)
{
  if (!ch)
    return FALSE;

  if (GET_CRAFT(ch).crafting_item_type == CRAFT_TYPE_WEAPON ||
      GET_CRAFT(ch).crafting_item_type == CRAFT_TYPE_ARMOR ||
      GET_CRAFT(ch).crafting_item_type == CRAFT_TYPE_MISC ||
      GET_CRAFT(ch).crafting_item_type == CRAFT_TYPE_INSTRUMENT)
    return TRUE;

  return FALSE;
}

static bool craft_has_pending_noncreate_project(struct char_data *ch)
{
  if (!ch)
    return FALSE;

  if (GET_CRAFT(ch).crafting_method != 0 &&
      GET_CRAFT(ch).crafting_method != SCMD_NEWCRAFT_CREATE)
    return TRUE;

  if (GET_CRAFT(ch).refining_result[0] != CRAFT_MAT_NONE ||
      GET_CRAFT(ch).refining_result[1] > 0)
    return TRUE;

  if (GET_CRAFT(ch).new_size != 0)
    return TRUE;

  if (GET_CRAFT(ch).supply_num_required > 0)
    return TRUE;

  if (GET_CRAFT(ch).golem_type != GOLEM_TYPE_NONE)
    return TRUE;

  if (GET_CRAFT(ch).butcher_material != CRAFT_MAT_NONE)
    return TRUE;

  return FALSE;
}

static bool obj_has_catalyst_target(struct obj_data *obj)
{
  int i;

  if (!obj)
    return FALSE;

  if (OBJ_FLAGGED(obj, ITEM_CRAFTED))
    return TRUE;

  for (i = 0; i < MAX_OBJ_AFFECT; i++)
  {
    if (is_catalyst_affect_eligible(&obj->affected[i]))
      return TRUE;
  }

  return FALSE;
}

static int catalyst_modifier_increase(int location)
{
  switch (location)
  {
  case APPLY_HIT:
  case APPLY_PSP:
    return 5;

  case APPLY_MOVE:
    return 10;

  case APPLY_RES_FIRE:
  case APPLY_RES_COLD:
  case APPLY_RES_AIR:
  case APPLY_RES_EARTH:
  case APPLY_RES_ACID:
  case APPLY_RES_HOLY:
  case APPLY_RES_ELECTRIC:
  case APPLY_RES_UNHOLY:
  case APPLY_RES_SLICE:
  case APPLY_RES_PUNCTURE:
  case APPLY_RES_BLUDGEON:
  case APPLY_RES_SOUND:
  case APPLY_RES_POISON:
  case APPLY_RES_DISEASE:
  case APPLY_RES_NEGATIVE:
  case APPLY_RES_ILLUSION:
  case APPLY_RES_MENTAL:
  case APPLY_RES_LIGHT:
  case APPLY_RES_ENERGY:
  case APPLY_RES_WATER:
  case APPLY_RES_FORCE:
  case APPLY_POWER_RES:
  case APPLY_SKILL:
    return 2;
  }

  return 1;
}

static bool catalyst_improve_instrument(struct obj_data *obj, char *buf, size_t buf_size)
{
  int roll = 0;

  if (!obj || GET_OBJ_TYPE(obj) != ITEM_INSTRUMENT || !buf || buf_size == 0)
    return FALSE;

  roll = rand_number(1, 3);

  switch (roll)
  {
  case 1:
    GET_OBJ_VAL(obj, 1) += 1;
    snprintf(buf, buf_size, "instrument quality +1");
    return TRUE;
  case 2:
    GET_OBJ_VAL(obj, 2) += 1;
    snprintf(buf, buf_size, "instrument effectiveness +1");
    return TRUE;
  default:
    if (GET_OBJ_VAL(obj, 3) > 0)
    {
      GET_OBJ_VAL(obj, 3) = MAX(0, GET_OBJ_VAL(obj, 3) - 1);
      snprintf(buf, buf_size, "instrument breakability -1");
    }
    else
    {
      GET_OBJ_VAL(obj, 1) += 1;
      snprintf(buf, buf_size, "instrument quality +1");
    }
    return TRUE;
  }
}

/* Process catalyst-driven critical success after a crafted item succeeds. */
void process_craft_critical_success(struct char_data *ch, struct obj_data *obj)
{
  int catalysts = 0;
  int i, total_bonuses = 0;
  int bonus_indices[MAX_OBJ_AFFECT];
  bool improved_enhancement = FALSE;
  bool improved_affect = FALSE;
  bool improved_instrument = FALSE;
  char affect_buf[128] = {'\0'};
  char instrument_buf[128] = {'\0'};
  char resonance_buf[256] = {'\0'};

  if (!ch || !obj)
    return;

  catalysts = MIN(MAX(GET_CRAFT(ch).catalysts_used, 0), CRAFT_CATALYST_MAX);
  if (catalysts <= 0)
    return;

  GET_CRAFT(ch).catalysts_used = 0;

  if (!obj_has_catalyst_target(obj))
    return;

  if (rand_number(1, 100) > catalysts)
  {
    send_to_char(ch, "The catalyst%s consumed, but no critical resonance occurs.\r\n",
                 catalysts == 1 ? " is" : "s are");
    return;
  }

  if (GET_OBJ_TYPE(obj) == ITEM_INSTRUMENT)
  {
    improved_instrument = catalyst_improve_instrument(obj, instrument_buf, sizeof(instrument_buf));
  }
  else if (OBJ_FLAGGED(obj, ITEM_CRAFTED))
  {
    GET_OBJ_VAL(obj, 4) += 1;
    improved_enhancement = TRUE;
  }

  for (i = 0; i < MAX_OBJ_AFFECT; i++)
  {
    if (is_catalyst_affect_eligible(&obj->affected[i]))
    {
      bonus_indices[total_bonuses] = i;
      total_bonuses++;
    }
  }

  if (total_bonuses > 0)
  {
    int random_idx = bonus_indices[rand_number(0, total_bonuses - 1)];
    int increase = catalyst_modifier_increase(obj->affected[random_idx].location);

    obj->affected[random_idx].modifier += increase;
    improved_affect = TRUE;
    snprintf(affect_buf, sizeof(affect_buf), "%s +%d",
             apply_types[obj->affected[random_idx].location], increase);
  }

  if (improved_enhancement)
  {
    strlcat(resonance_buf, "enhancement +1", sizeof(resonance_buf));
  }
  if (improved_instrument)
  {
    if (*resonance_buf)
      strlcat(resonance_buf, ", ", sizeof(resonance_buf));
    strlcat(resonance_buf, instrument_buf, sizeof(resonance_buf));
  }
  if (improved_affect)
  {
    if (*resonance_buf)
      strlcat(resonance_buf, ", ", sizeof(resonance_buf));
    strlcat(resonance_buf, affect_buf, sizeof(resonance_buf));
  }

  if (*resonance_buf)
    send_to_char(ch, "\tY**CATALYST CRITICAL!**\tn The craft resonates: %s.\r\n",
                 resonance_buf);
}

void craft_create_complete(struct char_data *ch)
{
  switch (GET_CRAFT(ch).crafting_item_type)
  {
  case CRAFT_TYPE_WEAPON:
    create_craft_weapon(ch);
    break;
  case CRAFT_TYPE_ARMOR:
    create_craft_armor(ch);
    break;
  case CRAFT_TYPE_MISC:
    create_craft_misc(ch);
    break;
  case CRAFT_TYPE_INSTRUMENT:
    create_craft_instrument(ch);
    break;
  }
  act("$n finishes crafting.", FALSE, ch, 0, 0, TO_ROOM);
}

void check_current_craft(struct char_data *ch, bool verbose)
{
  if (!is_craft_ready(ch, verbose))
  {
    send_to_char(ch,
                 "\tRThis craft is not yet ready to begin. Please fix the above errors.\tn\r\n");
  }
  else
  {
    send_to_char(ch, "\tGThis craft is ready to begin, though you still may want to add more to "
                     "it. Type 'craft start' to begin crafting.\tn\r\n");
  }
}

void set_crafting_variant(struct char_data *ch, char *arg2)
{
  int i = 0, j = 0, variant = 0, recipe = 0;
  bool found = FALSE;
  char materials[200];
  char mat_one[60], mat_two[60], mat_three[60];

  if (GET_CRAFT(ch).craft_variant != -1)
  {
    send_to_char(ch, "You've already set a variant type. To change it you must reset the crafting "
                     "project with: craft reset.\r\n");
    return;
  }

  if (GET_CRAFT(ch).crafting_item_type == CRAFT_TYPE_NONE)
  {
    send_to_char(ch, "You need to set the item type first with 'craft itemtype'.\r\n");
    return;
  }

  if (GET_CRAFT(ch).crafting_specific == 0)
  {
    send_to_char(ch, "You need to set the item specific type first with 'craft specifictype'.\r\n");
    return;
  }

  if (!*arg2)
  {
    send_to_char(ch, "You can select from the following variants:\r\n");
    for (i = 0; i < NUM_CRAFTING_RECIPES; i++)
    {
      if (crafting_recipes[i].object_type ==
              craft_recipe_by_type(GET_CRAFT(ch).crafting_item_type) &&
          (crafting_recipes[i].practical_type == GET_CRAFT(ch).crafting_specific))
      {
        for (j = 0; j < NUM_CRAFT_VARIANTS; j++)
        {
          if (crafting_recipes[i].materials[0][j][0] == 0)
            continue;
          snprintf(mat_one, sizeof(mat_one), "\tn");
          snprintf(mat_two, sizeof(mat_two), "\tn");
          snprintf(mat_three, sizeof(mat_three), "\tn");
          if (crafting_recipes[i].materials[0][j][0] > 0)
            snprintf(mat_one, sizeof(mat_one), "%d unit%s of %s",
                     crafting_recipes[i].materials[0][j][1],
                     crafting_recipes[i].materials[0][j][1] > 1 ? "s" : "",
                     crafting_material_groups[crafting_recipes[i].materials[0][j][0]]);
          if (crafting_recipes[i].materials[1][j][0] > 0)
            snprintf(mat_two, sizeof(mat_two), ", %d unit%s of %s",
                     crafting_recipes[i].materials[1][j][1],
                     crafting_recipes[i].materials[1][j][1] > 1 ? "s" : "",
                     crafting_material_groups[crafting_recipes[i].materials[1][j][0]]);
          if (crafting_recipes[i].materials[2][j][0] > 0)
            snprintf(mat_three, sizeof(mat_three), ", %d unit%s of %s",
                     crafting_recipes[i].materials[2][j][1],
                     crafting_recipes[i].materials[2][j][1] > 1 ? "s" : "",
                     crafting_material_groups[crafting_recipes[i].materials[2][j][0]]);
          snprintf(materials, sizeof(materials), "%s%s%s", mat_one, mat_two, mat_three);
          send_to_char(ch, "%-30s : %s\r\n", crafting_recipes[i].variant_descriptions[j],
                       materials);
        }
      }
    }
  }
  else
  {
    for (i = 0; i < NUM_CRAFTING_RECIPES; i++)
    {
      if (crafting_recipes[i].object_type ==
              craft_recipe_by_type(GET_CRAFT(ch).crafting_item_type) &&
          (crafting_recipes[i].practical_type == GET_CRAFT(ch).crafting_specific))
      {
        for (j = 0; j < NUM_CRAFT_VARIANTS; j++)
        {
          if (crafting_recipes[i].materials[0][j][0] == 0)
            continue;
          if (is_abbrev(arg2, crafting_recipes[i].variant_descriptions[j]))
          {
            variant = j;
            recipe = i;
            found = TRUE;
            break;
          }
        }
        if (found)
          break;
      }
    }
    if (!found)
    {
      send_to_char(
          ch,
          "That is not a valid variant type. Enter 'craft variant' by itself to see options.\r\n");
      return;
    }
    GET_CRAFT(ch).craft_variant = variant;
    GET_CRAFT(ch).crafting_recipe = recipe;
    send_to_char(ch, "You have chosen '%s' as the variant type for your craft.\r\n",
                 crafting_recipes[recipe].variant_descriptions[variant]);
    /* Free old strings before allocating new ones to prevent memory leaks */
    if (GET_CRAFT(ch).keywords)
      free(GET_CRAFT(ch).keywords);
    if (GET_CRAFT(ch).short_description)
      free(GET_CRAFT(ch).short_description);
    if (GET_CRAFT(ch).room_description)
      free(GET_CRAFT(ch).room_description);
    if (GET_CRAFT(ch).ex_description)
      free(GET_CRAFT(ch).ex_description);
    GET_CRAFT(ch).keywords = strdup("not set");
    GET_CRAFT(ch).short_description = strdup("not set");
    GET_CRAFT(ch).room_description = strdup("not set");
    GET_CRAFT(ch).ex_description = NULL;
    return;
  }
}

void set_crafting_materials(struct char_data *ch, const char *arg2)
{
  if (!*arg2)
  {
    send_to_char(ch, "You need to specify the material transaction and type. Eg. craft materials "
                     "add steel or craft materials remove steel.\r\n");
    return;
  }
  char mat[100], amount[100];
  int num_mats = 0, mat_type = 0, group = 0, i = 0;

  half_chop_c(arg2, amount, sizeof(amount), mat, sizeof(mat));

  if (!*amount || !*mat)
  {
    send_to_char(ch, "You need to specify the material transaction and type. Eg. craft materials "
                     "add steel or craft materials remove steel.\r\n");
    return;
  }

  for (i = 0; i < NUM_CRAFT_MATS; i++)
  {
    if (is_abbrev(mat, crafting_materials[i]))
    {
      mat_type = i;
      break;
    }
  }

  if (mat_type == 0)
  {
    send_to_char(ch, "That is not a valid material type. Type 'materials' for a list of which "
                     "materials you possess.\r\n");
    return;
  }

  if (is_abbrev(amount, "add"))
    num_mats = 1;
  else if (is_abbrev(amount, "remove"))
    num_mats = -1;
  else
  {
    send_to_char(ch, "You need to specify the material transaction and type. Eg. craft materials "
                     "add steel or craft materials remove steel.\r\n");
    return;
  }

  group = craft_group_by_material(mat_type);

  if (group == CRAFT_GROUP_NONE)
  {
    send_to_char(ch, "That material type cannot be used in crafting recipes.\r\n");
    return;
  }

  if (GET_CRAFT(ch).crafting_item_type == 0 || GET_CRAFT(ch).crafting_specific == 0 ||
      GET_CRAFT(ch).craft_variant == -1)
  {
    send_to_char(ch, "You must select the item type, specific type and variant type before adding "
                     "materials.\r\n");
    return;
  }

  for (i = 0; i < 3; i++)
  {
    if (crafting_recipes[GET_CRAFT(ch).crafting_recipe]
            .materials[i][GET_CRAFT(ch).craft_variant][0] == group)
    {
      process_crafting_materials(ch, group, mat_type, num_mats, i);
      return;
    }
  }

  // crafting material type is not used in this recipe
  send_to_char(ch, "The crafting project does not use that type of material.\r\n");
  return;
}

void process_crafting_materials(struct char_data *ch, int group, int mat_type, int num_mats,
                                int mat_slot)
{
  int base_amount = 0, owned_amount = 0;

  // removing mats
  if (num_mats < 0)
  {
    if (GET_CRAFT(ch).materials[group][0] != mat_type &&
        GET_CRAFT(ch).materials[group][0] != CRAFT_GROUP_NONE)
    {
      send_to_char(ch, "You are currently using '%s' for this project.\r\n",
                   crafting_materials[GET_CRAFT(ch).materials[group][0]]);
      return;
    }
    base_amount = GET_CRAFT(ch).materials[group][1];
    GET_CRAFT_MAT(ch, mat_type) += base_amount;
    send_to_char(ch, "You recover %d unit%s of %s (%s) from the crafting project.\r\n", base_amount,
                 base_amount == 1 ? "" : "s", crafting_materials[mat_type],
                 crafting_material_groups[craft_group_by_material(mat_type)]);
    GET_CRAFT(ch).materials[group][0] = 0;
    GET_CRAFT(ch).materials[group][1] = 0;
    return;
  }
  else
  {
    if (GET_CRAFT(ch).materials[group][1] > 0)
    {
      send_to_char(ch,
                   "There are already %d units of %s allocated. To remove them, type: craft "
                   "materials remove %s.\r\n",
                   GET_CRAFT(ch).materials[group][1],
                   crafting_material_groups[GET_CRAFT(ch).materials[group][0]],
                   crafting_material_groups[GET_CRAFT(ch).materials[group][0]]);
      return;
    }
    base_amount = crafting_recipes[GET_CRAFT(ch).crafting_recipe]
                      .materials[mat_slot][GET_CRAFT(ch).craft_variant][1];
    owned_amount = GET_CRAFT_MAT(ch, mat_type);
    if (GET_CRAFT_MAT(ch, mat_type) < base_amount)
    {
      send_to_char(ch,
                   "This project requires %d unit%s of %s (%s), but you only have %d unit%s.\r\n",
                   base_amount, base_amount == 1 ? "" : "s", crafting_materials[mat_type],
                   crafting_material_groups[craft_group_by_material(mat_type)], owned_amount,
                   owned_amount == 1 ? "" : "s");
      return;
    }
    GET_CRAFT_MAT(ch, mat_type) -= GET_CRAFT(ch).materials[group][1];

    /* Silently check for efficient talent - chance to save half materials */
    int efficient_chance = get_efficient_talent_bonus(ch, GET_CRAFT(ch).skill_type);
    int saved_amount = 0;
    if (efficient_chance > 0 && rand_number(1, 100) <= efficient_chance)
    {
      saved_amount = base_amount / 2;
      if (saved_amount < 1)
        saved_amount = 0;
      /* Store the saved materials to return upon completion */
      GET_CRAFT(ch).efficient_saved_materials[group][0] = mat_type;
      GET_CRAFT(ch).efficient_saved_materials[group][1] = saved_amount;
    }
    else
    {
      /* Clear any previous saved materials for this slot */
      GET_CRAFT(ch).efficient_saved_materials[group][0] = 0;
      GET_CRAFT(ch).efficient_saved_materials[group][1] = 0;
    }

    send_to_char(ch, "You add %d unit%s of %s (%s) to the crafting project.\r\n", base_amount,
                 base_amount == 1 ? "" : "s", crafting_materials[mat_type],
                 crafting_material_groups[craft_group_by_material(mat_type)]);
    GET_CRAFT(ch).materials[group][0] = mat_type;
    GET_CRAFT(ch).materials[group][1] = base_amount;
    /* Take all materials initially */
    GET_CRAFT_MAT(ch, mat_type) -= base_amount;
  }
}

const int craft_skills_alphabetic[END_HARVEST_ABILITIES - START_CRAFT_ABILITIES + 1] = {
    ABILITY_CRAFT_ALCHEMY,        ABILITY_CRAFT_ARMORSMITHING, ABILITY_CRAFT_BOWMAKING,
    ABILITY_HARVEST_BUTCHERING,
    ABILITY_CRAFT_COOKING,        ABILITY_CRAFT_FISHING,       ABILITY_HARVEST_FORESTRY,
    ABILITY_HARVEST_GATHERING,    ABILITY_HARVEST_HUNTING,     ABILITY_CRAFT_JEWELCRAFTING,
    ABILITY_CRAFT_LEATHERWORKING, ABILITY_CRAFT_METALWORKING,  ABILITY_HARVEST_MINING,
    ABILITY_CRAFT_POISONMAKING,   ABILITY_HARVEST_SURVEYING,   ABILITY_CRAFT_TAILORING,
    ABILITY_CRAFT_TRAPMAKING,     ABILITY_CRAFT_WEAPONSMITHING, ABILITY_CRAFT_WOODWORKING};

void show_craft_score(struct char_data *ch, const char *arg2)
{
  int i = 0, abil = 0, base_rank = 0, modifier = 0, total = 0;

  send_to_char(ch, "\r\n");


  send_to_char(ch, "\tC%-25s %-10s %-4s %-4s %-5s %-6s %-6s\tn\r\n", "SKILL", "TYPE", "BASE",
               "MODS", "TOTAL", "EXP", "TNL");
  send_to_char(ch, "\tc");
  draw_line(ch, 90, '-', '-');
  send_to_char(ch, "\tn");

  for (i = START_CRAFT_ABILITIES; i <= END_HARVEST_ABILITIES; i++)
  {
    abil = craft_skills_alphabetic[i - START_CRAFT_ABILITIES];
    if (crafting_skill_type(abil) != CRAFT_SKILL_TYPE_CRAFT)
      continue;
    base_rank = GET_ABILITY(ch, abil);
    modifier = get_proficient_talent_bonus(ch, abil);
    total = base_rank + modifier;
    send_to_char(ch, "%-25s %-10s %-4d %-4d %-5d %-6d %-6d\r\n", ability_names[abil], "Craft",
                 base_rank, modifier, total, GET_CRAFT_SKILL_EXP(ch, abil),
                 craft_skill_level_exp(ch, base_rank + 1));
  }

  send_to_char(ch, "\tc");
  draw_line(ch, 90, '-', '-');
  send_to_char(ch, "\tn");

  for (i = START_CRAFT_ABILITIES; i < END_HARVEST_ABILITIES; i++)
  {
    abil = craft_skills_alphabetic[i - START_CRAFT_ABILITIES];
    if (crafting_skill_type(abil) != CRAFT_SKILL_TYPE_HARVEST)
      continue;
    base_rank = GET_ABILITY(ch, abil);
    modifier = get_proficient_talent_bonus(ch, abil);
    total = base_rank + modifier;
    send_to_char(ch, "%-25s %-10s %-4d %-4d %-5d %-6d %-6d\r\n", ability_names[abil], "Harvest",
                 base_rank, modifier, total, GET_CRAFT_SKILL_EXP(ch, abil),
                 craft_skill_level_exp(ch, base_rank + 1));
  }
  send_to_char(ch, "\tc");
  draw_line(ch, 90, '-', '-');
  send_to_char(ch, "\tn");

  if (HAS_FEAT(ch, FEAT_CRAFT_MAGICAL_ARMS_AND_ARMOR) || HAS_FEAT(ch, FEAT_CRAFT_WONDEROUS_ITEM))
  {
    bool has_both = (HAS_FEAT(ch, FEAT_CRAFT_MAGICAL_ARMS_AND_ARMOR) && HAS_FEAT(ch, FEAT_CRAFT_WONDEROUS_ITEM));
    if (HAS_FEAT(ch, FEAT_CRAFT_MAGICAL_ARMS_AND_ARMOR))
    {
      send_to_char(ch, "Craft Magical Arms and Armor -> +5 when crafting weapons, armor and shields.\r\n");
    }
    if (HAS_FEAT(ch, FEAT_CRAFT_WONDEROUS_ITEM))
    {
      send_to_char(ch, "Craft Wonderous Item         -> +5 when crafting misc items, such as rings, bracers, etc.\r\n");
    }
    send_to_char(ch, "%s bonus%s %s listed here because they are tied to the type of item being crafted,\r\n"
                     "not to a specific crafting skill.\r\n", has_both ? "These" : "This", has_both ? "s" : "", has_both ? "are" : "is");
    send_to_char(ch, "\tc");
    draw_line(ch, 90, '-', '-');
    send_to_char(ch, "\tn");
  }

  /* Display talent points and artisan points */
  send_to_char(ch, "\r\n");
  send_to_char(ch, "\tCTalent Points (TP):\tn %d unspent\r\n", GET_TALENT_POINTS(ch));
  send_to_char(ch, "\tCArtisan Points (AP):\tn %d total\r\n", GET_ARTISAN_EXP(ch));
  send_to_char(ch, "\r\n");

  /* Display crafting specializations */
  send_to_char(ch, "\tc");
  draw_line(ch, 90, '-', '-');
  send_to_char(ch, "\tn");
  send_to_char(ch, "\tYCrafting Specializations:\tn\r\n");
  
  if (GET_CRAFT(ch).craft_specialization[0] == -1 && GET_CRAFT(ch).craft_specialization[1] == -1)
  {
    send_to_char(ch, "  You have not yet chosen your specializations.\r\n");
    send_to_char(ch, "  Use 'craft specialize <skill>' to choose up to 2 crafting/harvesting skills.\r\n");
    send_to_char(ch, "  Specialized skills gain +5 to skill checks and double experience.\r\n");
  }
  else
  {
    int spec_count = 0;
    if (GET_CRAFT(ch).craft_specialization[0] >= 0)
    {
      send_to_char(ch, "  \tC1.\tn %s (+5 skill, 2x exp)\r\n", 
                   ability_names[GET_CRAFT(ch).craft_specialization[0]]);
      spec_count++;
    }
    if (GET_CRAFT(ch).craft_specialization[1] >= 0)
    {
      send_to_char(ch, "  \tC2.\tn %s (+5 skill, 2x exp)\r\n", 
                   ability_names[GET_CRAFT(ch).craft_specialization[1]]);
      spec_count++;
    }
    if (spec_count < 2)
    {
      send_to_char(ch, "  You have %d specialization slot%s remaining.\r\n", 
                   2 - spec_count, (2 - spec_count == 1) ? "" : "s");
      send_to_char(ch, "  Use 'craft specialize <skill>' to choose another skill.\r\n");
    }
  }
  send_to_char(ch, "\r\n");
}

void set_crafting_enhancement(struct char_data *ch, const char *arg2)
{
  int amount = 0, max = 8;

  if (GET_CRAFT(ch).short_description == NULL ||
      GET_CRAFT(ch).crafting_item_type == CRAFT_TYPE_NONE)
  {
    send_to_char(ch,
                 "You must provide the crafting object short description and item type first.\r\n");
    return;
  }

  if (!*arg2)
  {
    send_to_char(ch, "Please specify the enhancement amount (weapons and armor only).\r\n");
    return;
  }

  if (GET_CRAFT(ch).crafting_item_type != CRAFT_TYPE_ARMOR &&
      GET_CRAFT(ch).crafting_item_type != CRAFT_TYPE_WEAPON)
  {
    send_to_char(ch, "You can only set an enhancement bonus on weapons, armor and shields.\r\n");
    return;
  }

  if (is_abbrev(arg2, "reset"))
  {
    send_to_char(ch, "You reset your project's enhancement bonus to 0.\r\n");
    int mote_type = get_enhancement_mote_type(ch, GET_CRAFT(ch).crafting_item_type,
                                              GET_CRAFT(ch).crafting_specific);
    if (GET_CRAFT(ch).enhancement_motes_required > 0)
    {
      send_to_char(ch, "You've recovered %d %s.\r\n", GET_CRAFT(ch).enhancement_motes_required,
                   crafting_motes[mote_type]);
      GET_CRAFT_MOTES(ch, mote_type) += GET_CRAFT(ch).enhancement_motes_required;
      GET_CRAFT(ch).enhancement_motes_required = 0;
    }
    GET_CRAFT(ch).enhancement = 0;
    return;
  }

  if (GET_CRAFT(ch).enhancement > 0)
  {
    send_to_char(ch, "You have already set the  item's enhancement bonus. To change it you'll need "
                     "to reset it first with: craft enhancement reset.\r\n");
    return;
  }

  amount = atoi(arg2);

  if (amount <= 0 || amount > max)
  {
    send_to_char(ch, "Please specify an amount between 1 and %d.\r\n", max);
    return;
  }

  GET_CRAFT(ch).enhancement = amount;
  send_to_char(ch, "You set your project's enhancement bonus to %d.\r\n", amount);
}

void set_craft_specialization(struct char_data *ch, const char *arg)
{
  int skill_num = -1;
  int i;
  bool is_craft_or_harvest = FALSE;

  if (!*arg)
  {
    send_to_char(ch, "Syntax: craft specialize <skill name>\r\n");
    send_to_char(ch, "You can specialize in up to 2 crafting or harvesting skills.\r\n");
    send_to_char(ch, "Specialized skills receive +5 to skill checks and double experience.\r\n");
    send_to_char(ch, "Use 'craft score' to see available skills and your current specializations.\r\n");
    return;
  }

  /* Check if both specialization slots are already filled */
  if (GET_CRAFT(ch).craft_specialization[0] >= 0 && GET_CRAFT(ch).craft_specialization[1] >= 0)
  {
    send_to_char(ch, "You have already specialized in two skills:\r\n");
    send_to_char(ch, "  1. %s\r\n", ability_names[GET_CRAFT(ch).craft_specialization[0]]);
    send_to_char(ch, "  2. %s\r\n", ability_names[GET_CRAFT(ch).craft_specialization[1]]);
    send_to_char(ch, "Specializations are permanent and cannot be changed.\r\n");
    return;
  }

  /* Find the skill by name */
  for (i = START_CRAFT_ABILITIES; i < END_HARVEST_ABILITIES; i++)
  {
    if (is_abbrev(arg, ability_names[i]))
    {
      skill_num = i;
      is_craft_or_harvest = TRUE;
      break;
    }
  }

  if (!is_craft_or_harvest || skill_num == -1)
  {
    send_to_char(ch, "'%s' is not a valid crafting or harvesting skill.\r\n", arg);
    send_to_char(ch, "Use 'craft score' to see the list of available skills.\r\n");
    return;
  }

  /* Check if already specialized in this skill */
  if (GET_CRAFT(ch).craft_specialization[0] == skill_num || 
      GET_CRAFT(ch).craft_specialization[1] == skill_num)
  {
    send_to_char(ch, "You are already specialized in %s.\r\n", ability_names[skill_num]);
    return;
  }

  /* Add the specialization to the first available slot */
  if (GET_CRAFT(ch).craft_specialization[0] == -1)
  {
    GET_CRAFT(ch).craft_specialization[0] = skill_num;
    send_to_char(ch, "\tGYou have specialized in %s!\tn\r\n", ability_names[skill_num]);
    send_to_char(ch, "You now receive +5 to %s skill checks and double experience.\r\n", 
                 ability_names[skill_num]);
    if (GET_CRAFT(ch).craft_specialization[1] == -1)
    {
      send_to_char(ch, "You have 1 specialization slot remaining.\r\n");
    }
  }
  else if (GET_CRAFT(ch).craft_specialization[1] == -1)
  {
    GET_CRAFT(ch).craft_specialization[1] = skill_num;
    send_to_char(ch, "\tGYou have specialized in %s!\tn\r\n", ability_names[skill_num]);
    send_to_char(ch, "You now receive +5 to %s skill checks and double experience.\r\n", 
                 ability_names[skill_num]);
    send_to_char(ch, "\tYYou have used both specialization slots.\tn\r\n");
  }
}

void newcraft_create(struct char_data *ch, const char *argument)
{
  char arg1[200], arg2[MAX_EXTRA_DESC];

  half_chop_c(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

  if (!*arg1)
  {
    send_to_char(ch, "%s", NEWCRAFT_CREATE_NOARG1);
    return;
  }
  else if (is_abbrev(arg1, "tutorial"))
  {
    show_craft_tutorial(ch);
    return;
  }
  else if (is_abbrev(arg1, "disenchant"))
  {
    newcraft_disenchant(ch, arg2);
    return;
  }
  else if (is_abbrev(arg1, "golem"))
  {
    if (CONFIG_CRAFTING_SYSTEM != CRAFTING_SYSTEM_MOTES)
    {
      send_to_char(ch, "Golem crafting is not enabled on this server.\r\n");
      return;
    }
    newcraft_golem(ch, arg2);
    return;
  }
  else if (is_abbrev(arg1, "itemtype") || is_abbrev(arg1, "type"))
  {
    set_crafting_itemtype(ch, arg2);
    return;
  }
  else if (is_abbrev(arg1, "specifictype"))
  {
    if (GET_CRAFT(ch).crafting_item_type == CRAFT_TYPE_NONE)
    {
      send_to_char(ch, "You need to set the crafting type first, using: craft itemtype (type)\r\n");
      return;
    }
    if (GET_CRAFT(ch).crafting_specific != 0)
    {
      send_to_char(ch, "You have already set the crafting specific type. To change it, you'll need "
                       "to reset it first with: craft reset.\r\n");
      return;
    }
    switch (GET_CRAFT(ch).crafting_item_type)
    {
    case CRAFT_TYPE_WEAPON:
      set_craft_weapon_type(ch, arg2);
      break;
    case CRAFT_TYPE_ARMOR:
      set_craft_armor_type(ch, arg2);
      break;
    case CRAFT_TYPE_INSTRUMENT:
      set_craft_instrument_type(ch, arg2);
      break;
    case CRAFT_TYPE_MISC:
      set_craft_misc_type(ch, arg2);
      break;
    default:
      send_to_char(ch, "You need to set the crafting type first, using: craft itemtype (type)\r\n");
      return;
    }
    return;
  }
  else if (is_abbrev(arg1, "variant"))
  {
    set_crafting_variant(ch, arg2);
    return;
  }
  else if (is_abbrev(arg1, "keywords"))
  {
    set_crafting_keywords(ch, arg2);
  }
  else if (is_abbrev(arg1, "shortdesc"))
  {
    set_crafting_short_desc(ch, arg2);
  }
  else if (is_abbrev(arg1, "roomdesc"))
  {
    set_crafting_room_desc(ch, arg2);
  }
  else if (is_abbrev(arg1, "extradesc"))
  {
    set_crafting_extra_desc(ch, arg2);
  }
  else if (is_abbrev(arg1, "bonuses"))
  {
    set_crafting_bonuses(ch, arg2);
  }
  else if (is_abbrev(arg1, "enhancement"))
  {
    set_crafting_enhancement(ch, arg2);
  }
  else if (is_abbrev(arg1, "materials"))
  {
    set_crafting_materials(ch, arg2);
  }
  else if (is_abbrev(arg1, "motes"))
  {
    set_crafting_motes(ch, arg2);
  }
  else if (is_abbrev(arg1, "catalysts") || is_abbrev(arg1, "catalyst"))
  {
    set_crafting_catalysts(ch, arg2);
  }
  else if (is_abbrev(arg1, "instrument"))
  {
    set_crafting_instrument(ch, arg2);
  }
  else if (is_abbrev(arg1, "leveladjust"))
  {
    set_craft_level_adjust(ch, arg2);
  }
  else if (is_abbrev(arg1, "score"))
  {
    show_craft_score(ch, arg2);
  }
  else if (is_abbrev(arg1, "specialize"))
  {
    set_craft_specialization(ch, arg2);
  }
  else if (is_abbrev(arg1, "display") || is_abbrev(arg1, "show") || is_abbrev(arg1, "review") ||
           is_abbrev(arg1, "information"))
  {
    show_current_craft(ch);
  }
  else if (is_abbrev(arg1, "reset"))
  {
    reset_current_craft(ch, arg2, TRUE, TRUE);
  }
  else if (is_abbrev(arg1, "check"))
  {
    check_current_craft(ch, TRUE);
  }
  else if (is_abbrev(arg1, "start") || is_abbrev(arg1, "begin"))
  {
    begin_current_craft(ch);
  }
  else
  {
    send_to_char(ch, "%s", NEWCRAFT_CREATE_NOARG1);
    return;
  }
}

void newcraft_survey(struct char_data *ch, const char *argument)
{
  int seconds = 0;

  if (GET_CRAFT(ch).craft_duration > 0)
  {
    send_to_char(ch, "You cannot survey until you complete your current task. To cancel, type "
                     "'cancel craft'.\r\n");
    return;
  }

  if (!is_valid_harvesting_sector(world[IN_ROOM(ch)].sector_type))
  {
    send_to_char(ch, "This locale does not provide harvested resources.\r\n");
    return;
  }

  if (GET_LEVEL(ch) >= LVL_IMMORT)
    seconds = 1;
  else
    seconds = SURVEY_BASE_TIME;

  GET_CRAFT(ch).crafting_method = SCMD_NEWCRAFT_SURVEY;
  GET_CRAFT(ch).craft_duration = seconds;

  send_to_char(ch, "You begin surveying the immediate area for harvestable materials.\r\n");
  act("$n starts surveying.", FALSE, ch, 0, 0, TO_ROOM);
}

void craft_refine_complete(struct char_data *ch)
{
  int roll, dc, skill, skill_type, num = 0, exp = 0, happy_hour_bonus = 0, batch_quantity = 1;

  if (GET_CRAFT(ch).refining_result[0] == 0 || GET_CRAFT(ch).refining_result[1] == 0)
  {
    send_to_char(ch, "Refining result error. Please inform staff.\r\n ");
    return;
  }

  roll = d20(ch);
  dc = GET_CRAFT(ch).dc;
  skill_type = GET_CRAFT(ch).skill_type;
  skill = get_craft_skill_value(ch, skill_type);

  /* Add proficient talent bonus */
  skill += get_proficient_talent_bonus(ch, skill_type);

  num = GET_CRAFT(ch).refining_result[1];
  batch_quantity = GET_CRAFT(ch).refining_batch_quantity;

  if ((20 + skill) < dc)
  {
    send_to_char(ch, "That refining type is too complex for you.\r\n");
    reset_current_craft(ch, NULL, TRUE, TRUE);
    return;
  }
  else if (roll == 1)
  {
    send_to_char(ch, "\tM[CRITICAL FAILURE]\tn You rolled a natural 1! Your refining attempt "
                     "failed and you lost your materials.\r\n");
    reset_current_craft(ch, NULL, FALSE, FALSE);
    return;
  }
  else if (roll == 20)
  {
    send_to_char(ch,
                 "\tM[CRITICAL SUCCESS]\tn You rolled a natural 20! Your refining attempt "
                 "succeeded and you gained an extra unit of %s.\r\n",
                 crafting_materials[GET_CRAFT(ch).refining_result[0]]);
    num++;
    exp = (REFINE_BASE_EXP + dc) * num;
    
    /* Apply happy hour bonus */
    if (HAPPY_CRAFTING_EXP > 0)
    {
      happy_hour_bonus = (exp * HAPPY_CRAFTING_EXP) / 100;
      send_to_char(ch, "\tY*HAPPY HOUR*\tn You gain %d bonus experience!\r\n", happy_hour_bonus);
    }
    
    gain_craft_exp(ch, exp + happy_hour_bonus, skill_type, TRUE);
  }
  else if ((roll + skill) < dc)
  {
    /* Check for specialization */
    int base_skill = GET_ABILITY(ch, skill_type);
    int spec_bonus = 0;
    bool is_specialized = (GET_CRAFT(ch).craft_specialization[0] == skill_type || 
                          GET_CRAFT(ch).craft_specialization[1] == skill_type);
    if (is_specialized)
      spec_bonus = 5;
    
    char bonus_text[128] = "";
    if (spec_bonus > 0)
      snprintf(bonus_text, sizeof(bonus_text), " + specialization [%d]", spec_bonus);
    
    send_to_char(ch,
                 "You rolled %d + base skill %d%s for a total of %d < dc of %d. You failed your refining "
                 "attempt but may try again.\r\n",
                 roll, base_skill, bonus_text, roll + skill, dc);
    return;
  }
  else
  {
    /* Check for specialization */
    int base_skill = GET_ABILITY(ch, skill_type);
    int spec_bonus = 0;
    bool is_specialized = (GET_CRAFT(ch).craft_specialization[0] == skill_type || 
                          GET_CRAFT(ch).craft_specialization[1] == skill_type);
    if (is_specialized)
      spec_bonus = 5;
    
    char bonus_text[128] = "";
    if (spec_bonus > 0)
      snprintf(bonus_text, sizeof(bonus_text), " + specialization [%d]", spec_bonus);
    
    send_to_char(ch, "You rolled %d + base skill %d%s for a total of %d >= dc of %d. You succeed!\r\n",
                 roll, base_skill, bonus_text, roll + skill, dc);
    exp = (REFINE_BASE_EXP + dc) * num;
    
    /* Apply happy hour bonus */
    if (HAPPY_CRAFTING_EXP > 0)
    {
      happy_hour_bonus = (exp * HAPPY_CRAFTING_EXP) / 100;
      send_to_char(ch, "\tY*HAPPY HOUR*\tn You gain %d bonus experience!\r\n", happy_hour_bonus);
    }
    
    gain_craft_exp(ch, exp + happy_hour_bonus, skill_type, TRUE);
  }

  GET_CRAFT_MAT(ch, GET_CRAFT(ch).refining_result[0]) += num;
  if (batch_quantity == 1)
  {
    send_to_char(ch, "You refine %d unit%s of %s.\r\n", num,
                 num > 1 ? "s" : "",
                 crafting_materials[GET_CRAFT(ch).refining_result[0]]);
  }
  else
  {
    send_to_char(ch, "You complete the refining of %d batches, producing %d unit%s of %s.\r\n",
                 batch_quantity, num,
                 num > 1 ? "s" : "",
                 crafting_materials[GET_CRAFT(ch).refining_result[0]]);
  }
  reset_current_craft(ch, NULL, FALSE, FALSE);
  act("$n finishes refining.", FALSE, ch, 0, 0, TO_ROOM);
}

void harvest_complete(struct char_data *ch)
{
  int skill = 0, skill_roll = 0, roll = 0, dc = 0, amount = 0, bonus = 0, harvest_level = 0;
  bool motes_found = FALSE;
  int num_motes, mote_type, bonus_motes, synergy_talent, synergy_rank;
  int prof_bonus = 0;

  if (world[IN_ROOM(ch)].harvest_material == CRAFT_MAT_NONE ||
      world[IN_ROOM(ch)].harvest_material_amount <= 0)
  {
    send_to_char(ch, "The resource is depleted.\r\n");
    return;
  }

  skill = harvesting_skill_by_material(world[IN_ROOM(ch)].harvest_material);

  if (skill == 0)
  {
    send_to_char(ch, "There was an error harvesting %s. Please inform staff.\r\n",
                 crafting_materials[world[IN_ROOM(ch)].harvest_material]);
    return;
  }

  roll = d20(ch);
  skill_roll = get_craft_skill_value(ch, skill);

  /* Add proficient talent bonus */
  prof_bonus = get_proficient_talent_bonus(ch, skill);

  /* Check for specialization */
  int base_skill = GET_ABILITY(ch, skill);
  int spec_bonus = 0;
  bool is_specialized = (GET_CRAFT(ch).craft_specialization[0] == skill || 
                        GET_CRAFT(ch).craft_specialization[1] == skill);
  if (is_specialized)
    spec_bonus = 5;

  /* DC based on material grade */
  harvest_level = MAX(1, material_grade(world[IN_ROOM(ch)].harvest_material) * 5);
  dc = HARVEST_BASE_DC + (harvest_level);

  if ((20 + skill_roll + prof_bonus) < dc)
  {
    send_to_char(ch, "You don't have the skill to harvest %s.\r\n",
                 crafting_material_nodes[world[IN_ROOM(ch)].harvest_material]);
    return;
  }

  if (roll == 1)
  {
    /* Critical failure */
    amount = dice(1, 6);
    amount = MIN(amount, world[IN_ROOM(ch)].harvest_material_amount);
    send_to_char(ch,
                 "\tM[CRITICAL FAILURE]\tn You rolled a natural 1! %d of the %s units in this room "
                 "have been ruined!\r\n",
                 amount, crafting_materials[world[IN_ROOM(ch)].harvest_material]);

    world[IN_ROOM(ch)].harvest_material_amount -= amount;

    if (world[IN_ROOM(ch)].harvest_material_amount <= 0)
    {
      send_to_char(ch, "%s has been depleted.\r\n",
                   crafting_material_nodes[world[IN_ROOM(ch)].harvest_material]);
      world[IN_ROOM(ch)].harvest_material = CRAFT_MAT_NONE;
      world[IN_ROOM(ch)].harvest_material_amount = 0;
    }
    GET_CRAFT(ch).craft_duration = 0;
    GET_CRAFT(ch).crafting_method = 0;
    return;
  }
  else if ((roll + skill_roll + prof_bonus) < dc)
  {
    char bonus_text[256];
    char *ptr = bonus_text;
    
    if (spec_bonus > 0)
      ptr += snprintf(ptr, sizeof(bonus_text) - (ptr - bonus_text), " + specialization [%d]", spec_bonus);
    if (prof_bonus > 0)
      ptr += snprintf(ptr, sizeof(bonus_text) - (ptr - bonus_text), " + proficiency [%d]", prof_bonus);
    
    send_to_char(ch,
                 "Roll [%d] + Base Skill [%d]%s = Total [%d] vs. DC [%d]. Failure. You have failed to "
                 "harvest %s.\r\n",
                 roll, base_skill, bonus_text, roll + skill_roll + prof_bonus, dc,
                 crafting_materials[world[IN_ROOM(ch)].harvest_material]);
    GET_CRAFT(ch).craft_duration = 0;
    GET_CRAFT(ch).crafting_method = 0;
    gain_craft_exp(ch, HARVEST_BASE_EXP / 2, skill, TRUE);
    return;
  }
  else
  {
    int happy_hour_mat_bonus = 0;
    int mote_chance = 0;
    
    amount = HARVEST_BASE_AMOUNT;
    amount = MIN(amount, world[IN_ROOM(ch)].harvest_material_amount);

    /* Harvesting success */
    if (roll == 20)
    {
      bonus = HARVEST_BASE_AMOUNT;
      send_to_char(ch,
                   "\tM[CRITICAL SUCCESS!]\tn You rolled a natural 20! You've harvested %d units "
                   "of %s, plus an extra %d units!\r\n",
                   amount, crafting_materials[world[IN_ROOM(ch)].harvest_material], bonus);
      gain_craft_exp(ch, HARVEST_BASE_EXP + (HARVEST_BASE_EXP * harvest_level / 2), skill, TRUE);
      motes_found = TRUE;
    }
    else
    {
      char bonus_text[256];
      char *ptr = bonus_text;
      
      if (spec_bonus > 0)
        ptr += snprintf(ptr, sizeof(bonus_text) - (ptr - bonus_text), " + specialization [%d]", spec_bonus);
      if (prof_bonus > 0)
        ptr += snprintf(ptr, sizeof(bonus_text) - (ptr - bonus_text), " + proficiency [%d]", prof_bonus);
      
      send_to_char(ch,
                   "Roll [%d] + Base Skill [%d]%s = Total [%d] vs. DC [%d]. Success!\r\nYou have harvested "
                   "%d %s from %s.\r\n",
                   roll, base_skill, bonus_text, roll + skill_roll + prof_bonus, dc, amount,
                   crafting_materials[world[IN_ROOM(ch)].harvest_material],
                   crafting_material_nodes[world[IN_ROOM(ch)].harvest_material]);
      gain_craft_exp(ch, HARVEST_BASE_EXP + ((HARVEST_BASE_EXP / 2) * harvest_level / 2), skill, TRUE);
    }

    /* Check for efficient talent - chance to gain 2 extra units */
    int efficient_chance = get_efficient_talent_bonus(ch, skill);
    int efficient_bonus = 0;
    if (efficient_chance > 0 && rand_number(1, 100) <= efficient_chance)
    {
      efficient_bonus = 2;
      send_to_char(ch, "\tC*EFFICIENT*\tn You gain 2 extra units from your efficient harvesting!\r\n");
    }

    world[IN_ROOM(ch)].harvest_material_amount -= amount;

    /* Apply happy hour bonus for materials harvested */
    if (HAPPY_HARVESTING_MATERIALS > 0)
    {
      happy_hour_mat_bonus = MAX(1, ((amount + bonus + efficient_bonus) * HAPPY_HARVESTING_MATERIALS) / 100);
      send_to_char(ch, "\tY*HAPPY HOUR*\tn You gain %d bonus materials!\r\n", happy_hour_mat_bonus);
    }

    GET_CRAFT_MAT(ch, world[IN_ROOM(ch)].harvest_material) += amount + bonus + efficient_bonus + happy_hour_mat_bonus;

    if (world[IN_ROOM(ch)].harvest_material_amount <= 0)
    {
      send_to_char(ch, "%s has been depleted.\r\n",
                   crafting_material_nodes[world[IN_ROOM(ch)].harvest_material]);
      world[IN_ROOM(ch)].harvest_material = CRAFT_MAT_NONE;
      world[IN_ROOM(ch)].harvest_material_amount = 0;
    }

    // random motes
    mote_chance = HARVEST_MOTE_CHANCE;
    if (HAPPY_HARVESTING_MOTES_CHANCE > 0)
    {
      mote_chance += HAPPY_HARVESTING_MOTES_CHANCE;
    }
    
    if ((dice(1, 100) <= mote_chance) || motes_found)
    {
      num_motes =
          dice(MAX(1, material_grade(world[IN_ROOM(ch)].harvest_material)), HARVEST_MOTE_DICE_SIZE);
      num_motes = num_motes * HARVEST_MOTE_MULTIPLIER / 100;
      
      /* Apply happy hour bonus for motes obtained */
      if (HAPPY_HARVESTING_MOTES_OBTAINED > 0)
      {
        num_motes = num_motes + ((num_motes * HAPPY_HARVESTING_MOTES_OBTAINED) / 100);
      }
      
      mote_type = dice(1, NUM_CRAFT_MOTES - 1);
      bonus_motes = 0;
      synergy_talent = TALENT_NONE;
      synergy_rank = 0;

      /* Determine which synergy talent applies to this mote type */
      switch (mote_type)
      {
      case CRAFTING_MOTE_AIR:
        synergy_talent = TALENT_AIR_MOTE_SYNERGY;
        break;
      case CRAFTING_MOTE_DARK:
        synergy_talent = TALENT_DARK_MOTE_SYNERGY;
        break;
      case CRAFTING_MOTE_EARTH:
        synergy_talent = TALENT_EARTH_MOTE_SYNERGY;
        break;
      case CRAFTING_MOTE_FIRE:
        synergy_talent = TALENT_FIRE_MOTE_SYNERGY;
        break;
      case CRAFTING_MOTE_ICE:
        synergy_talent = TALENT_ICE_MOTE_SYNERGY;
        break;
      case CRAFTING_MOTE_LIGHT:
        synergy_talent = TALENT_LIGHT_MOTE_SYNERGY;
        break;
      case CRAFTING_MOTE_LIGHTNING:
        synergy_talent = TALENT_LIGHTNING_MOTE_SYNERGY;
        break;
      case CRAFTING_MOTE_WATER:
        synergy_talent = TALENT_WATER_MOTE_SYNERGY;
        break;
      }

      /* Check for mote synergy talent and apply bonus */
      if (synergy_talent != TALENT_NONE)
      {
        synergy_rank = get_talent_rank(ch, synergy_talent);
        if (synergy_rank > 0)
        {
          bonus_motes = synergy_rank; /* +1 mote per rank */
          send_to_char(ch, "\tC*MOTE SYNERGY (+%d)*\tn ", bonus_motes);
        }
      }
      if (HAPPY_HARVESTING_MOTES_CHANCE > 0)
      {
        send_to_char(ch, "\tY*HAPPY HOUR*\tn ");
      }

      send_to_char(ch, "\tYYou have extracted a small cache of %d %ss!.\r\n",
                   num_motes + bonus_motes, crafting_motes[mote_type]);
      GET_CRAFT_MOTES(ch, mote_type) += (num_motes + bonus_motes);
    }

    GET_CRAFT(ch).craft_duration = 0;
    GET_CRAFT(ch).crafting_method = 0;
    act("$n finishes harvesting.", FALSE, ch, 0, 0, TO_ROOM);
  }
}

void newcraft_harvest(struct char_data *ch, const char *argument)
{
  int seconds = 0;
  int harvest_skill = 0;
  int rapid_reduction = 0;

  if (GET_CRAFT(ch).craft_duration > 0)
  {
    send_to_char(ch, "You cannot harvest until you complete your current task. To cancel, type "
                     "'cancel craft'.\r\n");
    return;
  }

  if (GET_SURVEY_ROOMS(ch) <= 0)
  {
    send_to_char(ch, "You must first survey to locate any resources here.\r\n");
    return;
  }

  if (GET_SURVEY_ROOMS(ch) <= 0)
  {
    send_to_char(ch, "You must survey the area before harvesting.\r\n");
    return;
  }

  if (world[IN_ROOM(ch)].harvest_material == CRAFT_MAT_NONE ||
      world[IN_ROOM(ch)].harvest_material_amount <= 0)
  {
    send_to_char(ch, "There's nothing here to harvest.\r\n");
    return;
  }

  if (!has_proper_harvesting_tool_equipped(ch))
  {
    show_harvesting_tool_needed(ch);
    return;
  }

  /* Get the harvesting skill for this material type */
  harvest_skill = harvesting_skill_by_material(world[IN_ROOM(ch)].harvest_material);

  if (GET_LEVEL(ch) >= LVL_IMMORT)
    seconds = 1;
  else
  {
    seconds = HARVEST_BASE_TIME;

    /* Apply rapid talent reduction if we have a valid skill */
    if (harvest_skill > 0)
    {
      rapid_reduction = get_rapid_talent_bonus(ch, harvest_skill);
      seconds -= rapid_reduction;
      if (seconds < 1)
        seconds = 1;
    }
  }

  GET_CRAFT(ch).crafting_method = SCMD_NEWCRAFT_HARVEST;
  GET_CRAFT(ch).craft_duration = seconds;

  send_to_char(ch, "You begin %s. Survey rooms remaining: %d\r\n", 
               harvesting_messages[world[IN_ROOM(ch)].harvest_material],
               GET_SURVEY_ROOMS(ch));
  act("$n starts harvesting.", FALSE, ch, 0, 0, TO_ROOM);
}

void newcraft_butcher(struct char_data *ch, const char *argument)
{
  char arg[MAX_INPUT_LENGTH];
  struct obj_data *corpse = NULL;
  int material = CRAFT_MAT_NONE;
  int race_type = 0;
  
  /* Parse the argument to find target corpse */
  one_argument(argument, arg, sizeof(arg));
  
  if (!*arg)
  {
    send_to_char(ch, "Butcher what corpse?\r\n");
    return;
  }
  
  /* Find the corpse in the room */
  if (!(corpse = get_obj_in_list_vis(ch, arg, NULL, world[IN_ROOM(ch)].contents)))
  {
    send_to_char(ch, "You don't see that here.\r\n");
    return;
  }
  
  /* Check if it's actually a corpse */
  if (!IS_CORPSE(corpse))
  {
    send_to_char(ch, "That's not a corpse!\r\n");
    return;
  }
  
  /* Check if it can be butchered */
  if (!can_butcher_corpse(corpse))
  {
    if (GET_OBJ_VAL(corpse, 4) != 0)
      send_to_char(ch, "You cannot butcher a player's corpse!\r\n");
    else if (GET_OBJ_VAL(corpse, 7) != 0)
      send_to_char(ch, "You cannot butcher someone's pet!\r\n");
    else if (OBJ_FLAGGED(corpse, ITEM_BUTCHERED))
      send_to_char(ch, "That corpse has already been butchered.\r\n");
    else
    {
      race_type = get_corpse_race_type(corpse);
      send_to_char(ch, "That corpse cannot be butchered for useful materials.\r\n");
    }
    return;
  }
  
  /* Check if player is already busy */
  if (GET_CRAFT(ch).craft_duration > 0)
  {
    send_to_char(ch, "You cannot butcher until you complete your current task. To cancel, type "
                     "'cancel craft'.\r\n");
    return;
  }
  
  /* Determine what material we'll get from this corpse */
  material = determine_butcher_material(corpse);
  
  if (material == CRAFT_MAT_NONE)
  {
    send_to_char(ch, "You can't figure out how to butcher this corpse.\r\n");
    return;
  }
  
  /* Require skinning knife for butchering */
  if (!GET_EQ(ch, WEAR_CRAFT_KNIFE))
  {
    send_to_char(ch, "You need a skinning knife equipped to butcher corpses.\r\n");
    return;
  }
  
  /* Set up the butchering process */
  GET_CRAFT(ch).butcher_material = material;
  
  /* Store the corpse description for progress display */
  if (GET_CRAFT(ch).butcher_corpse_desc)
    free(GET_CRAFT(ch).butcher_corpse_desc);
  GET_CRAFT(ch).butcher_corpse_desc = strdup(corpse->short_description);
  
  /* Start the butchering process */
  int seconds = HARVEST_BASE_TIME;
  int harvest_skill = harvesting_skill_by_material(material);
  
  if (GET_LEVEL(ch) >= LVL_IMMORT)
    seconds = 1;
  else if (harvest_skill > 0)
  {
    int rapid_reduction = get_rapid_talent_bonus(ch, harvest_skill);
    seconds -= rapid_reduction;
    if (seconds < 1)
      seconds = 1;
  }
  
  GET_CRAFT(ch).crafting_method = SCMD_NEWCRAFT_BUTCHER;
  GET_CRAFT(ch).craft_duration = seconds;
  
  race_type = get_corpse_race_type(corpse);
  if (race_type == RACE_TYPE_DRAGON)
    send_to_char(ch, "You begin carefully butchering the dragon corpse...\r\n");
  else if (race_type == RACE_TYPE_MAGICAL_BEAST)
    send_to_char(ch, "You begin skinning the magical beast for its hide...\r\n");
  else
    send_to_char(ch, "You begin skinning the corpse...\r\n");
  
  act("$n starts butchering a corpse.", FALSE, ch, 0, 0, TO_ROOM);
}

void butcher_complete(struct char_data *ch)
{
  int skill = 0, skill_roll = 0, roll = 0, dc = 0;
  int prof_bonus = 0;
  int tool_bonus = 0;
  struct obj_data *skinning_knife = NULL;
  struct obj_data *butcher_corpse = NULL;
  int corpse_level = 0;
  int corpse_race_type = 0;
  int material = GET_CRAFT(ch).butcher_material;
  
  /* Check for skinning knife bonus */
  skinning_knife = GET_EQ(ch, WEAR_CRAFT_KNIFE);
  if (skinning_knife && GET_OBJ_TYPE(skinning_knife) == ITEM_CRAFTING_TOOL)
  {
    /* Tool bonus is stored in value[1] */
    tool_bonus = MAX(0, GET_OBJ_VAL(skinning_knife, 1));
  }

  /* Find a butcherable corpse in the room */
  butcher_corpse = find_butcherable_corpse_in_room(ch);
  if (!butcher_corpse)
  {
    send_to_char(ch, "The corpse you were butchering is gone!\r\n");
    GET_CRAFT(ch).craft_duration = 0;
    GET_CRAFT(ch).crafting_method = 0;
    GET_CRAFT(ch).butcher_material = CRAFT_MAT_NONE;
    if (GET_CRAFT(ch).butcher_corpse_desc)
    {
      free(GET_CRAFT(ch).butcher_corpse_desc);
      GET_CRAFT(ch).butcher_corpse_desc = NULL;
    }
    return;
  }

  corpse_level = get_corpse_level(butcher_corpse);
  corpse_race_type = get_corpse_race_type(butcher_corpse);
  bool is_dragon = (corpse_race_type == RACE_TYPE_DRAGON);

  skill = ABILITY_HARVEST_BUTCHERING;

  roll = d20(ch);
  skill_roll = get_craft_skill_value(ch, skill);
  prof_bonus = get_proficient_talent_bonus(ch, skill);
  
  /* Total skill bonus includes proficiency and tool bonus */
  int total_bonus = prof_bonus + tool_bonus;

  int base_skill = GET_ABILITY(ch, skill);
  int spec_bonus = 0;
  bool is_specialized = (GET_CRAFT(ch).craft_specialization[0] == skill || 
                        GET_CRAFT(ch).craft_specialization[1] == skill);
  if (is_specialized)
    spec_bonus = 5;

  /* DC based on corpse level */
  dc = 10 + (corpse_level / 2);

  if ((20 + skill_roll + total_bonus) < dc)
  {
    send_to_char(ch, "Your skill is too low to butcher this corpse. You will only succeed on a natural 20.\r\n");
  }

  /* Critical failure */
  if (roll == 1)
  {
    send_to_char(ch, "\tM[CRITICAL FAILURE]\tn You rolled a natural 1! You have completely ruined "
                 "the corpse!\r\n");
    SET_BIT_AR(GET_OBJ_EXTRA(butcher_corpse), ITEM_BUTCHERED);
    GET_CRAFT(ch).craft_duration = 0;
    GET_CRAFT(ch).crafting_method = 0;
    GET_CRAFT(ch).butcher_material = CRAFT_MAT_NONE;
    if (GET_CRAFT(ch).butcher_corpse_desc)
    {
      free(GET_CRAFT(ch).butcher_corpse_desc);
      GET_CRAFT(ch).butcher_corpse_desc = NULL;
    }
    return;
  }

  /* Failure */
  if ((roll + skill_roll + total_bonus) < dc)
  {
    char bonus_text[512];
    char *ptr = bonus_text;
    
    if (spec_bonus > 0)
      ptr += snprintf(ptr, sizeof(bonus_text) - (ptr - bonus_text), " + specialization [%d]", spec_bonus);
    if (prof_bonus > 0)
      ptr += snprintf(ptr, sizeof(bonus_text) - (ptr - bonus_text), " + proficiency [%d]", prof_bonus);
    if (tool_bonus > 0)
      ptr += snprintf(ptr, sizeof(bonus_text) - (ptr - bonus_text), " + tool [%d]", tool_bonus);
    
    send_to_char(ch,
                 "Roll [%d] + Base Skill [%d]%s = Total [%d] vs. DC [%d]. Failure. You have failed to "
                 "butcher the corpse.\r\n",
                 roll, base_skill, bonus_text, roll + skill_roll + total_bonus, dc);
    GET_CRAFT(ch).craft_duration = 0;
    GET_CRAFT(ch).crafting_method = 0;
    GET_CRAFT(ch).butcher_material = CRAFT_MAT_NONE;
    if (GET_CRAFT(ch).butcher_corpse_desc)
    {
      free(GET_CRAFT(ch).butcher_corpse_desc);
      GET_CRAFT(ch).butcher_corpse_desc = NULL;
    }
    gain_craft_exp(ch, HARVEST_BASE_EXP / 2, skill, TRUE);
    return;
  }

  /* Success */
  int material_amount = 1 + (roll == 20 ? 1 : 0); /* Crit gives bonus material */
  int efficient_bonus = 0;
  int expertise_bonus = 0;
  
  /* Check for efficient talent - chance to gain 2 extra units */
  int efficient_chance = get_efficient_talent_bonus(ch, skill);
  if (efficient_chance > 0 && rand_number(1, 100) <= efficient_chance)
  {
    efficient_bonus = 2;
    send_to_char(ch, "\tC*EFFICIENT*\tn You gain 2 extra units from your efficient butchering!\r\n");
  }
  
  /* Check for expertise talent - on critical success, chance for extra materials */
  int expertise_rank = 0;
  if (roll == 20)
  {
    expertise_rank = get_talent_rank(ch, TALENT_BUTCHERING_EXPERTISE);
    if (expertise_rank > 0 && rand_number(1, 100) <= (expertise_rank * 10))
    {
      expertise_bonus = 1;
      send_to_char(ch, "\tC*EXPERTISE*\tn You gain an extra material from your expertise!\r\n");
    }
  }
  
  if (roll == 20)
  {
    send_to_char(ch, "\tM[CRITICAL SUCCESS!]\tn You rolled a natural 20! You've harvested %d units of %s!\r\n",
                 material_amount + expertise_bonus, crafting_materials[material]);
    gain_craft_exp(ch, HARVEST_BASE_EXP + (HARVEST_BASE_EXP * corpse_level / 10), skill, TRUE);
  }
  else
  {
    char bonus_text[512];
    char *ptr = bonus_text;
    
    if (spec_bonus > 0)
      ptr += snprintf(ptr, sizeof(bonus_text) - (ptr - bonus_text), " + specialization [%d]", spec_bonus);
    if (prof_bonus > 0)
      ptr += snprintf(ptr, sizeof(bonus_text) - (ptr - bonus_text), " + proficiency [%d]", prof_bonus);
    if (tool_bonus > 0)
      ptr += snprintf(ptr, sizeof(bonus_text) - (ptr - bonus_text), " + tool [%d]", tool_bonus);
    
    send_to_char(ch, "Roll [%d] + Base Skill [%d]%s = Total [%d] vs. DC [%d]. Success!\r\n",
                 roll, base_skill, bonus_text, roll + skill_roll + total_bonus, dc);
    send_to_char(ch, "You have harvested %d %s from the corpse.\r\n",
                 material_amount + efficient_bonus, crafting_materials[material]);
    gain_craft_exp(ch, HARVEST_BASE_EXP + ((HARVEST_BASE_EXP / 2) * corpse_level / 10), skill, TRUE);
  }
  
  GET_CRAFT_MAT(ch, material) += material_amount + efficient_bonus + expertise_bonus;
  
  if (is_dragon)
  {
    act("$n carefully harvests materials from the dragon corpse.", TRUE, ch, 0, 0, TO_ROOM);
  }
  else
  {
    act("$n carefully skins the corpse.", TRUE, ch, 0, 0, TO_ROOM);
  }

  /* Mark corpse as butchered */
  SET_BIT_AR(GET_OBJ_EXTRA(butcher_corpse), ITEM_BUTCHERED);
  send_to_char(ch, "The corpse has been fully butchered.\r\n");

  GET_CRAFT(ch).craft_duration = 0;
  GET_CRAFT(ch).crafting_method = 0;
  GET_CRAFT(ch).butcher_material = CRAFT_MAT_NONE;
  
  /* Free the corpse description */
  if (GET_CRAFT(ch).butcher_corpse_desc)
  {
    free(GET_CRAFT(ch).butcher_corpse_desc);
    GET_CRAFT(ch).butcher_corpse_desc = NULL;
  }
}

void show_harvesting_tool_needed(struct char_data *ch)
{
  int mat_type;
  int mat_group;

  if (!ch || IN_ROOM(ch) == NOWHERE)
  {
    send_to_char(ch, "You must be in a valid room to check harvesting tools.\r\n");
    return;
  }

  if ((mat_type = world[IN_ROOM(ch)].harvest_material) == CRAFT_MAT_NONE)
  {
    send_to_char(ch, "There are no harvestable materials in this room.\r\n");
    return;
  }

  if (mat_type == CRAFT_MAT_COAL)
  {
    send_to_char(ch, "You need a pickaxe equipped to harvest %s.\r\n",
                 crafting_material_nodes[mat_type]);
    return;
  }

  if ((mat_group = craft_group_by_material(mat_type)) == CRAFT_GROUP_NONE)
  {
    send_to_char(ch, "There was an error determining the material group. Please inform staff.\r\n");
    return;
  }

  switch (mat_group)
  {
  case CRAFT_GROUP_CLOTH:
    send_to_char(ch, "You need a harvesting sickle equipped to harvest %s.\r\n",
                 crafting_material_nodes[mat_type]);
    break;
  case CRAFT_GROUP_HARD_METALS:
  case CRAFT_GROUP_SOFT_METALS:
  case CRAFT_GROUP_STONE:
    send_to_char(ch, "You need a pickaxe equipped to harvest %s.\r\n",
                 crafting_material_nodes[mat_type]);
    break;
  case CRAFT_GROUP_HIDES:
    send_to_char(ch, "You need a skinning knife equipped to harvest %s.\r\n",
                 crafting_material_nodes[mat_type]);
    break;
  case CRAFT_GROUP_WOOD:
    send_to_char(ch, "You need a wood axe equipped to harvest %s.\r\n",
                 crafting_material_nodes[mat_type]);
    break;
    default:
    send_to_char(ch, "There was an error determining the required harvesting tool. Please inform "
                     "staff.\r\n");
    break;
  }
}

bool has_proper_harvesting_tool_equipped(struct char_data *ch)
{
  int mat_type;
  int mat_group;
  bool has_tool = FALSE;

  if (!ch || IN_ROOM(ch) == NOWHERE)
    return false;

  if ((mat_type = world[IN_ROOM(ch)].harvest_material) == CRAFT_MAT_NONE)
    return false;

  if (mat_type == CRAFT_MAT_COAL)
    return GET_EQ(ch, WEAR_CRAFT_PICKAXE);

  if ((mat_group = craft_group_by_material(mat_type)) == CRAFT_GROUP_NONE)
    return false;

  switch (mat_group)
  {
  case CRAFT_GROUP_CLOTH:
    has_tool = GET_EQ(ch, WEAR_CRAFT_SICKLE);
    break;
  case CRAFT_GROUP_HARD_METALS:
  case CRAFT_GROUP_SOFT_METALS:
  case CRAFT_GROUP_STONE:
    has_tool = GET_EQ(ch, WEAR_CRAFT_PICKAXE);
    break;
  case CRAFT_GROUP_HIDES:
    has_tool = GET_EQ(ch, WEAR_CRAFT_KNIFE);
    break;
  case CRAFT_GROUP_WOOD:
    has_tool = GET_EQ(ch, WEAR_CRAFT_AXE);
    break;
  }

  return has_tool;
}

void gain_craft_exp(struct char_data *ch, int exp, int abil, bool verbose)
{
  int bonus_percentage = 0;
  int bonus_exp = 0;
  int total_exp = exp;
  int happy_hour_bonus = 0;
  int specialization_bonus = 0;

  // Validate ability/skill type to prevent array out-of-bounds crashes
  if (abil < 0 || abil > NUM_ABILITIES)
  {
    if (verbose)
    {
      send_to_char(ch, "Invalid skill type %d - experience gain cancelled.\r\n", abil);
    }
    return;
  }

  /* Check for specialization bonus (2x experience) */
  if (GET_CRAFT(ch).craft_specialization[0] == abil || 
      GET_CRAFT(ch).craft_specialization[1] == abil)
  {
    specialization_bonus = exp; /* Double the base exp */
    total_exp += specialization_bonus;
  }

  /* Check for insightful talent bonus */
  bonus_percentage = get_insightful_talent_bonus(ch, abil);
  if (bonus_percentage > 0)
  {
    bonus_exp = (exp * bonus_percentage) / 100;
    total_exp += bonus_exp;
  }

  /* Check for happy hour bonus */
  if (abil >= ABILITY_CRAFT_WOODWORKING && abil <= ABILITY_CRAFT_COOKING)
  {
    /* Crafting skills */
    if (HAPPY_CRAFTING_EXP > 0)
    {
      happy_hour_bonus = (exp * HAPPY_CRAFTING_EXP) / 100;
      total_exp += happy_hour_bonus;
    }
  }
  else if (abil >= ABILITY_HARVEST_MINING && abil <= ABILITY_HARVEST_SURVEYING)
  {
    /* Harvesting skills */
    if (HAPPY_HARVESTING_EXP > 0)
    {
      happy_hour_bonus = (exp * HAPPY_HARVESTING_EXP) / 100;
      total_exp += happy_hour_bonus;
    }
  }

  GET_CRAFT_SKILL_EXP(ch, abil) += total_exp;
  if (verbose)
  {
    if (specialization_bonus > 0 || bonus_exp > 0 || happy_hour_bonus > 0)
    {
      send_to_char(ch,
                   "You've gained %d experience points in the '%s' skill",
                   total_exp, ability_names[abil]);
      if (specialization_bonus > 0)
        send_to_char(ch, " (+%d specialization bonus)", specialization_bonus);
      if (bonus_exp > 0)
        send_to_char(ch, " (+%d insightful talent)", bonus_exp);
      if (happy_hour_bonus > 0)
        send_to_char(ch, " (+%d happy hour)", happy_hour_bonus);
      send_to_char(ch, ".\r\n");
    }
    else
    {
      send_to_char(ch, "You've gained %d experience points in the '%s' skill.\r\n", total_exp,
                   ability_names[abil]);
    }
  }
  if (GET_CRAFT_SKILL_EXP(ch, abil) >=
      craft_skill_level_exp(ch, get_craft_skill_value(ch, abil) + 1))
  {
    send_to_char(ch, "\tYYour skill in '%s' has increased from %d to %d!\r\n\tn",
                 ability_names[abil], get_craft_skill_value(ch, abil),
                 get_craft_skill_value(ch, abil) + 1);
    SET_ABILITY(ch, abil, get_craft_skill_value(ch, abil) + 1);
    /* Award 1 crafting talent point per crafting/harvesting skill level up */
    gain_talent_point(ch, 1);
  }
  send_to_char(ch, "You have %d experience points in the '%s' skill and need %d more for rank %d.\r\n", 
    GET_CRAFT_SKILL_EXP(ch, abil), ability_names[abil], 
    craft_skill_level_exp(ch, get_craft_skill_value(ch, abil) + 1) - GET_CRAFT_SKILL_EXP(ch, abil),
    get_craft_skill_value(ch, abil) + 1);
}

void show_refine_noargs(struct char_data *ch)
{
  int i, j;

  send_to_char(ch, "To refine you need to add materials and have the appropriate refining "
                   "equipment in the same room as you.\r\n"
                   "You need to specify one of the following command parameters:\r\n"
                   "- refine add (refine type) [quantity] - Add materials (optional quantity for batches)\r\n"
                   "- refine remove - Cancel your refining project\r\n"
                   "- refine show - Display your current refining project\r\n"
                   "- refine begin - Start the refining process\r\n"
                   "Here are the available refining recipes:\r\n");
  for (i = 1; i < NUM_REFINING_RECIPES; i++)
  {
    send_to_char(ch, "-- %20s (x%d): ", crafting_materials[refining_recipes[i].result[0]],
                 refining_recipes[i].result[1]);
    for (j = 0; j < 3; j++)
    {
      if (refining_recipes[i].materials[j][0] == CRAFT_MAT_NONE)
        continue;
      if (j > 0)
        send_to_char(ch, " & ");
      send_to_char(ch, "%s (x%d)", crafting_materials[refining_recipes[i].materials[j][0]],
                   refining_recipes[i].materials[j][1]);
    }
    send_to_char(ch, " - requires %.*s.\r\n", 9,
                 extra_bits[refining_recipes[i].crafting_station_flag] + 9);
  }
}

void newcraft_refine(struct char_data *ch, const char *argument)
{
  char arg1[50], arg2[50], arg3[50], output[200];
  int i = 0, recipe = 0, material = 0, batch_quantity = 1;
  struct obj_data *obj;
  bool fail = FALSE, station = FALSE;

  three_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2), arg3, sizeof(arg3));

  if (!*arg1)
  {
    show_refine_noargs(ch);
    return;
  }

  if (is_abbrev(arg1, "add"))
  {
    if (!*arg2)
    {
      show_refine_noargs(ch);
      return;
    }

    if (GET_CRAFT(ch).refining_result[0] != 0)
    {
      send_to_char(ch,
                   "You've already prepared a refining project for %s. Type 'refine remove' to "
                   "start over.\r\n",
                   crafting_materials[GET_CRAFT(ch).refining_result[0]]);
      return;
    }

    for (i = 1; i < NUM_REFINING_RECIPES; i++)
    {
      if (is_abbrev(arg2, crafting_materials[refining_recipes[i].result[0]]))
      {
        break;
      }
    }

    if (i >= NUM_REFINING_RECIPES)
    {
      send_to_char(ch, "That is not a valid refining recipe.\r\n");
      return;
    }

    recipe = i;

    /* Check if a quantity was specified */
    if (*arg3)
    {
      batch_quantity = atoi(arg3);
      if (batch_quantity < 1)
      {
        send_to_char(ch, "You must refine at least 1 batch.\r\n");
        return;
      }
      if (batch_quantity > 20)
      {
        send_to_char(ch, "You cannot refine more than 20 batches at once.\r\n");
        return;
      }
    }

    for (i = 0; i < 3; i++)
    {
      if ((material = refining_recipes[recipe].materials[i][0]) != 0)
      {
        /* Special handling for dragonmetal: second material can be any hard metal grade 2+ */
        if (recipe == REFINE_RECIPE_DRAGONMETAL && i == 1)
        {
          /* Check for any grade 2+ hard metal */
          int has_hard_metal_grade2 = 0;
          int j = 0;
          int materials_needed = refining_recipes[recipe].materials[i][1] * batch_quantity;

          for (j = CRAFT_MAT_COPPER; j < NUM_CRAFT_MATS; j++)
          {
            if (craft_group_by_material(j) == CRAFT_GROUP_HARD_METALS && material_grade(j) >= 3 &&
                GET_CRAFT_MAT(ch, j) >= materials_needed)
            {
              has_hard_metal_grade2 = 1;
              break;
            }
          }

          if (!has_hard_metal_grade2)
          {
            send_to_char(
                ch,
                "You need %d units of a grade 3 or higher hard metal to make %d batches of %d units each (%d total units) of %s.\r\n",
                materials_needed, batch_quantity, refining_recipes[recipe].result[1],
                refining_recipes[recipe].result[1] * batch_quantity,
                crafting_materials[refining_recipes[recipe].result[0]]);
            fail = TRUE;
          }
        }
        else
        {
          int materials_needed = refining_recipes[recipe].materials[i][1] * batch_quantity;
          if (GET_CRAFT_MAT(ch, material) < materials_needed)
          {
            send_to_char(ch, "You need %d units of %s to make %d batches of %d units each (%d total units) of %s.\r\n",
                         materials_needed,
                         crafting_materials[refining_recipes[recipe].materials[i][0]],
                         batch_quantity, refining_recipes[recipe].result[1],
                         refining_recipes[recipe].result[1] * batch_quantity,
                         crafting_materials[refining_recipes[recipe].result[0]]);
            fail = TRUE;
          }
        }
      }
    }

    for (obj = world[IN_ROOM(ch)].contents; obj; obj = obj->next_content)
    {
      if (OBJ_FLAGGED(obj, refining_recipes[recipe].crafting_station_flag))
      {
        station = TRUE;
        break;
      }
    }

    if (!station)
    {
      send_to_char(ch, "You need a %.*s in the room with you.\r\n", 9,
                   extra_bits[refining_recipes[recipe].crafting_station_flag] + 9);
      fail = TRUE;
    }


    if (fail)
    {
      send_to_char(ch, "You do not meet all of the requirements to refine %s.\r\n",
                   crafting_materials[refining_recipes[recipe].result[0]]);
      return;
    }

    for (i = 0; i < 3; i++)
    {
      if ((material = refining_recipes[recipe].materials[i][0]) != 0)
      {
        int materials_to_deduct = refining_recipes[recipe].materials[i][1] * batch_quantity;

        /* Special handling for dragonmetal: second material can be any hard metal grade 2+ */
        if (recipe == REFINE_RECIPE_DRAGONMETAL && i == 1)
        {
          /* Find and use the first available grade 2+ hard metal */
          int j = 0;
          for (j = CRAFT_MAT_COPPER; j < NUM_CRAFT_MATS; j++)
          {
            if (craft_group_by_material(j) == CRAFT_GROUP_HARD_METALS && material_grade(j) >= 3 &&
                GET_CRAFT_MAT(ch, j) >= materials_to_deduct)
            {
              material = j;
              GET_CRAFT_MAT(ch, material) -= materials_to_deduct;
              GET_CRAFT(ch).refining_materials[i][0] = material;
              GET_CRAFT(ch).refining_materials[i][1] = materials_to_deduct;
              send_to_char(ch,
                           "You allocate %d units of %s to your refining of %d batches (%d total units) of %s.\r\n",
                           materials_to_deduct, crafting_materials[material],
                           batch_quantity, refining_recipes[recipe].result[1] * batch_quantity,
                           crafting_materials[refining_recipes[recipe].result[0]]);
              break;
            }
          }
        }
        else
        {
          GET_CRAFT_MAT(ch, material) -= materials_to_deduct;
          GET_CRAFT(ch).refining_materials[i][0] = material;
          GET_CRAFT(ch).refining_materials[i][1] = materials_to_deduct;
          send_to_char(ch, "You allocate %d units of %s to your refining of %d batches (%d total units) of %s.\r\n",
                       materials_to_deduct,
                       crafting_materials[refining_recipes[recipe].materials[i][0]],
                       batch_quantity, refining_recipes[recipe].result[1] * batch_quantity,
                       crafting_materials[refining_recipes[recipe].result[0]]);
        }
      }
    }

    GET_CRAFT(ch).crafting_recipe = recipe;
    GET_CRAFT(ch).dc = refining_recipes[recipe].dc;
    GET_CRAFT(ch).refining_result[0] = refining_recipes[recipe].result[0];
    GET_CRAFT(ch).refining_result[1] = refining_recipes[recipe].result[1] * batch_quantity;
    GET_CRAFT(ch).refining_batch_quantity = batch_quantity;

    if (batch_quantity == 1)
    {
      send_to_char(ch, "You are ready to begin refining your %s. Type 'refine begin' to execute.\r\n",
                   crafting_materials[refining_recipes[recipe].result[0]]);
    }
    else
    {
      send_to_char(ch, "You are ready to begin refining %d batches of %s (%d total units). Type 'refine begin' to execute.\r\n",
                   batch_quantity, crafting_materials[refining_recipes[recipe].result[0]],
                   refining_recipes[recipe].result[1] * batch_quantity);
    }
    return;
  }
  else if (is_abbrev(arg1, "remove"))
  {
    if (GET_CRAFT(ch).refining_result[0] == 0)
    {
      send_to_char(ch, "You don't have a refining project going.\r\n");
      return;
    }

    send_to_char(ch, "You cancel your refining project.\r\n");
    reset_current_craft(ch, NULL, TRUE, TRUE);
    return;
  }
  else if (is_abbrev(arg1, "show") || is_abbrev(arg1, "display") || is_abbrev(arg1, "info"))
  {
    if (GET_CRAFT(ch).refining_result[0] == 0)
    {
      send_to_char(ch, "You have not started a refining project.\r\n");
      return;
    }

    send_to_char(ch, "\tc");
    snprintf(output, sizeof(output), "REFINING %s",
             crafting_materials[GET_CRAFT(ch).refining_result[0]]);
    for (i = 0; i < strlen(output); i++)
      output[i] = toupper(output[i]);
    text_line(ch, output, 80, '-', '-');
    send_to_char(ch, "\tc");

    send_to_char(ch, "MATERIALS:\r\n");

    // materials
    for (i = 0; i < 3; i++)
    {
      if (GET_CRAFT(ch).refining_materials[i][0] != 0)
      {
        send_to_char(ch, "-- %d/%d %s unit%s allocated.\r\n",
                     GET_CRAFT(ch).refining_materials[i][1],
                     refining_recipes[GET_CRAFT(ch).crafting_recipe].materials[i][1],
                     crafting_materials[GET_CRAFT(ch).refining_materials[i][0]],
                     GET_CRAFT(ch).refining_materials[i][1] > 1 ? "" : "s");
      }
    }

    send_to_char(ch, "\r\n");

    // result
    send_to_char(ch, "RESULT:\r\n");
    if (GET_CRAFT(ch).refining_batch_quantity == 1)
    {
      send_to_char(ch, "-- %d unit%s of %s.\r\n", GET_CRAFT(ch).refining_result[1],
                   GET_CRAFT(ch).refining_result[1] > 1 ? "s" : "",
                   crafting_materials[GET_CRAFT(ch).refining_result[0]]);
    }
    else
    {
      int per_batch = GET_CRAFT(ch).refining_result[1] / GET_CRAFT(ch).refining_batch_quantity;
      send_to_char(ch, "-- %d batches x %d units = %d total unit%s of %s.\r\n",
                   GET_CRAFT(ch).refining_batch_quantity,
                   per_batch,
                   GET_CRAFT(ch).refining_result[1],
                   GET_CRAFT(ch).refining_result[1] > 1 ? "s" : "",
                   crafting_materials[GET_CRAFT(ch).refining_result[0]]);
    }
    send_to_char(ch, "\r\n");

    // dc and skill
    send_to_char(ch, "SKILL CHECK:\r\n");
    send_to_char(ch, "-- 1d20 + %s skill of %d vs. dc of %d.\r\n",
                 ability_names[refining_recipes[GET_CRAFT(ch).crafting_recipe].skill],
                 get_craft_skill_value(ch, refining_recipes[GET_CRAFT(ch).crafting_recipe].skill),
                 GET_CRAFT(ch).dc);

    send_to_char(ch, "\tc");
    draw_line(ch, 80, '-', '-');
    send_to_char(ch, "\tn");

    return;
  }
  else if (is_abbrev(arg1, "begin") || is_abbrev(arg1, "start"))
  {
    if (GET_CRAFT(ch).refining_result[0] == 0)
    {
      send_to_char(ch, "You don't have a refining project going.\r\n");
      return;
    }

    if (is_refine_ready(ch, TRUE))
    {
      send_to_char(
          ch, "Your refining project is not ready yet. Please type: refine add (refine type).\r\n");
      return;
    }

    for (i = 0; i < NUM_REFINING_RECIPES; i++)
    {
      if (refining_recipes[i].result[0] == GET_CRAFT(ch).refining_result[0])
      {
        break;
      }
    }

    if (i >= NUM_REFINING_RECIPES)
    {
      send_to_char(
          ch,
          "There was an error beginning you refining project. Please inform a staff member.\r\n");
      return;
    }

    recipe = i;

    GET_CRAFT(ch).skill_type = refining_recipes[recipe].skill;
    GET_CRAFT(ch).dc = refining_recipes[recipe].dc;

    // Check if the player is in a room with the required crafting station
    int skill = GET_CRAFT(ch).skill_type;
    if (!has_crafting_station_in_room(ch, skill))
    {
      send_to_char(ch, "You need to be in a room with %s to refine this material.\r\n",
                   get_crafting_station_name(skill));
      return;
    }

    GET_CRAFT(ch).crafting_method = SCMD_NEWCRAFT_REFINE;
    GET_CRAFT(ch).craft_duration = 10 * GET_CRAFT(ch).refining_result[1];
    send_to_char(ch, "You begin refining %d unit%s of %s.\r\n",
                 GET_CRAFT(ch).refining_result[1],
                 GET_CRAFT(ch).refining_result[1] > 1 ? "s" : "",
                 crafting_materials[GET_CRAFT(ch).refining_result[0]]);
    act("$n starts refining.", FALSE, ch, 0, 0, TO_ROOM);
    return;
  }
  else
  {
    show_refine_noargs(ch);
    return;
  }
}

int get_craft_skill_value(struct char_data *ch, int skill_num)
{
  int base_value = GET_ABILITY(ch, skill_num);
  int specialization_bonus = 0;

  /* Check if this skill is specialized (+5 bonus) */
  if (GET_CRAFT(ch).craft_specialization[0] == skill_num || 
      GET_CRAFT(ch).craft_specialization[1] == skill_num)
  {
    specialization_bonus = 5;
  }

  return base_value + specialization_bonus;
}

bool is_refine_ready(struct char_data *ch, bool verbose)
{
  bool fail = FALSE;

  if (GET_CRAFT(ch).refining_result[0] == 0)
  {
    if (verbose)
      send_to_char(ch, "You haven't set a refining result/type\r\n");
    fail = TRUE;
  }

  if (GET_CRAFT(ch).refining_result[1] == 0)
  {
    if (verbose)
      send_to_char(ch, "You haven't set a refining result/type\r\n");
    fail = TRUE;
  }

  if ((refining_recipes[GET_CRAFT(ch).crafting_recipe].materials[0][1] != 0 &&
       (GET_CRAFT(ch).refining_materials[0][1] <
        refining_recipes[GET_CRAFT(ch).crafting_recipe].materials[0][1])))
  {
    if (verbose)
      send_to_char(ch, "You haven't added the primary refining ingredient.\r\n");
    fail = TRUE;
  }

  if ((refining_recipes[GET_CRAFT(ch).crafting_recipe].materials[1][1] != 0 &&
       (GET_CRAFT(ch).refining_materials[1][1] <
        refining_recipes[GET_CRAFT(ch).crafting_recipe].materials[1][1])))
  {
    if (verbose)
      send_to_char(ch, "You haven't added the secondary refining ingredient.\r\n");
    fail = TRUE;
  }

  if ((refining_recipes[GET_CRAFT(ch).crafting_recipe].materials[2][1] != 0 &&
       (GET_CRAFT(ch).refining_materials[2][1] <
        refining_recipes[GET_CRAFT(ch).crafting_recipe].materials[2][1])))
  {
    if (verbose)
      send_to_char(ch, "You haven't added the tertiary refining ingredient.\r\n");
    fail = TRUE;
  }

  return fail;
}

bool is_valid_craft_ability(int ability)
{
  switch (ability)
  {
  case ABILITY_ACROBATICS:
  case ABILITY_STEALTH:
  case ABILITY_RELIGION:
  case ABILITY_PERCEPTION:
  case ABILITY_ATHLETICS:
  case ABILITY_MEDICINE:
  case ABILITY_INTIMIDATE:
  case ABILITY_CONCENTRATION:
  case ABILITY_SPELLCRAFT:
  case ABILITY_APPRAISE:
  case ABILITY_DISCIPLINE:
  case ABILITY_TOTAL_DEFENSE:
  case ABILITY_ARCANA:
  case ABILITY_RIDE:
  case ABILITY_HISTORY:
  case ABILITY_SLEIGHT_OF_HAND:
  case ABILITY_DECEPTION:
  case ABILITY_PERSUASION:
  case ABILITY_DISABLE_DEVICE:
  case ABILITY_DISGUISE:
  case ABILITY_HANDLE_ANIMAL:
  case ABILITY_INSIGHT:
  case ABILITY_NATURE:
  case ABILITY_USE_MAGIC_DEVICE:
  case ABILITY_PERFORM:
  case ABILITY_CRAFT_WOODWORKING:
  case ABILITY_CRAFT_TAILORING:
  case ABILITY_CRAFT_ALCHEMY:
  case ABILITY_CRAFT_ARMORSMITHING:
  case ABILITY_CRAFT_WEAPONSMITHING:
  case ABILITY_CRAFT_BOWMAKING:
  case ABILITY_CRAFT_JEWELCRAFTING:
  case ABILITY_CRAFT_LEATHERWORKING:
  case ABILITY_CRAFT_TRAPMAKING:
  case ABILITY_CRAFT_POISONMAKING:
  case ABILITY_CRAFT_METALWORKING:
  case ABILITY_CRAFT_FISHING:
  case ABILITY_CRAFT_COOKING:
  case ABILITY_HARVEST_MINING:
  case ABILITY_HARVEST_HUNTING:
  case ABILITY_HARVEST_FORESTRY:
  case ABILITY_HARVEST_GATHERING:
    return TRUE;
  }
  return FALSE;
}

int crafting_skill_type(int skill)
{
  switch (skill)
  {
  case ABILITY_CRAFT_WOODWORKING:
  case ABILITY_CRAFT_TAILORING:
  case ABILITY_CRAFT_ARMORSMITHING:
  case ABILITY_CRAFT_WEAPONSMITHING:
  case ABILITY_CRAFT_LEATHERWORKING:
  case ABILITY_CRAFT_JEWELCRAFTING:
  case ABILITY_CRAFT_METALWORKING:
  case ABILITY_CRAFT_ALCHEMY:
    return CRAFT_SKILL_TYPE_CRAFT;

  case ABILITY_HARVEST_MINING:
  case ABILITY_HARVEST_HUNTING:
  case ABILITY_HARVEST_FORESTRY:
  case ABILITY_HARVEST_GATHERING:
  case ABILITY_HARVEST_SURVEYING:
  case ABILITY_HARVEST_BUTCHERING:
    return CRAFT_SKILL_TYPE_HARVEST;

  case ABILITY_CRAFT_BOWMAKING:
  case ABILITY_CRAFT_TRAPMAKING:
  case ABILITY_CRAFT_POISONMAKING:
  case ABILITY_CRAFT_FISHING:
  case ABILITY_CRAFT_COOKING:
    return CRAFT_SKILL_TYPE_NONE;
  }
  return CRAFT_SKILL_TYPE_NONE;
}

bool is_valid_craft_feat(int feat)
{
  if (feat <= FEAT_UNDEFINED || feat >= FEAT_LAST_FEAT)
    return FALSE;

  // must bne learnable
  if (!feat_list[feat].can_learn)
    return FALSE;

  // cannot be epic
  if (feat_list[feat].epic)
    return FALSE;

  // no combat feats for now. Requires extra code to pick which weapon that we aren't spending time on yet
  if (!feat_list[feat].combat_feat)
    return FALSE;

  // no skill or spell focus for similar reason
  switch (feat)
  {
  case FEAT_SPELL_FOCUS:
  case FEAT_GREATER_SPELL_FOCUS:
  case FEAT_SKILL_FOCUS:
    return FALSE;
  }

  // we only allow general, combat, spellcasting, metamagic, psionic and teamwork feats
  if (feat_list[feat].feat_type == FEAT_TYPE_GENERAL ||
      feat_list[feat].feat_type == FEAT_TYPE_COMBAT ||
      feat_list[feat].feat_type == FEAT_TYPE_SPELLCASTING ||
      feat_list[feat].feat_type == FEAT_TYPE_PSIONIC ||
      feat_list[feat].feat_type == FEAT_TYPE_METAMAGIC ||
      feat_list[feat].feat_type == FEAT_TYPE_TEAMWORK)
    return TRUE;

  return FALSE;
}

bool is_valid_craft_class(int ch_class, int location)
{
  if (!IS_SPELLCASTER_CLASS(ch_class))
    return FALSE;

  if (ch_class == CLASS_WARLOCK)
    return FALSE;

  switch (location)
  {
  case APPLY_SPELL_CIRCLE_1:
  case APPLY_SPELL_CIRCLE_2:
  case APPLY_SPELL_CIRCLE_3:
  case APPLY_SPELL_CIRCLE_4:
    switch (ch_class)
    {
    case CLASS_WIZARD:
    case CLASS_SORCERER:
    case CLASS_BARD:
    case CLASS_INQUISITOR:
    case CLASS_SUMMONER:
    case CLASS_RANGER:
    case CLASS_PALADIN:
    case CLASS_CLERIC:
    case CLASS_DRUID:
    case CLASS_ALCHEMIST:
      return TRUE;
    }
    break;
  case APPLY_SPELL_CIRCLE_5:
  case APPLY_SPELL_CIRCLE_6:
    switch (ch_class)
    {
    case CLASS_WIZARD:
    case CLASS_SORCERER:
    case CLASS_BARD:
    case CLASS_INQUISITOR:
    case CLASS_SUMMONER:
    case CLASS_CLERIC:
    case CLASS_DRUID:
    case CLASS_ALCHEMIST:
      return TRUE;
    }
    break;
  case APPLY_SPELL_CIRCLE_7:
  case APPLY_SPELL_CIRCLE_8:
  case APPLY_SPELL_CIRCLE_9:
    switch (ch_class)
    {
    case CLASS_WIZARD:
    case CLASS_SORCERER:
    case CLASS_CLERIC:
    case CLASS_DRUID:
      return TRUE;
    }
    break;
  }
  return FALSE;
}

int craft_recipe_by_type(int type)
{
  switch (type)
  {
  case CRAFT_TYPE_WEAPON:
    return ITEM_WEAPON;
  case CRAFT_TYPE_ARMOR:
    return ITEM_ARMOR;
  case CRAFT_TYPE_MISC:
    return ITEM_WORN;
  case CRAFT_TYPE_INSTRUMENT:
    return ITEM_INSTRUMENT;
  }
  return 0;
}

int craft_misc_type_by_wear_loc(int wear_loc)
{
  switch (wear_loc)
  {
  case ITEM_WEAR_FINGER:
  case ITEM_WEAR_NECK:
  case ITEM_WEAR_WRIST:
  case ITEM_WEAR_EAR:
  case ITEM_WEAR_EYES:
  case ITEM_WEAR_FEET:
  case ITEM_WEAR_HANDS:
  case ITEM_WEAR_ABOUT:
  case ITEM_WEAR_WAIST:
  case ITEM_WEAR_FACE:
  case ITEM_WEAR_ANKLE:
  case ITEM_WEAR_SHOULDERS:
    return CRAFT_TYPE_MISC;
  }
  return CRAFT_TYPE_NONE;
}

void craft_update(void)
{
  struct descriptor_data *d, *d_next;
  struct char_data *ch;
  int i = 0;
  char buf[200];

  for (d = descriptor_list; d; d = d_next)
  {
    d_next = d->next;
    ch = d->character;

    if (!ch)
      continue;

    if (IN_ROOM(ch) == NOWHERE)
    {
      continue;
    }

    if (GET_CRAFT(ch).craft_duration > 0)
    {
      GET_CRAFT(ch).craft_duration--;
      GET_CRAFT(ch).craft_duration = MAX(GET_CRAFT(ch).craft_duration, 0);

      if (GET_CRAFT(ch).craft_duration == 0)
      {
        switch (GET_CRAFT(ch).crafting_method)
        {
        case SCMD_NEWCRAFT_CREATE:
          craft_create_complete(ch);
          break;
        case SCMD_NEWCRAFT_REFINE:
          craft_refine_complete(ch);
          break;
        case SCMD_NEWCRAFT_RESIZE:
          craft_resize_complete(ch);
          break;
        case SCMD_NEWCRAFT_GOLEM:
          craft_golem_complete(ch);
          break;
        case SCMD_NEWCRAFT_SURVEY:
          if (GET_CRAFT(ch).craft_duration && !PRF_FLAGGED(ch, PRF_NO_CRAFT_PROGRESS))
          {
            send_to_char(ch, "Surveying. ");
            for (i = 0; i < GET_CRAFT(ch).craft_duration; i++)
              send_to_char(ch, "*");
            send_to_char(ch, "\r\n");
          }
          else
          {
            survey_complete(ch);
          }
          break;
        case SCMD_NEWCRAFT_HARVEST:
          if (GET_CRAFT(ch).craft_duration && !PRF_FLAGGED(ch, PRF_NO_CRAFT_PROGRESS))
          {
            if (crafting_material_nodes[world[IN_ROOM(ch)].harvest_material_amount] <= 0)
            {
              send_to_char(ch, "The resource is depleted.\r\n");
              GET_CRAFT(ch).crafting_method = 0;
              GET_CRAFT(ch).craft_duration = 0;
            }
            else
            {
              snprintf(buf, sizeof(buf), "%s",
                       harvesting_messages[world[IN_ROOM(ch)].harvest_material]);
              CAP(buf);
              send_to_char(ch, "%s. ", buf);
              for (i = 0; i < GET_CRAFT(ch).craft_duration; i++)
                send_to_char(ch, "*");
              send_to_char(ch, "\r\n");
            }
          }
          else
          {
            harvest_complete(ch);
          }
          break;
        case SCMD_NEWCRAFT_BUTCHER:
          if (GET_CRAFT(ch).craft_duration && !PRF_FLAGGED(ch, PRF_NO_CRAFT_PROGRESS))
          {
            send_to_char(ch, "Butchering. ");
            for (i = 0; i < GET_CRAFT(ch).craft_duration; i++)
              send_to_char(ch, "*");
            send_to_char(ch, "\r\n");
          }
          else
          {
            butcher_complete(ch);
          }
          break;
        case SCMD_NEWCRAFT_SUPPLYORDER:
          craft_supplyorder_complete(ch);
          break;
        default: 
          mudlog(NRM, LVL_STAFF, TRUE, "Error in craft_update complete: invalid crafting method %d for %s", GET_CRAFT(ch).crafting_method, GET_NAME(ch));
          break;
        }
      }
      else
      {
        if (!PRF_FLAGGED(ch, PRF_NO_CRAFT_PROGRESS))
        {
          switch (GET_CRAFT(ch).crafting_method)
          {
          case SCMD_NEWCRAFT_CREATE:
            if (GET_CRAFT(ch).craft_duration % 5 == 0)
            {
              send_to_char(ch, "Crafting %s. ", GET_CRAFT(ch).short_description);
              for (i = 0; i < GET_CRAFT(ch).craft_duration; i++)
                send_to_char(ch, "*");
              send_to_char(ch, "\r\n");
            }
            break;
          case SCMD_NEWCRAFT_REFINE:
            send_to_char(ch, "Refining %s. ", crafting_materials[GET_CRAFT(ch).refining_result[0]]);
            for (i = 0; i < GET_CRAFT(ch).craft_duration; i++)
              send_to_char(ch, "*");
            send_to_char(ch, "\r\n");
            break;
          case SCMD_NEWCRAFT_SURVEY:
            send_to_char(ch, "Surveying. ");
            for (i = 0; i < GET_CRAFT(ch).craft_duration; i++)
              send_to_char(ch, "*");
            send_to_char(ch, "\r\n");
            break;
          case SCMD_NEWCRAFT_RESIZE:
            send_to_char(ch, "Resizing. ");
            for (i = 0; i < GET_CRAFT(ch).craft_duration; i++)
              send_to_char(ch, "*");
            send_to_char(ch, "\r\n");
            break;
          case SCMD_NEWCRAFT_GOLEM:
          {
            const char *golem_type_names[] = {"", "wood", "stone", "iron"};
            const char *golem_size_names[] = {"small", "medium", "large", "huge"};
            if (GET_CRAFT(ch).craft_duration % 5 == 0)
            {
              send_to_char(ch,
                           "Constructing a %s %s golem. (Turn on 'no craft progress' in prefedit "
                           "to hide this)",
                           golem_size_names[GET_CRAFT(ch).golem_size],
                           golem_type_names[GET_CRAFT(ch).golem_type]);
              for (i = 0; i < GET_CRAFT(ch).craft_duration; i++)
                send_to_char(ch, "*");
              send_to_char(ch, "\r\n");
            }
          }
          break;
          case SCMD_NEWCRAFT_BUTCHER:
            if (GET_CRAFT(ch).butcher_material == CRAFT_MAT_NONE)
            {
              send_to_char(ch, "The corpse is gone.\r\n");
              GET_CRAFT(ch).crafting_method = 0;
              GET_CRAFT(ch).craft_duration = 0;
              if (GET_CRAFT(ch).butcher_corpse_desc)
              {
                free(GET_CRAFT(ch).butcher_corpse_desc);
                GET_CRAFT(ch).butcher_corpse_desc = NULL;
              }
            }
            else if (GET_CRAFT(ch).craft_duration % 2 == 0)
            {
              if (GET_CRAFT(ch).butcher_corpse_desc)
              {
                send_to_char(ch, "Butchering %s. ",
                             GET_CRAFT(ch).butcher_corpse_desc);
              }
              else
              {
                send_to_char(ch, "Butchering %s. ",
                             crafting_materials[GET_CRAFT(ch).butcher_material]);
              }
              for (i = 0; i < GET_CRAFT(ch).craft_duration; i++)
                send_to_char(ch, "*");
              send_to_char(ch, "\r\n");
            }
            break;
          case SCMD_NEWCRAFT_HARVEST:
            if (crafting_material_nodes[world[IN_ROOM(ch)].harvest_material_amount] <= 0)
            {
              send_to_char(ch, "The resource is depleted.\r\n");
              GET_CRAFT(ch).crafting_method = 0;
              GET_CRAFT(ch).craft_duration = 0;
            }
            else if (GET_CRAFT(ch).craft_duration % 2 == 0)
            {
              snprintf(buf, sizeof(buf), "%s",
                       harvesting_messages[world[IN_ROOM(ch)].harvest_material]);
              CAP(buf);
              send_to_char(ch, "%s. ", buf);
              for (i = 0; i < GET_CRAFT(ch).craft_duration; i++)
                send_to_char(ch, "*");
              send_to_char(ch, "\r\n");
            }
            break;
          case SCMD_NEWCRAFT_SUPPLYORDER:
            if (GET_CRAFT(ch).craft_duration % 5 == 0)
            {
              int current_item = GET_CRAFT(ch).supply_num_required - num_supply_order_requisitions_to_go(ch) + 1;
              int total_items = GET_CRAFT(ch).supply_num_required;
              
              send_to_char(ch, "Supply order for %s %d of %d. ", get_supply_order_item_desc(ch), current_item, total_items);
              for (i = 0; i < GET_CRAFT(ch).craft_duration; i++)
                send_to_char(ch, "*");
              send_to_char(ch, "\r\n");
            }
            break;
          default:
              mudlog(NRM, LVL_STAFF, TRUE, "Error in craft_update: invalid crafting method %d for %s", GET_CRAFT(ch).crafting_method, GET_NAME(ch));
              break;
          }
        }
      }
    }
  }
}

ACMD(do_setmaterial)
{
  int i = 0, j = 0, type = 0, amount = 0;
  char target[100], mat_type[100], mat_amount[100], buf[200];
  struct char_data *tch = NULL;

  three_arguments(argument, target, sizeof(target), mat_type, sizeof(mat_type), mat_amount,
                  sizeof(mat_amount));

  if (!*target || !*mat_type || !*mat_amount)
  {
    if (subcmd == SCMD_SETMATERIALS)
    {
      send_to_char(ch, "You need to specify whose materials to change, the material type, and how "
                     "many to set to.\r\n"
                     "Eg. setmaterial gicker alchemical-silver 10.\r\n"
                     "This will set gicker's alchemical silver material count to 10.\r\n"); 
    }
    else
    {
      send_to_char(ch, "You need to specify whose materials to change, the material type, and how "
                     "many to give/reduce.\r\n"
                     "Eg. setmaterial gicker alchemical-silver 10.\r\n"
                     "This will give gicker additional alchemical silver material count of 10.\r\n"); 
    }
  }

  if (!(tch = get_char_vis(ch, target, NULL, FIND_CHAR_WORLD)))
  {
    send_to_char(ch, "There is no one by that name online.\r\n");
    return;
  }

  if (IS_NPC(tch))
  {
    send_to_char(ch, "You cannot set materials on NPCs.\r\n");
    return;
  }

  for (i = 1; i < NUM_CRAFT_MATS; i++)
  {
    for (j = 0; j < strlen(mat_type); j++)
      if (mat_type[j] == '-')
        mat_type[j] = ' ';
    if (is_abbrev(mat_type, crafting_materials[i]))
    {
      type = i;
      break;
    }
  }

  if (type == 0)
  {
    for (i = 1; i < NUM_CRAFT_MOTES; i++)
    {
      for (j = 0; j < strlen(mat_type); j++)
        if (mat_type[j] == '-')
          mat_type[j] = ' ';
      if (is_abbrev(mat_type, crafting_motes[i]))
      {
        type = i;
        break;
      }
    }
    if (type == 0)
    {
      send_to_char(ch, "That is not a valid crafting material. If the material name has multiple "
                       "words, connect them with a dash - instead of a space.\r\n");
      return;
    }
    if ((amount = atoi(mat_amount)) == 0)
    {
      send_to_char(ch, "You must specify a positive or negative number. Positive will give mote "
                       "units, negative will take them away.\r\n");
      return;
    }
    else
    {
      if (subcmd == SCMD_SETMATERIALS)
        GET_CRAFT_MOTES(tch, type) = amount;
      else
        GET_CRAFT_MOTES(tch, type) += amount;

      if (GET_CRAFT_MOTES(tch, type) < 0)
      {
        snprintf(buf, sizeof(buf), "$n has set your %s units to 0.", crafting_motes[type]);
        act(buf, FALSE, ch, 0, tch, TO_VICT);
        snprintf(buf, sizeof(buf), "You have set $N's %s units to 0.", crafting_motes[type]);
        act(buf, FALSE, ch, 0, tch, TO_CHAR);
        GET_CRAFT_MOTES(tch, type) = 0;
      }
      else
      {
        if (subcmd == SCMD_SETMATERIALS)
        {
          snprintf(buf, sizeof(buf), "$n has set your %s units to %d.", crafting_motes[type], GET_CRAFT_MOTES(tch, type));
          act(buf, FALSE, ch, 0, tch, TO_VICT);
          snprintf(buf, sizeof(buf), "You have set $N's %s units to %d.", crafting_motes[type], GET_CRAFT_MOTES(tch, type));
          act(buf, FALSE, ch, 0, tch, TO_CHAR);
        }
        else
        {
          snprintf(buf, sizeof(buf), "$n has given you %d %s units making your new total on-hand %d.", amount, crafting_motes[type], GET_CRAFT_MOTES(tch, type));
          act(buf, FALSE, ch, 0, tch, TO_VICT);
          snprintf(buf, sizeof(buf), "You have given $N %d %s units making their new total on-hand %d.", amount, crafting_motes[type], GET_CRAFT_MOTES(tch, type));
          act(buf, FALSE, ch, 0, tch, TO_CHAR);
        }
      }
      return;
    }
  }

  if ((amount = atoi(mat_amount)) == 0)
  {
    send_to_char(ch, "You must specify a positive or negative number. Positive will give material "
                     "units, negative will take them away.\r\n");
    return;
  }
  else
  {
    if (subcmd == SCMD_SETMATERIALS)
        GET_CRAFT_MAT(tch, type) = amount;
      else
        GET_CRAFT_MAT(tch, type) += amount;

      if (GET_CRAFT_MAT(tch, type) < 0)
      {
        snprintf(buf, sizeof(buf), "$n has set your %s units to 0.", crafting_materials[type]);
        act(buf, FALSE, ch, 0, tch, TO_VICT);
        snprintf(buf, sizeof(buf), "You have set $N's %s units to 0.", crafting_materials[type]);
        act(buf, FALSE, ch, 0, tch, TO_CHAR);
        GET_CRAFT_MAT(tch, type) = 0;
      }
      else
      {
        if (subcmd == SCMD_SETMATERIALS)
        {
          snprintf(buf, sizeof(buf), "$n has set your %s units to %d.", crafting_materials[type], GET_CRAFT_MAT(tch, type));
          act(buf, FALSE, ch, 0, tch, TO_VICT);
          snprintf(buf, sizeof(buf), "You have set $N's %s units to %d.", crafting_materials[type], GET_CRAFT_MAT(tch, type));
          act(buf, FALSE, ch, 0, tch, TO_CHAR);
        }
        else
        {
          snprintf(buf, sizeof(buf), "$n has given you %d %s units making your new total on-hand %d.", amount, crafting_materials[type], GET_CRAFT_MAT(tch, type));
          act(buf, FALSE, ch, 0, tch, TO_VICT);
          snprintf(buf, sizeof(buf), "You have given $N %d %s units making their new total on-hand %d.", amount, crafting_materials[type], GET_CRAFT_MAT(tch, type));
          act(buf, FALSE, ch, 0, tch, TO_CHAR);
        }
      }
  }
}

ACMD(do_list_craft_materials)
{
  char arg[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  char buf[MAX_INPUT_LENGTH];
  struct obj_data *obj = NULL;
  int craft_material = CRAFT_MAT_NONE;
  int quantity = 1;
  int i = 0, mat = 0, count = 0;

  half_chop_c(argument, arg, sizeof(arg), arg2, sizeof(arg2));

  /* Handle 'list_craft_materials store <item>' */
  if (*arg && !str_cmp(arg, "store"))
  {
    if (!*arg2)
    {
      send_to_char(ch, "Store which item?\r\n");
      return;
    }

    /* Find the item in inventory */
    if (!(obj = get_obj_in_list_vis(ch, arg2, NULL, ch->carrying)))
    {
      send_to_char(ch, "You don't have %s %s.\r\n", AN(arg2), arg2);
      return;
    }

    /* Check if it's a material item */
    if (GET_OBJ_TYPE(obj) != ITEM_MATERIAL)
    {
      send_to_char(ch, "%s is not a crafting material.\r\n", CAP(GET_OBJ_SHORT(obj)));
      return;
    }

    /* Get quantity from object (VAL 0) */
    quantity = MAX(1, GET_OBJ_VAL(obj, 0));

    /* Convert object material to craft material type */
    craft_material = obj_material_to_craft_material(GET_OBJ_MATERIAL(obj));

    if (craft_material == CRAFT_MAT_NONE)
    {
      send_to_char(ch, "%s cannot be stored as crafting material.\r\n", CAP(GET_OBJ_SHORT(obj)));
      return;
    }

    int stored = quantity;

    if (stored > 0)
    {
      GET_CRAFT_MAT(ch, craft_material) += stored;
      send_to_char(ch, "You store %d unit%s of %s.\r\n", stored, stored == 1 ? "" : "s",
                   crafting_materials[craft_material]);
      extract_obj(obj);
    }
    else
    {
      send_to_char(ch, "You don't have enough storage space for that material.\r\n");
    }

    return;
  }

  /* Handle 'list_craft_materials unstore <# to unstore> <name of material>' */
  if (*arg && !str_cmp(arg, "unstore")) // disabled for now
  {
    char quantity_str[MAX_INPUT_LENGTH];
    char material_name_buf[MAX_INPUT_LENGTH];
    int unstore_quantity = 0;
    int material_type = CRAFT_MAT_NONE;
    int j = 0;
    struct obj_data *new_mat_obj = NULL;
    int obj_material = MATERIAL_UNDEFINED;

    /* Parse arguments using half_chop: unstore <qty> <material> */
    half_chop(arg2, quantity_str, material_name_buf);

    if (!*quantity_str || !*material_name_buf)
    {
      send_to_char(ch, "Usage: materials unstore <quantity> <material name>\r\n");
      send_to_char(ch, "Example: materials unstore 5 copper\r\n");
      return;
    }

    unstore_quantity = atoi(quantity_str);

    if (unstore_quantity <= 0)
    {
      send_to_char(ch, "You must unstore at least 1 unit.\r\n");
      return;
    }

    /* Find the material type by name */
    for (j = 0; j < NUM_CRAFT_MATS; j++)
    {
      if (is_abbrev(material_name_buf, crafting_materials[j]))
      {
        material_type = j;
        break;
      }
    }

    if (material_type == CRAFT_MAT_NONE)
    {
      send_to_char(ch, "Unknown material '%s'.\r\n", material_name_buf);
      return;
    }

    if (GET_CRAFT_MAT(ch, material_type) < unstore_quantity)
    {
      send_to_char(
          ch, "You only have %d unit%s of %s stored.\r\n", GET_CRAFT_MAT(ch, material_type),
          GET_CRAFT_MAT(ch, material_type) == 1 ? "" : "s", crafting_materials[material_type]);
      return;
    }

    /* Create ITEM_MATERIAL object */
    if ((new_mat_obj = read_object(ITEM_PROTOTYPE, VIRTUAL)) == NULL)
    {
      send_to_char(ch, "Error: Could not create material object.\r\n");
      return;
    }

    /* Convert craft material type to object material */
    obj_material = craft_material_to_obj_material(material_type);
    if (obj_material == MATERIAL_UNDEFINED)
    {
      send_to_char(ch, "%s cannot be unstored as a physical material bundle.\r\n",
                   crafting_materials[material_type]);
      return;
    }

    /* Set up the material object */
    GET_OBJ_TYPE(new_mat_obj) = ITEM_MATERIAL;
    GET_OBJ_MATERIAL(new_mat_obj) = obj_material;
    GET_OBJ_VAL(new_mat_obj, 0) = unstore_quantity;
    GET_OBJ_VAL(new_mat_obj, 1) = 0;
    GET_OBJ_VAL(new_mat_obj, 2) = 0;
    GET_OBJ_VAL(new_mat_obj, 3) = 0;

    /* Set keywords for interaction */
    snprintf(buf, sizeof(buf), "material %s bundle", crafting_materials[material_type]);
    new_mat_obj->name = strdup(buf);

    /* Set short description */
    snprintf(buf, sizeof(buf), "bundle of %d %s", unstore_quantity,
             crafting_materials[material_type]);
    new_mat_obj->short_description = strdup(buf);

    /* Set long description */
    snprintf(buf, sizeof(buf), "A bundle of %d %s material (%d unit%s) lies here.",
             unstore_quantity, crafting_materials[material_type], unstore_quantity,
             unstore_quantity == 1 ? "" : "s");
    new_mat_obj->description = strdup(buf);

    /* Give to player */
    obj_to_char(new_mat_obj, ch);

    /* Deduct from storage */
    GET_CRAFT_MAT(ch, material_type) -= unstore_quantity;

    send_to_char(ch, "You unstore %d unit%s of %s.\r\n", unstore_quantity,
                 unstore_quantity == 1 ? "" : "s", crafting_materials[material_type]);

    return;
  }

  /* Handle invalid arguments */
  if (*arg)
  {
    send_to_char(ch, "Usage: materials [store <item>] [unstore <quantity> <material>]\r\n");
    send_to_char(ch, "  materials        - Show your stored materials\r\n");
    send_to_char(ch, "  materials store <item> - Store a material item\r\n");
    send_to_char(ch, "  materials unstore <qty> <material> - Unstore materials\r\n");
    return;
  }

  /* Show crafting materials by default */

  send_to_char(ch, "\tc");
  text_line(ch, "HARD METALS", 80, '-', '-');
  send_to_char(ch, "\tn");

  for (i = 0; i < NUM_CRAFT_MATS; i++)
  {
    mat = materials_sort_info[i];
    if (craft_group_by_material(mat) != CRAFT_GROUP_HARD_METALS)
      continue;
    send_to_char(ch, "%5d %-20s ", GET_CRAFT_MAT(ch, mat), crafting_materials[mat]);
    if ((count % 3) == 2)
      send_to_char(ch, "\r\n");
    count++;
  }

  if ((count % 3) == 1)
    send_to_char(ch, "\r\n");

  count = 0;
  send_to_char(ch, "\tc");
  text_line(ch, "SOFT METALS", 80, '-', '-');
  send_to_char(ch, "\tn");

  for (i = 0; i < NUM_CRAFT_MATS; i++)
  {
    mat = materials_sort_info[i];
    if (craft_group_by_material(mat) != CRAFT_GROUP_SOFT_METALS)
      continue;
    send_to_char(ch, "%5d %-20s ", GET_CRAFT_MAT(ch, mat), crafting_materials[mat]);
    if ((count % 3) == 2)
      send_to_char(ch, "\r\n");
    count++;
  }

  if ((count % 3) == 0)
    send_to_char(ch, "\r\n");
  send_to_char(ch, "\r\n");

  count = 0;
  send_to_char(ch, "\tc");
  text_line(ch, "HIDES", 80, '-', '-');
  send_to_char(ch, "\tn");

  for (i = 0; i < NUM_CRAFT_MATS; i++)
  {
    mat = materials_sort_info[i];
    if (craft_group_by_material(mat) != CRAFT_GROUP_HIDES)
      continue;
    send_to_char(ch, "%5d %-20s ", GET_CRAFT_MAT(ch, mat), crafting_materials[mat]);
    if ((count % 3) == 2)
      send_to_char(ch, "\r\n");
    count++;
  }

  if ((count % 3) == 0)
    send_to_char(ch, "\r\n");
  send_to_char(ch, "\r\n");

  count = 0;
  send_to_char(ch, "\tc");
  text_line(ch, "WOOD", 80, '-', '-');
  send_to_char(ch, "\tn");

  for (i = 0; i < NUM_CRAFT_MATS; i++)
  {
    mat = materials_sort_info[i];
    if (craft_group_by_material(mat) != CRAFT_GROUP_WOOD)
      continue;
    send_to_char(ch, "%5d %-20s ", GET_CRAFT_MAT(ch, mat), crafting_materials[mat]);
    if ((count % 3) == 2)
      send_to_char(ch, "\r\n");
    count++;
  }

  if ((count % 3) == 1)
    send_to_char(ch, "\r\n");

  count = 0;
  send_to_char(ch, "\tc");
  text_line(ch, "CLOTH", 80, '-', '-');
  send_to_char(ch, "\tn");

  for (i = 0; i < NUM_CRAFT_MATS; i++)
  {
    mat = materials_sort_info[i];
    if (craft_group_by_material(mat) != CRAFT_GROUP_CLOTH)
      continue;
    send_to_char(ch, "%5d %-20s ", GET_CRAFT_MAT(ch, mat), crafting_materials[mat]);
    if ((count % 3) == 2)
      send_to_char(ch, "\r\n");
    count++;
  }

  if ((count % 3) == 0)
    send_to_char(ch, "\r\n");
  send_to_char(ch, "\r\n");

  count = 0;
  send_to_char(ch, "\tc");
  text_line(ch, "STONE", 80, '-', '-');
  send_to_char(ch, "\tn");

  for (i = 0; i < NUM_CRAFT_MATS; i++)
  {
    mat = materials_sort_info[i];
    if (craft_group_by_material(mat) != CRAFT_GROUP_STONE)
      continue;
    send_to_char(ch, "%5d %-20s ", GET_CRAFT_MAT(ch, mat), crafting_materials[mat]);
    if ((count % 3) == 2)
      send_to_char(ch, "\r\n");
    count++;
  }

  if ((count % 3) == 0)
    send_to_char(ch, "\r\n");
  send_to_char(ch, "\r\n");

  count = 0;
  send_to_char(ch, "\tc");
  text_line(ch, "REFINING MATERIALS", 80, '-', '-');
  send_to_char(ch, "\tn");

  for (i = 0; i < NUM_CRAFT_MATS; i++)
  {
    mat = materials_sort_info[i];
    if (craft_group_by_material(mat) != CRAFT_GROUP_REFINING)
      continue;
    send_to_char(ch, "%5d %-20s ", GET_CRAFT_MAT(ch, mat), crafting_materials[mat]);
    if ((count % 3) == 2)
      send_to_char(ch, "\r\n");
    count++;
  }

  if ((count % 3) == 0)
    send_to_char(ch, "\r\n");
  send_to_char(ch, "\r\n");

  count = 0;
  send_to_char(ch, "\tc");
  text_line(ch, "ELEMENTAL MOTES", 80, '-', '-');
  send_to_char(ch, "\tn");

  for (i = 1; i < NUM_CRAFT_MOTES; i++)
  {
    send_to_char(ch, "%5d %-20s ", GET_CRAFT_MOTES(ch, i), crafting_motes[i]);
    if ((count % 3) == 2)
      send_to_char(ch, "\r\n");
    count++;
  }

  if ((count % 3) == 0)
    send_to_char(ch, "\r\n");
  send_to_char(ch, "\r\n");

  send_to_char(ch, "\tc");
  draw_line(ch, 80, '-', '-');
  send_to_char(ch, "\tn");
}

int compare_materials(const void *x, const void *y)
{
  int a = *(const int *)x, b = *(const int *)y;

  return strcmp(crafting_materials[a], crafting_materials[b]);
}

/* sort materials called at boot up */
void sort_materials(void)
{
  int a;

  /* initialize array, avoiding reserved. */
  for (a = 0; a < NUM_CRAFT_MATS; a++)
    materials_sort_info[a] = a;

  qsort(&materials_sort_info[0], NUM_CRAFT_MATS, sizeof(int), compare_materials);
}

int craft_skill_level_exp(struct char_data *ch, int level)
{
  if (level == 0)
    return 0;
  else
    return (level * 1000) + craft_skill_level_exp(ch, level - 1);
}

int get_level_adjustment_by_apply_and_modifier(int apply, int mod, int btype)
{
  if (!is_valid_apply(apply) || mod == 0 || btype == BONUS_TYPE_UNDEFINED)
    return 0;

  int level_adj = 0; // this is the level adjustment returned, added to the object min level to use
  float div = 1.0;   // this is how much to divide the modifier by.

  switch (apply)
  {
  case APPLY_STR:
  case APPLY_DEX:
  case APPLY_INT:
  case APPLY_WIS:
  case APPLY_CON:
  case APPLY_CHA:
  case APPLY_SPELL_PENETRATION:
  case APPLY_PSP_REGEN:
  case APPLY_HP_REGEN:
  case APPLY_FAST_HEALING:
  case APPLY_DAMROLL:
  case APPLY_SKILL:
    div = 6.0;
    break;

  case APPLY_HIT:
    div = 0.5;
    break;

  case APPLY_RES_FIRE:
  case APPLY_RES_SLICE:
  case APPLY_RES_ELECTRIC:
  case APPLY_RES_SOUND:
  case APPLY_RES_COLD:
  case APPLY_RES_PUNCTURE:
  case APPLY_RES_POISON:
  case APPLY_RES_WATER:
  case APPLY_RES_EARTH:
  case APPLY_RES_ACID:
  case APPLY_RES_AIR:
  case APPLY_RES_FORCE:
  case APPLY_RES_ILLUSION:
  case APPLY_RES_ENERGY:
  case APPLY_RES_HOLY:
  case APPLY_RES_MENTAL:
  case APPLY_RES_LIGHT:
  case APPLY_RES_UNHOLY:
  case APPLY_RES_DISEASE:
  case APPLY_RES_NEGATIVE:
  case APPLY_POWER_RES:
  case APPLY_SPELL_DURATION:
  case APPLY_SPELL_POTENCY:
  case APPLY_ENCUMBRANCE:
    div = 3.0;
    break;

  case APPLY_SAVING_REFL:
  case APPLY_SAVING_WILL:
  case APPLY_SAVING_FORT:
  case APPLY_INITIATIVE:
  case APPLY_AC_NEW:
    div = 5.0;
    break;

  case APPLY_PSP:
    div = 1.0;
    break;

  case APPLY_SPELL_CIRCLE_1:
  case APPLY_SPELL_CIRCLE_2:
  case APPLY_SPELL_CIRCLE_3:
  case APPLY_SPELL_RES:
  case APPLY_SPELL_DC:
    div = 10.0;
    break;
  case APPLY_SPELL_CIRCLE_4:
  case APPLY_SPELL_CIRCLE_5:
  case APPLY_SPELL_CIRCLE_6:
    div = 20.0;
    break;
  case APPLY_SPELL_CIRCLE_7:
  case APPLY_SPELL_CIRCLE_8:
  case APPLY_SPELL_CIRCLE_9:
  case APPLY_FEAT:
    div = 30.0;
    break;

  case APPLY_MOVE:
    div = 0.3;
    break;

  case APPLY_MV_REGEN:
    div = 0.6;
    break;

  case APPLY_HITROLL:
    div = 7.5;
    break;
  }

  if (btype == BONUS_TYPE_ENHANCEMENT)
    div /= 2;

  level_adj = (int)MAX(1, mod * div);

  return level_adj;
}

int get_level_adjustment_by_enhancement_bonus(int bonus_amt)
{
  return bonus_amt * 3.75;
}

int get_craft_obj_level(struct obj_data *obj, struct char_data *ch)
{
  if (!obj)
    return 1;

  int i = 0, level = 0;

  for (i = 0; i < MAX_OBJ_AFFECT; i++)
  {
    if (!is_valid_apply(obj->affected[i].location))
      continue;
    if (obj->affected[i].modifier == 0)
      continue;
    if (obj->affected[i].bonus_type != BONUS_TYPE_UNIVERSAL &&
        obj->affected[i].bonus_type != BONUS_TYPE_ENHANCEMENT)
      continue;
    level += get_level_adjustment_by_apply_and_modifier(
        obj->affected[i].location, obj->affected[i].modifier, obj->affected[i].bonus_type);
    // send_to_char(ch, "%s (%s) +%d = %d\r\n",
    //     apply_types[obj->affected[i].location], bonus_types[obj->affected[i].bonus_type], obj->affected[i].modifier,
    //     get_level_adjustment_by_apply_and_modifier(obj->affected[i].location, obj->affected[i].modifier, obj->affected[i].bonus_type));
  }

  if (GET_OBJ_TYPE(obj) == ITEM_WEAPON)
  {
    level += get_level_adjustment_by_enhancement_bonus(GET_OBJ_VAL(obj, 4));
    // send_to_char(ch, "Weapon Enh +%d = %d\r\n", GET_OBJ_VAL(obj, 4), get_level_adjustment_by_enhancement_bonus(GET_OBJ_VAL(obj, 4)));
  }
  else if (GET_OBJ_TYPE(obj) == ITEM_ARMOR)
  {
    level += get_level_adjustment_by_enhancement_bonus(GET_OBJ_VAL(obj, 4));
    // send_to_char(ch, "Armor Enh +%d = %d\r\n", GET_OBJ_VAL(obj, 4), get_level_adjustment_by_enhancement_bonus(GET_OBJ_VAL(obj, 4)));
  }

  // material adjustment
  level += get_craft_material_final_level_adjustment(ch);
  // send_to_char(ch, "Material Adjustment = %d\r\n", get_craft_material_final_level_adjustment(ch));

  // crafdter's attempted level adjustment
  level += GET_CRAFT(ch).level_adjust;

  return level;
}

int get_craft_project_level(struct char_data *ch)
{
  int i = 0, level = 0;

  // bonuses
  for (i = 0; i < MAX_OBJ_AFFECT; i++)
  {
    if (!is_valid_apply(GET_CRAFT(ch).affected[i].location))
      continue;
    if (GET_CRAFT(ch).affected[i].modifier == 0)
      continue;
    if (GET_CRAFT(ch).affected[i].bonus_type != BONUS_TYPE_UNIVERSAL &&
        GET_CRAFT(ch).affected[i].bonus_type != BONUS_TYPE_ENHANCEMENT)
      continue;
    level += get_level_adjustment_by_apply_and_modifier(GET_CRAFT(ch).affected[i].location,
                                                        GET_CRAFT(ch).affected[i].modifier,
                                                        GET_CRAFT(ch).affected[i].bonus_type);
  }

  // enhancement bonus
  if (GET_CRAFT(ch).crafting_item_type == CRAFT_TYPE_WEAPON)
    level += get_level_adjustment_by_enhancement_bonus(GET_CRAFT(ch).enhancement);
  else if (GET_CRAFT(ch).crafting_item_type == CRAFT_TYPE_ARMOR)
    level += get_level_adjustment_by_enhancement_bonus(GET_CRAFT(ch).enhancement);

  // material adjustment
  level += get_craft_material_final_level_adjustment(ch);

  if (GET_CRAFT(ch).crafting_item_type == CRAFT_TYPE_INSTRUMENT)
    level += get_crafting_instrument_dc_modifier(ch);

  return MAX(1, level);
}

int get_craft_material_final_level_adjustment(struct char_data *ch)
{
  int i = 0, level = 0, mat = 0, num_mats = 0, total_mats = 0, mat_level = 0;

  for (i = 0; i < NUM_CRAFT_GROUPS; i++)
  {
    mat = GET_CRAFT(ch).materials[i][0];
    num_mats = GET_CRAFT(ch).materials[i][1];
    if (mat != CRAFT_MAT_NONE && num_mats > 0)
    {
      mat_level += craft_material_level_adjustment(mat) * num_mats;
      total_mats += num_mats;
    }
  }
  // the material level adjustment is the average adjustment for all materials
  if (total_mats > 0)
    level -= (mat_level / total_mats);

  return level;
}

ACMD(do_craftbonuses)
{
  int i = 0, count = 0;

  send_to_char(ch, "\tCPOSSIBLE CRAFTING BONUSES\tn\r\n");
  send_to_char(ch, "\tc");
  draw_line(ch, 80, '-', '-');
  send_to_char(ch, "\tn");

  for (i = 0; i < NUM_APPLIES; i++)
  {
    if (!is_valid_apply(i))
      continue;
    send_to_char(ch, "%-25s ", apply_types[i]);
    if ((count % 3) == 2)
      send_to_char(ch, "\r\n");
    count++;
  }
  if ((count % 3) == 0)
    send_to_char(ch, "\r\n");
}

ACMD(do_craft_score)
{
  switch (CONFIG_CRAFTING_SYSTEM)
  {
  case CRAFTING_SYSTEM_KITS:
    do_practice(ch, argument, cmd, subcmd);
    break;
  case CRAFTING_SYSTEM_MOTES:
    do_craft_score_new(ch, argument, cmd, subcmd);
    break;
  default:
    send_to_char(ch, "There is no crafting system implemented right now.\r\n");
    break;
  }
}

ACMD(do_craft_score_new)
{
  show_craft_score(ch, argument);
}

struct obj_data *find_obj_rnum_in_inventory(struct char_data *ch, obj_rnum obj_rnum)
{
  struct obj_data *obj;

  for (obj = ch->carrying; obj; obj = obj->next_content)
  {
    if (GET_OBJ_RNUM(obj) == obj_rnum)
      return obj;
  }
  return NULL;
}

void craft_resize_complete(struct char_data *ch)
{
  int size;
  struct obj_data *obj = find_obj_rnum_in_inventory(ch, GET_CRAFT(ch).craft_obj_rnum);

  if (!obj)
  {
    send_to_char(ch,
                 "There's an issue with your resize project. Please inform a staff member.\r\n");
    return;
  }

  size = GET_CRAFT(ch).new_size;

  send_to_char(ch, "You've resized %s to %s!\r\n", obj->short_description, sizes[size]);
  GET_OBJ_SIZE(obj) = size;
  reset_crafting_obj(ch);
  GET_CRAFT(ch).new_size = 0;
  reset_current_craft(ch, NULL, FALSE, FALSE);
}

/**
 * Retrieves the description of a supply order item for a given character.
 *
 * @param ch The character for which to retrieve the supply order item description.
 * @return A pointer to the description of the supply order item.
 */
char *get_supply_order_item_desc(struct char_data *ch)
{
  int recipe = get_current_craft_project_recipe(ch);
  int variant = GET_CRAFT(ch).craft_variant;

  if (recipe <= CRAFT_RECIPE_NONE || variant == -1)
  {
    return "unknown item";
  }

  // Validate array bounds to prevent crashes
  if (recipe < 0 || recipe >= NUM_CRAFTING_RECIPES || variant < 0 || variant >= NUM_CRAFT_VARIANTS)
  {
    return "invalid item";
  }

  // Don't use strdup to avoid memory management issues
  // Return pointer to static string instead
  return (char *)crafting_recipes[recipe].variant_descriptions[variant];
}

int determine_supply_order_exp(struct char_data *ch)
{
  int base = NSUPPLY_ORDER_BASE_EXP, exp = 0;
  int mat_level_adj[NUM_CRAFT_GROUPS] = {0, 0, 0, 0, 0, 0, 0, 0};
  int recipe = 0, variant = 0, i = 0, j = 0;
  int material = 0, num_mats = 0, level_adj = 0, group = 0, total_mats = 0;

  recipe = get_current_craft_project_recipe(ch);
  variant = GET_CRAFT(ch).craft_variant;

  if (recipe <= CRAFT_RECIPE_NONE || variant == -1)
  {
    return 0;
  }

  for (i = 0; i < NUM_CRAFT_VARIANTS; i++)
  {
    for (j = 0; j < 3; j++)
    {
      material = crafting_recipes[recipe].materials[j][i][0];
      num_mats = crafting_recipes[recipe].materials[j][i][1];
      if (material == CRAFT_MAT_NONE || num_mats == 0)
        continue;
      total_mats += num_mats;
      group = craft_group_by_material(material);
      level_adj = MAX(1, 1 + craft_material_level_adjustment(material));
      mat_level_adj[group] = level_adj * num_mats;
    }
  }

  for (i = 0; i < NUM_CRAFT_GROUPS; i++)
  {
    if (mat_level_adj[i] > 0)
      exp += mat_level_adj[i] * 3;
  }

  // Prevent division by zero crash
  if (total_mats > 0)
  {
    exp /= total_mats;
  }

  exp += base;

  // // Cap the experience at 300 to prevent excessive rewards
  // exp = MIN(300, exp);

  return MAX(NSUPPLY_ORDER_BASE_EXP, exp);
}

void craft_supplyorder_complete(struct char_data *ch)
{
  int exp = 0;
  int recipe, skill;

  // Consume materials for this item
  if (!consume_supply_order_materials(ch))
  {
    send_to_char(
        ch, "You don't have enough materials to complete this item. Supply order cancelled.\r\n");
    abandon_supply_order(ch);
    return;
  }

  GET_NSUPPLY_NUM_MADE(ch)++;

  {
    const char *desc = get_supply_order_item_desc(ch);
    send_to_char(ch, "You've completed part of your supply order for %s%s.",
                 desc,
                 (desc[strlen(desc) - 1] == 's' ? "" : "s"));
  }

  if ((GET_CRAFT(ch).supply_num_required - GET_NSUPPLY_NUM_MADE(ch)) > 0)
  {
    send_to_char(ch, " %d to go.\r\n", num_supply_order_requisitions_to_go(ch));
    /* Stop crafting - player must manually restart for next item */
    GET_CRAFT(ch).craft_duration = 0;
  }
  else
  {
    send_to_char(ch, "\r\n\tGSupply order complete!\tn Visit a supply order NPC to collect your reward.\r\n");
    /* All items complete, stop crafting */
    GET_CRAFT(ch).craft_duration = 0;
  }

  exp = determine_supply_order_exp(ch);

  recipe = get_current_craft_project_recipe(ch);
  skill = recipe_skill_to_actual_crafting_skill(
      crafting_recipes[recipe].variant_skill[GET_CRAFT(ch).craft_variant]);

  gain_craft_exp(ch, exp, skill, TRUE);
}

int recipe_skill_to_actual_crafting_skill(int recipe_skill)
{
  switch (recipe_skill)
  {
  case CRAFT_SKILL_WEAPONSMITH:
    return ABILITY_CRAFT_WEAPONSMITHING;
  case CRAFT_SKILL_ARMORSMITH:
    return ABILITY_CRAFT_ARMORSMITHING;
  case CRAFT_SKILL_JEWELER:
    return ABILITY_CRAFT_JEWELCRAFTING;
  case CRAFT_SKILL_TINKER:
    return ABILITY_CRAFT_METALWORKING;
  case CRAFT_SKILL_CARPENTER:
    return ABILITY_CRAFT_WOODWORKING;
  case CRAFT_SKILL_TAILOR:
    return ABILITY_CRAFT_TAILORING;
  case CRAFT_SKILL_BREWING:
    return ABILITY_CRAFT_ALCHEMY;
  }
  return ABILITY_CRAFT_METALWORKING; // default fallback
}

/* Check if there's a quartermaster mob in the room */
bool has_quartermaster_in_room(struct char_data *ch)
{
  struct char_data *mob;

  if (!ch || !IN_ROOM(ch))
    return FALSE;

  for (mob = world[IN_ROOM(ch)].people; mob; mob = mob->next_in_room)
  {
    if (IS_NPC(mob) && MOB_FLAGGED(mob, MOB_QUARTERMASTER))
      return TRUE;
  }

  return FALSE;
}

/* Get the required crafting station flag for a given crafting skill */
int get_required_crafting_station(int skill)
{
  switch (skill)
  {
  case ABILITY_CRAFT_ALCHEMY:
    return ITEM_CRAFTING_ALCHEMY_LAB;
  case ABILITY_CRAFT_ARMORSMITHING:
    return ITEM_CRAFTING_FORGE;
  case ABILITY_CRAFT_JEWELCRAFTING:
    return ITEM_CRAFTING_JEWELCRAFTING_STATION;
  case ABILITY_CRAFT_LEATHERWORKING:
    return ITEM_CRAFTING_TANNERY;
  case ABILITY_CRAFT_METALWORKING:
    return ITEM_CRAFTING_FORGE;
  case ABILITY_CRAFT_TAILORING:
    return ITEM_CRAFTING_LOOM;
  case ABILITY_CRAFT_WEAPONSMITHING:
    return ITEM_CRAFTING_FORGE;
  case ABILITY_CRAFT_WOODWORKING:
    return ITEM_CRAFTING_CARPENTRY_TABLE;
  default:
    return -1; // No station required
  }
}

/* Get the name of the required crafting station for a given skill */
const char *get_crafting_station_name(int skill)
{
  switch (skill)
  {
  case ABILITY_CRAFT_ALCHEMY:
    return "an alchemy lab";
  case ABILITY_CRAFT_ARMORSMITHING:
    return "a forge";
  case ABILITY_CRAFT_JEWELCRAFTING:
    return "a jewelcrafting station";
  case ABILITY_CRAFT_LEATHERWORKING:
    return "a tannery";
  case ABILITY_CRAFT_METALWORKING:
    return "a forge";
  case ABILITY_CRAFT_TAILORING:
    return "a loom";
  case ABILITY_CRAFT_WEAPONSMITHING:
    return "a forge";
  case ABILITY_CRAFT_WOODWORKING:
    return "a carpentry table";
  default:
    return "a crafting station";
  }
}

/* Check if there's a required crafting station in the room */
bool has_crafting_station_in_room(struct char_data *ch, int skill)
{
  struct obj_data *obj;
  int required_flag;

  if (!ch || !IN_ROOM(ch))
    return FALSE;

  required_flag = get_required_crafting_station(skill);

  // If no station required, return TRUE
  if (required_flag == -1)
    return TRUE;

  // Check objects in the room
  for (obj = world[IN_ROOM(ch)].contents; obj; obj = obj->next_content)
  {
    if (OBJ_FLAGGED(obj, required_flag))
      return TRUE;
  }

  return FALSE;
}

void complete_supply_order(struct char_data *ch)
{
  int gold_reward = 0;
  int bonus_exp = 0;
  int artisan_points = 0;
  int skill = 0;
  int recipe = 0;
  int i = 0;

  if (!has_quartermaster_in_room(ch))
  {
    send_to_char(ch,
                 "You need to be in a room with a quartermaster to complete a supply order.\r\n");
    return;
  }

  if (!player_has_supply_order(ch))
  {
    send_to_char(ch, "You don't have an active supply order.\r\n");
    return;
  }

  if (num_supply_order_requisitions_to_go(ch) > 0)
  {
    send_to_char(
        ch,
        "You haven't completed your supply order yet. You still need to make %d more items.\r\n",
        num_supply_order_requisitions_to_go(ch));
    return;
  }

  // Calculate rewards
  gold_reward = calculate_supply_order_reward(ch);
  bonus_exp = gold_reward / 2; // Bonus exp is half the gold reward

  // Cap the bonus experience to prevent excessive rewards
  bonus_exp = MIN(250, bonus_exp);

  // Award artisan points based on quantity completed
  artisan_points = GET_CRAFT(ch).supply_num_required * 10;

  // Apply happy hour bonus to artisan points
  int happy_hour_artisan_bonus = 0;
  if (HAPPY_CRAFTING_EXP > 0)
  {
    happy_hour_artisan_bonus = (artisan_points * HAPPY_CRAFTING_EXP) / 100;
    artisan_points += happy_hour_artisan_bonus;
  }

  // Award rewards
  GET_GOLD(ch) += gold_reward;
  recipe = get_current_craft_project_recipe(ch);
  skill = recipe_skill_to_actual_crafting_skill(crafting_recipes[recipe].variant_skill[GET_CRAFT(ch).craft_variant]);
  gain_craft_exp(ch, bonus_exp, skill, TRUE);
  GET_ARTISAN_EXP(ch) += artisan_points;

  send_to_char(ch, "Congratulations! You have completed your supply order.\r\n");
  if (happy_hour_artisan_bonus > 0)
  {
    send_to_char(
        ch, "You receive %d gold coins, %d bonus experience points, and \tC%d artisan points\tn (\tY+%d happy hour bonus\tn)!\r\n",
        gold_reward, bonus_exp, artisan_points - happy_hour_artisan_bonus, happy_hour_artisan_bonus);
  }
  else
  {
    send_to_char(
        ch, "You receive %d gold coins, %d bonus experience points, and \tC%d artisan points\tn!\r\n",
        gold_reward, bonus_exp, artisan_points);
  }
  send_to_char(ch, "You now have \tC%d total artisan points\tn.\r\n", GET_ARTISAN_EXP(ch));

  /* Start the cooldown window if it hasn't been started yet */
  time_t now = time(NULL);
  
  if (GET_CRAFT(ch).supply_cooldown_start_time == 0 || 
      (now - GET_CRAFT(ch).supply_cooldown_start_time >= 7200))
  {
    /* Start a new cooldown window or restart if expired */
    GET_CRAFT(ch).supply_cooldown_start_time = now;
    GET_CRAFT(ch).supply_orders_completed_count = 1;
    send_to_char(ch, "\tY[Cooldown Started]\tn You can now complete up to 3 supply orders before the cooldown limit is reached.\r\n");
  }
  else
  {
    /* Cooldown window is active, increment counter */
    GET_CRAFT(ch).supply_orders_completed_count++;
    int remaining = 3 - GET_CRAFT(ch).supply_orders_completed_count;
    
    if (remaining > 0)
    {
      send_to_char(ch, "\tY[Cooldown Active]\tn You can complete %d more supply order%s before hitting the limit.\r\n",
                   remaining, (remaining > 1 ? "s" : ""));
    }
    else if (remaining == 0)
    {
      time_t cooldown_remaining = 7200 - (now - GET_CRAFT(ch).supply_cooldown_start_time);
      int hours = cooldown_remaining / 3600;
      int minutes = (cooldown_remaining % 3600) / 60;
      
      send_to_char(ch, "\tR[Cooldown Limit Reached]\tn You have completed 3 supply orders. Your supply orders will refresh in approximately %d hour%s and %d minute%s.\r\n",
                   hours, (hours != 1 ? "s" : ""),
                   minutes, (minutes != 1 ? "s" : ""));
    }
  }

  /* Clear supply order data without penalty/cooldown since it was completed successfully */
  GET_CRAFT(ch).crafting_method = 0;
  GET_CRAFT(ch).crafting_item_type = 0;
  GET_CRAFT(ch).crafting_specific = 0;
  GET_CRAFT(ch).craft_variant = -1;
  GET_CRAFT(ch).supply_num_required = 0;
  GET_CRAFT(ch).supply_active_slot = -1;
  GET_CRAFT(ch).skill_type = 0;
  GET_CRAFT(ch).craft_duration = 0;
  GET_NSUPPLY_NUM_MADE(ch) = 0;
  
  /* Clear materials (already consumed during crafting) */
  for (i = 0; i < NUM_CRAFT_GROUPS; i++)
  {
    GET_CRAFT(ch).materials[i][0] = 0;
    GET_CRAFT(ch).materials[i][1] = 0;
  }
}

int calculate_supply_order_reward(struct char_data *ch)
{
  int base_reward = SUPPLY_BASE_REWARD;
  int material_bonus = 0;
  int quantity_bonus = 0;
  int skill_bonus = 0;
  int total_reward = 0;
  int i = 0;

  // Calculate material quality bonus
  for (i = 0; i < NUM_CRAFT_GROUPS; i++)
  {
    if (GET_CRAFT(ch).materials[i][0] > CRAFT_MAT_NONE)
    {
      int mat_grade = material_grade(GET_CRAFT(ch).materials[i][0]);
      material_bonus += mat_grade * SUPPLY_MATERIAL_BONUS_MULTIPLIER;
    }
  }

  // Calculate quantity bonus
  quantity_bonus = GET_CRAFT(ch).supply_num_required * SUPPLY_QUANTITY_BONUS;

  // Calculate skill bonus
  skill_bonus = get_craft_skill_value(ch, GET_CRAFT(ch).skill_type) * SUPPLY_SKILL_BONUS_MULTIPLIER;

  total_reward = base_reward + material_bonus + quantity_bonus + skill_bonus;

  // Apply quality tier bonuses
  int quality_bonus = calculate_quality_tier_bonus(ch);
  total_reward += (total_reward * quality_bonus / 100);

  // Apply bulk efficiency bonuses
  int bulk_bonus = calculate_bulk_efficiency_bonus(GET_CRAFT(ch).supply_num_required);
  total_reward += (total_reward * bulk_bonus / 100);

  return MAX(50, total_reward); // Minimum 50 gold reward
}

bool consume_supply_order_materials(struct char_data *ch)
{
  int i = 0;
  int recipe = 0;
  int variant = 0;
  int mat_type = 0;
  int num_mats = 0;

  if ((recipe = get_current_craft_project_recipe(ch)) <= CRAFT_RECIPE_NONE)
  {
    return FALSE;
  }

  if ((variant = GET_CRAFT(ch).craft_variant) == -1)
  {
    return FALSE;
  }

  // Consume materials for one item (called each time an item is completed)
  for (i = 0; i < 3; i++)
  {
    mat_type = crafting_recipes[recipe].materials[i][variant][0];
    num_mats = crafting_recipes[recipe].materials[i][variant][1];

    if (mat_type == CRAFT_GROUP_NONE || num_mats == 0)
      continue;

    if (GET_CRAFT(ch).materials[mat_type][1] < num_mats)
    {
      send_to_char(ch, "You don't have enough %s materials to complete this item!\r\n",
                   crafting_material_groups[mat_type]);
      return FALSE;
    }

    // Consume the materials
    GET_CRAFT(ch).materials[mat_type][1] -= num_mats;

    if (GET_CRAFT(ch).materials[mat_type][1] <= 0)
    {
      GET_CRAFT(ch).materials[mat_type][0] = CRAFT_MAT_NONE;
      GET_CRAFT(ch).materials[mat_type][1] = 0;
    }
  }

  return TRUE;
}

bool validate_supply_order_materials(struct char_data *ch)
{
  int i = 0;
  int recipe = 0;
  int variant = 0;
  int mat_type = 0;
  int num_mats = 0;

  if ((recipe = get_current_craft_project_recipe(ch)) <= CRAFT_RECIPE_NONE)
  {
    return FALSE;
  }

  if ((variant = GET_CRAFT(ch).craft_variant) == -1)
  {
    return FALSE;
  }

  /* Check if we have enough materials for the NEXT item (not all remaining items) */
  for (i = 0; i < 3; i++)
  {
    mat_type = crafting_recipes[recipe].materials[i][variant][0];
    num_mats = crafting_recipes[recipe].materials[i][variant][1];

    if (mat_type == CRAFT_GROUP_NONE || num_mats == 0)
      continue;

    if (GET_CRAFT(ch).materials[mat_type][1] < num_mats)
    {
      send_to_char(ch, "You need %d more %s materials to complete the next item.\r\n",
                   num_mats - GET_CRAFT(ch).materials[mat_type][1],
                   crafting_material_groups[mat_type]);
      return FALSE;
    }
  }

  return TRUE;
}

bool check_resize(struct char_data *ch, bool verbose)
{
  bool fail = FALSE;
  struct obj_data *obj = find_obj_rnum_in_inventory(ch, GET_CRAFT(ch).craft_obj_rnum);

  if (verbose)
  {
    send_to_char(ch, "\r\n");
    send_to_char(ch, "\tc");
  }
  if (!obj)
  {
    if (verbose)
    {
      send_to_char(
          ch,
          "You need to set an object to resize first. Use 'resize (object-name) (new-size)\r\n");
      send_to_char(ch, NEWCRAFT_RESIZE_SYNTAX);
    }
    fail = TRUE;
  }

  if (GET_CRAFT(ch).new_size == 0)
  {
    if (verbose)
    {
      send_to_char(ch,
                   "You need to set the new object size. Use 'resize (object-name) (new-size)\r\n");
    }
    fail = TRUE;
  }

  if (verbose)
  {
    send_to_char(ch, "\tn");
    send_to_char(ch, "\r\n");
  }
  return (!fail);
}

void newcraft_resize(struct char_data *ch, const char *argument)
{
  struct obj_data *obj;
  int i, size;
  char arg1[200], arg2[200], buf[200];

  two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

  if (!*arg1)
  {
    send_to_char(ch, NEWCRAFT_RESIZE_SYNTAX);
    return;
  }

  if (is_abbrev(arg1, "reset"))
  {
    if (!(obj = find_obj_rnum_in_inventory(ch, GET_CRAFT(ch).craft_obj_rnum)))
    {
      send_to_char(ch, "You don't have any object set to resize.\r\n");
      return;
    }

    send_to_char(ch, "You cancel your resizing of %s.\r\n", obj->short_description);
    GET_CRAFT(ch).new_size = 0;
    reset_crafting_obj(ch);
    return;
  }
  else if (is_abbrev(arg1, "show") || is_abbrev(arg1, "check"))
  {
    if (!(obj = find_obj_rnum_in_inventory(ch, GET_CRAFT(ch).craft_obj_rnum)))
    {
      check_resize(ch, TRUE);
    }
    else
    {
      send_to_char(ch, "\r\n");
      send_to_char(ch, "\tc");
      text_line(ch, "RESIZING", 80, '-', '-');
      send_to_char(ch, "\tn");
      send_to_char(ch, "-- Resize Object : %s\r\n", obj->short_description);
      send_to_char(ch, "-- Existing Size : %s\r\n", sizes[GET_OBJ_SIZE(obj)]);
      send_to_char(ch, "-- New Size      : %s\r\n",
                   GET_CRAFT(ch).new_size ? sizes[GET_CRAFT(ch).new_size] : "Not Set");
      send_to_char(ch, "\tc");
      draw_line(ch, 80, '-', '-');
      send_to_char(ch, "\tn");
    }
    return;
  }
  else if (is_abbrev(arg1, "start") || is_abbrev(arg1, "begin"))
  {
    if (!check_resize(ch, TRUE))
    {
      send_to_char(ch, "\tRYou are not ready to resize yet.\tn\r\n");
      return;
    }
    if (!(obj = find_obj_rnum_in_inventory(ch, GET_CRAFT(ch).craft_obj_rnum)))
    {
      send_to_char(ch, "You need to set you resize object first.\r\n");
      return;
    }
    GET_CRAFT(ch).crafting_method = SCMD_NEWCRAFT_RESIZE;
    GET_CRAFT(ch).craft_duration = 10;
    send_to_char(ch, "You begin resizing %s to %s.\r\n", obj->short_description,
                 sizes[GET_CRAFT(ch).new_size]);
    return;
  }
  else
  {
    if ((obj = find_obj_rnum_in_inventory(ch, GET_CRAFT(ch).craft_obj_rnum)))
    {
      send_to_char(ch, "You have already assigned an object to resize: %s\r\n",
                   obj->short_description);
      send_to_char(ch, NEWCRAFT_RESIZE_SYNTAX);
      return;
    }

    if (!(obj = get_obj_in_list_vis(ch, arg1, 0, ch->carrying)))
    {
      send_to_char(ch, "There's no object by that name in your inventory.\r\n");
      send_to_char(ch, NEWCRAFT_RESIZE_SYNTAX);
      return;
    }

    if (!*arg2)
    {
      send_to_char(ch, "Please specify the size you would like to change %s to.\r\n",
                   obj->short_description);
      return;
    }

    for (i = 0; i < NUM_SIZES; i++)
    {
      snprintf(buf, sizeof(buf), "%s", size_names[i]);
      buf[0] = tolower(buf[0]);
      if (is_abbrev(arg2, buf))
        break;
    }

    size = i;

    if (size >= NUM_SIZES)
    {
      send_to_char(ch, "That is not a valid size name. Please select one of:\r\n");
      for (i = 0; i < NUM_SIZES; i++)
      {
        snprintf(buf, sizeof(buf), "%s", size_names[i]);
        buf[0] = tolower(buf[0]);
        send_to_char(ch, "-- %s\r\n", buf);
      }
      send_to_char(ch, "\r\n");
      return;
    }

    if (GET_OBJ_SIZE(obj) == size)
    {
      act("$p is already that size.", TRUE, ch, obj, 0, TO_CHAR);
      return;
    }

    GET_CRAFT(ch).craft_obj_rnum = GET_OBJ_RNUM(obj);

    send_to_char(ch, "You've set %s to be resized from %s to %s.\r\n", obj->short_description,
                 size_names[GET_OBJ_SIZE(obj)], size_names[size]);
    send_to_char(ch, "Type: resize begin to execute.\r\n");
    GET_CRAFT(ch).new_size = size;
  }
}

void newcraft_supplyorder(struct char_data *ch, const char *argument)
{
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH], arg3[MAX_INPUT_LENGTH];

  three_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2), arg3, sizeof(arg3));

  if (!*arg1)
  {
    send_to_char(ch, SUPPLY_ORDER_NOARG1);
    return;
  }

  if (is_abbrev(arg1, "request"))
  {
    request_new_supply_order(ch);
    return;
  }

  if (is_abbrev(arg1, "show") || is_abbrev(arg1, "status"))
  {
    show_supply_order(ch);
    return;
  }

  if (is_abbrev(arg1, "start") || is_abbrev(arg1, "begin"))
  {
    start_supply_order(ch);
    return;
  }

  if (is_abbrev(arg1, "material") || is_abbrev(arg1, "materials"))
  {
    set_supply_order_materials(ch, arg2, arg3);
    return;
  }

  if (is_abbrev(arg1, "complete") || is_abbrev(arg1, "finish"))
  {
    complete_supply_order(ch);
    return;
  }

  if (is_abbrev(arg1, "reset"))
  {
    reset_supply_order(ch);
    return;
  }

  if (is_abbrev(arg1, "abandon"))
  {
    abandon_supply_order(ch);
    return;
  }

  if (is_abbrev(arg1, "cooldown") || is_abbrev(arg1, "cooldowns") || is_abbrev(arg1, "timers"))
  {
    show_supply_order_cooldowns(ch);
    return;
  }

  // Invalid command
  send_to_char(ch, SUPPLY_ORDER_NOARG1);
}

static int disenchant_fragment_yield(struct obj_data *obj)
{
  int level;

  if (!obj)
    return 0;

  level = MAX(1, GET_OBJ_LEVEL(obj));
  return MAX(1, level / 5);
}

static bool can_disenchant_obj(struct obj_data *obj)
{
  int i;
  bool has_magic = FALSE;

  if (!obj)
    return FALSE;

  if (OBJ_FLAGGED(obj, ITEM_QUEST) || OBJ_FLAGGED(obj, ITEM_NODROP) ||
      OBJ_FLAGGED(obj, ITEM_NORENT) || OBJ_FLAGGED(obj, ITEM_NOSAC) ||
      OBJ_FLAGGED(obj, ITEM_NO_SALVAGE) || OBJ_FLAGGED(obj, ITEM_ACCOUNT_EXP) ||
      OBJ_FLAGGED(obj, ITEM_ARTISANPOINTS))
    return FALSE;

  if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER && obj->contains)
    return FALSE;

  if (OBJ_FLAGGED(obj, ITEM_MAGIC))
    has_magic = TRUE;

  if ((GET_OBJ_TYPE(obj) == ITEM_WEAPON || GET_OBJ_TYPE(obj) == ITEM_ARMOR) &&
      GET_OBJ_VAL(obj, 4) > 0)
    has_magic = TRUE;

  for (i = 0; i < MAX_OBJ_AFFECT; i++)
  {
    if (is_valid_apply(obj->affected[i].location) && obj->affected[i].modifier != 0)
    {
      has_magic = TRUE;
      break;
    }
  }

  return has_magic;
}

void newcraft_disenchant(struct char_data *ch, const char *argument)
{
  char arg[MAX_INPUT_LENGTH];
  struct obj_data *obj = NULL;
  int fragments = 0;

  one_argument(argument, arg, sizeof(arg));

  if (CONFIG_CRAFTING_SYSTEM != CRAFTING_SYSTEM_MOTES)
  {
    send_to_char(ch, "Disenchanting is handled by crafting kits on this server.\r\n");
    return;
  }

  if (!*arg)
  {
    send_to_char(ch, "Disenchant what?\r\n");
    return;
  }

  if (!has_crafting_station_in_room(ch, ABILITY_CRAFT_ALCHEMY))
  {
    send_to_char(ch, "You need to be in a room with %s to disenchant items.\r\n",
                 get_crafting_station_name(ABILITY_CRAFT_ALCHEMY));
    return;
  }

  if (!(obj = get_obj_in_list_vis(ch, arg, NULL, ch->carrying)))
  {
    send_to_char(ch, "You don't have anything like that in your inventory.\r\n");
    return;
  }

  if (GET_OBJ_LEVEL(obj) < DISENCHANT_MIN_LEVEL)
  {
    send_to_char(ch, "That item is not powerful enough to yield catalyst fragments.\r\n");
    return;
  }

  if (!can_disenchant_obj(obj))
  {
    send_to_char(ch, "That item cannot be disenchanted.\r\n");
    return;
  }

  fragments = disenchant_fragment_yield(obj);
  GET_CRAFT_MAT(ch, CRAFT_MAT_CATALYST_FRAGMENT) += fragments;

  send_to_char(ch, "You disenchant %s and recover %d catalyst fragment%s.\r\n",
               GET_OBJ_SHORT(obj), fragments, fragments == 1 ? "" : "s");
  act("$n disenchants $p, drawing out faint catalyst fragments.", FALSE, ch, obj, 0, TO_ROOM);

  obj_from_char(obj);
  extract_obj(obj);

  save_char(ch, 0);
  Crash_crashsave(ch);
}


ACMD(do_newcraft)
{
  char arg[MAX_INPUT_LENGTH];

  if (!IS_HUMANOID(ch))
  {
    send_to_char(ch, "Only humanoids can craft.\r\n");
    return;
  }

  if (IS_NPC(ch))
  {
    send_to_char(ch, "NPCs cannot craft.\r\n");
    return;
  }

  if (GET_CRAFT(ch).crafting_method != subcmd && GET_CRAFT(ch).crafting_method != 0 &&
      GET_CRAFT(ch).craft_duration > 0)
  {
    send_to_char(ch,
                 "You are already working on another project of type: %s. Please finish or cancel "
                 "it before continuing.\r\n",
                 crafting_methods_short[GET_CRAFT(ch).crafting_method]);
    return;
  }

  // Handle equipment/gear subcommands for the main craft command
  if (subcmd == SCMD_NEWCRAFT_CREATE)
  {
    one_argument(argument, arg, sizeof(arg));

    if (!str_cmp(arg, "equipment") || !str_cmp(arg, "gear") || !str_cmp(arg, "tools"))
    {
      newcraft_show_tools(ch, argument);
      return;
    }

    newcraft_create(ch, argument);
    return;
  }
  else if (subcmd == SCMD_NEWCRAFT_SURVEY)
  {
    newcraft_survey(ch, argument);
    return;
  }
  else if (subcmd == SCMD_NEWCRAFT_HARVEST)
  {
    newcraft_harvest(ch, argument);
    return;
  }
  else if (subcmd == SCMD_NEWCRAFT_REFINE)
  {
    newcraft_refine(ch, argument);
    return;
  }
  else if (subcmd == SCMD_NEWCRAFT_RESIZE)
  {
    newcraft_resize(ch, argument);
    return;
  }
  else if (subcmd == SCMD_NEWCRAFT_SUPPLYORDER)
  {
    newcraft_supplyorder(ch, argument);
    return;
  }
  else if (subcmd == SCMD_NEWCRAFT_EQUIPMENT)
  {
    newcraft_equipment(ch, argument);
    return;
  }
  else if (subcmd == SCMD_NEWCRAFT_GOLEM)
  {
    newcraft_golem(ch, argument);
    return;
  }
  else if (subcmd == SCMD_NEWCRAFT_BUTCHER)
  {
    newcraft_butcher(ch, argument);
    return;
  }
  else if (subcmd == SCMD_NEWCRAFT_DISENCHANT)
  {
    newcraft_disenchant(ch, argument);
    return;
  }
}

bool does_craft_apply_type_have_specific_value(int location)
{
  switch (location)
  {
  case APPLY_SKILL:
  case APPLY_FEAT:
  case APPLY_SPELL_CIRCLE_1:
  case APPLY_SPELL_CIRCLE_2:
  case APPLY_SPELL_CIRCLE_3:
  case APPLY_SPELL_CIRCLE_4:
  case APPLY_SPELL_CIRCLE_5:
  case APPLY_SPELL_CIRCLE_6:
  case APPLY_SPELL_CIRCLE_7:
  case APPLY_SPELL_CIRCLE_8:
  case APPLY_SPELL_CIRCLE_9:
    return 1;
  }
  return 0;
}

/**
 * Retrieves the craft material associated with the given name.
 *
 * @param ch The character data structure.
 * @param arg2 The name of the craft material.
 * @return The craft material associated with the given name, or NULL if not found.
 */
int get_craft_material_by_name(struct char_data *ch, char *arg)
{
  int i = 0;
  char buf[200];

  if (!*arg)
  {
    send_to_char(ch, "You need to specify a material type.\r\n");
    return CRAFT_MAT_NONE;
  }

  for (i = 1; i < NUM_CRAFT_MATS; i++)
  {
    snprintf(buf, sizeof(buf), "%s", crafting_materials[i]);
    buf[0] = tolower(buf[0]);
    if (is_abbrev(arg, buf))
      return i;
  }
  return CRAFT_MAT_NONE;
}

/**
 * Retrieves the recipe for the current craft project of a character.
 *
 * @param ch A pointer to the character data structure.
 * @return The recipe for the current craft project, or NULL if no project is active.
 */
int get_current_craft_project_recipe(struct char_data *ch)
{
  int item_type = GET_CRAFT(ch).crafting_item_type;
  int crafting_specific = GET_CRAFT(ch).crafting_specific;
  int i = 0;

  if (GET_CRAFT(ch).crafting_recipe != CRAFT_RECIPE_NONE)
  {
    return GET_CRAFT(ch).crafting_recipe;
  }

  if (item_type == 0 || crafting_specific == 0)
  {
    return CRAFT_RECIPE_NONE;
  }

  for (i = 0; i < NUM_CRAFTING_RECIPES; i++)
  {
    if (craft_recipe_by_type(item_type) == crafting_recipes[i].object_type)
    {
      if (crafting_specific == crafting_recipes[i].object_subtype)
      {
        return i;
      }
    }
  }
  return CRAFT_RECIPE_NONE;
}

/**
 * Retrieves the number of materials required by a specific material type and craft recipe.
 *
 * @param ch The character data.
 * @param material The material type.
 * @param recipe The craft recipe.
 * @return The number of materials required.
 */
int get_num_mats_required_by_material_type_and_craft_recipe(struct char_data *ch, int material,
                                                            int recipe)
{
  int i = 0, j = 0;
  int num_mats = 0;
  int variant = GET_CRAFT(ch).craft_variant;
  int mat_type = CRAFT_GROUP_NONE;

  if (variant == -1)
  {
    send_to_char(ch, "You need to set the variant type first.\r\n");
    return 0;
  }

  if (material <= CRAFT_MAT_NONE || recipe <= CRAFT_RECIPE_NONE)
  {
    return 0;
  }

  if ((mat_type = craft_group_by_material(material)) <= CRAFT_GROUP_NONE)
  {
    return 0;
  }

  for (i = 0; i < NUM_CRAFTING_RECIPES; i++)
  {
    if (recipe == i)
    {
      for (j = 0; j < 3; j++)
      {
        if (crafting_recipes[i].materials[j][variant][0] == mat_type)
        {
          num_mats = crafting_recipes[i].materials[j][variant][1];
          return num_mats;
        }
      }
    }
  }
  return num_mats;
}

/* Find the lowest grade material of a given group that the player has */
int find_lowest_grade_material_in_group(struct char_data *ch, int material_group)
{
  int mat = 0;
  int lowest_grade = INT_MAX;
  int lowest_material = CRAFT_MAT_NONE;

  for (mat = 1; mat < NUM_CRAFT_MATS; mat++)
  {
    if (craft_group_by_material(mat) == material_group && GET_CRAFT_MAT(ch, mat) > 0)
    {
      int grade = material_grade(mat);
      if (grade < lowest_grade)
      {
        lowest_grade = grade;
        lowest_material = mat;
      }
    }
  }

  return lowest_material;
}

bool remove_supply_order_materials(struct char_data *ch)
{
  bool found = FALSE;
  int i;
  int material, num_mats;

  for (i = 1; i < NUM_CRAFT_GROUPS; i++)
  {
    if (GET_CRAFT(ch).materials[i][0] > CRAFT_MAT_NONE)
    {
      material = GET_CRAFT(ch).materials[i][0];
      num_mats = GET_CRAFT(ch).materials[i][1];
      GET_CRAFT_MAT(ch, material) += num_mats;
      send_to_char(ch, "You've removed %d unit%s of %s from your supply order.\r\n", num_mats,
                   num_mats > 1 ? "s" : "", crafting_materials[material]);
      GET_CRAFT(ch).materials[i][0] = 0;
      GET_CRAFT(ch).materials[i][1] = 0;
      found = TRUE;
    }
  }
  return found;
}


/**
 * Sets the supply order materials for a character.
 *
 * This function sets the supply order materials for the specified character.
 * It takes in a character data structure, `ch`, and two string arguments, `arg` and `arg2`,
 * which represent the materials for the supply order.
 *
 * @param ch    The character for which to set the supply order materials.
 * @param arg   Whether to 'add' or 'remove' materials
 * @param arg2  The material type for the supply order. Ie. 'steel'
 */
void set_supply_order_materials(struct char_data *ch, char *arg, char *arg2)
{
  int material = 0;
  int num_mats = 0;
  int recipe = 0;
  int mat_type = 0;
  bool found = FALSE;

  if (num_supply_order_requisitions_to_go(ch) == 0)
  {
    send_to_char(ch, "You have already completed your supply order. Go to a supply order "
                     "requisition NPC and type 'supplyorder complete' for your reward.\r\n");
    return;
  }

  if (!*arg)
  {
    send_to_char(ch, "You need to specify whether to add or remove materials.\r\n");
  }

  if (!is_abbrev(arg, "add") && !is_abbrev(arg, "remove"))
  {
    send_to_char(ch, "You need to specify whether to add or remove materials.\r\n");
    return;
  }

  if (!*arg2 && is_abbrev(arg, "add"))
  {
    send_to_char(ch, "You need to specify a material type or 'all'.\r\n");
    return;
  }

  // remove will remove all materials currently assigned.
  if (is_abbrev(arg, "remove"))
  {
    found = remove_supply_order_materials(ch);
    if (!found)
    {
      send_to_char(ch, "You don't have any materials assigned to your supply order.\r\n");
      return;
    }
    return;
  }

  /* Handle "add all" case */
  if (is_abbrev(arg2, "all"))
  {
    int i;
    int variant = 0;
    int required_group = 0;
    int lowest_material = 0;
    int required_amount = 0;
    bool can_fill = TRUE;
    int num_filled = 0;

    if ((recipe = get_current_craft_project_recipe(ch)) <= CRAFT_RECIPE_NONE)
    {
      send_to_char(ch,
                   "You need to set the supply order item type, specific type and variant type first.\r\n");
      return;
    }

    if ((variant = GET_CRAFT(ch).craft_variant) == -1)
    {
      send_to_char(ch, "You need to set the variant type first.\r\n");
      return;
    }

    /* Check if any materials already assigned */
    for (i = 1; i < NUM_CRAFT_GROUPS; i++)
    {
      if (GET_CRAFT(ch).materials[i][0] > CRAFT_MAT_NONE)
      {
        send_to_char(ch, "You already have materials assigned. Please remove them first.\r\n");
        return;
      }
    }

    /* First pass: verify player has all required materials */
    for (i = 0; i < 3; i++)
    {
      if (crafting_recipes[recipe].materials[i][variant][0] != CRAFT_GROUP_NONE)
      {
        required_group = crafting_recipes[recipe].materials[i][variant][0];
        required_amount = crafting_recipes[recipe].materials[i][variant][1];

        lowest_material = find_lowest_grade_material_in_group(ch, required_group);

        if (lowest_material == CRAFT_MAT_NONE || GET_CRAFT_MAT(ch, lowest_material) < required_amount)
        {
          send_to_char(ch, "You don't have enough materials to fill all slots. Missing %s.\r\n",
                       crafting_material_groups[required_group]);
          can_fill = FALSE;
          break;
        }
      }
    }

    if (!can_fill)
    {
      return;
    }

    /* Second pass: add all materials */
    for (i = 0; i < 3; i++)
    {
      if (crafting_recipes[recipe].materials[i][variant][0] != CRAFT_GROUP_NONE)
      {
        required_group = crafting_recipes[recipe].materials[i][variant][0];
        required_amount = crafting_recipes[recipe].materials[i][variant][1];

        lowest_material = find_lowest_grade_material_in_group(ch, required_group);

        GET_CRAFT(ch).materials[required_group][0] = lowest_material;
        GET_CRAFT(ch).materials[required_group][1] = required_amount;
        GET_CRAFT_MAT(ch, lowest_material) -= required_amount;

        send_to_char(ch, "Added %d unit%s of %s (%s).\r\n", required_amount,
                     required_amount > 1 ? "s" : "", crafting_materials[lowest_material],
                     crafting_material_groups[required_group]);
        num_filled++;
      }
    }

    send_to_char(ch, "\tgSuccessfully filled all %d material slots!\tn\r\n", num_filled);
    return;
  }

  if ((material = get_craft_material_by_name(ch, arg2)) <= CRAFT_MAT_NONE)
  {
    send_to_char(ch, "That is not a valid type of material. Type 'materials' for a list.\r\n");
    return;
  }

  if ((recipe = get_current_craft_project_recipe(ch)) <= CRAFT_RECIPE_NONE)
  {
    send_to_char(
        ch,
        "You need to set the supply order item type, specific type and variant type first.\r\n");
    return;
  }

  if ((num_mats = get_num_mats_required_by_material_type_and_craft_recipe(ch, material, recipe)) <=
      0)
  {
    send_to_char(ch, "That is not a valid type of material for this supply order.\r\n");
    return;
  }

  if ((mat_type = craft_group_by_material(material)) <= CRAFT_GROUP_NONE)
  {
    send_to_char(ch, "That is not a valid type of material.\r\n");
    return;
  }

  // add requires specifying which material to add, as certain materials provide higher bonuses
  if (is_abbrev(arg, "add"))
  {
    if (GET_CRAFT(ch).materials[mat_type][0] > CRAFT_GROUP_NONE)
    {
      send_to_char(ch, "You already have a material of that type assigned to your supply order. "
                       "Please remove it first.\r\n");
      return;
    }

    if (GET_CRAFT_MAT(ch, material) < num_mats)
    {
      send_to_char(ch, "You need %d unit%s of %s, but only have %d.\r\n", num_mats,
                   num_mats > 1 ? "s" : "", crafting_materials[material],
                   GET_CRAFT_MAT(ch, material));
      return;
    }

    GET_CRAFT(ch).materials[mat_type][0] = material;
    GET_CRAFT(ch).materials[mat_type][1] = num_mats;
    GET_CRAFT_MAT(ch, material) -= num_mats;
    send_to_char(ch, "You've added %d unit%s of %s to your supply order.\r\n", num_mats,
                 num_mats > 1 ? "s" : "", crafting_materials[material]);
    return;
  }
  else
  {
    send_to_char(ch, "You need to specify whether to add or remove materials.\r\n");
    return;
  }
}

int select_random_craft_recipe(void)
{
  int type = 0;
  int choice = 0;
  int attempts = 0;

  // -2 insteadf of -1 for now, as we're not including instruments yet
  type = craft_recipe_by_type(dice(CRAFT_TYPE_NONE + 1, NUM_CRAFT_TYPES - 2));

  choice = dice(1, NUM_CRAFTING_RECIPES - 1);

  while ((type != crafting_recipes[choice].object_type || 
          crafting_recipes[choice].object_type == ITEM_INSTRUMENT) &&
         attempts < 100)
  {
    choice = dice(1, NUM_CRAFTING_RECIPES - 1);
    attempts++;
  }

  /* Return -1 if we couldn't find a non-instrument recipe */
  if (crafting_recipes[choice].object_type == ITEM_INSTRUMENT)
    return CRAFT_RECIPE_NONE;

  return choice;
}

int select_random_craft_variant(int recipe)
{
  if (recipe <= CRAFT_RECIPE_NONE || recipe >= NUM_CRAFTING_RECIPES)
  {
    return -1;
  }

  // we're not ready for instruments yet
  if (crafting_recipes[recipe].object_type == ITEM_INSTRUMENT)
  {
    return -1;
  }

  int variant = 0;

  variant = dice(1, NUM_CRAFT_VARIANTS) - 1;

  while (crafting_recipes[recipe].variant_skill[variant] == 0)
  {
    variant = dice(1, NUM_CRAFT_VARIANTS) - 1;
  }

  return variant;
}

// Stable versions for supply order contracts - use seed for consistent results
int select_stable_craft_recipe(int seed)
{
  int type = 0;
  int choice = 0;

  // Use seed to select type consistently
  type = craft_recipe_by_type((seed % (NUM_CRAFT_TYPES - 2)) + 1);

  // Use seed to select recipe within that type
  choice = (seed % (NUM_CRAFTING_RECIPES - 1)) + 1;

  int attempts = 0;
  while (type != crafting_recipes[choice].object_type && attempts < 100)
  {
    choice = ((choice + seed + attempts) % (NUM_CRAFTING_RECIPES - 1)) + 1;
    attempts++;
  }

  return choice;
}

int select_stable_craft_variant(int recipe, int seed)
{
  if (recipe <= CRAFT_RECIPE_NONE || recipe >= NUM_CRAFTING_RECIPES)
  {
    return -1;
  }

  // we're not ready for instruments yet
  if (crafting_recipes[recipe].object_type == ITEM_INSTRUMENT)
  {
    return -1;
  }

  int variant = seed % NUM_CRAFT_VARIANTS;
  int attempts = 0;

  while (crafting_recipes[recipe].variant_skill[variant] == 0 && attempts < NUM_CRAFT_VARIANTS)
  {
    variant = (variant + 1) % NUM_CRAFT_VARIANTS;
    attempts++;
  }

  // If no valid variant found, return -1
  if (attempts >= NUM_CRAFT_VARIANTS)
    return -1;

  return variant;
}

int generate_contract_quantity(int contract_type)
{
  int base_quantity = 0;

  switch (contract_type)
  {
  case SUPPLY_CONTRACT_BASIC:
    base_quantity = dice(1, 3) + 2; // 3-5 items
    break;
  case SUPPLY_CONTRACT_RUSH:
    base_quantity = dice(1, 4) + 2; // 3-6 items
    break;
  case SUPPLY_CONTRACT_BULK:
    base_quantity = dice(1, 5) + 5; // 6-10 items
    break;
  case SUPPLY_CONTRACT_QUALITY:
    base_quantity = dice(1, 2) + 2; // 3-4 items (fewer but higher quality)
    break;
  default:
    base_quantity = NSUPPLY_ORDER_NUM_REQUIRED; // fallback to default
    break;
  }

  return URANGE(MIN_CONTRACT_QUANTITY, base_quantity, MAX_CONTRACT_QUANTITY);
}

int generate_contract_reward(int contract_type, int quantity, int difficulty)
{
  int base_reward = BASE_CONTRACT_REWARD;
  int type_multiplier = 100;

  switch (contract_type)
  {
  case SUPPLY_CONTRACT_BASIC:
    type_multiplier = 100;
    break;
  case SUPPLY_CONTRACT_RUSH:
    type_multiplier = SUPPLY_RUSH_BONUS_MULTIPLIER;
    break;
  case SUPPLY_CONTRACT_BULK:
    type_multiplier = SUPPLY_BULK_BONUS_MULTIPLIER;
    break;
  case SUPPLY_CONTRACT_QUALITY:
    type_multiplier = SUPPLY_QUALITY_BONUS_MULTIPLIER;
    break;
  }

  int final_reward = (base_reward * quantity * type_multiplier * difficulty) / 10000;

  // Cap the contract reward to prevent excessive experience rewards
  final_reward = MIN(300, final_reward);

  return final_reward;
}

bool player_has_supply_order(struct char_data *ch)
{
  if (GET_CRAFT(ch).crafting_method == SCMD_NEWCRAFT_SUPPLYORDER)
  {
    return TRUE;
  }
  return FALSE;
}

void show_available_contracts(struct char_data *ch)
{
  send_to_char(ch, "\r\n\tgSupply Order System:\tn\r\n");
  send_to_char(ch, "\tW=====================================\tn\r\n");
  send_to_char(ch, "Use '\tCsupplyorder request\tn' to get a new supply order.\r\n");
  send_to_char(ch, "Complete supply orders to earn \tCartisan points\tn!\r\n");
  send_to_char(ch, "\r\nYou currently have \tC%d artisan points\tn.\r\n", GET_ARTISAN_EXP(ch));
  send_to_char(ch, "(Artisan points will be usable for special rewards in the future)\r\n\r\n");
}

void request_new_supply_order(struct char_data *ch)
{
  int recipe = 0;
  int variant = 0;
  time_t now = time(NULL);
  
  if (!has_quartermaster_in_room(ch))
  {
    send_to_char(ch, "You must be in a room with a quartermaster to request a supply order.\r\n");
    return;
  }

  /* Check if cooldown window is active and whether it has expired */
  if (GET_CRAFT(ch).supply_cooldown_start_time > 0)
  {
    time_t time_elapsed = now - GET_CRAFT(ch).supply_cooldown_start_time;
    
    if (time_elapsed >= 7200) /* 2 hours have passed, reset cooldown */
    {
      GET_CRAFT(ch).supply_cooldown_start_time = 0;
      GET_CRAFT(ch).supply_orders_completed_count = 0;
    }
    else if (GET_CRAFT(ch).supply_orders_completed_count >= 3)
    {
      /* Cooldown is active and they've used all 3 slots */
      int cooldown_remaining = 7200 - time_elapsed;
      int hours = cooldown_remaining / 3600;
      int minutes = (cooldown_remaining % 3600) / 60;
      
      send_to_char(ch, "You have already completed 3 supply orders. The cooldown is active.\r\n");
      send_to_char(ch, "Your supply orders will refresh in approximately %d hours and %d minutes.\r\n", hours, minutes);
      return;
    }
  }

  if (GET_CRAFT(ch).crafting_method == SCMD_NEWCRAFT_SUPPLYORDER)
  {
    send_to_char(ch, "You are already working on a supply order. Type supplyorder show to see the details or supplyorder reset to start over.\r\n");
    return;
  }

  if (GET_CRAFT(ch).crafting_method != 0)
  {
    send_to_char(ch, "You are already working on a crafting project.\r\n");
    return;
  }

  if (GET_CRAFT(ch).crafting_item_type > CRAFT_TYPE_NONE || GET_CRAFT(ch).crafting_specific > 0 ||
      GET_CRAFT(ch).craft_variant != -1)
  {
    send_to_char(ch, "You already have a supply order going. Type supplyorder show to see the details or supplyorder reset to start over.\r\n");
    return;
  }

  if ((recipe = select_random_craft_recipe()) <= CRAFT_RECIPE_NONE)
  {
    send_to_char(ch, "There seems to be an issue with your supply order. Please type supplyorder "
                     "reset to start over. This is error 1.\r\n");
    return;
  }

  if ((variant = select_random_craft_variant(recipe)) < 0)
  {
    send_to_char(ch, "There seems to be an issue with your supply order. Please type supplyorder "
                     "reset to start over. This is error 2.\r\n");
    return;
  }

  if (GET_CRAFT(ch).crafting_item_type == CRAFT_TYPE_NONE)
  {
    int quantity = dice(1, 3) + 1; // 2-4 items per supply order

    GET_CRAFT(ch).crafting_recipe = recipe;
    GET_CRAFT(ch).crafting_item_type = crafting_recipes[recipe].object_type;
    GET_CRAFT(ch).crafting_specific = crafting_recipes[recipe].object_subtype;
    GET_CRAFT(ch).craft_variant = variant;
    GET_CRAFT(ch).crafting_method = SCMD_NEWCRAFT_SUPPLYORDER;
    GET_CRAFT(ch).supply_num_required = quantity;
    GET_CRAFT(ch).skill_type = crafting_recipes[recipe].variant_skill[variant];
    
    send_to_char(ch, "You've requested a new supply order to make %d %ss.\r\n", quantity,
                 crafting_recipes[recipe].variant_descriptions[variant]);
    send_to_char(ch, "Complete this order to earn \tCartisan points\tn!\r\n");
  }
  else
  {
    send_to_char(ch, "There seems to be an issue with your supply order. Please type supplyorder "
                     "reset to start over. This is error 3.\r\n");
    return;
  }
}

int num_supply_order_requisitions_to_go(struct char_data *ch)
{
  int num_required = 0, num_done = 0, num_to_go = 0;

  num_required = GET_CRAFT(ch).supply_num_required;
  num_done = GET_NSUPPLY_NUM_MADE(ch);
  num_to_go = num_required - num_done;

  return num_to_go;
}

void start_supply_order(struct char_data *ch)
{
  if (!player_has_supply_order(ch))
  {
    send_to_char(ch, "You need to request a supply order first.\r\n");
  }
  else if (num_supply_order_requisitions_to_go(ch) == 0)
  {
    send_to_char(ch, "You have already completed your supply order. Go to a supply order "
                     "requisition NPC and type 'supplyorder complete' for your reward.\r\n");
  }
  else if (!validate_supply_order_materials(ch))
  {
    send_to_char(ch, "You don't have enough materials to complete your supply order. Please add "
                     "more materials first.\r\n");
  }
  else
  {
    // Check if the player is in a room with the required crafting station
    int recipe_skill = GET_CRAFT(ch).skill_type;
    int actual_skill = recipe_skill_to_actual_crafting_skill(recipe_skill);
    if (!has_crafting_station_in_room(ch, actual_skill))
    {
      send_to_char(ch, "You need to be in a room with %s to work on this supply order.\r\n",
                   get_crafting_station_name(actual_skill));
      return;
    }

    GET_CRAFT(ch).craft_duration = NSUPPLY_ORDER_DURATION;
    send_to_char(ch, "You begin working on your supply order for %s %s.\r\n", 
      AN(crafting_recipes[GET_CRAFT(ch).crafting_recipe].variant_descriptions[GET_CRAFT(ch).craft_variant]),
      crafting_recipes[GET_CRAFT(ch).crafting_recipe].variant_descriptions[GET_CRAFT(ch).craft_variant]);
  }
}

void show_supply_order_materials(struct char_data *ch, int recipe, int variant)
{
  int i = 0;
  int material = 0, num_mats = 0, mat_type = 0;
  for (i = 0; i < 3; i++)
  {
    mat_type = crafting_recipes[recipe].materials[i][variant][0];
    num_mats = crafting_recipes[recipe].materials[i][variant][1];
    if (mat_type == CRAFT_GROUP_NONE || num_mats == 0)
      continue;
    send_to_char(ch, "%-15s (x%d) ", crafting_material_groups[mat_type], num_mats);
    if ((material = GET_CRAFT(ch).materials[mat_type][0]) != CRAFT_MAT_NONE)
    {
      send_to_char(ch, "(%s/x%d)\r\n", crafting_materials[material], num_mats);
    }
    else
    {
      send_to_char(ch, "(unassigned)\r\n");
    }
  }
}

void show_supply_order(struct char_data *ch)
{
  int recipe = 0;
  int variant = -1;

  if (GET_CRAFT(ch).crafting_method != SCMD_NEWCRAFT_SUPPLYORDER)
  {
    send_to_char(ch, "You don't have a supply order in progress.\r\n");
    return;
  }

  if ((recipe = get_current_craft_project_recipe(ch)) == CRAFT_RECIPE_NONE)
  {
    send_to_char(ch, "You do not have a supply order project type created. Please reset the supply "
                     "order and request a new one. Error #1.\r\n");
    return;
  }

  if ((variant = GET_CRAFT(ch).craft_variant) == -1)
  {
    send_to_char(ch, "You do not have a supply order type created. Please reset the supply order "
                     "and request a new one. Error #2.\r\n");
    return;
  }

  text_line(ch, "SUPPLY ORDER DETAILS", 90, '-', '-');

  // Check for contract expiration first
  if (is_contract_expired(ch))
  {
    send_to_char(ch, "\tr*** CONTRACT EXPIRED ***\tn\r\n");
    send_to_char(ch, "Your supply order contract has expired and is no longer valid.\r\n");
    send_to_char(ch, "Type 'supplyorder reset' to clear it and request a new contract.\r\n");
    return;
  }

  // Show expiration info if applicable
  if (GET_CRAFT(ch).supply_contract_expiration > 0)
  {
    time_t now = time(0);
    time_t expires = GET_CRAFT(ch).supply_contract_expiration;
    int hours_left = (expires - now) / 3600;

    if (hours_left > 0)
    {
      const char *urgency_color = (hours_left <= 6) ? "\tr" : (hours_left <= 24) ? "\ty" : "\tg";
      send_to_char(ch, "-- Time Remaining: %s%d hours\tn\r\n", urgency_color, hours_left);
    }
    else
    {
      send_to_char(ch, "-- Status: \trEXPIRED\tn\r\n");
    }
  }

  // show the type of item being created
  send_to_char(ch, "-- Item: %s\r\n", get_supply_order_item_desc(ch));

  // Show the required crafting station
  {
    int actual_skill = recipe_skill_to_actual_crafting_skill(GET_CRAFT(ch).skill_type);
    send_to_char(ch, "-- Crafting Station: %s\r\n", get_crafting_station_name(actual_skill));
  }

  // Show progress
  send_to_char(ch, "-- Progress: %d of %d items completed\r\n", GET_NSUPPLY_NUM_MADE(ch),
               GET_CRAFT(ch).supply_num_required);

  // the materials required
  send_to_char(ch, "-- Materials Required (per item):\r\n");
  show_supply_order_materials(ch, recipe, variant);

  // Show estimated reward
  if (num_supply_order_requisitions_to_go(ch) == 0)
  {
    send_to_char(ch, "-- Status: READY FOR COMPLETION!\r\n");
    send_to_char(ch, "-- Estimated Reward: %d gold coins\r\n", calculate_supply_order_reward(ch));
    send_to_char(
        ch,
        "-- Go to a supply order NPC and type 'supplyorder complete' to collect your reward.\r\n");
  }
  else
  {
    send_to_char(ch, "-- Status: In Progress (%d items remaining)\r\n",
                 num_supply_order_requisitions_to_go(ch));
    send_to_char(ch, "-- Estimated Reward: %d gold coins\r\n", calculate_supply_order_reward(ch));

    if (validate_supply_order_materials(ch))
    {
      send_to_char(ch, "-- Materials: SUFFICIENT - Ready to start crafting\r\n");
    }
    else
    {
      send_to_char(ch, "-- Materials: INSUFFICIENT - Add more materials before starting\r\n");
    }
  }

  text_line(ch, "", 90, '-', '-');
}

void reset_supply_order(struct char_data *ch)
{
  if (GET_CRAFT(ch).crafting_method != SCMD_NEWCRAFT_SUPPLYORDER)
  {
    send_to_char(ch, "You don't have an active supply order to reset.\r\n");
    return;
  }

  /* Refund any materials that were added for the current item */
  if (remove_supply_order_materials(ch))
  {
    send_to_char(ch, "Materials have been refunded. You can add materials and continue with your supply order.\r\n");
  }
  else
  {
    send_to_char(ch, "No materials were assigned to refund.\r\n");
  }

  /* Stop current crafting work */
  GET_CRAFT(ch).craft_duration = 0;
}

void abandon_supply_order(struct char_data *ch)
{
  int i = 0;

  reset_acraft(ch);

  /* Clear all supply order data */
  GET_CRAFT(ch).crafting_method = 0;
  GET_CRAFT(ch).crafting_item_type = 0;
  GET_CRAFT(ch).crafting_specific = 0;
  GET_CRAFT(ch).craft_variant = -1;
  GET_CRAFT(ch).supply_num_required = 0;
  GET_CRAFT(ch).supply_active_slot = -1;
  GET_CRAFT(ch).skill_type = 0;
  GET_CRAFT(ch).craft_duration = 0;
  
  /* Materials already refunded above */
  for (i = 0; i < NUM_CRAFT_GROUPS; i++)
  {
    GET_CRAFT(ch).materials[i][0] = 0;
    GET_CRAFT(ch).materials[i][1] = 0;
  }

  if (GET_CRAFT(ch).crafting_method != SCMD_NEWCRAFT_SUPPLYORDER)
  {
    send_to_char(ch, "You don't have an active supply order to abandon.\r\n");
    return;
  }

  /* Refund any materials that were added */
  remove_supply_order_materials(ch);
  
  send_to_char(ch, "You have abandoned your supply order and lost all progress.\r\n");
}

/**
 * @brief Clear or set supply order cooldowns for a character
 * @param ch The character whose cooldowns to clear/set
 * @param cooldown_seconds Number of seconds for cooldown (0 to clear immediately)
 */
void clear_supply_order_cooldowns(struct char_data *ch, int cooldown_seconds)
{
  int slot;
  time_t cooldown_time = 0;

  if (!ch)
    return;

  /* Calculate cooldown expiration time */
  if (cooldown_seconds > 0)
    cooldown_time = time(NULL) + cooldown_seconds;

  /* Reset all individual slot cooldowns */
  for (slot = 0; slot < 5; slot++)
  {
    GET_CRAFT(ch).supply_slot_cooldowns[slot] = cooldown_time;
  }

  /* Reset slot refresh timer */
  GET_CRAFT(ch).supply_slots_next_refresh = cooldown_time;

  /* Reset supply order completion cooldown */
  GET_CRAFT(ch).supply_cooldown_start_time = 0;
  GET_CRAFT(ch).supply_orders_completed_count = 0;
}


/**
 * @brief Handles the special behavior for new supply orders.
 */
SPECIAL(new_supply_orders)
{
  if (!CMD_IS("supplyorder"))
  {
    return 0;
  }

  char arg1[200], arg2[200], arg3[200];

  three_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2), arg3, sizeof(arg3));

  /**
     * Handle different commands related to crafting supply orders.
     *
     * @param arg1 The command argument.
     * @param ch The character executing the command.
     * @return 1 if the command was handled successfully, 0 otherwise.
     */
  if (!*arg1)
  {
    send_to_char(ch, "%s", SUPPLY_ORDER_NOARG1);
    return 1;
  }

  if (is_abbrev(arg1, "request"))
  {
    request_new_supply_order(ch);
  }
  else if (is_abbrev(arg1, "info") || is_abbrev(arg1, "show"))
  {
    show_supply_order(ch);
  }
  else if (is_abbrev(arg1, "start"))
  {
    start_supply_order(ch);
  }
  else if (is_abbrev(arg1, "material"))
  {
    set_supply_order_materials(ch, arg2, arg3);
  }
  else if (is_abbrev(arg1, "complete"))
  {
    complete_supply_order(ch);
  }
  else if (is_abbrev(arg1, "reset"))
  {
    reset_supply_order(ch);
  }
  else if (is_abbrev(arg1, "abandon"))
  {
    abandon_supply_order(ch);
  }
  else
  {
    send_to_char(ch, "%s", SUPPLY_ORDER_NOARG1);
    return 1;
  }

  return 1;
}

void show_mote_bonuses(struct char_data *ch, int mote)
{
  int i, j, length = 0;
  bool found = FALSE;

  send_to_char(ch, "\r\n");

  // weapon enhancements
  send_to_char(ch, "\tcWeapon Enhancement Bonuses for:\tn\r\n");
  for (i = 1; i < NUM_WEAPON_TYPES; i++)
  {
    if (get_enhancement_mote_type(ch, CRAFT_TYPE_WEAPON, i) == mote)
    {
      send_to_char(ch, "%s", weapon_list[i].name);
      send_to_char(ch, ", ");
      length += strlen(weapon_list[i].name);
      if (length > 80)
      {
        send_to_char(ch, "\r\n");
        length = 0;
      }
      found = TRUE;
    }
  }
  if (!found)
    send_to_char(ch, "None");
  send_to_char(ch, "\r\n");
  send_to_char(ch, "\r\n");

  // armor enhancements
  found = FALSE;
  length = 0;
  send_to_char(ch, "\tcArmor Enhancement Bonuses for:\tn\r\n");
  for (i = 1; i < NUM_SPEC_ARMOR_TYPES; i++)
  {
    if (get_enhancement_mote_type(ch, CRAFT_TYPE_ARMOR, i) == mote)
    {
      send_to_char(ch, "%s", armor_list[i].name);
      send_to_char(ch, ", ");
      length += strlen(armor_list[i].name);
      if (length > 80)
      {
        send_to_char(ch, "\r\n");
        length = 0;
      }

      found = TRUE;
    }
  }
  if (!found)
    send_to_char(ch, "None");
  send_to_char(ch, "\r\n");
  send_to_char(ch, "\r\n");


  send_to_char(ch, "\tcOther Bonuses:\tn\r\n");
  found = FALSE;
  length = 0;
  for (i = 0; i < NUM_APPLIES; i++)
  {
    switch (i)
    {
    case APPLY_SKILL:
      for (j = 1; j <= END_GENERAL_ABILITIES; j++)
      {
        if (crafting_mote_by_bonus_location(i, j, 0) == mote)
        {
          send_to_char(ch, "%s (%s), ", apply_types[i], ability_names[j]);
          length += strlen(apply_types[i]) + strlen(ability_names[j]) +
                    2; // +2 for the parentheses and comma
          if (length > 80)
          {
            send_to_char(ch, "\r\n");
            length = 0;
          }
          found = TRUE;
        }
      }
      break;
    case APPLY_AC_NEW:
      if (crafting_mote_by_bonus_location(i, 0, BONUS_TYPE_UNIVERSAL) == mote)
      {
        send_to_char(ch, "%s (Universal/Enhancement), ", apply_types[i]);
        length += strlen(apply_types[i]) + 26;
        if (length > 80)
        {
          send_to_char(ch, "\r\n");
          length = 0;
        }
        found = TRUE;
      }
      if (crafting_mote_by_bonus_location(i, 0, BONUS_TYPE_DEFLECTION) == mote)
      {
        send_to_char(ch, "%s (Deflection), ", apply_types[i]);
        length += strlen(apply_types[i]) + 14; // +14 for " (Deflection), "
        if (length > 80)
        {
          send_to_char(ch, "\r\n");
          length = 0;
        }
        found = TRUE;
      }
      if (crafting_mote_by_bonus_location(i, 0, BONUS_TYPE_NATURALARMOR) == mote)
      {
        send_to_char(ch, "%s (Natural), ", apply_types[i]);
        length += strlen(apply_types[i]) + 12; // +12 for " (Natural), "
        if (length > 80)
        {
          send_to_char(ch, "\r\n");
          length = 0;
        }
        found = TRUE;
      }
      if (crafting_mote_by_bonus_location(i, 0, BONUS_TYPE_DODGE) == mote)
      {
        send_to_char(ch, "%s (Dodge), ", apply_types[i]);
        send_to_char(ch, "%s, ", ability_names[j]);
        length += strlen(apply_types[i]) + 8; // +8 for " (Dodge), "
        if (length > 80)
        {
          send_to_char(ch, "\r\n");
          length = 0;
        }
        found = TRUE;
      }
      break;
    default:
      if (crafting_mote_by_bonus_location(i, 0, 0) == mote)
      {
        send_to_char(ch, "%s, ", apply_types[i]);
        length += strlen(apply_types[i]) + 2; // +2 for the comma
        if (length > 80)
        {
          send_to_char(ch, "\r\n");
          length = 0;
        }
        found = TRUE;
      }
      break;
    }
  }

  for (i = 1; i < NUM_SCHOOLS; i++)
  {
    if (get_mote_type_for_school(i) == mote)
    {
      send_to_char(ch, "%s spells.\r\n", spell_schools[i]);
    }
  }

  send_to_char(ch, "\r\n");

  // crafting_mote_by_bonus_location
}

static void impl_do_motes_(struct char_data *ch, char *argument, int cmd, int subcmd);
void do_motes(struct char_data *ch, const char *argument, int cmd, int subcmd)
{
  if (!argument)
  {
    impl_do_motes_(ch, NULL, cmd, subcmd);
  }
  else
  {
    char arg_buf[MAX_INPUT_LENGTH];
    strlcpy(arg_buf, argument, sizeof(arg_buf));
    impl_do_motes_(ch, arg_buf, cmd, subcmd);
  }
}
static void impl_do_motes_(struct char_data *ch, char *argument,
                           int cmd __attribute__((unused)),
                           int subcmd __attribute__((unused)))
{
  int i;
  char mote[50];

  skip_spaces(&argument);

  if (!*argument)
  {
    send_to_char(ch,
                 "Please specify one of the following mote types to see associated bonuses:\r\n");
    for (i = 1; i < NUM_CRAFT_MOTES; i++)
    {
      if (i > 1)
        send_to_char(ch, ", ");
      send_to_char(ch, "%s", crafting_motes[i]);
    }
    send_to_char(ch, ".\r\n");
    return;
  }

  for (i = 1; i < NUM_CRAFT_MOTES; i++)
  {
    if (is_abbrev(argument, crafting_motes[i]))
    {
      snprintf(mote, sizeof(mote), "%s", crafting_motes[i]);
      send_to_char(ch, "\tC%ss provide the following bonuses:\tn\r\n", CAP(mote));
      show_mote_bonuses(ch, i);
      return;
    }
  }

  send_to_char(ch, "That is not a valid mote type. Please specify one of the following:\r\n");
  for (i = 1; i < NUM_CRAFT_MOTES; i++)
  {
    if (i > 1)
      send_to_char(ch, ", ");
    send_to_char(ch, "%s", crafting_motes[i]);
  }
  send_to_char(ch, ".\r\n");
}

int get_craft_wear_loc(struct char_data *ch)
{
  if (!ch)
    return ITEM_WEAR_TAKE;

  int cr_type = GET_CRAFT(ch).crafting_item_type;
  int cr_specific = GET_CRAFT(ch).crafting_specific;
  int cr_recipe = GET_CRAFT(ch).crafting_recipe;

  if (cr_type <= CRAFT_TYPE_NONE || cr_specific <= 0 || cr_recipe <= CRAFT_RECIPE_NONE)
  {
    return ITEM_WEAR_TAKE; // default to take if no crafting project is set
  }

  if (cr_type == CRAFT_TYPE_WEAPON)
  {
    return ITEM_WEAR_WIELD;
  }
  else if (cr_type == CRAFT_TYPE_ARMOR)
  {
    return get_wear_location_by_armor_type(crafting_recipes[cr_recipe].object_subtype);
  }
  else if (cr_type == ITEM_INSTRUMENT)
  {
    return ITEM_WEAR_INSTRUMENT;
  }
  else
  {
    return crafting_recipes[cr_recipe].object_subtype;
  }
  return ITEM_WEAR_TAKE;
}


// Contract generation and management functions

// Cleanup supply slots when player logs out or quits
void cleanup_supply_slots(struct char_data *ch)
{
  int i;

  if (!ch)
  {
    return;
  }

  // Free any allocated strings in slots
  for (i = 0; i < 5; i++)
  {
    struct supply_contract *contract = &GET_CRAFT(ch).supply_slots[i];

    if (contract->description)
    {
      free(contract->description);
      contract->description = NULL;
    }
    if (contract->requirements)
    {
      free(contract->requirements);
      contract->requirements = NULL;
    }

    GET_CRAFT(ch).supply_slot_active[i] = FALSE;
    memset(contract, 0, sizeof(struct supply_contract));
  }
}

// Check if it's time to refresh supply order slots (1 hour = 3600 seconds)
bool should_refresh_supply_slots(struct char_data *ch)
{
  time_t now = time(NULL);

  // If never refreshed (or initialized but with 0), force refresh
  if (GET_CRAFT(ch).supply_slots_last_refresh == 0)
  {
    return TRUE;
  }

  // If 1 hour has passed since last refresh
  if ((now - GET_CRAFT(ch).supply_slots_last_refresh) >= 3600)
  {
    return TRUE;
  }

  return FALSE;
}

// Initialize supply slots if not already done
void initialize_supply_slots(struct char_data *ch)
{
  int i;
  bool needs_init = FALSE;

  // Check if we need to initialize - if any data looks uninitialized
  if (GET_CRAFT(ch).supply_slots_last_refresh == 0)
  {
    needs_init = TRUE;
  }

  if (!needs_init)
  {
    return;
  }

  // Clear all slots
  for (i = 0; i < 5; i++)
  {
    GET_CRAFT(ch).supply_slot_active[i] = FALSE;
    memset(&GET_CRAFT(ch).supply_slots[i], 0, sizeof(struct supply_contract));
  }

  // Set initial refresh time to NOW to trigger immediate refresh on next call
  // but prevent infinite loop of checks
  time_t now = time(NULL);
  GET_CRAFT(ch).supply_slots_last_refresh = now;
  GET_CRAFT(ch).supply_slots_next_refresh = now; // Force refresh on next call
}

// Refresh available supply order slots
void refresh_supply_slots(struct char_data *ch)
{
  int available_types[NUM_SUPPLY_CONTRACT_TYPES];
  int num_available = 0;
  int i;
  time_t now = time(NULL);

  // Always include all contract types
  available_types[num_available++] = SUPPLY_CONTRACT_BASIC;
  available_types[num_available++] = SUPPLY_CONTRACT_RUSH;
  available_types[num_available++] = SUPPLY_CONTRACT_BULK;
  available_types[num_available++] = SUPPLY_CONTRACT_QUALITY;
  available_types[num_available++] = SUPPLY_CONTRACT_PRESTIGE;

  /* Add event contracts if available */
  if (is_special_event_active())
  {
    available_types[num_available++] = SUPPLY_CONTRACT_EVENT;
  }

  // Generate contracts for empty slots only
  int used_recipes[NUM_CRAFTING_RECIPES];
  int num_used = 0;

  // Track recipes already used in existing slots
  for (i = 0; i < 5; i++)
  {
    if (GET_CRAFT(ch).supply_slot_active[i] && GET_CRAFT(ch).supply_slots[i].recipe > 0)
    {
      used_recipes[num_used++] = GET_CRAFT(ch).supply_slots[i].recipe;
    }
  }

  // Create stable seed based on player ID and refresh time
  int player_seed = GET_IDNUM(ch);
  int base_seed = (player_seed * 997 + (int)(now / 3600)) % 10000;

  for (i = 0; i < 5; i++)
  {
    // Skip slots that already have active contracts, or are on cooldown, or are currently being worked on
    if (GET_CRAFT(ch).supply_slot_active[i] || 
        (GET_CRAFT(ch).supply_slot_cooldowns[i] > 0 && now < GET_CRAFT(ch).supply_slot_cooldowns[i]) ||
        (GET_CRAFT(ch).supply_active_slot == i && GET_CRAFT(ch).crafting_method == SCMD_NEWCRAFT_SUPPLYORDER))
    {
      continue;
    }

    struct supply_contract *contract = &GET_CRAFT(ch).supply_slots[i];

    // Free any existing strings before clearing the slot
    if (contract->description)
    {
      free(contract->description);
      contract->description = NULL;
    }
    if (contract->requirements)
    {
      free(contract->requirements);
      contract->requirements = NULL;
    }

    // Clear the slot
    memset(contract, 0, sizeof(struct supply_contract));

    // Basic contract setup
    contract->contract_id = i + 1;
    contract->contract_type = available_types[i % num_available];

    // Select a unique recipe
    int attempts = 0;
    bool unique_found = FALSE;
    int contract_seed = base_seed + (i * 137);

    while (!unique_found && attempts < 100)
    {
      contract->recipe = select_stable_craft_recipe(contract_seed + attempts);

      // Check if this recipe is already used
      bool already_used = FALSE;
      int j;
      for (j = 0; j < num_used; j++)
      {
        if (used_recipes[j] == contract->recipe)
        {
          already_used = TRUE;
          break;
        }
      }

      if (!already_used && contract->recipe > 0)
      {
        used_recipes[num_used++] = contract->recipe;
        unique_found = TRUE;
      }
      attempts++;
    }

    // If we couldn't find a unique recipe, try systematic approach
    if (!unique_found)
    {
      int recipe_id;
      for (recipe_id = 1; recipe_id <= NUM_CRAFTING_RECIPES && !unique_found; recipe_id++)
      {
        bool already_used = FALSE;
        int j;
        for (j = 0; j < num_used; j++)
        {
          if (used_recipes[j] == recipe_id)
          {
            already_used = TRUE;
            break;
          }
        }
        if (!already_used)
        {
          contract->recipe = recipe_id;
          used_recipes[num_used++] = recipe_id;
          unique_found = TRUE;
        }
      }
    }

    // Only activate slot if we found a valid recipe
    if (unique_found)
    {
      contract->variant = (contract->recipe > 0)
                              ? select_stable_craft_variant(contract->recipe, contract_seed + 73)
                              : 0;

      // Set quality requirements
      contract->quality_tier_requirement = (contract->contract_type == SUPPLY_CONTRACT_QUALITY ||
                                            contract->contract_type == SUPPLY_CONTRACT_PRESTIGE)
                                               ? QUALITY_TIER_SUPERIOR
                                               : QUALITY_TIER_STANDARD;

      // Generate quantity based on contract type
      contract->quantity = generate_contract_quantity(contract->contract_type);

      // Add bulk efficiency bonus for large quantities
      if (contract->quantity >= BULK_EFFICIENCY_THRESHOLD_1)
      {
        int efficiency_bonus = calculate_bulk_efficiency_bonus(contract->quantity);
        contract->quantity = contract->quantity + (contract->quantity * efficiency_bonus / 100);
      }

      // Calculate difficulty modifier based on player level and recipe
      int player_level = GET_LEVEL(ch);
      int recipe_level = MAX(1, contract->recipe);
      contract->difficulty_modifier = MAX(0, recipe_level - player_level);

      // Generate reward with advanced bonuses
      contract->reward = generate_contract_reward(contract->contract_type, contract->quantity,
                                                  contract->difficulty_modifier + recipe_level);

      // Remove time limits as requested
      contract->time_limit = 0;

      // Generate specialized contracts
      if (contract->contract_type == SUPPLY_CONTRACT_PRESTIGE)
      {
        generate_prestige_contract(contract, ch);
      }
      else if (contract->contract_type == SUPPLY_CONTRACT_EVENT)
      {
        generate_event_contract(contract, ch);
      }
      else
      {
        // Generate standard descriptions
        char desc_buf[384], req_buf[256];
        const char *item_name = "unknown item";
        if (contract->recipe > 0 && contract->variant >= 0)
        {
          item_name = crafting_recipes[contract->recipe].variant_descriptions[contract->variant];
        }

        // Helper to add plural without double 's'
        char plural_item[128]; /* Item names are typically short (~20 chars max) */
        int len = strlen(item_name);
        if (contract->quantity > 1 && len > 0 && item_name[len - 1] != 's')
        {
          snprintf(plural_item, sizeof(plural_item), "%ss", item_name);
        }
        else
        {
          strcpy(plural_item, item_name);
        }

        switch (contract->contract_type)
        {
        case SUPPLY_CONTRACT_BASIC:
          snprintf(desc_buf, sizeof(desc_buf), "Craft %d %s", contract->quantity,
                   (contract->quantity > 1) ? plural_item : item_name);
          snprintf(req_buf, sizeof(req_buf), "Standard materials for %s crafting", item_name);
          break;
        case SUPPLY_CONTRACT_RUSH:
          snprintf(desc_buf, sizeof(desc_buf), "Craft %d %s", contract->quantity,
                   (contract->quantity > 1) ? plural_item : item_name);
          snprintf(req_buf, sizeof(req_buf), "Standard materials + 50%% bonus experience");
          break;
        case SUPPLY_CONTRACT_BULK:
          snprintf(desc_buf, sizeof(desc_buf), "Bulk order: Craft %d %s with efficiency bonus",
                   contract->quantity, (contract->quantity > 1) ? plural_item : item_name);
          snprintf(req_buf, sizeof(req_buf), "Standard materials, %d%% bulk efficiency bonus",
                   calculate_bulk_efficiency_bonus(contract->quantity));
          break;
        case SUPPLY_CONTRACT_QUALITY:
          snprintf(desc_buf, sizeof(desc_buf), "Premium quality %d %s required", contract->quantity,
                   (contract->quantity > 1) ? plural_item : item_name);
          snprintf(req_buf, sizeof(req_buf), "Superior-grade materials + quality tier bonus");
          break;
        default:
          snprintf(desc_buf, sizeof(desc_buf), "Standard contract for %d %s", contract->quantity,
                   (contract->quantity > 1) ? plural_item : item_name);
          snprintf(req_buf, sizeof(req_buf), "Standard materials");
          break;
        }

        contract->description = strdup(desc_buf);
        contract->requirements = strdup(req_buf);
      }

      // No expiration time since we're removing timers
      contract->expiration_time = 0;

      // Mark slot as active
      GET_CRAFT(ch).supply_slot_active[i] = TRUE;
    }
  }

  // Update refresh timestamps
  GET_CRAFT(ch).supply_slots_last_refresh = now;
  GET_CRAFT(ch).supply_slots_next_refresh = now + 3600; // Next refresh in 1 hour
}

struct supply_contract *generate_available_contracts(struct char_data *ch, int *num_contracts)
{
  // Initialize slots if needed
  initialize_supply_slots(ch);

  // Check if we should refresh slots
  if (should_refresh_supply_slots(ch))
  {
    refresh_supply_slots(ch);
  }

  // Count active slots and create return array
  int active_count = 0;
  int i;
  for (i = 0; i < 5; i++)
  {
    if (GET_CRAFT(ch).supply_slot_active[i])
    {
      active_count++;
    }
  }

  *num_contracts = active_count;

  if (active_count == 0)
  {
    return NULL;
  }

  // Allocate return array
  struct supply_contract *contracts = malloc(sizeof(struct supply_contract) * active_count);
  if (!contracts)
  {
    *num_contracts = 0;
    return NULL;
  }

  // Copy active contracts to return array
  int contract_index = 0;
  for (i = 0; i < 5; i++)
  {
    if (GET_CRAFT(ch).supply_slot_active[i])
    {
      memcpy(&contracts[contract_index], &GET_CRAFT(ch).supply_slots[i],
             sizeof(struct supply_contract));
      // Update contract_id to be sequential
      contracts[contract_index].contract_id = contract_index + 1;

      // Duplicate the strings to avoid double-free issues
      if (GET_CRAFT(ch).supply_slots[i].description)
      {
        contracts[contract_index].description = strdup(GET_CRAFT(ch).supply_slots[i].description);
      }
      else
      {
        contracts[contract_index].description = NULL;
      }

      if (GET_CRAFT(ch).supply_slots[i].requirements)
      {
        contracts[contract_index].requirements = strdup(GET_CRAFT(ch).supply_slots[i].requirements);
      }
      else
      {
        contracts[contract_index].requirements = NULL;
      }

      contract_index++;
    }
  }

  return contracts;
}

void free_contract_list(struct supply_contract *contracts, int num_contracts)
{
  int i;

  if (!contracts)
    return;

  for (i = 0; i < num_contracts; i++)
  {
    if (contracts[i].description)
    {
      free(contracts[i].description);
    }
    if (contracts[i].requirements)
    {
      free(contracts[i].requirements);
    }
  }
  free(contracts);
}

int select_contract_by_id(struct char_data *ch, int contract_id)
{
  int num_contracts = 0;
  struct supply_contract *contracts = generate_available_contracts(ch, &num_contracts);

  if (!contracts || contract_id < 1 || contract_id > num_contracts)
  {
    if (contracts)
    {
      free_contract_list(contracts, num_contracts);
    }
    return 0; // Invalid contract
  }

  struct supply_contract *contract = &contracts[contract_id - 1];

  // Find which slot this contract corresponds to and deactivate it
  int slot_found = -1;
  int i;
  for (i = 0; i < 5; i++)
  {
    if (GET_CRAFT(ch).supply_slot_active[i] &&
        GET_CRAFT(ch).supply_slots[i].recipe == contract->recipe &&
        GET_CRAFT(ch).supply_slots[i].variant == contract->variant &&
        GET_CRAFT(ch).supply_slots[i].contract_type == contract->contract_type)
    {
      slot_found = i;
      break;
    }
  }

  if (slot_found == -1)
  {
    send_to_char(ch, "Error: Could not find the corresponding contract slot.\r\n");
    free_contract_list(contracts, num_contracts);
    return 0;
  }

  // Deactivate the slot and set cooldown
  GET_CRAFT(ch).supply_slot_active[slot_found] = FALSE;
  GET_CRAFT(ch).supply_slot_cooldowns[slot_found] = time(NULL) + 3600; /* 1 hour cooldown */
  GET_CRAFT(ch).supply_active_slot = slot_found; /* Track which slot is being worked on */

  // Set up the player's crafting project
  GET_CRAFT(ch).crafting_recipe = contract->recipe;
  GET_CRAFT(ch).crafting_item_type = crafting_recipes[contract->recipe].object_type;
  GET_CRAFT(ch).crafting_specific = crafting_recipes[contract->recipe].object_subtype;
  GET_CRAFT(ch).craft_variant = contract->variant;
  GET_CRAFT(ch).crafting_method = SCMD_NEWCRAFT_SUPPLYORDER;
  GET_CRAFT(ch).supply_num_required = contract->quantity;
  GET_CRAFT(ch).skill_type = crafting_recipes[contract->recipe].variant_skill[contract->variant];

  // Store contract type and advanced features
  GET_CRAFT(ch).supply_contract_type = contract->contract_type;
  GET_CRAFT(ch).supply_quality_tier_requirement = contract->quality_tier_requirement;
  // Note: No longer setting expiration time as per user request

  const char *type_name = "Basic";
  const char *type_color = "\tc";
  switch (contract->contract_type)
  {
  case SUPPLY_CONTRACT_RUSH:
    type_name = "Rush";
    type_color = "\ty";
    break;
  case SUPPLY_CONTRACT_BULK:
    type_name = "Bulk";
    type_color = "\tb";
    break;
  case SUPPLY_CONTRACT_QUALITY:
    type_name = "Quality";
    type_color = "\tm";
    break;
  case SUPPLY_CONTRACT_PRESTIGE:
    type_name = "Prestige";
    type_color = "\tM";
    break;
  case SUPPLY_CONTRACT_EVENT:
    type_name = "Event";
    type_color = "\tR";
    break;
  }

  send_to_char(ch, "You've accepted the %s%s\tn contract to %s.\r\n", type_color, type_name,
               contract->description);
  send_to_char(ch, "Reward upon completion: %d experience points.\r\n", contract->reward);
  send_to_char(ch, "This contract slot will refresh after 1 hour of online time.\r\n");

  if (contract->quality_tier_requirement > QUALITY_TIER_STANDARD)
  {
    send_to_char(ch, "\tYThis contract requires superior quality materials!\tn\r\n");
  }

  // Award reputation points for accepting challenging contracts
  int rep_bonus = contract->contract_type * 2;
  add_reputation_points(ch, rep_bonus);

  free_contract_list(contracts, num_contracts);
  return 1; // Success
}

int reject_contract_by_id(struct char_data *ch, int contract_id)
{
  int num_contracts = 0;
  struct supply_contract *contracts = generate_available_contracts(ch, &num_contracts);

  if (!contracts || contract_id < 1 || contract_id > num_contracts)
  {
    if (contracts)
    {
      free_contract_list(contracts, num_contracts);
    }
    send_to_char(ch, "Invalid contract selection.\r\n");
    return 0; // Invalid contract
  }

  struct supply_contract *contract = &contracts[contract_id - 1];

  // Find which slot this contract corresponds to and deactivate it
  int slot_found = -1;
  int i;
  for (i = 0; i < 5; i++)
  {
    if (GET_CRAFT(ch).supply_slot_active[i] &&
        GET_CRAFT(ch).supply_slots[i].recipe == contract->recipe &&
        GET_CRAFT(ch).supply_slots[i].variant == contract->variant &&
        GET_CRAFT(ch).supply_slots[i].contract_type == contract->contract_type)
    {
      slot_found = i;
      break;
    }
  }

  if (slot_found == -1)
  {
    send_to_char(ch, "Error: Could not find the corresponding contract slot.\r\n");
    free_contract_list(contracts, num_contracts);
    return 0;
  }

  // Deactivate the slot
  GET_CRAFT(ch).supply_slot_active[slot_found] = FALSE;

  const char *type_name = "Basic";
  const char *type_color = "\tc";
  switch (contract->contract_type)
  {
  case SUPPLY_CONTRACT_RUSH:
    type_name = "Rush";
    type_color = "\ty";
    break;
  case SUPPLY_CONTRACT_BULK:
    type_name = "Bulk";
    type_color = "\tb";
    break;
  case SUPPLY_CONTRACT_QUALITY:
    type_name = "Quality";
    type_color = "\tm";
    break;
  case SUPPLY_CONTRACT_PRESTIGE:
    type_name = "Prestige";
    type_color = "\tM";
    break;
  case SUPPLY_CONTRACT_EVENT:
    type_name = "Event";
    type_color = "\tR";
    break;
  }

  send_to_char(ch, "You've rejected the %s%s\tn contract for %s.\r\n", type_color, type_name,
               contract->description);
  send_to_char(ch, "This slot will refresh after 1 hour of online time.\r\n");

  free_contract_list(contracts, num_contracts);
  return 1; // Success
}

// Called from comm.c every second to check and refresh supply slots for all online players
void update_supply_slots_for_all_players(void)
{
  struct descriptor_data *d;
  struct char_data *ch;
  time_t now = time(NULL);

  for (d = descriptor_list; d; d = d->next)
  {
    if (STATE(d) != CON_PLAYING || !(ch = d->character))
      continue;

    if (IS_NPC(ch))
      continue;

    // Do NOT refresh supply slots while player is actively working on a supply order
    // to avoid interfering with their current crafting session
    if (GET_CRAFT(ch).crafting_method == SCMD_NEWCRAFT_SUPPLYORDER)
    {
      // Just check cooldown status, don't refresh slots
      if (GET_CRAFT(ch).supply_cooldown_start_time > 0)
      {
        time_t elapsed = now - GET_CRAFT(ch).supply_cooldown_start_time;
        
        if (elapsed >= 7200)
        {
          /* Cooldown window has expired */
          send_to_char(ch, "\tgYour supply order cooldown window has expired!\tn\r\n");
          send_to_char(ch, "\tYYou can now request new supply orders.\tn\r\n");
          
          /* Reset cooldown */
          GET_CRAFT(ch).supply_cooldown_start_time = 0;
          GET_CRAFT(ch).supply_orders_completed_count = 0;
        }
      }
      continue; // Skip slot refresh while actively crafting
    }

    // Update this player's online time and check for slot refresh
    if (should_refresh_supply_slots(ch))
    {
      refresh_supply_slots(ch);
    }

    /* Check if supply order cooldown window has expired and notify player */
    if (GET_CRAFT(ch).supply_cooldown_start_time > 0)
    {
      time_t elapsed = now - GET_CRAFT(ch).supply_cooldown_start_time;
      
      if (elapsed >= 7200)
      {
        /* Cooldown window has expired */
        send_to_char(ch, "\tgYour supply order cooldown window has expired!\tn\r\n");
        send_to_char(ch, "\tYYou can now request new supply orders.\tn\r\n");
        
        /* Reset cooldown */
        GET_CRAFT(ch).supply_cooldown_start_time = 0;
        GET_CRAFT(ch).supply_orders_completed_count = 0;
      }
    }
  }
}

// Phase 3: Advanced Features Implementation

// Reputation system functions
int get_player_reputation_rank(struct char_data *ch)
{
  int points = get_player_reputation_points(ch);

  if (points >= REP_THRESHOLD_GRANDMASTER)
    return REP_RANK_GRANDMASTER;
  if (points >= REP_THRESHOLD_MASTER)
    return REP_RANK_MASTER;
  if (points >= REP_THRESHOLD_EXPERT)
    return REP_RANK_EXPERT;
  if (points >= REP_THRESHOLD_JOURNEYMAN)
    return REP_RANK_JOURNEYMAN;
  if (points >= REP_THRESHOLD_APPRENTICE)
    return REP_RANK_APPRENTICE;

  return REP_RANK_NOVICE;
}

int get_player_reputation_points(struct char_data *ch)
{
  return GET_CRAFT(ch).supply_reputation_points;
}

void add_reputation_points(struct char_data *ch, int points)
{
  int old_rank = get_player_reputation_rank(ch);
  GET_CRAFT(ch).supply_reputation_points += points;
  int new_rank = get_player_reputation_rank(ch);

  if (new_rank > old_rank)
  {
    send_to_char(ch, "\tgYour crafting reputation has improved!\tn\r\n");
    send_to_char(ch, "\tYYou are now a %s crafter!\tn\r\n", get_reputation_rank_name(new_rank));
  }
}

const char *get_reputation_rank_name(int rank)
{
  switch (rank)
  {
  case REP_RANK_NOVICE:
    return "Novice";
  case REP_RANK_APPRENTICE:
    return "Apprentice";
  case REP_RANK_JOURNEYMAN:
    return "Journeyman";
  case REP_RANK_EXPERT:
    return "Expert";
  case REP_RANK_MASTER:
    return "Master";
  case REP_RANK_GRANDMASTER:
    return "Grandmaster";
  default:
    return "Unknown";
  }
}

// Quality tier system
int calculate_quality_tier_bonus(struct char_data *ch)
{
  int bonus = 0;
  int i = 0;

  // Calculate bonus based on material quality
  for (i = 0; i < NUM_CRAFT_GROUPS; i++)
  {
    if (GET_CRAFT(ch).materials[i][0] > CRAFT_MAT_NONE)
    {
      int mat_grade = material_grade(GET_CRAFT(ch).materials[i][0]);
      bonus += mat_grade * 5; // 5% bonus per material grade
    }
  }

  // Cap at legendary tier (100% bonus)
  return MIN(bonus, 100);
}

// Bulk efficiency system
int calculate_bulk_efficiency_bonus(int quantity)
{
  if (quantity >= BULK_EFFICIENCY_THRESHOLD_3)
    return 30; // 30% bonus
  if (quantity >= BULK_EFFICIENCY_THRESHOLD_2)
    return 20; // 20% bonus
  if (quantity >= BULK_EFFICIENCY_THRESHOLD_1)
    return 10; // 10% bonus
  return 0;
}

// Contract expiration system
bool is_contract_expired(struct char_data *ch)
{
  if (GET_CRAFT(ch).supply_contract_expiration == 0)
    return FALSE; // No expiration set
  return (time(0) > GET_CRAFT(ch).supply_contract_expiration);
}

void update_contract_expiration(struct char_data *ch, int contract_type)
{
  time_t now = time(0);
  int hours = 0;

  switch (contract_type)
  {
  case SUPPLY_CONTRACT_BASIC:
    hours = CONTRACT_EXPIRE_BASIC;
    break;
  case SUPPLY_CONTRACT_RUSH:
    hours = CONTRACT_EXPIRE_RUSH;
    break;
  case SUPPLY_CONTRACT_BULK:
    hours = CONTRACT_EXPIRE_BULK;
    break;
  case SUPPLY_CONTRACT_QUALITY:
    hours = CONTRACT_EXPIRE_QUALITY;
    break;
  case SUPPLY_CONTRACT_PRESTIGE:
    hours = CONTRACT_EXPIRE_PRESTIGE;
    break;
  case SUPPLY_CONTRACT_EVENT:
    hours = CONTRACT_EXPIRE_EVENT;
    break;
  default:
    hours = CONTRACT_EXPIRE_BASIC;
    break;
  }

  GET_CRAFT(ch).supply_contract_expiration = now + (hours * 3600); // Convert hours to seconds
}

// Special event system
int get_event_contract_availability(void)
{
  // Simple time-based event system - events every 7 days
  time_t now = time(0);
  struct tm *local = localtime(&now);

  // Event active on Sundays (day 0)
  return (local->tm_wday == 0) ? 1 : 0;
}

bool is_special_event_active(void)
{
  return get_event_contract_availability() > 0;
}

void generate_prestige_contract(struct supply_contract *contract, struct char_data *ch)
{
  // Prestige contracts are high-level, high-reward contracts
  contract->contract_type = SUPPLY_CONTRACT_PRESTIGE;
  contract->quantity = dice(3, 3) + 2; // 5-11 items
  contract->quality_tier_requirement = QUALITY_TIER_EXCEPTIONAL;

  // Higher base reward
  contract->reward = generate_contract_reward(SUPPLY_CONTRACT_PRESTIGE, contract->quantity,
                                              contract->difficulty_modifier + 5);

  // Enhanced descriptions
  char desc_buf[256], req_buf[256];
  const char *item_name = "masterwork item";
  if (contract->recipe > 0 && contract->variant >= 0)
  {
    item_name = crafting_recipes[contract->recipe].variant_descriptions[contract->variant];
  }

  snprintf(desc_buf, sizeof(desc_buf), "PRESTIGE: Craft %d exceptional %s%s for the royal court",
           contract->quantity, item_name, (contract->quantity > 1) ? "s" : "");
  snprintf(req_buf, sizeof(req_buf), "Master-grade materials + exceptional craftsmanship required");

  contract->description = strdup(desc_buf);
  contract->requirements = strdup(req_buf);
}

void generate_event_contract(struct supply_contract *contract, struct char_data *ch)
{
  // Event contracts are time-limited special opportunities
  contract->contract_type = SUPPLY_CONTRACT_EVENT;
  contract->quantity = dice(2, 4) + 1; // 3-9 items
  contract->quality_tier_requirement = QUALITY_TIER_SUPERIOR;

  // Bonus reward for limited time
  contract->reward = generate_contract_reward(SUPPLY_CONTRACT_EVENT, contract->quantity,
                                              contract->difficulty_modifier) *
                     2; // Double reward

  // Event-specific descriptions
  char desc_buf[256], req_buf[256];
  const char *item_name = "festival item";
  if (contract->recipe > 0 && contract->variant >= 0)
  {
    item_name = crafting_recipes[contract->recipe].variant_descriptions[contract->variant];
  }

  snprintf(desc_buf, sizeof(desc_buf), "FESTIVAL EVENT: Craft %d %s%s for the harvest celebration",
           contract->quantity, item_name, (contract->quantity > 1) ? "s" : "");
  snprintf(req_buf, sizeof(req_buf), "Superior materials + limited time offer (ends soon!)");

  contract->description = strdup(desc_buf);
  contract->requirements = strdup(req_buf);
}

void show_supply_order_cooldowns(struct char_data *ch)
{
  time_t now = time(NULL);

  /* Character validation */
  if (!ch)
  {
    return;
  }

  text_line(ch, "SUPPLY ORDER TIMING INFORMATION", 90, '-', '-');

  /* Supply order completion cooldown (3 per 2 hours) */
  send_to_char(ch, "\r\nSupply Order Completion Cooldown (3 per 2 hours):\r\n");

  if (GET_CRAFT(ch).supply_cooldown_start_time == 0)
  {
    send_to_char(ch, "  \tgNo cooldown active - you can start a new supply order immediately.\tn\r\n");
  }
  else
  {
    time_t elapsed = now - GET_CRAFT(ch).supply_cooldown_start_time;
    
    if (elapsed >= 7200)
    {
      /* Cooldown has expired */
      send_to_char(ch, "  \tgCooldown expired - you can start a new supply order cycle.\tn\r\n");
    }
    else
    {
      /* Cooldown is active */
      int remaining_seconds = 7200 - elapsed;
      int hours = remaining_seconds / 3600;
      int minutes = (remaining_seconds % 3600) / 60;
      
      send_to_char(ch, "  \tyActive Cooldown Window\tn\r\n");
      send_to_char(ch, "  Supply orders completed: %d/3\r\n", GET_CRAFT(ch).supply_orders_completed_count);
      send_to_char(ch, "  Time remaining: %d hours, %d minutes\r\n", hours, minutes);
      
      if (GET_CRAFT(ch).supply_orders_completed_count < 3)
      {
        int remaining_orders = 3 - GET_CRAFT(ch).supply_orders_completed_count;
        send_to_char(ch, "  Can complete %d more order%s before hitting limit\r\n",
                     remaining_orders, (remaining_orders > 1 ? "s" : ""));
      }
      else
      {
        send_to_char(ch, "  \trLimit reached\tn - Must wait for cooldown to expire\r\n");
      }
    }
  }

  send_to_char(ch, "\r\n");
}

// Function to get the display name for each crafting tool slot
const char *get_craft_tool_name(int wear_slot)
{
  switch (wear_slot)
  {
  case WEAR_CRAFT_SICKLE:
    return "harvesting sickle (gathering)";
  case WEAR_CRAFT_AXE:
    return "chopping axe (forestry)";
  case WEAR_CRAFT_KNIFE:
    return "skinning knife (hunting)";
  case WEAR_CRAFT_PICKAXE:
    return "pickaxe (mining)";
  case WEAR_CRAFT_ALCHEMY:
    return "alchemy set (alchemy)";
  case WEAR_CRAFT_ARMOR_HAMMER:
    return "armorsmith's hammer (armorsmithing)";
  case WEAR_CRAFT_JEWEL_PLIERS:
    return "jewel's pliers (jewelcraft)";
  case WEAR_CRAFT_NEEDLE:
    return "sewing needle (tailoring)";
  case WEAR_CRAFT_WEAPON_HAMMER:
    return "weaponsmith's hammer (weaponsmithing)";
  default:
    return "unknown tool";
  }
}

// Function to display equipped crafting tools
void show_craft_equipment(struct char_data *ch)
{
  int craft_slots[] = {WEAR_CRAFT_SICKLE,       WEAR_CRAFT_AXE,     WEAR_CRAFT_KNIFE,
                       WEAR_CRAFT_PICKAXE,      WEAR_CRAFT_ALCHEMY, WEAR_CRAFT_ARMOR_HAMMER,
                       WEAR_CRAFT_JEWEL_PLIERS, WEAR_CRAFT_NEEDLE,  WEAR_CRAFT_WEAPON_HAMMER};
  int num_slots = sizeof(craft_slots) / sizeof(craft_slots[0]);
  int i;
  int has_equipment = 0;

  send_to_char(ch, "\r\n\tgYour Crafting Equipment:\tn\r\n");
  send_to_char(ch, "\tW================================\tn\r\n");

  for (i = 0; i < num_slots; i++)
  {
    int slot = craft_slots[i];
    struct obj_data *obj = GET_EQ(ch, slot);

    if (obj)
    {
      send_to_char(ch, "\tc%-35s\tn : %s\r\n", get_craft_tool_name(slot), obj->short_description);
      has_equipment = 1;
    }
    else
    {
      send_to_char(ch, "\tD%-35s\tn : \tDnothing\tn\r\n", get_craft_tool_name(slot));
    }
  }

  if (!has_equipment)
  {
    send_to_char(ch, "\r\n\tYYou have no crafting tools equipped.\tn\r\n");
    send_to_char(ch, "Use '\tcwear <tool>\tn' to equip crafting tools.\r\n");
  }

  send_to_char(ch, "\r\n");
}

// Main craft equipment command handler
void newcraft_equipment(struct char_data *ch, const char *argument)
{
  show_craft_equipment(ch);
}

void newcraft_show_tools(struct char_data *ch, const char *argument)
{
  struct obj_data *tool = NULL;
  int ability, i, found_tools = 0;

  send_to_char(ch, "\tcCrafting and Harvesting Tools Status:\tn\r\n");
  send_to_char(ch,
               "==========================================================================\r\n");
  send_to_char(ch, "%-20s %-22s %-15s %-10s\r\n", "Skill", "Equipment Slot", "Bonus",
               "Tool Equipped");
  send_to_char(ch,
               "==========================================================================\r\n");

  // Loop through all crafting and harvesting abilities
  for (ability = START_CRAFT_ABILITIES; ability <= END_HARVEST_ABILITIES; ability++)
  {
    if (!is_crafting_skill_in_game(ability))
      continue;
    bool tool_found = FALSE;
    char bonus_string[20];
    char where_string[50];

    // Check all equipment slots for a tool that matches this ability
    for (i = 0; i < NUM_WEARS; i++)
    {
      snprintf(bonus_string, sizeof(bonus_string), "---");
      tool = GET_EQ(ch, i);
      if (tool && GET_OBJ_TYPE(tool) == ITEM_CRAFTING_TOOL)
      {
        int tool_skill = GET_OBJ_VAL(tool, 0);
        int tool_bonus = GET_OBJ_VAL(tool, 1);

        if (tool_skill == ability && tool_bonus >= 0)
        {
          snprintf(bonus_string, sizeof(bonus_string), "+%d", tool_bonus);
          snprintf(where_string, sizeof(where_string), "%s", wear_where[i]);
          strip_colors(where_string);
          send_to_char(ch, "%-20s %-22s %3s \tc%-15s\tn\r\n", ability_names[ability], where_string,
                       bonus_string,
                       tool->short_description ? tool->short_description : "a crafting tool");
          tool_found = TRUE;
          found_tools++;
          break; // Found the tool for this ability, move to next ability
        }
      }
    }

    // If no tool found for this ability, show empty slot
    if (!tool_found)
    {
      send_to_char(ch, "%-20s %-22s %3s \ty%-15s\tn\r\n", ability_names[ability], "---", "---",
                   "None");
    }
  }

  send_to_char(ch,
               "==========================================================================\r\n");
  if (found_tools == 0)
  {
    send_to_char(ch, "\tyNo crafting or harvesting tools currently equipped.\tn\r\n");
    send_to_char(
        ch,
        "\twEquip crafting tools to gain bonuses to your crafting and harvesting skills!\tn\r\n");
  }
  else
  {
    send_to_char(ch, "\tcTotal Tools Equipped: \ty%d\tn\r\n", found_tools);
  }
  send_to_char(ch, "\twUse 'craft help' for more crafting information.\tn\r\n");
}

bool is_crafting_skill_in_game(int skill)
{
  switch (skill)
  {
  case ABILITY_CRAFT_TAILORING:
  case ABILITY_CRAFT_ARMORSMITHING:
  case ABILITY_CRAFT_WEAPONSMITHING:
  case ABILITY_CRAFT_JEWELCRAFTING:
  case ABILITY_CRAFT_ALCHEMY:
  case ABILITY_HARVEST_FORESTRY:
  case ABILITY_HARVEST_MINING:
  case ABILITY_HARVEST_HUNTING:
  case ABILITY_HARVEST_GATHERING:
    return true;
  }
  return false;
}

/**
 * Get base DC for creating a golem based on type and size
 * Small base DC is 15, increases by 5 for each size tier
 */
int get_golem_base_dc(int golem_type, int golem_size)
{
  int base_dc = 15; // Small golem DC
  switch (golem_type)
  {
  case GOLEM_TYPE_WOOD:
    base_dc += 0; // Wood golems are easier
    break;
  case GOLEM_TYPE_STONE:
    base_dc += 10; // Stone golems are moderate
    break;
  case GOLEM_TYPE_IRON:
    base_dc += 20; // Iron golems are hardest
    break;
  default:
    base_dc += 0; // Default to wood golem DC
    break;
  }

  // Adjust by size tier
  base_dc += (golem_size * 5);

  return base_dc;
}

/**
 * Get material requirements for a golem
 * Returns the number of material groups required
 */
int get_golem_material_requirements(int golem_type, int golem_size, int *material_types,
                                    int *material_amounts)
{
  int base_material_amount = 0;
  int secondary_material_amount = 0;
  int primary_material = 0;

  if (!material_types || !material_amounts)
    return 0;

  // Base amounts for small golem
  switch (golem_type)
  {
  case GOLEM_TYPE_WOOD:
    primary_material = CRAFT_MAT_ASH_WOOD;
    base_material_amount = 50;     // Small wood golem needs 50 units of wood
    secondary_material_amount = 10; // and 10 units of metal
    break;
  case GOLEM_TYPE_STONE:
    primary_material = CRAFT_MAT_STONE;
    base_material_amount = 50;     // Small stone golem needs 50 units of stone
    secondary_material_amount = 10; // and 10 units of metal
    break;
  case GOLEM_TYPE_IRON:
    primary_material = CRAFT_MAT_IRON;
    base_material_amount = 50; // Small iron golem needs 50 units of iron
    secondary_material_amount = 10;
    break;
  default:
    return 0;
  }

  // Scale material amounts by size
  base_material_amount *= (golem_size + 1);
  secondary_material_amount *= (golem_size + 1);

  material_types[0] = primary_material;
  material_amounts[0] = base_material_amount;

  if (secondary_material_amount > 0)
  {
    // Iron or steel as secondary material depending on primary
    material_types[1] = (primary_material == CRAFT_MAT_STONE || primary_material == CRAFT_MAT_IRON)
                            ? CRAFT_MAT_STEEL
                            : CRAFT_MAT_BRONZE;
    material_amounts[1] = secondary_material_amount;
    return 2;
  }

  return 1;
}

/**
 * Get mote requirements for a golem
 * Returns number of mote types required
 */
int get_golem_mote_requirements(int golem_type, int golem_size, int *mote_types, int *mote_amounts)
{
  int base_motes = 0;
  int i = 0;
  float size_multiplier = 1.0;

  if (!mote_types || !mote_amounts)
    return 0;

  // Determine base mote amount based on golem type
  switch (golem_type)
  {
  case GOLEM_TYPE_WOOD:
    base_motes = 5;
    break;
  case GOLEM_TYPE_STONE:
    base_motes = 10;
    break;
  case GOLEM_TYPE_IRON:
    base_motes = 20;
    break;
  default:
    return 0;
  }

  // Calculate size multiplier: 20% more per size tier (1.0, 1.2, 1.4, 1.6)
  size_multiplier = 1.0 + (golem_size * 0.2);

  // All 9 mote types required
  for (i = 1; i <= NUM_CRAFT_MOTES - 1; i++)
  {
    mote_types[i - 1] = i;
    mote_amounts[i - 1] = (int)(base_motes * size_multiplier);
  }

  return NUM_CRAFT_MOTES - 1; // Return 9 (one of each mote type)
}

void set_golem_type(struct char_data *ch, const char *arg)
{
  if (!*arg)
  {
    send_to_char(ch, "Available golem types:\r\n");
    send_to_char(ch, "  wood  - Nimble golem made from wood and metal\r\n");
    send_to_char(ch, "  stone - Sturdy golem made from stone and metal\r\n");
    send_to_char(ch, "  iron  - Powerful golem made from iron\r\n");
    return;
  }

  if (is_abbrev(arg, "wood"))
  {
    GET_CRAFT(ch).golem_type = GOLEM_TYPE_WOOD;
    send_to_char(ch, "Golem type set to: Wood\r\n");
  }
  else if (is_abbrev(arg, "stone"))
  {
    GET_CRAFT(ch).golem_type = GOLEM_TYPE_STONE;
    send_to_char(ch, "Golem type set to: Stone\r\n");
  }
  else if (is_abbrev(arg, "iron"))
  {
    GET_CRAFT(ch).golem_type = GOLEM_TYPE_IRON;
    send_to_char(ch, "Golem type set to: Iron\r\n");
  }
  else
  {
    send_to_char(ch, "Unknown golem type. Use: wood, stone, or iron\r\n");
  }
}

void set_golem_size(struct char_data *ch, const char *arg)
{
  if (!*arg)
  {
    send_to_char(ch, "Available golem sizes:\r\n");
    send_to_char(ch, "  small  - Easier to construct, requires fewer materials\r\n");
    send_to_char(ch, "  medium - Standard size golem\r\n");
    send_to_char(ch, "  large  - Larger construction, more difficult\r\n");
    send_to_char(ch, "  huge   - Most powerful, requires large amount of materials\r\n");
    return;
  }

  if (is_abbrev(arg, "small"))
  {
    GET_CRAFT(ch).golem_size = GOLEM_SIZE_SMALL;
    send_to_char(ch, "Golem size set to: Small\r\n");
  }
  else if (is_abbrev(arg, "medium"))
  {
    GET_CRAFT(ch).golem_size = GOLEM_SIZE_MEDIUM;
    send_to_char(ch, "Golem size set to: Medium\r\n");
  }
  else if (is_abbrev(arg, "large"))
  {
    GET_CRAFT(ch).golem_size = GOLEM_SIZE_LARGE;
    send_to_char(ch, "Golem size set to: Large\r\n");
  }
  else if (is_abbrev(arg, "huge"))
  {
    GET_CRAFT(ch).golem_size = GOLEM_SIZE_HUGE;
    send_to_char(ch, "Golem size set to: Huge\r\n");
  }
  else
  {
    send_to_char(ch, "Unknown golem size. Use: small, medium, large, or huge\r\n");
  }
}

void show_current_golem_craft(struct char_data *ch)
{
  int i = 0, material_types[3] = {0}, material_amounts[3] = {0};
  int mote_types[NUM_CRAFT_MOTES] = {0}, mote_amounts[NUM_CRAFT_MOTES] = {0};
  int num_mats = 0, num_motes = 0;
  const char *golem_type_names[] = {"None", "Wood", "Stone", "Iron"};
  const char *golem_size_names[] = {"Small", "Medium", "Large", "Huge"};

  send_to_char(ch, "\tc=== Current Golem Craft Project ===\tn\r\n");
  send_to_char(ch, "Golem Type: %s\r\n", golem_type_names[GET_CRAFT(ch).golem_type]);
  send_to_char(ch, "Golem Size: %s\r\n", golem_size_names[GET_CRAFT(ch).golem_size]);

  if (GET_CRAFT(ch).golem_type != GOLEM_TYPE_NONE && GET_CRAFT(ch).golem_size != -1)
  {
    int dc = get_golem_base_dc(GET_CRAFT(ch).golem_type, GET_CRAFT(ch).golem_size);
    send_to_char(ch, "Base DC: %d (vs Arcana skill)\r\n", dc);

    num_mats = get_golem_material_requirements(GET_CRAFT(ch).golem_type, GET_CRAFT(ch).golem_size,
                                               material_types, material_amounts);
    send_to_char(ch, "\tcMaterials Required:\tn\r\n");
    for (i = 0; i < num_mats; i++)
    {
      if (material_types[i] > 0)
      {
        int owned;

        // For wood golems (primary material), must use a single wood type
        if (GET_CRAFT(ch).golem_type == GOLEM_TYPE_WOOD && i == 0)
        {
          int mat;
          int has_sufficient_wood = false;

          // Check if any single wood type has enough
          for (mat = 1; mat < NUM_CRAFT_MATS; mat++)
          {
            if (craft_group_by_material(mat) == CRAFT_GROUP_WOOD)
            {
              if (GET_CRAFT_MAT(ch, mat) >= material_amounts[i])
              {
                has_sufficient_wood = true;
                break;
              }
            }
          }

          // Show status
          owned = 0;
          for (mat = 1; mat < NUM_CRAFT_MATS; mat++)
          {
            if (craft_group_by_material(mat) == CRAFT_GROUP_WOOD)
            {
              owned += GET_CRAFT_MAT(ch, mat);
            }
          }

          send_to_char(ch,
                       "  Wood (any single type): %d units needed (you have %d total across all "
                       "types, single-type sufficient: %s) %s\r\n",
                       material_amounts[i], owned, has_sufficient_wood ? "\tGYes\tn" : "\tRNo\tn",
                       has_sufficient_wood ? "\tG+\tn" : "\tR!\tn");
        }
        else
        {
          owned = GET_CRAFT_MAT(ch, material_types[i]);
          send_to_char(ch, "  %s: %d units (owned: %d) %s\r\n",
                       crafting_materials[material_types[i]], material_amounts[i], owned,
                       owned >= material_amounts[i] ? "\tG+\tn" : "\tR!\tn");
        }
      }
    }

    num_motes = get_golem_mote_requirements(GET_CRAFT(ch).golem_type, GET_CRAFT(ch).golem_size,
                                            mote_types, mote_amounts);
    send_to_char(ch, "\tcMotes Required:\tn\r\n");
    for (i = 0; i < num_motes; i++)
    {
      if (mote_types[i] > 0)
      {
        int owned = GET_CRAFT_MOTES(ch, mote_types[i]);
        send_to_char(ch, "  %s: %d (owned: %d) %s\r\n", crafting_motes[mote_types[i]],
                     mote_amounts[i], owned, owned >= mote_amounts[i] ? "\tG+\tn" : "\tR!\tn");
      }
    }
  }
}

bool has_golem_constructed(struct char_data *ch)
{
    if (ch->char_specials.saved.golem_stored_type > 0 || ch->char_specials.saved.golem_stored_size > 0)
    {
        return true;
    }
    return false;
}

void reset_current_golem_craft(struct char_data *ch)
{
  GET_CRAFT(ch).golem_type = GOLEM_TYPE_NONE;
  GET_CRAFT(ch).golem_size = GOLEM_SIZE_SMALL;
  memset(GET_CRAFT(ch).golem_materials, 0, sizeof(GET_CRAFT(ch).golem_materials));
  memset(GET_CRAFT(ch).golem_motes_required, 0, sizeof(GET_CRAFT(ch).golem_motes_required));
  send_to_char(ch, "Golem crafting project reset.\r\n");
}

bool begin_golem_craft(struct char_data *ch)
{
  int i = 0, material_types[3] = {0}, material_amounts[3] = {0};
  int mote_types[NUM_CRAFT_MOTES] = {0}, mote_amounts[NUM_CRAFT_MOTES] = {0};
  int num_mats = 0, num_motes = 0;
  int seconds = 0;

  if (has_golem_follower(ch))
  {
    send_to_char(ch, "You cannot create a new golem while you have an active golem follower.\r\n");
    return false;
  }

  if (has_golem_constructed(ch))
  {
    send_to_char(ch, "You cannot create a new golem while you have an existing golem construct. Shutdown your current golem before starting a new one.\r\n");
    return false;
  }

  if (GET_CRAFT(ch).golem_type == GOLEM_TYPE_NONE)
  {
    send_to_char(
        ch, "You must set a golem type first. Use: craft create golem type (wood|stone|iron)\r\n");
    return false;
  }

  // Check for required feats based on golem type
  switch (GET_CRAFT(ch).golem_type)
  {
  case GOLEM_TYPE_WOOD:
    if (!HAS_FEAT(ch, FEAT_CONSTRUCT_WOOD_GOLEM))
    {
      send_to_char(
          ch,
          "You do not have the 'construct wood golem' feat required to create wood golems.\r\n");
      return false;
    }
    break;
  case GOLEM_TYPE_STONE:
    if (!HAS_FEAT(ch, FEAT_CONSTRUCT_STONE_GOLEM))
    {
      send_to_char(
          ch,
          "You do not have the 'construct stone golem' feat required to create stone golems.\r\n");
      return false;
    }
    break;
  case GOLEM_TYPE_IRON:
    if (!HAS_FEAT(ch, FEAT_CONSTRUCT_IRON_GOLEM))
    {
      send_to_char(
          ch,
          "You do not have the 'construct iron golem' feat required to create iron golems.\r\n");
      return false;
    }
    break;
  }

  num_mats = get_golem_material_requirements(GET_CRAFT(ch).golem_type, GET_CRAFT(ch).golem_size,
                                             material_types, material_amounts);
  num_motes = get_golem_mote_requirements(GET_CRAFT(ch).golem_type, GET_CRAFT(ch).golem_size,
                                          mote_types, mote_amounts);

  // Check materials
  for (i = 0; i < num_mats; i++)
  {
    if (material_types[i] > 0)
    {
      // For wood golems (primary material), must use a single wood type with sufficient units
      if (GET_CRAFT(ch).golem_type == GOLEM_TYPE_WOOD && i == 0)
      {
        // Find the first wood type that has enough units
        int mat;
        int found_wood_type = -1;

        for (mat = 1; mat < NUM_CRAFT_MATS; mat++)
        {
          if (craft_group_by_material(mat) == CRAFT_GROUP_WOOD)
          {
            if (GET_CRAFT_MAT(ch, mat) >= material_amounts[i])
            {
              found_wood_type = mat;
              break; // Lowest grade wood type will be used first
            }
          }
        }

        if (found_wood_type == -1)
        {
          send_to_char(ch,
                       "You do not have a single wood type with enough materials. Need %d units of "
                       "one wood type.\r\n",
                       material_amounts[i]);
          return false;
        }

        // Store the selected wood type for consumption
        GET_CRAFT(ch).golem_materials[i][0] = found_wood_type;
      }
      else
      {
        // For non-wood materials or secondary materials, check specific type
        if (GET_CRAFT_MAT(ch, material_types[i]) < material_amounts[i])
        {
          send_to_char(ch, "You do not have enough %s. Need %d, have %d.\r\n",
                       crafting_materials[material_types[i]], material_amounts[i],
                       GET_CRAFT_MAT(ch, material_types[i]));
          return false;
        }
      }
    }
  }

  // Check motes
  for (i = 0; i < num_motes; i++)
  {
    if (mote_types[i] > 0 && GET_CRAFT_MOTES(ch, mote_types[i]) < mote_amounts[i])
    {
      send_to_char(ch, "You do not have enough %s. Need %d, have %d.\r\n",
                   crafting_motes[mote_types[i]], mote_amounts[i],
                   GET_CRAFT_MOTES(ch, mote_types[i]));
      return false;
    }
  }

  // Store requirements for later consumption
  for (i = 0; i < num_mats; i++)
  {
    GET_CRAFT(ch).golem_materials[i][0] = material_types[i];
    GET_CRAFT(ch).golem_materials[i][1] = material_amounts[i];
  }

  for (i = 0; i < num_motes; i++)
  {
    GET_CRAFT(ch).golem_motes_required[i] = mote_amounts[i];
  }

  // Calculate craft time (base 2 minutes per size tier + 1 minute base)
  seconds = CREATE_BASE_TIME + (GET_CRAFT(ch).golem_size * 60);

  GET_CRAFT(ch).crafting_method = SCMD_NEWCRAFT_GOLEM;
  GET_CRAFT(ch).craft_duration = seconds;
  GET_CRAFT(ch).dc = get_golem_base_dc(GET_CRAFT(ch).golem_type, GET_CRAFT(ch).golem_size);
  GET_CRAFT(ch).skill_type = ABILITY_ARCANA; // Use Arcana skill for golem crafting

  const char *golem_type_names[] = {"", "wood", "stone", "iron"};
  const char *golem_size_names[] = {"small", "medium", "large", "huge"};

  send_to_char(ch,
               "You begin constructing a %s %s golem. This will take approximately %d minutes and "
               "%d seconds.\r\n",
               golem_size_names[GET_CRAFT(ch).golem_size],
               golem_type_names[GET_CRAFT(ch).golem_type], seconds / 60, seconds % 60);
  act("$n begins constructing a golem.", FALSE, ch, 0, 0, TO_ROOM);

  return true;
}

/**
 * Get the mob VNUM for a golem based on type and size
 */
int get_golem_vnum(int golem_type, int golem_size)
{
  // VNUMs: Wood 16500-16503, Stone 16504-16507, Iron 16508-16511
  // Each type has 4 sizes: small(0), medium(1), large(2), huge(3)

  int base_vnum = 0;

  switch (golem_type)
  {
  case GOLEM_TYPE_WOOD:
    base_vnum = GOLEM_WOOD_SMALL;
    break;
  case GOLEM_TYPE_STONE:
    base_vnum = GOLEM_STONE_SMALL;
    break;
  case GOLEM_TYPE_IRON:
    base_vnum = GOLEM_IRON_SMALL;
    break;
  default:
    return NOBODY;
  }

  // Add size offset (0 for small, 1 for medium, 2 for large, 3 for huge)
  return base_vnum + golem_size;
}

/**
 * Get golem type from VNUM (reverse of get_golem_vnum)
 */
int get_golem_type_from_vnum(int vnum)
{
  if (vnum >= GOLEM_WOOD_SMALL && vnum <= GOLEM_WOOD_HUGE)
    return GOLEM_TYPE_WOOD;
  else if (vnum >= GOLEM_STONE_SMALL && vnum <= GOLEM_STONE_HUGE)
    return GOLEM_TYPE_STONE;
  else if (vnum >= GOLEM_IRON_SMALL && vnum <= GOLEM_IRON_HUGE)
    return GOLEM_TYPE_IRON;

  return -1;
}

/**
 * Get golem size from VNUM (reverse of get_golem_vnum)
 */
int get_golem_size_from_vnum(int vnum)
{
  int golem_type = get_golem_type_from_vnum(vnum);
  int base_vnum = 0;

  if (golem_type < 0)
    return -1;

  switch (golem_type)
  {
  case GOLEM_TYPE_WOOD:
    base_vnum = GOLEM_WOOD_SMALL;
    break;
  case GOLEM_TYPE_STONE:
    base_vnum = GOLEM_STONE_SMALL;
    break;
  case GOLEM_TYPE_IRON:
    base_vnum = GOLEM_IRON_SMALL;
    break;
  default:
    return -1;
  }

  // Size is the offset from the base VNUM
  return vnum - base_vnum;
}

/**
 * Check if character already has a golem follower
 */
bool has_golem_follower(struct char_data *ch)
{
  struct follow_type *k;

  if (!ch->followers)
    return false;

  for (k = ch->followers; k; k = k->next)
  {
    if (IS_PET(k->follower) && MOB_FLAGGED(k->follower, MOB_GOLEM))
      return true;
  }

  return false;
}

struct char_data *get_active_golem_follower(struct char_data *ch)
{
  struct follow_type *k;

  if (!ch)
    return NULL;

  for (k = ch->followers; k; k = k->next)
  {
    if (IS_PET(k->follower) && MOB_FLAGGED(k->follower, MOB_GOLEM))
      return k->follower;
  }

  return NULL;
}

static void battlefield_retrieve_active_golem(struct char_data *ch, struct char_data *golem)
{
  if (!ch || !golem)
    return;

  if (!has_artificer_battlefield_retrieval(ch))
  {
    send_to_char(ch, "You have not purchased Battlefield Retrieval.\r\n");
    return;
  }

  if (!can_recall_golem(ch))
    return;

  ch->char_specials.saved.golem_stored_hp = MAX(1, GET_HIT(golem));
  ch->char_specials.saved.golem_recall_cooldown = time(NULL) + GOLEM_BATTLEFIELD_RETRIEVAL_COOLDOWN;

  send_to_char(ch, "You collapse your golem into a compact battlefield retrieval matrix.\r\n");
  act("$n gestures sharply and $s golem folds into a compact lattice of arcane components.",
      FALSE, ch, 0, 0, TO_ROOM);

  extract_char(golem);
}

/**
 * Recover materials from a golem when it's destroyed or dies
 * recovery_percent: percentage of original materials to recover (e.g., 50 for 50%)
 */
void recover_golem_materials(struct char_data *ch, struct char_data *golem, int recovery_percent)
{
  extern int get_golem_type_from_vnum(int vnum);
  extern int get_golem_size_from_vnum(int vnum);
  extern const char *material_name[];

  int golem_type, golem_size, golem_vnum;
  int material_types[3] = {0}, material_amounts[3] = {0};
  int num_mats = 0, i = 0;
  int recovered = 0;

  if (!ch || !golem || !IS_NPC(golem))
    return;

  golem_vnum = GET_MOB_VNUM(golem);
  golem_type = get_golem_type_from_vnum(golem_vnum);
  golem_size = get_golem_size_from_vnum(golem_vnum);

  if (golem_type < 0 || golem_size < 0)
    return;

  num_mats =
      get_golem_material_requirements(golem_type, golem_size, material_types, material_amounts);

  send_to_char(ch, "\tGMaterials recovered:\tn\r\n");

  for (i = 0; i < num_mats; i++)
  {
    if (material_types[i] > 0 && material_amounts[i] > 0)
    {
      recovered = (material_amounts[i] * recovery_percent) / 100;
      if (recovered > 0)
      {
        GET_CRAFT_MAT(ch, material_types[i]) += recovered;
        send_to_char(ch, "  %d %s\r\n", recovered, material_name[material_types[i]]);
      }
    }
  }
}

void craft_golem_complete(struct char_data *ch)
{
  int roll, dc, skill, i = 0;
  int material_types[3] = {0}, material_amounts[3] = {0};
  int mote_types[NUM_CRAFT_MOTES] = {0}, mote_amounts[NUM_CRAFT_MOTES] = {0};
  int num_mats = 0, num_motes = 0;
  int golem_vnum = NOBODY;
  struct char_data *golem = NULL;
  const char *golem_type_names[] = {"", "wood", "stone", "iron"};
  const char *golem_size_names[] = {"small", "medium", "large", "huge"};

  roll = d20(ch);
  dc = GET_CRAFT(ch).dc;
  skill = compute_ability(ch, ABILITY_ARCANA);

  send_to_char(ch, "You rolled %d + your Arcana skill of %d = total of %d vs. dc %d.\r\n", roll,
               skill, roll + skill, dc);

  num_mats = get_golem_material_requirements(GET_CRAFT(ch).golem_type, GET_CRAFT(ch).golem_size,
                                             material_types, material_amounts);
  num_motes = get_golem_mote_requirements(GET_CRAFT(ch).golem_type, GET_CRAFT(ch).golem_size,
                                          mote_types, mote_amounts);

  if ((roll + skill) >= dc)
  {
    // Success! First check if they already have a golem
    if (has_golem_follower(ch))
    {
      send_to_char(ch, "\tRYou already have a golem!\tn You must destroy your current golem before "
                       "creating a new one.\r\n");
      reset_current_golem_craft(ch);
      return;
    }

    // Check if they have a stored golem of a different type
    if (ch->char_specials.saved.golem_stored_type != GOLEM_TYPE_NONE &&
        ch->char_specials.saved.golem_stored_type != GET_CRAFT(ch).golem_type)
    {
      send_to_char(ch, "\tRYou already have a %s golem stored!\tn You must shutdown your stored golem "
                       "before creating a different type.\r\n",
                   ch->char_specials.saved.golem_stored_type == GOLEM_TYPE_WOOD ? "wood" :
                   ch->char_specials.saved.golem_stored_type == GOLEM_TYPE_STONE ? "stone" : "iron");
      reset_current_golem_craft(ch);
      return;
    }

    // Get the appropriate VNUM for this golem
    golem_vnum = get_golem_vnum(GET_CRAFT(ch).golem_type, GET_CRAFT(ch).golem_size);

    if (golem_vnum == NOBODY)
    {
      send_to_char(ch, "\tRError:\tn Invalid golem type/size combination!\r\n");
      reset_current_golem_craft(ch);
      return;
    }

    // Check if we can add this follower
    if (!can_add_follower(ch, golem_vnum))
    {
      send_to_char(ch, "\tRYou cannot control another follower right now!\tn\r\n");
      reset_current_golem_craft(ch);
      return;
    }

    // Consume materials and motes
    for (i = 0; i < num_mats; i++)
    {
      if (material_types[i] > 0)
      {
        // For wood golems, the material type was already selected in begin_golem_craft
        // and is stored in GET_CRAFT(ch).golem_materials[i][0]
        if (GET_CRAFT(ch).golem_type == GOLEM_TYPE_WOOD && i == 0)
        {
          int selected_wood_type = GET_CRAFT(ch).golem_materials[i][0];
          GET_CRAFT_MAT(ch, selected_wood_type) -= material_amounts[i];
        }
        else
        {
          // For non-wood materials or secondary materials, consume specific type
          GET_CRAFT_MAT(ch, material_types[i]) -= material_amounts[i];
        }
      }
    }

    for (i = 0; i < num_motes; i++)
    {
      if (mote_types[i] > 0)
        GET_CRAFT_MOTES(ch, mote_types[i]) -= mote_amounts[i];
    }

    // Create the golem mob
    golem = read_mobile(golem_vnum, VIRTUAL);

    if (!golem)
    {
      send_to_char(ch, "\tRError:\tn Failed to create golem mob! Contact an administrator.\r\n");
      reset_current_golem_craft(ch);
      return;
    }

    char_to_room(golem, IN_ROOM(ch));

    IS_CARRYING_W(golem) = 0;
    IS_CARRYING_N(golem) = 0;
    SET_BIT_AR(AFF_FLAGS(golem), AFF_CHARM);
    SET_BIT_AR(MOB_FLAGS(golem), MOB_GOLEM);
    /* Ensure golems are treated as constructs for all immunity checks */
    GET_REAL_RACE(golem) = RACE_TYPE_CONSTRUCT;

    send_to_char(ch, "\tGSuccess!\tn You have successfully constructed a %s %s golem!\r\n",
                 golem_size_names[GET_CRAFT(ch).golem_size],
                 golem_type_names[GET_CRAFT(ch).golem_type]);
    send_to_char(ch, "Your golem can be recalled for free if it is destroyed (5 minute cooldown).\r\n");
    act("$n successfully constructs a golem!", FALSE, ch, 0, 0, TO_ROOM);
    act("$N springs to life and begins following $n!", FALSE, ch, 0, golem, TO_ROOM);

    load_mtrigger(golem);
    add_follower(golem, ch);

    sync_artificer_construct_command_bonuses(golem);

    // Auto-join group if creator is leading a group
    if (!GROUP(golem) && GROUP(ch) && GROUP_LEADER(GROUP(ch)) == ch)
      join_group(golem, GROUP(ch));
    
    // Store golem info for recall
    ch->char_specials.saved.golem_stored_type = GET_CRAFT(ch).golem_type;
    ch->char_specials.saved.golem_stored_size = GET_CRAFT(ch).golem_size;
    ch->char_specials.saved.golem_stored_hp = 0;
  }
  else
  {
    send_to_char(ch, "\tRFailed!\tn The construction fails, and some of the materials are wasted.\r\n");

    // Consume materials even on failure for realism
    for (i = 0; i < num_mats; i++)
    {
      if (material_types[i] > 0)
        GET_CRAFT_MAT(ch, material_types[i]) -= MAX(1, material_amounts[i] / 10); // Lose 10% on fail
    }

    for (i = 0; i < num_motes; i++)
    {
      if (mote_types[i] > 0)
        GET_CRAFT_MOTES(ch, mote_types[i]) -= MAX(1, mote_amounts[i] / 10); // Lose 10% on fail
    }
  }

  reset_current_golem_craft(ch);
}

// Check if player can recall their golem based on cooldown
bool can_recall_golem(struct char_data *ch)
{
  time_t now = time(NULL);
  
  // Check cooldown
  if (ch->char_specials.saved.golem_recall_cooldown > now)
  {
    int seconds_left = ch->char_specials.saved.golem_recall_cooldown - now;
    int minutes_left = seconds_left / 60;
    seconds_left = seconds_left % 60;
    send_to_char(ch, "Your golem cannot be recalled yet. Cooldown: %d minute%s %d second%s.\r\n",
                 minutes_left, (minutes_left == 1) ? "" : "s",
                 seconds_left, (seconds_left == 1) ? "" : "s");
    return false;
  }
  
  return true;
}

// Recall a destroyed golem for free
void recall_golem(struct char_data *ch)
{
  int golem_vnum = NOBODY;
  struct char_data *golem = NULL;
  const char *golem_type_names[] = {"", "wood", "stone", "iron"};
  const char *golem_size_names[] = {"small", "medium", "large", "huge"};
  
  // Check if they have a stored golem type
  if (ch->char_specials.saved.golem_stored_type == GOLEM_TYPE_NONE)
  {
    send_to_char(ch, "You don't have a golem to recall.\r\n");
    return;
  }
  
  // Check if they already have a golem
  if (has_golem_follower(ch))
  {
    send_to_char(ch, "You already have an active golem.\r\n");
    return;
  }
  
  // Check cooldown
  if (!can_recall_golem(ch))
    return;
  
  // Get the golem VNUM
  golem_vnum = get_golem_vnum(ch->char_specials.saved.golem_stored_type, ch->char_specials.saved.golem_stored_size);
  
  if (golem_vnum == NOBODY)
  {
    send_to_char(ch, "\tRError:\tn Invalid stored golem type!\r\n");
    return;
  }
  
  // Check if we can add this follower
  if (!can_add_follower(ch, golem_vnum))
  {
    send_to_char(ch, "\tRYou cannot control another follower right now!\tn\r\n");
    return;
  }
  
  // Create the golem
  golem = read_mobile(golem_vnum, VIRTUAL);
  
  if (!golem)
  {
    send_to_char(ch, "\tRError:\tn Failed to recall golem! Contact an administrator.\r\n");
    return;
  }
  
  char_to_room(golem, IN_ROOM(ch));
  
  IS_CARRYING_W(golem) = 0;
  IS_CARRYING_N(golem) = 0;
  SET_BIT_AR(AFF_FLAGS(golem), AFF_CHARM);
  SET_BIT_AR(MOB_FLAGS(golem), MOB_GOLEM);
  GET_REAL_RACE(golem) = RACE_TYPE_CONSTRUCT;
  
  send_to_char(ch, "\tGSuccess!\tn You recall your %s %s golem!\r\n",
               golem_size_names[ch->char_specials.saved.golem_stored_size],
               golem_type_names[ch->char_specials.saved.golem_stored_type]);
  act("$N materializes before $n in a flash of arcane energy!", FALSE, ch, 0, golem, TO_ROOM);
  
  load_mtrigger(golem);
  add_follower(golem, ch);

  sync_artificer_construct_command_bonuses(golem);

  if (ch->char_specials.saved.golem_stored_hp > 0)
  {
    GET_HIT(golem) = MIN(GET_MAX_HIT(golem), MAX(1, ch->char_specials.saved.golem_stored_hp));
  }
  
  // Auto-join group if creator is leading a group
  if (!GROUP(golem) && GROUP(ch) && GROUP_LEADER(GROUP(ch)) == ch)
    join_group(golem, GROUP(ch));
  
  // Set cooldown
  ch->char_specials.saved.golem_recall_cooldown =
      time(NULL) + (ch->char_specials.saved.golem_stored_hp > 0
                        ? GOLEM_BATTLEFIELD_RETRIEVAL_COOLDOWN
                        : GOLEM_RECALL_COOLDOWN);
  ch->char_specials.saved.golem_stored_hp = 0;
}

// Shutdown an active golem to allow crafting a different type
void shutdown_golem(struct char_data *ch, struct char_data *golem)
{
  const char *golem_type_names[] = {"", "wood", "stone", "iron"};
  const char *golem_size_names[] = {"small", "medium", "large", "huge"};
  int golem_vnum, golem_type, golem_size;
  int material_types[3] = {0}, material_amounts[3] = {0};
  int mote_types[NUM_CRAFT_MOTES] = {0}, mote_amounts[NUM_CRAFT_MOTES] = {0};
  int num_mats = 0, num_motes = 0;
  int i = 0;
  
  if (!golem || !IS_NPC(golem) || !MOB_FLAGGED(golem, MOB_GOLEM))
  {
    send_to_char(ch, "That's not a golem.\r\n");
    return;
  }
  
  if (golem->master != ch)
  {
    send_to_char(ch, "That golem doesn't belong to you.\r\n");
    return;
  }
  
  golem_vnum = GET_MOB_VNUM(golem);
  golem_type = get_golem_type_from_vnum(golem_vnum);
  golem_size = get_golem_size_from_vnum(golem_vnum);
  
  if (golem_type < 0 || golem_size < 0)
  {
    send_to_char(ch, "That golem is invalid!\r\n");
    return;
  }
  
  // Get materials and motes used to craft this golem
  num_mats = get_golem_material_requirements(golem_type, golem_size, material_types, material_amounts);
  num_motes = get_golem_mote_requirements(golem_type, golem_size, mote_types, mote_amounts);
  
  send_to_char(ch, "You shut down your %s %s golem.\r\n",
               golem_size_names[golem_size], golem_type_names[golem_type]);
  act("$n's golem powers down and vanishes in a shimmer of magic.", FALSE, ch, 0, 0, TO_ROOM);
  
  // Recover half of the materials used
  send_to_char(ch, "\tGYou recover the following materials:\tn\r\n");
  for (i = 0; i < num_mats; i++)
  {
    if (material_types[i] > 0 && material_amounts[i] > 0)
    {
      int recovered_amount = material_amounts[i] / 2;
      if (golem_type == GOLEM_TYPE_WOOD && i == 0)
      {
        // For wood golems, return the selected wood type
        int selected_wood_type = GET_CRAFT(ch).golem_materials[i][0];
        GET_CRAFT_MAT(ch, selected_wood_type) += recovered_amount;
      }
      else
      {
        GET_CRAFT_MAT(ch, material_types[i]) += recovered_amount;
      }
      send_to_char(ch, "  %d units of material\r\n", recovered_amount);
    }
  }
  
  // Recover half of the motes used
  send_to_char(ch, "\tGYou recover the following motes:\tn\r\n");
  for (i = 0; i < num_motes; i++)
  {
    if (mote_types[i] > 0 && mote_amounts[i] > 0)
    {
      int recovered_amount = mote_amounts[i] / 2;
      GET_CRAFT_MOTES(ch, mote_types[i]) += recovered_amount;
      send_to_char(ch, "  %d motes\r\n", recovered_amount);
    }
  }
  
  // Clear stored golem so they can build a new type
  ch->char_specials.saved.golem_stored_type = GOLEM_TYPE_NONE;
  ch->char_specials.saved.golem_stored_size = GOLEM_SIZE_SMALL;
  ch->char_specials.saved.golem_stored_hp = 0;
  ch->char_specials.saved.golem_recall_cooldown = 0;
  
  extract_char(golem);
}

// Upgrade an active golem to a larger size of the same type
void upgrade_golem(struct char_data *ch, struct char_data *golem, int new_size)
{
  int golem_vnum, golem_type, current_size;
  int material_types_current[3] = {0}, material_amounts_current[3] = {0};
  int material_types_new[3] = {0}, material_amounts_new[3] = {0};
  int mote_types_current[NUM_CRAFT_MOTES] = {0}, mote_amounts_current[NUM_CRAFT_MOTES] = {0};
  int mote_types_new[NUM_CRAFT_MOTES] = {0}, mote_amounts_new[NUM_CRAFT_MOTES] = {0};
  int num_mats_current = 0, num_mats_new = 0;
  int num_motes_current = 0, num_motes_new = 0;
  int i = 0;
  const char *golem_type_names[] = {"", "wood", "stone", "iron"};
  const char *golem_size_names[] = {"small", "medium", "large", "huge"};
  
  if (!golem || !IS_NPC(golem) || !MOB_FLAGGED(golem, MOB_GOLEM))
  {
    send_to_char(ch, "That's not a golem.\r\n");
    return;
  }
  
  if (golem->master != ch)
  {
    send_to_char(ch, "That golem doesn't belong to you.\r\n");
    return;
  }
  
  golem_vnum = GET_MOB_VNUM(golem);
  golem_type = get_golem_type_from_vnum(golem_vnum);
  current_size = get_golem_size_from_vnum(golem_vnum);
  
  if (golem_type < 0 || current_size < 0)
  {
    send_to_char(ch, "That golem is invalid!\r\n");
    return;
  }
  
  if (new_size <= current_size)
  {
    send_to_char(ch, "The new size must be larger than the current size.\r\n");
    return;
  }
  
  if (new_size >= NUM_GOLEM_SIZES)
  {
    send_to_char(ch, "That size is too large.\r\n");
    return;
  }
  
  /* Get materials and motes for current and new size */
  num_mats_current = get_golem_material_requirements(golem_type, current_size, material_types_current, material_amounts_current);
  num_mats_new = get_golem_material_requirements(golem_type, new_size, material_types_new, material_amounts_new);
  num_motes_current = get_golem_mote_requirements(golem_type, current_size, mote_types_current, mote_amounts_current);
  num_motes_new = get_golem_mote_requirements(golem_type, new_size, mote_types_new, mote_amounts_new);
  
  /* Calculate difference (what's needed for upgrade) */
  for (i = 0; i < num_mats_new; i++)
  {
    if (material_types_new[i] > 0 && material_amounts_new[i] > 0)
    {
      int diff = material_amounts_new[i] - (i < num_mats_current ? material_amounts_current[i] : 0);
      if (diff > 0)
      {
        /* Check if player has enough materials */
        if (GET_CRAFT_MAT(ch, material_types_new[i]) < diff)
        {
          send_to_char(ch, "You don't have enough materials to upgrade your golem.\r\n");
          return;
        }
      }
    }
  }
  
  for (i = 0; i < num_motes_new; i++)
  {
    if (mote_types_new[i] > 0 && mote_amounts_new[i] > 0)
    {
      int diff = mote_amounts_new[i] - (i < num_motes_current ? mote_amounts_current[i] : 0);
      if (diff > 0)
      {
        /* Check if player has enough motes */
        if (GET_CRAFT_MOTES(ch, mote_types_new[i]) < diff)
        {
          send_to_char(ch, "You don't have enough motes to upgrade your golem.\r\n");
          return;
        }
      }
    }
  }
  
  /* Consume the difference in materials */
  for (i = 0; i < num_mats_new; i++)
  {
    if (material_types_new[i] > 0 && material_amounts_new[i] > 0)
    {
      int diff = material_amounts_new[i] - (i < num_mats_current ? material_amounts_current[i] : 0);
      if (diff > 0)
        GET_CRAFT_MAT(ch, material_types_new[i]) -= diff;
    }
  }
  
  /* Consume the difference in motes */
  for (i = 0; i < num_motes_new; i++)
  {
    if (mote_types_new[i] > 0 && mote_amounts_new[i] > 0)
    {
      int diff = mote_amounts_new[i] - (i < num_motes_current ? mote_amounts_current[i] : 0);
      if (diff > 0)
        GET_CRAFT_MOTES(ch, mote_types_new[i]) -= diff;
    }
  }
  
  send_to_char(ch, "\tGSuccess!\tn You upgrade your %s %s golem to %s size!\r\n",
               golem_type_names[golem_type], golem_size_names[current_size],
               golem_size_names[new_size]);
  act("$N begins to glow brightly as $n enhances the golem's form, growing larger!", 
      FALSE, ch, 0, golem, TO_ROOM);

  struct follow_type *k;
  
  // Find the golem follower
  for (k = ch->followers; k; k = k->next)
  {
    if (IS_PET(k->follower) && MOB_FLAGGED(k->follower, MOB_GOLEM))
    {
      golem = k->follower;
      break;
    }
  }

  // Clear stored golem so they can build a new type
  ch->char_specials.saved.golem_stored_type = golem_type;
  ch->char_specials.saved.golem_stored_size = new_size;
  
  extract_char(golem);
  
  golem_vnum = get_golem_vnum(golem_type, new_size);
  golem = read_mobile(golem_vnum, VIRTUAL);
  char_to_room(golem, IN_ROOM(ch));
  
  IS_CARRYING_W(golem) = 0;
  IS_CARRYING_N(golem) = 0;
  SET_BIT_AR(AFF_FLAGS(golem), AFF_CHARM);
  SET_BIT_AR(MOB_FLAGS(golem), MOB_GOLEM);
  GET_REAL_RACE(golem) = RACE_TYPE_CONSTRUCT;

  load_mtrigger(golem);
  add_follower(golem, ch);

  sync_artificer_construct_command_bonuses(golem);

  if (!GROUP(golem) && GROUP(ch) && GROUP_LEADER(GROUP(ch)) == ch)
    join_group(golem, GROUP(ch));
  
  /* Update the stored golem size */
  ch->char_specials.saved.golem_stored_size = new_size;
  ch->char_specials.saved.golem_stored_hp = 0;
}

#define MAX_GOLEM_TRANSFER_REQUESTS 20
#define GOLEM_TRANSFER_CODE_MIN 1000
#define GOLEM_TRANSFER_CODE_MAX 9999
#define GOLEM_TRANSFER_TIMEOUT 60

struct golem_transfer_request
{
  bool in_use;
  long sender_id;
  long target_id;
  int golem_vnum;
  int code;
  time_t created;
  bool sender_confirmed;
  bool target_confirmed;
  char sender_name[MAX_INPUT_LENGTH];
  char target_name[MAX_INPUT_LENGTH];
};

static struct golem_transfer_request golem_transfer_requests[MAX_GOLEM_TRANSFER_REQUESTS];

static struct golem_transfer_request *find_golem_transfer_for_id(long idnum)
{
  int i;

  for (i = 0; i < MAX_GOLEM_TRANSFER_REQUESTS; i++)
  {
    if (!golem_transfer_requests[i].in_use)
      continue;
    if (golem_transfer_requests[i].sender_id == idnum ||
        golem_transfer_requests[i].target_id == idnum)
      return &golem_transfer_requests[i];
  }

  return NULL;
}

static struct golem_transfer_request *alloc_golem_transfer_request(void)
{
  int i;

  for (i = 0; i < MAX_GOLEM_TRANSFER_REQUESTS; i++)
  {
    if (!golem_transfer_requests[i].in_use)
    {
      memset(&golem_transfer_requests[i], 0, sizeof(golem_transfer_requests[i]));
      golem_transfer_requests[i].in_use = true;
      return &golem_transfer_requests[i];
    }
  }

  return NULL;
}

static void clear_golem_transfer_request(struct golem_transfer_request *req)
{
  if (req)
    memset(req, 0, sizeof(*req));
}

static bool is_golem_transfer_expired(struct golem_transfer_request *req)
{
  if (!req || !req->in_use)
    return false;
  return (time(NULL) - req->created) > GOLEM_TRANSFER_TIMEOUT;
}

static void notify_golem_transfer_expired(struct golem_transfer_request *req, struct char_data *ch)
{
  struct char_data *sender = NULL;
  struct char_data *target = NULL;

  if (!req)
    return;

  if (*req->sender_name)
    sender = get_player_vis(ch, req->sender_name, NULL, FIND_CHAR_WORLD);
  if (*req->target_name)
    target = get_player_vis(ch, req->target_name, NULL, FIND_CHAR_WORLD);

  if (sender)
    send_to_char(sender, "Your golem transfer to %s has expired.\r\n", req->target_name);
  if (target)
    send_to_char(target, "The golem transfer from %s has expired.\r\n", req->sender_name);
}

static struct char_data *find_pc_in_room_by_id(struct char_data *ch, long idnum)
{
  struct char_data *tch;

  if (!ch || IN_ROOM(ch) == NOWHERE)
    return NULL;

  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
  {
    if (!IS_NPC(tch) && GET_IDNUM(tch) == idnum)
      return tch;
  }

  return NULL;
}

static struct char_data *find_golem_in_room_by_master(room_rnum room, struct char_data *master)
{
  struct char_data *tch;

  if (room == NOWHERE || !master)
    return NULL;

  for (tch = world[room].people; tch; tch = tch->next_in_room)
  {
    if (IS_NPC(tch) && MOB_FLAGGED(tch, MOB_GOLEM) && tch->master == master)
      return tch;
  }

  return NULL;
}

void transfer_golem(struct char_data *ch, struct char_data *golem, struct char_data *target)
{
  int golem_vnum, golem_type, golem_size;
  const char *golem_type_names[] = {"", "wood", "stone", "iron"};
  const char *golem_size_names[] = {"small", "medium", "large", "huge"};

  if (!ch || !golem || !target)
    return;

  if (!MOB_FLAGGED(golem, MOB_GOLEM))
  {
    send_to_char(ch, "That's not a golem!\r\n");
    return;
  }

  if (golem->master != ch)
  {
    send_to_char(ch, "That golem doesn't belong to you!\r\n");
    return;
  }

  if (IS_NPC(target) || target == ch)
  {
    send_to_char(ch, "You can only transfer a golem to another player.\r\n");
    return;
  }

  if (IN_ROOM(golem) != IN_ROOM(ch) || IN_ROOM(target) != IN_ROOM(ch))
  {
    send_to_char(ch, "The golem and the recipient must be in the same room.\r\n");
    return;
  }

  golem_vnum = GET_MOB_VNUM(golem);
  golem_type = get_golem_type_from_vnum(golem_vnum);
  golem_size = get_golem_size_from_vnum(golem_vnum);

  if (golem_type < 0 || golem_size < 0)
  {
    send_to_char(ch, "That golem is invalid!\r\n");
    return;
  }

  switch (golem_type)
  {
  case GOLEM_TYPE_WOOD:
    if (!HAS_FEAT(ch, FEAT_CONSTRUCT_WOOD_GOLEM))
    {
      send_to_char(ch, "You must have the construct wood golem feat to transfer this golem.\r\n");
      return;
    }
    break;
  case GOLEM_TYPE_STONE:
    if (!HAS_FEAT(ch, FEAT_CONSTRUCT_STONE_GOLEM))
    {
      send_to_char(ch, "You must have the construct stone golem feat to transfer this golem.\r\n");
      return;
    }
    break;
  case GOLEM_TYPE_IRON:
    if (!HAS_FEAT(ch, FEAT_CONSTRUCT_IRON_GOLEM))
    {
      send_to_char(ch, "You must have the construct iron golem feat to transfer this golem.\r\n");
      return;
    }
    break;
  }

  if (has_golem_follower(target))
  {
    send_to_char(ch, "That player already has an active golem.\r\n");
    return;
  }

  if (target->char_specials.saved.golem_stored_type != GOLEM_TYPE_NONE)
  {
    send_to_char(ch, "That player already has a stored golem.\r\n");
    return;
  }

  if (!can_add_follower(target, golem_vnum))
  {
    send_to_char(ch, "That player cannot control another follower right now.\r\n");
    return;
  }

  if (GROUP(golem))
    leave_group(golem);
  stop_follower_engine(golem);
  golem->master = NULL;

  add_follower(golem, target);

  if (!GROUP(golem) && GROUP(target) && GROUP_LEADER(GROUP(target)) == target)
    join_group(golem, GROUP(target));

  ch->char_specials.saved.golem_stored_type = GOLEM_TYPE_NONE;
  ch->char_specials.saved.golem_stored_size = GOLEM_SIZE_SMALL;
  ch->char_specials.saved.golem_stored_hp = 0;
  ch->char_specials.saved.golem_recall_cooldown = 0;

  target->char_specials.saved.golem_stored_type = golem_type;
  target->char_specials.saved.golem_stored_size = golem_size;
  target->char_specials.saved.golem_stored_hp = 0;
  target->char_specials.saved.golem_recall_cooldown = 0;

  sync_artificer_construct_command_bonuses(golem);

  send_to_char(ch, "You transfer your %s %s golem to %s.\r\n",
               golem_size_names[golem_size], golem_type_names[golem_type], GET_NAME(target));
  send_to_char(target, "%s transfers a %s %s golem to you.\r\n",
               GET_NAME(ch), golem_size_names[golem_size], golem_type_names[golem_type]);
  act("$n transfers a golem to $N.", TRUE, ch, 0, target, TO_NOTVICT);

  save_char_pets(ch);
  save_char_pets(target);
  save_char(ch, 0);
  save_char(target, 0);
}

void start_golem_transfer(struct char_data *ch, struct char_data *golem, struct char_data *target)
{
  int golem_vnum, golem_type, golem_size;
  const char *golem_type_names[] = {"", "wood", "stone", "iron"};
  const char *golem_size_names[] = {"small", "medium", "large", "huge"};
  struct golem_transfer_request *req;

  if (!ch || !golem || !target)
    return;

  if (!MOB_FLAGGED(golem, MOB_GOLEM))
  {
    send_to_char(ch, "That's not a golem!\r\n");
    return;
  }

  if (golem->master != ch)
  {
    send_to_char(ch, "That golem doesn't belong to you!\r\n");
    return;
  }

  if (IS_NPC(target) || target == ch)
  {
    send_to_char(ch, "You can only transfer a golem to another player.\r\n");
    return;
  }

  if (IN_ROOM(golem) != IN_ROOM(ch) || IN_ROOM(target) != IN_ROOM(ch))
  {
    send_to_char(ch, "The golem and the recipient must be in the same room.\r\n");
    return;
  }

  if (find_golem_transfer_for_id(GET_IDNUM(ch)) || find_golem_transfer_for_id(GET_IDNUM(target)))
  {
    struct golem_transfer_request *existing = NULL;

    existing = find_golem_transfer_for_id(GET_IDNUM(ch));
    if (existing && is_golem_transfer_expired(existing))
    {
      notify_golem_transfer_expired(existing, ch);
      clear_golem_transfer_request(existing);
    }

    existing = find_golem_transfer_for_id(GET_IDNUM(target));
    if (existing && is_golem_transfer_expired(existing))
    {
      notify_golem_transfer_expired(existing, ch);
      clear_golem_transfer_request(existing);
    }

    if (find_golem_transfer_for_id(GET_IDNUM(ch)) || find_golem_transfer_for_id(GET_IDNUM(target)))
    {
      send_to_char(ch, "There is already a pending golem transfer involving one of you.\r\n");
      return;
    }
  }

  golem_vnum = GET_MOB_VNUM(golem);
  golem_type = get_golem_type_from_vnum(golem_vnum);
  golem_size = get_golem_size_from_vnum(golem_vnum);

  if (golem_type < 0 || golem_size < 0)
  {
    send_to_char(ch, "That golem is invalid!\r\n");
    return;
  }

  switch (golem_type)
  {
  case GOLEM_TYPE_WOOD:
    if (!HAS_FEAT(ch, FEAT_CONSTRUCT_WOOD_GOLEM))
    {
      send_to_char(ch, "You must have the construct wood golem feat to transfer this golem.\r\n");
      return;
    }
    break;
  case GOLEM_TYPE_STONE:
    if (!HAS_FEAT(ch, FEAT_CONSTRUCT_STONE_GOLEM))
    {
      send_to_char(ch, "You must have the construct stone golem feat to transfer this golem.\r\n");
      return;
    }
    break;
  case GOLEM_TYPE_IRON:
    if (!HAS_FEAT(ch, FEAT_CONSTRUCT_IRON_GOLEM))
    {
      send_to_char(ch, "You must have the construct iron golem feat to transfer this golem.\r\n");
      return;
    }
    break;
  }

  if (has_golem_follower(target))
  {
    send_to_char(ch, "That player already has an active golem.\r\n");
    return;
  }

  if (target->char_specials.saved.golem_stored_type != GOLEM_TYPE_NONE)
  {
    send_to_char(ch, "That player already has a stored golem.\r\n");
    return;
  }

  if (!can_add_follower(target, golem_vnum))
  {
    send_to_char(ch, "That player cannot control another follower right now.\r\n");
    return;
  }

  req = alloc_golem_transfer_request();
  if (!req)
  {
    send_to_char(ch, "Golem transfers are busy right now. Try again shortly.\r\n");
    return;
  }

  req->sender_id = GET_IDNUM(ch);
  req->target_id = GET_IDNUM(target);
  req->golem_vnum = golem_vnum;
  req->code = rand_number(GOLEM_TRANSFER_CODE_MIN, GOLEM_TRANSFER_CODE_MAX);
  req->created = time(NULL);
  req->sender_confirmed = false;
  req->target_confirmed = false;
  snprintf(req->sender_name, sizeof(req->sender_name), "%s", GET_NAME(ch));
  snprintf(req->target_name, sizeof(req->target_name), "%s", GET_NAME(target));

  send_to_char(ch,
               "Transfer request created. Confirmation code: %d\r\n"
               "Type 'craft golem confirm %d' to confirm your transfer.\r\n",
               req->code, req->code);
  send_to_char(target,
               "%s wants to transfer a %s %s golem to you.\r\n"
               "Ask them for the confirmation code and type 'craft golem confirm <code>' to accept.\r\n",
               GET_NAME(ch), golem_size_names[golem_size], golem_type_names[golem_type]);
}

void confirm_golem_transfer(struct char_data *ch, const char *code_arg)
{
  struct golem_transfer_request *req;
  struct char_data *sender = NULL;
  struct char_data *target = NULL;
  struct char_data *golem = NULL;
  int code = 0;

  if (!ch || !code_arg || !*code_arg || !is_number(code_arg))
  {
    send_to_char(ch, "Usage: craft golem confirm <code>\r\n");
    return;
  }

  code = atoi(code_arg);
  req = find_golem_transfer_for_id(GET_IDNUM(ch));
  if (!req)
  {
    send_to_char(ch, "You have no pending golem transfer.\r\n");
    return;
  }

  if (is_golem_transfer_expired(req))
  {
    notify_golem_transfer_expired(req, ch);
    clear_golem_transfer_request(req);
    return;
  }

  if (req->code != code)
  {
    send_to_char(ch, "That confirmation code is invalid.\r\n");
    return;
  }

  if (GET_IDNUM(ch) == req->sender_id)
  {
    if (req->sender_confirmed)
    {
      send_to_char(ch, "You have already confirmed this transfer.\r\n");
      return;
    }
    req->sender_confirmed = true;
    send_to_char(ch, "You confirm the golem transfer.\r\n");
  }
  else if (GET_IDNUM(ch) == req->target_id)
  {
    if (req->target_confirmed)
    {
      send_to_char(ch, "You have already confirmed this transfer.\r\n");
      return;
    }
    req->target_confirmed = true;
    send_to_char(ch, "You accept the golem transfer.\r\n");
  }
  else
  {
    send_to_char(ch, "You are not part of that golem transfer.\r\n");
    return;
  }

  if (!req->sender_confirmed || !req->target_confirmed)
    return;

  sender = find_pc_in_room_by_id(ch, req->sender_id);
  target = find_pc_in_room_by_id(ch, req->target_id);

  if (!sender || !target)
  {
    send_to_char(ch, "Both players must be in the same room to finalize the transfer.\r\n");
    return;
  }

  golem = find_golem_in_room_by_master(IN_ROOM(ch), sender);
  if (!golem)
  {
    send_to_char(ch, "The golem is not here. Transfer canceled.\r\n");
    clear_golem_transfer_request(req);
    return;
  }

  if (has_golem_follower(target) || target->char_specials.saved.golem_stored_type != GOLEM_TYPE_NONE)
  {
    send_to_char(ch, "The recipient already has a golem. Transfer canceled.\r\n");
    clear_golem_transfer_request(req);
    return;
  }

  transfer_golem(sender, golem, target);
  clear_golem_transfer_request(req);
}

void cancel_golem_transfer(struct char_data *ch)
{
  struct golem_transfer_request *req;
  struct char_data *sender = NULL;
  struct char_data *target = NULL;

  if (!ch)
    return;

  req = find_golem_transfer_for_id(GET_IDNUM(ch));
  if (!req)
  {
    send_to_char(ch, "You have no pending golem transfer.\r\n");
    return;
  }

  sender = get_player_vis(ch, req->sender_name, NULL, FIND_CHAR_WORLD);
  target = get_player_vis(ch, req->target_name, NULL, FIND_CHAR_WORLD);

  if (sender)
    send_to_char(sender, "The golem transfer to %s has been canceled.\r\n", req->target_name);
  if (target)
    send_to_char(target, "The golem transfer from %s has been canceled.\r\n", req->sender_name);

  clear_golem_transfer_request(req);
}

void newcraft_golem(struct char_data *ch, const char *argument)
{
  char arg1[200], arg2[MAX_INPUT_LENGTH];
  const char *directive_name = "none";

  half_chop_c(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

  if (ch->char_specials.saved.golem_directive == GOLEM_DIRECTIVE_GUARD)
    directive_name = "guard";
  else if (ch->char_specials.saved.golem_directive == GOLEM_DIRECTIVE_PRESSURE)
    directive_name = "pressure";
  else if (ch->char_specials.saved.golem_directive == GOLEM_DIRECTIVE_SUPPORT)
    directive_name = "support";

  if (!*arg1)
  {
    send_to_char(ch, "Golem crafting commands:\r\n");
    send_to_char(ch, "  craft golem type (wood|stone|iron) - Set golem type\r\n");
    send_to_char(ch, "  craft golem size (small|medium|large|huge) - Set golem size\r\n");
    send_to_char(ch, "  craft golem show - Display current golem project\r\n");
    send_to_char(ch, "  craft golem reset - Reset golem project\r\n");
    send_to_char(ch, "  craft golem start - Begin construction\r\n");
    send_to_char(ch, "  craft golem recall - Recall your stored golem; with Battlefield Retrieval this also stows an active one\r\n");
    send_to_char(ch, "  craft golem deploy - Alias for recall when redeploying a stored golem\r\n");
    send_to_char(ch, "  craft golem shutdown - Shutdown your golem to build a new type\r\n");
    send_to_char(ch, "  craft golem upgrade (small|medium|large|huge) - Upgrade golem to larger size\r\n");
    if (get_artificer_tactical_directives_rank(ch) > 0)
    {
      send_to_char(ch, "  craft golem guard - Toggle the guard directive (+AC stance)\r\n");
      send_to_char(ch, "  craft golem pressure - Toggle the pressure directive (+hit stance)\r\n");
      send_to_char(ch, "  craft golem stance - Show the current directive\r\n");
    }
    if (has_artificer_arcana_siege_frame(ch))
      send_to_char(ch, "  craft golem slam <target> <direction> - Use your siege-frame knockback slam\r\n");
    if (has_artificer_omni_forge_commander(ch))
      send_to_char(ch, "  craft golem refit <defense|offense|support> - Refit your active construct out of combat\r\n");
    send_to_char(ch, "  craft golem transfer <golem> <player> - Transfer your golem to another player\r\n");
    send_to_char(ch, "  craft golem confirm <code> - Confirm a pending golem transfer\r\n");
    send_to_char(ch, "  craft golem cancel - Cancel a pending golem transfer\r\n");
    return;
  }

  if (is_abbrev(arg1, "type"))
  {
    set_golem_type(ch, arg2);
  }
  else if (is_abbrev(arg1, "size"))
  {
    set_golem_size(ch, arg2);
  }
  else if (is_abbrev(arg1, "show") || is_abbrev(arg1, "display"))
  {
    show_current_golem_craft(ch);
  }
  else if (is_abbrev(arg1, "reset"))
  {
    reset_current_golem_craft(ch);
  }
  else if (is_abbrev(arg1, "start") || is_abbrev(arg1, "begin"))
  {
    if (begin_golem_craft(ch))
    {
      GET_CRAFT(ch).craft_duration = MAX(GET_CRAFT(ch).craft_duration, 1);
    }
  }
  else if (is_abbrev(arg1, "recall") || is_abbrev(arg1, "deploy"))
  {
    struct char_data *golem = get_active_golem_follower(ch);

    if (golem)
    {
      battlefield_retrieve_active_golem(ch, golem);
    }
    else
    {
      recall_golem(ch);
    }
  }
  else if (is_abbrev(arg1, "shutdown"))
  {
    struct char_data *golem = get_active_golem_follower(ch);
    
    if (!golem)
    {
      send_to_char(ch, "You don't have an active golem to shutdown. Try using 'craft golem recall' first, then try shutdown again.\r\n");
      return;
    }
    
    shutdown_golem(ch, golem);
  }
  else if (is_abbrev(arg1, "upgrade"))
  {
    struct char_data *golem = NULL;
    int new_size = -1;
    
    if (!*arg2)
    {
      send_to_char(ch, "Upgrade to which size? (small|medium|large|huge)\r\n");
      return;
    }
    
    if (is_abbrev(arg2, "small"))
      new_size = GOLEM_SIZE_SMALL;
    else if (is_abbrev(arg2, "medium"))
      new_size = GOLEM_SIZE_MEDIUM;
    else if (is_abbrev(arg2, "large"))
      new_size = GOLEM_SIZE_LARGE;
    else if (is_abbrev(arg2, "huge"))
      new_size = GOLEM_SIZE_HUGE;
    else
    {
      send_to_char(ch, "Unknown size. Use: small, medium, large, or huge\r\n");
      return;
    }
    
    /* Find the golem follower */
    golem = get_active_golem_follower(ch);
    
    if (!golem)
    {
      send_to_char(ch, "You don't have an active golem to upgrade.\r\n");
      return;
    }
    
    upgrade_golem(ch, golem, new_size);
  }
  else if (is_abbrev(arg1, "stance") || is_abbrev(arg1, "directive"))
  {
    if (get_artificer_tactical_directives_rank(ch) <= 0)
    {
      send_to_char(ch, "You have not purchased Tactical Directives.\r\n");
      return;
    }

    send_to_char(ch, "Your current golem directive is %s.\r\n", directive_name);
  }
  else if (is_abbrev(arg1, "guard") || is_abbrev(arg1, "pressure"))
  {
    struct char_data *golem = NULL;
    int new_directive = is_abbrev(arg1, "guard") ? GOLEM_DIRECTIVE_GUARD : GOLEM_DIRECTIVE_PRESSURE;

    if (get_artificer_tactical_directives_rank(ch) <= 0)
    {
      send_to_char(ch, "You have not purchased Tactical Directives.\r\n");
      return;
    }

    if (ch->char_specials.saved.golem_directive == new_directive)
    {
      ch->char_specials.saved.golem_directive = GOLEM_DIRECTIVE_NONE;
      send_to_char(ch, "You clear your golem's active directive.\r\n");
    }
    else
    {
      ch->char_specials.saved.golem_directive = new_directive;
      send_to_char(ch, "You set your golem's directive to %s.\r\n",
                   new_directive == GOLEM_DIRECTIVE_GUARD ? "guard" : "pressure");
    }

    golem = get_active_golem_follower(ch);
    if (golem)
    {
      sync_artificer_construct_command_bonuses(golem);
      send_to_char(ch, "%s adjusts to the new directive.\r\n", GET_NAME(golem));
    }
    else
    {
      send_to_char(ch, "The directive will apply when your next golem is active.\r\n");
    }
  }
  else if (is_abbrev(arg1, "slam"))
  {
    char target_arg[MAX_INPUT_LENGTH] = {'\0'};
    char dir_arg[MAX_INPUT_LENGTH] = {'\0'};
    struct char_data *golem = NULL;
    struct char_data *victim = NULL;
    int dir = -1;
    int slam_damage = 0;
    bool free_command = FALSE;
    int i = 0;

    if (!has_artificer_arcana_siege_frame(ch))
    {
      send_to_char(ch, "You have not purchased Arcana Siege Frame.\r\n");
      return;
    }

    two_arguments(arg2, target_arg, sizeof(target_arg), dir_arg, sizeof(dir_arg));
    if (!*target_arg || !*dir_arg)
    {
      send_to_char(ch, "Usage: craft golem slam <target> <direction>\r\n");
      return;
    }

    golem = get_active_golem_follower(ch);
    if (!golem)
    {
      send_to_char(ch, "You do not have an active golem.\r\n");
      return;
    }

    if (!(victim = get_char_room_vis(ch, target_arg, NULL)))
    {
      send_to_char(ch, "There is no one here by that name.\r\n");
      return;
    }

    if (victim == ch || victim == golem)
    {
      send_to_char(ch, "Choose a hostile target for the slam.\r\n");
      return;
    }

    for (i = 0; i < NUM_OF_DIRS; i++)
    {
      if (is_abbrev(dirs[i], dir_arg))
      {
        dir = i;
        break;
      }
    }

    if (dir < 0 || dir >= NUM_OF_DIRS)
    {
      send_to_char(ch, "That is not a valid direction.\r\n");
      return;
    }

    free_command = ch->player_specials->saved.omni_forge_free_command &&
                   (FIGHTING(ch) || FIGHTING(golem));
    if (!free_command && ch->player_specials->saved.siege_frame_slam_cooldown > time(0))
    {
      int seconds_left = (int)(ch->player_specials->saved.siege_frame_slam_cooldown - time(0));
      send_to_char(ch, "Your siege-frame slam is recharging for %d more second%s.\r\n",
                   seconds_left, seconds_left == 1 ? "" : "s");
      return;
    }

    if (attack_roll(golem, victim, ATTACK_TYPE_PRIMARY, FALSE, 1) <= 0)
    {
      act("$N braces and your golem's siege-frame slam glances away harmlessly.", FALSE, ch, 0,
          victim, TO_CHAR);
      act("$n's golem slams into you, but you hold your ground.", FALSE, ch, 0, victim, TO_VICT);
      act("$n's golem slams into $N, but the impact fails to dislodge $M.", FALSE, ch, 0, victim,
          TO_NOTVICT);
      ch->player_specials->saved.siege_frame_slam_cooldown =
          time(0) + GOLEM_SIEGE_FRAME_SLAM_COOLDOWN;
      if (free_command)
        ch->player_specials->saved.omni_forge_free_command = FALSE;
      return;
    }

    slam_damage = dice(MAX(2, GET_LEVEL(golem) / 10), 6) + MAX(0, GET_STR_BONUS(golem));
    act("You command $N to release a siege-frame slam!", FALSE, ch, 0, golem, TO_CHAR);
    act("$n's golem crashes forward in a wave of force!", FALSE, ch, 0, 0, TO_ROOM);
    damage(golem, victim, slam_damage, TYPE_UNDEFINED, DAM_FORCE, FALSE);

    if (victim && GET_POS(victim) > POS_DEAD && GET_SIZE(victim) < GET_SIZE(golem) &&
        !MOB_FLAGGED(victim, MOB_NOBASH) && GET_PUSHED_TIMER(victim) <= 0 &&
        push_attempt(golem, victim, TRUE))
    {
      GET_PUSHED_TIMER(victim) = 10;
      perform_move_full(victim, dir, FALSE, FALSE);
    }
    else if (victim && GET_POS(victim) > POS_DEAD)
    {
      perform_knockdown(golem, victim, SKILL_BASH, FALSE, TRUE);
    }

    ch->player_specials->saved.siege_frame_slam_cooldown =
        time(0) + GOLEM_SIEGE_FRAME_SLAM_COOLDOWN;
    if (free_command)
      ch->player_specials->saved.omni_forge_free_command = FALSE;
  }
  else if (is_abbrev(arg1, "refit"))
  {
    struct char_data *golem = NULL;
    int new_directive = GOLEM_DIRECTIVE_NONE;

    if (!has_artificer_omni_forge_commander(ch))
    {
      send_to_char(ch, "You have not purchased Omni-Forge Commander.\r\n");
      return;
    }

    golem = get_active_golem_follower(ch);
    if (!golem)
    {
      send_to_char(ch, "You need an active golem to refit.\r\n");
      return;
    }

    if (FIGHTING(ch) || FIGHTING(golem))
    {
      send_to_char(ch, "Omni-Forge refits can only be performed out of combat.\r\n");
      return;
    }

    if (ch->player_specials->saved.omni_forge_commander_cooldown > time(0))
    {
      int seconds_left = (int)(ch->player_specials->saved.omni_forge_commander_cooldown - time(0));
      int minutes_left = seconds_left / 60;
      seconds_left %= 60;
      send_to_char(ch, "Omni-Forge Commander is recharging for %d minute%s %d second%s.\r\n",
                   minutes_left, minutes_left == 1 ? "" : "s", seconds_left,
                   seconds_left == 1 ? "" : "s");
      return;
    }

    if (is_abbrev(arg2, "defense") || is_abbrev(arg2, "guard"))
      new_directive = GOLEM_DIRECTIVE_GUARD;
    else if (is_abbrev(arg2, "offense") || is_abbrev(arg2, "pressure"))
      new_directive = GOLEM_DIRECTIVE_PRESSURE;
    else if (is_abbrev(arg2, "support"))
      new_directive = GOLEM_DIRECTIVE_SUPPORT;
    else
    {
      send_to_char(ch, "Usage: craft golem refit <defense|offense|support>\r\n");
      return;
    }

    ch->char_specials.saved.golem_directive = new_directive;
    ch->player_specials->saved.omni_forge_commander_cooldown =
        time(0) + GOLEM_OMNI_FORGE_COMMANDER_COOLDOWN;
    ch->player_specials->saved.omni_forge_free_command = TRUE;
    sync_artificer_construct_command_bonuses(golem);

    send_to_char(ch,
                 "You refit your golem into %s mode. The next eligible command in combat will be free.\r\n",
                 new_directive == GOLEM_DIRECTIVE_GUARD ? "defense" :
                 new_directive == GOLEM_DIRECTIVE_PRESSURE ? "offense" : "support");
    act("$n rapidly reconfigures $s golem with a flurry of omni-forge adjustments.", FALSE, ch,
        0, 0, TO_ROOM);
  }
  else if (is_abbrev(arg1, "transfer"))
  {
    char golem_arg[MAX_INPUT_LENGTH] = {'\0'};
    char target_arg[MAX_INPUT_LENGTH] = {'\0'};
    struct char_data *golem = NULL;
    struct char_data *target = NULL;

    two_arguments(arg2, golem_arg, sizeof(golem_arg), target_arg, sizeof(target_arg));

    if (!*golem_arg || !*target_arg)
    {
      send_to_char(ch, "Usage: craft golem transfer <golem> <player>\r\n");
      return;
    }

    if (!(golem = get_char_vis(ch, golem_arg, NULL, FIND_CHAR_ROOM)))
    {
      send_to_char(ch, "You don't see that golem here.\r\n");
      return;
    }

    if (!(target = get_char_vis(ch, target_arg, NULL, FIND_CHAR_ROOM)))
    {
      send_to_char(ch, "You don't see that player here.\r\n");
      return;
    }

    start_golem_transfer(ch, golem, target);
  }
  else if (is_abbrev(arg1, "confirm"))
  {
    confirm_golem_transfer(ch, arg2);
  }
  else if (is_abbrev(arg1, "cancel"))
  {
    cancel_golem_transfer(ch);
  }
  else
  {
    send_to_char(ch, "Unknown golem subcommand. Use 'craft golem' for help.\r\n");
  }
}

// Todo:
// greygem shards - extract motes, get feats out, store feat motes separately
// restring, reforge, other old craft commands
// ability to trade materials and motes with other players and shops
// ability to augment existing items.
// crafting feat system
// show craft object level and dc in 'show'
// supply order system
// add effects for new materials, not craft materials, but object materials (ie dragonmetal)

/**
 * Calculate repair cost in materials for a golem
 * Returns amount of primary material needed per 10% of missing HP
 */
int get_golem_repair_material_cost(int golem_type, int golem_size)
{
  int material_types[3] = {0}, material_amounts[3] = {0};
  int num_mats =
      get_golem_material_requirements(golem_type, golem_size, material_types, material_amounts);

  if (num_mats > 0 && material_amounts[0] > 0)
  {
    /* Reduced repair cost: 0.3125% of the primary material requirement per 10% repair. */
    return material_amounts[0] / 320;
  }
  return 0;
}

/**
 * Calculate repair DC for a golem
 */
int get_golem_repair_dc(int golem_type, int golem_size)
{
  /* Same as construction DC */
  return get_golem_base_dc(golem_type, golem_size);
}

/**
 * Get the primary material type needed for golem repair
 * For wood golems, returns ASH_WOOD as a placeholder (any wood type will be accepted)
 */
int get_golem_repair_material_type(int golem_type)
{
  switch (golem_type)
  {
  case GOLEM_TYPE_WOOD:
    return CRAFT_MAT_ASH_WOOD; /* Placeholder - any wood type accepted */
  case GOLEM_TYPE_STONE:
    return CRAFT_MAT_STONE;
  case GOLEM_TYPE_IRON:
    return CRAFT_MAT_IRON;
  default:
    return -1;
  }
}

/* Pet saves preserve the prototype, but not flags assigned during crafting. */
void restore_crafted_golem_identity(struct char_data *golem)
{
  if (!golem || !IS_NPC(golem))
    return;

  if (get_golem_type_from_vnum(GET_MOB_VNUM(golem)) < 0 ||
      get_golem_size_from_vnum(GET_MOB_VNUM(golem)) < 0)
    return;

  SET_BIT_AR(MOB_FLAGS(golem), MOB_GOLEM);
  GET_REAL_RACE(golem) = RACE_TYPE_CONSTRUCT;
}

/**
 * Check if a player can repair a golem
 * Returns true if player has enough materials and meets requirements
 */
bool can_repair_golem(struct char_data *ch, struct char_data *golem, int *material_needed,
                      int *material_type)
{
  int golem_vnum, golem_type, golem_size;
  int missing_hp, repair_percent;

  restore_crafted_golem_identity(golem);

  if (!ch || !golem || !IS_NPC(golem) || !MOB_FLAGGED(golem, MOB_GOLEM))
    return false;

  /* Must be out of combat */
  if (FIGHTING(ch) || FIGHTING(golem))
  {
    send_to_char(ch, "You cannot repair a golem while in combat!\r\n");
    return false;
  }

  /* Golem must be alive and in same room */
  if (GET_POS(golem) <= POS_DEAD || IN_ROOM(golem) != IN_ROOM(ch))
  {
    send_to_char(ch, "Your golem is not in a repairable state.\r\n");
    return false;
  }

  /* Must be master */
  if (golem->master != ch)
  {
    send_to_char(ch, "That golem doesn't belong to you!\r\n");
    return false;
  }

  golem_vnum = GET_MOB_VNUM(golem);
  golem_type = get_golem_type_from_vnum(golem_vnum);
  golem_size = get_golem_size_from_vnum(golem_vnum);

  if (golem_type < 0 || golem_size < 0)
  {
    send_to_char(ch, "That golem is invalid!\r\n");
    return false;
  }

  /* Check if golem needs repair */
  missing_hp = GET_MAX_HIT(golem) - GET_HIT(golem);
  if (missing_hp <= 0)
  {
    send_to_char(ch, "Your golem is at full health.\r\n");
    return false;
  }

  /* Calculate repair percentage and reduced material cost. */
  repair_percent = (missing_hp * 100) / GET_MAX_HIT(golem);
  *material_needed = MAX(1, (repair_percent + 49) / 50);
  *material_type = get_golem_repair_material_type(golem_type);

  /* Check if player has enough materials */
  if (golem_type == GOLEM_TYPE_WOOD)
  {
    /* For wood golems, must have a single wood type with enough units */
    int mat;
    int found_wood_type = -1;

    for (mat = 1; mat < NUM_CRAFT_MATS; mat++)
    {
      if (craft_group_by_material(mat) == CRAFT_GROUP_WOOD)
      {
        if (GET_CRAFT_MAT(ch, mat) >= *material_needed)
        {
          found_wood_type = mat;
          *material_type = mat; // Store the selected wood type
          break;                // Lowest grade wood type will be used first
        }
      }
    }

    if (found_wood_type == -1)
    {
      send_to_char(ch,
                   "You don't have a single wood type with enough materials to repair your golem. "
                   "Need: %d units of one wood type.\r\n",
                   *material_needed);
      return false;
    }
  }
  else
  {
    /* For non-wood golems, check specific material type */
    if (GET_CRAFT_MAT(ch, *material_type) < *material_needed)
    {
      send_to_char(
          ch, "You don't have enough materials to repair your golem. Need: %d %s, Have: %d\r\n",
          *material_needed, crafting_materials[*material_type], GET_CRAFT_MAT(ch, *material_type));
      return false;
    }
  }

  return true;
}

int material_type_to_crafting_skill(int material)
{
  switch (material)
  {
  case MATERIAL_COTTON:
  case MATERIAL_PAPER:
  case MATERIAL_SATIN:
  case MATERIAL_SILK:
  case MATERIAL_BURLAP:
  case MATERIAL_VELVET:
  case MATERIAL_WOOL:
  case MATERIAL_HEMP:
  case MATERIAL_LINEN:
  case MATERIAL_ZINC:
  case MATERIAL_FLAX:
    return ABILITY_CRAFT_TAILORING;

  case MATERIAL_LEATHER:
  case MATERIAL_DRAGONHIDE:
    return ABILITY_CRAFT_LEATHERWORKING;

  case MATERIAL_GLASS:
  case MATERIAL_CRYSTAL:
  case MATERIAL_CERAMIC:
  case MATERIAL_OBSIDIAN:
  case MATERIAL_ONYX:
  case MATERIAL_IVORY:
  case MATERIAL_RUBY:
  case MATERIAL_SAPPHIRE:
  case MATERIAL_EMERALD:
  case MATERIAL_GEMSTONE:
  case MATERIAL_GRANITE:
  case MATERIAL_STONE:
  case MATERIAL_DIAMOND:
  case MATERIAL_SEA_IVORY:
    return ABILITY_CRAFT_JEWELCRAFTING;

  case MATERIAL_GOLD:
  case MATERIAL_COPPER:
  case MATERIAL_PLATINUM:
  case MATERIAL_BRASS:
  case MATERIAL_PEWTER:
  case MATERIAL_SILVER:
    return ABILITY_CRAFT_METALWORKING;

  case MATERIAL_ORGANIC:
  case MATERIAL_BONE:
  case MATERIAL_ETHER:
  case MATERIAL_ENERGY:
  case MATERIAL_EARTH:
    return ABILITY_CRAFT_ALCHEMY;

  case MATERIAL_STEEL:
  case MATERIAL_ADAMANTINE:
  case MATERIAL_MITHRIL:
  case MATERIAL_IRON:
  case MATERIAL_BRONZE:
  case MATERIAL_ALCHEMAL_SILVER:
  case MATERIAL_COLD_IRON:
  case MATERIAL_DRAGONSCALE:
  case MATERIAL_DRAGONBONE:
  case MATERIAL_TIN:
  case MATERIAL_COAL:
  case MATERIAL_DRAGONMETAL:
    return ABILITY_CRAFT_ARMORSMITHING;

  case MATERIAL_WOOD:
  case MATERIAL_DARKWOOD:
  case MATERIAL_ASH:
  case MATERIAL_MAPLE:
  case MATERIAL_MAHAGONY:
  case MATERIAL_VALENWOOD:
  case MATERIAL_IRONWOOD:
    return ABILITY_CRAFT_WOODWORKING;
  }
  return 0;
}

/* Helper: case-insensitive substring replace (replace all occurrences).
 * Returns newly allocated string; caller should free original before assigning. */
static char *replace_substring_ci(const char *src, const char *find, const char *repl)
{
  size_t src_len, find_len, repl_len;
  size_t count = 0, out_len, o = 0, i;

  if (!src)
    return NULL;
  if (!find || !*find)
    return strdup(src);

  src_len = strlen(src);
  find_len = strlen(find);
  repl_len = (repl) ? strlen(repl) : 0;
  if (find_len == 0)
    return strdup(src);

  /* Count matches to size the output */
  for (i = 0; i + find_len <= src_len;)
  {
    if (strncasecmp(src + i, find, find_len) == 0)
    {
      count++;
      i += find_len;
    }
    else
    {
      i++;
    }
  }
  if (count == 0)
    return strdup(src);

  out_len = src_len + count * (repl_len - find_len);
  {
    char *out = (char *)malloc(out_len + 1);
    if (!out)
      return strdup(src);

    /* Build output */
    for (i = 0; i < src_len;)
    {
      if (i + find_len <= src_len && strncasecmp(src + i, find, find_len) == 0)
      {
        if (repl_len)
        {
          memcpy(out + o, repl, repl_len);
          o += repl_len;
        }
        i += find_len;
      }
      else
      {
        out[o++] = src[i++];
      }
    }
    out[o] = '\0';
    return out;
  }
}

/* Manual wrapper without profiling to avoid crash */
static void impl_do_reforge_new_(struct char_data *ch, char *argument, int cmd, int subcmd);
void do_reforge_new(struct char_data *ch, const char *argument, int cmd, int subcmd)
{
  if (!argument)
  {
    impl_do_reforge_new_(ch, NULL, cmd, subcmd);
  }
  else
  {
    char arg_buf[MAX_INPUT_LENGTH];
    strlcpy(arg_buf, argument, sizeof(arg_buf));
    impl_do_reforge_new_(ch, arg_buf, cmd, subcmd);
  }
}
static void impl_do_reforge_new_(struct char_data *ch, char *argument,
                                 int cmd __attribute__((unused)),
                                 int subcmd __attribute__((unused)))
{
  struct obj_data *obj = NULL;
  struct obj_data *i = NULL;
  char item_arg[MAX_INPUT_LENGTH];
  char target_arg[MAX_INPUT_LENGTH];
  int material, skill_required;
  int fast_craft_bonus;
  int cost, orig_cost, enhancement;
  char buf[1024]; /* Buffer for room message */
  int weapon_index = 0;
  int armor_index = 0;

  if (CONFIG_CRAFTING_SYSTEM != CRAFTING_SYSTEM_MOTES)
  {
    send_to_char(ch, "Sorry, but you cannot do that here!\r\n");
    return;
  }

  fast_craft_bonus = GET_SKILL(ch, SKILL_FAST_CRAFTER) / 33;

  half_chop(argument, item_arg, target_arg);

  if (!*item_arg || !*target_arg)
  {
    send_to_char(ch, "Usage: reforge <item name> <reforge into>\r\n");
    return;
  }

  /* Search inventory for matching reforgeable item */
  for (i = ch->carrying; i; i = i->next_content)
  {
    if (isname(item_arg, i->name) && OBJ_FLAGGED(i, ITEM_REFORGEABLE))
    {
      obj = i;
      break;
    }
  }

  if (!obj)
  {
    send_to_char(ch, "You don't have an item by that description that can be reforged.\r\n"
                     "An object must be flagged as reforgeable to use this command on it.\r\n");
    return;
  }

  /* Check item type - can only reforge weapons and armor */
  if (GET_OBJ_TYPE(obj) != ITEM_ARMOR && GET_OBJ_TYPE(obj) != ITEM_WEAPON)
  {
    send_to_char(ch, "You can only reforge armor, shields and weapons.\r\n");
    return;
  }

  /* Determine material and get required crafting skill */
  material = GET_OBJ_MATERIAL(obj);
  skill_required = material_type_to_crafting_skill(material);

  /* Check if required crafting station is present */
  if (!has_crafting_station_in_room(ch, skill_required))
  {
    send_to_char(ch, "You need %s to reforge this item.\r\n",
                 get_crafting_station_name(skill_required));
    return;
  }

  /* Store original values */
  orig_cost = GET_OBJ_COST(obj);
  enhancement = GET_OBJ_VAL(obj, 4);

  /* Determine what to reforge it into */
  switch (GET_OBJ_TYPE(obj))
  {
  case ITEM_WEAPON:
    /* Search for matching weapon type */
    for (weapon_index = 0; weapon_index < NUM_WEAPON_TYPES; weapon_index++)
    {
      if (is_abbrev(weapon_list[weapon_index].name, target_arg))
        break;
    }
    if (weapon_index >= NUM_WEAPON_TYPES)
    {
      send_to_char(ch, "That is not a valid weapon type. Type weaponlist for options.\r\n");
      return;
    }
    if (weapon_index == GET_OBJ_VAL(obj, 0))
    {
      send_to_char(ch, "The item is already %s %s.\r\n", AN(weapon_list[weapon_index].name),
                   weapon_list[weapon_index].name);
      return;
    }
    break;

  case ITEM_ARMOR:
    if (IS_SHIELD(GET_OBJ_VAL(obj, 1)))
    {
      /* Reforging a shield */
      for (armor_index = 0; armor_index < NUM_SPEC_ARMOR_TYPES; armor_index++)
      {
        if (!IS_SHIELD(armor_index))
          continue;
        if (is_abbrev(armor_list[armor_index].name, target_arg))
          break;
      }
      if (armor_index >= NUM_SPEC_ARMOR_TYPES)
      {
        send_to_char(ch, "That is not a valid shield type. Type armorlistfull for options.\r\n");
        return;
      }
      if (armor_index == GET_OBJ_VAL(obj, 1))
      {
        send_to_char(ch, "The item is already %s %s.\r\n", AN(armor_list[armor_index].name),
                     armor_list[armor_index].name);
        return;
      }
    }
    else
    {
      /* Reforging non-shield armor - must match wear slot */
      for (armor_index = 0; armor_index < NUM_SPEC_ARMOR_TYPES; armor_index++)
      {
        if (IS_SHIELD(armor_index))
          continue;

        /* Check if wear slot matches */
        if (CAN_WEAR(obj, ITEM_WEAR_HEAD) && armor_list[armor_index].wear != ITEM_WEAR_HEAD)
          continue;
        else if (CAN_WEAR(obj, ITEM_WEAR_BODY) && armor_list[armor_index].wear != ITEM_WEAR_BODY)
          continue;
        else if (CAN_WEAR(obj, ITEM_WEAR_ARMS) && armor_list[armor_index].wear != ITEM_WEAR_ARMS)
          continue;
        else if (CAN_WEAR(obj, ITEM_WEAR_LEGS) && armor_list[armor_index].wear != ITEM_WEAR_LEGS)
          continue;

        if (is_abbrev(armor_list[armor_index].name, target_arg))
          break;
      }
      if (armor_index >= NUM_SPEC_ARMOR_TYPES)
      {
        send_to_char(
            ch,
            "That is not a valid armor type for this slot. Type armorlistfull for options.\r\n");
        return;
      }
      if (armor_index == GET_OBJ_VAL(obj, 1))
      {
        send_to_char(ch, "The item is already %s %s.\r\n", AN(armor_list[armor_index].name),
                     armor_list[armor_index].name);
        return;
      }
    }
    break;

  default:
    send_to_char(ch, "You can only reforge armor, shields and weapons.\r\n");
    return;
  }

  /* Calculate cost (half the object's value) */
  cost = orig_cost / 2;

  if (GET_GOLD(ch) < cost)
  {
    send_to_char(ch, "You need %d coins on hand for supplies to reforge this item.\r\n", cost);
    return;
  }

  /* Update item properties */
  if (GET_OBJ_TYPE(obj) == ITEM_WEAPON)
  {
    set_weapon_object(obj, weapon_index);
  }
  else
  {
    GET_OBJ_VAL(obj, 1) = armor_index;
    set_armor_object(obj, armor_index);
  }

  /* If the object has a restring identifier, update keywords, short/long desc using it */
  if (obj->restring_identifier && *obj->restring_identifier)
  {
    const char *new_type_str = (GET_OBJ_TYPE(obj) == ITEM_WEAPON)
                                   ? weapon_list[GET_OBJ_VAL(obj, 0)].name
                                   : armor_list[GET_OBJ_VAL(obj, 1)].name;
    char *new_restring_id = NULL;

    /* Make a copy of the new type string first to avoid using freed memory */
    new_restring_id = strdup(new_type_str);
    if (!new_restring_id)
    {
      log("SYSERR: do_reforge: Failed to allocate memory for new restring_identifier");
      return;
    }

    /* Replace in keywords */
    if (obj->name)
    {
      char *updated = replace_substring_ci(obj->name, obj->restring_identifier, new_type_str);
      if (updated)
      {
        free(obj->name);
        obj->name = updated;
      }
    }

    /* Replace in short description */
    if (obj->short_description)
    {
      char *updated =
          replace_substring_ci(obj->short_description, obj->restring_identifier, new_type_str);
      if (updated)
      {
        free(obj->short_description);
        obj->short_description = updated;
      }
    }

    /* Replace in long description */
    if (obj->description)
    {
      char *updated2 =
          replace_substring_ci(obj->description, obj->restring_identifier, new_type_str);
      if (updated2)
      {
        free(obj->description);
        obj->description = updated2;
      }
    }

    /* Update restring identifier to the current item subtype for future partial restrings */
    if (obj->restring_identifier)
      free(obj->restring_identifier);
    obj->restring_identifier = new_restring_id;
  }

  /* Restore original cost and enhancement */
  GET_OBJ_COST(obj) = orig_cost;
  GET_OBJ_VAL(obj, 4) = enhancement;

  /* Restore material if compatible */
  if (IS_HARD_METAL(GET_OBJ_MATERIAL(obj)) && IS_HARD_METAL(material))
    GET_OBJ_MATERIAL(obj) = material;
  else if (IS_LEATHER(GET_OBJ_MATERIAL(obj)) && IS_LEATHER(material))
    GET_OBJ_MATERIAL(obj) = material;
  else if (IS_CLOTH(GET_OBJ_MATERIAL(obj)) && IS_CLOTH(material))
    GET_OBJ_MATERIAL(obj) = material;
  else if (IS_WOOD(GET_OBJ_MATERIAL(obj)) && IS_WOOD(material))
    GET_OBJ_MATERIAL(obj) = material;

  /* Deduct the cost */
  if (cost > 0)
  {
    send_to_char(ch, "It cost you %d coins to reforge this item.\r\n", cost);
    GET_GOLD(ch) -= cost;
  }

  /* Prepare messages */
  send_to_char(ch, "You begin to reforge %s into %s %s.\r\n", obj->short_description,
               (GET_OBJ_TYPE(obj) == ITEM_WEAPON) ? AN(weapon_list[GET_OBJ_VAL(obj, 0)].name)
                                                  : AN(armor_list[GET_OBJ_VAL(obj, 1)].name),
               (GET_OBJ_TYPE(obj) == ITEM_WEAPON) ? weapon_list[GET_OBJ_VAL(obj, 0)].name
                                                  : armor_list[GET_OBJ_VAL(obj, 1)].name);
  snprintf(buf, sizeof(buf), "$n begins to reforge %s into %s %s.", obj->short_description,
           (GET_OBJ_TYPE(obj) == ITEM_WEAPON) ? AN(weapon_list[GET_OBJ_VAL(obj, 0)].name)
                                              : AN(armor_list[GET_OBJ_VAL(obj, 1)].name),
           (GET_OBJ_TYPE(obj) == ITEM_WEAPON) ? weapon_list[GET_OBJ_VAL(obj, 0)].name
                                              : armor_list[GET_OBJ_VAL(obj, 1)].name);
  act(buf, FALSE, ch, obj, 0, TO_ROOM);

  /* Set up crafting state */
  GET_CRAFTING_OBJ(ch) = obj;
  GET_CRAFTING_TYPE(ch) = SCMD_REFORGE;
  if (cost == 0)
    GET_CRAFTING_TICKS(ch) = 1;
  else
    GET_CRAFTING_TICKS(ch) = 10 - fast_craft_bonus;

  /* Start crafting event - save after all modifications including restring_identifier */
  save_char(ch, 0);
  Crash_crashsave(ch);
  NEW_EVENT(eCRAFTING, ch, NULL, 1 * PASSES_PER_SEC);
}

/**
 * ============================================================================
 * ARTISAN POINT SPENDING SYSTEM
 * Allows crafters to spend artisan points on materials and motes
 * ============================================================================
 */

/**
 * @brief Get the cost in artisan points for a material based on its grade
 * @param material_id The material ID (CRAFT_MAT_*)
 * @return The AP cost (5, 10, 20, 50, 100, or 150)
 */
int get_material_cost(int material_id)
{
  int grade = material_grade(material_id);
  
  switch (grade)
  {
  case 1:
    return 10;
  case 2:
    return 25;
  case 3:
    return 40;
  case 4:
    return 75;
  case 5:
    return 180;
  case 6: /* Dragon metal - even higher */
    return 300;
  default:
    return 0;
  }
}

/**
 * @brief Get the cost in artisan points for a mote
 * @return The AP cost (20)
 */
int get_mote_cost(void)
{
  return 20;
}

/**
 * @brief Display available materials and motes for purchase
 * Materials are ordered by grade (1-6) then alphabetically
 * Color coded by material type (hard metal, wood, cloth, etc)
 */
void show_artisan_shop(struct char_data *ch)
{
  int i, j, grade, cost, mat_group;
  char type_color[10];
  
  send_to_char(ch, "\tCArtisan Shop - Materials & Motes\tn\r\n");
  send_to_char(ch, "Your Artisan Points: \tC%d\tn\r\n\r\n", GET_ARTISAN_EXP(ch));
  
  send_to_char(ch, "\tW=== MATERIALS ===\tn\r\n");
  send_to_char(ch, "Format: artisan buy <quantity> <name>\r\n\r\n");
  
  /* Sort materials by grade, then display alphabetically within each grade */
  for (grade = 1; grade <= 6; grade++)
  {
    int materials_in_grade[NUM_CRAFT_MATS];
    int count = 0;
    
    /* Collect all materials of this grade */
    for (i = 1; i < NUM_CRAFT_MATS; i++)
    {
      if (material_grade(i) == grade)
      {
        materials_in_grade[count++] = i;
      }
    }
    
    /* Sort collected materials alphabetically */
    for (i = 0; i < count - 1; i++)
    {
      for (j = i + 1; j < count; j++)
      {
        if (strcasecmp(crafting_materials[materials_in_grade[i]],
                       crafting_materials[materials_in_grade[j]]) > 0)
        {
          int temp = materials_in_grade[i];
          materials_in_grade[i] = materials_in_grade[j];
          materials_in_grade[j] = temp;
        }
      }
    }
    
    /* Display materials in this grade */
    if (count > 0)
    {
      send_to_char(ch, "\t\tGrade %d Materials\tn\r\n", grade);
      
      for (i = 0; i < count; i++)
      {
        int mat_id = materials_in_grade[i];
        mat_group = craft_group_by_material(mat_id);
        cost = get_material_cost(mat_id);
        
        /* Color code by material type */
        switch (mat_group)
        {
        case CRAFT_GROUP_HARD_METALS:
          snprintf(type_color, sizeof(type_color), "\tR"); /* Red for hard metals */
          break;
        case CRAFT_GROUP_SOFT_METALS:
          snprintf(type_color, sizeof(type_color), "\tY"); /* Yellow for soft metals */
          break;
        case CRAFT_GROUP_WOOD:
          snprintf(type_color, sizeof(type_color), "\tw"); /* White for wood */
          break;
        case CRAFT_GROUP_HIDES:
          snprintf(type_color, sizeof(type_color), "\tB"); /* Black for hides */
          break;
        case CRAFT_GROUP_CLOTH:
          snprintf(type_color, sizeof(type_color), "\tC"); /* Cyan for cloth */
          break;
        case CRAFT_GROUP_STONE:
          snprintf(type_color, sizeof(type_color), "\tG"); /* Green for stone */
          break;
        default:
          snprintf(type_color, sizeof(type_color), "\tw"); /* White default */
          break;
        }
        
        send_to_char(ch, "  %s%-28s\tn %3d AP each\r\n", 
                     type_color, crafting_materials[mat_id], cost);
      }
      send_to_char(ch, "\r\n");
    }
  }
  
  send_to_char(ch, "\tW=== MOTES ===\tn\r\n");
  send_to_char(ch, "All Mote Types: %d AP each\r\n", get_mote_cost());
  send_to_char(ch, "Format: artisan buy <quantity> <mote name>\r\n\r\n");
  send_to_char(ch, "Types: air, dark, earth, fire, ice, light, lightning, water\r\n");
}

/**
 * @brief Get material ID from name
 */
int get_material_by_name(const char *name)
{
  int i;
  
  for (i = 1; i < NUM_CRAFT_MATS; i++)
  {
    if (is_abbrev(name, crafting_materials[i]))
      return i;
  }
  
  return CRAFT_MAT_NONE;
}

/**
 * @brief Get mote type from name (air, dark, earth, fire, ice, light, lightning, water)
 */
int get_mote_type_by_name(const char *name)
{
  if (is_abbrev(name, "air"))
    return CRAFTING_MOTE_AIR;
  if (is_abbrev(name, "dark"))
    return CRAFTING_MOTE_DARK;
  if (is_abbrev(name, "earth"))
    return CRAFTING_MOTE_EARTH;
  if (is_abbrev(name, "fire"))
    return CRAFTING_MOTE_FIRE;
  if (is_abbrev(name, "ice"))
    return CRAFTING_MOTE_ICE;
  if (is_abbrev(name, "light"))
    return CRAFTING_MOTE_LIGHT;
  if (is_abbrev(name, "lightning"))
    return CRAFTING_MOTE_LIGHTNING;
  if (is_abbrev(name, "water"))
    return CRAFTING_MOTE_WATER;
  
  return CRAFTING_MOTE_NONE;  /* Invalid mote type */
}

/**
 * @brief Handle buying materials with artisan points
 */
void handle_artisan_buy(struct char_data *ch, const char *argument)
{
  char arg1[100], arg2[200], item_name[200], buf[500];
  int material_id, quantity, cost, total_cost, mote_type;
  size_t len;
  
  if (!*argument)
  {
    show_artisan_shop(ch);
    return;
  }
  
  /* Parse: quantity and item name */
  half_chop_c(argument, arg1, sizeof(arg1), buf, sizeof(buf)); // chopping off first argument because we don't need it.
  half_chop_c(buf, arg1, sizeof(arg1), arg2, sizeof(arg2));
  
  if (!*arg1 || !*arg2)
  {
    send_to_char(ch, "Usage: artisan buy <quantity> <material or mote name>\r\n");
    send_to_char(ch, "Examples:\r\n");
    send_to_char(ch, "  artisan buy 10 iron ore\r\n");
    send_to_char(ch, "  artisan buy 5 fire\r\n");
    send_to_char(ch, "  artisan buy 5 fire mote\r\n");
    return;
  }
  
  quantity = atoi(arg1);
  if (quantity <= 0)
  {
    send_to_char(ch, "Quantity must be a number above zero.\r\n");
    return;
  }
  else if (quantity > 1000)
    quantity = 1000;
  
  /* Copy arg2 to item_name for modification */
  snprintf(item_name, sizeof(item_name), "%s", arg2);
  
  /* Check if item_name ends with " mote" and strip it if present */
  len = strlen(item_name);
  if (len > 5 && !strcasecmp(item_name + len - 5, " mote"))
  {
    item_name[len - 5] = '\0';  /* Strip " mote" from end */
  }
  
  /* Try to match as a mote type (by name or number) */
  if (is_number(item_name))
  {
    send_to_char(ch, "Please specify the mote type you would like to buy.\r\n");
    return;
  }
  else
  {
    mote_type = get_mote_type_by_name(item_name);
  }
  
  if (mote_type >= 1 && mote_type <= 8)
  {
    /* It's a mote purchase */
    cost = get_mote_cost();
    total_cost = cost * quantity;
    
    if (GET_ARTISAN_EXP(ch) < total_cost)
    {
      send_to_char(ch, "You don't have enough artisan points. You need %d AP but only have %d AP.\r\n",
                   total_cost, GET_ARTISAN_EXP(ch));
      return;
    }
    
    /* Purchase motes */
    GET_ARTISAN_EXP(ch) -= total_cost;
    GET_CRAFT_MOTES(ch, mote_type) += quantity;
    
    send_to_char(ch, "\tYSale Complete!\tn\r\n");
    send_to_char(ch, "You purchased %d %s(s) for %d AP.\r\n", quantity, crafting_motes[mote_type], total_cost);
    send_to_char(ch, "You now have %d artisan points.\r\n", GET_ARTISAN_EXP(ch));
    
    /* Save character */
    save_char(ch, 0);
    return;
  }
  
  /* Not a mote, try as a material - use original arg2 in case the material name has "mote" in it */
  material_id = get_material_by_name(arg2);
  if (material_id == CRAFT_MAT_NONE)
  {
    send_to_char(ch, "That item is not available. Type 'artisan shop' to see available items.\r\n");
    return;
  }
  
  cost = get_material_cost(material_id);
  if (cost == 0)
  {
    send_to_char(ch, "That material cannot be purchased.\r\n");
    return;
  }
  
  total_cost = cost * quantity;
  
  if (GET_ARTISAN_EXP(ch) < total_cost)
  {
    send_to_char(ch, "You don't have enough artisan points. You need %d AP but only have %d AP.\r\n",
                 total_cost, GET_ARTISAN_EXP(ch));
    return;
  }
  
  /* Purchase materials */
  GET_ARTISAN_EXP(ch) -= total_cost;
  GET_CRAFT_MAT(ch, material_id) += quantity;
  
  send_to_char(ch, "\tYSale Complete!\tn\r\n");
  send_to_char(ch, "You purchased %d %s for %d AP.\r\n",
               quantity, crafting_materials[material_id], total_cost);
  send_to_char(ch, "You now have %d artisan points.\r\n", GET_ARTISAN_EXP(ch));
  
  /* Save character */
  save_char(ch, 0);
}

/* Login-time notification for supply order cooldowns */
void notify_supply_order_cooldown_on_login(struct char_data *ch)
{
  time_t now = time(NULL);
  int any_cooldowns = 0;
  int any_refresh_pending = 0;
  int slot;

  if (!ch)
    return;

  /* Check for slot cooldowns */
  for (slot = 0; slot < 5; slot++)
  {
    if (GET_CRAFT(ch).supply_slot_cooldowns[slot] > 0 && now < GET_CRAFT(ch).supply_slot_cooldowns[slot])
    {
      any_cooldowns = 1;
      break;
    }
  }

  /* Check for refresh cooldown */
  if (GET_CRAFT(ch).supply_slots_next_refresh > 0 && now < GET_CRAFT(ch).supply_slots_next_refresh)
  {
    any_refresh_pending = 1;
  }

  /* Only display notification if there are active cooldowns */
  if (any_cooldowns || any_refresh_pending)
  {
    send_to_char(ch, "\r\n[Supply Order Status]\r\n");

    if (any_refresh_pending)
    {
      int refresh_hours = (GET_CRAFT(ch).supply_slots_next_refresh - now) / 3600;
      int refresh_minutes = ((GET_CRAFT(ch).supply_slots_next_refresh - now) % 3600) / 60;
      send_to_char(ch, "  Contract slots refresh in: %d hours, %d minutes\r\n", refresh_hours, refresh_minutes);
    }

    if (any_cooldowns)
    {
      send_to_char(ch, "  Slot cooldowns:\r\n");
      for (slot = 0; slot < 5; slot++)
      {
        if (GET_CRAFT(ch).supply_slot_cooldowns[slot] > 0 &&
            now < GET_CRAFT(ch).supply_slot_cooldowns[slot])
        {
          int cooldown_hours = (GET_CRAFT(ch).supply_slot_cooldowns[slot] - now) / 3600;
          int cooldown_minutes = ((GET_CRAFT(ch).supply_slot_cooldowns[slot] - now) % 3600) / 60;
          send_to_char(ch, "    Slot %d: %d hour%s, %d minute%s\r\n", 
                       slot + 1, 
                       cooldown_hours, (cooldown_hours != 1 ? "s" : ""),
                       cooldown_minutes, (cooldown_minutes != 1 ? "s" : ""));
        }
      }
    }

    send_to_char(ch, "  Type 'supplyorder cooldown' for detailed timing information.\r\n\r\n");
  }
}

/**
 * @brief Spec proc for artisan shop - allows NPC to sell materials and motes
 * Usage: Attach to an NPC to allow players to buy materials with artisan points
 * Commands: artisan shop, artisan buy <item> [qty]
 */
SPECIAL(artisan_shop)
{
  char arg[200];
  
  if (!CMD_IS("artisanshop"))
    return 0;
  
  if (FIGHTING(ch))
  {
    send_to_char(ch, "You're too busy fighting!\r\n");
    return 1;
  }
  
  one_argument(argument, arg, sizeof(arg));
  
  if (!*arg)
  {
    show_artisan_shop(ch);
    return 1;
  }
  
  if (is_abbrev(arg, "shop"))
  {
    show_artisan_shop(ch);
    return 1;
  }
  
  if (is_abbrev(arg, "buy"))
  {
    skip_spaces(&argument);
    handle_artisan_buy(ch, argument);
    return 1;
  }
  
  send_to_char(ch, "Usage:\r\n");
  send_to_char(ch, "  artisan shop                     - View materials and motes for sale\r\n");
  send_to_char(ch, "  artisan buy <qty> <name>        - Purchase any material or mote by name\r\n");
  return 1;
}
