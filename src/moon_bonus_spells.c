/**************************************************************************
 *  File: moon_bonus_spells.c                      Part of LuminariMUD *
 *  Usage: Moon-based bonus spell slot system implementation               *
 *                                                                         *
 *  All rights reserved.  See license for complete information.            *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "handler.h"
#include "db.h"
#include "moon_bonus_spells.h"

/* Externals */
extern struct weather_data weather_info;

/* Define for moon bonus spell regeneration - one spell per 5 minutes */
#define MOON_BONUS_REGEN_TICKS (5 * 60 * 10) /* 5 minutes in ticks (assuming 10 ticks per second) */

/**
 * Initialize moon bonus spells for a character based on their alignment
 * and the current moon phases
 */
void init_moon_bonus_spells(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return;

  if (!ch->player_specials)
    return;

  /* Check if arcane moon phases are enabled in config */
  if (!CONFIG_ARCANE_MOON_PHASES)
  {
    ch->player_specials->saved.moon_bonus_spells = 0;
    ch->player_specials->saved.moon_bonus_spells_used = 0;
    ch->player_specials->saved.moon_bonus_regen_timer = 0;
    return;
  }

  /* Only eligible arcane casters get moon bonus spells */
  if (!is_moon_phase_affected(ch))
  {
    ch->player_specials->saved.moon_bonus_spells = 0;
    ch->player_specials->saved.moon_bonus_spells_used = 0;
    ch->player_specials->saved.moon_bonus_regen_timer = 0;
    return;
  }

  /* Initialize based on alignment and moon phase */
  update_moon_bonus_spells(ch);

  /* Reset usage and timer */
  ch->player_specials->saved.moon_bonus_spells_used = 0;
  ch->player_specials->saved.moon_bonus_regen_timer = MOON_BONUS_REGEN_TICKS;
}

/**
 * Update the moon bonus spells based on current moon phases
 * Called when moon phases change
 */
void update_moon_bonus_spells(struct char_data *ch)
{
  int max_bonus_spells = 0;
  int current_used = 0;
  int alignment_bonus = 0;

  if (!ch || IS_NPC(ch))
    return;

  if (!ch->player_specials)
    return;

  /* Check if arcane moon phases are enabled in config */
  if (!CONFIG_ARCANE_MOON_PHASES)
  {
    ch->player_specials->saved.moon_bonus_spells = 0;
    ch->player_specials->saved.moon_bonus_spells_used = 0;
    ch->player_specials->saved.moon_bonus_regen_timer = 0;
    return;
  }

  /* Only eligible arcane casters get moon bonus spells */
  if (!is_moon_phase_affected(ch))
  {
    ch->player_specials->saved.moon_bonus_spells = 0;
    ch->player_specials->saved.moon_bonus_spells_used = 0;
    ch->player_specials->saved.moon_bonus_regen_timer = 0;
    return;
  }

  /* Store current usage to apply to new maximum if needed */
  current_used = ch->player_specials->saved.moon_bonus_spells_used;

  /* Determine alignment and get corresponding bonus spells */
  if (IS_GOOD(ch))
    alignment_bonus = weather_info.moons.solinari_sp;
  else if (IS_NEUTRAL(ch))
    alignment_bonus = weather_info.moons.lunitari_sp;
  else
    alignment_bonus = weather_info.moons.nuitari_sp;

  max_bonus_spells = alignment_bonus;

  /* Update the maximum */
  ch->player_specials->saved.moon_bonus_spells = max_bonus_spells;

  /* If the new maximum is lower than what's currently used, cap the usage */
  if (current_used > max_bonus_spells)
  {
    ch->player_specials->saved.moon_bonus_spells_used = max_bonus_spells;
  }
}

/**
 * Get the current moon bonus spells available for a character
 * This is the maximum allowed based on their alignment and moon phase
 */
int get_max_moon_bonus_spells(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;

  if (!ch->player_specials)
    return 0;

  if (!is_moon_phase_affected(ch))
    return 0;

  return ch->player_specials->saved.moon_bonus_spells;
}

