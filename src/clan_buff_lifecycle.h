#ifndef KRYNN_CLAN_BUFF_LIFECYCLE_H
#define KRYNN_CLAN_BUFF_LIFECYCLE_H
#include "clan_buff_activation.h"
enum clan_repository_result clan_buff_lifecycle_schema_check(MYSQL *);
struct clan_buff_cancel_request {char operation_uid[33],clan_uid[33],activation_uid[33];int64_t actor_id;};
/* Cancellation has the same release/current-member/management checks as
 * activation. No refund and no reset of any family timestamp. Exact replay is
 * allowed after departure/disablement; mismatched operation reuse conflicts. */
enum clan_repository_result clan_buff_cancel(MYSQL *,const struct clan_progression_config *,
    const struct clan_buff_cancel_request *,int64_t now,unsigned int *reasons,struct json_object **receipt);
/* One transaction: durable exactly-once expiration events plus a coherent
 * membership/core/timer snapshot. Offline time uses absolute saved timestamps.
 * This maintenance query never activates, extends, refunds or spends. */
enum clan_repository_result clan_buff_lifecycle_reconcile(MYSQL *,int64_t now,
    struct json_object **snapshot,struct clan_buff_state *rows,size_t capacity,size_t *count);
#ifdef CLAN_BUFF_LIFECYCLE_TEST
extern int clan_buff_lifecycle_fault,clan_buff_lifecycle_crash;
#endif
#endif
