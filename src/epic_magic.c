#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "spells.h"
#include "fight.h"
#include "mud_event.h"
#include "dg_scripts.h"
#include "domains_schools.h"
#include "evolutions.h"
#include "quest.h"
#include "missions.h"
#include "epic_magic.h"

/* NPC combat does not require player PVP consent; player-owned pets do. */
static bool epic_hostile_ok(struct char_data *ch, struct char_data *target)
{
  if (!IS_NPC(target) ||
      (AFF_FLAGGED(target, AFF_CHARM) && target->master && !IS_NPC(target->master)))
    return pvp_ok(ch, target, false);
  return true;
}

bool epic_spell_uses_preparation_pool(int spellnum)
{
  return spellnum == SPELL_EPIC_MAGE_ARMOR || spellnum == SPELL_EPIC_WARDING ||
         spellnum == PSIONIC_EPIC_PSIONIC_WARD || spellnum == SPELL_MUMMY_DUST ||
         spellnum == SPELL_SUMMON_SOLAR || spellnum == SPELL_DRAGON_KNIGHT;
}

bool epic_ward_surge_active(struct char_data *ch)
{
  return ch && GET_STONESKIN(ch) > 0 && affected_by_spell(ch, SPELL_EPIC_WARDING) &&
         affected_by_spell(ch, AFFECT_EPIC_WARD_SURGE);
}

int epic_preparation_casts_max(struct char_data *ch)
{
  return MIN(3, get_epic_spell_casts_max(ch));
}

void normalize_epic_preparation_casts(struct char_data *ch)
{
  int maximum;
  if (!ch || IS_NPC(ch) || !ch->player_specials)
    return;
  maximum = epic_preparation_casts_max(ch);
  if (GET_EPIC_PREPARATION_CASTS(ch) < 0)
    GET_EPIC_PREPARATION_CASTS(ch) = maximum;
  GET_EPIC_PREPARATION_CASTS(ch) = MIN(maximum, GET_EPIC_PREPARATION_CASTS(ch));
  if (GET_EPIC_PREPARATION_CASTS(ch) >= maximum || GET_EPIC_PREPARATION_REGEN_TIMER(ch) < 0)
    GET_EPIC_PREPARATION_REGEN_TIMER(ch) = 0;
}

void regenerate_epic_preparation_cast(struct char_data *ch)
{
  int maximum;
  if (!ch || IS_NPC(ch) || !ch->player_specials)
    return;
  normalize_epic_preparation_casts(ch);
  maximum = epic_preparation_casts_max(ch);
  if (GET_EPIC_PREPARATION_CASTS(ch) >= maximum)
    return;
  GET_EPIC_PREPARATION_REGEN_TIMER(ch)++;
  while (GET_EPIC_PREPARATION_REGEN_TIMER(ch) >= EPIC_SPELL_CAST_REGEN_TICKS &&
         GET_EPIC_PREPARATION_CASTS(ch) < maximum)
  {
    GET_EPIC_PREPARATION_CASTS(ch)++;
    GET_EPIC_PREPARATION_REGEN_TIMER(ch) -= EPIC_SPELL_CAST_REGEN_TICKS;
    send_to_char(ch, "You recover an epic preparation cast (%d/%d).\r\n",
                 GET_EPIC_PREPARATION_CASTS(ch), maximum);
  }
  if (GET_EPIC_PREPARATION_CASTS(ch) >= maximum)
    GET_EPIC_PREPARATION_REGEN_TIMER(ch) = 0;
}

/* One enhancement at a time. Empower only improves damaging epic spells. */
int epic_spell_cast_cost(int spellnum, int metamagic)
{
  if (metamagic & ~(METAMAGIC_EMPOWER | METAMAGIC_QUICKEN))
    return -1;
  if (IS_SET(metamagic, METAMAGIC_EMPOWER) && IS_SET(metamagic, METAMAGIC_QUICKEN))
    return -1;
  if (IS_SET(metamagic, METAMAGIC_EMPOWER) && epic_spell_uses_preparation_pool(spellnum))
    return -1;
  return metamagic ? 2 : 1;
}

