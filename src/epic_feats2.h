#ifndef EPIC_FEATS2_H
#define EPIC_FEATS2_H
#define EPIC_SECOND(ch) ((ch)->player_specials->saved.epic_second)
enum epic_second_use
{
  E2_REBUTTAL,
  E2_MANTLE,
  E2_REPAIR,
  E2_AEGIS,
  E2_INFUSION,
  E2_BOND,
  E2_SHARING,
  E2_COMMAND,
  E2_TRIAGE,
  E2_BULWARK,
  E2_SMITE,
  E2_COMPANION,
  E2_BOMB,
  E2_UNUSED
};
struct epic_second_attack
{
  int command_damage, smite_damage;
};
bool epic_second_feat(int feat);
int epic_second_class(int feat);
bool epic_second_prerequisites(struct char_data *ch, int feat, bool study);
bool epic_second_command(struct char_data *ch, const char *verb, const char *argument);
int epic_rebuttal_roll(struct char_data *ch, int resistance, int roll);
int epic_temporary_remaining(struct char_data *ch);
void epic_temporary_clear(struct char_data *ch);
void epic_second_departure(struct char_data *ch);
int epic_temporary_absorb(struct char_data *ch, int damage);
bool epic_resilient_bond(struct char_data *pet);
void epic_second_update(void);
void epic_second_smite(struct char_data *ch, int smite);
int epic_second_attack_begin(struct char_data *ch, int type, int attack_type,
                             struct epic_second_attack *attack);
void epic_second_attack_hit(struct char_data *ch, struct char_data *victim, struct obj_data *weapon,
                            int attack_type, struct epic_second_attack *attack);
void epic_infusion_unwield(struct char_data *ch, struct obj_data *obj);
void epic_summon_created(struct char_data *ch, struct char_data *pet, int spell, int seconds);
bool epic_summon_doctrine(struct char_data *pet, int doctrine);
int epic_bomb_consume(struct char_data *ch, int slot);
void epic_bomb_changed(struct char_data *ch, int slot);
bool epic_shared_valid(struct char_data *ch);
void epic_shared_complete(struct char_data *ch, int spell, int level, int metamagic);
int cast_epic_shared_spell(struct char_data *ch, int spell);
#endif
