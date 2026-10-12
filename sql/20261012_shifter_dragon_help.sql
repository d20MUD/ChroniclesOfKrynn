-- Install during deployment. Preserve the existing DRAGON lore entry.
INSERT INTO help_entries(tag, category, entry, min_level, max_level, auto_generated)
VALUES ('SHIFTERDRAGON', 'general', REPLACE('SHIFTER DRAGON SHAPE

This entry describes the dragon forms available through Shifter Shapes V.
For the history and nature of dragons, see HELP DRAGON.

ACCESS AND COMMANDS

Shifter level 10 grants Shifter Shapes V, unlocking these wild shapes:

  wildshape red dragon        wildshape blue dragon
  wildshape green dragon      wildshape black dragon
  wildshape white dragon

Use wildshape with no argument to list available forms. Use wildshape
return to resume your normal form. The shifter''s Limitless Shapes ability
removes the usual wild-shape use limit; normal action costs still apply.

Dragon forms are Huge. Their base form modifiers are +8 Strength,
+8 Constitution, and +20 natural armor. Other feats, perks, and effects
can change the final values; use stats to inspect your current form.
Natural-attack progression scales with your combined Druid and Shifter
levels. These forms fight with their natural attacks rather than granting
a dragon''s entire monster stat block.

COMMON DRAGON ABILITIES

All five forms grant True Sight, Paralysis Immunity, Ultravision,
Blindsense, Wings, and Dragon Magic. Use fly to take flight and land to
come down. Wings do not mean you are automatically flying.

Each color also grants a retaliatory elemental shield. Black and green
forms grant water breathing. Do not confuse an elemental shield with
blanket immunity to that damage type.

  Color   Breath damage    Shield        Other granted feats
  Red     Fire             Fire          Iron Will, Power Attack,
                                         Cleave, Improved Initiative
  Blue    Electricity      Electricity   Combat Casting,
                                         Improved Initiative
  Green   Poison           Acid          Alertness, Cleave, Great Cleave,
                                         Iron Will, Power Attack,
                                         Trackless Step, Woodland Stride
  Black   Acid             Acid          Alertness, Improved Initiative,
                                         Power Attack
  White   Cold             Cold          Alertness, Improved Initiative,
                                         Power Attack

BREATH WEAPON

Use breathe while in dragon form. It is a room-area breath attack using
the damage type listed above and normal area-effect targeting rules.
Damage scales with your Druid and Shifter levels. Breathe costs a standard
action and has a 12-second recovery period. It cannot be used in a
peaceful room. Resistances and saving throws can affect its damage.

DRAGON MAGIC

Use dragonmagic <target> <effect>. For example:

  dragonmagic self detect-magic
  dragonmagic self shield
  dragonmagic <enemy-name> lightning-bolt  (blue form)

The target must be in the room. Dragon magic has its own limited-use
pool and reports remaining uses. Its effects scale with Druid and Shifter
levels. Invoke dragonmagic without arguments while in the form to see
its syntax and available effects. Peaceful-room restrictions apply.

All colors offer these effects:
  detect-magic, continual-flame, dispel-magic, invisibility,
  see-invisibility, shield, true-strike, endure-elements

Additional effects depend on your current color:
  Red:    magic-missile, haste
  Blue:   mage-armor, lightning-bolt
  Green:  mage-armor, mirror-image
  Black:  stinking-cloud, slow
  White:  chill-touch, ice-storm

These innate abilities are separate from ordinary spellcasting. The
Natural Spell feat is still needed to cast your regular spells while
wild shaped. Use feats known descriptions and affects to inspect granted
abilities and active shields.

Returning to normal form removes the form''s granted abilities and
shields. This is a Shifter form, separate from Dragon Disciple''s Dragon
Form ability, dragon companions, and dragon lore.

See also: SHIFTER, WILDSHAPE, NATURAL SPELL, BREATHE, DRAGONMAGIC, DRAGON
', CHAR(10), CONCAT(CHAR(13), CHAR(10))), 0, 1000, FALSE)
ON DUPLICATE KEY UPDATE
  category = VALUES(category), entry = VALUES(entry),
  min_level = VALUES(min_level), max_level = VALUES(max_level), auto_generated = FALSE;
INSERT IGNORE INTO help_keywords(help_tag, keyword) VALUES
('SHIFTERDRAGON', 'shifterdragon'),
('SHIFTERDRAGON', 'shifter dragon'),
('SHIFTERDRAGON', 'dragonshape'),
('SHIFTERDRAGON', 'dragon shape'),
('SHIFTERDRAGON', 'shifter dragon shape'),
('SHIFTERDRAGON', 'shifterdragonshape'),
('SHIFTERDRAGON', 'shifter shapes v');

-- Append a pointer once, keeping the lore text and its original keywords.
UPDATE help_entries
SET entry = CONCAT(entry, CHAR(13), CHAR(10), CHAR(13), CHAR(10),
  'For the Shifter dragon shape and its abilities, see help shifterdragon.',
  CHAR(13), CHAR(10))
WHERE tag IN ('DRAGON', 'DRAGONS')
  AND LOCATE('help shifterdragon', entry) = 0;
