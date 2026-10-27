/**************************************************************************
 *  File: mob_utils.c                                 Part of LuminariMUD *
 *  Usage: Mobile utility functions                                       *
 *                                                                         *
 *  All rights reserved.  See license for complete information.           *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.              *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "spells.h"
#include "act.h"
#include "graph.h"
#include "fight.h"
#include "mud_event.h"
#include "modify.h"
#include "mob_memory.h"
#include "mob_utils.h"

/* External declarations */
extern const char *dirs[];

void save_char_pets(struct char_data *ch);

/* XP is spent on advancement, rather than becoming a bounty for killing a pet. */
int purchased_pet_level_cost(struct char_data *pet)
{
  return 2000 * (GET_LEVEL(pet) + 1);
}

void award_purchased_pet_exp(struct char_data *owner, struct char_data *victim, int reward)
{
  struct follow_type *f;
  bool changed = FALSE;
  int cap;

  if (!owner || IS_NPC(owner) || !victim || !IS_NPC(victim) ||
      AFF_FLAGGED(victim, AFF_CHARM) || victim->mob_specials.purchased_pet ||
      reward <= 1 || IN_ROOM(owner) == NOWHERE || IN_ROOM(owner) != IN_ROOM(victim))
    return;

  cap = MIN(GET_LEVEL(owner), PURCHASED_PET_LEVEL_CAP);
  for (f = owner->followers; f; f = f->next)
  {
    struct char_data *pet = f->follower;
    int gain, cost, max_progress = 0, level;
    if (!IS_PET(pet) || pet->master != owner || !pet->mob_specials.purchased_pet ||
        IN_ROOM(pet) != IN_ROOM(owner) || GET_POS(pet) <= POS_STUNNED ||
        FIGHTING(pet) != victim || GET_LEVEL(pet) < 1 || GET_LEVEL(pet) >= cap)
      continue;

    /* Bound the bank to the XP needed to reach the owner's level cap. */
    for (level = GET_LEVEL(pet) + 1; level <= cap; level++)
      max_progress += 2000 * level;
    pet->mob_specials.pet_experience = MAX(0, MIN(max_progress, pet->mob_specials.pet_experience));
    gain = MIN(reward, max_progress - pet->mob_specials.pet_experience);
    pet->mob_specials.pet_experience += gain;
    changed = TRUE;
    send_to_char(owner, "%s gains %d pet experience points.\r\n", GET_NAME(pet), gain);

    while (GET_LEVEL(pet) < cap &&
           pet->mob_specials.pet_experience >= purchased_pet_level_cost(pet))
    {
      cost = purchased_pet_level_cost(pet);
      int hit = GET_HIT(pet);
      pet->mob_specials.pet_experience -= cost;
      GET_LEVEL(pet)++;
      GET_REAL_MAX_HIT(pet) += 10;
      GET_REAL_AC(pet) = MIN(32767, GET_REAL_AC(pet) + 10);
      if (GET_LEVEL(pet) % 2 == 0)
      {
        GET_REAL_HITROLL(pet) = MIN(127, GET_REAL_HITROLL(pet) + 1);
        GET_REAL_DAMROLL(pet) = MIN(127, GET_REAL_DAMROLL(pet) + 1);
      }
      if (GET_LEVEL(pet) % 4 == 0)
      {
        GET_REAL_SAVE(pet, SAVING_FORT)++;
        GET_REAL_SAVE(pet, SAVING_REFL)++;
        GET_REAL_SAVE(pet, SAVING_WILL)++;
        GET_REAL_SAVE(pet, SAVING_POISON)++;
        GET_REAL_SAVE(pet, SAVING_DEATH)++;
      }
      affect_total(pet);
      GET_HIT(pet) = hit; /* Growing does not heal wounds. */
      send_to_char(owner, "\tG%s has grown to level %d!\tn\r\n", GET_NAME(pet), GET_LEVEL(pet));
    }
  }
  if (changed)
    save_char_pets(owner);
}

