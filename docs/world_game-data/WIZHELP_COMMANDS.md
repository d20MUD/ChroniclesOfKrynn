# Wizhelp command reference

Brief descriptions of all **182 commands** in the current build's default
`wizhelp` list, grouped by their configured minimum level (source reviewed
2026-10-23). Runtime `cmdlev` changes can alter the list; some commands also
apply additional permission checks. Aliases and placeholders are included.

## Level 34

| Command | Brief explanation |
| --- | --- |
| `abilityset` | Set a character's ability/skill values. |
| `analyzeworld` | Generate a wilderness/world knowledge report in WILD_KB.md. |
| `cedit` | Edit global game configuration. |
| `clanset` | Change administrative clan settings. |
| `database` | Inspect, verify or initialize database systems and reference data. |
| `db_init_system` | Initialize database tables for a selected game subsystem. |
| `discord` | Inspect and manage the Discord bridge. |
| `export` | Export a zone's world data. |
| `featset` | Set a character's feats or feat ranks. |
| `genmap` | Generate a named wilderness feature through the current river/path generator. |
| `helpgen` | Generate, repair, import or export help entries. |
| `oconvert` | Convert matching weapon prototypes to a selected weapon subtype. |
| `perfmon` | Inspect game performance measurements. |
| `reloadimm` | Reload supported staff/game data files without a full restart. |
| `relock` | Remove an account's unlocked races, classes or both. |
| `resetpassword` | Reset an account password. |
| `saveeverything` | Mark every zone's supported world-data categories for saving. |
| `saveobjstodb` | Export object data to the database. |
| `setsail` | Placeholder for setting sail; currently unimplemented. |
| `shipload` | Placeholder for ship loading; currently unimplemented. |
| `shutdow` | Incomplete-shutdown safeguard; tells you to use the full command. |
| `shutdown` | Shut down or restart the server using the selected mode. |
| `skillset` | Set a character's spell/skill proficiency. |
| `talento` | Inspect or set a player's talent ranks and talent points. |
| `wizlock` | Restrict game login access by level. |

## Level 33

| Command | Brief explanation |
| --- | --- |
| `addmaterials` | Add to or subtract from a character's crafting material balance. |
| `advance` | Change a character's level. |
| `ai` | Inspect and manage the NPC conversational AI service. |
| `ban` | Manage site/login bans. |
| `effectsadmin` | Inspect and assign region effects. |
| `freeze` | Freeze a player to restrict their commands. |
| `hcontrol` | Manage legacy player-house ownership and access settings. |
| `hsedit` | Create or edit legacy house records. |
| `materialadmin` | Add or remove materials in a player's material storage. |
| `pubsubtopic` | Manage publish/subscribe messaging topics. |
| `regenadmin` | Inspect resource-regeneration history and logging. |
| `reroll` | Reroll a character's base attributes. |
| `resourceadmin` | Inspect and manage wilderness resource data and caches. |
| `restore` | Restore a character's resources and supported recovery state. |
| `setmaterials` | Set a character's stored quantity of a crafting material. |
| `settestchar` | Configure a test character's race, classes and levels. |
| `settestkit` | Give a character a generated test equipment kit. |
| `setworldsect` | Mark the current room as worldmap terrain and set its sector. |
| `terrainapi` | Inspect and manage the terrain API integration. |
| `thaw` | Remove a player's frozen restriction. |
| `unban` | Remove a site/login ban. |
| `wizupdate` | Regenerate the staff/immortal lists. |

## Level 32

| Command | Brief explanation |
| --- | --- |
| `aedit` | Create or edit social commands and their messages. |
| `award` | Grant supported staff awards or rewards to a player. |
| `dc` | Disconnect a specified network descriptor. |
| `deletepath` | Delete a saved wilderness path by vnum. |
| `finddoor` | Locate doors associated with a key object or vnum. |
| `findmaterial` | Locate rooms containing a specified harvest material. |
| `force` | Make a character execute a command. |
| `gecho` | Send an unattributed message throughout the game. |
| `genriver` | Generate a named wilderness river/path and reload path data. |
| `iedit` | Edit an individual live object rather than its prototype. |
| `mcopy` | Copy one mobile prototype to another vnum. |
| `msdplist` | List MSDP protocol variables. |
| `msdpvalue` | Inspect the value of an MSDP variable. |
| `msgedit` | Edit combat messages. |
| `mute` | Toggle a player's no-shout communication restriction. |
| `notitle` | Toggle a player's permission to change their title. |
| `obind` | Bind an object to a character. |
| `ocopy` | Copy one object prototype to another vnum. |
| `pardon` | Remove a player's thief and killer flags. |
| `players` | List connected players with account, location, race and class details. |
| `plist` | List player records with supported filters. |
| `qcopy` | Copy one quest definition to another vnum. |
| `qecho` | Send a message through the quest communication channel. |
| `questcomplete` | Set or clear a player's completion flags across a quest-line range. |
| `quitlog` | Read recorded player quit information. |
| `rcopy` | Copy one room definition to another vnum. |
| `scopy` | Copy one shop definition to another vnum. |
| `send` | Send a direct staff message to a character. |
| `set` | Change supported administrative character fields. |
| `snoop` | Monitor another connection's input/output. |
| `switch` | Take control of an NPC body. |
| `tcopy` | Copy one DG trigger definition to another vnum. |
| `tedit` | Edit selected global text files such as news and MOTD. |
| `unaffect` | Remove a character's spell affects. |
| `unbind` | Remove an object's character binding. |
| `users` | List active network connections and descriptors. |
| `zlock` | Lock zones against building and editing. |
| `zunlock` | Unlock zones for building and editing. |

## Level 31

