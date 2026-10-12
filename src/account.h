/*/ \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \
\
/  Luminari Account System, Inspired by D20mud's Account System
/  Created By: zusuk
\
/
\         todo: move header stuff into account.h
/         Created on January 24, 2018, 3:37 PM
\ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ /*/

#ifndef ACCOUNT_H
#define ACCOUNT_H

struct char_data;

#ifdef __cplusplus
extern "C"
{
#endif

/*******************************************************/
/* defines */
#define ACCOUNT_REWARD_BONUS_CAP 10
#define ACCOUNT_REWARD_BONUS_COST 2000
enum account_reward_bonus
{
  ACCOUNT_BONUS_EXPERIENCE,
  ACCOUNT_BONUS_TREASURE,
  ACCOUNT_BONUS_GOLD,
  ACCOUNT_BONUS_QUESTPOINTS
};
/*******************************************************/
/* external functions */
void perform_do_account(struct char_data *ch, struct char_data *vict);
int change_account_xp(struct char_data *ch, int change_val);
int get_account_reward_bonus(struct char_data *ch, int bonus);
int apply_account_reward_bonus(struct char_data *ch, int amount, int bonus);
/*******************************************************/

#ifdef __cplusplus
}
#endif

#endif /* ACCOUNT_H */
