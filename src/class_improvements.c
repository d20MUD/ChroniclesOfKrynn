#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "spells.h"
#include "comm.h"
#include "handler.h"
#include "fight.h"
#include "class.h"
#include "bardic_performance.h"
#include "mud_event.h"
#include "actions.h"
#include "assign_wpn_armor.h"
#include "perks.h"
#include "class_improvements.h"
#include "db.h"
#include "dg_scripts.h"
#include "domains_schools.h"
#include "missions.h"

/* One shared gate prevents cross-character counterattack recursion. */
static int reaction_depth;
static bool counter_pending;
bool class_reaction_active(void) { return reaction_depth != 0; }
bool class_reaction_allow_hit(void)
{
  if (!reaction_depth)
    return TRUE;
  if (!counter_pending)
    return FALSE;
  counter_pending = FALSE;
  return TRUE;
}

static bool conscious(struct char_data *ch)
{
  return ch && GET_HIT(ch) > 0 && GET_POS(ch) >= POS_FIGHTING && !AFF_FLAGGED(ch, AFF_STUN) &&
         !AFF_FLAGGED(ch, AFF_DAZED) && !AFF_FLAGGED(ch, AFF_PARALYZED) &&
         !char_has_mud_event(ch, eSTUNNED);
}

bool class_feature_ready(struct char_data *ch) { return conscious(ch) && !IS_NPC(ch); }

/* Highest positive existing bonus; penalties must not let like bonuses stack. */
static int typed_bonus(struct char_data *ch, int location, int type)
{
  struct affected_type *af;
  struct obj_data *obj;
  int best = 0, i, j;
  for (af = ch->affected; af; af = af->next)
    if (af->location == location && af->bonus_type == type)
      best = MAX(best, af->modifier);
  for (i = 0; i < NUM_WEARS; i++)
    if ((obj = GET_EQ(ch, i)))
      for (j = 0; j < MAX_OBJ_AFFECT; j++)
        if (obj->affected[j].location == location && obj->affected[j].bonus_type == type)
          best = MAX(best, obj->affected[j].modifier);
  return best;
}

bool class_duelist_gear(struct char_data *ch)
{
  struct obj_data *weapon;
  if (!class_feature_ready(ch) || IS_CASTING(ch) || IS_WILDSHAPED(ch) || IS_MORPHED(ch) ||
      !HAS_FREE_HAND(ch) || compute_gear_armor_type(ch) > ARMOR_TYPE_LIGHT)
    return FALSE;
  weapon = get_wielded(ch, ATTACK_TYPE_PRIMARY);
  return weapon && GET_OBJ_TYPE(weapon) == ITEM_WEAPON && GET_WEAPON_TYPE(weapon) >= 0 &&
         GET_WEAPON_TYPE(weapon) < NUM_WEAPON_TYPES &&
         !IS_SET(weapon_list[GET_WEAPON_TYPE(weapon)].weaponFlags, WEAPON_FLAG_RANGED);
}

static bool same_group(struct char_data *a, struct char_data *b)
{
  return a == b || (GROUP(a) && GROUP(a) == GROUP(b));
}

static bool active_song(struct char_data *ch, int slot)
{
  int song = slot ? GET_PERFORMANCE_VAR(ch, 2) : GET_PERFORMING(ch);
  return class_feature_ready(ch) && IS_PERFORMING(ch) && song >= 0 && song < MAX_PERFORMANCES &&
         (!slot || has_bard_master_of_motifs(ch)) &&
         HAS_FEAT(ch, performance_info[song][PERFORMANCE_FEATNUM]) &&
         can_perform(ch, song, FALSE, TRUE);
}

int class_rhythm_bonus(struct char_data *ch, int location)
{
  int level, bonus;
  if (!ch || IS_NPC(ch) || (level = CLASS_LEVEL(ch, CLASS_BARD)) < 3 ||
      (!active_song(ch, 0) && !active_song(ch, 1)))
    return 0;
  bonus = level >= 15 ? 3 : level >= 9 ? 2 : 1;
  return MAX(0, bonus - MAX(0, typed_bonus(ch, location, BONUS_TYPE_COMPETENCE)));
}

int class_rally_save(struct char_data *ch, int type)
{
  if (!conscious(ch) || CI_STATE(ch).rally_until <= time(NULL))
    return 0;
  return MAX(0, CI_STATE(ch).rally_bonus -
                    MAX(0, typed_bonus(ch, APPLY_SAVING_FORT + type, BONUS_TYPE_MORALE)));
}