/**
 * Get the number of moon bonus spells currently available (not yet used)
 */
int get_available_moon_bonus_spells(struct char_data *ch)
{
  int max_spells;
  int used_spells;

  if (!ch || IS_NPC(ch))
    return 0;

  if (!ch->player_specials || !is_moon_phase_affected(ch))
    return 0;

  max_spells = ch->player_specials->saved.moon_bonus_spells;
  used_spells = ch->player_specials->saved.moon_bonus_spells_used;

  return MAX(0, max_spells - used_spells);
}

/**
 * Get the number of moon bonus spells currently in use
 */
int get_used_moon_bonus_spells(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;

  if (!ch->player_specials || !is_moon_phase_affected(ch))
    return 0;

  return ch->player_specials->saved.moon_bonus_spells_used;
}

/**
 * Check if a character has available moon bonus spells
 */
bool has_moon_bonus_spells(struct char_data *ch)
{
  if (!CONFIG_ARCANE_MOON_PHASES)
    return FALSE;

  return get_available_moon_bonus_spells(ch) > 0;
}

/**
 * Consume a moon bonus spell for a character
 */
bool use_moon_bonus_spell(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;

  if (!ch->player_specials)
    return FALSE;

  if (!has_moon_bonus_spells(ch))
    return FALSE;

  /* Increment the used counter */
  ch->player_specials->saved.moon_bonus_spells_used++;

  return TRUE;
}

/**
 * Regenerate one moon bonus spell if the timer has elapsed
 * Should be called once per tick/heartbeat
 */
void regenerate_moon_bonus_spell(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return;

  if (!ch->player_specials)
    return;

  if (!is_moon_phase_affected(ch))
  {
    update_moon_bonus_spells(ch);
    return;
  }

  /* Check if we're at max already */
  if (ch->player_specials->saved.moon_bonus_spells_used <= 0)
    return;

  /* Decrement the timer */
  ch->player_specials->saved.moon_bonus_regen_timer--;

  /* Check if it's time to regenerate */
  if (ch->player_specials->saved.moon_bonus_regen_timer <= 0)
  {
    /* Regenerate one spell slot */
    ch->player_specials->saved.moon_bonus_spells_used--;

    /* Reset the timer */
    ch->player_specials->saved.moon_bonus_regen_timer = MOON_BONUS_REGEN_TICKS;
  }
}

/**
 * Reset moon bonus spells when a character levels up or resets
 */
void reset_moon_bonus_spells(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return;

  if (!ch->player_specials)
    return;

  if (!is_moon_phase_affected(ch))
  {
    update_moon_bonus_spells(ch);
    return;
  }

  /* Reset usage and timer when leveling */
  ch->player_specials->saved.moon_bonus_spells_used = 0;
  ch->player_specials->saved.moon_bonus_regen_timer = MOON_BONUS_REGEN_TICKS;
}

/**
 * Helper function to check if a character is an arcane caster
 * Includes: Wizard, Sorcerer, Bard
 */
bool is_arcane_caster(struct char_data *ch)
{
  if (!ch)
    return FALSE;

  if (ARCANE_LEVEL(ch) > 0)
    return TRUE;

  return FALSE;
}

/* Preserve arcane classification while excluding Thorn PCs from lunar effects. */
bool is_moon_phase_affected(struct char_data *ch)
{
  if (!ch || (!IS_NPC(ch) && CLASS_LEVEL(ch, CLASS_KNIGHT_OF_THE_THORN) > 0))
    return FALSE;

  return is_arcane_caster(ch);
}

/* A daily chart box spans an arc of the orbit. Use 504 angular units (the
 * common multiple of 36, 28 and 8) so overlap is exact integer arithmetic.
 * Matching broad sanctions alone is not a conjunction. */
