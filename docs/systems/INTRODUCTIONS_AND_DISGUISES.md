# Introductions and disguises

Player names are learned directionally: when Alice introduces herself to Bob,
Bob learns Alice's name. Alice must receive Bob's introduction to learn his name.
NPCs and immortals retain their existing exemptions. Introductions are enabled
by default; an explicit `use_introduction_system = 0` in the runtime configuration
still disables them. Existing short-description setup is required at login when
the system is enabled.

## Player commands

```
introduce <description keyword or known name>
introduce list
introduce forget <known name>

disguise <race> | <age> | <gender> | <feature:adjective> | <feature:adjective>
disguise <race>
disguise descriptors [feature]
disguise remove
```

For example:

```
disguise human | adolescent | female | hair:43 | eyes:2
```

This appears as `an adolescent female human with short silver hair and green eyes`.
Disguises use the same two feature/adjective ID pairs, tables, menus, and formatter
as natural short descriptions in `char_descs.c`. Browse feature types with
`disguise descriptors` and choices with `disguise descriptors hair`. For example,
`hair:43` selects short silver hair and `eyes:2` selects green eyes. Numeric feature
IDs also work (`6:43` means `hair:43`). Choose `none` to omit a feature. Supplying
one pair omits the second; omitting both pairs copies your natural selections.
Invalid feature/adjective IDs are rejected before the skill roll.
The race must be a humanoid of the character's size. Type `disguise` to list
eligible races, including human (race zero is supported). Accepted ages are
`adult`, `adolescent`, `middle-aged`, `old-aged`, and `venerable`; genders are
`neutral`, `male`, and `female`. A race alone uses the character's actual age and
gender and natural descriptor/adjective selections.

Applying a disguise requires the Disguise ability and consumes a standard action.
Removing it also consumes a standard action, even if the ability has since been
lost. Remove the current disguise before applying another. Wildshape and
shapechange cannot be combined with a cosmetic disguise. Introductions while
disguised are refused; this implementation does not create aliases.

## Recognition

Applying a disguise rolls `d20 + Disguise` once and stores the result. Each player
observer rolls `d20 + Perception` once when they first see that instance. A tie
succeeds. Room displays, arrival/social/combat acts, and scans use this recognition
state. A remote player listing cannot trigger a new perception roll. Returning,
looking again, reconnecting, and restarting reuse the recorded result. Removing
and reapplying a disguise creates a new instance and permits one new check.

A failed observer sees only the disguised short description, even if they already
know the character's name. A successful observer sees the known name, or the real
short description if the character has never introduced themselves. Room/look
and scan displays show a separate line:

```
Alice is disguised as an adolescent female human with short silver hair and green eyes.
```

An unknown character's real short description replaces `Alice`; seeing through a
disguise never teaches a name. A successful first check triggered by an act emits
the notice after that act line. Subsequent acts do not repeatedly emit it.
Pronouns in act messages follow the appearance the recipient perceives.

True sight uses the game's `has_true_sight()` helper, including spell, feat, and
terrain-granted true sight. It sees through the disguise without rolling. When it
expires, the observer returns to their saved perception outcome, or receives their
one normal check if they have only seen this instance using true sight.

Unknown names cannot be used as hidden targeting keywords. Target the visible
description instead (including numbered matching such as `2.human`). Player lists,
whois, groups, MSDP group/combat names, scan, and common direct combat/ability
messages respect identity recognition. Administrative storage keys and logs still
use actual names.

## Storage and upgrading

Appearance is separate from actual race, age, gender, and descriptors. It grants
no racial mechanics or attribute changes. Existing magical transformation fields
are unchanged. Old nonmagical race-only disguises are cleared on loading and must
be reapplied using the new command. Earlier free-text `DgAp` disguises are also
cleared, while their generation is retained to prevent recognition-cache reuse.

Pfiles save `DgGn` (per-character instance generation), `DgA2` (active appearance,
roll, and both feature/adjective ID pairs), and `DgOb` (subject player ID, instance, outcome). Outcomes are `0` for
failure, `1` for success, and `-1` when the observer has only used true sight.
Only the latest observed instance per subject is retained. Introduction names
continue using the existing `Intr` format and existing 100-person limit. Learning
or forgetting a name saves immediately. Full introduction lists now correctly
consume their terminating `~` without consuming the next pfile tag.

