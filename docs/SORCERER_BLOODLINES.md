# Sorcerer bloodlines

The implemented roster is Arcane, Draconic, Fey, Undead, Shadow, Stormborn,
Destined, Abyssal, Elemental and Celestial. Arcane chooses a spell school;
Draconic chooses a dragon heritage; Elemental chooses Air, Earth, Fire or Water.
Study previews benefits before confirmation. Dragon Disciples require Draconic.

The complete player rules and suboptions are in
[SORCERER_BLOODLINES_OVERVIEW.txt](SORCERER_BLOODLINES_OVERVIEW.txt), revised from
the user's supplied rules on 2026-11-17. Descriptions in `src/feats.c` also appear
in Study. `extended_bloodlines` in `src/class.c` is authoritative for the six
new bloodlines' powers, spells, bonus feats and class skills.

## Revised rules

- Bloodline use pools refresh every five real minutes from the first use.
  Repeated uses do not extend the timer. Conviction instead refreshes after
  30 seconds. Other classes' daily-use recovery remains unchanged.
- Arcane Metamagic Adept and Draconic attack pools use the shared event system.
  Fey and Undead retain their saved round counters, now 50 six-second rounds;
  old longer counters are capped to 50 on load.
- Shadow Arcana grants a competence Stealth bonus for 50 rounds (five minutes).
  Skill competence effects and equipment use their strongest positive bonus.
- Shadowstrike, Abyssal Claws, Elemental Ray and Heavenly Fire use swift actions.
  Abyssal Claws still requires both hands free. Shadowstrike cannot kill and
  remains ineffective against undead and constructs.
- Shadow Well grants 50% concealment to self or a grouped ally for half sorcerer
  level rounds. It requires darkness when activated and does not redirect
  combat. It uses native concealment counters and does not stack another 50%
  onto existing concealment. Hiding while observed in darkness is retained.
- Thunderstaff retains its full duration after level 9. Stormchild grants +4
  knockdown defense, weather sight at level 9 and a ten-percentage-point
  reduction in the target's concealment for its owner's attacks. Wind immunity
  remains. Storm Lord absorbs one damage packet per five minutes.
- Destined Arcana triggers on a personal-range sorcerer spell, granting saves
  for one minute, at most once per five minutes. Touch of Destiny lasts three
  rounds. It Was Meant to Be is a passive 1% proc on every native `d20` call,
  including natural 1 and 20; it keeps the second die without recursively
  proccing. Caps per five minutes are 1 at level 9, 2 at 10, and 3 at 20.
  Destiny Realized has a separate 5% chance to reroll failed spell-resistance
  checks, retaining fixed modifiers. Legacy reroll/penetration commands explain
  that these powers are passive. Critical-confirmation benefits are retained.
- Abyssal and Celestial summons receive fixed DR 5/good and DR 5/evil,
  respectively. Added Summonings grants evil-aligned summon creature targets
  +2 AC, hitroll and damage; it does not produce extra creatures. Summoning
  spell IDs are checked explicitly because they are not contiguous.
- Earth movement subtracts ten movement points on land and ignores difficult
  terrain, respecting normal minimum costs and legal exits. Water swimming
  does not grant water breathing or bypass boat-only sectors. Air grants
  flight and Fire increases ground speed. Ride the Lightning uses normal
  movement validation, including directional blockers and stealth checks.
- Heavenly Fire damages or heals for 1d12 + half level. Its shared recipient
  limit is once per five minutes; player-owned companions share their owner's
  receipt limit because companion events are not saved independently.
- `bloodline conviction [attack/save/skill/any]` arms the next matching roll
  within ten rounds. Other categories leave it armed. It keeps the second
  result, even if worse, and defaults to any eligible category.
- Stormborn's skill is Nature; Celestial's is Medicine. These use the existing
  Survival and Heal ID aliases, without renumbering skills.

## Engine integration and persistence

Feat, effect and event IDs are appended without renumbering existing IDs.
Spent uses and Heavenly Fire receipt limits use the existing player event
format. An appended event tracks Destined Arcana. Stored effects retain their
native round durations. Rebuilding and restarting the server is required.

Summon wards use native alignment-bypassed DR, which selects the best applicable
reduction. Reapplication replaces the old ward. Restored living conjured
companions regain the owner's fixed ward and applicable Added Summonings
bonuses. Purchased pets, undead and constructs are excluded from this restore
step. No pet database schema change is required.

Unavailable spells use the existing catalogue: protection from spells replaces
Abyssal's unholy aura; stoneskin, fire shield, transformation and spell mantle
replace Elemental body forms. Shadow Master uses magical-darkness sight,
illusion DC and shadow shield duration instead of shadow-conjuration realism.
Universal speech/comprehension adapts telepathy and tongues. No remote
telepathy or new petrification system is introduced. Lineage does not impose
alignment or deity restrictions.

## Deployment and validation

Apply these idempotent help migrations in order:

1. `sql/20261117_sorcerer_bloodlines_help.sql`
2. `sql/20261117_planar_bloodlines_help.sql`
3. `sql/20261117_revised_bloodline_rules_help.sql`

Matching authored help is in `lib/text/help`. These migrations have not been
applied to the live database. Native feat information and Study descriptions
use the revised definitions.

```sh
python3 tests/run_sorcerer_bloodlines.py
python3 tests/run_bloodline_cooldowns.py
python3 tests/run_sorcerer_casting.py
python3 tests/run_sorcerer_perks.py
python3 tests/run_natural_swimming.py
python3 tests/run_blocker_stealth.py
python3 tests/run_clan_guard.py
python3 tests/run_feat_info_export.py
```

The bloodline tests compile production functions with address and undefined
behavior sanitizers. They cover progression, Study previews and rejection,
spell grants, action use, targeting, native concealment, resistances, summon
wards, rerolls, category selection, full cooldown pools and persisted events.
A complete server build verifies integration.
