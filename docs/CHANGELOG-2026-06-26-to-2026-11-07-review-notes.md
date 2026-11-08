# Changelog review notes — June 26–November 7, 2026

The publication-oriented overview is
[CHANGELOG-2026-06-26-to-2026-11-07.md](CHANGELOG-2026-06-26-to-2026-11-07.md).
It deliberately omits implementation instructions, formulas, prices and most
individual balance values.

## Scope and method

- Repository: [d20MUD/ChroniclesOfKrynn](https://github.com/d20MUD/ChroniclesOfKrynn).
- Date boundary: June 26, 2026, inclusive, using America/Halifax time.
- Latest reviewed commit: [`333efa96`](https://github.com/d20MUD/ChroniclesOfKrynn/commit/333efa96b2a6f6b7fe22a89a423fb7a408e7c152), November 7.
- Baseline: `96a02b8f`, the last first-parent commit before the boundary.
- Complete comparison: [baseline through reviewed HEAD](https://github.com/d20MUD/ChroniclesOfKrynn/compare/96a02b8f328fd58fcac43c3784d7e24e74addf1f...333efa96b2a6f6b7fe22a89a423fb7a408e7c152).
- Coverage: 139 commits in the merged history, including 128 non-merge commits.
- Reviewed GitHub commit messages and selected GitHub commit diffs, the full local
  merged history, changed-file inventories, targeted implementation diffs,
  current implementations, and accompanying help and design documents.
- GitHub's latest returned commit matched local HEAD. The local repository is
  not shallow; its full merged history supplies changes beyond GitHub search's
  result limit. The older dated changelog was used as an index and checked
  against the narrower date range and code, rather than copied as evidence.
- Older branch commits merged after June 26 are included. A simple author-date
  list would miss some of the June 26 fixes. Merge summaries, duplicate fixes,
  implementation plans and formatting/build-artifact changes are not counted
  as separate gameplay additions.

## Evidence by topic

| Topic | Representative commits | Implementation checked |
| --- | --- | --- |
| Loremaster and prestige-class perk handling | `3b904297`, `40e1bc46`, `0eed9d92`, `4990bfa9`, `4cc7b5c1` | `class.c`, `study.c`, `perks.c`, `feats.c`, `players.c` |
| Advanced races and racial abilities | `fe7d94e7`, `91a888d2`, `7c824c2c`, `ae5cc055`, `e96e427d`, `5f76e9b7` | `race.c`, `char_descs.c`, `act.offensive.c`, `spell_parser.c`, `spells.c`, `structs.h`; current race help |
| Introductions, anonymity, disguises and senses | `dba57f83`, `c26cb746`, `9c5a8318` | `introduce.c`, `char_descs.c`, `act.informative.c`, `comm.c`, `utils.c`, identity-aware communication and movement changes |
| Trading, housing, auctions and stalls | `4a6eb5a8`, `00ec6eca`, `3c9f9b5d`, `3082bf12`, `333efa96` | `act.item.c`, `housing.c`, `auction_house.c`, `player_shop.c`, `estate.c` |
| Legendary equipment | `ae5cc055` | `legendary.c`, `legendary_commands.c`, `legendary_equipment.c`, `legendary_foundations.c`, `legendary_effects.c`, `legendary_techniques.c`, `legendary_earnings.c`, transaction/storage modules and `infuse` help |
| Formations, range, ammunition and room size | `7717f436`, `c19c93b8`, `ec2a19e3`, `b9cee288`, `5db62848` | `fight.c`, `act.offensive.c`, `assign_wpn_armor.c`, `combat_modes.c`, `utils.c`, `magic.c`, `genwld.c`, `redit.c`, `oedit.c`, `feats.c` |
| Cleric casting, domain spells, Wizard/Sorcerer perks | `8b725471`, `6a04cc89`, `ec2a19e3`, `71d2de72`, `a408d4da`, `0d735329`, `35699086`, `5f76e9b7`, `ad9f427e` | `domains_schools.c`, `spell_prep.c`, `spell_parser.c`, `perks.c`, `study.c` |
| Epic magic, moons, new spells and psionic fixes | `1c6c9938`, `3b290cd9`, `1a7741b7`, `f1bebe6c`, `91a888d2`, `ffc0f982`, `7473dca8` | `epic_magic.c`, `moon_bonus_spells.c`, `magic.c`, `spell_parser.c`, `spells.c`, `psionics.c`, `limits.c` |
| Pets, golems, revival and travel | `26b0aee7`, `080b0fe8`, `024e140b`, `c80f87a2`, `0abd79b3`, `a4cd8b25`, `f923c037`, `ad9f427e` | `mob_utils.c`, `players.c`, `limits.c`, `crafting_new.c`, `act.other.c`, `act.informative.c`, `movement_validation.c` |
| Crafting, catalysts, harvesting and account rewards | `4cc7b5c1`, `5f4fd9ba`, `0abd79b3`, `6ba2d502`, `2ad3cebf`, `66789d53`, `51058122`, `43421b0d`, `bb399ea7`, `f315e524` | `crafting_new.c`, `crafting_recipes.c`, `crafts.c`, `brew.c`, `talents.c`, `treasure.c`, `account.c`, `cedit.c` |
| Quests, hunts and clans | `4990bfa9`, `5a309022`, `7aa56d9d`, `7c824c2c`, `91a888d2`, `90b4f21a`, `b907a20c`, `43421b0d` | `quest.c`, `act.item.c`, `dg_triggers.c`, `hunts.c`, `clan.c`, login notifications |
| Buffs, devices, mail, help and presentation | `9a0d92e8`, `beb1db14`, `e71d53c3`, `f7243800`, `e96e427d`, `be90435e`, `c477f512`, `d3370ee5` | `act.other.c`, `new_mail.c`, `spellhelp.c`, `help.c`, `feats.c`, `improved-edit.c`, `interpreter.c` |
| Builder tools, persistence and zone promotion | `8b2c5b48`, `88f3eb31`, `e96e427d`, `af9f9402`, `333efa96` | `act.wizard.c`, `oasis_copy.c`, `objsave.c`, `comm.c`, `limits.c`, `housing.c`, `lib/misc/zone_promote.py` |
| Faerûn shared-code work | `86d2f39a` | `wilderness.c`, `asciimap.c`, campaign conditionals and travel/mapping integrations |

The large legendary-item commit also includes Irda racial powers and the
feat-information export. The pet/persistence commit also includes mail replies
and BuildWalk overlap handling. The ranged-combat commit includes room sizing,
combat advancement, thrown weapons, ammunition and feat changes. These were
checked beyond their short commit subjects.

## Interpretation decisions

- Dragon Disciple itself predates June 26; this period's corrections are listed,
  rather than presenting the class as newly added.
- Feral Mutagen/Multiattack and level-range NPC-stat configuration were already
  present at the baseline and are not described as additions in this interval.
- Sorcerer perks were separated from Wizard perk storage/tables; the overview
  does not claim the planned later Sorcerer-specific redesign is complete.
- Epic magic changed more than once. The overview describes the final reworked
  system without repeating obsolete intermediate recovery rates or limits.
- Cross-room ranged-kill fixes are recorded as historical work before the later
  transition to same-room ranged combat; cross-room firing is not presented as
  a current supported feature.
- Legendary systems are implemented in code but use feature gates and world
  setup. Their inclusion is not a claim that every gate, boss or infusion smith
  is active on live. Proposed true-dragonlance content is not described as released.
- Faerûn wilderness work is explicitly separated as Forgotten Realms build work,
  not a new Krynn wilderness area.
- No versioned zone-data changes appear in this comparison. Unversioned live/dev
  world-building work, including the dev-only Palanthas invasion copy, is outside
  the source-history changelog and is not represented as a live content release.
- This review did not change source code, deploy game updates or run gameplay
  regression tests. It produced documentation from the existing implementation.

## Non-merge commit index

This preserves commit subjects for tracing coverage; it is not part of the publication copy.

| Commit | Merged-history subject |
| --- | --- |
| [576f0f2c](https://github.com/d20MUD/ChroniclesOfKrynn/commit/576f0f2c) | Add 'domain spells' subcommand to list a character's domain spells |
| [7e91ef84](https://github.com/d20MUD/ChroniclesOfKrynn/commit/7e91ef84) | Report available bonus domain spell slots and drop dead regen loop |
| [8368dabc](https://github.com/d20MUD/ChroniclesOfKrynn/commit/8368dabc) | Apply Weapon Finesse to unarmed strikes for any character with the feat |
| [6c5e2333](https://github.com/d20MUD/ChroniclesOfKrynn/commit/6c5e2333) | Prevent '!UNUSED!' placeholder spells from leaking to players |
| [325fcab4](https://github.com/d20MUD/ChroniclesOfKrynn/commit/325fcab4) | Fix Greater Mirror Image: distinct cast emote and fuller description |
| [334de362](https://github.com/d20MUD/ChroniclesOfKrynn/commit/334de362) | Fix Mirror Image emote grammar: 'smile' -> 'smiles' |
| [a9460632](https://github.com/d20MUD/ChroniclesOfKrynn/commit/a9460632) | Fix Acid Fog cascade and restore per-round damage |
| [3107d616](https://github.com/d20MUD/ChroniclesOfKrynn/commit/3107d616) | Fix Defensive Strike (and Bastion) paladin perk abilities |
| [0eed9d92](https://github.com/d20MUD/ChroniclesOfKrynn/commit/0eed9d92) | Fix Dragon Disciple perk points for non-Sorcerer characters |
| [cef52de6](https://github.com/d20MUD/ChroniclesOfKrynn/commit/cef52de6) | Linkdead characters will now be extracted after 5 minutes. Included a number of pull requests from Qworg for various fixes. |
| [26b0aee7](https://github.com/d20MUD/ChroniclesOfKrynn/commit/26b0aee7) | Pulled in some code fixes by Qworg. Improved crafted golem healing mechanics |
| [4990bfa9](https://github.com/d20MUD/ChroniclesOfKrynn/commit/4990bfa9) | Bug fixes for dialogue quest with no aabandon quest set causing questb progress deletion, and for taking perk in perk trees you don't have class levels in but have prestige class levels that gain perks in that tree. |
| [8b2c5b48](https://github.com/d20MUD/ChroniclesOfKrynn/commit/8b2c5b48) | Added the questcomplete staff command to assist characters who have lost quest progress. |
| [4cc7b5c1](https://github.com/d20MUD/ChroniclesOfKrynn/commit/4cc7b5c1) | Characters with prestige classes who gain perks in base classes can spend those perks now without requiring levels in the base class itself.   Sanction quests should no longer reset progress accidentally.   Added ability for dragon mounts to fly via 'order dragon fly'   Increased max page length and screenwidth to 200 each   Implemented disenchanting magical items and catalysts for crafting.   combined light weapon and finesse weapon checks into a single function check |
| [4a6eb5a8](https://github.com/d20MUD/ChroniclesOfKrynn/commit/4a6eb5a8) | Added the trade command and new trade system. Includes a REJECTTRADES flag so people can't get spammed with trade requests if they aren't interested. |
| [5f4fd9ba](https://github.com/d20MUD/ChroniclesOfKrynn/commit/5f4fd9ba) | Catalysts can be used on any crafted item, not just weapons and armor. |
| [cb61d5cf](https://github.com/d20MUD/ChroniclesOfKrynn/commit/cb61d5cf) | Fix 'Unused Feat' prerequisite shown for psionic focus feats |
| [6a04cc89](https://github.com/d20MUD/ChroniclesOfKrynn/commit/6a04cc89) | Added spopntaneous spellcasting for clerics. |
| [ec2a19e3](https://github.com/d20MUD/ChroniclesOfKrynn/commit/ec2a19e3) | Moved spell resistance, concealment and damage reduction into the defenses command. Damage Reduction from Gear and spells/spell affects should now properly apply. Added the following weapon types: estoc, fauchard, elven curve blade, falcata Added spontaneous casting for clerics. See HELP SPONTANEOUS |
| [81095394](https://github.com/d20MUD/ChroniclesOfKrynn/commit/81095394) | Fixed estoc as a finesse weapons. |
| [511528ee](https://github.com/d20MUD/ChroniclesOfKrynn/commit/511528ee) | Fxied a bug where monk attaqcks would always sneak attack if they have the deadly precision perk. |
| [2dd8859b](https://github.com/d20MUD/ChroniclesOfKrynn/commit/2dd8859b) | Fixed a bug with weapon training feat triggering crits when it shouldn't. Added power critical as a combat feat for warriors, eldritch knights, etc. |
| [5e0203bd](https://github.com/d20MUD/ChroniclesOfKrynn/commit/5e0203bd) | Add Loremaster implementation plan |
| [b9cee288](https://github.com/d20MUD/ChroniclesOfKrynn/commit/b9cee288) | Added the following feats as warrior bonus feats (also gained by eldritch knights, spellswords, Knights of Solamnia and Knights of the Lily: Power Critical, Double Weapon Critical, Bleeding Critical, Censoring Critical, Staggering Critical, Stunning Critical, Sickening Critical, Critical Mastery, Overwhelming Critical, Devastating Critical All perks cost 1 additional perk point. |
| [1c6c9938](https://github.com/d20MUD/ChroniclesOfKrynn/commit/1c6c9938) | Increased duration of all summoned charmies Increased damage on epic spells Adjusted # of epic spell slots to be based on spellcraft skill, and regens at a rate of 1 slot per 3 minutes. |
| [3b904297](https://github.com/d20MUD/ChroniclesOfKrynn/commit/3b904297) | Implement Loremaster prestige class |
| [71d2de72](https://github.com/d20MUD/ChroniclesOfKrynn/commit/71d2de72) | Fixed arcane recovery, recall spell and masterofelements perks. |
| [a408d4da](https://github.com/d20MUD/ChroniclesOfKrynn/commit/a408d4da) | Fixed the metamagic wizard perks. |
| [c26cb746](https://github.com/d20MUD/ChroniclesOfKrynn/commit/c26cb746) | Blindsense and blindsight now work as per PF1e rules. |
| [e666108f](https://github.com/d20MUD/ChroniclesOfKrynn/commit/e666108f) | Perk info can now be accessed via the help command. |
| [0d735329](https://github.com/d20MUD/ChroniclesOfKrynn/commit/0d735329) | fixed arcane suipremacy perk. |
| [86d2f39a](https://github.com/d20MUD/ChroniclesOfKrynn/commit/86d2f39a) | Added wilderness to faerun that is created based on a provided worldmap fo faerun. |
| [34a49c59](https://github.com/d20MUD/ChroniclesOfKrynn/commit/34a49c59) | oedit weapon spells bug fix. |
| [d7e62bf4](https://github.com/d20MUD/ChroniclesOfKrynn/commit/d7e62bf4) | Fixed a bug with dance of protection. increased bard song base duration. |
| [c4b19a22](https://github.com/d20MUD/ChroniclesOfKrynn/commit/c4b19a22) | Fixed a bug with epic mage armor. |
| [77e8b6e4](https://github.com/d20MUD/ChroniclesOfKrynn/commit/77e8b6e4) | Fixed a bug with psionic focus that was reducing damage by mistake. |
| [72a04f95](https://github.com/d20MUD/ChroniclesOfKrynn/commit/72a04f95) | Bug fix for swift death. |
| [bc0519c7](https://github.com/d20MUD/ChroniclesOfKrynn/commit/bc0519c7) | Bug fix for master of elements perk. |
| [da6a24f0](https://github.com/d20MUD/ChroniclesOfKrynn/commit/da6a24f0) | Fix to epic mage armor AC apply amount. |
| [c38e4da3](https://github.com/d20MUD/ChroniclesOfKrynn/commit/c38e4da3) | Fixed the object flag: can only equip one so that only one of the flagged item can be equipped at once. |
| [5a309022](https://github.com/d20MUD/ChroniclesOfKrynn/commit/5a309022) | Quest objectives completed by one group member now complete for all group members on the same quest. Also, charmie kills will apply the quest objective to the charmie's owner. |
| [f8485c0e](https://github.com/d20MUD/ChroniclesOfKrynn/commit/f8485c0e) | Fixed a bug with applypoison routing to the wrong command. |
| [730d7f1c](https://github.com/d20MUD/ChroniclesOfKrynn/commit/730d7f1c) | bard performances now check for all equipped instruments. |
| [9c5a8318](https://github.com/d20MUD/ChroniclesOfKrynn/commit/9c5a8318) | Fixed a bug where hiding NPCs could never be spotted by players regardless of perception roll.s Fixed a colour bleeding issue. |
| [eba818a4](https://github.com/d20MUD/ChroniclesOfKrynn/commit/eba818a4) | Fixed a bug with storing scrolls. |
| [e5cb57e9](https://github.com/d20MUD/ChroniclesOfKrynn/commit/e5cb57e9) | Master assassin now properly checking prerequisites. |
| [283f2aac](https://github.com/d20MUD/ChroniclesOfKrynn/commit/283f2aac) | Fixed touch of undeath. |
| [a840c20f](https://github.com/d20MUD/ChroniclesOfKrynn/commit/a840c20f) | Fixed litany of righteousness. |
| [8772eeee](https://github.com/d20MUD/ChroniclesOfKrynn/commit/8772eeee) | NPCs killed by a ranged attack from another room needed to have the player move into the same room to finish the kill. This is no longer required. |
| [93b54219](https://github.com/d20MUD/ChroniclesOfKrynn/commit/93b54219) | Fixed: armor-class enchantments lacked mote mappings for universal and enhancement bonuses. Both now require and display water motes. The mote reference list also includes them. |
| [ce29bd2b](https://github.com/d20MUD/ChroniclesOfKrynn/commit/ce29bd2b) | Fixed some bugs in the FOG room flag. |
| [04dfa83b](https://github.com/d20MUD/ChroniclesOfKrynn/commit/04dfa83b) | Fixed bozak draconian death throes. |
| [c80f87a2](https://github.com/d20MUD/ChroniclesOfKrynn/commit/c80f87a2) | Fixed a golem-loading bug: crafted golems lost their identifying flag after login, so golemrepair rejected them despite their names still working. Loading now restores that flag and construct identity. Repair also recovers them for already-loaded crafted golems. |
| [024e140b](https://github.com/d20MUD/ChroniclesOfKrynn/commit/024e140b) | Fixed: the crafting check classified an unflagged golem prototype as an ordinary charmee, causing other followers to block completion and reset the project. Crafted golems now use their separate follower allowance. Mercenaries, mounts, and other charmies can remain; a second golem is still blocked. |
| [f896e10a](https://github.com/d20MUD/ChroniclesOfKrynn/commit/f896e10a) | Fixed: reforging no longer awards general XP or advances the obsolete Fast Crafter, Weapon Smithing, and Armor Smithing skills. Eligible items can still be reforged repeatedly, but the reward loop is removed. |
| [d898edc2](https://github.com/d20MUD/ChroniclesOfKrynn/commit/d898edc2) | Fixed stored-wand spell-name parsing |
| [49b161a2](https://github.com/d20MUD/ChroniclesOfKrynn/commit/49b161a2) | Fixed: Artificers can now scribe eligible Wizard spells without memorizing them, using their Artificer level for eligibility and scroll caster level. |
| [c6d41fad](https://github.com/d20MUD/ChroniclesOfKrynn/commit/c6d41fad) | Added Lich Touch to the cooldown menu, showing its remaining time in seconds. |
| [3a6d75e8](https://github.com/d20MUD/ChroniclesOfKrynn/commit/3a6d75e8) | Hunt dragons now use their configured breath element instead of defaulting to fire. |
| [9407a702](https://github.com/d20MUD/ChroniclesOfKrynn/commit/9407a702) | Fixed: hoopak crafting now accepts an equipped weaponsmith’s hammer. Its Woodworking recipe previously had no supported tool in the validation check. |
| [46bcac82](https://github.com/d20MUD/ChroniclesOfKrynn/commit/46bcac82) | Fixed a check that could block shopkeeper price replies: NPCs were required to know the player through the introduction system before sending a tell. |
| [bc542780](https://github.com/d20MUD/ChroniclesOfKrynn/commit/bc542780) | Fixed: the combat loop was allowing targets with Psionic Blast’s stun flag to keep fighting. It now stops their combat turn for the stun’s duration, including augmentation. |
| [2f584331](https://github.com/d20MUD/ChroniclesOfKrynn/commit/2f584331) | Fixed several supply-order issues locally: - Old offices no longer issue obsolete orders under mote crafting. - Abandoning a current order refunds materials and clears its recipe and progress. - Completing an order clears the leftover recipe. |
| [644a3a67](https://github.com/d20MUD/ChroniclesOfKrynn/commit/644a3a67) | Fixed: adding a shop to a quest master now preserves the yellow (!) marker. |
| [5fc592b8](https://github.com/d20MUD/ChroniclesOfKrynn/commit/5fc592b8) | Fixed: the 256-color preference now survives linkdead saves and is restored on reconnect. Both ON and explicitly OFF settings are preserved. |
| [02e405bb](https://github.com/d20MUD/ChroniclesOfKrynn/commit/02e405bb) | Fixed: Deadly Fear now uses area damage when augmented by 8 or more points. Below 8, it remains single-target. Normal AoE target protections still apply. |
| [381cc1cb](https://github.com/d20MUD/ChroniclesOfKrynn/commit/381cc1cb) | Fixed: Alchemists now receive Apply Poison at level 6, alongside Swift Poisoning. Existing characters with Swift Poisoning can also pass the poison command’s feat check. |
| [90b4f21a](https://github.com/d20MUD/ChroniclesOfKrynn/commit/90b4f21a) | Added: proposed hunt rooms are rejected unless they have an exit path to 6530, 2200, or 1317. Rejected rooms trigger another selection; after 1,024 failed attempts, that hunt slot is skipped. This is so we can add hunts to road zones without worrying about the hujnt loading in a room that's not connected. |
| [7aa56d9d](https://github.com/d20MUD/ChroniclesOfKrynn/commit/7aa56d9d) | Implemented: successful dialogue completion now also marks the fallback quest completed and follows its next quest when set. Otherwise, it uses the dialogue quest’s own next quest. Failed checks still activate the fallback quest normally. |
| [6bf60ab9](https://github.com/d20MUD/ChroniclesOfKrynn/commit/6bf60ab9) | Implemented random valid weapon families, skills, and spell schools for typed gear feats. Combat, skill, and spell checks now read the equipped item’s specific value, and item details show the assigned type. |
| [0844cdd2](https://github.com/d20MUD/ChroniclesOfKrynn/commit/0844cdd2) | Fixed: pet saves were storing the NPC’s colored display description as its target keywords. Saves now use the actual keywords, and loading restores prototype aliases for already-affected pets. |
| [f71f83de](https://github.com/d20MUD/ChroniclesOfKrynn/commit/f71f83de) | Fixed: Dazzling Display now applies dazzle to eligible enemies instead of its user, and processes each target correctly. |
| [88f3eb31](https://github.com/d20MUD/ChroniclesOfKrynn/commit/88f3eb31) | Added 'vnum r' command for searching rooms. |
| [b907a20c](https://github.com/d20MUD/ChroniclesOfKrynn/commit/b907a20c) | Fixed: moving an active hunt target out of its starting room no longer allows a duplicate to spawn. The spawn check now looks for it anywhere in the world. |
| [2c352a4b](https://github.com/d20MUD/ChroniclesOfKrynn/commit/2c352a4b) | charmies of charmies will assist the charmie's PC master |
| [00ec6eca](https://github.com/d20MUD/ChroniclesOfKrynn/commit/00ec6eca) | Implemented a more robust and feature rich housing system that allows players to buy and build houses on their own using neighbourhoods. Also allows the creation of storage chests inside houses. Created an auction house that allows players to sell to other players via said AH. |
| [3c9f9b5d](https://github.com/d20MUD/ChroniclesOfKrynn/commit/3c9f9b5d) | Moved the ah subcommands into the ah command itself. |
| [9a0d92e8](https://github.com/d20MUD/ChroniclesOfKrynn/commit/9a0d92e8) | Fixed a number of compiler warnings. Added up to 10 buff lists for the buff command. |
| [beb1db14](https://github.com/d20MUD/ChroniclesOfKrynn/commit/beb1db14) | Added more functionality for buffing spells with special arguments. |
| [7717f436](https://github.com/d20MUD/ChroniclesOfKrynn/commit/7717f436) | Added combat group formations with ranks in front, middle and back. |
| [0f93d9b5](https://github.com/d20MUD/ChroniclesOfKrynn/commit/0f93d9b5) | Added back the statscap command. |
| [f923c037](https://github.com/d20MUD/ChroniclesOfKrynn/commit/f923c037) | Added dragon flight ability for dragon riders allowing them to cast overland flight spell for a dragoon point. |
| [3b290cd9](https://github.com/d20MUD/ChroniclesOfKrynn/commit/3b290cd9) | Made some improvements to epic spells, increasing and expanding their eaffect and accessibility. |
| [b7aaa7c6](https://github.com/d20MUD/ChroniclesOfKrynn/commit/b7aaa7c6) | Added practiced spellcaster feat. |
| [b240021d](https://github.com/d20MUD/ChroniclesOfKrynn/commit/b240021d) | affects shows remaining hit points for Stoneskin, Ironskin, and Epic Warding. Recasting refreshes protection, including fixing Stoneskin’s refresh bug and removing Ironskin’s recast threshold. |
| [aed7f63a](https://github.com/d20MUD/ChroniclesOfKrynn/commit/aed7f63a) | Implemented: - Bomb creation shows prepared/maximum counts. - New Faster Bomb Preparation feat halves preparation cooldown to three seconds. - Bomb throws show attack rolls and damage dice. - Pure alchemists no longer see the three unsuitable feats. - Airborne characters display “is flying here.” - Ordinary alchemist extracts work silently, including in silenced and soundproof areas. |
| [cfb8ffe6](https://github.com/d20MUD/ChroniclesOfKrynn/commit/cfb8ffe6) | Implemented: bombs now share the ranged attack category. Ranged weapon-focus feats improve bomb throws, and bomb accuracy bonuses apply to ranged rolls without double-counting. |
| [f315e524](https://github.com/d20MUD/ChroniclesOfKrynn/commit/f315e524) | Implemented permanent account-wide bonuses for XP, treasure drops, gold, and quest points. - Each +1% costs 2,000 account XP, capped at 10% per category. - Use accexp bonus to view bonuses; accexp bonus <type> to buy. - Bonuses persist and update across logged-in characters. - Treasure upgrades add percentage points to drop chance. |
| [1a7741b7](https://github.com/d20MUD/ChroniclesOfKrynn/commit/1a7741b7) | Fully implemented the entire moon alignment system for arcane spelcasters. See HELP MOONS |
| [e33b6fba](https://github.com/d20MUD/ChroniclesOfKrynn/commit/e33b6fba) | shadowcasting doesn't require concentration checks. |
| [f7243800](https://github.com/d20MUD/ChroniclesOfKrynn/commit/f7243800) | Implemented: news, motd, who, and other informational/chat commands now work during device creation and repair without interrupting progress. Movement, combat, casting, and new crafting actions remain blocked. Build and focused regression checks passed. |
| [2ad3cebf](https://github.com/d20MUD/ChroniclesOfKrynn/commit/2ad3cebf) | Implemented rare material and mote drops alongside existing random treasure. - Higher material grades unlock with mob level. - Grade 5 materials and dragonmetal require level 30+ mobs and are especially rare. - Drops enter crafting balances and save immediately; view them with materials. |
| [66789d53](https://github.com/d20MUD/ChroniclesOfKrynn/commit/66789d53) | Increased Insightful for mining, hunting, forestry, gathering, and butchering to 20% bonus XP per talent rank, reaching 100% at five ranks. |
| [a4cd8b25](https://github.com/d20MUD/ChroniclesOfKrynn/commit/a4cd8b25) | Implemented automatic swimming success for crocodiles, water elementals, aquatic-tagged pets, polar bears, and black/green dragon forms. They no longer need Athletics rolls in swimmable water. Wildshape benefits end when reverting; boat-only waters and ocean ship requirements remain. Removed detect magic, detect poison, and detect alignment from automatic mob buffing. Detect invisibility remains available, and true seeing now works in wizard pre-buffing, subject to existing class and level requirements. |
| [e71d53c3](https://github.com/d20MUD/ChroniclesOfKrynn/commit/e71d53c3) | For example, device move 7 1 moves device 7 first and shifts the others down. The order is saved, preserving charges, cooldowns, and condition. |
| [080b0fe8](https://github.com/d20MUD/ChroniclesOfKrynn/commit/080b0fe8) | implemented pet progression and reviving. |
| [0abd79b3](https://github.com/d20MUD/ChroniclesOfKrynn/commit/0abd79b3) | Added refine batching. Added golem appearance changing. Added zone climates and terrain. |
| [6ba2d502](https://github.com/d20MUD/ChroniclesOfKrynn/commit/6ba2d502) |   Added a harvest material exchange spec proc, to allow exchanging materials of the same type and grade. Eg grade 1 ash wood can be exchanged 1:1 for grade 1 pine wood   Added a variety of different harvest material types, based on climate and terrain. |
| [6a0c767b](https://github.com/d20MUD/ChroniclesOfKrynn/commit/6a0c767b) | added materials by climate and material changer spec proc. Staves can now be wielded in one hand or two |
| [51058122](https://github.com/d20MUD/ChroniclesOfKrynn/commit/51058122) | Implemented consistent happy-hour bonuses for crafting and gathering XP, refining and harvested material yields, butchering, and mote quantities. Fixed duplicate refining XP bonuses and bonuses lingering after happy hour ends. Harvest messages now show total material yields. |
| [d3370ee5](https://github.com/d20MUD/ChroniclesOfKrynn/commit/d3370ee5) | Removed @ and /@ as save shortcuts. Lines beginning with @ or color codes now remain in the editor; use /s to save. |
| [f1bebe6c](https://github.com/d20MUD/ChroniclesOfKrynn/commit/f1bebe6c) | Added litany of damnation spell. |
| [dba57f83](https://github.com/d20MUD/ChroniclesOfKrynn/commit/dba57f83) | Completed the introducton system, and added disguises and more true sight functionality. |
| [fe7d94e7](https://github.com/d20MUD/ChroniclesOfKrynn/commit/fe7d94e7) | Added new races: Irda, Sivak and Aurak. |
| [91a888d2](https://github.com/d20MUD/ChroniclesOfKrynn/commit/91a888d2) | Added new spells: protection from poison and panacea Some fixes to quest object turn ins Some edits to sivak race, |
| [7c824c2c](https://github.com/d20MUD/ChroniclesOfKrynn/commit/7c824c2c) | Added quest sharing, Shifters get disguise and alter self spells. Auraks can cast their energy rays as swift actions. |
| [3082bf12](https://github.com/d20MUD/ChroniclesOfKrynn/commit/3082bf12) | Added a player shop system, using the 'stalls' command. Added builder help on the web site admin area. |
| [955e4eb5](https://github.com/d20MUD/ChroniclesOfKrynn/commit/955e4eb5) | Created a number of guides for staff purposes, posted on the web site. |
| [ae5cc055](https://github.com/d20MUD/ChroniclesOfKrynn/commit/ae5cc055) | Initial legendary items system in place. |
| [e96e427d](https://github.com/d20MUD/ChroniclesOfKrynn/commit/e96e427d) | Bug fixes around pet persistance and progression. Added alerts for imms and clan members when there's a pending application. Made aurak innate spells consume points instead of being unlimited at-will. |
| [394a2c87](https://github.com/d20MUD/ChroniclesOfKrynn/commit/394a2c87) | Updated the crafting tutorial and guides with the recent-ish crafting changes, from since the guides were initially made. |
| [43421b0d](https://github.com/d20MUD/ChroniclesOfKrynn/commit/43421b0d) | Added notifications for staff and clan members on log in when clan has pending applications. Added ability to  approve and reject clan applications even if applicant is not online. Fixed a bug with elbow grease bonus not applying. |
| [06ba4151](https://github.com/d20MUD/ChroniclesOfKrynn/commit/06ba4151) | Fixed a bug with supply order types not making sense. Removed the practice command, as crafting system no longer supports that screen. |
| [bb399ea7](https://github.com/d20MUD/ChroniclesOfKrynn/commit/bb399ea7) | Fixed a bug where jack of all trades bonuses werent applying. |
| [af9f9402](https://github.com/d20MUD/ChroniclesOfKrynn/commit/af9f9402) | Some fixes to item persistance to prevent unplanned item loss. |
| [ffc0f982](https://github.com/d20MUD/ChroniclesOfKrynn/commit/ffc0f982) | Fixed low DCs for psionic powers. |
| [9513bd60](https://github.com/d20MUD/ChroniclesOfKrynn/commit/9513bd60) | removed component from clone spell. Holy weapon now works for unarmed attack types. |
| [a234f8b9](https://github.com/d20MUD/ChroniclesOfKrynn/commit/a234f8b9) | Fixed an error with perks/level gain messaging. |
| [5ece8e6b](https://github.com/d20MUD/ChroniclesOfKrynn/commit/5ece8e6b) | Fixed lore to use your highest lore-type skill. Poisoned staus no longer breaks stealth. |
| [7473dca8](https://github.com/d20MUD/ChroniclesOfKrynn/commit/7473dca8) | Fixes to deadly fear psionic power. |
| [be90435e](https://github.com/d20MUD/ChroniclesOfKrynn/commit/be90435e) | created spellhelp that gives spell/psionic power info based on the coded mehcanics. |
| [c477f512](https://github.com/d20MUD/ChroniclesOfKrynn/commit/c477f512) | Changes to spellhelp, removing unnecessary info. |
| [35699086](https://github.com/d20MUD/ChroniclesOfKrynn/commit/35699086) | Fixed arcane augmentation perk and stunning blow saving throw process. |
| [6887e239](https://github.com/d20MUD/ChroniclesOfKrynn/commit/6887e239) | Removed aurak spells from the study spell list. Slam and draconian bite now show damage done. |
| [5f76e9b7](https://github.com/d20MUD/ChroniclesOfKrynn/commit/5f76e9b7) | Fixed a bug with aurak spellcasting. Fixed sorcerer spell slot allocation. Copied the wizard perk list so that sorcerer has its own table. We will customize sorcerer later. |
| [ad9f427e](https://github.com/d20MUD/ChroniclesOfKrynn/commit/ad9f427e) | Fixed some bugs with sorcerer spell selection, and allowing owners to view progression progress of their charmies. |
| [c19c93b8](https://github.com/d20MUD/ChroniclesOfKrynn/commit/c19c93b8) | Implemented the new ranged weapon system. |
| [333efa96](https://github.com/d20MUD/ChroniclesOfKrynn/commit/333efa96) | Added ability to change the name on your house. Added the zpromote command for copying zones from the dev port to live. |
