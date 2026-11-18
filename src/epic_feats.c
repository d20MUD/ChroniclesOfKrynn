/* The first catalogue release. Feat ownership is separate from eligibility:
 * runtime helpers suspend benefits when permanent prerequisites are lost. */
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "spells.h"
#include "spell_prep.h"
#include "psionics.h"
#include "feats.h"
#include "class.h"
#include "mud_event.h"
#include "actions.h"
#include "fight.h"
#include "crafts.h"
#include "bloodlines.h"
#include "epic_feats.h"
#include "epic_feats2.h"
#include "epic_feats3.h"
void save_char_pets(struct char_data *ch);

bool is_expansion_epic(int feat)
{
  return feat == FEAT_EPIC_DISCIPLINE_FOCUS || (feat >= FEAT_EPIC_POWER_KNOWLEDGE && feat <= FEAT_RUINOUS_RAGE) || epic_second_feat(feat) || epic_third_feat(feat);
}

int epic_feat_class(int feat)
{
  if (feat == FEAT_EPIC_DISCIPLINE_FOCUS) return CLASS_PSIONICIST;
  if (epic_third_feat(feat)) return epic_third_class(feat);
  if (epic_second_feat(feat)) return epic_second_class(feat);
  if (feat >= FEAT_EPIC_POWER_KNOWLEDGE && feat <= FEAT_EPIC_MANIFESTING_POISE)
    return CLASS_PSIONICIST;
  if (feat >= FEAT_EPIC_ARTISANS_INSIGHT && feat <= FEAT_EPIC_ANCHORED_CHASSIS)
    return CLASS_ARTIFICER;
  if (feat >= FEAT_EPIC_EVOLUTION_RESERVE && feat <= FEAT_EPIC_PLANAR_ANCHORAGE)
    return CLASS_SUMMONER;
  if (feat == FEAT_RUINOUS_RAGE)
    return CLASS_BERSERKER;
  return -1;
}

int epic_capacity_circle(int class)
{
  switch (class)
  {
  case CLASS_WIZARD:
  case CLASS_SORCERER:
  case CLASS_CLERIC:
  case CLASS_DRUID:
    return 9;
  case CLASS_BARD:
  case CLASS_INQUISITOR:
  case CLASS_SUMMONER:
  case CLASS_ALCHEMIST:
    return 6;
  case CLASS_PALADIN:
  case CLASS_BLACKGUARD:
  case CLASS_RANGER:
    return 4;
  default:
    return 0;
  }
}

bool epic_capacity_choice(struct char_data *ch, int class)
{
  int cap = epic_capacity_circle(class);
  return cap && CLASS_LEVEL(ch, class) > 0 && get_class_highest_circle(ch, class) >= cap;
}

static int owned(struct char_data *ch, int feat, bool study)
{
  return study ? has_feat_requirement_check(ch, feat) : HAS_REAL_FEAT(ch, feat);
}