static bool guarded(struct char_data *guard, struct char_data *ally)
{
  return guard != ally && class_feature_ready(guard) && conscious(ally) &&
         GUARDING(guard) == ally && same_group(guard, ally) && IN_ROOM(guard) != NOWHERE &&
         IN_ROOM(guard) == IN_ROOM(ally) && affected_by_spell(guard, SKILL_DEFENSIVE_STANCE) &&
         !AFF_FLAGGED(guard, AFF_BLIND) &&
         combat_distance(guard, ally) <=
             combat_melee_range(guard, get_wielded(guard, ATTACK_TYPE_PRIMARY));
}

int class_guard_ac(struct char_data *ch)
{
  struct char_data *guard;
  int bonus = 0, level;
  if (!ch || IN_ROOM(ch) == NOWHERE)
    return 0;
  for (guard = world[IN_ROOM(ch)].people; guard; guard = guard->next_in_room)
    if (!IS_NPC(guard) && (level = CLASS_LEVEL(guard, CLASS_STALWART_DEFENDER)) >= 2 &&
        guarded(guard, ch))
      bonus = MAX(bonus, level >= 10 ? 3 : level >= 6 ? 2 : 1);
  return bonus;
}

ACMD(do_rally)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *ally;
  bool eligible = FALSE;
  int i, song;
  one_argument(argument, arg, sizeof(arg));
  ally = *arg ? get_char_room_vis(ch, arg, NULL) : ch;
  if (!class_feature_ready(ch) || CLASS_LEVEL(ch, CLASS_BARD) < 7 || !ally || !conscious(ally) ||
      !same_group(ch, ally) || AFF_FLAGGED(ally, AFF_DEAF))
  {
    send_to_char(ch, "Rally yourself or a conscious groupmate here while performing (Bard 7).\r\n");
    return;
  }
  for (i = 0; i < 2; i++)
  {
    song = i ? GET_PERFORMANCE_VAR(ch, 2) : GET_PERFORMING(ch);
    if (active_song(ch, i) && performance_info[song][PERFORMANCE_AOE] == PERFORM_AOE_GROUP)
      eligible = TRUE;
  }
  if (!eligible)
  {
    send_to_char(ch, "You must maintain a group performance that can reach your ally.\r\n");
    return;
  }
  if (CI_READY(ch, CI_RALLY) > time(NULL))
  {
    send_to_char(ch, "Your rallying verse is still recovering.\r\n");
    return;
  }
  if (!is_action_available(ch, atSWIFT, TRUE))
    return;
  CI_READY(ch, CI_RALLY) = time(NULL) + 30;
  CI_STATE(ally).rally_bonus =
      MAX(CI_STATE(ally).rally_until > time(NULL) ? CI_STATE(ally).rally_bonus : 0,
          CLASS_LEVEL(ch, CLASS_BARD) >= 15 ? 3 : 2);
  CI_STATE(ally).rally_until = time(NULL) + 2 * CI_ROUND_SECONDS;
  USE_SWIFT_ACTION(ch);
  act("Your rallying verse strengthens $N's resolve!", FALSE, ch, NULL, ally, TO_CHAR);
  act("$n's rallying verse strengthens your resolve!", FALSE, ch, NULL, ally, TO_VICT);
}

ACMD(do_opening)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *vict;
  one_argument(argument, arg, sizeof(arg));
  vict = *arg ? get_char_room_vis(ch, arg, NULL) : FIGHTING(ch);
  if (!class_duelist_gear(ch) || CLASS_LEVEL(ch, CLASS_DUELIST) < 8 || !vict ||
      vict != FIGHTING(ch) || IN_ROOM(ch) != IN_ROOM(vict) ||
      !combat_attack_in_range(ch, vict, ATTACK_TYPE_PRIMARY,
                              get_wielded(ch, ATTACK_TYPE_PRIMARY)) ||
      ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL) || MOB_FLAGGED(vict, MOB_NOKILL) ||
      !is_mission_mob(ch, vict))
  {
    send_to_char(ch, "Find an opening against your current melee opponent with a free off hand "
                     "(Duelist 8).\r\n");
    return;
  }
  if (CI_READY(ch, CI_OPENING) > time(NULL))
  {
    send_to_char(ch, "You must wait before exploiting another opening.\r\n");
    return;
  }
  if (!is_action_available(ch, atSWIFT, TRUE))
    return;
  CI_READY(ch, CI_OPENING) = time(NULL) + 18;
  CI_STATE(ch).opening_until = time(NULL) + 2 * CI_ROUND_SECONDS;
  CI_STATE(ch).opening_target = GET_ID(vict);
  USE_SWIFT_ACTION(ch);
  act("You spot an opening in $N's defense!", FALSE, ch, NULL, vict, TO_CHAR);
}

