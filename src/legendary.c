/* Dragonmetal Awakening Phase 2: immutable catalog and pure rules only. */
#include "legendary.h"
#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

const char *const legendary_rank_names[6] = {
  "Unawakened", "Kindled", "Awakened", "Tempered", "Exalted", "Legendary"
};
const char *const legendary_essence_names[6] = {
  "Draconic", "Martial", "Arcane", "Spiritual", "Primal", "Shadow"
};
const char *const legendary_family_names[4] = {"None", "Assault", "Guard", "Focus"};
const char *const legendary_modifier_names[4] = {"None", "Enduring", "Ready", "Distant aid"};

const struct legendary_rules legendary_rules = {
  .daily_fragments=200, .daily_essences=40, .daily_catalysts=2,
  .global_cooldown=6, .quote_seconds=60, .boss_cooldown_seconds=72000,
  .participation_actions=3, .participation_recent=30,
  .encounter_expiry=180, .offense_limit=60, .prevention_limit=100, .healing_limit=100,
  .rolling_seconds=60, .resource_limit=20, .resource_seconds=180,
  .essence_exchange_ratio=3, .catalyst_exchange_fragments=100, .catalyst_exchange_essences=20
};
const struct legendary_cost legendary_rank_costs[6] = {
  {0,0,0,0,0}, {2000000,50000,500,0,0}, {5000000,100000,1000,200,0},
  {10000000,200000,2000,400,20}, {20000000,400000,4000,800,40}, {40000000,800000,6500,1400,60}
};
const struct legendary_cost legendary_reshape_costs[5] = {
  {0,20000,100,0,0}, {0,30000,200,0,0}, {0,50000,300,0,0},
  {0,20000,100,0,0}, {0,100000,500,200,0}
}; /* Path/family fee requires all 200 essences to match, not the rank half split. */
const struct legendary_boss_reward legendary_boss_rewards[3] = {
  {10,2,0}, {20,4,0}, {30,6,1}
};

const struct legendary_foundation_def legendary_foundations[16] = {
  {LEG_F01, "F01", "Accuracy",
   "+1 attack bonus on ordinary weapon attacks.", 3, 1, 1, 1, 1},
  {LEG_F02, "F02", "Force",
   "+1 weapon damage per successful attack, including ranged; no spell or proc damage.", 3, 1, 2, 2, 1},
  {LEG_F03, "F03", "Warding",
   "+1 deflection AC. Normal typed stacking applies.", 3, 1, 1, 1, 1},
  {LEG_F04, "F04", "Fortitude",
   "+1 Fortitude save.", 2, 1, 1, 1, 1},
  {LEG_F05, "F05", "Reflex",
   "+1 Reflex save.", 2, 1, 1, 1, 1},
  {LEG_F06, "F06", "Will",
   "+1 Will save.", 2, 1, 1, 1, 1},
  {LEG_F07, "F07", "Aptitude",
   "+1 STR, DEX, CON, INT, WIS or CHA, selected on purchase. Ordinary caps apply.", 3, 1, 1, 1, 2},
  {LEG_F08, "F08", "Expertise",
   "+2 to one selected trained skill; excludes crafting progression ranks and custom/non-roll abilities.", 2, 2, 2, 1, 3},
  {LEG_F09, "F09", "Vitality",
   "+20 maximum HP. Removal cannot heal or grant current HP.", 2, 20, 40, 2, 1},
  {LEG_F10, "F10", "Endurance",
   "+40 maximum movement. Removal cannot restore movement.", 1, 40, 80, 2, 1},
  {LEG_F11, "F11", "Reservoir",
   "+20 maximum spell/psionic points in the character's actual resource pool. No resource or spell access if none exists.", 2, 20, 20, 1, 1},
  {LEG_F12, "F12", "Elemental weave",
   "Resistance 3 percentage points against one of acid, cold, electricity, fire or sonic.", 2, 3, 3, 1, 2},
  {LEG_F13, "F13", "Sure footing",
   "+2 on actual checks resisting trip or forced movement. No check means no automatic immunity.", 1, 2, 2, 1, 1},
  {LEG_F14, "F14", "Steady grip",
   "+2 on actual checks resisting disarm.", 1, 2, 2, 1, 1},
  {LEG_F15, "F15", "Trailwise",
   "Reduce ordinary walking movement expenditure by 10%, minimum cost 1. Applies after normal reductions.", 1, 10, 10, 1, 1},
  {LEG_F16, "F16", "Recovery",
   "+5 HP on a normal regeneration tick while out of combat.", 1, 5, 5, 1, 1},
};

