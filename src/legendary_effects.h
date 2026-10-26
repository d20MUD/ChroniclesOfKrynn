#ifndef KRYNN_LEGENDARY_EFFECTS_H
#define KRYNN_LEGENDARY_EFFECTS_H
#include "legendary.h"
struct char_data;
/* These origins are packet-local, never mutable character-wide flags. */
enum legendary_origin { LEG_ORIGIN_NORMAL=0, LEG_ORIGIN_BONUS=1,
  LEG_ORIGIN_REDIRECT=2, LEG_ORIGIN_NO_TRIGGER=4 };
enum legendary_budget { LEG_BUDGET_OFFENSE=0, LEG_BUDGET_PREVENTION=1,
  LEG_BUDGET_HEALING=2, LEG_BUDGET_RESOURCE=3 };
enum legendary_target { LEG_TARGET_SELF=0, LEG_TARGET_ALLY=1, LEG_TARGET_ENEMY=2 };
enum legendary_effect_kind { LEG_EFFECT_PREPARED=0, LEG_EFFECT_REDUCTION=1,
  LEG_EFFECT_REDIRECT=2, LEG_EFFECT_HEAL_TICKS=3, LEG_EFFECT_TEMP_HP=4 };
enum legendary_effect_flags { LEG_EFFECT_PHYSICAL=1, LEG_EFFECT_BREATH=2,
  LEG_EFFECT_ROOM_BOUND=4, LEG_EFFECT_SOURCE_STATIONARY=8, LEG_EFFECT_GROUP=16, LEG_EFFECT_MOUNT=32 };
struct legendary_activation {
  struct char_data *source, *target;
  char uuid[33], receipt[33];
  int channel, power, rank;
  int64_t source_version;
  struct legendary_resolved_power resolved;
};
/* Catalog adapters validate before cooldown commit. apply must not retain this
 * stack pointer, extract characters, or perform economic transactions. A resisted
 * valid use still spends recovery. Catalog handlers register once before live activation. */
typedef bool (*legendary_power_check)(const struct legendary_activation *);
typedef void (*legendary_power_apply)(const struct legendary_activation *);
bool legendary_activation_register(int power, int target_kind,
    legendary_power_check check, legendary_power_apply apply);
bool legendary_activate(struct char_data *, int power, struct char_data *target);
void legendary_use(struct char_data *, const char *power, const char *target);
bool legendary_target_legal(struct char_data *, struct char_data *, int kind);
/* All payments commit before gameplay mutation. Failure returns zero, never
 * an uncapped fallback. Windows are (now-window, now], including future rows
 * after wall-clock rollback. NPC recipients use bounded in-memory ledgers. */
bool legendary_budget_prime(struct char_data *);
bool legendary_budget_flush(void);
void legendary_budget_unload(struct char_data *);
int legendary_budget_available(struct char_data *,int kind); /* Read-only PC quote; payment revalidates. */
int legendary_budget_debit(struct char_data *, int kind, int requested);
int legendary_heal(struct char_data *source, struct char_data *target, int amount);
int legendary_temporary_hp(const struct legendary_activation *,int amount,int duration);
int legendary_temporary_absorb(struct char_data *,int damage);
int legendary_temporary_remaining(struct char_data *);
int legendary_resource_restore(struct char_data *, int amount);
int legendary_resource_pay(struct char_data *, int ordinary_cost, int discount);
/* Spec is copied. Timers live only in memory and are never replayed at login.
 * Wards with charges=0 last until expiry or pool exhaustion; pool=0 means no
 * cumulative pool. One-shot charges are consumed only on an actual change.
 * Identical key+kind on a recipient selects strongest, then latest expiry. */
struct legendary_effect_spec {
  int key, kind, flags, magnitude, percent, maximum, charges, pool;
  int duration, interval, ticks;
};
bool legendary_effect_prepare(const struct legendary_activation *,
    const struct legendary_effect_spec *);
int64_t legendary_effects_now(void);
bool legendary_effect_peek(struct char_data *,int,struct legendary_activation *,struct legendary_effect_spec *);
bool legendary_effect_from(struct char_data *,int,struct legendary_activation *);
void legendary_effect_clear(struct char_data *,int);
void legendary_effect_clear_source(struct char_data *,int);
int legendary_effect_value(struct char_data *target, int key);
bool legendary_effect_consume(struct char_data *target, int key);
int legendary_effects_damage(struct char_data *source, struct char_data *target,
    int damage, int dam_type, bool breath, unsigned int origin);
void legendary_effects_pulse(void);
void legendary_effects_depart(struct char_data *);
void legendary_effects_cancel(struct char_data *); /* death/logout; retains budgets */
void legendary_effects_free(struct char_data *); /* extraction; forget NPC ledger */
#ifdef LEGENDARY_EFFECTS_TEST
extern int64_t legendary_effects_test_time;
#endif
#endif