bool epic_spell_preflight(struct char_data *ch, struct char_data *victim, int spellnum,
                          int metamagic, bool display)
{
  int cost, available, maximum, timer, flag = 0;
  mob_vnum summon = 0;
  bool preparation = epic_spell_uses_preparation_pool(spellnum);
  if (!ch)
    return false;
  cost = epic_spell_cast_cost(spellnum, metamagic);
  if (cost < 0)
  {
    if (display)
      send_to_char(ch, "Epic spells allow Empower or Quicken, each costing one extra cast. "
                       "They cannot combine, and Empower requires a damaging spell.\r\n");
    return false;
  }
  switch (spellnum)
  {
  case SPELL_MUMMY_DUST: summon = MOB_MUMMY_LORD; flag = MOB_MUMMY_DUST; break;
  case SPELL_SUMMON_SOLAR: summon = MOB_SOLAR; flag = MOB_SUMMON_SOLAR; break;
  case SPELL_DRAGON_KNIGHT: summon = MOB_RED_DRAGON; flag = MOB_DRAGON_KNIGHT; break;
  }
  if (summon && (real_mobile(summon) == NOBODY || !can_add_follower_by_flag(ch, flag)))
  {
    if (display)
      send_to_char(ch, "You cannot summon that epic ally now. Only one epic summon may be "
                       "controlled at a time; no epic cast was spent.\r\n");
    return false;
  }
  if (((spellnum == SPELL_EPIC_MAGE_ARMOR || spellnum == SPELL_EPIC_WARDING ||
        spellnum == PSIONIC_EPIC_PSIONIC_WARD) ||
       (IS_SET(spell_info[spellnum].targets, TAR_CHAR_ROOM | TAR_FIGHT_VICT) &&
        !IS_SET(spell_info[spellnum].targets, TAR_IGNORE))) && !victim)
  {
    if (display)
      send_to_char(ch, "That epic spell needs a recipient; no epic cast was spent.\r\n");
    return false;
  }
  if (spell_info[spellnum].violent && IS_SET(spell_info[spellnum].routines, MAG_AREAS))
  {
    struct char_data *target;
    if (IN_ROOM(ch) == NOWHERE)
      return false;
    for (target = world[IN_ROOM(ch)].people; target; target = target->next_in_room)
      if (aoeOK(ch, target, spellnum) && !MOB_FLAGGED(target, MOB_NOKILL) &&
          is_mission_mob(ch, target) && epic_hostile_ok(ch, target))
        break;
    if (!target)
    {
      if (display)
        send_to_char(ch, "There are no valid enemies for that epic spell; no cast was spent.\r\n");
      return false;
    }
  }
  if (IS_NPC(ch))
    return true;
  normalize_epic_spell_casts(ch);
  normalize_epic_preparation_casts(ch);
  maximum = preparation ? epic_preparation_casts_max(ch) : get_epic_spell_casts_max(ch);
  available = preparation ? GET_EPIC_PREPARATION_CASTS(ch) : GET_EPIC_SPELL_CASTS(ch);
  timer = preparation ? GET_EPIC_PREPARATION_REGEN_TIMER(ch) : GET_EPIC_SPELL_REGEN_TIMER(ch);
  if (available >= cost)
    return true;
  if (display)
  {
    if (maximum < cost)
      send_to_char(ch, "You need more Spellcraft capacity for this epic cast (cost %d).\r\n", cost);
    else
      send_to_char(ch, "You need %d epic %s cast%s; %d available. Next recovery in %d seconds.\r\n",
                   cost, preparation ? "preparation" : "combat", cost == 1 ? "" : "s",
                   available, MAX(1, EPIC_SPELL_CAST_REGEN_TICKS - timer) * 6);
  }
  return false;
}

