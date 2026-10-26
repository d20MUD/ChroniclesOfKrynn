#ifndef KRYNN_LEGENDARY_TRANSACTION_H
#define KRYNN_LEGENDARY_TRANSACTION_H
#include "legendary_store.h"
struct char_data;
struct obj_data;
struct legendary_quote;
/* Server-only plans. Never construct a plan from player-supplied prices or
 * balances. Phase 5/6 award and purchase handlers supply their validated rules.
 * All amounts are exact, signed ledger changes; gold supports deductions only. */
enum legendary_operation_kind { LEG_OP_PURCHASE, LEG_OP_RESHAPE, LEG_OP_EXCHANGE, LEG_OP_GRANT };
struct legendary_transaction_plan {
  enum legendary_operation_kind kind;
  struct legendary_wallet delta;
  const struct legendary_item_record *item; /* desired record, or NULL */
  int channel; /* 0/1 when item supplied */
  /* For first awakening, item.uuid is empty and object is a carried/equipped
   * ordinary item. Quoting does not bind it. Confirmation creates its identity;
   * cancellation restores its previous binding and persists that restoration. */
  struct obj_data *object;
  /* Optional server participant: validate eligibility and atomically record
   * provenance/claims/counters inside this transaction. Return false to roll
   * back everything. Do not commit, save objects/files, or change wallet/version.
   * Context must remain alive until confirm/discard. Never called on replay. */
  bool (*participate)(struct char_data *, const char *operation_id, void *context);
  void *context;
};
/* Quote lifetime 60 seconds; opaque and single-use. No player command yet. */
struct legendary_quote *legendary_transaction_quote(struct char_data *,
    const struct legendary_transaction_plan *);
void legendary_transaction_discard(struct legendary_quote *);
const char *legendary_transaction_id(const struct legendary_quote *);
bool legendary_transaction_confirm(struct char_data *, struct legendary_quote *);
/* Resolves prepared (cancel, no charges), committed (receipt, checkpoint,
 * activate), and applied (clear lock) operations. True only when fully ready. */
bool legendary_transaction_recover(struct char_data *);
bool legendary_transaction_locked(const struct char_data *);
#ifdef LEGENDARY_TRANSACTION_TEST
/* Stop immediately at numbered boundaries, simulating lost acknowledgments. */
extern int legendary_transaction_fault;
extern int legendary_transaction_crash;
extern int legendary_transaction_clock_offset;
#endif
#endif