/* Temporary summons and eidolon reflections do not retain training. */
int companion_training_kind(struct char_data *pet)
{
  if (!pet || !IS_NPC(pet) || pet->mob_specials.purchased_pet || char_has_mud_event(pet, ePURGEMOB))
    return 0;
  if (MOB_FLAGGED(pet, MOB_C_ANIMAL)) return 1;
  if (MOB_FLAGGED(pet, MOB_C_FAMILIAR)) return 2;
  if (MOB_FLAGGED(pet, MOB_C_MOUNT)) return 3;
  if (MOB_FLAGGED(pet, MOB_EIDOLON)) return 4;
  if (MOB_FLAGGED(pet, MOB_C_DRAGON)) return 5;
  if (MOB_FLAGGED(pet, MOB_GOLEM)) return 6;
  if (MOB_FLAGGED(pet, MOB_MERCENARY)) return 7;
  if (MOB_FLAGGED(pet, MOB_MOUNTABLE)) return 8;
  return 0;
}

void restore_companion_kind(struct char_data *pet, int kind)
{
  const int flags[] = {MOB_C_ANIMAL, MOB_C_FAMILIAR, MOB_C_MOUNT, MOB_EIDOLON,
                       MOB_C_DRAGON, MOB_GOLEM, MOB_MERCENARY, MOB_MOUNTABLE};
  if (kind >= 1 && kind <= 8)
    SET_BIT_AR(MOB_FLAGS(pet), flags[kind - 1]);
}

bool is_persistent_pet(struct char_data *pet)
{
  return pet && IS_PET(pet) && !IS_NPC(pet->master) &&
         !char_has_mud_event(pet, ePURGEMOB) &&
         (pet->mob_specials.purchased_pet || companion_training_kind(pet) > 0);
}

/* The main eidolon is a continuing bond, even when its summoned body is gone.
 * Temporary reflections never read or write that bond's training. */
void remember_eidolon_training(struct char_data *pet)
{
  struct char_data *owner;
  int i, changed;
  if (companion_training_kind(pet) != 4 || !(owner = pet->master) || IS_NPC(owner))
    return;
  /* A stale stored body or an expiring, untrained reflection must never roll
   * the continuing bond backwards, even after its purge event is removed. */
  if (owner->player_specials->saved.eidolon_training_saved &&
      (pet->mob_specials.pet_training_rank < owner->player_specials->saved.eidolon_training_rank ||
       (pet->mob_specials.pet_training_rank == owner->player_specials->saved.eidolon_training_rank &&
        pet->mob_specials.pet_training_experience < owner->player_specials->saved.eidolon_training_experience)))
    return;
  if (owner->player_specials->saved.eidolon_training_saved)
    for (i = 0; i < PET_TRAINING_SKILLS; i++)
      if (pet->mob_specials.pet_training_skills[i] < owner->player_specials->saved.eidolon_training_skills[i])
        return;
  changed = !owner->player_specials->saved.eidolon_training_saved ||
      owner->player_specials->saved.eidolon_training_rank != pet->mob_specials.pet_training_rank ||
      owner->player_specials->saved.eidolon_training_experience != pet->mob_specials.pet_training_experience;
  for (i = 0; i < PET_TRAINING_SKILLS; i++)
    if (owner->player_specials->saved.eidolon_training_skills[i] != pet->mob_specials.pet_training_skills[i])
      changed = TRUE;
  if (!changed)
    return;
  owner->player_specials->saved.eidolon_training_saved = TRUE;
  owner->player_specials->saved.eidolon_training_rank = pet->mob_specials.pet_training_rank;
  owner->player_specials->saved.eidolon_training_experience = pet->mob_specials.pet_training_experience;
  for (i = 0; i < PET_TRAINING_SKILLS; i++)
    owner->player_specials->saved.eidolon_training_skills[i] = pet->mob_specials.pet_training_skills[i];
  save_char(owner, 0);
}