void spend_epic_spell_casts(struct char_data *ch, int spellnum, int metamagic)
{
  int cost = epic_spell_cast_cost(spellnum, metamagic);
  if (!ch || IS_NPC(ch) || cost < 1)
    return;
  normalize_epic_spell_casts(ch);
  normalize_epic_preparation_casts(ch);
  if (epic_spell_uses_preparation_pool(spellnum))
  {
    if (GET_EPIC_PREPARATION_CASTS(ch) < cost)
      return;
    GET_EPIC_PREPARATION_CASTS(ch) -= cost;
    send_to_char(ch, "Epic preparation casts remaining: %d/%d.\r\n",
                 GET_EPIC_PREPARATION_CASTS(ch), epic_preparation_casts_max(ch));
  }
  else
  {
    if (GET_EPIC_SPELL_CASTS(ch) < cost)
      return;
    GET_EPIC_SPELL_CASTS(ch) -= cost;
    send_to_char(ch, "Epic combat casts remaining: %d/%d.\r\n",
                 GET_EPIC_SPELL_CASTS(ch), get_epic_spell_casts_max(ch));
  }
}

static void epic_status(struct char_data *victim, int spell, int duration, int modifier)
{
  struct affected_type af;
  affect_from_char(victim, spell);
  new_affect(&af);
  af.spell = spell;
  af.duration = duration;
  af.modifier = modifier;
  af.location = APPLY_NONE;
  affect_to_char(victim, &af);
}

void epic_spell_damage_effects(struct char_data *ch, struct char_data *victim,
                               int spellnum, int level, int damage_dealt)
{
  char variables[128];
  if (!ch || damage_dealt <= 0 || !victim || GET_HIT(victim) <= 0 || GET_POS(victim) <= POS_DEAD)
    return;
  if (spellnum == SPELL_GREATER_RUIN)
  {
    epic_status(victim, AFFECT_EPIC_RUIN, 3, 10);
    act("Your ruin fractures $N's defenses: damage reduction weakened by 10!", FALSE,
        ch, NULL, victim, TO_CHAR);
    send_to_char(victim, "Greater Ruin fractures your defenses for three rounds!\r\n");
  }
  else if (spellnum == SPELL_HELLBALL && !affected_by_spell(victim, AFFECT_EPIC_HELLFIRE))
  {
    epic_status(victim, AFFECT_EPIC_HELLFIRE, 3, 0);
    snprintf(variables, sizeof(variables), "%ld %d %d", GET_ID(victim), MAX(1, level * 2), 2);
    NEW_EVENT(eEPIC_HELLFIRE, ch, variables, PULSE_VIOLENCE);
    send_to_char(victim, "Hellfire clings to you for two more rounds!\r\n");
  }
}

EVENTFUNC(event_epic_hellfire)
{
  struct mud_event_data *event = event_obj;
  struct char_data *ch, *victim;
  long target, caster_id;
  int amount, rounds;
  char variables[128];
  if (!event || !event->sVariables ||
      sscanf(event->sVariables, "%ld %d %d", &target, &amount, &rounds) != 3)
    return 0;
  ch = event->pStruct;
  victim = find_char(target);
  if (!ch || !victim || IN_ROOM(ch) == NOWHERE || IN_ROOM(ch) != IN_ROOM(victim) ||
      GET_POS(victim) <= POS_DEAD || ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL) ||
      !affected_by_spell(victim, AFFECT_EPIC_HELLFIRE) ||
      MOB_FLAGGED(victim, MOB_NOKILL) || !is_mission_mob(ch, victim) ||
      !epic_hostile_ok(ch, victim))
    return 0;
  caster_id = GET_ID(ch);
  /* Update the event before damage, which can extract the victim. */
  rounds--;
  snprintf(variables, sizeof(variables), "%ld %d %d", target, amount, rounds);
  free(event->sVariables);
  event->sVariables = strdup(variables);
  send_to_char(victim, "The lingering hellfire burns you!\r\n");
  damage(ch, victim, amount, AFFECT_EPIC_HELLFIRE, DAM_FIRE, FALSE);
  /* -1 can also mean absorbed damage. Resolve IDs again instead of assuming death. */
  victim = find_char(target);
  if (!victim || GET_POS(victim) <= POS_DEAD || find_char(caster_id) != ch)
    return 0;
  if (rounds <= 0)
  {
    affect_from_char(victim, AFFECT_EPIC_HELLFIRE);
    return 0;
  }
  return PULSE_VIOLENCE;
}

