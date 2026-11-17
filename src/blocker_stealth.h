/* Optional stealth passage through directional NPC blockers. */
#ifndef BLOCKER_STEALTH_H
#define BLOCKER_STEALTH_H

#define BLOCKER_STEALTH_SECONDS 300

void prune_blocker_stealth_checks(struct char_data *ch, time_t now);
void remember_blocker_stealth_check(struct char_data *ch, mob_vnum guard_vnum,
                                  time_t expires, bool passed);
bool blocker_stealth_passes(struct char_data *ch, struct char_data *guard);

#endif
