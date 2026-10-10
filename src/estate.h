#ifndef KRYNN_ESTATE_H
#define KRYNN_ESTATE_H
#include "mysql.h"
int estate_init(void);
int estate_player_ready(struct char_data *);
unsigned long long estate_prepare_escrow(struct char_data *, struct obj_data *);
int estate_commit_escrow(unsigned long long);
int estate_queue_delivery(struct char_data *, struct obj_data *, long);
int estate_receipt_saved(struct char_data *);
void estate_recover(struct char_data *);
int estate_query(const char *format, ...) __attribute__((format(printf, 1, 2)));
MYSQL_RES *estate_select(const char *format, ...) __attribute__((format(printf, 1, 2)));
char *estate_quote(const char *text);
char *estate_pack(struct obj_data *obj);
struct obj_data *estate_unpack(const char *text);
void estate_save_player(struct char_data *ch);
#endif
