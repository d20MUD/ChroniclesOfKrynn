#ifndef KRYNN_AUCTION_HOUSE_H
#define KRYNN_AUCTION_HOUSE_H
#include "interpreter.h"
void auction_house_command(struct char_data *ch, const char *argument);
void auction_house_collect(struct char_data *ch);
void estate_recover(struct char_data *ch);
SPECIAL_DECL(mysql_auction_house);
ACMD_DECL(do_auctionhouse);
#endif