const struct legendary_power_def legendary_powers[31] = {
  {LEG_T01, "T01", "Measured strike",
   "Your next ordinary weapon attack within 12 seconds gains +2 attack; on a hit add 10 damage. Miss consumes it.",
   2, 90, LEG_CD_OFFENSE,
   LEGENDARY_FLAG(LEG_MOD_READY) | LEGENDARY_FLAG(LEG_MOD_ENDURING), false,
   {.attack=2, .damage=10, .window=12, .charges=1},
   {.attack=3, .damage=15, .window=12, .charges=1}},
  {LEG_T02, "T02", "Steady hands",
   "Self gains +3 to opposed trip, disarm and shove checks for 18 seconds; does not add attack or damage.",
   2, 90, LEG_CD_UTILITY,
   LEGENDARY_FLAG(LEG_MOD_READY) | LEGENDARY_FLAG(LEG_MOD_ENDURING) | LEGENDARY_FLAG(LEG_MOD_DISTANT_AID), false,
   {.check_bonus=3, .duration=18},
   {.check_bonus=4, .duration=24}},
  {LEG_T03, "T03", "Opening",
   "Your next weapon hit within 12 seconds marks the foe for 12 seconds. The next ordinary attack by another group member consumes the mark for +2 attack. One mark per target.",
   2, 90, LEG_CD_OFFENSE,
   LEGENDARY_FLAG(LEG_MOD_READY) | LEGENDARY_FLAG(LEG_MOD_ENDURING), false,
   {.attack=2, .window=12, .duration=12, .charges=1},
   {.attack=3, .window=12, .duration=12, .charges=1}},
  {LEG_T04, "T04", "Assess",
   "Display target's currently effective fire/cold/acid/electricity/sonic resistances and whether it is hard-control immune. No hidden scripts, gear or vulnerabilities invented.",
   2, 60, LEG_CD_UTILITY,
   LEGENDARY_FLAG(LEG_MOD_READY), false,
   {0},
   {.physical_info=1}},
  {LEG_T05, "T05", "Brace",
   "Self absorbs 20% of the next physical hit, maximum 30 damage, within 12 seconds.",
   2, 90, LEG_CD_PROTECTION,
   LEGENDARY_FLAG(LEG_MOD_READY) | LEGENDARY_FLAG(LEG_MOD_ENDURING) | LEGENDARY_FLAG(LEG_MOD_DISTANT_AID), false,
   {.percent=20, .pool=30, .window=12, .charges=1},
   {.percent=20, .pool=40, .window=12, .charges=1}},
  {LEG_T06, "T06", "Hold ground",
   "Self gains +4 to checks against forced movement and trip for 18 seconds. No immunity to effects without a check.",
   2, 90, LEG_CD_PROTECTION,
   LEGENDARY_FLAG(LEG_MOD_READY) | LEGENDARY_FLAG(LEG_MOD_ENDURING) | LEGENDARY_FLAG(LEG_MOD_DISTANT_AID), false,
   {.check_bonus=4, .duration=18},
   {.check_bonus=6, .duration=24}},
  {LEG_T07, "T07", "Interpose",
   "Prepare for 12 seconds to redirect 20% of the next physical hit on one ally to yourself, maximum 30. Redirected damage bypasses your additional reductions, cannot recurse, and cannot reduce you below 1 HP; excess stays on the ally. Recipient prevention budget applies.",
   2, 120, LEG_CD_PROTECTION,
   LEGENDARY_FLAG(LEG_MOD_READY) | LEGENDARY_FLAG(LEG_MOD_ENDURING), false,
   {.percent=20, .pool=30, .window=12, .charges=1, .targets=1},
   {.percent=20, .pool=40, .window=12, .charges=1, .targets=1}},
  {LEG_T08, "T08", "Recover footing",
   "Remove a supported trip/knockdown condition from self and restore standing if ordinarily allowed; cannot act while unconscious, stunned, paralyzed or otherwise action-locked. No recovery bypass. At Legendary rank, also briefly strengthen your defense against trip.",
   2, 120, LEG_CD_UTILITY,
   LEGENDARY_FLAG(LEG_MOD_READY) | LEGENDARY_FLAG(LEG_MOD_ENDURING) | LEGENDARY_FLAG(LEG_MOD_DISTANT_AID), false,
   {0},
   {.check_bonus=2, .duration=12}},
  {LEG_T09, "T09", "Field dressing",
   "Heal self or one ally for 30 HP, capped by missing HP.",
   2, 120, LEG_CD_RESTORATIVE,
   LEGENDARY_FLAG(LEG_MOD_READY), false,
   {.heal=30, .targets=1},
   {.heal=40, .targets=1}},
  {LEG_T10, "T10", "Resolve",
   "Remove one fear effect with 12s or less remaining; longer fear instead loses 12s. Permanent, scripted and boss-locked fear is unaffected. Must otherwise be able to act.",
   2, 120, LEG_CD_RESTORATIVE,
   LEGENDARY_FLAG(LEG_MOD_READY) | LEGENDARY_FLAG(LEG_MOD_DISTANT_AID), false,
   {.fear_seconds=12},
   {.fear_seconds=18}},
  {LEG_T11, "T11", "Clean wound",
   "Remove one supported nonmagical bleed, oldest first, and heal 10 HP. No poison/disease/curse removal.",
   2, 120, LEG_CD_RESTORATIVE,
   LEGENDARY_FLAG(LEG_MOD_READY) | LEGENDARY_FLAG(LEG_MOD_DISTANT_AID), false,
   {.heal=10},
   {.heal=20}},
  {LEG_T12, "T12", "Trail stance",
   "For 30s, reduce ordinary walking movement cost by 20%, minimum 1; does not stack with F15. No teleport or mounted refund.",
   2, 180, LEG_CD_UTILITY,
   LEGENDARY_FLAG(LEG_MOD_READY) | LEGENDARY_FLAG(LEG_MOD_ENDURING), false,
   {.percent=20, .duration=30},
   {.percent=20, .duration=45}},
  {LEG_T13, "T13", "Pursuit mark",
   "Mark one enemy for 30s; show its direction only while it is in an immediately adjacent room connected by a traversable exit. No auto-movement or visibility bypass.",
   2, 120, LEG_CD_UTILITY,
   LEGENDARY_FLAG(LEG_MOD_READY) | LEGENDARY_FLAG(LEG_MOD_ENDURING), false,
   {.duration=30},
   {.duration=45}},
  {LEG_T14, "T14", "Coordinate",
   "One ally's next ordinary weapon attack or supported skill check within 12s gains +2, then consumes the effect. Excludes crafting, infusion, saving throws and spell DC.",
   2, 90, LEG_CD_UTILITY,
   LEGENDARY_FLAG(LEG_MOD_READY) | LEGENDARY_FLAG(LEG_MOD_ENDURING), false,
   {.check_bonus=2, .window=12, .charges=1, .targets=1},
   {.check_bonus=3, .window=12, .charges=1, .targets=1}},
  {LEG_T15, "T15", "Failed weave",
   "For 18s, prepare a refund for the next eligible spell/power that spends at least 20 points and fails concentration before producing any effect. Refund 20% of actual expenditure, maximum 10 points. Excludes scrolls, wands, free casts, resisted spells and cancelled casts.",
   2, 180, LEG_CD_RESOURCE,
   LEGENDARY_FLAG(LEG_MOD_READY) | LEGENDARY_FLAG(LEG_MOD_ENDURING), false,
   {.window=18, .resource_min=20, .refund_percent=20, .refund_cap=10, .charges=1},
   {.window=18, .resource_min=20, .refund_percent=20, .refund_cap=15, .charges=1}},
  {LEG_T16, "T16", "Focused reserve",
   "For 18s, your next eligible spell/power costing at least 20 points costs 5 fewer, minimum final cost 1. No refund, metamagic discount multiplication or effect on item casts.",
   2, 180, LEG_CD_RESOURCE,
   LEGENDARY_FLAG(LEG_MOD_READY) | LEGENDARY_FLAG(LEG_MOD_ENDURING), false,
   {.window=18, .resource_min=20, .resource_discount=5, .charges=1},
   {.window=18, .resource_min=20, .resource_discount=8, .charges=1}},
  {LEG_T17, "T17", "Breath brace",
   "Self absorbs 20% of the next dragon-breath damage packet within 18s, maximum 40. One packet, not every element or victim.",
   2, 120, LEG_CD_PROTECTION,
   LEGENDARY_FLAG(LEG_MOD_READY) | LEGENDARY_FLAG(LEG_MOD_ENDURING) | LEGENDARY_FLAG(LEG_MOD_DISTANT_AID), false,
   {.percent=20, .pool=40, .window=18, .charges=1},
   {.percent=20, .pool=50, .window=18, .charges=1}},
  {LEG_T18, "T18", "Veiled retreat",
   "Self gains +3 stealth for 18s. Does not grant invisibility, disengage, concealment or a free flee attempt.",
   2, 120, LEG_CD_UTILITY,
   LEGENDARY_FLAG(LEG_MOD_READY) | LEGENDARY_FLAG(LEG_MOD_ENDURING) | LEGENDARY_FLAG(LEG_MOD_DISTANT_AID), false,
   {.check_bonus=3, .duration=18},
   {.check_bonus=4, .duration=24}},
  {LEG_S01, "S01", "Bulwark",
   "Self or one ally absorbs 20% of incoming physical damage for 18s, up to 60 total damage. Ends on pool exhaustion.",
   3, 300, LEG_CD_PROTECTION,
   LEGENDARY_FLAG(LEG_MOD_READY) | LEGENDARY_FLAG(LEG_MOD_ENDURING), false,
   {.percent=20, .pool=60, .duration=18, .targets=1},
   {.percent=20, .pool=80, .duration=18, .targets=1}},
  {LEG_S02, "S02", "Formation ward",
   "Up to three same-room group members including self gain +2 on checks resisting trip and forced movement for 18s. Targets must be in an actual formation; no row bypass.",
   3, 300, LEG_CD_PROTECTION,
   LEGENDARY_FLAG(LEG_MOD_READY) | LEGENDARY_FLAG(LEG_MOD_ENDURING), false,
   {.check_bonus=2, .duration=18, .targets=3},
   {.check_bonus=3, .duration=24, .targets=3}},
  {LEG_S03, "S03", "Sure passage",
   "Self and up to two allies reduce ordinary walking cost by 20% for 45s, minimum 1. Does not bypass doors, terrain access, travel restrictions or F15/T12 stacking. Ends for a recipient leaving the source's group; room departure alone does not end this travel effect.",
   3, 300, LEG_CD_UTILITY,
   LEGENDARY_FLAG(LEG_MOD_READY) | LEGENDARY_FLAG(LEG_MOD_ENDURING), false,
   {.percent=20, .duration=45, .targets=3},
   {.percent=20, .duration=60, .targets=3}},
  {LEG_S04, "S04", "Mental bastion",
   "Self or one ally gains +2 Will saves against fear, charm and confusion for 18s. No immunity or removal of existing effects.",
   3, 300, LEG_CD_PROTECTION,
   LEGENDARY_FLAG(LEG_MOD_READY) | LEGENDARY_FLAG(LEG_MOD_ENDURING), false,
   {.check_bonus=2, .duration=18, .targets=1},
   {.check_bonus=3, .duration=24, .targets=1}},
  {LEG_S05, "S05", "Renewing touch",
   "Self or one ally heals 20 HP immediately and 10 HP at 6, 12 and 18 seconds, total at most 50. No revival; later ticks end on source invalidation or recipient death/room departure.",
   3, 300, LEG_CD_RESTORATIVE,
   LEGENDARY_FLAG(LEG_MOD_READY), false,
   {.heal=20, .interval=6, .ticks=3, .tick_heal=10, .duration=18, .targets=1},
   {.heal=30, .interval=6, .ticks=3, .tick_heal=10, .duration=18, .targets=1}},
  {LEG_S06, "S06", "Decoy stance",
   "Self gains +2 dodge AC against the next two ordinary weapon attacks within 18s. Each attack attempt consumes a charge even if it misses; ordinary dodge stacking applies.",
   3, 300, LEG_CD_PROTECTION,
   LEGENDARY_FLAG(LEG_MOD_READY) | LEGENDARY_FLAG(LEG_MOD_ENDURING) | LEGENDARY_FLAG(LEG_MOD_DISTANT_AID), false,
   {.ac=2, .charges=2, .duration=18},
   {.ac=2, .charges=3, .duration=18}},
  {LEG_S07, "S07", "Runic economy",
   "For 24s, your next two eligible spells/powers costing at least 20 points each cost 5 fewer, minimum 1. Same exclusions as T16.",
   3, 300, LEG_CD_RESOURCE,
   LEGENDARY_FLAG(LEG_MOD_READY) | LEGENDARY_FLAG(LEG_MOD_ENDURING), false,
   {.window=24, .resource_min=20, .resource_discount=5, .charges=2},
   {.window=24, .resource_min=20, .resource_discount=8, .charges=2}},
  {LEG_S08, "S08", "Shared focus",
   "One ally gains +3 concentration checks for 18s, and 20 temporary HP for the same duration.",
   3, 300, LEG_CD_PROTECTION,
   LEGENDARY_FLAG(LEG_MOD_READY) | LEGENDARY_FLAG(LEG_MOD_ENDURING), false,
   {.check_bonus=3, .temporary_hp=20, .duration=18, .targets=1},
   {.check_bonus=4, .temporary_hp=30, .duration=18, .targets=1}},
  {LEG_S09, "S09", "Hunter's opening",
   "Your next ordinary attack within 18s against one designated foe gains +2 attack and +20 damage. Miss consumes it.",
   3, 300, LEG_CD_OFFENSE,
   LEGENDARY_FLAG(LEG_MOD_READY) | LEGENDARY_FLAG(LEG_MOD_ENDURING), false,
   {.attack=2, .damage=20, .window=18, .charges=1},
   {.attack=3, .damage=30, .window=18, .charges=1}},
  {LEG_S10, "S10", "Dragonward",
   "Self or one ally absorbs 25% of the next dragon-breath damage packet within 18s, maximum 60.",
   3, 300, LEG_CD_PROTECTION,
   LEGENDARY_FLAG(LEG_MOD_READY) | LEGENDARY_FLAG(LEG_MOD_ENDURING), false,
   {.percent=25, .pool=60, .window=18, .charges=1, .targets=1},
   {.percent=25, .pool=80, .window=18, .charges=1, .targets=1}},
  {LEG_S11, "S11", "Grounding lance",
   "Registered dragonlance only: next ordinary lance hit within 12s forces DC25 Fortitude. On failure prevent flee and forced positional/formation relocation for 6s; do not prevent attacks/casts. Hard-control-immune dragons instead take 15 bonus damage; save success has no extra effect. Applies only to dragons.",
   3, 300, LEG_CD_CONTROL,
   LEGENDARY_FLAG(LEG_MOD_READY) | LEGENDARY_FLAG(LEG_MOD_ENDURING), true,
   {.damage=15, .window=12, .save_dc=25, .control_seconds=6, .charges=1},
   {.damage=25, .window=12, .save_dc=25, .control_seconds=6, .charges=1}},
  {LEG_S12, "S12", "Rider's defiance",
   "Registered dragonlance, riding your own living dragon mount: rider and mount each absorb 20% of the next physical hit within 18s, maximum 30 each. No extra mount attack.",
   3, 300, LEG_CD_PROTECTION,
   LEGENDARY_FLAG(LEG_MOD_READY) | LEGENDARY_FLAG(LEG_MOD_ENDURING), true,
   {.percent=20, .pool=30, .window=18, .charges=1, .targets=2},
   {.percent=20, .pool=40, .window=18, .charges=1, .targets=2}},
  {LEG_S13, "S13", "Rallying strike",
   "Your next ordinary weapon hit within 12s heals one ally designated at activation for 30 HP. Miss consumes the effect. Heal requires ally still visible, grouped and in the room.",
   3, 300, LEG_CD_RESTORATIVE,
   LEGENDARY_FLAG(LEG_MOD_READY) | LEGENDARY_FLAG(LEG_MOD_ENDURING), false,
   {.heal=30, .window=12, .charges=1, .targets=1},
   {.heal=40, .window=12, .charges=1, .targets=1}},
};