/* Each epic ally has one special action every three combat rounds. */
bool epic_summon_combat_turn(struct char_data *mob)
{
  struct char_data *master, *victim, *target, *next;
  int amount;
  long mob_id;
  if (!mob || !IS_NPC(mob) || !AFF_FLAGGED(mob, AFF_CHARM) ||
      !is_epic_summon_mob(mob) || !AWAKE(mob) || !FIGHTING(mob) ||
      !(master = mob->master) || IN_ROOM(master) != IN_ROOM(mob) ||
      IN_ROOM(mob) == NOWHERE || ROOM_FLAGGED(IN_ROOM(mob), ROOM_PEACEFUL) ||
      char_has_mud_event(mob, eEPIC_SUMMON_ACTION))
    return true;
  victim = FIGHTING(mob);
  if (IN_ROOM(victim) != IN_ROOM(mob))
    return true;
  mob_id = GET_ID(mob);
  NEW_EVENT(eEPIC_SUMMON_ACTION, mob, NULL, PULSE_VIOLENCE * 3);
  if (MOB_FLAGGED(mob, MOB_SUMMON_SOLAR))
  {
    target = (long long)GET_HIT(master) * GET_MAX_HIT(mob) <
                     (long long)GET_HIT(mob) * GET_MAX_HIT(master) ? master : mob;
    amount = MIN(GET_MAX_HIT(target) - GET_HIT(target), GET_LEVEL(mob) * 5);
    if (amount > 0)
    {
      GET_HIT(target) += amount;
      update_pos(target);
      act("$n bathes $N in restorative celestial light!", FALSE, mob, NULL, target, TO_ROOM);
      send_to_char(master, "Your solar restores %d hit points to %s.\r\n", amount, GET_NAME(target));
    }
  }
  else if (MOB_FLAGGED(mob, MOB_MUMMY_DUST))
  {
    if (!IS_UNDEAD(victim) && !MOB_FLAGGED(victim, MOB_NOKILL) &&
        is_mission_mob(master, victim) && epic_hostile_ok(mob, victim) &&
        formation_can_melee_target(mob, victim) &&
        attack_roll(mob, victim, ATTACK_TYPE_UNARMED, true, 0))
    {
      epic_status(victim, AFFECT_EPIC_MUMMY_DREAD, 3, 0);
      send_to_char(victim, "The mummy's dreadful touch weakens your attacks and defenses!\r\n");
    }
  }
  else if (MOB_FLAGGED(mob, MOB_DRAGON_KNIGHT))
  {
    act("$n exhales a torrent of dragonfire!", FALSE, mob, NULL, NULL, TO_ROOM);
    for (target = world[IN_ROOM(mob)].people; target; target = next)
    {
      next = target->next_in_room;
      if (target != victim && FIGHTING(target) != master && FIGHTING(target) != mob)
        continue;
      if (target == master || target == mob || is_player_grouped(master, target) ||
          (AFF_FLAGGED(target, AFF_CHARM) && target->master == master) ||
          MOB_FLAGGED(target, MOB_NOKILL) || !epic_hostile_ok(mob, target) ||
          !is_mission_mob(master, target))
        continue;
      amount = GET_LEVEL(mob) * 4;
      if (savingthrow(mob, target, SAVING_REFL, 0, CAST_INNATE, GET_LEVEL(mob), EVOCATION))
        amount /= 2;
      damage(mob, target, amount, SPELL_FIRE_BREATHE, DAM_FIRE, FALSE);
      if (find_char(mob_id) != mob)
        return false;
    }
  }
  return true;
}
