#ifndef KRYNN_PLAYER_SHOP_H
#define KRYNN_PLAYER_SHOP_H
int player_shop_init(void);
void player_shop_command(struct char_data *ch, const char *argument);
void player_shop_show_room(struct char_data *ch);
void do_stall(struct char_data *ch, const char *argument, int cmd, int subcmd);
#endif
