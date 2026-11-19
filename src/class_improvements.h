#ifndef CLASS_IMPROVEMENTS_H
#define CLASS_IMPROVEMENTS_H
#include "interpreter.h"
#define CI_RALLY 0
#define CI_OPENING 1
#define CI_PARRY 2
#define CI_ADVANCE 3
#define CI_INTERCEPT 4
#define CI_COOLDOWNS 5
#define CI_ROUND_SECONDS (PULSE_VIOLENCE / PASSES_PER_SEC)
#define CI_STATE(ch) ((ch)->char_specials.class_improvements)
#define CI_READY(ch, n) ((ch)->player_specials->saved.class_improvement_ready[n])
bool class_feature_ready(struct char_data *ch);
bool class_duelist_gear(struct char_data *ch);
bool class_reaction_active(void);
bool class_reaction_allow_hit(void);
int class_rhythm_bonus(struct char_data *ch, int location);
int class_rally_save(struct char_data *ch, int type);
int class_guard_ac(struct char_data *ch);
int class_opening_begin(struct char_data *ch, struct char_data *vict, int type, int attack_type);
int class_opening_accuracy(struct char_data *ch, struct char_data *vict, int insight);
int class_opening_damage(struct char_data *ch, struct char_data *vict);
int class_seam_damage(struct char_data *ch, struct char_data *vict, int attack_type);
bool class_measured_parry(struct char_data *ch, struct char_data *vict, int roll, bool critical,
                          int attack_type);
struct char_data *class_intercept(struct char_data *ch, struct char_data *vict);
int class_recovery_absorb(struct char_data *vict, int dam, int w_type, int attack_type,
                          unsigned int origin);
void class_recovery_start(struct char_data *ch);
bool class_revelation(struct char_data *ch, struct char_data *vict);
void class_features_depart(struct char_data *ch);
ACMD_DECL(do_rally);
ACMD_DECL(do_opening);
ACMD_DECL(do_intercept);
ACMD_DECL(do_classfeatures);
#endif