const struct legendary_slot_def legendary_slots[10] = {
  {LEG_SLOT_WEAPON, "Weapon", {LEG_F01, LEG_F02, LEG_F14, LEG_F08}, {LEG_T01, LEG_T02, LEG_T03, LEG_T04}, {LEG_S09, LEG_S10, LEG_S13}},
  {LEG_SLOT_ARMS, "Arms", {LEG_F13, LEG_F14, LEG_F08, LEG_F06}, {LEG_T02, LEG_T05, LEG_T06, LEG_T07}, {LEG_S01, LEG_S02, LEG_S13}},
  {LEG_SLOT_LEGS, "Legs", {LEG_F10, LEG_F13, LEG_F15, LEG_F05}, {LEG_T05, LEG_T06, LEG_T08, LEG_T12}, {LEG_S02, LEG_S03, LEG_S06}},
  {LEG_SLOT_HEAD, "Head", {LEG_F06, LEG_F08, LEG_F11, LEG_F04}, {LEG_T04, LEG_T10, LEG_T14, LEG_T16}, {LEG_S04, LEG_S07, LEG_S08}},
  {LEG_SLOT_BODY, "Body", {LEG_F03, LEG_F09, LEG_F04, LEG_F12}, {LEG_T05, LEG_T07, LEG_T09, LEG_T11}, {LEG_S01, LEG_S04, LEG_S05}},
  {LEG_SLOT_ABOUT, "About", {LEG_F05, LEG_F08, LEG_F12, LEG_F15}, {LEG_T06, LEG_T10, LEG_T12, LEG_T18}, {LEG_S02, LEG_S06, LEG_S10}},
  {LEG_SLOT_SHIELD, "Shield", {LEG_F03, LEG_F13, LEG_F04, LEG_F12}, {LEG_T05, LEG_T06, LEG_T07, LEG_T17}, {LEG_S01, LEG_S02, LEG_S10}},
  {LEG_SLOT_HELD, "Held", {LEG_F11, LEG_F08, LEG_F06, LEG_F16}, {LEG_T09, LEG_T14, LEG_T15, LEG_T16}, {LEG_S05, LEG_S07, LEG_S08}},
  {LEG_SLOT_FEET, "Feet", {LEG_F10, LEG_F13, LEG_F15, LEG_F05}, {LEG_T06, LEG_T08, LEG_T12, LEG_T13}, {LEG_S03, LEG_S06, LEG_S09}},
  {LEG_SLOT_HANDS, "Hands", {LEG_F08, LEG_F14, LEG_F04, LEG_F16}, {LEG_T02, LEG_T03, LEG_T09, LEG_T11}, {LEG_S05, LEG_S08, LEG_S13}},
};

