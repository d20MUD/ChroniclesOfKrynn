# QEDIT: quest editor and builder reference

Source reviewed: 2026-10-23. This guide covers QEDIT's main menu, all **25 quest
types**, all **three quest flags**, rewards, prerequisites, dialogue branches,
and saving in the current Krynn codebase.

QEDIT edits automatic quest definitions identified by quest vnums. It does not
edit the separate mobile-attached legacy quest system in `hlquest.c`/`hlqedit.c`,
or a player's active quest record. NPC prototypes belong to
[MEDIT](MEDIT_GUIDE.md), objects to [OEDIT](OEDIT_GUIDE.md), and rooms to
[REDIT](REDIT_GUIDE.md).

## Contents

- [Opening, copying and saving](#opening-copying-and-saving)
- [Main menu](#main-menu)
- [Text and defaults](#text-and-defaults)
- [Complete quest type reference](#complete-quest-type-reference)
- [Complete quest flag reference](#complete-quest-flag-reference)
- [Prerequisites, chains and timers](#prerequisites-chains-and-timers)
- [Rewards](#rewards)
- [Dialogue quests](#dialogue-quests)
- [Building and testing a quest](#building-and-testing-a-quest)
- [Current limitations and common mistakes](#current-limitations-and-common-mistakes)
- [Source references](#source-references)

## Opening, copying and saving

| Command | Purpose |
| --- | --- |
| `qedit <quest-vnum>` | Edit an existing quest, or create one at an unused numeric vnum in a zone you may edit. |
| `qedit save <vnum-in-zone>` | Save the quests belonging to the zone containing this vnum. |
| `qedit save` | Resolve the configured OLC zone to its bottom vnum and save quests in that zone. |
| `qcopy <source-vnum> <destination-vnum>` | Copy an existing quest into an unused destination vnum; requires staff-level command access and permission for the destination zone. |
| `qlist` or `qlist .` | List quests in your current room's zone. |
| `qlist <zone-number>` | List quests in a specific zone. |
| `qlist <lowest-vnum> <highest-vnum>` | List quests in a vnum range. |
| `vnum q <text>` | Search quest titles for text, ignoring case. |

**QEDIT's explicit save argument is a vnum within the zone, not the zone number.**
Unlike several other editor commands, QEDIT resolves it through
`real_zone_by_thing()`. For example, after editing quest `13816`, use
`qedit save 13816` to save its zone's quests. Substitute vnums from your own
assigned zone in all examples.

Bare `qedit` asks for a vnum. `qedit new` and `qedit create` are not supported.
Builder command access and zone editing permission are required; another
builder cannot edit the same quest simultaneously.

Choose `Q` to leave the main menu, then `Y` to save changed work or `N` to
cancel it. A newly opened blank quest starts with its changed flag unset, so
make a change before quitting if you intend to create it. Saving commits the
quest to memory. If automatic OLC saving is enabled, it also writes to disk;
otherwise run the explicit save command afterward. Read the actual save
message rather than assuming a memory save is persistent.

`qcopy` copies the definition into memory immediately; it does not open the
menu or automatically perform QEDIT's disk-save step. Open the copied quest,
correct its references, and save its destination zone.

Quest definitions are stored in `lib/world/qst/<zone-number>.qst`, with an index
in that directory. The configured library root determines the full path; for
the live installation, the directory is `~/live/mud/lib/world/qst`. Use the
editor for normal building rather than editing live files by hand.

`X) Delete Quest` acts after its own confirmation, independently of the normal
quit/save prompt. It marks the zone for saving and writes it immediately when
automatic OLC saving is enabled. Deleting a quest does not safely redesign the
other quests or player histories that refer to it; review those references first.

## Main menu

| Choice | Setting | Purpose and accepted input |
| --- | --- | --- |
| `1` | Quest Name | Title used in quest listings and searches; single-line text. |
| `2` | Description | Brief description; single-line text. |
| `3` | Accept Message | Multiline instructions displayed when the quest begins. |
| `4` | Completion Message | Multiline text displayed on normal completion. |
| `5` | Quit Message | Multiline text displayed when the player abandons the quest. |
| `6` | Quest Flags | Toggle repeatability, replacement reward, and sharing flags. |
| `7` | Quest Type | Choose the objective; return-object and give-gold types then ask for the recipient mobile vnum. |
| `8` | Quest Master | Mobile vnum offering the quest, or `-1` for none. An entered mobile must exist. |
| `9` | Quest Target | Meaning depends on type: object, room or mobile vnum; comma-separated mobile vnums; gold amount; or mission difficulty. |
| `H` | X-Coord (wild) | Wilderness objective's X coordinate; stored as an integer. |
| `I` | Y-Coord | Wilderness objective's Y coordinate; stored as an integer. |
| `A` | Quantity | Required objective count, clamped to **1–50**. |
| `B` | Completed | Quest points awarded, **0–999999**. |
| `C` | Abandoned | Quest points deducted on voluntary abandonment, **0–999999**. |
| `G` | Gold Coins | Gold reward, **0–99999**. |
| `T` | Exp Points | Experience reward, **0–999999**. |
| `O` | Object | Reward object vnum, or `-1` for none; an entered object must exist. |
| `R` | Race | Lich or vampire race number from the prompt, or `-1` for none. Other races are rejected. |
| `J` | Follower | Reward mobile vnum, or `-1` for none; an entered mobile must exist. |
| `D` | Lower Level | Minimum accepting level, **0–34**, no higher than Upper Level. |
| `E` | Upper Level | Maximum accepting level, **0–34**, no lower than Lower Level. |
| `F` | Prerequisite | Object vnum required to accept the quest, or `-1` for none. |
| `L` | Time Limit | **-1–100** game ticks; `-1` disables the timer. Use positive values for timed quests. |
| `N` | Next Quest | Existing quest vnum assigned automatically on completion, or `-1` for none. |
| `P` | Previous Quest | Existing quest vnum whose completion is required to accept this quest, or `-1` for none. |
| `S` | Quest Dialogue Options | Social skill DCs and fallback quest. |
| `X` | Delete Quest | Delete after confirmation. |
| `Q` | Quit | Leave, with a save prompt when changed. |

## Text and defaults

Names and descriptions use buffers of 120 characters, leaving up to 119 text
characters. Acceptance, completion and quit messages use the multiline editor
with a 4096-byte maximum buffer. Follow its displayed editing instructions.
Write the actual task, destination, recipient and available dialogue methods in
the acceptance message; QEDIT does not derive good instructions from the target.

A new quest has an undefined type, no questmaster, no target or linked quests,
no item prerequisite, quantity 1, level limits 0–34, no time limit, and zero
point, gold and experience rewards. Dialogue DCs start at `-1`. Wilderness
coordinates start at `(0, 0)`. Placeholder text is supplied for every message.
Set a real type and valid references before offering the quest to players.

## Complete quest type reference

**Menu choices are one greater than the stored type ID.** For example, choose
`4` for Kill Mob, whose internal ID is `AQ_MOB_KILL = 3`. The undefined new-quest
type (`-1`) is not a selectable objective.

| Menu | Internal constant / ID | Displayed type | Target and completion behavior |
| --- | --- | --- | --- |
| 1 | `AQ_OBJ_FIND` / 0 | Acquire Object | Target is an object vnum. Matching acquisition events advance the objective. |
| 2 | `AQ_ROOM_FIND` / 1 | Find Room | Target is a room vnum. Reaching that room advances it. |
| 3 | `AQ_MOB_FIND` / 2 | Find Mob | Target is a mobile vnum. A room check finds that prototype among the room's NPCs. |
| 4 | `AQ_MOB_KILL` / 3 | Kill Mob | Target is a mobile vnum. Matching kills advance it. |
| 5 | `AQ_MOB_SAVE` / 4 | Save Mob | Target is the mobile to preserve. The combat/death check verifies that other remaining, uncharmed NPCs do not have a different vnum. Test the encounter carefully. |
| 6 | `AQ_OBJ_RETURN` / 5 | Return Object | Target is an object vnum; the additional recipient is a mobile vnum. Giving that object to that recipient advances it. |
| 7 | `AQ_ROOM_CLEAR` / 6 | Clear Room | Target is a room vnum. The check requires no remaining uncharmed NPCs that are not marked for death. |
| 8 | `AQ_AUTOCRAFT` / 7 | Complete a Supplyorder | Legacy supply-order completion event; target is not compared by the quest checker. |
| 9 | `AQ_CRAFT` / 8 | Craft Item | Legacy craft completion event; target is not compared. |
| 10 | `AQ_CRAFT_RESIZE` / 9 | ReSize Item | Legacy resize event; target is not compared. |
| 11 | `AQ_CRAFT_DIVIDE` / 10 | Divide Item | Legacy divide event; target is not compared. |
| 12 | `AQ_CRAFT_MINE` / 11 | Mine Crafting Mat | Legacy mining event; target is not compared. |
| 13 | `AQ_CRAFT_HUNT` / 12 | Hunt for Crafting Mat | Legacy crafting-material hunt event; target is not compared. |
| 14 | `AQ_CRAFT_KNIT` / 13 | Knit Crafting Mat | Legacy knitting event; target is not compared. |
| 15 | `AQ_CRAFT_FOREST` / 14 | Forest for Crafting Mat | Legacy forestry event; target is not compared. |
| 16 | `AQ_CRAFT_DISENCHANT` / 15 | Disenchant Item | Legacy disenchant event; target is not compared. |
| 17 | `AQ_CRAFT_AUGMENT` / 16 | Augment Item | Legacy augment event; target is not compared. |
| 18 | `AQ_CRAFT_CONVERT` / 17 | Convert Item | Legacy convert event; target is not compared. |
| 19 | `AQ_CRAFT_RESTRING` / 18 | ReString Item | Legacy restring event; target is not compared. |
| 20 | `AQ_COMPLETE_MISSION` / 19 | Complete a Mission | Target is minimum mission difficulty: **0 easy, 1 normal, 2 tough, 3 challenging, 4 arduous, 5 severe**. Equal or harder missions qualify. Invalid target input is reset to easy. |
| 21 | `AQ_HOUSE_FIND` / 20 | Find a Player House | Target is unused. The check requires entering a private house owned by the player. |
| 22 | `AQ_WILD_FIND` / 21 | Get to Wilderness Coordinates | Use `H` and `I`. Requires a wilderness-flagged zone and exact matching X/Y coordinates. |
| 23 | `AQ_GIVE_GOLD` / 22 | Give Gold | Target is the gold threshold; the additional recipient is a mobile vnum. Current code checks the recipient's total gold after giving, then clears that gold. |
| 24 | `AQ_MOB_MULTI_KILL` / 23 | Kill Multi Mob (comma-separated vnums) | Enter mobile vnums such as `13801,13802,13803`. A kill of any listed prototype advances the shared quantity counter. |
| 25 | `AQ_DIALOGUE` / 24 | Dialogue Quest | Target is the NPC to speak to; configure enabled skill DCs and the fallback through `S`. |

Quantity is a single remaining-goals counter. Multi-kill does **not** track a
separate quota for each listed mobile: quantity 5 means five qualifying kills
in total, not five of every entry. Use quantity 1 for normal dialogue, room,
coordinate, house and other one-step objectives.

The old crafting types have hooks in `craft.c`. Their presence in QEDIT does
not establish that the current crafting commands emit those events. Verify an
actual completion through the current player workflow before using these types
in a new quest line; choosing a target does not restrict them to that item.

## Complete quest flag reference

Enter the displayed number to toggle a flag; enter `0` to return to the main
menu. These are menu numbers, not bit values.

| Menu | Flag | Purpose |
| --- | --- | --- |
| 1 | `AQ_REPEATABLE` — REPEATABLE | Allows accepting a completed quest again through normal eligibility checks. Earlier completion remains in history. |
| 2 | `AQ_REPLACE_OBJ_REWARD` — REPLACE-OBJ-REWARD | Makes a completed quest's object reward eligible for the dedicated reward-replacement special procedure. It does not itself install that procedure on an NPC. |
| 3 | `AQ_NO_SHARE` — NO-SHARE | Rejects attempts to share the quest with party members through the quest-sharing system. |

REPLACE-OBJ-REWARD is not a lost-item detector. The replacement procedure checks
completion, this flag and the reward vnum; it does not verify that the player
has actually lost their earlier copy. Account for that when selecting rewards.
NO-SHARE concerns offering the quest to another player; it is not a general
switch disabling all group objective credit.

## Prerequisites, chains and timers

`F) Prerequisite` requires possession of an object with the specified prototype
vnum, either directly in inventory or equipped. It is checked on acceptance and
is not consumed by that check. It is distinct from `P) Previous Quest`, which
requires a completed quest in the player's history.

`N) Next Quest` and `P) Previous Quest` are independent fields. Setting one does
not create the reverse link. For a normal A → B chain, set A's Next to B and
B's Previous to A. Create and save both quest definitions to memory before
linking them: the editor rejects references to quests that do not exist yet.

Normal completion automatically inserts a valid, different, not-yet-completed
Next Quest into the freed active slot and displays its acceptance message.
This direct transition does not go through the normal acceptance eligibility
checks. Do not rely on the successor's level or object prerequisite to prevent
an automatic transition; design the chain accordingly.

Time limits count game-hour ticks while the player is present in the character
list. They are decremented by the main tick handler, not once per combat round
or real-world minute. Expiration clears the active quest and reports a timeout;
it does not run the voluntary abandonment penalty path. Use `-1` for unlimited
or **1–100** for a timer. Although the editor accepts zero, the decrement-before-
comparison implementation does not expire it as an immediate timeout.

## Rewards

Completion can award quest points, gold, experience, an object, a race change,
and a follower. Set only rewards appropriate to the quest and test their actual
completion behavior: reward processing includes additional game rules and can
modify the final award. Menu limits are input limits rather than guaranteed
final payouts.

Race rewards are limited to the lich and vampire choices displayed by QEDIT;
completion performs race-specific checks. They are transformations with gameplay
requirements, not a general-purpose race selection field.

Follower rewards load the selected NPC prototype and attach it as a follower.
The completion handler can also join it to a group led by the player. This does
not create suitable combat AI or companion behavior for an arbitrary mobile;
configure and test that prototype separately with MEDIT.

`C) Abandoned` is a **positive amount to subtract**, not an award for quitting.

## Dialogue quests

Set type **25) Dialogue Quest**, target the conversation NPC, and use quantity
**1**. Setting DCs alone does not change the quest's type, despite the dialogue
submenu's explanatory text. The command handler explicitly searches active
`AQ_DIALOGUE` quests and a visible matching NPC in the same room.

| Dialogue menu | Setting | Meaning |
| --- | --- | --- |
| `1` | Diplomacy DC | Enables `convince`, using the Diplomacy/Persuasion skill. |
| `2` | Intimidate DC | Enables `threaten`, using Intimidate. |
| `3` | Bluff DC | Enables `beguile`, using Bluff/Deception. |
| `4` | Quest VNum on Dialogue Fail | Existing fallback quest vnum, or `-1` for none. |
| `Q` | Exit to Main Quest Menu | Return and mark the definition changed. |

DC input is clamped to **-1–100**. A method is enabled only when its DC is
**greater than zero**; `-1` or `0` disables it. The check succeeds when d20 plus
the computed skill meets or exceeds the chosen DC. Players use `convince`,
`threaten`, or `beguile`; the handler finds the quest NPC itself.

With a valid fallback quest configured:

- **Success:** the dialogue quest completes normally and grants its rewards.
  The fallback is also recorded as completed, **without fallback rewards**.
  If the fallback has a Next Quest, that successor takes precedence over the
  dialogue quest's own Next Quest.
- **Failure:** the original dialogue quest is recorded as completed and failed,
  without its normal completion rewards. It is replaced in the active slot by
  the fallback quest, whose acceptance message is shown.
- **Fallback completion:** grants the fallback's own rewards and follows its
  Next Quest as normal.

Without a valid fallback, a failed check leaves the dialogue quest active and
explicitly permits another attempt. The submenu's statement that only one
attempt is possible therefore depends on providing a valid fallback.

### A shared continuation after success or failure

For example, create three quests at unused vnums in your assigned zone:

| Quest | Type | Previous | Next | Dialogue fallback |
| --- | --- | --- | --- | --- |
| A: convince the guard | Dialogue | Earlier story quest, or none | C, or none | B |
| B: obtain the guard's papers | Return Object, for example | A | C | None |
| C: enter the next story stage | Appropriate objective | B | Later quest, or none | None |

Succeeding at A marks B completed and advances to C via B's Next field.
Failing A assigns B; completing B then advances to C. C can simply require B's
completion. Give A and B their own appropriate reward amounts, since successful
dialogue skips B's reward.

## Building and testing a quest

1. Prepare the quest giver, target NPCs, objects and rooms in your assigned zone.
2. Create each quest at a free vnum. Fill in its title, description and messages.
3. Set type, target and quantity; provide the recipient when selecting Return
   Object or Give Gold. Re-selecting the type lets you change that recipient.
4. Set accepting level limits, prerequisites, flags, timer and rewards.
5. Save the definitions to memory, then add Previous, Next and dialogue links.
6. Save the affected zones to disk with `qedit save <vnum-in-zone>`.
7. At the quest giver, use `quest list`, `quest list <number>` and
   `quest join <number>`. Check `quest progress` and `quest history` throughout.
8. Test as an ordinary character: wrong targets, exact quantity, hand-in
   recipient, reward, successor, abandonment, timer, and persistence after a
   normal reconnect. For dialogue, test each enabled method and both branches.
9. If sharing is allowed, test `quest share`/acceptance and objective credit
   with a second eligible grouped character.

Quest giver list numbers differ from active indexes. `quest progress` shows
active indexes starting at **0**; commands operating on an active quest must
use those indexes rather than its quest-giver list number or vnum.

## Current limitations and common mistakes

- Target input usually accepts integers without validating that the target
  prototype or room exists. Confirm all targets yourself. Questmaster, reward,
  prerequisite and quest-link fields perform stronger existence checks.
- Questmasters are assigned the questmaster special procedure by quest loading
  and saving code. Do not overwrite unrelated NPC procedures casually; inspect
  interactions when combining a quest giver with a shop or other special role.
- Multi-kill's menu displays names for only the first five entries. That display
  is not a separate per-mobile progress tracker. Always enter a nonempty, valid
  comma-separated list before using the type.
- Changing the type does not reset all old targets, coordinates or quantities.
  Review those fields after every type change.
- Give Gold currently checks the recipient's accumulated gold, rather than
  comparing only the amount in the player's latest gift. It then clears the
  NPC's gold. Test with the NPC's normal gold state.
- Some player instructions incorrectly decide whether to show `beguile` by
  checking the Diplomacy DC rather than the Bluff DC. The actual command uses
  the Bluff DC; document enabled methods in your acceptance message.
- The quest-abandonment code has a dialogue-parent restriction that uses a
  Previous Quest value as a table index rather than resolving its vnum. Do not
  rely on that restriction behaving correctly for all quest chains.
- Staff acceptance can bypass some normal level/completion restrictions. A staff
  test alone does not establish that a mortal character can take the quest.

## Source references

- [qedit.c](../../src/qedit.c): command syntax, menus, defaults, input limits and save prompts.
- [quest.h](../../src/quest.h): all quest types, flags and stored fields.
- [quest.c](../../src/quest.c): eligibility, objectives, rewards, chains, timers and sharing.
- [act.comm.c](../../src/act.comm.c): dialogue commands and success/failure handling.
- [genqst.c](../../src/genqst.c): copying, questmaster assignment, deletion and `.qst` saving.
- [oasis_copy.c](../../src/oasis_copy.c) and [oasis_list.c](../../src/oasis_list.c): copying and listing commands.
- [craft.c](../../src/craft.c): legacy crafting quest hooks.
- [spec_procs.c](../../src/spec_procs.c): quest reward replacement procedure.
- [comm.c](../../src/comm.c): quest timer tick scheduling.
