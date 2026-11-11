#ifndef KRYNN_CLAN_COMBAT_H
#define KRYNN_CLAN_COMBAT_H
#include "clan_progression.h"
#define CLAN_COMBAT_MAX_ACTORS 128
#define CLAN_COMBAT_MAX_ACTIONS 1024
/* Trusted occurrence snapshot, not browser/command input. owner=-1 is a player;
 * controlled actors resolve to an included player, never a separate account. */
struct clan_combat_actor {
  int owner,level;
  int64_t player_id,account_id,room,non_clan_max_hp;
  char clan_uid[33];
  bool accepted,alive,playing,participated;
};
enum clan_combat_action_kind {CLAN_COMBAT_DAMAGE,CLAN_COMBAT_HEAL,CLAN_COMBAT_MITIGATE,
  CLAN_COMBAT_SUPPORT,CLAN_COMBAT_DEFENSE,CLAN_COMBAT_HOSTILE_HIT};
struct clan_combat_action {
  int actor,ally; /* ally is an engaged participant; -1 when acting on the victim */
  enum clan_combat_action_kind kind;
  uint64_t application; /* stable attack/effect application identity, not a packet */
  int64_t at,round,room,amount,available;
  bool useful,ally_engaged;
};
struct clan_combat_encounter {
  int victim_level;
  int64_t victim_prototype;
  char encounter_uid[33];
  int64_t room,started_at,occurred_at,start_max_hp;
  bool normal_death,world_npc,owned,charmed,summoned,pet,test,staff_test,manipulated;
  const struct clan_combat_actor *actors;size_t actor_count;
  const struct clan_combat_action *actions;size_t action_count;
};
enum clan_combat_reason {CLAN_COMBAT_BAD_VICTIM=1,CLAN_COMBAT_TRIVIAL=2,
  CLAN_COMBAT_IDENTITY=4,CLAN_COMBAT_INACTIVE=8,CLAN_COMBAT_LEVEL=16,
  CLAN_COMBAT_NO_PARTICIPATION=32,CLAN_COMBAT_ACCOUNT_DUPLICATE=64};
struct clan_combat_result {
  int64_t player_id,account_id;char clan_uid[33];
  int highest_level;unsigned int reasons;
};
/* Pure, bounded, deterministic query. No SQL, live effects, character XP,
 * rewards, prototype counters or killing-blow privilege. Results are indexed
 * like actors; controlled entries have IDENTITY and cannot claim separately.
 * Invalid/corrupt facts leave all outputs unchanged. Phase 26 supplies live
 * source collection, occurrence membership snapshots and durable reward hooks. */
bool clan_combat_query(const struct clan_progression_config *,const struct clan_combat_encounter *,
    struct clan_combat_result *out,size_t capacity);
#endif