const struct legendary_path_def legendary_paths[12] = {
  {LEG_PATH_DRAGONBANE, "Dragonbane", LEG_ESS_DRACONIC, {LEG_T17, LEG_S10, LEG_S11, LEG_S12}},
  {LEG_PATH_GUARDIAN, "Guardian", LEG_ESS_MARTIAL, {LEG_T07, LEG_S01, LEG_S02}},
  {LEG_PATH_SPELLWEAVER, "Spellweaver", LEG_ESS_ARCANE, {LEG_T15, LEG_S07, LEG_S08}},
  {LEG_PATH_PURSUER, "Pursuer", LEG_ESS_PRIMAL, {LEG_T13, LEG_S09, 0}},
  {LEG_PATH_LIFEBINDER, "Lifebinder", LEG_ESS_SPIRITUAL, {LEG_T09, LEG_S05, 0}},
  {LEG_PATH_TRICKSTER, "Trickster", LEG_ESS_SHADOW, {LEG_T18, LEG_S06, 0}},
  {LEG_PATH_IRONWILL, "Ironwill", LEG_ESS_SPIRITUAL, {LEG_T10, LEG_S04, 0}},
  {LEG_PATH_WAYFARER, "Wayfarer", LEG_ESS_PRIMAL, {LEG_T12, LEG_S03, 0}},
  {LEG_PATH_RUNEKEEPER, "Runekeeper", LEG_ESS_ARCANE, {LEG_T16, LEG_S07, 0}},
  {LEG_PATH_COMMANDER, "Commander", LEG_ESS_MARTIAL, {LEG_T14, LEG_S02, 0}},
  {LEG_PATH_SLAYER, "Slayer", LEG_ESS_MARTIAL, {LEG_T01, LEG_S09, 0}},
  {LEG_PATH_RENEWAL, "Renewal", LEG_ESS_SPIRITUAL, {LEG_T11, LEG_S05, 0}},
};

