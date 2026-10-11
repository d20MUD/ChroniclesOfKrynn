-- Install the manual epic magic help entry; apply during deployment.
INSERT INTO help_entries(tag, category, entry, min_level, max_level, auto_generated)
VALUES ('EPICSPELLS', 'general', REPLACE('EPIC SPELLS

Epic magic uses two pools. Combat casts fuel damaging epic spells and
powers. Preparation casts fuel Epic Mage Armor, Epic Warding, Epic Psionic
Ward, Mummy Dust, Summon Solar, and Dragon Knight.

Your combat capacity is Spellcraft / 5, capped at 10. Preparation capacity
uses the same calculation, capped at 3. Each pool independently recovers
one cast every 90 seconds while your character is being updated online.
Remaining casts and recovery progress are saved. Use cooldowns to check.
Psionic powers also retain their normal PSP and augmentation costs.

Epic damaging spells and powers gain +4 effective save DC and +10 spell
or power penetration. A successful damage save halves epic damage rather
than negating it. Immunities and damage resistance still apply.

With the corresponding metamagic feat, epic spells allow one enhancement:
  cast empowered ''greater ruin'' <target>
  cast quickened ''epic warding'' <target>
Either costs two casts from the relevant pool. Empower is only for damage.
Empower and Quicken cannot combine; other metamagic is unavailable for
epic spells. Pending free metamagic perks remain for ordinary spells.

Greater Ruin deals force damage. A surviving damaged target loses up to
10 damage reduction for three rounds. Recasting refreshes the weakness.

Hellball retains its initial energy explosion. Surviving damaged targets
also take fire damage equal to twice caster level on each of the next two
rounds. This lingering fire does not stack. It ends if the caster or target
leaves, disappears, or enters a peaceful room, or combat becomes invalid.

Epic Mage Armor grants a 20 armor bonus and +3 dodge bonus to AC. Normal
bonus stacking and AC caps apply; it does not grant a Dexterity increase.

Epic Warding holds 60 times caster level in absorption, normally absorbing
up to 75 damage per hit. Its initial three-round surge raises that limit
to 105 and prevents concentration failure and knockdowns while the ward
is intact. Absorbed damage and remaining strength are shown in combat.

Epic summons scale from level 30 to 35 with caster level. Only one epic
summon may be controlled at a time. Each gains a special action every
three combat rounds while fighting beside its master:
  Solar: heals itself or its master, choosing the lower health percentage,
         for up to five times its level in hit points.
  Mummy: attempts a touch against a living enemy, imposing -4 attack and
         -4 AC for three rounds. This weakness does not stack.
  Dragon: breathes fire at engaged enemies for four times its level in
          damage, with a Reflex save for half. Allies are excluded.

Invalid summon limits, missing summon prototypes, absent recipients and
area spells without valid enemies are rejected before a cast is spent.
An enemy resisting a valid spell can still consume the cast.

See also: COOLDOWNS, SPELLCRAFT, METAMAGIC
', CHAR(10), CONCAT(CHAR(13), CHAR(10))), 0, 1000, FALSE)
ON DUPLICATE KEY UPDATE
  category = VALUES(category), entry = VALUES(entry),
  min_level = VALUES(min_level), max_level = VALUES(max_level), auto_generated = FALSE;
INSERT IGNORE INTO help_keywords(help_tag, keyword) VALUES
('EPICSPELLS', 'epicspells'), ('EPICSPELLS', 'epicmagic');
