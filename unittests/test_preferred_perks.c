/* Exercise the production perk resolver and stage awards; only logging,
 * messages and XP thresholds are supplied by this fixture. */
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "spells.h"
#include "class.h"
#include "constants.h"
#include "perks.h"
#include <assert.h>

struct player_special_data dummy_mob;
struct class_table class_list[NUM_CLASSES];
const char *class_names[NUM_CLASSES + 1];
struct config_data config_info;
static struct char_data pc;
static struct player_special_data saved;
static int notices;
void basic_mud_log(const char *format, ...) { (void)format; }
size_t send_to_char(struct char_data *ch, const char *format, ...)
{ (void)ch; (void)format; notices++; return 0; }
long level_exp(struct char_data *ch, int level) { (void)ch; return level * 1000L; }

static void fixture(void)
{
  int i;
  memset(&pc, 0, sizeof(pc));
  memset(&saved, 0, sizeof(saved));
  pc.player_specials = &saved;
  GET_LEVEL(&pc) = 10;
  GET_CLASS(&pc) = CLASS_ARCANE_ARCHER;
  GET_PREFERRED_ARCANE(&pc) = CLASS_UNDEFINED;
  GET_PREFERRED_DIVINE(&pc) = CLASS_UNDEFINED;
  GET_PREFERRED_MARTIAL(&pc) = CLASS_UNDEFINED;
  saved.saved.sorcerer_perks_migrated = TRUE;
  CONFIG_PERK_SYSTEM = TRUE;
  for (i = 0; i < NUM_CLASSES; i++)
  { class_names[i] = "test class"; class_list[i].name = "test class"; }
  for (i = 0; i < MAX_LOREMASTER_LEVELS; i++)
    GET_LOREMASTER_CASTER_CLASS((&pc), i) = CLASS_UNDEFINED;
  class_list[CLASS_WEAPON_MASTER].prestige_class = TRUE;
  class_list[CLASS_STALWART_DEFENDER].prestige_class = TRUE;
  class_list[CLASS_DRAGONRIDER].prestige_class = TRUE;
  class_list[CLASS_SHIFTER].prestige_class = TRUE;
  class_list[CLASS_LOREMASTER].prestige_class = TRUE;
  notices = 0;
}

static void expect_award(int class_id, int first, int second)
{
  int a, b;
  get_award_perk_classes(&pc, class_id, &a, &b);
  if (a != first || b != second)
    fprintf(stderr, "Class %d: got %d/%d, expected %d/%d\n", class_id, a, b, first, second);
  assert(a == first && b == second);
}