`which_desc()` and the `current_*_desc()` functions return allocated strings that
the caller frees. `PERS()` / `show_pers()` return borrowed strings; copy them if
retaining them across subsequent output.

## Verification

```
python3 tests/run_introductions_disguises.py
cmake -S . -B /tmp/luminari-identity-build -DSTATIC_ANALYSIS=OFF
cmake --build /tmp/luminari-identity-build --target circle -j6
```

The focused harness compiles production identity functions, disguise/introduction
commands, description builders, and persistence codecs using deterministic dice.
It runs under AddressSanitizer and UndefinedBehaviorSanitizer without requiring a
database or live player files.

The bundled `lib/text/help/introductions_disguises.hlp` supplies help text for
installation through the deployment's help workflow (production help uses MySQL/
hedit). Legacy file-based deployments can include it in `lib/text/help/index`
before the `$` terminator. The disguise command also displays usage directly.

## Appearance spells (game-specific rules)

```
cast 'disguise self'
cast 'alter self' <player-selectable race>
disguise dismiss disguise self
disguise dismiss alter self
```

Disguise Self only grants +10 to the Disguise skill for ten minutes per caster
level. It does not create an appearance, change equipment, conceal a race, roll
perception, or provide a Will-disbelief mechanic. Applying a mundane disguise
while the bonus is active includes it in the single application roll.

Alter Self changes `GET_RACE()` to the selected player-selectable race for one
minute per caster level; it supports every race marked player-selectable,
including locked races, human (ID zero), and Medium or larger forms. The original
race is retained separately and restored when the spell expires, is dispelled,
or is dismissed. Small forms grant +2 Dexterity; Medium or larger forms grant
+2 Strength. These affects use **racial** bonuses. Tiny or smaller selectable
forms change race and size without either stat bonus.

The new race is visible to ordinary observers and cannot be exposed by a
Perception check. True sight reveals the original form and separately describes
the current Alter Self form. A mundane disguise can still conceal gender,
age, or descriptor choices; recognition checks continue to govern those details
and name recognition. The displayed race is the active Alter Self race even when
an observer fails to see through the mundane disguise. Casting or recasting Alter
Self does not create a new mundane disguise instance or grant another perception
attempt. Both spells can coexist, and Alter Self grants no additional Disguise
bonus, mind blank, or special senses.

Appearance-spell durations support Extend Spell. Alter Self's selected race is
stored per caster during delayed casting so another player's command cannot
overwrite it. Its active race is saved as `AlRc` alongside the standard timed
spell affects. Loading an orphaned field or a legacy spell without a saved race
clears the incomplete transformation. These choices intentionally override the
linked tabletop spell rules for this game.


## True sight: original form and role-play context

Every source recognized by `has_true_sight()` (the spell, feat, or other granted
effect) renders original character traits using `GET_REAL_RACE`, while leaving
active race, size, bonuses, and all transformation mechanics unchanged.
Introductions still control names: an unknown person's natural short description
is shown, rather than teaching their name.

For example, a true-sight observer sees a known character's original identity and:

```
Alice's true form is an adult male human with short red hair and green eyes.
Alice's altered form is a wolf (wildshaped).
```

Alter Self, modern wildshape/polymorph race forms, legacy animal-shape subtypes,
and creature-family shapechanges all supply their current altered form. If a
mundane disguise is also present, the existing `is disguised as ...` line shows
that appearance as well. True sight does not overwrite the saved perception
result or change physical character state.

Room, look, scan, and affected-by displays show this context explicitly. Arrival,
speech, and combat acts show it on the first relevant message and when the form
changes, without repeating it after every message. Losing true sight restores the
normal appearance and saved disguise recognition result.

## Irda advanced race

Irda use the existing reserved race ID 52 and are an advanced, Medium humanoid
race with a 5,000 account experience unlock and level adjustment 2. Their racial
ability adjustments are +2 Dexterity, Intelligence, Wisdom, and Charisma.

An Irda can assume any registered humanoid race, including a different size,
using the shared appearance selections:

```
disguise form human | old-aged | female | hair:43 | eyes:2
disguise form remove
```