int get_moon_conjunctions(void)
{
  int phases[3] = {weather_info.moons.solinari_phase,
                   weather_info.moons.lunitari_phase,
                   weather_info.moons.nuitari_phase};
  const int periods[3] = {36, 28, 8};
  int start[3], end[3], moon, mask = 0;

  if (!CONFIG_ARCANE_MOON_PHASES)
    return 0;
  for (moon = 0; moon < 3; moon++)
  {
    if (phases[moon] < 1 || phases[moon] > periods[moon])
      return 0;
    start[moon] = (phases[moon] - 1) * (504 / periods[moon]);
    end[moon] = phases[moon] * (504 / periods[moon]);
  }
  if (MAX(start[0], start[1]) < MIN(end[0], end[1]))
    mask |= MOON_CONJUNCTION_SOL_LUN;
  if (MAX(start[0], start[2]) < MIN(end[0], end[2]))
    mask |= MOON_CONJUNCTION_SOL_NUIT;
  if (MAX(start[1], start[2]) < MIN(end[1], end[2]))
    mask |= MOON_CONJUNCTION_LUN_NUIT;
  if (MAX(MAX(start[0], start[1]), start[2]) < MIN(MIN(end[0], end[1]), end[2]))
    mask |= MOON_CONJUNCTION_ALL;
  return mask;
}

/* Classic AD&D conjunction additions, stacked with the ordinary phase.
 * A true triple conjunction replaces pair bonuses; pairs never add twice. */
void apply_moon_conjunction_bonuses(void)
{
  int mask = get_moon_conjunctions();
  struct moon_data *moons = &weather_info.moons;

  if (mask & MOON_CONJUNCTION_ALL)
  {
    moons->solinari_st += 2; moons->lunitari_st += 2; moons->nuitari_st += 2;
    moons->solinari_sp += 2; moons->lunitari_sp += 2; moons->nuitari_sp += 2;
    moons->solinari_lv++; moons->lunitari_lv++; moons->nuitari_lv++;
    return;
  }
  if (mask & (MOON_CONJUNCTION_SOL_LUN | MOON_CONJUNCTION_SOL_NUIT))
    moons->solinari_st++;
  if (mask & (MOON_CONJUNCTION_SOL_LUN | MOON_CONJUNCTION_LUN_NUIT))
    moons->lunitari_st++;
  if (mask & (MOON_CONJUNCTION_SOL_NUIT | MOON_CONJUNCTION_LUN_NUIT))
    moons->nuitari_st++;
  if (mask & MOON_CONJUNCTION_SOL_LUN)
  {
    moons->solinari_sp++; moons->solinari_lv++;
  }
  if (mask & (MOON_CONJUNCTION_SOL_LUN | MOON_CONJUNCTION_LUN_NUIT))
  {
    moons->lunitari_sp++; moons->lunitari_lv++;
  }
  if (mask & MOON_CONJUNCTION_LUN_NUIT)
  {
    moons->nuitari_sp++; moons->nuitari_lv++;
  }
}

void show_moon_conjunctions(struct char_data *ch)
{
  int mask = get_moon_conjunctions();
  if (!mask)
    return;
  if (mask & MOON_CONJUNCTION_ALL)
  {
    if (weather_info.moons.solinari_phase > 18 && weather_info.moons.solinari_phase <= 27)
      send_to_char(ch, "The Night of the Eye: all three moons align at High Sanction!\r\n");
    else
      send_to_char(ch, "All three moons of magic are in conjunction.\r\n");
  }
  else
  {
    if (mask & MOON_CONJUNCTION_SOL_LUN)
      send_to_char(ch, "Solinari and Lunitari are in conjunction.\r\n");
    if (IS_EVIL(ch) || GET_LEVEL(ch) >= LVL_IMMORT)
    {
      if (mask & MOON_CONJUNCTION_SOL_NUIT)
        send_to_char(ch, "Solinari and Nuitari are in conjunction.\r\n");
      if (mask & MOON_CONJUNCTION_LUN_NUIT)
        send_to_char(ch, "Lunitari and Nuitari are in conjunction.\r\n");
    }
  }
}