bool legendary_add(int64_t a, int64_t b, int64_t *out)
{
  if (!out || a < 0 || b < 0 || a > INT64_MAX - b)
    return false;
  *out = a + b;
  return true;
}

bool legendary_subtract(int64_t a, int64_t b, int64_t *out)
{
  if (!out || a < 0 || b < 0 || b > a)
    return false;
  *out = a - b;
  return true;
}

bool legendary_to_int(int64_t value, int *out)
{
  if (!out || value < 0 || value > INT_MAX)
    return false;
  *out = (int)value;
  return true;
}

static bool valid_rank(int rank)
{
  return rank >= LEG_RANK_NONE && rank <= LEG_RANK_V;
}

static bool cost_add(const struct legendary_cost *a, const struct legendary_cost *b,
                     struct legendary_cost *out)
{
  struct legendary_cost sum;
  if (!legendary_add(a->xp, b->xp, &sum.xp) ||
      !legendary_add(a->gold, b->gold, &sum.gold) ||
      !legendary_add(a->fragments, b->fragments, &sum.fragments) ||
      !legendary_add(a->essences, b->essences, &sum.essences) ||
      !legendary_add(a->catalysts, b->catalysts, &sum.catalysts))
    return false;
  *out = sum;
  return true;
}

bool legendary_cost_between(int from_rank, int to_rank, struct legendary_cost *out)
{
  struct legendary_cost cost = {0,0,0,0,0};
  int rank;
  if (!out || !valid_rank(from_rank) || !valid_rank(to_rank) || to_rank < from_rank)
    return false;
  for (rank = from_rank + 1; rank <= to_rank; rank++)
    if (!cost_add(&cost, &legendary_rank_costs[rank], &cost))
      return false;
  *out = cost;
  return true;
}

bool legendary_pair_cost(int primary_rank, int supporting_rank, struct legendary_cost *out)
{
  struct legendary_cost primary, supporting;
  return out && legendary_cost_between(0, primary_rank, &primary) &&
         legendary_cost_between(0, supporting_rank, &supporting) &&
         cost_add(&primary, &supporting, out);
}

const struct legendary_foundation_def *legendary_foundation(int id)
{
  if (id < LEG_F01 || id > LEG_F16)
    return NULL;
  return &legendary_foundations[id - LEG_F01];
}

const struct legendary_power_def *legendary_power(int id)
{
  size_t i;
  for (i = 0; i < LEGENDARY_POWER_COUNT; i++)
    if (legendary_powers[i].id == id)
      return &legendary_powers[i];
  return NULL;
}

const struct legendary_path_def *legendary_path(int id)
{
  if (id < LEG_PATH_DRAGONBANE || id > LEG_PATH_RENEWAL)
    return NULL;
  return &legendary_paths[id - LEG_PATH_DRAGONBANE];
}

const struct legendary_slot_def *legendary_slot(int id)
{
  if (id < LEG_SLOT_WEAPON || id > LEG_SLOT_HANDS)
    return NULL;
  return &legendary_slots[id];
}

int legendary_parse_code(const char *code)
{
  int id, number;
  if (!code || strlen(code) != 3 || code[1] < '0' || code[1] > '9' ||
      code[2] < '0' || code[2] > '9')
    return 0;
  number = (code[1] - '0') * 10 + code[2] - '0';
  switch (toupper((unsigned char)code[0]))
  {
  case 'F': id = number; return legendary_foundation(id) ? id : 0;
  case 'T': id = 100 + number; return legendary_power(id) ? id : 0;
  case 'S': id = 200 + number; return legendary_power(id) ? id : 0;
  default: return 0;
  }
}

bool legendary_allocate_essences(const int64_t balances[6], int matching_type,
                                  int64_t amount, int64_t matching_required, int64_t payment[6])
{
  int64_t remaining[6], result[6] = {0}, needed, take;
  int i, best;
  if (!balances || !payment || matching_type < 0 || matching_type >= 6 ||
      amount < 0 || amount > LEGENDARY_BALANCE_MAX || matching_required < 0 ||
      matching_required > amount)
    return false;
  for (i = 0; i < 6; i++)
  {
    if (balances[i] < 0 || balances[i] > LEGENDARY_BALANCE_MAX)
      return false;
    remaining[i] = balances[i];
  }
  if (remaining[matching_type] < matching_required)
    return false;
  result[matching_type] = matching_required;
  remaining[matching_type] -= matching_required;
  needed = amount - matching_required;
  while (needed > 0)
  {
    best = 0;
    for (i = 1; i < 6; i++)
      if (remaining[i] > remaining[best])
        best = i; /* Strict greater: equal balances prefer lower stable type ID. */
    if (remaining[best] == 0)
      return false;
    take = remaining[best] < needed ? remaining[best] : needed;
    result[best] += take;
    remaining[best] -= take;
    needed -= take;
  }
  memcpy(payment, result, sizeof(result));
  return true;
}

bool legendary_plan_essences(const int64_t balances[6], int matching_type,
                             int64_t amount, int64_t payment[6])
{
  /* Rank prices are even; reject odd requests instead of rounding a requirement. */
  return amount >= 0 && amount % 2 == 0 &&
         legendary_allocate_essences(balances, matching_type, amount, amount / 2, payment);
}

