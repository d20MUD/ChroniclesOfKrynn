#ifndef BLOODLINES_H
#define BLOODLINES_H

#define NUM_EXTENDED_BLOODLINES 6
#define BLOODLINE_WINDOW_SECONDS 300
#define BLOODLINE_ROUND_SECONDS 6
#define BLOODLINE_ROLL_ANY 0
#define BLOODLINE_ROLL_ATTACK 1
#define BLOODLINE_ROLL_SAVE 2
#define BLOODLINE_ROLL_SKILL 3
#define BLOODLINE_ELEMENT_AIR 1
#define BLOODLINE_ELEMENT_EARTH 2
#define BLOODLINE_ELEMENT_FIRE 3
#define BLOODLINE_ELEMENT_WATER 4
struct char_data;
struct sorcerer_bloodline
{
  int feat, type, skill;
  const char *name;
  int powers[6];
  int spells[9];
  int bonus_feats[8];
};
extern const struct sorcerer_bloodline extended_bloodlines[NUM_EXTENDED_BLOODLINES];
const struct sorcerer_bloodline *extended_bloodline_by_feat(int feat);
const struct sorcerer_bloodline *extended_bloodline_by_type(int type);
void grant_extended_bloodline_powers(struct char_data *ch);
int bloodline_affect_bonus(struct char_data *ch, int spell);
int bloodline_fated_bonus(struct char_data *ch);
int bloodline_reroll(struct char_data *ch, int roll);
int bloodline_retry_spell_resistance(struct char_data *ch, int challenge, int resistance, int first_die);
void bloodline_cast_arcana(struct char_data *ch, int spellnum, struct char_data *target);
int bloodline_spell_dc_bonus(struct char_data *ch, int spellnum);
int bloodline_shadow_spell(int spellnum);
int extended_bloodline_spell_circle(struct char_data *ch, int spellnum);
bool bloodline_sees_weather(struct char_data *ch);
bool bloodline_sees_magical_dark(struct char_data *ch);
int bloodline_modify_damage(struct char_data *victim, int dam, int w_type, int dam_type);
int bloodline_element(struct char_data *ch);
int bloodline_element_damage(struct char_data *ch);
const char *bloodline_element_name(int element);
bool bloodline_elemental_movement(struct char_data *ch, int element);
int bloodline_convert_energy(struct char_data *ch, int spellnum, int element);
int bloodline_energy_resistance(struct char_data *ch, int element);
bool bloodline_energy_immunity(struct char_data *ch, int element);
int bloodline_poison_save_bonus(struct char_data *ch);
int bloodline_strength_bonus(struct char_data *ch);
int bloodline_conviction_roll(struct char_data *ch, int roll, int category);
int bloodline_cooldown_seconds(int feat);
int bloodline_knockdown_bonus(struct char_data *ch);
int bloodline_reduce_concealment(struct char_data *attacker, int concealment);
bool bloodline_summon_creature_spell(int spellnum);
bool bloodline_summoning_spell(int spellnum);
void bloodline_summon_benefits(struct char_data *ch, struct char_data *mob, int spellnum);
void bloodline_restore_summon_ward(struct char_data *mob);
#endif
