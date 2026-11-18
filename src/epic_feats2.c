/* Explicit resource commitment and ordinary-attack hooks for catalogue batch two. */
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
#include "db.h"
#include "evolutions.h"
#include "constants.h"
#include "domains_schools.h"
#include "dg_scripts.h"
#include "mob_utils.h"
#include "legendary_effects.h"
#include "legendary_identity.h"
#include "alchemy.h"
#include "epic_feats.h"
#include "epic_feats2.h"

bool epic_second_feat(int feat)
{
  return (feat >= FEAT_EPIC_RESISTANCE_REBUTTAL && feat <= FEAT_EPIC_AUGMENTED_ALCHEMY) ||
         feat == FEAT_GREAT_SMITING;
}
int epic_second_class(int feat)
{
  if (feat <= FEAT_SHARED_PSIONIC_MANTLE && feat >= FEAT_EPIC_RESISTANCE_REBUTTAL)
    return CLASS_PSIONICIST;
  if (feat >= FEAT_EPIC_CHARGE_FED_REPAIR && feat <= FEAT_EPIC_CONSTRUCTBANE_INFUSION)
    return CLASS_ARTIFICER;
  if (feat >= FEAT_EPIC_RESILIENT_BOND && feat <= FEAT_EPIC_EVOLVED_SUMMONS)
    return CLASS_SUMMONER;
  if (feat == FEAT_EPIC_INSPIRATION)
    return CLASS_BARD;
  if (feat == FEAT_EPIC_AUGMENTED_ALCHEMY)
    return CLASS_ALCHEMIST;
  return -1;
}
static bool knows(struct char_data *ch, int spell, int class)
{
  return is_a_known_spell(ch, class, spell);
}
bool epic_second_prerequisites(struct char_data *ch, int feat, bool study)
{
  int cl = epic_second_class(feat);
#define F(f) (study ? has_feat_requirement_check(ch, f) : HAS_REAL_FEAT(ch, f))
#define R(s, n) (GET_ABILITY(ch, s) >= n)
  if (!ch || IS_NPC(ch) || GET_LEVEL(ch) < 21 || (cl >= 0 && CLASS_LEVEL(ch, cl) < 20))
    return FALSE;
  switch (feat)
  {
  case FEAT_EPIC_RESISTANCE_REBUTTAL:
    return F(FEAT_GREATER_POWER_PENETRATION) && R(ABILITY_SPELLCRAFT, 20);
  case FEAT_SHARED_PSIONIC_MANTLE:
    return CLASS_LEVEL(ch, CLASS_PSIONICIST) >= 23 && GET_REAL_INT(ch) >= 23 &&
           (knows(ch, PSIONIC_INERTIAL_ARMOR, CLASS_PSIONICIST) ||
            knows(ch, PSIONIC_BIOFEEDBACK, CLASS_PSIONICIST) ||
            knows(ch, PSIONIC_VIGOR, CLASS_PSIONICIST));
  case FEAT_EPIC_CHARGE_FED_REPAIR:
    return F(FEAT_CONSTRUCT_STONE_GOLEM) && R(ABILITY_USE_MAGIC_DEVICE, 20);
  case FEAT_EPIC_GOLEM_AEGIS:
    return CLASS_LEVEL(ch, CLASS_ARTIFICER) >= 23 && GET_REAL_INT(ch) >= 23 &&
           F(FEAT_CONSTRUCT_STONE_GOLEM);
  case FEAT_EPIC_CONSTRUCTBANE_INFUSION:
    return CLASS_LEVEL(ch, CLASS_ARTIFICER) >= 25 && F(FEAT_CRAFT_MAGICAL_ARMS_AND_ARMOR) &&
           R(ABILITY_ARCANA, 23);
  case FEAT_EPIC_RESILIENT_BOND:
    return CLASS_LEVEL(ch, CLASS_SUMMONER) >= 23 && GET_REAL_CHA(ch) >= 23 && F(FEAT_LIFE_LINK);
  case FEAT_EPIC_SPELL_SHARING:
    return R(ABILITY_SPELLCRAFT, 20) &&
           (knows(ch, SPELL_BARKSKIN, CLASS_SUMMONER) || knows(ch, SPELL_BLUR, CLASS_SUMMONER) ||
            knows(ch, SPELL_RESIST_ENERGY, CLASS_SUMMONER) ||
            knows(ch, SPELL_STONESKIN, CLASS_SUMMONER));
  case FEAT_EPIC_CONJURATION_COMMAND:
    return GET_REAL_CHA(ch) >= 21 && F(FEAT_AUGMENT_SUMMONING);
  case FEAT_EPIC_PLANAR_TRIAGE:
    return GET_REAL_CHA(ch) >= 21 && get_class_highest_circle(ch, CLASS_SUMMONER) >= 6;
  case FEAT_EPIC_EVOLVED_SUMMONS:
    return CLASS_LEVEL(ch, CLASS_SUMMONER) >= 23 && F(FEAT_IMPROVED_AUGMENT_SUMMONING) &&
           R(ABILITY_SPELLCRAFT, 23);
  case FEAT_EPIC_TRAILMASTER:
    return GET_REAL_WIS(ch) >= 21 && F(FEAT_TRACK) && R(ABILITY_NATURE, 23) &&
           R(ABILITY_PERCEPTION, 20);
  case FEAT_EPIC_INSPIRATION:
    return GET_REAL_CHA(ch) >= 23 && F(FEAT_BARDIC_MUSIC) && R(ABILITY_PERFORM, 20);
  case FEAT_POSITIVE_ENERGY_BULWARK:
    return (CLASS_LEVEL(ch, CLASS_CLERIC) >= 20 || CLASS_LEVEL(ch, CLASS_PALADIN) >= 20) &&
           GET_REAL_CHA(ch) >= 21 && F(FEAT_CHANNEL_ENERGY) &&
           (IS_GOOD(ch) || (IS_NEUTRAL(ch) && ch->player_specials->saved.channel_energy_type ==
                                                  CHANNEL_ENERGY_TYPE_POSITIVE));
  case FEAT_GREAT_SMITING:
    return GET_REAL_CHA(ch) >= 23 &&
           ((CLASS_LEVEL(ch, CLASS_PALADIN) >= 20 && F(FEAT_SMITE_EVIL)) ||
            (CLASS_LEVEL(ch, CLASS_BLACKGUARD) >= 20 && F(FEAT_SMITE_GOOD)));
  case FEAT_LEGENDARY_COMPANION_ELEMENTAL_BOND:
    return (CLASS_LEVEL(ch, CLASS_DRUID) >= 20 || CLASS_LEVEL(ch, CLASS_RANGER) >= 20) &&
           GET_REAL_WIS(ch) >= 21 && F(FEAT_ANIMAL_COMPANION) && R(ABILITY_NATURE, 20);
  case FEAT_EPIC_AUGMENTED_ALCHEMY:
    return GET_REAL_INT(ch) >= 23 && F(FEAT_BOMBS) && R(ABILITY_CRAFT_ALCHEMY, 20);
  }
#undef F
#undef R
  return FALSE;
}
static struct char_data *owner(struct char_data *pet)
{
  return pet && IS_NPC(pet) && pet->master && !IS_NPC(pet->master) && AFF_FLAGGED(pet, AFF_CHARM)
             ? pet->master
             : NULL;
}
static bool primary(struct char_data *pet)
{
  return owner(pet) && MOB_FLAGGED(pet, MOB_EIDOLON) && !char_has_mud_event(pet, ePURGEMOB);
}
static struct char_data *companion(struct char_data *ch, bool golem)
{
  struct follow_type *f;
  for (f = ch->followers; f; f = f->next)
    if (owner(f->follower) == ch && IN_ROOM(ch) == IN_ROOM(f->follower) &&
        GET_HIT(f->follower) > 0 &&
        (golem ? MOB_FLAGGED(f->follower, MOB_GOLEM) : primary(f->follower)))
      return f->follower;
  return NULL;
}
static bool normal_summon(struct char_data *pet)
{
  return owner(pet) && pet->epic_summon_expiry > time(NULL) && char_has_mud_event(pet, ePURGEMOB) &&
         !MOB_FLAGGED(pet, MOB_EIDOLON);
}
int epic_rebuttal_roll(struct char_data *ch, int resistance, int roll)
{
  if (resistance > 0 && epic_feat_rank(ch, FEAT_EPIC_RESISTANCE_REBUTTAL) &&
      EPIC_SECOND(ch).rebuttal > time(NULL) && ch->epic_cast_context == CAST_SPELL &&
      ch->epic_spell_depth == 1 && ch->epic_spell_context >= PSIONIC_POWER_START &&
      ch->epic_spell_context < PSIONIC_IMPALE_MIND)
  {
    EPIC_SECOND(ch).rebuttal = 0;
    save_char(ch, 0);
    return MAX(roll, d20(ch));
  }
  return roll;
}
/* Finite pools never increase maximum HP or heal. All real pools compete. */
static bool aegis_valid(struct char_data *ch)
{
  struct char_data *g;
  if (IS_NPC(ch))
    return FALSE;
  g = companion(ch, TRUE);
  if (EPIC_SECOND(ch).aegis_expiry <= time(NULL) || !epic_feat_rank(ch, FEAT_EPIC_GOLEM_AEGIS) ||
      !g || !*g->epic_pet_uuid || strcmp(g->epic_pet_uuid, EPIC_SECOND(ch).golem_uuid))
  {
    EPIC_SECOND(ch).aegis_hp = 0;
    return FALSE;
  }
  return EPIC_SECOND(ch).aegis_hp > 0;
}
int epic_temporary_remaining(struct char_data *ch)
{
  struct affected_type *af;
  int amount = aegis_valid(ch) ? EPIC_SECOND(ch).aegis_hp : 0;
  if (IN_ROOM(ch) != NOWHERE && ROOM_FLAGGED(IN_ROOM(ch), ROOM_NOMAGIC))
    return 0;
  for (af = ch->affected; af; af = af->next)
    if (af->spell == AFFECT_EPIC_SHARED_VIGOR && af->legendary_expiry > time(NULL))
      amount = MAX(amount, af->modifier);
  return amount;
}
void epic_temporary_clear(struct char_data *ch)
{
  if (!IS_NPC(ch))
    EPIC_SECOND(ch).aegis_hp = 0;
  affect_from_char(ch, AFFECT_EPIC_SHARED_VIGOR);
}
int epic_temporary_absorb(struct char_data *ch, int damage)
{
  struct affected_type *af;
  int amount;
  if (damage <= 0)
    return damage;
  if (aegis_valid(ch))
  {
    amount = MIN(damage, EPIC_SECOND(ch).aegis_hp);
    EPIC_SECOND(ch).aegis_hp -= amount;
    save_char(ch, 0);
    return damage - amount;
  }
  for (af = ch->affected; af; af = af->next)
    if (af->spell == AFFECT_EPIC_SHARED_VIGOR && af->legendary_expiry > time(NULL))
    {
      amount = MIN(damage, MAX(0, af->modifier));
      af->modifier -= amount;
      return damage - amount;
    }
  return damage;
}
void epic_second_departure(struct char_data *ch)
{
  struct char_data *o = owner(ch);
  if (!ch)
    return;
  if (!IS_NPC(ch) && EPIC_SECOND(ch).aegis_hp)
  {
    EPIC_SECOND(ch).aegis_hp = 0;
    save_char(ch, 0);
  }
  if (o && EPIC_SECOND(o).aegis_hp && !strcmp(ch->epic_pet_uuid, EPIC_SECOND(o).golem_uuid))
  {
    EPIC_SECOND(o).aegis_hp = 0;
    save_char(o, 0);
  }
}
bool epic_resilient_bond(struct char_data *pet)
{
  struct char_data *ch = owner(pet);
  if (!pet || !pet->epic_lethal_packet || !primary(pet) || !AWAKE(ch) || GET_HIT(ch) <= 0 ||
      IN_ROOM(ch) != IN_ROOM(pet) || !epic_feat_rank(ch, FEAT_EPIC_RESILIENT_BOND) ||
      EPIC_SECOND(ch).ready[E2_BOND] > time(NULL))
    return FALSE;
  EPIC_SECOND(ch).ready[E2_BOND] = time(NULL) + 300;
  save_char(ch, 0);
  return savingthrow_flatdc(NULL, pet, SAVING_WILL, 0, 20, CAST_INNATE, SCHOOL_NOSCHOOL, 0);
}
static bool spend_slot(struct char_data *ch, int circle, bool commit)
{
  if (compute_slots_by_circle(ch, CLASS_SUMMONER, circle) -
          count_total_slots(ch, CLASS_SUMMONER, circle) <=
      0)
    return FALSE;
  if (commit)
    innate_magic_add(ch, CLASS_SUMMONER, circle, 0,
                     compute_spells_prep_time(ch, CLASS_SUMMONER, circle, 0), 0);
  return TRUE;
}
static void bulwark_pulse(struct char_data *ch)
{
  struct char_data *vict, *next;
  for (vict = world[IN_ROOM(ch)].people; vict; vict = next)
  {
    int amount;
    next = vict->next_in_room;
    if (!IS_UNDEAD(vict) || !aoeOK(ch, vict, ABILITY_CHANNEL_POSITIVE_ENERGY) ||
        (FIGHTING(ch) != vict && (!FIGHTING(vict) || !is_player_grouped(FIGHTING(vict), ch))))
      continue;
    amount = dice(2, 6);
    if (savingthrow_flatdc(NULL, vict, SAVING_WILL, 0, EPIC_SECOND(ch).bulwark_dc, CAST_INNATE,
                           SCHOOL_NOSCHOOL, ABILITY_CHANNEL_POSITIVE_ENERGY))
      amount /= 2;
    damage_with_origin(ch, vict, amount, ABILITY_CHANNEL_POSITIVE_ENERGY, DAM_HOLY, 0,
                       LEG_ORIGIN_BONUS | LEG_ORIGIN_NO_TRIGGER);
  }
}
void epic_second_update(void)
{
  struct char_data *ch, *next;
  time_t now = time(NULL);
  for (ch = character_list; ch; ch = next)
  {
    next = ch->next;
    if (IS_NPC(ch))
      continue;
    if (EPIC_SECOND(ch).aegis_hp > 0 && !aegis_valid(ch))
      save_char(ch, 0);
    if (!EPIC_SECOND(ch).bulwark_pulses)
      continue;
    if (!AWAKE(ch) || GET_HIT(ch) <= 0 || IN_ROOM(ch) == NOWHERE ||
        EPIC_SECOND(ch).bulwark_end <= now || !epic_feat_rank(ch, FEAT_POSITIVE_ENERGY_BULWARK))
    {
      EPIC_SECOND(ch).bulwark_pulses = 0;
      save_char(ch, 0);
      continue;
    }
    if (EPIC_SECOND(ch).bulwark_next <= now)
    {
      /* Never catch up offline pulses: an overdue pulse is lost. */
      bool on_time = now - EPIC_SECOND(ch).bulwark_next < 2;
      EPIC_SECOND(ch).bulwark_pulses--;
      EPIC_SECOND(ch).bulwark_next += 6;
      save_char(ch, 0);
      if (on_time)
        bulwark_pulse(ch);
    }
  }
}
bool epic_summon_doctrine(struct char_data *pet, int doctrine)
{
  struct char_data *ch = owner(pet);
  return ch && pet->epic_summon_doctrine == doctrine && pet->epic_summon_expiry > time(NULL) &&
         epic_feat_rank(ch, FEAT_EPIC_EVOLVED_SUMMONS);
}
void epic_summon_created(struct char_data *ch, struct char_data *pet, int spell, int seconds)
{
  switch (spell)
  {
  case SPELL_SUMMON_CREATURE_1:
  case SPELL_SUMMON_CREATURE_2:
  case SPELL_SUMMON_CREATURE_3:
  case SPELL_SUMMON_CREATURE_4:
  case SPELL_SUMMON_CREATURE_5:
  case SPELL_SUMMON_CREATURE_6:
  case SPELL_SUMMON_CREATURE_7:
  case SPELL_SUMMON_CREATURE_8:
  case SPELL_SUMMON_CREATURE_9:
    break;
  default:
    return;
  }
  pet->epic_summon_expiry = time(NULL) + MAX(1, seconds);
  if (epic_feat_rank(ch, FEAT_EPIC_EVOLVED_SUMMONS))
    pet->epic_summon_doctrine = EPIC_CHOICES(ch).summon_doctrine;
}
void epic_infusion_unwield(struct char_data *ch, struct obj_data *obj)
{
  if (!ch || IS_NPC(ch) || ch->legendary_serializing || !obj || !*obj->epic_binding_id)
    return;
  if (!strcmp(EPIC_SECOND(ch).weapon_id, obj->epic_binding_id))
  {
    EPIC_SECOND(ch).infusion_expiry = 0;
    EPIC_SECOND(ch).weapon_id[0] = 0;
  }
}
void epic_second_smite(struct char_data *ch, int smite)
{
  int cl = smite == SKILL_SMITE_EVIL ? CLASS_PALADIN : CLASS_BLACKGUARD;
  if (!IS_NPC(ch) && EPIC_SECOND(ch).smite_expiry > time(NULL) &&
      epic_feat_rank(ch, FEAT_GREAT_SMITING))
  {
    EPIC_SECOND(ch).smite_class = cl + 1;
    save_char(ch, 0);
  }
}
int epic_second_attack_begin(struct char_data *ch, int type, int attack_type,
                             struct epic_second_attack *attack)
{
  struct char_data *o = owner(ch);
  int bonus = 0;
  memset(attack, 0, sizeof(*attack));
  if (ch->epic_attack_depth != 1 || type != TYPE_UNDEFINED || attack_type == ATTACK_TYPE_PSIONICS ||
      attack_type == ATTACK_TYPE_ELDRITCH_BLAST)
    return 0;
  if (o && normal_summon(ch) && EPIC_SECOND(o).command_expiry > time(NULL) &&
      !strcmp(EPIC_SECOND(o).command_uuid, ch->epic_pet_uuid) &&
      epic_feat_rank(o, FEAT_EPIC_CONJURATION_COMMAND))
  {
    EPIC_SECOND(o).command_expiry = 0;
    attack->command_damage = 8;
    bonus = epic_typed_increment(ch, APPLY_HITROLL, 0, BONUS_TYPE_MORALE, 4);
    save_char(o, 0);
  }
  if (!IS_NPC(ch) && EPIC_SECOND(ch).smite_class && EPIC_SECOND(ch).smite_expiry > time(NULL) &&
      epic_feat_rank(ch, FEAT_GREAT_SMITING))
  {
    int cl = EPIC_SECOND(ch).smite_class - 1;
    if ((cl == CLASS_PALADIN || cl == CLASS_BLACKGUARD) && CLASS_LEVEL(ch, cl) >= 20)
      attack->smite_damage = (cl == CLASS_PALADIN ? 1 : -1) * CLASS_LEVEL(ch, cl);
    EPIC_SECOND(ch).smite_class = 0;
    EPIC_SECOND(ch).smite_expiry = 0;
    save_char(ch, 0);
  }
  return bonus;
}
void epic_second_attack_hit(struct char_data *ch, struct char_data *victim, struct obj_data *weapon,
                            int attack_type, struct epic_second_attack *attack)
{
  struct char_data *o = owner(ch);
  int physical = 0, infusion = 0, energy = 0, element = DAM_FIRE;
  long victim_id = GET_ID(victim), attacker_id = GET_ID(ch);
  if (attack->smite_damage > 0 && IS_EVIL(victim))
    physical += attack->smite_damage;
  if (attack->smite_damage < 0 && IS_GOOD(victim))
    physical -= attack->smite_damage;
  if (!IS_NPC(ch) && weapon && *weapon->epic_binding_id && IS_CONSTRUCT(victim) &&
      EPIC_SECOND(ch).infusion_expiry > time(NULL) && EPIC_SECOND(ch).infusion_proc <= time(NULL) &&
      !strcmp(EPIC_SECOND(ch).weapon_id, weapon->epic_binding_id) &&
      epic_feat_rank(ch, FEAT_EPIC_CONSTRUCTBANE_INFUSION))
  {
    EPIC_SECOND(ch).infusion_proc = time(NULL) + 6;
    infusion = dice(2, 6);
    save_char(ch, 0);
  }
  if (o && attack_type != ATTACK_TYPE_RANGED && companion_training_kind(ch) == 1 &&
      epic_feat_rank(o, FEAT_LEGENDARY_COMPANION_ELEMENTAL_BOND) &&
      EPIC_SECOND(o).ready[E2_COMPANION] <= time(NULL))
  {
    const int types[] = {0, DAM_ACID, DAM_COLD, DAM_ELECTRIC, DAM_FIRE};
    struct follow_type *f;
    for (f = o->followers; f; f = f->next)
      if (companion_training_kind(f->follower) == 1)
        break;
    if (f && f->follower == ch && EPIC_CHOICES(o).companion_element >= 1 &&
        EPIC_CHOICES(o).companion_element <= 4)
    {
      EPIC_SECOND(o).ready[E2_COMPANION] = time(NULL) + 6;
      element = types[EPIC_CHOICES(o).companion_element];
      energy = dice(1, 6);
      save_char(o, 0);
    }
  }
  if (physical > 0)
  {
    physical = MAX(0, apply_damage_reduction(ch, victim, weapon, physical, FALSE, attack_type));
    damage_with_origin(ch, victim, physical, TYPE_HIT, DAM_BLUDGEON, attack_type,
                       LEG_ORIGIN_BONUS | LEG_ORIGIN_NO_TRIGGER);
  }
  if (infusion && find_char(attacker_id) == ch && find_char(victim_id) == victim &&
      GET_POS(victim) != POS_DEAD)
  {
    infusion = MAX(0, apply_damage_reduction(ch, victim, weapon, infusion, FALSE, attack_type));
    damage_with_origin(ch, victim, infusion, TYPE_HIT, DAM_BLUDGEON, attack_type,
                       LEG_ORIGIN_BONUS | LEG_ORIGIN_NO_TRIGGER);
  }
  if (energy && find_char(attacker_id) == ch && find_char(victim_id) == victim &&
      GET_POS(victim) != POS_DEAD)
    damage_with_origin(ch, victim, energy, TYPE_HIT, element, attack_type,
                       LEG_ORIGIN_BONUS | LEG_ORIGIN_NO_TRIGGER);
}
void epic_bomb_changed(struct char_data *ch, int slot)
{
  if (!IS_NPC(ch) && EPIC_SECOND(ch).bomb_slot == slot + 1)
  {
    EPIC_SECOND(ch).bomb_slot = 0;
    EPIC_SECOND(ch).bomb_expiry = 0;
  }
}
int epic_bomb_consume(struct char_data *ch, int slot)
{
  bool armed = !IS_NPC(ch) && EPIC_SECOND(ch).bomb_slot == slot + 1 &&
               EPIC_SECOND(ch).bomb_expiry > time(NULL) &&
               epic_feat_rank(ch, FEAT_EPIC_AUGMENTED_ALCHEMY);
  epic_bomb_changed(ch, slot);
  return armed ? dice(4, 6) : 0;
}
bool epic_shared_valid(struct char_data *ch)
{
  struct char_data *pet;
  if (!ch->epic_shared_spell)
    return TRUE;
  pet = companion(ch, FALSE);
  return pet && GET_ID(pet) == ch->epic_shared_pet && epic_feat_rank(ch, FEAT_EPIC_SPELL_SHARING);
}
void epic_shared_complete(struct char_data *ch, int spell, int level, int metamagic)
{
  struct char_data *pet = companion(ch, FALSE);
  if (ch->epic_shared_spell != spell || !pet || GET_ID(pet) != ch->epic_shared_pet ||
      !epic_feat_rank(ch, FEAT_EPIC_SPELL_SHARING))
    return;
  mag_affects(level, ch, pet, NULL, spell, SAVING_WILL, CAST_SPELL, metamagic);
}

