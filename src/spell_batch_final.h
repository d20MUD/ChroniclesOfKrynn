#ifndef SPELL_BATCH_FINAL_H
#define SPELL_BATCH_FINAL_H
#include "interpreter.h"
#define FERVOR_DEFENSE 0
#define FERVOR_SPEED 1
#define FERVOR_ATTACK 2
#define FERVOR_RISE 3
#define BATTLESENSE_ATTACK 0
#define BATTLESENSE_ARMOR 1
#define BATTLESENSE_DAMAGE 2
#define BATTLESENSE_SPEED 3
void final_batch_affect(int level, struct char_data *ch, struct char_data *victim, int spell, int metamagic);
void final_batch_round(void);
int final_batch_bonds_dc(struct char_data *ch);
int final_batch_bonus(struct char_data *ch, int location);
bool final_batch_fervor(struct char_data *ch, int mode);
int final_batch_blade_bonus(struct char_data *ch);
bool final_batch_other_dr(struct char_data *ch);
int final_batch_natural_hit(struct char_data *ch, struct char_data *victim, int attack_type);
int final_batch_thorns(struct char_data *attacker, struct char_data *victim, int attack_type);
int final_batch_outgoing(struct char_data *ch, int damage, int spell);
bool final_batch_cancel_critical(struct char_data *ch);
bool final_batch_struggle(struct char_data *ch);
void do_spellchoice(struct char_data *ch, const char *argument, int cmd, int subcmd);
#endif
