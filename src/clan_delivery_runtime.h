#ifndef KRYNN_CLAN_DELIVERY_RUNTIME_H
#define KRYNN_CLAN_DELIVERY_RUNTIME_H
struct char_data;
/* First-login scan also finds SQL holds whose final file acknowledgement was
 * interrupted. Pending economic locks block command mutations until recovery. */
bool clan_delivery_recover(struct char_data *);
#endif
