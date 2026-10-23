# MEDIT: mobile editor and builder reference

Source reviewed: 2026-10-23. This guide covers the current Krynn mobile editor,
its menus, all **107 `MOB_` flags**, NPC stats, resistance settings, class and
spell configuration, echoes, paths, scripts, and saving.

A mobile is an NPC. MEDIT edits its **prototype**, identified by a mobile vnum;
it does not directly edit a particular live NPC's combat state. Room and object
settings belong to [REDIT](REDIT_GUIDE.md) and [OEDIT](OEDIT_GUIDE.md).
The [affect guide](AFFECT_FLAGS_GUIDE.md) explains every AFF/AFF2 flag separately.

## Contents

- [Opening, creating, and saving](#opening-creating-and-saving)
- [Main menu](#main-menu)
- [Descriptions and position](#descriptions-and-position)
- [Race, subraces, class and size](#race-subraces-class-and-size)
- [Stats and automatic generation](#stats-and-automatic-generation)
- [Resistances](#resistances)
- [Feats, spells and combat behavior](#feats-spells-and-combat-behavior)
- [Complete NPC flag reference](#complete-npc-flag-reference)
- [Blocking, grouping and assistance](#blocking-grouping-and-assistance)
- [AFF and AFF2 settings](#aff-and-aff2-settings)
- [Plot NPC shortcuts](#plot-npc-shortcuts)
- [Echoes and movement messages](#echoes-and-movement-messages)
- [Patrol paths](#patrol-paths)
- [Scripts, special procedures and spawning](#scripts-special-procedures-and-spawning)
- [Building workflow and common mistakes](#building-workflow-and-common-mistakes)
- [Source references](#source-references)

## Opening, creating, and saving

| Command | Purpose |
| --- | --- |
| `medit <mobile-vnum>` | Edit an existing NPC prototype, or create one at an unused numeric vnum in a zone you may edit. |
| `medit save <zone-number>` | Save the zone's mobile prototypes to disk. The argument is a zone number, not a mobile vnum. |
| `medit save` | Attempts to use the configured OLC zone; prefer the explicit zone-number form. |

Bare `medit` requires a vnum; it does not select a nearby NPC. Neither `medit
new` nor `medit create <vnum>` is supported by the command parser. Builder
access and zone editing permission are required. Two builders cannot edit the
same prototype simultaneously.

The shorthand save path converts the configured OLC zone to its bottom room
vnum before looking it up as a zone number. That can fail or resolve the wrong
zone. Use `medit save <zone-number>` explicitly.

A new mobile starts with unfinished keywords and descriptions. Make a change
before quitting: its initial changed flag is false. Choose `Q`, then `Y` to save
or `N` to discard at the confirmation. If automatic OLC saving is disabled,
this commits only to memory; run `medit save <zone-number>` afterward. Watch
for `Mobile saved to memory.` versus `Mobile saved to disk.` Static prototypes
normally reside in the zone's `.mob` world file.

Saving an existing prototype updates matching live NPC **strings** and reattaches
their scripts. It does not re-run the whole mobile load/stat pipeline on each
existing NPC. Test a newly loaded copy when checking changed stats, flags,
spells or load-dependent behavior. This differs from OEDIT's broader live-object
update path. Script reattachment can also affect NPCs already in the world.

`X) Delete mob` invokes the deletion routine immediately after confirmation;
it does not wait for the later normal `Q` save prompt. Do not assume discarding
subsequent editor work undoes deletion.

## Main menu

| Choice | Setting | Purpose |
| --- | --- | --- |
| `1` | Sex | Pronouns/gender selection from the displayed list. |
| `2` | Keywords | Targeting aliases, such as `guard watchman soldier`. |
| `3` | S-Desc | Short name used in action messages and identification. |
| `4` | L-Desc | Sentence shown when the NPC stands in a room. Single-line input. |
| `5` | D-Desc | Detailed description shown when examining the NPC; multiline editor. |
| `6` | Position | Initial/current prototype position. |
| `7` | Default | Default position used by supported return-to-idle behavior. |
| `8` | Attack | Bare-hand/natural attack message/type selection. Not a breath-weapon setting. |
| `9` | Stats Menu | Level, autoroll, HP/damage dice, AC, rewards, alignment and optional advanced stats. |
| `G` | Resists | Damage-type resistance/vulnerability settings. |
| `R` | Race | NPC race family, such as Humanoid, Dragon or Undead. |
| `D`, `E`, `F` | SubRace | Three NPC subrace/subtype slots. |
| `C` | Class | Primary NPC class used by supported combat/casting and stat-generation paths. |
| `H` | Feats | Toggle explicit NPC feats, assign ranks, or add class feats. |
| `N` | Spells | Toggle explicitly known spells in addition to class-based access. |
| `I` | Size | Creature size; affects combat, traversal and other size checks. |
| `J` | Walk-In | Custom arrival-message prefix. |
| `K` | Walk-Out | Custom departure-message prefix. |
| `L` | Echo Menu | Ambient text, frequency, ordering and room/zone scope. |
| `M` | Plot Mob Settings | Apply a preset package of level/race/class/stats and protective flags. Does not create a shop or quest. |
| `O` | Random Descriptions | Replace gender, name, short/long description and detailed description with generated text. |
| `A` | NPC Flags | Mobile behavior, immunity, role and ability flags; see full table below. |
| `B` | AFF Flags | First-bank character affects. |
| `U` | AFF2 Flags | Second-bank character affects. |
| `S` | Script | Attach mobile DG triggers. |
| `V` | Path Edit | Replace the patrol route and delay count. |
| `Z` | SpecProc | Select one registered C special procedure, or `0` for none. |
| `W` | Copy mob | Copy an existing prototype into this edit buffer; review inherited settings. |
| `X` | Delete mob | Delete after confirmation. |
| `Q` | Quit | Leave the editor, saving/discarding if changed. |

## Descriptions and position

Use aliases for player targeting, a short description for action messages, a
long description for the room listing, and a detailed description for close
inspection. These fields have different jobs; a long description is not a list
of aliases. Blank long-description input becomes `undefined`.

The detailed description uses the multiline editor with a 1,024-byte buffer.
Use `/s` to save that text, `/a` to abort it, `/c` to clear it and `/h` for the
editor's help. `/s` returns to MEDIT; it does not save the whole prototype.

Position menu numbers are **code index + 1**. Selecting Fighting is converted
to Standing; combat must establish an opponent rather than loading a generic
NPC already fighting nobody.

| Choice | Code index | Position |
| --- | --- | --- |
| 1 | 0 | Dead |
| 2 | 1 | Mortally wounded |
| 3 | 2 | Incapacitated |
| 4 | 3 | Stunned |
| 5 | 4 | Sleeping |
| 6 | 5 | Reclining |
| 7 | 6 | Resting |
| 8 | 7 | Sitting |
| 9 | 8 | Fighting |
| 10 | 9 | Standing |

Sleeping, stunned, unconscious and dead positions are not interchangeable with
their similarly named timed affects. Use normal action/condition routines when
a creature needs a timed sleep, stun, knockout or recovery.

## Race, subraces, class and size

Race families are NPC creature categories, not simply the playable-race menu.
Choose the family, up to three relevant NPC subtypes, and the intended class
before using Auto Set Stats. They influence abilities, immunities, stat scaling
and racial behavior. A dragon's descriptive name alone does not configure its
breath type or natural abilities.

Race, NPC subrace and class lists use their **direct displayed IDs**. Entering
`99` chooses a random entry immediately; it is not a saved instruction to
randomize on every subsequent load.

### Race families

| ID | Family |
| --- | --- |
| 0 | Unknown |
| 1 | Humanoid |
| 2 | Undead |
| 3 | Animal |
| 4 | Dragon |
| 5 | Giant |
| 6 | Aberration |
| 7 | Construct |
| 8 | Elemental |
| 9 | Fey |
| 10 | Magical Beast |
| 11 | Monstrous Humanoid |
| 12 | Ooze |
| 13 | Outsider |
| 14 | Plant |
| 15 | Vermin |
| 16 | Lycanthrope |

The three subrace slots use the NPC subtype list shown in their menus. They can
represent traits relevant to the creature's rules, not three arbitrary playable
races. Use compatible combinations and verify the actual immunity/ability code.

The size menu also uses **direct IDs**, unlike OEDIT's size index + 1. It displays
`-1) DEFAULT`, but the current parser clamps input to 0–`NUM_SIZES - 1`, so -1
becomes 0 (RESERVED), rather than preserving a default-size sentinel. Choose an
explicit valid size for predictable results. Loading can subsequently normalize
invalid sizes or apply family-specific rules.

| ID | Size |
| --- | --- |
| 0 | RESERVED |
| 1 | Fine |
| 2 | Diminutive |
| 3 | Tiny |
| 4 | Small |
| 5 | Medium |
| 6 | Large |
| 7 | Huge |
| 8 | Gargantuan |
| 9 | Colossal |

## Stats and automatic generation

### Stats menu

| Choice | Setting | Current manual-input clamp |
| --- | --- | --- |
| `1` | Level | 1–`LVL_IMPL` |
| `2` | Auto Set Stats | Rebuilds multiple fields from level, race and class; overwrites manual tuning. |
| `3` | HP NumDice | 0–30 |
| `4` | HP SizeDice | 0–1,000 |
| `5` | HP Addition | 0–30,000 |
| `6` | Bare Hand Damage NumDice | 0–30 |
| `7` | Bare Hand Damage SizeDice | 0–127 |
| `8` | DamRoll | 0–50 |
| `A` | Armor Class | 0–600 raw units; base AC uses the internal d20 × 10 scale. |
| `B` | Exp Points | 0–150,000 through this input path |
| `C` | Gold | 0–100,000 through this input path |
| `D` | Hitroll | 0–50; separate from class BAB and other attack modifiers. |
| `E` | Alignment | Select one of the nine displayed alignment IDs. |
| `R` | Damage Reduction | 0–100 base DR modifier; not a percentage resistance entry. |
| `Q` | Back | Return to the main menu. |

These are editor clamps, not guarantees that loaded totals or auto-generated
values use exactly the same limits. Higher effective d20 AC is better; the raw
armor field and the computed parenthesized AC shown by the menu are different
values. Dexterity, class, size, effects and configuration can affect the latter.

### HP and damage dice

The normal prototype HP setup is **xDy + z**:

- x: HP NumDice.
- y: HP SizeDice.
- z: HP Addition.

For example, `10d20 + 100` has a mathematical range of 110–300 before other
load-time changes. Bare-hand damage similarly uses its dice and DamRoll, but
actual combat adds other rules and bonuses.

The prototype loader historically uses fields named hit/PSP/move to store the
HP dice parameters before converting them into runtime pools. **HP SizeDice is
not an editor for a caster's live PSP pool**, and HP Addition is not its live
movement pool. Some prototype formats/load paths also support a different
random-HP interpretation. Inspect a fresh mobile, not just these raw fields.

### Advanced stats

The extra section is available only when `CONFIG_MEDIT_ADVANCED` is enabled.
Attribute inputs clamp to 3–50; save fields clamp to 0–100.

| Choice | Actual field | Display caveat |
| --- | --- | --- |
| `F` | Strength | The menu also displays the legacy exceptional-Strength field. |
| `G` | Intelligence | Not the main-menu Resistances option. |
| `H` | Wisdom | Not the main-menu Feats option. |
| `I` | Dexterity | Not the main-menu Size option. |
| `J` | Constitution | Not the main-menu Walk-In option. |
| `K` | Charisma | Not the main-menu Walk-Out option. |
| `L` | Fortitude save field | Still labeled Paralysis. |
| `M` | Reflex save field | Still labeled Rods/Staves. |
| `N` | Will save field | Still labeled Petrification. |
| `O` | Poison save field | Still labeled Breath. |
| `P` | Death save field | Still labeled Spells. |

The old save labels do not match their current `SAVING_*` fields; use the actual
mapping above when tuning a creature.

### Auto Set Stats

Set level, family, class and intended size first. Auto Set Stats calls
`autoroll_mob()` in prototype mode. Its starting formulas include:

- Base HP addition: `level² + 10 × level`, then class/race/high-level adjustments.
- Initial hitroll and damroll: `level / 6 + 1`.
- Initial save modifiers: `level / 4`.
- Initial attributes: 10, with selected class stats receiving `level / 2`.
- Initial raw armor: `100 + 10 × level`, then class adjustments.
- Initial XP: `75 × level²`; initial gold: `10 × level`, then applicable changes.
- Bare-hand damage starts at one die with sides based on level; it is adjusted
  by class, with a minimum die size of 4 afterward.

Integer divisions truncate. Race adjustments include cases such as animal
mental-stat changes, dragon bonuses/SR, and zero gold for some families. Levels
above 30 have extra group-challenge scaling. These formulas are a starting
point, not a complete prediction of every loaded NPC stat.

### Load-time changes and the two custom flags

There are two separate settings that are easy to confuse:

| Flag | Effect |
| --- | --- |
| `MOB_CUSTOM_GOLD` | Preserves configured gold in the prototype parser. In the Dragonlance (`CAMPAIGN_DL`) mobile load path, its absence also triggers `autoroll_mob()` for multiple stats. It controls more than gold there. |
| `MOB_CUSTOM_MOB_STATS` | Skips the additional global category/level-range percentage modifier pass. It does not itself disable every autoroll or offer per-mobile percentage fields in this editor. |

Normal loading rolls HP, can apply powerful-being bonuses, normalizes some
state, applies campaign autoroll behavior and then calls
`apply_mob_stat_modifiers()`. The configurable category pass uses Warrior,
Arcane, Divine and Rogue groupings across level bands 1–5, 6–10, 11–15, 16–20,
21–25, 26–30, and above 30. Its settings live in the server configuration,
not MEDIT's per-mobile Stats menu.

For custom encounters, decide deliberately which automatic passes should apply,
then compare the saved prototype with a newly loaded copy. Enabling one custom
flag does not mean every other generation step is bypassed.

## Resistances

Main-menu `G` edits damage-type **percentage** resistance/vulnerability values,
clamped to **-100 through +100**. Positive values reduce eligible incoming damage;
negative values increase it. Before other bonuses/overrides, +50 corresponds to
half damage and -50 to 1.5 times damage. The actual calculation combines the
base field with racial, spell and other modifiers and uses integer rounding.
Not all damage types permit resistance.

This is distinct from flat DR and flat energy absorption. Do not interpret
resistance 10 as automatically subtracting exactly ten damage from each hit.

| Choice | Damage type | Choice | Damage type |
| --- | --- | --- | --- |
| `A` | Fire | `K` | Bludgeoning |
| `B` | Cold | `L` | Sound |
| `C` | Air | `M` | Poison |
| `D` | Earth | `N` | Disease |
| `E` | Acid | `O` | Negative |
| `F` | Holy | `P` | Illusion |
| `G` | Electric | `R` | Mental |
| `H` | Unholy | `S` | Light |
| `I` | Slashing | `T` | Energy |
| `J` | Piercing | `U` | Water |
| `V` | Force | `Q` | Return to main menu |

The parser supports **V for Force**, even though the current displayed
resistance-menu text omits that entry.

## Feats, spells and combat behavior

### H: explicit feats

Enter a feat name to toggle it; unambiguous names are preferable to short
prefixes. Stackable in-game feats prompt for a rank count; enter 0 to remove.
Other feats toggle between present and absent.

- `addclassfeats`: add assigned class feats through the mobile's current level.
- `erase`: clear all explicitly stored mobile feat entries.
- `quit`: return to the main menu.

`addclassfeats` increments matching assignments, so repeatedly invoking it is
not an idempotent refresh. Set class and level first, and review the displayed
rank counts. This picker does not provide a player-style feat prerequisite
wizard or a complete typed-feat specialization interface.

### N: explicitly known spells

Enter a spell name to toggle its explicit known entry. `erase` clears that
explicit list; `quit` returns. These entries supplement spells the NPC may
access from its class, so clearing them is not necessarily a complete ban on
class spellcasting.

The load path initializes class spell slots and a separate known-spell slot
system. Explicit known spells begin with two slots per known spell in that
system. Spell selection, actions, cooldowns and recovery still depend on the
NPC casting routines. `MOB_UNLIMITED_SPELL_SLOTS` bypasses supported slot checks;
it does not automatically teach every spell or grant unlimited actions.

### Automatic behavior

Race, class, assigned spells, known spells, feats and scripts are separate
inputs. A list of known spells does not specify a fixed cast order or a target
script. The NPC activity loop selects among supported racial behavior,
assigned spells, caster behavior and class routines. It can also pre-buff
outside combat.

`MOB_NOCLASS` disables supported class-behavior paths; it does not delete the
class field, and some other routines can still use it. `MOB_NO_AI` skips the
main mobile activity loop. `MOB_AI_ENABLED` is a separate conversational/ask/tell
integration flag, not a master switch for combat intelligence.

The generic `npc_ability_behave()` path does not implement all of the old
`MOB_ABIL_*` labels: it currently delegates to racial behavior or casting and
returns. Several ability flags are therefore declarations rather than working
automatic attacks. Poison and Corruption have explicit combat consumers; the
complete table below distinguishes the rest.

## Complete NPC flag reference

There are **107 code flags**, but only **105 displayed choices**. The NPC flag
menu hides `MOB_ISNPC` (index 3) and `MOB_NOTDEADYET` (index 20), then numbers the
remaining entries consecutively from 1. Consequently, its choice is **not
always code index + 1**. Use the exact menu choice below. `0` returns.

For example, menu choice 20 is **Mountable** in this version. Character Hiding
is `AFF_HIDE` in the separate `B) AFF Flags` menu, not a general NPC behavior bit.

| NPC menu choice | Code index | Flag | Display label | Purpose and current behavior |
| --- | --- | --- | --- | --- |
| 1 | 0 | `MOB_SPEC` | <spec> | Special-procedure marker used by periodic/combat proc dispatch. Attach a real Z-menu procedure; the bit alone supplies no behavior. |
| 2 | 1 | `MOB_SENTINEL` | Sentinel | Suppresses ordinary random wandering. Not a universal block on scripted movement, following, patrols or teleportation. |
| 3 | 2 | `MOB_SCAVENGER` | Scavenger | Picks up eligible ground items during scavenging behavior; item visibility/value and activity rules apply. |
| Hidden | 3 | `MOB_ISNPC` | Mob | Internal NPC identity flag, set by the system/save path. Hidden from the builder flag menu. |
| 4 | 4 | `MOB_AWARE` | Aware | Aware NPC state used by supported surprise/backstab checks. Not universal immunity to every precision attack. |
| 5 | 5 | `MOB_AGGRESSIVE` | Aggro | Aggressive automatic attack selection against eligible nearby targets, subject to visibility, safe rooms and other rules. |
| 6 | 6 | `MOB_STAY_ZONE` | Zone-Sentinel | Restricts supported random wandering to the current zone. Other movement systems can have separate rules. |
| 7 | 7 | `MOB_WIMPY` | Wimpy | Wimpy behavior: affects target selection and injury/flee checks. Not an absolute prohibition on fighting. |
| 8 | 8 | `MOB_AGGR_EVIL` | Aggro-Evil | Aggression toward eligible evil targets, subject to normal checks. |
| 9 | 9 | `MOB_AGGR_GOOD` | Aggro-Good | Aggression toward eligible good targets, subject to normal checks. |
| 10 | 10 | `MOB_AGGR_NEUTRAL` | Aggro-Neutral | Aggression toward eligible neutral targets, subject to normal checks. |
| 11 | 11 | `MOB_MEMORY` | Memory | Remembers supported attackers and can retaliate on later encounters; needs the memory system to populate targets. |
| 12 | 12 | `MOB_HELPER` | Helper | Helper behavior can join fights involving eligible NPC allies. Group, pet, mission and assistance rules still apply. |
| 13 | 13 | `MOB_NOCHARM` | Uncharmable | Charm immunity checked by supported charm application paths. |
| 14 | 14 | `MOB_NOSUMMON` | Unsummonable | Summoning immunity checked by supported summon paths; separate from no-teleport. |
| 15 | 15 | `MOB_NOSLEEP` | Immune-Sleep | Sleep immunity checked by supported sleep effects. Not a requirement for an NPC to remain standing. |
| 16 | 16 | `MOB_NOBASH` | Unbashable | Bash/knockdown immunity in supported maneuver checks; not immunity to every physical attack. |
| 17 | 17 | `MOB_NOBLIND` | Unblindable | Blindness immunity in supported application checks. |
| 18 | 18 | `MOB_NOKILL` | Unkillable | Unkillable/unattackable checks used by supported attacks and damage paths. Use with No-Fight for protected service NPCs; not a universal script invulnerability guarantee. |
| 19 | 19 | `MOB_SENTIENT` | Sentient | Treats a non-humanoid NPC as sentient in the IS_SENTIENT predicate. Does not enable conversational AI by itself. |
| Hidden | 20 | `MOB_NOTDEADYET` | !DEAD! | Internal extraction/death-in-progress marker. Hidden from the menu; never set on a normal prototype. |
| 20 | 21 | `MOB_MOUNTABLE` | Mountable | Marks the NPC mountable and affects loaded movement capacity. Still needs compatible riding/ownership/state checks. |
| 21 | 22 | `MOB_NODEAF` | Immune-Deaf | Deafness immunity in supported application checks. |
| 22 | 23 | `MOB_NOFIGHT` | Does-Not-Fight | Does-not-fight marker used by combat eligibility helpers. Not the same as No-AI or Unkillable. |
| 23 | 24 | `MOB_NOCLASS` | Classless | Classless flag disables supported NPC class/psionic behavior paths. Does not erase the class field or every class-based calculation. |
| 24 | 25 | `MOB_NOGRAPPLE` | Ungrappleable | Grapple immunity in supported maneuver checks. |
| 25 | 26 | `MOB_C_ANIMAL` | Animal-Companion | Animal-companion role marker. Full companion behavior needs master/follower, summon and persistence setup. |
| 26 | 27 | `MOB_C_FAMILIAR` | Familiar | Familiar role marker. Does not create a familiar bond or owner by itself. |
| 27 | 28 | `MOB_C_MOUNT` | Paladin/Blackguard-Mount | Paladin/blackguard mount role marker. Needs the mount/summon ownership system. |
| 28 | 29 | `MOB_ELEMENTAL` | Summoned-Elemental | Summoned-elemental role marker. Distinct from selecting Elemental as the race family. |
| 29 | 30 | `MOB_ANIMATED_DEAD` | Animated-Dead | Animated-undead role marker used by summon/control handling. Distinct from simply choosing Undead race. |
| 30 | 31 | `MOB_GUARD` | Guard | Guard assistance behavior protecting eligible citizens/allies; can interact with Helper and mission restrictions. |
| 31 | 32 | `MOB_CITIZEN` | Citizen | Citizen marker recognized by Guard assistance checks. Does not create a faction or civic quest record. |
| 32 | 33 | `MOB_HUNTER` | Hunter | Hunter behavior pursues a known hunt/memory target through the navigation system. Does not by itself create a bounty-hunt encounter. |
| 33 | 34 | `MOB_LISTEN` | Listen | Can react to fighting in an adjacent accessible room and move toward it, subject to activity/ownership checks. |
| 34 | 35 | `MOB_LIT` | Lit-Up | Lit-up NPC marker used by visibility/light handling. Not a room-sector setting. |
| 35 | 36 | `MOB_PLANAR_ALLY` | Planar-Ally | Declared Planar Ally role marker. No active direct consumer of this bit found in the current source. |
| 36 | 37 | `MOB_NOSTEAL` | No-Steal | Prevents supported stealing attempts against the NPC. |
| 37 | 38 | `MOB_INFO_KILL` | Info-Kill | Broadcast/info handling when this NPC is killed, through the corresponding death-message path. |
| 38 | 39 | `MOB_CUSTOM_GOLD` | Custom-Gold | Preserves configured gold; also controls the Dragonlance load-time autoroll condition for several stats. See load-time notes. |
| 39 | 40 | `MOB_NO_AI` | No-AI | Skips the main mobile activity loop, including its autonomous behavior and echoes. Other combat/script paths are separate. |
| 40 | 41 | `MOB_MERCENARY` | Mercenary | Mercenary follower role marker, used by purchase/companion rules. Not a complete mercenary shop setup. |
| 41 | 42 | `MOB_ENCOUNTER` | Encounter | Marks a wilderness/random-encounter NPC for encounter lifecycle/reward handling. Do not use as a generic wandering toggle. |
| 42 | 43 | `MOB_SHADOW` | Shadow | Shadowdancer summoned-shadow role marker. Requires its owning summon system. |
| 43 | 44 | `MOB_IS_OBJ` | Mob-Is-Obj | NPC represents an object-like entity, such as a quest board, in supported display/interaction logic. Does not change it into an item prototype. |
| 44 | 45 | `MOB_BLOCK_N` | Mob-Block-North | Blocks northward departure through the current room, subject to exemptions described below. |
| 45 | 46 | `MOB_BLOCK_E` | Mob-Block-East | Blocks eastward departure through the current room. |
| 46 | 47 | `MOB_BLOCK_S` | Mob-Block-South | Blocks southward departure through the current room. |
| 47 | 48 | `MOB_BLOCK_W` | Mob-Block-West | Blocks westward departure through the current room. |
| 48 | 49 | `MOB_BLOCK_NE` | Mob-Block-NE | Blocks northeastward departure through the current room. |
| 49 | 50 | `MOB_BLOCK_SE` | Mob-Block-SE | Blocks southeastward departure through the current room. |
| 50 | 51 | `MOB_BLOCK_SW` | Mob-Block-SW | Blocks southwestward departure through the current room. |
| 51 | 52 | `MOB_BLOCK_NW` | Mob-Block-NW | Blocks northwestward departure through the current room. |
| 52 | 53 | `MOB_BLOCK_U` | Mob-Block-Up | Blocks upward departure through the current room. |
| 53 | 54 | `MOB_BLOCK_D` | Mob-Block-Down | Blocks downward departure through the current room. |
| 54 | 55 | `MOB_BLOCK_CLASS` | Mob-Block-Class | Directional blocking exemption: travelers with the same primary class as the blocker pass. |
| 55 | 56 | `MOB_BLOCK_RACE` | Mob-Block-Race | Directional blocking exemption: travelers whose GET_RACE value matches the blocker pass. Not a free-form allowed-race list. |
| 56 | 57 | `MOB_BLOCK_LEVEL` | Mob-Block-Level | Directional blocking exemption: travelers strictly higher level than the blocker pass; equal level is not sufficient. |
| 57 | 58 | `MOB_BLOCK_ALIGN` | Mob-Block-Alignment | Directional blocking modifier using the current good/evil/neutral boolean comparisons; a different alignment category can clear the block. See blocking notes. |
| 58 | 59 | `MOB_BLOCK_ETHOS` | Mob-Block-Ethos | Declared ethos-based blocking modifier. No active direct consumer found; do not assume lawful/chaotic filtering works from this bit. |
| 59 | 60 | `MOB_INFO_KILL_PLR` | Info-Kill-of-Player | Broadcast/info handling when this NPC kills a player, through the corresponding death-message path. |
| 60 | 61 | `MOB_MOB_ASSIST` | Mob-Assist | Enables supported NPC group/follower assistance. The explicit mob-to-mob path requires flags on both allies and excludes pets/charmed helpers; grouping alone is insufficient. |
| 61 | 62 | `MOB_NOCONFUSE` | No-Confuse | Confusion immunity checked by supported confusion effects, including relevant bombs/special abilities. |
| 62 | 63 | `MOB_HUNTS_TARGET` | Hunts-Target | Active hunt/mission target marker used by hunt lifecycle and combat checks. Do not assign casually to ordinary NPCs or expect the whole hunt system to be created. |
| 63 | 64 | `MOB_ABIL_GRAPPLE` | Ability-Grapple | Declared Ability-Grapple capability; no active direct consumer of this flag found. The generic ability path is unfinished; configure a working racial, spell, feat or script behavior instead. |
| 64 | 65 | `MOB_ABIL_PETRIFY` | Ability-Petrify | Declared Ability-Petrify capability; no active direct consumer of this flag found. The generic ability path is unfinished; configure a working racial, spell, feat or script behavior instead. |
| 65 | 66 | `MOB_ABIL_TAIL_SPIKES` | Ability-Tail-Spikes | Declared Ability-Tail-Spikes capability; no active direct consumer of this flag found. The generic ability path is unfinished; configure a working racial, spell, feat or script behavior instead. |
| 66 | 67 | `MOB_ABIL_LEVEL_DRAIN` | Ability-Level-Drain | Declared Ability-Level-Drain capability; no active direct consumer of this flag found. The generic ability path is unfinished; configure a working racial, spell, feat or script behavior instead. |
| 67 | 68 | `MOB_ABIL_CHARM` | Ability-Charm | Declared Ability-Charm capability; no active direct consumer of this flag found. The generic ability path is unfinished; configure a working racial, spell, feat or script behavior instead. |
| 68 | 69 | `MOB_ABIL_BLINK` | Ability-Blink | Declared Ability-Blink capability; no active direct consumer of this flag found. The generic ability path is unfinished; configure a working racial, spell, feat or script behavior instead. |
| 69 | 70 | `MOB_ABIL_ENGULF` | Ability-Engulf | Declared Ability-Engulf capability; no active direct consumer of this flag found. The generic ability path is unfinished; configure a working racial, spell, feat or script behavior instead. |
| 70 | 71 | `MOB_ABIL_CAUSE_FEAR` | Ability-Cause-Fear | Declared Ability-Cause-Fear capability; no active direct consumer of this flag found. The generic ability path is unfinished; configure a working racial, spell, feat or script behavior instead. |
| 71 | 72 | `MOB_ABIL_CORRUPTION` | Ability-Corruption | Corruption on-hit capability: an explicit combat check can trigger the corruption spell on a 1-in-5 roll. Uses normal spell/target handling. |
| 72 | 73 | `MOB_ABIL_SWALLOW` | Ability-Swallow | Declared Ability-Swallow capability; no active direct consumer of this flag found. The generic ability path is unfinished; configure a working racial, spell, feat or script behavior instead. |
| 73 | 74 | `MOB_ABIL_FLIGHT` | Ability-Flight | Declared Ability-Flight capability; no active direct consumer of this flag found. The generic ability path is unfinished; configure a working racial, spell, feat or script behavior instead. |
| 74 | 75 | `MOB_ABIL_POISON` | Ability-Poison | Poison on-hit capability: an explicit combat check invokes the NPC poison effect. Not a complete definition of poison level, damage or saves. |
| 75 | 76 | `MOB_ABIL_REGENERATION` | Ability-Regenerate | Declared Ability-Regenerate capability; no active direct consumer of this flag found. The generic ability path is unfinished; configure a working racial, spell, feat or script behavior instead. |
| 76 | 77 | `MOB_ABIL_PARALYZE` | Ability-Paralyze | Declared Ability-Paralyze capability; no active direct consumer of this flag found. The generic ability path is unfinished; configure a working racial, spell, feat or script behavior instead. |
| 77 | 78 | `MOB_ABIL_FIRE_BREATH` | Ability-Fire-Breath | Declared Ability-Fire-Breath capability; no active direct consumer of this flag found. The generic ability path is unfinished; configure a working racial, spell, feat or script behavior instead. |
| 78 | 79 | `MOB_ABIL_LIGHTNING_BREATH` | Ability-Lightning-Breath | Declared Ability-Lightning-Breath capability; no active direct consumer of this flag found. The generic ability path is unfinished; configure a working racial, spell, feat or script behavior instead. |
| 79 | 80 | `MOB_ABIL_POISON_BREATH` | Ability-Poison-Breath | Declared Ability-Poison-Breath capability; no active direct consumer of this flag found. The generic ability path is unfinished; configure a working racial, spell, feat or script behavior instead. |
| 80 | 81 | `MOB_ABIL_ACID_BREATH` | Ability-Acid-Breath | Declared Ability-Acid-Breath capability; no active direct consumer of this flag found. The generic ability path is unfinished; configure a working racial, spell, feat or script behavior instead. |
| 81 | 82 | `MOB_ABIL_FROST_BREATH` | Ability-Frost-Breath | Declared Ability-Frost-Breath capability; no active direct consumer of this flag found. The generic ability path is unfinished; configure a working racial, spell, feat or script behavior instead. |
| 82 | 83 | `MOB_ABIL_MAGIC_IMMUNITY` | Ability-Magic-Immunity | Declared Ability-Magic-Immunity capability; no active direct consumer of this flag found. The generic ability path is unfinished; configure a working racial, spell, feat or script behavior instead. |
| 83 | 84 | `MOB_ABIL_INVISIBILITY` | Ability-Invisibility | Declared Ability-Invisibility capability; no active direct consumer of this flag found. The generic ability path is unfinished; configure a working racial, spell, feat or script behavior instead. |
| 84 | 85 | `MOB_C_O_T_N` | Child-of-the-Night | Child of the Night companion role marker; needs its parent summon/control system. |
| 85 | 86 | `MOB_VAMP_SPWN` | Vampire-Spawn | Vampire-spawn role marker; not the same as giving every vampire race/feat mechanic. |
| 86 | 87 | `MOB_DRAGON_KNIGHT` | Dragon-Knight | Epic Dragon Knight summon role marker. The epic-summon system uses it; do not apply as a generic dragon subtype. |
| 87 | 88 | `MOB_MUMMY_DUST` | Mummy-Dust | Epic Mummy Dust summon role marker. Needs its summon setup and owner. |
| 88 | 89 | `MOB_EIDOLON` | Eidolon | Eidolon role marker. Needs the summoner/eidolon system, ownership and evolution data. |
| 89 | 90 | `MOB_BLOCK_EVIL` | Mob-Block-Evil | With a directional block flag, blocks evil travelers while neutral/good travelers are exempt. |
| 90 | 91 | `MOB_BLOCK_NEUTRAL` | Mob-Block-Neutral | With a directional block flag, blocks neutral travelers while evil/good travelers are exempt. |
| 91 | 92 | `MOB_BLOCK_GOOD` | Mob-Block-Good | With a directional block flag, blocks good travelers while neutral/evil travelers are exempt. |
| 92 | 93 | `MOB_GENIEKIND` | Mob-Geniekind | Geniekind role marker used by the corresponding transformation/summon system. |
| 93 | 94 | `MOB_C_DRAGON` | Mob-Dragon-Mount | Dragon-mount role marker. Does not create a dragon rider bond, owner or flight state by itself. |
| 94 | 95 | `MOB_RETAINER` | Retainer | Retainer companion role marker used by retainer handling. Does not create a service contract by itself. |
| 95 | 96 | `MOB_BUFF_OUTSIDE_COMBAT` | UNUSED-96 | Explicitly unused legacy flag retained for compatibility. Does not enable out-of-combat buffing. |
| 96 | 97 | `MOB_NOPARALYZE` | Immune-Paralysis | Paralysis immunity in supported immunity helpers. |
| 97 | 98 | `MOB_AI_ENABLED` | AI-Enabled | Conversational AI integration for supported ask/tell interactions. Requires configured backend/service; separate from ordinary combat AI. |
| 98 | 99 | `MOB_QUARTERMASTER` | Quartermaster | Quartermaster marker used by the supply-order system. Does not define orders or guarantee every crafting workflow is supported. |
| 99 | 100 | `MOB_UNLIMITED_SPELL_SLOTS` | Unlimited-Spell-Slots | Bypasses supported NPC spell-slot availability/consumption checks. Does not teach spells or bypass action/casting restrictions. |
| 100 | 101 | `MOB_CUSTOM_MOB_STATS` | Custom-Mob-Stats | Skips the global category/level-range stat-percentage pass. Does not disable the separate campaign autoroll condition. |
| 101 | 102 | `MOB_NO_BLOCK_BYPASS` | No-Block-Bypass | Prevents the supported Ghost-perk bypass of directional mob blocking. Normal class/race/level/alignment exemptions still apply. |
| 102 | 103 | `MOB_GOLEM` | Golem | Constructed-golem follower marker used by golem/companion tracking and repair-related systems. Not the same as race Construct alone. |
| 103 | 104 | `MOB_NOTELEPORT` | No-Teleport | No-teleport restriction in supported NPC teleport/travel checks. Not a ban on walking or every scripted relocation. |
| 104 | 105 | `MOB_SUMMON_SOLAR` | Summon-Solar | Epic Solar summon role marker, used by epic-summon behavior and ownership checks. |
| 105 | 106 | `MOB_FLIGHTLESS` | Flightless | Suppresses intrinsic dragon flight in the is_flying helper. Does not categorically cancel all separate magical flight sources. |

Role flags are not complete companion configurations. A familiar, mount, golem,
eidolon or epic summon normally needs a master relationship, creation path,
runtime state and possibly persisted data. Avoid borrowing those flags just to
obtain one side effect on an unrelated encounter mob.

## Blocking, grouping and assistance

### Directional blockers

A blocker must have at least one direction flag and be in the room the traveler
is **leaving**. Merely giving it a Block-Class or Block-Level flag does not
select an exit.

The current checks clear an established block when:

- Block-Race is enabled and traveler/blocker race values match.
- Block-Class is enabled and primary classes match.
- Block-Level is enabled and the traveler is strictly higher level.
- Block-Alignment's current category comparisons allow the traveler through.
- Block-Evil/Neutral/Good is enabled and the traveler is outside that blocked
  category.
- A supported Ghost-perk bypass succeeds, unless No-Block-Bypass is set.

These exemptions are sequential **ways to pass**, not necessarily a requirement
to satisfy all selected flags simultaneously. Combining several filters can
therefore admit more travelers than expected. Test the exact combinations.
Block-Ethos currently has no active direct implementation. Staff/no-hassle and
NPC handling can differ from ordinary mortal movement.

### Grouping and helping

`AFF_GROUP` is an on-load NPC grouping marker. Compatible flagged NPCs loading
in the same room can acquire actual group/master relationships through
`set_mob_grouping()`. The bit alone is not the player's normal group state.

Helper and Guard participate in assistance selection; Guard recognizes Citizen.
The explicit Mob-Assist path checks real group or follower relationships,
requires Mob-Assist on both NPCs, excludes pet/charmed-helper cases, and does
not indiscriminately assist attacks against other NPCs. Mission ownership can
also prevent assistance. Test the master/follower situation you actually use.

## AFF and AFF2 settings

Main-menu `B` edits first-bank character affects; `U` edits second-bank affects.
Both use the **direct displayed index**, unlike the filtered NPC flag menu.
Index 0 returns. Do not type the count values 127/3 as additional flag choices.

MEDIT strips Charm, Poison, Acid Coat and Sleep after first-bank menu toggles.
Use their source abilities/scripts rather than trying to force those prototype
states. Other flags may require image counts, wards, events, durations or master
relationships. Hiding and invisibility are character conditions here, distinct
from NPC behavior flags.

Direct live AFF2 flags have real consumers, but the current common timed-affect
and equipment handler does not merge stored second-bank vectors into live flags.
See the [AFF/AFF2 guide](AFFECT_FLAGS_GUIDE.md) for all flag purposes, dependencies
and current implementation limitations.

## Plot NPC shortcuts

### M: plot NPC flags and settings

This is a broad preset, not a single harmless toggle. It sets:

- Humanoid family, Warrior class, level 10, then auto-generated stats.
- Medium size.
- Sentient, Sentinel and Aware.
- No Charm, Summon, Sleep, Bash, Blindness, Deafness, Grapple, Steal and Confuse.
- Unkillable, Does-Not-Fight and No-AI.

It adds those flags; it does not necessarily clear every previously assigned
aggression, role or special flag. Review the complete result. No-AI also skips
main-loop ambient echoes and autonomous routines, so do not assume a plot NPC
preset will still perform all ordinary idle behavior.

This does **not** register a shop, quest master, auction house, player stall,
quartermaster inventory or quest chain. Those require their respective records
and systems. Use the service's editor/configuration separately.

### O: random descriptions

This replaces sex, aliases/name, short description, room description and detailed
text using random name/description tables. It does not change the actual race
field to match every randomly chosen descriptive term. Review flavor against
the configured race, class and role, especially if you previously wrote a custom
identity.

## Echoes and movement messages

### J/K: Walk-In and Walk-Out

These are action-message prefixes with the direction appended by movement:

| Field | Example input | Intended result |
| --- | --- | --- |
| Walk-In | `$n stomps in from` | The monster stomps in from the north. |
| Walk-Out | `$n stomps` | The monster stomps north. |

Use `$n` for the NPC's name where action formatting applies. Blank input restores
default movement messaging. Do not append a fixed compass direction if the same
NPC can travel in several directions.

### L: ambient echoes

| Choice | Action |
| --- | --- |
| `A` | Add text; up to 20 echoes. |
| `D` | Delete a numbered entry. |
| `E` | Edit an entry; blank edited text removes it. |
| `F` | Set the frequency field, clamped 0–100. |
| `T` | Toggle random/sequential selection. |
| `Z` | Toggle room/zone scope; requires `LVL_STAFF` or higher. |
| `Q` | Return to main menu. |

Echo text is single-line input. Room echoes use action formatting. Zone echoes
are sent as raw text to eligible awake, non-writing players in the zone; do not
assume room-style substitutions behave identically in a zone message.

The frequency label says percent, but current runtime selection is:

```
roll 1..75 <= integer(frequency / 4)
```

Thus 100 gives about a one-third chance per eligible echo check, not guaranteed
every check; values below 4 never succeed in this check. Check frequency also
depends on the mobile activity loop and its earlier conditions. No-AI skips that
loop, so enabling echoes alone does not make a No-AI NPC broadcast them.

## Patrol paths

`V) Path Edit` starts by requesting a delay count, then accepts room vnums one
at a time. Enter **0** to terminate the route. Starting this editor clears the
old route immediately in the edit buffer. With `MAX_PATH` currently 50 and the
parser limit `MAX_PATH - 1`, it accepts up to **49** waypoints.

The runtime delay is an activity counter, not a number of real seconds. On an
eligible path check, it counts down; when ready, the NPC seeks a first step
toward the next waypoint. The path index advances after the attempt, rather
than waiting indefinitely until a distant waypoint is reached.

Use nearby, connected waypoints and verify the whole loop, including the last
back to the first. Destination vnums are not fully validated by the input loop.
Invalid/unreachable waypoints are unsafe: the legacy path implementation assumes
valid path results in some code before its error handling. Do not use missing
rooms as deliberate path markers.

Sentinel only controls ordinary random wandering; it is not a guarantee that
patrol code will be disabled. No-AI skips the main activity loop that drives
patrols. Combat, position, room exits, closed doors and other movement checks
can interrupt travel.

## Scripts, special procedures and spawning

`S) Script` attaches mobile DG triggers. Choose triggers intended for mobiles,
with valid owner/actor variables. Greeting, speech, fight and other trigger
semantics are different; attaching a trigger does not replace its conditions
and commands.

`Z) SpecProc` selects one registered C special procedure. Choose `0` to clear
or `Q` to return. Periodic/combat procedure calls can also require `MOB_SPEC`;
a flag with no callable procedure does nothing. Command handlers and script
triggers have their own dispatch rules. No-AI can skip main-loop proc dispatch.

Creating a mobile prototype does not spawn it or equip it. Use a controlled
staff load to test, then define normal room population/equipment through zone
resets and OEDIT item prototypes. Shop inventory uses shop configuration; quest
roles use quest data. A protected plot preset is not a complete service setup.

MEDIT saving refreshes scripts on existing copies, but full stat/flag testing
should use a newly loaded instance. Editing a commonly used NPC prototype can
also change its displayed identity or script behavior on already loaded copies.

## Building workflow and common mistakes

### Example: a patrolling city guard

1. Create an unused mobile vnum in your assigned zone.
2. Write guard aliases and all descriptions.
3. Choose the appropriate race family/subtypes, Warrior class, explicit size
   and intended level.
4. Use Auto Set Stats, then tune HP, AC, attack, damage, alignment and rewards.
   Decide whether the load-time custom flags are needed for your tuning.
5. Add Guard and the intended assistance/grouping flags. Use Citizen on the
   NPCs the guard should protect. Do not automatically use the plot preset if
   the guard should fight or perform ordinary AI behavior.
6. Add any explicit class feats once, review them, and assign needed spells
   or scripts separately.
7. Enter a short connected patrol route and an appropriate delay count.
8. Save the prototype and its zone to disk. Load a fresh test copy and test
   patrol, assistance, target selection, rewards and combat with mortals.
9. Add zone resets/equipment only after the prototype behaves correctly.

### Common mistakes

- Using code index + 1 for every NPC flag. The menu omits two internal flags.
- Treating A) NPC Flags as the AFF flag list, or expecting “Hiding” there.
- Setting level/class/race after autoroll and expecting all stats to refresh.
- Assuming manual stat input, custom gold and custom mob stats all control the
  same load-time behavior.
- Reading HP SizeDice as live PSP or HP Addition as live movement points.
- Reading old save labels as the current saving-throw category names.
- Treating resistance percentages as flat damage reduction.
- Expecting every declared Ability flag to cause an automatic attack.
- Expecting a dragon's descriptive name to choose its breath element.
- Expecting No-AI and AI-Enabled to be opposites of the same system.
- Repeatedly using `addclassfeats` without checking accumulated ranks.
- Assuming cleared explicit spells remove every class spell.
- Assuming group membership alone guarantees pet/NPC assistance.
- Expecting a plot preset to create shop or quest records.
- Expecting an echo frequency of 100 to mean one echo every check.
- Entering -1 for size and assuming it preserves the displayed default option.
- Testing only an existing NPC after changing its prototype stats.
- Saving descriptions but forgetting the prototype/zone disk save.

## Source references

- [medit.c](../../src/medit.c): syntax, menus, parser, filtered flag numbering,
  stats, autoroll, echoes, patrol entry and script updates.
- [structs.h](../../src/structs.h): mobile flags, field storage, enums and limits.
- [constants.c](../../src/constants.c): mobile flag labels, race families and menus.
- [oasis.h](../../src/oasis.h): editor bounds.
- [genmob.c](../../src/genmob.c): prototype saving, live string updates and deletion.
- [db.c](../../src/db.c): mobile parsing, loading, HP generation and slot initialization.
- [utils.c](../../src/utils.c): group setup, stat-category modifiers, immunities and flight.
- [mob_act.c](../../src/mob_act.c): activity loop, aggression, helpers, guards and assistance.
- [mob_utils.c](../../src/mob_utils.c): patrol movement and echo selection/delivery.
- [mob_class.c](../../src/mob_class.c): class behavior and unfinished generic ability path.
- [mob_race.c](../../src/mob_race.c): racial combat behavior.
- [mob_spells.c](../../src/mob_spells.c), [mob_known_spells.c](../../src/mob_known_spells.c)
  and [mob_spellslots.c](../../src/mob_spellslots.c): spell selection and resources.
- [fight.c](../../src/fight.c): damage, resistance percentages, defenses and on-hit abilities.
- [movement.c](../../src/movement.c): directional NPC blockers and exemptions.
- [cedit.c](../../src/cedit.c): global NPC stat configuration.