All age, gender, and descriptor choices come from `char_descs.c`. Use
`disguise descriptors [feature]` for the adjective menus. A race alone keeps
natural age, gender, and descriptors; `none` omits a descriptor. Reapply the
command to select another appearance, including an altered Irda appearance.

This innate ability requires no Disguise training, Disguise roll, Perception
roll, spell slot, or mana, and has no duration. Changing or removing a form uses
a standard action. It changes active race, visible description, pronouns, and
size without granting the Alter Self ability bonuses. Racial ability scores
remain those of the character. Return to natural form before advancing or
using another transformation.

An assumed form conceals a known identity and does not reveal the character's
natural long description. True Sight shows the natural identity (if introduced)
and description, plus the complete assumed appearance as roleplay context.
Ordinary disguises may be worn over an innate form and use their own normal
opposed checks. The innate form itself cannot be exposed by Perception.

The validated `AiFm` player-file tag preserves the complete assumed appearance
across reconnects. Characters whose real race is not Irda cannot activate it.

## Sivak Draconian advanced race

Sivaks use reserved race ID 50. They are a Large advanced race with a 5,000
account experience unlock and level adjustment 2. Ability adjustments are
+2 Strength, Dexterity, Constitution, and Intelligence. Their existing racial
feats grant flight (`fly`), ultravision, disease immunity, spell resistance
(10 + character level), a swift-action bite (`bite`), draconic devotion,
draconian gallop, and Improved Trip to represent their tail-tripping aptitude.
Their racial language is Draconic.

```
disguise forms
disguise form <number>
disguise form remove
```

A player Sivak learns a reusable snapshot when they personally kill a humanoid
NPC or PC. The snapshot is captured in `raw_kill`, after pet archival succeeds
and before death cleanup removes transformations. Group members and pet owners
do not automatically receive the form. Self-kills, animals, other non-humanoids,
and arena defeats do not grant forms. There is no victim size or gender restriction.
Only learned form numbers are accepted; a Sivak cannot freely edit them like an
Irda. Identical repeat kills do not grow the collection; different appearances
of the same enemy can be retained separately.

Snapshots retain the victim's short description, long description, apparent
sex, and size. Player short descriptions come from the shared descriptor
formatter; NPC descriptions are copied from the NPC. Existing magical changes
are copied as visible appearances, without exposing the victim's hidden natural
long description. Mundane costumes, names, titles, equipment, memories, skills,
and spells are not copied. NPC creature-family IDs are not interpreted as player
race indices. NPC appearance is described directly instead of displaying a false
racial label. Snapshot text is bounded to 511 bytes for the short description
and 2,047 bytes for the long description.

Changing form is a standard action with no Disguise training, opposed check,
mana cost, or duration. Return to natural form before advancing, wildshaping,
shapechanging, or casting Alter Self. Ordinary disguises can still be layered
on top and use their own opposed checks. Assumed forms hide the Sivak's known
identity; they never teach the victim's name. True Sight reveals the Sivak's
natural form alongside the copied appearance.

### Player-file persistence

Every learned appearance is saved immediately to the Sivak's normal player file.
Repeated `SvFm` records contain the stable form ID, NPC/player source identity,
source namespace, race metadata, size, and sex, followed by two tilde-terminated
snapshot strings. The `SvAc` record saves the currently assumed form ID.

The loader validates records and restores the collection and active appearance.
It preserves snapshot bytes and consumes invalid record strings without losing
the next pfile tag. Saves use bounded chunks below the existing save buffer's
limit. Snapshots own their data and survive logout, server restarts, NPC extraction,
and the victim changing their appearance later. Removing a form does not forget it.

### Research and adaptation