static void routing(void)
{
  int i;
  fixture();
  for (i = 0; i < NUM_CLASSES; i++)
  {
    bool martial = i == CLASS_WARRIOR || i == CLASS_BERSERKER || i == CLASS_PALADIN ||
                   i == CLASS_BLACKGUARD || i == CLASS_RANGER || i == CLASS_MONK;
    assert(valid_preferred_class(PREFERRED_MARTIAL, i) == martial);
  }
  assert(!valid_preferred_class(PREFERRED_MARTIAL, -1));
  assert(!valid_preferred_class(PREFERRED_ARCANE, CLASS_ROGUE));
  assert(!perk_preferences_ready(&pc, CLASS_ARCANE_ARCHER, TRUE) && notices);
  expect_award(CLASS_ARCANE_ARCHER, CLASS_UNDEFINED, CLASS_UNDEFINED);
  GET_PREFERRED_ARCANE(&pc) = CLASS_SORCERER;
  assert(perk_preferences_ready(&pc, CLASS_ARCANE_SHADOW, FALSE));
  expect_award(CLASS_ARCANE_SHADOW, CLASS_SORCERER, CLASS_ROGUE);
  assert(!perk_preferences_ready(&pc, CLASS_ARCANE_ARCHER, FALSE));
  GET_PREFERRED_MARTIAL(&pc) = CLASS_WARRIOR;
  assert(perk_preferences_ready(&pc, CLASS_ARCANE_ARCHER, FALSE));
  expect_award(CLASS_ARCANE_ARCHER, CLASS_WARRIOR, CLASS_SORCERER);
  GET_PREFERRED_ARCANE(&pc) = CLASS_WIZARD;
  GET_PREFERRED_MARTIAL(&pc) = CLASS_RANGER;
  expect_award(CLASS_ARCANE_ARCHER, CLASS_RANGER, CLASS_WIZARD);
  expect_award(CLASS_ELDRITCH_KNIGHT, CLASS_WIZARD, CLASS_RANGER);
  expect_award(CLASS_SPELLSWORD, CLASS_WIZARD, CLASS_RANGER);
  expect_award(CLASS_DUELIST, CLASS_RANGER, CLASS_ROGUE);
  expect_award(CLASS_ARCANE_SHADOW, CLASS_WIZARD, CLASS_ROGUE);
  assert(!perk_preferences_ready(&pc, CLASS_MYSTIC_THEURGE, FALSE));
  assert(!perk_preferences_ready(&pc, CLASS_SACRED_FIST, FALSE));
  GET_PREFERRED_DIVINE(&pc) = CLASS_DRUID;
  expect_award(CLASS_MYSTIC_THEURGE, CLASS_WIZARD, CLASS_DRUID);
  expect_award(CLASS_SACRED_FIST, CLASS_RANGER, CLASS_DRUID);
  expect_award(CLASS_KNIGHT_OF_SOLAMNIA, CLASS_RANGER, CLASS_DRUID);
  expect_award(CLASS_KNIGHT_OF_THE_SKULL, CLASS_RANGER, CLASS_DRUID);
  expect_award(CLASS_KNIGHT_OF_THE_THORN, CLASS_WIZARD, CLASS_RANGER);
  expect_award(CLASS_KNIGHT_OF_THE_LILY, CLASS_RANGER, CLASS_WIZARD);
  expect_award(CLASS_NECROMANCER, CLASS_WIZARD, CLASS_DRUID);
  CLASS_LEVEL((&pc), CLASS_SORCERER) = 10;
  CLASS_LEVEL((&pc), CLASS_WARRIOR) = 5;
  expect_award(CLASS_DRAGON_DISCIPLE, CLASS_WIZARD, CLASS_RANGER);
  expect_award(CLASS_LOREMASTER, CLASS_WIZARD, CLASS_WIZARD);
  CLASS_LEVEL((&pc), CLASS_SORCERER) = CLASS_LEVEL((&pc), CLASS_WARRIOR) = 0;
  GET_PREFERRED_ARCANE(&pc) = CLASS_UNDEFINED;
  GET_PREFERRED_DIVINE(&pc) = CLASS_UNDEFINED;
  assert(perk_preferences_ready(&pc, CLASS_DUELIST, FALSE));
  expect_award(CLASS_DUELIST, CLASS_RANGER, CLASS_ROGUE);
  GET_PREFERRED_ARCANE(&pc) = CLASS_WIZARD;
  GET_PREFERRED_DIVINE(&pc) = CLASS_DRUID;
  GET_PREFERRED_MARTIAL(&pc) = CLASS_WIZARD;
  assert(!perk_preferences_ready(&pc, CLASS_ARCANE_ARCHER, FALSE));
  expect_award(CLASS_ARCANE_ARCHER, CLASS_UNDEFINED, CLASS_UNDEFINED);
  /* Single-tree classes keep both points and need no preferences. */
  GET_PREFERRED_ARCANE(&pc) = CLASS_UNDEFINED;
  GET_PREFERRED_DIVINE(&pc) = CLASS_UNDEFINED;
  expect_award(CLASS_WARRIOR, CLASS_WARRIOR, CLASS_WARRIOR);
  assert(!perk_preferences_ready(&pc, CLASS_WEAPON_MASTER, FALSE));
  expect_award(CLASS_WEAPON_MASTER, CLASS_UNDEFINED, CLASS_UNDEFINED);
  expect_award(CLASS_SORCERER, CLASS_SORCERER, CLASS_SORCERER);
  assert(perk_preferences_ready(&pc, CLASS_WARRIOR, FALSE));
  assert(!perk_preferences_ready(&pc, NUM_CLASSES, FALSE));
}

