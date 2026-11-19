# Bard, Duelist and Stalwart Defender improvements

Implemented from [the design proposal](UNDERUTILIZED_CLASS_IMPROVEMENTS.md).
The Ranger, Monk, Rogue and Assassin suggestions remain follow-up candidates,
as specified in the proposal; they are not part of this implementation.

## Player commands

| Command | Behavior |
| --- | --- |
| `classfeatures` | Shows class ranks, current feature requirements, readiness and defender buffer. |
| `perform list` / `perform status` | Lists known songs or shows both active performance slots. |
| `perform [1\|2] <song>` | Starts or replaces a song; numbered slots allow explicit dual-performance control. |
| `perform stop` | Stops both songs; the original no-argument behavior remains supported. |
| `rally [groupmate]` | Bard 7: swift action, +2 morale to saves for 12 seconds (+3 at Bard 15), 30-second cooldown. Defaults to self. |
| `opening [opponent]` | Duelist 8: swift action, next normal primary attack against the current melee opponent within 12 seconds receives +2 insight to hit and +1d6 ordinary damage; 18-second cooldown. |
| `guard <groupmate>` / `guard` / `guard stop` | Chooses the protected companion, shows status, or stops guarding. |
| `intercept` | Defender 6: enables/disables automatic weapon interception for the guarded companion. |
| `advance [opponent]` | Defender 4+: in stance, moves up to 10 feet within the room; costs a move action and one movement point per foot. |

## Automatic benefits

- **Bard Battle Rhythm:** +1/+2/+3 competence to weapon attack and damage rolls at Bard 3/9/15 while maintaining a valid performance. Works with melee, projectile and thrown weapons. Stronger existing bonuses replace it; additional songs do not multiply it.
- **Revealing Refrain:** the existing Song of Revelation gains a concealment rider at Bard 11. The strongest eligible same-room bard supplies the DC, `10 + floor(Bard ranks / 2) + Charisma modifier`. The first concealed weapon attack against an engaged visible enemy attempts spell resistance and one Will save per target per native combat interval. Failure suppresses concealment for the performing bard's same-room group during that interval. Existing song effects remain.
- **Measured Parry:** Duelist 5, once per native combat interval during ordinary combat. Against a successful non-critical melee weapon hit, `d20 + Duelist ranks + Dexterity modifier` must exceed the attack total. A success negates the attack and grants one Riposte at −2 if the defender has that feature. A failed attempt also spends the interval. Light/no armor, a melee weapon, a free hand, no casting and no total defense are required.
- **Find the Seam:** Duelist 6, eligible dueling melee weapon hits against critical-immune creatures receive half Duelist ranks in ordinary damage, capped at 5, replacing unavailable Precise Strike. It does not restore critical hits or precision riders.
- **Guarded Companion:** Defender 2/6/10, +1/+2/+3 AC against weapon attacks for one guarded conscious groupmate in melee reach while the defender maintains stance. Multiple defenders provide only the strongest bonus. The check is dynamic, so there is no stale AC affect to remove.
- **Measured Advance:** at Defender 4–8, stance advances recover every 18 seconds; at Defender 9, the interval is removed. Movement restrictions and action costs remain. Existing Mobile Defense exit traversal remains unchanged.
- **Intercepting Guard:** Defender 6, optional redirection of one successful single-target weapon attack every 18 seconds. Original-target avoidance resolves first. The defender receives the attack without a second roll, using their own mitigation and critical immunity. There is no second interception or reactive attack chain. Native guard/rescue can still change the target before an attack begins.
- **Shielded Recovery:** successful Renewed Defense at Defender 8 grants a buffer of twice Defender ranks, capped at 20, lasting 18 seconds. It absorbs final weapon damage after mitigation, and leaves existing Renewed Defense usage limits intact.

## Reliability and integration

Performance requests validate song ownership, requirements and action availability
before changing either slot. Master of Motifs can genuinely start its second
song. Switching preserves the other slot and does not reset Crescendo's used
flag. Invalid requests preserve all state and spend no action.

The total-defense Riposte modifier now checks the defender's feature.
Measured-Parry counters preserve normal attack counters, do not execute queued
abilities, and use a shared reaction gate to prevent nested attack chains.
Attack cleanup checks character identity after callbacks that can extract an
attacker. Opening uses target IDs and clears when either participant departs.

The native combat period is `PULSE_VIOLENCE / PASSES_PER_SEC` (currently six
seconds). New short effects and recovery intervals use that real-time period,
like existing combat recovery events; they are not world-tick affect durations
or timers that pause outside combat. Five tagged `CiCd` values persist the new
personal cooldowns across logout and class respec. Temporary buffs and target
selections do not persist across logout. No retroactive pfile migration is
needed: missing fields default to zero.

Revelation is a divination rider rather than a mind-affecting compulsion.
It requires an active eligible bard and does not defeat blocked targeting,
inability to see, cover, spell resistance or separate incorporeal defenses.
Blinking retains its ethereal miss chance. New support bonuses respect their
named bonus types. Opening and Find the Seam damage enter after critical
multiplication and before normal damage mitigation.

Class descriptions used by class/study information and relevant existing feat
descriptions explain the additions. New help files cover `classfeatures`,
`rally`, `opening`, `intercept` and updated `perform` usage. For a database-backed
help installation, apply `sql/20261118_class_improvements_help.sql` through the
normal deployment process. The migration is safe to reapply; it has not been
executed against a live database by this change.

## Validation

`python3 tests/run_class_improvements.py` exercises the production feature and
performance command code with native character structures under AddressSanitizer
and UndefinedBehaviorSanitizer, including native interception/critical routing,
shared precision-immunity decisions and the production cooldown save/load format. Combat-distance tests additionally cover stance
rank gates, ten-foot advances, movement-point costs and recovery. Existing shot
resolution and throwing tests cover shared hit-wrapper behavior and nested shot
cleanup. A full CMake build verifies integration.

These are initial balance values. Live encounter comparisons and class usage
measurements remain useful after deployment; no live server restart or gameplay
experiment is performed by the implementation.