void restore_eidolon_training(struct char_data *owner, struct char_data *pet)
{
  int i, delta, hit;
  if (!owner || IS_NPC(owner) || companion_training_kind(pet) != 4 ||
      !owner->player_specials->saved.eidolon_training_saved)
    return;
  hit = GET_HIT(pet);
  /* Apply only the difference: DB-loaded bodies already include their bonuses. */
  for (i = 0; i < PET_TRAINING_SKILLS; i++)
  {
    delta = owner->player_specials->saved.eidolon_training_skills[i] - pet->mob_specials.pet_training_skills[i];
    switch (i)
    {
    case 0: GET_REAL_MAX_HIT(pet) += 20 * delta; break;
    case 1: GET_REAL_HITROLL(pet) = MIN(127, MAX(-128, GET_REAL_HITROLL(pet) + delta)); break;
    case 2: GET_REAL_DAMROLL(pet) = MIN(127, MAX(-128, GET_REAL_DAMROLL(pet) + delta)); break;
    case 3: GET_REAL_AC(pet) = MIN(32767, MAX(-32768, GET_REAL_AC(pet) + 10 * delta)); break;
    case 4:
      {
        int save;
        for (save = 0; save < NUM_OF_SAVING_THROWS; save++) GET_REAL_SAVE(pet, save) += delta;
      }
      break;
    }
    pet->mob_specials.pet_training_skills[i] = owner->player_specials->saved.eidolon_training_skills[i];
  }
  pet->mob_specials.pet_training_rank = owner->player_specials->saved.eidolon_training_rank;
  pet->mob_specials.pet_training_experience = owner->player_specials->saved.eidolon_training_experience;
  affect_total(pet);
  GET_HIT(pet) = MIN(hit, GET_MAX_HIT(pet));
}

static int companion_unspent_points(struct char_data *pet)
{
  int i, points = pet->mob_specials.pet_training_rank;
  for (i = 0; i < PET_TRAINING_SKILLS; i++)
    points -= pet->mob_specials.pet_training_skills[i];
  return MAX(0, points);
}

void award_companion_training_exp(struct char_data *owner, struct char_data *victim, int reward)
{
  struct follow_type *f;
  bool changed = FALSE;
  if (!owner || IS_NPC(owner) || !victim || !IS_NPC(victim) ||
      AFF_FLAGGED(victim, AFF_CHARM) || victim->mob_specials.purchased_pet || reward <= 1 ||
      IN_ROOM(owner) == NOWHERE || IN_ROOM(owner) != IN_ROOM(victim))
    return;
  for (f = owner->followers; f; f = f->next)
  {
    struct char_data *pet = f->follower;
    int rank, bank = 0, gain;
    if (!is_persistent_pet(pet) || pet->master != owner || !companion_training_kind(pet) ||
        IN_ROOM(pet) != IN_ROOM(owner) || GET_POS(pet) <= POS_STUNNED ||
        (FIGHTING(pet) != victim && RIDDEN_BY(pet) != owner && !MOB_FLAGGED(pet, MOB_C_FAMILIAR)) ||
        pet->mob_specials.pet_training_rank >= PET_TRAINING_RANK_CAP)
      continue;
    for (rank = pet->mob_specials.pet_training_rank + 1; rank <= PET_TRAINING_RANK_CAP; rank++)
      bank += 2000 * rank;
    pet->mob_specials.pet_training_experience = MAX(0, MIN(bank, pet->mob_specials.pet_training_experience));
    gain = MIN(reward, bank - pet->mob_specials.pet_training_experience);
    pet->mob_specials.pet_training_experience += gain;
    changed = TRUE;
    send_to_char(owner, "%s gains %d companion training XP.\r\n", GET_NAME(pet), gain);
    while (pet->mob_specials.pet_training_rank < PET_TRAINING_RANK_CAP &&
           pet->mob_specials.pet_training_experience >= 2000 * (pet->mob_specials.pet_training_rank + 1))
    {
      pet->mob_specials.pet_training_experience -= 2000 * (pet->mob_specials.pet_training_rank + 1);
      pet->mob_specials.pet_training_rank++;
      send_to_char(owner, "\tG%s reaches training rank %d and earns a skill point!\tn\r\n",
                   GET_NAME(pet), pet->mob_specials.pet_training_rank);
    }
  }
  if (changed)
    save_char_pets(owner);
}

