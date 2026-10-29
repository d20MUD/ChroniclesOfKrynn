# Spellhelp

`spellhelp <name>` displays school (or psionic discipline), classes and levels
gained, domains and circles gained, target types, augmentation, and effect
mechanics. Exact names take priority over unambiguous prefixes. Quotes are optional.

Labels are cyan and descriptions reset to normal. The requested `@c` / `@n`
colors use the engine's tab-prefixed color encoding (`\tc` / `\tn`) so they
render correctly through the existing protocol without changing global colors.

Regular output excludes casting internals, raw cost tables, memorization and
component settings, expiration messages, and audit boilerplate. Entries without
mechanics records simply state that detailed effects are not yet documented.
`spellhelp coverage` and the audit tool retain development coverage information.

The command is read-only and does not require knowing the spell. Internal
abilities, unused entries, skills, and player-disabled spells are excluded
unless explicitly documented as unavailable IDs.
Long descriptions and ambiguity lists use the normal pager.

## Architecture

- `src/spellhelp.c`: runtime metadata renderer, structured effect records, lookup,
  and coverage report. Each record separates description, damage amount/type,
  saves, duration, augmentation, restrictions, and developer source references.
- `src/spellhelp.h`: public eligibility and owned-string rendering interface.
- `src/spells.h`: thresholds and multipliers shared with gameplay.
- Metadata comes from final runtime registration, class minimum levels with
  `get_spell_circle()`, and domain assignment slots. Placeholder duration/save
  fields are not treated as authoritative.
- The renderer never invokes gameplay handlers, which can mutate casting state.

There are 586 core mechanics entries covering damage, repeated attacks, character
conditions and buffs, healing, condition removal, object alteration, creation,
summoning, groups, mass effects, room effects, and manual spells. These include
multi-handler behavior such as Heal's healing plus blindness removal, Bless's
character bonuses plus object blessing, and Remove Poison's character and object
cleansing. Group Heal intentionally describes healing only, matching its actual
dispatch. Defined ordinary spells through ID 540 and all 87 registered psionic powers now
have records. Retired/perk-only psionic IDs are also documented as unavailable.
Warlock invocation coverage and full cross-cutting interaction audits remain pending.

Numeric IDs 1–50 are covered in [the first batch audit](SPELLHELP_BATCH_001_050.md);
IDs 51–100 are covered in [the second batch audit](SPELLHELP_BATCH_051_100.md);
IDs 101–150 are covered in [the third batch audit](SPELLHELP_BATCH_101_150.md);
IDs 151–200 are covered in [the fourth batch audit](SPELLHELP_BATCH_151_200.md);
IDs 201–250 are covered in [the fifth batch audit](SPELLHELP_BATCH_201_250.md);
IDs 251–300 are covered in [the sixth batch audit](SPELLHELP_BATCH_251_300.md);
IDs 301–350 are covered in [the seventh batch audit](SPELLHELP_BATCH_301_350.md);
IDs 351–400 are covered in [the eighth batch audit](SPELLHELP_BATCH_351_400.md);
IDs 401–450 are covered in [the ninth batch audit](SPELLHELP_BATCH_401_450.md);
IDs 451–500 are covered in [the tenth batch audit](SPELLHELP_BATCH_451_500.md);
IDs 501–550 are covered in [the eleventh batch audit](SPELLHELP_BATCH_501_550.md).
IDs 325–381, 485, and 490 are unused slots. Ordinary spell definitions end at
540; IDs 541–550 contain no ordinary spells. The latest batch adds 39 records
and reviews the existing Solid Fog entry. Repulsion, campaign recalls, and
rapid-buff effects remain unavailable for normal casting.

The [complete psionic audit](SPELLHELP_PSIONICS.md) covers all 105 definitions
in IDs 1500–1604: 87 registered powers, 15 retired power IDs, and three perk-only
IDs. It adds 102 records and verifies the three existing power entries. The two
range boundary markers, 1499 and 1605, are not powers.
Retired/unfinished IDs and internal effects are marked unavailable in help;
they are not enabled for player casting. Plane Shift is unavailable in
Dragonlance/Forgotten Realms builds, and Gate describes its missing creation
handler in those campaigns.

Durations describe base combat rounds (affects update on `PULSE_VIOLENCE`) or
seconds where explicitly known. Feats, class features, and other duration or
potency modifiers can change the final result. No character-specific preview
is calculated.

## Extending coverage

1. Find a spell's registration, later setters, class/domain assignments, and all
   effect handlers. Follow shared helpers and indirect call paths as well.
