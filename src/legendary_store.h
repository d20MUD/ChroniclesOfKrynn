#ifndef KRYNN_LEGENDARY_STORE_H
#define KRYNN_LEGENDARY_STORE_H
#include "legendary.h"
#define LEGENDARY_SCHEMA_VERSION 8
#define LEGENDARY_UUID_SIZE 33

enum legendary_record_state { LEG_RECORD_PENDING=0, LEG_RECORD_ACTIVE=1, LEG_RECORD_DELETED=2 };
enum legendary_load_result { LEG_LOAD_ERROR=-1, LEG_LOAD_MISSING=0, LEG_LOAD_OK=1 };
/* Release controls are deliberately independent.  A disabled control is a
 * fail-closed gameplay gate, not a schema or data deletion mechanism. */
enum legendary_feature { LEG_FEATURE_EARNINGS=0, LEG_FEATURE_PURCHASES=1,
  LEG_FEATURE_MAJOR_EFFECTS=2, LEG_FEATURE_COUNT=3 };
struct legendary_channel_record {
  bool present;
  int item_passive; /* primary only; supporting inherits the same selection */
  int rank, path, family, foundation, subtype, technique, signature, modifier, modifier_target;
  unsigned int first_choices;
  struct legendary_cost paid;
  int64_t version;
};
struct legendary_item_record {
  char uuid[LEGENDARY_UUID_SIZE];
  int64_t owner, version;
  int prototype, slot, state;
  struct legendary_channel_record channels[2];
};
struct legendary_registry_record { int vnum, tier, essence; bool enabled, hard_control_immune; int64_t version; };
/* No schema creation, implicit commits, or economic mutations. Caller owns the
 * SQL transaction and expected versions. Missing schema is an error, not empty data. */
bool legendary_schema_ready(void);
bool legendary_feature_enabled(enum legendary_feature feature);
bool legendary_features_refresh(void);
unsigned long long legendary_dragonlance_registry_epoch(void);
int legendary_store_load(const char *uuid, struct legendary_item_record *out);
bool legendary_store_insert(const struct legendary_item_record *record);
bool legendary_store_save_channel(const struct legendary_item_record *item, int channel, int64_t expected_version);
bool legendary_store_set_state(const char *uuid, int64_t owner, int64_t expected_version, int state);
bool legendary_store_tombstone(const char *uuid, int64_t owner, int64_t expected_version);
bool legendary_retire_owner(long player_id,const char *player_name);
bool legendary_registry_refresh(void);
bool legendary_registry_cached(int,bool,struct legendary_registry_record *);
int legendary_registry_load(int vnum, bool dragonlance, struct legendary_registry_record *out);
bool legendary_registry_save(const struct legendary_registry_record *record, bool dragonlance,
                             int64_t expected_version); /* -1 inserts, >=0 updates */
bool legendary_record_valid(const struct legendary_item_record *item);
#endif