int class_opening_begin(struct char_data *ch, struct char_data *vict, int type, int attack_type)
{
  int best;
  if (!ch || IS_NPC(ch) || reaction_depth || ch->epic_reaction_depth ||
      ch->epic_attack_depth != 1 || type != TYPE_UNDEFINED || attack_type != ATTACK_TYPE_PRIMARY ||
      !class_duelist_gear(ch) || CLASS_LEVEL(ch, CLASS_DUELIST) < 8 ||
      CI_STATE(ch).opening_until <= time(NULL) || CI_STATE(ch).opening_target != GET_ID(vict))
    return 0;
  CI_STATE(ch).opening_until = 0;
  CI_STATE(ch).opening_target = 0;
  CI_STATE(ch).opening_attack_target = GET_ID(vict);
  best = typed_bonus(ch, APPLY_HITROLL, BONUS_TYPE_INSIGHT);
  return MAX(0, 2 - MAX(0, best));
}

int class_opening_accuracy(struct char_data *ch, struct char_data *vict, int insight)
{
  if (!vict || reaction_depth || CI_STATE(ch).opening_attack_target != GET_ID(vict))
    return 0;
  return MAX(0, 2 - MAX(insight, typed_bonus(ch, APPLY_HITROLL, BONUS_TYPE_INSIGHT)));
}

int class_opening_damage(struct char_data *ch, struct char_data *vict)
{
  if (CI_STATE(ch).opening_attack_target != GET_ID(vict) || reaction_depth)
    return 0;
  CI_STATE(ch).opening_attack_target = 0;
  return dice(1, 6);
}

int class_seam_damage(struct char_data *ch, struct char_data *vict, int attack_type)
{
  if (!class_duelist_gear(ch) || CLASS_LEVEL(ch, CLASS_DUELIST) < 6 ||
      !HAS_FEAT(ch, FEAT_PRECISE_STRIKE) || attack_type == ATTACK_TYPE_RANGED ||
      !CI_STATE(ch).precision_blocked)
    return 0;
  CI_STATE(ch).precision_blocked = FALSE;
  return MIN(5, CLASS_LEVEL(ch, CLASS_DUELIST) / 2);
}

bool class_measured_parry(struct char_data *ch, struct char_data *vict, int roll, bool critical,
                          int attack_type)
{
  struct obj_data *weapon;
  long attacker_id, defender_id;
  int previous_attacks;
  if (reaction_depth || ch->epic_reaction_depth || vict->epic_reaction_depth ||
      ch->epic_attack_depth != 1 || critical || attack_type == ATTACK_TYPE_RANGED ||
      !class_duelist_gear(vict) || CLASS_LEVEL(vict, CLASS_DUELIST) < 5 || TOTAL_DEFENSE(vict) ||
      AFF_FLAGGED(vict, AFF_TOTAL_DEFENSE) || CI_READY(vict, CI_PARRY) > time(NULL))
    return FALSE;
  weapon = get_wielded(ch, attack_type);
  if (!weapon || GET_OBJ_TYPE(weapon) != ITEM_WEAPON ||
      !combat_attack_in_range(vict, ch, ATTACK_TYPE_PRIMARY,
                              get_wielded(vict, ATTACK_TYPE_PRIMARY)))
    return FALSE;
  CI_READY(vict, CI_PARRY) = time(NULL) + CI_ROUND_SECONDS;
  if (d20(vict) + CLASS_LEVEL(vict, CLASS_DUELIST) + GET_DEX_BONUS(vict) <= roll)
    return FALSE;
  act("You parry $N's attack!", FALSE, vict, NULL, ch, TO_CHAR);
  act("$n parries your attack!", FALSE, vict, NULL, ch, TO_VICT);
  if (HAS_FEAT(vict, FEAT_RIPOSTE))
  {
    attacker_id = GET_ID(ch);
    defender_id = GET_ID(vict);
    previous_attacks = GET_ATTACKS_THIS_ROUND(vict);
    counter_pending = TRUE;
    reaction_depth++;
    ch->epic_reaction_depth++;
    vict->epic_reaction_depth++;
    hit(vict, ch, TYPE_UNDEFINED, DAM_RESERVED_DBC, -2, ATTACK_TYPE_PRIMARY);
    if (find_char(attacker_id) == ch)
      ch->epic_reaction_depth--;
    if (find_char(defender_id) == vict)
    {
      vict->epic_reaction_depth--;
      GET_ATTACKS_THIS_ROUND(vict) = previous_attacks;
    }
    counter_pending = FALSE;
    reaction_depth--;
  }
  return TRUE;
}

