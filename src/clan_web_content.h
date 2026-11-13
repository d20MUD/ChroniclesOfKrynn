#ifndef KRYNN_CLAN_WEB_CONTENT_H
#define KRYNN_CLAN_WEB_CONTENT_H
#include "clan_web.h"
/* Compose within the existing consistent read transaction. No reset, acceptance,
 * quote creation, claim, inventory mutation or receipt creation. */
struct json_object *clan_web_content_read(MYSQL *, const struct clan_progression_config *,
                                          const char *clan, int64_t player, int64_t account,
                                          int64_t now);
/* Add only server-owned live facts after releasing SQL. Never client selectors. */
void clan_web_content_enrich(struct char_data *, struct json_object *, int64_t now);
#endif
