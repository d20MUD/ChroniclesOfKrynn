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
