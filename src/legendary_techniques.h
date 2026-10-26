#ifndef KRYNN_LEGENDARY_TECHNIQUES_H
#define KRYNN_LEGENDARY_TECHNIQUES_H
#include "legendary_effects.h"
struct obj_data;
struct affected_type;
struct legendary_attack {
  bool measured, opening, hunted, rallying, grounding;
  struct legendary_activation strike, mark, hunt, rally, ground;
};
void legendary_techniques_init(void);
void legendary_signatures_init(void);
int legendary_signature_dodge(struct char_data *,bool consume);
int legendary_signature_save(struct char_data *,int type,int spell);
bool legendary_grounded(struct char_data *);
bool legendary_activation_live(const struct legendary_activation *);
int legendary_technique_attack(struct char_data *,struct char_data *,struct obj_data *,struct legendary_attack *);
int legendary_technique_damage(struct char_data *,struct char_data *,int,int,struct legendary_attack *);
int legendary_technique_maneuver(struct char_data *,int,bool defense);
int legendary_technique_skill(struct char_data *,int,bool consume);
int legendary_technique_walk(struct char_data *,int,bool);
void legendary_track(struct char_data *);
bool legendary_exit_traversable(struct char_data *,int);
bool legendary_technique_affordable(struct char_data *,int cost);
int legendary_technique_pay(struct char_data *,int spell,int cost);
void legendary_cast_failed(struct char_data *,int spell);
void legendary_cast_clear(struct char_data *);
void legendary_cast_effect(struct char_data *,int,int);
bool legendary_cast_queue(struct char_data *,int,int);
bool legendary_cast_commit(struct char_data *,int);
void legendary_affect_round(void);
void legendary_affect_schedule(int remaining_pulses);
void legendary_condition_origin(struct affected_type *,struct char_data *);
void legendary_condition_expire(struct char_data *);
extern int legendary_script_depth;
#define LEG_CONDITION_LOCKED 1
#endif