static bool prerequisites(struct char_data *ch, int feat, bool study)
{
  int class, count = 0;
  if (!ch || IS_NPC(ch) || GET_LEVEL(ch) < 21 || !is_expansion_epic(feat))
    return FALSE;
  if (epic_third_feat(feat)) return epic_third_prerequisites(ch,feat,study);
  if (epic_second_feat(feat)) return epic_second_prerequisites(ch, feat, study);
  class = epic_feat_class(feat);
  if (class >= 0 && CLASS_LEVEL(ch, class) < 20)
    return FALSE;
#define F(f) owned(ch, (f), study)
#define R(a, n) (GET_ABILITY(ch, (a)) >= (n))
  switch (feat)
  {
  case FEAT_EPIC_DISCIPLINE_FOCUS:
    return GET_REAL_INT(ch) >= 23 && R(ABILITY_SPELLCRAFT, 20);
  case FEAT_EPIC_POWER_KNOWLEDGE:
    return GET_REAL_INT(ch) >= 21 && R(ABILITY_SPELLCRAFT, 20) &&
           get_class_highest_circle(ch, CLASS_PSIONICIST) >= 9;
  case FEAT_EPIC_AUGMENTATION_EFFICIENCY:
    return GET_REAL_INT(ch) >= 21 && F(FEAT_EXPERT_AUGMENTING);
  case FEAT_EPIC_FOCUS_CONTINUITY:
    return F(FEAT_PSIONIC_FOCUS) && R(ABILITY_CONCENTRATION, 20);
  case FEAT_EPIC_MENTAL_BASTION:
    return GET_REAL_WIS(ch) >= 19 && R(ABILITY_CONCENTRATION, 20);
  case FEAT_EPIC_MANIFESTING_POISE:
    return F(FEAT_COMBAT_MANIFESTATION) && R(ABILITY_CONCENTRATION, 20);
  case FEAT_EPIC_ARTISANS_INSIGHT:
    return GET_REAL_INT(ch) >= 23 && F(FEAT_CRAFT_WONDEROUS_ITEM) && R(ABILITY_ARCANA, 20);
  case FEAT_EPIC_EFFICIENT_ITEM_CREATION:
    return CLASS_LEVEL(ch, CLASS_ARTIFICER) >= 23 && R(ABILITY_ARCANA, 23) &&
           (F(FEAT_CRAFT_WONDEROUS_ITEM) || F(FEAT_CRAFT_MAGICAL_ARMS_AND_ARMOR));
  case FEAT_EPIC_METAMAGIC_SCIENCE:
    count += !!F(FEAT_EMPOWER_SPELL);
    count += !!F(FEAT_ENLARGE_SPELL);
    count += !!F(FEAT_EXTEND_SPELL);
    count += !!F(FEAT_HEIGHTEN_SPELL);
    count += !!F(FEAT_MAXIMIZE_SPELL);
    count += !!F(FEAT_QUICKEN_SPELL);
    count += !!F(FEAT_SILENT_SPELL);
    count += !!F(FEAT_STILL_SPELL);
    return CLASS_LEVEL(ch, CLASS_ARTIFICER) >= 23 && F(FEAT_METAMAGIC_SCIENCE) &&
           R(ABILITY_USE_MAGIC_DEVICE, 23) && count >= 3;
  case FEAT_EPIC_DEVICE_PENETRATION:
    return F(FEAT_WEIRD_SCIENCE) && F(FEAT_SPELL_PENETRATION) &&
           F(FEAT_GREATER_SPELL_PENETRATION) && R(ABILITY_USE_MAGIC_DEVICE, 20);
  case FEAT_EPIC_ANCHORED_CHASSIS:
    return F(FEAT_CONSTRUCT_STONE_GOLEM) && GET_REAL_INT(ch) >= 21 &&
           CONFIG_CRAFTING_SYSTEM == CRAFTING_SYSTEM_MOTES;
  case FEAT_EPIC_EVOLUTION_RESERVE:
    return GET_REAL_CHA(ch) >= 21 && F(FEAT_EIDOLON);
  case FEAT_EPIC_ADAPTIVE_EIDOLON_WARD:
  case FEAT_EPIC_PLANAR_ANCHORAGE:
    return GET_REAL_CHA(ch) >= 21 && R(ABILITY_SPELLCRAFT, 20);
  case FEAT_EPIC_FORTITUDE:
    return GET_REAL_CON(ch) >= 21 && F(FEAT_GREAT_FORTITUDE);
  case FEAT_EPIC_REFLEXES:
    return GET_REAL_DEX(ch) >= 21 && F(FEAT_LIGHTNING_REFLEXES);
  case FEAT_EPIC_WILL:
    return GET_REAL_WIS(ch) >= 21 && F(FEAT_IRON_WILL);
  case FEAT_SUPERIOR_INITIATIVE:
    return GET_REAL_DEX(ch) >= 21 && F(FEAT_IMPROVED_INITIATIVE);
  case FEAT_LEGENDARY_ATHLETE:
    return GET_REAL_STR(ch) >= 19 && GET_REAL_CON(ch) >= 19 && F(FEAT_ENDURANCE) &&
           R(ABILITY_ATHLETICS, 20);
  case FEAT_IMPROVED_SPELL_CAPACITY:
    if (!R(ABILITY_SPELLCRAFT, 20))
      return FALSE;
    for (class = 0; class < NUM_CLASSES; class++)
      if (epic_capacity_choice(ch, class))
        return TRUE;
    return FALSE;
  case FEAT_RUINOUS_RAGE:
    return GET_REAL_STR(ch) >= 23 && F(FEAT_GREATER_RAGE) && F(FEAT_POWER_ATTACK);
  }
#undef F
#undef R
  return FALSE;
}