bool legendary_can_afford(const struct legendary_wallet *wallet,
                         const struct legendary_cost *cost, int path, bool all_matching,
                         int64_t payment[6])
{
  const struct legendary_path_def *definition = legendary_path(path);
  if (!wallet || !cost || !payment || !definition ||
      wallet->xp < 0 || wallet->xp > LEGENDARY_BALANCE_MAX ||
      wallet->gold < 0 || wallet->gold > INT_MAX ||
      wallet->fragments < 0 || wallet->fragments > LEGENDARY_BALANCE_MAX ||
      wallet->catalysts < 0 || wallet->catalysts > LEGENDARY_BALANCE_MAX ||
      cost->xp < 0 || cost->gold < 0 || cost->fragments < 0 ||
      cost->essences < 0 || cost->catalysts < 0 ||
      wallet->xp < cost->xp || wallet->gold < cost->gold ||
      wallet->fragments < cost->fragments || wallet->catalysts < cost->catalysts)
    return false;
  if (all_matching)
    return legendary_allocate_essences(wallet->essences, definition->essence,
                                       cost->essences, cost->essences, payment);
  return legendary_plan_essences(wallet->essences, definition->essence, cost->essences, payment);
}

int legendary_family_slot(int family)
{
  switch (family)
  {
  case LEG_FAMILY_ASSAULT: return LEG_SLOT_WEAPON;
  case LEG_FAMILY_GUARD: return LEG_SLOT_SHIELD;
  case LEG_FAMILY_FOCUS: return LEG_SLOT_HELD;
  default: return -1;
  }
}

bool legendary_item_eligible(const struct legendary_item_facts *facts, int slot)
{
  if (!facts || !legendary_slot(slot) || facts->npc || facts->character_level != 30 ||
      facts->character_id <= 0 || facts->bound_owner < -1 ||
      (facts->bound_owner > 0 && facts->bound_owner != facts->character_id) ||
      facts->item_level < 20 || facts->item_level > 30 || !facts->permanent ||
      facts->temporary || facts->expiring || facts->transformation || !facts->ordinary_allowed ||
      !(facts->wearable_slots & LEGENDARY_FLAG(slot)))
    return false;
  if (slot == LEG_SLOT_WEAPON)
    return facts->kind == LEG_ITEM_WEAPON;
  if (slot == LEG_SLOT_SHIELD)
    return facts->kind == LEG_ITEM_SHIELD;
  if (slot == LEG_SLOT_HELD)
    return facts->kind == LEG_ITEM_FOCUS;
  return facts->kind == LEG_ITEM_ARMOR;
}

bool legendary_supporting_active(const struct legendary_item_facts *facts)
{
  return legendary_item_eligible(facts, LEG_SLOT_WEAPON) && facts->actually_wielded &&
         !facts->offhand_occupied && (facts->two_hand_slot || facts->mandatory_two_hand_ranged);
}

bool legendary_power_available(int slot, int path, int power, bool dragonlance)
{
  const struct legendary_slot_def *s = legendary_slot(slot);
  const struct legendary_path_def *p = legendary_path(path);
  const struct legendary_power_def *definition = legendary_power(power);
  int i;
  if (!s || !p || !definition || (definition->requires_dragonlance && !dragonlance))
    return false;
  for (i = 0; i < 4; i++)
    if (s->techniques[i] == power)
      return true;
  for (i = 0; i < 3; i++)
    if (s->signatures[i] == power)
      return true;
  for (i = 0; i < 4; i++)
    if (p->powers[i] == power)
      return true;
  return false;
}

bool legendary_modifier_allowed(int power, int modifier)
{
  const struct legendary_power_def *p = legendary_power(power);
  if (!p || modifier < LEG_MOD_NONE || modifier > LEG_MOD_DISTANT_AID)
    return false;
  return modifier == LEG_MOD_NONE || (p->modifiers & LEGENDARY_FLAG(modifier)) != 0;
}

bool legendary_resolve_power(int power, int rank, int modifier,
                             struct legendary_resolved_power *out)
{
  const struct legendary_power_def *p = legendary_power(power);
  struct legendary_resolved_power result;
  if (!out || !p || !valid_rank(rank) || rank < p->minimum_rank ||
      !legendary_modifier_allowed(power, modifier) ||
      (modifier != LEG_MOD_NONE && rank < LEG_RANK_IV))
    return false;
  memset(&result, 0, sizeof(result));
  result.values = rank == LEG_RANK_V ? p->final : p->base;
  result.cooldown = p->cooldown;
  if (p->family == LEG_CD_PROTECTION || p->family == LEG_CD_RESTORATIVE)
    result.family_cooldown = 60;
  else if (p->family == LEG_CD_CONTROL)
    result.family_cooldown = 30;
  if (modifier == LEG_MOD_ENDURING)
  {
    /* Prefer the prepared trigger window over a later effect's own duration.
     * Never lengthen control_seconds or a periodic-healing tick schedule. */
    if (result.values.window >= 12)
      result.values.window += 6;
    else if (result.values.duration >= 12)
      result.values.duration += 6;
    else
      return false;
  }
  else if (modifier == LEG_MOD_READY)
    result.cooldown = (result.cooldown * 85 + 99) / 100;
  else if (modifier == LEG_MOD_DISTANT_AID)
    result.target_ally_instead = true;
  *out = result;
  return true;
}

