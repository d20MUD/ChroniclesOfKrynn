/**
 * @file introduce.c
 * @author Zusuk
 * @brief Introduction system - allows players to introduce themselves to each other
 *
 * This file contains the implementation of the introduction system, which allows
 * players to introduce themselves to other players. When CONFIG_USE_INTRO_SYSTEM
 * is enabled, players will see short descriptions instead of names until they've
 * been introduced.
 *
 * Part of the LuminariMUD project.
 */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "screen.h"
#include "constants.h"
#include "act.h"
#include "mud_event.h"
#include "spells.h"
#include "class.h"
#include "spec_procs.h"
#include "char_descs.h"
#include "roleplay.h"
#include "race.h"

/**
 * Check if one character knows another through introduction
 *
 * @param ch The character checking
 * @param vict The character being checked
 * @return TRUE if ch knows vict, FALSE otherwise
 */
bool knows_character(struct char_data *ch, struct char_data *vict)
{
  int i;

  if (!ch || !vict)
    return FALSE;

  /* NPCs and immortals always know everyone */
  if (IS_NPC(ch) || IS_NPC(vict))
    return TRUE;

  if (GET_LEVEL(ch) >= LVL_IMMORT || GET_LEVEL(vict) >= LVL_IMMORT)
    return TRUE;

  /* If intro system is disabled, everyone knows everyone */
  if (!CONFIG_USE_INTRO_SYSTEM)
    return TRUE;

  /* You always know yourself */
  if (ch == vict)
    return TRUE;

  /* Check if victim's name is in ch's introduction list */
  for (i = 0; i < MAX_INTROS; i++)
  {
    if (ch->player_specials->saved.intro_list[i] == NULL)
      break; /* End of list */

    if (!str_cmp(ch->player_specials->saved.intro_list[i], GET_NAME(vict)))
      return TRUE;
  }

  return FALSE;
}

/**
 * Add a character to another's introduction list
 *
 * @param ch The character who is learning the name
 * @param vict The character being learned
 * @return TRUE if successfully added, FALSE if list is full or already known
 */
bool add_introduction(struct char_data *ch, struct char_data *vict)
{
  int i;

  /* Don't add NPCs or immortals to intro lists */
  if (IS_NPC(ch) || IS_NPC(vict))
    return FALSE;

  /* You always know yourself */
  if (ch == vict)
    return FALSE;

  /* Check if already in list */
  if (knows_character(ch, vict))
    return FALSE;

  /* Find first empty slot and add */
  for (i = 0; i < MAX_INTROS; i++)
  {
    if (ch->player_specials->saved.intro_list[i] == NULL)
    {
      ch->player_specials->saved.intro_list[i] = strdup(GET_NAME(vict));
      save_char(ch, 0);
      return TRUE;
    }
  }

  /* List is full */
  return FALSE;
}

/**
 * The introduce command
 *
 * Usage: introduce <person>  - Introduce yourself to someone
 *        introduce list      - List people who have introduced themselves to you
 */