void show_companion_training(struct char_data *owner, struct char_data *pet)
{
  if (!companion_training_kind(pet))
    return;
  if (pet->mob_specials.pet_training_rank >= PET_TRAINING_RANK_CAP)
    send_to_char(owner, "%s: training rank %d/%d (maximum), unspent points %d.\r\n",
                 GET_NAME(pet), pet->mob_specials.pet_training_rank, PET_TRAINING_RANK_CAP,
                 companion_unspent_points(pet));
  else
    send_to_char(owner, "%s: training rank %d/%d, XP %d/%d, unspent points %d.\r\n",
                 GET_NAME(pet), pet->mob_specials.pet_training_rank, PET_TRAINING_RANK_CAP,
                 pet->mob_specials.pet_training_experience, 2000 * (pet->mob_specials.pet_training_rank + 1),
                 companion_unspent_points(pet));
  send_to_char(owner, "  Skills (maximum 5 each): health %d, attack %d, damage %d, armor %d, saves %d.\r\n",
               pet->mob_specials.pet_training_skills[0], pet->mob_specials.pet_training_skills[1],
               pet->mob_specials.pet_training_skills[2], pet->mob_specials.pet_training_skills[3],
               pet->mob_specials.pet_training_skills[4]);
}

void train_companion_skill(struct char_data *owner, struct char_data *pet, const char *skill)
{
  const char *names[] = {"health", "attack", "damage", "armor", "saves"};
  int i, index = -1, hit;
  if (!pet || !is_persistent_pet(pet) || pet->master != owner || !companion_training_kind(pet) ||
      IN_ROOM(pet) != IN_ROOM(owner))
  {
    send_to_char(owner, "Train one of your permanent companions here.\r\n");
    return;
  }
  if (FIGHTING(owner) || FIGHTING(pet))
  {
    send_to_char(owner, "Finish combat before training your companion.\r\n");
    return;
  }
  for (i = 0; i < PET_TRAINING_SKILLS; i++)
    if (!str_cmp(skill, names[i])) index = i;
  if (index < 0)
  {
    send_to_char(owner, "Skills: health (+20 HP), attack (+1), damage (+1), armor (+1 AC), saves (+1 all).\r\n");
    return;
  }
  if (pet->mob_specials.pet_training_skills[index] >= PET_TRAINING_SKILL_CAP || !companion_unspent_points(pet))
  {
    send_to_char(owner, "You need an unspent pet skill point and fewer than five purchases in that skill.\r\n");
    return;
  }
  hit = GET_HIT(pet);
  pet->mob_specials.pet_training_skills[index]++;
  switch (index)
  {
  case 0: GET_REAL_MAX_HIT(pet) += 20; break;
  case 1: GET_REAL_HITROLL(pet) = MIN(127, GET_REAL_HITROLL(pet) + 1); break;
  case 2: GET_REAL_DAMROLL(pet) = MIN(127, GET_REAL_DAMROLL(pet) + 1); break;
  case 3: GET_REAL_AC(pet) = MIN(32767, GET_REAL_AC(pet) + 10); break;
  case 4:
    for (i = 0; i < NUM_OF_SAVING_THROWS; i++) GET_REAL_SAVE(pet, i)++;
    break;
  }
  affect_total(pet);
  GET_HIT(pet) = hit;
  save_char_pets(owner);
  send_to_char(owner, "You spend one pet skill point to train %s in %s.\r\n", GET_NAME(pet), names[index]);
}

/*** UTILITY FUNCTIONS ***/

/* function to return possible targets for mobile, used for
 * npc that is fighting  */
struct char_data *npc_find_target(struct char_data *ch, int *num_targets)
{
  struct list_data *target_list = NULL;
  struct char_data *tch = NULL;

  if (!ch || IN_ROOM(ch) == NOWHERE || !FIGHTING(ch))
    return NULL;

  /* dynamic memory allocation required */
  target_list = create_list();

