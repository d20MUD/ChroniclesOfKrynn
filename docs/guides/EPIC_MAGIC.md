# Epic magic

Epic magic has a combat pool capped at 10 and a preparation pool capped at
3. Both use the existing Spellcraft / 5 capacity calculation and regenerate
one cast independently every 15 six-second character updates. Counts and
partial regeneration progress persist in player files. Existing characters
start with a full preparation pool when the new fields are absent.

Preparation spells are Epic Mage Armor, Epic Warding, Epic Psionic Ward,
Mummy Dust, Summon Solar, and Dragon Knight. Other epic spells/powers use
combat casts. Epic powers retain PSP costs and augmentation rules.

Damaging epic magic has +4 effective save DC, +10 spell/power penetration,
and half damage on a successful save. Existing immunities remain. The
existing 150% epic damage multiplier is retained and now consistently
includes the four epic psionic powers.

Empower or Quicken costs one extra cast. They cannot combine, and Empower
requires a damaging spell. Other metamagic and free metamagic perks do not
apply to epic spells; the normal feats are still required by command parsing.
Instant epic casts use the actual caster/manifester level instead of zero.

## Spell effects

| Spell | Effect |
| --- | --- |
| Greater Ruin | Force damage; surviving damaged targets have each applicable physical DR layer reduced by up to 10 for three affect rounds. Refreshes without stacking. |
| Hellball | Initial energy explosion plus two timed fire pulses, each twice caster level. No overlapping lingering fire on the same target. |
| Epic Mage Armor | Existing +20 armor bonus plus +3 dodge AC, with normal stacking and caps. |
| Epic Warding | Existing reserve of caster level × 60 and absorption limit of 75 per hit. For three affect rounds, an intact ward absorbs up to 105 per hit and prevents concentration failure and knockdowns. |
| Epic summons | Level `min(35, max(30, caster_level + 5))`, rerolled statistics, and caster_level / 3 additional hitroll and damroll. Existing summon configuration and augmentation bonuses still apply. |

Solar, mummy, and dragon special actions occur once every three combat
rounds, supplementing their existing behavior. The solar heals the lower
health percentage of itself/master by up to summon level × 5. The mummy
needs a successful melee touch against a non-undead enemy and applies
nonstacking -4 attack and -4 AC for three affect rounds. The dragon breathes
at engaged enemies for summon level × 4 fire damage, Reflex half, excluding
its master and allies. Special actions require the master in the same room.

Lingering fire uses unique target IDs, rather than tracking the caster's
current opponent. It stops on departure, extraction, death, peaceful rooms,
or loss of valid PVP/mission permissions. It is not restored after logout or
restart. Summon action events likewise are runtime events.

## Spending and persistence

Casting validates metamagic cost, pool capacity, summon prototypes/limits,
protection recipients, and area targets before committing resources. These
checks run at casting initiation and completion. At resolution, casts are
spent after trigger, room, protection, and spell-mantle checks, before spell
turning, so the original caster pays for a reflected spell. Valid attacks
that meet resistance or immunity still spend a cast.

Player tags `EpCs` and `EpRg` remain the combat pool and progress. New tags
`EpPc` and `EpPr` store preparation casts and progress. A full pool omits its
count tag and reloads using the existing uninitialized/full convention.
`cooldowns` shows both pools. No database schema change is required.

The legacy help artifact is `lib/text/help/epicspells.hlp`. For installations
using database help, `sql/20261011_epic_magic_help.sql` installs the matching
manual entry. The SQL is supplied for deployment and is not applied by tests.

Validation: `python3 tests/run_epic_magic.py`, followed by the normal server
build. The regression harness compiles production pool, persistence, save,
protection, event, and summon code with AddressSanitizer and UBSan.
