#ifndef EPIC_FEATS3_H
#define EPIC_FEATS3_H
#define EPIC_THIRD(ch) ((ch)->player_specials->saved.epic_third)
enum epic_third_use
{
  E3_CONTINGENCY,
  E3_SENTINEL,
  E3_INTERCESSION,
  E3_ASPECT,
  E3_OPPORTUNITY,
  E3_STOWAWAY,
  E3_DODGE,
  E3_RECOVERY,
  E3_ANATOMY,
  E3_RESERVED,
  E3_UNUSED
};
enum epic_third_value
{
  E3_POWER,
  E3_POWER_ML,
  E3_INTER_TARGET,
  E3_INTER_END,
  E3_ASPECT_OLD,
  E3_ASPECT_NEW,
  E3_ASPECT_END,
  E3_OP_TARGET,
  E3_OP_SPELL,
  E3_OP_CLASS,
  E3_OP_END,
  E3_STOW_TARGET,
  E3_STOW_SPELL,
  E3_STOW_END
};
#define E3V(ch, n) EPIC_THIRD(ch).value[n]
#define E3_PERFORMANCE 1
#define E3_TAIL 2
bool epic_third_feat(int feat);
int epic_third_class(int feat);
bool epic_third_prerequisites(struct char_data *, int, bool);
bool epic_third_command(struct char_data *, const char *, const char *);
void epic_third_update(void);
void epic_third_respec(struct char_data *);
void epic_third_provenance(struct char_data *, struct char_data *, struct affected_type *, int, int,
                           int);
int epic_third_dispel_cl(struct char_data *, struct char_data *);
void epic_third_failed_save(struct char_data *, struct char_data *, int, int, int);
int epic_third_sentinel(struct char_data *, int, int, int, int, int);
void epic_third_after_damage(struct char_data *, struct char_data *, int, unsigned int);
int epic_third_intercept(struct char_data *, struct char_data *, int, unsigned int);
void epic_third_cast_done(struct char_data *, struct char_data *, int, int, int, bool);
void epic_third_performance(struct char_data *, struct affected_type *, int);
void epic_third_stop_music(struct char_data *);
void epic_third_stop_song(struct char_data *, int);
int epic_third_evolution(struct char_data *, int, int);
bool epic_third_dodge(struct char_data *, struct char_data *, int, int);
enum epic_precision_immunity
{
  EPIC_PRECISION_NONE,
  EPIC_PRECISION_ORDINARY,
  EPIC_PRECISION_HARD
};
int epic_third_precision_immunity(struct char_data *vict);
bool epic_third_anatomy(struct char_data *, struct char_data *, struct obj_data *);
bool epic_third_weapon_choice(struct char_data *, int, bool);
#endif