bool epic_feat_available(struct char_data *ch, int feat)
{
  int rank;
  if (!prerequisites(ch, feat, TRUE))
    return FALSE;
  rank = owned(ch, feat, TRUE);
  if (feat == FEAT_EPIC_POWER_KNOWLEDGE)
    return rank < 3 && GET_LEVEL(ch) >= 21 + 3 * rank;
  if (feat == FEAT_EPIC_DISCIPLINE_FOCUS || feat == FEAT_EPIC_EVOLUTION_RESERVE || feat == FEAT_IMPROVED_SPELL_CAPACITY)
    return rank < 2 && (!rank || GET_LEVEL(ch) >= 27);
  if (feat == FEAT_EPIC_INSPIRATION)
    return rank < 2 && (!rank || (GET_LEVEL(ch)>=27 && CLASS_LEVEL(ch,CLASS_BARD)>=23));
  return !rank;
}

int epic_study_rank(struct char_data *ch, int feat)
{
  int rank;
  if (!prerequisites(ch, feat, TRUE))
    return 0;
  rank = owned(ch, feat, TRUE);
  if (feat == FEAT_EPIC_POWER_KNOWLEDGE)
    return MIN(MIN(rank, 3), 1 + (GET_LEVEL(ch) - 21) / 3);
  if (feat == FEAT_EPIC_DISCIPLINE_FOCUS || feat == FEAT_EPIC_EVOLUTION_RESERVE || feat == FEAT_IMPROVED_SPELL_CAPACITY)
    return MIN(rank, GET_LEVEL(ch) >= 27 ? 2 : 1);
  if (feat == FEAT_EPIC_INSPIRATION) return MIN(rank, GET_LEVEL(ch)>=27 && CLASS_LEVEL(ch,CLASS_BARD)>=23 ? 2 : 1);
  return MIN(rank, 1);
}

int epic_feat_rank(struct char_data *ch, int feat)
{
  int rank;
  if (!prerequisites(ch, feat, FALSE))
    return 0;
  if (feat == FEAT_EPIC_EFFICIENT_ITEM_CREATION &&
      ((EPIC_CHOICES(ch).craft_category == 1 && !HAS_REAL_FEAT(ch, FEAT_CRAFT_WONDEROUS_ITEM)) ||
       (EPIC_CHOICES(ch).craft_category == 2 &&
        !HAS_REAL_FEAT(ch, FEAT_CRAFT_MAGICAL_ARMS_AND_ARMOR))))
    return 0;
  rank = HAS_REAL_FEAT(ch, feat);
  if (feat == FEAT_EPIC_POWER_KNOWLEDGE)
    return MIN(MIN(rank, 3), 1 + (GET_LEVEL(ch) - 21) / 3);
  if (feat == FEAT_EPIC_DISCIPLINE_FOCUS || feat == FEAT_EPIC_EVOLUTION_RESERVE || feat == FEAT_IMPROVED_SPELL_CAPACITY)
    return MIN(rank, GET_LEVEL(ch) >= 27 ? 2 : 1);
  if (feat == FEAT_EPIC_INSPIRATION) return MIN(rank, GET_LEVEL(ch)>=27 && CLASS_LEVEL(ch,CLASS_BARD)>=23 ? 2 : 1);
  return MIN(rank, 1);
}

int epic_augmentation_discount(struct char_data *ch, int spell, int augmentation)
{
  if (spell < PSIONIC_POWER_START || spell >= PSIONIC_IMPALE_MIND ||
      !epic_feat_rank(ch, FEAT_EPIC_AUGMENTATION_EFFICIENCY))
    return 0;
  return MIN(2, MAX(0, augmentation));
}

bool epic_power_choice(struct char_data *ch, int power)
{
  int i, circle;
  if (power < PSIONIC_POWER_START || power > PSIONIC_POWER_END || power >= PSIONIC_IMPALE_MIND ||
      power == PSIONIC_RAZOR_STORM || power == PSIONIC_PSYCHOKINETIC_THRASHING ||
      power == PSIONIC_EPIC_PSIONIC_WARD ||
      spell_info[power].min_level[CLASS_PSIONICIST] >= LVL_IMMORT ||
      psionic_powers[power].psp_cost <= 0 || is_a_known_spell(ch, CLASS_PSIONICIST, power))
    return FALSE;
  circle = compute_powers_circle(CLASS_PSIONICIST, power, METAMAGIC_NONE);
  if (circle < 1 || circle > get_class_highest_circle(ch, CLASS_PSIONICIST))
    return FALSE;
  if (ch->desc && LEVELUP(ch))
  {
    if (LEVELUP(ch)->spells_learned[power])
      return FALSE;
    for (i = 0; i < 6; i++)
      if (LEVELUP(ch)->epic_choices.powers[i] == power)
        return FALSE;
  }
  return TRUE;
}

