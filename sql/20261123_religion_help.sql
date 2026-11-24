-- Religion phases 1 and 2 help; safe to reapply. No game-state schema changes.
START TRANSACTION;
INSERT INTO help_entries(tag,category,entry,min_level,max_level,auto_generated)
VALUES ('RELIGION','general',REPLACE('RELIGION

RELIGION

  religion [status]
  religion list
  religion info <deity>
  religion devote <deity>
  religion leave
  religion confirm
  religion cancel
  religion hospitality
  religion task [list|status]
  religion task accept <slot 1..4>
  religion task confirm
  religion task cancel
  religion task turnin
  religion task abandon
  religion say <message>
  religion channel on|off
  religion who

Your existing deity is your active church affiliation. Existing worshippers
start as Devotees with zero Church Standing. Church rank is separate from
your class and your customizable title. Standing is retained for each faith.

Devotion and departure show a preview and require religion confirm within
two minutes. Leaving makes you wait seven real days before rejoining or
choosing another faith. Your first devotion has no cooldown. A divine class
or selected domains may require staff assistance for a supported patron
change. Selecting a church does not grant domains or class abilities.

If a faith has an available registered temple, devote at its priest in the
public entrance. Religion info lists available temples. Faiths without
completed temples still offer identity and the channel; their services are
marked unavailable before devotion. Devote choose now uses the same preview
and confirmation flow.

An affiliated priest with hospitality enabled gives one random food item
and one random drink once per 24 real hours. Each has a random existing
food or drink buff, applied when you eat or drink it under the normal
consumable rules. Hospitality does not change hunger or thirst. Make room
in your inventory for both items before claiming them. The shared character
cooldown survives logout and faith changes. Rewards and cooldown are saved
together; interrupted deliveries can be recovered without another claim.

Temple service boards offer up to four authored tasks per week. Preview an
assignment at an affiliated priest, then confirm within two minutes. You may
have one church task in addition to your three ordinary quests. Complete its
native quest objective, then return to its temple priest and use religion
task turnin. Task status and abandonment are available away from the priest.

Eligible tasks give 2 Standing: at most three rewards per calendar week,
shared across all faiths. A template can reward you only once in that week.
Reset is Monday 00:00 UTC; status shows the next reset. Active tasks survive
resets and use the completion week''s allowance. Story-only assignments never
become rewarded later. At the weekly cap or Luminary, tasks remain story-only.
Standing stops at 336 and never decays. Church rank names do not change your
customizable title. Abandoning does not reroll the weekly board.

The religion channel reaches participating online members of your faith.
Normal communication restrictions and visibility rules apply. Religion who
lists visible participants only. Staff monitor under the normal clan-channel
monitoring policy. There is no religion message history or public faith
roster. Your channel preference is saved across logins.

Tasks, earned Standing awards, deity-specific rank names, blessings and rank
appearances are scheduled for later implementation phases.

See also: DEITY, DEVOTE, RELIGIONADMIN, BLOCK-RELIGION
',CHAR(10),CONCAT(CHAR(13),CHAR(10))),0,65,0),
('RELIGIONADMIN','general',REPLACE('RELIGION BUILDING

MEDIT has deity and templerank choices. Rank 0 means no temple rank;
1..8 run from Devotee through Luminary, with 8 highest. These settings
are independent of player Standing and clan rank. Mob prototypes retain
them through save, reload, copies and new spawns.

Pets with both deity and temple rank set require buyers of the same faith
and at least that rank, even without blocker flags. OEDIT has the same deity
and templerank settings for object purchases. Both values must be nonzero.
NPC shops, player stalls and auctions check requirements before payment;
pet-token objects also check the resulting mob. Clan requirements still
apply. Religion requirements also govern wear, wield and hold. Object
requirements survive saves and resale; carrying and giving remain possible.

Block-Religion plus a directional block flag admits only that mob''s faith.
Block-Temple-Rank additionally requires the mob''s rank or higher in the
same faith; it also implies the faith requirement when used alone.
Block-Clan, when present, must also pass. Ghost, stealth and ordinary
class/race/alignment exemptions do not bypass these requirements. The
normal staff bypass remains available. A malformed requirement denies
passage and logs a boot warning. These flags guard directions, not portals,
summons or every scripted way to enter a room. Always build an outward exit.

  religionadmin list
  religionadmin show <id>
  religionadmin create <id> <deity-id> <name>
  religionadmin set <id> name <name>
  religionadmin set <id> deity <deity-id>
  religionadmin set <id> entrance <public-room-vnum>
  religionadmin set <id> priest <mob-vnum>
  religionadmin set <id> hospitality <0|1>
  religionadmin set <id> enabled <0|1>
  religionadmin delete <id> confirm
  religionadmin reload
  religionadmin task <temple-id> [<slot 1..4> <quest-vnum; 0 clears>]
  religionadmin rankname <deity-id> <rank 1..8> <rank name>
  religionadmin chest <deity-id> <temple-rank 1..8> <item-limit 1..10000>

Builder access is required. New temples are disabled drafts. Enabled temples
need a valid deity and public entrance, plus a unique priest prototype of
that faith with temple rank 1..8. The priest must be awake, visible and outside
combat at the public entrance to provide services. Spawn/reset a priest after
changing its prototype. One priest prototype belongs to one temple. Several
temples can belong to one deity.

CHURCH TASKS AND RANK NAMES

Use QEDIT to author repeatable, untimed standalone quests with positive
objective quantity and the temple''s priest as questmaster. Room/mob visits,
kills, object recovery/return, room clearing and authored dialogue are
supported. No chains or dialogue fallback; no gold, XP, quest points,
reward items, followers or race changes. Use valid native level bounds and
prerequisites. Normal objective and recipient validation remains in force.

Assign up to four distinct templates per faith with religionadmin task. With only a
temple ID it lists configuration; quest vnum 0 clears a slot. Configure all
slots are shared across all temples of the same faith. Set them before players
open the board: weekly offerings are snapshotted and
configuration changes apply to the next week''s board. Changing an active
objective invalidates its old credit; restore the definition or abandon it.

Rewards are fixed at 2 Standing, at most three completions per week across
all churches, and no template can reward twice in that week. Monday 00:00
UTC is the boundary. Story-only acceptance never banks a later reward.
Standing caps at 336. Quest slot 3 is reserved for church tasks; native
quest slots 0..2 retain their ordinary use. Standing and completion receipts
commit together in SQL and synchronize character files on login/status.

Use religionadmin rankname to author a deity''s rank display label. Numeric
rank and access checks are unchanged; customizable character titles are
never overwritten. Unauthored labels use the generic rank and deity name.

The optional zone 699 pilot package and setup commands are under
examples/religion_phase2. Install and playtest it on a test server before
connecting its public courtyard to the live world.

TEMPLE CHESTS

Stand in a permanent public room you may edit and use religionadmin chest
with a deity ID, minimum temple rank and item limit. This creates or updates
one immovable chest in that room. It does not require a priest or temple
registry entry. Use the same command to change its settings; capacity cannot
be reduced below the number of stored items. Player houses are excluded.

Players use look in chest, examine chest, put <item> chest and get <item>
chest, including get all chest and keyword/number selection. Only players
of the same faith and at least the configured rank can view or transfer its
contents; staff have no faith bypass. Nested contents and sheathed weapons
count toward capacity. Normal storage exclusions (money, no-rent and no-drop
items, including nested contents) apply. Legendary ownership checks
remain in force.

Chest settings and contents are stored in SQL and survive reboots. The
chest appears automatically after boot without a zone object reset. Storage
must be available for transfers; failed deposits leave the item with its
owner, and failed withdrawals leave it in storage or the delivery journal.

Edits to the temple registry are saved atomically to etc/religion.json. Missing files start an empty
registry. Malformed files disable services and edits until corrected and
reloaded. Unknown or removed world references make the affected temple
unavailable. Disable an existing temple before changing its configuration.
No temple, mob, room, Standing or rank is created by registering a temple.

See also: RELIGION, MEDIT, BLOCK-STEALTH
',CHAR(10),CONCAT(CHAR(13),CHAR(10))),31,65,0)
ON DUPLICATE KEY UPDATE entry=VALUES(entry), min_level=VALUES(min_level), max_level=VALUES(max_level);
INSERT IGNORE INTO help_keywords(help_tag, keyword) VALUES
('RELIGION','RELIGION'), ('RELIGION','CHURCH'), ('RELIGION','HOSPITALITY'),
('RELIGIONADMIN','RELIGIONADMIN'), ('RELIGIONADMIN','BLOCK-RELIGION'),
('RELIGIONADMIN','BLOCK-TEMPLE-RANK');
COMMIT;