bool legendary_plan_passives(const struct legendary_passive_request *requests, size_t count,
                             struct legendary_passive_result *out, int *points_used)
{
  struct legendary_passive_result results[LEGENDARY_MAX_CHANNELS];
  const struct legendary_foundation_def *f;
  size_t i, j;
  int used = 0, copies, distinct, value;
  if (!points_used || count > LEGENDARY_MAX_CHANNELS || (count && (!requests || !out)))
    return false;
  memset(results, 0, sizeof(results));
  for (i = 0; i < count; i++)
  {
    f = legendary_foundation(requests[i].id);
    if (!f || requests[i].subtype < 0 ||
        (f->id == LEG_F07 && requests[i].subtype > 5) ||
        (f->id == LEG_F12 && (requests[i].subtype > 4 ||
         requests[i].current_resistance < -100 || requests[i].current_resistance > 100)) ||
        (f->id == LEG_F08 && requests[i].subtype == 0) ||
        (f->id != LEG_F07 && f->id != LEG_F08 && f->id != LEG_F12 && requests[i].subtype != 0))
    {
      results[i].reason = LEG_PASSIVE_INVALID;
      continue;
    }
    if (!requests[i].eligible)
    {
      results[i].reason = LEG_PASSIVE_INELIGIBLE;
      continue;
    }
    copies = 0;
    distinct = 0;
    for (j = 0; j < i; j++)
      if (results[j].reason == LEG_PASSIVE_ACTIVE && requests[j].id == f->id)
      {
        distinct++; /* Subtyped foundations allow only one copy per subtype. */
        if (requests[j].subtype == requests[i].subtype)
          copies++;
      }
    value = f->value;
    if (f->id == LEG_F12 && value > 75 - requests[i].current_resistance)
      value = 75 - requests[i].current_resistance;
    if (copies >= f->copies ||
        ((f->id == LEG_F07 || f->id == LEG_F08 || f->id == LEG_F12) &&
         distinct >= f->distinct_subtypes) || value <= 0)
      results[i].reason = LEG_PASSIVE_CAPPED;
    else if (used + f->points > LEGENDARY_PASSIVE_POINTS)
      results[i].reason = LEG_PASSIVE_BUDGET;
    else
    {
      results[i].reason = LEG_PASSIVE_ACTIVE;
      results[i].value = value;
      results[i].points = f->points;
      used += f->points;
    }
  }
  if (count)
    memcpy(out, results, count * sizeof(*out));
  *points_used = used;
  return true;
}

static bool definition_error(char *error, size_t size, const char *message)
{
  if (error && size)
    snprintf(error, size, "%s", message);
  return false;
}

bool legendary_validate_definitions(char *error, size_t error_size)
{
  struct legendary_cost total;
  struct legendary_resolved_power resolved;
  const struct legendary_power_def *power;
  bool reachable[LEGENDARY_POWER_COUNT] = {false};
  size_t i, j, k;
  if (error && error_size)
    *error = '\0';
  for(i=0;i<LEGENDARY_ITEM_PASSIVE_COUNT;i++) {
    const struct legendary_item_passive_def *p=&legendary_item_passives[i];
    if(p->id!=(int)i+1||!p->code||strlen(p->code)!=3||!p->name||!p->description||
       !p->slots||(p->slots&~((1U<<LEGENDARY_SLOT_COUNT)-1))||p->per_rank<0||
       legendary_item_passive_amount(p->id,1)<1||p->ceiling<legendary_item_passive_amount(p->id,5))
      return definition_error(error,error_size,"Invalid item passive definition");
  }
  for (i = 0; i < LEGENDARY_FOUNDATION_COUNT; i++)
  {
    const struct legendary_foundation_def *f = &legendary_foundations[i];
    if (f->id != (int)i + 1 || legendary_parse_code(f->code) != f->id ||
        !f->name || !*f->name || !f->description || !*f->description ||
        f->points < 1 || f->points > 3 || f->value < 1 ||
        f->ceiling != f->value * f->copies || f->distinct_subtypes < 1)
      return definition_error(error, error_size, "Invalid foundation definition");
  }
  for (i = 0; i < LEGENDARY_POWER_COUNT; i++)
  {
    power = &legendary_powers[i];
    if (legendary_parse_code(power->code) != power->id || !power->name || !*power->name ||
        !power->description || !*power->description || power->cooldown <= 0 ||
        power->minimum_rank != (power->id < 200 ? LEG_RANK_II : LEG_RANK_III) ||
        !legendary_resolve_power(power->id, LEG_RANK_IV, LEG_MOD_READY, &resolved))
      return definition_error(error, error_size, "Invalid power or missing rank-IV option");
    for (j = i + 1; j < LEGENDARY_POWER_COUNT; j++)
      if (power->id == legendary_powers[j].id || !strcmp(power->code, legendary_powers[j].code))
        return definition_error(error, error_size, "Duplicate power identity");
  }
  for (i = 0; i < LEGENDARY_SLOT_COUNT; i++)
  {
    const struct legendary_slot_def *s = &legendary_slots[i];
    if (s->id != (int)i || !s->name || !*s->name)
      return definition_error(error, error_size, "Invalid slot identity");
    for (j = 0; j < 4; j++)
    {
      if (!legendary_foundation(s->foundations[j]))
        return definition_error(error, error_size, "Unknown slot foundation");
      power = legendary_power(s->techniques[j]);
      if (!power || power->minimum_rank != LEG_RANK_II)
        return definition_error(error, error_size, "Invalid slot technique");
      reachable[power - legendary_powers] = true;
      for (k = j + 1; k < 4; k++)
        if (s->techniques[j] == s->techniques[k] || s->foundations[j] == s->foundations[k])
          return definition_error(error, error_size, "Duplicate slot choice");
    }
    for (j = 0; j < 3; j++)
    {
      power = legendary_power(s->signatures[j]);
      if (!power || power->minimum_rank != LEG_RANK_III)
        return definition_error(error, error_size, "Invalid slot signature");
      reachable[power - legendary_powers] = true;
      for (k = j + 1; k < 3; k++)
        if (s->signatures[j] == s->signatures[k])
          return definition_error(error, error_size, "Duplicate slot signature");
    }
  }
  for (i = 0; i < LEGENDARY_PATH_COUNT; i++)
  {
    const struct legendary_path_def *p = &legendary_paths[i];
    if (p->id != (int)i + 1 || !p->name || !*p->name || p->essence < 0 || p->essence >= 6)
      return definition_error(error, error_size, "Invalid path");
    if (!p->powers[0])
      return definition_error(error, error_size, "Empty path");
    for (j = 0; j < 4; j++)
    {
      if (!p->powers[j])
        continue;
      power = legendary_power(p->powers[j]);
      if (!power)
        return definition_error(error, error_size, "Unknown path power");
      reachable[power - legendary_powers] = true;
      for (k = j + 1; k < 4; k++)
        if (p->powers[j] == p->powers[k])
          return definition_error(error, error_size, "Duplicate path power");
    }
  }
  for (i = 0; i < LEGENDARY_POWER_COUNT; i++)
    if (!reachable[i])
      return definition_error(error, error_size, "Unreachable power");
  for (i = 1; i <= LEG_RANK_V; i++)
  {
    const struct legendary_cost *c = &legendary_rank_costs[i];
    if (c->xp <= 0 || c->gold <= 0 || c->fragments <= 0 || c->essences < 0 ||
        c->essences % 2 != 0 || c->catalysts < 0)
      return definition_error(error, error_size, "Invalid rank cost");
  }
  if (!legendary_cost_between(0, LEG_RANK_V, &total) || total.xp != 77000000 ||
      total.gold != 1550000 || total.fragments != 14000 || total.essences != 2800 || total.catalysts != 120)
    return definition_error(error, error_size, "Rank totals differ from approved proposal");
  return true;
}