/* Return only the improvement over an existing typed affect/equipment bonus. */
int epic_typed_increment(struct char_data *ch, int location, int specific, int type, int amount)
{
  struct affected_type *af;
  int best = 0, i, j;
  for (af = ch->affected; af; af = af->next)
    if (af->location == location && af->bonus_type == type &&
        (location != APPLY_SKILL || af->specific == specific))
      best = MAX(best, af->modifier);
  for (i = 0; i < NUM_WEARS; i++)
    if (GET_EQ(ch, i))
      for (j = 0; j < 6; j++)
        if (GET_EQ(ch, i)->affected[j].location == location &&
            GET_EQ(ch, i)->affected[j].bonus_type == type &&
            (location != APPLY_SKILL || GET_EQ(ch, i)->affected[j].specific == specific))
          best = MAX(best, GET_EQ(ch, i)->affected[j].modifier);
  return MAX(0, amount - best);
}

int epic_artisan_roll(struct char_data *ch, int roll, int modifier, int dc, bool criticals)
{
  if (epic_feat_rank(ch, FEAT_EPIC_ARTISANS_INSIGHT) && EPIC_TIMERS(ch).armed[0] > time(NULL) &&
      ((criticals && roll == 1) || ((!criticals || roll != 20) && roll + modifier < dc)))
  {
    EPIC_TIMERS(ch).armed[0] = 0;
    save_char(ch, 0);
    roll = d20(ch);
    send_to_char(ch, "Epic Artisan's Insight: rerolled %d + %d against DC %d.\r\n", roll, modifier,
                 dc);
  }
  return roll;
}

int epic_craft_seconds(struct char_data *ch, int base, int other_seconds, int category,
                       bool efficient)
{
  int reduction = MAX(0, other_seconds), pct = efficient ? 10 : 0;
  if (epic_feat_rank(ch, FEAT_EPIC_EFFICIENT_ITEM_CREATION) && category &&
      EPIC_CHOICES(ch).craft_category == category)
  {
    pct += 20;
    reduction = MIN(base / 2, reduction + (base * pct) / 100);
    return MAX(1, base - reduction);
  }
  return MAX(1, ((base - reduction) * (100 - pct)) / 100);
}

int epic_trigger_cost(struct char_data *ch, int charges, bool commit)
{
  if (charges > 1 && epic_feat_rank(ch, FEAT_EPIC_METAMAGIC_SCIENCE) &&
      EPIC_TIMERS(ch).armed[1] > time(NULL))
  {
    if (commit)
    {
      EPIC_TIMERS(ch).armed[1] = 0;
      save_char(ch, 0);
    }
    return charges - 1;
  }
  return charges;
}

void epic_device_begin(struct char_data *ch)
{
  ch->epic_device_check = FALSE;
  ch->epic_device_depth = ch->epic_spell_depth + 1;
  if (epic_feat_rank(ch, FEAT_EPIC_DEVICE_PENETRATION) && EPIC_TIMERS(ch).armed[2] > time(NULL))
  {
    ch->epic_device_check = TRUE;
    EPIC_TIMERS(ch).armed[2] = 0;
    save_char(ch, 0);
  }
}
void epic_device_end(struct char_data *ch) { ch->epic_device_check = FALSE; }
int epic_device_penetration(struct char_data *ch)
{
  if (!ch || !ch->epic_device_check || ch->epic_spell_depth != ch->epic_device_depth ||
      ch->epic_cast_context != CAST_DEVICE)
    return 0;
  ch->epic_device_check = FALSE;
  return 4;
}

