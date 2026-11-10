#ifndef KRYNN_CLAN_PERSISTENCE_H
#define KRYNN_CLAN_PERSISTENCE_H
#include "clan_progression.h"
struct char_data;
/* Boot-only selection: NULL/legacy (default), shadow or mysql. Never imports
 * implicitly. MYSQL requires explicit offline cutover; failure stops startup. */
bool clan_persistence_boot(const char *selection);
bool clan_persistence_boot_environment(void);
bool clan_persistence_boot_buff(size_t index, struct clan_buff_state *out);
/* Internal boot latch: pin before any storage access, never fall back. */
void clan_persistence_pin_mysql(void);
/* Attach already reconciled runtime metadata to a checked authoritative store. */
bool
clan_persistence_configure(enum clan_store_mode mode);
bool
clan_persistence_sql_authority(void);
bool
clan_persistence_ready(void);
bool
clan_persistence_save(void);
bool
clan_persistence_before_player_save(struct char_data* ch);
void
clan_persistence_reconcile_player(struct char_data* ch);
bool
clan_persistence_member_update(int64_t player_id, int vnum, int rank);
bool
clan_persistence_access(struct char_data* ch);
bool
clan_persistence_economy_available(struct char_data* ch);
#endif