const struct legendary_item_passive_def legendary_item_passives[16] = {
  {1, "P01", "Vital thread", "maximum HP", LEGENDARY_FLAG(0)|LEGENDARY_FLAG(1)|LEGENDARY_FLAG(2)|LEGENDARY_FLAG(4)|LEGENDARY_FLAG(5)|LEGENDARY_FLAG(6), 5, 50},
  {2, "P02", "Mystic reservoir", "maximum spell/psionic points; requires an existing resource class", LEGENDARY_FLAG(0)|LEGENDARY_FLAG(3)|LEGENDARY_FLAG(4)|LEGENDARY_FLAG(7)|LEGENDARY_FLAG(9), 4, 40},
  {3, "P03", "Enduring stride", "maximum movement", LEGENDARY_FLAG(1)|LEGENDARY_FLAG(2)|LEGENDARY_FLAG(4)|LEGENDARY_FLAG(8), 8, 80},
  {4, "P04", "Renewing thread", "HP per ordinary out-of-combat regeneration tick", LEGENDARY_FLAG(0)|LEGENDARY_FLAG(4)|LEGENDARY_FLAG(5)|LEGENDARY_FLAG(6)|LEGENDARY_FLAG(9), 1, 10},
  {5, "P05", "Wayfarer", "percent reduction to ordinary unmounted walking cost", LEGENDARY_FLAG(2)|LEGENDARY_FLAG(5)|LEGENDARY_FLAG(8), 2, 20},
  {6, "P06", "Watchful", "trained Perception checks", LEGENDARY_FLAG(0)|LEGENDARY_FLAG(3)|LEGENDARY_FLAG(5)|LEGENDARY_FLAG(7), 1, 10},
  {7, "P07", "Veiled", "trained Stealth checks", LEGENDARY_FLAG(2)|LEGENDARY_FLAG(5)|LEGENDARY_FLAG(8)|LEGENDARY_FLAG(9), 1, 10},
  {8, "P08", "Centered", "trained Concentration checks", LEGENDARY_FLAG(0)|LEGENDARY_FLAG(1)|LEGENDARY_FLAG(3)|LEGENDARY_FLAG(4)|LEGENDARY_FLAG(7), 1, 10},
  {9, "P09", "Healer's touch", "trained Heal checks", LEGENDARY_FLAG(1)|LEGENDARY_FLAG(3)|LEGENDARY_FLAG(7)|LEGENDARY_FLAG(9), 1, 10},
  {10, "P10", "Runic insight", "trained Spellcraft checks", LEGENDARY_FLAG(0)|LEGENDARY_FLAG(3)|LEGENDARY_FLAG(7)|LEGENDARY_FLAG(9), 1, 10},
  {11, "P11", "Artificer's touch", "trained Use Magic Device checks", LEGENDARY_FLAG(1)|LEGENDARY_FLAG(3)|LEGENDARY_FLAG(7)|LEGENDARY_FLAG(9), 1, 10},
  {12, "P12", "Unyielding grip", "checks resisting disarm; does not grant immunity", LEGENDARY_FLAG(0)|LEGENDARY_FLAG(1)|LEGENDARY_FLAG(6)|LEGENDARY_FLAG(9), 1, 10},
  {13, "P13", "Rooted", "checks resisting trip/forced movement; does not grant immunity", LEGENDARY_FLAG(0)|LEGENDARY_FLAG(2)|LEGENDARY_FLAG(4)|LEGENDARY_FLAG(6)|LEGENDARY_FLAG(8), 1, 10},
  {14, "P14", "Stalwart", "Fortitude saves (+1/+1/+2/+2/+3 by rank)", LEGENDARY_FLAG(0)|LEGENDARY_FLAG(1)|LEGENDARY_FLAG(2)|LEGENDARY_FLAG(4)|LEGENDARY_FLAG(6), 0, 3},
  {15, "P15", "Agile", "Reflex saves (+1/+1/+2/+2/+3 by rank)", LEGENDARY_FLAG(2)|LEGENDARY_FLAG(5)|LEGENDARY_FLAG(8)|LEGENDARY_FLAG(9), 0, 3},
  {16, "P16", "Resolute", "Will saves (+1/+1/+2/+2/+3 by rank)", LEGENDARY_FLAG(0)|LEGENDARY_FLAG(3)|LEGENDARY_FLAG(5)|LEGENDARY_FLAG(6)|LEGENDARY_FLAG(7), 0, 3},
};
const struct legendary_item_passive_def *legendary_item_passive(int id)
{return id>=1&&id<=LEGENDARY_ITEM_PASSIVE_COUNT?&legendary_item_passives[id-1]:NULL;}
bool legendary_item_passive_available(int slot,int id)
{const struct legendary_item_passive_def *p=legendary_item_passive(id);return p&&slot>=0&&slot<10&&(p->slots&LEGENDARY_FLAG(slot));}
int legendary_item_passive_amount(int id,int rank)
{const struct legendary_item_passive_def *p=legendary_item_passive(id);if(!p||rank<1||rank>5)return 0;return p->per_rank?rank*p->per_rank:(rank+1)/2;}