  /* loop through chars in room to find possible targets to build list */
  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
  {
    if (tch == ch)
      continue;

    if (!CAN_SEE(ch, tch))
      continue;

    if (!IS_NPC(tch) && PRF_FLAGGED(tch, PRF_NOHASSLE))
      continue;

    /* in mobile memory? */
    if (is_in_memory(ch, tch))
      add_to_list(tch, target_list);

    /* hunting target? */
    if (HUNTING(ch) == tch)
      add_to_list(tch, target_list);

    /* fighting me? */
    if (FIGHTING(tch) == ch)
      add_to_list(tch, target_list);

    /* me fighting? */
    if (FIGHTING(ch) == tch)
      add_to_list(tch, target_list);
  }

  /* did we snag anything? */
  if (target_list->iSize == 0)
  {
    free_list(target_list);
    return NULL;
  }

  /* ok should be golden, go ahead snag a random and free list */
  /* always can just return fighting target */
  tch = random_from_list(target_list);
  *num_targets = target_list->iSize; // yay pointers!

  if (target_list)
    free_list(target_list);

  if (tch) // dummy check
    return tch;
  else // backup plan
    return NULL;
}

/* a very simplified switch opponents engine */
bool npc_switch_opponents(struct char_data *ch, struct char_data *vict)
{
  // mudlog(NRM, LVL_IMMORT, TRUE, "%s trying to switch opponents!", ch->player.short_descr);
  if (!ch || !vict)
    return FALSE;

  if (!CAN_SEE(ch, vict))
    return FALSE;

  if (GET_POS(ch) <= POS_SITTING)
  {
    send_to_char(ch, "You are in no position to switch opponents!\r\n");
    return FALSE;
  }

  if (FIGHTING(ch) == vict)
  {
    send_to_char(ch, "You can't switch opponents to an opponent you are already fighting!\r\n");
    return FALSE;
  }

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SINGLEFILE))
  {
    if (ch->next_in_room != vict && vict->next_in_room != ch)
    {
      send_to_char(ch, "You can't reach to switch opponents!\r\n");
      return FALSE;
    }
  }

  /* should be a valid opponent */
  if (FIGHTING(ch))
    stop_fighting(ch);
  send_to_char(ch, "You switch opponents!\r\n");
  act("$n switches opponents to $N!", FALSE, ch, 0, vict, TO_ROOM);
  hit(ch, vict, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);

  return TRUE;
}

/* this function will attempt to rescue
   1)  master
   2)  group member
 * returns TRUE if rescue attempt is made
 */
/* how many times will you loop group list to find rescue target cap */
#define RESCUE_LOOP 20
bool npc_rescue(struct char_data *ch)
{
  if (!ch)
    return false;

  if (ch->master && !IS_NPC(ch->master) && PRF_FLAGGED(ch->master, PRF_NO_CHARMIE_RESCUE))
    return false;

  struct char_data *victim = NULL;
  int loop_counter = 0;

  if (GET_HIT(ch) <= 1)
    return FALSE; /* too weak */

  // going to prioritize rescuing master (if it has one)
  if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master && !rand_number(0, 1) &&
      (GET_MAX_HIT(ch) / GET_HIT(ch)) <= 2)
  {
    if (FIGHTING(ch->master) && ((GET_MAX_HIT(ch->master) / GET_HIT(ch->master)) <= 3))
    {
      perform_rescue(ch, ch->master);
      return TRUE;
    }
  }

  /* determine victim (someone in group, including self) */
  if (GROUP(ch) && GROUP(ch)->members->iSize && !rand_number(0, 1) &&
      (GET_MAX_HIT(ch) / GET_HIT(ch)) <= 2)
  {
    do
    {
      victim = (struct char_data *)random_from_list(GROUP(ch)->members);

      if (!victim || GET_HIT(victim) <= 1)
        continue;

      loop_counter++;

      if (loop_counter >= RESCUE_LOOP)
        break;

    } while (!victim || victim == ch || !FIGHTING(victim) ||
             ((GET_MAX_HIT(victim) / MAX(1, GET_HIT(victim))) > 3));

    if (loop_counter < RESCUE_LOOP && FIGHTING(victim))
    {
      perform_rescue(ch, victim);
      return TRUE;
    }
  }

  return FALSE;
}
#undef RESCUE_LOOP