ACMD(do_introduce)
{
  struct char_data *vict;
  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  int i, j, count = 0;
  bool found = FALSE;
  const char *known_names[MAX_INTROS];
  char name_buf[MAX_NAME_LENGTH + 1];

  /* Check if introduction system is enabled */
  if (!CONFIG_USE_INTRO_SYSTEM)
  {
    send_to_char(ch, "The introduction system is not enabled on this MUD.\r\n");
    return;
  }

  if (IS_NPC(ch))
    return;

  two_arguments(argument, arg, sizeof(arg), arg2, sizeof(arg2));

  if (!*arg)
  {
    send_to_char(ch, "Introduce yourself to whom?\r\n"
                     "Usage: introduce <person>\r\n"
                     "       introduce list\r\n"
                     "       introduce forget <person>\r\n");
    return;
  }

  /* Check if they want to forget someone */
  if (is_abbrev(arg, "forget"))
  {
    if (!*arg2)
    {
      send_to_char(ch, "Forget whom?\r\n"
                       "Usage: introduce forget <person>\r\n");
      return;
    }

    /* Search for the name in the introduction list */
    for (i = 0; i < MAX_INTROS; i++)
    {
      if (ch->player_specials->saved.intro_list[i] == NULL)
        break; /* End of list */

      if (!str_cmp(ch->player_specials->saved.intro_list[i], arg2))
      {
        /* Found the name - remove it */
        found = TRUE;

        /* Free the memory for this name */
        free(ch->player_specials->saved.intro_list[i]);

        /* Shift all subsequent names down one slot */
        for (j = i; j < MAX_INTROS - 1; j++)
        {
          ch->player_specials->saved.intro_list[j] = ch->player_specials->saved.intro_list[j + 1];
          if (ch->player_specials->saved.intro_list[j] == NULL)
            break;
        }

        /* Ensure the last slot is NULL */
        ch->player_specials->saved.intro_list[MAX_INTROS - 1] = NULL;

        save_char(ch, 0);
        send_to_char(ch, "You have forgotten %s.\r\n", CAP(arg2));
        return;
      }
    }

    if (!found)
    {
      send_to_char(ch, "You don't know anyone by that name.\r\n");
    }
    return;
  }

  /* Check if they want to list known people */
  if (is_abbrev(arg, "list"))
  {
    send_to_char(ch, "\tYPeople who have introduced themselves to you:\tn\r\n");
    send_to_char(ch, "\tc----------------------------------------\tn\r\n");

    /* Build array of known names */
    for (i = 0; i < MAX_INTROS; i++)
    {
      if (ch->player_specials->saved.intro_list[i] == NULL)
        break; /* End of list */

      /* Copy and capitalize the name */
      strncpy(name_buf, ch->player_specials->saved.intro_list[i], sizeof(name_buf) - 1);
      name_buf[sizeof(name_buf) - 1] = '\0';
      CAP(name_buf);
      known_names[count++] = strdup(name_buf);
    }

    if (count == 0)
    {
      send_to_char(ch, "  No one has introduced themselves to you yet.\r\n");
    }
    else
    {
      /* Display in 3 columns */
      column_list(ch, 3, known_names, count, FALSE);

      /* Free the duplicated strings */
      for (i = 0; i < count; i++)
        free((void *)known_names[i]);
    }
    return;
  }

  if (HAS_DISGUISE(ch))
  {
    send_to_char(ch, "Remove your disguise before introducing yourself.\r\n");
    return;
  }

  /* Find the target */
  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
  {
    send_to_char(ch, "%s", CONFIG_NOPERSON);
    return;
  }

  /* Can't introduce to yourself */
  if (vict == ch)
  {
    send_to_char(ch, "You already know who you are.\r\n");
    return;
  }

  /* Can't introduce to NPCs */
  if (IS_NPC(vict))
  {
    send_to_char(ch, "There's no need to introduce yourself to %s.\r\n", show_pers(vict, ch));
    return;
  }

  /* Check if already introduced */
  if (knows_character(vict, ch))
  {
    send_to_char(ch, "%s already knows who you are.\r\n", show_pers(vict, ch));
    return;
  }

  /* Check if ch can act */
  if (char_has_mud_event(ch, eSTUNNED))
  {
    send_to_char(ch, "You are stunned and cannot act!\r\n");
    return;
  }

  if (AFF_FLAGGED(ch, AFF_PARALYZED))
  {
    send_to_char(ch, "You are paralyzed and cannot move!\r\n");
    return;
  }

  /* Perform the introduction - add ch's name to vict's list */
  if (add_introduction(vict, ch))
  {
    send_to_char(ch, "You introduce yourself to %s.\r\n", show_pers(vict, ch));
    send_to_char(vict, "%s introduces %sself to you.\r\n", GET_NAME(ch), GET_SEX(ch) == SEX_FEMALE ? "her" : GET_SEX(ch) == SEX_MALE ? "him" : "it");
    act("$n introduces $mself to $N.", FALSE, ch, 0, vict, TO_NOTVICT);
  }
  else
  {
    send_to_char(ch, "%s's introduction list is full!\r\n", show_pers(vict, ch));
  }
}