The [Dragonlance Nexus Pathfinder conversion](https://dragonlancenexus.com/wp-content/uploads/PFCG-Creatures-of-Ansalon.pdf)
describes Sivaks with flight, disease immunity, spell resistance, a bite, a tail
trip, and kill-based shapeshifting. [Steve Kenson's Dragonlance Nexus adventure](https://dragonlancenexus.com/turnabout-rescue-2/)
also describes flight and the broader draconian traits.

The game uses the requested ability adjustments and reusable forms for all
Sivaks, rather than the sources' gender, timing, and size restrictions. Improved
Trip is the existing-system adaptation of tail tripping. The older editions'
corpse imitation, explosive death throes, female blending, and additional monster
stat-block armor and attacks are not added by this implementation.

## Aurak Draconian advanced race

Auraks use reserved race ID 51 and are the strongest advanced draconian option.
They are Medium, cost 15,000 account experience to unlock, and have level
adjustment 4. Racial ability adjustments are +2 Dexterity, +4 Intelligence,
and +4 Charisma. They gain +6 natural armor, spell resistance of 12 + character
level, ultravision, disease immunity, a bite, draconic devotion, Combat Casting,
and Improved Initiative. They speak Draconic and have no natural flight or glide.

### Racial magic

Use the normal `cast` command. No caster class or prepared spell slot is required.
Racial caster level is character level; racial spell save DCs use Charisma and
half caster level, with the normal spell DC bonuses. Access checks are read-only.
The two signature powers have first-circle handling without adding them to class
spell lists or accidentally using the default staff-only concentration difficulty.

| Character level | Available racial spells |
| --- | --- |
| 1 | aurak energy ray, aurak breath, disguise self, magic missile |
| 3 | charm, mirror image |
| 5 | fireball, dispel magic |
| 7 | greater invisibility |
| 9 | dominate person |

Examples:

```
cast 'aurak energy ray' <enemy>
cast 'aurak breath'
cast 'disguise self'
cast 'greater invisibility'
cast 'dominate person' <enemy>
```

The energy ray makes one ranged touch attack for 4d6 force damage, allows spell
resistance, and has no saving throw. A miss deals no damage. Noxious breath is a
room-area adaptation using the normal area target protections: eligible enemies
make Fortitude saves or take a temporary 1d2 Strength penalty for 1d4 rounds,
with blindness for the same duration if susceptible. Poison-immune targets are
unaffected. The breath recharges after 1d4 six-second combat rounds.

Dominate person is available once per rolling 24 real hours. Energy ray has no
use limit, and breath keeps its own recharge; neither costs spell points.
The other at-will racial spells share **10 points**, costing their base wizard
circle: disguise self, magic missile and charm cost 1; mirror image costs 2;
fireball and dispel magic cost 3; greater invisibility costs 4. The entire pool
refills five minutes after its first use. Later casts do not postpone that refill.
`innates` shows the remaining points and refill timer. Points persist through
logout and restart via the `AuMp` player-file tag. Prepared class casts and
devices do not spend this pool. Normal casting actions and restrictions apply.
Cooldowns and points are
spent only at effect dispatch after the existing room, target, PvP and antimagic
checks; a resisted invocation still spends its use. Independent magic devices
do not consume racial cooldowns. The `AuDm` and `AuBr` player-file tags save
absolute recharge timestamps, so logout and server restart cannot reset them.
After the racial domination use is spent, prepared class spells remain available
and consume their normal slots; device casts also remain independent. Racial
source is preserved through delayed casting so queued casts cannot bypass the
daily use.
Disguise Self retains the game's +10 Disguise-only behavior.

### Death throes

When slain, an Aurak releases 3d6 force damage against eligible enemies in the
room, with Reflex saves for half damage. This uses the existing draconian death
hook and area protections, including while the Aurak has an altered appearance.
It cannot be cast directly. Equipment and resurrection follow the normal game
rules; no multi-stage dying frenzy is added.

### Sources and adaptation

The published [Dragons of Krynn racial traits](https://files.spawningpool.net/docs/Vault2.0.-.TTRPG-Gamebooks/Dungeons%20%26%20Dragons%20%5Bmulti%5D/D%26D%203.5/Dragonlance%20Setting/Dragons%20of%20Krynn.pdf)
support the ability adjustments and +6 natural armor. The
[Dragonlance Nexus Pathfinder conversion](https://dragonlancenexus.com/wp-content/uploads/PFCG-Creatures-of-Ansalon.pdf)
describes energy rays, noxious breath, defensive magic and domination.

The unlock cost, level adjustment, progressive spell access and recharge rules
are game adaptations. Charm represents part of their mind-influencing magic;
it is not a new Suggestion spell. The implementation uses racial spell access
rather than granting free sorcerer class levels or racial Hit Dice. Animal
alternate form and dimensional step are not included in this release.