static bool mind_control(int spell)
{
  return is_fear_spell(spell) || spell == SPELL_FEAR || spell == SPELL_SCARE ||
         spell == SPELL_DRAGONFEAR || spell == PSIONIC_DEMORALIZE || spell == PSIONIC_DEADLY_FEAR ||
         spell == PSIONIC_MOMENT_OF_TERROR || spell == PSIONIC_PSYCHOSIS ||
         spell == SPELL_SCINT_PATTERN || spell == AFFECT_AURA_OF_TERROR ||
         spell == EVOLUTION_FRIGHTFUL_EFFECT || spell == WARLOCK_FRIGHTFUL_BLAST ||
         spell == WARLOCK_BEWITCHING_BLAST || spell == SPELL_CHARM ||
         spell == SPELL_CHARM_MONSTER || spell == SPELL_CHARM_ANIMAL ||
         spell == SPELL_DOMINATE_PERSON || spell == SPELL_MASS_DOMINATION ||
         spell == SPELL_CONFUSION || spell == ABILITY_VAMPIRIC_DOMINATION || spell == WARLOCK_CHARM;
}
int epic_mind_save_bonus(struct char_data *ch, int save, int spell)
{
  static const int locs[] = {APPLY_SAVING_FORT, APPLY_SAVING_REFL, APPLY_SAVING_WILL,
                             APPLY_SAVING_POISON, APPLY_SAVING_DEATH};
  int loc, flash = 0;
  if (!epic_feat_rank(ch, FEAT_EPIC_MENTAL_BASTION) || !mind_control(spell))
    return 0;
  /* Save constants are not assumed contiguous/in this array's order. */
  loc = save == SAVING_FORT     ? locs[0]
        : save == SAVING_REFL   ? locs[1]
        : save == SAVING_WILL   ? locs[2]
        : save == SAVING_POISON ? locs[3]
                                : locs[4];
  if (ch->player_specials->saved.flash_insight_expires > time(NULL) &&
      (!ch->player_specials->saved.flash_insight_survival_only || save == SAVING_POISON ||
       save == SAVING_DEATH))
    flash = MAX(0, ch->player_specials->saved.flash_insight_bonus);
  return MAX(0, epic_typed_increment(ch, loc, 0, BONUS_TYPE_INSIGHT, 4) - flash -
                    bloodline_affect_bonus(ch, AFFECT_TOUCH_OF_DESTINY));
}

static struct char_data *epic_owner(struct char_data *pet)
{
  if (!pet || !IS_NPC(pet) || !pet->master || IS_NPC(pet->master) || !AFF_FLAGGED(pet, AFF_CHARM))
    return NULL;
  return pet->master;
}
static bool primary_eidolon(struct char_data *pet)
{
  return epic_owner(pet) && MOB_FLAGGED(pet, MOB_EIDOLON) && !char_has_mud_event(pet, ePURGEMOB);
}
int epic_planar_save_bonus(struct char_data *ch, int save, int spell)
{
  struct char_data *owner = epic_owner(ch);
  int loc;
  if (!owner || !epic_feat_rank(owner, FEAT_EPIC_PLANAR_ANCHORAGE) ||
      !(primary_eidolon(ch) || is_summon_creature_mob(GET_MOB_VNUM(ch))))
    return 0;
  if (!(spell == SPELL_DISMISSAL || spell == SPELL_BANISH || spell == SPELL_CHARM ||
        spell == SPELL_CHARM_MONSTER || spell == SPELL_CHARM_ANIMAL ||
        spell == SPELL_DOMINATE_PERSON || spell == SPELL_MASS_DOMINATION ||
        spell == ABILITY_VAMPIRIC_DOMINATION || spell == WARLOCK_CHARM))
    return 0;
  loc = save == SAVING_WILL     ? APPLY_SAVING_WILL
        : save == SAVING_FORT   ? APPLY_SAVING_FORT
        : save == SAVING_REFL   ? APPLY_SAVING_REFL
        : save == SAVING_POISON ? APPLY_SAVING_POISON
                                : APPLY_SAVING_DEATH;
  return epic_typed_increment(ch, loc, 0, BONUS_TYPE_CIRCUMSTANCE, 4);
}
int epic_chassis_bonus(struct char_data *ch, int maneuver)
{
  struct char_data *owner = epic_owner(ch);
  if (!owner || !IS_GOLEM(ch) || !epic_feat_rank(owner, FEAT_EPIC_ANCHORED_CHASSIS))
    return 0;
  if (maneuver != COMBAT_MANEUVER_TYPE_FORCED_MOVEMENT &&
      maneuver != COMBAT_MANEUVER_TYPE_KNOCKDOWN)
    return 0;
  return 4;
}
void epic_snapshot_ward(struct char_data *ch)
{
  struct follow_type *f;
  struct affected_type *af;
  if (!ch || IS_NPC(ch))
    return;
  for (f = ch->followers; f; f = f->next)
    if (primary_eidolon(f->follower))
    {
      EPIC_TIMERS(ch).ward_expiry = 0;
      for (af = f->follower->affected; af; af = af->next)
        if (af->spell == AFFECT_EPIC_EIDOLON_WARD && af->legendary_expiry > time(NULL))
        {
          EPIC_TIMERS(ch).ward_expiry = af->legendary_expiry;
          EPIC_TIMERS(ch).ward_type = af->specific;
        }
      return;
    }
  /* No live primary: don't erase the snapshot while restoring/extracting pets. */
}