bool epic_second_command(struct char_data *ch, const char *verb, const char *argument)
{
  static const struct
  {
    const char *name;
    int feat, use, seconds, action;
  } commands[] = {
      {"psi-rebuttal", FEAT_EPIC_RESISTANCE_REBUTTAL, E2_REBUTTAL, 60, ACTION_SWIFT},
      {"psi-mantle", FEAT_SHARED_PSIONIC_MANTLE, E2_MANTLE, 60, ACTION_STANDARD},
      {"charge-repair", FEAT_EPIC_CHARGE_FED_REPAIR, E2_REPAIR, 300, ACTION_STANDARD},
      {"golem-aegis", FEAT_EPIC_GOLEM_AEGIS, E2_AEGIS, 120, ACTION_STANDARD},
      {"constructbane", FEAT_EPIC_CONSTRUCTBANE_INFUSION, E2_INFUSION, 60, ACTION_STANDARD},
      {"shared-cast", FEAT_EPIC_SPELL_SHARING, E2_SHARING, 60, ACTION_NONE},
      {"conjuration-command", FEAT_EPIC_CONJURATION_COMMAND, E2_COMMAND, 30, ACTION_SWIFT},
      {"planar-triage", FEAT_EPIC_PLANAR_TRIAGE, E2_TRIAGE, 300, ACTION_STANDARD},
      {"positive-bulwark", FEAT_POSITIVE_ENERGY_BULWARK, E2_BULWARK, 300, ACTION_STANDARD},
      {"great-smite", FEAT_GREAT_SMITING, E2_SMITE, 300, ACTION_SWIFT},
      {"augment-bomb", FEAT_EPIC_AUGMENTED_ALCHEMY, E2_BOMB, 60, ACTION_SWIFT}};
  char input[MAX_INPUT_LENGTH], discard[MAX_INPUT_LENGTH], a[MAX_INPUT_LENGTH], b[MAX_INPUT_LENGTH];
  char *args;
  struct char_data *pet = NULL;
  struct obj_data *obj = NULL;
  struct affected_type af, *aff;
  int i, spell = 0, cost = 0, slot = -1, sacrifice = -1, circle, oldaugment;
  for (i = 0; i < (int)(sizeof(commands) / sizeof(commands[0])); i++)
    if (!str_cmp(verb, commands[i].name))
      break;
  if (i == (int)(sizeof(commands) / sizeof(commands[0])))
    return FALSE;
  strlcpy(input, argument, sizeof(input));
  args = (char *)one_argument(input, discard, sizeof(discard));
  args = (char *)one_argument(args, a, sizeof(a));
  skip_spaces(&args);
  strlcpy(b, args, sizeof(b));
  if (!epic_feat_rank(ch, commands[i].feat))
  {
    send_to_char(ch, "You do not meet this epic feat's prerequisites.\r\n");
    return TRUE;
  }
  if (IS_CASTING(ch) || !AWAKE(ch) || GET_HIT(ch) <= 0 || !can_continue(ch, TRUE))
    return TRUE;
  if (EPIC_SECOND(ch).ready[commands[i].use] > time(NULL))
  {
    send_to_char(ch, "That epic ability recovers in %ld seconds.\r\n",
                 (long)(EPIC_SECOND(ch).ready[commands[i].use] - time(NULL)));
    return TRUE;
  }
  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_NOMAGIC))
  {
    send_to_char(ch, "Magic is suppressed here.\r\n");
    return TRUE;
  }