/* function to move a mobile along a specified path (patrols) */
bool move_on_path(struct char_data *ch)
{
  int dir = -1;
  int next = 0;

  if (!ch)
    return FALSE;

  if (FIGHTING(ch))
    return FALSE;

  if (IN_ROOM(ch) == NOWHERE)
    return FALSE;

  /* finished path */
  if (PATH_SIZE(ch) < 1)
    return FALSE;

  /* stuck in a spot for a moment */
  if (PATH_DELAY(ch) > 0)
  {
    PATH_DELAY(ch)--;
    return FALSE;
  }

  send_to_char(ch, "OK, I am in room %d (%d)...  ", GET_ROOM_VNUM(IN_ROOM(ch)), IN_ROOM(ch));

  PATH_DELAY(ch) = PATH_RESET(ch);

  if (PATH_INDEX(ch) >= PATH_SIZE(ch) || PATH_INDEX(ch) < 0)
    PATH_INDEX(ch) = 0;

  next = GET_PATH(ch, PATH_INDEX(ch));

  if (next == NOWHERE)
  {
    send_to_char(ch, "Hrm, it appears I have no where to go!.\r\n");
    return FALSE;
  }

  send_to_char(ch, "PATH:  Path-Index:  %d, Next (get-path vnum):  %d (%d).\r\n", PATH_INDEX(ch),
               next, real_room(next));

  dir = find_first_step(IN_ROOM(ch), real_room(next));

  if (EXIT(ch, dir)->to_room != real_room(next))
  {
    send_to_char(ch, "Hrm, it appears I am off-path...\r\n");
    send_to_char(ch,
                 "I want to go %s, which is room %d, but I need to get to"
                 " room %d..\r\n",
                 dirs[dir], GET_ROOM_VNUM(EXIT(ch, dir)->to_room), next);
  }
  else
  {
    send_to_char(ch, "... it looks like my path is perfect so far!\r\n");
  }

  switch (dir)
  {
  case BFS_ERROR:
    send_to_char(ch, "Hmm.. something seems to be seriously wrong.\r\n");
    log("PATH ERROR: Mob %s, in room %d, trying to get to %d", GET_NAME(ch),
        world[IN_ROOM(ch)].number, next);
    break;
  case BFS_ALREADY_THERE:
    send_to_char(ch, "I seem to be in the right room already!\r\n");
    break;
  case BFS_NO_PATH:
    send_to_char(ch, "I can't sense a trail to %d (%d) from here.\r\n", next, real_room(next));
    // log("NO PATH: Mob %s, in room %d, trying to get to %d", GET_NAME(ch), world[IN_ROOM(ch)].number, next);
    break;
  default: /* Success! */
    send_to_char(ch, "I sense a trail %s from here!\r\n", dirs[dir]);
    perform_move(ch, dir, 1);
    break;
  }

  PATH_INDEX(ch)
  ++;

  return TRUE;
}

/* mobile echo system, from homeland ported by nashak */
void mobile_echos(struct char_data *ch)
{
  char *echo = NULL;
  struct descriptor_data *d = NULL;

  if (!ECHO_COUNT(ch))
    return;
  if (!ECHO_ENTRIES(ch))
    return;

  if (rand_number(1, 75) > (ECHO_FREQ(ch) / 4))
    return;

  if (CURRENT_ECHO(ch) > ECHO_COUNT(ch)) /* dummy check */
    CURRENT_ECHO(ch) = 0;

  if (ECHO_SEQUENTIAL(ch))
  {
    echo = ECHO_ENTRIES(ch)[CURRENT_ECHO(ch)++];
    if (CURRENT_ECHO(ch) >= ECHO_COUNT(ch))
      CURRENT_ECHO(ch) = 0;
  }
  else
  {
    echo = ECHO_ENTRIES(ch)[rand_number(0, ECHO_COUNT(ch) - 1)];
  }

  if (!echo)
    return;

  parse_at(echo);

  if (ECHO_IS_ZONE(ch))
  {
    for (d = descriptor_list; d; d = d->next)
    {
      if (!d->character)
        continue;
      if (d->character->in_room == NOWHERE || ch->in_room == NOWHERE)
        continue;
      if (world[d->character->in_room].zone != world[ch->in_room].zone)
        continue;
      if (!AWAKE(d->character))
        continue;

      if (!PLR_FLAGGED(d->character, PLR_WRITING))
        send_to_char(d->character, "%s\r\n", echo);
    }
  }
  else
  {
    act(echo, FALSE, ch, 0, 0, TO_ROOM);
  }
}