ACMD(do_intercept)
{
  if (!class_feature_ready(ch) || CLASS_LEVEL(ch, CLASS_STALWART_DEFENDER) < 6)
  {
    send_to_char(ch, "Intercepting Guard requires Stalwart Defender 6.\r\n");
    return;
  }
  CI_STATE(ch).intercept = !CI_STATE(ch).intercept;
  send_to_char(ch, "Intercepting Guard %s for your guarded groupmate.\r\n",
               CI_STATE(ch).intercept ? "enabled" : "disabled");
}

struct char_data *class_intercept(struct char_data *ch, struct char_data *vict)
{
  struct char_data *guard;
  if (reaction_depth || ch->epic_reaction_depth || ch->epic_attack_depth != 1)
    return vict;
  for (guard = world[IN_ROOM(vict)].people; guard; guard = guard->next_in_room)
    if (guard != ch && !IS_NPC(guard) && CLASS_LEVEL(guard, CLASS_STALWART_DEFENDER) >= 6 &&
        CI_STATE(guard).intercept && CI_READY(guard, CI_INTERCEPT) <= time(NULL) &&
        guarded(guard, vict))
    {
      CI_READY(guard, CI_INTERCEPT) = time(NULL) + 3 * CI_ROUND_SECONDS;
      act("You intercept an attack meant for $N!", FALSE, guard, NULL, vict, TO_CHAR);
      act("$n intercepts an attack meant for you!", FALSE, guard, NULL, vict, TO_VICT);
      return guard;
    }
  return vict;
}

void class_recovery_start(struct char_data *ch)
{
  if (!class_feature_ready(ch) || CLASS_LEVEL(ch, CLASS_STALWART_DEFENDER) < 8)
    return;
  CI_STATE(ch).recovery_hp = MIN(20, 2 * CLASS_LEVEL(ch, CLASS_STALWART_DEFENDER));
  CI_STATE(ch).recovery_until = time(NULL) + 3 * CI_ROUND_SECONDS;
  send_to_char(ch, "Shielded Recovery grants a %d-point weapon damage buffer.\r\n",
               CI_STATE(ch).recovery_hp);
}

int class_recovery_absorb(struct char_data *vict, int dam, int w_type, int attack_type,
                          unsigned int origin)
{
  int absorb;
  if (!class_feature_ready(vict) || CLASS_LEVEL(vict, CLASS_STALWART_DEFENDER) < 8 || origin ||
      w_type < TYPE_HIT || w_type >= TYPE_HIT + NUM_ATTACK_TYPES ||
      attack_type == ATTACK_TYPE_PSIONICS || attack_type == ATTACK_TYPE_ELDRITCH_BLAST ||
      CI_STATE(vict).recovery_until <= time(NULL))
    return dam;
  absorb = MIN(MAX(0, dam), CI_STATE(vict).recovery_hp);
  CI_STATE(vict).recovery_hp -= absorb;
  if (absorb)
    send_to_char(vict, "Shielded Recovery absorbs %d damage (%d left).\r\n", absorb,
                 CI_STATE(vict).recovery_hp);
  return dam - absorb;
}

bool class_revelation(struct char_data *ch, struct char_data *vict)
{
  struct char_data *bard, *best = NULL;
  int dc, best_dc = 0, i, song;
  time_t now = time(NULL);
  if (!ch || !vict || IN_ROOM(ch) == NOWHERE || IN_ROOM(ch) != IN_ROOM(vict) ||
      (FIGHTING(ch) != vict && FIGHTING(vict) != ch) || !CAN_SEE(ch, vict) ||
      AFF_FLAGGED(vict, AFF_BLINKING))
    return FALSE;
  for (bard = world[IN_ROOM(ch)].people; bard; bard = bard->next_in_room)
    if (!IS_NPC(bard) && CLASS_LEVEL(bard, CLASS_BARD) >= 11 && same_group(bard, ch) &&
        !AFF_FLAGGED(ch, AFF_DEAF))
      for (i = 0; i < 2; i++)
      {
        song = i ? GET_PERFORMANCE_VAR(bard, 2) : GET_PERFORMING(bard);
        if (active_song(bard, i) &&
            performance_info[song][PERFORMANCE_SKILLNUM] == SKILL_SONG_OF_REVELATION &&
            (dc = 10 + CLASS_LEVEL(bard, CLASS_BARD) / 2 + GET_CHA_BONUS(bard)) > best_dc)
        {
          best = bard;
          best_dc = dc;
        }
      }
  if (!best)
    return FALSE;
  /* One save per target per native combat interval, across all bards. */
  if (CI_STATE(vict).revelation_check <= now)
  {
    CI_STATE(vict).revelation_check = now + CI_ROUND_SECONDS;
    CI_STATE(vict).revealed_until = 0;
    CI_STATE(vict).revealing_group = GET_ID(best);
    if (!mag_resistance(best, vict, 0) &&
        !savingthrow_flatdc(best, vict, SAVING_WILL, 0, best_dc, CAST_INNATE, DIVINATION,
                            SKILL_SONG_OF_REVELATION))
      CI_STATE(vict).revealed_until = now + CI_ROUND_SECONDS;
  }
  bard = find_char(CI_STATE(vict).revealing_group);
  return CI_STATE(vict).revealed_until > now && bard && same_group(bard, ch) &&
         IN_ROOM(bard) == IN_ROOM(ch);
}