2. Audit base effects, scaling/caps, saves and outcomes, resistance/immunities,
   durations, targets, augmentation thresholds, and special requirements.
3. Add a structured core record with source references. Describe **implemented**
   behavior; report gameplay bugs separately rather than documenting intended
   behavior as if it were implemented.
4. Prefer shared constants or side-effect-free calculation helpers when useful.
   Do not execute gameplay handlers to construct help.
5. Add behavior-focused checks and compare documentation with gameplay. Keep
   full interaction coverage pending until class, feat, perk, equipment, event,
   and configuration paths have been audited.

Use the source-reference index to start an audit:

```sh
python3 tools/audit_spellhelp.py --spell PSIONIC_DEADLY_FEAR
python3 tools/audit_spellhelp.py --json
python3 tools/audit_spellhelp.py --range 1 50 --json
python3 tools/audit_spellhelp.py --range 51 100 --json
python3 tools/audit_spellhelp.py --range 101 150 --json
python3 tools/audit_spellhelp.py --range 151 200 --json
python3 tools/audit_spellhelp.py --range 201 250 --json
python3 tools/audit_spellhelp.py --range 251 300 --json
python3 tools/audit_spellhelp.py --range 301 350 --json
python3 tools/audit_spellhelp.py --range 351 400 --json
python3 tools/audit_spellhelp.py --since HEAD~1 --json
```

`--since` flags entries referencing changed source files for review. This is
intentionally conservative. The index ignores commented-out registration calls and finds literal calls and
symbol references, including inactive preprocessor branches; it is not a C
parser and cannot prove completeness. Shared behavior without a direct spell-ID
reference still needs human review. The runtime tables determine public coverage.

## Validation and build

```sh
python3 tests/run_spellhelp.py --sanitize
python3 tests/run_epic_magic.py
python3 tests/run_undead_spell_routing.py
python3 tests/run_deadly_fear_saves.py
python3 tests/run_deadly_fear_augmentation.py
cmake -S . -B build-agent-check
cmake --build build-agent-check -j4
```

The command test compiles the actual implementation with controlled metadata
and checks exact/prefix/quoted names, ambiguous and invalid names, public-entry
filtering, table updates, class/domain data, mechanics records, coverage,
paging, and unchanged player/casting/resource state.

A help-file template is supplied at `docs/help/spellhelp.hlp`. World help files
are locally managed and ignored by Git in this checkout. To install the ordinary
`help spellhelp` topic, copy that template into the server's `lib/text/help/`
directory and add `spellhelp.hlp` before `$` in its index, then reload help using
the server's normal procedure. The command's own usage text requires no help-file
installation.

Both CMake and the Automake source manifest include the new module. Existing
Autotools build trees should regenerate their Makefiles using the project's
normal `autoreconf -fi` / configure process when updating source manifests.

Next milestones: expand audited mechanics in batches, finish interaction audits,
introduce optional character-specific
previews through side-effect-free helpers. These are not part of this first release.

## Live server configuration

Spellhelp reads the current cedit settings on every lookup. Base damage formulas
stay explicit; colored Global damage bonus lines show additive bonuses and the resulting
percentage for applicable casting classes. They describe player casts, not NPC
casts. Damage bonuses do not scale healing, status bonuses, DCs, or duration.
Zero global damage bonuses are omitted, together with their explanatory notes.
Labels occupy a fixed 20-character cyan column; normal descriptions start at
column 23 (counting from 1), with wrapped lines indented to match at 80 columns.
The class mapping follows `fight.c`, including classes that currently receive no
configured damage adjustment. Domain access adds the divine casting route.

Preparation percentages follow the separate switch in `spell_prep.c`: Summoner
uses arcane preparation; Blackguard and Inquisitor use divine preparation;
Alchemy uses its own percentage. Psionicists do not prepare powers, and the
psionic prep setting is currently unused. Preparation settings are not casting
or effect-duration multipliers.

Summoning entries show all three level bands from `mag_summons`, based on the
summoned creature's level. The final band applies at level 21 and above, even
though cedit labels it 21-30. Percentages scale maximum HP, AC, hitroll, damroll,
and both damage-dice count and size separately; later summon bonuses still apply.
Documented AC buffs, including Mage Armor and Epic Mage Armor, also show the
configured total AC cap.

Current implementation exceptions are documented rather than changed here:
repeated-hit spells do not restore the casting class between damage events,
so their cedit bonus is consumed by the first event; Acid Fog's periodic damage
uses the affected creature as source and does not preserve the original caster's
bonus. Level-up HP/movement and experience/death settings do not modify these
spell formulas and are not included as spell multipliers.
