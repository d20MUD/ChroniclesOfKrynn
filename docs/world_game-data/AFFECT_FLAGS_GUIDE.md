# Affect flags: AFF_ and AFF2_ builder reference

Source reviewed: 2026-10-23. This guide covers every currently defined numeric
flag in both character-affect banks: **127 AFF entries (0–126)** and **3 AFF2
entries (0–2)**. Each bank includes its reserved index 0. The usable ranges
are AFF 1–126 and AFF2 1–2.

For related settings, see the [OEDIT guide](OEDIT_GUIDE.md) and
[REDIT guide](REDIT_GUIDE.md). This document describes current source behavior;
a flag's name or display text can describe an intended effect that needs more
implementation or supporting data.

## Contents

- [What an affect flag does](#what-an-affect-flag-does)
- [Banks and menu numbers](#banks-and-menu-numbers)
- [Builder use and restrictions](#builder-use-and-restrictions)
- [Complete AFF reference](#complete-aff-reference)
- [Complete AFF2 reference](#complete-aff2-reference)
- [Important interactions](#important-interactions)
- [Current implementation limitations](#current-implementation-limitations)
- [Verification workflow](#verification-workflow)
- [Source references](#source-references)

## What an affect flag does

An affect flag is a character-state bit checked by another subsystem. It is
not automatically a spell, a duration, a numeric bonus, or a complete behavior.

Three different pieces commonly work together:

| Data | Purpose | Example |
| --- | --- | --- |
| Affect flag | A condition or capability is active. | `AFF_MIRROR_IMAGED` identifies image-related state. |
| Timed affect record | Stores source spell/ability ID, duration, modifier, apply location, bonus type and flag vectors. | Mirror Image's actual spell record is checked by combat. |
| Additional runtime state | Counters, events, ownership links or other mechanics. | Image count, Spell Mantle charges, a charm master, or a stun event. |

Setting a flag does not execute the spell that normally sets it. For example:

- `AFF_MIRROR_IMAGED` does not create image counters or the required spell record.
- `AFF_SPELL_MANTLE` does not fill remaining mantle charges.
- `AFF_WARDED` does not create a damage-absorption reserve.
- `AFF_CHARM` does not create a master/follower relationship.
- `AFF_WILD_SHAPE` does not perform a character transformation.
- `AFF_STUN` is recognized by combat, but durations/events and recovery still
  need a valid source or cleanup path.

Likewise, an affect can contain an `APPLY_*` modifier without any status flag.
Do not assume every numeric spell bonus has a matching AFF bit.

## Banks and menu numbers

| Bank | Character storage | Record field | Check |
| --- | --- | --- | --- |
| AFF | `char_specials.saved.affected_by` | `affected_type.bitvector` | `AFF_FLAGGED(ch, AFF_...)` |
| AFF2 | `char_specials.saved.affected2_by` | `affected_type.bitvector2` | `AFF2_FLAGGED(ch, AFF2_...)` |

The banks are independent. **AFF2 is not the upper half of AFF**, and its
indices do not continue at 127. For example:

- AFF 1 is Blind.
- AFF2 1 is Magic Attacks.
- AFF2 2 is Cowering.

Use the matching macro and bank. Testing an AFF2 constant with `AFF_FLAGGED`
checks the wrong state. Both banks use array-bit operations (`SET_BIT_AR`,
`REMOVE_BIT_AR`, `IS_SET_AR`), not a single integer mask.

In OEDIT's `P) Perm Affects` and `V) Perm2 Affects` menus, the displayed number
is the **actual flag index**. `0` leaves the menu and is not a useful effect.
MEDIT's affect menus also use the displayed direct flag index. This differs
from OEDIT extra/wear flags and REDIT room flags, which generally use code
index + 1.

Only use valid displayed entries. The AFF/AFF2 menu parsers have inclusive
upper-bound checks in some paths, although `NUM_AFF_FLAGS` and
`NUM_AFF2_FLAGS` are counts, not extra valid flags. Do not enter 127 or 3 as
additional AFF/AFF2 choices.

`AFF_WATER_BREATH` is an alias for `AFF_SCUBA`, index **18**. They are the same
bit, not two separate effects.

## Builder use and restrictions

### Mobile affects

MEDIT's affect menus set prototype character-state flags. Use these for
supported innate capabilities or carefully chosen passive states. The current
MEDIT parser immediately removes **Charm, Poison, Acid Coat and Sleep** from
the first bank after an affect-menu toggle. Use the corresponding spells,
abilities or scripts to establish those conditions properly.

The source also marks Blind, Group, Poison, Sleep and Acid Coat as reserved
or runtime-oriented in its comments. Some older conventions differ from actual
usage: `AFF_GROUP`, for example, is used by NPC on-load grouping. Review the
actual handler rather than treating the comment as the complete mechanic.

### Object permanent affects

OEDIT `P` stores original-bank flags on an object's permanent affect vector.
Equipment recalculation applies the supported first-bank bits while the item
is equipped. This is distinct from an item being invisible or magical itself
through `ITEM_*` extra flags. OEDIT explicitly refuses `AFF_CHARM` here.

OEDIT `V` stores second-bank flags, but the current common equipment/affect
handler does not merge those AFF2 vectors into the wearer's live flags. See
[implementation limitations](#current-implementation-limitations) before
relying on AFF2 equipment or timed effects.

Permanent flags have no ordinary spell expiration by themselves. Combat modes,
one-shot effects, control conditions and linked-resource defenses are poor
choices for permanent equipment unless their full behavior and removal have
been designed and tested.

### Reference categories

| Category | Meaning |
| --- | --- |
| Passive | Has a direct capability/defense check, though normal spell behavior may add more data. |
| Condition | A harmful or restrictive state. Apply through the proper ability and recovery path. |
| Mode | Combat/action mode that normally uses a command, feat, class state or resource rules. |
| Linked | Needs source records, counters, events, relationships or additional numeric modifiers. |
| Special | Specialized subsystem, mobile, perk or scripted behavior. |
| Legacy | No active direct flag consumer found, or only limited marker/display use. |
| Reserved | Sentinel/internal entry that should not be assigned as gameplay equipment. |

These labels describe safe expectations, not an editor permission system.

## Complete AFF reference

The **index is the displayed choice** in the relevant affect menu. Index 0 is
reserved and exits the menu. Labels come from `affected_bits[]`; they can differ
from spell names or source constants.

| Index / choice | Flag | Display label | Category | Purpose and current behavior |
| --- | --- | --- | --- | --- |
| 0 | `AFF_DONTUSE` | (reserved) | Reserved | Do not use. Index 0 is a sentinel, not a usable character effect. |
| 1 | `AFF_BLIND` | Blinded | Condition | Blindness; affects vision, combat and movement checks. Some special senses bypass particular penalties. Use a source with proper duration/removal. |
| 2 | `AFF_INVISIBLE` | Invisible | Passive | Character invisibility. Visibility still depends on observer detection and effects that reveal invisible targets. |
| 3 | `AFF_DETECT_ALIGN` | Detect-Alignment | Passive | Detect-alignment capability used by alignment-sensitive displays. Does not set the target's alignment or override every concealment. |
| 4 | `AFF_DETECT_INVIS` | Detect-Invisible | Passive | See invisible creatures through supported detection checks. Not the same as finding hiding/sneaking creatures. |
| 5 | `AFF_DETECT_MAGIC` | Detect-Magic | Passive | Detect magical properties/auras in supported displays; does not identify every item statistic. |
| 6 | `AFF_SENSE_LIFE` | Sense-Life | Passive | Sense-life presence detection in supported room displays, including some situations where ordinary vision is unavailable. Does not guarantee full identification. |
| 7 | `AFF_WATERWALK` | Water-Walk | Passive | Water-walking capability accepted by water-travel checks; does not provide underwater breathing. |
| 8 | `AFF_SANCTUARY` | Sanctuary | Legacy | Sanctuary aura/display marker, set by Sanctuary. No direct combat damage-reduction or attack-prevention check of this bit was found; do not assume tabletop protection or half damage. |
| 9 | `AFF_GROUP` | OnLoad MobGroup | Special | NPC on-load grouping marker, displayed as OnLoad MobGroup. Used to group suitable NPCs; not a substitute for the normal player group relationship. |
| 10 | `AFF_CURSE` | Cursed | Linked | Cursed-state marker used by curse/removal handling. Any numeric attack/save penalties must come from associated affects, not the bit alone. |
| 11 | `AFF_INFRAVISION` | Infra-Vision | Passive | Infravision capability. Vision helpers also consider racial/feat senses and special room lighting rules. |
| 12 | `AFF_POISON` | Poison | Condition | Poisoned state used by periodic poison and recovery checks. Poison type, spell record, damage and duration depend on the source; MEDIT strips this bit. |
| 13 | `AFF_PROTECT_EVIL` | Protect-Evil | Passive | Protection-from-evil checks, including applicable alignment-specific combat and control interactions. Not blanket immunity to evil creatures. |
| 14 | `AFF_PROTECT_GOOD` | Protect-Good | Passive | Protection-from-good counterpart, with applicable good-opponent combat/control checks. |
| 15 | `AFF_SLEEP` | Sleep | Condition | Magical sleep state used by wake/position and combat checks. A proper sleep source also manages position and duration; MEDIT strips this bit. |
| 16 | `AFF_NOTRACK` | Not-Trackable | Passive | No-track capability checked by tracking paths; not immunity to all perception, scrying or scripted discovery. |
| 17 | `AFF_FLYING` | Flying | Passive | Active flying state used by flight, falling, movement and speed checks. Subject to no-fly rooms and other restrictions. |
| 18 | `AFF_SCUBA` | Underwater-Breathing | Passive | Scuba/water-breathing capability used by underwater entry and drowning checks. Also named AFF_WATER_BREATH. |
| 19 | `AFF_SNEAK` | Sneaking | Passive | Sneaking state for quiet movement and detection checks. Does not automatically confer Hide or perfect stealth. |
| 20 | `AFF_HIDE` | Hiding | Passive | Hidden-character state used by visibility/search logic. Detection depends on observer checks and circumstances; not object invisibility. |
| 21 | `AFF_VAMPIRIC_CURSE` | Vampiric-Curse | Condition | Vampiric curse is on the VICTIM: hitting a living cursed victim can heal the attacker for 1d10, capped by missing HP. Not a general life-steal buff on the wielder. |
| 22 | `AFF_CHARM` | Charmed | Linked | Charm/control state. Needs master/follower and related ownership data. MEDIT removes it, and OEDIT refuses it as a permanent affect. |
| 23 | `AFF_BLUR` | Blurred | Passive | Blur contributes 20% concealment in the applicable concealment chain. It does not simply add on top of every stronger concealment effect. |
| 24 | `AFF_POWER_ATTACK` | Mode-PowerAttack | Mode | Power Attack mode; actual attack/damage tradeoff follows the combat mode and feat rules. |
| 25 | `AFF_EXPERTISE` | Mode-Expertise | Mode | Combat Expertise mode; actual defensive/attack tradeoff follows its combat rules. |
| 26 | `AFF_HASTE` | Hasted | Passive | Haste state used by extra-attack and speed calculations; speed gains and additional attacks have their own stacking/eligibility rules. |
| 27 | `AFF_TOTAL_DEFENSE` | Mode-Total-Defense | Mode | Total Defense mode, consumed by the relevant defensive combat/action checks. |
| 28 | `AFF_ELEMENT_PROT` | Protect-Elements | Passive | Elemental-protection state used by environmental checks, including lava hazard protection. Spell-specific resistances may need separate numeric affects. |
| 29 | `AFF_DEAF` | Deaf | Condition | Deafened state; blocks hearing/communication paths that check it. Not a room soundproof flag. |
| 30 | `AFF_FEAR` | Fear | Condition | Frightened/fear state; affects supported combat behavior, fleeing and fear-related checks. Bravery and other immunities can counter it. |
| 31 | `AFF_STUN` | Stunned | Condition | Stun state recognized by combat/incapacitation checks. Separate stun events can also exist; use proper duration and action cleanup. |
| 32 | `AFF_PARALYZED` | Paralyzed | Condition | Paralysis state recognized by action/incapacitation checks. Does not create a complete timed paralysis spell by itself. |
| 33 | `AFF_ULTRAVISION` | Ultra-Vision | Passive | Ultravision capability in the dark-vision helper. Very bright or magically obscured rooms still have separate checks. |
| 34 | `AFF_GRAPPLED` | Grappled | Linked | Grappled state blocks ordinary room movement and participates in maneuver restrictions. Use the grapple system to establish opponents and escape state. |
| 35 | `AFF_TAMED` | Tamed | Linked | Tamed creature state used by mount/control movement logic. Does not create a mount master or a complete riding relationship. |
| 36 | `AFF_CLIMB` | Climb | Passive | Climbing capability accepted by the climbing helper; not a numeric Climb skill-rank bonus. |
| 37 | `AFF_NAUSEATED` | Nauseated | Condition | Nauseated state affects physical actions, NPC behavior and spellcasting checks. Exact restrictions depend on the action path. |
| 38 | `AFF_NON_DETECTION` | Non-Detection | Passive | Nondetection state used by supported scrying checks; not universal invisibility or untrackability. |
| 39 | `AFF_SLOW` | Slowed | Condition | Slow affects speed and attack routines. It is distinct from fatigue, entanglement and a spell's numeric penalties. |
| 40 | `AFF_FSHIELD` | Fire-Shielded | Passive | Fire-shield state used by supported retaliation and fire-related defense checks. Retaliation conditions and damage are handled in combat. |
| 41 | `AFF_CSHIELD` | Cold-Shielded | Passive | Cold-shield state used by supported retaliation checks; not an arbitrary flat damage-reduction value. |
| 42 | `AFF_MINOR_GLOBE` | Minor-Globed | Passive | Minor Globe rejects eligible hostile/damaging spells using the low spell-level cutoff. Device/warlock exclusions and actual cutoff are described below. |
| 43 | `AFF_ASHIELD` | Acid-Shielded | Passive | Acid-shield state used by supported retaliation checks. Does not automatically apply an acid-damage weapon bonus. |
| 44 | `AFF_SIZECHANGED` | Size-Changed | Legacy | Intended unusual-size marker. No active direct flag use found; actual size changes require size data/APPLY_SIZE and the relevant source. |
| 45 | `AFF_TRUE_SIGHT` | True-Sight | Passive | True-sight detection capability used by vision/illusion helpers. Does not universally bypass every room or scripted visibility restriction. |
| 46 | `AFF_SPOT` | Spot-Mode | Mode | Spot/search-awareness mode; movement cost doubles while active. Detection still uses the relevant perception mechanics. |
| 47 | `AFF_FATIGUED` | Fatigued | Condition | Fatigued state; directly penalizes applicable attack calculations and participates in fatigue rules. Do not assume the complete tabletop condition is supplied by the bit alone. |
| 48 | `AFF_REGEN` | Regenerating | Passive | Regeneration flag doubles the natural HP regeneration calculation, with a minimum before doubling. Blackmantle/no-heal can still stop regeneration. |
| 49 | `AFF_DISEASE` | Diseased | Condition | Disease state checked by periodic disease/recovery logic. The disease source provides its actual severity, penalties and duration. |
| 50 | `AFF_TFORM` | T-Transform | Linked | Tenser's Transformation state. Class/combat and casting checks recognize it; spell-provided numeric changes are separate. |
| 51 | `AFF_GLOBE_OF_INVULN` | Globe-of-Invuln | Passive | Globe of Invulnerability rejects eligible hostile/damaging spells at the higher cutoff. It is not immunity to all spell levels or devices. |
| 52 | `AFF_LISTEN` | Listen-Mode | Mode | Listen awareness mode; movement cost doubles while active. Combines multiplicatively with Spot cost. |
| 53 | `AFF_DISPLACE` | Displaced | Passive | Displacement supplies 50% concealment in the supported concealment chain; other detection and concealment rules still apply. |
| 54 | `AFF_SPELL_MANTLE` | Spell-Mantled | Linked | Spell Mantle absorption requires a positive GET_SPELL_MANTLE counter. The spell initializes charges; the bit alone does not. |
| 55 | `AFF_CONFUSED` | Confused | Linked | Confusion state used by combat behavior. Normal application can track a confuser/source; duration and behavior require the appropriate affect setup. |
| 56 | `AFF_REFUGE` | Refuged | Special | Refuge marker used by a specialized room procedure. Not a universal invisibility or peaceful-room effect. |
| 57 | `AFF_SPELL_TURNING` | Spell-Turning | Passive | Spell Turning reflects an eligible hostile/damaging targeted spell in the resolution path, then consumes the bit. Device handling is excluded there. |
| 58 | `AFF_MIND_BLANK` | Mind-Blanked | Passive | Mind Blank is checked by mental/mind-affecting immunity and certain spell targeting logic; not a blanket block against all magic. |
| 59 | `AFF_SHADOW_SHIELD` | Shadow-Shielded | Legacy | Shadow Shield flag is declared, but direct checks in the damage code are commented out. The actual spell can have other records/modifiers; this bit alone does not supply them. |
| 60 | `AFF_TIME_STOPPED` | Time-Stopped | Linked | Time Stop affects the supported casting-action and preparation-time paths. Not a global freeze of every other creature; apply via the spell. |
| 61 | `AFF_BRAVERY` | Bravery | Passive | Bravery is a recognized fear-immunity source. It does not grant unrelated morale bonuses by itself. |
| 62 | `AFF_FREE_MOVEMENT` | Free-Movement | Passive | Freedom of Movement is checked by supported movement-impairing/maneuver logic. Not a bypass of every exit, size, house or room restriction. |
| 63 | `AFF_FAERIE_FIRE` | Faerie-Fired | Passive | Faerie Fire marks/reveals the outlined character through supported visibility and concealment rules. Not fire damage. |
| 64 | `AFF_BATTLETIDE` | Battletide | Linked | Battletide state participates in transformation/casting restrictions. Normal spell bonuses are separate associated affects. |
| 65 | `AFF_SPELL_RESISTANT` | Spell-Resistant | Passive | Raises the SR candidate to at least 12 + character level in the resistance helper, before remaining modifier/cap processing. Separate from numeric APPLY_SPELL_RES. |
| 66 | `AFF_DIM_LOCK` | Dimensional-Locked | Passive | Dimensional lock state blocks supported dimensional travel when the destination helper is asked to honor that check. Not a room flag. |
| 67 | `AFF_DEATH_WARD` | Death-Ward | Passive | Death Ward is checked by supported death/negative-energy effects and immunity logic. Not invulnerability to every lethal attack. |
| 68 | `AFF_SPELLBATTLE` | Mode-Spellbattle | Mode | Arcana golem Spellbattle mode. Uses supporting class/event handling; do not assign as a generic passive enchantment. |
| 69 | `AFF_VAMPIRIC_TOUCH` | Vampiric-Touch | Linked | Next successful damaging attack against a living target heals the attacker up to damage dealt/missing HP and consumes the bit. Distinct from Vampiric Curse on a victim. |
| 70 | `AFF_BLACKMANTLE` | Blackmantled | Condition | Blackmantle stops natural HP regeneration and alters supported healing checks. Not equivalent to disabling all resources or all regeneration systems. |
| 71 | `AFF_DANGERSENSE` | Danger-Sense | Passive | Danger-sense state enables nearby-danger information in supported room displays. Does not reveal every hidden threat. |
| 72 | `AFF_SAFEFALL` | SafeFall | Passive | Safefall cancels landing damage in the falling event and consumes its bit. Not permanent free flight. |
| 73 | `AFF_TOWER_OF_IRON_WILL` | Tower-of-Iron-Will | Legacy | Tower of Iron Will flag is declared as unfinished; no active direct flag consumer found. Do not assume psionic damage reduction from the bit. |
| 74 | `AFF_INERTIAL_BARRIER` | Inertial-Barrier | Linked | Inertial Barrier can absorb eligible damage on its random check, spending PSP; depleted PSP removes it. Does not create a PSP reserve. |
| 75 | `AFF_NOTELEPORT` | Not-teleportable | Passive | No-teleport targeting state used by supported summon/teleport-to-target paths. Different from dimensional lock and ROOM_NOTELEPORT. |
| 76 | `AFF_MAX_DAMAGE` | DamageBoost | Legacy | Intended next-attack damage boost, displayed as DamageBoost. No active direct flag consumer found; does not maximize damage by itself. |
| 77 | `AFF_IMMATERIAL` | Incorporeal | Passive | Immaterial/incorporeal state used by supported physical-interaction and combat checks. Not immunity to every damage type or action. |
| 78 | `AFF_CAGE` | Caged | Legacy | Intended caged/non-interacting state. No active direct flag consumer found; do not rely on it to imprison a creature. |
| 79 | `AFF_MAGE_FLAME` | Mage-Flamed | Passive | Mage Flame state contributes light in character room-light handling. Does not equip a light object or set the room magical-light bit. |
| 80 | `AFF_DARKVISION` | Dark-Vision | Passive | Darkvision participates in vision helpers and brightness handling. Some magical darkness/fog paths perform independent checks. |
| 81 | `AFF_BODYWEAPONRY` | Body-Weaponry | Legacy | Intended body-as-weapon state. No active direct flag consumer found; does not grant martial arts damage automatically. |
| 82 | `AFF_FARSEE` | Farsee | Special | Farsee is set by a bardic effect, but no direct range-extension flag consumer was found. Do not assume scan range increases from the bit. |
| 83 | `AFF_MENZOCHOKER` | Menz-Choker | Special | Menzochoker item-synergy marker managed by a specific object procedure. Not a general equipment-set bonus configuration. |
| 84 | `AFF_RAPID_SHOT` | Mode-RapidShot | Mode | Rapid Shot mode; additional ranged attacks and penalties follow the feat/combat routines. |
| 85 | `AFF_DAZED` | Dazed | Condition | Dazed state recognized by incapacitation/combat checks; separate from Dazzled's small penalties. |
| 86 | `AFF_FLAT_FOOTED` | Flat-footed | Condition | Flat-footed state used by awareness/defense checks. Distinct from a successful Feint and its source record. |
| 87 | `AFF_DUAL_WIELD` | Dual-wield | Mode | Dual-wield mode. Does not create an off-hand weapon or grant every two-weapon feat. |
| 88 | `AFF_FLURRY_OF_BLOWS` | Flurry-of-Blows | Mode | Flurry of Blows mode; attack routines enforce relevant weapon/class requirements and modifiers. |
| 89 | `AFF_COUNTERSPELL` | Counterspell | Mode | Counterspell mode; relies on casting, available spells and the counterspell system. |
| 90 | `AFF_DEFENSIVE_CASTING` | Defensive-Casting | Mode | Defensive Casting mode used by spellcasting/concentration logic. Not automatic casting success. |
| 91 | `AFF_WHIRLWIND_ATTACK` | Whirlwind-Attack | Mode | Whirlwind Attack mode distributes eligible attacks through its combat routine. Does not directly create an area spell. |
| 92 | `AFF_CHARGING` | Charging | Mode | Charging combat state; bonuses, penalties and duration come from the charge/action path. |
| 93 | `AFF_WILD_SHAPE` | WildShape | Linked | Wild-shape/morph state. Actual race/form, equipment handling and transformation resources need the shapechange system. |
| 94 | `AFF_FEINTED` | Feinted | Linked | Feinted state used by flat-footed checks together with Feint source records. Apply through the maneuver, not a permanent item. |
| 95 | `AFF_PINNED` | Pinned | Linked | Pinned grapple state; needs the maneuver system and release/escape handling. |
| 96 | `AFF_MIRROR_IMAGED` | Mirror-Imaged | Linked | Mirror-image marker. Combat uses the appropriate illusion spell records and GET_IMAGES count; the bit alone creates no working images. |
| 97 | `AFF_WARDED` | Warded | Linked | Ward marker set by protective spells. Actual absorption depends on damage-reduction records, ward/stoneskin counters and the source spell. |
| 98 | `AFF_ENTANGLED` | Entangled | Condition | Entangled blocks ordinary room movement; speed and action checks also recognize it. Escape/source duration needs appropriate handling. |
| 99 | `AFF_ACROBATIC` | Acrobatic | Special | Acrobatic marker used by specialized traversal procedures, including a Jump-related path. Not a universal numeric Acrobatics bonus. |
| 100 | `AFF_BLINKING` | Blinking | Passive | Blinking participates in concealment and spell/physical miss checks. Appropriate attacker senses/ethereal attacks can reduce its protection. |
| 101 | `AFF_AWARE` | Aware | Passive | Awareness state can prevent supported surprise/backstab paths while awake. Does not create complete immunity to every sneak attack. |
| 102 | `AFF_CRIPPLING_CRITICAL` | Crippled-by-Critical | Linked | Crippling Critical marker on timed critical-hit affects; the actual penalties are stored in associated numeric apply records. |
| 103 | `AFF_LEVITATE` | Levitating | Passive | Levitation prevents supported falling/water-travel hazards. It is not identical to the full flying capability checked by every flight-required sector. |
| 104 | `AFF_BLEED` | Bleeding | Linked | Bleeding damage is read from associated AFF_BLEED affect records during periodic processing. The flag alone does not specify damage per round. |
| 105 | `AFF_STAGGERED` | Staggered | Condition | Staggered affects action availability and restricts attack routines, including a one-attack limit. Older label text about a blanket 50% failure chance is not a complete description of current code. |
| 106 | `AFF_DAZZLED` | Dazzled | Condition | Dazzled supplies a -1 attack penalty and affects relevant perception/skill checks. Different from Dazed. |
| 107 | `AFF_SHAKEN` | Shaken | Condition | Shaken is a fear-related state with -2 penalties in supported attack/save/skill checks. Fear escalation and immunity use separate source logic. |
| 108 | `AFF_ESHIELD` | Electric-Shielded | Passive | Electric-shield state used by supported damage retaliation. Not an always-on electric weapon enchantment. |
| 109 | `AFF_SICKENED` | Sickened | Condition | Sickened supplies -2 penalties in supported attack, weapon damage, save and skill/ability calculations; source may add numeric penalties too. |
| 110 | `AFF_SILENCED` | Silenced | Condition | Silenced blocks supported communication and ordinary casting speech. Psionic powers and alchemist extracts have explicit casting exceptions. |
| 111 | `AFF_HIDE_ALIGNMENT` | Hidden-Alignment | Passive | Hide-alignment state obscures alignment through supported detection paths. Does not change the character's actual alignment. |
| 112 | `AFF_WIND_WALL` | Wind-Wall | Passive | Wind Wall participates in ranged/immaterial combat and ongoing environmental checks. Clearing room fog occurs when the spell is cast, not merely by setting this bit. |
| 113 | `AFF_FEAR_AURA` | Fear-Aura | Special | Fear-aura capability used by the aura command/NPC race behavior. Saves, targets and repeated application belong to that subsystem. |
| 114 | `AFF_SPIDER_CLIMB` | Spider-Climb | Special | Spider-climb state used by specialized traversal procedures. The generic climb helper also checks SPELL_SPIDER_CLIMB separately; this bit alone is not a substitute for every climbing path. |
| 115 | `AFF_DEADLY_AIM` | Deadly-Aim | Mode | Deadly Aim mode for the supported ranged attack/damage tradeoff; requires the appropriate combat/feat context. |
| 116 | `AFF_ACID_COAT` | Acid-Coated | Condition | Acid-coated state used by periodic acid-fog damage/recovery checks. MEDIT strips it; use the appropriate source with duration. |
| 117 | `AFF_REPULSION` | Repulsion-Fielded | Passive | Repulsion state checked by melee engagement/attack processing. Saves and effect scope depend on its source and combat path. |
| 118 | `AFF_ON_FIRE` | On-Fire | Condition | On-fire state causes periodic 2d6 fire damage through normal damage processing and is recognized by extinguishing handling. |
| 119 | `AFF_FLAME_BLADE` | Flame-Blade | Passive | Flame Blade adds its supported melee fire rider through special-attack processing. Not an ITEM_FLAMING weapon toggle. |
| 120 | `AFF_SICKENING_AURA` | Sickening-Aura | Special | Sickening aura is processed against eligible nearby targets with saves, timers and source rules; it can apply Sickened and a numeric penalty. |
| 121 | `AFF_RAPID_BUFF` | Rapid-Buff | Mode | Rapid Buff speeds the supported buff-list casting/preparation path. Not a universal quickening of every spell. |
| 122 | `AFF_CRIPPLED` | Crippled | Condition | Crippled participates in speed halving and gives a 50% failure chance on ordinary movement attempts. Source duration/removal remains separate. |
| 123 | `AFF_ENCASED_IN_ICE` | Encased-in-Ice | Linked | Encased in Ice directly provides effective cold immunity and DR 5/- in supported damage calculations. Its source must supply paralysis/other condition records; the bit alone is not the entire spell. |
| 124 | `AFF_NEXTATTACK_STUN` | Nextattack-Stun | Mode | Berserker next-hit Stunning Blow state. A successful hit consumes it, then attempts a Fortitude save against DC 10 + character level + Strength bonus; failure applies two-round stun. |
| 125 | `AFF_HIVE_MARKED` | Hive-Marked | Linked | Hive Commander mark: the spell record identifies the target for later +3 DC logic, while separate affects grant allied hit bonuses. The raw marker alone is insufficient. |
| 126 | `AFF_PERFECT_DEFLECTION_ACTIVE` | Perfect-Deflection | Mode | Perfect Deflection readiness: consumes itself to negate the next qualifying ranged/bomb attack and retaliates with force damage. Not universal melee/spell immunity. |

## Complete AFF2 reference

These entries belong to the **second bank**, not the AFF bank. Use their direct
indices in OEDIT `V` or the MEDIT AFF2 menu.

| Index / choice | Flag | Display label | Purpose and current behavior |
| --- | --- | --- | --- |
| 0 | `AFF2_DONTUSE` | (reserved) | Do not use. Index 0 is a sentinel and exits the editor menu. |
| 1 | `AFF2_MAGIC_ATTACKS` | Magic-Attacks | The attacker's supported attacks count as magical for the DR/magic bypass check. Used for summons/construct improvements. Does not grant attack/damage enhancement, bypass every material/alignment DR, or supply magical spells. |
| 2 | `AFF2_COWERING` | Cowering | Strong fear state used by fear escalation and blackguard perks. Current direct checks impose a -4 attack penalty, -4 in the relevant ability calculation, and a 10% chance each fighting round to consume standard, move and swift actions. It is not unconditionally identical to paralysis. |

Both have real consumers of the **live second-bank flags**. However, their
presence in a timed affect record or an object's stored vector does not ensure
that they reach that live bank under the current common handler. That matters
for testing spell/perk applications; see the limitation below.

## Important interactions

### Detection is not one universal sense

| State | Intended scope | Does not automatically mean |
| --- | --- | --- |
| Detect Invisible | Detect invisible creatures through supported helpers. | Reveal every hidden or sneaking creature. |
| Sense Life | Sense life/presence in supported displays. | Display complete identities, objects or room descriptions. |
| Infravision / Ultravision / Darkvision | Feed vision helpers alongside racial and feat senses. | Bypass every fog, magical-darkness or bright-light check. |
| True Sight | Strong detection through supported vision/illusion helpers. | Ignore every scripted restriction. |
| Hide / Sneak | Character stealth state. | A guaranteed perception DC or an undetectable creature. |

The vision helpers overlap: for example, `char_has_ultra()` also considers
infravision, races and perks. Other look/scan paths explicitly test room flags.
Use the actual mortal viewing action to test darkness, hiding and fog; a staff
character's successful view does not prove the same result for players.

### Flight, water and climbing

- **Flying** is the active flight state used by several movement checks.
- **Levitate** can prevent falling and satisfy some water-travel checks, while
  the generic flight helper has its own conditions.
- **Water Walk** can satisfy water traversal without granting underwater breath.
- **Scuba / Water Breath** permits underwater breathing through supported checks.
- **Climb** supplies climbing capability, not Climb skill ranks. The separate
  **Spider Climb** flag appears in specialized procedures; the generic climb
  helper also checks the actual Spider Climb spell record.
- **Safefall** is consumed at landing. It does not keep the character airborne.

Rooms add separate requirements: `ROOM_NOFLY`, `ROOM_FLY_NEEDED`, size flags,
terrain and underwater checks still matter. Do not use a passive flag as a
shortcut around all room restrictions.

### Movement and combat modes

Spot and Listen each double ordinary walking cost; enabling both multiplies
that part of the cost by four. Haste changes speed and eligible attack routines.
Slow, Entangled and Crippled participate in the speed-reduction chain; those
branches do not all multiply together indiscriminately. Entangled and Grappled
also directly prevent ordinary room movement. Crippled separately has a 50%
movement-failure roll.

Power Attack, Expertise, Total Defense, Rapid Shot, Dual Wield, Flurry,
Counterspell, Defensive Casting, Whirlwind and Deadly Aim normally belong to
commands and class/feat/action systems. Setting their bits permanently is not
the same as using the mode command and satisfying its prerequisites.

Staggered currently links move and standard action consumption and limits
attack routines. Do not rely solely on its older display description of a
50% spell/attack failure chance. Dazed, Dazzled and Staggered are different
conditions, despite similar names.

### Defenses requiring extra state

| Flag | Supporting state / restriction |
| --- | --- |
| Mirror Imaged | Correct illusion spell affect plus `GET_IMAGES()` count; some image spells have specialized handling. |
| Spell Mantle | Positive mantle counter, initialized by the source spell and consumed on absorption. |
| Warded | Appropriate ward/stoneskin/DR source records and reserve data. |
| Spell Turning | Consumed when an eligible hostile/damaging targeted spell is turned; device handling is excluded at this check. |
| Inertial Barrier | A random eligible-damage absorption check (currently 50%) spends `1 + damage / 5` PSP and can remove the barrier on depletion. |
| Spell Resistant | Supplies an SR candidate floor of `12 + character level` in the resistance helper, before its remaining modifier/cap processing; not an arbitrary SR number encoded in the bit. |
| Nextattack Stun | Successful-hit trigger, save calculation and timed stun creation; not an unconditional stun aura. |
| Perfect Deflection Active | One qualifying ranged/bomb attack, then consumes readiness and reflects force damage. |

Minor Globe uses `lowest_spell_level(spellnum) <= 4`; Globe of Invulnerability
uses `<= 8` in the current resolution branch. The comments describe these as
second-circle and fourth-circle equivalents. These are **spell-level tests**,
not a comparison against the attacking caster's character level. The branch
also excludes warlock magic and device casting and requires an eligible
hostile/damaging effect.

### Conditions, immunities and cleanup

Fear, poison, disease, sleep, bleeding, charm, grapple, transformation and aura
systems have their own application, saves, immunity, duration and cleanup.
Toggling a bit does not call those application routines.

Use the normal spell/ability where possible. Give NPCs ongoing capabilities
through supported template flags only when that is how the system expects to
receive them. Do not assign master-dependent or event-dependent states as
simple permanent item bonuses.

Removing a raw bit is also not necessarily a full cure: a remaining timed
record, master link, stored counter or event can require cleanup, or restore
state during recalculation. Conversely, removing one source must not
accidentally remove a capability supplied by another legitimate source.

## Current implementation limitations

### AFF2 records are not merged by the common affect/equipment handler

The source stores second-bank data in several places:

- Mobile prototype/live character `affected2_by` vectors.
- Timed affects' `bitvector2` fields, including player-save serialization.
- Objects' permanent `bitvector2` fields, editable with OEDIT `V`.

The direct live-bank checks are real. For example, the DR/magic branch checks
`AFF2_MAGIC_ATTACKS`, and combat/skills check `AFF2_COWERING`. Certain summon
creation paths explicitly set a live AFF2 bit.

However, in the current `handler.c`, `affect_to_char()`, removal and total
recalculation pass the first `bitvector`/object-affect vector through
`affect_modify_ar()`. That helper changes `AFF_FLAGS`, and no equivalent
second-bank merge/removal is present in those common paths. Merely recording
`bitvector2` does not make `AFF2_FLAGGED()` true.

Practical consequences:

- A mobile with AFF2 set directly can exercise its live-bit consumers.
- A spell/perk may store an AFF2 affect without activating the corresponding
  live-bank check through the common handler.
- An OEDIT permanent AFF2 flag can be stored/displayed without becoming active
  on its wearer.
- The existence of a flag in an editor or save record does not prove complete
  application, expiry or unequip behavior.

This is a source-level limitation documented here, not a gameplay fix performed
as part of this guide. Verify the live flag and actual behavior separately.

### Legacy and marker-only flags

Some labels describe more than their direct implementation currently supplies:

| Flag(s) | Current limitation |
| --- | --- |
| `AFF_SANCTUARY` | Applied/displayed as an aura; no direct damage-reduction/attack-prevention consumer of this bit found. |
| `AFF_SIZECHANGED` | No active direct consumer found; real size data changes are separate. |
| `AFF_SHADOW_SHIELD` | Direct flag checks in damage code are commented out; use the actual spell's supported effects. |
| `AFF_TOWER_OF_IRON_WILL` | Unfinished flag, with no active direct consumer found. The corresponding power may use other numeric effects. |
| `AFF_MAX_DAMAGE` | No active direct consumer found; the label does not implement maximized damage. |
| `AFF_CAGE` | No active direct interaction-lock consumer found. |
| `AFF_BODYWEAPONRY` | No active direct damage/capability consumer found. |
| `AFF_FARSEE` | Set by a bardic effect, but no direct scan-range consumer found. |
| `AFF_HIVE_MARKED` | The +3 DC consumer checks `SPELL_HIVE_COMMANDER_MARK`, not the raw AFF bit; linked allies receive separate hit-bonus affects. |

“No direct consumer” refers to current active source references to the bit;
it does not mean the same-named spell or power cannot work through other data.

## Verification workflow

1. Identify the exact bank, flag index and source spell/ability, if applicable.
2. Check whether it is passive, a condition, a mode, or a linked/legacy marker.
3. Inspect a working source before putting it on equipment or a mobile.
4. Test with the intended mortal character or NPC, including prerequisite
   class, size, vision and resource conditions.
5. Confirm both displayed state and the actual effect: attacks, movement,
   damage, visibility, saves or resource consumption.
6. For equipment, test equip, unequip and replacement by another source.
7. For timed effects, test duration, recovery, dispel/cure and source removal.
8. Check login/reload and expiry behavior when the effect uses saved state.
9. For one-shot effects, verify consumption and that an unrelated action does
   not consume or reproduce the effect unexpectedly.

Use `affects` and the relevant staff character/object inspection tools as
observations, not sole proof that a flag-dependent subsystem is working. A
message can exist without the required counter or live-bank bit.

## Source references

- [structs.h](../../src/structs.h): AFF/AFF2 constants, aliases, banks and affect records.
- [constants.c](../../src/constants.c): editor labels and display descriptions.
- [utils.h](../../src/utils.h): bank macros and object permanent-affect accessors.
- [utils.c](../../src/utils.c): vision, immunity and affect initialization helpers.
- [handler.c](../../src/handler.c): applying/removing affects and equipment recalculation.
- [oedit.c](../../src/oedit.c): permanent-affect menus and Charm rejection.
- [medit.c](../../src/medit.c): mobile affect menus and stripped runtime conditions.
- [fight.c](../../src/fight.c): concealment, defenses, incapacitation, attack modes,
  vampiric effects and one-shot attack triggers.
- [spell_parser.c](../../src/spell_parser.c): globe, mantle, turning and casting checks.
- [magic.c](../../src/magic.c): spell application, spell resistance and linked state.
- [movement.c](../../src/movement.c), [movement_cost.c](../../src/movement_cost.c),
  [movement_validation.c](../../src/movement_validation.c) and
  [movement_falling.c](../../src/movement_falling.c): movement/capability rules.
- [actions.c](../../src/actions.c): Staggered action consumption.
- [limits.c](../../src/limits.c): periodic states, auras and condition processing.
- [perks.c](../../src/perks.c): Cowering, Hive Commander and other perk applications.
- [spec_procs.c](../../src/spec_procs.c): specialized traversal, refuge and item synergy.
- [players.c](../../src/players.c): timed-affect save/load fields, including AFF2 data.