/* True sight is a temporary override, never a permanent cached success. */
static bool check_disguise(struct char_data *observer, struct char_data *subject, bool seen)
{
  struct disguise_observation *entry;
  bool true_sight, visible;
  if (!observer || !subject || !HAS_DISGUISE(subject))
    return FALSE;
  if (observer == subject || GET_LEVEL(observer) >= LVL_IMMORT)
    return TRUE;
  true_sight = has_true_sight(observer);
  if (IS_NPC(observer))
    return true_sight;
  visible = CAN_SEE(observer, subject) && (seen || IN_ROOM(observer) == IN_ROOM(subject)) &&
            AWAKE(observer);
  for (entry = observer->player_specials->saved.disguise_observations; entry; entry = entry->next)
    if (entry->subject == GET_IDNUM(subject))
      break;
  if (entry && entry->instance == COSMETIC_DISGUISE(subject).instance && !true_sight)
  {
    entry->true_sight_noticed = FALSE;
    if (entry->detected >= 0)
      return entry->detected;
  }
  if (!visible)
    return true_sight;
  if (!entry)
  {
    CREATE(entry, struct disguise_observation, 1);
    entry->subject = GET_IDNUM(subject);
    entry->next = observer->player_specials->saved.disguise_observations;
    observer->player_specials->saved.disguise_observations = entry;
  }
  if (entry->instance != COSMETIC_DISGUISE(subject).instance)
  {
    entry->instance = COSMETIC_DISGUISE(subject).instance;
    entry->detected = -1;
    entry->notice_pending = FALSE;
    entry->true_sight_noticed = FALSE;
  }
  if (true_sight)
  {
    if (!entry->true_sight_noticed)
      entry->notice_pending = TRUE;
    entry->true_sight_noticed = TRUE;
    return TRUE;
  }
  entry->detected = d20(observer) + compute_ability(observer, ABILITY_PERCEPTION) >=
                    COSMETIC_DISGUISE(subject).roll;
  entry->notice_pending = entry->detected;
  save_char(observer, 0);
  return entry->detected;
}

bool disguise_detected(struct char_data *observer, struct char_data *subject)
{
  return check_disguise(observer, subject, FALSE);
}

/* Called by visual displays such as scanning into an adjacent room. */
void observe_disguise(struct char_data *observer, struct char_data *subject)
{
  check_disguise(observer, subject, TRUE);
}

bool identity_known(struct char_data *observer, struct char_data *subject)
{
  if (!observer || !subject)
    return FALSE;
  if (HAS_DISGUISE(subject) && !disguise_detected(observer, subject))
    return FALSE;
  return knows_character(observer, subject);
}

bool identity_matches(struct char_data *observer, struct char_data *subject, const char *name)
{
  char *desc;
  bool matches;
  if (!observer || !subject || !name)
    return FALSE;
  if (IS_NPC(subject) || GET_LEVEL(observer) >= LVL_IMMORT || observer == subject)
    return isname(name, subject->player.name);
  if (identity_known(observer, subject) && isname(name, subject->player.name))
    return TRUE;
  desc = which_desc(observer, subject);
  matches = desc && isname(name, desc);
  free(desc);
  return matches;
}

int visible_sex(struct char_data *subject, struct char_data *observer)
{
  if (HAS_DISGUISE(subject) && !disguise_detected(observer, subject))
    return COSMETIC_DISGUISE(subject).sex;
  return GET_SEX(subject);
}

/* Context follows the primary true-form display, rather than replacing it. */
static unsigned long long true_form_signature(struct char_data *subject)
{
  unsigned long long signature = 1;
  signature = signature * 257 + (GET_REAL_RACE(subject) + 1);
  signature = signature * 257 + (GET_DISGUISE_RACE(subject) + 1);
  signature = signature * 257 + IS_MORPHED(subject);
  signature = signature * 257 + SUBRACE(subject);
  signature = signature * 257 + (IS_ALTER_SELF(subject) ? ALTER_SELF_RACE(subject) + 1 : 0);
  signature = signature * 257 + AFF_FLAGGED(subject, AFF_WILD_SHAPE);
  signature ^= COSMETIC_DISGUISE(subject).instance;
  return signature ? signature : 1;
}

static struct disguise_observation *form_notice_entry(struct char_data *observer, struct char_data *subject)
{
  struct disguise_observation *entry;
  for (entry = observer->player_specials->saved.disguise_observations; entry; entry = entry->next)
    if (entry->subject == GET_IDNUM(subject))
      return entry;
  CREATE(entry, struct disguise_observation, 1);
  entry->subject = GET_IDNUM(subject);
  entry->detected = -1;
  entry->next = observer->player_specials->saved.disguise_observations;
  observer->player_specials->saved.disguise_observations = entry;
  return entry;
}