void epic_restore_ward(struct char_data *ch, struct char_data *pet)
{
  struct affected_type af;
  time_t remaining;
  if (!primary_eidolon(pet) || !epic_feat_rank(ch, FEAT_EPIC_ADAPTIVE_EIDOLON_WARD))
    return;
  remaining = EPIC_TIMERS(ch).ward_expiry - time(NULL);
  if (remaining <= 0 || remaining > 60)
    return;
  new_affect(&af);
  af.spell = AFFECT_EPIC_EIDOLON_WARD;
  af.location = APPLY_SPECIAL;
  af.modifier = 20;
  af.specific = EPIC_TIMERS(ch).ward_type;
  af.duration = (remaining + 5) / 6;
  af.legendary_expiry = EPIC_TIMERS(ch).ward_expiry;
  affect_to_char(pet, &af);
}

int epic_eidolon_resistance(struct char_data *ch, int damtype)
{
  struct affected_type *af;
  if (!primary_eidolon(ch) || !epic_feat_rank(ch->master, FEAT_EPIC_ADAPTIVE_EIDOLON_WARD) ||
      ROOM_AFFECTED(IN_ROOM(ch), RAFF_ANTI_MAGIC))
    return 0;
  for (af = ch->affected; af; af = af->next)
    if (af->spell == AFFECT_EPIC_EIDOLON_WARD && af->specific == damtype &&
        af->legendary_expiry > time(NULL))
      return 20;
  return 0;
}
int epic_rage_penetration(struct char_data *ch, int attack_type)
{
  return (attack_type == ATTACK_TYPE_PRIMARY || attack_type == ATTACK_TYPE_OFFHAND ||
          attack_type == ATTACK_TYPE_TWOHAND || attack_type == ATTACK_TYPE_PRIMARY_SNEAK ||
          attack_type == ATTACK_TYPE_OFFHAND_SNEAK) &&
                 epic_feat_rank(ch, FEAT_RUINOUS_RAGE) && affected_by_spell(ch, SKILL_RAGE)
             ? 5
             : 0;
}