#define REJECT(msg)                                                                                \
  do                                                                                               \
  {                                                                                                \
    send_to_char(ch, msg "\r\n");                                                                  \
    return TRUE;                                                                                   \
  } while (0)
  switch (commands[i].use)
  {
  case E2_REBUTTAL:
    cost = 5;
    if (EPIC_SECOND(ch).rebuttal > time(NULL))
      REJECT("Resistance Rebuttal is already armed.");
    break;
  case E2_MANTLE:
    pet = get_char_vis(ch, a, NULL, FIND_CHAR_ROOM);
    if (!pet || pet == ch || !is_player_grouped(ch, pet) || GET_HIT(pet) <= 0)
      REJECT("Choose a living, visible group ally here, other than yourself.");
    if (!str_cmp(b, "inertial armor"))
      spell = PSIONIC_INERTIAL_ARMOR;
    else if (!str_cmp(b, "biofeedback"))
      spell = PSIONIC_BIOFEEDBACK;
    else if (!str_cmp(b, "vigor"))
      spell = PSIONIC_VIGOR;
    if (!spell || !knows(ch, spell, CLASS_PSIONICIST))
      REJECT("Choose a known power: inertial armor, biofeedback or vigor.");
    if (affected_by_spell(pet, spell) ||
        (spell == PSIONIC_VIGOR && affected_by_spell(pet, AFFECT_EPIC_SHARED_VIGOR)))
      REJECT("That power already affects your ally.");
    if (spell == PSIONIC_VIGOR &&
        MAX(epic_temporary_remaining(pet), legendary_temporary_remaining(pet)) >= 5)
      REJECT("Your ally already has a stronger temporary HP pool.");
    cost = psionic_powers[spell].psp_cost + 8;
    break;
  case E2_REPAIR:
    pet = get_char_vis(ch, a, NULL, FIND_CHAR_ROOM);
    obj = get_obj_in_list_vis(ch, b, NULL, ch->carrying);
    if (!pet || pet != companion(ch, TRUE) || GET_HIT(pet) >= GET_MAX_HIT(pet))
      REJECT("Choose your injured active crafted golem here.");
    if (!obj || GET_OBJ_TYPE(obj) != ITEM_WAND || GET_OBJ_VAL(obj, 2) < 3 ||
        GET_OBJ_VAL(obj, 3) <= 0 || GET_OBJ_VAL(obj, 3) >= NUM_SPELLS)
      REJECT("Supply a carried ordinary wand with at least three charges.");
    circle = NUM_CIRCLES + 1;
    for (oldaugment = 0; oldaugment < NUM_CLASSES; oldaugment++)
      circle = MIN(circle, compute_spells_circle(ch, oldaugment, GET_OBJ_VAL(obj, 3), 0, 0));
    if (circle < 1 || circle > 3 || GET_OBJ_VAL(obj, 1) <= 0)
      REJECT("The wand must hold an ordinary circle 1-3 spell.");
    break;
  case E2_AEGIS:
    pet = companion(ch, TRUE);
    if (!pet || GET_HIT(pet) < 31)
      REJECT("Your active crafted golem must be here with at least 31 HP.");
    if (EPIC_SECOND(ch).aegis_hp > 0 && EPIC_SECOND(ch).aegis_expiry > time(NULL))
      REJECT("Your Golem Aegis is already active.");
    if (legendary_temporary_remaining(ch) >= 30)
      REJECT("You already have a stronger temporary HP pool.");
    break;
  case E2_INFUSION:
    obj = get_obj_in_equip_vis(ch, a, NULL, ch->equipment);
    if (!obj || GET_OBJ_TYPE(obj) != ITEM_WEAPON ||
        (obj != GET_EQ(ch, WEAR_WIELD_1) && obj != GET_EQ(ch, WEAR_WIELD_OFFHAND) &&
         obj != GET_EQ(ch, WEAR_WIELD_2H)))
      REJECT("Choose a weapon you are wielding.");
    if (!*obj->epic_binding_id && !legendary_uuid_generate(obj->epic_binding_id))
      REJECT("The weapon cannot be bound right now.");
    break;
  case E2_SHARING:
    /* Multiword spell names are accepted with or without native quotation marks. */
    args = (char *)one_argument(input, discard, sizeof(discard));
    skip_spaces(&args);
    if (*args == '\'' || *args == '\"')
    {
      char quote = *args++;
      char *end = strchr(args, quote);
      if (end)
        *end = 0;
    }
    if (!str_cmp(args, "barkskin"))
      spell = SPELL_BARKSKIN;
    else if (!str_cmp(args, "blur"))
      spell = SPELL_BLUR;
    else if (!str_cmp(args, "resist energy"))
      spell = SPELL_RESIST_ENERGY;
    else if (!str_cmp(args, "stoneskin"))
      spell = SPELL_STONESKIN;
    pet = companion(ch, FALSE);
    if (!spell || !knows(ch, spell, CLASS_SUMMONER) || !pet)
      REJECT("Choose barkskin, blur, resist energy or stoneskin, with your primary eidolon here.");
    circle = compute_spells_circle(ch, CLASS_SUMMONER, spell, 0, 0) + 1;
    if (circle > 6 || GET_CHA(ch) < 10 + circle || !spend_slot(ch, circle, FALSE))
      REJECT("You need a Summoner slot one circle above the spell, up to circle six.");
    ch->epic_shared_spell = spell;
    ch->epic_shared_pet = GET_ID(pet);
    if (!cast_epic_shared_spell(ch, spell))
    {
      ch->epic_shared_spell = 0;
      ch->epic_shared_pet = 0;
      return TRUE;
    }
    EPIC_SECOND(ch).ready[E2_SHARING] = time(NULL) + 60;
    save_char(ch, 0);
    return TRUE;
  case E2_COMMAND:
    pet = get_char_vis(ch, a, NULL, FIND_CHAR_ROOM);
    if (!pet || owner(pet) != ch || !normal_summon(pet))
      REJECT("Choose your own normal conjured creature here.");
    break;
  case E2_TRIAGE:
    pet = companion(ch, FALSE);
    if (!pet || GET_HIT(pet) >= GET_MAX_HIT(pet))
      REJECT("Your living injured primary eidolon must be here.");
    if (!spend_slot(ch, 6, FALSE))
      REJECT("You need an available sixth-circle Summoner slot.");
    break;
  case E2_BULWARK:
    if (EPIC_SECOND(ch).bulwark_pulses)
      REJECT("Your bulwark is already active.");
    if (daily_uses_remaining(ch, FEAT_CHANNEL_ENERGY) <= 0)
      REJECT("You need an available Channel Energy use.");
    break;
  case E2_SMITE:
    if (EPIC_SECOND(ch).smite_expiry > time(NULL))
      REJECT("Great Smiting is already armed.");
    break;
  case E2_BOMB:
    slot = atoi(a) - 1;
    sacrifice = atoi(b) - 1;
    if (slot < 0 || sacrifice < 0 || slot >= MAX_BOMBS_ALLOWED || sacrifice >= MAX_BOMBS_ALLOWED ||
        slot == sacrifice || GET_BOMB(ch, slot) != BOMB_NORMAL ||
        GET_BOMB(ch, sacrifice) != BOMB_NORMAL)
      REJECT("Choose two distinct prepared normal fire bomb slots.");
    if (EPIC_SECOND(ch).bomb_expiry > time(NULL))
      REJECT("A bomb is already augmented.");
    break;
  }
  if (pet && (commands[i].use == E2_AEGIS || commands[i].use == E2_COMMAND))
  {
    if (!*pet->epic_pet_uuid && !legendary_uuid_generate(pet->epic_pet_uuid))
      REJECT("Your companion cannot be identified right now.");
    if (!save_char_pets_checked(ch))
      REJECT("Your companion cannot be saved right now.");
  }
  if (cost && GET_PSP(ch) < cost)
    REJECT("You do not have enough PSP.");
  if (!is_action_available(ch, commands[i].action, TRUE))
    return TRUE;
  EPIC_SECOND(ch).ready[commands[i].use] = time(NULL) + commands[i].seconds;
  GET_PSP(ch) -= cost;
  switch (commands[i].use)
  {
  case E2_REBUTTAL:
    EPIC_SECOND(ch).rebuttal = time(NULL) + 18;
    break;
  case E2_REPAIR:
    GET_OBJ_VAL(obj, 2) -= 3;
    break;
  case E2_AEGIS:
    epic_temporary_clear(ch);
    legendary_temporary_clear(ch);
    GET_HIT(pet) -= 30;
    strlcpy(EPIC_SECOND(ch).golem_uuid, pet->epic_pet_uuid, 33);
    EPIC_SECOND(ch).aegis_hp = 30;
    EPIC_SECOND(ch).aegis_expiry = time(NULL) + 18;
    break;
  case E2_INFUSION:
    strlcpy(EPIC_SECOND(ch).weapon_id, obj->epic_binding_id, 33);
    EPIC_SECOND(ch).infusion_expiry = time(NULL) + 18;
    break;
  case E2_COMMAND:
    strlcpy(EPIC_SECOND(ch).command_uuid, pet->epic_pet_uuid, 33);
    EPIC_SECOND(ch).command_expiry = time(NULL) + 12;
    break;
  case E2_TRIAGE:
    spend_slot(ch, 6, TRUE);
    break;
  case E2_BULWARK:
    start_daily_use_cooldown(ch, FEAT_CHANNEL_ENERGY);
    EPIC_SECOND(ch).bulwark_dc =
        10 +
        MAX(CLASS_LEVEL(ch, CLASS_CLERIC) >= 20 ? CLASS_LEVEL(ch, CLASS_CLERIC) : 0,
            CLASS_LEVEL(ch, CLASS_PALADIN) >= 20 ? CLASS_LEVEL(ch, CLASS_PALADIN) : 0) /
            2 +
        GET_CHA_BONUS(ch);
    EPIC_SECOND(ch).bulwark_pulses = 2;
    EPIC_SECOND(ch).bulwark_next = time(NULL) + 6;
    EPIC_SECOND(ch).bulwark_end = time(NULL) + 18;
    break;
  case E2_SMITE:
    EPIC_SECOND(ch).smite_expiry = time(NULL) + 18;
    EPIC_SECOND(ch).smite_class = 0;
    break;
  case E2_BOMB:
    epic_bomb_changed(ch, sacrifice);
    GET_BOMB(ch, sacrifice) = BOMB_NONE;
    EPIC_SECOND(ch).bomb_slot = slot + 1;
    EPIC_SECOND(ch).bomb_expiry = time(NULL) + 18;
    break;
  }
  if (commands[i].action == ACTION_SWIFT)
    USE_SWIFT_ACTION(ch);
  else
    USE_STANDARD_ACTION(ch);
  if (commands[i].use == E2_AEGIS && !save_char_pets_checked(ch))
  {
    EPIC_SECOND(ch).aegis_hp = 0;
    save_char(ch, 0);
    send_to_char(ch, "Unable to save the golem payment. The Aegis was cancelled.\r\n");
    return TRUE;
  }
  if (!save_char_checked(ch, 0))
  {
    /* No new effect can remain live after an unsuccessful commitment save. */
    switch (commands[i].use)
    {
    case E2_REBUTTAL:
      EPIC_SECOND(ch).rebuttal = 0;
      break;
    case E2_AEGIS:
      EPIC_SECOND(ch).aegis_hp = 0;
      break;
    case E2_INFUSION:
      EPIC_SECOND(ch).infusion_expiry = 0;
      break;
    case E2_COMMAND:
      EPIC_SECOND(ch).command_expiry = 0;
      break;
    case E2_SMITE:
      EPIC_SECOND(ch).smite_expiry = 0;
      break;
    case E2_BOMB:
      EPIC_SECOND(ch).bomb_expiry = 0;
      break;
    case E2_BULWARK:
      EPIC_SECOND(ch).bulwark_pulses = 0;
      break;
    }
    send_to_char(ch, "Unable to save the committed epic ability. Its effect was not applied.\r\n");
    return TRUE;
  }
  if ((commands[i].use == E2_REPAIR || commands[i].use == E2_INFUSION) &&
      !Crash_crashsave_checked(ch))
  {
    if (commands[i].use == E2_INFUSION)
    {
      EPIC_SECOND(ch).infusion_expiry = 0;
      save_char(ch, 0);
    }
    send_to_char(ch, "Unable to save the item change. The item ability was cancelled.\r\n");
    return TRUE;
  }
  switch (commands[i].use)
  {
  case E2_MANTLE:
    if (spell == PSIONIC_VIGOR)
    {
      epic_temporary_clear(pet);
      legendary_temporary_clear(pet);
      new_affect(&af);
      af.spell = AFFECT_EPIC_SHARED_VIGOR;
      af.location = APPLY_SPECIAL;
      af.modifier = 5;
      af.duration = 10;
      af.legendary_expiry = time(NULL) + 60;
      affect_to_char(pet, &af);
    }
    else
    {
      oldaugment = GET_AUGMENT_PSP(ch);
      GET_AUGMENT_PSP(ch) = 0;
      mag_affects(GET_PSIONIC_LEVEL(ch), ch, pet, NULL, spell, SAVING_WILL, CAST_SPELL, 0);
      GET_AUGMENT_PSP(ch) = oldaugment;
      for (aff = pet->affected; aff; aff = aff->next)
        if (aff->spell == spell)
        {
          aff->duration = MIN(10, aff->duration);
          aff->legendary_expiry = time(NULL) + MIN(60, MAX(1, aff->duration) * 6);
        }
    }
    break;
  case E2_REPAIR:
    process_construct_repair(ch, pet, MIN(60, 10 + 2 * CLASS_LEVEL(ch, CLASS_ARTIFICER)));
    break;
  case E2_TRIAGE:
    process_healing(ch, pet, 0, MAX(1, MIN(100, GET_MAX_HIT(pet) / 5)), 0, 0);
    break;
  }
  if (pet)
    save_char_pets_checked(ch);
  send_to_char(ch, "%s activated.\r\n", feat_list[commands[i].feat].name);
  if (commands[i].use == E2_BULWARK)
    bulwark_pulse(ch);
  return TRUE;
#undef REJECT
}
