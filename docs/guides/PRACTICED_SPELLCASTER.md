# Practiced Spellcaster

Practiced Spellcaster is a learnable spellcasting feat with a class choice.
`study` allows each eligible spellcasting class once. Choices cost normal
feat points, remain pending until study is saved, and persist in player
files as repeated `PScC: <class ID>` records. Existing files default to no
choices. Respec clears both the feat and its class selections.

The bonus is up to four caster levels, bounded by total character level
(Hit Dice). Existing prestige spellcasting advancement also counts when
determining the remaining room below Hit Dice. The feat's bonus grows
automatically as that room grows. Existing caster-level calculations and
global limits are retained; the bonus applies only to the selected casting
class. NPCs already use their Hit Dice as caster level and gain no bonus.

The bonus is separate from `BONUS_CASTER_LEVEL`, which controls spell
progression. It never grants circles, slots, known spells, class levels,
or psionic manifester levels. Normal and instant spell resolution use the
bonus, as do caster-level effect and penetration checks. Epic casting now
selects an actual casting class when it skips spell preparation, allowing
the class-specific feat to apply and avoiding an undefined class index.

Item/consumable resolution suppresses this personal caster-level bonus.
Pending study choices do not improve casting before they are committed.
Equipment versions use the object's `specific` field for the class ID,
require levels in that class, and do not stack with learned or other item
copies. Random treasure assigns a valid spellcasting class.

The Loremaster Applicable Knowledge picker excludes feats requiring
subtype selection, including this feat, since that path does not provide
a class-choice submenu.

Run `python3 tests/run_practiced_spellcaster.py` to exercise production
study spending, duplicate/cancel behavior, class isolation, HD and prestige
caps, equipment checks, save/load records, and consumable context under
AddressSanitizer and UBSan. Build with `make` from the repository root.

Legacy help: `lib/text/help/practiced_spellcaster.hlp`. Database help:
`sql/20261011_practiced_spellcaster_help.sql`, supplied for deployment.