ACMD(do_epic)
{
  char arg[MAX_INPUT_LENGTH], type[MAX_INPUT_LENGTH];
  struct char_data *pet = NULL;
  struct follow_type *f;
  struct affected_type af;
  int feat = 0, slot = -1, cooldown = 0, damtype = 0;
  if (IS_NPC(ch))
    return;
  two_arguments(argument, arg, sizeof(arg), type, sizeof(type));
  if (epic_third_command(ch,arg,argument)) return;
  if (epic_second_command(ch, arg, argument)) return;
  if (!str_cmp(arg, "artisan-insight"))
  {
    feat = FEAT_EPIC_ARTISANS_INSIGHT;
    slot = 0;
    cooldown = 300;
  }
  else if (!str_cmp(arg, "efficient-trigger"))
  {
    feat = FEAT_EPIC_METAMAGIC_SCIENCE;
    slot = 1;
    cooldown = 60;
  }
  else if (!str_cmp(arg, "pierce-device"))
  {
    feat = FEAT_EPIC_DEVICE_PENETRATION;
    slot = 2;
    cooldown = 60;
  }
  else if (!str_cmp(arg, "eidolon-ward"))
  {
    feat = FEAT_EPIC_ADAPTIVE_EIDOLON_WARD;
    slot = 3;
    cooldown = 120;
    if (!str_cmp(type, "acid"))
      damtype = DAM_ACID;
    else if (!str_cmp(type, "cold"))
      damtype = DAM_COLD;
    else if (!str_cmp(type, "electricity"))
      damtype = DAM_ELECTRIC;
    else if (!str_cmp(type, "fire"))
      damtype = DAM_FIRE;
    else if (!str_cmp(type, "sonic"))
      damtype = DAM_SOUND;
    else
    {
      send_to_char(ch, "Choose acid, cold, electricity, fire or sonic.\r\n");
      return;
    }
    for (f = ch->followers; f; f = f->next)
      if (primary_eidolon(f->follower) && IN_ROOM(f->follower) == IN_ROOM(ch) &&
          GET_HIT(f->follower) > 0)
      {
        pet = f->follower;
        break;
      }
    if (!pet)
    {
      send_to_char(ch, "Your primary eidolon must be here and alive.\r\n");
      return;
    }
  }
  else
  {
    send_to_char(ch, "Epic abilities: artisan-insight, efficient-trigger, pierce-device, "
                     "eidolon-ward <energy>; psi-rebuttal, psi-mantle <ally> <power>, "
                     "charge-repair <golem> <wand>, golem-aegis, constructbane <weapon>, "
                     "shared-cast <spell>, conjuration-command <summon>, planar-triage, "
                     "positive-bulwark, great-smite, augment-bomb <slot> <sacrifice>; "
                     "psi-contingency <power>, sentinel-mind, intercession <ally>, "
                     "adaptive-aspect <aspect ID> <replacement>, spell-opportunity <enemy> <spell>, "
                     "stowaway <ally> <spell>, recover <effect number>.\r\n");
    return;
  }
  if (!epic_feat_rank(ch, feat))
  {
    send_to_char(ch, "You do not meet this epic feat's prerequisites.\r\n");
    return;
  }
  if (EPIC_TIMERS(ch).ready[slot] > time(NULL))
  {
    send_to_char(ch, "That epic ability recovers in %ld seconds.\r\n",
                 (long)(EPIC_TIMERS(ch).ready[slot] - time(NULL)));
    return;
  }
  if (slot == 0 && FIGHTING(ch))
  {
    send_to_char(ch, "Artisan's Insight requires you to be out of combat.\r\n");
    return;
  }
  if (slot < 3 && EPIC_TIMERS(ch).armed[slot] > time(NULL))
  {
    send_to_char(ch, "That effect is already armed.\r\n");
    return;
  }
  if (!is_action_available(ch, ACTION_SWIFT, TRUE))
    return;
  EPIC_TIMERS(ch).ready[slot] = time(NULL) + cooldown;
  if (slot < 3)
    EPIC_TIMERS(ch).armed[slot] = time(NULL) + (slot == 0 ? 60 : 18);
  USE_SWIFT_ACTION(ch);
  save_char(ch, 0);
  if (pet)
  {
    EPIC_TIMERS(ch).ward_expiry = time(NULL) + 60;
    EPIC_TIMERS(ch).ward_type = damtype;
    affect_from_char(pet, AFFECT_EPIC_EIDOLON_WARD);
    new_affect(&af);
    af.spell = AFFECT_EPIC_EIDOLON_WARD;
    af.location = APPLY_SPECIAL;
    af.modifier = 20;
    af.specific = damtype;
    af.duration = 10;
    af.legendary_expiry = time(NULL) + 60;
    affect_to_char(pet, &af);
    save_char_pets(ch);
    save_char(ch, 0);
  }
  send_to_char(ch, "%s %s.\r\n", feat_list[feat].name, pet ? "protects your eidolon" : "armed");
}

/* This changes saving throws only, never the independent resistance roll. */
int epic_discipline_dc(struct char_data *ch, int spell)
{
  if (!ch || IS_NPC(ch) || spell < PSIONIC_POWER_START || spell > PSIONIC_POWER_END ||
      psionic_powers[spell].is_epic || EPIC_CHOICES(ch).discipline < 1 ||
      EPIC_CHOICES(ch).discipline >= NUM_PSIONIC_POWER_TYPES ||
      psionic_powers[spell].power_type != EPIC_CHOICES(ch).discipline)
    return 0;
  return epic_feat_rank(ch, FEAT_EPIC_DISCIPLINE_FOCUS);
}
