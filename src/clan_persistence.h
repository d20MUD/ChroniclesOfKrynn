#ifndef KRYNN_CLAN_PERSISTENCE_H
#define KRYNN_CLAN_PERSISTENCE_H
#include "clan_progression.h"
struct char_data;
/* Phase 4c writer integration, NOT a boot loader/cutover switch. Default LEGACY.
 * MYSQL attachment requires already reconciled runtime clan data, an imported
 * store explicitly marked MYSQL, and the checked schema. No startup caller yet. */
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
