#ifndef EPIC_FEATS_H
#define EPIC_FEATS_H

#define EPIC_CHOICES(ch) ((ch)->char_specials.saved.epic_choices)
#define EPIC_TIMERS(ch) ((ch)->player_specials->saved.epic_timers)

int epic_discipline_dc(struct char_data *ch, int spell);
int epic_augmentation_discount(struct char_data *ch, int spell, int augmentation);
bool is_expansion_epic(int feat);
int epic_feat_class(int feat);
bool epic_feat_available(struct char_data *ch, int feat);
int epic_study_rank(struct char_data *ch, int feat);
void epic_snapshot_ward(struct char_data *ch);
void epic_restore_ward(struct char_data *ch, struct char_data *pet);
int epic_feat_rank(struct char_data *ch, int feat);
int epic_capacity_circle(int class);
bool epic_capacity_choice(struct char_data *ch, int class);
bool epic_power_choice(struct char_data *ch, int power);
int epic_typed_increment(struct char_data *ch, int location, int specific, int type, int amount);
int epic_artisan_roll(struct char_data *ch, int roll, int modifier, int dc, bool criticals);
int epic_craft_seconds(struct char_data *ch, int base, int other_seconds, int category,
                       bool efficient);
int epic_trigger_cost(struct char_data *ch, int charges, bool commit);
void epic_device_begin(struct char_data *ch);
void epic_device_end(struct char_data *ch);
int epic_device_penetration(struct char_data *ch);
int epic_mind_save_bonus(struct char_data *ch, int save, int spell);
int epic_planar_save_bonus(struct char_data *ch, int save, int spell);
int epic_chassis_bonus(struct char_data *ch, int maneuver);
int epic_eidolon_resistance(struct char_data *ch, int damtype);
int epic_rage_penetration(struct char_data *ch, int attack_type);
ACMD_DECL(do_epic);

#endif