static void awards_and_stages(void)
{
  int awarded, i;
  fixture();
  GET_EXP(&pc) = 11000;
  assert(!check_stage_advancement(&pc, &awarded));
  assert(!awarded && saved.saved.stage_info.current_stage == 0);
  award_stage_perk_points(&pc, CLASS_ARCANE_ARCHER);
  for (i = 0; i < NUM_CLASSES; i++) assert(!saved.saved.perk_points[i]);
  GET_PREFERRED_ARCANE(&pc) = CLASS_SORCERER;
  GET_PREFERRED_MARTIAL(&pc) = CLASS_WARRIOR;
  assert(check_stage_advancement(&pc, &awarded));
  assert(awarded == 3 && saved.saved.stage_info.current_stage == 4);
  assert(saved.saved.perk_points[CLASS_SORCERER] == 3);
  assert(saved.saved.perk_points[CLASS_WARRIOR] == 3);
  assert(!check_stage_advancement(&pc, &awarded) && !awarded);
  GET_PREFERRED_ARCANE(&pc) = CLASS_WIZARD;
  GET_PREFERRED_MARTIAL(&pc) = CLASS_RANGER;
  award_stage_perk_points(&pc, CLASS_ARCANE_ARCHER);
  assert(saved.saved.perk_points[CLASS_WIZARD] == 1);
  assert(saved.saved.perk_points[CLASS_RANGER] == 1);
  assert(saved.saved.perk_points[CLASS_SORCERER] == 3);
  assert(saved.saved.perk_points[CLASS_WARRIOR] == 3);
  award_stage_perk_points(&pc, CLASS_WARRIOR);
  assert(saved.saved.perk_points[CLASS_WARRIOR] == 5);
}

static void automatic_preferences(void)
{
  int i;
  fixture();
  auto_select_preferred_classes(&pc);
  assert(GET_PREFERRED_ARCANE(&pc) == CLASS_UNDEFINED);
  assert(GET_PREFERRED_DIVINE(&pc) == CLASS_UNDEFINED);
  assert(GET_PREFERRED_MARTIAL(&pc) == CLASS_UNDEFINED);
  CLASS_LEVEL((&pc), CLASS_SORCERER) = 5;
  CLASS_LEVEL((&pc), CLASS_WARRIOR) = 3;
  CLASS_LEVEL((&pc), CLASS_CLERIC) = 2;
  assert(perk_preferences_ready(&pc, CLASS_ARCANE_ARCHER, FALSE));
  assert(GET_PREFERRED_ARCANE(&pc) == CLASS_SORCERER);
  assert(GET_PREFERRED_DIVINE(&pc) == CLASS_CLERIC);
  assert(GET_PREFERRED_MARTIAL(&pc) == CLASS_WARRIOR);
  expect_award(CLASS_ARCANE_ARCHER, CLASS_WARRIOR, CLASS_SORCERER);
  /* A saved/manual choice is retained, including after a second ranked class. */
  GET_PREFERRED_ARCANE(&pc) = CLASS_WIZARD;
  CLASS_LEVEL((&pc), CLASS_RANGER) = 1;
  auto_select_preferred_classes(&pc);
  assert(GET_PREFERRED_ARCANE(&pc) == CLASS_WIZARD);
  assert(GET_PREFERRED_MARTIAL(&pc) == CLASS_WARRIOR);
  fixture();
  CLASS_LEVEL((&pc), CLASS_WIZARD) = 10;
  CLASS_LEVEL((&pc), CLASS_SORCERER) = 1;
  CLASS_LEVEL((&pc), CLASS_RANGER) = 10;
  CLASS_LEVEL((&pc), CLASS_MONK) = 1;
  CLASS_LEVEL((&pc), CLASS_CLERIC) = 10;
  CLASS_LEVEL((&pc), CLASS_DRUID) = 1;
  auto_select_preferred_classes(&pc);
  assert(GET_PREFERRED_ARCANE(&pc) == CLASS_UNDEFINED);
  assert(GET_PREFERRED_DIVINE(&pc) == CLASS_UNDEFINED);
  assert(GET_PREFERRED_MARTIAL(&pc) == CLASS_UNDEFINED);
  assert(!perk_preferences_ready(&pc, CLASS_ARCANE_ARCHER, FALSE));
  /* Every supported martial class is eligible as a sole ranked candidate. */
  for (i = 0; i < NUM_CLASSES; i++)
    if (valid_preferred_class(PREFERRED_MARTIAL, i))
    {
      fixture();
      CLASS_LEVEL((&pc), i) = 1;
      GET_PREFERRED_MARTIAL(&pc) = CLASS_WIZARD; /* invalid category */
      auto_select_preferred_classes(&pc);
      assert(GET_PREFERRED_MARTIAL(&pc) == i);
    }
  fixture();
  CLASS_LEVEL((&pc), CLASS_ARCANE_ARCHER) = 1;
  auto_select_preferred_classes(&pc);
  assert(GET_PREFERRED_ARCANE(&pc) == CLASS_UNDEFINED);
  assert(GET_PREFERRED_MARTIAL(&pc) == CLASS_UNDEFINED);
}

