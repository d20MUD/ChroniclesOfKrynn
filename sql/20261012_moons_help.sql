-- Install the manual moons help entry during deployment; safe to reapply.
INSERT INTO help_entries(tag, category, entry, min_level, max_level, auto_generated)
VALUES ('MOONS', 'general', REPLACE('MOONS OF MAGIC

The three moons of Krynn influence eligible arcane spellcasters. Your
alignment determines which moon governs your magic:

  Good:     Solinari, the white moon (36-day cycle)
  Neutral:  Lunitari, the red moon   (28-day cycle)
  Evil:     Nuitari, the black moon  ( 8-day cycle)

Each moon advances one position per game day. Its phase provides the
following modifiers to followers of that moon:

  Moon phase       Spell save DC   Bonus spells   Effective caster level
  Low Sanction          -1              0                  -1
  Waning                 0              0                   0
  High Sanction         +1             +2                  +1
  Waxing                 0             +1                   0

A higher spell save DC makes your spells harder for targets to resist.
Effective caster level improves level-dependent effects such as spell
damage and duration. It does not grant character levels, new spell
circles, or spells known. These modifiers do not increase your own
saving throws.

CONJUNCTIONS

When moons occupy overlapping positions along the same line from Krynn,
they are in conjunction. Simply sharing the same phase is not enough.
Conjunctions add these bonuses to the ordinary phase modifiers:

  Moons aligned         Spell save DC   Bonus spells   Caster level
  Solinari + Lunitari         +1             +1             +1
  Lunitari + Nuitari          +1             +1             +1
  Solinari + Nuitari          +1              0              0
  All three                  +2             +2             +1

Pair bonuses apply only to followers of the participating moons. A true
three-moon conjunction replaces the pair bonuses; it does not add all
three pairs together. Two separate pair conjunctions do not count as a
true three-moon conjunction, and overlapping pair bonuses do not stack.

NIGHT OF THE EYE

When all three moons align at High Sanction, the Night of the Eye brings
magic to its peak. Phase and conjunction bonuses combine to give:

  +3 spell save DC, +4 bonus spells, and +2 effective caster levels.

BONUS SPELLS

Your moon bonus spells form one shared pool for your character, not a
separate pool for each spell circle. When available, an eligible arcane
cast automatically spends a moon bonus spell instead of consuming the
normal prepared spell or spell slot. You must already know or have
prepared the spell; normal casting requirements still apply.

Used moon bonus spells recover automatically over time while you are
online. The maximum changes with phases and conjunctions. A larger
maximum does not erase spent uses. When the maximum falls, spent uses
are capped to the new maximum.

KNIGHTS OF THE THORN

A PC with at least one Knight of the Thorn level is unaffected by moon
phases and conjunctions, even when multiclassed. Such characters receive
neither lunar bonuses nor lunar penalties, including bonus spells.

CHECKING THE MOONS

  score       Shows your combined lunar modifiers and conjunctions.
  look moons  Describes the moons and current conjunctions. Go outside
              at night for a clear view. Nuitari is normally visible
              only to evil characters.

Moon bonus spells and conjunctions require the server''s arcane moon
phases system to be enabled. These are the rules implemented here;
tabletop rules vary by edition.

See also: SCORE, LOOK, SPELLS, PREPARATION, KNIGHTOFTHETHORN
', CHAR(10), CONCAT(CHAR(13), CHAR(10))), 0, 1000, FALSE)
ON DUPLICATE KEY UPDATE
  category = VALUES(category), entry = VALUES(entry),
  min_level = VALUES(min_level), max_level = VALUES(max_level), auto_generated = FALSE;
INSERT IGNORE INTO help_keywords(help_tag, keyword) VALUES
('MOONS', 'moons'),
('MOONS', 'moon'),
('MOONS', 'moons of magic'),
('MOONS', 'moonmagic'),
('MOONS', 'moon magic'),
('MOONS', 'moon phases'),
('MOONS', 'lunar phases'),
('MOONS', 'conjunction'),
('MOONS', 'conjunctions'),
('MOONS', 'nightoftheeye'),
('MOONS', 'night of the eye'),
('MOONS', 'solinari'),
('MOONS', 'lunitari'),
('MOONS', 'nuitari');