void show_disguise_notice(struct char_data *subject, struct char_data *observer)
{
  char *desc, *identity;
  bool true_sight;
  if (!subject || !observer || !CAN_SEE(observer, subject))
    return;
  true_sight = has_true_sight(observer);
  if (!(true_sight && has_altered_form(subject)) &&
      !(HAS_DISGUISE(subject) && disguise_detected(observer, subject)))
    return;
  identity = strdup(PERS(subject, observer));
  CAP(identity);
  if (true_sight)
  {
    desc = current_true_desc(subject);
    send_to_char(observer, "%s's true form is %s.\r\n", identity, desc);
    free(desc);
    if (has_altered_form(subject))
    {
      const char *kind = IS_ALTER_SELF(subject) ? "alter self" :
                         IS_WILDSHAPED(subject) ? "wildshaped" : "shapechanged";
      desc = current_altered_desc(subject);
      send_to_char(observer, "%s's altered form is %s (%s).\r\n", identity, desc, kind);
      free(desc);
    }
  }
  if (HAS_DISGUISE(subject) && disguise_detected(observer, subject))
  {
    desc = current_disguise_desc(subject);
    send_to_char(observer, "%s is disguised as %s.\r\n", identity, desc);
    free(desc);
  }
  free(identity);
  if (!IS_NPC(observer) && !IS_NPC(subject))
  {
    struct disguise_observation *entry = form_notice_entry(observer, subject);
    entry->notice_pending = FALSE;
    if (true_sight)
      entry->true_form_signature = true_form_signature(subject);
  }
}

/* Arrival, speech, and combat acts show context once when the visible form changes.
 * Explicit look/room/scan displays always show it. No recognition roll is added. */
void show_pending_disguise_notice(struct char_data *subject, struct char_data *observer)
{
  struct disguise_observation *entry;
  if (!subject || !observer || IS_NPC(observer) || IS_NPC(subject))
    return;
  if (has_true_sight(observer) && (has_altered_form(subject) || HAS_DISGUISE(subject)) &&
      CAN_SEE(observer, subject))
  {
    entry = form_notice_entry(observer, subject);
    if (entry->true_form_signature != true_form_signature(subject))
      show_disguise_notice(subject, observer);
    return;
  }
  for (entry = observer->player_specials->saved.disguise_observations; entry; entry = entry->next)
    if (entry->subject == GET_IDNUM(subject))
    {
      entry->true_form_signature = 0;
      if (HAS_DISGUISE(subject) && entry->instance == COSMETIC_DISGUISE(subject).instance &&
          entry->notice_pending)
        show_disguise_notice(subject, observer);
      return;
    }
}

void free_disguise_observations(struct char_data *ch)
{
  struct disguise_observation *entry, *next;
  if (IS_NPC(ch) || !ch->player_specials)
    return;
  for (entry = ch->player_specials->saved.disguise_observations; entry; entry = next)
  {
    next = entry->next;
    free(entry);
  }
  ch->player_specials->saved.disguise_observations = NULL;
}

void load_disguise_observation(struct char_data *ch, const char *line)
{
  struct disguise_observation *entry;
  long subject;
  unsigned long long instance;
  int detected;
  if (sscanf(line, "%ld %llu %d", &subject, &instance, &detected) != 3 || subject <= 0 ||
      !instance || (detected < -1 || detected > 1))
    return;
  for (entry = ch->player_specials->saved.disguise_observations; entry; entry = entry->next)
    if (entry->subject == subject)
      break;
  if (!entry)
  {
    CREATE(entry, struct disguise_observation, 1);
    entry->next = ch->player_specials->saved.disguise_observations;
    ch->player_specials->saved.disguise_observations = entry;
  }
  entry->subject = subject;
  entry->instance = instance;
  entry->detected = detected;
}

void load_cosmetic_disguise(struct char_data *ch, const char *line)
{
  struct disguise_appearance appearance;
  memset(&appearance, 0, sizeof(appearance));
  if (sscanf(line, "%llu %d %d %d %d %d %d %d %d", &appearance.instance, &appearance.race,
             &appearance.age, &appearance.sex, &appearance.roll, &appearance.descriptor1,
             &appearance.adjective1, &appearance.descriptor2, &appearance.adjective2) != 9 ||
      !appearance.instance || appearance.race < 0 || appearance.race >= NUM_EXTENDED_RACES ||
      appearance.age < 0 || appearance.age >= NUM_CHARACTER_AGES ||
      appearance.sex < SEX_NEUTRAL || appearance.sex > SEX_FEMALE ||
      !valid_short_desc_feature(appearance.descriptor1, appearance.adjective1) ||
      !valid_short_desc_feature(appearance.descriptor2, appearance.adjective2))
    return;
  COSMETIC_DISGUISE(ch) = appearance;
  if (appearance.instance > ch->player_specials->saved.disguise_generation)
    ch->player_specials->saved.disguise_generation = appearance.instance;
}
