#ifndef KRYNN_CLAN_ENCHANT_H
#define KRYNN_CLAN_ENCHANT_H
struct obj_data;
struct char_data;
struct obj_special_ability;
struct clan_enchant_choice
{
  int effect, rank;
  const char *name, *words;
};
extern const struct clan_enchant_choice clan_enchant_choices[];
extern const size_t clan_enchant_count;
struct obj_special_ability *clan_enchant_ability(struct obj_data *);
void clan_enchant_activate(struct obj_data *, bool);
int clan_enchant_process(struct obj_data *, struct char_data *, struct char_data *, int,
                         const char *);
const struct clan_enchant_choice *clan_enchant_choice(int effect);
bool clan_enchant_wielded(struct char_data *, int effect);
#endif
