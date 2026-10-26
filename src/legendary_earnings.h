#ifndef KRYNN_LEGENDARY_EARNINGS_H
#define KRYNN_LEGENDARY_EARNINGS_H
#include "legendary.h"
struct char_data;
struct trap_data;
/* Context-free gain_exp remains excluded. These are server-generated sources. */
enum legendary_xp_origin { LEG_XP_NONE, LEG_XP_KILL, LEG_XP_QUEST, LEG_XP_TRAP };
struct legendary_xp_context {
  enum legendary_xp_origin origin;
  char award_id[33];
  int source_vnum;
  bool eligible;
};
struct legendary_balance {
  struct legendary_wallet wallet;
  int64_t version, utc_day;
  int fragments_today, essences_today, catalysts_today;
};
struct legendary_exchange;
bool legendary_earnings_available(void);
bool legendary_balance_load(struct char_data *, struct legendary_balance *);
void legendary_xp_kill_context(struct legendary_xp_context *, struct char_data *victim);
void legendary_xp_quest_context(struct legendary_xp_context *, int quest_vnum);
void legendary_xp_trap_context(struct legendary_xp_context *, struct trap_data *);
void legendary_credit_xp(struct char_data *, int actual_gain, const struct legendary_xp_context *);
/* Resolve durable accepted awards before commands. Does not change normal XP. */
bool legendary_earnings_recover(struct char_data *);
enum legendary_action { LEG_ACTION_DAMAGE, LEG_ACTION_HEAL, LEG_ACTION_INTERCEPT,
                        LEG_ACTION_DEBUFF, LEG_ACTION_DEFENSE };
void legendary_record_action(struct char_data *actor, struct char_data *boss,
    enum legendary_action, struct char_data *ally, int actual_effect);
void legendary_record_support(struct char_data *, struct char_data *ally,
    enum legendary_action, int actual_effect);
void legendary_boss_death(struct char_data *boss, struct char_data *killer);
void legendary_activity_expire(void);
void legendary_earnings_maintenance(void);
/* First proposal only. The command/smith validation is implemented in Phase 6. */
struct legendary_exchange *legendary_exchange_quote(struct char_data *, int from_essence,
    int to_essence, int quantity); /* to=-1: catalyst; from ignored for catalyst */
bool legendary_exchange_confirm(struct char_data *, struct legendary_exchange *);
void legendary_exchange_discard(struct legendary_exchange *);
const struct legendary_wallet *legendary_exchange_delta(const struct legendary_exchange *);
const char *legendary_exchange_id(const struct legendary_exchange *);
#ifdef LEGENDARY_EARNINGS_TEST
extern int64_t legendary_earnings_test_time;
#endif
#endif
