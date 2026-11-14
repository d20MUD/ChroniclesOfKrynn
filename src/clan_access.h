#ifndef KRYNN_CLAN_ACCESS_H
#define KRYNN_CLAN_ACCESS_H
struct char_data;
struct clan_requirement;
/* Zero requirements are unrestricted. Rank 1 is the highest officer rank. */
bool clan_access_check(struct char_data *, const struct clan_requirement *, bool explain);
#endif