| Command | Brief explanation |
| --- | --- |
| `;` | Alias for wiznet. |
| `aqref` | Find automatic-quest references to an object vnum. |
| `astat` | Display a social command's configuration. |
| `at` | Execute a command as though you were in another room. |
| `attach` | Attach a DG trigger to a mobile, object or room. |
| `bedit` | Create or edit a MySQL message board. |
| `blist` | List configured message boards. |
| `boardfind` | Locate message-board objects, optionally filtering by name. |
| `buildwalk` | Toggle room creation while walking into unbuilt exits. |
| `changelog` | Read the game's change log. |
| `checkapproved` | List mobile-attached quests that still need approval. |
| `checkload` | Find where a mobile, object or trigger vnum is loaded or referenced. |
| `cmdlev` | Temporarily change a command's required level until reboot. |
| `copyover` | Restart the game through the connection-preserving copyover process. |
| `copyroom` | Copy another room's name, description, sector and flags into the current room. |
| `craftedit` | Create or edit crafting recipes. |
| `detach` | Remove a DG trigger attachment. |
| `dig` | Create or link rooms through directional exits. |
| `echo` | Send an unattributed message to the current room. |
| `encounterinfo` | Display wilderness encounter coverage and terrain information. |
| `eqrating` | Show equipment ratings and item balance information. |
| `featlisting` | List feat names and numeric IDs. |
| `file` | Read selected server logs and text files in game. |
| `findmagic` | Find potions, scrolls, wands or staves containing a specified spell. |
| `goto` | Move yourself to a specified character, object or room. |
| `handbook` | Read the staff handbook. |
| `hedit` | Create or edit help entries. |
| `helpcheck` | List commands missing help entries. |
| `holylight` | Toggle enhanced staff vision. |
| `i3admin` | Inspect or manage the Intermud3 connection and configuration. |
| `immtitle` | Set your staff title. |
| `imotd` | Read the immortal message of the day. |
| `invis` | Set your staff invisibility level. |
| `keycheck` | Check a zone's door keys and their object references. |
| `last` | Display a character's recorded login information. |
| `links` | List exits connecting a zone to other zones. |
| `lms` | Alias for loadmagicspecific. |
| `load` | Create a mobile or object from its prototype. |
| `loadmagic` | Generate random magical loot. |
| `loadmagicspecific` | Generate loot of a selected grade and equipment/item category. |
| `medit` | Create or edit mobile prototypes. |
| `mlist` | List mobile prototypes. |
| `noclantalk` | Toggle whether you receive clan-channel communication. |
| `nohassle` | Toggle staff immunity to supported mortal restrictions and NPC harassment. |
| `nowiz` | Toggle whether you receive the staff channel. |
| `objcheck` | Check live-object and prototype-index consistency. |
| `oedit` | Create or edit object prototypes. |
| `olc` | Display the pending OLC save list. |
| `olist` | List object prototypes, with supported filters. |
| `olistapplies` | List objects by their apply bonuses. |
| `oset` | Change fields on an object through command arguments. |
| `pathlist` | List saved wilderness paths. |
| `peace` | Stop fighting in the current room. |
| `pubsubqueue` | Inspect or process the publish/subscribe event queue. |
| `purge` | Remove targeted NPCs/objects or clear eligible room contents. |
| `qedit` | Create or edit quest definitions. |
| `qlist` | List quest definitions. |
| `qref` | Find object references in mobile-attached quests. |
| `qview` | Display quests attached to a mobile prototype. |
| `recent` | List recently connected players. |
| `redit` | Create or edit room definitions. |
| `reglist` | List wilderness regions and their supported classifications. |
| `rlist` | List room definitions. |
| `roomflags` | Toggle staff room-vnum, flag and sector display. |
| `roomvnum` | Display the current room's vnum. |
| `saveall` | Save pending world/OLC changes and house files. |
| `sedit` | Create or edit shop definitions. |
| `setroomdesc` | Change the current room's description. |
| `setroomflags` | Change flags on the current room. |
| `setroomname` | Change the current room's name. |
| `setroomsect` | Change the current room's sector type. |
| `settime` | Set the in-game hour and optionally day/month. |
| `setweather` | Set the current weather state. |
| `shoplist` | List shops. |
| `shopstat` | Display detailed shop information. |
| `show` | Display selected game, world or player administrative information. |
| `showblockers` | List loaded NPCs with directional blocking flags. |
| `showwearoff` | Display a spell or skill's wear-off message. |
| `singlefile` | List single-file rooms and flag unusual exit counts. |
| `slist` | List shop definitions through the OLC listing system. |
| `spellquests` | Placeholder for a spell-quest listing; currently unimplemented. |
| `stat` | Inspect detailed character, object or room information. |
| `teleport` | Move a character to another location. |
| `tlist` | List DG trigger definitions. |
| `todo` | View or update your personal staff to-do list. |
| `transfer` | Bring a character to your current room. |
| `trigedit` | Create or edit DG trigger definitions. |
| `tstat` | Inspect a DG trigger's definition. |
| `vdelete` | Delete a world prototype by type and vnum. |
| `vnum` | Search supported prototype names, including room names and quest titles. |
| `vstat` | Inspect a prototype by type and vnum. |
| `wizhelp` | List privileged commands grouped by required staff level. |
| `wiznet` | Send messages or manage listening on the staff channel. |
| `zcheck` | Check a zone's content for building and balance issues. |
| `zedit` | Create or edit zones and their reset commands. |
| `zpurge` | Remove eligible NPCs and objects throughout a zone. |
| `zreset` | Run a zone reset immediately. |

Source: [command table](../../src/interpreter.c) and
[wizhelp listing function](../../src/act.informative.c).
