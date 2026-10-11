#ifndef EPIC_MAGIC_H
#define EPIC_MAGIC_H

bool epic_spell_uses_preparation_pool(int spellnum);
bool epic_ward_surge_active(struct char_data *ch);
int epic_preparation_casts_max(struct char_data *ch);
void normalize_epic_preparation_casts(struct char_data *ch);
void regenerate_epic_preparation_cast(struct char_data *ch);
int epic_spell_cast_cost(int spellnum, int metamagic);
bool epic_spell_preflight(struct char_data *ch, struct char_data *victim, int spellnum,
                          int metamagic, bool display);
void spend_epic_spell_casts(struct char_data *ch, int spellnum, int metamagic);
void epic_spell_damage_effects(struct char_data *ch, struct char_data *victim,
                               int spellnum, int level, int damage_dealt);
bool epic_summon_combat_turn(struct char_data *mob);

#endif