void class_features_depart(struct char_data *ch)
{
  struct char_data *other;
  CI_STATE(ch).opening_until = CI_STATE(ch).opening_target = CI_STATE(ch).opening_attack_target = 0;
  if (IN_ROOM(ch) == NOWHERE)
    return;
  for (other = world[IN_ROOM(ch)].people; other; other = other->next_in_room)
    if (CI_STATE(other).opening_target == GET_ID(ch))
      CI_STATE(other).opening_until = CI_STATE(other).opening_target = 0;
}

ACMD(do_classfeatures)
{
  int level;
  if (IS_NPC(ch))
    return;
  send_to_char(ch, "Class features (actual class ranks):\r\n");
  if ((level = CLASS_LEVEL(ch, CLASS_BARD)) > 0)
    send_to_char(ch, "Bard %d: Battle Rhythm %s; Rallying Verse %s. Use perform status.\r\n", level,
                 class_rhythm_bonus(ch, APPLY_HITROLL) > 0 ? "active" : "inactive or superseded",
                 level < 7                             ? "requires Bard 7"
                 : CI_READY(ch, CI_RALLY) > time(NULL) ? "recovering"
                                                       : "ready");
  if ((level = CLASS_LEVEL(ch, CLASS_DUELIST)) > 0)
  {
    send_to_char(
        ch, "  Combat readiness: %s; casting: %s; melee dueling gear: %s; total defense: %s.\r\n",
        class_feature_ready(ch) ? "ready" : "position or condition prevents reactions",
        IS_CASTING(ch) ? "yes" : "no", class_duelist_gear(ch) ? "eligible" : "ineligible",
        AFF_FLAGGED(ch, AFF_TOTAL_DEFENSE) || TOTAL_DEFENSE(ch) ? "active" : "off");
    send_to_char(
        ch,
        "Duelist %d: Canny Defense/Precise Strike require light or no armor and a free hand.\r\n"
        "  Current armor/free hand: %s/%s. Precise Strike normally fails against critical "
        "immunity.\r\n"
        "  Measured Parry requires Duelist 5, a melee weapon, no casting, and no total defense: "
        "%s.\r\n"
        "  Find the Seam: %s. Opening: %s.\r\n",
        level, compute_gear_armor_type(ch) <= ARMOR_TYPE_LIGHT ? "eligible" : "too heavy",
        HAS_FREE_HAND(ch) ? "free" : "occupied",
        level >= 5 && class_duelist_gear(ch) && !AFF_FLAGGED(ch, AFF_TOTAL_DEFENSE) &&
                !TOTAL_DEFENSE(ch)
            ? (CI_READY(ch, CI_PARRY) > time(NULL) ? "recovering" : "ready")
            : "inactive",
        level >= 6 ? "available against critical immunity" : "requires Duelist 6",
        level < 8                               ? "requires Duelist 8"
        : CI_READY(ch, CI_OPENING) > time(NULL) ? "recovering"
                                                : "ready");
  }
  if ((level = CLASS_LEVEL(ch, CLASS_STALWART_DEFENDER)) > 0)
    send_to_char(ch,
                 "Stalwart Defender %d: stance %s; guarded companion %s.\r\n"
                 "  Intercept %s (%ld seconds recovery); Shielded Recovery buffer %d.\r\n",
                 level, affected_by_spell(ch, SKILL_DEFENSIVE_STANCE) ? "active" : "inactive",
                 GUARDING(ch) && guarded(ch, GUARDING(ch)) ? "eligible" : "inactive",
                 CI_STATE(ch).intercept ? "enabled" : "disabled",
                 (long)MAX(0, CI_READY(ch, CI_INTERCEPT) - time(NULL)),
                 CI_STATE(ch).recovery_until > time(NULL) ? CI_STATE(ch).recovery_hp : 0);
}
