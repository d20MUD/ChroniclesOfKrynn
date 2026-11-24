#ifndef KRYNN_RELIGION_H
#define KRYNN_RELIGION_H

#include "bool.h"

#define RELIGION_CHANGE_SECONDS (7 * 24 * 60 * 60)
#define RELIGION_HOSPITALITY_SECONDS (24 * 60 * 60)

struct char_data;
struct obj_data;
struct religion_requirement;
bool religion_parse_requirement(const char *text, struct religion_requirement *out);
bool religion_purchase_allows(struct char_data *buyer, int deity, int rank, bool explain);
bool religion_object_wear_allows(struct char_data *ch, struct obj_data *obj, bool explain);
bool religion_pet_purchase_allows(struct char_data *buyer, struct char_data *pet, bool explain);
bool religion_object_purchase_allows(struct char_data *buyer, struct obj_data *obj, bool explain);
bool religion_deity_valid(int deity);
int religion_rank(struct char_data *ch);
const char *religion_rank_name(int rank);
bool religion_mob_valid(const struct char_data *mob);
bool religion_guard_allows(struct char_data *ch, struct char_data *guard);
const char *religion_rank_title(int deity, int rank);
void religion_tasks_boot(void);
void religion_tasks_restore(struct char_data *ch);
void religion_tasks_status(struct char_data *ch);
void religion_tasks_command(struct char_data *ch, const char *argument);
void religion_tasks_admin(struct char_data *ch, int temple, int deity, int priest, const char *argument);
void religion_rank_admin(struct char_data *ch, const char *argument);
void religion_task_objective_complete(struct char_data *ch);
bool religion_service_context(struct char_data *ch, int requested, int *temple, int *deity, int *priest);
enum church_component { CHURCH_SAVE, CHURCH_AC, CHURCH_HP, CHURCH_POTENCY,
  CHURCH_DURATION, CHURCH_ATTACK, CHURCH_DR, CHURCH_MOVE, CHURCH_INIT };
int religion_blessing_value(struct char_data *, enum church_component);
int religion_blessing_typed(struct char_data *, int location, int bonus_type);
void religion_blessing_restore(struct char_data *);
void religion_blessing_tick(struct char_data *);
bool religion_blessing_remove(struct char_data *);
bool religion_blessing_command(struct char_data *, const char *, const char *);
void religion_boot(void);
bool religion_load_temples(const char *path);
bool religion_load_player_standing(struct char_data *ch, const char *line);
bool religion_load_player_cooldowns(struct char_data *ch, const char *line);
void religion_clear_confirmation(struct char_data *ch);
void do_religion(struct char_data *ch, const char *argument, int cmd, int subcmd);
void do_religionadmin(struct char_data *ch, const char *argument, int cmd, int subcmd);

#endif