static void single_tree_prestige(void)
{
  fixture();
  assert(!perk_preferences_ready(&pc, CLASS_WEAPON_MASTER, FALSE));
  GET_PREFERRED_MARTIAL(&pc) = CLASS_RANGER;
  expect_award(CLASS_WEAPON_MASTER, CLASS_RANGER, CLASS_RANGER);
  expect_award(CLASS_STALWART_DEFENDER, CLASS_RANGER, CLASS_RANGER);
  expect_award(CLASS_DRAGONRIDER, CLASS_RANGER, CLASS_RANGER);
  award_stage_perk_points(&pc, CLASS_WEAPON_MASTER);
  assert(saved.saved.perk_points[CLASS_RANGER] == 2);
  assert(!saved.saved.perk_points[CLASS_WARRIOR]);
  assert(!perk_preferences_ready(&pc, CLASS_SHIFTER, FALSE));
  GET_PREFERRED_DIVINE(&pc) = CLASS_INQUISITOR;
  expect_award(CLASS_SHIFTER, CLASS_INQUISITOR, CLASS_INQUISITOR);
  GET_PREFERRED_ARCANE(&pc) = CLASS_BARD;
  expect_award(CLASS_LOREMASTER, CLASS_BARD, CLASS_BARD);
  expect_award(CLASS_WARRIOR, CLASS_WARRIOR, CLASS_WARRIOR);
  fixture();
  CLASS_LEVEL((&pc), CLASS_MONK) = 2;
  assert(perk_preferences_ready(&pc, CLASS_WEAPON_MASTER, FALSE));
  expect_award(CLASS_WEAPON_MASTER, CLASS_MONK, CLASS_MONK);
}

static void warrior_weapon_bonuses(void)
{
  struct char_perk_data damage = {0}, focus = {0}, other = {0};
  struct perk_data *perk;
  fixture();
  /* Define only the production fields consumed by the bonus resolver. */
  extern struct perk_data perk_list[NUM_PERKS];
  perk = &perk_list[PERK_FIGHTER_WEAPON_SPEC_1];
  perk->id = PERK_FIGHTER_WEAPON_SPEC_1;
  perk->associated_class = CLASS_WARRIOR;
  perk->effect_type = PERK_EFFECT_WEAPON_DAMAGE;
  perk->effect_value = 2;
  damage.perk_id = perk->id; damage.current_rank = 3; damage.next = &focus;
  perk = &perk_list[PERK_FIGHTER_WEAPON_FOCUS_1];
  perk->id = PERK_FIGHTER_WEAPON_FOCUS_1;
  perk->associated_class = CLASS_WARRIOR;
  perk->effect_type = PERK_EFFECT_WEAPON_TOHIT;
  perk->effect_value = 1;
  focus.perk_id = perk->id; focus.current_rank = 2; focus.next = &other;
  perk = &perk_list[PERK_CLERIC_DIVINE_FAVOR_1];
  perk->id = PERK_CLERIC_DIVINE_FAVOR_1;
  perk->associated_class = CLASS_CLERIC;
  perk->effect_type = PERK_EFFECT_WEAPON_DAMAGE;
  perk->effect_value = 99;
  other.perk_id = perk->id; other.current_rank = 1;
  saved.saved.perks = &damage;
  assert(get_warrior_weapon_bonus(&pc, PERK_EFFECT_WEAPON_DAMAGE) == 6);
  assert(get_warrior_weapon_bonus(&pc, PERK_EFFECT_WEAPON_TOHIT) == 2);
  assert(!get_warrior_weapon_bonus(&pc, PERK_EFFECT_HP));
  assert(!get_warrior_weapon_bonus(NULL, PERK_EFFECT_WEAPON_DAMAGE));
  saved.saved.perks = NULL;
  assert(!get_warrior_weapon_bonus(&pc, PERK_EFFECT_WEAPON_DAMAGE));
}

int main(void)
{
  routing(); awards_and_stages(); automatic_preferences(); single_tree_prestige(); warrior_weapon_bonuses();
  puts("Preferred perk routing: prerequisites, atomic awards, stage catch-up and preserved pools passed.");
  return 0;
}
