-- Install advanced race help using the standard race-help entry format.
-- The help system renders the tag, keywords, and last-updated header.
START TRANSACTION;
INSERT INTO help_entries(tag, category, entry, min_level, max_level, auto_generated)
VALUES
('race-irda', 'races', REPLACE('	g>Description:	n

Irda are graceful, magically gifted humanoids whose
keen minds and commanding presence suit a variety of
magical professions. Their innate ability to change
shape lets them travel among other peoples without
revealing their natural identity. An Irda may assume
another humanoid appearance, choosing its apparent
age, gender, and distinctive features. True Sight
reveals the Irda beneath the assumed form.

	g>Racial Abilities:	n

Change Shape:          - Assume any humanoid appearance at will; each change uses a standard action.
                       - No Disguise training or opposed Perception check is required.
                       - Use disguise form <race> to assume an appearance.
                       - Use disguise form remove to return to your natural form.
Custom Appearance:     - Choose age, gender, and up to two features with disguise form.
                       - Use disguise descriptors [feature] to see available feature choices.
Persistent Form:       - Your assumed form persists across reconnects until changed or removed.
True Sight:            - Reveals your natural form alongside your assumed appearance.
Natural Form:          - Return to natural form before advancing or using another transformation.
Size:                  - Medium.
Racial Language:       - Common.
Race Unlock:           - Advanced race; costs 5,000 account experience to unlock.

	g>Ability Adjustments:	n  +2 Dex, +2 Int, +2 Wis, +2 Cha

	g>Experience Modifier:	n  +2 level adjustment

	ySee also:	n RACES, DISGUISE, TRUEFORM, INTRODUCE
', CHAR(10), CONCAT(CHAR(13), CHAR(10))), 0, 1000, FALSE),
('race-sivak-draconian', 'races', REPLACE('	g>Description:	n

Sivaks are silver-scaled, winged draconians who
combine physical strength with an infiltrator''s
talent for borrowed identities. Their large size and
natural flight make them formidable combatants. By
personally slaying a humanoid enemy, a Sivak can
learn its appearance and later assume that form.
These stolen identities remain available for reuse,
even after the Sivak returns to its natural shape.

	g>Racial Abilities:	n

Sivak Scales:          - +3 natural armor.
Wings:                 - Natural flight; use fly.
Ultravision:           - Can see in darkness.
Disease Immunity:      - Immune to disease.
Spell Resistance:      - 10 + character level.
Draconian Bite:        - Make a bite attack with a swift action; use bite.
Draconic Devotion:     - +2 to attack rolls when grouped with another draconian or dragon.
Draconian Gallop:      - Additional movement points when advancing in level.
Improved Trip:         - Granted as a racial feat.
Stolen Forms:          - Learn appearances from humanoid NPCs or players you personally kill.
                       - Group members'' and pets'' kills, self-kills, and arena defeats grant no forms.
                       - Use disguise forms to list learned appearances.
                       - Use disguise form <number> to assume a learned appearance.
                       - Use disguise form remove to return to your natural form.
                       - Each change uses a standard action; no opposed Disguise check is required.
Persistent Forms:      - Learned appearances and the active form persist across sessions.
True Sight:            - Reveals your natural form alongside your assumed appearance.
Natural Form:          - Return to natural form before advancing or using another transformation.
Size:                  - Large.
Racial Language:       - Draconic.
Race Unlock:           - Advanced race; costs 8,000 account experience to unlock.

	g>Ability Adjustments:	n  +2 Str, +2 Dex, +2 Con, +2 Int

	g>Experience Modifier:	n  +2 level adjustment

	ySee also:	n RACES, DISGUISE, TRUEFORM, FLY, BITE, TRIP
', CHAR(10), CONCAT(CHAR(13), CHAR(10))), 0, 1000, FALSE),
('race-aurak-draconian', 'races', REPLACE('	g>Description:	n

Auraks are wingless, golden-scaled masters of innate
magic and the most powerful of the draconian races.
Their intelligence, presence, and arcane talents
make them dangerous spellcasters and manipulators.
Although they lack the natural flight of winged
draconians, Auraks wield force rays, noxious breath,
and an expanding array of racial spells. Even death
releases a final burst of their destructive magic.

	g>Racial Abilities:	n

Aurak Scales:          - +6 natural armor.
Ultravision:           - Can see in darkness.
Disease Immunity:      - Immune to disease.
Spell Resistance:      - 12 + character level.
Draconian Bite:        - Make a bite attack with a swift action; use bite.
Draconic Devotion:     - +2 to attack rolls when grouped with another draconian or dragon.
Combat Casting:        - Granted as a racial feat.
Improved Initiative:   - Granted as a racial feat.
Aurak Magic:           - Uses character level and Charisma; requires no caster class or prepared slots.
                       - Level 1: aurak energy ray, aurak breath, disguise self, magic missile.
                       - Level 3: charm, mirror image. Level 5: fireball, dispel magic.
                       - Level 7: greater invisibility. Level 9: dominate person.
                       - Use cast ''<spell name>'' [target]; normal casting actions apply except for energy ray.
Energy Ray:            - One ranged touch attack dealing 4d6 force damage; uses a swift action.
Noxious Breath:        - Affects eligible enemies in the room; Fortitude negates.
                       - Inflicts a 1d2 Strength penalty and blindness for 1d4 rounds.
                       - Poison immunity prevents the breath; blindness immunity prevents blindness.
                       - Recharges after 1d4 combat rounds.
Aurak Spell Points:    - Other at-will racial spells share 10 points; each costs its wizard circle.
                       - Full refill five minutes after first spending points. Use innates to check.
                       - 1 point: disguise self, magic missile, charm. 2 points: mirror image.
                       - 3 points: fireball, dispel magic. 4 points: greater invisibility.
                       - Energy ray and breath cost no points; breath keeps its own recharge.
Dominate Person:       - Once per rolling 24 real hours; does not spend spell points.
Persistent Recharge:   - Spell points, breath and domination recharge persist across sessions.
Disguise Self:         - +10 to Disguise; use disguise to change your visible appearance.
Death Throes:          - On death, a 3d6 force burst hits eligible enemies; Reflex halves damage.
                       - Equipment and resurrection follow normal rules.
Size:                  - Medium; cannot fly naturally.
Racial Language:       - Draconic.
Race Unlock:           - Advanced race; costs 15,000 account experience to unlock.

	g>Ability Adjustments:	n  +2 Dex, +4 Int, +4 Cha

	g>Experience Modifier:	n  +4 level adjustment

	ySee also:	n RACES, DISGUISE, DISGUISESELF, CAST, BITE
', CHAR(10), CONCAT(CHAR(13), CHAR(10))), 0, 1000, FALSE)
ON DUPLICATE KEY UPDATE category=VALUES(category), entry=VALUES(entry),
  min_level=VALUES(min_level), max_level=VALUES(max_level), auto_generated=VALUES(auto_generated);

INSERT IGNORE INTO help_keywords(help_tag, keyword) VALUES
('race-irda', 'irda'),
('race-irda', 'race-irda'),
('race-irda', 'irdaform'),
('race-sivak-draconian', 'sivak'),
('race-sivak-draconian', 'sivak-draconian'),
('race-sivak-draconian', 'sivakdraconian'),
('race-sivak-draconian', 'race-sivak-draconian'),
('race-sivak-draconian', 'sivakforms'),
('race-sivak-draconian', 'sivak draconian'),
('race-aurak-draconian', 'aurak'),
('race-aurak-draconian', 'aurak-draconian'),
('race-aurak-draconian', 'aurakdraconian'),
('race-aurak-draconian', 'race-aurak-draconian'),
('race-aurak-draconian', 'aurak draconian');
COMMIT;
