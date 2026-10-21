# New Faces of Krynn: Introductions, Disguises, and Three Advanced Races

*Chronicles of Krynn development update — October 21, 2026*

Every stranger has a story—and some have more than one face. This update brings
remembered introductions, disguises, clearer True Sight, and three new advanced
race choices: **Irda, Sivak Draconian, and Aurak Draconian**.

## A name worth learning

Until another player introduces themselves to you, you see their short
description instead of their name. Introductions work in each direction:
introducing yourself teaches them your name, but you still need their
introduction to learn theirs. NPCs and immortals retain their exemptions.

```text
introduce <person>
introduce list
introduce forget <name>
```

You can target strangers using words from their visible descriptions, such as
`human`. Learned introductions are remembered across sessions.

## Choose the face you show the world

The Disguise skill lets you change your apparent race, age, gender, and up to two
descriptive features. Feature choices come from the same menus used for your
character's normal short description.

```text
disguise descriptors
disguise descriptors hair
disguise human | adult | female | hair:43 | eyes:2
disguise remove
```

Your Disguise check is rolled when you apply the disguise. Each observer gets
**one opposing Perception check when they first see that disguise**. Looking
again or reconnecting does not grant another attempt. An ordinary disguise can
represent a humanoid race of your own size.

A convincing disguise hides your known identity. Someone who sees through it
sees your known name—or your real description if you are still a stranger—with
a separate line describing your disguise. Detecting a disguise never teaches
someone your name. Return to your natural appearance before introducing yourself.

### Two ways to support a disguise with magic

**Disguise Self** grants **+10 to the Disguise skill for 10 minutes per caster
level**. Cast it before applying your disguise to benefit from the bonus.

**Alter Self** changes your active race to **any player-selectable race** for
one minute per caster level. Small forms grant **+2 Dexterity**; Medium or larger
forms grant **+2 Strength**. Both bonuses use the racial bonus type. Perception
cannot expose the racial transformation, although other disguise details can
still be detected.

```text
cast 'disguise self'
cast 'alter self' <race>
disguise dismiss disguise self
disguise dismiss alter self
```

## Three new advanced races

| Race | Ability bonuses | Size | Account XP unlock | Level adjustment |
| --- | --- | --- | --- | --- |
| **Irda** | +2 Dexterity, Intelligence, Wisdom, and Charisma | Medium | 5,000 | +2 |
| **Sivak Draconian** | +2 Strength, Dexterity, Constitution, and Intelligence | Large | 5,000 | +2 |
| **Aurak Draconian** | +2 Dexterity, +4 Intelligence, +4 Charisma | Medium | 15,000 | +4 |

### Irda: a form of your own choosing

Irda can assume **any humanoid race at will**, including a different size. They
can choose their apparent age, gender, and descriptive features using the shared
descriptor menus.

```text
disguise form human | old-aged | female | hair:43 | eyes:2
disguise form remove
```

This innate transformation requires **no Disguise training or opposed Perception
check**. Changing form takes a standard action. Your chosen appearance persists
across sessions until you change it or return to your natural form.

### Sivak Draconians: the faces of fallen enemies

Sivaks learn appearances from **humanoid enemies they personally kill**, whether
NPCs or players. Their learned forms reproduce the victim's appearance, visible
gender, and size. All Sivaks can use this ability.

```text
disguise forms
disguise form <number>
disguise form remove
```

Learned forms are reusable. **Both your collection and currently assumed form
are saved in your player file**, surviving logout and server restarts. Returning
to your natural form does not erase your collection.

Changing form takes a standard action and requires no Disguise or Perception
roll. Copied appearances grant no equipment, skills, spells, or knowledge of the
victim's name. Arena defeats, non-humanoids, and another character's kills do not
grant forms.

Sivaks also gain **flight, ultravision, disease immunity, draconian spell
resistance, a bite, draconic devotion, gallop, and Improved Trip**.

### Aurak Draconians: masters of innate magic

Auraks are the most powerful advanced draconian choice. These wingless
spellcasters gain **+6 natural armor**, **spell resistance of 12 + character
level**, ultravision, disease immunity, a bite, draconic devotion, Combat Casting,
and Improved Initiative.

Their racial magic becomes available as they advance, without requiring a
caster class or prepared spell slots:

| Character level | New racial spells |
| --- | --- |
| 1 | Aurak energy ray, Aurak breath, Disguise Self, Magic Missile |
| 3 | Charm, Mirror Image |
| 5 | Fireball, Dispel Magic |
| 7 | Greater Invisibility |
| 9 | Dominate Person |

```text
cast 'aurak energy ray' <enemy>
cast 'aurak breath'
cast 'greater invisibility'
cast 'dominate person' <enemy>
```

The **energy ray** makes a ranged touch attack for **4d6 force damage**.
**Noxious breath** can briefly weaken and blind eligible enemies in the room,
with a Fortitude save to resist and a recharge of **1d4 combat rounds**.

Racial **Dominate Person** is usable once per rolling **24 real hours**. Other
racial spells are at will, subject to normal casting rules. Recharge times
persist across sessions; ordinary class spell slots remain separate from the
racial domination use.

On death, an Aurak releases a **3d6 force burst** against eligible enemies in the
room, with Reflex saves for half damage. Auraks have no natural flight.

## True Sight reveals the whole picture

True Sight reveals a character's **original form** through disguises, Alter
Self, Irda and Sivak transformations, wildshape, and other shapechanges. It also
shows their **altered appearance on a separate line**, preserving the context
of what everyone else sees. A mundane disguise is described separately too.

For a character whose name you already know, this might look like:

> Alice's true form is an adult female irda with green eyes.
>
> Alice's altered form is an old-aged female human with short silver hair.

True Sight does not teach an unfamiliar person's name or remove their
transformation. Their actual transformed size, race effects, and bonuses remain
in place.
