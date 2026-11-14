#ifndef KRYNN_CLAN_QM_SERVICES_H
#define KRYNN_CLAN_QM_SERVICES_H
struct char_data;
/* -1 unavailable/pending, 0 no new-service receipt, 1 recovered. */
int clan_qm_services_recover(struct char_data *);
void clan_qm_services_command(struct char_data *, const char *);
#endif