/* function encapsulating conditions that will stop the mobile from
 acting */
int can_continue(struct char_data *ch, bool fighting)
{
  // dummy checks
  if (fighting && (!FIGHTING(ch) || IN_ROOM(ch) != IN_ROOM(FIGHTING(ch))))
  {
    stop_fighting(ch);
    return 0;
  }
  if (GET_MOB_WAIT(ch) > 0)
    return 0;
  if (HAS_WAIT(ch))
    return 0;
  if (GET_POS(ch) <= POS_SITTING)
    return 0;
  if (IS_CASTING(ch))
    return 0;
  if (GET_HIT(ch) <= 0)
    return 0;
  /* almost finished victims, they will stop using these skills -zusuk */
  if (FIGHTING(ch) && GET_HIT(FIGHTING(ch)) <= 5)
    return 0;
  return 1;
}

/* Utility function to check if NPC should call a companion
 * Returns true if NPC should attempt to call companion
 * Takes into account combat state, existing companions, and appropriate timing
 */
bool npc_should_call_companion(struct char_data *ch, int call_type)
{
  struct follow_type *k = NULL;

  /* Basic checks */
  if (!ch || !IS_NPC(ch))
    return FALSE;

  /* Don't call if we're almost dead */
  if (GET_HIT(ch) < (GET_MAX_HIT(ch) / 4))
    return FALSE;

  /* Check if companion already exists */
  for (k = ch->followers; k; k = k->next)
  {
    if (IS_NPC(k->follower) && AFF_FLAGGED(k->follower, AFF_CHARM) &&
        MOB_FLAGGED(k->follower, call_type))
    {
      /* Companion already exists */
      return FALSE;
    }
  }

  /* Class and feat checks */
  switch (call_type)
  {
  case MOB_C_ANIMAL:
    /* Rangers and Druids */
    if (GET_CLASS(ch) != CLASS_RANGER && GET_CLASS(ch) != CLASS_DRUID)
      return FALSE;
    if (GET_CLASS(ch) == CLASS_RANGER && GET_LEVEL(ch) < 4)
      return FALSE;
    break;

  case MOB_C_FAMILIAR:
    /* Wizards and Sorcerers */
    if (GET_CLASS(ch) != CLASS_WIZARD && GET_CLASS(ch) != CLASS_SORCERER)
      return FALSE;
    break;

  case MOB_C_MOUNT:
    /* Paladins and Blackguards */
    if (GET_CLASS(ch) != CLASS_PALADIN && GET_CLASS(ch) != CLASS_BLACKGUARD)
      return FALSE;
    break;

  case MOB_EIDOLON:
    /* Summoners and Necromancers */
    if (GET_CLASS(ch) != CLASS_SUMMONER && GET_CLASS(ch) != CLASS_NECROMANCER)
      return FALSE;
    break;

  case MOB_SHADOW:
    /* Shadowdancers */
    if (GET_CLASS(ch) != CLASS_SHADOWDANCER)
      return FALSE;
    break;

  case MOB_C_DRAGON:
    /* Dragonriders */
    if (GET_CLASS(ch) != CLASS_DRAGONRIDER)
      return FALSE;
    break;

  default:
    return FALSE;
  }

  /* In combat - high priority to call companion */
  if (FIGHTING(ch))
  {
    /* 50% chance when entering combat or outnumbered */
    if (!rand_number(0, 1))
      return TRUE;
  }
  else
  {
    /* Out of combat - lower chance */
    /* 10% chance to call when not in combat */
    if (!rand_number(0, 9))
      return TRUE;
  }

  return FALSE;
}
