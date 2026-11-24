# Religion phase 2

Phase 2 adds native-quest service boards, durable Standing progression, weekly
completion records and deity-specific rank names. Phase 1's directional faith
and rank checks remain the access mechanism. Two optional authored pilot churches
are provided in [examples/religion_phase2](../examples/religion_phase2/README.txt).
The expanded privilege catalogue remains excluded. Shared restrings and cumulative
blessings remain phase 3.

## Player commands

```
religion task list
religion task accept <slot 1..4>
religion task confirm
religion task cancel
religion task status
religion task turnin
religion task abandon
```

Visit an awake, visible affiliated priest at its registered public entrance to
view its board, preview/confirm an assignment and turn in a completed task.
`religion task` also opens the board. Normal native objective credit marks the
assignment ready; return to the assigned temple and use `religion task turnin`.
Status and abandonment work away from the priest. An old-faith task cannot be
turned in while following a different deity; abandon it or return to that faith.

Acceptance previews the authored instructions, objective, return priest, level
and prerequisite requirements and reward. Confirm within two minutes. Membership,
priest presence, weekly offering, native eligibility, objective definition and
reward eligibility are rechecked. Crossing a weekly reset invalidates a preview.

There is one active church task per character. Native quest slot 3 is reserved for
it; ordinary quests retain their three slots (0, 1 and 2). Native quest progress,
objective checks and party credit are reused. Ordinary quest joining and sharing
cannot consume the reserved slot. Native `quest leave 3` uses durable church task
abandonment. Church objectives do not run ordinary quest rewards or chains.

## Standing and weekly rules

- Each eligible task gives 2 Standing, clamped to the 336 Standing ceiling.
- At most three rewarded completions per character per calendar week, shared
  across all faiths. A quest template can award Standing only once in that week.
- The boundary is Monday 00:00 UTC. `religion status` and task status show the next
  boundary and rewarded completion count. Unused allowances do not accumulate.
- Up to four distinct offerings are snapshotted per faith each week. Abandoning,
  reconnecting and changing faith do not reroll them. An empty board is also a
  stable snapshot: configure tasks before opening a new week's board.
- An active task survives the reset; its completion uses the completion week's
  cap. Offers accepted as story-only remain story-only after a reset. They never
  become banked Standing rewards.
- At the weekly cap, after that template's rewarded completion, or at Luminary,
  preview and board mark new assignments story-only. They grant no Standing or
  extra money, XP, quest points, items or followers.

Rank thresholds remain 0, 12, 36, 72, 120, 180, 252 and 336. Standing does not decay.
Ranks follow Standing automatically and never modify customizable character titles.

## Builder setup

Build a native repeatable quest in QEDIT. Supported objectives are room visits,
mob visits, mob kills, finding an object, returning an object to its designated
recipient, clearing a room and authored dialogue. Use the existing objective and
recipient validation; generic healing, crafting and donations do not award Standing.

A service template must have:

- The registered temple priest as questmaster.
- Repeatable enabled, no timer, positive objective quantity and valid level bounds.
- No previous/next quest or dialogue fallback chain.
- Zero gold, XP and quest-point rewards; no reward item, follower or race change.

Then configure up to four distinct quests per faith before players open that week's board:

```
religionadmin task <temple-id> <slot 1..4> <quest-vnum>
religionadmin task <temple-id>
religionadmin task <temple-id> <slot 1..4> 0
religionadmin rankname <deity-id> <rank 1..8> <rank name>
```

The first command assigns a slot, the second lists configuration and the third
clears a slot. Slots are shared by all temples of that faith; assigning a slot
through another temple changes its source for future boards. Players see the
offerings belonging to the priest at their current temple. Configuration changes
affect the next weekly snapshot, not an
already opened board. An objective changed during an assignment cannot earn
credit under the old objective fingerprint: restore the intended definition or
have the player abandon and take a fresh assignment. Avoid removing active priests.

Rank names save immediately. Without a custom name, the display uses the generic
rank followed by the deity's name. The optional pilot setup authors all eight rank
names for Mishakal and Gilean. Mechanics always use numeric ranks 1 through 8.

## Persistence and failure recovery

Install `sql/20261123_religion_phase2.sql` before deployment if the game account
cannot create tables. Otherwise `estate_init` creates missing tables. The master
schema includes the same definitions. Settings and reward records use InnoDB:

| Table | Purpose |
| --- | --- |
| `religion_rank_names` | Authored rank display names. |
| `religion_task_config` | Four configurable slots per faith, each with its source temple. |
| `religion_task_weeks`, `religion_task_offers` | Stable weekly board snapshots. |
| `religion_task_active` | One durable enrollment and ready/story-only state per player. |
| `religion_task_completions` | Enrollment receipts and per-week reward accounting. |
| `religion_standing` | Authoritative per-character, per-faith earned Standing. |

Existing player-file Standing seeds a deity's SQL row on its first assignment.
Once present, that SQL row is authoritative and synchronizes the native saved
Standing on login and task/status commands. An interrupted player-file save cannot
duplicate awards. The completion receipt, Standing increment and removal of the
active assignment commit together. A failed transaction leaves the ready task
available to retry; an ambiguous commit is resolved by status/reload.

Native quest progress persists in `Qcu3`, `Qcn3`, `Qti3`; existing tags for the
three ordinary slots remain unchanged. Missing slots recover from enrollment,
and a ready SQL record survives a stale player file. Recovery of a missing
in-progress slot restarts that objective rather than fabricating completion.
Task completion writes ready credit immediately instead of deferring it to a
volatile completion event. SQL failures preserve zero-counter evidence in the
checked player save for a later turn-in retry. If neither storage can save,
completion cannot be guaranteed until storage recovers.

## Validation

```
python3 tests/run_religion.py
python3 tests/run_religion_pilot.py
python3 tests/run_quest_sharing.py
python3 tests/run_msdp_quests.py
python3 tests/run_estate_integration.py
```

The religion suite exercises the production guard loop with multiple guards,
directions, clan combinations, faith/rank boundaries and bypass policies. The
pilot suite parses authored quests with the native parser, exercises native
objective dispatch and fourth-slot pfile tags, validates protected-room topology
and executes the native teleport destination check. The estate suite uses a
disposable MariaDB database with both adapter and production object codecs to
verify transactions, replay protection, weekly boundaries, faith changes,
story-only rollover, failed player saves, enrollment recovery and the rank cap.
It never accesses the game's database. Live pilot installation and playtesting
are deployment steps described in the pilot README.
