/**************************************************************************
 *  File: mob_utils.h                                 Part of LuminariMUD *
 *  Usage: Header for mobile utility functions                            *
 *                                                                         *
 *  All rights reserved.  See license for complete information.           *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.              *
 **************************************************************************/

#ifndef _MOB_UTILS_H_
#define _MOB_UTILS_H_

/* Bought pets gain levels up to their owner's level, with a mortal cap. */
#define PURCHASED_PET_LEVEL_CAP 30
#define PET_TRAINING_RANK_CAP 10
#define PET_TRAINING_SKILL_CAP 5
#define PET_TRAINING_SKILLS 5
int companion_training_kind(struct char_data *pet);
void remember_eidolon_training(struct char_data *pet);
bool return_eidolon_gear(struct char_data *pet);
bool return_all_eidolon_gear(struct char_data *owner);
void restore_eidolon_training(struct char_data *owner, struct char_data *pet);
void restore_companion_kind(struct char_data *pet, int kind);
bool is_persistent_pet(struct char_data *pet);
bool save_char_pets_checked(struct char_data *ch);
void award_companion_training_exp(struct char_data *owner, struct char_data *victim, int reward);
void train_companion_skill(struct char_data *owner, struct char_data *pet, const char *skill);
void show_companion_training(struct char_data *owner, struct char_data *pet);
void discard_fallen_pet(struct char_data *ch, const char *id_arg);
int purchased_pet_level_cost(struct char_data *pet);
bool format_companion_tnl(struct char_data *owner, struct char_data *pet, char *buf, size_t size);
void purchased_pet_revival(struct char_data *ch, const char *id_arg);
bool archive_fallen_pet(struct char_data *pet);
void award_purchased_pet_exp(struct char_data *owner, struct char_data *victim, int reward);

/* Function prototypes for mob utility functions */

/* find a target for ch to attack */
struct char_data *npc_find_target(struct char_data *ch, int *num_targets);

/* attempt to switch opponents for ch */
bool npc_switch_opponents(struct char_data *ch, struct char_data *vict);

/* attempt to rescue someone */
bool npc_rescue(struct char_data *ch);

/* move ch along a pre-defined path */
bool move_on_path(struct char_data *ch);

/* handle mobile echo behavior */
void mobile_echos(struct char_data *ch);

/* check if ch can continue acting */
int can_continue(struct char_data *ch, bool fighting);

/* check if NPC should call a companion */
bool npc_should_call_companion(struct char_data *ch, int call_type);

#endif /* _MOB_UTILS_H_ */
