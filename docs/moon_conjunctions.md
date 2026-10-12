# Lunar conjunctions

Conjunctions add to the existing phase modifiers for the characters already
eligible for moon magic. A PC with any Knight of the Thorn levels stays exempt.
The feature uses the existing arcane moon phases setting.

| Conjunction | Spell DC | Bonus spells | Effective caster level |
| --- | ---: | ---: | ---: |
| Solinari and Lunitari | +1 | +1 | +1 |
| Lunitari and Nuitari | +1 | +1 | +1 |
| Solinari and Nuitari | +1 | 0 | 0 |
| All three | +2 | +2 | +1 |

Only the participating moons benefit. Triple bonuses replace pair bonuses;
overlapping pairs never award the same bonus twice. Combined with High Sanction,
the Night of the Eye gives +3 spell DC, four bonus spells, and +2 effective caster
levels. The existing shared, regenerating bonus-spell pool remains in use.

`score` shows combined modifiers and conjunctions; `look moons` shows conjunctions
when the normal viewing requirements are met. Night of the Eye is named only for
a genuine triple conjunction at High Sanction.

## Sources and adaptation

The [Dragonlance Nexus chart](https://dragonlancenexus.com/moon-tracking-chart/)
and its [PDF](https://dragonlancenexus.com/wp-content/uploads/Moon-Tracking-Chart.pdf)
use Solinari's 36-day, Lunitari's 28-day, and Nuitari's 8-day tracks. Each day
advances one box. Boxes sharing a radial arc form a conjunction; sharing a broad
sanction alone does not. The implementation scales the boxes to 504 angular
units and tests actual overlap. A triple needs a common overlap for all three,
rather than two separate pair conjunctions.

The table follows the classic alignment table in
[Dragonlance Adventures, AD&D 1e](https://1eonline.info/12dla/wohs.htm).
The phase/alignment model also appears in
[DLE2 Dragon Magic, AD&D 2e](https://anyflip.com/cmjgl/kscm/basic/).
For this d20 game, the saving-throw benefit is expressed as a bonus to the caster's
spell DC. This is a game adaptation; the older save column is not a 3e DC rule.

The [2003 Dragonlance Campaign Setting, 3.5e](https://pdfcoffee.com/dragonlance-campaign-setting-6-pdf-free.html)
uses a different conjunction table: any pair grants +1 caster level and spell DC,
and a triple grants +2 to both. It supplies no matching AD&D bonus-spell column.
We retain the classic table to match this game's existing phase/bonus-spell model.
This does not add edition-specific minimum-level or Intelligence requirements,
or restrict the game's existing eligible arcane classes to robed wizards.

The phase thresholds, startup positions, and sky descriptions now consistently
use the supplied chart's cycle lengths. Some older AD&D charts used reversed
Lunitari/Nuitari periods. The runtime previously mixed those old thresholds with
the newer daily cycles, which prevented sensible phase/conjunction tracking.
The lunar save modifier previously strengthened the defender's roll; it now
modifies spell DC, as displayed.

Phase recalculation resets ordinary bonuses before adding conjunctions, so
repeated recalculations cannot accumulate bonuses. Changing conjunctions updates
online bonus-spell pools while preserving spent slots and capping them to the
new maximum.
